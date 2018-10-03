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

/* Interrupts */
#define IRQ_VECTOR_TABLE	.gnu.linkonce.irq_vector_table
#define SW_ISR_TABLE		.gnu.linkonce.sw_isr_table

/* Architecture-specific sections */
#if defined(CONFIG_ARM)
#define KINETIS_FLASH_CONFIG  kinetis_flash_config
#define TI_CCFG	.ti_ccfg

#define _CCM_DATA_SECTION_NAME		ccm_data
#define _CCM_BSS_SECTION_NAME		ccm_bss
#define _CCM_NOINIT_SECTION_NAME	ccm_noinit

#endif

#include <linker/section_tags.h>

#endif /* ZEPHYR_INCLUDE_LINKER_SECTIONS_H_ */
