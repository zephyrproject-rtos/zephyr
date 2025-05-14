/*
 * Copyright (c) 2024-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/pm.h>
#include <zephyr/irq.h>

#include <esp_cpu.h>
#include <esp_sleep.h>
#include <esp_timer_impl.h>
#include <esp_private/esp_clk.h>
#include <esp_private/systimer.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

#define MIN_RESIDENCY_SLEEP_US DT_PROP(DT_NODELABEL(light_sleep), min_residency_us)
#define WAKEUP_MARGIN_US       200

static const struct device *const rtc_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(rtc_timer));

static bool sleep_enabled;
static uint64_t sleep_time_us;

#if defined(CONFIG_XTENSA)
static uint32_t intenable;
#endif

static bool rtc_wakeup_enable(enum pm_state state, bool enable)
{
	bool wakeup_allowed;
	bool result = false;

	if (rtc_dev == NULL || !device_is_ready(rtc_dev)) {
		LOG_WRN("Sleep skipped. Make sure RTC counter driver is enabled.");
		return false;
	}

	if (sleep_time_us == 0) {
		return false;
	}

	/* Light sleep: DT-declared wakeup capability is sufficient.
	 * Deep sleep: wakeup must be explicitly enabled by PM policy.
	 */
	if (state == PM_STATE_STANDBY) {
		wakeup_allowed = pm_device_wakeup_is_capable(rtc_dev);
	} else {
		wakeup_allowed = pm_device_wakeup_is_enabled(rtc_dev);
	}

	if (!wakeup_allowed) {
		if (state == PM_STATE_STANDBY) {
			LOG_WRN("Sleep skipped. Make sure '&rtc_timer' is enabled as a wakeup "
				"source.");
		}
		return false;
	}

	if (enable) {
		/* Here we subtract a time margin from the sleep period to wake up before the
		 * systimer/ccount scheduler event. Since the RTC timer is used as a wakeup
		 * source instead of the systimer, the compensation must be applied based on
		 * the programmed wakeup time in order to resume execution just in time to
		 * execute the scheduler event. Because of this, we keep 'exit-latency-us' to
		 * a minimum in the device tree, and apply the actual compensation here. Also,
		 * HAL already compensates exit latency according to config/dynamic parameters,
		 * so we'll leave HAL to manage it.
		 */
		if (sleep_time_us >= (MIN_RESIDENCY_SLEEP_US + WAKEUP_MARGIN_US)) {
			esp_sleep_enable_timer_wakeup(sleep_time_us - WAKEUP_MARGIN_US);
			result = true;
		}
	} else {
		esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
	}

	return result;
}

/* PM hooks/callbacks */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_STANDBY:
#if defined(CONFIG_XTENSA)
		intenable = XTENSA_RSR("INTENABLE");
#endif
		/* Check if RTC is enabled and there's enough time for sleep.
		 * Sleep is skipped otherwise.
		 */
		sleep_enabled = rtc_wakeup_enable(state, true);

		if (sleep_enabled) {
			esp_light_sleep_start();
		}
		break;

	case PM_STATE_SOFT_OFF:
		rtc_wakeup_enable(state, true);
#if defined(SOC_PM_SUPPORT_RTC_PERIPH_PD)
		esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
#else
		esp_sleep_pd_config(ESP_PD_DOMAIN_XTAL, ESP_PD_OPTION_ON);
#endif
		esp_deep_sleep_start();
		break;

	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);

	switch (state) {
	case PM_STATE_STANDBY:
		if (sleep_enabled) {
			rtc_wakeup_enable(state, false);
		}

#if defined(CONFIG_RISCV)
		rv_utils_intr_global_enable();
		irq_unlock(0);
#elif defined(CONFIG_XTENSA)
		z_xt_ints_on(intenable);
		/* We don't have the key used to lock interruptions here.
		 * Just set PS.INTLEVEL to 0.
		 */
		__asm__ volatile("rsil a2, 0");
#endif
		break;

	default:
		LOG_DBG("Unsupported power state %u", state);
		break;
	}
}

#if defined(CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK)
uint64_t z_xtensa_lptim_hook_on_lpm_entry(uint64_t max_lpm_time_us)
#else
uint64_t esp32_lptim_hook_on_lpm_entry(uint64_t max_lpm_time_us)
#endif
{
	/* Since LPM state is not yet confirmed, we only program
	 * wakeup at pm_state_set() event.
	 */
	sleep_time_us = max_lpm_time_us;

	return esp_timer_impl_get_counter_reg();
}

#if defined(CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK)
uint64_t z_xtensa_lptim_hook_on_lpm_exit(void)
#else
uint64_t esp32_lptim_hook_on_lpm_exit(void)
#endif
{
	sleep_time_us = 0;

	return esp_timer_impl_get_counter_reg();
}

#if defined(CONFIG_XTENSA_TIMER_LPM_TIMER_HOOK)
uint64_t z_xtensa_lptim_hook_get_freq(void)
#else
uint64_t esp32_lptim_hook_get_freq(void)
#endif
{
#if defined(SOC_SYSTIMER_SUPPORTED)
	return systimer_us_to_ticks(1) * 1000000ULL;
#elif defined(CONFIG_ESP_TIMER_IMPL_TG0_LAC)
	return LACT_TICKS_PER_US * 1000000ULL;
#endif
}
