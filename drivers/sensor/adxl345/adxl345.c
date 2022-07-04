/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adi_adxl345

#include <zephyr/drivers/sensor.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include "adxl345.h"

LOG_MODULE_REGISTER(ADXL345, CONFIG_SENSOR_LOG_LEVEL);

static int adxl345_read_sample(const struct device *dev,
			       struct adxl345_sample *sample)
{
	const struct adxl345_dev_config *cfg = dev->config;
	int16_t raw_x, raw_y, raw_z;
	uint8_t axis_data[6];

	int rc = i2c_burst_read_dt(&cfg->i2c, ADXL345_X_AXIS_DATA_0_REG, axis_data, 6);

	if (rc < 0) {
		LOG_ERR("Samples read failed with rc=%d\n", rc);
		return rc;
	}

	raw_x = axis_data[0] | (axis_data[1] << 8);
	raw_y = axis_data[2] | (axis_data[3] << 8);
	raw_z = axis_data[4] | (axis_data[5] << 8);

	sample->x = raw_x;
	sample->y = raw_y;
	sample->z = raw_z;

	return 0;
}

static void adxl345_accel_convert(struct sensor_value *val, int16_t sample)
{
	if (sample & BIT(9)) {
		sample |= ADXL345_COMPLEMENT;
	}

	val->val1 = (sample * 1000) / 32;
	val->val2 = 0;
}

static int adxl345_sample_fetch(const struct device *dev,
				enum sensor_channel chan)
{
	struct adxl345_dev_data *data = dev->data;
	const struct adxl345_dev_config *cfg = dev->config;
	struct adxl345_sample sample;
	uint8_t samples_count;
	int rc;

	data->sample_number = 0;
	rc = i2c_reg_read_byte_dt(&cfg->i2c, ADXL345_FIFO_STATUS_REG, &samples_count);
	if (rc < 0) {
		LOG_ERR("Failed to read FIFO status rc = %d\n", rc);
		return rc;
	}

	__ASSERT_NO_MSG(samples_count <= ARRAY_SIZE(data->bufx));

	for (uint8_t s = 0; s < samples_count; s++) {
		rc = adxl345_read_sample(dev, &sample);
		if (rc < 0) {
			LOG_ERR("Failed to fetch sample rc=%d\n", rc);
			return rc;
		}
		data->bufx[s] = sample.x;
		data->bufy[s] = sample.y;
		data->bufz[s] = sample.z;
	}

	return samples_count;
}

static int adxl345_channel_get(const struct device *dev,
			       enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct adxl345_dev_data *data = dev->data;

	if (data->sample_number >= ARRAY_SIZE(data->bufx)) {
		data->sample_number = 0;
	}

	switch (chan) {
	case SENSOR_CHAN_ACCEL_X:
		adxl345_accel_convert(val, data->bufx[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_Y:
		adxl345_accel_convert(val, data->bufy[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_Z:
		adxl345_accel_convert(val, data->bufz[data->sample_number]);
		data->sample_number++;
		break;
	case SENSOR_CHAN_ACCEL_XYZ:
		adxl345_accel_convert(val++, data->bufx[data->sample_number]);
		adxl345_accel_convert(val++, data->bufy[data->sample_number]);
		adxl345_accel_convert(val,   data->bufz[data->sample_number]);
		data->sample_number++;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api adxl345_api_funcs = {
	.sample_fetch = adxl345_sample_fetch,
	.channel_get = adxl345_channel_get,
};

static int adxl345_init(const struct device *dev)
{
	int rc;
	struct adxl345_dev_data *data = dev->data;
	const struct adxl345_dev_config *cfg = dev->config;
	uint8_t dev_id;

	data->sample_number = 0;

	if (!device_is_ready(cfg->i2c.bus)) {
		LOG_ERR("I2C bus device is not ready");
		return -ENODEV;
	}

	rc = i2c_reg_read_byte_dt(&cfg->i2c, ADXL345_DEVICE_ID_REG, &dev_id);
	if (rc < 0 || dev_id != ADXL345_PART_ID) {
		LOG_ERR("Read PART ID failed: 0x%x\n", rc);
		return -ENODEV;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, ADXL345_FIFO_CTL_REG, ADXL345_FIFO_STREAM_MODE);
	if (rc < 0) {
		LOG_ERR("FIFO enable failed\n");
		return -EIO;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, ADXL345_DATA_FORMAT_REG, ADXL345_RANGE_16G);
	if (rc < 0) {
		LOG_ERR("Data format set failed\n");
		return -EIO;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, ADXL345_RATE_REG, ADXL345_RATE_25HZ);
	if (rc < 0) {
		LOG_ERR("Rate setting failed\n");
		return -EIO;
	}

	rc = i2c_reg_write_byte_dt(&cfg->i2c, ADXL345_POWER_CTL_REG, ADXL345_ENABLE_MEASURE_BIT);
	if (rc < 0) {
		LOG_ERR("Enable measure bit failed\n");
		return -EIO;
	}

	return 0;
}

#define ADXL345_DEFINE(inst)								\
	static struct adxl345_dev_data adxl345_data_##inst;				\
											\
	static const struct adxl345_dev_config adxl345_config_##inst = {		\
		.i2c = I2C_DT_SPEC_INST_GET(inst),					\
	};										\
											\
	DEVICE_DT_INST_DEFINE(inst, adxl345_init, NULL,					\
			      &adxl345_data_##inst, &adxl345_config_##inst, POST_KERNEL,\
			      CONFIG_SENSOR_INIT_PRIORITY, &adxl345_api_funcs);		\

DT_INST_FOREACH_STATUS_OKAY(ADXL345_DEFINE)
