/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/drivers/adc.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

/* Invalid value that is not supposed to be written by the driver. It is used
 * to mark the sample buffer entries as empty. If needed, it can be overridden
 * for a particular board by providing a specific definition above.
 */
#if !defined(INVALID_ADC_VALUE)
#define INVALID_ADC_VALUE SHRT_MIN
#endif

#define BUFFER_SIZE  6
static ZTEST_BMEM int16_t m_sample_buffer[BUFFER_SIZE];

#define DT_SPEC_AND_COMMA(node_id, prop, idx) ADC_DT_SPEC_GET_BY_IDX(node_id, idx),

#if DT_NODE_HAS_PROP(DT_PATH(zephyr_user), io_channels)
/* Data of ADC io-channels specified in devicetree. */
static const struct adc_dt_spec adc_channels[] = {
	DT_FOREACH_PROP_ELEM(DT_PATH(zephyr_user), io_channels, DT_SPEC_AND_COMMA)
};
static const int adc_channels_count = ARRAY_SIZE(adc_channels);
#else
#error "Unsupported board."
#endif

const struct device *get_adc_device(void)
{
	if (!adc_is_ready_dt(&adc_channels[0])) {
		printk("ADC device is not ready\n");
		return NULL;
	}

	return adc_channels[0].dev;
}

static void init_adc(void)
{
	int i, ret;

	zassert_true(adc_is_ready_dt(&adc_channels[0]), "ADC device is not ready");

	for (i = 0; i < adc_channels_count; i++) {
		ret = adc_channel_setup_dt(&adc_channels[i]);
		zassert_equal(ret, 0, "Setting up of channel %d failed with code %d", i, ret);
	}

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
	}
}

static void check_samples(int expected_count)
{
	int i;

	TC_PRINT("Samples read: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		int16_t sample_value = m_sample_buffer[i];

		TC_PRINT("0x%04x ", sample_value);
		if (i < expected_count) {
			zassert_not_equal(INVALID_ADC_VALUE, sample_value,
				"[%u] should be filled", i);
		} else {
			zassert_equal(INVALID_ADC_VALUE, sample_value,
				"[%u] should be empty", i);
		}
	}
	TC_PRINT("\n");
}

/*
 * test_adc_sample_one_channel
 */
static int test_task_one_channel(void)
{
	int ret;
	struct adc_sequence sequence = {
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	init_adc();
	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	ret = adc_read_dt(&adc_channels[0], &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1);

	return TC_PASS;
}

ZTEST_USER(adc_basic, test_adc_sample_one_channel)
{
	zassert_true(test_task_one_channel() == TC_PASS);
}

/*
 * test_adc_sample_multiple_channels
 */
static int test_task_multiple_channels(void)
{
	int ret;
	struct adc_sequence sequence = {
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	init_adc();
	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	for (int i = 1; i < adc_channels_count; i++) {
		sequence.channels |= BIT(adc_channels[i].channel_id);
	}

	ret = adc_read_dt(&adc_channels[0], &sequence);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(adc_channels_count);

	return TC_PASS;
}

ZTEST_USER(adc_basic, test_adc_sample_two_channels)
{
	if (adc_channels_count > 1) {
		zassert_true(test_task_multiple_channels() == TC_PASS);
	} else {
		ztest_test_skip();
	}
}

/*
 * test_adc_asynchronous_call
 */
#if defined(CONFIG_ADC_ASYNC)
struct k_poll_signal async_sig;

static int test_task_asynchronous_call(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.extra_samplings = 4,
		/* Start consecutive samplings as fast as possible. */
		.interval_us     = 0,
	};
	struct adc_sequence sequence = {
		.options     = &options,
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};
	struct k_poll_event  async_evt =
		K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
					 K_POLL_MODE_NOTIFY_ONLY,
					 &async_sig);
	init_adc();

	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	ret = adc_read_async(adc_channels[0].dev, &sequence, &async_sig);
	zassert_equal(ret, 0, "adc_read_async() failed with code %d", ret);

	ret = k_poll(&async_evt, 1, K_MSEC(1000));
	zassert_equal(ret, 0, "k_poll failed with error %d", ret);

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}
#endif /* defined(CONFIG_ADC_ASYNC) */

