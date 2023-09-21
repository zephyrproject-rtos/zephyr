/*
 * Copyright (c) 2010-2014, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Macros to abstract toolchain specific capabilities
 *
 * This file contains various macros to abstract compiler capabilities that
 * utilize toolchain specific attributes and/or pragmas.
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_H_

/**
 * @def HAS_BUILTIN(x)
 * @brief Check if the compiler supports the built-in function \a x.
 *
 * This macro is for use with conditional compilation to enable code using a
 * builtin function that may not be available in every compiler.
 */
#ifdef __has_builtin
#define HAS_BUILTIN(x) __has_builtin(x)
#else
/*
 * The compiler doesn't provide the __has_builtin() macro, so instead we depend
 * on the toolchain-specific headers to define HAS_BUILTIN_x for the builtins
 * supported.
 */
#define HAS_BUILTIN(x) HAS_BUILTIN_##x
#endif

#if defined(__TOOLCHAIN_CUSTOM__)
/* This include line exists for off-tree definitions of compilers,
 * and therefore this header is not meant to exist in-tree
 */
#include <toolchain/other.h>
#elif defined(__XCC__)
#include <zephyr/toolchain/xcc.h>
#elif defined(__CCAC__)
#include <zephyr/toolchain/mwdt.h>
#elif defined(__ARMCOMPILER_VERSION)
#include <zephyr/toolchain/armclang.h>
#elif defined(__llvm__) || (defined(_LINKER) && defined(__LLD_LINKER_CMD__))
#include <zephyr/toolchain/llvm.h>
#elif defined(__GNUC__) || (defined(_LINKER) && defined(__GCC_LINKER_CMD__))
#include <zephyr/toolchain/gcc.h>
#else
#error "Invalid/unknown toolchain configuration"
#endif

/**
 * @def __noasan
 * @brief Disable address sanitizer
 *
 * When used in the definiton of a symbol, prevents that symbol (be it
 * a function or data) from being instrumented by the address
 * sanitizer feature of the compiler.  Most commonly, this is used to
 * prevent padding around data that will be treated specially by the
 * Zephyr link (c.f. SYS_INIT records, STRUCT_SECTION_ITERABLE
 * definitions) in ways that don't understand the guard padding.
 */
#ifndef __noasan
#define __noasan /**/
#endif

/**
 * @def TOOLCHAIN_GCC_VERSION
 * @brief GCC version in xxyyzz for xx.yy.zz. Zero if not GCC compatible.
 */
#ifndef TOOLCHAIN_GCC_VERSION
#define TOOLCHAIN_GCC_VERSION 0
#endif

/**
 * @def TOOLCHAIN_CLANG_VERSION
 * @brief Clang version in xxyyzz for xx.yy.zz. Zero if not Clang compatible.
 */
#ifndef TOOLCHAIN_CLANG_VERSION
#define TOOLCHAIN_CLANG_VERSION 0
#endif

/**
 * @def TOOLCHAIN_HAS_PRAGMA_DIAG
 * @brief Indicate if toolchain supports \#pragma diagnostics.
 */
#ifndef TOOLCHAIN_HAS_PRAGMA_DIAG
#define TOOLCHAIN_HAS_PRAGMA_DIAG 0
#endif

/**
 * @def TOOLCHAIN_HAS_C_GENERIC
 * @brief Indicate if toolchain supports C Generic.
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
/* _Generic is introduced in C11, so it is supported. */
# ifdef TOOLCHAIN_HAS_C_GENERIC
#  undef TOOLCHAIN_HAS_C_GENERIC
# endif
# define TOOLCHAIN_HAS_C_GENERIC 1
#else
# ifndef TOOLCHAIN_HAS_C_GENERIC
#  define TOOLCHAIN_HAS_C_GENERIC 0
# endif
#endif

/**
 * @def TOOLCHAIN_HAS_C_AUTO_TYPE
 * @brief Indicate if toolchain supports C __auto_type.
 */
#ifndef TOOLCHAIN_HAS_C_AUTO_TYPE
#define TOOLCHAIN_HAS_C_AUTO_TYPE 0
#endif

/**
 * @def TOOLCHAIN_HAS_ZLA
 * @brief Indicate if toolchain supports Zero Length Arrays.
 */
#ifndef TOOLCHAIN_HAS_ZLA
#define TOOLCHAIN_HAS_ZLA 0
#endif

/**
 * @def TOOLCHAIN_IGNORE_WSHADOW_BEGIN
 * @brief Begin of block to ignore -Wshadow.
 *
 * To be used inside another macro.
 * Only for toolchain supporting _Pragma("GCC diagnostic ...").
 */
#ifndef TOOLCHAIN_IGNORE_WSHADOW_BEGIN
#define TOOLCHAIN_IGNORE_WSHADOW_BEGIN
#endif

/**
 * @def TOOLCHAIN_IGNORE_WSHADOW_END
 * @brief End of block to ignore -Wshadow.
 *
 * To be used inside another macro.
 * Only for toolchain supporting _Pragma("GCC diagnostic ...").
 */
#ifndef TOOLCHAIN_IGNORE_WSHADOW_END
#define TOOLCHAIN_IGNORE_WSHADOW_END
#endif

/*
 * Ensure that __BYTE_ORDER__ and related preprocessor definitions are defined,
 * and that they match the Kconfig option that is used in the code itself to
 * check for endianness.
 */
#ifndef _LINKER
#if !defined(__BYTE_ORDER__) || !defined(__ORDER_BIG_ENDIAN__) || \
    !defined(__ORDER_LITTLE_ENDIAN__)

/*
 * Displaying values unfortunately requires #pragma message which can't
 * be taken for granted + STRINGIFY() which is not available in this .h
 * file.
 */
#error "At least one byte _ORDER_ macro is not defined"

#else

#if (defined(CONFIG_BIG_ENDIAN) && (__BYTE_ORDER__ != __ORDER_BIG_ENDIAN__)) || \
    (defined(CONFIG_LITTLE_ENDIAN) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__))

#  error "Kconfig/toolchain endianness mismatch:"

#  if (__BYTE_ORDER__ != __ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__)
#    error "Unknown __BYTE_ORDER__ value"
#  else
#    ifdef CONFIG_BIG_ENDIAN
#      error "CONFIG_BIG_ENDIAN but __ORDER_LITTLE_ENDIAN__"
#    endif
#    ifdef CONFIG_LITTLE_ENDIAN
#      error "CONFIG_LITTLE_ENDIAN but __ORDER_BIG_ENDIAN__"
#   endif
# endif

#endif  /* Endianness mismatch */

#endif /* all _ORDER_ macros defined */

#endif /* !_LINKER */

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_H_ */
