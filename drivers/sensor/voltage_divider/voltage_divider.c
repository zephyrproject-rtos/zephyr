/*
 * Copyright (c) 2022 Aurelien Jarno
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(voltage_divider, CONFIG_SENSOR_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(voltage_divider)
#define DT_DRV_COMPAT voltage_divider
#else
#error "No compatible devicetree node found"
#endif

struct voltage_divider_data {
	const struct adc_dt_spec adc_channel;
	const struct gpio_dt_spec power_gpios;
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	int16_t sample_buffer;
	int16_t raw; /* raw adc Sensor value */
};

struct voltage_divider_config {
	/* output_ohm is used as a flag value: if it is nonzero then
	 * the voltage is measured through a voltage divider;
	 * otherwise it is assumed to be directly connected.
	 */
	uint32_t output_ohms;
	uint32_t full_ohms;
};

static int voltage_divider_enable(const struct device *dev, bool enable)
{
	struct voltage_divider_data *data = dev->data;
	int rc = 0;

	if (data->power_gpios.port) {
		rc = gpio_pin_set_dt(&data->power_gpios, 1);
		if (rc) {
			LOG_ERR("Failed to set GPIO %s.%u: %d",
				data->power_gpios.port->name, data->power_gpios.pin, rc);
		}
	}

	return rc;
}

static int voltage_divider_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct voltage_divider_data *data = dev->data;
	struct adc_sequence *sp = &data->adc_seq;
	int rc;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	rc = voltage_divider_enable(dev, 1);
	if (rc) {
		goto unlock;
	}

	rc = adc_channel_setup_dt(&data->adc_channel);

	if (rc) {
		LOG_ERR("Setup AIN%u got %d", data->adc_channel.channel_id, rc);
		goto unlock;
	}

	rc = adc_read(data->adc_channel.dev, sp);
	if (rc == 0) {
		data->raw = data->sample_buffer;
	}

unlock:
	rc = voltage_divider_enable(dev, 0);

	k_mutex_unlock(&data->mutex);

	return rc;
}

static int voltage_divider_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct voltage_divider_data *data = dev->data;
	const struct voltage_divider_config *cfg = dev->config;
	int32_t mv;
	float voltage;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	/* Sensor value in millivolts */
	mv = data->raw;
	adc_raw_to_millivolts_dt(&data->adc_channel, &mv);

	/* Convert to volts */
	voltage = mv / 1000.0f;

	/* considering the voltage input through a resistor bridge */
	if (cfg->output_ohms != 0) {
		voltage *= (float)cfg->full_ohms / cfg->output_ohms;
	}

	return sensor_value_from_double(val, voltage);
}

static const struct sensor_driver_api voltage_divider_driver_api = {
	.sample_fetch = voltage_divider_sample_fetch,
	.channel_get = voltage_divider_channel_get,
};

static int voltage_divider_init(const struct device *dev)
{
	struct voltage_divider_data *data = dev->data;
	struct adc_sequence *asp = &data->adc_seq;
	int rc;

	k_mutex_init(&data->mutex);

	if (!device_is_ready(data->adc_channel.dev)) {
		LOG_ERR("Device %s is not ready", data->adc_channel.dev->name);
		return -ENODEV;
	}

	/* Configure the power GPIO if available */
	if (data->power_gpios.port) {
		if (!device_is_ready(data->power_gpios.port)) {
			LOG_ERR("Failed to get GPIO %s", data->power_gpios.port->name);
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(&data->power_gpios, GPIO_OUTPUT_INACTIVE);
		if (rc) {
			LOG_ERR("Failed to control feed %s.%u: %d",
				data->power_gpios.port->name, data->power_gpios.pin, rc);
			return rc;
		}
	}

	adc_sequence_init_dt(&data->adc_channel, asp);
	asp->buffer = &data->sample_buffer;
	asp->buffer_size = sizeof(data->sample_buffer);

	return 0;
}

/* Main instantiation macro */
#define DEFINE_VOLTAGE_DIVIDER(inst) \
	static const struct voltage_divider_config voltage_divider_dev_config_##inst = { \
		.output_ohms = DT_INST_PROP(inst, output_ohms), \
		.full_ohms = DT_INST_PROP(inst, full_ohms), \
	}; \
	static struct voltage_divider_data voltage_divider_dev_data_##inst = { \
		.adc_channel = ADC_DT_SPEC_INST_GET(inst), \
		.power_gpios = GPIO_DT_SPEC_INST_GET_OR(inst, power_gpios, {0}) \
	}; \
	DEVICE_DT_INST_DEFINE(inst, voltage_divider_init, NULL, \
		&voltage_divider_dev_data_##inst, &voltage_divider_dev_config_##inst, \
		POST_KERNEL, \
		CONFIG_SENSOR_INIT_PRIORITY, &voltage_divider_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DEFINE_VOLTAGE_DIVIDER)
