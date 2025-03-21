/*
 * Copyright (c) 2025 IAR Systems AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_TOOLCHAIN_IAR_H_
#define ZEPHYR_INCLUDE_TOOLCHAIN_IAR_H_

#define TOOLCHAIN_HAS_PRAGMA_DIAG

#define _TOOLCHAIN_DISABLE_WARNING(warning) TOOLCHAIN_PRAGMA(diag_suppress = warning)
#define _TOOLCHAIN_ENABLE_WARNING(warning)  TOOLCHAIN_PRAGMA(diag_default = warning)

#define TOOLCHAIN_DISABLE_WARNING(warning) _TOOLCHAIN_DISABLE_WARNING(warning)
#define TOOLCHAIN_ENABLE_WARNING(warning)  _TOOLCHAIN_ENABLE_WARNING(warning)

#define TOOLCHAIN_DISABLE_IAR_WARNING(warning) _TOOLCHAIN_DISABLE_WARNING(warning)
#define TOOLCHAIN_ENABLE_IAR_WARNING(warning)  _TOOLCHAIN_ENABLE_WARNING(warning)

/* Generic warnings */


/**
 * @def TOOLCHAIN_WARNING_ADDRESS_OF_PACKED_MEMBER
 * @brief Toolchain-specific warning for taking the address of a packed member.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_ADDRESS_OF_PACKED_MEMBER
#define TOOLCHAIN_WARNING_ADDRESS_OF_PACKED_MEMBER Pe001
#endif

/**
 * @def TOOLCHAIN_WARNING_ARRAY_BOUNDS
 * @brief Toolchain-specific warning for array bounds violations.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_ARRAY_BOUNDS
#define TOOLCHAIN_WARNING_ARRAY_BOUNDS Pe001
#endif

/**
 * @def TOOLCHAIN_WARNING_ATTRIBUTES
 * @brief Toolchain-specific warning for unknown attributes.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_ATTRIBUTES
#define TOOLCHAIN_WARNING_ATTRIBUTES Pe1097
#endif

/**
 * @def TOOLCHAIN_WARNING_DELETE_NON_VIRTUAL_DTOR
 * @brief Toolchain-specific warning for deleting a pointer to an object
 * with a non-virtual destructor.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_DELETE_NON_VIRTUAL_DTOR
#define TOOLCHAIN_WARNING_DELETE_NON_VIRTUAL_DTOR Pe001
#endif

/**
 * @def TOOLCHAIN_WARNING_EXTRA
 * @brief Toolchain-specific warning for extra warnings.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_EXTRA
#define TOOLCHAIN_WARNING_EXTRA Pe001
#endif

/**
 * @def TOOLCHAIN_WARNING_NONNULL
 * @brief Toolchain-specific warning for null pointer arguments to functions marked with "nonnull".
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_NONNULL
#define TOOLCHAIN_WARNING_NONNULL Pe001
#endif

/**
 * @def TOOLCHAIN_WARNING_POINTER_ARITH
 * @brief Toolchain-specific warning for pointer arithmetic.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_POINTER_ARITH
#define TOOLCHAIN_WARNING_POINTER_ARITH Pe1143
#endif

/**
 * @def TOOLCHAIN_WARNING_SHADOW
 * @brief Toolchain-specific warning for shadow variables.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_SHADOW
#define TOOLCHAIN_WARNING_SHADOW Pe001
#endif

/**
 * @def TOOLCHAIN_WARNING_UNUSED_LABEL
 * @brief Toolchain-specific warning for unused labels.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#ifndef TOOLCHAIN_WARNING_UNUSED_LABEL
#define TOOLCHAIN_WARNING_UNUSED_LABEL Pe001
#endif

/**
 * @def TOOLCHAIN_WARNING_UNUSED_VARIABLE
 * @brief Toolchain-specific warning for unused variables.
 *
 * Use this as an argument to the @ref TOOLCHAIN_DISABLE_WARNING and
 * @ref TOOLCHAIN_ENABLE_WARNING family of macros.
 */
#define TOOLCHAIN_WARNING_UNUSED_VARIABLE Pe001

#define TOOLCHAIN_WARNING_UNUSED_FUNCTION Pe001

#ifdef __ICCARM__
#include "iar/iccarm.h"
#endif
#ifdef __ICCRISCV__
#include "iar/iccriscv.h"
#endif

#endif /* ZEPHYR_INCLUDE_TOOLCHAIN_ICCARM_H_ */
