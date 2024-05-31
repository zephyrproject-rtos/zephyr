/** @file
 * @brief Network timer with wrap around
 *
 * Timer that runs longer than about 49 days needs to calculate wraps.
 */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
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
#include <stdbool.h>
#include <limits.h>
#include <zephyr/types.h>
#include <zephyr/sys/slist.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Divisor used to support ms resolution timeouts.
 *
 * Because delays are processed in work queues which are not invoked
 * synchronously with clock changes we need to be able to detect timeouts
 * after they occur, which requires comparing "deadline" to "now" with enough
 * "slop" to handle any observable latency due to "now" advancing past
 * "deadline".
 *
 * The simplest solution is to use the native conversion of the well-defined
 * 32-bit unsigned difference to a 32-bit signed difference, which caps the
 * maximum delay at INT32_MAX.  This is compatible with the standard mechanism
 * for detecting completion of deadlines that do not overflow their
 * representation.
 */
#define NET_TIMEOUT_MAX_VALUE ((uint32_t)INT32_MAX)

/** Generic struct for handling network timeouts.
 *
 * Except for the linking node, all access to state from these objects must go
 * through the defined API.
 */
struct net_timeout {
	/** Used to link multiple timeouts that share a common timer infrastructure.
	 *
	 * For examples a set of related timers may use a single delayed work
	 * structure, which is always scheduled at the shortest time to a
	 * timeout event.
	 */
	sys_snode_t node;

	/** Time at which the timer was last set.
	 *
	 * This usually corresponds to the low 32 bits of k_uptime_get().
	 */
	uint32_t timer_start;

	/** Portion of remaining timeout that does not exceed
	 * NET_TIMEOUT_MAX_VALUE.
	 *
	 * This value is updated in parallel with timer_start and wrap_counter
	 * by net_timeout_evaluate().
	 */
	uint32_t timer_timeout;

	/** Timer wrap count.
	 *
	 * This tracks multiples of NET_TIMEOUT_MAX_VALUE milliseconds that
	 * have yet to pass.  It is also updated along with timer_start and
	 * wrap_counter by net_timeout_evaluate().
	 */
	uint32_t wrap_counter;
};

/** @brief Configure a network timeout structure.
 *
 * @param timeout a pointer to the timeout state.
 *
 * @param lifetime the duration of the timeout in seconds.
 *
 * @param now the time at which the timeout started counting down, in
 * milliseconds.  This is generally a captured value of k_uptime_get_32().
 */
void net_timeout_set(struct net_timeout *timeout,
		     uint32_t lifetime,
		     uint32_t now);

/** @brief Return the 64-bit system time at which the timeout will complete.
 *
 * @note Correct behavior requires invocation of net_timeout_evaluate() at its
 * specified intervals.
 *
 * @param timeout state a pointer to the timeout state, initialized by
 * net_timeout_set() and maintained by net_timeout_evaluate().
 *
 * @param now the full-precision value of k_uptime_get() relative to which the
 * deadline will be calculated.
 *
 * @return the value of k_uptime_get() at which the timeout will expire.
 */
int64_t net_timeout_deadline(const struct net_timeout *timeout,
			     int64_t now);

/** @brief Calculate the remaining time to the timeout in whole seconds.
 *
 * @note This function rounds the remaining time down, i.e. if the timeout
 * will occur in 3500 milliseconds the value 3 will be returned.
 *
 * @note Correct behavior requires invocation of net_timeout_evaluate() at its
 * specified intervals.
 *
 * @param timeout a pointer to the timeout state
 *
 * @param now the time relative to which the estimate of remaining time should
 * be calculated.  This should be recently captured value from
 * k_uptime_get_32().
 *
 * @retval 0 if the timeout has completed.
 * @retval positive the remaining duration of the timeout, in seconds.
 */
uint32_t net_timeout_remaining(const struct net_timeout *timeout,
			       uint32_t now);

/** @brief Update state to reflect elapsed time and get new delay.
 *
 * This function must be invoked periodically to (1) apply the effect of
 * elapsed time on what remains of a total delay that exceeded the maximum
 * representable delay, and (2) determine that either the timeout has
 * completed or that the infrastructure must wait a certain period before
 * checking again for completion.
 *
 * @param timeout a pointer to the timeout state
 *
 * @param now the time relative to which the estimate of remaining time should
 * be calculated.  This should be recently captured value from
 * k_uptime_get_32().
 *
 * @retval 0 if the timeout has completed
 * @retval positive the maximum delay until the state of this timeout should
 * be re-evaluated, in milliseconds.
 */
uint32_t net_timeout_evaluate(struct net_timeout *timeout,
			      uint32_t now);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_NET_NET_TIMEOUT_H_ */
