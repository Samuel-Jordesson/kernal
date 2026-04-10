#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f
#define BLUE_ON_BLACK  0x01
#define LIGHT_BLUE_ON_BLACK 0x09
#define GREEN_ON_BLACK 0x02
#define RED_ON_BLACK 0x04
#define YELLOW_ON_BLACK 0x0e

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
int strcmp(char *s1, char *s2);
int strlen(char *s);
void strncpy(char *dest, char *src, int n);
void strlcat(char *dest, char *src, int n);
unsigned char get_scancode();
char scancode_to_char(unsigned char scancode);
void execute_command(char *input);
void open_editor(int file_idx);
void print_prompt();

int cursor_x = 0;
int cursor_y = 0;

void kernel_main() {
    clear_screen();
    kprint_color("--- Shell Hierarquico v2.1 ---\n", GREEN_ON_BLACK);
    kprint("Comandos: ls, mkdir, aqdir, cd, ler, clear, help\n\n");
    
    for(int i = 0; i < MAX_FILES; i++) fs[i].is_active = 0;

    char key_buffer[256];
    int buffer_idx = 0;

    print_prompt();

    while (1) {
        unsigned char scancode = get_scancode();
        if (scancode == 0) continue;

        if (scancode == 0x1D) { ctrl_pressed = 1; continue; }
        if (scancode == 0x9D) { ctrl_pressed = 0; continue; }
        if (scancode > 0x80) continue; 

        char c = scancode_to_char(scancode);

        if (scancode == 0x1C) { // Enter
            kprint("\n");
            key_buffer[buffer_idx] = '\0';
            execute_command(key_buffer);
            buffer_idx = 0;
            print_prompt();
        } 
        else if (scancode == 0x0E) { // Backspace
            if (buffer_idx > 0) {
                buffer_idx--;
                kprint_backspace();
            }
        } 
        else if (c != 0) {
            if (buffer_idx < 255) {
                key_buffer[buffer_idx++] = c;
                kprint_char(c, WHITE_ON_BLACK);
            }
        }
    }
}

void print_prompt() {
    kprint_color("admin@meu-os:", LIGHT_BLUE_ON_BLACK);
    kprint_color(current_path, LIGHT_BLUE_ON_BLACK);
    kprint_color("$ ", LIGHT_BLUE_ON_BLACK);
}

void execute_command(char *input) {
    if (strcmp(input, "clear") == 0) {
        clear_screen();
    } 
    else if (strcmp(input, "help") == 0) {
        kprint_color("Comandos Disponiveis:\n", YELLOW_ON_BLACK);
        kprint("  ls             - Lista arquivos e pastas no diretorio atual\n");
        kprint("  mkdir <nome>   - Cria uma nova pasta\n");
        kprint("  aqdir <nome>   - Cria um novo arquivo vazio\n");
        kprint("  cd <nome>      - Entra em uma pasta\n");
        kprint("  cd ..          - Volta para a pasta anterior\n");
        kprint("  ler <nome>     - Abre o editor para ler/escrever em um arquivo\n");
        kprint("  clear          - Limpa toda a tela do terminal\n");
        kprint("  cpuid          - Exibe informacoes do processador (Vendor ID)\n");
        kprint("  time           - Exibe a hora atual do sistema (RTC)\n");
        kprint("  echo <texto>   - Repete o texto digitado na tela\n");
        kprint("  help           - Mostra esta lista de ajuda\n");
    } 
    else if (strcmp(input, "ls") == 0) {
        for (int i = 0; i < MAX_FILES; i++) {
            if (fs[i].is_active && fs[i].parent_idx == current_dir_idx) {
                if (fs[i].type == TYPE_DIR) kprint_color("[DIR] ", BLUE_ON_BLACK);
                else kprint_color("[ARQ] ", WHITE_ON_BLACK);
                kprint(fs[i].name);
                kprint("\n");
            }
        }
    }
    else if (input[0] == 'm' && input[1] == 'k' && input[2] == 'd' && input[3] == 'i' && input[4] == 'r') {
        char *name = input + 6;
        if (file_count < MAX_FILES) {
            strncpy(fs[file_count].name, name, 31);
            fs[file_count].type = TYPE_DIR;
            fs[file_count].parent_idx = current_dir_idx;
            fs[file_count].is_active = 1;
            file_count++;
        }
    }
    else if (input[0] == 'a' && input[1] == 'q' && input[2] == 'd' && input[3] == 'i' && input[4] == 'r') {
        char *name = input + 6;
        if (file_count < MAX_FILES) {
            strncpy(fs[file_count].name, name, 31);
            fs[file_count].type = TYPE_FILE;
            fs[file_count].parent_idx = current_dir_idx;
            fs[file_count].content[0] = '\0';
            fs[file_count].is_active = 1;
            file_count++;
        }
    }
    else if (input[0] == 'c' && input[1] == 'd') {
        char *name = input + 3;
        if (strcmp(name, "..") == 0) {
            if (current_dir_idx != -1) {
                current_dir_idx = fs[current_dir_idx].parent_idx;
                if (current_dir_idx == -1) {
                    current_path[0] = '/'; current_path[1] = '\0';
                } else {
                    current_path[0] = '/'; current_path[1] = '\0';
                    strlcat(current_path, fs[current_dir_idx].name, 63);
                }
            }
        } else {
            int found = -1;
            for (int i = 0; i < MAX_FILES; i++) {
                if (fs[i].is_active && fs[i].type == TYPE_DIR && 
                    fs[i].parent_idx == current_dir_idx && strcmp(fs[i].name, name) == 0) {
                    found = i; break;
                }
            }
            if (found != -1) {
                current_dir_idx = found;
                if (current_path[strlen(current_path)-1] != '/') strlcat(current_path, "/", 63);
                strlcat(current_path, fs[found].name, 63);
            } else {
                kprint_color("Erro: Diretorio nao encontrado.\n", RED_ON_BLACK);
            }
        }
    }
    else if (input[0] == 'l' && input[1] == 'e' && input[2] == 'r') {
        char *name = input + 4;
        int found = -1;
        for (int i = 0; i < MAX_FILES; i++) {
            if (fs[i].is_active && fs[i].type == TYPE_FILE && 
                fs[i].parent_idx == current_dir_idx && strcmp(fs[i].name, name) == 0) {
                found = i; break;
            }
        }
        if (found != -1) open_editor(found);
        else kprint_color("Erro: Arquivo nao encontrado.\n", RED_ON_BLACK);
    }
}

