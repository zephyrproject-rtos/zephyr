/*
 * Copyright (c) 2021 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Real-time clock control based on the MCUX IMX SNVS counter API.
 *
 * The core Zephyr API to this device is as a counter.
 *
 * Additional implementation details a user should take into account:
 * * an optional SRTC can be enabled (default) with configuration
 *   options
 * * the high power channel (id 0) is always available, the low power
 *   channel (id 1) is optional
 * * the low power alarm can be used to assert a wake-up
 * * the counter has a fixed 1Hz period
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_MCUX_SNVS_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_MCUX_SNVS_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set the current counter value
 *
 * As the counter advances at 1Hz this will usually be set to the
 * current UNIX time stamp.
 *
 * @param dev the IMX SNVS RTC device pointer.
 *
 * @param ticks the new value of the internal counter
 *
 * @retval non-negative on success
 */
int mcux_snvs_rtc_set(const struct device *dev, uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_MCUX_SNVS_H_ */
