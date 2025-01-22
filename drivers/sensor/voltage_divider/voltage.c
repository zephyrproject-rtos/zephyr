/*
 * Copyright (c) 2023 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT voltage_divider

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/voltage_divider.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(voltage, CONFIG_SENSOR_LOG_LEVEL);

struct voltage_config {
	struct voltage_divider_dt_spec voltage;
	struct gpio_dt_spec gpio_power;
	uint32_t sample_delay_us;
};

struct voltage_data {
	struct adc_sequence sequence;
	k_timeout_t earliest_sample;
	uint16_t raw;
};

static int fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct voltage_config *config = dev->config;
	struct voltage_data *data = dev->data;
	int ret;

	if ((chan != SENSOR_CHAN_VOLTAGE) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	/* Wait until sampling is valid */
	k_sleep(data->earliest_sample);

	/* configure the active channel to be converted */
	ret = adc_channel_setup_dt(&config->voltage.port);
	if (ret != 0) {
		LOG_ERR("adc_setup failed: %d", ret);
		return ret;
	}

	/* start conversion */
	ret = adc_read(config->voltage.port.dev, &data->sequence);
	if (ret != 0) {
		LOG_ERR("adc_read: %d", ret);
	}

	return ret;
}

static int get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	const struct voltage_config *config = dev->config;
	struct voltage_data *data = dev->data;
	int32_t raw_val;
	int32_t v_mv;
	int ret;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	if (config->voltage.port.channel_cfg.differential) {
		raw_val = (int16_t)data->raw;
	} else {
		raw_val = data->raw;
	}

	ret = adc_raw_to_millivolts_dt(&config->voltage.port, &raw_val);
	if (ret != 0) {
		LOG_ERR("raw_to_mv: %d", ret);
		return ret;
	}

	v_mv = raw_val;

	/* Note if full_ohms is not specified then unscaled voltage is returned */
	(void)voltage_divider_scale_dt(&config->voltage, &v_mv);

	LOG_DBG("%d of %d, %dmV, voltage:%dmV", data->raw,
		(1 << data->sequence.resolution) - 1, raw_val, v_mv);
	val->val1 = v_mv / 1000;
	val->val2 = (v_mv * 1000) % 1000000;

	return ret;
}

static DEVICE_API(sensor, voltage_api) = {
	.sample_fetch = fetch,
	.channel_get = get,
};

static int pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct voltage_config *config = dev->config;
	struct voltage_data *data = dev->data;
	int ret = 0;

	if (config->gpio_power.port == NULL) {
		/* No work to do */
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
		break;
#ifdef CONFIG_PM_DEVICE
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gpio_pin_set_dt(&config->gpio_power, 0);
		if (ret != 0) {
			LOG_ERR("failed to set GPIO for PM suspend");
		}
		break;
	case PM_DEVICE_ACTION_TURN_OFF:
		break;
#endif /* CONFIG_PM_DEVICE */
	default:
		return -ENOTSUP;
	}

	return ret;
}

static int voltage_init(const struct device *dev)
{
	const struct voltage_config *config = dev->config;
	struct voltage_data *data = dev->data;
	int ret;

	/* Default value to use if `power-gpios` does not exist */
	data->earliest_sample = K_TIMEOUT_ABS_TICKS(0);

	if (!adc_is_ready_dt(&config->voltage.port)) {
		LOG_ERR("ADC is not ready");
		return -ENODEV;
	}

	if (config->gpio_power.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_power)) {
			LOG_ERR("Power GPIO is not ready");
			return -ENODEV;
		}
	}

	ret = adc_channel_setup_dt(&config->voltage.port);
	if (ret != 0) {
		LOG_ERR("setup: %d", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(&config->voltage.port, &data->sequence);
	if (ret != 0) {
		LOG_ERR("sequence init: %d", ret);
		return ret;
	}

	data->sequence.buffer = &data->raw;
	data->sequence.buffer_size = sizeof(data->raw);

	return pm_device_driver_init(dev, pm_action);
}

#define VOLTAGE_INIT(inst)                                                                         \
	static struct voltage_data voltage_##inst##_data;                                          \
                                                                                                   \
	static const struct voltage_config voltage_##inst##_config = {                             \
		.voltage = VOLTAGE_DIVIDER_DT_SPEC_GET(DT_DRV_INST(inst)),                         \
		.gpio_power = GPIO_DT_SPEC_INST_GET_OR(inst, power_gpios, {0}),                    \
		.sample_delay_us = DT_INST_PROP(inst, power_on_sample_delay_us),                   \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, pm_action);                                                 \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &voltage_init, PM_DEVICE_DT_INST_GET(inst),             \
			      &voltage_##inst##_data, &voltage_##inst##_config, POST_KERNEL,       \
			      CONFIG_SENSOR_INIT_PRIORITY, &voltage_api);

DT_INST_FOREACH_STATUS_OKAY(VOLTAGE_INIT)
