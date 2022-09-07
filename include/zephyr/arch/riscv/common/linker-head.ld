/*
 * Copyright (c) 2016-2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Generic linker script header for RISC-V.
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
