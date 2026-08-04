/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Philipp Steiner
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Adapter from PTP clock drivers to precision timing clocks.
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PTP_H_
#define ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PTP_H_

#include <zephyr/device.h>
#include <zephyr/precision_timing/precision_clock.h>
#include <zephyr/precision_timing/precision_time.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Precision Timing PTP clock adapter
 * @defgroup precision_clock_ptp Precision Timing PTP clock adapter
 * @since 4.5
 * @version 0.1.0
 * @ingroup precision_timing
 * @{
 */

/** Adapter that exposes a PTP clock device as a precision clock. */
struct precision_clock_ptp_adapter {
	/** Protocol-neutral precision clock interface. */
	struct precision_clock clock;
	/** Wrapped PTP clock device. */
	const struct device *ptp_clock;
	/** Capabilities reported by the PTP clock driver, or a conservative fallback. */
	struct precision_clock_caps caps;
};

/**
 * @brief Initialize a precision clock adapter for a PTP clock device.
 *
 * The adapter derives capabilities from the mandatory driver callbacks when the
 * PTP clock driver does not implement the optional capability callback, and
 * uses the driver-reported capabilities and limits when one is available.
 *
 * @param adapter Adapter storage to initialize.
 * @param ptp_clock PTP clock device to wrap.
 * @param domain Domain assigned to timestamps read from the clock.
 *
 * @retval 0 Adapter initialized successfully.
 * @retval -EINVAL @p adapter or @p ptp_clock is null.
 */
int precision_clock_ptp_init(struct precision_clock_ptp_adapter *adapter,
			     const struct device *ptp_clock, struct precision_time_domain domain);

/**
 * @brief Get the protocol-neutral clock exposed by a PTP clock adapter.
 *
 * @param adapter Initialized PTP clock adapter.
 *
 * @return Pointer to the embedded precision clock.
 */
static inline const struct precision_clock *
precision_clock_ptp_get(const struct precision_clock_ptp_adapter *adapter)
{
	return &adapter->clock;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_PRECISION_TIMING_PRECISION_CLOCK_PTP_H_ */
