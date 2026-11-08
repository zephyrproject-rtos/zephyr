/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/sys_io.h>

#include <reg/hi3861_timer_regs.h>

LOG_MODULE_REGISTER(hi3861_timer);

#define TIMER_NODE   DT_CHOSEN(zephyr_system_timer)
#define ALARM_BASE   DT_REG_ADDR_BY_NAME(TIMER_NODE, alarm)
#define COUNTER_BASE DT_REG_ADDR_BY_NAME(TIMER_NODE, counter)
#define TIMER_IRQ    DT_IRQN(TIMER_NODE)

BUILD_ASSERT(DT_NODE_HAS_COMPAT(TIMER_NODE, hisilicon_hi3861_timer),
	     "zephyr,system-timer must select a Hi3861 timer");
BUILD_ASSERT(DT_PROP(TIMER_NODE, clock_frequency) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "Hi3861 timer clock frequency must match SYS_CLOCK_HW_CYCLES_PER_SEC");

static inline uint32_t timer_driver_cycle_get(void)
{
	/* Latch the cycle counter before reading CURRENT_VALUE. */
	sys_write32(sys_read32(COUNTER_BASE + TIMER_CONTROL) | TIMER_SNAPSHOT,
		    COUNTER_BASE + TIMER_CONTROL);

	for (int i = 0; i < 1024; i++) {
		if (!(sys_read32(COUNTER_BASE + TIMER_CONTROL) & TIMER_SNAPSHOT)) {
			return UINT32_MAX - sys_read32(COUNTER_BASE + TIMER_VALUE);
		}
	}

	LOG_ERR("Timer counter snapshot timed out");
	k_panic();
	CODE_UNREACHABLE;
}

static void timer_driver_set_reload(uint32_t cycles)
{
	sys_write32(0, ALARM_BASE + TIMER_CONTROL);
	(void)sys_read32(ALARM_BASE + TIMER_EOI);
	sys_write32(cycles - 1, ALARM_BASE + TIMER_LOAD);
	sys_write32(TIMER_ENABLE | TIMER_PERIODIC, ALARM_BASE + TIMER_CONTROL);
}

#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_ALARM_MIN_CYCLES 256
#define TIMER_CORE_COUNTER_NONATOMIC

#include "system_timer_generic.h"

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);
	(void)sys_read32(ALARM_BASE + TIMER_EOI);
	timer_core_announce();
}

static int sys_clock_driver_init(void)
{
	sys_write32(0, ALARM_BASE + TIMER_CONTROL);
	sys_write32(0, COUNTER_BASE + TIMER_CONTROL);
	(void)sys_read32(ALARM_BASE + TIMER_EOI);
	(void)sys_read32(COUNTER_BASE + TIMER_EOI);
	sys_write32(UINT32_MAX, COUNTER_BASE + TIMER_LOAD);
	sys_write32(TIMER_ENABLE | TIMER_MASK_IRQ, COUNTER_BASE + TIMER_CONTROL);
	IRQ_CONNECT(TIMER_IRQ, 0, timer_isr, NULL, 0);
	timer_core_init();
	irq_enable(TIMER_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
