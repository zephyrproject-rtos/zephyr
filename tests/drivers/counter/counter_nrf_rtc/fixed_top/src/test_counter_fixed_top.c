/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_rtc.h>
LOG_MODULE_REGISTER(test);

static volatile uint32_t top_cnt;

static const struct device *devices[] = {
#ifdef CONFIG_COUNTER_RTC0
	/* Nordic RTC0 may be reserved for Bluetooth */
	DEVICE_DT_GET(DT_NODELABEL(rtc0)),
#endif
	/* Nordic RTC1 is used for the system clock */
#ifdef CONFIG_COUNTER_RTC2
	DEVICE_DT_GET(DT_NODELABEL(rtc2)),
#endif

};

typedef void (*counter_test_func_t)(const struct device *dev);

static void counter_setup_instance(const struct device *dev)
{
	top_cnt = 0U;
}

static void counter_tear_down_instance(const struct device *dev)
{
	int err;

	err = counter_stop(dev);
	zassert_equal(0, err, "%s: Counter failed to stop", dev->name);

}

static void test_all_instances(counter_test_func_t func)
{
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		counter_setup_instance(devices[i]);
		func(devices[i]);
		counter_tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
}

void test_set_custom_top_value_fails_on_instance(const struct device *dev)
{
	int err;
	struct counter_top_cfg top_cfg = {
		.callback = NULL,
		.flags = 0
	};

	top_cfg.ticks = counter_get_max_top_value(dev) - 1;

	err = counter_set_top_value(dev, &top_cfg);
	zassert_true(err != 0, "%s: Expected error code", dev->name);
}

void test_set_custom_top_value_fails(void)
{
	test_all_instances(test_set_custom_top_value_fails_on_instance);
}

static void top_handler(const struct device *dev, void *user_data)
{
	top_cnt++;
}

void test_top_handler_on_instance(const struct device *dev)
{
	uint32_t tmp_top_cnt;
	int err;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.flags = 0
	};

	top_cfg.ticks = counter_get_max_top_value(dev);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Unexpected error code (%d)", dev->name, err);

#ifdef CONFIG_COUNTER_RTC0
	nrf_rtc_task_trigger(NRF_RTC0, NRF_RTC_TASK_TRIGGER_OVERFLOW);
#endif
#ifdef CONFIG_COUNTER_RTC2
	nrf_rtc_task_trigger(NRF_RTC2, NRF_RTC_TASK_TRIGGER_OVERFLOW);
#endif

	counter_start(dev);
	k_busy_wait(10000);

	tmp_top_cnt = top_cnt;
	zassert_equal(tmp_top_cnt, 1, "%s: Expected top handler", dev->name);
}

void test_top_handler(void)
{
	test_all_instances(test_top_handler_on_instance);
}

void test_main(void)
{
	ztest_test_suite(test_counter,
		ztest_unit_test(test_set_custom_top_value_fails),
		ztest_unit_test(test_top_handler)
			 );
	ztest_run_test_suite(test_counter);
}
