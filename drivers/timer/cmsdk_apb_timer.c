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
#include <zephyr/spinlock.h>
#include <zephyr/sys_clock.h>
#include "timer_cmsdk_apb.h"

#define TIMER_NODE DT_CHOSEN(zephyr_system_timer)

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an arm,cmsdk-timer node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(TIMER_NODE, arm_cmsdk_timer),
	     "zephyr,system-timer must point to an arm,cmsdk-timer compatible node");

#define TIMER_IRQ      DT_IRQN(TIMER_NODE)
#define TIMER_IRQ_PRIO DT_IRQ(TIMER_NODE, priority)
#define TIMER_BASE     DT_REG_ADDR(TIMER_NODE)

#define CYC_PER_TICK                                                                               \
	((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() /                                      \
		    (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))
#define MAX_CYC UINT32_MAX

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

typedef uint32_t cycle_t;

struct tmr_cmsdk_apb_cfg {
	volatile struct timer_cmsdk_apb *timer;
};

struct tmr_cmsdk_apb_dev_data {
	uint32_t load;
	cycle_t cycle_count;
	cycle_t announced_cycles;
	/* ticks last reported via sys_clock_elapsed(), cleared in the ISR */
	cycle_t last_elapsed;
};

static const struct tmr_cmsdk_apb_cfg cfg_inst0 = {
	.timer = ((volatile struct timer_cmsdk_apb *)TIMER_BASE),
};

static struct tmr_cmsdk_apb_dev_data data_inst0;

static uint32_t elapsed(uint32_t *val_out)
{
	const struct tmr_cmsdk_apb_cfg *const cfg = &cfg_inst0;
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;

	uint32_t value = cfg->timer->value;

	if (val_out != NULL) {
		*val_out = value;
	}

	return data->load - value;
}

/* Elapsed cycles since the last reload, including a wrap that fired but has not
 * yet been accounted by the ISR.
 *
 * elapsed() above returns only the in-period offset (load - value). Between a
 * wrap (the counter reloads) and the ISR crediting that period to cycle_count,
 * that offset drops back near zero, so cycle_count + elapsed() briefly goes
 * backwards. It is a race under real time but is hit deterministically under
 * QEMU icount, and a non-monotonic hardware cycle counter breaks k_busy_wait()
 * and the tick accounting.
 *
 * Detect the pending wrap the way the SysTick driver does: sample the value
 * either side of the interrupt-status flag and add a full period if the flag is
 * set or the counter was seen reloading (v1 < v2). This is added to the
 * returned value only; the ISR commits the period into cycle_count and clears
 * the flag, so it is never counted twice. Kept separate from elapsed() so
 * sys_clock_set_timeout()'s reprogramming still works on the raw offset.
 */
static uint32_t elapsed_monotonic(void)
{
	const struct tmr_cmsdk_apb_cfg *const cfg = &cfg_inst0;
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;

	uint32_t v1 = cfg->timer->value;
	uint32_t wrapped = cfg->timer->intstatus & TIMER_CTRL_INT_CLEAR;
	uint32_t v2 = cfg->timer->value;
	uint32_t pending = (wrapped || (v1 < v2)) ? data->load : 0;

	return (data->load - v2) + pending;
}

void sys_clock_unused(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	const struct tmr_cmsdk_apb_cfg *const cfg = &cfg_inst0;

	/* No pending timeout: reload the counter with the maximum interval so
	 * the hardware waits as long as it can before the next interrupt.
	 *
	 * This is an auto-reload down-counter, not a free-running compare, so
	 * it cannot simply stop reprogramming the way the compare timers do:
	 * that would keep firing at the previous, possibly short, interval.
	 * Programming the maximum reload matches what non-sloppy idle already
	 * does via next_timeout(). Stopping the counter here (clearing
	 * TIMER_CTRL_EN and re-enabling it in sys_clock_set_timeout()) would
	 * avoid the wakeup entirely; that is left out for now to avoid a
	 * riskier change to the cycle accounting.
	 */
	cfg->timer->reload = MAX_CYC;
	cfg->timer->value = MAX_CYC;
}

void sys_clock_set_timeout(uint32_t ticks)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}


	const struct tmr_cmsdk_apb_cfg *const cfg = &cfg_inst0;
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	uint32_t last_load = data->load;
	uint32_t val1;
	uint32_t val2;

	/* store the current cfg->timer->value in val1 */
	uint32_t pending_cycles = elapsed(&val1);
	uint32_t load_to_be_set = 0;
	uint32_t unannounced_cycles = 0;

	data->cycle_count += pending_cycles;
	unannounced_cycles = data->cycle_count - data->announced_cycles;

	if ((int32_t)unannounced_cycles < 0) {
		load_to_be_set = min_delay;
	} else {
		int64_t want = ((uint64_t)data->last_elapsed + ticks) * CYC_PER_TICK;
		int64_t delta_cycles = want - unannounced_cycles;

		load_to_be_set = CLAMP(delta_cycles, (int64_t)min_delay, (int64_t)MAX_CYC);
	}

	data->load = load_to_be_set;

	val2 = cfg->timer->value;

	cfg->timer->reload = data->load;
	cfg->timer->value = data->load;

	/* verify if underflow occurred after reading val1 and before reading val2 */
	if (val1 < val2) {
		data->cycle_count += (val1 + (last_load - val2));
	} else {
		data->cycle_count += (val1 - val2);
	}
}

uint32_t sys_clock_elapsed(void)
{
	__ASSERT(sys_clock_is_locked(), "system clock lock not held");

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	uint32_t unannounced = data->cycle_count - data->announced_cycles;
	uint32_t cycles = elapsed_monotonic() + unannounced;
	uint32_t ret = cycles / CYC_PER_TICK;

	data->last_elapsed = ret;
	return ret;
}

uint32_t sys_clock_cycle_get_32(void)
{
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	k_spinlock_key_t key = sys_clock_lock();
	uint32_t cycles = data->cycle_count + elapsed_monotonic();

	sys_clock_unlock(key);

	return cycles;
}

static void cmsdk_apb_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);
	const struct tmr_cmsdk_apb_cfg *const cfg = &cfg_inst0;
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	uint32_t ticks = 1;
	k_spinlock_key_t key = sys_clock_lock();

	data->cycle_count += data->load;

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t unannounced_cycles = data->cycle_count - data->announced_cycles;

		ticks = unannounced_cycles / CYC_PER_TICK;
		data->announced_cycles += ticks * CYC_PER_TICK;
		data->last_elapsed = 0;
	}

	cfg->timer->intclear = TIMER_CTRL_INT_CLEAR;
	NVIC_ClearPendingIRQ(TIMER_IRQ);
	sys_clock_announce_locked(ticks, key);
}

static int sys_clock_driver_init(void)
{
	struct tmr_cmsdk_apb_dev_data *data = &data_inst0;
	const struct tmr_cmsdk_apb_cfg *cfg = &cfg_inst0;

	min_delay = cmsdk_min_delay();
	data->last_elapsed = 0;
	data->load = CYC_PER_TICK;
	cfg->timer->reload = CYC_PER_TICK;
	cfg->timer->value = CYC_PER_TICK;
	cfg->timer->ctrl = TIMER_CTRL_EN | TIMER_CTRL_IRQ_EN;

	IRQ_CONNECT(TIMER_IRQ, TIMER_IRQ_PRIO, cmsdk_apb_timer_isr, NULL, 0);
	irq_enable(TIMER_IRQ);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2, CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
