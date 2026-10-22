BrackOS v1.7 - Complete Build Package
======================================

FOLDER STRUCTURE
================
brackos-v1.7-build/
├── src/                    All source files (.c, .h, .s, .ld)
├── Makefile                Build script
├── README.txt              This file
└── docs/                   Documentation

QUICK START (Linux or WSL2 Ubuntu)
==================================
Build on Linux or WSL2. The old MSYS2/MinGW route produced PE objects,
which do not link with the ELF assembly files.

1. Install the tools (Ubuntu/Debian):
   sudo apt install build-essential nasm binutils grub-pc-bin grub-common xorriso mtools qemu-system-x86

2. Build:
   make clean
   make all          (produces kernel.bin)
   make iso          (produces kernel.iso with GRUB)

3. Run in QEMU:
   make run
   (or: qemu-system-x86_64 -cdrom kernel.iso -m 512)

WHAT'S INSIDE
=============

CORE SYSTEM (v1.0-v1.4)
  - boot.s: Multiboot2 header, 32→64-bit, paging
  - kernel.c: Main kernel with shell
  - vga.c/h: VGA text mode (80x25)
  - keyboard.c/h: PS/2 keyboard
  - idt.c/h/s: Interrupt system (IDT, PIC, IRQ handlers)
  - io.h: I/O port access

NEW v1.7: TIMER & RTC
  - rtc.c/h: CMOS real-time clock (date/time reading)
  - timer.c/h: PIT timer, IRQ0 handler (@100Hz)

RECOVERY SYSTEM (v1.5)
  - recovery.c/h: Boot diagnostics, safe mode

FILESYSTEM (v1.5+)
  - fat32.c/h: FAT32 driver, disk I/O
  - fs_shell.c/h: A: drive, ls, cd, mkdir, nf, rm, rmdir
  - paper.c/h: Text editor (1000 lines max)

GUI SYSTEM (v1.2+)
  - fb.c/h: Framebuffer 1024×768@32bpp VBE
  - font.c/h: 8×8 bitmap font
  - window.c/h: Window manager
  - gui_commands.c/h: Desktop UI
  - mouse.c/h: PS/2 mouse driver
  - cursor.c/h: Cursor rendering

COMMANDS (23 TOTAL)
===================

Core (16):
  help, echo, clear, info, mem, cpu, time, date, uptime,
  ver, credits, mouse, fs, gui, shutdown, reboot, repeat

Filesystem (7):
  ls, cd, mkdir, nf, paper, rm, rmdir

FEATURES
========
✅ x86-64 Long Mode (Multiboot2)
✅ VGA text mode (80×25)
✅ PS/2 keyboard & mouse
✅ Interrupt system (IDT, PIC, IRQs 0-47)
✅ Real-time clock (CMOS RTC) - NEW v1.7
✅ System timer (PIT IRQ0 @100Hz) - NEW v1.7
✅ Clock commands: time, date, uptime - NEW v1.7
✅ FAT32 filesystem (A: drive)
✅ Directory navigation (ls, cd, mkdir, rmdir)
✅ File operations (nf, rm, paper editor)
✅ Recovery system (boot diagnostics)
✅ GUI desktop (1024×768@32bpp)

REQUIREMENTS
============
Linux or WSL2 (Ubuntu/Debian) with: gcc, binutils (ld, objcopy), nasm,
grub-pc-bin + grub-common + xorriso (for grub-mkrescue), qemu-system-x86.

BUILD COMMANDS
==============
  make clean       - Remove old build files
  make all         - Compile kernel
  make iso         - Create GRUB ISO (kernel.iso)
  make run         - Build the ISO and boot it in QEMU
  make rebuild     - Clean + all

TEST IN EMULATOR
================
QEMU:
  qemu-system-x86_64 -cdrom kernel.iso -m 512

Bochs:
  bochs -f bochsrc.bxrc

VERIFY INSTALLATION
====================
After boot, you should see:
  BrackOS v1.7 Shell
  Type 'help' for commands

Try these commands:
  time              (shows HH:MM:SS)
  date              (shows YYYY-MM-DD)
  uptime            (shows days, hours:minutes:seconds)
  ls                (list A: drive)
  mkdir test        (create folder)
  cd test           (change directory)
  nf hello.txt      (create file)
  paper hello.txt   (open text editor)

DOCUMENTATION
==============
See docs/ folder:
  - BRACKOS_V1.7_RELEASE_NOTES.md     Full feature docs
  - BUILD_CHECKLIST.txt               Step-by-step guide
  - LATEST_FILES_MANIFEST.txt         Complete file listing
  - README_x86_64_CLEAN.md            GitHub README

TROUBLESHOOTING
===============
If compilation fails:
  1. Check Makefile paths are correct
  2. Verify all .c files have .h headers
  3. Run: make clean && make all (from fresh)
  4. Check MSYS2 has gcc, nasm, binutils

