/*
 * Copyright (c) 2023 The ChromiumOS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/adc/voltage_divider.h>
#include <zephyr/drivers/adc/current_sense_amplifier.h>

/* The devicetree node identifier for the adc aliases. */
#define VBUS_V_MEAS_NODE DT_ALIAS(vbus)
#define VBUS_C_MEAS_NODE DT_ALIAS(cbus)

static const struct voltage_divider_dt_spec adc_vbus_v =
	VOLTAGE_DIVIDER_DT_SPEC_GET(VBUS_V_MEAS_NODE);
static const struct current_sense_amplifier_dt_spec adc_vbus_c =
	CURRENT_SENSE_AMPLIFIER_DT_SPEC_GET(VBUS_C_MEAS_NODE);

int meas_vbus_v(int32_t *v)
{
	int ret;
	int32_t sample_buffer = 0;

	/* Structure defining an ADC sampling sequence */
	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(sample_buffer),
		.calibrate = true,
	};
	adc_sequence_init_dt(&adc_vbus_v.port, &sequence);

	ret = adc_read_dt(&adc_vbus_v.port, &sequence);
	if (ret != 0) {
		return ret;
	}

	*v = sample_buffer;
	ret = adc_raw_to_millivolts_dt(&adc_vbus_v.port, v);
	if (ret != 0) {
		return ret;
	}

	ret = voltage_divider_scale_dt(&adc_vbus_v, v);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int meas_vbus_c(int32_t *c)
{
	int ret;
	int32_t sample_buffer = 0;

	/* Structure defining an ADC sampling sequence */
	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(sample_buffer),
		.calibrate = true,
	};
	adc_sequence_init_dt(&adc_vbus_c.port, &sequence);

	ret = adc_read(adc_vbus_c.port.dev, &sequence);
	if (ret != 0) {
		return ret;
	}

	*c = sample_buffer;
	ret = adc_raw_to_millivolts_dt(&adc_vbus_c.port, c);
	if (ret != 0) {
		return ret;
	}

	/* prescaling the voltage offset */
	*c -= adc_vbus_c.port.vref_mv / 2;
	current_sense_amplifier_scale_dt(&adc_vbus_c, c);

	return 0;
}

int meas_init(void)
{
	int ret;

	ret = adc_channel_setup_dt(&adc_vbus_v.port);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_vbus_c.port);
	if (ret != 0) {
		return ret;
	}

	return 0;
}
