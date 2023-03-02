/*
 * Copyright (c) 2023 Sensorfy
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Real-time clock control based on the STM32 internal RTC.
 *
 * The core Zephyr API to this device is as a counter.
 *
 * Additional implementation details a user should take into account:
 * * The STM32 counter must be initialized before using the set time
 *   function below
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_STM32_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_STM32_H_

#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Set the current counter value
 *
 * As the counter advances at 1Hz this will usually be set to the
 * current UNIX time stamp.
 *
 * @param dev the STM32 RTC device pointer.
 *
 * @param ticks the new value of the internal counter
 *
 * @retval non-negative on success
 */
int rtc_stm32_set_value(const struct device *dev, uint32_t ticks);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_STM32_H_ */
