/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020, NXP
 * Copyright (c) 2023, Nobleo Technology
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_BOARD_FRDM_K64F)

#define ADC_DEVICE_NODE DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION 12
#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID 26
#define COUNTER_NODE_NAME pit0
#define HW_TRIGGER_INTERVAL (2U)
#define SAMPLE_INTERVAL_US HW_TRIGGER_INTERVAL

#elif defined(CONFIG_BOARD_FRDM_K82F)

#define ADC_DEVICE_NODE DT_INST(0, nxp_kinetis_adc16)
#define ADC_RESOLUTION 12
#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID 26
#define COUNTER_NODE_NAME pit0
#define HW_TRIGGER_INTERVAL (2U)
#define SAMPLE_INTERVAL_US HW_TRIGGER_INTERVAL

#elif defined(CONFIG_BOARD_NUCLEO_H743ZI)

#define ADC_DEVICE_NODE DT_INST(0, st_stm32_adc)
#define ADC_RESOLUTION 12
#define ADC_GAIN ADC_GAIN_1
#define ADC_REFERENCE ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME ADC_ACQ_TIME_DEFAULT
#define ADC_1ST_CHANNEL_ID 1
#define ADC_2ND_CHANNEL_ID 7
#define ALIGNMENT 32
#define BUFFER_MEM_REGION __attribute__((__section__("SRAM4.dma")))

#endif

/* Invalid value that is not supposed to be written by the driver. It is used
 * to mark the sample buffer entries as empty. If needed, it can be overridden
 * for a particular board by providing a specific definition above.
 */
#if !defined(INVALID_ADC_VALUE)
#define INVALID_ADC_VALUE SHRT_MIN
#endif

/* Memory region where buffers will be placed. By default placed in ZTEST_BMEM
 * but can be overwritten for a particular board.
 */
#if !defined(BUFFER_MEM_REGION)
#define BUFFER_MEM_REGION EMPTY
#endif

/* The sample interval between consecutive samplings. Some drivers require
 * specific values to function.
 */
#if !defined(SAMPLE_INTERVAL_US)
#define SAMPLE_INTERVAL_US 0
#endif

#define BUFFER_SIZE 24
#ifndef ALIGNMENT
#define ALIGNMENT DMA_BUF_ADDR_ALIGNMENT(DT_NODELABEL(test_dma))
#endif

static BUFFER_MEM_REGION __aligned(ALIGNMENT) int16_t m_sample_buffer[BUFFER_SIZE];
static BUFFER_MEM_REGION __aligned(ALIGNMENT) int16_t m_sample_buffer2[2][BUFFER_SIZE];
static int current_buf_inx;

#if defined(CONFIG_ADC_ASYNC)
static struct k_poll_signal async_sig;
#endif

static const struct adc_channel_cfg m_1st_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_1ST_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_1ST_CHANNEL_INPUT,
#endif
};
#if defined(ADC_2ND_CHANNEL_ID)
static const struct adc_channel_cfg m_2nd_channel_cfg = {
	.gain = ADC_GAIN,
	.reference = ADC_REFERENCE,
	.acquisition_time = ADC_ACQUISITION_TIME,
	.channel_id = ADC_2ND_CHANNEL_ID,
#if defined(CONFIG_ADC_CONFIGURABLE_INPUTS)
	.input_positive = ADC_2ND_CHANNEL_INPUT,
#endif
};
#endif /* defined(ADC_2ND_CHANNEL_ID) */

#if defined(COUNTER_NODE_NAME)
static void init_counter(void)
{
	int err;
	const struct device *const dev = DEVICE_DT_GET(DT_NODELABEL(COUNTER_NODE_NAME));
	struct counter_top_cfg top_cfg = { .callback = NULL,
					   .user_data = NULL,
					   .flags = 0 };

	zassert_true(device_is_ready(dev), "Counter device is not ready");

	counter_start(dev);
	top_cfg.ticks = counter_us_to_ticks(dev, HW_TRIGGER_INTERVAL);
	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Counter failed to set top value (err: %d)",
		      dev->name, err);
}
#endif

