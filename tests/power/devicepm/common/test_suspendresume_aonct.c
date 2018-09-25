/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <device.h>
#include <rtc.h>
#include <counter.h>

extern int rtc_wakeup;
static struct device *dev;

void test_aonct_state(int state)
{
	rtc_wakeup = true;
	dev = device_get_binding(CONFIG_AON_COUNTER_QMSI_DEV_NAME);
	/**TESTPOINT: suspend external devices*/
	zassert_false(device_set_power_state(dev, state), NULL);
}

/*todo: leverage basic tests from aonct test set*/
void test_aonct_func(void)
{
	u32_t cnt0;

	zassert_false(counter_start(dev), NULL);

	for (volatile int delay = 5000; delay--;) {
	}
	/**TESTPOINT: verify aon counter read*/
	cnt0 = counter_read(dev);
	k_sleep(MSEC_PER_SEC);
	/**TESTPOINT: verify duration reference from rtc_read()*/
	zassert_true((counter_read(dev) - cnt0) >= RTC_ALARM_SECOND, NULL);
	counter_stop(dev);
}
