/* sections.h - Definitions of various linker Sections. */

/*
 * Copyright (c) 2013-2014, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
  DESCRIPTION
  Linker Section declarations used by linker script, C files and Assembly files.
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
#if defined(CONFIG_X86_32)
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

#ifdef CONFIG_SW_ISR_TABLE_DYNAMIC
#define ISR_TABLE_SECTION  DATA
#else  /* !CONFIG_SW_ISR_TABLE_DYNAMIC */
#define ISR_TABLE_SECTION  RODATA
#endif /* CONFIG_SW_ISR_TABLE_DYNAMIC */

#define SECURITY_FRDM_K64F  security_frdm_k64f
#define IRQ_VECTOR_TABLE    irq_vector_table

#if defined(CONFIG_GDB_INFO) && !defined(CONFIG_SW_ISR_TABLE)
#define GDB_STUB_IRQ_VECTOR_TABLE  gdb_stub_irq_vector_table
#endif  /* CONFIG_GDB_INFO && !CONFIG_SW_ISR_TABLE */

#elif defined(CONFIG_ARC)

	#ifdef CONFIG_SW_ISR_TABLE_DYNAMIC
		#define ISR_TABLE_SECTION  DATA
	#else
		#define ISR_TABLE_SECTION  RODATA
	#endif

	#define IRQ_VECTOR_TABLE irq_vector_table

#endif

#include <section_tags.h>

#endif /* _SECTIONS_H */
