/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Light sleep statistics: one log line per completed sleep window.
 *
 * A single k_sleep can be split into several sleeps when events (such as radio
 * wakes) interrupt it. All fragments share the same wake deadline, so we group
 * them by deadline and report one summary line per window instead of one line
 * per fragment.
 *
 * A window is reported only when it completes: a fragment sleeps to within the
 * sleep floor of its deadline, so no room is left for another fragment and the
 * wait is effectively over. This prints at the window's own end. The tiny residue
 * before the deadline is discarded, which does not matter for a sleep check.
 */

#include <stdio.h>
#include <string.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/clock.h>
#include <zephyr/logging/log.h>

#include <esp_timer.h>
#include <esp_private/esp_sleep_internal.h>
#if SOC_PMU_SUPPORTED
#include <esp_private/esp_pmu.h>
#endif

#include <pmstats.h>

LOG_MODULE_DECLARE(soc_pm, CONFIG_SOC_LOG_LEVEL);

static esp_sleep_context_t sleep_ctx;

/* ls_*: prefix for symbols related to a light sleep window */
static bool ls_window_open;         /* true while fragments share one deadline */
static int64_t ls_enter_us;         /* esp_timer us at last fragment entry */
static int64_t ls_deadline_tick;    /* kernel tick of this window's wake */
static uint64_t ls_deadline_abs_us; /* same wake, absolute esp_timer us */
static uint64_t ls_floor_us;        /* min residency; below this, window closes */
static struct {
	uint32_t fragments;
	uint32_t slept;
	uint32_t skipped;
	uint64_t req_us;
	uint64_t exec_us;
	int64_t wake_margin_us;
	uint32_t pd_and;
	int32_t err;
} ls_window;

static struct esp32_sleep_window ls_last;
static uint32_t ls_seq;
static struct k_spinlock ls_lock;

static void sleep_stats_print(void);

