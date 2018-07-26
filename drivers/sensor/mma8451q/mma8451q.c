/*
 * Copyright (c) 2018 Lars Knudsen
 * Copyright (c) 2016 Freescale Semiconductor, Inc. (sample conversion)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <i2c.h>
#include <kernel.h>
#include <misc/util.h>
#include <sensor.h>

#include "mma8451q.h"

/* Data-sheet: https://www.nxp.com/docs/en/data-sheet/MMA8451Q.pdf */

static int mma8451q_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	const struct mma8451q_config *config = dev->config->config_info;
	struct mma8451q_data *data = dev->driver_data;

	u8_t buf[6];

	/* Read data from all three axes at the same time. */
	if (i2c_burst_read(data->i2c, config->i2c_address,
			   MMA8451Q_OUT_X_MSB, buf, 6) < 0) {
		SYS_LOG_ERR("Could not read accelerometer data");
		return -EIO;
	}

	data->x = (buf[0] << 8) | buf[1];
	data->y = (buf[2] << 8) | buf[3];
	data->z = (buf[4] << 8) | buf[5];

	return 0;
}

static void mma8451q_accel_convert(struct sensor_value *val, s16_t raw,
				   enum mma8451q_range range)
{
	u8_t frac_bits;
	s64_t micro_ms2;

	/* The range encoding is convenient to compute the number of fractional
	 * bits:
	 * - 2g mode (range = 0) has 14 fractional bits
	 * - 4g mode (range = 1) has 13 fractional bits
	 * - 8g mode (range = 2) has 12 fractional bits
	 */
	frac_bits = 14 - range;

	/* Convert units to micro m/s^2. Intermediate results before the shift
	 * are 40 bits wide.
	 */
	micro_ms2 = (raw * SENSOR_G) >> frac_bits;

	/* The maximum possible value is 8g, which in units of micro m/s^2
	 * always fits into 32-bits. Cast down to s32_t so we can use a
	 * faster divide.
	 */
	val->val1 = (s32_t) micro_ms2 / 1000000;
	val->val2 = (s32_t) micro_ms2 % 1000000;
}

static int mma8451q_channel_get(struct device *dev,
				   enum sensor_channel chan,
				   struct sensor_value *val)
{
	const struct mma8451q_config *config = dev->config->config_info;
	struct mma8451q_data *data = dev->driver_data;

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		mma8451q_accel_convert(val, data->x, config->range);
		break;

	case SENSOR_CHAN_ACCEL_Y:
		mma8451q_accel_convert(val, data->y, config->range);
		break;

	case SENSOR_CHAN_ACCEL_Z:
		mma8451q_accel_convert(val, data->z, config->range);
		break;

	case SENSOR_CHAN_ACCEL_XYZ:
		mma8451q_accel_convert(val++, data->x, config->range);
		mma8451q_accel_convert(val++, data->y, config->range);
		mma8451q_accel_convert(val++, data->z, config->range);
		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api mma8451q_driver_api = {
	.sample_fetch = mma8451q_sample_fetch,
	.channel_get = mma8451q_channel_get,
};

int mma8451q_init(struct device *dev)
{
	const struct mma8451q_config *config = dev->config->config_info;
	struct mma8451q_data *data = dev->driver_data;

	data->i2c = device_get_binding(config->i2c_name);
	if (data->i2c == NULL) {
		SYS_LOG_ERR("Failed to get pointer to %s device!",
				config->i2c_name);
		return -EINVAL;
	}

	u8_t whoami;

	if (i2c_reg_read_byte(data->i2c, config->i2c_address,
			      MMA8451Q_REG_WHOAMI, &whoami)) {
		SYS_LOG_ERR("Could not get WHOAMI value");
		return -EIO;
	}

	if (whoami != config->whoami) {
		SYS_LOG_ERR("WHOAMI value received 0x%x, expected 0x%x",
			    whoami, MMA8451Q_REG_WHOAMI);
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
				   MMA8451Q_CTRL_REG1, 0) < 0) {
		SYS_LOG_ERR("Could not set accel in config mode");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
				   MMA8451Q_XYZ_DATA_CFG, config->range) < 0) {
		SYS_LOG_ERR("Could not set range");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
				   MMA8451Q_CTRL_REG2, 0) < 0) {
		SYS_LOG_ERR("Could not set to normal mode");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
				   MMA8451Q_CTRL_REG3, 0) < 0) {
		SYS_LOG_ERR("Could not set to low polarity, push-pull output");
		return -EIO;
	}

	if (i2c_reg_write_byte(data->i2c, config->i2c_address,
				   MMA8451Q_CTRL_REG1, 0x09) < 0) {
		SYS_LOG_ERR("Could not set data rate to 800Hz");
		return -EIO;
	}

	SYS_LOG_DBG("Init complete");

	return 0;
}

static const struct mma8451q_config mma8451q_config = {
	.i2c_name = CONFIG_MMA8451Q_I2C_NAME,
	.i2c_address = CONFIG_MMA8451Q_I2C_ADDRESS,
	.whoami = CONFIG_MMA8451Q_WHOAMI,
#if CONFIG_MMA8451Q_RANGE_8G
	.range = MMA8451Q_RANGE_8G,
#elif CONFIG_MMA8451Q_RANGE_4G
	.range = MMA8451Q_RANGE_4G,
#else
	.range = MMA8451Q_RANGE_2G,
#endif
};

static struct mma8451q_data mma8451q_data;

DEVICE_AND_API_INIT(mma8451q, CONFIG_MMA8451Q_NAME, mma8451q_init,
	&mma8451q_data, &mma8451q_config, POST_KERNEL,
	CONFIG_SENSOR_INIT_PRIORITY, &mma8451q_driver_api);
