/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * @addtogroup test_adc_basic_operations
 * @{
 * @defgroup t_adc_basic_basic_operations test_adc_sample
 * @brief TestPurpose: verify ADC driver handles different sampling scenarios
 * @}
 */

#include <adc.h>
#include <zephyr.h>
#include <ztest.h>

#if defined(CONFIG_BOARD_NRF52840_PCA10056)
#define TEST_ADC_DEV_NAME	CONFIG_ADC_0_NAME
#define TEST_ADC_CH_0_INPUT	1
#define TEST_ADC_CH_2_INPUT	2
#define TEST_ADC_RESOLUTION	10
#define TEST_ADC_ACQ_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define TEST_ADC_GAIN		ADC_GAIN_1_6
#define TEST_ADC_REFERENCE	ADC_REF_INTERNAL
#else
#error "Unsupported board."
#endif

#define BUFFER_SIZE	6

static s16_t m_sample_buffer[BUFFER_SIZE];

static const struct adc_channel_cfg m_channel_0_cfg = {
	.gain             = TEST_ADC_GAIN,
	.reference        = TEST_ADC_REFERENCE,
	.acquisition_time = TEST_ADC_ACQ_TIME,
	.channel_id       = 0,
	.differential     = false,
	.input_positive   = TEST_ADC_CH_0_INPUT,
};
static const struct adc_channel_cfg m_channel_2_cfg = {
	.gain             = TEST_ADC_GAIN,
	.reference        = TEST_ADC_REFERENCE,
	.acquisition_time = TEST_ADC_ACQ_TIME,
	.channel_id       = 2,
	.differential     = false,
	.input_positive   = TEST_ADC_CH_2_INPUT,
};

static void show_samples(void)
{
	int i;

	TC_PRINT("Samples read: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		TC_PRINT("0x%04x ", m_sample_buffer[i]);
	}
	TC_PRINT("\n");
}

/*******************************************************************************
 * test_adc_sample_one_channel
 */
static int test_task_one_channel(void)
{
	int ret;
	const struct adc_sequence sequence = {
		.options      = NULL,
		.channels     = (1U << 0),
		.buffer       = m_sample_buffer,
		.buffer_size  = sizeof(m_sample_buffer),
		.resolution   = TEST_ADC_RESOLUTION,
		.oversampling = 0,
	};

	struct device *adc_dev = device_get_binding(TEST_ADC_DEV_NAME);

	if (!adc_dev) {
		TC_PRINT("Cannot get ADC device\n");
		return TC_FAIL;
	}

	ret = adc_channel_setup(adc_dev, &m_channel_0_cfg);
	if (ret != 0) {
		TC_PRINT("Setting up of channel 0 failed with code %d\n", ret);
		return TC_FAIL;
	}

	memset(m_sample_buffer, 0, sizeof(m_sample_buffer));

	ret = adc_read(adc_dev, &sequence);
	if (ret != 0) {
		TC_PRINT("adc_read() failed with code %d\n", ret);
		return TC_FAIL;
	}

	show_samples();

	return TC_PASS;
}
void test_adc_sample_one_channel(void)
{
	zassert_true(test_task_one_channel() == TC_PASS, NULL);
}

/*******************************************************************************
 * test_adc_sample_two_channels
 */
static int test_task_two_channels(void)
{
	int ret;
	const struct adc_sequence sequence = {
		.options      = NULL,
		.channels     = (1U << 0) | (1U << 2),
		.buffer       = m_sample_buffer,
		.buffer_size  = sizeof(m_sample_buffer),
		.resolution   = TEST_ADC_RESOLUTION,
		.oversampling = 0,
	};

	struct device *adc_dev = device_get_binding(TEST_ADC_DEV_NAME);

	if (!adc_dev) {
		TC_PRINT("Cannot get ADC device\n");
		return TC_FAIL;
	}

	ret = adc_channel_setup(adc_dev, &m_channel_0_cfg);
	if (ret != 0) {
		TC_PRINT("Setting up of channel 0 failed with code %d\n", ret);
		return TC_FAIL;
	}

	ret = adc_channel_setup(adc_dev, &m_channel_2_cfg);
	if (ret != 0) {
		TC_PRINT("Setting up of channel 2 failed with code %d\n", ret);
		return TC_FAIL;
	}

	memset(m_sample_buffer, 0, sizeof(m_sample_buffer));

	ret = adc_read(adc_dev, &sequence);
	if (ret != 0) {
		TC_PRINT("adc_read() failed with code %d\n", ret);
		return TC_FAIL;
	}

	show_samples();

	return TC_PASS;
}
void test_adc_sample_two_channels(void)
{
	zassert_true(test_task_two_channels() == TC_PASS, NULL);
}


