/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_NPM2100_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_NPM2100_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mdf_interface_npm2100 MFD NPM2100 Interface
 * @ingroup mfd_interfaces
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

enum mfd_npm2100_event {
	NPM2100_EVENT_SYS_DIETEMP_WARN,
	NPM2100_EVENT_SYS_SHIPHOLD_FALL,
	NPM2100_EVENT_SYS_SHIPHOLD_RISE,
	NPM2100_EVENT_SYS_PGRESET_FALL,
	NPM2100_EVENT_SYS_PGRESET_RISE,
	NPM2100_EVENT_SYS_TIMER_EXPIRY,
	NPM2100_EVENT_ADC_VBAT_READY,
	NPM2100_EVENT_ADC_DIETEMP_READY,
	NPM2100_EVENT_ADC_DROOP_DETECT,
	NPM2100_EVENT_ADC_VOUT_READY,
	NPM2100_EVENT_GPIO0_FALL,
	NPM2100_EVENT_GPIO0_RISE,
	NPM2100_EVENT_GPIO1_FALL,
	NPM2100_EVENT_GPIO1_RISE,
	NPM2100_EVENT_BOOST_VBAT_WARN,
	NPM2100_EVENT_BOOST_VOUT_MIN,
	NPM2100_EVENT_BOOST_VOUT_WARN,
	NPM2100_EVENT_BOOST_VOUT_DPS,
	NPM2100_EVENT_BOOST_VOUT_OK,
	NPM2100_EVENT_LDOSW_OCP,
	NPM2100_EVENT_LDOSW_VINTFAIL,
	NPM2100_EVENT_MAX
};

enum mfd_npm2100_timer_mode {
	NPM2100_TIMER_MODE_GENERAL_PURPOSE,
	NPM2100_TIMER_MODE_WDT_RESET,
	NPM2100_TIMER_MODE_WDT_POWER_CYCLE,
	NPM2100_TIMER_MODE_WAKEUP,
};

/**
 * @brief Write npm2100 timer register
 *
 * The timer tick resolution is 1/64 seconds.
 * This function does not start the timer (see mfd_npm2100_start_timer()).
 *
 * @param dev npm2100 mfd device
 * @param time_ms timer value in ms
 * @param mode timer mode
 * @retval 0 If successful
 * @retval -EINVAL if time value is too large
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm2100_set_timer(const struct device *dev, uint32_t time_ms,
			  enum mfd_npm2100_timer_mode mode);

/**
 * @brief Start npm2100 timer
 *
 * @param dev npm2100 mfd device
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm2100_start_timer(const struct device *dev);

/**
 * @brief npm2100 full power reset
 *
 * @param dev npm2100 mfd device
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm2100_reset(const struct device *dev);

/**
 * @brief npm2100 hibernate
 *
 * Enters low power state, and wakes after specified time or "shphld" pin signal.
 *
 * @param dev npm2100 mfd device
 * @param time_ms timer value in ms. Set to 0 to disable timer.
 * @retval 0 If successful
 * @retval -EINVAL if time value is too large
 * @retval -EBUSY if the timer is already in use.
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm2100_hibernate(const struct device *dev, uint32_t time_ms);

/**
 * @brief Add npm2100 event callback
 *
 * @param dev npm2100 mfd device
 * @param callback callback
 * @return 0 on success, -errno on failure
 */
int mfd_npm2100_add_callback(const struct device *dev, struct gpio_callback *callback);

/**
 * @brief Remove npm2100 event callback
 *
 * @param dev npm2100 mfd device
 * @param callback callback
 * @return 0 on success, -errno on failure
 */
int mfd_npm2100_remove_callback(const struct device *dev, struct gpio_callback *callback);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_NPM2100_H_ */
