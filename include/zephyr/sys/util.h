/*
 * Copyright (c) 2011-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Misc utilities
 *
 * Misc utilities usable by the kernel and application code.
 */

#ifndef ZEPHYR_INCLUDE_SYS_UTIL_H_
#define ZEPHYR_INCLUDE_SYS_UTIL_H_

#include <zephyr/sys/util_macro.h>

/* needs to be outside _ASMLANGUAGE so 'true' and 'false' can turn
 * into '1' and '0' for asm or linker scripts
 */
#include <stdbool.h>

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stddef.h>

/** @brief Number of bits that make up a type */
#define NUM_BITS(t) (sizeof(t) * 8)

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sys-util Utility Functions
 * @{
 */

/** @brief Cast @p x, a pointer, to an unsigned integer. */
#define POINTER_TO_UINT(x) ((uintptr_t) (x))
/** @brief Cast @p x, an unsigned integer, to a <tt>void*</tt>. */
#define UINT_TO_POINTER(x) ((void *) (uintptr_t) (x))
/** @brief Cast @p x, a pointer, to a signed integer. */
#define POINTER_TO_INT(x)  ((intptr_t) (x))
/** @brief Cast @p x, a signed integer, to a <tt>void*</tt>. */
#define INT_TO_POINTER(x)  ((void *) (intptr_t) (x))

#if !(defined(__CHAR_BIT__) && defined(__SIZEOF_LONG__) && defined(__SIZEOF_LONG_LONG__))
#	error Missing required predefined macros for BITS_PER_LONG calculation
#endif

/** Number of bits in a long int. */
#define BITS_PER_LONG	(__CHAR_BIT__ * __SIZEOF_LONG__)

/** Number of bits in a long long int. */
#define BITS_PER_LONG_LONG	(__CHAR_BIT__ * __SIZEOF_LONG_LONG__)

/**
 * @brief Create a contiguous bitmask starting at bit position @p l
 *        and ending at position @p h.
 */
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

/**
 * @brief Create a contiguous 64-bit bitmask starting at bit position @p l
 *        and ending at position @p h.
 */
#define GENMASK64(h, l) \
	(((~0ULL) - (1ULL << (l)) + 1) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

/** @brief Extract the Least Significant Bit from @p value. */
#define LSB_GET(value) ((value) & -(value))

/**
 * @brief Extract a bitfield element from @p value corresponding to
 *	  the field mask @p mask.
 */
#define FIELD_GET(mask, value)  (((value) & (mask)) / LSB_GET(mask))

/**
 * @brief Prepare a bitfield element using @p value with @p mask representing
 *	  its field position and width. The result should be combined
 *	  with other fields using a logical OR.
 */
#define FIELD_PREP(mask, value) (((value) * LSB_GET(mask)) & (mask))

/** @brief 0 if @p cond is true-ish; causes a compile error otherwise. */
#define ZERO_OR_COMPILE_ERROR(cond) ((int) sizeof(char[1 - 2 * !(cond)]) - 1)

#if defined(__cplusplus)

/* The built-in function used below for type checking in C is not
 * supported by GNU C++.
 */
#define ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#else /* __cplusplus */

/**
 * @brief Zero if @p array has an array type, a compile error otherwise
 *
 * This macro is available only from C, not C++.
 */
#define IS_ARRAY(array) \
	ZERO_OR_COMPILE_ERROR( \
		!__builtin_types_compatible_p(__typeof__(array), \
					      __typeof__(&(array)[0])))

/**
 * @brief Number of elements in the given @p array
 *
 * In C++, due to language limitations, this will accept as @p array
 * any type that implements <tt>operator[]</tt>. The results may not be
 * particularly meaningful in this case.
 *
 * In C, passing a pointer as @p array causes a compile error.
 */
#define ARRAY_SIZE(array) \
	((size_t) (IS_ARRAY(array) + (sizeof(array) / sizeof((array)[0]))))

#endif /* __cplusplus */

/**
 * @brief Whether @p ptr is an element of @p array
 *
 * This macro can be seen as a slightly stricter version of @ref PART_OF_ARRAY
 * in that it also ensures that @p ptr is aligned to an array-element boundary
 * of @p array.
 *
 * In C, passing a pointer as @p array causes a compile error.
 *
 * @param array the array in question
 * @param ptr the pointer to check
 *
 * @return 1 if @p ptr is part of @p array, 0 otherwise
 */
#define IS_ARRAY_ELEMENT(array, ptr)                                                               \
	((ptr) && POINTER_TO_UINT(array) <= POINTER_TO_UINT(ptr) &&                          \
	 POINTER_TO_UINT(ptr) < POINTER_TO_UINT(&(array)[ARRAY_SIZE(array)]) &&                    \
	 (POINTER_TO_UINT(ptr) - POINTER_TO_UINT(array)) % sizeof((array)[0]) == 0)

/**
 * @brief Index of @p ptr within @p array
 *
 * With `CONFIG_ASSERT=y`, this macro will trigger a runtime assertion
 * when @p ptr does not fall into the range of @p array or when @p ptr
 * is not aligned to an array-element boundary of @p array.
 *
 * In C, passing a pointer as @p array causes a compile error.
 *
 * @param array the array in question
 * @param ptr pointer to an element of @p array
 *
 * @return the array index of @p ptr within @p array, on success
 */
#define ARRAY_INDEX(array, ptr)                                                                    \
	({                                                                                         \
		__ASSERT_NO_MSG(IS_ARRAY_ELEMENT(array, ptr));                                     \
		(__typeof__((array)[0]) *)(ptr) - (array);                                         \
	})

/**
 * @brief Check if a pointer @p ptr lies within @p array.
 *
 * In C but not C++, this causes a compile error if @p array is not an array
 * (e.g. if @p ptr and @p array are mixed up).
 *
 * @param array an array
 * @param ptr a pointer
 * @return 1 if @p ptr is part of @p array, 0 otherwise
 */
#define PART_OF_ARRAY(array, ptr)                                                                  \
	((ptr) && POINTER_TO_UINT(array) <= POINTER_TO_UINT(ptr) &&                                \
	 POINTER_TO_UINT(ptr) < POINTER_TO_UINT(&(array)[ARRAY_SIZE(array)]))

/**
 * @brief Array-index of @p ptr within @p array, rounded down
 *
 * This macro behaves much like @ref ARRAY_INDEX with the notable
 * difference that it accepts any @p ptr in the range of @p array rather than
 * exclusively a @p ptr aligned to an array-element boundary of @p array.
 *
 * With `CONFIG_ASSERT=y`, this macro will trigger a runtime assertion
 * when @p ptr does not fall into the range of @p array.
 *
 * In C, passing a pointer as @p array causes a compile error.
 *
 * @param array the array in question
 * @param ptr pointer to an element of @p array
 *
 * @return the array index of @p ptr within @p array, on success
 */
#define ARRAY_INDEX_FLOOR(array, ptr)                                                              \
	({                                                                                         \
		__ASSERT_NO_MSG(PART_OF_ARRAY(array, ptr));                                        \
		(POINTER_TO_UINT(ptr) - POINTER_TO_UINT(array)) / sizeof((array)[0]);              \
	})

/**
 * @brief Get a pointer to a structure containing the element
 *
 * Example:
 *
 *	struct foo {
 *		int bar;
 *	};
 *
 *	struct foo my_foo;
 *	int *ptr = &my_foo.bar;
 *
 *	struct foo *container = CONTAINER_OF(ptr, struct foo, bar);
 *
 * Above, @p container points at @p my_foo.
 *
 * @param ptr pointer to a structure element
 * @param type name of the type that @p ptr is an element of
 * @param field the name of the field within the struct @p ptr points to
 * @return a pointer to the structure that contains @p ptr
 */
#define CONTAINER_OF(ptr, type, field) \
	((type *)(((char *)(ptr)) - offsetof(type, field)))

/**
 * @brief Value of @p x rounded up to the next multiple of @p align,
 *        which must be a power of 2.
 */
#define ROUND_UP(x, align)                                   \
	(((unsigned long)(x) + ((unsigned long)(align) - 1)) & \
	 ~((unsigned long)(align) - 1))

/**
 * @brief Value of @p x rounded down to the previous multiple of @p
 *        align, which must be a power of 2.
 */
#define ROUND_DOWN(x, align)                                 \
	((unsigned long)(x) & ~((unsigned long)(align) - 1))

/** @brief Value of @p x rounded up to the next word boundary. */
#define WB_UP(x) ROUND_UP(x, sizeof(void *))

/** @brief Value of @p x rounded down to the previous word boundary. */
#define WB_DN(x) ROUND_DOWN(x, sizeof(void *))

/**
 * @brief Divide and round up.
 *
 * Example:
 * @code{.c}
 * DIV_ROUND_UP(1, 2); // 1
 * DIV_ROUND_UP(3, 2); // 2
 * @endcode
 *
 * @param n Numerator.
 * @param d Denominator.
 *
 * @return The result of @p n / @p d, rounded up.
 */
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

/**
 * @brief Ceiling function applied to @p numerator / @p divider as a fraction.
 * @deprecated Use DIV_ROUND_UP() instead.
 */
#define ceiling_fraction(numerator, divider) __DEPRECATED_MACRO \
	DIV_ROUND_UP(numerator, divider)

/**
 * @brief Obtain the maximum of two values.
 *
 * @note Arguments are evaluated twice. Use Z_MAX for a GCC-only, single
 * evaluation version
 *
 * @param a First value.
 * @param b Second value.
 *
 * @returns Maximum value of @p a and @p b.
 */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/**
 * @brief Obtain the minimum of two values.
 *
 * @note Arguments are evaluated twice. Use Z_MIN for a GCC-only, single
 * evaluation version
 *
 * @param a First value.
 * @param b Second value.
 *
 * @returns Minimum value of @p a and @p b.
 */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

/**
 * @brief Clamp a value to a given range.
 *
 * @note Arguments are evaluated multiple times. Use Z_CLAMP for a GCC-only,
 * single evaluation version.
 *
 * @param val Value to be clamped.
 * @param low Lowest allowed value (inclusive).
 * @param high Highest allowed value (inclusive).
 *
 * @returns Clamped value.
 */
#define CLAMP(val, low, high) (((val) <= (low)) ? (low) : MIN(val, high))

/**
 * @brief Checks if a value is within range.
 *
 * @note @p val is evaluated twice.
 *
 * @param val Value to be checked.
 * @param min Lower bound (inclusive).
 * @param max Upper bound (inclusive).
 *
 * @retval true If value is within range
 * @retval false If the value is not within range
 */
#define IN_RANGE(val, min, max) ((val) >= (min) && (val) <= (max))

/**
 * @brief Is @p x a power of two?
 * @param x value to check
 * @return true if @p x is a power of two, false otherwise
 */
static inline bool is_power_of_two(unsigned int x)
{
	return IS_POWER_OF_TWO(x);
}

/**
 * @brief Arithmetic shift right
 * @param value value to shift
 * @param shift number of bits to shift
 * @return @p value shifted right by @p shift; opened bit positions are
 *         filled with the sign bit
 */
static inline int64_t arithmetic_shift_right(int64_t value, uint8_t shift)
{
	int64_t sign_ext;

	if (shift == 0U) {
		return value;
	}

	/* extract sign bit */
	sign_ext = (value >> 63) & 1;

	/* make all bits of sign_ext be the same as the value's sign bit */
	sign_ext = -sign_ext;

	/* shift value and fill opened bit positions with sign bit */
	return (value >> shift) | (sign_ext << (64 - shift));
}

/**
 * @brief byte by byte memcpy.
 *
 * Copy `size` bytes of `src` into `dest`. This is guaranteed to be done byte by byte.
 *
 * @param dst Pointer to the destination memory.
 * @param src Pointer to the source of the data.
 * @param size The number of bytes to copy.
 */
static inline void bytecpy(void *dst, const void *src, size_t size)
{
	size_t i;

	for (i = 0; i < size; ++i) {
		((volatile uint8_t *)dst)[i] = ((volatile const uint8_t *)src)[i];
	}
}

/**
 * @brief byte by byte swap.
 *
 * Swap @a size bytes between memory regions @a a and @a b. This is
 * guaranteed to be done byte by byte.
 *
 * @param a Pointer to the the first memory region.
 * @param b Pointer to the the second memory region.
 * @param size The number of bytes to swap.
 */
static inline void byteswp(void *a, void *b, size_t size)
{
	uint8_t t;
	uint8_t *aa = (uint8_t *)a;
	uint8_t *bb = (uint8_t *)b;

	for (; size > 0; --size) {
		t = *aa;
		*aa++ = *bb;
		*bb++ = t;
	}
}

/**
 * @brief      Convert a single character into a hexadecimal nibble.
 *
 * @param c     The character to convert
 * @param x     The address of storage for the converted number.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int char2hex(char c, uint8_t *x);

/**
 * @brief      Convert a single hexadecimal nibble into a character.
 *
 * @param c     The number to convert
 * @param x     The address of storage for the converted character.
 *
 *  @return Zero on success or (negative) error code otherwise.
 */
int hex2char(uint8_t x, char *c);

/**
 * @brief      Convert a binary array into string representation.
 *
 * @param buf     The binary array to convert
 * @param buflen  The length of the binary array to convert
 * @param hex     Address of where to store the string representation.
 * @param hexlen  Size of the storage area for string representation.
 *
 * @return     The length of the converted string, or 0 if an error occurred.
 */
size_t bin2hex(const uint8_t *buf, size_t buflen, char *hex, size_t hexlen);

/**
 * @brief      Convert a hexadecimal string into a binary array.
 *
 * @param hex     The hexadecimal string to convert
 * @param hexlen  The length of the hexadecimal string to convert.
 * @param buf     Address of where to store the binary data
 * @param buflen  Size of the storage area for binary data
 *
 * @return     The length of the binary array, or 0 if an error occurred.
 */
size_t hex2bin(const char *hex, size_t hexlen, uint8_t *buf, size_t buflen);

/**
 * @brief Convert a binary coded decimal (BCD 8421) value to binary.
 *
 * @param bcd BCD 8421 value to convert.
 *
 * @return Binary representation of input value.
 */
static inline uint8_t bcd2bin(uint8_t bcd)
{
	return ((10 * (bcd >> 4)) + (bcd & 0x0F));
}

/**
 * @brief Convert a binary value to binary coded decimal (BCD 8421).
 *
 * @param bin Binary value to convert.
 *
 * @return BCD 8421 representation of input value.
 */
static inline uint8_t bin2bcd(uint8_t bin)
{
	return (((bin / 10) << 4) | (bin % 10));
}

/**
 * @brief      Convert a uint8_t into a decimal string representation.
 *
 * Convert a uint8_t value into its ASCII decimal string representation.
 * The string is terminated if there is enough space in buf.
 *
 * @param buf     Address of where to store the string representation.
 * @param buflen  Size of the storage area for string representation.
 * @param value   The value to convert to decimal string
 *
 * @return     The length of the converted string (excluding terminator if
 *             any), or 0 if an error occurred.
 */
uint8_t u8_to_dec(char *buf, uint8_t buflen, uint8_t value);

/**
 * @brief Properly truncate a NULL-terminated UTF-8 string
 *
 * Take a NULL-terminated UTF-8 string and ensure that if the string has been
 * truncated (by setting the NULL terminator) earlier by other means, that
 * the string ends with a properly formatted UTF-8 character (1-4 bytes).
 *
 * @htmlonly
 * Example:
 *      char test_str[] = "€€€";
 *      char trunc_utf8[8];
 *
 *      printf("Original : %s\n", test_str); // €€€
 *      strncpy(trunc_utf8, test_str, sizeof(trunc_utf8));
 *      trunc_utf8[sizeof(trunc_utf8) - 1] = '\0';
 *      printf("Bad      : %s\n", trunc_utf8); // €€�
 *      utf8_trunc(trunc_utf8);
 *      printf("Truncated: %s\n", trunc_utf8); // €€
 * @endhtmlonly
 *
 * @param utf8_str NULL-terminated string
 *
 * @return Pointer to the @p utf8_str
 */
char *utf8_trunc(char *utf8_str);

/**
 * @brief Copies a UTF-8 encoded string from @p src to @p dst
 *
 * The resulting @p dst will always be NULL terminated if @p n is larger than 0,
 * and the @p dst string will always be properly UTF-8 truncated.
 *
 * @param dst The destination of the UTF-8 string.
 * @param src The source string
 * @param n   The size of the @p dst buffer. Maximum number of characters copied
 *            is @p n - 1. If 0 nothing will be done, and the @p dst will not be
 *            NULL terminated.
 *
 * @return Pointer to the @p dst
 */
char *utf8_lcpy(char *dst, const char *src, size_t n);

#define __z_log2d(x) (32 - __builtin_clz(x) - 1)
#define __z_log2q(x) (64 - __builtin_clzll(x) - 1)
#define __z_log2(x) (sizeof(__typeof__(x)) > 4 ? __z_log2q(x) : __z_log2d(x))

/**
 * @brief Compute log2(x)
 *
 * @note This macro expands its argument multiple times (to permit use
 *       in constant expressions), which must not have side effects.
 *
 * @param x An unsigned integral value
 *
 * @param x value to compute logarithm of (positive only)
 *
 * @return log2(x) when 1 <= x <= max(x), -1 when x < 1
 */
#define LOG2(x) ((x) < 1 ? -1 : __z_log2(x))

/**
 * @brief Compute ceil(log2(x))
 *
 * @note This macro expands its argument multiple times (to permit use
 *       in constant expressions), which must not have side effects.
 *
 * @param x An unsigned integral value
 *
 * @return ceil(log2(x)) when 1 <= x <= max(type(x)), 0 when x < 1
 */
#define LOG2CEIL(x) ((x) < 1 ?  0 : __z_log2((x)-1) + 1)

/**
 * @brief Compute next highest power of two
 *
 * Equivalent to 2^ceil(log2(x))
 *
 * @note This macro expands its argument multiple times (to permit use
 *       in constant expressions), which must not have side effects.
 *
 * @param x An unsigned integral value
 *
 * @return 2^ceil(log2(x)) or 0 if 2^ceil(log2(x)) would saturate 64-bits
 */
#define NHPOT(x) ((x) < 1 ? 1 : ((x) > (1ULL<<63) ? 0 : 1ULL << LOG2CEIL(x)))

#ifdef __cplusplus
}
#endif

/* This file must be included at the end of the !_ASMLANGUAGE guard.
 * It depends on macros defined in this file above which cannot be forward declared.
 */
#include <zephyr/sys/time_units.h>

#endif /* !_ASMLANGUAGE */

/** @brief Number of bytes in @p x kibibytes */
#ifdef _LINKER
/* This is used in linker scripts so need to avoid type casting there */
#define KB(x) ((x) << 10)
#else
#define KB(x) (((size_t)x) << 10)
#endif
/** @brief Number of bytes in @p x mebibytes */
#define MB(x) (KB(x) << 10)
/** @brief Number of bytes in @p x gibibytes */
#define GB(x) (MB(x) << 10)

/** @brief Number of Hz in @p x kHz */
#define KHZ(x) ((x) * 1000)
/** @brief Number of Hz in @p x MHz */
#define MHZ(x) (KHZ(x) * 1000)

/**
 * @brief For the POSIX architecture add a minimal delay in a busy wait loop.
 * For other architectures this is a no-op.
 *
 * In the POSIX ARCH, code takes zero simulated time to execute,
 * so busy wait loops become infinite loops, unless we
 * force the loop to take a bit of time.
 * Include this macro in all busy wait/spin loops
 * so they will also work when building for the POSIX architecture.
 *
 * @param t Time in microseconds we will busy wait
 */
#if defined(CONFIG_ARCH_POSIX)
#define Z_SPIN_DELAY(t) k_busy_wait(t)
#else
#define Z_SPIN_DELAY(t)
#endif

/**
 * @brief Wait for an expression to return true with a timeout
 *
 * Spin on an expression with a timeout and optional delay between iterations
 *
 * Commonly needed when waiting on hardware to complete an asynchronous
 * request to read/write/initialize/reset, but useful for any expression.
 *
 * @param expr Truth expression upon which to poll, e.g.: XYZREG & XYZREG_EN
 * @param timeout Timeout to wait for in microseconds, e.g.: 1000 (1ms)
 * @param delay_stmt Delay statement to perform each poll iteration
 *                   e.g.: NULL, k_yield(), k_msleep(1) or k_busy_wait(1)
 *
 * @retval expr As a boolean return, if false then it has timed out.
 */
#define WAIT_FOR(expr, timeout, delay_stmt)                                                        \
	({                                                                                         \
		uint32_t cycle_count = k_us_to_cyc_ceil32(timeout);                                \
		uint32_t start = k_cycle_get_32();                                                 \
		while (!(expr) && (cycle_count > (k_cycle_get_32() - start))) {                    \
			delay_stmt;                                                                \
			Z_SPIN_DELAY(10);                                                          \
		}                                                                                  \
		(expr);                                                                            \
	})

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_H_ */
