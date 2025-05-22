/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Linker script for the Cortex-M platforms.
 */

#include <zephyr/linker/sections.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain/common.h>

#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

/* physical address of RAM */
#ifdef CONFIG_XIP
#define ROMABLE_REGION FLASH
#else
#define ROMABLE_REGION RAM
#endif
#define RAMABLE_REGION RAM

#if !defined(CONFIG_XIP) && (CONFIG_FLASH_SIZE == 0)
#define ROM_ADDR RAM_ADDR
#else
#define ROM_ADDR (CONFIG_FLASH_BASE_ADDRESS + CONFIG_FLASH_LOAD_OFFSET)
#endif

#if defined(CONFIG_ROM_END_OFFSET)
#define ROM_END_OFFSET CONFIG_ROM_END_OFFSET
#else
#define ROM_END_OFFSET 0
#endif

#if CONFIG_FLASH_LOAD_SIZE > 0
#define ROM_SIZE (CONFIG_FLASH_LOAD_SIZE - ROM_END_OFFSET)
#else
#define ROM_SIZE (CONFIG_FLASH_SIZE * 1024 - CONFIG_FLASH_LOAD_OFFSET - ROM_END_OFFSET)
#endif

#define RAM_SIZE (CONFIG_SRAM_SIZE * 1K)
#define RAM_ADDR CONFIG_SRAM_BASE_ADDRESS

#if defined(CONFIG_CUSTOM_SECTION_ALIGN)
_region_min_align = CONFIG_CUSTOM_SECTION_MIN_ALIGN_SIZE;
#else
/* Set alignment to CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE
 * to make linker section alignment comply with MPU granularity.
 */
#if defined(CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE)
_region_min_align = CONFIG_ARM_MPU_REGION_MIN_ALIGN_AND_SIZE;
#else
/* If building without MPU support, use default 4-byte alignment. */
_region_min_align = 4;
#endif
#endif

#if !defined(CONFIG_CUSTOM_SECTION_ALIGN) && defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define MPU_ALIGN(region_size) \
    . = ALIGN(_region_min_align); \
    . = ALIGN( 1 << LOG2CEIL(region_size))
#else
#define MPU_ALIGN(region_size) \
    . = ALIGN(_region_min_align)
#endif

#include <zephyr/linker/linker-devnull.h>

MEMORY
    {
    FLASH (rx) : ORIGIN = ROM_ADDR, LENGTH = ROM_SIZE
    RAM   (wx) : ORIGIN = RAM_ADDR, LENGTH = RAM_SIZE
#if defined(CONFIG_LINKER_DEVNULL_MEMORY)
    DEVNULL_ROM (rx) : ORIGIN = DEVNULL_ADDR, LENGTH = DEVNULL_SIZE
#endif
    LINKER_DT_REGIONS()
    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST (wx) : ORIGIN = 0xFFFF7FFF, LENGTH = 32K
    }

ENTRY(CONFIG_KERNEL_ENTRY)

#include <bootstrap.ld>

