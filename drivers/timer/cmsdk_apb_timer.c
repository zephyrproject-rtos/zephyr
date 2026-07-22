/*
 * Copyright (c) 2026 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT arm_cmsdk_timer

#include <zephyr/device.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/sys_clock.h>
#include <zephyr/sys/util.h>
#include <cmsis_core.h>
#include "timer_cmsdk_apb.h"

#define TIMER_NODE DT_CHOSEN(zephyr_system_timer)

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an arm,cmsdk-timer node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(TIMER_NODE, arm_cmsdk_timer),
	     "zephyr,system-timer must point to an arm,cmsdk-timer compatible node");

#define TIMER_IRQ      DT_IRQN(TIMER_NODE)
#define TIMER_IRQ_PRIO DT_IRQ(TIMER_NODE, priority)
#define TIMER_BASE     DT_REG_ADDR(TIMER_NODE)

/* Minimum reload the driver will program: the closest-in timeout it can
 * schedule. It must exceed the longest the timer interrupt can stay masked, or
 * the counter wraps more than once between elapsed_cyc()'s reads and a period
 * is lost. That is a wall-clock property, so derive it from a fixed time
 * (converted to cycles at the counter rate, at init) rather than a fixed cycle
 * count, which is a different wall-clock time on every clock. The
 * CMSDK_APB_TIMER_MIN_DELAY_OVERRIDE Kconfig still takes precedence, and a
 * two-cycle hardware floor keeps the reload from being degenerate on a slow
 * clock where the time budget rounds below it.
 */
#define CMSDK_MIN_DELAY_US 10U

static inline uint32_t cmsdk_min_delay(void)
{
#ifdef CONFIG_CMSDK_APB_TIMER_MIN_DELAY_OVERRIDE
	uint32_t cyc = CONFIG_CMSDK_APB_TIMER_MIN_DELAY_CYCLES;
#else
	uint32_t cyc = k_us_to_cyc_ceil32(CMSDK_MIN_DELAY_US);
#endif
	return MAX(2U, cyc);
}

/* Minimum reload, derived from the cycle rate in sys_clock_driver_init(). See
 * cmsdk_min_delay().
 */
static uint32_t min_delay;

static volatile struct timer_cmsdk_apb *const timer =
	(volatile struct timer_cmsdk_apb *)TIMER_BASE;

/* Currently programmed reload value, and the synthesized absolute cycle count
 * (committed on each wrap and each reprogram). The core owns the announce
 * baseline and the elapsed report.
 */
static uint32_t load;
static uint32_t cycle_count;

/* HW cycles counted since the last reload (the counter counts down to zero).
 *
 * A wrap that the ISR has not yet accounted for must still be included, or the
 * synthesized count goes backwards across the (counter-reloaded, ISR-pending)
 * window. That window is a race under real time but is hit deterministically
 * under QEMU icount. Detect the wrap the same way SysTick does: read the value
 * either side of the interrupt-status flag, and add a full period if the flag
 * is set or the counter was seen reloading (v1 < v2). The ISR commits that
 * period into cycle_count and clears the flag, so it is never counted twice.
 */
static inline uint32_t elapsed_cyc(void)
{
	uint32_t v1 = timer->value;
	uint32_t wrapped = timer->intstatus & TIMER_CTRL_INT_CLEAR;
	uint32_t v2 = timer->value;
	uint32_t pending = (wrapped || (v1 < v2)) ? load : 0;

	return (load - v2) + pending;
}

static inline uint64_t timer_driver_cycle_get(void)
{
	return cycle_count + elapsed_cyc();
}

static void timer_driver_set_reload(uint32_t rel)
{
	uint32_t last_load = load;
	uint32_t val1 = timer->value;
	uint32_t wrapped = timer->intstatus & TIMER_CTRL_INT_CLEAR;
	uint32_t val2 = timer->value;
	uint32_t pending = (wrapped || (val1 < val2)) ? last_load : 0;

	/* Commit the cycles counted down on the old reload up to the val2 read,
	 * plus a full period for a wrap the ISR has not yet committed. As in
	 * elapsed_cyc(), the pending flag and the val1 < val2 read describe the
	 * same possible wrap, so at most one period is added: adding them as
	 * separate terms would double-count a wrap that lands between the reads.
	 */
	cycle_count += (last_load - val2) + pending;
	load = rel;

	timer->reload = rel;
	timer->value = rel;

	/* The wrap folded in above is now committed; clear the status flag and
	 * pending IRQ so the ISR does not count it again. The counter has just
	 * been reloaded with rel (>= MIN_DELAY), so no genuine new wrap can be
	 * dropped here.
	 */
	timer->intclear = TIMER_CTRL_INT_CLEAR;
	NVIC_ClearPendingIRQ(TIMER_IRQ);
}

/*
 * Auto-reload 32-bit down-counter: a RELOAD backend. timer_driver_cycle_get()
 * synthesizes a monotonic count from the reloads and timer_driver_set_reload()
 * programs the next interval, recovering any cycles that pass between reading
 * the counter and rewriting it (the val1/val2 drift compensation) so none are
 * lost. That synthesized read is non-atomic (cycle_count and elapsed_cyc() are
 * shared with the ISR and reprogram paths), so TIMER_CORE_CYCLE_GET_NONATOMIC
 * has the core serialise sys_clock_cycle_get_32() under the clock lock.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_MIN_DELAY min_delay
#define TIMER_CORE_CYCLE_GET_NONATOMIC

#include "system_timer_generic.h"

void sys_clock_unused(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	/* Nothing is pending and uptime may drift. This is an auto-reload
	 * down-counter, so it cannot just stop being reprogrammed the way the
	 * compare timers do (it would keep firing at the previous interval);
	 * reload the longest safe interval instead so it waits as long as it can.
	 * Cap it at TIMER_CORE_CYCLES_MAX, not the full counter span: the
	 * synthesized count is 32-bit, so a full-span period would wrap it and
	 * alias the wrap announce to a near-zero delta (a whole period of uptime
	 * lost). TIMER_CORE_CYCLES_MAX keeps that announce unambiguous.
	 */
	k_spinlock_key_t key = sys_clock_lock();

	timer_driver_set_reload(TIMER_CORE_CYCLES_MAX);
	sys_clock_unlock(key);
}

static void cmsdk_apb_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	k_spinlock_key_t key = sys_clock_lock();

	/* The counter wrapped to fire this interrupt: account that interval. This
	 * must be atomic with the acknowledge and announce, else a concurrent
	 * timer_driver_set_reload() (or cycle read) sees a torn cycle_count.
	 */
	cycle_count += load;

	timer->intclear = TIMER_CTRL_INT_CLEAR;
	NVIC_ClearPendingIRQ(TIMER_IRQ);

	timer_core_announce_from(key);
}

static int sys_clock_driver_init(void)
{
	min_delay = cmsdk_min_delay();
	load = TIMER_CORE_CYC_PER_TICK;
	timer->reload = TIMER_CORE_CYC_PER_TICK;
	timer->value = TIMER_CORE_CYC_PER_TICK;
	timer->ctrl = TIMER_CTRL_EN | TIMER_CTRL_IRQ_EN;

	IRQ_CONNECT(TIMER_IRQ, TIMER_IRQ_PRIO, cmsdk_apb_timer_isr, NULL, 0);
	timer_core_init();
	irq_enable(TIMER_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
