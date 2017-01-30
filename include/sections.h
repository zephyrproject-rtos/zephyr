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

#ifndef _SECTIONS_H
#define _SECTIONS_H

#define _TEXT_SECTION_NAME text
#define _RODATA_SECTION_NAME rodata
#define _CTOR_SECTION_NAME ctors
/* Linker issue with XIP where the name "data" cannot be used */
#define _DATA_SECTION_NAME datas
#define _BSS_SECTION_NAME bss
#define _NOINIT_SECTION_NAME noinit

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

#if defined(CONFIG_ARM)
#define SCS_SECTION scs
#define SCP_SECTION scp

#define KINETIS_FLASH_CONFIG  kinetis_flash_config
#define IRQ_VECTOR_TABLE    irq_vector_table

#elif defined(CONFIG_ARC)

	#define IRQ_VECTOR_TABLE irq_vector_table

#endif

#include <section_tags.h>

#endif /* _SECTIONS_H */