If timer/RTC doesn't work:
  1. Boot should show: "RTC: 20XX-MM-DD HH:MM:SS"
  2. "info" should show "Timer: IRQ0 active"
  3. Try "uptime" - should increment every second

If filesystem fails:
  1. "fs" command should show FAT32 info
  2. "ls" should show directory
  3. "mkdir test" should create folder

VERSION HISTORY
===============
v1.7  - Timer (PIT IRQ0), RTC (CMOS), clock commands
v1.6  - Filesystem shell (A: drive), Paper editor
v1.5  - Recovery system, FAT32 driver
v1.4  - Interrupt system (IDT, PIC)
v1.3  - Mouse driver, cursor
v1.2  - GUI desktop, windows, taskbar
v1.1  - GUI framework, framebuffer
v1.0  - Basic kernel, VGA, keyboard

NEXT STEPS (v1.8)
=================
- Continuous RTC sync
- File modification timestamps
- Timezone support
- Performance monitoring

SUPPORT
=======
Check BUILD_CHECKLIST.txt for detailed troubleshooting
All files are in src/ and ready to compile

Good luck! 🚀

KNOWN LIMITATIONS (v1.7 build fixes)
====================================
- nasm was not available in the environment used for these fixes. The
  assembly was checked by translating it to GNU as and linking the kernel:
  the multiboot2 header, checksum, load addresses and entry point are correct.
  Real nasm may still report syntax errors; run `make` and report them.
- The GRUB ISO (grub-mkrescue) was not run in that environment.
- Nothing has been booted yet. Test with `make run` and check the screen.
- The recovery boot counter is not stored between boots, so Safe Mode never
  triggers automatically.
- The paper editor clips lines at 79 characters and has no scrolling sideways.
- rmdir does not free the directory's clusters yet (it only marks the entry deleted).
- Directories cannot grow beyond their first cluster in mkdir (nf and file
  writes do grow directories).
- info, cpu and mem: cpu and mem report real values; info still shows static text.

BUILD FIX (post-nasm-test)
===========================
- src/idt.s was renamed to src/idt_stubs.s. It and src/idt.c both compiled
  to src/idt.o under the old name, so `make` silently linked only the
  assembly stubs and dropped the C interrupt handlers (idt_init,
  exception_handler, irq_handler, interrupts_enable/disable,
  pic_enable_irq), causing "undefined reference" errors at link time.
  Confirmed fixed: the renamed build links cleanly with all six symbols
  resolved (verified via a GNU-as translation, since nasm is still not
  available in the environment used for these fixes).

BUILD FIX (display output)
===========================
- boot.s no longer requests a 1024x768 graphics mode from GRUB. That
  request was being honored, which left the screen in graphics mode
  before the kernel ran -- so the shell's VGA text-mode writes (to
  0xB8000) had no visible effect. The boot screen now stays in standard
  VGA text mode, which is what vga.c actually writes to.
- KNOWN CONSEQUENCE: the "gui" command still assumes a linear
  framebuffer is already active at 0xfd000000. With the framebuffer
  request removed, that address is no longer set up by GRUB, so "gui"
  will likely show nothing or draw to invalid memory until a real
  VBE/VESA mode switch is added. Avoid "gui" for now; it is a separate,
  lower-priority fix.

ABOUT THIS BUILD (x86 / 32-bit)
================================
This is BrackOS compiled for 32-bit protected mode, for CPUs that do not
support 64-bit long mode. It is the same kernel source as the x86-64
build, with a 32-bit-specific boot.s and idt_stubs.s:

- boot.s: GRUB already hands a Multiboot2 kernel control in 32-bit
  protected mode, so this build does not do any PAE/paging/long-mode
  setup -- it just sets up its own GDT and a stack before calling kmain().
- idt_stubs.s: 32-bit interrupt stubs. These use the cdecl calling
  convention (arguments passed on the stack), which is different from
  the register-passed convention x86-64 code uses.
- idt.c/idt.h use the correct interrupt-descriptor layout for 32-bit
  protected mode (8 bytes per gate, vs. 16 bytes in long mode).
- This build runs flat, with no page tables, relying on segment limits
  instead -- that is expected, not a bug, and the recovery screen's
  diagnostics reflect that.
- Two spots that did 64-bit decimal division (in meminfo.c and timer.c)
  were rewritten to avoid it, since i386 has no hardware 64-bit divide
  and a 32-bit libgcc may not be installed on every build machine.

VERIFICATION STATUS
====================
- All C files compile cleanly with -Wall -Wextra (m32) and zero warnings.
- The assembly was checked by translating it to GNU as syntax and
  linking the real kernel objects, since nasm was not available in the
  environment used for these fixes. The Multiboot2 header, checksum,
  and entry point were all confirmed correct this way.
- nasm itself has not run on this code, and the kernel has NOT been
  booted (no display/disk output confirmed). Build it and test in QEMU
  (`make run`) before trusting it -- if nasm reports errors, or the
  screen is blank, share the output and it can be debugged from there.
