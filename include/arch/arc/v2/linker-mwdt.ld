/*
 * Copyright (c) 2020 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common parts of the linker scripts for the ARCv2/EM targets.
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
	#elif CONFIG_ARC_MPU_VER == 3
		#define MPU_MIN_SIZE 32
	#endif
	#define MPU_MIN_SIZE_ALIGN . = ALIGN(MPU_MIN_SIZE);
	#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
		#define MPU_REGION_SIZE(region_size) \
			(1 << LOG2CEIL(region_size))
	#else
		#define MPU_REGION_SIZE(region_size) \
			(region_size)
	#endif
#else
	#define MPU_MIN_SIZE_ALIGN
	#define MPU_REGION_SIZE(region_size) \
			(region_size)
#endif

#if defined(CONFIG_XIP)
	#define _DATA_IN_ROM __data_rom_start
#else
	#define _DATA_IN_ROM
#endif

ENTRY(CONFIG_KERNEL_ENTRY)

MEMORY {
#ifdef FLASH_START
	FLASH : ORIGIN = FLASH_START, LENGTH = FLASH_SIZE
#endif
#ifdef ICCM_START
	ICCM  : ORIGIN = ICCM_START,  LENGTH = ICCM_SIZE
#endif
#ifdef SRAM_START
	SRAM  : ORIGIN = SRAM_START,  LENGTH = SRAM_SIZE
#endif
#ifdef DCCM_START
	DCCM  : ORIGIN = DCCM_START,  LENGTH = DCCM_SIZE
#endif
	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST : ORIGIN = 0xFFFFF800, LENGTH = 2K
}

SECTIONS {

	GROUP_START(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,ALIGN(1024)) {
		_image_rom_start = .;
		_image_text_start = .;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rom-start.ld>
		*(.text .text* .text.*)

#ifdef CONFIG_USERSPACE
/* to make sure .kobject_data.text occupies a fixed hole, we use the trick
 * of ALIGN here with a cost of memfoort print. If assigning program counter
 * with an identifier is supported, this trick can be removed
 */
	. = ALIGN(CONFIG_KOBJECT_TEXT_AREA);
	_kobject_text_area_start = .;
	*(".kobject_data.text*")
	_kobject_text_area_end = .;
	_kobject_text_area_used = _kobject_text_area_end - _kobject_text_area_start;
#ifndef LINKER_PASS2
	. = ALIGN(4);
#ifdef CONFIG_DYNAMIC_OBJECTS
	PROVIDE(z_object_gperf_find = .);
	PROVIDE(z_object_gperf_wordlist_foreach = .);
#else
	PROVIDE(z_object_find = .);
	PROVIDE(z_object_wordlist_foreach = .);
#endif
	. = . + CONFIG_KOBJECT_TEXT_AREA;
#endif
	. = ALIGN(CONFIG_KOBJECT_TEXT_AREA);
#endif /* CONFIG_USERSPACE */


		_image_text_end = .;
		_image_rodata_start = .;

	} GROUP_LINK_IN(ROMABLE_REGION)

#include <linker/common-rom.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,) {
		KEEP(*(.openocd_dbg))
		KEEP(*(".openocd_dbg.*"))
		*(".rodata")
		*(".rodata.*")

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>

#include <linker/kobject-rom.ld>

#if defined (CONFIG_CPLUSPLUS)
/* \todo: add mwdt c++ sections */
#endif

		_image_rodata_end = .;
		MPU_MIN_SIZE_ALIGN
		_image_rom_end =  _image_rom_start + MPU_REGION_SIZE(. - _image_rom_start);
		_image_rom_size = _image_rom_end - _image_rom_start;

	} GROUP_LINK_IN(ROMABLE_REGION)

	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)


/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#if defined(CONFIG_USERSPACE)
/*
 * because the linker of metware toolchain is strong as gcc's linker,
 * e.g., the limitation on . operator, no LOG2CEIL, so for MPU wiht power of 2
 * requirement, the alignment of app smem is fixed to MPU_MIN_SIZE which
 * is not a correct value for partition with size > MPU_MIN_SIZE.
 */
#define APP_SHARED_ALIGN . = ALIGN(MPU_MIN_SIZE)
#define SMEM_PARTITION_ALIGN(x) . = ALIGN(MPU_MIN_SIZE)

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
#include <linker/kobject.ld>
#if defined (CONFIG_CPLUSPLUS)
/* \todo add mwdt specific c++ sections */
#endif

/* an empty region to hold symbols, mwdt does not support of setting .
 * outside sections
 */
	SECTION_DATA_PROLOGUE(_mwdt_ram_end,,) {
	__data_ram_end = .;
	MPU_MIN_SIZE_ALIGN
	/* Define linker symbols */
	_image_ram_end = .;
	_end = .; /* end of image */

	__kernel_ram_end = .;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;
/* mwdt requires _fstack, _estack which will be used in _stkchk.
 * _stkchk is inserted by mwdt automatically, if _fstack, _estack is not
 * set correctly, the  brk_s instruction will be called
 * here, we use a trick to deceive the compiler.
 */
	_fstack = _image_ram_start;
	_estack = .;

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	GROUP_END(RAMABLE_REGION)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

	}
