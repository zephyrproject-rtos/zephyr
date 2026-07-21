/*
 * Copyright (c) 2026 Dotcom IoT LLP
 *
 * SPDX-License-Identifier: Apache-2.0
 * Author: Mahendra Sondagar <mahendra@dotcom.co.in>
 * Author: Parin Baudhanwala <parin.baudhanwala@dnkmail.in>
 */

#define DT_DRV_COMPAT ams_as7341

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/emul_sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include "ams_as7341.h"

LOG_MODULE_DECLARE(ams_as7341, CONFIG_SENSOR_LOG_LEVEL);

struct as7341_emul_data {
	uint8_t reg[256];
};

static int as7341_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				    int addr)
{
	struct as7341_emul_data *data = target->data;
	int reg;

	switch (num_msgs) {
	case 2:
		reg = msgs->buf[0];
		msgs++;
		if (msgs->flags & I2C_MSG_READ) {
			msgs->buf[0] = data->reg[reg];
		} else {
			data->reg[reg] = msgs->buf[0];
		}
		break;
	default:
		return -EIO;
	}
	return 0;
}

static int as7341_emul_init(const struct emul *target, const struct device *parent)
{
	struct as7341_emul_data *data = target->data;

	memset(data->reg, 0, sizeof(data->reg));
	return 0;
}

static const struct i2c_emul_api as7341_emul_api_i2c = {
	.transfer = as7341_emul_transfer_i2c,
};

#define AS7341_EMUL(n)                                                                             \
	struct as7341_emul_data as7341_emul_data_##n;                                              \
	EMUL_DT_INST_DEFINE(n, as7341_emul_init, &as7341_emul_data_##n, NULL,                      \
			    &as7341_emul_api_i2c, NULL)

DT_INST_FOREACH_STATUS_OKAY(AS7341_EMUL)
