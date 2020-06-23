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

/* needs to be outside _ASMLANGUAGE so 'true' and 'false' can turn
 * into '1' and '0' for asm or linker scripts
 */
#include <stdbool.h>

/*
 * Most of the eldritch implementation details for all the macrobatics
 * below (APIs like IS_ENABLED(), COND_CODE_1(), etc.) are hidden away
 * in this file.
 */
#include "util_internal.h"

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup sys-util Zephyr utilities
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

#if !(defined(__CHAR_BIT__) && defined(__SIZEOF_LONG__))
#	error Missing required predefined macros for BITS_PER_LONG calculation
#endif

/** Number of bits in a long int. */
#define BITS_PER_LONG	(__CHAR_BIT__ * __SIZEOF_LONG__)

/**
 * @brief Create a contiguous bitmask starting at bit position @p l
 *        and ending at position @p h.
 */
#define GENMASK(h, l) \
	(((~0UL) - (1UL << (l)) + 1) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

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
 * particulary meaningful in this case.
 *
 * In C, passing a pointer as @p array causes a compile error.
 */
#define ARRAY_SIZE(array) \
	((long) (IS_ARRAY(array) + (sizeof(array) / sizeof((array)[0]))))

#endif /* __cplusplus */

/**
 * @brief Check if a pointer @p ptr lies within @p array.
 *
 * In C but not C++, this causes a compile error if @p array is not an array
 * (e.g. if @p ptr and @p array are mixed up).
 *
 * @param ptr a pointer
 * @param array an array
 * @return 1 if @p ptr is part of @p array, 0 otherwise
 */
#define PART_OF_ARRAY(array, ptr) \
	((ptr) && ((ptr) >= &array[0] && (ptr) < &array[ARRAY_SIZE(array)]))

/**
 * @brief Get a pointer to a container structure from an element
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
 * @brief Ceiling function applied to @p numerator / @p divider as a fraction.
 */
#define ceiling_fraction(numerator, divider) \
	(((numerator) + ((divider) - 1)) / (divider))

/**
 * @def MAX
 * @brief The larger value between @p a and @p b.
 * @note Arguments are evaluated twice.
 */
#ifndef MAX
/* Use Z_MAX for a GCC-only, single evaluation version */
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

/**
 * @def MIN
 * @brief The smaller value between @p a and @p b.
 * @note Arguments are evaluated twice.
 */
#ifndef MIN
/* Use Z_MIN for a GCC-only, single evaluation version */
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

/**
 * @brief Is @p x a power of two?
 * @param x value to check
 * @return true if @p x is a power of two, false otherwise
 */
static inline bool is_power_of_two(unsigned int x)
{
	return (x != 0U) && ((x & (x - 1)) == 0U);
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

#ifndef BIT
#if defined(_ASMLANGUAGE)
#define BIT(n)  (1 << (n))
#else
/**
 * @brief Unsigned integer with bit position @p n set (signed in
 * assembly language).
 */
#define BIT(n)  (1UL << (n))
#endif
#endif

/** @brief 64-bit unsigned integer with bit position @p _n set. */
#define BIT64(_n) (1ULL << (_n))

/**
 * @brief Set or clear a bit depending on a boolean value
 *
 * The argument @p var is a variable whose value is written to as a
 * side effect.
 *
 * @param var Variable to be altered
 * @param bit Bit number
 * @param set if 0, clears @p bit in @p var; any other value sets @p bit
 */
#define WRITE_BIT(var, bit, set) \
	((var) = (set) ? ((var) | BIT(bit)) : ((var) & ~BIT(bit)))

/**
 * @brief Bit mask with bits 0 through <tt>n-1</tt> (inclusive) set,
 * or 0 if @p n is 0.
 */
#define BIT_MASK(n) (BIT(n) - 1)

/**
 * @brief Check for macro definition in compiler-visible expressions
 *
 * This trick was pioneered in Linux as the config_enabled() macro. It
 * has the effect of taking a macro value that may be defined to "1"
 * or may not be defined at all and turning it into a literal
 * expression that can be handled by the C compiler instead of just
 * the preprocessor. It is often used with a @p CONFIG_FOO macro which
 * may be defined to 1 via Kconfig, or left undefined.
 *
 * That is, it works similarly to <tt>\#if defined(CONFIG_FOO)</tt>
 * except that its expansion is a C expression. Thus, much <tt>\#ifdef</tt>
 * usage can be replaced with equivalents like:
 *
 *     if (IS_ENABLED(CONFIG_FOO)) {
 *             do_something_with_foo
 *     }
 *
 * This is cleaner since the compiler can generate errors and warnings
 * for @p do_something_with_foo even when @p CONFIG_FOO is undefined.
 *
 * @param config_macro Macro to check
 * @return 1 if @p config_macro is defined to 1, 0 otherwise (including
 *         if @p config_macro is not defined)
 */
#define IS_ENABLED(config_macro) Z_IS_ENABLED1(config_macro)
/* INTERNAL: the first pass above is just to expand any existing
 * macros, we need the macro value to be e.g. a literal "1" at
 * expansion time in the next macro, not "(1)", etc... Standard
 * recursive expansion does not work.
 */

/**
 * @brief Insert code depending on whether @p _flag expands to 1 or not.
 *
 * This relies on similar tricks as IS_ENABLED(), but as the result of
 * @p _flag expansion, results in either @p _if_1_code or @p
 * _else_code is expanded.
 *
 * To prevent the preprocessor from treating commas as argument
 * separators, the @p _if_1_code and @p _else_code expressions must be
 * inside brackets/parentheses: <tt>()</tt>. These are stripped away
 * during macro expansion.
 *
 * Example:
 *
 *     COND_CODE_1(CONFIG_FLAG, (uint32_t x;), (there_is_no_flag();))
 *
 * If @p CONFIG_FLAG is defined to 1, this expands to:
 *
 *     uint32_t x;
 *
 * It expands to <tt>there_is_no_flag();</tt> otherwise.
 *
 * This could be used as an alternative to:
 *
 *     #if defined(CONFIG_FLAG) && (CONFIG_FLAG == 1)
 *     #define MAYBE_DECLARE(x) uint32_t x
 *     #else
 *     #define MAYBE_DECLARE(x) there_is_no_flag()
 *     #endif
 *
 *     MAYBE_DECLARE(x);
 *
 * However, the advantage of COND_CODE_1() is that code is resolved in
 * place where it is used, while the @p \#if method defines @p
 * MAYBE_DECLARE on two lines and requires it to be invoked again on a
 * separate line. This makes COND_CODE_1() more concise and also
 * sometimes more useful when used within another macro's expansion.
 *
 * @note @p _flag can be the result of preprocessor expansion, e.g.
 *	 an expression involving <tt>NUM_VA_ARGS_LESS_1(...)</tt>.
 *	 However, @p _if_1_code is only expanded if @p _flag expands
 *	 to the integer literal 1. Integer expressions that evaluate
 *	 to 1, e.g. after doing some arithmetic, will not work.
 *
 * @param _flag evaluated flag
 * @param _if_1_code result if @p _flag expands to 1; must be in parentheses
 * @param _else_code result otherwise; must be in parentheses
 */
#define COND_CODE_1(_flag, _if_1_code, _else_code) \
	Z_COND_CODE_1(_flag, _if_1_code, _else_code)

/**
 * @brief Like COND_CODE_1() except tests if @p _flag is 0.
 *
 * This is like COND_CODE_1(), except that it tests whether @p _flag
 * expands to the integer literal 0. It expands to @p _if_0_code if
 * so, and @p _else_code otherwise; both of these must be enclosed in
 * parentheses.
 *
 * @param _flag evaluated flag
 * @param _if_0_code result if @p _flag expands to 0; must be in parentheses
 * @param _else_code result otherwise; must be in parentheses
 * @see COND_CODE_1()
 */
#define COND_CODE_0(_flag, _if_0_code, _else_code) \
	Z_COND_CODE_0(_flag, _if_0_code, _else_code)

/**
 * @brief Insert code if @p _flag is defined and equals 1.
 *
 * Like COND_CODE_1(), this expands to @p _code if @p _flag is defined to 1;
 * it expands to nothing otherwise.
 *
 * Example:
 *
 *     IF_ENABLED(CONFIG_FLAG, (uint32_t foo;))
 *
 * If @p CONFIG_FLAG is defined to 1, this expands to:
 *
 *     uint32_t foo;
 *
 * and to nothing otherwise.
 *
 * It can be considered as a more compact alternative to:
 *
 *     #if defined(CONFIG_FLAG) && (CONFIG_FLAG == 1)
 *     uint32_t foo;
 *     #endif
 *
 * @param _flag evaluated flag
 * @param _code result if @p _flag expands to 1; must be in parentheses
 */
#define IF_ENABLED(_flag, _code) \
	COND_CODE_1(_flag, _code, ())

/**
 * @brief Check if a macro has a replacement expression
 *
 * If @p a is a macro defined to a nonempty value, this will return
 * true, otherwise it will return false. It only works with defined
 * macros, so an additional @p \#ifdef test may be needed in some cases.
 *
 * This macro may be used with COND_CODE_1() and COND_CODE_0() while
 * processing <tt>__VA_ARGS__</tt> to avoid processing empty arguments.
 *
 * Note that this macro is intended to check macro names that evaluate
 * to replacement lists being empty or containing numbers or macro name
 * like tokens.
 *
 * @note Not all arguments are accepted by this macro and compilation will fail
 *	 if argument cannot be concatenated with literal constant. That will
 *	 happen if argument does not start with letter or number. Example
 *	 arguments that will fail during compilation: .arg, (arg), "arg", {arg}.
 *
 * Example:
 *
 *	#define EMPTY
 *	#define NON_EMPTY	1
 *	#undef  UNDEFINED
 *	IS_EMPTY(EMPTY)
 *	IS_EMPTY(NON_EMPTY)
 *	IS_EMPTY(UNDEFINED)
 *	#if defined(EMPTY) && IS_EMPTY(EMPTY) == true
 *	some_conditional_code
 *	#endif
 *
 * In above examples, the invocations of IS_EMPTY(...) return @p true,
 * @p false, and @p true; @p some_conditional_code is included.
 *
 * @param a macro to check for emptiness
 */
#define IS_EMPTY(a) Z_IS_EMPTY_(a, 1, 0,)

/**
 * @brief Remove empty arguments from list.
 *
 * During macro expansion, <tt>__VA_ARGS__</tt> and other preprocessor
 * generated lists may contain empty elements, e.g.:
 *
 *	#define LIST ,a,b,,d,
 *
 * Using EMPTY to show each empty element, LIST contains:
 *
 *      EMPTY, a, b, EMPTY, d
 *
 * When processing such lists, e.g. using FOR_EACH(), all empty elements
 * will be processed, and may require filtering out.
 * To make that process easier, it is enough to invoke LIST_DROP_EMPTY
 * which will remove all empty elements.
 *
 * Example:
 *
 *	LIST_DROP_EMPTY(LIST)
 *
 * expands to:
 *
 *	a, b, d
 *
 * @param ... list to be processed
 */
#define LIST_DROP_EMPTY(...) \
	Z_LIST_DROP_FIRST(FOR_EACH(Z_LIST_NO_EMPTIES, (), __VA_ARGS__))

/**
 * @brief Macro with an empty expansion
 *
 * This trivial definition is provided for readability when a macro
 * should expand to an empty result, which e.g. is sometimes needed to
 * silence checkpatch.
 *
 * Example:
 *
 *	#define LIST_ITEM(n) , item##n
 *
 * The above would cause checkpatch to complain, but:
 *
 *	#define LIST_ITEM(n) EMPTY, item##n
 *
 * would not.
 */
#define EMPTY

/**
 * @brief Get nth argument from argument list.
 *
 * @param N Argument index to fetch. Counter from 1.
 * @param ... Variable list of argments from which one argument is returned.
 *
 * @return Nth argument.
 */
#define GET_ARG_N(N, ...) _Z_GET_ARG_N(N, 1, __VA_ARGS__)

/**
 * @brief Strips n first arguments from the argument list.
 *
 * @param N Number of arguments to discard.
 * @param ... Variable list of argments.
 *
 * @return argument list without N first arguments.
 */
#define GET_ARGS_LESS_N(N, ...) _Z_GET_ARG_N(UTIL_INC(N), 0, __VA_ARGS__)

/** Expands to the first argument.
 *
 * @deprecated Use GET_ARG_N instead.
 */
#define GET_ARG1(...) GET_ARG_N(1, __VA_ARGS__)

/** Expands to the second argument.
 *
 * @deprecated Use GET_ARG_N instead.
 */
#define GET_ARG2(...) __DEPRECATED GET_ARG_N(2, __VA_ARGS__)

/** Expands to all arguments except the first one.
 *
 * @deprecated Use GET_ARGS_LESS_N instead.
 */
#define GET_ARGS_LESS_1(...) __DEPRECATED GET_ARGS_LESS_N(1, __VA_ARGS__)

/**
 * @brief Like <tt>a || b</tt>, but does evaluation and
 * short-circuiting at C preprocessor time.
 *
 * This is not the same as the binary @p || operator; in particular,
 * @p a should expand to an integer literal 0 or 1. However, @p b
 * can be any value.
 *
 * This can be useful when @p b is an expression that would cause a
 * build error when @p a is 1.
 */
#define UTIL_OR(a, b) COND_CODE_1(UTIL_BOOL(a), (a), (b))

/**
 * @brief Like <tt>a && b</tt>, but does evaluation and
 * short-circuiting at C preprocessor time.
 *
 * This is not the same as the binary @p &&, however; in particular,
 * @p a should expand to an integer literal 0 or 1. However, @p b
 * can be any value.
 *
 * This can be useful when @p b is an expression that would cause a
 * build error when @p a is 0.
 */
#define UTIL_AND(a, b) COND_CODE_1(UTIL_BOOL(a), (b), (0))

/**
 * @brief Generates a sequence of code.
 *
 * Example:
 *
 *     #define FOO(i, _) MY_PWM ## i ,
 *     { UTIL_LISTIFY(PWM_COUNT, FOO) }
 *
 * The above two lines expand to:
 *
 *    { MY_PWM0 , MY_PWM1 , }
 *
 * @param LEN The length of the sequence. Must be an integer literal less
 *            than 255.
 * @param F A macro function that accepts at least two arguments:
 *          <tt>F(i, ...)</tt>. @p F is called repeatedly in the expansion.
 *          Its first argument @p i is the index in the sequence, and
 *          the variable list of arguments passed to UTIL_LISTIFY are passed
 *          through to @p F.
 *
 * @note Calling UTIL_LISTIFY with undefined arguments has undefined
 * behavior.
 */
#define UTIL_LISTIFY(LEN, F, ...) UTIL_EVAL(UTIL_REPEAT(LEN, F, __VA_ARGS__))

/**
 * @brief Call a macro @p F on each provided argument with a given
 *        separator between each call.
 *
 * Example:
 *
 *     #define F(x) int a##x
 *     FOR_EACH(F, (;), 4, 5, 6);
 *
 * This expands to:
 *
 *     int a4;
 *     int a5;
 *     int a6;
 *
 * @param F Macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... Variable argument list. The macro @p F is invoked as
 *            <tt>F(element)</tt> for each element in the list.
 */
#define FOR_EACH(F, sep, ...) \
	Z_FOR_EACH_IDX2(NUM_VA_ARGS_LESS_1(__VA_ARGS__, _), \
			0, Z_FOR_EACH_SWALLOW_INDEX_FIXED_ARG, sep, \
			F, 0, __VA_ARGS__)

/**
 * @brief Like FOR_EACH(), but with a terminator instead of a separator,
 *        and drops empty elements from the argument list
 *
 * The @p sep argument to <tt>FOR_EACH(F, (sep), a, b)</tt> is a
 * separator which is placed between calls to @p F, like this:
 *
 *     FOR_EACH(F, (sep), a, b) // F(a) sep F(b)
 *                              //               ^^^ no sep here!
 *
 * By contrast, the @p term argument to <tt>FOR_EACH_NONEMPTY_TERM(F, (term),
 * a, b)</tt> is added after each time @p F appears in the expansion:
 *
 *     FOR_EACH_NONEMPTY_TERM(F, (term), a, b) // F(a) term F(b) term
 *                                             //                ^^^^
 *
 * Further, any empty elements are dropped:
 *
 *     FOR_EACH_NONEMPTY_TERM(F, (term), a, EMPTY, b) // F(a) term F(b) term
 *
 * This is more convenient in some cases, because FOR_EACH_NONEMPTY_TERM()
 * expands to nothing when given an empty argument list, and it's
 * often cumbersome to write a macro @p F that does the right thing
 * even when given an empty argument.
 *
 * One example is when <tt>__VA_ARGS__</tt> may or may not be empty,
 * and the results are embedded in a larger initializer:
 *
 *     #define SQUARE(x) ((x)*(x))
 *
 *     int my_array[] = {
 *             FOR_EACH_NONEMPTY_TERM(SQUARE, (,), FOO(...))
 *             FOR_EACH_NONEMPTY_TERM(SQUARE, (,), BAR(...))
 *             FOR_EACH_NONEMPTY_TERM(SQUARE, (,), BAZ(...))
 *     };
 *
 * This is more convenient than:
 *
 * 1. figuring out whether the @p FOO, @p BAR, and @p BAZ expansions
 *    are empty and adding a comma manually (or not) between FOR_EACH()
 *    calls
 * 2. rewriting SQUARE so it reacts appropriately when "x" is empty
 *    (which would be necessary if e.g. @p FOO expands to nothing)
 *
 * @param F Macro to invoke on each nonempty element of the variable
 *          arguments
 * @param term Terminator (e.g. comma or semicolon) placed after each
 *             invocation of F. Must be in parentheses; this is required
 *             to enable providing a comma as separator.
 * @param ... Variable argument list. The macro @p F is invoked as
 *            <tt>F(element)</tt> for each nonempty element in the list.
 */
#define FOR_EACH_NONEMPTY_TERM(F, term, ...)				\
	COND_CODE_0(							\
		/* are there zero non-empty arguments ? */		\
		NUM_VA_ARGS_LESS_1(LIST_DROP_EMPTY(__VA_ARGS__, _)),	\
		/* if so, expand to nothing */				\
		(),							\
		/* otherwise, expand to: */				\
		(/* FOR_EACH() on nonempty elements, */		\
			FOR_EACH(F, term, LIST_DROP_EMPTY(__VA_ARGS__))	\
			/* plus a final terminator */			\
			__DEBRACKET term				\
		))

/**
 * @brief Call macro @p F on each provided argument, with the argument's index
 *        as an additional parameter.
 *
 * This is like FOR_EACH(), except @p F should be a macro which takes two
 * arguments: <tt>F(index, variable_arg)</tt>.
 *
 * Example:
 *
 *     #define F(idx, x) int a##idx = x
 *     FOR_EACH_IDX(F, (;), 4, 5, 6);
 *
 * This expands to:
 *
 *     int a0 = 4;
 *     int a1 = 5;
 *     int a2 = 6;
 *
 * @param F Macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param ... Variable argument list. The macro @p F is invoked as
 *            <tt>F(index, element)</tt> for each element in the list.
 */
#define FOR_EACH_IDX(F, sep, ...) \
	Z_FOR_EACH_IDX2(NUM_VA_ARGS_LESS_1(__VA_ARGS__, _), \
			0, Z_FOR_EACH_SWALLOW_FIXED_ARG, sep, \
			F, 0, __VA_ARGS__)

/**
 * @brief Call macro @p F on each provided argument, with an additional fixed
 *	  argument as a parameter.
 *
 * This is like FOR_EACH(), except @p F should be a macro which takes two
 * arguments: <tt>F(variable_arg, fixed_arg)</tt>.
 *
 * Example:
 *
 *     static void func(int val, void *dev);
 *     FOR_EACH_FIXED_ARG(func, (;), dev, 4, 5, 6);
 *
 * This expands to:
 *
 *     func(4, dev);
 *     func(5, dev);
 *     func(6, dev);
 *
 * @param F Macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            this is required to enable providing a comma as separator.
 * @param fixed_arg Fixed argument passed to @p F as the second macro parameter.
 * @param ... Variable argument list. The macro @p F is invoked as
 *            <tt>F(element, fixed_arg)</tt> for each element in the list.
 */
#define FOR_EACH_FIXED_ARG(F, sep, fixed_arg, ...) \
	Z_FOR_EACH_IDX2(NUM_VA_ARGS_LESS_1(__VA_ARGS__, _), \
			0, Z_FOR_EACH_SWALLOW_INDEX, sep, \
			F, fixed_arg, __VA_ARGS__)

/**
 * @brief Calls macro @p F for each variable argument with an index and fixed
 *        argument
 *
 * This is like the combination of FOR_EACH_IDX() with FOR_EACH_FIXED_ARG().
 *
 * Example:
 *
 *     #define F(idx, x, fixed_arg) int fixed_arg##idx = x
 *     FOR_EACH_IDX_FIXED_ARG(F, (;), a, 4, 5, 6);
 *
 * This expands to:
 *
 *     int a0 = 4;
 *     int a1 = 5;
 *     int a2 = 6;
 *
 * @param F Macro to invoke
 * @param sep Separator (e.g. comma or semicolon). Must be in parentheses;
 *            This is required to enable providing a comma as separator.
 * @param fixed_arg Fixed argument passed to @p F as the third macro parameter.
 * @param ... Variable list of arguments. The macro @p F is invoked as
 *            <tt>F(index, element, fixed_arg)</tt> for each element in
 *            the list.
 */
#define FOR_EACH_IDX_FIXED_ARG(F, sep, fixed_arg, ...) \
	Z_FOR_EACH_IDX2(NUM_VA_ARGS_LESS_1(__VA_ARGS__, _), \
			0, Z_FOR_EACH_SWALLOW_NOTHING, sep, \
			F, fixed_arg, __VA_ARGS__)

/**
 * @brief Number of arguments in the variable arguments list minus one.
 *
 * @param ... List of arguments
 * @return  Number of variadic arguments in the argument list, minus one
 */
#define NUM_VA_ARGS_LESS_1(...) \
	NUM_VA_ARGS_LESS_1_IMPL(__VA_ARGS__, 63, 62, 61, \
	60, 59, 58, 57, 56, 55, 54, 53, 52, 51,		 \
	50, 49, 48, 47, 46, 45, 44, 43, 42, 41,		 \
	40, 39, 38, 37, 36, 35, 34, 33, 32, 31,		 \
	30, 29, 28, 27, 26, 25, 24, 23, 22, 21,		 \
	20, 19, 18, 17, 16, 15, 14, 13, 12, 11,		 \
	10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, ~)

/**
 * @brief Mapping macro that pastes results together
 *
 * This is similar to FOR_EACH() in that it invokes a macro repeatedly
 * on each element of <tt>__VA_ARGS__</tt>. However, unlike FOR_EACH(),
 * MACRO_MAP_CAT() pastes the results together into a single token.
 *
 * For example, with this macro FOO:
 *
 *     #define FOO(x) item_##x##_
 *
 * <tt>MACRO_MAP_CAT(FOO, a, b, c),</tt> expands to the token:
 *
 *     item_a_item_b_item_c_
 *
 * @param ... Macro to expand on each argument, followed by its
 *            arguments. (The macro should take exactly one argument.)
 * @return The results of expanding the macro on each argument, all pasted
 *         together
 */
#define MACRO_MAP_CAT(...) MACRO_MAP_CAT_(__VA_ARGS__)

/**
 * @brief Mapping macro that pastes a fixed number of results together
 *
 * Similar to @ref MACRO_MAP_CAT(), but expects a fixed number of
 * arguments. If more arguments are given than are expected, the rest
 * are ignored.
 *
 * @param N   Number of arguments to map
 * @param ... Macro to expand on each argument, followed by its
 *            arguments. (The macro should take exactly one argument.)
 * @return The results of expanding the macro on each argument, all pasted
 *         together
 */
#define MACRO_MAP_CAT_N(N, ...) MACRO_MAP_CAT_N_(N, __VA_ARGS__)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_UTIL_H_ */
