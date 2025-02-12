/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bosch_bma280

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/logging/log.h>

#include "bma280.h"

LOG_MODULE_REGISTER(BMA280, CONFIG_SENSOR_LOG_LEVEL);

static int bma280_sample_fetch(const struct device *dev,
			       enum sensor_channel chan)
{
	struct bma280_data *drv_data = dev->data;
	const struct bma280_config *config = dev->config;
	uint8_t buf[6];
	uint8_t lsb;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/*
	 * since all accel data register addresses are consecutive,
	 * a burst read can be used to read all the samples
	 */
	if (i2c_burst_read_dt(&config->i2c,
			      BMA280_REG_ACCEL_X_LSB, buf, 6) < 0) {
		LOG_DBG("Could not read accel axis data");
		return -EIO;
	}

	lsb = (buf[0] & BMA280_ACCEL_LSB_MASK) >> BMA280_ACCEL_LSB_SHIFT;
	drv_data->x_sample = (((int8_t)buf[1]) << BMA280_ACCEL_LSB_BITS) | lsb;

	lsb = (buf[2] & BMA280_ACCEL_LSB_MASK) >> BMA280_ACCEL_LSB_SHIFT;
	drv_data->y_sample = (((int8_t)buf[3]) << BMA280_ACCEL_LSB_BITS) | lsb;

	lsb = (buf[4] & BMA280_ACCEL_LSB_MASK) >> BMA280_ACCEL_LSB_SHIFT;
	drv_data->z_sample = (((int8_t)buf[5]) << BMA280_ACCEL_LSB_BITS) | lsb;

	if (i2c_reg_read_byte_dt(&config->i2c,
				 BMA280_REG_TEMP,
				 (uint8_t *)&drv_data->temp_sample) < 0) {
		LOG_DBG("Could not read temperature data");
		return -EIO;
	}

	return 0;
}

static void bma280_channel_accel_convert(struct sensor_value *val,
					int64_t raw_val)
{
	/*
	 * accel_val = (sample * BMA280_PMU_FULL_RAGE) /
	 *             (2^data_width * 10^6)
	 */
	raw_val = (raw_val * BMA280_PMU_FULL_RANGE) /
		  (1 << (8 + BMA280_ACCEL_LSB_BITS));
	val->val1 = raw_val / 1000000;
	val->val2 = raw_val % 1000000;

	/* normalize val to make sure val->val2 is positive */
	if (val->val2 < 0) {
		val->val1 -= 1;
		val->val2 += 1000000;
	}
}

static int bma280_channel_get(const struct device *dev,
			      enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct bma280_data *drv_data = dev->data;

	/*
	 * See datasheet "Sensor data" section for
	 * more details on processing sample data.
	 */
	if (chan == SENSOR_CHAN_ACCEL_X) {
		bma280_channel_accel_convert(val, drv_data->x_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_Y) {
		bma280_channel_accel_convert(val, drv_data->y_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_Z) {
		bma280_channel_accel_convert(val, drv_data->z_sample);
	} else if (chan == SENSOR_CHAN_ACCEL_XYZ) {
		bma280_channel_accel_convert(val, drv_data->x_sample);
		bma280_channel_accel_convert(val + 1, drv_data->y_sample);
		bma280_channel_accel_convert(val + 2, drv_data->z_sample);
	} else if (chan == SENSOR_CHAN_DIE_TEMP) {
		/* temperature_val = 23 + sample / 2 */
		val->val1 = (drv_data->temp_sample >> 1) + 23;
		val->val2 = 500000 * (drv_data->temp_sample & 1);
		return 0;
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static const struct sensor_driver_api bma280_driver_api = {
#if CONFIG_BMA280_TRIGGER
	.attr_set = bma280_attr_set,
	.trigger_set = bma280_trigger_set,
#endif
	.sample_fetch = bma280_sample_fetch,
	.channel_get = bma280_channel_get,
};

int bma280_init(const struct device *dev)
{
	const struct bma280_config *config = dev->config;
	uint8_t id = 0U;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* read device ID */
	if (i2c_reg_read_byte_dt(&config->i2c,
				 BMA280_REG_CHIP_ID, &id) < 0) {
		LOG_DBG("Could not read chip id");
		return -EIO;
	}

	if (id != BMA280_CHIP_ID) {
		LOG_DBG("Unexpected chip id (%x)", id);
		return -EIO;
	}

	if (i2c_reg_write_byte_dt(&config->i2c,
				  BMA280_REG_PMU_BW, BMA280_PMU_BW) < 0) {
		LOG_DBG("Could not set data filter bandwidth");
		return -EIO;
	}

	/* set g-range */
	if (i2c_reg_write_byte_dt(&config->i2c,
				  BMA280_REG_PMU_RANGE, BMA280_PMU_RANGE) < 0) {
		LOG_DBG("Could not set data g-range");
		return -EIO;
	}

#ifdef CONFIG_BMA280_TRIGGER
	if (config->int1_gpio.port) {
		if (bma280_init_interrupt(dev) < 0) {
			LOG_DBG("Could not initialize interrupts");
			return -EIO;
		}
	}
#endif

	return 0;
}

#define BMA280_DEFINE(inst)									\
	static struct bma280_data bma280_data_##inst;						\
												\
	static const struct bma280_config bma280_config##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_BMA280_TRIGGER,						\
			   (.int1_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, bma280_init, NULL, &bma280_data_##inst,		\
			      &bma280_config##inst, POST_KERNEL,				\
			      CONFIG_SENSOR_INIT_PRIORITY, &bma280_driver_api);			\

DT_INST_FOREACH_STATUS_OKAY(BMA280_DEFINE)
