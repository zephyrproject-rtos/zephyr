/*
 * Copyright (c) 2025, Applied Materials
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for the USB3803 driver
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_MISC_USB3803_H_
#define INCLUDE_ZEPHYR_DRIVERS_MISC_USB3803_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

enum usb3803_regs {
	USB3803_VENDOR_ID_LSB      = 0x00,
	USB3803_VENDOR_ID_MSB      = 0x01,
	USB3803_PROD_ID_LSB        = 0x02,
	USB3803_PROD_ID_MSB        = 0x03,
	USB3803_DEV_ID_LSB         = 0x04,
	USB3803_DEV_ID_MSB         = 0x05,
	USB3803_SP_INTR_LOCK_CTRL  = 0xE7,
	USB3803_SP_INT_STATUS	   = 0xE8,
	USB3803_SP_INT_MSK	   = 0xE9,
};

enum usb3803_modes {
	USB3803_BYPASS = 0,
	USB3803_HUB,
	USB3803_UNINIT,
	USB3803_STANDBY,
	USB3803_ERROR,
};

struct usb3803_config {
	struct i2c_dt_spec bus;
	struct gpio_dt_spec rst_pin;
	struct gpio_dt_spec bypass_pin;
};

struct usb3803_dev {
	enum usb3803_modes mode;
};

struct usb3803_data {
	uint8_t dev;
	struct usb3803_dev *usb_dev;
};

/**
 * @brief reset usb3803 into specific mode
 *
 *
 * @param cfg struct for usb3803
 * @param mode for the usb3803 to enter
 * @return EOK on success
 * @retval -EINVAL If @p dev is invalid
 */
int usb3803_reset(const struct usb3803_config *cfg, struct usb3803_dev *dev,
		  enum usb3803_modes mode);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif


#endif /* INCLUDE_ZEPHYR_DRIVERS_MISC_USB3803_H_ */
