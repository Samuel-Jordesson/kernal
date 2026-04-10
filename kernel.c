#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80

// VGA DAC Ports
#define PALETTE_ADDR 0x3C8
#define PALETTE_DATA 0x3C9

// Indices VGA de Cores Dracula
#define DR_BACK      0x00 // Cinza Escuro (Base)
#define DR_WHITE     0x0F // Creme (Texto)
#define DR_RED       0x01 // Vermelho (Note: mudei o indice para evitar conflito com 0x0C antigo se quiser, mas manterei os indices 0-15)
#define DR_GREEN     0x02 
#define DR_YELLOW    0x03
#define DR_PURPLE    0x04
#define DR_CYAN      0x05
#define DR_COMMENT   0x06

#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

// Sistema de Arquivos (VFS Hierarquico)
#define MAX_FILES 64
#define MAX_FILE_SIZE 1024
#define TYPE_FILE 0
#define TYPE_DIR 1

typedef struct {
    char name[32];
    char content[MAX_FILE_SIZE];
    int size;
    int type;
    int is_active;
    int parent_idx;
} File;

File fs[MAX_FILES];
int file_count = 0;
int ctrl_pressed = 0;
int current_dir_idx = -1;
char current_path[64] = "/";

// Protótipos
void clear_screen();
void kprint(char *message);
void kprint_color(char *message, char color);
void kprint_char(char c, char color);
void kprint_backspace();
void update_cursor(int x, int y);
void outb(unsigned short port, unsigned char data);
unsigned char inb(unsigned short port);
void set_vga_color(int idx, int r, int g, int b);
void init_dracula_palette();

int strcmp(char *s1, char *s2);
int strlen(char *s);
void strncpy(char *dest, char *src, int n);
void strlcat(char *dest, char *src, int n);
int str_starts_with(char *str, char *prefix);
int atoi(char *s);
void itoa(int n, char *s);
void reverse(char *s);
unsigned char get_scancode();
char scancode_to_char(unsigned char scancode);
void execute_command(char *input);
void open_editor(int file_idx);
void print_prompt();

int cursor_x = 0;
int cursor_y = 0;
int prompt_len = 0;

void kernel_main() {
    init_dracula_palette(); // <--- O MÁGICO ACONTECE AQUI
    clear_screen();
    
    kprint_color("--- Kernel Terminal v2.5 (True Dracula) ---\n", DR_GREEN);
    kprint_color("Hardware calibrado para cores customizadas.\n\n", DR_WHITE);
    
    for(int i = 0; i < MAX_FILES; i++) fs[i].is_active = 0;

    char key_buffer[256];
    int buffer_idx = 0;

    print_prompt();

    while (1) {
        unsigned char scancode = get_scancode();
        if (scancode == 0) continue;
        if (scancode == 0x1D) { ctrl_pressed = 1; continue; }
        if (scancode == 0x9D) { ctrl_pressed = 0; continue; }
        if (scancode & 0x80) continue; 

        char c = scancode_to_char(scancode);
        if (scancode == 0x1C) { // Enter
            kprint("\n");
            key_buffer[buffer_idx] = '\0';
            execute_command(key_buffer);
            buffer_idx = 0;
            print_prompt();
        } 
        else if (scancode == 0x0E) { // Backspace
            if (buffer_idx > 0) { buffer_idx--; kprint_backspace(); }
        } 
        else if (c != 0) {
            if (buffer_idx < 255) { key_buffer[buffer_idx++] = c; kprint_char(c, DR_WHITE); }
        }
    }
}

