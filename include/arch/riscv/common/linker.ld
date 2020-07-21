/*
 * Copyright (c) 2016-2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Contributors: 2020 RISE Research Institutes of Sweden <www.ri.se>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file
 *
 * Generic Linker script for the riscv platform
 */

#include <soc.h>
#include <devicetree.h>

#include <autoconf.h>
#include <linker/sections.h>

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#ifdef CONFIG_XIP
#define ROMABLE_REGION ROM
#else
#define ROMABLE_REGION RAM
#endif
#define RAMABLE_REGION RAM

#define _VECTOR_SECTION_NAME	   vector
#define _EXCEPTION_SECTION_NAME	exceptions
#define _RESET_SECTION_NAME		reset

#if defined(CONFIG_CUSTOM_SECTION_ALIGN)
_region_min_align = CONFIG_CUSTOM_SECTION_MIN_ALIGN_SIZE;
#else
_region_min_align = 8;
#endif

#if !defined(CONFIG_CUSTOM_SECTION_ALIGN) && defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
#define MPU_ALIGN(region_size) \
	. = ALIGN(_region_min_align); \
	. = ALIGN( 1 << LOG2CEIL(region_size))
#else
#define MPU_ALIGN(region_size) \
	. = ALIGN(_region_min_align)
#endif

MEMORY
{
#ifdef CONFIG_XIP
#if DT_NODE_HAS_COMPAT_STATUS(DT_CHOSEN(zephyr_flash), soc_nv_flash, okay)
#define ROM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#define ROM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#elif DT_NODE_HAS_COMPAT_STATUS(DT_CHOSEN(zephyr_flash), jedec_spi_nor, okay)
/* For jedec,spi-nor we expect the spi controller to memory map the flash
 * and for that mapping to be the second register property of the spi
 * controller.
 */
#define SPI_CTRL DT_PARENT(DT_CHOSEN(zephyr_flash))
#define ROM_BASE DT_REG_ADDR_BY_IDX(SPI_CTRL, 1)
#define ROM_SIZE DT_REG_SIZE_BY_IDX(SPI_CTRL, 1)
#endif
	ROM (rx)  : ORIGIN = ROM_BASE, LENGTH = ROM_SIZE
#endif
	RAM (rwx) : ORIGIN = CONFIG_SRAM_BASE_ADDRESS, LENGTH = KB(CONFIG_SRAM_SIZE)
	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST  (wx)	 : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
	{

#include <linker/rel-sections.ld>

	/*
	* The .plt and .iplt are here according to
	* 'riscv32-zephyr-elf-ld --verbose', before text section.
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
	_image_rom_start = .;

	SECTION_PROLOGUE(_VECTOR_SECTION_NAME,,)
	{
		. = ALIGN(4);
		KEEP(*(.vectors.*))
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_RESET_SECTION_NAME,,)
	{
		KEEP(*(.reset.*))
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_EXCEPTION_SECTION_NAME,,)
	{
		 KEEP(*(".exception.entry.*"))
		 *(".exception.other.*")
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,)
	{
		. = ALIGN(4);
		KEEP(*(.openocd_debug))
		KEEP(*(".openocd_debug.*"))

#ifdef CONFIG_USERSPACE	
		MPU_ALIGN(1 << LOG2CEIL(_image_text_area_used));
#endif /* CONFIG_USERSPACE */

		_image_text_start = .;

#include <linker/kobject-text.ld>

		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)
		*(.eh_frame)

	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;

#ifdef CONFIG_USERSPACE
	_image_text_area_used = _image_text_end - _image_text_start;
	MPU_ALIGN(_image_text_area_used);
	MPU_ALIGN(_image_rodata_area_used);
#endif /* CONFIG_USERSPACE */

	_image_rodata_start = .;

#include <linker/common-rom.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		 . = ALIGN(4);
		 *(.srodata)
		 *(".srodata.*")
		 *(.rodata)
		 *(".rodata.*")
		 *(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <linker/kobject-rom.ld>

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

#include <linker/cplusplus-rom.ld>

	_image_rodata_end = .;
	_image_rodata_area_used = _image_rodata_end - _image_rodata_start;

	_image_rom_end = .;
	__data_rom_start = .;

	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		 . = ALIGN(4);
		 _image_ram_start = .;
		 __data_ram_start = .;

		 *(.data)
		 *(".data.*")

		 *(.sdata .sdata.* .gnu.linkonce.s.*)
		 *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

	}  GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#include <linker/common-ram.ld>
#include <linker/kobject.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>
	__data_ram_end = .;

#if defined(CONFIG_USERSPACE)

#define APP_SHARED_ALIGN . = ALIGN(_region_min_align);
#define SMEM_PARTITION_ALIGN MPU_ALIGN

#include <app_smem.ld>

	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);

#endif /* CONFIG_USERSPACE */

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
	}  GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),)
	{
		/*
		 * This section is used for non-initialized objects that
		 * will not be cleared during the boot process.
		 */
		 *(.noinit)
		 *(".noinit.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-noinit.ld>

	} GROUP_LINK_IN(RAMABLE_REGION)


#include <linker/cplusplus-ram.ld>

	_image_ram_end = .;
	_end = .; /* end of image */

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

	GROUP_END(RAMABLE_REGION)

#include <linker/debug-sections.ld>

	/DISCARD/ : { *(.note.GNU-stack) }

	SECTION_PROLOGUE(.riscv.attributes, 0,)
	{
		KEEP(*(.riscv.attributes))
		KEEP(*(.gnu.attributes))
	}

}
