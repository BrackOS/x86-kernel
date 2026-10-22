# BrackOS (x86 / 32-bit protected mode)
#
# A 32-bit build of the BrackOS kernel, for CPUs that do not support
# 64-bit long mode. Unlike long mode, GRUB already hands a Multiboot2
# kernel control in 32-bit protected mode, so boot.s here does not need
# any PAE/paging/long-mode setup -- it just sets up its own GDT and a
# stack before calling into C.
#
# Build on Linux or WSL2 (Ubuntu/Debian). Install the tools with:
#   sudo apt install build-essential nasm binutils grub-pc-bin grub-common xorriso mtools qemu-system-x86
#
# Override any tool from the command line, e.g. `make CC=x86_64-elf-gcc`.

CC            ?= gcc
AS            = nasm
LD            ?= ld
OBJCOPY       ?= objcopy
GRUB_MKRESCUE ?= grub-mkrescue
QEMU          ?= qemu-system-x86_64

CFLAGS  = -ffreestanding -fno-stack-protector -fno-builtin -fno-pic -fno-pie \
          -m32 -Wall -Wextra
LDFLAGS = -m elf_i386 -T src/linker.ld -nostdlib -z noexecstack
ASFLAGS = -f elf32

# Source files
BOOT     = src/boot.s
IDT_ASM  = src/idt_stubs.s
KERNEL   = src/kernel.c
SOURCES  = src/vga.c src/keyboard.c src/fb.c src/font.c src/window.c \
           src/gui_commands.c src/mouse.c src/cursor.c src/idt.c \
           src/recovery.c src/fat32.c src/fs_shell.c src/paper.c \
           src/rtc.c src/timer.c src/string.c src/heap.c src/cpu.c src/meminfo.c src/multiboot.c

OBJS       = $(BOOT:.s=.o) $(IDT_ASM:.s=.o) $(KERNEL:.c=.o) $(SOURCES:.c=.o)
KERNEL_ELF = kernel.elf
KERNEL_BIN = kernel.bin
ISO        = kernel.iso

.PHONY: all clean rebuild iso run

all: $(KERNEL_BIN)

%.o: %.s
	$(AS) $(ASFLAGS) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(KERNEL_ELF): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# Flat binary: GRUB loads it at the address given in the Multiboot2 header
$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary $^ $@

iso: $(KERNEL_BIN)
	rm -rf iso
	mkdir -p iso/boot/grub
	cp $(KERNEL_BIN) iso/boot/kernel.bin
	cp grub/grub.cfg iso/boot/grub/grub.cfg
	$(GRUB_MKRESCUE) -o $(ISO) iso

run: iso
	$(QEMU) -cdrom $(ISO) -m 512

clean:
	rm -f $(OBJS) $(KERNEL_ELF) $(KERNEL_BIN) $(ISO)
	rm -rf iso

rebuild: clean all
