/* toolchain/gcc.h - GCC toolchain abstraction */

/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
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
  Macros to abstract compiler capabilities for GCC toolchain.

  \NOMANUAL
*/

#include <toolchain/common.h>

#define FUNC_ALIAS(func, aliasToFunc, retType)			\
	retType aliasToFunc() __attribute__((alias(#func)))

#define _ALIAS_OF(of) __attribute__((alias(#of)))

#define __prekernel_init_level(level) __attribute__((section(".ctors." \
				_STRINGIFY(level))))

#define CODE_UNREACHABLE __builtin_unreachable()
#define FUNC_NORETURN    __attribute__((__noreturn__))

/* The GNU assembler for Cortex-M3 uses # for immediate values, not
 * comments, so the @nobits# trick does not work.
 */
#if defined(VXMICRO_ARCH_arm)
#define _NODATA_SECTION(segment)  __attribute__((section(#segment)))
#else
#define _NODATA_SECTION(segment)				\
	__attribute__((section(#segment ",\"wa\",@nobits#")))
#endif

/*
 * Unaligned reads and writes
 *
 * To prevent GCC from generating a "breaking strict-aliasing rule" warning,
 * use the __may_alias__ attribute to inform it that the pointer may alias
 * another type.
 */

#ifdef CONFIG_UNALIGNED_WRITE_UNSUPPORTED
#define UNALIGNED_READ(p)    _Unaligned32Read((p))

#define UNALIGNED_WRITE(p, v)						\
	do  {								\
		unsigned int __attribute__((__may_alias__)) *pp = (unsigned int *)(p); \
		_Unaligned32Write(pp, (v));				\
	}								\
	while (0)
#else  /* !CONFIG_UNALIGNED_WRITE_UNSUPPORTED */
#define UNALIGNED_READ(p) (*(p))

#define UNALIGNED_WRITE(p, v)						\
	do  {								\
		unsigned int __attribute__((__may_alias__)) *pp = (unsigned int *)(p); \
		*pp = (v);						\
	}								\
	while (0)
#endif /* !CONFIG_UNALIGNED_WRITE_UNSUPPORTED */

/* Unaligned access */
#define UNALIGNED_GET(p)						\
__extension__ ({							\
	struct  __attribute__((__packed__)) {				\
		__typeof__(*(p)) __v;					\
	} *__p = (__typeof__(__p)) (p);					\
	__p->__v;							\
})

#define _GENERIC_SECTION(segment) __attribute__((section(#segment)))

/*
 * Do not use either PACK_STRUCT or ALIGN_STRUCT(x).
 * Use __packed or __aligned(x) instead.
 */

#define PACK_STRUCT     __attribute__((__packed__))
#define ALIGN_STRUCT(x) __attribute__((aligned(x)))

#define __packed        __attribute__((__packed__))
#define __aligned(x)    __attribute__((aligned(x)))

#define ARG_UNUSED(x) (void)(x)

#define likely(x)   __builtin_expect((long)!!(x), 1L)
#define unlikely(x) __builtin_expect((long)!!(x), 0L)

/* These macros allow having ARM asm functions callable from thumb */

#if defined(_ASMLANGUAGE) && !defined(_LINKER)

#ifdef VXMICRO_ARCH_arm

#if defined(CONFIG_ISA_THUMB)

#define FUNC_CODE()				\
	.code 16;				\
	.thumb_func;

#define FUNC_INSTR(a)				\
	BX pc;					\
	NOP;					\
	.code 32;				\
A##a:

#elif defined(CONFIG_ISA_THUMB2)

#define FUNC_CODE() .thumb;
#define FUNC_INSTR(a)

#elif defined(CONFIG_ISA_ARM)

#define FUNC_CODE() .code 32;
#define FUNC_INSTR(a)

#else

#error unknown instruction set

#endif /* ISA */

#else

#define FUNC_CODE()
#define FUNC_INSTR(a)

#endif /* !VXMICRO_ARCH_arm */

#endif /* _ASMLANGUAGE && !_LINKER */

/*
 * These macros are used to declare assembly language symbols that need
 * to be typed properly(func or data) to be visible to the OMF tool.
 * So that the build tool could mark them as an entry point to be linked
 * correctly.  This is an elfism. Use #if 0 for a.out.
 */

#if defined(_ASMLANGUAGE) && !defined(_LINKER)

#ifdef VXMICRO_ARCH_arm
#define GTEXT(sym) .global FUNC(sym); .type FUNC(sym),%function
#define GDATA(sym) .global FUNC(sym); .type FUNC(sym),%object
#define WTEXT(sym) .weak FUNC(sym); .type FUNC(sym),%function
#define WDATA(sym) .weak FUNC(sym); .type FUNC(sym),%object
#elif defined(VXMICRO_ARCH_arc)
/*
 * Need to use assembly macros because ';' is interpreted as the start of
 * a single line comment in the ARC assembler.
 */

.macro glbl_text symbol
	.globl FUNC(\symbol)
	.type FUNC(\symbol), %function
.endm

.macro glbl_data symbol
	.globl FUNC(\symbol)
	.type FUNC(\symbol), %object
.endm

#define GTEXT(sym) glbl_text sym
#define GDATA(sym) glbl_data sym
#else  /* !VXMICRO_ARCH_arm && !VXMICRO_ARCH_arc */
#define GTEXT(sym) .globl FUNC(sym); .type FUNC(sym),@function
#define GDATA(sym) .globl FUNC(sym); .type FUNC(sym),@object
#endif

/*
 * These macros specify the section in which a given function or variable
 * resides.
 *
 * - SECTION_FUNC	allows only one function to reside in a sub-section
 * - SECTION_SUBSEC_FUNC allows multiple functions to reside in a sub-section
 *   This ensures that garbage collection only discards the section
 *   if all functions in the sub-section are not referenced.
 */

#if defined(VXMICRO_ARCH_arc)
/*
 * Need to use assembly macros because ';' is interpreted as the start of
 * a single line comment in the ARC assembler.
 *
 * Also, '\()' is needed in the .section directive of these macros for
 * correct substitution of the 'section' variable.
 */

.macro section_var section, symbol
	.section .\section\().FUNC(\symbol)
	FUNC(\symbol):
.endm

.macro section_func section, symbol
	.section .\section\().FUNC(\symbol), "ax"
	FUNC_CODE()
	PERFOPT_ALIGN
	FUNC(\symbol):
	FUNC_INSTR(\symbol)
.endm

.macro section_subsec_func section, subsection, symbol
	.section .\section\().\subsection, "ax"
	PERFOPT_ALIGN
	FUNC(\symbol):
.endm

#define SECTION_VAR(sect, sym) section_var sect, sym
#define SECTION_FUNC(sect, sym) section_func sect, sym
#define SECTION_SUBSEC_FUNC(sect, subsec, sym) \
	section_subsec_func sect, subsec, sym
#else /* !VXMICRO_ARCH_arc */

#define SECTION_VAR(sect, sym)  .section .sect.FUNC(sym); FUNC(sym):
#define SECTION_FUNC(sect, sym)						\
	.section .sect.FUNC(sym), "ax";					\
				FUNC_CODE()				\
				PERFOPT_ALIGN; FUNC(sym):		\
							FUNC_INSTR(sym)
#define SECTION_SUBSEC_FUNC(sect, subsec, sym)				\
		.section .sect.subsec, "ax"; PERFOPT_ALIGN; FUNC(sym):

#endif /* VXMICRO_ARCH_arc */

#endif /* _ASMLANGUAGE && !_LINKER */

#if defined(CONFIG_ARM) && defined(_ASMLANGUAGE)
#if defined(CONFIG_ISA_THUMB2)
/* '.syntax unified' is a gcc-ism used in thumb-2 asm files */
#define _ASM_FILE_PROLOGUE .text; .syntax unified; .thumb
#elif defined(CONFIG_ISA_THUMB)
#define _ASM_FILE_PROLOGUE .text; .code 16
#else
#define _ASM_FILE_PROLOGUE .text; .code 32
#endif
#endif

/*
 * These macros generate absolute symbols for GCC
 */

/* create an extern reference to the absolute symbol */

#define GEN_OFFSET_EXTERN(name) extern const char name[]

#define GEN_ABS_SYM_BEGIN(name) \
	extern void name(void); \
	void name(void)         \
	{

#define GEN_ABS_SYM_END }

#if defined(VXMICRO_ARCH_arm)

/*
 * GNU/ARM backend does not have a proper operand modifier which does not
 * produces prefix # followed by value, such as %0 for PowerPC, Intel, and
 * MIPS. The workaround performed here is using %B0 which converts
 * the value to ~(value). Thus "n"(~(value)) is set in operand constraint
 * to output (value) in the ARM specific GEN_OFFSET macro.
 */

#define GEN_ABSOLUTE_SYM(name, value)               \
	__asm__(".globl\t" #name "\n\t.equ\t" #name \
		",%B0"                              \
		"\n\t.type\t" #name ",%%object" :  : "n"(~(value)))

#elif defined(VXMICRO_ARCH_x86) || defined(VXMICRO_ARCH_arc)

#define GEN_ABSOLUTE_SYM(name, value)               \
	__asm__(".globl\t" #name "\n\t.equ\t" #name \
		",%c0"                              \
		"\n\t.type\t" #name ",@object" :  : "n"(value))

#else
#error processor architecture not supported
#endif
