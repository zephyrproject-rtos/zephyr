/*
 * Copyright (c) 2021-2025 Espressif Systems (Shanghai) Co., Ltd.
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

#define CYC_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec()	\
			      / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_CYC 0xffffffffu
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = DT_IRQN(DT_NODELABEL(systimer0));
#endif

#define TICKLESS IS_ENABLED(CONFIG_TICKLESS_KERNEL)

static struct k_spinlock lock;
static uint64_t last_count;

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

static void sys_timer_isr(void *arg)
{
	ARG_UNUSED(arg);
	systimer_ll_clear_alarm_int(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = get_systimer_alarm();

	uint64_t dticks = (uint64_t)((now - last_count) / CYC_PER_TICK);

	last_count += dticks * CYC_PER_TICK;

	if (!TICKLESS) {
		uint64_t next = last_count + CYC_PER_TICK;

		if ((int64_t)(next - now) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_systimer_alarm(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(dticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = get_systimer_alarm();
	uint32_t adj, cyc = ticks * CYC_PER_TICK;

	/* Round up to next tick boundary. */
	adj = (uint32_t)(now - last_count) + (CYC_PER_TICK - 1);
	if (cyc <= MAX_CYC - adj) {
		cyc += adj;
	} else {
		cyc = MAX_CYC;
	}
	cyc = (cyc / CYC_PER_TICK) * CYC_PER_TICK;

	if ((int32_t)(cyc + last_count - now) < MIN_DELAY) {
		cyc += CYC_PER_TICK;
	}

	set_systimer_alarm(cyc + last_count);
	k_spin_unlock(&lock, key);
#endif
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t ret = ((uint32_t)get_systimer_alarm() - (uint32_t)last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)get_systimer_alarm();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return get_systimer_alarm();
}

void sys_clock_disable(void)
{
	systimer_ll_enable_alarm(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, false);
	systimer_ll_enable_alarm_int(systimer_hal.dev, SYSTIMER_ALARM_OS_TICK_CORE0, false);
	systimer_hal_deinit(&systimer_hal);
}

static int sys_clock_driver_init(void)
{
	int ret;

	ret = esp_intr_alloc(DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 0, irq),
		ESP_PRIO_TO_FLAGS(DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 0, priority)) |
		ESP_INT_FLAGS_CHECK(DT_IRQ_BY_IDX(DT_NODELABEL(systimer0), 0, flags)),
		sys_timer_isr,
		NULL,
		NULL);

	if (ret != 0) {
		return ret;
	}

	systimer_hal_init(&systimer_hal);
	systimer_hal_connect_alarm_counter(&systimer_hal,
		SYSTIMER_ALARM_OS_TICK_CORE0, SYSTIMER_COUNTER_OS_TICK);

	systimer_hal_enable_counter(&systimer_hal, SYSTIMER_COUNTER_OS_TICK);
	systimer_hal_counter_can_stall_by_cpu(&systimer_hal, SYSTIMER_COUNTER_OS_TICK, 0, true);
	last_count = get_systimer_alarm();
	set_systimer_alarm(last_count + CYC_PER_TICK);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
