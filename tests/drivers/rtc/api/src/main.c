/*
 * Copyright (c) 2020 Paratronic
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <drivers/rtc.h>
#include <ztest.h>
#include <strings.h>
#include <stdlib.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(test);

#define RTC_NAME DT_RTC_0_NAME

void test_time(void)
{
	const int waitTime_ms = 1000;
	int ret;
	struct device *dev;
	struct timespec ts0;
	struct timespec ts;
	s64_t delta;

	dev = device_get_binding(RTC_NAME);
	zassert_not_null(dev, "Unable to get rtc device");

	/* Set a particular time: 2020-03-23 12:22:40 */
	ts0.tv_sec = 1584966160;
	ts0.tv_nsec = 0;
	ret = rtc_set_time(dev, &ts0);
	zassert_equal(ret, 0, "Fail to set realtime clock");

	k_sleep(waitTime_ms);

	ret = rtc_get_time(dev, &ts);
	zassert_equal(ret, 0, "Fail to get realtime clock");
	delta = (ts.tv_sec - ts0.tv_sec) * NSEC_PER_SEC +
		(ts.tv_nsec - ts0.tv_nsec);

	delta = delta / (NSEC_PER_SEC / MSEC_PER_SEC);

	zassert_true((abs(waitTime_ms - delta) < 100),
		     "Clock inaccurate: %dms instead of %dms",
		     (int)delta, waitTime_ms);
}

void test_main(void)
{
	struct device *dev;

	dev = device_get_binding(RTC_NAME);
	zassert_not_null(dev, "Unable to get rtc device");
	k_object_access_grant(dev, k_current_get());
	ztest_test_suite(rtc_driver,
			 ztest_user_unit_test(test_time));
	ztest_run_test_suite(rtc_driver);
}
