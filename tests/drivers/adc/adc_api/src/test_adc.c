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

#if defined(CONFIG_BOARD_NRF51_PCA10028)

#include <hal/nrf_adc.h>
#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1_3
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0
#define ADC_1ST_CHANNEL_INPUT	NRF_ADC_CONFIG_INPUT_2
#define ADC_2ND_CHANNEL_ID	2
#define ADC_2ND_CHANNEL_INPUT	NRF_ADC_CONFIG_INPUT_3

#elif defined(CONFIG_BOARD_NRF52_PCA10040) || \
      defined(CONFIG_BOARD_NRF52840_PCA10056)

#include <hal/nrf_saadc.h>
#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1_6
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME(ADC_ACQ_TIME_MICROSECONDS, 10)
#define ADC_1ST_CHANNEL_ID	0
#define ADC_1ST_CHANNEL_INPUT	NRF_SAADC_INPUT_AIN1
#define ADC_2ND_CHANNEL_ID	2
#define ADC_2ND_CHANNEL_INPUT	NRF_SAADC_INPUT_AIN2

#elif defined(CONFIG_BOARD_FRDM_K64F)

#define ADC_DEVICE_NAME		CONFIG_ADC_1_NAME
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	14
#define ADC_1ST_CHANNEL_INPUT	0

#elif defined(CONFIG_BOARD_FRDM_KL25Z)

#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	12
#define ADC_1ST_CHANNEL_INPUT	0

#elif defined(CONFIG_BOARD_FRDM_KW41Z)

#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	3
#define ADC_1ST_CHANNEL_INPUT	0

#elif defined(CONFIG_BOARD_HEXIWEAR_K64)

#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	16
#define ADC_1ST_CHANNEL_INPUT	0

#elif defined(CONFIG_BOARD_HEXIWEAR_KW40Z)

#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	1
#define ADC_1ST_CHANNEL_INPUT	0

#elif defined(CONFIG_BOARD_SAM_E70_XPLAINED)

#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		12
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_EXTERNAL0
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	0
#define ADC_1ST_CHANNEL_INPUT	0

#elif defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD_SS) || \
	defined(CONFIG_BOARD_ARDUINO_101_SSS)
#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	10
#define ADC_2ND_CHANNEL_ID	11

#elif defined(CONFIG_BOARD_QUARK_D2000_CRB)
#define ADC_DEVICE_NAME		CONFIG_ADC_0_NAME
#define ADC_RESOLUTION		10
#define ADC_GAIN		ADC_GAIN_1
#define ADC_REFERENCE		ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME	ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID	3
#define ADC_2ND_CHANNEL_ID	4

#else
#error "Unsupported board."
#endif

#define BUFFER_SIZE  6
static s16_t m_sample_buffer[BUFFER_SIZE];

static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive   = ADC_1ST_CHANNEL_INPUT,
#endif
};
#if defined(ADC_2ND_CHANNEL_ID)
static const struct adc_channel_cfg m_2nd_channel_cfg = {
	.gain             = ADC_GAIN,
	.reference        = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id       = ADC_2ND_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive   = ADC_2ND_CHANNEL_INPUT,
#endif
};
#endif /* defined(ADC_2ND_CHANNEL_ID) */

static struct device *init_adc(void)
{
	int ret;
	struct device *adc_dev = device_get_binding(ADC_DEVICE_NAME);

	zassert_not_null(adc_dev, "Cannot get ADC device");

	ret = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
	zassert_equal(ret, 0,
		"Setting up of the first channel failed with code %d", ret);

#if defined(ADC_2ND_CHANNEL_ID)
	ret = adc_channel_setup(adc_dev, &m_2nd_channel_cfg);
	zassert_equal(ret, 0,
		"Setting up of the second channel failed with code %d", ret);
#endif /* defined(ADC_2ND_CHANNEL_ID) */

	(void)memset(m_sample_buffer, 0, sizeof(m_sample_buffer));

	return adc_dev;
}

static void check_samples(int expected_count)
{
	int i;

	TC_PRINT("Samples read: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		s16_t sample_value = m_sample_buffer[i];

		TC_PRINT("0x%04x ", sample_value);
		if (i < expected_count) {
			zassert_not_equal(0, sample_value,
				"[%u] should be non-zero", i);
		} else {
			zassert_equal(0, sample_value,
				"[%u] should be zero", i);
		}
	}
	TC_PRINT("\n");
}

/*******************************************************************************
 * test_adc_sample_one_channel
 */
static int test_task_one_channel(void)
{
	int ret;

#if defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD_SS) || \
	defined(CONFIG_BOARD_ARDUINO_101_SSS)
	const struct adc_sequence_options options = {
		.interval_us     = 10,
		.extra_samplings = 0,
	};
#endif
	const struct adc_sequence sequence = {
#if defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD_SS) || \
	defined(CONFIG_BOARD_ARDUINO_101_SSS)
		.options     = &options,
#endif
		.channels    = BIT(ADC_1ST_CHANNEL_ID),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1);

	return TC_PASS;
}
void test_adc_sample_one_channel(void)
{
	zassert_true(test_task_one_channel() == TC_PASS, NULL);
}

/*******************************************************************************
 * test_adc_sample_two_channels
 */