void esp32_sleep_stats_before(uint64_t sleep_time_us, uint64_t min_sleep_us,
			      uint64_t deadline_abs_us)
{
	int64_t rem_ticks =
		(int64_t)((sleep_time_us * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / USEC_PER_SEC);
	int64_t deadline = sys_clock_tick_get() + rem_ticks;
	int64_t tick_delta = deadline - ls_deadline_tick;
	bool same_window = ls_window_open && (tick_delta <= 1 && tick_delta >= -1);

	/* A different deadline starts a new wait. Any window still open here never
	 * reached its deadline (the wait ended early), so drop it unreported and
	 * begin a fresh window.
	 */
	if (!same_window) {
		memset(&ls_window, 0, sizeof(ls_window));
		ls_window.pd_and = UINT32_MAX;
		ls_window.req_us = sleep_time_us;
		ls_deadline_tick = deadline;
		ls_window_open = true;
	}

	ls_floor_us = min_sleep_us;
	ls_deadline_abs_us = deadline_abs_us;
	ls_enter_us = esp_timer_get_time();
}

void esp32_sleep_stats_after(void)
{
	int64_t now = esp_timer_get_time();
	int64_t elapsed_us = now - ls_enter_us;
	int32_t hal_err = sleep_ctx.sleep_request_result;

	ls_window.fragments++;

	/* Wake vs deadline: positive = late, negative = early. */
	ls_window.wake_margin_us = now - (int64_t)ls_deadline_abs_us;

	/* Reject or too-short means we did not actually sleep, so neither its
	 * duration nor its flags reflect a real power down. Only count time and
	 * power down state from real sleeps.
	 */
	if (hal_err == 0) {
		ls_window.slept++;
		ls_window.pd_and &= sleep_ctx.sleep_flags;
		if (elapsed_us > 0) {
			ls_window.exec_us += (uint64_t)elapsed_us;
		}
	} else if (hal_err == ESP_ERR_SLEEP_REJECT ||
		   hal_err == ESP_ERR_SLEEP_TOO_SHORT_SLEEP_DURATION) {
		ls_window.skipped++;
	} else {
		ls_window.err = hal_err;
	}

	/* If less than the sleep floor remains until the absolute deadline, no
	 * further fragment can sleep, so the wait is done: print and close.
	 */
	if ((int64_t)ls_deadline_abs_us - now < (int64_t)ls_floor_us) {
		sleep_stats_print();
	}
}

#if CONFIG_SOC_LOG_LEVEL >= LOG_LEVEL_DBG
#if SOC_PMU_SUPPORTED
/* Build string with why TOP stayed on. TOP can only power down when the domains
 * under it do, so the reason is either that power down was never enabled
 * ("config"), or the names of the domains that stayed on (e.g. "modem").
 * "?" means something else held it, such as a clock gate, with no flag of its
 * own in the sleep mask.
 */
static void sleep_stats_top_reason(char *buf, size_t len)
{
	size_t n = 0;

	if (!IS_ENABLED(CONFIG_ESP32_PM_POWER_DOWN_PERIPHERAL_IN_LIGHT_SLEEP)) {
		snprintf(buf, len, "config");
		return;
	}

	buf[0] = '\0';
#if SOC_PM_SUPPORT_MODEM_PD
	if (!(ls_window.pd_and & PMU_SLEEP_PD_MODEM) && n < len) {
		n += snprintf(buf + n, len - n, "%smodem", n ? ", " : "");
	}
#endif
	if (!(ls_window.pd_and & PMU_SLEEP_PD_HP_PERIPH) && n < len) {
		n += snprintf(buf + n, len - n, "%speriph", n ? ", " : "");
	}
	if (!(ls_window.pd_and & PMU_SLEEP_PD_CPU) && n < len) {
		n += snprintf(buf + n, len - n, "%scpu", n ? ", " : "");
	}
	if (!(ls_window.pd_and & PMU_SLEEP_PD_XTAL) && n < len) {
		n += snprintf(buf + n, len - n, "%sxtal", n ? ", " : "");
	}

	if (n == 0) {
		snprintf(buf, len, "?");
	}
}

/* "config" if flash PD is off; "hal" if we asked and HAL kept flash on. */
static const char *sleep_stats_flash_reason(void)
{
	return IS_ENABLED(CONFIG_ESP32_SLEEP_POWER_DOWN_FLASH) ? "hal" : "config";
}
#endif

/* Append the power-domain tags to the log line. "OFF" means the domain powered
 * down in every sleep of the window, "ON" means it stayed on at least once. We
 * report the state only; whether a domain should power down is up to the
 * application (an active radio keeps TOP on, for example). When a domain stays
 * on we add the reason in parentheses, see sleep_stats_top_reason() and
 * sleep_stats_flash_reason().
 */
static void sleep_stats_pd_tags(char *buf, size_t len)
{
	buf[0] = '\0';

#if SOC_PMU_SUPPORTED
	if (ls_window.slept == 0) {
		return;
	}

	char flash[24];
	char reason[32];

	if (ls_window.pd_and & PMU_SLEEP_PD_VDDSDIO) {
		snprintf(flash, sizeof(flash), "OFF");
	} else {
		snprintf(flash, sizeof(flash), "ON (%s)", sleep_stats_flash_reason());
	}

	if (ls_window.pd_and & PMU_SLEEP_PD_TOP) {
		snprintf(buf, len, " top=OFF flash=%s", flash);
		return;
	}

	sleep_stats_top_reason(reason, sizeof(reason));
	snprintf(buf, len, " top=ON (%s) flash=%s", reason, flash);
#else
	ARG_UNUSED(len);
#endif
}

static void sleep_stats_result(char *buf, size_t len)
{
	if (ls_window.err != 0) {
		snprintf(buf, len, "Sleep NOK -> err 0x%x", (unsigned int)ls_window.err);
		return;
	}

	if (ls_window.slept == 0) {
		snprintf(buf, len, "Sleep skipped");
		return;
	}

	snprintf(buf, len, "Sleep OK");
}
#endif /* CONFIG_SOC_LOG_LEVEL >= LOG_LEVEL_DBG */

static void sleep_stats_print(void)
{
	if (!ls_window_open) {
		return;
	}

	ls_window_open = false;

#if CONFIG_SOC_LOG_LEVEL >= LOG_LEVEL_DBG
	static char pd_tags[80];
	static char result[40];
	uint32_t eff_pct =
		ls_window.req_us ? (uint32_t)((ls_window.exec_us * 100) / ls_window.req_us) : 0;
	uint32_t pd_flags = ls_window.slept ? ls_window.pd_and : 0;

	sleep_stats_pd_tags(pd_tags, sizeof(pd_tags));
	sleep_stats_result(result, sizeof(result));

	LOG_DBG("sleep: n=%u slept=%u skip=%u req=%lluus exec=%lluus margin=%lldus "
		"eff=%u%% pd=0x%08x%s [%s]",
		ls_window.fragments, ls_window.slept, ls_window.skipped,
		(unsigned long long)ls_window.req_us, (unsigned long long)ls_window.exec_us,
		(long long)ls_window.wake_margin_us, eff_pct, pd_flags, pd_tags, result);
#endif

	struct esp32_sleep_window completed = {
		.fragments = ls_window.fragments,
		.slept = ls_window.slept,
		.skipped = ls_window.skipped,
		.req_us = ls_window.req_us,
		.exec_us = ls_window.exec_us,
		.wake_margin_us = ls_window.wake_margin_us,
		.err = ls_window.err,
	};

#if SOC_PMU_SUPPORTED
	if (ls_window.slept) {
		completed.top_down = (ls_window.pd_and & PMU_SLEEP_PD_TOP) != 0;
		completed.flash_down = (ls_window.pd_and & PMU_SLEEP_PD_VDDSDIO) != 0;
		completed.cpu_down = (ls_window.pd_and & PMU_SLEEP_PD_CPU) != 0;
	}
#endif

	k_spinlock_key_t key = k_spin_lock(&ls_lock);

	ls_last = completed;
	ls_seq++;
	k_spin_unlock(&ls_lock, key);
}

uint32_t esp32_sleep_stats_get(struct esp32_sleep_window *out)
{
	k_spinlock_key_t key = k_spin_lock(&ls_lock);
	uint32_t seq = ls_seq;

	if (out != NULL) {
		*out = ls_last;
	}
	k_spin_unlock(&ls_lock, key);

	return seq;
}

static int esp32_sleep_stats_init(void)
{
	esp_sleep_set_sleep_context(&sleep_ctx);
	return 0;
}

SYS_INIT(esp32_sleep_stats_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
