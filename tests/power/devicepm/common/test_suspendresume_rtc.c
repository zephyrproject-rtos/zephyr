/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <device.h>
#include <rtc.h>

static struct device *dev;
void test_rtc_state(int state)
{
	dev = device_get_binding(CONFIG_RTC_0_NAME);
	zassert_not_null(dev, NULL);
	/**TESTPOINT: suspend external devices*/
	zassert_false(device_set_power_state(dev, state), NULL);
}

/*todo: leverage basic tests from rtc test set*/
static struct k_sem rtc_sema;
static void rtc_cb(void)
{
	TC_PRINT("%s\n", __func__);
	k_sem_give(&rtc_sema);
}
void test_rtc_func(void)
{
	struct rtc_config config;
	s64_t t0;

	k_sem_init(&rtc_sema, 0, 1);

	config.init_val = 0;
	config.alarm_enable = 1;
	config.alarm_val = RTC_ALARM_SECOND;
	config.cb_fn = (void *)rtc_cb;
	rtc_enable(dev);
	t0 = k_uptime_get();
	zassert_false(rtc_set_config(dev, &config), NULL);

	/**TESTPOINT: check rtc alarm callback*/
	k_sem_take(&rtc_sema, K_FOREVER);
	/**TESTPOINT: check rtc alarm duration*/
	zassert_true(k_uptime_delta(&t0) >= MSEC_PER_SEC, NULL);
	/**TESTPOINT: check rtc read*/
	zassert_true(rtc_read(dev) >= RTC_ALARM_SECOND, NULL);
}
