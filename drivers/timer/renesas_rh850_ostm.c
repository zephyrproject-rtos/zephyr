/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>

#define OSTM_NODE DT_NODELABEL(ostm0)
#define OSTM_BASE ((mem_addr_t)DT_REG_ADDR(OSTM_NODE))
#define OSTM_IRQ  DT_IRQN(OSTM_NODE)

#define OSTM_CMP 0x00
#define OSTM_CNT 0x04
#define OSTM_TS  0x14
#define OSTM_TT  0x18
#define OSTM_CTL 0x20

#define OSTM_START       BIT(0)
#define OSTM_CTL_IE      BIT(7)
#define OSTM_CTL_FREERUN BIT(1)

BUILD_ASSERT(DT_PROP(OSTM_NODE, clock_frequency) == CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC,
	     "OSTM clock-frequency must match CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC");

#if defined(CONFIG_TEST)
const int32_t z_sys_timer_irq_for_test = OSTM_IRQ;
#endif

/* A CMP value the counter has already passed is missed for a whole period */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH 32

static uint32_t timer_driver_cycle_get(void)
{
	return sys_read32(OSTM_BASE + OSTM_CNT);
}

static void timer_driver_set_compare(uint32_t cycles)
{
	sys_write32(cycles, OSTM_BASE + OSTM_CMP);
	/* Read back so the write has reached the timer before CNT is read */
	(void)sys_read32(OSTM_BASE + OSTM_CMP);
}

#include "system_timer_generic.h"

void sys_clock_disable(void)
{
	sys_write8(OSTM_START, OSTM_BASE + OSTM_TT);
	irq_disable(OSTM_IRQ);
}

static void sys_clock_isr(const void *arg)
{
	ARG_UNUSED(arg);

	timer_core_announce();
}

static int sys_clock_driver_init(void)
{
	IRQ_CONNECT(OSTM_IRQ, DT_IRQ(OSTM_NODE, priority), sys_clock_isr, NULL, 0);

	sys_write8(OSTM_START, OSTM_BASE + OSTM_TT);
	sys_write8(OSTM_CTL_IE | OSTM_CTL_FREERUN, OSTM_BASE + OSTM_CTL);
	sys_write8(OSTM_START, OSTM_BASE + OSTM_TS);

	timer_core_init();

	irq_enable(OSTM_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
