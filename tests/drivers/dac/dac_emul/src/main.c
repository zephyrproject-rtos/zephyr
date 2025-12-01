/*
 * Copyright (c) 2025 Vaisala Oyj
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/dac/dac_emul.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/ztest.h>

LOG_MODULE_REGISTER(dac_emul_test);

/* Device tree nodes */
#define DAC_EMUL0_NODE             DT_NODELABEL(dac_emul0)
#define DAC_EMUL_UNCONFIGURED_NODE DT_NODELABEL(dac_emul_unconfigured)
#define DAC_EMUL_MANY_NODE         DT_NODELABEL(dac_emul_many)

/* Test devices */
static const struct device *dac_emul0 = DEVICE_DT_GET(DAC_EMUL0_NODE);
static const struct device *dac_emul_unconfigured = DEVICE_DT_GET(DAC_EMUL_UNCONFIGURED_NODE);
static const struct device *dac_emul_many = DEVICE_DT_GET(DAC_EMUL_MANY_NODE);

/*** Setup and Teardown ***/

static void *dac_emul_setup(void)
{
	zassert_true(device_is_ready(dac_emul0), "DAC emulator 0 not ready");
	zassert_true(device_is_ready(dac_emul_unconfigured), "DAC emulator 1 not ready");
	zassert_true(device_is_ready(dac_emul_many), "DAC emulator many not ready");

	return NULL;
}

/*** Channel Setup Tests ***/

ZTEST(dac_emul_tests, test_channel_setup_valid)
{
	struct dac_channel_cfg cfg = {
		.channel_id = 0,
		.resolution = 12,
	};

	int ret = dac_channel_setup(dac_emul0, &cfg);

	zassert_equal(ret, 0, "Valid channel setup should succeed");
}

ZTEST(dac_emul_tests, test_channel_setup_all_channels)
{
	/* Setup all 4 channels on dac_emul0 */
	for (uint8_t i = 0; i < 4; i++) {
		struct dac_channel_cfg cfg = {
			.channel_id = i, .resolution = 8 + i, /* 8, 9, 10, 11 bits */
		};

		int ret = dac_channel_setup(dac_emul0, &cfg);

		zassert_equal(ret, 0, "Channel %u setup should succeed", i);
	}
}

ZTEST(dac_emul_tests, test_channel_setup_invalid_channel)
{
	struct dac_channel_cfg cfg = {
		.channel_id = 10, /* Invalid for 4-channel device */
		.resolution = 12,
	};

	int ret = dac_channel_setup(dac_emul0, &cfg);

	zassert_equal(ret, -EINVAL, "Invalid channel should return -EINVAL");
}

ZTEST(dac_emul_tests, test_channel_setup_zero_resolution)
{
	struct dac_channel_cfg cfg = {
		.channel_id = 0, .resolution = 0, /* Invalid */
	};

	int ret = dac_channel_setup(dac_emul0, &cfg);

	zassert_equal(ret, -EINVAL, "Zero resolution should return -EINVAL");
}

ZTEST(dac_emul_tests, test_channel_setup_excessive_resolution)
{
	struct dac_channel_cfg cfg = {
		.channel_id = 0, .resolution = 33, /* Exceeds 32-bit maximum */
	};

	int ret = dac_channel_setup(dac_emul0, &cfg);

	zassert_equal(ret, -EINVAL, "Excessive resolution should return -EINVAL");
}

ZTEST(dac_emul_tests, test_channel_setup_resolution_boundaries)
{
	/* Test minimum valid resolution (1 bit) */
	struct dac_channel_cfg cfg1 = {
		.channel_id = 0,
		.resolution = 1,
	};
	int ret = dac_channel_setup(dac_emul0, &cfg1);

	zassert_equal(ret, 0, "1-bit resolution should be valid");

	/* Test maximum valid resolution (32 bits) */
	struct dac_channel_cfg cfg32 = {
		.channel_id = 1,
		.resolution = 32,
	};
	ret = dac_channel_setup(dac_emul0, &cfg32);
	zassert_equal(ret, 0, "32-bit resolution should be valid");
}

/*** Write Value Tests ***/

