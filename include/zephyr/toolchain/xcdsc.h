/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_XCDSC_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_XCDSC_H_

#ifndef __COUNTER__
/* XCC (GCC-based compiler) doesn't support __COUNTER__
 * but this should be good enough
 */
#define __COUNTER__ __LINE__
#endif

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#error Please do not include toolchain-specific headers directly, use <zephyr/toolchain.h> instead
#endif

#include <stdbool.h>
#include <zephyr/toolchain/common.h>

#define Z_STRINGIFY(x) #x
#define STRINGIFY(s)   Z_STRINGIFY(s)

/* Double indirection to ensure section names are expanded before
 * stringification
 */
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define Z_GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#define __GENERIC_DOT_SECTION(segment) __attribute__((section("." STRINGIFY(segment))))
#define Z_GENERIC_DOT_SECTION(segment) __GENERIC_DOT_SECTION(segment)
/*
 * XC-DSC does not support using deprecated attribute in enum,
 * so just nullify it here to avoid compilation errors.
 */
#define __deprecated

#define __in_section(a, b, c)                                                                      \
	__attribute__((section("." STRINGIFY(a) "." STRINGIFY(b) "." STRINGIFY(c))))
#define __in_section_unique(seg)                                                                   \
	__attribute__((section("." STRINGIFY(seg) "." STRINGIFY(__COUNTER__))))

#define __in_section_unique_named(seg, name)                                                       \
	__attribute__((section("." STRINGIFY(seg) "." STRINGIFY(__COUNTER__) "." STRINGIFY(name))))
#define CODE_UNREACHABLE __builtin_unreachable()

#define __unused       __attribute__((__unused__))
#define __maybe_unused __attribute__((__unused__))

/*
 * *********REDEFINED******
 * #if defined(__cplusplus) && (__cplusplus >= 201103L)
 * #define BUILD_ASSERT(EXPR, MSG...)  static_assert(EXPR, "" MSG)
 * #elif defined(__XC_DSC__)
 * #define BUILD_ASSERT(EXPR, MSG...) _Static_assert(EXPR, "" MSG)
 * #endif
 */
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

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifndef __aligned
#define __aligned(x) __attribute__((__aligned__(x)))
#endif

#ifndef __noinline
#define __noinline __attribute__((noinline))
#endif

#ifndef __attribute_const__
#define __attribute_const__ __attribute__((__const__))
#endif

#ifndef __attribute_nonnull
#define __attribute_nonnull(...) __attribute__((nonnull(__VA_ARGS__)))
#endif

#ifndef __fallthrough
#if __GNUC__ >= 7
#define __fallthrough __attribute__((fallthrough))
#else
#define __fallthrough
#endif /* __GNUC__ >= 7 */
#endif

#define ARG_UNUSED(x) (void)(x)

#define likely(x)   (__builtin_expect((bool)!!(x), true) != 0L)
#define unlikely(x) (__builtin_expect((bool)!!(x), false) != 0L)

#ifndef _ASMLANGUAGE

static inline int popcount(unsigned int x)
{
	int count = 0;

	while (x) {
		count += x & 1;
		x >>= 1;
	}
	return count;
}

#define POPCOUNT(x) popcount(x)
#else
#define POPCOUNT(x) __builtin_popcount(x)
#endif

#ifndef __no_optimization
#define __no_optimization __attribute__((optimize("-O0")))
#endif

#ifndef __weak
#define __weak __attribute__((__weak__))
#endif

#define _NODATA_SECTION(segment) __attribute__((section(#segment)))
#define FUNC_NO_STACK_PROTECTOR
#define FUNC_NORETURN __attribute__((__noreturn__))

/* create an extern reference to the absolute symbol */

#define GEN_OFFSET_EXTERN(name) extern const char name[]

