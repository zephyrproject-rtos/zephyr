/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_ICCARM_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_ICCARM_H_

/**
 * @file
 * @brief ICCARM toolchain abstraction
 *
 * Macros to abstract compiler capabilities for ICCARM toolchain.
 */

/* ICCARM supports its own #pragma diag_{warning,default,error,warning}. */
/* #define TOOLCHAIN_HAS_PRAGMA_DIAG 0 */

#define TOOLCHAIN_HAS_C_GENERIC 1

#define TOOLCHAIN_HAS_C_AUTO_TYPE 1

/* #define TOOLCHAIN_HAS_ZLA 1 */

/*
 * IAR do not define __BYTE_ORDER__, so it must be manually
 * detected and defined using arch-specific definitions.
 */

#ifndef _LINKER

#ifndef __ORDER_BIG_ENDIAN__
#define __ORDER_BIG_ENDIAN__            (1)
#endif /* __ORDER_BIG_ENDIAN__ */

#ifndef __ORDER_LITTLE_ENDIAN__
#define __ORDER_LITTLE_ENDIAN__         (2)
#endif /* __ORDER_LITTLE_ENDIAN__ */

#ifndef __ORDER_PDP_ENDIAN__
#define __ORDER_PDP_ENDIAN__            (3)
#endif /* __ORDER_PDP_ENDIAN__ */

#ifndef __BYTE_ORDER__

#if __LITTLE_ENDIAN__ == 1
#define __BYTE_ORDER__                  __ORDER_LITTLE_ENDIAN__
#else
#define __BYTE_ORDER__                  __ORDER_BIG_ENDIAN__
#endif /* __LITTLE_ENDIAN__ == 1 */

#endif /* __BYTE_ORDER__ */


#if defined(__cplusplus) && (__cplusplus >= 201103L)
#define BUILD_ASSERT(EXPR, MSG...)  static_assert(EXPR, "" MSG)
#elif defined(__ICCARM__)
#define BUILD_ASSERT(EXPR, MSG...) _Static_assert(EXPR, "" MSG)
#endif

/* Zephyr makes use of __ATOMIC_SEQ_CST */
#ifdef __STDC_NO_ATOMICS__
#ifndef __ATOMIC_SEQ_CST
#define __MEMORY_ORDER_SEQ_CST__ 5
#endif
#endif
#ifndef __ATOMIC_SEQ_CST
#define __ATOMIC_SEQ_CST __MEMORY_ORDER_SEQ_CST__
#endif

/* By default, restrict is recognized in Standard C
 * __restrict is always recognized
 */
#define ZRESTRICT __restrict

#include <zephyr/toolchain/common.h>
#include <stdbool.h>

#define ALIAS_OF(of) __attribute__((alias(#of)))

#define FUNC_ALIAS(real_func, new_alias, return_type) \
	return_type new_alias() ALIAS_OF(real_func)

#define CODE_UNREACHABLE __builtin_unreachable()
#define FUNC_NORETURN    __attribute__((__noreturn__))

#define _NODATA_SECTION(segment)  __attribute__((section(#segment)))

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
} while (false)


/* Double indirection to ensure section names are expanded before
 * stringification
 */
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define Z_GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#define __GENERIC_DOT_SECTION(segment) \
	__attribute__((section("." STRINGIFY(segment))))
#define Z_GENERIC_DOT_SECTION(segment) __GENERIC_DOT_SECTION(segment)

#define ___in_section(a, b, c) \
	__attribute__((section("." Z_STRINGIFY(a)			\
				"." Z_STRINGIFY(b)			\
				"." Z_STRINGIFY(c))))
#define __in_section(a, b, c) ___in_section(a, b, c)

#define __in_section_unique(seg) ___in_section(seg, __FILE__, __COUNTER__)

#define __in_section_unique_named(seg, name) \
	___in_section(seg, __FILE__, name)

/* When using XIP, using '__ramfunc' places a function into RAM instead
 * of FLASH. Make sure '__ramfunc' is defined only when
 * CONFIG_ARCH_HAS_RAMFUNC_SUPPORT is defined, so that the compiler can
 * report an error if '__ramfunc' is used but the architecture does not
 * support it.
 */
#if !defined(CONFIG_XIP)
#define __ramfunc
#elif defined(CONFIG_ARCH_HAS_RAMFUNC_SUPPORT)
/* Use this instead of the IAR keyword __ramfunc to make sure it
 * ends up in the correct section.
 */
#define __ramfunc __attribute__((noinline, section(".ramfunc")))
#endif /* !CONFIG_XIP */

/* TG-WG: ICCARM does not support __fallthrough */
#define __fallthrough  [[fallthrough]]

#ifndef __packed
#define __packed        __attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(x)	__attribute__((__aligned__(x)))
#endif

