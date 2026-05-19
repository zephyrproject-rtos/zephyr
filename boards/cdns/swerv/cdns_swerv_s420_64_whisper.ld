/*
 * Copyright (c) 2016-2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Linker command/script file for cdns/swerv/s420/whisper
 */

#include <zephyr/devicetree.h>

#include <zephyr/linker/sections.h>
#include <zephyr/linker/devicetree_regions.h>

#include <zephyr/linker/linker-defs.h>
#include <zephyr/linker/linker-tool.h>

#define ROMABLE_REGION              ICCM
#define RAMABLE_REGION              DCCM

#define _EXCEPTION_SECTION_NAME     exceptions
#define _RESET_SECTION_NAME         reset

#define ICCM_BASE (DT_REG_ADDR(DT_CHOSEN(zephyr_iccm)))
#define ICCM_SIZE (DT_REG_SIZE(DT_CHOSEN(zephyr_iccm)))

#define DCCM_BASE (DT_REG_ADDR(DT_CHOSEN(zephyr_dccm)))
#define DCCM_SIZE (DT_REG_SIZE(DT_CHOSEN(zephyr_dccm)))

#define ROM_BASE DCCM_BASE
#define ROM_SIZE DCCM_SIZE

#define RAM_BASE DCCM_BASE
#define RAM_SIZE DCCM_SIZE

#ifdef CONFIG_RISCV_PMP
	#define MPU_MIN_SIZE CONFIG_PMP_GRANULARITY
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
    ICCM (rx) : ORIGIN = ICCM_BASE, LENGTH = ICCM_SIZE

    DCCM (rwx) : ORIGIN = DCCM_BASE, LENGTH = DCCM_SIZE

    LINKER_DT_REGIONS()

    /* Used by and documented in include/linker/intlist.ld */
    IDT_LIST  (wx)      : ORIGIN = 0xFFFFF7FF, LENGTH = 2K
}

ENTRY(CONFIG_KERNEL_ENTRY)

SECTIONS
    {

#include <zephyr/linker/rel-sections.ld>

#ifdef CONFIG_LLEXT
#include <zephyr/linker/llext-sections.ld>
#endif

    /*
     * The .plt and .iplt are here according to
     * 'riscv64-zephyr-elf-ld --verbose', before text section.
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
/* Located in generated directory. This file is populated by calling
 * zephyr_linker_sources(ROM_SECTIONS ...). Useful for grouping iterable RO structs.
 */
#include <snippets-rom-sections.ld>
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

#ifdef CONFIG_RISCV_PMP
	SECTION_PROLOGUE(rom_mpu_padding,,)
	{
		MPU_ALIGN(__rodata_region_end - __rom_region_start);
	} GROUP_LINK_IN(ROMABLE_REGION)
#endif /* CONFIG_RISCV_PMP */

	__rom_region_end = .;
	__rom_region_size = __rom_region_end - __rom_region_start;

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

	__kernel_ram_end = .;
	__kernel_ram_size = __kernel_ram_end - __kernel_ram_start;

/* Located in generated directory. This file is populated by the
 * zephyr_linker_sources() Cmake function.
 */
#include <snippets-sections.ld>

#define LAST_RAM_ALIGN MPU_MIN_SIZE_ALIGN

#include <zephyr/linker/ram-end.ld>

    GROUP_END(RAMABLE_REGION)

#include <zephyr/linker/debug-sections.ld>

    /DISCARD/ : { *(.note.GNU-stack) }

    SECTION_PROLOGUE(.riscv.attributes, 0,)
	{
	KEEP(*(.riscv.attributes))
	KEEP(*(.gnu.attributes))
	}

/* Output section descriptions are needed for these sections to suppress
 * warnings when "--orphan-handling=warn" is set for lld.
 */
#if defined(CONFIG_LLVM_USE_LLD)
    SECTION_PROLOGUE(.symtab, 0,) { *(.symtab) }
    SECTION_PROLOGUE(.strtab, 0,) { *(.strtab) }
    SECTION_PROLOGUE(.shstrtab, 0,) { *(.shstrtab) }
#endif

    /* Sections generated from 'zephyr,memory-region' nodes */
    LINKER_DT_SECTIONS()
}
