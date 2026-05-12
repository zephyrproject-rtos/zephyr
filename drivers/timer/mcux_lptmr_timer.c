/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <fsl_lptmr.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/wuc.h>

BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_system_timer),
	     "zephyr,system-timer must be set to an nxp,lptmr node");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_system_timer), nxp_lptmr),
	     "zephyr,system-timer must point to an nxp,lptmr compatible node");

/* Prescaler clock mapping */
#define TO_LPTMR_CLK_SEL(val) _DO_CONCAT(kLPTMR_PrescalerClock_, val)

/* Devicetree node selected as system timer via zephyr,system-timer chosen */
#define LPTMR_NODE DT_CHOSEN(zephyr_system_timer)

/* Devicetree properties */
#define LPTMR_BASE ((LPTMR_Type *)(DT_REG_ADDR(LPTMR_NODE)))
#define LPTMR_CLK_SOURCE TO_LPTMR_CLK_SEL(DT_PROP_OR(LPTMR_NODE, clk_source, 0))
#define LPTMR_PRESCALER DT_PROP_OR(LPTMR_NODE, prescale_glitch_filter, 0)
/*
 * Default must be false so prescale-glitch-filter can be used without requiring
 * an explicit bypass property.
 */
#define LPTMR_PRESCALER_BYPASS DT_PROP_OR(LPTMR_NODE, prescale_glitch_filter_bypass, false)
#define LPTMR_IRQN DT_IRQN(LPTMR_NODE)
#define LPTMR_IRQ_PRIORITY DT_IRQ(LPTMR_NODE, priority)

/* Timer cycles per tick */
#define CYCLES_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() \
			/ (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

/* Counter maximum value based on resolution */
#define LPTMR_RESOLUTION DT_PROP(LPTMR_NODE, resolution)
#define COUNTER_MAX GENMASK(LPTMR_RESOLUTION - 1, 0)

#define MAX_TICKS ((COUNTER_MAX / CYCLES_PER_TICK) - 1)
#define MAX_CYCLES (MAX_TICKS * CYCLES_PER_TICK)
#define MIN_DELAY MAX(1024U, ((uint32_t)CYCLES_PER_TICK/16U))

/* 32 bit cycle counter, the variable only used in tickful mode */
static volatile uint32_t cycles;

/*
 * Stores the current number of cycles the system has had announced to it,
 * since the last rollover of the free running counter.
 */
static uint32_t announced_cycles;

/* Lock on shared variables */
static struct k_spinlock lock;

static inline uint32_t counter_delta(uint32_t now, uint32_t then)
{
	return (now - then) & COUNTER_MAX;
}

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	ARG_UNUSED(idle);

	if (idle && (ticks == K_TICKS_FOREVER)) {
		LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	}

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	k_spinlock_key_t key;
	uint32_t next, adj, now;

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	/* Clamp ticks. We subtract one since we round up to next tick */
	ticks = CLAMP((ticks - 1), 0, (int32_t)MAX_TICKS);

	key = k_spin_lock(&lock);

	/* Read current timer value */
	now = LPTMR_GetCurrentTimerCount(LPTMR_BASE);

	/* Adjustment value, used to ensure next capture is on tick boundary */
	adj = counter_delta(now, announced_cycles) + (CYCLES_PER_TICK - 1);

	next = ticks * CYCLES_PER_TICK;
	/*
	 * The following section rounds the capture value up to the next tick
	 * boundary
	 */
	if (next <= MAX_CYCLES - adj) {
		next += adj;
	} else {
		next = MAX_CYCLES;
	}
	next = (next / CYCLES_PER_TICK) * CYCLES_PER_TICK;

	/* Keep the comparison in LPTMR counter space so targets after a
	 * hardware counter wrap are treated as future compare values.
	 */
	if (counter_delta(next + announced_cycles, now) < MIN_DELAY) {
		next += CYCLES_PER_TICK;
	}

	next = (next + announced_cycles) & COUNTER_MAX;

	/* Update CMR safely while the timer is running.
	 *
	 * CMR writes are not hardware‑synchronized. If TCF is cleared while the
	 * interrupt is still enabled, TCF may be reasserted in a narrow race window
	 * and generate an unexpected interrupt.
	 *
	 * To avoid this, disable the LPTMR interrupt first, then clear TCF,
	 * program the new compare value, and finally re-enable the interrupt.
	 */
	LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	LPTMR_ClearStatusFlags(LPTMR_BASE, kLPTMR_TimerCompareFlag);
	LPTMR_SetTimerPeriod(LPTMR_BASE, next);
	LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);

	k_spin_unlock(&lock, key);
}

