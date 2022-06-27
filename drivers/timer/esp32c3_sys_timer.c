/*
 * Copyright (c) 2021 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <soc/soc_caps.h>
#include <soc/soc.h>
#include <soc/interrupt_core0_reg.h>
#include <soc/periph_defs.h>
#include <soc/system_reg.h>
#include <hal/systimer_hal.h>
#include <hal/systimer_ll.h>
#include <rom/ets_sys.h>
#include <esp_attr.h>

#include <zephyr/drivers/interrupt_controller/intc_esp32c3.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys_clock.h>
#include <soc.h>
#include <zephyr/device.h>

#define CYC_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec()	\
			      / (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_CYC 0xffffffffu
#define MAX_TICKS ((MAX_CYC - CYC_PER_TICK) / CYC_PER_TICK)
#define MIN_DELAY 1000

#define TICKLESS IS_ENABLED(CONFIG_TICKLESS_KERNEL)

static struct k_spinlock lock;
static uint64_t last_count;

static void set_systimer_alarm(uint64_t time)
{
	systimer_hal_select_alarm_mode(SYSTIMER_ALARM_0, SYSTIMER_ALARM_MODE_ONESHOT);
	systimer_hal_set_alarm_target(SYSTIMER_ALARM_0, time);
	systimer_hal_enable_alarm_int(SYSTIMER_ALARM_0);
}

static uint64_t systimer_alarm(void)
{
	return systimer_hal_get_time(SYSTIMER_COUNTER_1);
}

static void sys_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);
	systimer_ll_clear_alarm_int(SYSTIMER_ALARM_0);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = systimer_alarm();

	uint32_t dticks = (uint32_t)((now - last_count) / CYC_PER_TICK);

	last_count = now;

	if (!TICKLESS) {
		uint64_t next = last_count + CYC_PER_TICK;

		if ((int64_t)(next - now) < MIN_DELAY) {
			next += CYC_PER_TICK;
		}
		set_systimer_alarm(next);
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? dticks : 1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

#if defined(CONFIG_TICKLESS_KERNEL)
	ticks = ticks == K_TICKS_FOREVER ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t)MAX_TICKS);

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint64_t now = systimer_alarm();
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
	uint32_t ret = ((uint32_t)systimer_alarm() - (uint32_t)last_count) / CYC_PER_TICK;

	k_spin_unlock(&lock, key);
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)systimer_alarm();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return systimer_alarm();
}

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	esp_intr_alloc(DT_IRQN(DT_NODELABEL(systimer0)),
		0,
		sys_timer_isr,
		NULL,
		NULL);

	systimer_hal_init();
	systimer_hal_connect_alarm_counter(SYSTIMER_ALARM_0, SYSTIMER_COUNTER_1);
	systimer_hal_enable_counter(SYSTIMER_COUNTER_1);
	systimer_hal_counter_can_stall_by_cpu(SYSTIMER_COUNTER_1, 0, true);
	last_count = systimer_alarm();
	set_systimer_alarm(last_count + CYC_PER_TICK);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
