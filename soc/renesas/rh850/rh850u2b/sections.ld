# Copyright (c) 2026 Renesas Electronics Corporation
/* SPDX-License-Identifier: Apache-2.0 */

SECTIONS
{
    .text : ALIGN(4)
    {
        KEEP(*(.vectors))
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
    } > FLASH

    .data : ALIGN(4)
    {
        *(.data*)
    } > RAM AT > FLASH

    .bss (NOLOAD) : ALIGN(4)
    {
        *(.bss*)
        *(COMMON)
    } > RAM
}
