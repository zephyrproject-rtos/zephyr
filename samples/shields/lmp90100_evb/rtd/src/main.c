/*
 * Copyright (c) 2019-2024 Vestas Wind Systems A/S
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

/* ADC maximum value (taking sign bit into consideration) */
#define ADC_MAX(resolution) BIT_MASK(resolution - 1)

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

int main(void)
{
	const struct adc_dt_spec ch_cfg = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
	double adc_max = ADC_MAX(ch_cfg.resolution);
	double resistance;
	int32_t buffer;
	int err;
	struct adc_sequence seq = {
		.buffer = &buffer,
		.buffer_size = sizeof(buffer),
	};

	if (!adc_is_ready_dt(&ch_cfg)) {
		LOG_ERR("LMP90100 device not ready");
		return 0;
	}

	err = adc_channel_setup_dt(&ch_cfg);
	if (err != 0) {
		LOG_ERR("failed to setup ADC channel (err %d)", err);
		return 0;
	}

	err = adc_sequence_init_dt(&ch_cfg, &seq);
	if (err != 0) {
		LOG_ERR("failed to initialize ADC sequence (err %d)", err);
		return 0;
	}

	while (true) {
		err = adc_read_dt(&ch_cfg, &seq);
		if (err != 0) {
			LOG_ERR("failed to read ADC (err %d)", err);
		} else {
			resistance = (buffer / adc_max) * BOTTOM_RESISTANCE;
			printf("R: %.02f ohm\n", resistance);
			printf("T: %.02f degC\n",
				rtd_temperature(RTD_NOMINAL_RESISTANCE,
						resistance));
		}

		k_sleep(K_MSEC(1000));
	}
	return 0;
}
