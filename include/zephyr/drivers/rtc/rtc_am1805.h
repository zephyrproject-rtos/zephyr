/*
 * Copyright (c) 2026 I-TEN S.A. (ITEN)
 *
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
int rtc_am1805_enter_sleep(const struct device *dev);

/**
 * @brief Read the level of the AM1805 EX1 input.
 *
 * Reads the EXIN bit from the extended RAM register.
 *
 * @param dev Pointer to the AM1805 device.
 * @param pin Pointer to the output variable for the input level: 0 if low,
 *            or 1 if high. It is set to 0 if the register read fails.
 *
 * @retval 0 If the input level was read successfully.
 * @retval -EINVAL If @p dev or @p pin is NULL.
 * @retval -ENODEV If the register could not be read over I2C.
 */
int rtc_am1805_get_ext_pin(const struct device *dev, uint8_t *pin);

/**
 * @brief Enable or disable the AM1805 EX1 interrupt source.
 *
 * Configures the EX1 input polarity and clears any pending EX1 interrupt
 * status. If @p enable is true, the EX1 interrupt source is then enabled in
 * the AM1805. The interrupt output must be connected and configured through
 * the @c am1805-gpios devicetree property for the driver to handle it.
 *
 * @param dev Pointer to the AM1805 device.
 * @param enable True to enable the EX1 interrupt source, false to disable it.
 * @param polarity 0 for a falling edge, or nonzero for a rising edge.
 *
 * @return 0 on success, -EINVAL if @p dev is NULL, or a negative I2C error
 *         code if an I2C operation failed.
 */
int rtc_am1805_enable_ext_pin_int(const struct device *dev, bool enable, int polarity);

/**
 * @brief EX1 interrupt callback.
 *
 * The callback is called in the driver's interrupt thread when an EX1
 * interrupt is processed.
 *
 * @param dev Pointer to the AM1805 device that generated the interrupt.
 * @param user_data User data supplied when registering the callback.
 */
typedef void (*rtc_am1805_ext_pin_callback_t)(const struct device *dev, void *user_data);

/**
 * @brief Register or remove the callback for AM1805 EX1 interrupts.
 *
 * Registering a new callback replaces the previously registered callback.
 * Pass NULL for @p callback to remove the callback. EX1 interrupt handling
 * requires @c CONFIG_RTC_ALARM to be enabled and the device's interrupt output
 * to be configured with the @c am1805-gpios devicetree property.
 *
 * @param dev Pointer to the AM1805 device.
 * @param callback Callback to invoke for an EX1 interrupt, or NULL to remove
 *                 the registered callback.
 * @param user_data Application data passed to @p callback.
 *
 * @retval 0 If the callback was registered or removed successfully.
 * @retval -EINVAL If @p dev is NULL.
 * @retval -ENOTSUP If EX1 interrupt handling is unavailable because
 *                  @c CONFIG_RTC_ALARM is disabled or no interrupt GPIO is
 *                  configured.
 */
int rtc_am1805_ext_pin_set_callback(const struct device *dev, rtc_am1805_ext_pin_callback_t callback,
				void *user_data);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_RTC_RTC_AM1805_H_ */
