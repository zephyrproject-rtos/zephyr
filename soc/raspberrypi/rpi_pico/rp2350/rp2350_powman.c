/*
 * SPDX-FileCopyrightText: 2026 Gabriel Germano
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rp2350_powman.h"

#include <zephyr/devicetree.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <hardware/structs/powman.h>
#include <hardware/powman.h>
#include <hardware/regs/powman.h>
#include <hardware/platform_defs.h>
#include <hardware/clocks.h>
#include <hardware/regs/clocks.h>

LOG_MODULE_REGISTER(rp2350_powman, CONFIG_SOC_LOG_LEVEL);

/* POWMAN_LAST_SWCORE_PWRUP is a 0-6 enum (1=pwrup0, 6=alarm_pwrup), not a bitmask.
 * https://github.com/zephyrproject-rtos/hal_rpi_pico/blob/266394b0f23dff8f77c54f67f89d62cee246f21d/src/rp2350/hardware_regs/include/hardware/regs/powman.h#L1842-L1859
 */
#define RP2350_LAST_SWCORE_PWRUP_GPIO0 1u
#define RP2350_LAST_SWCORE_PWRUP_ALARM 6u

#define RP2350_LPOSC_NOMINAL_HZ 32768u

#define POWMAN_NODE DT_NODELABEL(powman)

/* Cached LPOSC frequency; 0 = not yet measured. */
static uint32_t lposc_freq_hz;

static void rp2350_powman_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* The alarm fires whenever time >= target, so it re-asserts until it is
	 * disabled; clearing alone would leave the interrupt latched again.
	 */
	powman_timer_disable_alarm();
	powman_clear_alarm();
}

void rp2350_powman_timer_init(void)
{
	/* In light sleep the core stays powered and SysTick is held in reset by
	 * SYSTEM_TIMER_RESET_BY_LPM, leaving the alarm as the only wake source.
	 * powman_enable_alarm_wakeup_at_ms() enables it at the peripheral only,
	 * so without the NVIC line WFI would never be woken by it.
	 */
	IRQ_CONNECT(DT_IRQ_BY_NAME(POWMAN_NODE, timer, irq),
		    DT_IRQ_BY_NAME(POWMAN_NODE, timer, priority), rp2350_powman_isr, NULL, 0);
	irq_enable(DT_IRQ_BY_NAME(POWMAN_NODE, timer, irq));

	if (lposc_freq_hz == 0) {
		uint32_t khz = frequency_count_khz(CLOCKS_FC0_SRC_VALUE_LPOSC_CLKSRC);

		if (khz != 0) {
			lposc_freq_hz = khz * 1000u;
			LOG_INF("LPOSC calibrated: %u Hz (%u kHz)", lposc_freq_hz, khz);
		} else {
			lposc_freq_hz = powman_timer_get_lposc_calib_freq();
			if (lposc_freq_hz != 0) {
				LOG_WRN("LPOSC frequency counter failed; using OTP "
					"factory calibration: %u Hz", lposc_freq_hz);
			} else {
				LOG_ERR("LPOSC frequency measured as 0 and OTP calibration "
					"unavailable; using %u Hz nominal",
					RP2350_LPOSC_NOMINAL_HZ);
				lposc_freq_hz = RP2350_LPOSC_NOMINAL_HZ;
			}
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

	/* Called with the system clock lock held, so k_usleep() would deadlock. */
	WAIT_FOR(powman_hw->timer & POWMAN_TIMER_USING_LPOSC_BITS, 10000, k_busy_wait(100));

	if (!(powman_hw->timer & POWMAN_TIMER_USING_LPOSC_BITS)) {
		LOG_ERR("Timeout waiting for POWMAN LPOSC tick source, proceeding anyway");
	}
}

/* App-armed deadline, tracked so z_sys_clock_lpm_enter() can merge it with
 * its own instead of one overwriting the other in the shared alarm register.
 */
static uint64_t app_alarm_ms;
static bool app_alarm_pending;

void rp2350_powman_arm_alarm(uint32_t seconds)
{
	rp2350_powman_timer_init();

	app_alarm_ms = powman_timer_get_ms() + ((uint64_t)seconds * MSEC_PER_SEC);
	app_alarm_pending = true;

	powman_enable_alarm_wakeup_at_ms(app_alarm_ms);
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
	app_alarm_pending = false;
	powman_disable_all_wakeups();
}

enum rp2350_pm_wakeup_source rp2350_powman_wakeup_source(void)
{
	uint32_t pwrup = powman_hw->last_swcore_pwrup;

	if (pwrup == RP2350_LAST_SWCORE_PWRUP_GPIO0) {
		return RP2350_PM_WAKEUP_GPIO;
	}
	if (pwrup == RP2350_LAST_SWCORE_PWRUP_ALARM) {
		return RP2350_PM_WAKEUP_ALARM;
	}

	return RP2350_PM_WAKEUP_NONE;
}

bool rp2350_powman_had_powerdown(void)
{
	return (powman_hw->chip_reset & POWMAN_CHIP_RESET_HAD_SWCORE_PD_BITS) != 0;
}

#if defined(CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS)
#include <zephyr/drivers/timer/system_timer_lpm.h>
#include <zephyr/sys/clock.h>
#include <zephyr/sys/util.h>

/* POWMAN timer value (ms) captured on entry; used to report elapsed time on exit. */
static uint64_t lpm_enter_ms;

void z_sys_clock_lpm_enter(uint64_t max_lpm_time_us)
{
	uint64_t deadline_ms;

	rp2350_powman_timer_init();

	lpm_enter_ms = powman_timer_get_ms();

	/* POWMAN's alarm is millisecond-granular; round down so we never wake later
	 * than max_lpm_time_us.
	 */
	deadline_ms = lpm_enter_ms + (max_lpm_time_us / USEC_PER_MSEC);

	/* Arm whichever deadline is sooner, ours or a still-pending app alarm. */
	if (app_alarm_pending && app_alarm_ms < deadline_ms) {
		deadline_ms = app_alarm_ms;
	}

	powman_enable_alarm_wakeup_at_ms(deadline_ms);
}

uint64_t z_sys_clock_lpm_exit(void)
{
	uint64_t now_ms;
	uint64_t elapsed_us;

	now_ms = powman_timer_get_ms();
	elapsed_us = (now_ms - lpm_enter_ms) * USEC_PER_MSEC;

	if (app_alarm_pending && now_ms >= app_alarm_ms) {
		app_alarm_pending = false;
	}

	/* The alarm condition latches until disabled; left set, it would block
	 * the next powman_set_power_state() call.
	 */
	powman_disable_alarm_wakeup();

	return elapsed_us;
}
#endif /* CONFIG_SYSTEM_TIMER_LPM_COMPANION_HOOKS */
