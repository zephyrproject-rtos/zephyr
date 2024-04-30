/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/ztest.h>

#define REF_V DT_PROP(DT_PATH(zephyr_user), reference_mv)

extern const struct adc_dt_spec *get_adc_channel(void);

static int test_ref_to_adc(void)
{
	int ret;
	int32_t sample_buffer = 0;

	struct adc_sequence sequence = {
		.buffer      = &sample_buffer,
		.buffer_size = sizeof(sample_buffer),
	};

	const struct adc_dt_spec *adc_channel = get_adc_channel();

	adc_sequence_init_dt(adc_channel, &sequence);

	ret = adc_read_dt(adc_channel, &sequence);
	zassert_equal(ret, 0, "adc_read_dt() failed with code %d", ret);

	ret = adc_raw_to_millivolts_dt(adc_channel, &sample_buffer);
	zassert_equal(ret, 0, "adc_raw_to_millivolts_dt() failed with code %d",
		      ret);
	zassert_within(sample_buffer, REF_V, 32,
		"Value %d mV read from ADC does not match expected range (%d mV).",
		sample_buffer, REF_V);

	return TC_PASS;
}

ZTEST(adc_accuracy_test, test_ref_to_adc)
{
	int i;

	for (i = 0; i < CONFIG_NUMBER_OF_PASSES; i++) {
		zassert_true(test_ref_to_adc() == TC_PASS);
	}
}
