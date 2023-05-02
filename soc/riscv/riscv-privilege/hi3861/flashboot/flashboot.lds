OUTPUT_ARCH( "riscv" )
ENTRY(_start)

MEMORY
{
    FLASH(rwx)              : ORIGIN = CONFIG_FLASH_BASE_ADDRESS + 0x040, LENGTH = CONFIG_FLASH_LOAD_OFFSET - 0x040 - 0x030
    STACK(rwx)              : ORIGIN = 0x00100000               , LENGTH = 8K
    HEAP(rwx)               : ORIGIN = 0x00100000 + 20K         , LENGTH = 20K
    FLASH_BOOT(rx)          : ORIGIN = 0x00100000 + 40K + 0x040 , LENGTH = 40K
    FIXED_ROM(rx)           : ORIGIN = 0x00000000 + 11K         , LENGTH = 21K
}

SECTIONS
{
    .text.entry : ALIGN(4)
    {
        KEEP(*(.text.entry))
    } > FLASH_BOOT AT > FLASH

    .stacks (NOLOAD) :
    {
        . = ALIGN(4);
        __SYSTEM_STACK_BEGIN__ = ORIGIN(STACK);
        KEEP(*(.stacks))
        __SYSTEM_STACK_END__ = ORIGIN(STACK) + LENGTH(STACK);
    } > STACK
    __SYSTEM_STACK_SIZE__ = __SYSTEM_STACK_END__ - __SYSTEM_STACK_BEGIN__;
    __stack_top = __SYSTEM_STACK_END__;

    .rom.data :
    {
        . = ALIGN(4);
        __global_pointer$ = .;
    } > FIXED_ROM

    .text : ALIGN(4)
    {
        __start_addr = .;
        *(.text*)
        . = ALIGN(4);
        __rodata_start = .;
        *(.rodata*)
        . = ALIGN(4);
        __rodata_end = .;
        __text_end = .;
    } > FLASH_BOOT AT > FLASH

    .data : ALIGN(4)
    {
        __data_load = LOADADDR(.data);
        __data_start = .;
        *(.data*)
        . = ALIGN(4);
        __data_end = .;
    } > FLASH_BOOT AT > FLASH

    .bss (NOLOAD) : ALIGN(4)
    {
        __bss_begin__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > FLASH_BOOT
    __bss_size__ = __bss_end__ - __bss_begin__;

    .heap (NOLOAD) :
    {
        . = ALIGN(4);
        __heap_begin__ = ORIGIN(HEAP);
        KEEP(*(.heap))
        __heap_end__ =  __heap_begin__ + LENGTH(HEAP);
    } > HEAP
}
