/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device.h>

#include <esp_private/esp_clk.h>
#include <pmstats.h>
#include <soc/rtc.h>
#include <soc/soc_caps.h>
#if defined(CONFIG_SOC_SERIES_ESP32C5)
#include <hal/efuse_hal.h>
#include <soc/chip_revision.h>
/* Match HAL top_domain_pd_allowed(): TOP PD from chip rev 1.2 (102). */
#define ESP32C5_TOP_PD_MIN_REV 102
#endif

static const uint32_t sleep_duration_ms[] = {20U, 50U, 100U};

/*
 * A relative k_sleep() expires on the tick boundary after the requested count,
 * so a sleep that is not late returns exactly requested_ticks + 1. Allow one
 * more for the case where a tick boundary falls between sampling uptime and
 * arming the timeout.
 */
#define LATE_TOL_TICKS 1

/*
 * How far the compensated kernel timer may diverge from the always-on LP timer
 * over one sleep. Covers LP-counter read granularity (one slow-clock tick,
 * ~7 us) and wake-path noise, which is larger on parts whose esp_timer is
 * resynced in software after sleep.
 */
#define DRIFT_TOL_US 200ULL

/*
 * wake_margin_us = wake - deadline (+late / -early).
 * Pass if margin <= 0 (not late) and margin >= -WAKE_EARLY_MAX_US.
 */
#define WAKE_EARLY_MAX_US ((int64_t)CONFIG_SOC_ESP32_PM_WAKEUP_MARGIN_US + 2000LL)

#define TC_BLANK() TC_PRINT("%s\n", " ")

static uint32_t standby_entries;
static uint32_t standby_exits;

static void pm_entry(enum pm_state state)
{
	if (state == PM_STATE_STANDBY) {
		standby_entries++;
	}
}

static void pm_exit(enum pm_state state)
{
	if (state == PM_STATE_STANDBY) {
		standby_exits++;
	}
}

static struct pm_notifier notifier = {
	.state_entry = pm_entry,
	.state_exit = pm_exit,
};

static uint64_t abs_diff_u64(uint64_t a, uint64_t b)
{
	return (a > b) ? (a - b) : (b - a);
}

static void print_us_as_ms(const char *label, int64_t us)
{
	bool neg = us < 0;
	uint64_t abs_us = neg ? (uint64_t)(-us) : (uint64_t)us;

	TC_PRINT("  %-28s %s%llu.%03llu ms\n", label, neg ? "-" : "", abs_us / 1000ULL,
		 abs_us % 1000ULL);
}

static void settle_console(void);

