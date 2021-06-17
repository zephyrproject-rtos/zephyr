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

#include <drivers/timer/system_timer.h>
#include <sys_clock.h>
#include <soc.h>

#define SYS_TIMER_CPU_IRQ               1

static void sys_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);
	systimer_ll_clear_alarm_int(SYSTIMER_ALARM_0);
	sys_clock_announce(1);
}

int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	esp_rom_intr_matrix_set(0,
		ETS_SYSTIMER_TARGET0_EDGE_INTR_SOURCE,
		SYS_TIMER_CPU_IRQ);
	IRQ_CONNECT(SYS_TIMER_CPU_IRQ, 0, sys_timer_isr, NULL, 0);
	irq_enable(SYS_TIMER_CPU_IRQ);

	systimer_hal_init();
	systimer_hal_connect_alarm_counter(SYSTIMER_ALARM_0, SYSTIMER_COUNTER_1);
	systimer_hal_enable_counter(SYSTIMER_COUNTER_1);
	systimer_hal_counter_can_stall_by_cpu(SYSTIMER_COUNTER_1, 0, true);
	systimer_hal_set_alarm_period(SYSTIMER_ALARM_0,
		CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	systimer_hal_select_alarm_mode(SYSTIMER_ALARM_0, SYSTIMER_ALARM_MODE_PERIOD);
	systimer_hal_enable_alarm_int(SYSTIMER_ALARM_0);

	return 0;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
	ARG_UNUSED(ticks);
}

uint32_t sys_clock_elapsed(void)
{
	/* Tickless is not supported yet */
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return systimer_ll_get_counter_value_low(SYSTIMER_COUNTER_1);
}
