/*
 * Copyright (c) 2023 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT current_sense_amplifier

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/current_sense_amplifier.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(current_amp, CONFIG_SENSOR_LOG_LEVEL);

struct current_sense_amplifier_data {
	struct adc_sequence sequence;
	int16_t raw;
};

static int fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct current_sense_amplifier_dt_spec *config = dev->config;
	struct current_sense_amplifier_data *data = dev->data;
	int ret;

	if ((chan != SENSOR_CHAN_CURRENT) && (chan != SENSOR_CHAN_ALL)) {
		return -ENOTSUP;
	}

	ret = adc_read_dt(&config->port, &data->sequence);
	if (ret != 0) {
		LOG_ERR("adc_read: %d", ret);
	}

	return ret;
}

static int get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	const struct current_sense_amplifier_dt_spec *config = dev->config;
	struct current_sense_amplifier_data *data = dev->data;
	int32_t raw_val = data->raw;
	int32_t i_ma;
	int ret;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	ret = adc_raw_to_millivolts_dt(&config->port, &raw_val);
	if (ret != 0) {
		LOG_ERR("raw_to_mv: %d", ret);
		return ret;
	}

	i_ma = raw_val;
	current_sense_amplifier_scale_dt(config, &i_ma);

	LOG_DBG("%d/%d, %dmV, current:%dmA", data->raw,
		(1 << data->sequence.resolution) - 1, raw_val, i_ma);

	val->val1 = i_ma / 1000;
	val->val2 = (i_ma % 1000) * 1000;

	return 0;
}

static const struct sensor_driver_api current_api = {
	.sample_fetch = fetch,
	.channel_get = get,
};

#ifdef CONFIG_PM_DEVICE
static int pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct current_sense_amplifier_dt_spec *config = dev->config;
	int ret;

	if (config->power_gpio.port == NULL) {
		LOG_ERR("PM not supported");
		return -ENOTSUP;
	}

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = gpio_pin_set_dt(&config->power_gpio, 1);
		if (ret != 0) {
			LOG_ERR("failed to set GPIO for PM resume");
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = gpio_pin_set_dt(&config->power_gpio, 0);
		if (ret != 0) {
			LOG_ERR("failed to set GPIO for PM suspend");
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static int current_init(const struct device *dev)
{
	const struct current_sense_amplifier_dt_spec *config = dev->config;
	struct current_sense_amplifier_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&config->port)) {
		LOG_ERR("ADC is not ready");
		return -ENODEV;
	}

#ifdef CONFIG_PM_DEVICE
	if (config->power_gpio.port != NULL) {
		if (!gpio_is_ready_dt(&config->power_gpio)) {
			LOG_ERR("Power GPIO is not ready");
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(&config->power_gpio, GPIO_OUTPUT_ACTIVE);
		if (ret != 0) {
			LOG_ERR("failed to config GPIO: %d", ret);
			return ret;
		}
	}
#endif

	ret = adc_channel_setup_dt(&config->port);
	if (ret != 0) {
		LOG_ERR("setup: %d", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(&config->port, &data->sequence);
	if (ret != 0) {
		LOG_ERR("sequence init: %d", ret);
		return ret;
	}

	data->sequence.buffer = &data->raw;
	data->sequence.buffer_size = sizeof(data->raw);

	return 0;
}

#define CURRENT_SENSE_AMPLIFIER_INIT(inst)                                                         \
	static struct current_sense_amplifier_data current_amp_##inst##_data;                      \
                                                                                                   \
	static const struct current_sense_amplifier_dt_spec current_amp_##inst##_config =          \
		CURRENT_SENSE_AMPLIFIER_DT_SPEC_GET(DT_DRV_INST(inst));                            \
                                                                                                   \
	PM_DEVICE_DT_INST_DEFINE(inst, pm_action);                                                 \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, &current_init, PM_DEVICE_DT_INST_GET(inst),             \
				     &current_amp_##inst##_data, &current_amp_##inst##_config,     \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &current_api);

DT_INST_FOREACH_STATUS_OKAY(CURRENT_SENSE_AMPLIFIER_INIT)
