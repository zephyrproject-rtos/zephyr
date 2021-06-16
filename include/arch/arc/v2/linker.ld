/*
 * Copyright (c) 2020 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common parts of the linker scripts for the ARCv2 targets for
 * GNU and MWDT toolchains.
 */

#include <linker/sections.h>
#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

/* physical address of RAM */
#ifdef CONFIG_HARVARD
	#define ROMABLE_REGION ICCM
	#define RAMABLE_REGION DCCM
#else
	#if defined(CONFIG_XIP) && (FLASH_SIZE != 0)
		#define ROMABLE_REGION FLASH
		#define RAMABLE_REGION SRAM
	#else
		#define ROMABLE_REGION SRAM
		#define RAMABLE_REGION SRAM
	#endif
#endif

#ifdef CONFIG_ARC_MPU_ENABLE
	#if CONFIG_ARC_MPU_VER == 2
		#define MPU_MIN_SIZE 2048
	#elif CONFIG_ARC_MPU_VER == 4
		#define MPU_MIN_SIZE 32
	#endif
	#define MPU_MIN_SIZE_ALIGN . = ALIGN(MPU_MIN_SIZE);
	#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
		#define MPU_ALIGN(region_size) \
			. = ALIGN(MPU_MIN_SIZE); \
			. = ALIGN( 1 << LOG2CEIL(region_size))
	#else
		#define MPU_ALIGN(region_size) \
			. = ALIGN(MPU_MIN_SIZE)
	#endif
#else
	#define MPU_MIN_SIZE_ALIGN
	#define MPU_ALIGN(region_size) . = ALIGN(4)
#endif

#if defined(CONFIG_XIP)
	#define _DATA_IN_ROM __data_rom_start
#else
	#define _DATA_IN_ROM
#endif

OUTPUT_ARCH(arc)
ENTRY(CONFIG_KERNEL_ENTRY)

MEMORY {
#ifdef FLASH_START
	FLASH (rx)  : ORIGIN = FLASH_START, LENGTH = FLASH_SIZE
#endif
#ifdef ICCM_START
	ICCM  (rwx)  : ORIGIN = ICCM_START,  LENGTH = ICCM_SIZE
#endif
#ifdef SRAM_START
	SRAM  (rwx) : ORIGIN = SRAM_START,  LENGTH = SRAM_SIZE
#endif
#ifdef DCCM_START
	DCCM  (rw)  : ORIGIN = DCCM_START,  LENGTH = DCCM_SIZE
#endif
	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

SECTIONS {

#include <linker/rel-sections.ld>

	GROUP_START(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,ALIGN(1024)) {
		_image_rom_start = .;
		_image_text_start = .;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rom-start.ld>
		*(.text .text.*)
		*(.gnu.linkonce.t.*)

	. = ALIGN(4);
#include <linker/kobject-text.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;
	_image_rodata_start = .;

#include <linker/common-rom.ld>
#include <linker/thread-local-storage.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,) {
		*(".rodata")
		*(".rodata.*")
		*(.gnu.linkonce.r.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <linker/kobject-rom.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

#ifdef __MWDT_LINKER_CMD__
/* TODO: add mwdt specific ROM C++ sections */
#else
#include <linker/cplusplus-rom.ld>
#endif /* __MWDT_LINKER_CMD__ */

	_image_rodata_end = .;
	MPU_ALIGN(_image_rodata_end - _image_rom_start);
	_image_rom_end = .;
	_image_rom_size = _image_rom_end - _image_rom_start;

	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

#include <app_data_alignment.ld>

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#if defined(CONFIG_USERSPACE)
#define APP_SHARED_ALIGN MPU_MIN_SIZE_ALIGN
#define SMEM_PARTITION_ALIGN MPU_ALIGN

#include <app_smem.ld>

	_image_ram_start = _app_smem_start;
	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#endif /* CONFIG_USERSPACE */

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),) {
		MPU_MIN_SIZE_ALIGN
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		. = ALIGN(4);
		__bss_start = .;
		_image_ram_start = .;
		__kernel_ram_start = .;
		*(".bss")
		*(".bss.*")
		*(COMMON)
		*(".kernel_bss.*")

		/*
		 * BSP clears this memory in words only and doesn't clear any
		 * potential left over bytes.
		 */
		__bss_end = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <linker/common-noinit.ld>

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,) {

/* when XIP, .text is in ROM, but vector table must be at start of .data */
		__data_ram_start = .;
		*(".data")
		*(".data.*")
		*(".kernel.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__data_rom_start = LOADADDR(_DATA_SECTION_NAME);

#include <linker/common-ram.ld>
#include <linker/kobject-data.ld>

#ifdef __MWDT_LINKER_CMD__
/* TODO: add mwdt specific RAM C++ sections */
#else
#include <linker/cplusplus-ram.ld>
#endif /* __MWDT_LINKER_CMD__ */

	__data_ram_end = .;
	MPU_MIN_SIZE_ALIGN
	/* Define linker symbols */
	_image_ram_end = .;
	_end = .; /* end of image */

	__kernel_ram_end = .;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

	GROUP_END(RAMABLE_REGION)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#include <linker/debug-sections.ld>

	SECTION_PROLOGUE(.ARC.attributes, 0,) {
		KEEP(*(.ARC.attributes))
		KEEP(*(.gnu.attributes))
	}

	/DISCARD/ : {
		*(.note.GNU-stack)
		*(.got.plt)
		*(.igot.plt)
		*(.got)
		*(.igot)
	}
}