ZTEST(dac_emul_tests, test_write_value_basic)
{
	/* Setup channel first */
	struct dac_channel_cfg cfg = {
		.channel_id = 0,
		.resolution = 12,
	};
	int ret = dac_channel_setup(dac_emul0, &cfg);

	zassert_equal(ret, 0, "Setup should succeed");

	/* Write a value */
	uint32_t test_value = 2048; /* Half of 12-bit range */

	ret = dac_write_value(dac_emul0, 0, test_value);
	zassert_equal(ret, 0, "Write should succeed");

	/* Verify the value was written */
	uint32_t read_value;

	ret = dac_emul_value_get(dac_emul0, 0, &read_value);
	zassert_equal(ret, 0, "Read should succeed");
	zassert_equal(read_value, test_value, "Read value should match written value");
}

ZTEST(dac_emul_tests, test_write_value_boundary)
{
	/* Setup 8-bit channel */
	struct dac_channel_cfg cfg = {
		.channel_id = 0,
		.resolution = 8,
	};
	int ret = dac_channel_setup(dac_emul0, &cfg);

	zassert_equal(ret, 0, "Setup should succeed");

	/* Test minimum value (0) */
	ret = dac_write_value(dac_emul0, 0, 0);
	zassert_equal(ret, 0, "Write min value should succeed");

	uint32_t value;

	ret = dac_emul_value_get(dac_emul0, 0, &value);
	zassert_equal(value, 0, "Min value should be 0");

	/* Test maximum value (255 for 8-bit) */
	ret = dac_write_value(dac_emul0, 0, 255);
	zassert_equal(ret, 0, "Write max value should succeed");

	ret = dac_emul_value_get(dac_emul0, 0, &value);
	zassert_equal(value, 255, "Max value should be 255");
}

ZTEST(dac_emul_tests, test_write_value_out_of_range)
{
	/* Setup 8-bit channel */
	struct dac_channel_cfg cfg = {
		.channel_id = 0,
		.resolution = 8,
	};
	int ret = dac_channel_setup(dac_emul0, &cfg);

	zassert_equal(ret, 0, "Setup should succeed");

	/* Try to write value exceeding 8-bit range */
	ret = dac_write_value(dac_emul0, 0, 256);
	zassert_equal(ret, -EINVAL, "Out of range value should return -EINVAL");

	ret = dac_write_value(dac_emul0, 0, 1000);
	zassert_equal(ret, -EINVAL, "Out of range value should return -EINVAL");
}

ZTEST(dac_emul_tests, test_write_value_unconfigured_channel)
{
	/* Try to write to unconfigured channel */
	int ret = dac_write_value(dac_emul_unconfigured, 0, 100);

	zassert_equal(ret, -ENXIO, "Write to unconfigured channel should return -ENXIO");
}

ZTEST(dac_emul_tests, test_write_value_invalid_channel)
{
	/* Try to write to non-existent channel */
	int ret = dac_write_value(dac_emul0, 10, 100);

	zassert_equal(ret, -EINVAL, "Write to invalid channel should return -EINVAL");
}

/*** Read Value Tests ***/

ZTEST(dac_emul_tests, test_read_value_unconfigured_channel)
{
	uint32_t value;
	int ret = dac_emul_value_get(dac_emul_unconfigured, 0, &value);

	zassert_equal(ret, -ENXIO, "Read from unconfigured channel should return -ENXIO");
	zassert_equal(value, 0, "Value should be 0 for unconfigured channel");
}

ZTEST(dac_emul_tests, test_read_value_invalid_channel)
{
	uint32_t value;
	int ret = dac_emul_value_get(dac_emul0, 10, &value);

	zassert_equal(ret, -EINVAL, "Read from invalid channel should return -EINVAL");
}

ZTEST(dac_emul_tests, test_read_value_null_pointer)
{
	/* Setup channel first */
	struct dac_channel_cfg cfg = {
		.channel_id = 0,
		.resolution = 12,
	};
	int ret = dac_channel_setup(dac_emul0, &cfg);

	zassert_equal(ret, 0, "Setup should succeed");

	/* Try to read with NULL pointer */
	ret = dac_emul_value_get(dac_emul0, 0, NULL);
	zassert_equal(ret, -EINVAL, "NULL pointer should return -EINVAL");
}

/*** Multiple Resolution Tests ***/

