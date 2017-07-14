/*
 * Copyright (c) 2010-2014,2017 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief GCC toolchain abstraction
 *
 * Macros to abstract compiler capabilities for GCC toolchain.
 */

/*
 * GCC 4.6 and higher have _Static_assert built in, and its output is
 * easier to understand than the common BUILD_ASSERT macros.
 */
#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#define BUILD_ASSERT(EXPR) _Static_assert(EXPR, "")
#define BUILD_ASSERT_MSG(EXPR, MSG) _Static_assert(EXPR, MSG)
#endif

#include <toolchain/common.h>

#define ALIAS_OF(of) __attribute__((alias(#of)))

#define FUNC_ALIAS(real_func, new_alias, return_type) \
	return_type new_alias() ALIAS_OF(real_func)

#define CODE_UNREACHABLE __builtin_unreachable()
#define FUNC_NORETURN    __attribute__((__noreturn__))

/* The GNU assembler for Cortex-M3 uses # for immediate values, not
 * comments, so the @nobits# trick does not work.
 */
#if defined(CONFIG_ARM)
#define _NODATA_SECTION(segment)  __attribute__((section(#segment)))
#else
#define _NODATA_SECTION(segment)				\
	__attribute__((section(#segment ",\"wa\",@nobits#")))
#endif

/* Unaligned access */
#define UNALIGNED_GET(p)						\
__extension__ ({							\
	struct  __attribute__((__packed__)) {				\
		__typeof__(*(p)) __v;					\
	} *__p = (__typeof__(__p)) (p);					\
	__p->__v;							\
})

#define UNALIGNED_PUT(v, p)                                             \
do {                                                                    \
	struct __attribute__((__packed__)) {                            \
		__typeof__(*p) __v;                                     \
	} *__p = (__typeof__(__p)) (p);                                 \
	__p->__v = (v);                                               \
} while (0)

/* Double indirection to ensure section names are expanded before
 * stringification
 */
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define _GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#define ___in_section(a, b, c) \
	__attribute__((section("." _STRINGIFY(a)			\
				"." _STRINGIFY(b)			\
				"." _STRINGIFY(c))))
#define __in_section(a, b, c) ___in_section(a, b, c)

#define __in_section_unique(seg) ___in_section(seg, __FILE__, __COUNTER__)

#ifdef CONFIG_APPLICATION_MEMORY
#define __kernel	__in_section_unique(kernel)
#define __kernel_noinit	__in_section_unique(kernel_noinit)
#define __kernel_bss	__in_section_unique(kernel_bss)
#else
#define __kernel
#define __kernel_noinit	__noinit
#define __kernel_bss
#endif

#ifndef __packed
#define __packed        __attribute__((__packed__))
#endif
#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif
#define __may_alias     __attribute__((__may_alias__))
#ifndef __printf_like
#define __printf_like(f, a)   __attribute__((format (printf, f, a)))
#endif
#define __used		__attribute__((__used__))
#define __deprecated	__attribute__((deprecated))
#define ARG_UNUSED(x) (void)(x)

#define likely(x)   __builtin_expect((long)!!(x), 1L)
#define unlikely(x) __builtin_expect((long)!!(x), 0L)

#define popcount(x) __builtin_popcount(x)

#define __weak __attribute__((__weak__))
#define __unused __attribute__((__unused__))

/* Be *very* careful with this, you cannot filter out with -wno-deprecated,
 * which has implications for -Werror
 */
#define __DEPRECATED_MACRO _Pragma("GCC warning \"Macro is deprecated\"")

/* These macros allow having ARM asm functions callable from thumb */

#if defined(_ASMLANGUAGE) && !defined(_LINKER)

#ifdef CONFIG_ARM

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

#endif /* !CONFIG_ARM */

#endif /* _ASMLANGUAGE && !_LINKER */

/*
 * These macros are used to declare assembly language symbols that need
 * to be typed properly(func or data) to be visible to the OMF tool.
 * So that the build tool could mark them as an entry point to be linked
 * correctly.  This is an elfism. Use #if 0 for a.out.
 */

#if defined(_ASMLANGUAGE) && !defined(_LINKER)

#if defined(CONFIG_ARM) || defined(CONFIG_NIOS2) || defined(CONFIG_RISCV32) \
	|| defined(CONFIG_XTENSA)
#define GTEXT(sym) .global sym; .type sym, %function
#define GDATA(sym) .global sym; .type sym, %object
#define WTEXT(sym) .weak sym; .type sym, %function
#define WDATA(sym) .weak sym; .type sym, %object
#elif defined(CONFIG_ARC)
/*
 * Need to use assembly macros because ';' is interpreted as the start of
 * a single line comment in the ARC assembler.
 */

.macro glbl_text symbol
	.globl \symbol
	.type \symbol, %function
.endm

.macro glbl_data symbol
	.globl \symbol
	.type \symbol, %object
.endm

.macro weak_data symbol
	.weak \symbol
	.type \symbol, %object
.endm

#define GTEXT(sym) glbl_text sym
#define GDATA(sym) glbl_data sym
#define WDATA(sym) weak_data sym

#else  /* !CONFIG_ARM && !CONFIG_ARC */
#define GTEXT(sym) .globl sym; .type sym, @function
#define GDATA(sym) .globl sym; .type sym, @object
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

#if defined(CONFIG_ARC)
/*
 * Need to use assembly macros because ';' is interpreted as the start of
 * a single line comment in the ARC assembler.
 *
 * Also, '\()' is needed in the .section directive of these macros for
 * correct substitution of the 'section' variable.
 */

.macro section_var section, symbol
	.section .\section\().\symbol
	\symbol :
.endm

.macro section_func section, symbol
	.section .\section\().\symbol, "ax"
	FUNC_CODE()
	PERFOPT_ALIGN
	\symbol :
	FUNC_INSTR(\symbol)
.endm

.macro section_subsec_func section, subsection, symbol
	.section .\section\().\subsection, "ax"
	PERFOPT_ALIGN
	\symbol :
.endm

#define SECTION_VAR(sect, sym) section_var sect, sym
#define SECTION_FUNC(sect, sym) section_func sect, sym
#define SECTION_SUBSEC_FUNC(sect, subsec, sym) \
	section_subsec_func sect, subsec, sym
#else /* !CONFIG_ARC */

#define SECTION_VAR(sect, sym)  .section .sect.##sym; sym :
#define SECTION_FUNC(sect, sym)						\
	.section .sect.sym, "ax";					\
				FUNC_CODE()				\
				PERFOPT_ALIGN; sym :		\
							FUNC_INSTR(sym)
#define SECTION_SUBSEC_FUNC(sect, subsec, sym)				\
		.section .sect.subsec, "ax"; PERFOPT_ALIGN; sym :

#endif /* CONFIG_ARC */

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
	EXTERN_C void name(void); \
	void name(void)         \
	{

#define GEN_ABS_SYM_END }

#if defined(CONFIG_ARM)

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

#elif defined(CONFIG_X86) || defined(CONFIG_ARC)

#define GEN_ABSOLUTE_SYM(name, value)               \
	__asm__(".globl\t" #name "\n\t.equ\t" #name \
		",%c0"                              \
		"\n\t.type\t" #name ",@object" :  : "n"(value))

#elif defined(CONFIG_NIOS2) || defined(CONFIG_RISCV32) || defined(CONFIG_XTENSA)

/* No special prefixes necessary for constants in this arch AFAICT */
#define GEN_ABSOLUTE_SYM(name, value)		\
	__asm__(".globl\t" #name "\n\t.equ\t" #name \
		",%0"                              \
		"\n\t.type\t" #name ",%%object" :  : "n"(value))

#else
#error processor architecture not supported
#endif

#define compiler_barrier() do { \
	__asm__ __volatile__ ("" ::: "memory"); \
} while ((0))
