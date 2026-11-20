/*
 * Copyright (c) 2010-2014 Wind River Systems, Inc.
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef NSI_COMMON_SRC_INCL_NSI_UTILS_H
#define NSI_COMMON_SRC_INCL_NSI_UTILS_H

/* Remove brackets from around a single argument: */
#define NSI_DEBRACKET(...) __VA_ARGS__

#define _NSI_STRINGIFY(x) #x
#define NSI_STRINGIFY(s) _NSI_STRINGIFY(s)

/* concatenate the values of the arguments into one */
#define NSI_DO_CONCAT(x, y) x ## y
#define NSI_CONCAT(x, y) NSI_DO_CONCAT(x, y)

#define NSI_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define NSI_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define NSI_ARRAY_SIZE(array) (sizeof(array) / sizeof((array)[0]))

#ifndef NSI_ARG_UNUSED
#define NSI_ARG_UNUSED(x) (void)(x)
#endif

#define NSI_CODE_UNREACHABLE __builtin_unreachable()

#define NSI_FUNC_NORETURN __attribute__((__noreturn__))
#define NSI_WEAK __attribute__((__weak__))
#define NSI_INLINE static __attribute__((__always_inline__)) inline

#if defined(__clang__)
  /* The address sanitizer in llvm adds padding (redzones) after data
   * But for those we are re-grouping using the linker script
   * we cannot have that extra padding as we intend to iterate over them
   */
#define NSI_NOASAN __attribute__((no_sanitize("address")))
#else
#define NSI_NOASAN
#endif

/*
 * Place a symbol in a named data or text section.
 *
 * On Mach-O the section name is the section part of a "__DATA,<sec>" or
 * "__TEXT,<sec>" specifier, at most 16 characters, and the section is marked so
 * the linker does not dead strip it. On ELF the name is used as is and keeping
 * the symbol is up to the linker script, as before.
 */
#if defined(__APPLE__)
#define NSI_SECTION_DATA(sec) \
	__attribute__((__used__)) \
	__attribute__((__section__("__DATA," sec ",regular,no_dead_strip")))
#define NSI_SECTION_TEXT(sec) \
	__attribute__((__used__)) \
	__attribute__((__section__("__TEXT," sec ",regular,pure_instructions+no_dead_strip")))
#else
#define NSI_SECTION_DATA(sec) __attribute__((__section__(sec)))
#define NSI_SECTION_TEXT(sec) __attribute__((__section__(sec)))
#endif

#endif /* NSI_COMMON_SRC_INCL_NSI_UTILS_H */
