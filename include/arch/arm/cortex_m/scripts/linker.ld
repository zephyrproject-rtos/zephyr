/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M3 platform.
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>
#include <generated_dts_board.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

/* physical address of RAM */
#ifdef CONFIG_XIP
  #define ROMABLE_REGION FLASH
  #define RAMABLE_REGION SRAM
#else
  #define ROMABLE_REGION SRAM
  #define RAMABLE_REGION SRAM
#endif

#if defined(CONFIG_XIP)
  #define _DATA_IN_ROM __data_rom_start
#else
  #define _DATA_IN_ROM
#endif

#if !defined(SKIP_TO_KINETIS_FLASH_CONFIG)
  #define SKIP_TO_KINETIS_FLASH_CONFIG
#endif

#define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)
#ifdef CONFIG_TI_CCFG_PRESENT
  #define CCFG_SIZE 88
  #define ROM_SIZE (CONFIG_FLASH_SIZE*1K - CONFIG_FLASH_LOAD_OFFSET - \
		    CCFG_SIZE)
  #define CCFG_ADDR (ROM_ADDR + ROM_SIZE)
#else
#if CONFIG_FLASH_LOAD_SIZE > 0
  #define ROM_SIZE CONFIG_FLASH_LOAD_SIZE
#else
  #define ROM_SIZE (CONFIG_FLASH_SIZE*1K - CONFIG_FLASH_LOAD_OFFSET)
#endif
#endif

#if defined(CONFIG_XIP)
	#if defined(CONFIG_IS_BOOTLOADER)
		#define RAM_SIZE (CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
		#define RAM_ADDR (CONFIG_SRAM_BASE_ADDRESS + \
			(CONFIG_SRAM_SIZE * 1K - RAM_SIZE))
	#else
		#define RAM_SIZE CONFIG_SRAM_SIZE * 1K
		#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
	#endif
#else
	#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K - CONFIG_BOOTLOADER_SRAM_SIZE * 1K)
	#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS
#endif

MEMORY
    {
    FLASH                 (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
#ifdef CONFIG_TI_CCFG_PRESENT
    FLASH_CCFG            (rwx): ORIGIN = CCFG_ADDR, LENGTH = CCFG_SIZE
#endif
    SRAM                  (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE

    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = (RAM_ADDR + RAM_SIZE), LENGTH = 2K
    }

SECTIONS
    {
    GROUP_START(ROMABLE_REGION)

	_image_rom_start = ROM_ADDR;

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
#ifdef CONFIG_CC3220SF_DEBUG
	/* Add CC3220SF flash header to disable flash verification */
	. = 0x0;
	KEEP(*(.dbghdr))
	KEEP(*(".dbghdr.*"))
#endif

	. = CONFIG_TEXT_SECTION_OFFSET;
	KEEP(*(.exc_vector_table))
	KEEP(*(".exc_vector_table.*"))

	KEEP(*(IRQ_VECTOR_TABLE))

	KEEP(*(.openocd_dbg))
	KEEP(*(".openocd_dbg.*"))

	/* Kinetis has to write 16 bytes at 0x400 */
	SKIP_TO_KINETIS_FLASH_CONFIG
	KEEP(*(.kinetis_flash_config))
	KEEP(*(".kinetis_flash_config.*"))

#ifdef CONFIG_GEN_SW_ISR_TABLE
	KEEP(*(SW_ISR_TABLE))
#endif

	_image_text_start = .;
	*(.text)
	*(".text.*")
	*(.gnu.linkonce.t.*)
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;

	SECTION_PROLOGUE(.ARM.exidx,,)
	{
	/*
	 * This section, related to stack and exception unwinding, is placed
	 * explicitly to prevent it from being shared between multiple regions.
	 * It  must be defined for gcc to support 64-bit math and avoid
	 * section overlap.
	 */
	__exidx_start = .;
#if defined (__GCC_LINKER_CMD__)
	*(.ARM.exidx* gnu.linkonce.armexidx.*)
#endif
	__exidx_end = .;
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/common-rom.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
	*(.rodata)
	*(".rodata.*")
	*(.gnu.linkonce.r.*)

#ifdef CONFIG_CUSTOM_RODATA_LD
/* Located in project source directory */
#include <custom-rodata.ld>
#endif
	/*
	 * For XIP images, in order to avoid the situation when __data_rom_start
	 * is 32-bit aligned, but the actual data is placed right after rodata
	 * section, which may not end exactly at 32-bit border, pad rodata
	 * section, so __data_rom_start points at data and it is 32-bit aligned.
	 *
	 * On non-XIP images this may enlarge image size up to 3 bytes. This
	 * generally is not an issue, since modern ROM and FLASH memory is
	 * usually 4k aligned.
	 */
	. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_rom_end = .;
    __data_rom_start = .;    /* XIP imaged DATA ROM start addr */

    GROUP_END(ROMABLE_REGION)

/* Some TI SoCs have a special configuration footer, at the end of flash. */
#ifdef CONFIG_TI_CCFG_PRESENT
    SECTION_PROLOGUE(.ti_ccfg,,)
    {
    KEEP(*(TI_CCFG))
    } > FLASH_CCFG
#endif

    GROUP_START(RAMABLE_REGION)

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
	_image_ram_start = .;
	__data_ram_start = .;
	*(.top_of_image_ram)
	*(.top_of_image_ram.*)
	*(.data)
	*(".data.*")

#ifdef CONFIG_CUSTOM_RWDATA_LD
/* Located in project source directory */
#include <custom-rwdata.ld>
#endif

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <linker/common-ram.ld>

    __data_ram_end = .;

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
	__bss_start = .;
	*(.bss)
	*(".bss.*")
	COMMON_SYMBOLS
        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		 */
	__bss_end = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

    SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
        {
        /*
         * This section is used for non-initialized objects that
         * will not be cleared during the boot process.
         */
        *(.noinit)
        *(".noinit.*")
        *(.bottom_of_image_ram)
        *(.bottom_of_image_ram.*)
        } GROUP_LINK_IN(RAMABLE_REGION)

    /* Define linker symbols */

	_image_ram_end = .;
    _end = .; /* end of image */

    GROUP_END(RAMABLE_REGION)

#ifdef CONFIG_CUSTOM_SECTIONS_LD
/* Located in project source directory */
#include <custom-sections.ld>
#endif

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif

    }
