/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Functional test for the NXP WKPU (Wake-up Unit) wakeup-controller (WUC)
 * driver on MCXE31x. It exercises the driver through an internal wakeup source
 * so that no external pin stimulus / loopback jumper is required.
 *
 * Per the MCXE31x Reference Manual ("WKPU wake-up source connectivity"), the RTC
 * timeout is WKPU wakeup source 1. The test enables that source through the WUC
 * API, programs an RTC counter alarm, and polls the WUC "triggered" status,
 * which the WKPU ISR latches when the RTC timeout fires.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/dt-bindings/wuc/wuc_nxp_wkpu.h>

/* counter_mcux_rtc_jdp: channel 0 == RTCVAL (the RTC timeout compare). */
#define RTC_ALARM_CHANNEL 0U

/* WKPU source 1 (RTC timeout), rising edge, interrupt event. */
#define WKPU_RTC_SOURCE NXP_WKPU_RTC_TIMEOUT(NXP_WKPU_EDGE_RISING, NXP_WKPU_EVENT_INT)

static const struct device *const wkpu = DEVICE_DT_GET(DT_NODELABEL(wkpu));
static const struct device *const rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));

static void rtc_alarm_callback(const struct device *dev, uint8_t chan_id, uint32_t ticks,
			       void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);
}

ZTEST(wuc_nxp_wkpu, test_rtc_timeout_wakeup_source)
{
	struct counter_alarm_cfg alarm_cfg;
	bool triggered = false;
	int err;

	zassert_true(device_is_ready(wkpu), "WKPU WUC device not ready");
	zassert_true(device_is_ready(rtc), "RTC counter device not ready");

	err = wuc_enable_wakeup_source(wkpu, WKPU_RTC_SOURCE);
	zassert_ok(err, "wuc_enable_wakeup_source() failed: %d", err);

	/* Drop any stale latched status from a previous run. */
	(void)wuc_clear_wakeup_source_triggered(wkpu, WKPU_RTC_SOURCE);

	alarm_cfg.callback = rtc_alarm_callback;
	alarm_cfg.ticks = counter_us_to_ticks(rtc, 3000000U); /* ~3 s */
	alarm_cfg.flags = 0U;
	alarm_cfg.user_data = NULL;

	err = counter_set_channel_alarm(rtc, RTC_ALARM_CHANNEL, &alarm_cfg);
	zassert_ok(err, "counter_set_channel_alarm() failed: %d", err);

	/* The RTC timeout drives WKPU source 1 -> WKPU ISR -> latched triggered. */
	for (int i = 0; i < 100; i++) {
		if (wuc_check_wakeup_source_triggered(wkpu, WKPU_RTC_SOURCE) == 1) {
			triggered = true;
			break;
		}
		k_msleep(100);
	}

	(void)counter_cancel_channel_alarm(rtc, RTC_ALARM_CHANNEL);
	(void)wuc_clear_wakeup_source_triggered(wkpu, WKPU_RTC_SOURCE);
	(void)wuc_disable_wakeup_source(wkpu, WKPU_RTC_SOURCE);

	zassert_true(triggered, "WKPU RTC-timeout wakeup source did not trigger within 10 s");
}

ZTEST_SUITE(wuc_nxp_wkpu, NULL, NULL, NULL, NULL, NULL);
