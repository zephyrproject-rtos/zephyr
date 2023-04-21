/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT hisilicon_hi3861_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/spinlock.h>
#include <zephyr/irq.h>
#include <zephyr/arch/common/sys_io.h>

#define TIMER_BASE DT_INST_REG_ADDR(0)

#define TIMER_0 (TIMER_BASE + 0x00)
#define TIMER_1 (TIMER_BASE + 0x14)
#define TIMER_2 (TIMER_BASE + 0x28)
#define TIMER_3 (TIMER_BASE + 0x3C)

#define TIMER_LOADCOUNT_REG(BASE)    (BASE + 0x000)
#define TIMER_CURRENTVALUE_REG(BASE) (BASE + 0x004)
#define TIMER_CONTROLREG_REG(BASE)   (BASE + 0x008)
#define TIMER_CONTROLREG_EN          (1 << 0)
#define TIMER_CONTROLREG_MODE_FREE   (0 << 1)
#define TIMER_CONTROLREG_MODE_CYCLE  (1 << 1)
#define TIMER_CONTROLREG_INT_MASK    (1 << 2)
#define TIMER_CONTROLREG_INT_UNMASK  (0 << 2)
#define TIMER_CONTROLREG_LOCK        (1 << 3)
#define TIMER_EOI_REG(BASE)          (BASE + 0x00C)
#define TIMER_INTSTATUS_REG(BASE)    (BASE + 0x010)

#define SYSTICK TIMER_3

#define CYCLES_PER_TICK (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

static volatile uint64_t announced_ticks;

static void sys_clock_isr(const void *arg)
{
	ARG_UNUSED(arg);

	/* Read EOI to clear interrupt */
	sys_read32(TIMER_EOI_REG(SYSTICK));

	announced_ticks++;

	sys_clock_announce(1);
}

/*
 * Since we're not tickless, this is identically no-op.
 */
void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(ticks);
	ARG_UNUSED(idle);
}

/*
 * Since we're not tickless, this is identically zero.
 */
uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return sys_clock_cycle_get_64() & BIT64_MASK(32);
}

uint64_t sys_clock_cycle_get_64(void)
{
	return announced_ticks * CYCLES_PER_TICK;
}

static int sys_clock_driver_init(void)
{
	uint32_t regval;

	/* Disable timer */
	regval = 0;
	sys_write32(regval, TIMER_CONTROLREG_REG(SYSTICK));

	sys_write32(CYCLES_PER_TICK, TIMER_LOADCOUNT_REG(SYSTICK));

	regval |= TIMER_CONTROLREG_MODE_CYCLE | TIMER_CONTROLREG_INT_UNMASK;
	sys_write32(regval, TIMER_CONTROLREG_REG(SYSTICK));

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), sys_clock_isr, NULL, 0);
	irq_enable(DT_INST_IRQN(0));

	/* Enable timer */
	regval |= TIMER_CONTROLREG_EN;
	sys_write32(regval, TIMER_CONTROLREG_REG(SYSTICK));

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
