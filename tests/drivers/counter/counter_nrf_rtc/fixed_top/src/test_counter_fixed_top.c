/*
 * Copyright (c) 2019, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <hal/nrf_rtc.h>
LOG_MODULE_REGISTER(test);

static volatile uint32_t top_cnt;

#define DEVICE_DT_GET_AND_COMMA(node_id) DEVICE_DT_GET(node_id),
/* Generate a list of devices for all instances of the "compat" */
#define DEVS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_AND_COMMA)

#define DEVICE_DT_GET_REG_AND_COMMA(node_id) (NRF_RTC_Type *)DT_REG_ADDR(node_id),
/* Generate a list of devices for all instances of the "compat" */
#define REGS_FOR_DT_COMPAT(compat) \
	DT_FOREACH_STATUS_OKAY(compat, DEVICE_DT_GET_REG_AND_COMMA)

static const struct device *const devices[] = {
#ifdef CONFIG_COUNTER_NRF_RTC
	DEVS_FOR_DT_COMPAT(nordic_nrf_rtc)
#endif

};

static NRF_RTC_Type *const regs[] = {
#ifdef CONFIG_COUNTER_NRF_RTC
	REGS_FOR_DT_COMPAT(nordic_nrf_rtc)
#endif

};

typedef void (*counter_test_func_t)(int idx);

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
	zassert_true(ARRAY_SIZE(devices) > 0);
	for (int i = 0; i < ARRAY_SIZE(devices); i++) {
		counter_setup_instance(devices[i]);
		func(i);
		counter_tear_down_instance(devices[i]);
		/* Allow logs to be printed. */
		k_sleep(K_MSEC(100));
	}
}

static void test_set_custom_top_value_fails_on_instance(int idx)
{
	const struct device *dev = devices[idx];
	int err;
	struct counter_top_cfg top_cfg = {
		.callback = NULL,
		.flags = 0
	};

	top_cfg.ticks = counter_get_max_top_value(dev) - 1;

	err = counter_set_top_value(dev, &top_cfg);
	zassert_true(err != 0, "%s: Expected error code", dev->name);
}

ZTEST(counter, test_set_custom_top_value_fails)
{
	test_all_instances(test_set_custom_top_value_fails_on_instance);
}

static void top_handler(const struct device *dev, void *user_data)
{
	top_cnt++;
}

static void test_top_handler_on_instance(int idx)
{
	const struct device *dev = devices[idx];
	NRF_RTC_Type *reg = regs[idx];
	uint32_t tmp_top_cnt;
	int err;
	struct counter_top_cfg top_cfg = {
		.callback = top_handler,
		.flags = 0
	};

	top_cfg.ticks = counter_get_max_top_value(dev);

	err = counter_set_top_value(dev, &top_cfg);
	zassert_equal(0, err, "%s: Unexpected error code (%d)", dev->name, err);

	nrf_rtc_task_trigger(reg, NRF_RTC_TASK_TRIGGER_OVERFLOW);

	counter_start(dev);
	k_busy_wait(10000);

	tmp_top_cnt = top_cnt;
	zassert_equal(tmp_top_cnt, 1, "%s: Expected top handler", dev->name);
}

ZTEST(counter, test_top_handler)
{
	test_all_instances(test_top_handler_on_instance);
}

ZTEST_SUITE(counter, NULL, NULL, NULL, NULL, NULL);
