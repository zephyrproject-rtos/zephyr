/*
 * Copyright (c) 2026 ASPEED Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aspeed_ast2700_bootmcu_timer

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#define TMR_BASE DT_INST_REG_ADDR(0)
#define TMR_IRQN DT_INST_IRQN(0)

#define TMR_COUNT_L  0x00U
#define TMR_COUNT_H  0x04U
#define TMR_ALARM_L  0x08U
#define TMR_ALARM_H  0x0cU
#define TMR_CTRL     0x10U
#define TMR_CTRL_CLR 0x14U

/* Alarm interrupt enable; the counter keeps running while it is clear */
#define TMR_CTRL_EN       BIT(0)
/* Count the 1 MHz reference clock instead of PCLK */
#define TMR_CTRL_CLK_1MHZ BIT(1)
#define TMR_CTRL_RESET_EN BIT(3)

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = TMR_IRQN;
#endif

/* The alarm fires only when the counter equals it; one already passed is missed. */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH 64
#define TIMER_CORE_HAVE_CYCLE_GET_32

static uint64_t timer_driver_cycle_get(void)
{
	uint32_t hi;
	uint32_t lo;

	/* Guard against a low-word rollover between the two register reads. */
	do {
		hi = sys_read32(TMR_BASE + TMR_COUNT_H);
		lo = sys_read32(TMR_BASE + TMR_COUNT_L);
	} while (hi != sys_read32(TMR_BASE + TMR_COUNT_H));

	return ((uint64_t)hi << 32) | lo;
}

static void timer_driver_set_compare(uint64_t cycles)
{
	sys_write32(TMR_CTRL_EN, TMR_BASE + TMR_CTRL_CLR);

	/* Writing both alarm words clears the latched interrupt status. */
	sys_write32((uint32_t)cycles, TMR_BASE + TMR_ALARM_L);
	sys_write32((uint32_t)(cycles >> 32), TMR_BASE + TMR_ALARM_H);
	sys_write32(TMR_CTRL_EN, TMR_BASE + TMR_CTRL);
}

#include "system_timer_generic.h"

uint32_t sys_clock_cycle_get_32(void)
{
	return sys_read32(TMR_BASE + TMR_COUNT_L);
}

static void timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Mask the interrupt until the next alarm is programmed. */
	sys_write32(TMR_CTRL_EN, TMR_BASE + TMR_CTRL_CLR);
	timer_core_announce();
}

static int sys_clock_driver_init(void)
{
	sys_write32(TMR_CTRL_EN, TMR_BASE + TMR_CTRL_CLR);
	sys_write32(UINT32_MAX, TMR_BASE + TMR_ALARM_L);
	sys_write32(UINT32_MAX, TMR_BASE + TMR_ALARM_H);
	sys_write32(TMR_CTRL_CLK_1MHZ | TMR_CTRL_RESET_EN, TMR_BASE + TMR_CTRL);

	IRQ_CONNECT(TMR_IRQN, 0, timer_isr, NULL, 0);
	timer_core_init();
	irq_enable(TMR_IRQN);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