void open_editor(int file_idx) {
    clear_screen();
    kprint_color("EDITOR - ", YELLOW_ON_BLACK);
    kprint_color(fs[file_idx].name, WHITE_ON_BLACK);
    kprint_color("\n[Ctrl+S] Salvar  [Ctrl+X] Sair\n---\n", GREEN_ON_BLACK);

    char editor_buffer[MAX_FILE_SIZE];
    strncpy(editor_buffer, fs[file_idx].content, MAX_FILE_SIZE - 1);
    int editor_pos = strlen(editor_buffer);
    kprint(editor_buffer);

    while (1) {
        unsigned char sc = get_scancode();
        if (sc == 0) continue;
        if (sc == 0x1D) { ctrl_pressed = 1; continue; }
        if (sc == 0x9D) { ctrl_pressed = 0; continue; }
        if (sc > 0x80) continue;

        if (ctrl_pressed && sc == 0x1F) { // S
            strncpy(fs[file_idx].content, editor_buffer, MAX_FILE_SIZE - 1);
            kprint_color("\n[Salvo!]", GREEN_ON_BLACK);
            for (volatile int d = 0; d < 1000000; d++);
            open_editor(file_idx);
            return;
        }
        if (ctrl_pressed && sc == 0x2D) { // X
            clear_screen();
            return;
        }

        char c = scancode_to_char(sc);
        if (sc == 0x1C) c = '\n'; 
        if (sc == 0x0E) {
            if (editor_pos > 0) {
                editor_pos--;
                editor_buffer[editor_pos] = '\0';
                kprint_backspace();
            }
            continue;
        }
        if (c != 0 && editor_pos < MAX_FILE_SIZE - 1) {
            editor_buffer[editor_pos++] = c;
            editor_buffer[editor_pos] = '\0';
            kprint_char(c, WHITE_ON_BLACK);
        }
    }
}

/* --- Utils --- */
void outb(unsigned short port, unsigned char data) {
    __asm__ volatile ("outb %0, %1" : : "a" (data), "Nd" (port));
}
unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}
unsigned char get_scancode() {
    if (inb(0x64) & 1) return inb(0x60);
    return 0;
}
unsigned char scancode_to_ascii[] = {
    0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', 0,
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};
char scancode_to_char(unsigned char scancode) {
    if (scancode < sizeof(scancode_to_ascii)) return scancode_to_ascii[scancode];
    return 0;
}
void kprint_char(char c, char color) {
    char *video = (char*) VIDEO_ADDRESS;
    int offset = (cursor_y * MAX_COLS + cursor_x) * 2;
    video[offset] = c;
    video[offset + 1] = color;
    cursor_x++;
    if (cursor_x >= MAX_COLS) { cursor_x = 0; cursor_y++; }
    update_cursor(cursor_x, cursor_y);
}
void kprint_color(char *message, char color) {
    for (int i = 0; message[i] != '\0'; i++) {
        if (message[i] == '\n') { cursor_x = 0; cursor_y++; }
        else kprint_char(message[i], color);
    }
}
void kprint(char *message) { kprint_color(message, WHITE_ON_BLACK); }
void kprint_backspace() {
    if (cursor_x > 0) {
        cursor_x--;
        char *video = (char*) VIDEO_ADDRESS;
        int offset = (cursor_y * MAX_COLS + cursor_x) * 2;
        video[offset] = ' '; video[offset + 1] = WHITE_ON_BLACK;
        update_cursor(cursor_x, cursor_y);
    }
}
void clear_screen() {
    char *video = (char*) VIDEO_ADDRESS;
    for (int i = 0; i < MAX_ROWS * MAX_COLS; i++) {
        video[i * 2] = ' '; video[i * 2 + 1] = WHITE_ON_BLACK;
    }
    cursor_x = 0; cursor_y = 0; update_cursor(0, 0);
}
void update_cursor(int x, int y) {
    unsigned short position = y * MAX_COLS + x;
    outb(REG_SCREEN_CTRL, 0x0F); outb(REG_SCREEN_DATA, (unsigned char)(position & 0xFF));
    outb(REG_SCREEN_CTRL, 0x0E); outb(REG_SCREEN_DATA, (unsigned char)((position >> 8) & 0xFF));
}
int strcmp(char *s1, char *s2) {
    int i;
    for (i = 0; s1[i] == s2[i]; i++) if (s1[i] == '\0') return 0;
    return s1[i] - s2[i];
}
int strlen(char *s) {
    int i = 0; while (s[i] != '\0') i++; return i;
}
void strncpy(char *dest, char *src, int n) {
    int i; for (i = 0; i < n && src[i] != '\0'; i++) dest[i] = src[i];
    for ( ; i < n; i++) dest[i] = '\0';
}
void strlcat(char *dest, char *src, int n) {
    int d_len = strlen(dest);
    int i;
    for (i = 0; i < n - d_len - 1 && src[i] != '\0'; i++) dest[d_len + i] = src[i];
    dest[d_len + i] = '\0';
}