#if defined(ADC_2ND_CHANNEL_ID)
static int test_task_two_channels(void)
{
	int ret;

#if defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD_SS) || \
	defined(CONFIG_BOARD_ARDUINO_101_SSS)
	const struct adc_sequence_options options = {
		.interval_us     = 30,
		.extra_samplings = 0,
	};
#endif
	const struct adc_sequence sequence = {
#if defined(CONFIG_BOARD_QUARK_SE_C1000_DEVBOARD_SS) || \
	defined(CONFIG_BOARD_ARDUINO_101_SSS)
		.options     = &options,
#endif
		.channels    = BIT(ADC_1ST_CHANNEL_ID) |
			       BIT(ADC_2ND_CHANNEL_ID),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(2);

	return TC_PASS;
}
#endif /* defined(ADC_2ND_CHANNEL_ID) */

void test_adc_sample_two_channels(void)
{
#if defined(ADC_2ND_CHANNEL_ID)
	zassert_true(test_task_two_channels() == TC_PASS, NULL);
#else
	ztest_test_skip();
#endif /* defined(ADC_2ND_CHANNEL_ID) */
}


/*******************************************************************************
 * test_adc_asynchronous_call
 */
#if defined(CONFIG_ADC_ASYNC)
static int test_task_asynchronous_call(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.extra_samplings = 4,
		.interval_us = 10,
	};
	const struct adc_sequence sequence = {
		.options     = &options,
		.channels    = BIT(ADC_1ST_CHANNEL_ID),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};
	struct k_poll_signal async_sig = K_POLL_SIGNAL_INITIALIZER(async_sig);
	struct k_poll_event  async_evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &async_sig);

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read_async(adc_dev, &sequence, &async_sig);
	zassert_equal(ret, 0, "adc_read_async() failed with code %d", ret);

	ret = k_poll(&async_evt, 1, K_MSEC(1000));
	zassert_equal(ret, 0, "async signal not received as expected");

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}
#endif /* defined(CONFIG_ADC_ASYNC) */
void test_adc_asynchronous_call(void)
{
#if defined(CONFIG_ADC_ASYNC)
	zassert_true(test_task_asynchronous_call() == TC_PASS, NULL);
#else
	ztest_test_skip();
#endif /* defined(CONFIG_ADC_ASYNC) */
}


/*******************************************************************************
 * test_adc_sample_with_interval
 */
static enum adc_action sample_with_interval_callback(
				struct device *dev,
				const struct adc_sequence *sequence,
				u16_t sampling_index)
{
	TC_PRINT("%s: sampling %d\n", __func__, sampling_index);
	return ADC_ACTION_CONTINUE;
}
static int test_task_with_interval(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.interval_us     = 100 * 1000UL,
		.callback        = sample_with_interval_callback,
		.extra_samplings = 4,
	};
	const struct adc_sequence sequence = {
		.options     = &options,
		.channels    = BIT(ADC_1ST_CHANNEL_ID),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}
void test_adc_sample_with_interval(void)
{
	zassert_true(test_task_with_interval() == TC_PASS, NULL);
}


/*******************************************************************************
 * test_adc_repeated_samplings
 */
static u8_t m_samplings_done;
static enum adc_action repeated_samplings_callback(
				struct device *dev,
				const struct adc_sequence *sequence,
				u16_t sampling_index)
{
	++m_samplings_done;
	TC_PRINT("%s: done %d\n", __func__, m_samplings_done);
	if (m_samplings_done == 1) {
		#if defined(ADC_2ND_CHANNEL_ID)
			check_samples(2);
		#else
			check_samples(1);
		#endif /* defined(ADC_2ND_CHANNEL_ID) */

		/* After first sampling continue normally. */
		return ADC_ACTION_CONTINUE;
	} else {
		#if defined(ADC_2ND_CHANNEL_ID)
			check_samples(4);
		#else
			check_samples(2);
		#endif /* defined(ADC_2ND_CHANNEL_ID) */

		/*
		 * The second sampling is repeated 9 times (the samples are
		 * written in the same place), then the sequence is finished
		 * prematurely.
		 */
		if (m_samplings_done < 10) {
			return ADC_ACTION_REPEAT;
		} else {
			return ADC_ACTION_FINISH;
		}
	}
}
static int test_task_repeated_samplings(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.callback        = repeated_samplings_callback,
		/*
		 * This specifies that 3 samplings are planned. However,
		 * the callback function above is constructed in such way
		 * that the first sampling is done normally, the second one
		 * is repeated 9 times, and then the sequence is finished.
		 * Hence, the third sampling will not take place.
		 */
		.extra_samplings = 2,
		.interval_us = 15,
	};
	const struct adc_sequence sequence = {
		.options     = &options,
#if defined(ADC_2ND_CHANNEL_ID)
		.channels    = BIT(ADC_1ST_CHANNEL_ID) |
			       BIT(ADC_2ND_CHANNEL_ID),
#else
		.channels    = BIT(ADC_1ST_CHANNEL_ID),
#endif /* defined(ADC_2ND_CHANNEL_ID) */
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = ADC_RESOLUTION,
	};

	struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	return TC_PASS;
}
void test_adc_repeated_samplings(void)
{
	zassert_true(test_task_repeated_samplings() == TC_PASS, NULL);
}
