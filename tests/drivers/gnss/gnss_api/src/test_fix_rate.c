/*
 * Copyright 2024 Trackunit Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/gnss.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/ztest.h>

#define TEST_VALIDATE_PERIOD_MS 10000
#define TEST_VALIDATE_PERIOD    K_MSEC(TEST_VALIDATE_PERIOD_MS)

#define TEST_MIN_CALLBACK_COUNT(_fix_interval) \
	(((TEST_VALIDATE_PERIOD_MS / _fix_interval) / 5) * 4)

#define TEST_MAX_CALLBACK_COUNT(_fix_interval) \
	(((TEST_VALIDATE_PERIOD_MS / _fix_interval) / 5) * 6)

#define TEST_CONFIG_DEFINE(_fix_interval)					\
	{									\
		.fix_interval = _fix_interval,					\
		.min_callback_count = TEST_MIN_CALLBACK_COUNT(_fix_interval),	\
		.max_callback_count = TEST_MAX_CALLBACK_COUNT(_fix_interval),	\
	}

static const struct device *dev = DEVICE_DT_GET(DT_ALIAS(gnss));

struct test_config {
	uint32_t fix_interval;
	uint32_t min_callback_count;
	uint32_t max_callback_count;
};

static const struct test_config configs[] = {
	TEST_CONFIG_DEFINE(100),
	TEST_CONFIG_DEFINE(500),
	TEST_CONFIG_DEFINE(1000),
	TEST_CONFIG_DEFINE(2000),
};

static atomic_t callback_count_atom = ATOMIC_INIT(0);

static void gnss_data_cb(const struct device *dev, const struct gnss_data *data)
{
	atomic_inc(&callback_count_atom);
}

GNSS_DATA_CALLBACK_DEFINE(DEVICE_DT_GET(DT_ALIAS(gnss)), gnss_data_cb);

static bool test_set_fix_rate(const struct test_config *config)
{
	int ret;

	ret = gnss_set_fix_rate(dev, config->fix_interval);
	if (ret == -ENOSYS) {
		ztest_test_skip();
	}
	if (ret == -EINVAL) {
		return false;
	}
	zassert_ok(ret, "failed to set fix rate %u", config->fix_interval);
	return true;
}

static void test_validate_fix_rate(const struct test_config *config)
{
	bool valid;
	uint32_t callback_count;

	atomic_set(&callback_count_atom, 0);
	k_sleep(TEST_VALIDATE_PERIOD);
	callback_count = atomic_get(&callback_count_atom);
	valid = (callback_count >= config->min_callback_count) &&
		(callback_count <= config->max_callback_count);
	zassert_true(valid, "callback count %u invalid", callback_count);
}

ZTEST(gnss_api, test_fix_rate)
{
	for (uint32_t i = 0; i < ARRAY_SIZE(configs); i++) {
		if (!test_set_fix_rate(&configs[i])) {
			continue;
		}
		test_validate_fix_rate(&configs[i]);
	}
}
