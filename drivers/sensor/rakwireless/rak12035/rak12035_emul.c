/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT rakwireless_rak12035

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "rak12035_emul.h"
#include "rak12035_reg.h"

LOG_MODULE_DECLARE(RAK12035, CONFIG_SENSOR_LOG_LEVEL);

struct rak12035_emul_data {
	uint16_t capacitance;
	uint16_t temperature;
	uint16_t dry_calibration;
	uint16_t wet_calibration;
	uint8_t moisture;
	uint8_t version;
	uint8_t command;
	int fail_next;
};

static size_t rak12035_emul_command_length(uint8_t command)
{
	switch (command) {
	case RAK12035_CMD_GET_CAPACITANCE:
	case RAK12035_CMD_GET_TEMPERATURE:
	case RAK12035_CMD_GET_HUMIDITY_FULL:
	case RAK12035_CMD_GET_HUMIDITY_ZERO:
		return 2;
	case RAK12035_CMD_GET_I2C_ADDRESS:
	case RAK12035_CMD_GET_VERSION:
	case RAK12035_CMD_GET_HUMIDITY:
		return 1;
	default:
		return 0;
	}
}

void rak12035_emul_reset(const struct emul *target)
{
	struct rak12035_emul_data *data = target->data;

	data->capacitance = 500;
	data->temperature = 235;
	data->dry_calibration = 800;
	data->wet_calibration = 300;
	data->moisture = 50;
	data->version = 0x22;
	data->command = 0;
	data->fail_next = 0;
}

void rak12035_emul_set_version(const struct emul *target, uint8_t version)
{
	struct rak12035_emul_data *data = target->data;

	data->version = version;
}

void rak12035_emul_set_temperature(const struct emul *target, int16_t temperature)
{
	struct rak12035_emul_data *data = target->data;

	data->temperature = (uint16_t)temperature;
}

void rak12035_emul_set_capacitance(const struct emul *target, uint16_t capacitance)
{
	struct rak12035_emul_data *data = target->data;

	data->capacitance = capacitance;
}

void rak12035_emul_set_moisture(const struct emul *target, uint8_t moisture)
{
	struct rak12035_emul_data *data = target->data;

	data->moisture = moisture;
}

void rak12035_emul_set_calibration(const struct emul *target, uint16_t dry, uint16_t wet)
{
	struct rak12035_emul_data *data = target->data;

	data->dry_calibration = dry;
	data->wet_calibration = wet;
}

void rak12035_emul_fail_next_transfer(const struct emul *target, int error)
{
	struct rak12035_emul_data *data = target->data;

	data->fail_next = error;
}

static int rak12035_emul_read(const struct emul *target, struct i2c_msg *msg, int addr)
{
	struct rak12035_emul_data *data = target->data;
	size_t expected_len = rak12035_emul_command_length(data->command);

	if (expected_len == 0 || msg->len != expected_len) {
		return -EIO;
	}

	switch (data->command) {
	case RAK12035_CMD_GET_CAPACITANCE:
		sys_put_be16(data->capacitance, msg->buf);
		break;
	case RAK12035_CMD_GET_I2C_ADDRESS:
		msg->buf[0] = addr;
		break;
	case RAK12035_CMD_GET_VERSION:
		msg->buf[0] = data->version;
		break;
	case RAK12035_CMD_GET_TEMPERATURE:
		sys_put_be16(data->temperature, msg->buf);
		break;
	case RAK12035_CMD_GET_HUMIDITY:
		msg->buf[0] = data->moisture;
		break;
	case RAK12035_CMD_GET_HUMIDITY_FULL:
		sys_put_be16(data->wet_calibration, msg->buf);
		break;
	case RAK12035_CMD_GET_HUMIDITY_ZERO:
		sys_put_be16(data->dry_calibration, msg->buf);
		break;
	default:
		return -EIO;
	}

	return 0;
}

static int rak12035_emul_transfer_i2c(const struct emul *target, struct i2c_msg *msgs,
				      int num_msgs, int addr)
{
	struct rak12035_emul_data *data = target->data;
	uint8_t command;

	if (data->fail_next != 0) {
		int error = data->fail_next;

		data->fail_next = 0;
		return error;
	}

	if (num_msgs != 1) {
		return -EIO;
	}

	i2c_dump_msgs_rw(target->dev, msgs, num_msgs, addr, false);

	if ((msgs[0].flags & I2C_MSG_READ) != 0U) {
		return rak12035_emul_read(target, &msgs[0], addr);
	}

	if ((msgs[0].flags & I2C_MSG_STOP) == 0U || msgs[0].len == 0U) {
		return -EIO;
	}

	command = msgs[0].buf[0];

	if (msgs[0].len == 1U && rak12035_emul_command_length(command) != 0U) {
		data->command = command;
		return 0;
	}

	if (msgs[0].len != 3U) {
		return -EIO;
	}

	switch (command) {
	case RAK12035_CMD_SET_HUMIDITY_ZERO:
		data->dry_calibration = sys_get_be16(&msgs[0].buf[1]);
		break;
	case RAK12035_CMD_SET_HUMIDITY_FULL:
		data->wet_calibration = sys_get_be16(&msgs[0].buf[1]);
		break;
	default:
		return -EIO;
	}

	return 0;
}

static int rak12035_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	rak12035_emul_reset(target);

	return 0;
}

static const struct i2c_emul_api rak12035_emul_api = {
	.transfer = rak12035_emul_transfer_i2c,
};

#define RAK12035_EMUL_DEFINE(inst)                                                                \
	static struct rak12035_emul_data rak12035_emul_data_##inst;                               \
	EMUL_DT_INST_DEFINE(inst, rak12035_emul_init, &rak12035_emul_data_##inst, NULL,            \
			    &rak12035_emul_api, NULL);

DT_INST_FOREACH_STATUS_OKAY(RAK12035_EMUL_DEFINE)