#ifndef __noinline
#define __noinline        __attribute__((noinline))
#endif

#if defined(__cplusplus)
#define __alignof(x) alignof(x)
#else
#define __alignof(x) _Alignof(x)
#endif

#define __may_alias     __attribute__((__may_alias__))

#ifndef __printf_like
/*
 * The Zephyr stdint convention enforces int32_t = int, int64_t = long long,
 * and intptr_t = long so that short string format length modifiers can be
 * used universally across ILP32 and LP64 architectures. Without that it
 * is possible for ILP32 toolchains to have int32_t = long and intptr_t = int
 * clashing with the Zephyr convention and generating pointless warnings
 * as they're still the same size. Inhibit the format argument type
 * validation in that case and let the other configs do it.
 */
#define __printf_like(f, a)
#endif

#define __used		__attribute__((__used__))
#define __unused	__attribute__((__unused__))
#define __maybe_unused  __attribute__((__unused__))

#ifndef __deprecated
#define __deprecated	__attribute__((deprecated))
#endif

#define FUNC_NO_STACK_PROTECTOR _Pragma("no_stack_protect")

#ifndef __attribute_const__
#if __VER__ > 0x09000000
#define __attribute_const__ __attribute__((const))
#else
#define __attribute_const__
#endif
#endif

#ifndef __must_check
/* #warning "The attribute __warn_unused_result is not supported in ICCARM". */
#define __must_check
/* #define __must_check __attribute__((warn_unused_result)) */
#endif

#define __PRAGMA(...) _Pragma(#__VA_ARGS__)
#define ARG_UNUSED(x) (void)(x)

#define likely(x)   (__builtin_expect((bool)!!(x), true) != 0L)
#define unlikely(x) (__builtin_expect((bool)!!(x), false) != 0L)
#define POPCOUNT(x) __builtin_popcount(x)

#ifndef __no_optimization
#define __no_optimization __PRAGMA(optimize = none)
#endif

#ifndef __attribute_nonnull
 #define __attribute_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))
#endif

/* __weak is an ICCARM built-in, but it doesn't work in all positions */
/* the Zephyr uses it so we replace it with an attribute((weak))      */
#define __weak __attribute__((__weak__))

/* Builtins */

#include <intrinsics.h>

/*
 * Be *very* careful with these. You cannot filter out __DEPRECATED_MACRO with
 * -wno-deprecated, which has implications for -Werror.
 */


/*
 * Expands to nothing and generates a warning. Used like
 *
 *   #define FOO __WARN("Please use BAR instead") ...
 *
 * The warning points to the location where the macro is expanded.
 */
#define __WARN(s) __PRAGMA(message = #s)
#define __WARN1(s) __PRAGMA(message = #s)

/* Generic message */
#ifndef __DEPRECATED_MACRO
#define __DEPRECATED_MACRO __WARN("Macro is deprecated")
#endif

/* These macros allow having ARM asm functions callable from thumb */

#if defined(_ASMLANGUAGE)

#if defined(CONFIG_ASSEMBLER_ISA_THUMB2)
#define FUNC_CODE() .code 32
#define FUNC_INSTR(a)
/* '.syntax unified' is a gcc-ism used in thumb-2 asm files */
#define _ASM_FILE_PROLOGUE .text; .syntax unified; .thumb
#else
#define FUNC_CODE()
#define FUNC_INSTR(a)
#define _ASM_FILE_PROLOGUE .text; .code 32
#endif /* CONFIG_ASSEMBLER_ISA_THUMB2 */

/*
 * These macros are used to declare assembly language symbols that need
 * to be typed properly(func or data) to be visible to the OMF tool.
 * So that the build tool could mark them as an entry point to be linked
 * correctly.  This is an elfism. Use #if 0 for a.out.
 */

/* This is not implemented yet for IAR */
#define GTEXT(sym)
#define GDATA(sym)
#define WTEXT(sym)
#define WDATA(sym)

#define SECTION_VAR(sect, sym)
#define SECTION_FUNC(sect, sym)
#define SECTION_SUBSEC_FUNC(sect, subsec, sym)

#endif /* _ASMLANGUAGE */


/*
 * These macros generate absolute symbols for IAR
 */

/* create an extern reference to the absolute symbol */

#define GEN_OFFSET_EXTERN(name) extern const char name[]

