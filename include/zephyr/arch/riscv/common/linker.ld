/*
 * Copyright (c) 2016-2017 Jean-Paul Etienne <fractalclone@gmail.com>
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
#include <zephyr/devicetree.h>

#include <zephyr/linker/sections.h>
#include <zephyr/linker/devicetree_regions.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#ifdef CONFIG_XIP
#define ROMABLE_REGION              ROM
#else
#define ROMABLE_REGION              RAM
#endif
#define RAMABLE_REGION              RAM

#define _EXCEPTION_SECTION_NAME     exceptions
#define _RESET_SECTION_NAME         reset

#ifdef CONFIG_XIP
#if DT_NODE_HAS_COMPAT_STATUS(DT_CHOSEN(zephyr_flash), soc_nv_flash, okay)
#ifdef CONFIG_FLASH_LOAD_OFFSET
#define ROM_BASE (DT_REG_ADDR(DT_CHOSEN(zephyr_flash)) + \
				CONFIG_FLASH_LOAD_OFFSET)
#else /* !CONFIG_FLASH_LOAD_OFFSET */
#define ROM_BASE DT_REG_ADDR(DT_CHOSEN(zephyr_flash))
#endif /* CONFIG_FLASH_LOAD_OFFSET */
#define ROM_SIZE DT_REG_SIZE(DT_CHOSEN(zephyr_flash))
#elif DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_flash), jedec_spi_nor)
/* For jedec,spi-nor we expect the spi controller to memory map the flash
 * and for that mapping to be the second register property of the spi
 * controller.
 */
#define SPI_CTRL DT_PARENT(DT_CHOSEN(zephyr_flash))
#define ROM_BASE DT_REG_ADDR_BY_IDX(SPI_CTRL, 1)
#define ROM_SIZE DT_REG_SIZE_BY_IDX(SPI_CTRL, 1)
#endif
#else /* CONFIG_XIP */
#define ROM_BASE CONFIG_SRAM_BASE_ADDRESS
#define ROM_SIZE KB(CONFIG_SRAM_SIZE)
#endif /* CONFIG_XIP */

#define RAM_BASE CONFIG_SRAM_BASE_ADDRESS
#define RAM_SIZE KB(CONFIG_SRAM_SIZE)

#ifdef CONFIG_RISCV_PMP
	#define MPU_MIN_SIZE 4
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

MEMORY
{
#ifdef CONFIG_XIP
    ROM (rx)  : ORIGIN = ROM_BASE, LENGTH = ROM_SIZE
#endif
    RAM (rwx) : ORIGIN = RAM_BASE, LENGTH = RAM_SIZE

    LINKER_DT_REGIONS()

    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
    {

#include <zephyr/linker/rel-sections.ld>

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
    __rom_region_start = ROM_BASE;

    SECTION_PROLOGUE(rom_start,,)
    {
		. = ALIGN(16);
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_START ...).
 */
#include <snippets-rom-start.ld>
    } GROUP_LINK_IN(ROMABLE_REGION)

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_relocate.ld>
#endif

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

		__text_region_start = .;

		*(.text)
		*(".text.*")
		*(.gnu.linkonce.t.*)
#include <zephyr/linker/kobject-text.ld>
	} GROUP_LINK_IN(ROMABLE_REGION)

    __text_region_end = .;

	__rodata_region_start = .;
#include <zephyr/linker/common-rom.ld>
#include <zephyr/linker/thread-local-storage.ld>

    SECTION_PROLOGUE(_RODATA_SECTION_NAME,,)
	{
		 . = ALIGN(4);
		 *(.srodata)
		 *(".srodata.*")
		 *(.rodata)
		 *(".rodata.*")
		 *(.gnu.linkonce.r.*)
		 *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rodata.ld>
#include <zephyr/linker/kobject-rom.ld>
	. = ALIGN(4);
	} GROUP_LINK_IN(ROMABLE_REGION)

#include <zephyr/linker/cplusplus-rom.ld>
	__rodata_region_end = .;

	/* For non-XIP system, __rom_region_end symbol should be set to
	 * the end of common ROMABLE_REGIONs (text and rodata) instead of
	 * the linker script end, so it wouldn't mistakenly contain
	 * RAMABLE_REGION in it.
	 */
