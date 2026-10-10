/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Precision timing shell registry.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_SHELL_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_SHELL_H_

#include <zephyr/precision_timing/precision_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Timing Shell APIs
 * @defgroup precision_timing_shell Precision Timing Shell
 * @since 4.5
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/** Maximum supported precision timing shell instance name length. */
#define PRECISION_TIMING_SHELL_NAME_MAX 31

/**
 * @brief Register a precision clock with the precision timing shell.
 *
 * The registry copies @p name. Names are case-sensitive and may contain only
 * ASCII letters, digits, '.', '-', and '_'. The shell help options "-h" and
 * "--help" are reserved and cannot be registered. The clock remains owned by
 * the caller and must remain valid until @ref precision_timing_shell_unregister
 * returns.
 *
 * @param name Unique shell instance name.
 * @param precision_clk Clock exposed by the shell.
 *
 * @retval 0 on success.
 * @retval -EINVAL if the name or clock is invalid.
 * @retval -EEXIST if @p name is already registered.
 * @retval -ENOSPC if the fixed-capacity registry is full.
 */
int precision_timing_shell_register(const char *name, const struct precision_clock *precision_clk);

/**
 * @brief Unregister a precision timing shell instance synchronously.
 *
 * The name becomes unavailable to new shell commands before this function
 * waits for in-flight shell operations to complete. After this function
 * returns, the caller may release the clock object.
 *
 * This function and @ref precision_timing_shell_register must be called from
 * thread context.
 *
 * @param name Registered shell instance name.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p name is null or invalid.
 * @retval -ENOENT if @p name is not registered.
 */
int precision_timing_shell_unregister(const char *name);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_TIMING_SHELL_H_ */
