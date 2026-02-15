/*
 * Test Application Linker Script
 * Places application at APP_BASE + sizeof(fw_header_t) (0x80010010)
 * This accounts for the 16-byte firmware header written by bootloader
 */

OUTPUT_ARCH(riscv)
ENTRY(_start)

MEMORY
{
    APP (rwx) : ORIGIN = 0x80010010, LENGTH = 448K - 16
}

SECTIONS
{
    . = ORIGIN(APP);

    .text :
    {
        *(.text.init)    /* Entry point first */
        *(.text .text.*)
        *(.rodata .rodata.*)
    }

    .data :
    {
        *(.data .data.*)
    }

    __bss_start = .;
    .bss :
    {
        *(.bss .bss.*)
        *(COMMON)
    }
    __bss_end = .;
}
