/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <drivers/adc.h>
#include <stdio.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(main);

#define RTD_NOMINAL_RESISTANCE 100

static double sqrt(double value)
{
	double sqrt = value / 3;
	int i;

	if (value <= 0) {
		return 0;
	}

	for (i = 0; i < 6; i++) {
		sqrt = (sqrt + value / sqrt) / 2;
	}

	return sqrt;
}

static double rtd_temperature(int nom, double resistance)
{
	double a0 =  3.90802E-3;
	double b0 = -0.58020E-6;
	double temp;

	temp = -nom * a0;
	temp += sqrt((nom * nom) * (a0 * a0) - 4.0 * nom * b0 *
		     (nom - resistance));
	temp /= 2.0 * nom * b0;

	return temp;
}

void main(void)
{
	const struct device *lmp90100;
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
		.resolution = 24,
		.oversampling = 0,
		.calibrate = 0
	};

	lmp90100 = device_get_binding(DT_LABEL(DT_INST(0, ti_lmp90100)));
	if (!lmp90100) {
		LOG_ERR("LMP90100 device not found");
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
			resistance = (buffer / 8388608.0) * 2000;
			printf("R: %.02f ohm\n", resistance);
			printf("T: %.02f degC\n",
				rtd_temperature(RTD_NOMINAL_RESISTANCE,
						resistance));
		}

		k_sleep(K_MSEC(1000));
	}
}
