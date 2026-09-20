# BrackOS x86-32

A minimal but functional 32-bit operating system kernel for x86 architecture, bootable via GRUB and runnable on real hardware or QEMU.

**Architecture:** x86 (32-bit)  
**Boot:** GRUB 2 Multiboot1  
**CPU Mode:** 32-bit Protected Mode  
**Display:** VGA text mode (80×25)  
**Input:** PS/2 keyboard  
**Shell:** Simple command-line interface

---

## Quick Facts

- **True 32-bit kernel** running in protected mode
- **Multiboot1 bootloader** supporting BIOS and UEFI firmware
- **Simple memory layout** with kernel at 1MB (0x100000)
- **VGA text display** with 16 foreground and 8 background colors
- **PS/2 keyboard input** with shift key support via polling
- **Simple shell** with three commands: help, echo, clear
- **~380 lines of C code** + ~32 lines of x86 assembly

---

## Build Requirements

**Linux/macOS (x86/x64):**
```bash
sudo apt-get install build-essential nasm grub-common xorriso qemu-system-x86
```

**Windows 11 (MSYS2):**
1. Download and install [MSYS2](https://www.msys2.org/)
2. In MSYS2 terminal:
   ```bash
   pacman -Syu
   pacman -S mingw-w64-i686-gcc mingw-w64-i686-binutils nasm xorriso grub
   ```

---

## Compile

```bash
make clean && make iso
```

Creates `myos.iso` (bootable ISO image).

---

## Run in QEMU

```bash
make run
```

Or manually:
```bash
qemu-system-i386 -cdrom myos.iso
```

Output:
```
MyOS booted successfully.
Type 'help' for a list of commands.

myos> █
```

Exit QEMU: `Ctrl+A` then `X`

---

## Boot on Real Hardware

1. Write ISO to USB:
   - Download [Balena Etcher](https://www.balena.io/etcher/)
   - Select `myos.iso`
   - Select USB drive
   - Click "Flash"

2. Boot x86 PC from USB:
   - Insert USB
   - Restart computer
   - Press `F9` (or `F12`/`Delete` - varies by manufacturer) during startup
   - Select USB device from boot menu
   - System boots

3. Use the shell:
   ```
   myos> help
   myos> echo Hello from MyOS!
   myos> clear
   ```

---

## Shell Commands

### `help`
Displays available commands.

### `echo <text>`
Prints text to screen. Example:
```
myos> echo Hello World
Hello World
```

### `clear`
Clears the screen and moves cursor to top-left.

### Unknown Command
Displays error in red: `error: unknown command: [input]`

---

## Technical Details

### Boot Process
1. GRUB 2 loads kernel as Multiboot1 image
2. Bootloader sets up 32-bit protected mode
3. Boot code (_start) jumps to kernel_main()
4. Kernel initializes VGA display and keyboard input

### Memory Layout
- **Kernel load:** 0x100000 (1 MB)
- **Stack:** 16 KB in .bss section
- **VGA buffer:** 0xB8000
- **Addressing:** Standard 32-bit flat memory model

### Display
- **Mode:** VGA text mode
- **Resolution:** 80 columns × 25 rows
- **Memory:** Direct access to 0xB8000
- **Colors:** 16 foreground (0-15) + 8 background (0-7)
- **Features:** Automatic scrolling, backspace support

### Input
- **Hardware:** PS/2 keyboard only
- **Method:** Port polling (0x60 data, 0x64 status)
- **Scancodes:** Set 1 with shift key support
- **Blocking:** Input waits for keypress
- **CPU PAUSE:** Used in polling loop for efficiency

### Shell
- **Command buffer:** 256 characters max
- **Parsing:** Case-sensitive, exact matching
- **Input echo:** Characters displayed as typed
- **Backspace:** Supported
- **Error display:** Red text

---

## Architecture

### CPU Mode
- 32-bit protected mode
- GDT (Global Descriptor Table) set up by GRUB

### CPU Features NOT Used
- Paging (disabled, flat memory)
- Interrupts (disabled)
- Exceptions (no handlers)
- IDT (not implemented)
- TSS (not implemented)

---

## Known Limitations

- **No paging** - Flat memory model, no virtual memory
- **No exception handling** - Page faults, GPF, etc. not handled
- **No interrupts** - All hardware interaction via polling
- **No multitasking** - Single process only
- **No filesystem** - No disk I/O
- **No heap** - No dynamic memory allocation
- **No protected memory** - Single privilege level
- **No serial I/O** - No serial port support
- **No networking** - No network drivers
- **No timers** - No clock or sleep functions
- **No PCI/ACPI** - No bus enumeration
- **Text mode only** - No graphics mode

---

## Source Structure

| File | Lines | Purpose |
|------|-------|---------|
| `src/boot.s` | 32 | Multiboot1 header and entry point |
| `src/kernel.c` | 106 | Main shell loop, command parser, line reader |
| `src/vga.c` | 94 | VGA text display driver (0xB8000 access) |
| `src/keyboard.c` | 103 | PS/2 keyboard driver with scancode translation |
| `src/io.h` | 16 | I/O port primitives (inb/outb) |
| `src/vga.h` | 14 | VGA function declarations |
| `src/keyboard.h` | 9 | Keyboard function declarations |
| `src/linker.ld` | 28 | Memory layout, section alignment, kernel base address |
| `iso/boot/grub/grub.cfg` | 13 | GRUB 2 boot configuration (Multiboot1) |

**Total:** ~415 lines of code (C + assembly + config)

---

## Bootloader Details

### Multiboot1 Compliance
- **Magic number:** 0x1BADB002
- **Flags:** 0x00000003 (ALIGN | MEMINFO)
- **Checksum:** Correct (calculated)

### GRUB Configuration
- **Boot protocol:** Multiboot1
- **Kernel image:** `/boot/myos.bin`
- **Graphics:** Handled by BIOS/UEFI firmware
- **Timeout:** 0 seconds (immediate boot)

### Supported Firmware
- **BIOS:** Yes (standard legacy boot)
- **UEFI:** Yes (with compatible boot mode)

---

## Compilation Details

### Compiler Flags
```
-m32              : 32-bit target
-ffreestanding    : Freestanding environment
-fno-stack-protector : Disable stack canaries
-nostdlib         : No standard library
-fno-builtin      : Disable compiler builtins
```

### Assembler
- NASM with `-f elf32` output format

### Linker
- GNU ld, `-m elf_i386` emulation
- Entry point: `_start`
- Load address: 0x100000

---

## Testing

Tested on:
- QEMU i386 emulation
- Real x86/x64 PC hardware (BIOS boot)

---

## File Structure

```
BrackOS/
├── src/
│   ├── boot.s           x86-32 bootloader entry
│   ├── kernel.c         Main kernel + shell
│   ├── vga.c/vga.h      VGA text display driver
│   ├── keyboard.c/h     PS/2 keyboard driver
│   ├── io.h             I/O port primitives
│   └── linker.ld        Memory layout (0x100000 start)
├── iso/
│   └── boot/grub/
│       └── grub.cfg     GRUB 2 boot configuration
├── Makefile             Build system
└── README.md            This file
```

---

## Next Steps

1. **Compile:** `make clean && make iso`
2. **Test:** `make run` (QEMU)
3. **Deploy:** Write to USB with Balena Etcher
4. **Boot:** Insert USB, press F9 at startup, select USB

---

## Differences from x86-64 Version

- **32-bit code** instead of 64-bit
- **No paging** - flat memory model instead of identity-mapped pages
- **Simpler bootloader** - Multiboot1 instead of Multiboot2
- **No mode transition** - Stays in 32-bit protected mode
- **Smaller binary** - No page table setup code

---

**Ready to build and boot!** 🚀
