/*
 * Copyright (c) 2023-2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT wch_ch32v_v3_systick

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>

#define CYCLES_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define STK_BASE DT_INST_REG_ADDR(0)

#define STK_R32_CTLR  (STK_BASE + 0x00)
#define STK_R64_CNT   (STK_BASE + 0x04)
#define STK_R64_CMPR  (STK_BASE + 0x0C)
#define STK_R32_CNTFG (STK_BASE + 0x14)

/* STK_R32_CTLR */
#define CTLR_STE    BIT(0)
#define CTLR_STIE   BIT(1)
#define CTLR_STCLK  BIT(2)
#define CTLR_RELOAD BIT(8)

/* STK_R32_CNTFG */
#define CNTFG_SWIE  BIT(0)
#define CNTFG_CNTIF BIT(1)

static volatile uint64_t announced_ticks;

static void sys_clock_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Clear interrupt */
	sys_write32(0, STK_R32_CNTFG);

	announced_ticks++;
	sys_clock_announce(1);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return (uint32_t)sys_clock_cycle_get_64();
}

uint64_t sys_clock_cycle_get_64(void)
{
	return announced_ticks * CYCLES_PER_TICK +
	       (sys_read64(STK_R64_CMPR) - sys_read64(STK_R64_CNT));
}

static int sys_clock_init(void)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), sys_clock_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	sys_write64(CYCLES_PER_TICK, STK_R64_CMPR);
	sys_write32(CTLR_STE | CTLR_STIE | CTLR_STCLK | CTLR_RELOAD, STK_R32_CTLR);

	return 0;
}

SYS_INIT(sys_clock_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