static const struct device *init_adc(void)
{
	int i, ret;
	const struct device *const adc_dev = DEVICE_DT_GET(ADC_DEVICE_NODE);

	zassert_true(device_is_ready(adc_dev), "ADC device is not ready");

	ret = adc_channel_setup(adc_dev, &m_1st_channel_cfg);
	zassert_equal(ret, 0,
		      "Setting up of the first channel failed with code %d",
		      ret);

#if defined(ADC_2ND_CHANNEL_ID)
	ret = adc_channel_setup(adc_dev, &m_2nd_channel_cfg);
	zassert_equal(ret, 0,
		      "Setting up of the second channel failed with code %d",
		      ret);
#endif /* defined(ADC_2ND_CHANNEL_ID) */

	for (i = 0; i < BUFFER_SIZE; ++i) {
		m_sample_buffer[i] = INVALID_ADC_VALUE;
		m_sample_buffer2[0][i] = INVALID_ADC_VALUE;
		m_sample_buffer2[1][i] = INVALID_ADC_VALUE;
	}

#if defined(CONFIG_ADC_ASYNC)
	k_poll_signal_init(&async_sig);
#endif

#if defined(COUNTER_NODE_NAME)
	init_counter();
#endif

	return adc_dev;
}

static void check_samples(int expected_count)
{
	int i;

	TC_PRINT("Samples read: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		int16_t sample_value = m_sample_buffer[i];

		TC_PRINT("0x%04x ", sample_value);
		if (i && i % 10 == 0) {
			TC_PRINT("\n");
		}

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

static void check_samples2(int expected_count)
{
	int i;

	TC_PRINT("Samples read: ");
	for (i = 0; i < BUFFER_SIZE; i++) {
		int16_t sample_value = m_sample_buffer2[current_buf_inx][i];

		TC_PRINT("0x%04x ", sample_value);
		if (i && i % 10 == 0) {
			TC_PRINT("\n");
		}

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
	const struct adc_sequence sequence = {
		.channels = BIT(ADC_1ST_CHANNEL_ID),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	const struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1);

	return TC_PASS;
}

ZTEST_USER(adc_dma, test_adc_sample_one_channel)
{
	zassert_true(test_task_one_channel() == TC_PASS);
}

/*
 * test_adc_sample_two_channels
 */
#if defined(ADC_2ND_CHANNEL_ID)
static int test_task_two_channels(void)
{
	int ret;
	const struct adc_sequence sequence = {
		.channels = BIT(ADC_1ST_CHANNEL_ID) | BIT(ADC_2ND_CHANNEL_ID),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	const struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(2);

	return TC_PASS;
}
#endif /* defined(ADC_2ND_CHANNEL_ID) */

ZTEST_USER(adc_dma, test_adc_sample_two_channels)
{
#if defined(ADC_2ND_CHANNEL_ID)
	zassert_true(test_task_two_channels() == TC_PASS);
#else
	ztest_test_skip();
#endif /* defined(ADC_2ND_CHANNEL_ID) */
}

/*
 * test_adc_asynchronous_call
 */
#if defined(CONFIG_ADC_ASYNC)
static int test_task_asynchronous_call(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.extra_samplings = 4,
		/* Start consecutive samplings as fast as possible. */
		.interval_us = SAMPLE_INTERVAL_US,
	};
	const struct adc_sequence sequence = {
		.options = &options,
		.channels = BIT(ADC_1ST_CHANNEL_ID),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};
	struct k_poll_event async_evt = K_POLL_EVENT_INITIALIZER(
		K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY, &async_sig);
	const struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read_async(adc_dev, &sequence, &async_sig);
	zassert_equal(ret, 0, "adc_read_async() failed with code %d", ret);

	ret = k_poll(&async_evt, 1, K_MSEC(1000));
	zassert_equal(ret, 0, "k_poll failed with error %d", ret);

	check_samples(1 + options.extra_samplings);

	return TC_PASS;
}
#endif /* defined(CONFIG_ADC_ASYNC) */

ZTEST_USER(adc_dma, test_adc_asynchronous_call)
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
static enum adc_action
sample_with_interval_callback(const struct device *dev,
			      const struct adc_sequence *sequence,
			      uint16_t sampling_index)
{
	struct adc_sequence *seq = (struct adc_sequence *)sequence;
	int _inx = current_buf_inx;

	memcpy(m_sample_buffer, m_sample_buffer2[_inx],
	       sizeof(m_sample_buffer));
	current_buf_inx = (current_buf_inx == 0) ? 1 : 0;
	seq->buffer = m_sample_buffer2[current_buf_inx];
	return ADC_ACTION_CONTINUE;
}

static int test_task_with_interval(void)
{
	int ret;
	int count = 2;
	int64_t time_stamp;
	int64_t milliseconds_spent;

	const struct adc_sequence_options options = {
		.interval_us = 100 * 1000, /* 10 ms - much larger than expected sampling time */
		.callback = sample_with_interval_callback,
		.extra_samplings = 1,
	};
	const struct adc_sequence sequence = {
		.options = &options,
		.channels = BIT(ADC_1ST_CHANNEL_ID),
		.buffer = m_sample_buffer2[0],
		.buffer_size = sizeof(m_sample_buffer2[0]),
		.resolution = ADC_RESOLUTION,
	};

	const struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	current_buf_inx = 0;

	while (count--) {
		time_stamp = k_uptime_get();
		ret = adc_read(adc_dev, &sequence);
		milliseconds_spent = k_uptime_delta(&time_stamp);
		zassert_true(milliseconds_spent >= (options.interval_us / 1000UL));
		zassert_equal(ret, 0, "adc_read() failed with code %d", ret);
	}
	check_samples2(1 + options.extra_samplings);
	return TC_PASS;
}

ZTEST(adc_dma, test_adc_sample_with_interval)
{
	zassert_true(test_task_with_interval() == TC_PASS);
}

/*
 * test_adc_repeated_samplings
 */
static uint8_t m_samplings_done;
static enum adc_action
repeated_samplings_callback(const struct device *dev,
			    const struct adc_sequence *sequence,
			    uint16_t sampling_index)
{
	++m_samplings_done;
	TC_PRINT("%s: done %d\n", __func__, m_samplings_done);
	if (m_samplings_done == 1U) {
#if defined(ADC_2ND_CHANNEL_ID)
		check_samples(2);
#else
		check_samples(1);
#endif /* defined(ADC_2ND_CHANNEL_ID) */

		/* After first sampling continue normally. */
		return ADC_ACTION_CONTINUE;
	}
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

static int test_task_repeated_samplings(void)
{
	int ret;
	const struct adc_sequence_options options = {
		.callback = repeated_samplings_callback,
		/*
		 * This specifies that 3 samplings are planned. However,
		 * the callback function above is constructed in such way
		 * that the first sampling is done normally, the second one
		 * is repeated 9 times, and then the sequence is finished.
		 * Hence, the third sampling will not take place.
		 */
		.extra_samplings = 2,
		/* Start consecutive samplings as fast as possible. */
		.interval_us = SAMPLE_INTERVAL_US,
	};
	const struct adc_sequence sequence = {
		.options = &options,
#if defined(ADC_2ND_CHANNEL_ID)
		.channels = BIT(ADC_1ST_CHANNEL_ID) | BIT(ADC_2ND_CHANNEL_ID),
#else
		.channels = BIT(ADC_1ST_CHANNEL_ID),
#endif /* defined(ADC_2ND_CHANNEL_ID) */
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = ADC_RESOLUTION,
	};

	const struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	return TC_PASS;
}

ZTEST(adc_dma, test_adc_repeated_samplings)
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
		.channels = BIT(ADC_1ST_CHANNEL_ID),
		.buffer = m_sample_buffer,
		.buffer_size = sizeof(m_sample_buffer),
		.resolution = 0, /* intentionally invalid value */
	};

	const struct device *adc_dev = init_adc();

	if (!adc_dev) {
		return TC_FAIL;
	}

	ret = adc_read(adc_dev, &sequence);
	zassert_not_equal(ret, 0, "adc_read() unexpectedly succeeded");

#if defined(CONFIG_ADC_ASYNC)
	ret = adc_read_async(adc_dev, &sequence, &async_sig);
	zassert_not_equal(ret, 0, "adc_read_async() unexpectedly succeeded");
#endif

	/*
	 * Make the sequence parameters valid, now the request should succeed.
	 */
	sequence.resolution = ADC_RESOLUTION;

	ret = adc_read(adc_dev, &sequence);
	zassert_equal(ret, 0, "adc_read() failed with code %d", ret);

	check_samples(1);

	return TC_PASS;
}

ZTEST_USER(adc_dma, test_adc_invalid_request)
{
	zassert_true(test_task_invalid_request() == TC_PASS);
}
