/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common parts of the linker scripts for the ARCv2/EM targets.
 */

#define _LINKER
#define _ASMLANGUAGE

#include <autoconf.h>
#include <linker/sections.h>

#if defined(CONFIG_UART_NSIM)
	EXTERN(_VectorTable)
#endif

#include <linker/linker-defs.h>
#include <linker/linker-tool.h>

#define KOBJECT_TEXT_AREA 256

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
	#define MPU_MIN_SIZE_ALIGN . = ALIGN(MPU_MIN_SIZE );
#else
	#define MPU_MIN_SIZE_ALIGN
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
	FLASH (rx)  : ORIGIN = FLASH_START, LENGTH = FLASH_SIZE*1k
#endif
#ifdef ICCM_START
	ICCM  (rwx)  : ORIGIN = ICCM_START,  LENGTH = ICCM_SIZE*1k
#endif
#ifdef SRAM_START
	SRAM  (rwx) : ORIGIN = SRAM_START,  LENGTH = SRAM_SIZE*1k
#endif
#ifdef DCCM_START
	DCCM  (rw)  : ORIGIN = DCCM_START,  LENGTH = DCCM_SIZE*1k
#endif
	/* Used by and documented in include/linker/intlist.ld */
	IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

SECTIONS {
	GROUP_START(ROMABLE_REGION)

	SECTION_PROLOGUE(_TEXT_SECTION_NAME,,ALIGN(1024)) {
		_image_rom_start = .;
		_image_text_start = .;

/* when !XIP, .text is in RAM, and vector table must be at its very start */

		KEEP(*(.exc_vector_table))
		KEEP(*(".exc_vector_table.*"))
		KEEP(*(IRQ_VECTOR_TABLE))

#ifdef CONFIG_GEN_SW_ISR_TABLE
		KEEP(*(SW_ISR_TABLE))
#endif
		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)

#include <linker/kobject-text.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_text_end = .;
	_image_rodata_start = .;

#include <linker/common-rom.ld>

	SECTION_PROLOGUE(_RODATA_SECTION_NAME,,) {
		KEEP(*(.openocd_dbg))
		KEEP(*(".openocd_dbg.*"))
		*(.rodata)
		*(".rodata.*")
		*(.gnu.linkonce.r.*)

#ifdef CONFIG_CUSTOM_RODATA_LD
/* Located in project source directory */
#include <custom-rodata.ld>
#endif

#include <linker/kobject-rom.ld>

	} GROUP_LINK_IN(ROMABLE_REGION)

	_image_rodata_end = .;
	MPU_MIN_SIZE_ALIGN
	_image_rom_end = .;
	_image_rom_size = _image_rom_end - _image_rom_start;

	GROUP_END(ROMABLE_REGION)

	GROUP_START(RAMABLE_REGION)

#include <app_data_alignment.ld>
	SECTION_PROLOGUE(_APP_SMEM_SECTION_NAME, (OPTIONAL),)
	{
		MPU_MIN_SIZE_ALIGN
		_image_ram_start = .;
		_app_smem_start = .;
#if defined(CONFIG_MPU_REQUIRES_POWER_OF_TWO_ALIGNMENT)
		#include <app_smem.ld>
#else
		APP_SMEM_SECTION()
#endif
		MPU_MIN_SIZE_ALIGN
		_app_smem_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#ifdef CONFIG_APPLICATION_MEMORY
	SECTION_DATA_PROLOGUE(_APP_DATA_SECTION_NAME, (OPTIONAL),)
	{
		MPU_MIN_SIZE_ALIGN
#include <app_data_alignment.ld>

		__app_ram_start = .;
		__app_data_ram_start = .;
		_image_ram_start = .;
		APP_INPUT_SECTION(.data)
		APP_INPUT_SECTION(".data.*")
		__app_data_ram_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__app_data_rom_start = LOADADDR(_APP_DATA_SECTION_NAME);

	SECTION_PROLOGUE(_APP_BSS_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
		__app_bss_start = .;
		APP_INPUT_SECTION(.bss)
		APP_INPUT_SECTION(".bss.*")
		APP_INPUT_SECTION(COMMON)
		__app_bss_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	__app_bss_num_words = (__app_bss_end - __app_bss_start) >> 2;

	SECTION_PROLOGUE(_APP_NOINIT_SECTION_NAME, (NOLOAD OPTIONAL),)
	{
		APP_INPUT_SECTION(.noinit)
		APP_INPUT_SECTION(".noinit.*")

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	__app_last_address_used = .;

	/* Pad out application ram area to make MPU friendly */
	SECTION_PROLOGUE(app_pad, (NOLOAD OPTIONAL),)
	{
		. = ALIGN(_app_data_align);
		MPU_MIN_SIZE_ALIGN
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)
	__app_ram_end = .;
	__app_ram_size = __app_ram_end - __app_ram_start;
#endif /* CONFIG_APPLICATION_MEMORY */

	SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),) {
		MPU_MIN_SIZE_ALIGN
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		. = ALIGN(4);
		__bss_start = .;

#ifndef CONFIG_APPLICATION_MEMORY
		_image_ram_start = .;
#endif
		__kernel_ram_start = .;
		KERNEL_INPUT_SECTION(.bss)
		KERNEL_INPUT_SECTION(".bss.*")
		KERNEL_INPUT_SECTION(COMMON)
		*(".kernel_bss.*")

		/*
		 * BSP clears this memory in words only and doesn't clear any
		 * potential left over bytes.
		 */
		__bss_end = ALIGN(4);
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

	SECTION_PROLOGUE(_NOINIT_SECTION_NAME,(NOLOAD),) {
		/*
		 * This section is used for non-initialized objects that
		 * will not be cleared during the boot process.
		 */
		 KERNEL_INPUT_SECTION(.noinit)
		 KERNEL_INPUT_SECTION(".noinit.*")
		 *(".kernel_noinit.*")

	} GROUP_LINK_IN(RAMABLE_REGION)

	SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,) {

/* when XIP, .text is in ROM, but vector table must be at start of .data */
		__data_ram_start = .;
		KERNEL_INPUT_SECTION(.data)
		KERNEL_INPUT_SECTION(".data.*")
		*(".kernel.*")

#ifdef CONFIG_CUSTOM_RWDATA_LD
/* Located in project source directory */
#include <custom-rwdata.ld>
#endif

	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	__data_rom_start = LOADADDR(_DATA_SECTION_NAME);

#include <linker/common-ram.ld>
#include <linker/kobject.ld>

	__data_ram_end = .;

	MPU_MIN_SIZE_ALIGN

	/* Define linker symbols */
	_image_ram_end = .;
	_end = .; /* end of image */

	__kernel_ram_end = .;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

	GROUP_END(RAMABLE_REGION)

#ifdef CONFIG_CUSTOM_SECTIONS_LD
/* Located in project source directory */
#include <custom-sections.ld>
#endif

#ifdef CONFIG_GEN_ISR_TABLES
#include <linker/intlist.ld>
#endif
	}
