/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>

/* The devicetree node identifier for the adc aliases. */
#define CC1_V_MEAS_NODE DT_ALIAS(vcc1)
#define CC2_V_MEAS_NODE DT_ALIAS(vcc2)
#define VBUS_V_MEAS_NODE DT_ALIAS(vbus)
#define VBUS_C_MEAS_NODE DT_ALIAS(cbus)
#define VCON_C_MEAS_NODE DT_ALIAS(ccon)

static const struct adc_dt_spec adc_cc1_v = ADC_DT_SPEC_GET(CC1_V_MEAS_NODE);
static const struct adc_dt_spec adc_cc2_v = ADC_DT_SPEC_GET(CC2_V_MEAS_NODE);
static const struct adc_dt_spec adc_vbus_v = ADC_DT_SPEC_GET(VBUS_V_MEAS_NODE);
static const struct adc_dt_spec adc_vbus_c = ADC_DT_SPEC_GET(VBUS_C_MEAS_NODE);
static const struct adc_dt_spec adc_vcon_c = ADC_DT_SPEC_GET(VCON_C_MEAS_NODE);

/* Standard Settings for ADCs the STM32 ADC */
#define ADC_RESOLUTION          12
#define ADC_GAIN                ADC_GAIN_1
#define ADC_REFERENCE           ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME    ADC_ACQ_TIME_DEFAULT
#define ADC_REF_MV              3300

/*shunt resistor and voltage amplifier for current sensors on Twinkie*/
#define VBUS_C_R_MOHM 3
#define VBUS_C_GAIN 100
#define VCON_C_R_MOHM 10
#define VCON_C_GAIN 25

int meas_vbus_v(int32_t *v)
{
	int ret;
	int32_t sample_buffer = 0;

	/* Structure defining an ADC sampling sequence */
	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(sample_buffer),
		.resolution = ADC_RESOLUTION,
		.channels = BIT(adc_vbus_v.channel_id),
	};

	ret = adc_read(adc_vbus_v.dev, &sequence);
	if (ret != 0) {
		return ret;
	}

	*v = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, v);
	if (ret != 0) {
		return ret;
	}

	/* voltage scaled by voltage divider values using DT binding */
	*v = *v * DT_PROP(VBUS_V_MEAS_NODE, full_ohms) / DT_PROP(VBUS_V_MEAS_NODE, output_ohms);

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
		.resolution = ADC_RESOLUTION,
		.channels = BIT(adc_vbus_c.channel_id),
	};

	ret = adc_read(adc_vbus_c.dev, &sequence);
	if (ret != 0) {
		return ret;
	}

	*c = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, c);
	if (ret != 0) {
		return ret;
	}

	/* multiplies by 1000 before dividing by shunt resistance
	 * in milliohms to keep everything as an integer.
	 * mathematically equivalent to dividing by ohms directly.
	 */
	*c = (*c - ADC_REF_MV / 2) * 1000 / VBUS_C_R_MOHM / VBUS_C_GAIN;

	return 0;
}

int meas_cc1_v(int32_t *v)
{
	int ret;
	int32_t sample_buffer = 0;

	/* Structure defining an ADC sampling sequence */
	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(sample_buffer),
		.resolution = ADC_RESOLUTION,
		.channels = BIT(adc_cc1_v.channel_id),
	};

	ret = adc_read(adc_cc1_v.dev, &sequence);
	if (ret != 0) {
		return ret;
	}

	/* cc pin measurements are one to one with actual voltage
	 * and does not need to be scaled
	 */
	*v = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, v);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int meas_cc2_v(int32_t *v)
{
	int ret;
	int32_t sample_buffer = 0;

	/* Structure defining an ADC sampling sequence */
	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(sample_buffer),
		.resolution = ADC_RESOLUTION,
		.channels = BIT(adc_cc2_v.channel_id),
	};

	ret = adc_read(adc_cc2_v.dev, &sequence);
	if (ret != 0) {
		return ret;
	}

	/* cc pin measurements are one to one with actual voltage
	 * and does not need to be scaled
	 */
	*v = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, v);
	if (ret != 0) {
		return ret;
	}

	return 0;
}

int meas_vcon_c(int32_t *c)
{
	int ret;
	int32_t sample_buffer = 0;

	/* Structure defining an ADC sampling sequence */
	struct adc_sequence sequence = {
		.buffer = &sample_buffer,
		/* buffer size in bytes, not number of samples */
		.buffer_size = sizeof(sample_buffer),
		.resolution = ADC_RESOLUTION,
		.channels = BIT(adc_vcon_c.channel_id),
	};

	ret = adc_read(adc_vcon_c.dev, &sequence);
	if (ret != 0) {
		return ret;
	}

	*c = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, c);
	if (ret != 0) {
		return ret;
	}

	/* multiplies by 1000 before dividing by shunt resistance
	 * in milliohms to keep everything as an integer
	 * mathematically equivalent to dividing by ohms directly.
	 */
	*c = *c * 1000 / VCON_C_R_MOHM / VCON_C_GAIN;

	return 0;
}

int meas_init(void)
{
	int ret;

	ret = adc_channel_setup_dt(&adc_cc1_v);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_cc2_v);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_vbus_v);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_vbus_c);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_vcon_c);
	if (ret != 0) {
		return ret;
	}

	return 0;

}
