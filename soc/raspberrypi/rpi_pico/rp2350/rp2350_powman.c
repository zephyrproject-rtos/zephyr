/*
 * Copyright (c) 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rp2350_powman.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <hardware/structs/powman.h>
#include <hardware/powman.h>
#include <hardware/regs/powman.h>
#include <hardware/platform_defs.h>
#include <hardware/clocks.h>
#include <hardware/regs/clocks.h>

LOG_MODULE_REGISTER(rp2350_powman, CONFIG_SOC_LOG_LEVEL);

#define RP2350_PWRUP_GPIO0_BIT (1u << 1)
#define RP2350_PWRUP_ALARM_BIT (1u << 6)

#define RP2350_LPOSC_NOMINAL_HZ 32768u

/* Cached LPOSC frequency; 0 = not yet measured. */
static uint32_t lposc_freq_hz;

void rp2350_powman_timer_init(void)
{
	if (lposc_freq_hz == 0) {
		uint32_t khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_LPOSC_CLKSRC);

		if (khz == 0) {
			LOG_ERR("LPOSC frequency measured as 0; using %u Hz nominal",
				RP2350_LPOSC_NOMINAL_HZ);
			lposc_freq_hz = RP2350_LPOSC_NOMINAL_HZ;
		} else {
			lposc_freq_hz = khz * 1000u;
			LOG_INF("LPOSC calibrated: %u Hz (%u kHz)", lposc_freq_hz, khz);
		}
	}

	/* Only reprogram the tick source if it isn't already LPOSC, so repeated
	 * calls don't disturb a running timer.
	 */
	if (!(powman_hw->timer & POWMAN_TIMER_USING_LPOSC_BITS)) {
		powman_timer_set_1khz_tick_source_lposc_with_hz(lposc_freq_hz);
	}

	if (!powman_timer_is_running()) {
		powman_timer_start();
	}

	int64_t deadline = k_uptime_get() + 10;

	while (!(powman_hw->timer & POWMAN_TIMER_USING_LPOSC_BITS)) {
		if (k_uptime_get() >= deadline) {
			LOG_ERR("Timeout waiting for POWMAN LPOSC tick source, proceeding anyway");
			break;
		}
		k_usleep(100);
	}
}

void rp2350_powman_arm_alarm(uint32_t seconds)
{
	rp2350_powman_timer_init();

	uint64_t wake_at_ms = powman_timer_get_ms() + ((uint64_t)seconds * 1000u);

	powman_enable_alarm_wakeup_at_ms(wake_at_ms);
}

bool rp2350_powman_arm_gpio_wakeup(uint32_t gpio, bool wake_on_high)
{
	if (gpio >= NUM_BANK0_GPIOS) {
		return false;
	}

	/* Edge-detect: level mode blocks the power-down if the line rests at the wake level. */
	powman_enable_gpio_wakeup(0, gpio, true, wake_on_high);

	return true;
}

void rp2350_powman_disarm_wakeups(void)
{
	powman_disable_all_wakeups();
}

enum rp2350_pm_wakeup_source rp2350_powman_wakeup_source(void)
{
	uint32_t pwrup = powman_hw->last_swcore_pwrup;

	if (pwrup & RP2350_PWRUP_GPIO0_BIT) {
		return RP2350_PM_WAKEUP_GPIO;
	}
	if (pwrup & RP2350_PWRUP_ALARM_BIT) {
		return RP2350_PM_WAKEUP_ALARM;
	}

	return RP2350_PM_WAKEUP_NONE;
}

bool rp2350_powman_had_powerdown(void)
{
	return (powman_hw->chip_reset & POWMAN_CHIP_RESET_HAD_SWCORE_PD_BITS) != 0;
}
