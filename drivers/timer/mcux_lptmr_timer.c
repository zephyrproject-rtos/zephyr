/*
 * Copyright (c) 2021 Vestas Wind Systems A/S
 * Copyright (c) 2025 NXP
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

#define LPTMR_COUNTER_MAX (0xFFFFFFFFU)


/* Define maximum ticks(By limit to half of counter size, we ensure that
 * timeout calculations will never overflow in sys_clock_set_timeout)
 */
#define MAX_TICKS (LPTMR_COUNTER_MAX / 2) / CYCLES_PER_TICK
/* 32 bit cycle counter */
static volatile uint32_t cycles;
static struct k_spinlock lock;
static uint32_t last_announced_cycles;

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	k_spinlock_key_t key = k_spin_lock(&lock);

	if (idle && (ticks == K_TICKS_FOREVER)) {
		LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
		k_spin_unlock(&lock, key);
		return;
	}

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		k_spin_unlock(&lock, key);
		return;
	}

	if (ticks < 1) {
		ticks = 1;
	} else if (ticks > (int32_t)MAX_TICKS) {
		ticks = (int32_t)MAX_TICKS;
	}
	uint32_t timeout_cycles = (uint32_t)ticks * CYCLES_PER_TICK;
	uint32_t current = LPTMR_GetCurrentTimerCount(LPTMR_BASE);
	/* Automatically handle overflow.
	 * uint32_t is an unsigned 32‑bit integer. In C, unsigned arithmetic
	 * wraps around modulo 2^32.
	 * If current is close to 0xFFFFFFFF and will add timeout_cycles,
	 * the result will wrap around to the low range automatically 
	 * (e.g., 0xFFFFFFF0 + 0x40 → 0x30).
	 * This wraparound behavior is intentional for timers/counters: 
	 * the arithmetic naturally works in a circular space (2^32),
	 * so we can schedule future times without special overflow checks.
	 */
	uint32_t compare = current + timeout_cycles;
	
	LPTMR_SetTimerPeriod(LPTMR_BASE, compare);

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
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		k_spinlock_key_t key = k_spin_lock(&lock);
		uint32_t current_cycles= LPTMR_GetCurrentTimerCount(LPTMR_BASE);
		uint32_t delta_cycles = current_cycles - last_announced_cycles;
		uint32_t elapsed_ticks = delta_cycles / CYCLES_PER_TICK;
		
		k_spin_unlock(&lock, key);
		return elapsed_ticks;
	} else {
		return 0;
	}
}

uint32_t sys_clock_cycle_get_32(void)
{
	return LPTMR_GetCurrentTimerCount(LPTMR_BASE) + cycles;
}

static void mcux_lptmr_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t next_compare;
	uint32_t current;
	uint32_t delta_ticks = 0U;
	k_spinlock_key_t key = k_spin_lock(&lock);

	LPTMR_ClearStatusFlags(LPTMR_BASE, kLPTMR_TimerCompareFlag);

	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		current = LPTMR_GetCurrentTimerCount(LPTMR_BASE);

		uint32_t delta_cycles;

		delta_cycles = current - last_announced_cycles;
		delta_ticks = delta_cycles / CYCLES_PER_TICK;
		last_announced_cycles = current;

		uint32_t next_timeout = MAX_TICKS * CYCLES_PER_TICK;
		next_compare = current + next_timeout;
		cycles += delta_cycles;
	} else {		
		current = LPTMR_GetCurrentTimerCount(LPTMR_BASE);
		next_compare = current + CYCLES_PER_TICK;
		cycles += CYCLES_PER_TICK;
	}
	k_spin_unlock(&lock, key);
	LPTMR_SetTimerPeriod(LPTMR_BASE, next_compare);
	sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL) ? delta_ticks : 1);
}

static int sys_clock_driver_init(void)
{
	lptmr_config_t config;
	uint32_t init_compare;

	LPTMR_GetDefaultConfig(&config);
	config.timerMode = kLPTMR_TimerModeTimeCounter;
	config.enableFreeRunning = true;
	config.prescalerClockSource = LPTMR_CLK_SOURCE;
	config.bypassPrescaler = (LPTMR_PRESCALER == 0);
	config.value = (LPTMR_PRESCALER == 0) ? 0 : (LPTMR_PRESCALER - 1);

	LPTMR_Init(LPTMR_BASE, &config);

	IRQ_CONNECT(LPTMR_IRQN, LPTMR_IRQ_PRIORITY, mcux_lptmr_timer_isr, NULL, 0);
	irq_enable(LPTMR_IRQN);

	LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);

	last_announced_cycles = LPTMR_GetCurrentTimerCount(LPTMR_BASE);
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		init_compare = MAX_TICKS * CYCLES_PER_TICK;
	} else {
		init_compare = CYCLES_PER_TICK;
	}
	LPTMR_SetTimerPeriod(LPTMR_BASE, init_compare);
	LPTMR_StartTimer(LPTMR_BASE);

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