#ifndef CONFIG_XIP
#ifdef CONFIG_RISCV_PMP
	SECTION_PROLOGUE(rom_mpu_padding,,)
	{
		MPU_ALIGN(__rodata_region_end - __rom_region_start);
#ifdef CONFIG_QEMU_TARGET
		/*
		 * QEMU doesn't vet each instruction fetch individually.
		 * Instead, it grabs a whole page and perform dynamic
		 * transation on it in a batch. It therefore validates
		 * PMP permissions using page-sized and -aligned chunks.
		 */
		. = ALIGN(0x1000);
#endif
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif /* CONFIG_RISCV_PMP */

	__rom_region_end = .;
	__rom_region_size = __rom_region_end - __rom_region_start;
#endif /* CONFIG_XIP */
    GROUP_END(ROMABLE_REGION)

    GROUP_START(RAMABLE_REGION)

	. = RAM_BASE;
	_image_ram_start = .;
/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-ram-sections.ld>

#if defined(CONFIG_USERSPACE)
#define APP_SHARED_ALIGN MPU_MIN_SIZE_ALIGN
#define SMEM_PARTITION_ALIGN MPU_ALIGN

#include <app_smem.ld>

	_app_smem_size = _app_smem_end - _app_smem_start;
	_app_smem_rom_start = LOADADDR(_APP_SMEM_SECTION_NAME);
#endif /* CONFIG_USERSPACE */

    SECTION_DATA_PROLOGUE(_BSS_SECTION_NAME,(NOLOAD),)
	{
		MPU_MIN_SIZE_ALIGN
		/*
		 * For performance, BSS section is assumed to be 4 byte aligned and
		 * a multiple of 4 bytes
		 */
		 . = ALIGN(4);
		 __bss_start = .;
		__kernel_ram_start = .;
		 *(.sbss)
		 *(".sbss.*")
		 *(.bss)
		 *(".bss.*")
		 COMMON_SYMBOLS

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_bss_relocate.ld>
#endif

		 /*
		  * As memory is cleared in words only, it is simpler to ensure the BSS
		  * section ends on a 4 byte boundary. This wastes a maximum of 3 bytes.
		  */
		  __bss_end = ALIGN(4);
	}  GROUP_DATA_LINK_IN(RAMABLE_REGION, RAMABLE_REGION)

#include <zephyr/linker/common-noinit.ld>

    SECTION_DATA_PROLOGUE(_DATA_SECTION_NAME,,)
	{
		 . = ALIGN(4);
		/* _image_ram_start = .; */
		 __data_region_start = .;
		 __data_start = .;

		 *(.data)
		 *(".data.*")

#ifdef CONFIG_RISCV_GP
		/*
		 * RISC-V architecture has 12-bit signed immediate offsets in the
		 * instructions. If we can put the most commonly accessed globals
		 * in a special 4K span of memory addressed by the GP register, then
		 * we can access those values in a single instruction, saving both
		 * codespace and runtime.
		 *
		 * Since these immediate offsets are signed, place gp 0x800 past the
		 * beginning of .sdata so that we can use both positive and negative
		 * offsets.
		 */
		 . = ALIGN(8);
		 PROVIDE (__global_pointer$ = . + 0x800);
#endif

		 *(.sdata .sdata.* .gnu.linkonce.s.*)

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-rwdata.ld>

#ifdef CONFIG_CODE_DATA_RELOCATION
#include <linker_sram_data_relocate.ld>
#endif

		 __data_end = .;

	}  GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
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

    MPU_MIN_SIZE_ALIGN

     _image_ram_end = .;
     _end = .; /* end of image */

	__kernel_ram_end = .;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_itcm), okay)
GROUP_START(ITCM)

	SECTION_PROLOGUE(_ITCM_SECTION_NAME,,SUBALIGN(8))
	{
		__itcm_start = .;
		*(.itcm)
		*(".itcm.*")
		__itcm_end = .;
	} GROUP_LINK_IN(ITCM AT> ROMABLE_REGION)

	__itcm_size = __itcm_end - __itcm_start;
	__itcm_load_start = LOADADDR(_ITCM_SECTION_NAME);

GROUP_END(ITCM)
#endif

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
GROUP_START(DTCM)

	SECTION_PROLOGUE(_DTCM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(8))
	{
		__dtcm_start = .;
		__dtcm_bss_start = .;
		*(.dtcm_bss)
		*(".dtcm_bss.*")
		__dtcm_bss_end = .;
	} GROUP_LINK_IN(DTCM)

	SECTION_PROLOGUE(_DTCM_NOINIT_SECTION_NAME, (NOLOAD),SUBALIGN(8))
	{
		__dtcm_noinit_start = .;
		*(.dtcm_noinit)
		*(".dtcm_noinit.*")
		__dtcm_noinit_end = .;
	} GROUP_LINK_IN(DTCM)

	SECTION_PROLOGUE(_DTCM_DATA_SECTION_NAME,,SUBALIGN(8))
	{
		__dtcm_data_start = .;
		*(.dtcm_data)
		*(".dtcm_data.*")
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

     GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

    /DISCARD/ : { *(.note.GNU-stack) }

    SECTION_PROLOGUE(.riscv.attributes, 0,)
	{
	KEEP(*(.riscv.attributes))
	KEEP(*(.gnu.attributes))
	}

    /* Sections generated from 'zephyr,memory-region' nodes */
    LINKER_DT_SECTIONS()

/* Because ROMABLE_REGION != RAMABLE_REGION in XIP-system, it is valid
 * to set __rom_region_end symbol at the end of linker script and
 * doesn't mistakenly contain the RAMABLE_REGION in it.
 */
#ifdef CONFIG_XIP
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
__rom_region_end = LOADADDR(.last_section) + SIZEOF(.last_section);
__rom_region_size = __rom_region_end - __rom_region_start;
#endif

}