void init_dracula_palette() {
    // Cores originais Dracula em 6-bits (0-63)
    set_vga_color(DR_BACK,    10, 10, 13); // #282A36 (Background)
    set_vga_color(DR_WHITE,   61, 61, 61); // #F8F8F2 (Text)
    set_vga_color(DR_RED,     63, 21, 21); // #FF5555 (Red)
    set_vga_color(DR_GREEN,   19, 61, 30); // #50FA7B (Green)
    set_vga_color(DR_YELLOW,  59, 61, 34); // #F1FA8C (Yellow)
    set_vga_color(DR_PURPLE,  46, 36, 61); // #BD93F9 (Purple)
    set_vga_color(DR_CYAN,    34, 57, 62); // #8BE9FD (Cyan)
    set_vga_color(DR_COMMENT, 24, 28, 40); // #6272A4 (Comment)
}

void set_vga_color(int idx, int r, int g, int b) {
    outb(PALETTE_ADDR, idx);
    outb(PALETTE_DATA, r);
    outb(PALETTE_DATA, g);
    outb(PALETTE_DATA, b);
}

void print_prompt() {
    kprint_color("admin@meu-os", DR_GREEN);
    kprint_char(':', DR_WHITE);
    kprint_color(current_path, DR_PURPLE);
    kprint_color("$ ", DR_WHITE);
    prompt_len = cursor_x;
}

void execute_command(char *input) {
    if (strlen(input) == 0) return;
    if (strcmp(input, "clear") == 0) clear_screen();
    else if (strcmp(input, "help") == 0) {
        kprint_color("Kernel 2.5 Dracula Commands:\n", DR_YELLOW);
        kprint("  ls, mkdir, aqdir, cd, apg, rn, calc, ler, cpuid, time, clear\n");
    } 
    else if (strcmp(input, "ls") == 0) {
        int cnt = 0;
        for (int i = 0; i < MAX_FILES; i++) {
            if (fs[i].is_active && fs[i].parent_idx == current_dir_idx) {
                if (fs[i].type == TYPE_DIR) kprint_color(fs[i].name, DR_PURPLE);
                else kprint_color(fs[i].name, DR_WHITE);
                kprint("\n"); cnt++;
            }
        }
        if (cnt == 0) kprint_color("(vazio)\n", DR_COMMENT);
    }
    else if (str_starts_with(input, "mkdir ")) {
        char *name = input + 6; if (file_count < MAX_FILES) {
            strncpy(fs[file_count].name, name, 31); fs[file_count].type = TYPE_DIR;
            fs[file_count].parent_idx = current_dir_idx; fs[file_count].is_active = 1;
            file_count++; kprint("Pasta criada.\n");
        }
    }
    else if (str_starts_with(input, "aqdir ")) {
        char *name = input + 6; if (file_count < MAX_FILES) {
            strncpy(fs[file_count].name, name, 31); fs[file_count].type = TYPE_FILE;
            fs[file_count].parent_idx = current_dir_idx; fs[file_count].is_active = 1;
            fs[file_count].content[0] = '\0'; file_count++; kprint("Arquivo criado.\n");
        }
    }
    else if (str_starts_with(input, "cd ")) {
        char *name = input + 3;
        if (strcmp(name, "..") == 0) {
            if (current_dir_idx != -1) {
                current_dir_idx = fs[current_dir_idx].parent_idx;
                if (current_dir_idx == -1) { current_path[0] = '/'; current_path[1] = '\0'; }
                else { current_path[0] = '/'; current_path[1] = '\0'; strlcat(current_path, fs[current_dir_idx].name, 63); }
            }
        } else {
            int f = -1; for (int i = 0; i < MAX_FILES; i++) {
                if (fs[i].is_active && fs[i].type == TYPE_DIR && fs[i].parent_idx == current_dir_idx && strcmp(fs[i].name, name) == 0) { f = i; break; }
            }
            if (f != -1) {
                current_dir_idx = f;
                if (current_path[strlen(current_path)-1] != '/') strlcat(current_path, "/", 63);
                strlcat(current_path, fs[f].name, 63);
            } else kprint_color("Nao encontrado.\n", DR_RED);
        }
    }
    else if (str_starts_with(input, "cpuid") == 0) {
        unsigned int b, c, d; __asm__ volatile ("cpuid" : "=b"(b), "=c"(c), "=d"(d) : "a"(0));
        char v[13]; v[0] = (char)(b & 0xFF); v[1] = (char)((b >> 8) & 0xFF); v[2] = (char)((b >> 16) & 0xFF); v[3] = (char)((b >> 24) & 0xFF);
        v[4] = (char)(d & 0xFF); v[5] = (char)((d >> 8) & 0xFF); v[6] = (char)((d >> 16) & 0xFF); v[7] = (char)((d >> 24) & 0xFF);
        v[8] = (char)(c & 0xFF); v[9] = (char)((c >> 8) & 0xFF); v[10] = (char)((c >> 16) & 0xFF); v[11] = (char)((c >> 24) & 0xFF); v[12] = '\0';
        kprint("Hardware: "); kprint_color(v, DR_CYAN); kprint("\n");
    }
    else if (strcmp(input, "time") == 0) {
        outb(0x70, 0x04); unsigned char hor = inb(0x71);
        outb(0x70, 0x02); unsigned char min = inb(0x71);
        hor = (hor & 0x0F) + ((hor >> 4) * 10); min = (min & 0x0F) + ((min >> 4) * 10);
        char t[6]; t[0] = (hor/10)+'0'; t[1] = (hor%10)+'0'; t[2] = ':'; t[3] = (min/10)+'0'; t[4] = (min%10)+'0'; t[5] = '\0';
        kprint("Hora: "); kprint_color(t, DR_CYAN); kprint("\n");
    }
    else {
        kprint_color("Erro: ", DR_RED); kprint(input); kprint("\n");
    }
}

