/*
 * SPDX-FileCopyrightText: Copyright RAKwireless Technology Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/counter/counter_capture_emul.h>
#include <zephyr/drivers/gnss.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>
#include <zephyr/ztest.h>

static const struct device *const gnss = DEVICE_DT_GET(DT_NODELABEL(gnss));
static const struct device *const counter = DEVICE_DT_GET(DT_NODELABEL(counter0));

static void pulse(void)
{
	zassert_ok(counter_capture_emul_input_set(counter, 0, 1));
	zassert_ok(counter_capture_emul_input_set(counter, 0, 0));
}

static void wake(void)
{
	int ret;

	if (!IS_ENABLED(CONFIG_PM_DEVICE)) {
		return;
	}

	ret = pm_device_action_run(gnss, PM_DEVICE_ACTION_RESUME);
	zassert_true((ret == 0) || (ret == -EALREADY), "PM resume returned %d", ret);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	zassert_ok(counter_capture_emul_input_set(counter, 0, 0));
	wake();
}

ZTEST(gnss_timepulse, test_devices_are_ready)
{
	zassert_true(device_is_ready(gnss));
	zassert_true(device_is_ready(counter));
}

ZTEST(gnss_timepulse, test_pulse_is_reported)
{
	k_ticks_t before_pulse = k_uptime_ticks();
	k_ticks_t timepulse;
	k_ticks_t after_pulse;

	pulse();
	after_pulse = k_uptime_ticks();

	zassert_ok(gnss_get_latest_timepulse(gnss, &timepulse));

	/* The capture happened between the two readings, so it has to land
	 * between them however the counter counts.
	 */
	zassert_true(timepulse >= before_pulse, "timepulse %lld precedes the pulse %lld",
		     (long long)timepulse, (long long)before_pulse);
	zassert_true(timepulse <= after_pulse, "timepulse %lld follows the pulse %lld",
		     (long long)timepulse, (long long)after_pulse);
}

ZTEST(gnss_timepulse, test_later_pulse_reads_later)
{
	k_ticks_t first;
	k_ticks_t second;

	pulse();
	zassert_ok(gnss_get_latest_timepulse(gnss, &first));

	k_msleep(20);

	pulse();
	zassert_ok(gnss_get_latest_timepulse(gnss, &second));

	zassert_true(second > first, "the second pulse did not read later");
}

ZTEST(gnss_timepulse, test_reading_twice_is_stable)
{
	k_ticks_t once;
	k_ticks_t twice;

	pulse();

	zassert_ok(gnss_get_latest_timepulse(gnss, &once));
	k_msleep(10);
	zassert_ok(gnss_get_latest_timepulse(gnss, &twice));

	zassert_equal(once, twice, "the stored timepulse moved on its own");
}

ZTEST(gnss_timepulse, test_suspend_stops_capture)
{
	k_ticks_t before_suspend;
	k_ticks_t after_pulse;

	Z_TEST_SKIP_IFNDEF(CONFIG_PM_DEVICE);

	pulse();
	zassert_ok(gnss_get_latest_timepulse(gnss, &before_suspend));

	zassert_ok(pm_device_action_run(gnss, PM_DEVICE_ACTION_SUSPEND));

	k_msleep(10);
	pulse();

	zassert_ok(gnss_get_latest_timepulse(gnss, &after_pulse));
	zassert_equal(before_suspend, after_pulse,
		      "a pulse was timestamped while the device was suspended");

	/* And the device picks the pulse up again once it is back. */
	wake();
	k_msleep(10);
	pulse();

	zassert_ok(gnss_get_latest_timepulse(gnss, &after_pulse));
	zassert_true(after_pulse > before_suspend, "the pulse was missed after resume");
}

ZTEST_SUITE(gnss_timepulse, NULL, NULL, before, NULL, NULL);
