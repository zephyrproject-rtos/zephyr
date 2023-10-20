/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 * Copyright (c) 2023 Florian Grandel, Zephyr Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_SYS_TIME_H_
#define ZEPHYR_INCLUDE_SYS_TIME_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file
 * @brief Types needed to represent and convert time in general.
 *
 * @defgroup timeutil_general_time_apis General Time Representation and Conversion Helpers
 * @ingroup timeutil_apis
 *
 * @brief Various helper APIs for representing and converting between time
 * units.
 *
 * @{
 */

/**
 * @brief System-wide macro to denote "forever" in milliseconds
 *
 * Usage of this macro is limited to APIs that want to expose a timeout value
 * that can optionally be unlimited, or "forever".  This macro can not be fed
 * into kernel functions or macros directly. Use @ref SYS_TIMEOUT_MS instead.
 */
#define SYS_FOREVER_MS (-1)

/**
 * @brief System-wide macro to denote "forever" in microseconds
 *
 * See @ref SYS_FOREVER_MS.
 */
#define SYS_FOREVER_US (-1)

/** @brief System-wide macro to convert milliseconds to kernel timeouts */
#define SYS_TIMEOUT_MS(ms) Z_TIMEOUT_TICKS((ms) == SYS_FOREVER_MS ? \
					   K_TICKS_FOREVER : Z_TIMEOUT_MS_TICKS(ms))

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SYS_TIME_H_ */
