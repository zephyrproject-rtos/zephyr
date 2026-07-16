/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_GNSS_GNSS_TIMEPULSE_H_
#define ZEPHYR_DRIVERS_GNSS_GNSS_TIMEPULSE_H_

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

/*
 * The current in-tree backend captures a 1 Hz PPS signal. Allow for a single
 * missed pulse before the latest timestamp is declared stale.
 */
#define GNSS_TIMEPULSE_PPS_1HZ_STALE_MS 1100

/**
 * @brief GNSS PPS capture state.
 *
 * Owned by the GNSS driver instance. A capture backend feeds it through
 * gnss_timepulse_report(), and the driver returns it from the GNSS
 * .get_latest_timepulse callback through gnss_timepulse_get_ticks().
 */
struct gnss_timepulse {
	struct k_spinlock lock;
	k_ticks_t last;
	bool available;
	bool valid;
};

/**
 * @brief Report a captured PPS edge.
 *
 * Called by a capture backend, typically from interrupt context.
 */
static inline void gnss_timepulse_report(struct gnss_timepulse *tp, k_ticks_t ticks)
{
	K_SPINLOCK(&tp->lock) {
		tp->last = ticks;
		tp->valid = true;
	}
}

/**
 * @brief Get the timestamp of the latest captured PPS edge.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if no PPS input is connected for this device.
 * @retval -EAGAIN if no recent PPS edge has been captured.
 */
static inline int gnss_timepulse_get_ticks(struct gnss_timepulse *tp, k_ticks_t *timestamp)
{
	k_spinlock_key_t key = k_spin_lock(&tp->lock);
	int ret = -EAGAIN;

	if (!tp->available) {
		ret = -ENOTSUP;
	} else if (tp->valid &&
		   (k_uptime_ticks() - tp->last) <
			   k_ms_to_ticks_ceil64(GNSS_TIMEPULSE_PPS_1HZ_STALE_MS)) {
		*timestamp = tp->last;
		ret = 0;
	}

	k_spin_unlock(&tp->lock, key);

	return ret;
}

/**
 * @brief Attach the compiled PPS capture backend to a GNSS device.
 *
 * The GNSS driver calls this from its init with its own capture state; the
 * capture backend (selected at build time) starts feeding @p tp on each edge.
 *
 * @param dev GNSS device whose PPS input is captured.
 * @param tp Capture state owned by the GNSS driver.
 *
 * @retval 0 on success, or when the device has no PPS input.
 * @retval -errno on backend setup failure.
 */
int gnss_timepulse_attach(const struct device *dev, struct gnss_timepulse *tp);

#endif /* ZEPHYR_DRIVERS_GNSS_GNSS_TIMEPULSE_H_ */
