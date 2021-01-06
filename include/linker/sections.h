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
#define _IRQ_VECTOR_TABLE_SECTION_NAME	.gnu.linkonce.irq_vector_table*
#define _SW_ISR_TABLE_SECTION_NAME	.gnu.linkonce.sw_isr_table*

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

#ifdef CONFIG_NOCACHE_MEMORY
#define _NOCACHE_SECTION_NAME nocache
#endif

/* Short section references for use in ASM files */
#if defined(_ASMLANGUAGE)
/* Various text section names */
#define TEXT text
#if defined(CONFIG_X86)
#define TEXT_START text_start /* beginning of TEXT section */
#else
#define TEXT_START text /* beginning of TEXT section */
#endif

/* Various data type section names */
#define BSS bss
#define RODATA rodata
#define DATA data
#define NOINIT noinit
#endif /* _ASMLANGUAGE */

#include <linker/section_tags.h>

#endif /* ZEPHYR_INCLUDE_LINKER_SECTIONS_H_ */
