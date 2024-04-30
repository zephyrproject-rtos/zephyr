/*
 * Copyright (c) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/ztest.h>

#define DIV 2

#define DAC_DEVICE_NODE	DT_PROP(DT_PATH(zephyr_user), dac)

extern const struct adc_dt_spec *get_adc_channel(void);

static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id = DT_PROP(DT_PATH(zephyr_user), dac_channel_id),
	.resolution = DT_PROP(DT_PATH(zephyr_user), dac_resolution),
	.buffered = true
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
	};

	const struct device *dac_dev = init_dac();
	const struct adc_dt_spec *adc_channel = get_adc_channel();

	write_val = (1U << dac_ch_cfg.resolution) / DIV;

	ret = dac_write_value(dac_dev, DT_PROP(DT_PATH(zephyr_user), dac_channel_id), write_val);

	zassert_equal(ret, 0, "dac_write_value() failed with code %d", ret);

	k_sleep(K_MSEC(10));

	adc_sequence_init_dt(adc_channel, &sequence);
	ret = adc_read_dt(adc_channel, &sequence);

	zassert_equal(ret, 0, "adc_read_dt() failed with code %d", ret);
	zassert_within(sample_buffer,
			(1U << adc_channel->resolution) / DIV, 32,
			"Value %d read from ADC does not match expected range.",
			sample_buffer);

	return TC_PASS;
}

ZTEST(adc_accuracy_test, test_dac_to_adc)
{
	int i;

	for (i = 0; i < CONFIG_NUMBER_OF_PASSES; i++) {
		zassert_true(test_dac_to_adc() == TC_PASS);
	}
}
