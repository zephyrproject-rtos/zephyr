/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_NPM1300_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_NPM1300_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mdf_interface_npm1300 MFD NPM1300 Interface
 * @ingroup mfd_interfaces
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

enum mfd_npm1300_event_t {
	NPM1300_EVENT_CHG_COMPLETED,
	NPM1300_EVENT_CHG_ERROR,
	NPM1300_EVENT_BATTERY_DETECTED,
	NPM1300_EVENT_BATTERY_REMOVED,
	NPM1300_EVENT_SHIPHOLD_PRESS,
	NPM1300_EVENT_SHIPHOLD_RELEASE,
	NPM1300_EVENT_WATCHDOG_WARN,
	NPM1300_EVENT_VBUS_DETECTED,
	NPM1300_EVENT_VBUS_REMOVED,
	NPM1300_EVENT_VBUS_CC1_CHANGE,
	NPM1300_EVENT_VBUS_CC2_CHANGE,
	NPM1300_EVENT_GPIO0_EDGE,
	NPM1300_EVENT_GPIO1_EDGE,
	NPM1300_EVENT_GPIO2_EDGE,
	NPM1300_EVENT_GPIO3_EDGE,
	NPM1300_EVENT_GPIO4_EDGE,
	NPM1300_EVENT_MAX
};

/**
 * @brief Read multiple registers from npm1300
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data Pointer to buffer for received data
 * @param len Number of bytes to read
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt())
 */
int mfd_npm1300_reg_read_burst(const struct device *dev, uint8_t base, uint8_t offset, void *data,
			       size_t len);

/**
 * @brief Read single register from npm1300
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data Pointer to buffer for received data
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt())
 */
int mfd_npm1300_reg_read(const struct device *dev, uint8_t base, uint8_t offset, uint8_t *data);

/**
 * @brief Write single register to npm1300
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data data to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm1300_reg_write(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data);

/**
 * @brief Write two registers to npm1300
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data1 first byte of data to write
 * @param data2 second byte of data to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm1300_reg_write2(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data1,
			   uint8_t data2);

/**
 * @brief Update selected bits in npm1300 register
 *
 * @param dev npm1300 mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data data to write
 * @param mask mask of bits to be modified
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt(), i2c_write_dt())
 */
int mfd_npm1300_reg_update(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data,
			   uint8_t mask);

/**
 * @brief Write npm1300 timer register
 *
 * @param dev npm1300 mfd device
 * @param time_ms timer value in ms
 * @retval 0 If successful
 * @retval -EINVAL if time value is too large
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm1300_set_timer(const struct device *dev, uint32_t time_ms);

/**
 * @brief npm1300 full power reset
 *
 * @param dev npm1300 mfd device
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm1300_reset(const struct device *dev);

/**
 * @brief npm1300 hibernate
 *
 * Enters low power state, and wakes after specified time
 *
 * @param dev npm1300 mfd device
 * @param time_ms timer value in ms
 * @retval 0 If successful
 * @retval -EINVAL if time value is too large
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm1300_hibernate(const struct device *dev, uint32_t time_ms);

/**
 * @brief Add npm1300 event callback
 *
 * @param dev npm1300 mfd device
 * @param callback callback
 * @return 0 on success, -errno on failure
 */
int mfd_npm1300_add_callback(const struct device *dev, struct gpio_callback *callback);

/**
 * @brief Remove npm1300 event callback
 *
 * @param dev npm1300 mfd device
 * @param callback callback
 * @return 0 on success, -errno on failure
 */
int mfd_npm1300_remove_callback(const struct device *dev, struct gpio_callback *callback);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_NPM1300_H_ */
