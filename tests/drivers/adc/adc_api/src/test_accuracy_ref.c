/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/ztest.h>

/* The board's known voltage and the channel it is wired to. */
#define REF_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(test_adc_reference_voltage)
#define REF_V DT_PROP(REF_NODE, reference_mv)
#define EXP_ACC DT_PROP(REF_NODE, expected_accuracy)

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(REF_NODE);

static int test_ref_to_adc(void)
{
	int ret;
	int32_t sample_buffer = 0;

	struct adc_sequence sequence = {
		.buffer      = &sample_buffer,
		.buffer_size = sizeof(sample_buffer),
		.calibrate = true,
	};

	adc_sequence_init_dt(&adc_channel, &sequence);

	ret = adc_read_dt(&adc_channel, &sequence);
	zassert_equal(ret, 0, "adc_read_dt() failed with code %d", ret);

	ret = adc_raw_to_millivolts_dt(&adc_channel, &sample_buffer);
	zassert_equal(ret, 0, "adc_raw_to_millivolts_dt() failed with code %d",
		      ret);
	zassert_within(sample_buffer, REF_V, EXP_ACC,
		"Value %d mV read from ADC does not match expected range (%d mV).",
		sample_buffer, REF_V);

	return TC_PASS;
}

ZTEST(adc_accuracy_ref, test_ref_to_adc)
{
	int i;

	for (i = 0; i < CONFIG_TEST_ADC_ACCURACY_PASSES; i++) {
		zassert_true(test_ref_to_adc() == TC_PASS);
	}
}

static void *adc_setup(void)
{
	int ret;

	zassert_true(adc_is_ready_dt(&adc_channel), "ADC device is not ready");
	ret = adc_channel_setup_dt(&adc_channel);
	zassert_equal(ret, 0,
		"Setting up of the ADC channel failed with code %d", ret);

	return NULL;
}

ZTEST_SUITE(adc_accuracy_ref, NULL, adc_setup, NULL, NULL, NULL);
