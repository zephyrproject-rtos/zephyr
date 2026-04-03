/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/net/net_timeout.h>
#include <zephyr/sys_clock.h>

void net_timeout_set(struct net_timeout *timeout,
		     uint32_t lifetime,
		     uint32_t now)
{
	uint64_t expire_timeout;

	timeout->timer_start = now;

	/* Highly unlikely, but a zero timeout isn't correctly handled by the
	 * standard calculation.
	 */
	if (lifetime == 0U) {
		timeout->wrap_counter = 0;
		timeout->timer_timeout = 0;
		return;
	}

	expire_timeout = (uint64_t)MSEC_PER_SEC * (uint64_t)lifetime;
	timeout->wrap_counter = expire_timeout /
		(uint64_t)NET_TIMEOUT_MAX_VALUE;
	timeout->timer_timeout = expire_timeout -
		(uint64_t)NET_TIMEOUT_MAX_VALUE *
		(uint64_t)timeout->wrap_counter;

	/* The implementation requires that the fractional timeout be zero
	 * only when the timeout has completed, so if the residual is zero
	 * copy over one timeout from the wrap.
	 */
	if (timeout->timer_timeout == 0U) {
		timeout->timer_timeout = NET_TIMEOUT_MAX_VALUE;
		timeout->wrap_counter -= 1;
	}
}

int64_t net_timeout_deadline(const struct net_timeout *timeout,
			     int64_t now)
{
	uint64_t start;
	uint64_t deadline;

	/* Reconstruct the full-precision start time assuming that the full
	 * precision start time is less than 2^32 ticks in the past.
	 */
	start = (uint64_t)now;
	start -= (uint32_t)now - timeout->timer_start;

	/* Offset the start time by the full precision remaining delay. */
	deadline = start + timeout->timer_timeout;
	deadline += (uint64_t)NET_TIMEOUT_MAX_VALUE
		* (uint64_t)timeout->wrap_counter;

	return (int64_t)deadline;
}

uint32_t net_timeout_remaining(const struct net_timeout *timeout,
			       uint32_t now)
{
	int64_t ret = timeout->timer_timeout;

	ret += timeout->wrap_counter * (uint64_t)NET_TIMEOUT_MAX_VALUE;
	ret -= (int64_t)(int32_t)(now - timeout->timer_start);
	if (ret <= 0) {
		return 0;
	}

	return (uint32_t)((uint64_t)ret / MSEC_PER_SEC);
}

uint32_t net_timeout_evaluate(struct net_timeout *timeout,
			      uint32_t now)
{
	uint32_t elapsed;
	uint32_t last_delay;
	int32_t remains;
	bool wraps;

	/* Time since last evaluation or set. */
	elapsed = now - timeout->timer_start;

	/* The delay used the last time this was evaluated. */
	wraps = (timeout->wrap_counter > 0U);
	last_delay = wraps
		? NET_TIMEOUT_MAX_VALUE
		: timeout->timer_timeout;

	/* Time remaining until completion of the last delay. */
	remains = (int32_t)(last_delay - elapsed);

	/* If the deadline for the next event hasn't been reached yet just
	 * return the remaining time.
	 */
	if (remains > 0) {
		return (uint32_t)remains;
	}

	/* Deadline has been reached.  If we're not wrapping we've completed
	 * the last portion of the full timeout, so return zero to indicate
	 * the timeout has completed.
	 */
	if (!wraps) {
		return 0U;
	}

	/* There's more to do.  We need to update timer_start to correspond to
	 * now, then reduce the remaining time by the elapsed time.  We know
	 * that's at least NET_TIMEOUT_MAX_VALUE, and can apply the
	 * reduction by decrementing the wrap count.
	 */
	timeout->timer_start = now;
	elapsed -= NET_TIMEOUT_MAX_VALUE;
	timeout->wrap_counter -= 1;

	/* The residual elapsed must reduce timer_timeout, which is capped at
	 * NET_TIMEOUT_MAX_VALUE.  But if subtracting would reduce the
	 * counter to zero or go negative we need to reduce the wrap
	 * counter once more and add the residual to the counter, so the
	 * counter remains positive.
	 */
	if (timeout->timer_timeout > elapsed) {
		timeout->timer_timeout -= elapsed;
	} else {
		timeout->timer_timeout += NET_TIMEOUT_MAX_VALUE - elapsed;
		timeout->wrap_counter -= 1U;
	}

	return (timeout->wrap_counter == 0U)
		? timeout->timer_timeout
		: NET_TIMEOUT_MAX_VALUE;
}
