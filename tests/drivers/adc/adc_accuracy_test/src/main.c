/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/ztest.h>

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DT_PATH(zephyr_user));
#else
#error "Unsupported board."
#endif

const struct adc_dt_spec *get_adc_channel(void)
{
	return &adc_channel;
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

ZTEST_SUITE(adc_accuracy_test, NULL, adc_setup, NULL, NULL, NULL);
