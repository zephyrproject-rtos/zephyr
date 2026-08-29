/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief PTP clock device adapter for precision clocks.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PTP_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PTP_H_

#include <zephyr/device.h>
#include <zephyr/precision_timing/precision_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Adapter that exposes a PTP clock device as a precision clock. */
struct precision_clock_ptp_adapter {
	/** Protocol-neutral precision clock. */
	struct precision_clock precision_clk;
	/** Wrapped PTP clock device. */
	const struct device *dev;
};

/**
 * @brief Initialize an adapter for a PTP clock device.
 *
 * @param adapter Adapter storage to initialize.
 * @param dev PTP clock device to wrap.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is null.
 */
int precision_clock_ptp_init(struct precision_clock_ptp_adapter *adapter, const struct device *dev);

/**
 * @brief Get the precision clock exposed by a PTP adapter.
 *
 * @param adapter Initialized PTP clock adapter.
 *
 * @return Pointer to the embedded precision clock.
 */
static inline const struct precision_clock *
precision_clock_ptp_get(const struct precision_clock_ptp_adapter *adapter)
{
	return &adapter->precision_clk;
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PTP_H_ */
