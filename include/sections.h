/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

#define SECURITY_FRDM_K64F  security_frdm_k64f
#define IRQ_VECTOR_TABLE    irq_vector_table

#if defined(CONFIG_GDB_INFO) && !defined(CONFIG_SW_ISR_TABLE)
#define GDB_STUB_IRQ_VECTOR_TABLE  gdb_stub_irq_vector_table
#endif  /* CONFIG_GDB_INFO && !CONFIG_SW_ISR_TABLE */

#elif defined(CONFIG_ARC)

	#define IRQ_VECTOR_TABLE irq_vector_table

#endif

#include <section_tags.h>

#endif /* _SECTIONS_H */
