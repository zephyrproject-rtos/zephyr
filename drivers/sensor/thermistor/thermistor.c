/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include <math.h>

#include "thermistor.h"

#define DT_DRV_COMPAT infineon_thermistor

#if !DT_NODE_EXISTS(DT_DRV_INST(0)) || \
	!DT_INST_NODE_HAS_PROP(0, io_channels)
	#error "No suitable devicetree overlay specified"
#endif


LOG_MODULE_REGISTER(thermistor, CONFIG_SENSOR_LOG_LEVEL);
#define CAL_RES 12

/* Zero Kelvin in degree C */
#define ABSOLUTE_ZERO (double)(-273.15)

struct thermistor_data {
	struct adc_sequence adc_seq;
	struct k_mutex mutex;
	uint16_t sample_buffer;
	uint16_t voltage_ref;
	uint16_t voltage_therm;
};

struct thermistor_config {
	const struct adc_dt_spec adc_chan;
	const struct gpio_dt_spec vdd_gpio;
	const struct gpio_dt_spec gnd_gpio;
	double r_ref;           /* Resistance of the reference resistor */
	double b_const;         /* Beta constant of the thermistor */
	double r_infinity;      /* Projected resistance of the thermistor at infinity */
	uint8_t wiring;         /* How the thermistor is wired up */
};

static int thermistor_init(const struct device *dev)
{
	const struct thermistor_config *cfg = dev->config;
	struct thermistor_data *data = dev->data;
	int return_val;

	k_mutex_init(&data->mutex);

	/* VDD pin initialization */
	if (!device_is_ready(cfg->vdd_gpio.port)) {
		LOG_ERR("GPIO port for vdd pin is not ready");
		return -ENODEV;
	}
	return_val = gpio_pin_configure_dt(&cfg->vdd_gpio, GPIO_OUTPUT_LOW);
	if (return_val < 0) {
		LOG_ERR("Configuration failure: vdd pin (%d)", return_val);
		return return_val;
	}

	/* GND pin initialization */
	if (!device_is_ready(cfg->gnd_gpio.port)) {
		LOG_ERR("GPIO port for gnd pin is not ready");
		return -ENODEV;
	}
	return_val = gpio_pin_configure_dt(&cfg->gnd_gpio, GPIO_OUTPUT_LOW);
	if (return_val < 0) {
		LOG_ERR("Configuration failure: gnd pin (%d)", return_val);
		return return_val;
	}

	/* ADC initialization */
	if (!device_is_ready(cfg->adc_chan.dev)) {
		LOG_ERR("Device %s is not ready", cfg->adc_chan.dev->name);
		return -ENODEV;
	}

	data->adc_seq = (struct adc_sequence) {
		.buffer = &data->sample_buffer,
		.buffer_size = sizeof(data->sample_buffer),
	};

	return_val = adc_channel_setup_dt(&cfg->adc_chan);
	if (return_val < 0) {
		LOG_ERR("Could not setup ADC channel (%d)\n", return_val);
		return return_val;
	}

	return 0;
}

static int thermistor_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct thermistor_data *data = dev->data;
	const struct thermistor_config *cfg = dev->config;
	int return_val;

	if ((chan != SENSOR_CHAN_ALL) && (chan != SENSOR_CHAN_AMBIENT_TEMP)) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->mutex, K_FOREVER);

	return_val = adc_sequence_init_dt(&cfg->adc_chan, &data->adc_seq);

	if (return_val == 0) {
		/* Measure voltage drop on the reference resistor */
		gpio_pin_set_dt(&cfg->vdd_gpio, 0);
		gpio_pin_set_dt(&cfg->gnd_gpio, 1);
		return_val = adc_read(cfg->adc_chan.dev, &data->adc_seq);

		if (return_val == 0) {
			data->voltage_ref = data->sample_buffer;

			/* Measure voltage drop on the thermistor */
			gpio_pin_set_dt(&cfg->vdd_gpio, 1);
			gpio_pin_set_dt(&cfg->gnd_gpio, 0);
			return_val = adc_read(cfg->adc_chan.dev, &data->adc_seq);

			if (return_val == 0) {
				data->voltage_therm = data->sample_buffer;
			}
		}
	}

	/* Disable power to thermistor circuit */
	gpio_pin_set_dt(&cfg->vdd_gpio, 0);

	k_mutex_unlock(&data->mutex);

	return return_val;
}

static int thermistor_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	struct thermistor_data *data = dev->data;
	const struct thermistor_config *cfg = dev->config;

	if (chan != SENSOR_CHAN_AMBIENT_TEMP) {
		return -ENOTSUP;
	}

	/* Calculate thermistor resistance */
	double rThermistor = (cfg->wiring == THERMISTOR_WIRING_VIN_R_THERM_GND)
		? ((cfg->r_ref * (data->voltage_therm)) / ((double)(data->voltage_ref)))
		: ((cfg->r_ref * (data->voltage_ref)) / ((double)(data->voltage_therm)));

	/* Calculate thermistor temperature */
	double temperature = (cfg->b_const / (logf(rThermistor / cfg->r_infinity))) +
			     ABSOLUTE_ZERO;

	return sensor_value_from_double(val, temperature);
}

static const struct sensor_driver_api thermistor_driver_api = {
	.sample_fetch = thermistor_sample_fetch,
	.channel_get = thermistor_channel_get,
};

/* Macros for THERMISTOR instance declaration */
#define THERMISTOR_DEFINE(n)									 \
	static struct thermistor_data thermistor_dev_data_##inst;				 \
												 \
	static const struct thermistor_config thermistor_dev_config_##inst = {			 \
		.adc_chan = ADC_DT_SPEC_INST_GET(n),						 \
		.vdd_gpio = GPIO_DT_SPEC_INST_GET(n, vdd_gpios),				 \
		.gnd_gpio = GPIO_DT_SPEC_INST_GET(n, gnd_gpios),				 \
		.r_ref = DT_INST_PROP(n, r_ref),						 \
		.b_const = DT_INST_PROP(n, b_const),						 \
		.r_infinity = (double)DT_INST_PROP(n, r_infinity) / (double)1000000,		 \
		.wiring = DT_INST_PROP(n, wiring),						 \
	};											 \
												 \
	SENSOR_DEVICE_DT_INST_DEFINE(n, thermistor_init, NULL,					 \
				     &thermistor_dev_data_##inst, &thermistor_dev_config_##inst, \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY,			 \
				     &thermistor_driver_api);					 \

DT_INST_FOREACH_STATUS_OKAY(THERMISTOR_DEFINE)