ZTEST_USER(adc_basic, test_adc_asynchronous_call)
{
#if defined(CONFIG_ADC_ASYNC)
	zassert_true(test_task_asynchronous_call() == TC_PASS);
#else
	ztest_test_skip();
#endif /* defined(CONFIG_ADC_ASYNC) */
}

/*
 * test_adc_sample_with_interval
 */
static uint32_t my_sequence_identifier = 0x12345678;
static void *user_data = &my_sequence_identifier;

static enum adc_action sample_with_interval_callback(const struct device *dev,
						     const struct adc_sequence *sequence,
						     uint16_t sampling_index)
{
	if (sequence->options->user_data != &my_sequence_identifier) {
		user_data = sequence->options->user_data;
		return ADC_ACTION_FINISH;
	}

	TC_PRINT("%s: sampling %d\n", __func__, sampling_index);
	return ADC_ACTION_CONTINUE;
}

static int test_task_with_interval(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.interval_us     = 100 * 1000UL,
		.callback        = sample_with_interval_callback,
		.user_data       = user_data,
		.extra_samplings = 4,
	};
	struct adc_sequence sequence = {
		.options     = &options,
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	init_adc();

	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	ret = adc_read_dt(&adc_channels[0], &sequence);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	zassert_equal(user_data, sequence.options->user_data,
		"Invalid user data: %p, expected: %p",
		user_data, sequence.options->user_data);

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}

ZTEST(adc_basic, test_adc_sample_with_interval)
{
	zassert_true(test_task_with_interval() == TC_PASS);
}

/*
 * test_adc_repeated_samplings
 */
static uint8_t m_samplings_done;
static enum adc_action repeated_samplings_callback(const struct device *dev,
						   const struct adc_sequence *sequence,
						   uint16_t sampling_index)
{
	++m_samplings_done;
	TC_PRINT("%s: done %d\n", __func__, m_samplings_done);
	if (m_samplings_done == 1U) {
		check_samples(MIN(adc_channels_count, 2));

		/* After first sampling continue normally. */
		return ADC_ACTION_CONTINUE;
	} else {
		check_samples(2 * MIN(adc_channels_count, 2));

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
		/* Start consecutive samplings as fast as possible. */
		.interval_us     = 0,
	};
	struct adc_sequence sequence = {
		.options     = &options,
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
	};

	init_adc();
	(void)adc_sequence_init_dt(&adc_channels[0], &sequence);

	if (adc_channels_count > 1) {
		sequence.channels |=  BIT(adc_channels[1].channel_id);
	}

	ret = adc_read_dt(&adc_channels[0], &sequence);
	if (ret == -ENOTSUP) {
		ztest_test_skip();
	}
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	return TC_PASS;
}

ZTEST(adc_basic, test_adc_repeated_samplings)
{
	zassert_true(test_task_repeated_samplings() == TC_PASS);
}

/*
 * test_adc_invalid_request
 */
static int test_task_invalid_request(void)
{
	int ret;
	struct adc_sequence sequence = {
		.channels    = BIT(adc_channels[0].channel_id),
		.buffer      = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution  = 0, /* intentionally invalid value */
	};

	init_adc();

	ret = adc_read_dt(&adc_channels[0], &sequence);
	zassert_not_equal(ret, 0, "adc_read() unexpectedly succeeded");

#if defined(CONFIG_ADC_ASYNC)
	ret = adc_read_async(adc_channels[0].dev, &sequence, &async_sig);
	zassert_not_equal(ret, 0, "adc_read_async() unexpectedly succeeded");
#endif

	/*
	 * Make the sequence parameters valid, now the request should succeed.
	 */
	sequence.resolution = adc_channels[0].resolution;

	ret = adc_read_dt(&adc_channels[0], &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1);

	return TC_PASS;
}

ZTEST_USER(adc_basic, test_adc_invalid_request)
{
	zassert_true(test_task_invalid_request() == TC_PASS);
}
