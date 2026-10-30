/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * A small simulation of the ADS1115 for the tests: the configuration and conversion registers,
 * the input multiplexer, the gain, the "conversion is running" flag and I2C failures.
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c_emul.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "ads1x1x_emul.h"

#define REG_CONVERSION 0
#define REG_CONFIG     1

#define CFG_OS         BIT(15)
#define CFG_MUX_SHIFT  12
#define CFG_MUX_MASK   0x7
#define CFG_PGA_SHIFT  9
#define CFG_PGA_MASK   0x7
#define CFG_POWER_UP   0x8583
#define THRESHOLD_LOW  0x8000
#define THRESHOLD_HIGH 0x7fff

/* Full scale of every PGA setting, in microvolts. 6 and 7 behave like 5. */
static const int32_t full_scale_uv[8] = {6144000, 4096000, 2048000, 1024000,
					 512000,  256000,  256000,  256000};

/* The inputs of the differential multiplexer settings 0 to 3: positive and negative. */
static const uint8_t differential_inputs[4][2] = {{0, 1}, {0, 3}, {1, 3}, {2, 3}};

void ads1x1x_emul_reset(const struct emul *target)
{
	struct ads1x1x_emul_data *data = target->data;

	memset(data, 0, sizeof(*data));
	data->reg[REG_CONFIG] = CFG_POWER_UP;
	data->reg[2] = THRESHOLD_LOW;
	data->reg[3] = THRESHOLD_HIGH;
}

/* What the converter would return for the multiplexer and gain in a configuration value. */
static int16_t convert(const struct ads1x1x_emul_data *data, uint16_t config)
{
	uint8_t mux = (config >> CFG_MUX_SHIFT) & CFG_MUX_MASK;
	int32_t full_scale = full_scale_uv[(config >> CFG_PGA_SHIFT) & CFG_PGA_MASK];
	int64_t microvolts;
	int64_t code;

	if (mux < ARRAY_SIZE(differential_inputs)) {
		microvolts = data->input_uv[differential_inputs[mux][0]] -
			     data->input_uv[differential_inputs[mux][1]];
	} else {
		microvolts = data->input_uv[mux - ARRAY_SIZE(differential_inputs)];
	}

	/* One count is full scale / 32768. */
	code = (microvolts * 32768) / full_scale;

	return CLAMP(code, INT16_MIN, INT16_MAX);
}

static void write_register(struct ads1x1x_emul_data *data, uint8_t reg, uint16_t value)
{
	if (reg != REG_CONFIG) {
		data->reg[reg] = value;
		return;
	}

	if (data->config_writes < ARRAY_SIZE(data->config_log)) {
		data->config_log[data->config_writes] = value;
	}
	data->config_writes++;

	/* A write while a conversion runs is the kind of thing that can spoil the conversion. */
	if (data->converting) {
		data->writes_while_converting++;
	}

	/* Writing a 1 to the OS bit starts a single conversion. OS then reads 0 while it runs. */
	data->reg[REG_CONFIG] = value & ~CFG_OS;
	if (value & CFG_OS) {
		data->reg[REG_CONVERSION] = convert(data, value);
		data->busy_left = data->busy_polls;
		data->converting = true;
		data->conversions++;
	}
}

static uint16_t read_register(struct ads1x1x_emul_data *data, uint8_t reg)
{
	if (reg != REG_CONFIG) {
		return data->reg[reg];
	}

	if (data->busy_left > 0) {
		data->busy_left--;
		return data->reg[REG_CONFIG];
	}

	/* The chip reports that the conversion is done. */
	data->converting = false;

	return data->reg[REG_CONFIG] | CFG_OS;
}

static int ads1x1x_emul_transfer(const struct emul *target, struct i2c_msg *msgs, int num_msgs,
				 int addr)
{
	struct ads1x1x_emul_data *data = target->data;

	ARG_UNUSED(addr);

	data->transfers++;

	if (data->fail_in > 0 && --data->fail_in == 0) {
		return data->fail_error;
	}

	if (num_msgs == 1 && !(msgs[0].flags & I2C_MSG_READ) && msgs[0].len == 3 &&
	    msgs[0].buf[0] < ARRAY_SIZE(data->reg)) {
		/* Register write: register, high byte, low byte. */
		write_register(data, msgs[0].buf[0], sys_get_be16(&msgs[0].buf[1]));
		return 0;
	}

	if (num_msgs == 2 && !(msgs[0].flags & I2C_MSG_READ) && msgs[0].len == 1 &&
	    (msgs[1].flags & I2C_MSG_READ) && msgs[1].len == 2 &&
	    msgs[0].buf[0] < ARRAY_SIZE(data->reg)) {
		/* Register read: write the register number, then read two bytes. */
		sys_put_be16(read_register(data, msgs[0].buf[0]), msgs[1].buf);
		return 0;
	}

	return -EIO;
}

static int ads1x1x_emul_init(const struct emul *target, const struct device *parent)
{
	ARG_UNUSED(parent);

	ads1x1x_emul_reset(target);

	return 0;
}

static const struct i2c_emul_api ads1x1x_emul_api_i2c = {
	.transfer = ads1x1x_emul_transfer,
};

#define ADS1X1X_EMUL(t, n)                                                                         \
	static struct ads1x1x_emul_data ads##t##_emul_data_##n;                                    \
	EMUL_DT_INST_DEFINE(n, ads1x1x_emul_init, &ads##t##_emul_data_##n, NULL,                   \
			    &ads1x1x_emul_api_i2c, NULL)

#define ADS1115_EMUL(n) ADS1X1X_EMUL(1115, n)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ads1115
DT_INST_FOREACH_STATUS_OKAY(ADS1115_EMUL)

#define ADS1114_EMUL(n) ADS1X1X_EMUL(1114, n)
#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT ti_ads1114
DT_INST_FOREACH_STATUS_OKAY(ADS1114_EMUL)
