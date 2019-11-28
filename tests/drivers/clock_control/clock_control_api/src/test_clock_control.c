/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <clock_control.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(test);

#ifdef DT_INST_0_NORDIC_NRF_CLOCK_LABEL
#include <drivers/clock_control/nrf_clock_control.h>
#endif

struct device_data {
	const char *name;
	u32_t startup_us;
};

static const struct device_data devices[] = {
#ifdef DT_INST_0_NORDIC_NRF_CLOCK_LABEL
	{
		.name = DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_16M",
		.startup_us = IS_ENABLED(CONFIG_SOC_SERIES_NRF91X) ? 3000 : 400
	},

	{
		.name = DT_INST_0_NORDIC_NRF_CLOCK_LABEL  "_32K",
		.startup_us = (CLOCK_CONTROL_NRF_K32SRC == NRF_CLOCK_LFCLK_RC) ?
				1000 : 300000
	}
#endif
};


typedef void (*test_func_t)(const char *dev_name, u32_t startup_us);

typedef bool (*test_capability_check_t)(const char *dev_name);

static void setup_instance(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);
	int err;

	k_busy_wait(1000);
	do {
		err = clock_control_off(dev, 0);
	} while (err == 0);
	LOG_INF("setup done");
}

static void tear_down_instance(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);

	clock_control_on(dev, NULL);
}

static void test_all_instances(test_func_t func,
				test_capability_check_t capability_check)
{
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		if ((capability_check == NULL) ||
			capability_check(devices[i].name)) {
			setup_instance(devices[i].name);
			func(devices[i].name, devices[i].startup_us);
			tear_down_instance(devices[i].name);
			/* Allow logs to be printed. */
			k_sleep(K_MSEC(100));
		}
	}
}

/*
 * Basic test for checking correctness of getting clock status.
 */
static void test_on_off_status_instance(const char *dev_name, u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;

	zassert_true(dev != NULL, "%s: Unknown device", dev_name);

	status = clock_control_get_status(dev, NULL);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);


	err = clock_control_on(dev, NULL);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	status = clock_control_get_status(dev, NULL);
	zassert_true((status == CLOCK_CONTROL_STATUS_STARTING) ||
			(status == CLOCK_CONTROL_STATUS_ON),
			"%s: Unexpected status (%d)", dev_name, status);

	k_busy_wait(startup_us);

	status = clock_control_get_status(dev, NULL);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, 0);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	status = clock_control_get_status(dev, NULL);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);
}

static void test_on_off_status(void)
{
	test_all_instances(test_on_off_status_instance, NULL);
}

/*
 * Test validates that if number of enabling requests matches disabling requests
 * then clock is disabled.
 */
static void test_multiple_users_instance(const char *dev_name, u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int users = 5;
	int err;

	status = clock_control_get_status(dev, NULL);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	for (int i = 0; i < users; i++) {
		err = clock_control_on(dev, NULL);
		zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);
	}

	status = clock_control_get_status(dev, NULL);
	zassert_true((status == CLOCK_CONTROL_STATUS_STARTING) ||
			(status == CLOCK_CONTROL_STATUS_ON),
			"%s: Unexpected status (%d)", dev_name, status);

	for (int i = 0; i < users; i++) {
		err = clock_control_off(dev, 0);
		zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);
	}

	status = clock_control_get_status(dev, NULL);
	zassert_true(status == CLOCK_CONTROL_STATUS_OFF,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, 0);
	zassert_equal(-EALREADY, err, "%s: Unexpected err (%d)", dev_name, err);
}

static void test_multiple_users(void)
{
	test_all_instances(test_multiple_users_instance, NULL);
}

static bool async_capable(const char *dev_name)
{
	struct device *dev = device_get_binding(dev_name);

	if (clock_control_async_on(dev, 0, NULL) != 0) {
		return false;
	}

	clock_control_off(dev, 0);

	return true;
}

/*
 * Test checks that callbacks are called after clock is started.
 */
static void clock_on_callback(struct device *dev, void *user_data)
{
	bool *executed = (bool *)user_data;

	*executed = true;
}

static void test_async_on_instance(const char *dev_name, u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	bool executed1 = false;
	bool executed2 = false;
	struct clock_control_async_data data1 = {
		.cb = clock_on_callback,
		.user_data = &executed1
	};
	struct clock_control_async_data data2 = {
		.cb = clock_on_callback,
		.user_data = &executed2
	};

	status = clock_control_get_status(dev, NULL);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_async_on(dev, 0, &data1);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	err = clock_control_async_on(dev, 0, &data2);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	/* wait for clock started. */
	k_busy_wait(startup_us);

	zassert_true(executed1, "%s: Expected flag to be true", dev_name);
	zassert_true(executed2, "%s: Expected flag to be true", dev_name);
}

static void test_async_on(void)
{
	test_all_instances(test_async_on_instance, async_capable);
}

/*
 * Test checks that when asynchronous clock enabling is scheduled but clock
 * is disabled before being started then callback is never called.
 */
static void test_async_on_stopped_on_instance(const char *dev_name,
						u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	int key;
	bool executed1 = false;
	struct clock_control_async_data data1 = {
		.cb = clock_on_callback,
		.user_data = &executed1
	};

	status = clock_control_get_status(dev, NULL);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	/* lock to prevent clock interrupt for fast starting clocks.*/
	key = irq_lock();
	err = clock_control_async_on(dev, 0, &data1);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	err = clock_control_off(dev, 0);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	irq_unlock(key);

	k_busy_wait(10000);

	zassert_false(executed1, "%s: Expected flag to be false", dev_name);
}

static void test_async_on_stopped(void)
{
	test_all_instances(test_async_on_stopped_on_instance, async_capable);
}

/*
 * Test checks that when asynchronous clock enabling is called when clock is
 * running then callback is immediate.
 */
static void test_immediate_cb_when_clock_on_on_instance(const char *dev_name,
							u32_t startup_us)
{
	struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	bool executed1 = false;
	struct clock_control_async_data data1 = {
		.cb = clock_on_callback,
		.user_data = &executed1
	};

	status = clock_control_get_status(dev, NULL);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_on(dev, NULL);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	/* wait for clock started. */
	k_busy_wait(startup_us);

	status = clock_control_get_status(dev, NULL);
	zassert_equal(CLOCK_CONTROL_STATUS_ON, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_async_on(dev, 0, &data1);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	zassert_true(executed1, "%s: Expected flag to be false", dev_name);
}

static void test_immediate_cb_when_clock_on(void)
{
	test_all_instances(test_immediate_cb_when_clock_on_on_instance,
			   async_capable);
}

void test_main(void)
{
	ztest_test_suite(test_clock_control,
		ztest_unit_test(test_on_off_status),
		ztest_unit_test(test_multiple_users),
		ztest_unit_test(test_async_on),
		ztest_unit_test(test_async_on_stopped),
		ztest_unit_test(test_immediate_cb_when_clock_on)
			 );
	ztest_run_test_suite(test_clock_control);
}
