/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Functional test for the NXP WKPU (Wake-up Unit) interrupt controller driver
 * on MCXE31x, exercised through an *internal* wake-up source so that no
 * external pin stimulus / loopback jumper is required.
 *
 * Per the MCXE31x Reference Manual (section "WKPU wake-up source connectivity"),
 * the RTC timeout event is hard-wired to WKPU wake-up source 1. The RTC counter
 * alarm channel 0 (RTCVAL) generates that timeout. So the test:
 *   1. arms WKPU source 1 for a rising-edge interrupt,
 *   2. programs an RTC counter alarm a few seconds out, and
 *   3. asserts that the WKPU interrupt (IRQ 83) fires and runs the WKPU
 *      driver callback.
 *
 * The WKPU callback firing is the proof: the RTC's own NVIC interrupt path is
 * separate and does not pass through WKPU.
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/interrupt_controller/intc_wkpu_nxp_s32.h>
#include <zephyr/drivers/wuc.h>
#include <zephyr/dt-bindings/wuc/wuc_nxp_s32_wkpu.h>
#include <zephyr/irq.h>

/* RM: WKPU wake-up source 1 == RTC timeout. */
#define WKPU_RTC_TIMEOUT_SOURCE 1U
/* counter_mcux_rtc_jdp: channel 0 == RTCVAL (the RTC timeout compare). */
#define RTC_ALARM_CHANNEL       0U

static const struct device *const wkpu = DEVICE_DT_GET(DT_NODELABEL(wkpu));
static const struct device *const rtc = DEVICE_DT_GET(DT_NODELABEL(rtc));

static K_SEM_DEFINE(wkpu_sem, 0, 1);
static volatile uint8_t fired_source = 0xFFU;

static void wkpu_callback(uint8_t source, void *arg)
{
	ARG_UNUSED(arg);
	fired_source = source;
	k_sem_give(&wkpu_sem);
}

/* Required by the counter API; the RTC NVIC path is not the proof here. */
static void rtc_alarm_callback(const struct device *dev, uint8_t chan_id,
			       uint32_t ticks, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(ticks);
	ARG_UNUSED(user_data);
}

ZTEST(wkpu_nxp, test_rtc_timeout_triggers_wkpu)
{
	uint32_t freq;
	int err;

	zassert_true(device_is_ready(wkpu), "WKPU device not ready");
	zassert_true(device_is_ready(rtc), "RTC counter device not ready");

	/* Arm WKPU source 1 (RTC timeout). The source index is passed as the
	 * driver's "pin" argument so the callback can report which line fired.
	 */
	err = wkpu_nxp_s32_set_callback(wkpu, WKPU_RTC_TIMEOUT_SOURCE,
					WKPU_RTC_TIMEOUT_SOURCE, wkpu_callback, NULL);
	zassert_ok(err, "wkpu_nxp_s32_set_callback() failed: %d", err);

	zassert_ok(wkpu_nxp_s32_enable_interrupt(wkpu, WKPU_RTC_TIMEOUT_SOURCE,
						WKPU_NXP_S32_RISING_EDGE));

	/* Program an RTC compare a few seconds out on the RTCVAL channel. The
	 * counter is free-running from driver init.
	 */
	freq = counter_get_frequency(rtc);
	zassert_true(freq > 0U, "invalid RTC counter frequency");

	struct counter_alarm_cfg alarm_cfg = {
		.callback = rtc_alarm_callback,
		.ticks = counter_us_to_ticks(rtc, 3000000U), /* ~3 s */
		.flags = 0U,
	};

	err = counter_set_channel_alarm(rtc, RTC_ALARM_CHANNEL, &alarm_cfg);
	zassert_ok(err, "counter_set_channel_alarm() failed: %d", err);

	/* The RTC timeout must drive WKPU source 1 -> WKPU ISR -> wkpu_callback. */
	err = k_sem_take(&wkpu_sem, K_SECONDS(10));

	wkpu_nxp_s32_disable_interrupt(wkpu, WKPU_RTC_TIMEOUT_SOURCE);
	(void)counter_cancel_channel_alarm(rtc, RTC_ALARM_CHANNEL);

	zassert_ok(err, "WKPU interrupt from RTC timeout did not fire within 10 s");
	zassert_equal(fired_source, WKPU_RTC_TIMEOUT_SOURCE,
		      "WKPU fired on unexpected source %u (expected %u)",
		      fired_source, WKPU_RTC_TIMEOUT_SOURCE);
}