#define GEN_ABS_SYM_BEGIN(name) \
	EXTERN_C void name(void); \
	void name(void)         \
	{

#define GEN_ABS_SYM_END }

/*
 * Note that GEN_ABSOLUTE_SYM(), depending on the architecture
 * and toolchain, may restrict the range of values permitted
 * for assignment to the named symbol.
 */
#define GEN_ABSOLUTE_SYM(name, value) \
	__PRAGMA(public_equ = #name, (unsigned int)value)

/*
 * GEN_ABSOLUTE_SYM_KCONFIG() is outputted by the build system
 * to generate named symbol/value pairs for kconfigs.
 */
#define GEN_ABSOLUTE_SYM_KCONFIG(name, value) \
	__PRAGMA(public_equ = #name, (unsigned int)value)

#define compiler_barrier() do { \
	__asm volatile("" ::: "memory"); \
} while (false)

/** @brief Return larger value of two provided expressions.
 *
 * Macro ensures that expressions are evaluated only once.
 *
 * @note Macro has limited usage compared to the standard macro as it cannot be
 *	 used:
 *	 - to generate constant integer, e.g. __aligned(Z_MAX(4,5))
 *	 - static variable, e.g. array like static uint8_t array[Z_MAX(...)];
 */
#define Z_MAX(a, b) ({ \
		/* random suffix to avoid naming conflict */ \
		__typeof__(a) _value_a_ = (a); \
		__typeof__(b) _value_b_ = (b); \
		_value_a_ > _value_b_ ? _value_a_ : _value_b_; \
	})

/** @brief Return smaller value of two provided expressions.
 *
 * Macro ensures that expressions are evaluated only once. See @ref Z_MAX for
 * macro limitations.
 */
#define Z_MIN(a, b) ({ \
		/* random suffix to avoid naming conflict */ \
		__typeof__(a) _value_a_ = (a); \
		__typeof__(b) _value_b_ = (b); \
		_value_a_ < _value_b_ ? _value_a_ : _value_b_; \
	})

/** @brief Return a value clamped to a given range.
 *
 * Macro ensures that expressions are evaluated only once. See @ref Z_MAX for
 * macro limitations.
 */
#define Z_CLAMP(val, low, high) ({                                             \
		/* random suffix to avoid naming conflict */                   \
		__typeof__(val) _value_val_ = (val);                           \
		__typeof__(low) _value_low_ = (low);                           \
		__typeof__(high) _value_high_ = (high);                        \
		(_value_val_ < _value_low_)  ? _value_low_ :                   \
		(_value_val_ > _value_high_) ? _value_high_ :                  \
					       _value_val_;                    \
	})

/**
 * @brief Calculate power of two ceiling for some nonzero value
 *
 * @param x Nonzero unsigned long value
 * @return X rounded up to the next power of two
 */
#define Z_POW2_CEIL(x) \
	((x) <= 2UL ? (x) : (1UL << (8 * sizeof(long) - __builtin_clzl((x) - 1))))

/**
 * @brief Check whether or not a value is a power of 2
 *
 * @param x The value to check
 * @return true if x is a power of 2, false otherwise
 */
#define Z_IS_POW2(x) (((x) != 0) && (((x) & ((x)-1)) == 0))

#ifndef __INT8_C
#define __INT8_C(x)	x
#endif

#ifndef INT8_C
#define INT8_C(x)	__INT8_C(x)
#endif

#ifndef __UINT8_C
#define __UINT8_C(x)	x ## U
#endif

#ifndef UINT8_C
#define UINT8_C(x)	__UINT8_C(x)
#endif

#ifndef __INT16_C
#define __INT16_C(x)	x
#endif

#ifndef INT16_C
#define INT16_C(x)	__INT16_C(x)
#endif

#ifndef __UINT16_C
#define __UINT16_C(x)	x ## U
#endif

#ifndef UINT16_C
#define UINT16_C(x)	__UINT16_C(x)
#endif

#ifndef __INT32_C
#define __INT32_C(x)	x
#endif

#ifndef INT32_C
#define INT32_C(x)	__INT32_C(x)
#endif

#ifndef __UINT32_C
#define __UINT32_C(x)	x ## U
#endif

#ifndef UINT32_C
#define UINT32_C(x)	__UINT32_C(x)
#endif

#ifndef __INT64_C
#define __INT64_C(x)	x ## LL
#endif

#ifndef INT64_C
#define INT64_C(x)	__INT64_C(x)
#endif

#ifndef __UINT64_C
#define __UINT64_C(x)	x ## ULL
#endif

#ifndef UINT64_C
#define UINT64_C(x)	__UINT64_C(x)
#endif

/* Convenience macros */
#undef _GLUE_B
#undef _GLUE
#define _GLUE_B(x, y) x##y
#define _GLUE(x, y)   _GLUE_B(x, y)

#ifndef INTMAX_C
#define INTMAX_C(x)  _GLUE(x, __INTMAX_C_SUFFIX__)
#endif

#ifndef UINTMAX_C
#define UINTMAX_C(x) _GLUE(x, __UINTMAX_C_SUFFIX__)
#endif

#endif /* !_LINKER */
#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_ICCARM_H_ */
