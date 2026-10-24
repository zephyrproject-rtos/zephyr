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
#include <esp_cpu.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/spinlock.h>

#include "esp32_sys_timer.h"

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

/*
 * SYSTIMER_COUNTER_OS_TICK is one free-running counter genuinely shared by
 * both cores, but each core has its own alarm/comparator channel watching
 * it (SYSTIMER_ALARM_OS_TICK_CORE0/1) and its own interrupt, so that core0
 * arming its deadline never disturbs core1's. Under CONFIG_SMP, every
 * function here that arms or acknowledges "the" alarm must target the
 * calling core's own channel rather than hardcoding core0's -- this is
 * what lets the exact same system_timer_generic.h tick accounting work
 * unmodified from either core.
 */
static inline uint32_t core_alarm_id(void)
{
	if (IS_ENABLED(CONFIG_SMP)) {
		return SYSTIMER_ALARM_OS_TICK_CORE0 + esp_cpu_get_core_id();
	}

	return SYSTIMER_ALARM_OS_TICK_CORE0;
}

static void set_systimer_alarm(uint32_t alarm_id, uint64_t time)
{
	systimer_hal_select_alarm_mode(&systimer_hal, alarm_id, SYSTIMER_ALARM_MODE_ONESHOT);

	systimer_counter_value_t alarm = {.val = time};

	systimer_ll_enable_alarm(systimer_hal.dev, alarm_id, false);
	systimer_ll_set_alarm_target(systimer_hal.dev, alarm_id, alarm.val);
	systimer_ll_apply_alarm_value(systimer_hal.dev, alarm_id);
	systimer_ll_enable_alarm(systimer_hal.dev, alarm_id, true);
	systimer_ll_enable_alarm_int(systimer_hal.dev, alarm_id, true);
}

static uint64_t get_systimer_alarm(void)
{
	return systimer_hal_get_counter_value(&systimer_hal, SYSTIMER_COUNTER_OS_TICK);
}

/* Counter value sampled by sys_clock_disable(), reported by the read below
 * once the hardware is gone.
 */
static uint64_t stopped_cycles;

/*
 * A free-running counter and a one-shot alarm: a COMPARE_ORDERED backend. The
 * counter is 52 bits wide, read back as a 64-bit value. Every SoC this driver
 * builds for defines SYSTIMER_LL_ALARM_MISS_COMPENSATE, so an alarm armed at or
 * behind the counter is raised at once instead of being lost, and the target
 * needs no minimum distance.
 */
#define TIMER_CORE_BACKEND_COMPARE_ORDERED
#define TIMER_CORE_COUNTER_WIDTH 52

static inline uint64_t timer_driver_cycle_get(void)
{
	/* sys_clock_disable() deinitializes the HAL and clears dev; a stray core
	 * read afterwards must not dereference it. Report the value the counter
	 * stopped at, so the elapsed span holds instead of stepping backwards.
	 */
	if (systimer_hal.dev == NULL) {
		return stopped_cycles;
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

	set_systimer_alarm(core_alarm_id(), cycles);
}

#include "system_timer_generic.h"

static void IRAM_ATTR sys_timer_isr(void *arg)
{
	ARG_UNUSED(arg);
	systimer_ll_clear_alarm_int(systimer_hal.dev, core_alarm_id());

	timer_core_announce();
}

void sys_clock_disable(void)
{
	stopped_cycles = get_systimer_alarm();

	systimer_ll_enable_alarm(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, false);
	systimer_ll_enable_alarm_int(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, false);
	systimer_hal_deinit(&systimer_hal);
}

#if defined(CONFIG_PM)
/*
 * systimer_pre_idle/lptim_pre_idle/timeout_idle are plain statics, not
 * per-core: fine for every existing single-core user of this driver, but
 * unaudited for two cores independently entering/exiting idle at once under
 * CONFIG_SMP (this driver's first). Neither of today's SMP+PM combinations
 * has been tested; CONFIG_PM stayed off for the SMP validation this change
 * is based on.
 */
void sys_clock_idle_enter(uint32_t ticks)
{
	sys_clock_set_timeout(ticks, false);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL) || systimer_hal.dev == NULL) {
		return;
	}

	uint64_t timeout_us = k_ticks_to_us_ceil64(ticks);

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

#if defined(CONFIG_SMP)
/*
 * Called once on each secondary CPU from smp_init_top() (see kernel/smp/
 * smp.c). CORE1's alarm channel still watches the same SYSTIMER_COUNTER_OS_TICK
 * core0's does -- that is the whole point of this driver over the CCOUNT-based
 * one -- so this only needs to connect and enable core1's own channel and
 * interrupt, then arm it one tick ahead of the shared baseline exactly like
 * sys_clock_driver_init() did for core0. esp_intr_alloc() routes an interrupt
 * source to whichever CPU calls it, so core1's own allocation has to happen
 * from core1 itself.
 */
void smp_timer_init(void)
{
	int ret;

	ret = esp_intr_alloc(
		DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 1, irq),
		ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 1, priority)) |
			ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 1, flags)) |
			ESP_INTR_FLAG_IRAM,
		sys_timer_isr, NULL, NULL);
	__ASSERT(ret == 0, "systimer core1 IRQ allocation failed: %d", ret);

	systimer_hal_connect_alarm_counter(&systimer_hal,
		SYSTIMER_ALARM_OS_TICK_CORE1, SYSTIMER_COUNTER_OS_TICK);

	timer_core_smp_prime();
}
#endif /* CONFIG_SMP */

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
