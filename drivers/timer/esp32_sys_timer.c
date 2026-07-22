/*
 * Copyright (c) 2021-2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc/soc_caps.h>
#include <soc/soc.h>

#include <hal/systimer_hal.h>
#include <hal/systimer_ll.h>
#include <esp_private/systimer.h>
#include <rom/ets_sys.h>
#include <esp_attr.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/spinlock.h>

#include "esp32_sys_timer.h"

#define MIN_DELAY 1

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(DT_NODELABEL(systimer0));
#endif

#if defined(CONFIG_PM)
static uint64_t systimer_pre_idle;
static uint64_t lptim_pre_idle;
static bool timeout_idle;
#endif

/* Systimer HAL layer object */
static systimer_hal_context_t systimer_hal;

static void set_systimer_alarm(uint64_t time)
{
	systimer_hal_select_alarm_mode(&systimer_hal,
		SYSTIMER_ALARM_OS_TICK_CORE0, SYSTIMER_ALARM_MODE_ONESHOT);

	systimer_counter_value_t alarm = {.val = time};

	systimer_ll_enable_alarm(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, false);
	systimer_ll_set_alarm_target(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, alarm.val);
	systimer_ll_apply_alarm_value(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0);
	systimer_ll_enable_alarm(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, true);
	systimer_ll_enable_alarm_int(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, true);
}

static uint64_t get_systimer_alarm(void)
{
	return systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_OS_TICK);
}

/*
 * Free-running 64-bit systimer counter plus a one-shot alarm: a COMPARE
 * backend. The core owns the tick accounting; the driver reads the counter and
 * arms the alarm. timer_driver_set_compare() keeps a target that is too close (or
 * behind after latency) at least MIN_DELAY ahead so the alarm is not missed.
 */
#define TIMER_CORE_BACKEND_COMPARE
#define TIMER_CORE_64BIT_CYCLES
/*
 * The systimer unit counter is 52-bit, read back as a 64-bit value. Declare its
 * real width: the default native register width would be 32 bits on the RISC-V
 * parts, masking announce deltas to 32 bits and truncating a light-sleep
 * compensation longer than 2^32 cycles (~268 s at 16 MHz) in sys_clock_idle_exit();
 * 52 also keeps the masked delta wrap-correct at the counter's true wrap.
 */
#define TIMER_CORE_CYCLES_WIDTH 52

static inline uint64_t timer_driver_cycle_get(void)
{
	/* sys_clock_disable() deinitializes the HAL and clears dev; a stray core
	 * read afterwards must not dereference it.
	 */
	if (systimer_hal.dev == NULL) {
		return 0;
	}
	return get_systimer_alarm();
}

static void timer_driver_set_compare(uint64_t cycles)
{
	/* Same post-sys_clock_disable() guard as timer_driver_cycle_get(): once
	 * the HAL is deinitialized and dev cleared, a stray arm must not
	 * dereference it.
	 */
	if (systimer_hal.dev == NULL) {
		return;
	}

	uint64_t curr = get_systimer_alarm();

	if ((int64_t)(cycles - curr) < MIN_DELAY) {
		cycles = curr + MIN_DELAY;
	}
	set_systimer_alarm(cycles);
}

#include "system_timer_generic.h"

static void IRAM_ATTR sys_timer_isr(void *arg)
{
	ARG_UNUSED(arg);
	systimer_ll_clear_alarm_int(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0);

	timer_core_announce();
}

void sys_clock_disable(void)
{
	systimer_ll_enable_alarm(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, false);
	systimer_ll_enable_alarm_int(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, false);
	systimer_hal_deinit(&systimer_hal);
}

#if defined(CONFIG_PM)
void sys_clock_idle_enter(uint32_t ticks)
{
	sys_clock_set_timeout(ticks);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) || systimer_hal.dev == NULL) {
		return;
	}

	/* The deadline is tick-aligned but the LP timer starts counting here,
	 * mid-tick, so a period of the full `ticks` would wake up to one tick
	 * after the deadline; in deep sleep the LP timer is the only wake
	 * source, so that is a timeout served late. Shorten the period by one
	 * tick so the wake lands at or before the deadline instead; the floor
	 * conversion rounds the same safe way.
	 */
	uint32_t lp_ticks = (ticks > 0U) ? (ticks - 1U) : 0U;
	uint64_t timeout_us = k_ticks_to_us_floor64(lp_ticks);

	lptim_pre_idle = esp32_lptim_hook_on_lpm_entry(timeout_us);
	systimer_pre_idle = get_systimer_alarm();
	timeout_idle = true;
}

void sys_clock_idle_exit(void)
{
	if (!timeout_idle) {
		return;
	}

	k_spinlock_key_t key = sys_clock_lock();

	uint64_t lptim_now = esp32_lptim_hook_on_lpm_exit();
	uint64_t systimer_now = get_systimer_alarm();
	uint64_t lptim_diff = lptim_now - lptim_pre_idle;
	uint64_t systimer_diff = systimer_now - systimer_pre_idle;
	uint64_t expected_cycles =
		(lptim_diff * sys_clock_hw_cycles_per_sec()) / esp32_lptim_hook_get_freq();
	uint64_t missed_cycles = 0;

	if (expected_cycles > systimer_diff) {
		missed_cycles = expected_cycles - systimer_diff;
	}

	uint64_t new_value =
		systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_OS_TICK) +
		missed_cycles;

	/* Compensate systimer missed cycles while in sleep */
	systimer_ll_set_counter_value(systimer_hal.dev, SYSTIMER_COUNTER_OS_TICK, new_value);
	systimer_ll_apply_counter_value(systimer_hal.dev, SYSTIMER_COUNTER_OS_TICK);

	timeout_idle = false;

	/* Announce OS ticks as systimer remained stalled while in light sleep */
	timer_core_announce_from(key);
}
#endif

static int sys_clock_driver_init(void)
{
	int ret;

	ret = esp_intr_alloc(
		DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 0, irq),
		ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 0, priority)) |
			ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 0, flags)) |
			ESP_INTR_FLAG_IRAM,
		sys_timer_isr, NULL, NULL);

	if (ret != 0) {
		return ret;
	}

	systimer_hal_init(&systimer_hal);
	systimer_hal_connect_alarm_counter(&systimer_hal,
		SYSTIMER_ALARM_OS_TICK_CORE0, SYSTIMER_COUNTER_OS_TICK);

	systimer_hal_enable_counter(&systimer_hal, SYSTIMER_COUNTER_OS_TICK);
	systimer_hal_counter_can_stall_by_cpu(&systimer_hal, SYSTIMER_COUNTER_OS_TICK, 0, true);
#if defined(CONFIG_SMP)
	systimer_hal_counter_can_stall_by_cpu(&systimer_hal, SYSTIMER_COUNTER_OS_TICK, 1, true);
#endif

	/* Seed the announce baseline from the systimer and arm the first tick. */
	timer_core_init();
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
