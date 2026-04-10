// Endereço da memória de vídeo VGA
#define VIDEO_ADDRESS 0xb8000
#define MAX_ROWS 25
#define MAX_COLS 80
#define WHITE_ON_BLACK 0x0f

#define REG_SCREEN_CTRL 0x3d4
#define REG_SCREEN_DATA 0x3d5

// Protótipos das funções auxiliares
void clear_screen();
void kprint(char *message);
char get_char();
void update_cursor(int x, int y);
void outb(unsigned short port, unsigned char data);
unsigned char inb(unsigned short port);

/* 
   IMPORTANTE: kernel_main deve ser a PRIMEIRA função do arquivo 
   para que o bootloader a execute corretamente ao pular para 0x1000.
*/
void kernel_main() {
    // Debug inicial direto na memória de vídeo
    char *video = (char*) VIDEO_ADDRESS;
    video[2] = 'K'; // 'K' de Kernel
    video[3] = 0x0e; // Amarelo

    clear_screen();
    kprint("--- Kernel iniciado ---\n");
    kprint("Digite algo: ");

    while (1) {
        char c = get_char();
        if (c != 0) {
            char str[2] = {c, '\0'};
            kprint(str);
        }
    }
}

/* --- Implementação das Funções Auxiliares --- */

void outb(unsigned short port, unsigned char data) {
    __asm__ volatile ("outb %0, %1" : : "a" (data), "Nd" (port));
}

unsigned char inb(unsigned short port) {
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

void update_cursor(int x, int y) {
    unsigned short position = y * MAX_COLS + x;
    outb(REG_SCREEN_CTRL, 0x0F);
    outb(REG_SCREEN_DATA, (unsigned char)(position & 0xFF));
    outb(REG_SCREEN_CTRL, 0x0E);
    outb(REG_SCREEN_DATA, (unsigned char)((position >> 8) & 0xFF));
}

int cursor_x = 0;
int cursor_y = 0;

void clear_screen() {
    char *video = (char*) VIDEO_ADDRESS;
    for (int i = 0; i < MAX_ROWS * MAX_COLS; i++) {
        video[i * 2] = ' ';
        video[i * 2 + 1] = WHITE_ON_BLACK;
    }
    cursor_x = 0;
    cursor_y = 0;
    update_cursor(0, 0);
}

void kprint(char *message) {
    char *video = (char*) VIDEO_ADDRESS;
    int i = 0;
    while (message[i] != '\0') {
        if (message[i] == '\n') {
            cursor_x = 0;
            cursor_y++;
        } else {
            int offset = (cursor_y * MAX_COLS + cursor_x) * 2;
            video[offset] = message[i];
            video[offset + 1] = WHITE_ON_BLACK;
            cursor_x++;
        }

        if (cursor_x >= MAX_COLS) {
            cursor_x = 0;
            cursor_y++;
        }
        i++;
    }
    update_cursor(cursor_x, cursor_y);
}

unsigned char scancode_to_ascii[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`', 0,
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*', 0, ' '
};

char get_char() {
    if (inb(0x64) & 1) {
        unsigned char scancode = inb(0x60);
        if (scancode < 0x80) {
            return scancode_to_ascii[scancode];
        }
    }
    return 0;
}