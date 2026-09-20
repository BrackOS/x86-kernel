SRC_DIR := src
BUILD_DIR := build
ISO_DIR := iso

CC := gcc
AS := nasm
LD := ld

CFLAGS := -m32 -std=gnu99 -ffreestanding -fno-stack-protector -fno-pie -no-pie \
          -Wall -Wextra -O2 -nostdlib -fno-builtin
ASFLAGS := -f elf32
LDFLAGS := -m elf_i386 -T $(SRC_DIR)/linker.ld -nostdlib

OBJS := $(BUILD_DIR)/boot.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/vga.o $(BUILD_DIR)/keyboard.o

.PHONY: all clean iso run

all: iso

$(BUILD_DIR)/boot.o: $(SRC_DIR)/boot.s | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/brackos.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

iso: $(BUILD_DIR)/brackos.bin
	mkdir -p $(ISO_DIR)/boot/grub
	cp $(SRC_DIR)/../iso/boot/grub/grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	cp $(BUILD_DIR)/brackos.bin $(ISO_DIR)/boot/brackos.bin
	grub-mkrescue -o brackos.iso $(ISO_DIR)

run: iso
	qemu-system-i386 -cdrom brackos.iso

clean:
	rm -rf $(BUILD_DIR) brackos.iso $(ISO_DIR)/boot/brackos.bin $(ISO_DIR)/boot/grub/grub.cfg
