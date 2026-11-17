/*
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AM1805 RTC driver-specific API.
 * @ingroup rtc_am1805
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_AM1805_H_
#define ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_AM1805_H_

#include <zephyr/device.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AM1805 RTC driver-specific API.
 * @defgroup rtc_am1805 AM1805 RTC driver-specific API
 * @ingroup rtc_interface_ext
 *
 * @{
 */

/**
 * @brief Put the AM1805 into sleep mode.
 *
 * Sets the SLP bit in the AM1805 sleep control register. Depending on the
 * device configuration, entering sleep mode can also switch off the external
 * power output.
 * This function may not return if the power switch is configured to cut off
 * power to the system.
 *
 * @param dev Pointer to the AM1805 device.
 *
 * @return 0 on success, -EINVAL if @p dev is NULL, -EIO if the AM1805 did
 *         not enter sleep mode, or a negative I2C error code if an I2C
 *         operation failed.
 */
int am1805_enter_sleep(const struct device *dev);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_AM1805_H_ */
