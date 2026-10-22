/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_p3t1755

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/util.h>

#include "p3t1755.h"
#include "p3t1755_emul.h"

struct p3t1755_emul_data {
	uint8_t temperature[2];
	uint8_t config;
	uint32_t config_write_count;
	bool read_failure;
	bool write_failure;
};

void p3t1755_emul_reset(const struct emul *target)
{
	struct p3t1755_emul_data *data = target->data;

	memset(data, 0, sizeof(*data));
}

void p3t1755_emul_set_temperature(const struct emul *target, int16_t sample)
{
	struct p3t1755_emul_data *data = target->data;
	uint16_t raw = (uint16_t)sample << P3T1755_TEMPERATURE_REG_SHIFT;

	data->temperature[0] = raw >> 8;
	data->temperature[1] = raw;
}

uint8_t p3t1755_emul_get_config(const struct emul *target)
{
	struct p3t1755_emul_data *data = target->data;

	return data->config;
}

void p3t1755_emul_set_config(const struct emul *target, uint8_t config)
{
	struct p3t1755_emul_data *data = target->data;

	data->config = config;
}

uint32_t p3t1755_emul_get_config_write_count(const struct emul *target)
{
	struct p3t1755_emul_data *data = target->data;

	return data->config_write_count;
}

void p3t1755_emul_set_read_failure(const struct emul *target, bool fail)
{
	struct p3t1755_emul_data *data = target->data;

	data->read_failure = fail;
}

void p3t1755_emul_set_write_failure(const struct emul *target, bool fail)
{
	struct p3t1755_emul_data *data = target->data;

	data->write_failure = fail;
}

static int p3t1755_emul_transfer(const struct emul *target, struct i2c_msg *msgs,
				 int num_msgs, int addr)
{
	struct p3t1755_emul_data *data = target->data;
	uint8_t reg;

	ARG_UNUSED(addr);

	if ((msgs == NULL) || (num_msgs < 1) || (num_msgs > 2) ||
	    ((msgs[0].flags & I2C_MSG_READ) != 0U) || (msgs[0].len < 1U)) {
		return -EIO;
	}

	reg = msgs[0].buf[0];

	if (num_msgs != 2) {
		return -EIO;
	}

	if ((msgs[1].flags & I2C_MSG_READ) != 0U) {
		if (data->read_failure) {
			return -EIO;
		}

		switch (reg) {
		case P3T1755_TEMPERATURE_REG:
			if (msgs[1].len != sizeof(data->temperature)) {
				return -EIO;
			}
			memcpy(msgs[1].buf, data->temperature, sizeof(data->temperature));
			return 0;
		case P3T1755_CONFIG_REG:
			if (msgs[1].len != 1U) {
				return -EIO;
			}
			msgs[1].buf[0] = data->config;
			return 0;
		default:
			return -EIO;
		}
	}

	if (data->write_failure || (msgs[1].len != 1U) ||
	    (reg != P3T1755_CONFIG_REG)) {
		return -EIO;
	}

	data->config = msgs[1].buf[0];
	data->config_write_count++;

	return 0;
}

static int p3t1755_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);
	p3t1755_emul_reset(target);

	return 0;
}

static const struct i2c_emul_api p3t1755_emul_api = {
	.transfer = p3t1755_emul_transfer,
};

#define P3T1755_EMUL_DEFINE(inst)                                                     \
	static struct p3t1755_emul_data p3t1755_emul_data_##inst;                     \
	EMUL_DT_INST_DEFINE(inst, p3t1755_emul_init, &p3t1755_emul_data_##inst, NULL,  \
			    &p3t1755_emul_api, NULL)

DT_INST_FOREACH_STATUS_OKAY(P3T1755_EMUL_DEFINE)
