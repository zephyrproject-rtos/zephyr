/*
 * SPDX-FileCopyrightText: 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Helpers shared by clock_monitor back-end drivers.
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_MONITOR_COMMON_H_
#define ZEPHYR_DRIVERS_CLOCK_MONITOR_COMMON_H_

#include <errno.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert a window in ns into a reference-clock cycle count.
 *
 * Callers must reject window_ns == 0 up front; this helper does not
 * special-case it. Logging of the requested-vs-HW values is left to the
 * caller so each back-end can prefix its own LOG module name.
 *
 * @param window_ns User-requested measurement window in nanoseconds (> 0).
 * @param ref_hz    Reference clock frequency in Hz (> 0).
 * @param max_cnt   HW counter field maximum (e.g. the REF_CNT mask).
 * @param out_cnt   Out: validated counter field value.
 *
 * @retval 0       success
 * @retval -ERANGE window_ns rounds to 0, or needs more than max_cnt cycles,
 *                 at the given ref_hz
 */
static inline int clock_monitor_compute_ref_cnt(uint32_t window_ns, uint32_t ref_hz,
						uint32_t max_cnt, uint32_t *out_cnt)
{
	uint64_t cnt = (uint64_t)window_ns * (uint64_t)ref_hz / 1000000000ULL;

	if (cnt == 0U) {
		return -ERANGE;
	}
	if (cnt > max_cnt) {
		return -ERANGE;
	}
	*out_cnt = (uint32_t)cnt;
	return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_CLOCK_MONITOR_COMMON_H_ */
