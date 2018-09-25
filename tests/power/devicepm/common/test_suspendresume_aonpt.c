/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <device.h>
#include <rtc.h>
#include <counter.h>

#define USER_DATA ((void *)0x1234)

static struct device *dev;
void test_aonpt_state(int state)
{
	dev = device_get_binding(CONFIG_AON_TIMER_QMSI_DEV_NAME);
	/**TESTPOINT: suspend external devices*/
	zassert_false(device_set_power_state(dev, state), NULL);
}

/*todo: leverage basic tests from rtc test set*/
static struct k_sem sync_sema;
static void aonpt_cb(struct device *aon_dev, void *user_data)
{
	/**TESTPOINT: check aonpt alarm callback param*/
	zassert_equal(aon_dev, dev, NULL);
	zassert_equal(user_data, USER_DATA, NULL);
	k_sem_give(&sync_sema);
}
void test_aonpt_func(void)
{
	u32_t cnt0;
	s64_t t0;

	k_sem_init(&sync_sema, 0, 1);

	/**TESTPOINT: check aonpt start*/
	zassert_false(counter_start(dev), NULL);
	zassert_false(counter_set_alarm(dev, aonpt_cb, RTC_ALARM_SECOND,
		      USER_DATA), NULL);
	t0 = k_uptime_get();
	/**TESTPOINT: check aonpt alarm callback*/
	k_sem_take(&sync_sema, K_FOREVER);
	/**TESTPOINT: check aonpt alarm duration*/
	zassert_true(k_uptime_delta(&t0) >= MSEC_PER_SEC, NULL);
	/**TESTPOINT: check aonpt counter read*/
	cnt0 = counter_read(dev);
	while (cnt0 == counter_read(dev))
		;
	/**TESTPOINT: check aonpt stop*/
	zassert_false(counter_stop(dev), NULL);
}
