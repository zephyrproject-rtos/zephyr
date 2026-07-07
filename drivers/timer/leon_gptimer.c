/*
 * Copyright (c) 2019-2020 Cobham Gaisler AB
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This driver uses two independent GPTIMER subtimers:
 * - subtimer 1 is a free-running up-counter clocked at the system cycle rate
 *   divided by the shared prescaler, the definitive time base used for both
 *   tick accounting and sys_clock_cycle_get_32().
 * - subtimer 0 is programmed as a one-shot down-counter for the next deadline
 *   in tickless mode, or as a periodic source when CONFIG_TICKLESS_KERNEL=n.
 *
 * GPTIMER has no absolute compare register, only a relative reload, so this is
 * a RELOAD backend: the free-running subtimer 1 is the cycle source and the
 * generic core turns a tick-aligned deadline into the relative delay loaded
 * into subtimer 0.
 */

#define DT_DRV_COMPAT gaisler_gptimer

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>

/* Shared prescaler division factor. The GPTIMER subtimers share a single
 * decrementer, so the minimum valid division factor is ntimers + 1 (GRLIB IP
 * Core User's Manual, GPTIMER section). A core has at most 7 timers, so 8 is
 * always valid; the subtimers are clocked at the system rate / PRESCALER.
 */
#define PRESCALER 8U

/* The subtimers run at the system cycle rate divided by the shared prescaler.
 * That is the rate timer_driver_cycle_get() reports, so the core derives its
 * cycles-per-tick from it; only the public sys_clock_cycle_get_32() scales back
 * up to the system cycle rate.
 */
#define TIMER_CORE_CYCLES_PER_SEC (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / PRESCALER)

/* Smallest delay programmed into subtimer 0. program_subtimer0() writes
 * reload = delay - 1, so delay must be at least 2 to avoid a reload of 0
 * (and the wraparound for delay == 0).
 */
#define MIN_DELAY_CYC 2U

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

/* Free-running up-counter (subtimer 1) in subtimer cycles. Wraps every 2^32
 * cycles; the core masks deltas against its baseline so this stays correct
 * across a single wrap.
 */
static inline uint32_t up_counter(volatile struct gptimer_regs *regs)
{
	return 0U - regs->timer[1].counter;
}

/* Program subtimer 0 to fire delay cycles from now. Periodic (auto-restart)
 * when the kernel is not tickless, one-shot otherwise.
 */
static void program_subtimer0(volatile struct gptimer_regs *regs, uint32_t delay)
{
	volatile struct gptimer_timer_regs *tmr = &regs->timer[0];
	uint32_t ctrl = GPTIMER_CTRL_IE | GPTIMER_CTRL_LD | GPTIMER_CTRL_EN;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		ctrl |= GPTIMER_CTRL_RS;
	}

	tmr->reload = delay - 1U;
	tmr->ctrl = ctrl;
}

#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_MIN_DELAY MIN_DELAY_CYC
#define TIMER_CORE_HAVE_CYCLE_GET_32

static inline uint64_t timer_driver_cycle_get(void)
{
	return up_counter(get_regs());
}

static void timer_driver_set_reload(uint32_t cycles)
{
	program_subtimer0(get_regs(), cycles);
}

#include "system_timer_generic.h"

uint32_t sys_clock_cycle_get_32(void)
{
	/* Scale the subtimer count back up to the system cycle rate the kernel
	 * expects.
	 */
	return up_counter(get_regs()) * PRESCALER;
}

static void timer_isr(const void *unused)
{
	ARG_UNUSED(unused);
	volatile struct gptimer_regs *regs = get_regs();
	volatile struct gptimer_timer_regs *tmr = &regs->timer[0];

	if ((tmr->ctrl & GPTIMER_CTRL_IP) == 0) {
		return; /* interrupt not for us */
	}

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* One-shot: stop and clear pending; sys_clock_set_timeout()
		 * reprograms the next deadline.
		 */
		tmr->ctrl = gptimer_ctrl_clear_ip;
	} else {
		/* Periodic: clear pending and keep running. */
		tmr->ctrl = GPTIMER_CTRL_IE | GPTIMER_CTRL_RS |
			    GPTIMER_CTRL_EN | gptimer_ctrl_clear_ip;
	}

	timer_core_announce();
}

static void init_downcounter(volatile struct gptimer_timer_regs *tmr)
{
	tmr->reload = 0xFFFFFFFF;
	tmr->ctrl = GPTIMER_CTRL_LD | GPTIMER_CTRL_RS | GPTIMER_CTRL_EN;
}

static int sys_clock_driver_init(void)
{
	const int timer_interrupt = get_timer_irq();
	volatile struct gptimer_regs *regs = get_regs();
	volatile struct gptimer_timer_regs *tmr = &regs->timer[0];

	/* Set the shared prescaler to its minimum always-valid division factor
	 * (reload = PRESCALER - 1), then start the free-running counter. A
	 * reload below ntimers is rejected by the hardware because the timers
	 * share one decrementer (GRLIB IP Core User's Manual, GPTIMER section).
	 */
	regs->scaler_reload = PRESCALER - 1U;
	init_downcounter(&regs->timer[1]);

	/* Stop subtimer 0 and probe how CTRL_IP is cleared (write 1 or 0). */
	tmr->ctrl = GPTIMER_CTRL_IP;
	if ((tmr->ctrl & GPTIMER_CTRL_IP) == 0) {
		/* IP bit is cleared by setting it to 1. */
		gptimer_ctrl_clear_ip = GPTIMER_CTRL_IP;
	}

	irq_connect_dynamic(timer_interrupt, 0, timer_isr, NULL, 0);
	irq_enable(timer_interrupt);

	/* Seed the announce baseline from the free-running counter. In tickless
	 * mode this also arms the first deadline (one tick out) and the kernel
	 * reprograms via sys_clock_set_timeout() thereafter.
	 */
	timer_core_init();

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* Periodic mode: subtimer 0 auto-restarts (CTRL_RS) every tick and
		 * the core never reprograms it.
		 */
		program_subtimer0(regs, TIMER_CORE_CYC_PER_TICK);
	}

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
