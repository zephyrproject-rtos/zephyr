/** @file
 * @brief Network timer with wrap around
 *
 * Timer that runs longer than about 49 days needs to calculate wraps.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_NET_TIMEOUT_H_
#define ZEPHYR_INCLUDE_NET_NET_TIMEOUT_H_

/**
 * @brief Network long timeout primitives and helpers
 * @defgroup net_timeout Network long timeout primitives and helpers
 * @ingroup networking
 * @{
 */

#include <string.h>
#include <zephyr/types.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Let the max timeout be 100 ms lower because of
 * possible rounding in delayed work implementation.
 */
#define NET_TIMEOUT_MAX_VALUE ((u32_t)(INT32_MAX - 100))

struct net_timeout {
	/** Used to track timers */
	sys_snode_t node;

	/** Address lifetime timer start time */
	u32_t timer_start;

	/** Address lifetime timer timeout in milliseconds. Note that this
	 * value is signed as k_delayed_work_submit() only supports signed
	 * delay value.
	 */
	s32_t timer_timeout;

	/** Timer wrap count. Used if the timer timeout is larger than
	 * about 24 days. The reason we need to track wrap arounds, is
	 * that the timer timeout used in k_delayed_work_submit() is
	 * 32-bit signed value and the resolution is 1ms.
	 */
	s32_t wrap_counter;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_NET_NET_TIMEOUT_H_ */