ZTEST(dac_emul_tests, test_different_resolutions)
{
	struct {
		uint8_t channel;
		uint8_t resolution;
		uint32_t test_value;
		uint32_t max_value;
	} tests[] = {
		{0, 1, 1, 1},          /* 1-bit: max = 1 */
		{1, 8, 128, 255},      /* 8-bit: max = 255 */
		{2, 12, 2048, 4095},   /* 12-bit: max = 4095 */
		{3, 16, 32768, 65535}, /* 16-bit: max = 65535 */
	};

	for (size_t i = 0; i < ARRAY_SIZE(tests); i++) {
		/* Setup channel */
		struct dac_channel_cfg cfg = {
			.channel_id = tests[i].channel,
			.resolution = tests[i].resolution,
		};
		int ret = dac_channel_setup(dac_emul0, &cfg);

		zassert_equal(ret, 0, "Setup channel %u should succeed", tests[i].channel);

		/* Write test value */
		ret = dac_write_value(dac_emul0, tests[i].channel, tests[i].test_value);
		zassert_equal(ret, 0, "Write to channel %u should succeed", tests[i].channel);

		/* Read back and verify */
		uint32_t value;

		ret = dac_emul_value_get(dac_emul0, tests[i].channel, &value);
		zassert_equal(ret, 0, "Read from channel %u should succeed", tests[i].channel);
		zassert_equal(value, tests[i].test_value, "Channel %u value mismatch",
			      tests[i].channel);

		/* Verify maximum value works */
		ret = dac_write_value(dac_emul0, tests[i].channel, tests[i].max_value);
		zassert_equal(ret, 0, "Write max value to channel %u should succeed",
			      tests[i].channel);

		/* Verify exceeding maximum fails */
		ret = dac_write_value(dac_emul0, tests[i].channel, tests[i].max_value + 1);
		zassert_equal(ret, -EINVAL, "Write beyond max to channel %u should fail",
			      tests[i].channel);
	}
}

/*** Multiple Device Tests ***/

ZTEST(dac_emul_tests, test_multiple_devices_independent)
{
	/* Setup channel on first device */
	struct dac_channel_cfg cfg0 = {
		.channel_id = 0,
		.resolution = 8,
	};
	int ret = dac_channel_setup(dac_emul0, &cfg0);

	zassert_equal(ret, 0, "Setup device 0 should succeed");

	ret = dac_write_value(dac_emul0, 0, 100);
	zassert_equal(ret, 0, "Write to device 0 should succeed");

	/* Setup channel on second device */
	struct dac_channel_cfg cfg1 = {
		.channel_id = 0,
		.resolution = 12,
	};
	ret = dac_channel_setup(dac_emul_many, &cfg1);
	zassert_equal(ret, 0, "Setup device 1 should succeed");

	ret = dac_write_value(dac_emul_many, 0, 2000);
	zassert_equal(ret, 0, "Write to device 1 should succeed");

	/* Verify both devices maintain independent values */
	uint32_t value0, value1;

	ret = dac_emul_value_get(dac_emul0, 0, &value0);
	zassert_equal(ret, 0, "Read device 0 should succeed");
	zassert_equal(value0, 100, "Device 0 value should be unchanged");

	ret = dac_emul_value_get(dac_emul_many, 0, &value1);
	zassert_equal(ret, 0, "Read device 1 should succeed");
	zassert_equal(value1, 2000, "Device 1 value should be correct");
}

/*** Concurrent Access Tests ***/

#define CONCURRENT_ITERATIONS         100
#define CONCURRENT_THREADS            4
#define CONCURRENT_THREADS_STACK_SIZE 1024
/* Create threads */
K_THREAD_STACK_ARRAY_DEFINE(stacks, CONCURRENT_THREADS, CONCURRENT_THREADS_STACK_SIZE);
struct k_thread threads[CONCURRENT_THREADS];

struct concurrent_test_data {
	const struct device *dev;
	uint8_t channel;
	uint32_t start_value;
	volatile bool start_flag;
};

static void concurrent_write_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct concurrent_test_data *data = arg1;

	/* Wait for start signal */
	while (!data->start_flag) {
		k_yield();
	}

	/* Perform writes */
	for (int i = 0; i < CONCURRENT_ITERATIONS; i++) {
		uint32_t value = data->start_value + i;
		int ret = dac_write_value(data->dev, data->channel, value % 256);

		zassert_equal(ret, 0, "Concurrent write should succeed");
		k_yield();
	}
}

