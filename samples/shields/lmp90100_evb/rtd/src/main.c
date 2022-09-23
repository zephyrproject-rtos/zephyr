/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <stdio.h>
#include <math.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main);

/* Nominal RTD (PT100) resistance in ohms */
#define RTD_NOMINAL_RESISTANCE 100

/* ADC resolution in bits */
#define ADC_RESOLUTION 24U

/* ADC maximum value (taking sign bit into consideration) */
#define ADC_MAX BIT_MASK(ADC_RESOLUTION - 1)

/* Bottom resistor value in ohms */
#define BOTTOM_RESISTANCE 2000

static double rtd_temperature(int nom, double resistance)
{
	const double a0 =  3.90802E-3;
	const double b0 = -0.58020E-6;
	double temp;

	temp = -nom * a0;
	temp += sqrt((nom * nom) * (a0 * a0) - 4.0 * nom * b0 *
		     (nom - resistance));
	temp /= 2.0 * nom * b0;

	return temp;
}

void main(void)
{
	const struct device *const lmp90100 = DEVICE_DT_GET_ONE(ti_lmp90100);
	double resistance;
	int32_t buffer;
	int err;
	const struct adc_channel_cfg ch_cfg = {
		.channel_id = 0,
		.differential = 1,
		.input_positive = 0,
		.input_negative = 1,
		.reference = ADC_REF_EXTERNAL1,
		.gain = ADC_GAIN_1,
		.acquisition_time = ADC_ACQ_TIME(ADC_ACQ_TIME_TICKS, 0)
	};
	const struct adc_sequence seq = {
		.options = NULL,
		.channels = BIT(0),
		.buffer = &buffer,
		.buffer_size = sizeof(buffer),
		.resolution = ADC_RESOLUTION,
		.oversampling = 0,
		.calibrate = 0
	};

	if (!device_is_ready(lmp90100)) {
		LOG_ERR("LMP90100 device not ready");
		return;
	}

	err = adc_channel_setup(lmp90100, &ch_cfg);
	if (err) {
		LOG_ERR("failed to setup ADC channel (err %d)", err);
		return;
	}

	while (true) {
		err = adc_read(lmp90100, &seq);
		if (err) {
			LOG_ERR("failed to read ADC (err %d)", err);
		} else {
			resistance = (buffer / (double)ADC_MAX) * BOTTOM_RESISTANCE;
			printf("R: %.02f ohm\n", resistance);
			printf("T: %.02f degC\n",
				rtd_temperature(RTD_NOMINAL_RESISTANCE,
						resistance));
		}

		k_sleep(K_MSEC(1000));
	}
}
