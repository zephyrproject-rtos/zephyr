/*
 * Copyright 2023 Jonas Otto
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_USBC_DEVICE_FUSB302B_PRIVATE_H_
#define ZEPHYR_DRIVERS_USBC_DEVICE_FUSB302B_PRIVATE_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/usb_c/usbc_tcpc.h>
#include <zephyr/drivers/gpio.h>

#define FUSB302_RX_BUFFER_SIZE 80

struct alert_info {
	void *data;
	tcpc_alert_handler_cb_t handler;
};

struct fusb302b_data {
	struct alert_info alert_info;
};

struct fusb302b_cfg {
	struct i2c_dt_spec i2c;
};

int fusb302b_init(const struct device *dev);

#endif /* ZEPHYR_DRIVERS_USBC_DEVICE_FUSB302B_PRIVATE_H_ */
