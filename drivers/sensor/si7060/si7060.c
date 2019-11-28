/*
 * Copyright (c) 2019 Actinius
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/i2c.h>
#include <drivers/sensor.h>
#include <logging/log.h>

#include "si7060.h"

#define SIGN_BIT_MASK 0x7F

LOG_MODULE_REGISTER(si7060, CONFIG_SENSOR_LOG_LEVEL);

struct si7060_data {
	struct device *i2c_dev;
	u16_t temperature;
};

static int si7060_reg_read(struct si7060_data *drv_data, u8_t reg,
			     u8_t *val)
{
	if (i2c_reg_read_byte(drv_data->i2c_dev,
		DT_INST_0_SILABS_SI7060_BASE_ADDRESS, reg, val)) {
		return -EIO;
	}

	return 0;
}

static int si7060_reg_write(struct si7060_data *drv_data, u8_t reg,
			      u8_t val)
{
	return i2c_reg_write_byte(drv_data->i2c_dev,
		DT_INST_0_SILABS_SI7060_BASE_ADDRESS, reg, val);
}

static int si7060_sample_fetch(struct device *dev, enum sensor_channel chan)
{
	struct si7060_data *drv_data = dev->driver_data;

	if (si7060_reg_write(drv_data, SI7060_REG_CONFIG,
		SI7060_ONE_BURST_VALUE) != 0) {
		return -EIO;
	}

	int retval;
	u8_t dspsigm;
	u8_t dspsigl;

	retval = si7060_reg_read(drv_data, SI7060_REG_TEMP_HIGH,
		&dspsigm);
	retval += si7060_reg_read(drv_data, SI7060_REG_TEMP_LOW,
		&dspsigl);

	if (retval == 0) {
		drv_data->temperature = (256 * (dspsigm & SIGN_BIT_MASK))
		+ dspsigl;
	} else {
		LOG_ERR("Read register err");
	}

	LOG_DBG("Sample_fetch retval: %d", retval);

	return retval;
}

static int si7060_channel_get(struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct si7060_data *drv_data = dev->driver_data;
	s32_t uval;

	if (chan == SENSOR_CHAN_AMBIENT_TEMP) {
		uval = ((55 * 160) + (drv_data->temperature - 16384)) >> 4;
		val->val1 = uval / 10;
		val->val2 = (uval % 10) * 100000;

		LOG_DBG("Temperature = val1:%d, val2:%d", val->val1, val->val2);

		return 0;
	} else {
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api si7060_api = {
	.sample_fetch = &si7060_sample_fetch,
	.channel_get = &si7060_channel_get,
};

static int si7060_chip_init(struct device *dev)
{
	struct si7060_data *drv_data = dev->driver_data;
	u8_t value;

	drv_data->i2c_dev = device_get_binding(
		DT_INST_0_SILABS_SI7060_BUS_NAME);

	if (!drv_data->i2c_dev) {
		LOG_ERR("Failed to get pointer to %s device!",
			DT_INST_0_SILABS_SI7060_BUS_NAME);
		return -EINVAL;
	}

	if (si7060_reg_read(drv_data, SI7060_REG_CHIP_INFO,
		&value) != 0) {
		return -EIO;
	}

	if ((value >> 4) != SI7060_CHIP_ID_VALUE) {
		LOG_ERR("Bad chip id 0x%x", value);
		return -ENOTSUP;
	}

	return 0;
}

static int si7060_init(struct device *dev)
{
	if (si7060_chip_init(dev) < 0) {
		return -EINVAL;
	}

	return 0;
}

static struct si7060_data si_data;

DEVICE_AND_API_INIT(si7060, DT_INST_0_SILABS_SI7060_LABEL, si7060_init,
	&si_data, NULL, POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &si7060_api);
