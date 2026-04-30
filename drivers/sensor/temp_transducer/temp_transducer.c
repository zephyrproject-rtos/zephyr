/*
 * Copyright (C) 2026 HawkEye 360
 * Copyright (C) 2026 Liam Beguin <liambeguin@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT temperature_transducer

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/temp_transducer.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

LOG_MODULE_REGISTER(temp_transducer, CONFIG_SENSOR_LOG_LEVEL);

struct temp_transducer_config {
	struct temp_transducer_dt_spec spec;
	struct gpio_dt_spec gpio_power;
	uint32_t sample_delay_us;
};

struct temp_transducer_data {
	struct adc_sequence sequence;
	k_timeout_t earliest_sample;
	int16_t raw;
};

static int temp_transducer_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct temp_transducer_config *config = dev->config;
	struct temp_transducer_data *data = dev->data;
	int ret;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* Wait until sampling is valid */
	k_sleep(data->earliest_sample);

	/* configure the active channel to be converted */
	ret = adc_channel_setup_dt(&config->spec.port);
	if (ret != 0) {
		LOG_ERR("adc_setup failed: %d", ret);
		return ret;
	}

	/* start conversion */
	ret = adc_read_dt(&config->spec.port, &data->sequence);
	if (ret < 0) {
		LOG_ERR("ADC read failed: %d", ret);
		return ret;
	}

	return 0;
}

static int temp_transducer_channel_get(const struct device *dev, enum sensor_channel chan,
					struct sensor_value *val)
{
	const struct temp_transducer_config *config = dev->config;
	struct temp_transducer_data *data = dev->data;
	int32_t millivolts = data->raw;
	int32_t millicelsius;
	int ret;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	ret = adc_raw_to_millivolts_dt(&config->spec.port, &millivolts);
	if (ret < 0) {
		LOG_ERR("ADC raw to millivolts conversion failed: %d", ret);
		return ret;
	}

	millicelsius = temp_transducer_scale_dt(&config->spec, millivolts);

	/* Convert to sensor_value (val1 = degrees, val2 = micro-degrees) */
	val->val1 = millicelsius / 1000;
	val->val2 = (millicelsius % 1000) * 1000;

	return 0;
}

static int pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct temp_transducer_config *config = dev->config;
	struct temp_transducer_data *data = dev->data;
	int ret = 0;

	if (!config->gpio_power.port) {
		return 0;
	}

	switch (action) {
	case PM_DEVICE_ACTION_TURN_ON:
		ret = gpio_pin_configure_dt(&config->gpio_power, GPIO_OUTPUT_INACTIVE);
		if (ret != 0) {
			LOG_ERR("failed to configure GPIO for PM on");
		}
		break;
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_pin_set_dt(&config->gpio_power, 1);
		if (ret != 0) {
			LOG_ERR("failed to set GPIO for PM resume");
		}
		data->earliest_sample = K_TIMEOUT_ABS_TICKS(
			k_uptime_ticks() + k_us_to_ticks_ceil32(config->sample_delay_us));
		/* Power up */
		pm_device_runtime_get(config->spec.port.dev);
		break;
#ifdef CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gpio_pin_set_dt(&config->gpio_power, 0);
		if (ret != 0) {
			LOG_ERR("failed to set GPIO for PM suspend");
		}
		/* Power down */
		pm_device_runtime_put(config->spec.port.dev);
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
#endif
	default:
		return -ENOTSUP;
	}

	return ret;
}

static int temp_transducer_init(const struct device *dev)
{
	const struct temp_transducer_config *config = dev->config;
	struct temp_transducer_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&config->spec.port)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	if (config->gpio_power.port) {
		if (!gpio_is_ready_dt(&config->gpio_power)) {
			LOG_ERR("Power GPIO is not ready");
			return -ENODEV;
		}
	}

	data->earliest_sample = K_TIMEOUT_ABS_TICKS(0);

	ret = adc_channel_setup_dt(&config->spec.port);
	if (ret < 0) {
		LOG_ERR("ADC channel setup failed: %d", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(&config->spec.port, &data->sequence);
	if (ret < 0) {
		LOG_ERR("ADC sequence init failed: %d", ret);
		return ret;
	}

	data->sequence.buffer = &data->raw;
	data->sequence.buffer_size = sizeof(data->raw);

	return pm_device_driver_init(dev, pm_action);
}

static DEVICE_API(sensor, temp_transducer_api) = {
	.sample_fetch = temp_transducer_sample_fetch,
	.channel_get = temp_transducer_channel_get,
};

#define TEMP_TRANSDUCER_INIT(inst)                                                                 \
	static struct temp_transducer_data temp_transducer_data_##inst;                            \
                                                                                                   \
	static const struct temp_transducer_config temp_transducer_config_##inst = {               \
		.spec = TEMP_TRANSDUCER_DT_SPEC_GET(DT_DRV_INST(inst)),                            \
		.gpio_power = GPIO_DT_SPEC_INST_GET_OR(inst, power_gpios, {0}),                    \
		.sample_delay_us = DT_INST_PROP(inst, power_on_sample_delay_us),                   \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, pm_action);                                                 \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, temp_transducer_init, PM_DEVICE_DT_INST_GET(inst),      \
				     &temp_transducer_data_##inst,                                 \
				     &temp_transducer_config_##inst, POST_KERNEL,                  \
				     CONFIG_SENSOR_INIT_PRIORITY, &temp_transducer_api);

DT_INST_FOREACH_STATUS_OKAY(TEMP_TRANSDUCER_INIT)
