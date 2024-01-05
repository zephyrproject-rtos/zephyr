/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ams_tsl2520

#include "tsl2520.h"

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>

#define TSL2520_CHIP_ID 0x5C

LOG_MODULE_REGISTER(tsl2520, CONFIG_SENSOR_LOG_LEVEL);

static int tsl2520_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct tsl2520_config *cfg = dev->config;
	struct tsl2520_data *data = dev->data;
	int ret = 0;

	__ASSERT_NO_MSG(chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT ||
			chan == SENSOR_CHAN_IR);

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT || chan == SENSOR_CHAN_IR) {
		uint8_t reg_status;
		uint16_t le16_buffer;

		/* In order to update ALS Data Registers ALS_STATUS must be read first */
		(void)i2c_reg_read_byte_dt(&cfg->i2c_spec, TSL2520_REG_STATUS, &reg_status);

		ret = i2c_reg_read_byte_dt(&cfg->i2c_spec, TSL2520_REG_STATUS, &reg_status);
		if (ret < 0) {
			LOG_ERR("Failed reading chip status");
			return -EIO;
		}
		ret = i2c_burst_read_dt(&cfg->i2c_spec, TSL2520_REG_ALS_DATA0_LOW,
					(uint8_t *)&le16_buffer, sizeof(le16_buffer));
		if (ret < 0) {
			LOG_ERR("Could not fetch als sensor value");
			return -EIO;
		}
		data->als_data0 = sys_le16_to_cpu(le16_buffer);
		ret = i2c_burst_read_dt(&cfg->i2c_spec, TSL2520_REG_ALS_DATA1_LOW,
					(uint8_t *)&le16_buffer, sizeof(le16_buffer));
		if (ret < 0) {
			LOG_ERR("Could not fetch als sensor value");
			return -EIO;
		}
		data->als_data1 = sys_le16_to_cpu(le16_buffer);
	}

	return ret;
}

static int tsl2520_channel_get(const struct device *dev, enum sensor_channel chan,
			       struct sensor_value *val)
{
	struct tsl2520_data *data = dev->data;

	if (chan == SENSOR_CHAN_ALL || chan == SENSOR_CHAN_LIGHT || chan == SENSOR_CHAN_IR) {
		val->val1 = data->als_data0;
		val->val2 = data->als_data1;
		return 0;
	}

	return -ENOTSUP;
}

static int tsl2520_setup(const struct device *dev)
{
	const struct tsl2520_config *cfg = dev->config;
	int ret;
	uint8_t tmp;
	uint16_t temp;

	ret = i2c_reg_write_byte_dt(&cfg->i2c_spec, TSL2520_ENABLE_ADDR, TSL2520_ENABLE_AEN_PON);
	if (ret < 0) {
		LOG_ERR("Failed to enable register");
		return ret;
	}

	tmp = cfg->sample_time;
	ret = i2c_reg_write_byte_dt(&cfg->i2c_spec, TSL2520_SAMPLE_TIME0, tmp);
	if (ret < 0) {
		LOG_ERR("Failed setting sample time");
		return ret;
	}

	temp = cfg->als_nr_samples;
	ret = i2c_burst_write_dt(&cfg->i2c_spec, TSL2520_ALS_NR_SAMPLES0, (uint8_t *)&temp,
				 sizeof(temp));
	if (ret < 0) {
		LOG_ERR("Failed setting als nr sample time");
		return ret;
	}

	temp = cfg->agc_nr_samples;
	ret = i2c_burst_write_dt(&cfg->i2c_spec, TSL2520_AGC_NR_SAMPLES_LOW, (uint8_t *)&temp,
				 sizeof(temp));
	if (ret < 0) {
		LOG_ERR("Failed setting agc nr sample time");
	}

	return ret;
}

static int tsl2520_init(const struct device *dev)
{
	const struct tsl2520_config *cfg = dev->config;
	uint8_t chip_id;
	int ret;

	if (!i2c_is_ready_dt(&cfg->i2c_spec)) {
		LOG_ERR("I2C dev %s not ready", cfg->i2c_spec.bus->name);
		return -ENODEV;
	}
	/* trying to read the id twice, as the sensor does not answer the first request */
	/* because of this no return code is checked in this line */
	(void)i2c_reg_read_byte_dt(&cfg->i2c_spec, TSL2520_REG_ID, &chip_id);

	ret = i2c_reg_read_byte_dt(&cfg->i2c_spec, TSL2520_REG_ID, &chip_id);
	if (ret < 0) {
		LOG_ERR("Failed reading chip id");
		return ret;
	}

	if (chip_id != TSL2520_CHIP_ID) {
		LOG_ERR("Chip id is invalid! Device @%02x is not TSL2520!", cfg->i2c_spec.addr);
		return -EIO;
	}

	if (tsl2520_setup(dev)) {
		LOG_ERR("Failed to setup ambient light functionality");
		return -EIO;
	}
	LOG_DBG("Init complete");

	return 0;
}

static const struct sensor_driver_api tsl2520_driver_api = {
	.sample_fetch = tsl2520_sample_fetch,
	.channel_get = tsl2520_channel_get,
};

#ifdef CONFIG_PM_DEVICE
static int tsl2520_pm_action(const struct device *dev, enum pm_device_action action)
{

	const struct tsl2520_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		return i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2520_ENABLE_ADDR,
					      TSL2520_ENABLE_MASK, TSL2520_ENABLE_MASK);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		return i2c_reg_update_byte_dt(&cfg->i2c_spec, TSL2520_ENABLE_ADDR,
					      TSL2520_ENABLE_MASK, TSL2520_ENABLE_DISABLE);
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

#define TSL2520_DEFINE(inst)                                                                       \
	static struct tsl2520_data tsl2520_prv_data_##inst;                                        \
	static const struct tsl2520_config tsl2520_config_##inst = {                               \
		.i2c_spec = I2C_DT_SPEC_INST_GET(inst),                                            \
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                \
		.sample_time = DT_INST_PROP(inst, sample_time),                                    \
		.als_nr_samples = DT_INST_PROP(inst, als_nr_samples),                              \
		.agc_nr_samples = DT_INST_PROP(inst, agc_nr_samples),                              \
		.als_gains = DT_INST_PROP(inst, als_gains),                                        \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, tsl2520_pm_action);                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &tsl2520_init, PM_DEVICE_DT_INST_GET(inst),             \
				     &tsl2520_prv_data_##inst, &tsl2520_config_##inst,             \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,                     \
				     &tsl2520_driver_api);

DT_INST_FOREACH_STATUS_OKAY(TSL2520_DEFINE)
