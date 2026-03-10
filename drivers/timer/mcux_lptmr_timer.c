/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lptmr

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <fsl_lptmr.h>
#include <zephyr/irq.h>

/* Count enabled `nxp,lptmr` instances with `role = "timer"`. */
#define LPTMR_TIMER_ROLE_COUNT(n) + DT_INST_ENUM_HAS_VALUE(n, role, timer)
BUILD_ASSERT((0 DT_INST_FOREACH_STATUS_OKAY(LPTMR_TIMER_ROLE_COUNT)) == 1,
	     "Exactly one enabled `nxp,lptmr` node must have `role = \"timer\"`");

/*
 * Collect devicetree properties from the timer-role instance.
 *
 * COND_CODE_1 expands to the property value for the `role = "timer"` instance
 * and to nothing for all others. The BUILD_ASSERT above guarantees exactly one
 * timer-role node, so DT_INST_FOREACH_STATUS_OKAY produces a single value.
 */
#define LPTMR_TIMER_REG(n) \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, role, timer), \
			(DT_INST_REG_ADDR(n)), ())
#define LPTMR_BASE \
	((LPTMR_Type *)(uintptr_t)(DT_INST_FOREACH_STATUS_OKAY(LPTMR_TIMER_REG)))

#define LPTMR_CLK_SRC(n) \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, role, timer), \
			(DT_INST_PROP_OR(n, clk_source, 0)), ())
#define LPTMR_CLK_SOURCE \
	((lptmr_prescaler_clock_select_t) \
	 (DT_INST_FOREACH_STATUS_OKAY(LPTMR_CLK_SRC)))

#define LPTMR_PRESCALER_VAL(n) \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, role, timer), \
			(DT_INST_PROP_OR(n, prescale_glitch_filter, 0)), ())
#define LPTMR_PRESCALER (DT_INST_FOREACH_STATUS_OKAY(LPTMR_PRESCALER_VAL))

#define LPTMR_BYPASS_VAL(n) \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, role, timer), \
			(DT_INST_PROP_OR(n, prescale_glitch_filter_bypass, 0)), ())
#define LPTMR_PRESCALER_BYPASS ((bool)(DT_INST_FOREACH_STATUS_OKAY(LPTMR_BYPASS_VAL)))

#define LPTMR_TIMER_IRQN(n) \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, role, timer), \
			(DT_INST_IRQN(n)), ())
#define LPTMR_IRQN (DT_INST_FOREACH_STATUS_OKAY(LPTMR_TIMER_IRQN))

#define LPTMR_IRQ_PRI(n) \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, role, timer), \
			(DT_INST_IRQ(n, priority)), ())
#define LPTMR_IRQ_PRIORITY (DT_INST_FOREACH_STATUS_OKAY(LPTMR_IRQ_PRI))

#define LPTMR_RESOLUTION_VAL(n) \
	COND_CODE_1(DT_INST_ENUM_HAS_VALUE(n, role, timer), \
			(DT_INST_PROP(n, resolution)), ())
#define LPTMR_RESOLUTION (DT_INST_FOREACH_STATUS_OKAY(LPTMR_RESOLUTION_VAL))

/* Timer cycles per tick */
#define CYCLES_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() \
			/ (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

/* Counter maximum value based on resolution */
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
	adj = (now - announced_cycles) + (CYCLES_PER_TICK - 1);

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

	if ((int32_t)(next + announced_cycles - now) < MIN_DELAY) {
		next += CYCLES_PER_TICK;
	}

	next += announced_cycles;

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

	now -= announced_cycles;
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
		tick += (now - announced_cycles) / CYCLES_PER_TICK;
		announced_cycles = now;
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
