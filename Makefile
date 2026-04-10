all: os.bin

boot.bin: boot.asm
	nasm -f bin boot.asm -o boot.bin

kernel.o: kernel.c
	gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -c kernel.c -o kernel.o

kernel.bin: kernel.o linker.ld
	ld -m elf_i386 -o kernel.bin -T linker.ld kernel.o --oformat binary

os.bin: boot.bin kernel.bin
	cat boot.bin kernel.bin > os.bin
	dd if=/dev/zero bs=512 count=20 >> os.bin
	# Garantir que a imagem tenha o tamanho total dos setores lidos

run: os.bin
	qemu-system-x86_64 -fda os.bin

clean:
	rm -f *.bin *.o
