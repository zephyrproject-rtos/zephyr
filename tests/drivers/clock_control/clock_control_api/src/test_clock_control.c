/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>
#include <drivers/clock_control.h>
#include <logging/log.h>
LOG_MODULE_REGISTER(test);

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_clock)
#include <drivers/clock_control/nrf_clock_control.h>
#endif

struct device_subsys_data {
	clock_control_subsys_t subsys;
	uint32_t startup_us;
};

struct device_data {
	const char *name;
	const struct device_subsys_data *subsys_data;
	uint32_t subsys_cnt;
};

static const struct device_data devices[] = {
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_clock)
	{
		.name = DT_LABEL(DT_INST(0, nordic_nrf_clock)),
		.subsys_data =  (const struct device_subsys_data[]){
			{
				.subsys = CLOCK_CONTROL_NRF_SUBSYS_HF,
				.startup_us =
					IS_ENABLED(CONFIG_SOC_SERIES_NRF91X) ?
						3000 : 400
			},
			{
				.subsys = CLOCK_CONTROL_NRF_SUBSYS_LF,
				.startup_us = (CLOCK_CONTROL_NRF_K32SRC ==
					NRF_CLOCK_LFCLK_RC) ? 1000 : 500000
			}
		},
		.subsys_cnt = CLOCK_CONTROL_NRF_TYPE_COUNT
	}
#endif
};


typedef void (*test_func_t)(const char *dev_name,
			    clock_control_subsys_t subsys,
			    uint32_t startup_us);

typedef bool (*test_capability_check_t)(const char *dev_name,
					clock_control_subsys_t subsys);

static void setup_instance(const char *dev_name, clock_control_subsys_t subsys)
{
	const struct device *dev = device_get_binding(dev_name);
	int err;
	k_busy_wait(1000);
	do {
		err = clock_control_off(dev, subsys);
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_clock)
		if (err == -EPERM) {
			struct onoff_manager *mgr =
				z_nrf_clock_control_get_onoff(subsys);

			err = onoff_release(mgr);
			if (err >= 0) {
				break;
			}
		}
#endif
	} while (clock_control_get_status(dev, subsys) !=
			CLOCK_CONTROL_STATUS_OFF);

	LOG_INF("setup done");
}

static void tear_down_instance(const char *dev_name,
				clock_control_subsys_t subsys)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_clock)
	/* Turn on LF clock using onoff service if it is disabled. */
	const struct device *clk =
		device_get_binding(DT_LABEL(DT_INST(0, nordic_nrf_clock)));
	struct onoff_client cli;
	struct onoff_manager *mgr =
		z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_LF);
	int err;

	if (clock_control_get_status(clk, CLOCK_CONTROL_NRF_SUBSYS_LF) !=
		CLOCK_CONTROL_STATUS_OFF) {
		return;
	}

	sys_notify_init_spinwait(&cli.notify);
	err = onoff_request(mgr, &cli);
	zassert_true(err >= 0, "");

	while (sys_notify_fetch_result(&cli.notify, &err) < 0) {
		/*empty*/
	}
	zassert_true(err >= 0, "");
#endif
}

static void test_with_single_instance(const char *dev_name,
				      clock_control_subsys_t subsys,
				      uint32_t startup_time,
				      test_func_t func,
				      test_capability_check_t capability_check)
{
	setup_instance(dev_name, subsys);

	if ((capability_check == NULL) || capability_check(dev_name, subsys)) {
		func(dev_name, subsys, startup_time);
	} else {
		PRINT("test skipped for subsys:%d\n", (int)subsys);
	}

	tear_down_instance(dev_name, subsys);
	/* Allow logs to be printed. */
	k_sleep(K_MSEC(100));
}
static void test_all_instances(test_func_t func,
				test_capability_check_t capability_check)
{
	for (size_t i = 0; i < ARRAY_SIZE(devices); i++) {
		for (size_t j = 0; j < devices[i].subsys_cnt; j++) {
			test_with_single_instance(devices[i].name,
					devices[i].subsys_data[j].subsys,
					devices[i].subsys_data[j].startup_us,
					func, capability_check);
		}
	}
}

/*
 * Basic test for checking correctness of getting clock status.
 */
static void test_on_off_status_instance(const char *dev_name,
					clock_control_subsys_t subsys,
					uint32_t startup_us)
{
	const struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;

	zassert_true(dev != NULL, "%s: Unknown device", dev_name);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_on(dev, subsys);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(status, CLOCK_CONTROL_STATUS_ON,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);
}

