/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

MEMORY
{
    /* External SPIFI flash - 4MB starting at 0x10000000 */
    FLASH (rx) : ORIGIN = 0x10000000, LENGTH = 4M
    
    /* Internal SRAM - using SRAM0 for simplicity */
    RAM (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

/* Entry point */
ENTRY(Reset_Handler)

/* Symbols */
_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    /* Vector table at start of flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* Program code */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* Data section */
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT> FLASH

    /* BSS section */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
    
    /* End of used RAM */
    . = ALIGN(4);
    _end = .;
    PROVIDE(end = .);
}