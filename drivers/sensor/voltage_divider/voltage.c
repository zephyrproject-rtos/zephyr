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
};

struct voltage_data {
	struct adc_sequence sequence;
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

static const struct sensor_driver_api voltage_api = {
	.sample_fetch = fetch,
	.channel_get = get,
};

#ifdef CONFIG_PM_DEVICE
static int pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct voltage_config *config = dev->config;
	int ret;

	if (config->gpio_power.port == NULL) {
		/* No work to do */
		return 0;
	}

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_pin_set_dt(&config->gpio_power, 1);
		if (ret != 0) {
			LOG_ERR("failed to set GPIO for PM resume");
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gpio_pin_set_dt(&config->gpio_power, 0);
		if (ret != 0) {
			LOG_ERR("failed to set GPIO for PM suspend");
		}
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif

static int voltage_init(const struct device *dev)
{
	const struct voltage_config *config = dev->config;
	struct voltage_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&config->voltage.port)) {
		LOG_ERR("ADC is not ready");
		return -ENODEV;
	}

	if (config->gpio_power.port != NULL) {
		if (!gpio_is_ready_dt(&config->gpio_power)) {
			LOG_ERR("Power GPIO is not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->gpio_power, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("failed to initialize GPIO for reset");
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

	return 0;
}

#define VOLTAGE_INIT(inst)                                                                         \
	static struct voltage_data voltage_##inst##_data;                                          \
                                                                                                   \
	static const struct voltage_config voltage_##inst##_config = {                             \
		.voltage = VOLTAGE_DIVIDER_DT_SPEC_GET(DT_DRV_INST(inst)),                         \
		.gpio_power = GPIO_DT_SPEC_INST_GET_OR(inst, power_gpios, {0}),                    \
	};                                                                                         \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, pm_action);                                                 \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &voltage_init, PM_DEVICE_DT_INST_GET(inst),             \
			      &voltage_##inst##_data, &voltage_##inst##_config, POST_KERNEL,       \
			      CONFIG_SENSOR_INIT_PRIORITY, &voltage_api);

DT_INST_FOREACH_STATUS_OKAY(VOLTAGE_INIT)
