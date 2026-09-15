/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/ztest.h>

#define DIV 2

/* The board's DAC output and the ADC channel it is wired to. */
#define DAC_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(test_adc_dac_loopback)
#define DAC_DEVICE_NODE	DT_PROP(DAC_NODE, dac)
#define DAC_CHANNEL_ID DT_PROP(DAC_NODE, dac_channel_id)

static const struct adc_dt_spec adc_channel = ADC_DT_SPEC_GET(DAC_NODE);

static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id = DAC_CHANNEL_ID,
	.resolution = DT_PROP(DAC_NODE, dac_resolution),
	.buffered = !IS_ENABLED(CONFIG_TEST_DAC_BUFFER_NOT_SUPPORTED),
};

static const struct device *init_dac(void)
{
	int ret;
	const struct device *const dac_dev = DEVICE_DT_GET(DAC_DEVICE_NODE);

	zassert_true(device_is_ready(dac_dev), "DAC device is not ready");

	ret = dac_channel_setup(dac_dev, &dac_ch_cfg);
	zassert_equal(ret, 0,
		      "Setting up of the first channel failed with code %d", ret);

	return dac_dev;
}

static int test_dac_to_adc(void)
{
	int ret, write_val;
	int32_t sample_buffer = 0;

	struct adc_sequence sequence = {
		.buffer      = &sample_buffer,
		.buffer_size = sizeof(sample_buffer),
#if CONFIG_TEST_ADC_CALIBRATE_REQUIRED
		.calibrate = true,
#endif
	};

	const struct device *dac_dev = init_dac();

	write_val = (1U << dac_ch_cfg.resolution) / DIV;

	ret = dac_write_value(dac_dev, DAC_CHANNEL_ID, write_val);

	zassert_equal(ret, 0, "dac_write_value() failed with code %d", ret);

	k_sleep(K_MSEC(10));

	adc_sequence_init_dt(&adc_channel, &sequence);
	ret = adc_read_dt(&adc_channel, &sequence);

	zassert_equal(ret, 0, "adc_read_dt() failed with code %d", ret);
	zassert_within(sample_buffer,
			(1U << adc_channel.resolution) / DIV, 32,
			"Value %d read from ADC does not match expected range.",
			sample_buffer);

	return TC_PASS;
}

ZTEST(adc_accuracy_dac, test_dac_to_adc)
{
	int i;

	for (i = 0; i < CONFIG_TEST_ADC_ACCURACY_PASSES; i++) {
		zassert_true(test_dac_to_adc() == TC_PASS);
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

ZTEST_SUITE(adc_accuracy_dac, NULL, adc_setup, NULL, NULL, NULL);
