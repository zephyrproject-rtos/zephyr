/*
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * TM6605 Datasheet:
 *   https://dfimg.dfrobot.com/wiki/23281/
 *     DRI0056_gravity-tm6605-haptic-motor-driver-module_datasheet_V1.0.pdf
 */

#define DT_DRV_COMPAT titanmec_tm6605

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/haptics.h>
#include <zephyr/drivers/haptics/tm6605.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(TM6605, CONFIG_HAPTICS_LOG_LEVEL);

#define TM6605_REG_EFFECT  0x04
#define TM6605_REG_CONTROL 0x0C

#define TM6605_CONTROL_PLAY 0x01
#define TM6605_CONTROL_STOP 0x00

struct tm6605_config {
	struct i2c_dt_spec i2c;
};

struct tm6605_data {
	uint8_t selected_effect;
};

static bool tm6605_effect_is_valid(uint8_t effect)
{
	switch (effect) {
	case TM6605_EFFECT_SHARP_CLICK:
	case TM6605_EFFECT_INSTANT_CLICK:
	case TM6605_EFFECT_LIGHT_TAP:
	case TM6605_EFFECT_DOUBLE_CLICK:
	case TM6605_EFFECT_LIGHT_PULSE:
	case TM6605_EFFECT_STRONG_ALERT:
	case TM6605_EFFECT_MEDIUM_DURATION_ALERT:
	case TM6605_EFFECT_SHARP_CLICK_2:
	case TM6605_EFFECT_MEDIUM_CLICK:
	case TM6605_EFFECT_FLASH_STRIKE:
	case TM6605_EFFECT_DOUBLE_HIGH_CLICK_SHORT:
	case TM6605_EFFECT_DOUBLE_MEDIUM_CLICK_SHORT:
	case TM6605_EFFECT_DOUBLE_FLASH_STRIKE_SHORT:
	case TM6605_EFFECT_DOUBLE_INSTANT_CLICK_LONG:
	case TM6605_EFFECT_DOUBLE_MEDIUM_INSTANT_CLICK_LONG:
	case TM6605_EFFECT_DOUBLE_FLASH_STRIKE_LONG:
	case TM6605_EFFECT_ALERT:
	case TM6605_EFFECT_TOGGLE_CLICK:
	case TM6605_EFFECT_LONG_ALERT:
	case TM6605_EFFECT_SOFT_NOISE:
	case TM6605_EFFECT_SLEEP_COMMAND:
		return true;
	default:
		/* Effects 70..93 form two contiguous transition blocks. */
		return IN_RANGE(effect, TM6605_EFFECT_LONG_SLOW_FADE_1,
				TM6605_EFFECT_SHORT_FAST_BOOST_2);
	}
}

int tm6605_select_effect(const struct device *dev, enum tm6605_effect effect)
{
	struct tm6605_data *data = dev->data;

	if (!tm6605_effect_is_valid((uint8_t)effect)) {
		return -EINVAL;
	}

	data->selected_effect = (uint8_t)effect;

	return 0;
}

static int tm6605_start_output(const struct device *dev)
{
	const struct tm6605_config *config = dev->config;
	struct tm6605_data *data = dev->data;
	int ret;

	ret = i2c_reg_write_byte_dt(&config->i2c, TM6605_REG_EFFECT, data->selected_effect);
	if (ret < 0) {
		LOG_DBG("Failed to write effect 0x%02x: %d", data->selected_effect, ret);
		return ret;
	}

	ret = i2c_reg_write_byte_dt(&config->i2c, TM6605_REG_CONTROL, TM6605_CONTROL_PLAY);
	if (ret < 0) {
		LOG_DBG("Failed to start playback: %d", ret);
		return ret;
	}

	return 0;
}

static int tm6605_stop_output(const struct device *dev)
{
	const struct tm6605_config *config = dev->config;

	return i2c_reg_write_byte_dt(&config->i2c, TM6605_REG_CONTROL, TM6605_CONTROL_STOP);
}

static int tm6605_init(const struct device *dev)
{
	const struct tm6605_config *config = dev->config;
	int ret;

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR_DEVICE_NOT_READY(config->i2c.bus);
		return -ENODEV;
	}

	/*
	 * The TM6605 I2C interface is write-only, so the only way to probe
	 * for the device is to issue a write and check that it is ACKed.
	 * Writing 0 to the playback-control register is benign (it just
	 * disables playback) and doubles as the reset-to-known-state step.
	 */
	ret = i2c_reg_write_byte_dt(&config->i2c, TM6605_REG_CONTROL, TM6605_CONTROL_STOP);
	if (ret < 0) {
		LOG_ERR("TM6605 not responding on I2C: %d", ret);
		return ret;
	}

	return 0;
}

static int tm6605_select_source(const struct device *dev, const enum haptics_source src,
				const union haptics_config *const cfg)
{
	return -ENOTSUP;
}

static DEVICE_API(haptics, tm6605_driver_api) = {
	.select_source = &tm6605_select_source,
	.start_output = &tm6605_start_output,
	.stop_output = &tm6605_stop_output,
};

#define HAPTICS_TM6605_DEFINE(inst)                                                                \
	static const struct tm6605_config tm6605_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
                                                                                                   \
	static struct tm6605_data tm6605_data_##inst = {                                           \
		.selected_effect = DT_INST_PROP(inst, default_effect),                             \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, tm6605_init, NULL, &tm6605_data_##inst,                        \
			      &tm6605_config_##inst, POST_KERNEL, CONFIG_HAPTICS_INIT_PRIORITY,    \
			      &tm6605_driver_api);

DT_INST_FOREACH_STATUS_OKAY(HAPTICS_TM6605_DEFINE)
