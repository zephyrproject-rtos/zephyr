/*
 * Copyright (c) 2020 Intive
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if CONFIG_BATT_MODE_DIRECT

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr.h>
#include <init.h>
#include <drivers/adc.h>
#include <drivers/sensor.h>
#ifdef CONFIG_ADC_NRFX_SAADC
#include <hal/nrf_saadc.h>
#endif
#include <logging/log.h>

#include "battery.h"

LOG_MODULE_REGISTER(BATTERY, CONFIG_ADC_LOG_LEVEL);

struct battery_data {
	struct device *adc;
	struct adc_channel_cfg adc_cfg;
	struct adc_sequence adc_seq;
	s32_t raw;
};
static struct battery_data battery_data;
static bool battery_ok;

static int adc_setup(void)
{
	battery_data.adc = device_get_binding("ADC_0");
	if (battery_data.adc == NULL) {
		LOG_ERR("Failed to get ADC_0");
		return -ENOENT;
	}

	battery_data.adc_seq =
		(struct adc_sequence){ .channels = BIT(0),
				       .buffer = &battery_data.raw,
				       .buffer_size = sizeof(battery_data.raw),
				       .oversampling = 8,
				       .calibrate = true,
				       .resolution = 12 };

#ifdef CONFIG_ADC_NRFX_SAADC
	battery_data.adc_cfg = (struct adc_channel_cfg){
		.channel_id = 0,
		.differential = 0,
		.input_positive =
			NRF_SAADC_INPUT_VDD, /* select VDD as ADC source */
		.input_negative = 0,
		.reference = ADC_REF_INTERNAL, /* vRef=0.6v */
		.gain = ADC_GAIN_1_6, /* set 1/6 because of vRef */
		.acquisition_time = ADC_ACQ_TIME_DEFAULT
	};
#else /* CONFIG_ADC_var */
#error Unsupported ADC
#endif /* CONFIG_ADC_var */

	int rc = adc_channel_setup(battery_data.adc, &battery_data.adc_cfg);
	LOG_INF("Setup ADC_0 got %d", rc);

	return rc;
}

static int battery_setup(struct device *arg)
{
	int rc = adc_setup();

	battery_ok = (rc == 0);
	LOG_INF("Battery setup: %d %d", rc, battery_ok);
	return rc;
}

SYS_INIT(battery_setup, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

int battery_measure_enable(bool enable)
{
	/* Nothing to do in this implementation */
	return battery_ok ? 0 : -ENOENT;
}

int battery_sample(void)
{
	int rc = -ENOENT;

	if (battery_ok) {
		rc = adc_read(battery_data.adc, &battery_data.adc_seq);
		battery_data.adc_seq.calibrate = false;
		if (rc == 0) {
			s32_t val = battery_data.raw;

			rc = adc_raw_to_millivolts(
				adc_ref_internal(battery_data.adc),
				battery_data.adc_cfg.gain,
				battery_data.adc_seq.resolution, &val);
			LOG_INF("raw %u ~ %u mV => %d mV\n", battery_data.raw,
				val, rc);
			rc = rc >= 0 ? val : rc;
		}
	}

	return rc;
}

#endif /* CONFIG_BATT_MODE_DIRECT */