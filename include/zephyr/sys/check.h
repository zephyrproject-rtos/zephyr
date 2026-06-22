/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for configurable error checking.
 * @ingroup sys_check_apis
 */

#ifndef ZEPHYR_INCLUDE_SYS_CHECK_H_
#define ZEPHYR_INCLUDE_SYS_CHECK_H_

#include <zephyr/sys/__assert.h>

/**
 * @defgroup sys_check_apis Error checking
 * @ingroup os_services
 * @{
 */

/**
 * @brief Guard error-handling code with a configurable check.
 *
 * Wrap a condition that evaluates to true when an error is detected, and a statement or block to
 * run when that condition holds.
 *
 * @code{.c}
 * CHECKIF(dev == NULL) {
 *     return -EINVAL;
 * }
 * @endcode
 *
 * The behavior is configurable using Kconfig:
 *
 * - @kconfig{CONFIG_RUNTIME_ERROR_CHECKS} (default) expands to <tt>if (expr)</tt>
 * - @kconfig{CONFIG_ASSERT_ON_ERRORS} asserts, the guarded block is not run
 * - @kconfig{CONFIG_NO_RUNTIME_CHECKS} checks are omitted at compile time
 *
 * @param expr Error condition expression.
 */
#if defined(CONFIG_ASSERT_ON_ERRORS)
#define CHECKIF(expr) \
	__ASSERT_NO_MSG(!(expr));   \
	if (0)
#elif defined(CONFIG_NO_RUNTIME_CHECKS)
#define CHECKIF(...) \
	if (0)
#else
#define CHECKIF(expr) \
	if (expr)
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_SYS_CHECK_H_ */