ZTEST(pm_sleep, test_delta_time)
{
	uint32_t entries_at_start = standby_entries;

	TC_BLANK();
	TC_PRINT("Goal: Verify that the kernel timer is properly compensated for time "
		 "spent in sleep,\n");
	TC_PRINT("      using LP timer (always-on) as reference.\n");
	TC_BLANK();
	TC_PRINT("  Pass if:\n");
	TC_PRINT("    - sleep ended on the tick its timeout was due, or at most %u tick(s) "
		 "later\n",
		 LATE_TOL_TICKS);
	TC_PRINT("    - kernel timer and LP timer match within %llu us\n", DRIFT_TOL_US);

	/* Let the prints above drain so the first sleep is not cut short by a
	 * console wake, then note the window count to confirm real sleep below.
	 */
	settle_console();
	uint32_t win_seq0 = esp32_sleep_stats_get(NULL);

	for (size_t i = 0; i < ARRAY_SIZE(sleep_duration_ms); i++) {
		uint32_t dur_ms = sleep_duration_ms[i];
		uint64_t requested_us = (uint64_t)dur_ms * USEC_PER_MSEC;
		uint32_t requested_ticks = k_ms_to_ticks_ceil32(dur_ms);
		uint32_t due_ticks = requested_ticks + 1;

		/*
		 * Take the LP timer as raw counter ticks and convert the
		 * difference with one calibration value, the way the HAL builds
		 * its own sleep compensation. esp_rtc_get_time_us() cannot be
		 * used as a reference here: it scales the absolute counter by
		 * whatever calibration is current, and the HAL installs a
		 * freshly measured one on every sleep entry, so two of its
		 * readings do not share a time base. Reading the cycle counter
		 * and the LP timer in the same order on both sides keeps the
		 * gap between the two reads out of the result.
		 */
		int64_t k0 = k_uptime_ticks();
		uint32_t c0 = k_cycle_get_32();
		uint64_t t0 = rtc_time_get();

		k_sleep(K_MSEC(dur_ms));

		int64_t k1 = k_uptime_ticks();
		uint32_t c1 = k_cycle_get_32();
		uint64_t t1 = rtc_time_get();
		int64_t dticks = k1 - k0;
		uint64_t clock_us = k_cyc_to_us_near64(c1 - c0);
		uint64_t lp_us = rtc_time_slowclk_to_us(t1 - t0, esp_clk_slowclk_cal_get());
		int64_t drift_us = (int64_t)lp_us - (int64_t)clock_us;

		TC_BLANK();
		TC_PRINT("  --- sleep request: %u ms ---\n", dur_ms);
		print_us_as_ms("requested", (int64_t)requested_us);
		TC_PRINT("  %-28s %lld  (due %u, allowed up to %u)\n", "kernel ticks",
			 (long long)dticks, due_ticks, due_ticks + LATE_TOL_TICKS);
		print_us_as_ms("kernel timer", (int64_t)clock_us);
		print_us_as_ms("LP timer", (int64_t)lp_us);
		print_us_as_ms("drift (LP - kernel)", drift_us);

		zassert_true(dticks >= (int64_t)requested_ticks,
			     "woke early: %lld ticks, requested %u", (long long)dticks,
			     requested_ticks);
		zassert_true(dticks <= (int64_t)due_ticks + LATE_TOL_TICKS,
			     "woke %lld tick(s) late: %lld ticks, due %u",
			     (long long)(dticks - due_ticks), (long long)dticks, due_ticks);
		zassert_true(abs_diff_u64(lp_us, clock_us) <= DRIFT_TOL_US,
			     "kernel timer and LP timer differ by %lld us, "
			     "limit %llu us",
			     (long long)drift_us, (unsigned long long)DRIFT_TOL_US);
	}

	TC_BLANK();

	struct esp32_sleep_window win;
	uint32_t win_seq1 = esp32_sleep_stats_get(&win);

	TC_PRINT("  --- light sleep check ---\n");
	TC_PRINT("  windows completed            %u\n", win_seq1 - win_seq0);
	TC_PRINT("  last window slept / err      %u / 0x%x\n", win.slept, (unsigned int)win.err);
	TC_BLANK();

	zassert_true(standby_entries > entries_at_start, "light sleep (standby) was never entered");
	zassert_equal(standby_entries, standby_exits, "unbalanced standby entry/exit: %u/%u",
		      standby_entries, standby_exits);

	/* standby entries count skips too; confirm a window actually completed
	 * with a real, error-free sleep so a rejected/too-short sleep cannot pass.
	 */
	zassert_true(win_seq1 != win_seq0,
		     "no light sleep window completed (nothing really slept)");
	zassert_true(win.slept > 0, "window completed but nothing actually slept");
	zassert_equal(win.err, 0, "HAL sleep error 0x%x", (unsigned int)win.err);
}

#if SOC_PMU_SUPPORTED
static const char *pd_str(bool down)
{
	return down ? "OFF" : "ON ";
}

static void print_pd_line(const char *name, bool measured, bool expected,
			  const char *not_expected_reason)
{
	if (!expected) {
		TC_PRINT("  %-8s measured=%s  (%s)\n", name, pd_str(measured), not_expected_reason);
		return;
	}

	TC_PRINT("  %-8s measured=%s  expected=OFF  %s\n", name, pd_str(measured),
		 measured ? "OK" : "FAIL");
}
#endif

static void light_sleep_and_get_window(struct esp32_sleep_window *w)
{
	uint32_t dur_ms = sleep_duration_ms[ARRAY_SIZE(sleep_duration_ms) - 1];
	uint32_t seq0 = esp32_sleep_stats_get(NULL);

	k_sleep(K_MSEC(dur_ms));

	uint32_t seq1 = esp32_sleep_stats_get(w);

	zassert_true(seq1 != seq0, "no light sleep window was reported");
	zassert_true(w->slept > 0, "window reported but nothing actually slept");
	zassert_equal(w->err, 0, "HAL sleep error 0x%x", (unsigned int)w->err);
}

/*
 * Goal/Pass-if TC_PRINT can still be draining on the console after this thread
 * calls k_sleep. That delays the first idle entry by ~10 ms and shrinks the
 * driver sleep budget. Let the console finish while we stay awake.
 */
static void settle_console(void)
{
	k_msleep(50);
}

