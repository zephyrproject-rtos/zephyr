/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Nios II platform
 */

#include <zephyr/linker/sections.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* These sections are specific to this CPU */
#define _EXCEPTION_SECTION_NAME exceptions
#define _RESET_SECTION_NAME reset

/* This linker script requires the following macros to be defined in the
 * SOC-specific linker script. All of these values can be found defined
 * in system.h for CPU configurations that can generate a HAL.
 *
 * _RESET_VECTOR     CPU entry point at boot
 * _EXC_VECTOR       General exception vector
 * _ROM_ADDR         Beginning of flash memory
 * _ROM_SIZE         Size in bytes of flash memory
 * _RAM_ADDR         Beginning of RAM
 * _RAM_SIZE         Size of RAM in bytes
 *
 * For now we support two scenarios:
 *
 * 1. Non-XIP systems where the reset vector is at the beginning of RAM
 * with the exception vector 0x20 bytes after it.
 * 2. XIP systems where the reset vector is at the beginning of ROM and
 * the exception vector is in RAM
 */


#ifdef CONFIG_XIP
	#define ROMABLE_REGION FLASH
	#define RAMABLE_REGION SRAM
#else
	#define ROMABLE_REGION SRAM
	#define RAMABLE_REGION SRAM
#endif

#ifdef CONFIG_XIP

ASSERT(_RESET_VECTOR == _ROM_ADDR, "Reset vector not at beginning of ROM!")

MEMORY
    {
    RESET (rx) : ORIGIN = _RESET_VECTOR, LENGTH = 0x20
    FLASH (rx) : ORIGIN = _RESET_VECTOR + 0x20 , LENGTH = (_ROM_SIZE - 0x20)
    SRAM  (wx) : ORIGIN = _EXC_VECTOR, LENGTH = _RAM_SIZE - (_EXC_VECTOR - _RAM_ADDR)
    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K

    }

#else

MEMORY
    {
    RESET (wx) : ORIGIN = _RESET_VECTOR, LENGTH = 0x20
    SRAM  (wx) : ORIGIN = _EXC_VECTOR, LENGTH = _RAM_SIZE - (_EXC_VECTOR - _RAM_ADDR)

    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}
#endif

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
    {

#include <zephyr/linker/rel-sections.ld>

    /*
     * .plt and .iplt are here according to
     * 'nios2-zephyr-elf-ld --verbose', before text section.
     */
    SECTION_PROLOGUE(.plt,,)
        {
        *(.plt)
        }

    SECTION_PROLOGUE(.iplt,,)
        {
        *(.iplt)
        }

    GROUP_START(ROMABLE_REGION)
    __rom_region_start = _ROM_ADDR;

    SECTION_PROLOGUE(_RESET_SECTION_NAME,,)
    {
        KEEP(*(.reset.*))
    } GROUP_LINK_IN(RESET)

#ifndef CONFIG_XIP
    SECTION_PROLOGUE(_EXCEPTION_SECTION_NAME,,)
        {
            KEEP(*(".exception.entry.*"))
            *(".exception.other.*")

        } GROUP_LINK_IN(ROMABLE_REGION)
#endif

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
        {
        /* XXX If ALT_CPU_RESET_ADDR is not the same as _ROM_ADDR
         * we are going to waste flash space? */
        . = ALT_CPU_RESET_ADDR;

        __text_region_start = .;

        *(.text)
        *(".text.*")
        *(.gnu.linkonce.t.*)
        } GROUP_LINK_IN(ROMABLE_REGION)

    __text_region_end = .;

#if defined(CONFIG_GP_ALL_DATA)
        _gp = ABSOLUTE(. + 0x8000);
        PROVIDE(gp = _gp);
#endif

    __rodata_region_start = .;

#include <zephyr/linker/common-rom.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
        {
        . = ALIGN(4);

        *(.rodata)
        *(".rodata.*")
        *(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

        . = ALIGN(4);
        } GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>

    __rodata_region_end = .;
    __rodata_region_size = __rodata_region_end - __rodata_region_start;

    __rom_region_end = .;
    __data_region_load_start = ALIGN(4);    /* XIP imaged DATA ROM start addr */

    GROUP_END(ROMABLE_REGION)

    GROUP_START(RAMABLE_REGION)

#ifdef CONFIG_XIP
    /* Altera strongly recommends keeping exception entry code in RAM
     * even on XIP systems
     *
     * This is code not data, but we need this copied just like XIP data
     */

    SECTION_DATA_PROLOGUE(_EXCEPTION_SECTION_NAME,,)
        {
            _image_ram_start = .;
            __data_region_start = .;

            KEEP(*(".exception.entry.*"))
            *(".exception.other.*")

        } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif

#ifndef CONFIG_XIP
        _image_ram_start = .;
#endif

#include <zephyr/linker/common-ram.ld>

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
        {
	__data_start = .;
        *(.data)
        *(".data.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

        /* the Nios2 architecture only has 16-bit signed immediate offsets in
         * the instructions, so accessing a general address requires typically
         * three instructions - basically, two for the two halves of the 32-bit
         * address, and one to merge them - but if we can put the most commonly
         * accessed globals in a special 64K span of memory addressed by the GP
         * register, then we can access those values in a single instruction,
         * saving both codespace and runtime.
         *
         * Since these immediate offsets are signed, place gp 0x8000 past the
         * beginning of .sdata so that we can use both positive and negative
         * offsets.
         */
#if defined(CONFIG_GP_LOCAL) || defined(CONFIG_GP_GLOBAL)
        _gp = ABSOLUTE(. + 0x8000);
        PROVIDE(gp = _gp);
#endif

        *(.sdata .sdata.* .gnu.linkonce.s.*)
        *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>
	__data_end = .;

        } GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
    __data_size = __data_end - __data_start;
    __data_load_start = LOADADDR(_DATA_SECTION_NAME);

#include <zephyr/linker/cplusplus-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

    __data_region_end = .;

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
        {
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
        __bss_start = .;
        *(.sbss)
        *(".sbss.*")
        *(.bss)
        *(".bss.*")
        COMMON_SYMBOLS
        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
                 */
        __bss_end = ALIGN(4);
        } GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

    /* Define linker symbols */
    _image_ram_end = .;
    _end = .; /* end of image */

    GROUP_END(RAMABLE_REGION)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#include <zephyr/linker/debug-sections.ld>

    }
