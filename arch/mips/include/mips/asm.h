/*
 * Copyright 2014-2015, Imagination Technologies Limited and/or its
 *                      affiliated group companies.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
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

#ifndef _MIPS_ASM_H_
#define _MIPS_ASM_H_

/*
 * asm.h: various macros to help assembly language writers
 */

/* ABI specific stack frame layout and manipulation. */
#if _MIPS_SIM == _ABIO32
/* Standard O32 */
#define SZREG		4	/* saved register size */
#define REG_S		sw	/* store saved register */
#define REG_L		lw	/* load saved register */
#define SZPTR		4	/* pointer size */
#define PTR_S		sw	/* store pointer */
#define PTR_L		lw	/* load pointer */
#define PTR_MFC0	mfc0	/* access CP0 pointer width register */
#define PTR_MTC0	mtc0	/* access CP0 pointer width register */
#define LA		la	/* load an address */
#define PTR		.word	/* pointer type pseudo */
#else
#error Unknown ABI
#endif

#ifdef __ASSEMBLER__

/* Name of standard code section. */
#ifndef _NORMAL_SECTION_UNNAMED
# define _NORMAL_SECTION_UNNAMED .section .text, "ax", @progbits
#endif

#ifndef _NORMAL_SECTION_NAMED
#  define _NORMAL_SECTION_NAMED(name) .pushsection .text, "ax", @progbits
#endif

/* Default code section. */
#ifndef _TEXT_SECTION_NAMED
#  define _TEXT_SECTION_NAMED _NORMAL_SECTION_NAMED
#endif

/*
 * Leaf functions declarations.
 */

/* Global leaf function. */
#define LEAF(name)			\
	_TEXT_SECTION_NAMED(name);	\
	.balign	4;			\
	.globl	name;			\
	.ent	name;			\
name:

/*
 * Function termination
 */
#define END(name)			\
	.size name, .-name;		\
	.end	name;			\
	.popsection

/*
 * Global data declaration.
 */
#define EXPORT(name) \
	.globl name; \
	.type name, @object; \
name:

#endif /* __ASSEMBLER__ */

#endif /*_MIPS_ASM_H_*/
