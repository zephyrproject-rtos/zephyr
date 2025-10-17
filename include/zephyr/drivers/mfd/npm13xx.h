/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_NPM13XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_NPM13XX_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup mfd_interface_npm13xx MFD NPM13XX Interface
 * @ingroup mfd_interfaces
 * @{
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

enum mfd_npm13xx_event_t {
	NPM13XX_EVENT_CHG_COMPLETED,
	NPM13XX_EVENT_CHG_ERROR,
	NPM13XX_EVENT_BATTERY_DETECTED,
	NPM13XX_EVENT_BATTERY_REMOVED,
	NPM13XX_EVENT_SHIPHOLD_PRESS,
	NPM13XX_EVENT_SHIPHOLD_RELEASE,
	NPM13XX_EVENT_WATCHDOG_WARN,
	NPM13XX_EVENT_VBUS_DETECTED,
	NPM13XX_EVENT_VBUS_REMOVED,
	NPM13XX_EVENT_GPIO0_EDGE,
	NPM13XX_EVENT_GPIO1_EDGE,
	NPM13XX_EVENT_GPIO2_EDGE,
	NPM13XX_EVENT_GPIO3_EDGE,
	NPM13XX_EVENT_GPIO4_EDGE,
	NPM13XX_EVENT_MAX
};

/**
 * @brief Read multiple registers from npm13xx
 *
 * @param dev npm13xx mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data Pointer to buffer for received data
 * @param len Number of bytes to read
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt())
 */
int mfd_npm13xx_reg_read_burst(const struct device *dev, uint8_t base, uint8_t offset, void *data,
			       size_t len);

/**
 * @brief Read single register from npm13xx
 *
 * @param dev npm13xx mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data Pointer to buffer for received data
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt())
 */
int mfd_npm13xx_reg_read(const struct device *dev, uint8_t base, uint8_t offset, uint8_t *data);

/**
 * @brief Write single register to npm13xx
 *
 * @param dev npm13xx mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data data to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm13xx_reg_write(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data);

/**
 * @brief Write multiple registers to npm13xx
 *
 * @param dev npm13xx mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset First register offset address (bits 7..0 of 16-bit address)
 * @param data Pointer to buffer to write
 * @param len Number of bytes to write
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm13xx_reg_write_burst(const struct device *dev, uint8_t base, uint8_t offset, void *data,
				size_t len);

/**
 * @brief Update selected bits in npm13xx register
 *
 * @param dev npm13xx mfd device
 * @param base Register base address (bits 15..8 of 16-bit address)
 * @param offset Register offset address (bits 7..0 of 16-bit address)
 * @param data data to write
 * @param mask mask of bits to be modified
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_read_dt(), i2c_write_dt())
 */
int mfd_npm13xx_reg_update(const struct device *dev, uint8_t base, uint8_t offset, uint8_t data,
			   uint8_t mask);

/**
 * @brief Write npm13xx timer register
 *
 * @param dev npm13xx mfd device
 * @param time_ms timer value in ms
 * @retval 0 If successful
 * @retval -EINVAL if time value is too large
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm13xx_set_timer(const struct device *dev, uint32_t time_ms);

/**
 * @brief npm13xx full power reset
 *
 * @param dev npm13xx mfd device
 * @retval 0 If successful
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm13xx_reset(const struct device *dev);

/**
 * @brief npm13xx hibernate
 *
 * Enters low power state, and wakes after specified time
 *
 * @param dev npm13xx mfd device
 * @param time_ms timer value in ms
 * @retval 0 If successful
 * @retval -EINVAL if time value is too large
 * @retval -errno In case of any bus error (see i2c_write_dt())
 */
int mfd_npm13xx_hibernate(const struct device *dev, uint32_t time_ms);

/**
 * @brief Add npm13xx event callback
 *
 * @param dev npm13xx mfd device
 * @param callback callback
 * @return 0 on success, -errno on failure
 */
int mfd_npm13xx_add_callback(const struct device *dev, struct gpio_callback *callback);

/**
 * @brief Remove npm13xx event callback
 *
 * @param dev npm13xx mfd device
 * @param callback callback
 * @return 0 on success, -errno on failure
 */
int mfd_npm13xx_remove_callback(const struct device *dev, struct gpio_callback *callback);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_NPM13XX_H_ */
