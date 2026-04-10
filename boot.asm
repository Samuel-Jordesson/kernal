[org 0x7c00]
KERNEL_OFFSET equ 0x1000

mov [BOOT_DRIVE], dl

; Setup stack
mov bp, 0x9000
mov sp, bp

; Print OK
mov ah, 0x0e
mov al, 'O'
int 0x10
mov al, 'K'
int 0x10

call load_kernel
call switch_to_pm ; Note: switch_to_pm does not return

jmp $

load_kernel:
    mov bx, KERNEL_OFFSET
    
    ; Setup segment registers for disk read
    push ax
    xor ax, ax
    mov es, ax
    pop ax

    mov dl, [BOOT_DRIVE]
    mov ah, 0x02
    mov al, 15        ; read 15 sectors
    mov ch, 0x00
    mov dh, 0x00
    mov cl, 0x02      ; sector 2

    int 0x13
    jc disk_error
    ret

disk_error:
    mov ah, 0x0e
    mov al, 'E'
    int 0x10
    jmp $

; GDT Definitions
gdt_start:
    dq 0x0          ; null descriptor
gdt_code:           ; code descriptor
    dw 0xffff       ; limit 0-15
    dw 0x0          ; base 0-15
    db 0x0          ; base 16-23
    db 10011010b    ; flags
    db 11001111b    ; flags + limit 16-19
    db 0x0          ; base 24-31
gdt_data:           ; data descriptor
    dw 0xffff       ; limit 0-15
    dw 0x0          ; base 0-15
    db 0x0          ; base 16-23
    db 10010010b    ; flags
    db 11001111b    ; flags + limit 16-19
    db 0x0          ; base 24-31
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[bits 16]
switch_to_pm:
    cli                     ; 1. disable interrupts
    lgdt [gdt_descriptor]   ; 2. load GDT
    mov eax, cr0
    or eax, 0x1             ; 3. set PM bit in CR0
    mov cr0, eax
    jmp CODE_SEG:init_pm    ; 4. far jump to 32-bit code

[bits 32]
init_pm:
    mov ax, DATA_SEG        ; 5. update segment registers
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebp, 0x90000        ; 6. setup stack
    mov esp, ebp

    ; Depuração: Escrever 'P' no topo da tela
    mov [0xb8000], byte 'P'
    mov [0xb8001], byte 0x0f

    call KERNEL_OFFSET      ; 7. jump to kernel
    jmp $


BOOT_DRIVE db 0

times 510 - ($ - $$) db 0
dw 0xaa55