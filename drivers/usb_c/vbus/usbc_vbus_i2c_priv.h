/*
 * Copyright (c) 2023, ithinx GmbH
 * Copyright (c) 2023, Tonies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USBC_VBUS_I2C_PRIV_H_
#define ZEPHYR_DRIVERS_USBC_VBUS_I2C_PRIV_H_

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>


struct usbc_vbus_config {
	struct i2c_dt_spec i2c;
};

#endif /* ZEPHYR_DRIVERS_USBC_VBUS_I2C_PRIV_H_ */