SECTIONS
    {

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

    /*
     * .plt and .iplt are here according to 'arm-zephyr-elf-ld --verbose',
     * before text section.
     */
    /DISCARD/ :
	{
	*(.plt)
	}

    /DISCARD/ :
	{
	*(.iplt)
	}

    GROUP_START(ROMABLE_REGION)

	__rom_region_start = ROM_ADDR;

    SECTION_PROLOGUE(rom_start,,)
	{


	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_CODE_DATA_RELOCATION

#include <linker_relocate.ld>

#endif /* CONFIG_CODE_DATA_RELOCATION */

    SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
	__text_region_start = .;

#include <zephyr/linker/kobject-text.ld>

	*(.text)
	*(".text.*")
	*(".TEXT.*")
	*(.gnu.linkonce.t.*)

	/*
	 * These are here according to 'arm-zephyr-elf-ld --verbose',
	 * after .gnu.linkonce.t.*
	 */
	*(.glue_7t) *(.glue_7) *(.vfp11_veneer) *(.v4_bx)
	. = ALIGN(4);

	} GROUP_LINK_IN(ROMABLE_REGION)

	__text_region_end = .;

#if defined (CONFIG_CPP)
	SECTION_PROLOGUE(.ARM.extab,,)
	{
	/*
	 * .ARM.extab section containing exception unwinding information.
	 */
	*(.ARM.extab* .gnu.linkonce.armextab.*)
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif

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

	__rodata_region_start = .;

#include <zephyr/linker/common-rom.ld>
#include <zephyr/linker/thread-local-storage.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
	*(.rodata)
	*(".rodata.*")
	*(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <zephyr/linker/kobject-rom.ld>

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

#include <zephyr/linker/cplusplus-rom.ld>

#if defined(CONFIG_BUILD_ALIGN_LMA)
    /*
     * Include a padding section here to make sure that the LMA address
     * of the sections in the RAMABLE_REGION are aligned with those
     * section's VMA alignment requirements.
     */
    SECTION_PROLOGUE(padding_section,,)
	{
	__rodata_region_end = .;
	MPU_ALIGN(__rodata_region_end - ADDR(rom_start));
	} GROUP_LINK_IN(ROMABLE_REGION)
#else
	__rodata_region_end = .;
	MPU_ALIGN(__rodata_region_end - ADDR(rom_start));
#endif
	__rom_region_end = __rom_region_start + . - ADDR(rom_start);

    GROUP_END(ROMABLE_REGION)

    /*
     * These are here according to 'arm-zephyr-elf-ld --verbose',
     * before data section.
     */
    /DISCARD/ : {
	*(.got.plt)
	*(.igot.plt)
	*(.got)
	*(.igot)
	}

    GROUP_START(RAMABLE_REGION)

	. = RAM_ADDR;
	/* Align the start of image RAM with the
	 * minimum granularity required by MPU.
	 */
	. = ALIGN(_region_min_align);
	_image_ram_start = .;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#if defined(CONFIG_USERSPACE)
#define APP_SHARED_ALIGN . = ALIGN(_region_min_align);
#define SMEM_PARTITION_ALIGN MPU_ALIGN

#include <app_smem.ld>

	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
	__bss_start = .;
	__kernel_ram_start = .;

	*(.bss)
	*(".bss.*")
	*(COMMON)
	*(".kernel_bss.*")

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_bss_relocate.ld>
#endif

        /*
         * As memory is cleared in words only, it is simpler to ensure the BSS
         * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		 */
	__bss_end = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

#endif  /* CONFIG_USERSPACE */

    GROUP_START(DATA_REGION)

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
	__data_region_start = .;
	__data_start = .;
	*(.data)
	*(".data.*")
	*(".kernel.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_data_relocate.ld>
#endif
	__data_end = .;

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
    __data_size = __data_end - __data_start;
    __data_load_start = LOADADDR(_DATA_SECTION_NAME);

    __data_region_load_start = LOADADDR(_DATA_SECTION_NAME);

#include <zephyr/linker/common-ram.ld>
#include <zephyr/linker/kobject-data.ld>

#include <zephyr/linker/cplusplus-ram.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-data-sections.ld>

    __data_region_end = .;

#ifndef CONFIG_USERSPACE
   SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
        /*
         * For performance, BSS section is assumed to be 4 byte aligned and
         * a multiple of 4 bytes
         */
        . = ALIGN(4);
	__bss_start = .;
	__kernel_ram_start = .;

	*(.bss)
	*(".bss.*")
	*(COMMON)
	*(".kernel_bss.*")

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_bss_relocate.ld>
#endif

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
	*(".kernel_noinit.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-noinit.ld>

        } GROUP_NOLOAD_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
#endif /* CONFIG_USERSPACE */

    /* Define linker symbols */

    __kernel_ram_end = RAM_ADDR + RAM_SIZE;
    __kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_itcm), okay)
GROUP_START(ITCM)

	SECTION_PROLOGUE(_ITCM_SECTION_NAME,,SUBALIGN(4))
	{
		__itcm_start = .;
		*(.itcm)
		*(".itcm.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function. */
#include <snippets-itcm-section.ld>

		__itcm_end = .;
	} GROUP_LINK_IN(ITCM AT> ROMABLE_REGION)

	__itcm_size = __itcm_end - __itcm_start;
	__itcm_load_start = LOADADDR(_ITCM_SECTION_NAME);

GROUP_END(ITCM)
#endif

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
GROUP_START(DTCM)

	SECTION_PROLOGUE(_DTCM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__dtcm_start = .;
		__dtcm_bss_start = .;
		*(.dtcm_bss)
		*(".dtcm_bss.*")
		__dtcm_bss_end = .;
	} GROUP_LINK_IN(DTCM)

	SECTION_PROLOGUE(_DTCM_NOINIT_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__dtcm_noinit_start = .;
		*(.dtcm_noinit)
		*(".dtcm_noinit.*")
		__dtcm_noinit_end = .;
	} GROUP_LINK_IN(DTCM)

	SECTION_PROLOGUE(_DTCM_DATA_SECTION_NAME,,SUBALIGN(4))
	{
		__dtcm_data_start = .;
		*(.dtcm_data)
		*(".dtcm_data.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function. */
#include <snippets-dtcm-section.ld>

		__dtcm_data_end = .;
	} GROUP_LINK_IN(DTCM AT> ROMABLE_REGION)

	__dtcm_end = .;

	__dtcm_data_load_start = LOADADDR(_DTCM_DATA_SECTION_NAME);

GROUP_END(DTCM)
#endif

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#include <zephyr/linker/ram-end.ld>

    GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

    /DISCARD/ : { *(.note.GNU-stack) }

    SECTION_PROLOGUE(.ARM.attributes, 0,)
	{
	KEEP(*(.ARM.attributes))
	KEEP(*(.gnu.attributes))
	}

    /* Sections generated from 'zephyr,memory-region' nodes */
    LINKER_DT_SECTIONS()

/* Must be last in romable region */
SECTION_PROLOGUE(.last_section,,)
{
#ifdef CONFIG_LINKER_LAST_SECTION_ID
  /* Fill last section with a word to ensure location counter and actual rom
   * region data usage match. */
  LONG(CONFIG_LINKER_LAST_SECTION_ID_PATTERN)
#endif
} GROUP_LINK_IN(ROMABLE_REGION)

/* To provide the image size as a const expression,
 * calculate this value here. */
_flash_used = LOADADDR(.last_section) + SIZEOF(.last_section) - __rom_region_start;

    }