void sys_clock_idle_exit(void)
{
	if (LPTMR_GetEnabledInterrupts(LPTMR_BASE) != kLPTMR_TimerInterruptEnable) {
		LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	}
}

void sys_clock_disable(void)
{
	const struct wuc_dt_spec wuc = WUC_DT_SPEC_INST_GET_OR(0, {0});

	if (wuc.dev != NULL) {
		(void)wuc_disable_wakeup_source_dt(&wuc);
	}

	LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	LPTMR_StopTimer(LPTMR_BASE);
}

uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);
	uint32_t now = LPTMR_GetCurrentTimerCount(LPTMR_BASE);

	now = counter_delta(now, announced_cycles);
	k_spin_unlock(&lock, key);

	return now / CYCLES_PER_TICK;
}

uint32_t sys_clock_cycle_get_32(void)
{
	return LPTMR_GetCurrentTimerCount(LPTMR_BASE) + cycles;
}

static void mcux_lptmr_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);
	k_spinlock_key_t key;
	uint32_t tick = 0;

	key = k_spin_lock(&lock);
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		uint32_t now = LPTMR_GetCurrentTimerCount(LPTMR_BASE);

		LPTMR_ClearStatusFlags(LPTMR_BASE, kLPTMR_TimerCompareFlag);
		tick += counter_delta(now, announced_cycles) / CYCLES_PER_TICK;
		/*
		 * Advance in whole ticks to avoid accumulating sub-tick
		 * ISR latency, which would otherwise cause long-term drift.
		 */
		announced_cycles = (announced_cycles + tick * CYCLES_PER_TICK) & COUNTER_MAX;
	} else {
		LPTMR_ClearStatusFlags(LPTMR_BASE, kLPTMR_TimerCompareFlag);
		cycles += CYCLES_PER_TICK;
	}

	k_spin_unlock(&lock, key);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? tick : 1);
}

static int sys_clock_driver_init(void)
{
	lptmr_config_t config;
	const struct wuc_dt_spec wuc = WUC_DT_SPEC_INST_GET_OR(0, {0});

	if ((wuc.dev != NULL) && (wuc_enable_wakeup_source_dt(&wuc) != 0)) {
		return -EIO;
	}

	LPTMR_GetDefaultConfig(&config);
	config.timerMode = kLPTMR_TimerModeTimeCounter;
#if defined(CONFIG_TICKLESS_KERNEL)
	config.enableFreeRunning = true;
#else
	config.enableFreeRunning = false;
#endif
	config.prescalerClockSource = LPTMR_CLK_SOURCE;
	config.bypassPrescaler = LPTMR_PRESCALER_BYPASS;
	config.value = LPTMR_PRESCALER;

	LPTMR_Init(LPTMR_BASE, &config);

	IRQ_CONNECT(LPTMR_IRQN, LPTMR_IRQ_PRIORITY, mcux_lptmr_timer_isr, NULL, 0);
	irq_enable(LPTMR_IRQN);

	LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	LPTMR_SetTimerPeriod(LPTMR_BASE, CYCLES_PER_TICK);
	LPTMR_StartTimer(LPTMR_BASE);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