#define GEN_ABS_SYM_BEGIN(name)                                                                    \
	EXTERN_C void name(void);                                                                  \
	void name(void)                                                                            \
	{

#define GEN_ABS_SYM_END }

/* No special prefixes necessary for constants in this arch AFAICT */
#define GEN_ABSOLUTE_SYM(name, value)                                                              \
	__asm__(".global\t" #name "\n\t.equ\t" #name ",%0"                                         \
		"\n\t.type\t" #name ",%%object"                                                    \
		:                                                                                  \
		: "n"(value))

#define GEN_ABSOLUTE_SYM_KCONFIG(name, value)                                                      \
	__asm__(".global\t" #name "\n\t.equ\t" #name "," #value "\n\t.type\t" #name ",%object")

#define compiler_barrier()                                                                         \
	do {                                                                                       \
		__asm__ __volatile__("" ::: "memory");                                             \
	} while (false)

/** @brief Return larger value of two provided expressions.
 *
 * Macro ensures that expressions are evaluated only once.
 *
 * @note Macro has limited usage compared to the standard macro as it cannot be
 *       used:
 *       - to generate constant integer, e.g. __aligned(Z_MAX(4,5))
 *       - static variable, e.g. array like static uint8_t array[Z_MAX(...)];
 */
#define Z_MAX(a, b)                                                                                \
	({                                                                                         \
		/* random suffix to avoid naming conflict */                                       \
		__typeof__(a) _value_a_ = (a);                                                     \
		__typeof__(b) _value_b_ = (b);                                                     \
		(_value_a_ > _value_b_) ? _value_a_ : _value_b_;                                   \
	})

/** @brief Return smaller value of two provided expressions.
 *
 * Macro ensures that expressions are evaluated only once. See @ref Z_MAX for
 * macro limitations.
 */
#define Z_MIN(a, b)                                                                                \
	({                                                                                         \
		/* random suffix to avoid naming conflict */                                       \
		__typeof__(a) _value_a_ = (a);                                                     \
		__typeof__(b) _value_b_ = (b);                                                     \
		(_value_a_ < _value_b_) ? _value_a_ : _value_b_;                                   \
	})

/** @brief Return a value clamped to a given range.
 *
 * Macro ensures that expressions are evaluated only once. See @ref Z_MAX for
 * macro limitations.
 */
#define Z_CLAMP(val, low, high)                                                                    \
	({                                                                                         \
		/* random suffix to avoid naming conflict */                                       \
		__typeof__(val) _value_val_ = (val);                                               \
		__typeof__(low) _value_low_ = (low);                                               \
		__typeof__(high) _value_high_ = (high);                                            \
		(_value_val_ < _value_low_)    ? _value_low_                                       \
		: (_value_val_ > _value_high_) ? _value_high_                                      \
					       : _value_val_;                                      \
	})

/**
 * @brief Calculate power of two ceiling for some nonzero value
 *
 * @param x Nonzero unsigned long value
 * @return X rounded up to the next power of two
 */
#define Z_POW2_CEIL(x) ((x) <= 2UL ? (x) : (1UL << (8 * sizeof(long) - __builtin_clzl((x) - 1))))

/**
 * @brief Check whether or not a value is a power of 2
 *
 * @param x The value to check
 * @return true if x is a power of 2, false otherwise
 */
#define Z_IS_POW2(x) (((x) != 0) && (((x) & ((x) - 1)) == 0))

/*
 * To reuse as much as possible from the llvm.h header we only redefine the
 * __GENERIC_SECTION and Z_GENERIC_SECTION macros here to include the `used` keyword.
 */

#define ZRESTRICT restrict

/* Double indirection to ensure section names are expanded before
 * stringification
 */
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define Z_GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#define __GENERIC_DOT_SECTION(segment) __attribute__((section("." STRINGIFY(segment))))
#define Z_GENERIC_DOT_SECTION(segment) __GENERIC_DOT_SECTION(segment)

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_XSDSC_H_ */

/* Double indirection to ensure section names are expanded before
 * stringification
 */
#define __GENERIC_SECTION(segment) __attribute__((section(STRINGIFY(segment))))
#define Z_GENERIC_SECTION(segment) __GENERIC_SECTION(segment)

#define __GENERIC_DOT_SECTION(segment) __attribute__((section("." STRINGIFY(segment))))
#define Z_GENERIC_DOT_SECTION(segment) __GENERIC_DOT_SECTION(segment)
