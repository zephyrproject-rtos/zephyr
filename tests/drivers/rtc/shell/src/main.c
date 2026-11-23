/*
 * Copyright (c) 2023 Prevas A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/rtc/rtc_fake.h>
#include <zephyr/fff.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

/**
 * @addtogroup t_rtc_driver
 * @{
 * @defgroup t_rtc_api test_rtc_shell
 * @}
 */

DEFINE_FFF_GLOBALS;

#define FAKE_RTC_NAME DEVICE_DT_NAME(DT_NODELABEL(fake_rtc))

/* Helper to mock time  */
struct time_mock_val {
	int ret_val;
	struct rtc_time rtc;
};
static struct time_mock_val get_time_mock;
static struct time_mock_val set_time_mock;

static int rtc_fake_get_time_mock(const struct device *dev, struct rtc_time *rtc)
{
	ARG_UNUSED(dev);

	*rtc = get_time_mock.rtc;
	return get_time_mock.ret_val;
}

static int rtc_fake_set_time_mock(const struct device *dev, const struct rtc_time *rtc)
{
	ARG_UNUSED(dev);

	set_time_mock.rtc = *rtc;
	return set_time_mock.ret_val;
}

static void configure_set_time_mock(int ret_val)
{
	set_time_mock.ret_val = ret_val;

	rtc_fake_set_time_fake.custom_fake = rtc_fake_set_time_mock;
}

static void configure_get_time_mock(int ret_val)
{
	get_time_mock.ret_val = ret_val;
	get_time_mock.rtc.tm_year = 2023 - 1900; /* rtc_time year offset */
	get_time_mock.rtc.tm_mon = 12 - 1;       /* rtc_time month offset */
	get_time_mock.rtc.tm_mday = 24;
	get_time_mock.rtc.tm_hour = 12;
	get_time_mock.rtc.tm_min = 34;
	get_time_mock.rtc.tm_sec = 56;

	rtc_fake_get_time_fake.custom_fake = rtc_fake_get_time_mock;
}

static void assert_set_time(int year, int mon, int mday, int hour, int min, int sec)
{
	const struct rtc_time *rtctime;

	zassert_equal(rtc_fake_set_time_fake.call_count, 1, "set_time not called");

	rtctime = &set_time_mock.rtc;

	zassert_equal(year, rtctime->tm_year + 1900, "Year mismatch");
	zassert_equal(mon, rtctime->tm_mon + 1, "Month mismatch");
	zassert_equal(mday, rtctime->tm_mday, "Day mismatch");
	zassert_equal(hour, rtctime->tm_hour, "Hour mismatch");
	zassert_equal(min, rtctime->tm_min, "Minute mismatch");
	zassert_equal(sec, rtctime->tm_sec, "Second mismatch");
}

ZTEST(rtc_shell, test_rtc_get_ok)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	configure_get_time_mock(0);

	err = shell_execute_cmd(sh, "rtc get " FAKE_RTC_NAME);
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(rtc_fake_get_time_fake.call_count, 1, "get_time not called");
}

ZTEST(rtc_shell, test_rtc_get_not_initialized)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	configure_get_time_mock(-ENODATA);

	err = shell_execute_cmd(sh, "rtc get " FAKE_RTC_NAME);
	zassert_ok(err, "shell command (err %d)", err);
	zassert_equal(rtc_fake_get_time_fake.call_count, 1, "get_time not called");
}

ZTEST(rtc_shell, test_rtc_get_error)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	configure_get_time_mock(-1);

	err = shell_execute_cmd(sh, "rtc get " FAKE_RTC_NAME);
	zassert_true(err != 0, "shell command (err %d)", err);
	zassert_equal(rtc_fake_get_time_fake.call_count, 1, "get_time not called");
}

ZTEST(rtc_shell, test_rtc_set_date)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	rtc_fake_set_time_fake.return_val = 0;

	configure_get_time_mock(0);
	configure_set_time_mock(0);

	err = shell_execute_cmd(sh, "rtc set " FAKE_RTC_NAME " 2022-05-17");
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(rtc_fake_get_time_fake.call_count, 1, "get_time not called");

	assert_set_time(2022, 5, 17, get_time_mock.rtc.tm_hour, get_time_mock.rtc.tm_min,
			get_time_mock.rtc.tm_sec);
}

ZTEST(rtc_shell, test_rtc_set_time)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	configure_get_time_mock(0);
	configure_set_time_mock(0);

	err = shell_execute_cmd(sh, "rtc set " FAKE_RTC_NAME " 23:45:16");
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(rtc_fake_get_time_fake.call_count, 1, "get_time not called");

	assert_set_time(2023, 12, 24, 23, 45, 16);
}

ZTEST(rtc_shell, test_rtc_set_full)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	configure_get_time_mock(0);
	configure_set_time_mock(0);

	err = shell_execute_cmd(sh, "rtc set " FAKE_RTC_NAME " 2022-05-17T23:45:16");
	zassert_ok(err, "failed to execute shell command (err %d)", err);
	zassert_equal(rtc_fake_get_time_fake.call_count, 1, "get_time not called");

	assert_set_time(2022, 5, 17, 23, 45, 16);
}

ZTEST(rtc_shell, test_rtc_set_error)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	int err;

	configure_get_time_mock(0);
	configure_set_time_mock(-EINVAL);

	err = shell_execute_cmd(sh, "rtc set " FAKE_RTC_NAME " 2022:05:17T23:45:16");
	zassert_true(err != 0, "failed to execute shell command (err %d)", err);
	zassert_equal(rtc_fake_get_time_fake.call_count, 1, "get_time not called");
	zassert_equal(rtc_fake_set_time_fake.call_count, 0, "set_time called");
}

static void *rtc_shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	/* Wait for the initialization of the shell dummy backend. */
	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(rtc_shell, NULL, rtc_shell_setup, NULL, NULL, NULL);
