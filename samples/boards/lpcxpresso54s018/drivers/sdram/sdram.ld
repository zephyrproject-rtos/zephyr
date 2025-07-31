/* SDRAM linker script additions
 * Copyright (c) 2025 Xtooltech
 * SPDX-License-Identifier: Apache-2.0
 *
 * This follows Zephyr conventions for external memory regions
 */

/* Example sections that can be placed in SDRAM */
SECTIONS
{
    /* Large data buffers in SDRAM - follows Zephyr naming convention */
    SECTION_DATA_PROLOGUE(sdram_data,,)
    {
        __sdram_data_start = .;
        *(.sdram_data)
        *(.sdram_data.*)
        __sdram_data_end = .;
    } GROUP_DATA_LINK_IN(SDRAM, SDRAM)

    /* BSS section in SDRAM - follows Zephyr naming convention */
    SECTION_PROLOGUE(sdram_bss,(NOLOAD),)
    {
        __sdram_bss_start = .;
        *(.sdram_bss)
        *(.sdram_bss.*)
        __sdram_bss_end = .;
    } GROUP_LINK_IN(SDRAM)
}