void open_editor(int f_idx) {
    clear_screen(); kprint_color("EDICAO: ", DR_YELLOW); kprint_color(fs[f_idx].name, DR_WHITE);
    kprint_color("\n[Ctrl+S] Gravar [Ctrl+X] Sair\n---\n", DR_GREEN);
    char b[MAX_FILE_SIZE]; strncpy(b, fs[f_idx].content, MAX_FILE_SIZE - 1);
    int p = strlen(b); kprint(b);
    while (1) {
        unsigned char sc = get_scancode(); if (sc == 0) continue;
        if (sc == 0x1D) { ctrl_pressed = 1; continue; }
        if (sc == 0x9D) { ctrl_pressed = 0; continue; }
        if (sc & 0x80) continue;
        if (ctrl_pressed && sc == 0x1F) { strncpy(fs[f_idx].content, b, MAX_FILE_SIZE - 1); kprint_color("\n[Gravado!]", DR_GREEN); for (volatile int dw = 0; dw < 1000000; dw++); open_editor(f_idx); return; }
        if (ctrl_pressed && sc == 0x2D) { clear_screen(); return; }
        char c = scancode_to_char(sc); if (sc == 0x1C) c = '\n'; 
        if (sc == 0x0E) { if (p > 0) { p--; b[p] = '\0'; kprint_backspace(); } continue; }
        if (c != 0 && p < MAX_FILE_SIZE - 1) { b[p++] = c; b[p] = '\0'; kprint_char(c, DR_WHITE); }
    }
}