/*******************************************************************************
 * test_adc_sample_with_delay_trigger
 */
static enum adc_action delay_trigger_callback(
				struct device *dev,
				const struct adc_sequence *sequence,
				u16_t sampling_index)
{
	TC_PRINT("delay_trigger_callback: sampling %d\n", sampling_index);
	return ADC_ACTION_CONTINUE;
}
static int test_task_delay_trigger(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.trigger_type        = ADC_TRIGGER_DELAY_MS,
		.trigger_param.value = 100,
		.callback            = delay_trigger_callback,
		.extra_samplings     = 4,
	};
	const struct adc_sequence sequence = {
		.options      = &options,
		.channels     = (1U << 0),
		.buffer       = m_sample_buffer,
		.buffer_size  = sizeof(m_sample_buffer),
		.resolution   = TEST_ADC_RESOLUTION,
		.oversampling = 0,
	};

	struct device *adc_dev = device_get_binding(TEST_ADC_DEV_NAME);

	if (!adc_dev) {
		TC_PRINT("Cannot get ADC device\n");
		return TC_FAIL;
	}

	ret = adc_channel_setup(adc_dev, &m_channel_0_cfg);
	if (ret != 0) {
		TC_PRINT("Setting up of channel 0 failed with code %d\n", ret);
		return TC_FAIL;
	}

	memset(m_sample_buffer, 0, sizeof(m_sample_buffer));

	ret = adc_read(adc_dev, &sequence);
	if (ret != 0) {
		TC_PRINT("adc_read() failed with code %d\n", ret);
		return TC_FAIL;
	}

	show_samples();

	return TC_PASS;
}
void test_adc_sample_with_delay_trigger(void)
{
	zassert_true(test_task_delay_trigger() == TC_PASS, NULL);
}


/*******************************************************************************
 * test_adc_repeated_samplings
 */
static u8_t m_samplings_done = 0;
static enum adc_action repeated_samplings_callback(
				struct device *dev,
				const struct adc_sequence *sequence,
				u16_t sampling_index)
{
	++m_samplings_done;
	TC_PRINT("repeated_samplings_callback: done %d\n", m_samplings_done);
	show_samples();
	return (m_samplings_done < 10) ? ADC_ACTION_REPEAT : ADC_ACTION_FINISH;
}
static int test_task_repeated_samplings(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.trigger_type        = ADC_TRIGGER_DELAY_MS,
		.trigger_param.value = 100,
		.callback            = repeated_samplings_callback,
		.extra_samplings     = 0,
	};
	const struct adc_sequence sequence = {
		.options      = &options,
		.channels     = (1U << 0) | (1U << 2),
		.buffer       = m_sample_buffer,
		.buffer_size  = sizeof(m_sample_buffer),
		.resolution   = TEST_ADC_RESOLUTION,
		.oversampling = 0,
	};

	struct device *adc_dev = device_get_binding(TEST_ADC_DEV_NAME);

	if (!adc_dev) {
		TC_PRINT("Cannot get ADC device\n");
		return TC_FAIL;
	}

	ret = adc_channel_setup(adc_dev, &m_channel_0_cfg);
	if (ret != 0) {
		TC_PRINT("Setting up of channel 0 failed with code %d\n", ret);
		return TC_FAIL;
	}

	ret = adc_channel_setup(adc_dev, &m_channel_2_cfg);
	if (ret != 0) {
		TC_PRINT("Setting up of channel 2 failed with code %d\n", ret);
		return TC_FAIL;
	}

	memset(m_sample_buffer, 0, sizeof(m_sample_buffer));

	ret = adc_read(adc_dev, &sequence);
	if (ret != 0) {
		TC_PRINT("adc_read() failed with code %d\n", ret);
		return TC_FAIL;
	}

	return TC_PASS;
}
void test_adc_repeated_samplings(void)
{
	zassert_true(test_task_repeated_samplings() == TC_PASS, NULL);
}