static void test_on_off_status(void)
{
	test_all_instances(test_on_off_status_instance, NULL);
}

static void async_capable_callback(const struct device *dev,
				   clock_control_subsys_t subsys,
				   void *user_data)
{
	/* empty */
}

/* Function checks if clock supports asynchronous starting. */
static bool async_capable(const char *dev_name, clock_control_subsys_t subsys)
{
	const struct device *dev = device_get_binding(dev_name);
	int err;

	err = clock_control_async_on(dev, subsys, async_capable_callback, NULL);
	if (err < 0) {
		printk("failed %d", err);
		return false;
	}

	while (clock_control_get_status(dev, subsys) !=
		CLOCK_CONTROL_STATUS_ON) {
		/* pend util clock is started */
	}

	err = clock_control_off(dev, subsys);
	if (err < 0) {
		printk("clock_control_off failed %d", err);
		return false;
	}

	return true;
}

/*
 * Test checks that callbacks are called after clock is started.
 */
static void clock_on_callback(const struct device *dev,
				clock_control_subsys_t subsys,
				void *user_data)
{
	bool *executed = (bool *)user_data;

	*executed = true;
}

static void test_async_on_instance(const char *dev_name,
				   clock_control_subsys_t subsys,
				   uint32_t startup_us)
{
	const struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	bool executed = false;

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_async_on(dev, subsys, clock_on_callback, &executed);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	/* wait for clock started. */
	k_busy_wait(startup_us);

	zassert_true(executed, "%s: Expected flag to be true", dev_name);
	zassert_equal(CLOCK_CONTROL_STATUS_ON,
			clock_control_get_status(dev, subsys),
			"Unexpected clock status");
}

static void test_async_on(void)
{
	test_all_instances(test_async_on_instance, async_capable);
}

/*
 * Test checks that when asynchronous clock enabling is scheduled but clock
 * is disabled before being started then callback is never called and error
 * is reported.
 */
static void test_async_on_stopped_on_instance(const char *dev_name,
					      clock_control_subsys_t subsys,
					      uint32_t startup_us)
{
	const struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;
	int key;
	bool executed = false;

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	/* lock to prevent clock interrupt for fast starting clocks.*/
	key = irq_lock();
	err = clock_control_async_on(dev, subsys, clock_on_callback, &executed);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	/* Attempt to stop clock while it is being started. */
	err = clock_control_off(dev, subsys);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	irq_unlock(key);

	k_busy_wait(10000);

	zassert_false(executed, "%s: Expected flag to be false", dev_name);
}

static void test_async_on_stopped(void)
{
	test_all_instances(test_async_on_stopped_on_instance, async_capable);
}

/*
 * Test checks that that second start returns error.
 */
static void test_double_start_on_instance(const char *dev_name,
						clock_control_subsys_t subsys,
						uint32_t startup_us)
{
	const struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_on(dev, subsys);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);

	err = clock_control_on(dev, subsys);
	zassert_true(err < 0, "%s: Unexpected return value:%d", dev_name, err);
}

static void test_double_start(void)
{
	test_all_instances(test_double_start_on_instance, NULL);
}

/*
 * Test checks that that second stop returns 0.
 * Test precondition: clock is stopped.
 */
static void test_double_stop_on_instance(const char *dev_name,
						clock_control_subsys_t subsys,
						uint32_t startup_us)
{
	const struct device *dev = device_get_binding(dev_name);
	enum clock_control_status status;
	int err;

	status = clock_control_get_status(dev, subsys);
	zassert_equal(CLOCK_CONTROL_STATUS_OFF, status,
			"%s: Unexpected status (%d)", dev_name, status);

	err = clock_control_off(dev, subsys);
	zassert_equal(0, err, "%s: Unexpected err (%d)", dev_name, err);
}

static void test_double_stop(void)
{
	test_all_instances(test_double_stop_on_instance, NULL);
}

void test_main(void)
{
	ztest_test_suite(test_clock_control,
		ztest_unit_test(test_on_off_status),
		ztest_unit_test(test_async_on),
		ztest_unit_test(test_async_on_stopped),
		ztest_unit_test(test_double_start),
		ztest_unit_test(test_double_stop)
			 );
	ztest_run_test_suite(test_clock_control);
}
