/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Generic Zephyr Linker Script
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>

#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>
#include <zephyr/linker/iterable_sections.h>

/* Define memory regions based on the original script */
#define FLASH_BASE_ADDRESS CONFIG_FLASH_BASE_ADDRESS
#define FLASH_SIZE CONFIG_FLASH_SIZE
#define SRAM_BASE_ADDRESS CONFIG_SRAM_BASE_ADDRESS
#define SRAM_SIZE CONFIG_SRAM_SIZE

/* Define Zephyr's standard memory region names */
#define ROMABLE_REGION program
#define RAMABLE_REGION data


CRT0_STARTUP(reset0.S.obj)
CRT1_STARTUP(reset1.S.obj)
CRT_STARTMODE(crt_start_mode_normal)
MEMORY
{
    /* Memory map from the original target-specific script */
    program (xr) : ORIGIN = FLASH_BASE_ADDRESS, LENGTH = (FLASH_SIZE * 1K)
    data (a!xr) : ORIGIN = SRAM_BASE_ADDRESS, LENGTH = (SRAM_SIZE * 1K)
    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST (wx) : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}
/*
 * After adding gc-section option compiler was discarding __start symbol
 * adding ENTRY directive avoid it from being discarded.
 */
ENTRY(__start)

SECTIONS
{
    /* Start of the ROM-based sections */
    GROUP_START(ROMABLE_REGION)

    SECTION_PROLOGUE(.rom_start,,)
    {
        LONG(ABSOLUTE(__start));
        . = ALIGN(4);
        /* Located in generated directory. This file is populated by calling
        * zephyr_linker_sources(ROM_START ...). This typically contains the vector
        * table and debug information.
        */
        #include <snippets-rom-start.ld>
    } GROUP_LINK_IN(ROMABLE_REGION)

    /*
     * This is the main code and read-only data section.
     * Zephyr's GROUP_LINK_IN macro handles all .text.* and .rodata.*
     * sections automatically. This replaces the manual .text section
     * from the original script.
     */
    SECTION_PROLOGUE(._TEXT_SECTION_NAME,,)
    {
        __text_region_start = .;
        #include <zephyr/linker/kobject-text.ld>
        *(.init);
        *(.text);
        *(.text*);
        *(.user_init);
        KEEP (*(.handle));
        KEEP (*(.isr*));
        *(.lib*);
        __text_region_end = .;
        . = ALIGN(4);
    } GROUP_LINK_IN(ROMABLE_REGION)

    ___rodata_region_start = .;
    #include <zephyr/linker/common-rom.ld>

    /* Located in generated directory. This file is populated by calling
    * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
    */
    #include <snippets-rom-sections.ld>
    #include <zephyr/linker/thread-local-storage.ld>

    SECTION_PROLOGUE(.rodata,,)
    {
        *(.rodata)
        *(.rodata.*)
        /* Located in generated directory. This file is populated by the
        * zephyr_linker_sources() Cmake function.
        */
        #include <snippets-rodata.ld>
        #include <zephyr/linker/kobject-rom.ld>
    } GROUP_LINK_IN(ROMABLE_REGION)

    . = ALIGN(4);
    ___rodata_region_end = .;
    #include <zephyr/linker/cplusplus-rom.ld>
    GROUP_END(ROMABLE_REGION)

    /* Start of the RAM-based sections */
    GROUP_START(RAMABLE_REGION)

    /*
     * This aligns the start of RAM as required.
     * The VMA (Virtual Memory Address) is in RAM,
     * while the LMA (Load Memory Address) is in FLASH.
     */
    . = SRAM_BASE_ADDRESS;

    /*
     * This section handles initialized data. The data is loaded from FLASH
     * and copied to RAM at boot time. This replaces the manual .data
     * section from the original script.
     */
    SECTION_PROLOGUE(.data,,)
    {
        __data_region_start = .;
        *(.data)
        *(".data.*")
	    *(".kernel.*")

        /* Located in generated directory. This file is populated by the
        * zephyr_linker_sources() Cmake function.
        */
        #include <snippets-rwdata.ld>
        __data_region_end = .;
    } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

    #include <zephyr/linker/common-ram.ld>

    #include <zephyr/linker/kobject-data.ld>

    #include <zephyr/linker/cplusplus-ram.ld>

    /* Located in generated directory. This file is populated by the
    * zephyr_linker_sources() Cmake function.
    */
    #include <snippets-data-sections.ld>

    /* Located in generated directory. This file is populated by the
    * zephyr_linker_sources() Cmake function.
    */
    #include <snippets-sections.ld>

    #include <zephyr/linker/debug-sections.ld>

    /*
     * This section handles uninitialized data (BSS). It occupies space
     * only in RAM and is zeroed out at boot. This replaces the manual
     * .bss section from the original script.
     */
    SECTION_DATA_PROLOGUE(._BSS_SECTION_NAME,,)
    {
        . = ALIGN(4);
        ___bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        ___bss_end = .;
    } GROUP_LINK_IN(RAMABLE_REGION)


    GROUP_END(RAMABLE_REGION)

}

PROVIDE(__isr_wrapper = __COMMONInterrupt);
