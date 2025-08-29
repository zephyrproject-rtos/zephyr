/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lptmr_hw_timer

#include <zephyr/init.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/time_units.h>
#include <fsl_lptmr.h>
#include <zephyr/irq.h>

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
	     "No LPTMR instance enabled in devicetree");

/* Prescaler clock mapping */
#define TO_LPTMR_CLK_SEL(val) _DO_CONCAT(kLPTMR_PrescalerClock_, val)

/* Devicetree properties */
#define LPTMR_BASE ((LPTMR_Type *)(DT_INST_REG_ADDR(0)))
#define LPTMR_CLK_SOURCE TO_LPTMR_CLK_SEL(DT_INST_PROP_OR(0, clk_source, 0))
#define LPTMR_PRESCALER DT_INST_PROP_OR(0, prescale_glitch_filter, 0)
#define LPTMR_IRQN DT_INST_IRQN(0)
#define LPTMR_IRQ_PRIORITY DT_INST_IRQ(0, priority)

/* Timer cycles per tick */
#define CYCLES_PER_TICK ((uint32_t)((uint64_t)sys_clock_hw_cycles_per_sec() \
			/ (uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC))

/* Counter maximum value based on resolution */
#define LPTMR_RESOLUTION DT_INST_PROP(0, resolution)
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
	config.bypassPrescaler = (LPTMR_PRESCALER == 0);
	config.value = (LPTMR_PRESCALER == 0) ? 0 : (LPTMR_PRESCALER - 1);

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