#if defined(CONFIG_WUC_NXP_S32_WKPU)
static const struct wuc_dt_spec rtc_wuc = {
	.dev = DEVICE_DT_GET(DT_NODELABEL(wkpu_wuc)),
	.id = NXP_S32_WKPU_SOURCE(WKPU_RTC_TIMEOUT_SOURCE, NXP_S32_WKPU_EDGE_RISING),
};

static void arm_rtc_alarm(void)
{
	struct counter_alarm_cfg alarm = {
		.callback = rtc_alarm_callback,
		.ticks = counter_us_to_ticks(rtc, 3000000U),
	};

	zassert_ok(counter_set_channel_alarm(rtc, RTC_ALARM_CHANNEL, &alarm));
}

ZTEST(wkpu_nxp, test_wuc_retains_status_after_irq)
{
	zassert_true(device_is_ready(rtc_wuc.dev));
	zassert_ok(wkpu_nxp_s32_set_callback(wkpu, WKPU_RTC_TIMEOUT_SOURCE,
					   WKPU_RTC_TIMEOUT_SOURCE, wkpu_callback, NULL));
	zassert_ok(wuc_enable_wakeup_source_dt(&rtc_wuc));
	zassert_ok(wkpu_nxp_s32_enable_interrupt(wkpu, WKPU_RTC_TIMEOUT_SOURCE,
						WKPU_NXP_S32_RISING_EDGE));
	arm_rtc_alarm();
	zassert_ok(k_sem_take(&wkpu_sem, K_SECONDS(10)));
	zassert_equal(fired_source, WKPU_RTC_TIMEOUT_SOURCE);
	zassert_equal(wuc_check_wakeup_source_triggered_dt(&rtc_wuc), 1);
	zassert_ok(wuc_clear_wakeup_source_triggered_dt(&rtc_wuc));
	zassert_equal(wuc_check_wakeup_source_triggered_dt(&rtc_wuc), 0);
}

ZTEST(wkpu_nxp, test_wuc_without_active_irq)
{
	bool triggered;

	zassert_true(device_is_ready(rtc_wuc.dev));
	zassert_ok(wuc_enable_wakeup_source_dt(&rtc_wuc));
	arm_rtc_alarm();
	/* Keep the originating peripheral's status until WUC has sampled it. */
	irq_disable(DT_IRQN(DT_NODELABEL(rtc)));
	triggered = WAIT_FOR(wuc_check_wakeup_source_triggered_dt(&rtc_wuc) == 1,
			     5000000U, k_usleep(1000));
	irq_enable(DT_IRQN(DT_NODELABEL(rtc)));

	zassert_true(triggered, "RTC event was not visible with the WKPU IRQ disabled");
	zassert_equal(wkpu_nxp_s32_get_pending(wkpu), 0U);
	zassert_equal(wuc_check_wakeup_source_triggered_dt(&rtc_wuc), 1);
	zassert_ok(wuc_clear_wakeup_source_triggered_dt(&rtc_wuc));
	zassert_equal(wuc_check_wakeup_source_triggered_dt(&rtc_wuc), 0);
}
#endif

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	zassert_true(device_is_ready(wkpu));
	zassert_true(device_is_ready(rtc));
	k_sem_reset(&wkpu_sem);
	fired_source = 0xffU;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);
	irq_enable(DT_IRQN(DT_NODELABEL(rtc)));
	wkpu_nxp_s32_disable_interrupt(wkpu, WKPU_RTC_TIMEOUT_SOURCE);
	wkpu_nxp_s32_unset_callback(wkpu, WKPU_RTC_TIMEOUT_SOURCE);
	zassert_ok(counter_cancel_channel_alarm(rtc, RTC_ALARM_CHANNEL));
#if defined(CONFIG_WUC_NXP_S32_WKPU)
	zassert_ok(wuc_disable_wakeup_source_dt(&rtc_wuc));
	zassert_ok(wuc_clear_wakeup_source_triggered_dt(&rtc_wuc));
#endif
}

ZTEST_SUITE(wkpu_nxp, NULL, NULL, before, after, NULL);
