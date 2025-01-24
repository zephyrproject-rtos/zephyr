/*
 * Copyright (c) 2025 Google, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_H_
#define ZEPHYR_TESTSUITE_INCLUDE_TEST_TOOLCHAIN_H_

#include <zephyr/toolchain.h>

#if defined(__llvm__) || (defined(_LINKER) && defined(__LLD_LINKER_CMD__))
#include <zephyr/test_toolchain/llvm.h>
#elif defined(__GNUC__) || (defined(_LINKER) && defined(__GCC_LINKER_CMD__))
#include <zephyr/test_toolchain/gcc.h>
#endif

/**
 * @def TOOLCHAIN_WARNING_ALLOC_SIZE_LARGER_THAN
 * @brief Toolchain-specific warning for allocations larger than a given size.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_ALLOC_SIZE_LARGER_THAN
#define TOOLCHAIN_WARNING_ALLOC_SIZE_LARGER_THAN
#endif

/**
 * @def TOOLCHAIN_WARNING_DANGLING_POINTER
 * @brief Toolchain-specific warning for dangling pointers.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_DANGLING_POINTER
#define TOOLCHAIN_WARNING_DANGLING_POINTER
#endif

/**
 * @def TOOLCHAIN_WARNING_FORMAT_TRUNCATION
 * @brief Toolchain-specific warning for format truncation.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_FORMAT_TRUNCATION
#define TOOLCHAIN_WARNING_FORMAT_TRUNCATION
#endif

/**
 * @def TOOLCHAIN_WARNING_INFINITE_RECURSION
 * @brief Toolchain-specific warning for infinite recursion.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_INFINITE_RECURSION
#define TOOLCHAIN_WARNING_INFINITE_RECURSION
#endif

/**
 * @def TOOLCHAIN_WARNING_INTEGER_OVERFLOW
 * @brief Toolchain-specific warning for integer overflow.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_INTEGER_OVERFLOW
#define TOOLCHAIN_WARNING_INTEGER_OVERFLOW
#endif

/**
 * @def TOOLCHAIN_WARNING_OVERFLOW
 * @brief Toolchain-specific warning for integer overflow.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_OVERFLOW
#define TOOLCHAIN_WARNING_OVERFLOW
#endif

/**
 * @def TOOLCHAIN_WARNING_PRAGMAS
 * @brief Toolchain-specific warning for unknown pragmas.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_PRAGMAS
#define TOOLCHAIN_WARNING_PRAGMAS
#endif

/**
 * @def TOOLCHAIN_WARNING_SIZEOF_ARRAY_DECAY
 * @brief Toolchain-specific warning for sizeof array decay.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_SIZEOF_ARRAY_DECAY
#define TOOLCHAIN_WARNING_SIZEOF_ARRAY_DECAY
#endif

/**
 * @def TOOLCHAIN_WARNING_STRINGOP_OVERFLOW
 * @brief Toolchain-specific warning for stringop overflow.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_STRINGOP_OVERFLOW
#define TOOLCHAIN_WARNING_STRINGOP_OVERFLOW
#endif

/**
 * @def TOOLCHAIN_WARNING_STRINGOP_TRUNCATION
 * @brief Toolchain-specific warning for stringop truncation.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_STRINGOP_TRUNCATION
#define TOOLCHAIN_WARNING_STRINGOP_TRUNCATION
#endif

/**
 * @def TOOLCHAIN_WARNING_UNUSED_FUNCTION
 * @brief Toolchain-specific warning for unused function.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_UNUSED_FUNCTION
#define TOOLCHAIN_WARNING_UNUSED_FUNCTION
#endif

#endif
