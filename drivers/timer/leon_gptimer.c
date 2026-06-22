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
 * GPTIMER has no absolute compare register, so a tick-aligned absolute
 * deadline is computed against the free-running counter and programmed as a
 * relative delay (subtimer 0 reload). The ISR derives the number of elapsed
 * ticks from the free-running counter, so a late or coalesced interrupt
 * announces every elapsed tick rather than dropping it.
 */

#include <limits.h>

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

/* Counter cycles per kernel tick (the subtimer clock is HW cycles / PRESCALER).
 * Derived from CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC so it tracks the system clock.
 */
#define CYC_PER_TICK \
	(CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / PRESCALER / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* A programmed delay is detected as already-passed via a signed 32-bit delta
 * in sys_clock_set_timeout() and is loaded into a 32-bit reload, so it must
 * stay below 2^31 cycles.
 */
#define MAX_CYCLES 0x7FFFFFFFU
#define MAX_TICKS  (MAX_CYCLES / CYC_PER_TICK)

/* Smallest delay programmed into subtimer 0. program_subtimer0() writes
 * reload = delay - 1, so delay must be at least 2 to avoid a reload of 0
 * (and the wraparound for delay == 0); a deadline at or before "now" fires
 * as soon as possible and the ISR catches up.
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

/* Free-running up-counter (subtimer 1) in cycles. Wraps every 2^32 cycles;
 * unsigned subtraction against the announce baseline stays correct across a
 * single wrap.
 */
static uint32_t announced_cyc; /* counter value at the last announce (tick-aligned) */
static uint32_t last_elapsed;  /* ticks reported by the last sys_clock_elapsed() */

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

static void timer_isr(const void *unused)
{
	ARG_UNUSED(unused);
	volatile struct gptimer_regs *regs = get_regs();
	volatile struct gptimer_timer_regs *tmr = &regs->timer[0];
	uint32_t dticks;

	if ((tmr->ctrl & GPTIMER_CTRL_IP) == 0) {
		return; /* interrupt not for us */
	}

	/* Whole ticks elapsed since the last announce, taken from the
	 * free-running counter so a late interrupt catches up every tick.
	 */
	dticks = (up_counter(regs) - announced_cyc) / CYC_PER_TICK;
	announced_cyc += dticks * CYC_PER_TICK;
	last_elapsed = 0;

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

	sys_clock_announce(dticks);
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);
	volatile struct gptimer_regs *regs = get_regs();
	uint32_t target, now;
	int32_t delay;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER || ticks > MAX_TICKS) {
		ticks = MAX_TICKS;
	}

	/* The ISR eventually announces last_elapsed + ticks (plus IRQ-servicing
	 * latency) and sys_clock_announce() takes an int32_t. Keep the sum
	 * within INT32_MAX, reserving half the range for the latency.
	 */
	if (ticks > (INT32_MAX / 2 - last_elapsed)) {
		ticks = INT32_MAX / 2 - last_elapsed;
	}

	/* Absolute, tick-aligned deadline from the last announce, programmed as
	 * a relative delay. last_elapsed is the tick count already reported via
	 * sys_clock_elapsed(), so the fire lands on the requested tick boundary
	 * regardless of the sub-tick offset of the counter.
	 */
	target = announced_cyc + (last_elapsed + (uint32_t)ticks) * CYC_PER_TICK;
	now = up_counter(regs);
	delay = target - now;
	if (delay < (int32_t)MIN_DELAY_CYC) {
		delay = MIN_DELAY_CYC;
	}
	program_subtimer0(regs, delay);
}

uint32_t sys_clock_elapsed(void)
{
	volatile struct gptimer_regs *regs = get_regs();

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	last_elapsed = (up_counter(regs) - announced_cyc) / CYC_PER_TICK;
	return last_elapsed;
}

uint32_t sys_clock_cycle_get_32(void)
{
	volatile struct gptimer_regs *regs = get_regs();

	/* Scale the counter back up to the system cycle rate the kernel expects. */
	return up_counter(regs) * PRESCALER;
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

	/* Anchor the announce baseline to "now" so curr_tick == 0 maps to the
	 * current free-running counter value.
	 */
	announced_cyc = up_counter(regs);

	irq_connect_dynamic(timer_interrupt, 0, timer_isr, NULL, 0);
	irq_enable(timer_interrupt);

	/* Program the first deadline: one tick away. In tickless mode the
	 * kernel reprograms via sys_clock_set_timeout(); in periodic mode this
	 * is the recurring tick.
	 */
	program_subtimer0(regs, CYC_PER_TICK);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