ZTEST(dac_emul_tests, test_concurrent_writes)
{
	struct dac_channel_cfg cfg = {
		.channel_id = 0,
		.resolution = 8,
	};
	int ret = dac_channel_setup(dac_emul_many, &cfg);

	zassert_equal(ret, 0, "Setup should succeed");

	struct concurrent_test_data test_data = {
		.dev = dac_emul_many,
		.channel = 0,
		.start_value = 0,
		.start_flag = false,
	};

	for (int i = 0; i < CONCURRENT_THREADS; i++) {
		k_thread_create(&threads[i], stacks[i], CONCURRENT_THREADS_STACK_SIZE,
				concurrent_write_thread, &test_data, NULL, NULL, K_PRIO_PREEMPT(5),
				0, K_NO_WAIT);
	}

	/* Start all threads */
	test_data.start_flag = true;

	/* Wait for threads to complete */
	for (int i = 0; i < CONCURRENT_THREADS; i++) {
		k_thread_join(&threads[i], K_FOREVER);
	}

	/* Verify device is still functional */
	ret = dac_write_value(dac_emul_many, 0, 123);
	zassert_equal(ret, 0, "Write after concurrent access should succeed");

	uint32_t value;

	ret = dac_emul_value_get(dac_emul_many, 0, &value);
	zassert_equal(ret, 0, "Read after concurrent access should succeed");
	zassert_equal(value, 123, "Value should be correct");
}

ZTEST(dac_emul_tests, test_many_channels)
{
	/* Setup and test all 16 channels */
	for (uint8_t ch = 0; ch < 16; ch++) {
		struct dac_channel_cfg cfg = {
			.channel_id = ch,
			.resolution = 8,
		};

		int ret = dac_channel_setup(dac_emul_many, &cfg);

		zassert_equal(ret, 0, "Setup channel %u should succeed", ch);

		/* Write unique value to each channel */
		uint32_t write_val = ch * 10;

		ret = dac_write_value(dac_emul_many, ch, write_val);
		zassert_equal(ret, 0, "Write to channel %u should succeed", ch);
	}

	/* Verify all channels have correct values */
	for (uint8_t ch = 0; ch < 16; ch++) {
		uint32_t value;

		int ret = dac_emul_value_get(dac_emul_many, ch, &value);

		zassert_equal(ret, 0, "Read from channel %u should succeed", ch);
		zassert_equal(value, ch * 10, "Channel %u value mismatch", ch);
	}
}

/*** Edge Case Tests ***/

ZTEST(dac_emul_tests, test_32bit_resolution)
{
	/* Test maximum resolution (32 bits) */
	struct dac_channel_cfg cfg = {
		.channel_id = 0,
		.resolution = 32,
	};
	int ret = dac_channel_setup(dac_emul_many, &cfg);

	zassert_equal(ret, 0, "32-bit resolution setup should succeed");

	/* Write maximum 32-bit value */
	uint32_t max_val = 0xFFFFFFFF;

	ret = dac_write_value(dac_emul_many, 0, max_val);
	zassert_equal(ret, 0, "Write max 32-bit value should succeed");

	/* Read back */
	uint32_t value;

	ret = dac_emul_value_get(dac_emul_many, 0, &value);

	zassert_equal(ret, 0, "Read should succeed");
	zassert_equal(value, max_val, "Value should match");
}

ZTEST(dac_emul_tests, test_1bit_resolution)
{
	/* Test minimum resolution (1 bit) */
	struct dac_channel_cfg cfg = {
		.channel_id = 0,
		.resolution = 1,
	};

	int ret = dac_channel_setup(dac_emul_many, &cfg);

	zassert_equal(ret, 0, "1-bit resolution setup should succeed");

	ret = dac_write_value(dac_emul_many, 0, 0);
	zassert_equal(ret, 0, "Write 0 should succeed");

	uint32_t value;

	ret = dac_emul_value_get(dac_emul_many, 0, &value);
	zassert_equal(value, 0, "Value should be 0");

	ret = dac_write_value(dac_emul_many, 0, 1);
	zassert_equal(ret, 0, "Write 1 should succeed");

	ret = dac_emul_value_get(dac_emul_many, 0, &value);
	zassert_equal(value, 1, "Value should be 1");

	/* Test invalid value (2) */
	ret = dac_write_value(dac_emul_many, 0, 2);
	zassert_equal(ret, -EINVAL, "Write 2 should fail for 1-bit");
}

/*** Test Suite Definition ***/

ZTEST_SUITE(dac_emul_tests, NULL, dac_emul_setup, NULL, NULL, NULL);
