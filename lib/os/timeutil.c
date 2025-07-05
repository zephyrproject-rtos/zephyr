/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/time_units.h>
#include <zephyr/sys/util.h>

void timespec_from_timeout(k_timeout_t timeout, struct timespec *ts)
{
	__ASSERT_NO_MSG(ts != NULL);
	__ASSERT_NO_MSG(Z_IS_TIMEOUT_RELATIVE(timeout) ||
			(IS_ENABLED(CONFIG_TIMEOUT_64BIT) && K_TIMEOUT_EQ(timeout, K_FOREVER)));

	if (K_TIMEOUT_EQ(timeout, K_FOREVER)) {
		/* duration == K_TICKS_FOREVER ticks */
		*ts = K_TS_FOREVER;
	} else if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		/* duration <= 0 ticks */
		*ts = K_TS_NO_WAIT;
	} else {
		*ts = (struct timespec){
			.tv_sec =
				(time_t)((uint64_t)timeout.ticks / CONFIG_SYS_CLOCK_TICKS_PER_SEC),
			.tv_nsec = (long)K_TICKS_TO_NSECS(timeout.ticks) % NSEC_PER_SEC,
		};
	}

	__ASSERT_NO_MSG(timespec_is_valid(ts));
}

static k_timeout_t timespec_timeout_rem(const struct timespec *ts, struct timespec *rem,
					k_timeout_t timeout)
{
	if (rem != NULL) {
		timespec_from_timeout(timeout, rem);
		timespec_sub(rem, ts);
		timespec_negate(rem);
	}

	return timeout;
}

k_timeout_t timespec_to_timeout_rem(const struct timespec *ts, struct timespec *rem)
{
	__ASSERT_NO_MSG((ts != NULL) && timespec_is_valid(ts));

	if (timespec_compare(ts, &K_TS_NO_WAIT) <= 0) {
		if (rem != NULL) {
			*rem = *ts;
		}
		return K_NO_WAIT;
	}

	if (timespec_compare(ts, &K_TS_FOREVER) == 0) {
		if (rem != NULL) {
			*rem = K_TS_NO_WAIT;
		}
		return K_FOREVER;
	}

	if (timespec_compare(ts, &K_TS_MIN) <= 0) {
		/* round up to align to min ticks */
		return timespec_timeout_rem(ts, rem, K_TICKS(K_TICK_MIN));
	}

	if (timespec_compare(ts, &K_TS_MAX) >= 0) {
		/* round down to align to max ticks */
		return timespec_timeout_rem(ts, rem, K_TICKS(K_TICK_MAX));
	}

	uint64_t ticks_s = k_sec_to_ticks_ceil64(ts->tv_sec);
	uint64_t ticks_ns = k_ns_to_ticks_ceil64(ts->tv_nsec);

	return timespec_timeout_rem(ts, rem, K_TICKS(ticks_s + ticks_ns));
}
