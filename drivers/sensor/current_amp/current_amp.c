/*
 * Copyright (c) 2023 FTP Technologies
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT current_sense_amplifier

#include <stdlib.h>

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/current_sense_amplifier.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/__assert.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(current_amp, CONFIG_SENSOR_LOG_LEVEL);

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(gain_extended_range)
#define INST_HAS_EXTENDED_RANGE 1
#endif

struct current_sense_amplifier_data {
#ifdef INST_HAS_EXTENDED_RANGE
	const struct adc_channel_cfg *sample_channel_cfg;
	struct adc_channel_cfg channel_cfg_extended_range;
	int16_t adc_max;
#endif /* INST_HAS_EXTENDED_RANGE */
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
#ifdef INST_HAS_EXTENDED_RANGE
	data->sample_channel_cfg = &config->port.channel_cfg;

	/* Initial measurement hit the limits, and an alternate gain has been defined */
	if ((data->raw == data->adc_max) && (config->gain_extended_range != 0xFF)) {
		int ret2;

		/* Reconfigure channel for the extended range setup.
		 * Updating configurations should not fail since they were validated in `init`.
		 */
		ret = adc_channel_setup(config->port.dev, &data->channel_cfg_extended_range);
		__ASSERT_NO_MSG(ret == 0);
		/* Sample again at the higher range/lower resolution */
		ret = adc_read_dt(&config->port, &data->sequence);
		if (ret != 0) {
			LOG_ERR("adc_read: %d", ret);
		}

		/* Put channel back to original configuration */
		ret2 = adc_channel_setup_dt(&config->port);
		__ASSERT_NO_MSG(ret2 == 0);

		/* Sample was measured with the extended range configuration */
		data->sample_channel_cfg = &data->channel_cfg_extended_range;
	}
#endif /* INST_HAS_EXTENDED_RANGE */

	return ret;
}

static int get(const struct device *dev, enum sensor_channel chan, struct sensor_value *val)
{
	const struct current_sense_amplifier_dt_spec *config = dev->config;
	struct current_sense_amplifier_data *data = dev->data;
	int32_t v_uv = data->raw;
	int32_t i_ua;
	int ret;

	__ASSERT_NO_MSG(val != NULL);

	if (chan != SENSOR_CHAN_CURRENT) {
		return -ENOTSUP;
	}

	if (abs(data->raw) < config->noise_threshold) {
		return sensor_value_from_micro(val, 0);
	}

#ifdef INST_HAS_EXTENDED_RANGE
	ret = adc_raw_to_x_dt_chan(adc_raw_to_microvolts, &config->port, data->sample_channel_cfg,
				   &v_uv);
#else
	ret = adc_raw_to_microvolts_dt(&config->port, &v_uv);
#endif
	if (ret != 0) {
		LOG_ERR("raw_to_mv: %d", ret);
		return ret;
	}

	i_ua = current_sense_amplifier_scale_ua_dt(config, v_uv);
	LOG_DBG("%d/%d, %d uV, current:%d uA", data->raw, (1 << data->sequence.resolution) - 1,
		v_uv, i_ua);

	return sensor_value_from_micro(val, i_ua);
}

static DEVICE_API(sensor, current_api) = {
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

	__ASSERT(config->sense_milli_ohms != 0, "Milli-ohms must not be 0");

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

#ifdef INST_HAS_EXTENDED_RANGE
	if (config->gain_extended_range != 0xFF) {
		memcpy(&data->channel_cfg_extended_range, &config->port.channel_cfg,
		       sizeof(data->channel_cfg_extended_range));
		data->channel_cfg_extended_range.gain = config->gain_extended_range;
		data->adc_max = (1 << config->port.resolution) - 1;

		/* Test setup of configuration to catch invalid values */
		ret = adc_channel_setup(config->port.dev, &data->channel_cfg_extended_range);
		if (ret != 0) {
			LOG_ERR("ext_setup: %d", ret);
			return ret;
		}
	}
#endif /* INST_HAS_EXTENDED_RANGE */

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
	data->sequence.calibrate = config->enable_calibration;

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
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &current_api);      \
                                                                                                   \
	BUILD_ASSERT((DT_INST_PROP(inst, zero_current_voltage_mv) == 0) ||                         \
			     (DT_INST_PROP(inst, sense_resistor_milli_ohms) == 1),                 \
		     "zero_current_voltage_mv requires sense_resistor_milli_ohms == 1");

DT_INST_FOREACH_STATUS_OKAY(CURRENT_SENSE_AMPLIFIER_INIT)
