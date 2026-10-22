ENTRY(_start)

SECTIONS
{
    . = 0x100000;
    __kernel_start = .;

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.multiboot)          /* Multiboot2 header must come first */
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }
    __kernel_load_end = .;

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }
    __kernel_bss_end = .;
}
