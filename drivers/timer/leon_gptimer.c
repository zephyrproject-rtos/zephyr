/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This driver uses two independent GPTIMER subtimers in the following way:
 * - subtimer 0 generates periodic interrupts and the ISR announces ticks.
 * - subtimer 1 is used as up-counter.
 */

#define DT_DRV_COMPAT gaisler_gptimer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>

/* GPTIMER subtimer increments each microsecond. */
#define PRESCALER (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000000)

/* GPTIMER Timer instance */
struct gptimer_timer_regs {
	uint32_t counter;
	uint32_t reload;
	uint32_t ctrl;
	uint32_t latch;
};

/* A GPTIMER can have maximum of 7 subtimers. */
#define GPTIMER_MAX_SUBTIMERS 7

/* GPTIMER common registers */
struct gptimer_regs {
	uint32_t scaler_value;
	uint32_t scaler_reload;
	uint32_t cfg;
	uint32_t latch_cfg;
	struct gptimer_timer_regs timer[GPTIMER_MAX_SUBTIMERS];
};

#define GPTIMER_CTRL_WN         (1 << 7)
#define GPTIMER_CTRL_IP         (1 << 4)
#define GPTIMER_CTRL_IE         (1 << 3)
#define GPTIMER_CTRL_LD         (1 << 2)
#define GPTIMER_CTRL_RS         (1 << 1)
#define GPTIMER_CTRL_EN         (1 << 0)
#define GPTIMER_CFG_EL          (1 << 11)
#define GPTIMER_CFG_DF          (1 << 9)
#define GPTIMER_CFG_SI          (1 << 8)
#define GPTIMER_CFG_IRQ         (0x1f << 3)
#define GPTIMER_CFG_TIMERS      (7 << 0)

static volatile struct gptimer_regs *get_regs(void)
{
	return (struct gptimer_regs *) DT_INST_REG_ADDR(0);
}

static int get_timer_irq(void)
{
	return DT_INST_IRQN(0);
}

static uint32_t gptimer_ctrl_clear_ip;

static void timer_isr(const void *unused)
{
	ARG_UNUSED(unused);
	volatile struct gptimer_regs *regs = get_regs();
	volatile struct gptimer_timer_regs *tmr = &regs->timer[0];
	uint32_t ctrl;

	ctrl = tmr->ctrl;
	if ((ctrl & GPTIMER_CTRL_IP) == 0) {
		return; /* interrupt not for us */
	}

	/* Clear pending */
	tmr->ctrl = GPTIMER_CTRL_IE | GPTIMER_CTRL_RS |
		    GPTIMER_CTRL_EN | gptimer_ctrl_clear_ip;

	sys_clock_announce(1);
}

uint32_t sys_clock_elapsed(void)
{
	return 0;
}

uint32_t sys_clock_cycle_get_32(void)
{
	volatile struct gptimer_regs *regs = get_regs();
	volatile struct gptimer_timer_regs *tmr = &regs->timer[1];
	uint32_t counter = tmr->counter;

	return (0 - counter) * PRESCALER;
}

static void init_downcounter(volatile struct gptimer_timer_regs *tmr)
{
	tmr->reload = 0xFFFFFFFF;
	tmr->ctrl = GPTIMER_CTRL_LD | GPTIMER_CTRL_RS | GPTIMER_CTRL_EN;
}

static int sys_clock_driver_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	const int timer_interrupt = get_timer_irq();
	volatile struct gptimer_regs *regs = get_regs();
	volatile struct gptimer_timer_regs *tmr = &regs->timer[0];

	init_downcounter(&regs->timer[1]);

	/* Stop timer and probe how CTRL_IP is cleared (write 1 or 0). */
	tmr->ctrl = GPTIMER_CTRL_IP;
	if ((tmr->ctrl & GPTIMER_CTRL_IP) == 0) {
		/* IP bit is cleared by setting it to 1. */
		gptimer_ctrl_clear_ip = GPTIMER_CTRL_IP;
	}

	/* Configure timer scaler for 1 MHz subtimer tick */
	regs->scaler_reload = PRESCALER - 1;
	tmr->reload = 1000000U / CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1;
	tmr->ctrl = GPTIMER_CTRL_IE | GPTIMER_CTRL_LD | GPTIMER_CTRL_RS |
		    GPTIMER_CTRL_EN;

	irq_connect_dynamic(timer_interrupt, 0, timer_isr, NULL, 0);
	irq_enable(timer_interrupt);
	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
