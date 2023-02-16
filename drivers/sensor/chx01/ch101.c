/*
 * Copyright (c) 2023 Google LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT invensense_ch101

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/sensor/ch101.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "soniclib.h"
#include "ch101_gpr.h"
#include "ch101_gpr_sr.h"
#include "chx01_common.h"

LOG_MODULE_REGISTER(CH101, CONFIG_SENSOR_LOG_LEVEL);

static int ch101_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct ch101_data *data = dev->data;
	uint32_t range;

	switch (chan) {
	case SENSOR_CHAN_DISTANCE:
	case SENSOR_CHAN_ALL:
		range = ch_get_range(&data->ch_driver, CH_RANGE_ECHO_ONE_WAY);

		if (range == 0) {
			LOG_ERR("Failed to calculate range");
			return -EIO;
		}
		if (range == CH_NO_TARGET) {
			LOG_DBG("No target detected");
			data->range_um = ((int64_t)INT32_MAX << 32) + 999999;
		} else {
			data->range_um = (range * 1000) / 32;
			LOG_DBG("Range = %" PRIi64 "um", data->range_um);
		}
		return 0;
	default:
		return -EINVAL;
	}
}

static int ch101_channel_get(const struct device *dev, enum sensor_channel chan,
			     struct sensor_value *val)
{
	struct ch101_data *data = dev->data;

	switch (chan) {
	case SENSOR_CHAN_DISTANCE:
	case SENSOR_CHAN_ALL:
		val->val1 = data->range_um / 1000000;
		val->val2 = data->range_um - (val->val1 * 1000000);
		return 0;
	default:
		return -EINVAL;
	}
}

static int ch101_attr_set(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, const struct sensor_value *val)
{
	struct ch101_data *data = dev->data;
	ch_config_t dev_config;
	int64_t uhz;

	switch (chan) {
	case SENSOR_CHAN_DISTANCE:
		if (attr != SENSOR_ATTR_SAMPLING_FREQUENCY) {
			return -EINVAL;
		}
		if (ch_get_config(&data->ch_driver, &dev_config)) {
			LOG_ERR("Failed to get current configuration");
			return -EINVAL;
		}

		uhz = val->val1 * 1000000 + val->val2;

		if (uhz == 0) {
			dev_config.sample_interval = 0;
		} else {
			dev_config.sample_interval = 1000000000 / val->val1;
		}
		if (ch_set_config(&data->ch_driver, &dev_config)) {
			LOG_ERR("Failed to set configuration");
			return -EIO;
		}
		break;
	default:
		return -ENOTSUP;
	}
	return -EINVAL;
}

static int ch101_attr_get(const struct device *dev, enum sensor_channel chan,
			  enum sensor_attribute attr, struct sensor_value *val)
{
	struct ch101_data *data = dev->data;
	ch_config_t dev_config;

	switch (chan) {
	case SENSOR_CHAN_DISTANCE:
		if (attr != SENSOR_ATTR_SAMPLING_FREQUENCY) {
			return -EINVAL;
		}
		if (ch_get_config(&data->ch_driver, &dev_config)) {
			LOG_ERR("Failed to get current configuration");
			return -EINVAL;
		}

		if (dev_config.sample_interval == 0) {
			val->val1 = 0;
			val->val2 = 0;
		} else {
			int64_t uhz = 1000000000 / dev_config.sample_interval;

			val->val1 = uhz / 1000000;
			val->val2 = uhz - (val->val1 * 1000000);
		}
		return 0;
	default:
		return -ENOTSUP;
	}
}

static const struct sensor_driver_api ch101_driver_api = {
	.sample_fetch = ch101_sample_fetch,
	.channel_get = ch101_channel_get,
	.attr_set = ch101_attr_set,
	.attr_get = ch101_attr_get,
};

static int ch101_init(const struct device *dev)
{
	const struct ch101_config *cfg = dev->config;
	struct ch101_data *data = dev->data;

	data->dev = dev;

	/* TODO actually enable groups */
	data->ch_group.num_ports = 1;
	data->ch_group.rtc_cal_pulse_ms = 200;

	switch (cfg->default_firmware) {
#ifdef CONFIG_CH101_GPR_FW
	case DEFAULT_FIRMWARE_GPR:
		LOG_DBG("Loading GPR firmware");
		if (ch_init(&data->ch_driver, &data->ch_group, 0, ch101_gpr_init)) {
			LOG_ERR("Failed to init GPR firmware");
			return -ENODEV;
		}
		break;
#endif
#ifdef CONFIG_CH101_GPR_SR_FW
	case DEFAULT_FIRMWARE_GPR_SR:
		LOG_DBG("Loading GPR-SR firmware");
		if (ch_init(&data->ch_driver, &data->ch_group, 0, ch101_gpr_sr_init)) {
			LOG_ERR("Failed to init GPR firmware");
			return -ENODEV;
		}
		break;
#endif
	case DEFAULT_FIRMWARE_NONE:
	default:
		break;
	}

	if (ch_group_start(&data->ch_group)) {
		LOG_ERR("Failed to start group");
		return -ENODEV;
	}

	ch_config_t dev_config = {
		.mode = CH_MODE_FREERUN,
		.max_range = UINT16_C(1000),
		.static_range = UINT16_C(0),
		.sample_interval = 0,
		.thresh_ptr = NULL,
		.time_plan = CH_TIME_PLAN_NONE,
		.enable_target_int = 1,
	};

	if (ch_set_config(&data->ch_driver, &dev_config)) {
		LOG_ERR("Failed to configure sensor");
		return -ENODEV;
	}

	return 0;
}

#define CH101_DEFINE(inst)                                                                         \
	BUILD_ASSERT(DT_INST_REG_ADDR(inst) < 0xff);                                               \
	static struct ch101_data ch101_data_##inst = {                                             \
		.ch_driver =                                                                       \
			{                                                                          \
				.i2c_address = DT_INST_REG_ADDR(inst),                             \
				.app_i2c_address = DT_INST_REG_ADDR(inst),                         \
				.i2c_drv_flags = 0,                                                \
				.part_number = CH101_PART_NUMBER,                                  \
			},                                                                         \
	};                                                                                         \
	static struct ch101_config ch101_config_##inst = {                                         \
		.common_config =                                                                   \
			{                                                                          \
				.i2c = I2C_DT_SPEC_INST_GET(inst),                                 \
				.gpio_int = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                \
				.gpio_program = GPIO_DT_SPEC_INST_GET(inst, program_gpios),        \
				.gpio_reset = GPIO_DT_SPEC_INST_GET(inst, reset_gpios),            \
			},                                                                         \
		.default_firmware = UTIL_CAT(DEFAULT_FIRMWARE_,                                    \
					     DT_INST_STRING_UPPER_TOKEN_OR(inst, firmware, NONE)), \
	};                                                                                         \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, ch101_init, NULL, &ch101_data_##inst,                   \
				     &ch101_config_##inst, POST_KERNEL,                            \
				     CONFIG_SENSOR_INIT_PRIORITY, &ch101_driver_api);

DT_INST_FOREACH_STATUS_OKAY(CH101_DEFINE)
