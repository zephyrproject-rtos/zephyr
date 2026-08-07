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

/* Counter maximum value based on resolution */
#define LPTMR_RESOLUTION DT_PROP(LPTMR_NODE, resolution)
#define COUNTER_MAX GENMASK(LPTMR_RESOLUTION - 1, 0)

/*
 * A free-running counter and a compare register that matches on equality, so a
 * value written after the counter has passed it is missed for a whole counter
 * period: a COMPARE_EXACT backend. The core writes the comparator through its
 * verify loop.
 * The counter is narrower than a register, so its width is declared and the
 * core masks every delta to it.
 */
#define TIMER_CORE_BACKEND_COMPARE_EXACT
#define TIMER_CORE_COUNTER_WIDTH LPTMR_RESOLUTION

static inline uint32_t timer_driver_cycle_get(void)
{
	return LPTMR_GetCurrentTimerCount(LPTMR_BASE);
}

static inline void timer_driver_set_compare(uint32_t cycles)
{
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
	LPTMR_SetTimerPeriod(LPTMR_BASE, cycles & COUNTER_MAX);
	LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
}

#include "system_timer_generic.h"

void sys_clock_no_timeout(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* The interrupt masked below is the periodic tick there, and
		 * sys_clock_set_timeout() would not bring it back.
		 */
		return;
	}

	/* Nothing pending: stop taking wakeups. The counter keeps running, so
	 * the cycle getters go on working; sys_clock_idle_exit() and the next
	 * arm both re-enable the interrupt.
	 */
	LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
}

void sys_clock_idle_exit(void)
{
	if (LPTMR_GetEnabledInterrupts(LPTMR_BASE) != kLPTMR_TimerInterruptEnable) {
		LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	}
}

void sys_clock_disable(void)
{
	const struct wuc_dt_spec wuc = WUC_DT_SPEC_GET_OR(LPTMR_NODE, {0});

	if (wuc.dev != NULL) {
		(void)wuc_disable_wakeup_source_dt(&wuc);
	}

	LPTMR_DisableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	LPTMR_StopTimer(LPTMR_BASE);
}

static void mcux_lptmr_timer_isr(const void *arg)
{
	ARG_UNUSED(arg);

	LPTMR_ClearStatusFlags(LPTMR_BASE, kLPTMR_TimerCompareFlag);

	timer_core_announce();
}

static int sys_clock_driver_init(void)
{
	lptmr_config_t config;
	const struct wuc_dt_spec wuc = WUC_DT_SPEC_GET_OR(LPTMR_NODE, {0});

	if ((wuc.dev != NULL) && (wuc_enable_wakeup_source_dt(&wuc) != 0)) {
		return -EIO;
	}

	LPTMR_GetDefaultConfig(&config);
	config.timerMode = kLPTMR_TimerModeTimeCounter;
	/* Free-running in both modes: the core arms an absolute compare, which
	 * a counter that resets on match could not express.
	 */
	config.enableFreeRunning = true;
	config.prescalerClockSource = LPTMR_CLK_SOURCE;
	config.bypassPrescaler = LPTMR_PRESCALER_BYPASS;
	config.value = LPTMR_PRESCALER;

	LPTMR_Init(LPTMR_BASE, &config);

	IRQ_CONNECT(LPTMR_IRQN, LPTMR_IRQ_PRIORITY, mcux_lptmr_timer_isr, NULL, 0);
	irq_enable(LPTMR_IRQN);

	LPTMR_EnableInterrupts(LPTMR_BASE, kLPTMR_TimerInterruptEnable);
	LPTMR_StartTimer(LPTMR_BASE);

	timer_core_init();

	return 0;
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