/* --- Utils --- */
void outb(unsigned short p, unsigned char d) { __asm__ volatile ("outb %0, %1" : : "a" (d), "Nd" (p)); }
unsigned char inb(unsigned short p) { unsigned char r; __asm__ volatile ("inb %1, %0" : "=a" (r) : "Nd" (p)); return r; }
unsigned char get_scancode() { if (inb(0x64) & 1) return inb(0x60); return 0; }
char scancode_to_char(unsigned char sc) {
    if (sc == 0x1E) return 'a'; if (sc == 0x30) return 'b'; if (sc == 0x2E) return 'c'; if (sc == 0x20) return 'd';
    if (sc == 0x12) return 'e'; if (sc == 0x21) return 'f'; if (sc == 0x22) return 'g'; if (sc == 0x23) return 'h';
    if (sc == 0x17) return 'i'; if (sc == 0x24) return 'j'; if (sc == 0x25) return 'k'; if (sc == 0x26) return 'l';
    if (sc == 0x32) return 'm'; if (sc == 0x31) return 'n'; if (sc == 0x18) return 'o'; if (sc == 0x19) return 'p';
    if (sc == 0x10) return 'q'; if (sc == 0x13) return 'r'; if (sc == 0x1F) return 's'; if (sc == 0x14) return 't';
    if (sc == 0x16) return 'u'; if (sc == 0x2F) return 'v'; if (sc == 0x11) return 'w'; if (sc == 0x2D) return 'x';
    if (sc == 0x15) return 'y'; if (sc == 0x2C) return 'z'; if (sc == 0x39) return ' ';
    if (sc >= 0x02 && sc <= 0x0B) return (sc == 0x0B) ? '0' : (sc - 1 + '0');
    return 0;
}
void kprint_char(char c, char col) { char *v = (char*)0xb8000; int o = (cursor_y * 80 + cursor_x) * 2; v[o] = c; v[o + 1] = col; cursor_x++; if (cursor_x >= 80) { cursor_x = 0; cursor_y++; } update_cursor(cursor_x, cursor_y); }
void kprint_color(char *m, char c) { for (int i = 0; m[i] != '\0'; i++) { if (m[i] == '\n') { cursor_x = 0; cursor_y++; } else kprint_char(m[i], c); } }
void kprint(char *m) { kprint_color(m, DR_WHITE); }
void kprint_backspace() { if (cursor_x > prompt_len) { cursor_x--; char *v = (char*)0xb8000; int o = (cursor_y * 80 + cursor_x) * 2; v[o] = ' '; v[o+1] = DR_BACK; update_cursor(cursor_x, cursor_y); } }
void clear_screen() { char *v = (char*)0xb8000; for (int i = 0; i < 25 * 80; i++) { v[i*2] = ' '; v[i*2+1] = DR_BACK | (DR_WHITE << 0); } cursor_x = 0; cursor_y = 0; update_cursor(0, 0); }
void update_cursor(int x, int y) { unsigned short p = y * 80 + x; outb(0x3d4, 0x0F); outb(0x3d5, (unsigned char)(p & 0xFF)); outb(0x3d4, 0x0E); outb(0x3d5, (unsigned char)((p >> 8) & 0xFF)); }
int strcmp(char *s1, char *s2) { int i; for (i = 0; s1[i] == s2[i]; i++) if (s1[i] == '\0') return 0; return s1[i] - s2[i]; }
int strlen(char *s) { int i = 0; while (s[i] != '\0') i++; return i; }
void strncpy(char *d, char *s, int n) { int i; for (i = 0; i < n && s[i] != '\0'; i++) d[i] = s[i]; for ( ; i < n; i++) d[i] = '\0'; }
void strlcat(char *d, char *s, int n) { int dl = strlen(d); int i; for (i = 0; i < n - dl - 1 && s[i] != '\0'; i++) d[dl + i] = s[i]; d[dl+i] = '\0'; }
int str_starts_with(char *s, char *p) { int i = 0; while (p[i] != '\0') { if (s[i] != p[i]) return 0; i++; } return 1; }
int atoi(char *s) { int r = 0; for (int i = 0; s[i] >= '0' && s[i] <= '9'; ++i) r = r * 10 + s[i] - '0'; return r; }
void itoa(int n, char *s) { int i = 0, sign; if ((sign = n) < 0) n = -n; do { s[i++] = n % 10 + '0'; } while ((n /= 10) > 0); if (sign < 0) s[i++] = '-'; s[i] = '\0'; reverse(s); }
void reverse(char *s) { int r, j; char c; for (r = 0, j = strlen(s)-1; r<j; r++, j--) { c = s[r]; s[r] = s[j]; s[j] = c; } }