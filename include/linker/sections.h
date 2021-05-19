/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Definitions of various linker Sections.
 *
 * Linker Section declarations used by linker script, C files and Assembly
 * files.
 */

#ifndef ZEPHYR_INCLUDE_LINKER_SECTIONS_H_
#define ZEPHYR_INCLUDE_LINKER_SECTIONS_H_

#define _TEXT_SECTION_NAME text
#define _RODATA_SECTION_NAME rodata
#define _CTOR_SECTION_NAME ctors
/* Linker issue with XIP where the name "data" cannot be used */
#define _DATA_SECTION_NAME datas
#define _BSS_SECTION_NAME bss
#define _NOINIT_SECTION_NAME noinit

#define _APP_SMEM_SECTION_NAME		app_smem
#define _APP_DATA_SECTION_NAME		app_datas
#define _APP_BSS_SECTION_NAME		app_bss
#define _APP_NOINIT_SECTION_NAME	app_noinit

#define _UNDEFINED_SECTION_NAME undefined

/* Interrupts */
#define _IRQ_VECTOR_TABLE_SECTION_NAME	.gnu.linkonce.irq_vector_table
#define _IRQ_VECTOR_TABLE_SECTION_SYMS	.gnu.linkonce.irq_vector_table*

#define _SW_ISR_TABLE_SECTION_NAME	.gnu.linkonce.sw_isr_table
#define _SW_ISR_TABLE_SECTION_SYMS	.gnu.linkonce.sw_isr_table*

/* Architecture-specific sections */
#if defined(CONFIG_ARM)
#define _KINETIS_FLASH_CONFIG_SECTION_NAME  kinetis_flash_config
#define _TI_CCFG_SECTION_NAME	.ti_ccfg

#define _CCM_DATA_SECTION_NAME		.ccm_data
#define _CCM_BSS_SECTION_NAME		.ccm_bss
#define _CCM_NOINIT_SECTION_NAME	.ccm_noinit

#define _ITCM_SECTION_NAME		.itcm

#define _DTCM_DATA_SECTION_NAME	.dtcm_data
#define _DTCM_BSS_SECTION_NAME		.dtcm_bss
#define _DTCM_NOINIT_SECTION_NAME	.dtcm_noinit
#endif

#define _IMX_BOOT_CONF_SECTION_NAME	.boot_hdr.conf
#define _IMX_BOOT_DATA_SECTION_NAME	.boot_hdr.data
#define _IMX_BOOT_IVT_SECTION_NAME	.boot_hdr.ivt
#define _IMX_BOOT_DCD_SECTION_NAME	.boot_hdr.dcd_data

#define _STM32_SDRAM1_SECTION_NAME	.stm32_sdram1
#define _STM32_SDRAM2_SECTION_NAME	.stm32_sdram2

#define _STM32_BACKUP_SRAM_SECTION_NAME	.stm32_backup_sram

#ifdef CONFIG_NOCACHE_MEMORY
#define _NOCACHE_SECTION_NAME nocache
#endif

#if defined(CONFIG_LINKER_USE_BOOT_SECTION)
#define BOOT_TEXT_SECTION_NAME		boot_text
#define BOOT_BSS_SECTION_NAME		boot_bss
#define BOOT_RODATA_SECTION_NAME	boot_rodata
#define BOOT_DATA_SECTION_NAME		boot_data
#define BOOT_NOINIT_SECTION_NAME	boot_noinit
#endif

#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
#define PINNED_TEXT_SECTION_NAME	pinned_text
#define PINNED_BSS_SECTION_NAME		pinned_bss
#define PINNED_RODATA_SECTION_NAME	pinned_rodata
#define PINNED_DATA_SECTION_NAME	pinned_data
#define PINNED_NOINIT_SECTION_NAME	pinned_noinit
#endif

/* Short section references for use in ASM files */
#if defined(_ASMLANGUAGE)
/* Various text section names */
#define TEXT text

/* Various data type section names */
#define BSS bss
#define RODATA rodata
#define DATA data
#define NOINIT noinit

#if defined(CONFIG_LINKER_USE_BOOT_SECTION)
#define BOOT_TEXT			BOOT_TEXT_SECTION_NAME
#define BOOT_BSS			BOOT_BSS_SECTION_NAME
#define BOOT_RODATA			BOOT_RODATA_SECTION_NAME
#define BOOT_DATA			BOOT_DATA_SECTION_NAME
#define BOOT_NOINIT			BOOT_NOINIT_SECTION_NAME
#else
#define BOOT_TEXT			TEXT
#define BOOT_BSS			BSS
#define BOOT_RODATA			RODATA
#define BOOT_DATA			DATA
#define BOOT_NOINIT			NOINIT
#endif /* CONFIG_LINKER_USE_BOOT_SECTION */

#if defined(CONFIG_LINKER_USE_PINNED_SECTION)
#define PINNED_TEXT			PINNED_TEXT_SECTION_NAME
#define PINNED_BSS			PINNED_BSS_SECTION_NAME
#define PINNED_RODATA			PINNED_RODATA_SECTION_NAME
#define PINNED_DATA			PINNED_DATA_SECTION_NAME
#define PINNED_NOINIT			PINNED_NOINIT_SECTION_NAME
#else
#define PINNED_TEXT			TEXT
#define PINNED_BSS			BSS
#define PINNED_RODATA			RODATA
#define PINNED_DATA			DATA
#define PINNED_NOINIT			NOINIT
#endif /* CONFIG_LINKER_USE_PINNED_SECTION */

#endif /* _ASMLANGUAGE */

#include <linker/section_tags.h>

#endif /* ZEPHYR_INCLUDE_LINKER_SECTIONS_H_ */
