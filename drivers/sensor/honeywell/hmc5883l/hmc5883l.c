/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT honeywell_hmc5883l

#include <zephyr/drivers/i2c.h>
#include <zephyr/init.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/drivers/sensor.h>
#include <string.h>
#include <zephyr/logging/log.h>

#include "hmc5883l.h"

LOG_MODULE_REGISTER(HMC5883L, CONFIG_SENSOR_LOG_LEVEL);

static void hmc5883l_convert(struct sensor_value *val, int16_t raw_val,
			     uint16_t divider)
{
	/* val = raw_val / divider */
	val->val1 = raw_val / divider;
	val->val2 = (((int64_t)raw_val % divider) * 1000000L) / divider;
}

static int hmc5883l_channel_get(const struct device *dev,
				enum sensor_channel chan,
				struct sensor_value *val)
{
	struct hmc5883l_data *drv_data = dev->data;

	if (chan == SENSOR_CHAN_MAGN_X) {
		hmc5883l_convert(val, drv_data->x_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
	} else if (chan == SENSOR_CHAN_MAGN_Y) {
		hmc5883l_convert(val, drv_data->y_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
	} else if (chan == SENSOR_CHAN_MAGN_Z) {
		hmc5883l_convert(val, drv_data->z_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
	} else if (chan == SENSOR_CHAN_MAGN_XYZ) {
		hmc5883l_convert(val, drv_data->x_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
		hmc5883l_convert(val + 1, drv_data->y_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
		hmc5883l_convert(val + 2, drv_data->z_sample,
				 hmc5883l_gain[drv_data->gain_idx]);
	} else {
		return -ENOTSUP;
	}

	return 0;
}

static int hmc5883l_sample_fetch(const struct device *dev,
				 enum sensor_channel chan)
{
	struct hmc5883l_data *drv_data = dev->data;
	const struct hmc5883l_config *config = dev->config;
	int16_t buf[3];

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL);

	/* fetch magnetometer sample */
	if (i2c_burst_read_dt(&config->i2c, HMC5883L_REG_DATA_START,
			      (uint8_t *)buf, 6) < 0) {
		LOG_ERR("Failed to fetch magnetometer sample.");
		return -EIO;
	}

	drv_data->x_sample = sys_be16_to_cpu(buf[0]);
	drv_data->z_sample = sys_be16_to_cpu(buf[1]);
	drv_data->y_sample = sys_be16_to_cpu(buf[2]);

	return 0;
}

static const struct sensor_driver_api hmc5883l_driver_api = {
#if CONFIG_HMC5883L_TRIGGER
	.trigger_set = hmc5883l_trigger_set,
#endif
	.sample_fetch = hmc5883l_sample_fetch,
	.channel_get = hmc5883l_channel_get,
};

int hmc5883l_init(const struct device *dev)
{
	struct hmc5883l_data *drv_data = dev->data;
	const struct hmc5883l_config *config = dev->config;
	uint8_t chip_cfg[3], id[3], idx;

	if (!device_is_ready(config->i2c.bus)) {
		LOG_ERR("I2C bus device not ready");
		return -ENODEV;
	}

	/* check chip ID */
	if (i2c_burst_read_dt(&config->i2c, HMC5883L_REG_CHIP_ID, id, 3) < 0) {
		LOG_ERR("Failed to read chip ID.");
		return -EIO;
	}

	if (id[0] != HMC5883L_CHIP_ID_A || id[1] != HMC5883L_CHIP_ID_B ||
	    id[2] != HMC5883L_CHIP_ID_C) {
		LOG_ERR("Invalid chip ID.");
		return -EINVAL;
	}

	/* check if CONFIG_HMC5883L_FS is valid */
	for (idx = 0U; idx < ARRAY_SIZE(hmc5883l_fs_strings); idx++) {
		if (!strcmp(hmc5883l_fs_strings[idx], CONFIG_HMC5883L_FS)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(hmc5883l_fs_strings)) {
		LOG_ERR("Invalid full-scale range value.");
		return -EINVAL;
	}

	drv_data->gain_idx = idx;

	/* check if CONFIG_HMC5883L_ODR is valid */
	for (idx = 0U; idx < ARRAY_SIZE(hmc5883l_odr_strings); idx++) {
		if (!strcmp(hmc5883l_odr_strings[idx], CONFIG_HMC5883L_ODR)) {
			break;
		}
	}

	if (idx == ARRAY_SIZE(hmc5883l_odr_strings)) {
		LOG_ERR("Invalid ODR value.");
		return -EINVAL;
	}

	/* configure device */
	chip_cfg[0] = idx << HMC5883L_ODR_SHIFT;
	chip_cfg[1] = drv_data->gain_idx << HMC5883L_GAIN_SHIFT;
	chip_cfg[2] = HMC5883L_MODE_CONTINUOUS;

	if (i2c_burst_write_dt(&config->i2c, HMC5883L_REG_CONFIG_A,
			       chip_cfg, 3) < 0) {
		LOG_ERR("Failed to configure chip.");
		return -EIO;
	}

#ifdef CONFIG_HMC5883L_TRIGGER
	if (config->int_gpio.port) {
		if (hmc5883l_init_interrupt(dev) < 0) {
			LOG_ERR("Failed to initialize interrupts.");
			return -EIO;
		}
	}
#endif

	return 0;
}

#define HMC5883L_DEFINE(inst)									\
	static struct hmc5883l_data hmc5883l_data_##inst;					\
												\
	static const struct hmc5883l_config hmc5883l_config_##inst = {				\
		.i2c = I2C_DT_SPEC_INST_GET(inst),						\
		IF_ENABLED(CONFIG_HMC5883L_TRIGGER,						\
			   (.int_gpio = GPIO_DT_SPEC_INST_GET_OR(inst, int_gpios, { 0 }),))	\
	};											\
												\
	SENSOR_DEVICE_DT_INST_DEFINE(inst, hmc5883l_init, NULL,					\
			      &hmc5883l_data_##inst, &hmc5883l_config_##inst, POST_KERNEL,	\
			      CONFIG_SENSOR_INIT_PRIORITY, &hmc5883l_driver_api);		\

DT_INST_FOREACH_STATUS_OKAY(HMC5883L_DEFINE)