ZTEST(pm_sleep, test_wake_margin)
{
	struct esp32_sleep_window w;
	uint32_t dur_ms = sleep_duration_ms[ARRAY_SIZE(sleep_duration_ms) - 1];

	TC_BLANK();
	TC_PRINT("Goal: Verify that light sleep wakes at or before the programmed "
		 "deadline (not late).\n");
	settle_console();

	light_sleep_and_get_window(&w);

	TC_BLANK();
	TC_PRINT("  --- sleep window ---\n");
	TC_PRINT("  fragments:  %u  slept: %u  skipped: %u  err: 0x%x\n", w.fragments, w.slept,
		 w.skipped, (unsigned int)w.err);
	print_us_as_ms("app sleep request", (int64_t)dur_ms * USEC_PER_MSEC);
	print_us_as_ms("driver sleep budget", (int64_t)w.req_us);
	print_us_as_ms("time asleep (exec)", (int64_t)w.exec_us);
	print_us_as_ms("wake margin", w.wake_margin_us);
	TC_PRINT("  %-28s need -%lld .. 0 us (+late / -early)\n", "wake margin limit",
		 (long long)WAKE_EARLY_MAX_US);
	TC_BLANK();

	zassert_true(w.wake_margin_us <= 0, "woke %lld us AFTER the deadline (need <= 0)",
		     (long long)w.wake_margin_us);
	zassert_true(w.wake_margin_us >= -WAKE_EARLY_MAX_US,
		     "woke %lld us early, exceeds early budget %lld us",
		     (long long)-w.wake_margin_us, (long long)WAKE_EARLY_MAX_US);
}

ZTEST(pm_sleep, test_domain_power_down)
{
	/*
	 * Domain ON/OFF in esp32_sleep_window comes from PMU sleep_flags.
	 * Non-PMU SoCs (e.g. ESP32-C3) cannot report them, so Kconfig-based
	 * expectations would false-fail even when power down is configured.
	 */
#if !SOC_PMU_SUPPORTED
	ztest_test_skip();
#else
	struct esp32_sleep_window w;
	bool expect_top = IS_ENABLED(CONFIG_ESP32_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP);
	bool expect_flash = IS_ENABLED(CONFIG_ESP32_SLEEP_POWER_DOWN_FLASH);
	bool expect_cpu = IS_ENABLED(CONFIG_ESP32_PM_POWER_DOWN_CPU_IN_LIGHT_SLEEP);
	const char *top_reason = "power down not enabled";
	const char *flash_reason = IS_ENABLED(CONFIG_SOC_ESP32_PM_FLASH_KEEP_POWER_IN_LSLP)
					   ? "unsupported: SoC requires flash to stay ON"
					   : "power down not enabled";

#if defined(CONFIG_SOC_SERIES_ESP32C5)
	uint32_t chip_rev = efuse_hal_chip_revision();
	char top_reason_buf[64];

	/*
	 * Silicon / HAL gate: ESP32-C5 below rev 1.2 cannot power down TOP.
	 * Keep expect_top only when peri PD is on and the chip supports it.
	 */
	if (expect_top && !ESP_CHIP_REV_ABOVE(chip_rev, ESP32C5_TOP_PD_MIN_REV)) {
		expect_top = false;
		snprintk(top_reason_buf, sizeof(top_reason_buf),
			 "unsupported: chip rev %u; TOP PD needs >= %u / v1.2", chip_rev,
			 ESP32C5_TOP_PD_MIN_REV);
		top_reason = top_reason_buf;
	}
#endif

	TC_BLANK();
	TC_PRINT("Goal: Verify that power domains are switched off in light sleep.\n");
	settle_console();

	light_sleep_and_get_window(&w);

	TC_BLANK();
	TC_PRINT("  --- power domains ---\n");
	print_pd_line("TOP", w.top_down, expect_top, top_reason);
	print_pd_line("flash", w.flash_down, expect_flash, flash_reason);
	print_pd_line("CPU", w.cpu_down, expect_cpu, "power down not enabled");
	TC_BLANK();

	if (expect_top) {
		zassert_true(w.top_down, "TOP stayed ON");
	}
	if (expect_flash) {
		zassert_true(w.flash_down, "flash stayed ON");
	}
	if (expect_cpu) {
		zassert_true(w.cpu_down, "CPU stayed ON");
	}
#endif
}

static void *pm_sleep_setup(void)
{
	const struct device *const rtc = DEVICE_DT_GET(DT_NODELABEL(rtc_timer));

	zassert_true(device_is_ready(rtc), "rtc_timer device not ready");

	if (pm_device_wakeup_is_capable(rtc)) {
		zassert_true(pm_device_wakeup_enable(rtc, true),
			     "could not enable rtc_timer as wakeup source");
	}

	pm_notifier_register(&notifier);
	return NULL;
}

static void pm_sleep_teardown(void *fixture)
{
	ARG_UNUSED(fixture);
	pm_notifier_unregister(&notifier);
}

ZTEST_SUITE(pm_sleep, NULL, pm_sleep_setup, NULL, NULL, pm_sleep_teardown);
