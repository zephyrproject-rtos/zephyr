/*
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/init.h>
#include <soc.h>
#include <stm32_ll_lptim.h>
#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_system.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/sys/clock.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/pm/policy.h>

#include <zephyr/spinlock.h>

/* Define for grep-ability, even though this will not be used */
#define DT_DRV_COMPAT st_stm32_lptim

#define LPTIM_SYSTIMER_NODE DT_CHOSEN(zephyr_system_timer)

#if DT_NUM_CLOCKS(LPTIM_SYSTIMER_NODE) <= 1
#error "LPTIM source clock must be provided in Device Tree"
#endif

#define LPTIM ((LPTIM_TypeDef *)DT_REG_ADDR(LPTIM_SYSTIMER_NODE))

#if defined(CONFIG_SOC_SERIES_STM32MP1X)
#define LL_LPTIM_ClearFlag_ARRM  LL_LPTIM_ClearFLAG_ARRM
#define LL_LPTIM_ClearFlag_CMPM  LL_LPTIM_ClearFLAG_CMPM
#endif /* CONFIG_SOC_SERIES_STM32MP1X */

#if defined(CONFIG_STM32_HAL2)
#define STM32_LPTIM_OCPOLARITY_HIGH    LL_LPTIM_OCPOLARITY_HIGH
#define STM32_LPTIM_OCPOLARITY_LOW     LL_LPTIM_OCPOLARITY_LOW
#define STM32_LPTIM_PRELOAD_DISABLED   LL_LPTIM_PRELOAD_DISABLED
#define STM32_LPTIM_PRELOAD_ENABLED    LL_LPTIM_PRELOAD_ENABLED
#else /* CONFIG_STM32_HAL2 */
#define STM32_LPTIM_OCPOLARITY_HIGH    LL_LPTIM_OUTPUT_POLARITY_REGULAR
#define STM32_LPTIM_OCPOLARITY_LOW     LL_LPTIM_OUTPUT_POLARITY_INVERSE
#define STM32_LPTIM_PRELOAD_DISABLED   LL_LPTIM_UPDATE_MODE_IMMEDIATE
#define STM32_LPTIM_PRELOAD_ENABLED    LL_LPTIM_UPDATE_MODE_ENDOFPERIOD
#endif /* CONFIG_STM32_HAL2 */

static const struct stm32_pclken lptim_clk[] = STM32_DT_CLOCKS(LPTIM_SYSTIMER_NODE);

static const struct device *const clk_ctrl = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

static const struct reset_dt_spec lptim_reset = RESET_DT_SPEC_GET(LPTIM_SYSTIMER_NODE);

/*
 * Assumptions and limitations:
 *
 * - system clock based on an LPTIM instance, clocked by LSI or LSE
 * - prescaler is set to a 2^value from 1 (division of the LPTIM source clock by 1)
 *   to 128 (division of the LPTIM source clock by 128)
 * - using LPTIM AutoReload capability to trig the IRQ (timeout irq)
 * - when timeout irq occurs the counter is already reset
 * - the maximum timeout duration is reached with the lptim_time_base value
 * - with prescaler of 1, the max timeout (LPTIM_TIMEBASE) is 2 seconds:
 *    0xFFFF / (LSE freq (32768Hz) / 1)
 * - with prescaler of 128, the max timeout (LPTIM_TIMEBASE) is 256 seconds:
 *    0xFFFF / (LSE freq (32768Hz) / 128)
 */

static uint32_t lptim_time_base;
static uint32_t lptim_clock_freq = CONFIG_STM32_LPTIM_CLOCK;
/* The prescaler given by the DTS and to apply to the lptim_clock_freq */
static uint32_t lptim_clock_presc = DT_PROP(LPTIM_SYSTIMER_NODE, st_prescaler);

/* Minimum nb of clock cycles to have to set autoreload register correctly */
#define LPTIM_GUARD_VALUE 2

/* A 32bit value cannot exceed 0xFFFFFFFF/LPTIM_TIMEBASE counting cycles.
 * This is for example about of 65000 x 2000ms when clocked by LSI
 */
static uint32_t accumulated_lptim_cnt;
/* Next autoreload value to set */
static uint32_t autoreload_next;
/* Indicate if the autoreload register is ready for a write */
static bool autoreload_ready = true;
/* Set while sys_clock_idle_enter() holds the LPTIM clock gated off. */
static bool lptim_clock_gated;

#ifdef CONFIG_STM32_LPTIM_STDBY_TIMER

/* This local variable indicates that the timeout was set right before
 * entering standby state.
 *
 * It is used for chips that has to use a separate standby timer in such
 * case because the LPTIM is not clocked in some low power mode state.
 */
static bool timeout_stdby;

/* Cycle counter before entering the standby state. */
static uint32_t lptim_cnt_pre_stdby;

/* Standby timer value before entering the standby state. */
static uint32_t stdby_timer_pre_stdby;

/* Standby timer used for timer while entering the standby state */
static const struct device *stdby_timer = DEVICE_DT_GET(DT_CHOSEN(st_lptim_stdby_timer));

#endif /* CONFIG_STM32_LPTIM_STDBY_TIMER */

/**
 * @brief Enable autonomous clock for the LPTIM instance in use
 *
 * Enables autonomous mode (if supported) for whichever LPTIM instance
 * is configured as the system timer. This allows the LPTIM to continue
 * running in low power modes.
 */
static void lptim_enable_autonomous_mode(void)
{
	const uint32_t lptim_base = (uint32_t)LPTIM;

	switch (lptim_base) {
#if DT_NODE_EXISTS(DT_NODELABEL(lptim1)) && defined(LL_SRDAMR_GRP1_PERIPH_LPTIM1AMEN)
	case DT_REG_ADDR(DT_NODELABEL(lptim1)):
		LL_SRDAMR_GRP1_EnableAutonomousClock(LL_SRDAMR_GRP1_PERIPH_LPTIM1AMEN);
		break;
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(lptim3)) && defined(LL_SRDAMR_GRP1_PERIPH_LPTIM3AMEN)
	case DT_REG_ADDR(DT_NODELABEL(lptim3)):
		LL_SRDAMR_GRP1_EnableAutonomousClock(LL_SRDAMR_GRP1_PERIPH_LPTIM3AMEN);
		break;
#endif
#if DT_NODE_EXISTS(DT_NODELABEL(lptim4)) && defined(LL_SRDAMR_GRP1_PERIPH_LPTIM4AMEN)
	case DT_REG_ADDR(DT_NODELABEL(lptim4)):
		LL_SRDAMR_GRP1_EnableAutonomousClock(LL_SRDAMR_GRP1_PERIPH_LPTIM4AMEN);
		break;
#endif
	default:
		/* Note: LPTIM2, LPTIM5, LPTIM6 do not support autonomous mode */
		break;
	}
}

/**
 * @brief Freeze LPTIM during debug for the instance in use
 *
 * Configures the debug subsystem to freeze the LPTIM counter when the CPU
 * is halted in a debugger. Handles all LPTIM instances across different buses.
 */
static void lptim_freeze_during_debug(void)
{
#ifdef CONFIG_DEBUG
	const uint32_t lptim_base = (uint32_t)LPTIM;

	switch (lptim_base) {
		/* LPTIM1 - can be on APB1_GRP1, APB3_GRP1, or APB7_GRP1 */
#if DT_NODE_EXISTS(DT_NODELABEL(lptim1))
	case DT_REG_ADDR(DT_NODELABEL(lptim1)):
#if defined(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP)
		LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP);
#elif defined(LL_DBGMCU_APB3_GRP1_LPTIM1_STOP)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_LPTIM1_STOP);
#elif defined(LL_DBGMCU_APB7_GRP1_LPTIM1_STOP)
		LL_DBGMCU_APB7_GRP1_FreezePeriph(LL_DBGMCU_APB7_GRP1_LPTIM1_STOP);
#endif
		break;
#endif
		/* LPTIM2 - can be on APB1_GRP1, APB1_GRP2, APB3_GRP1, or APB4_GRP1 */
#if DT_NODE_EXISTS(DT_NODELABEL(lptim2))
	case DT_REG_ADDR(DT_NODELABEL(lptim2)):
#if defined(LL_DBGMCU_APB1_GRP1_LPTIM2_STOP)
		LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_LPTIM2_STOP);
#elif defined(LL_DBGMCU_APB1_GRP2_LPTIM2_STOP)
		LL_DBGMCU_APB1_GRP2_FreezePeriph(LL_DBGMCU_APB1_GRP2_LPTIM2_STOP);
#elif defined(LL_DBGMCU_APB3_GRP1_LPTIM2_STOP)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_LPTIM2_STOP);
#elif defined(LL_DBGMCU_APB4_GRP1_LPTIM2_STOP)
		LL_DBGMCU_APB4_GRP1_FreezePeriph(LL_DBGMCU_APB4_GRP1_LPTIM2_STOP);
#endif
		break;
#endif
		/* LPTIM3 - can be on APB1_GRP2, APB3_GRP1, or APB4_GRP1 */
#if DT_NODE_EXISTS(DT_NODELABEL(lptim3))
	case DT_REG_ADDR(DT_NODELABEL(lptim3)):
#if defined(LL_DBGMCU_APB1_GRP2_LPTIM3_STOP)
		LL_DBGMCU_APB1_GRP2_FreezePeriph(LL_DBGMCU_APB1_GRP2_LPTIM3_STOP);
#elif defined(LL_DBGMCU_APB3_GRP1_LPTIM3_STOP)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_LPTIM3_STOP);
#elif defined(LL_DBGMCU_APB4_GRP1_LPTIM3_STOP)
		LL_DBGMCU_APB4_GRP1_FreezePeriph(LL_DBGMCU_APB4_GRP1_LPTIM3_STOP);
#endif
		break;
#endif
		/* LPTIM4 - can be on APB3_GRP1 or APB4_GRP1 */
#if DT_NODE_EXISTS(DT_NODELABEL(lptim4))
	case DT_REG_ADDR(DT_NODELABEL(lptim4)):
#if defined(LL_DBGMCU_APB3_GRP1_LPTIM4_STOP)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_LPTIM4_STOP);
#elif defined(LL_DBGMCU_APB4_GRP1_LPTIM4_STOP)
		LL_DBGMCU_APB4_GRP1_FreezePeriph(LL_DBGMCU_APB4_GRP1_LPTIM4_STOP);
#endif
		break;
#endif
		/* LPTIM5 - can be on APB3_GRP1 or APB4_GRP1 */
#if DT_NODE_EXISTS(DT_NODELABEL(lptim5))
	case DT_REG_ADDR(DT_NODELABEL(lptim5)): {
#if defined(LL_DBGMCU_APB3_GRP1_LPTIM5_STOP)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_LPTIM5_STOP);
#elif defined(LL_DBGMCU_APB4_GRP1_LPTIM5_STOP)
		LL_DBGMCU_APB4_GRP1_FreezePeriph(LL_DBGMCU_APB4_GRP1_LPTIM5_STOP);
#endif
	}
#endif
	/* LPTIM6 - on APB3_GRP1 */
#if DT_NODE_EXISTS(DT_NODELABEL(lptim6))
	case DT_REG_ADDR(DT_NODELABEL(lptim6)):
#if defined(LL_DBGMCU_APB3_GRP1_LPTIM6_STOP)
		LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_LPTIM6_STOP);
#endif
		break;
#endif
	default:
		break;
	}
#endif /* CONFIG_DEBUG */
}

static inline bool arrm_state_get(void)
{
	return (LL_LPTIM_IsActiveFlag_ARRM(LPTIM) && LL_LPTIM_IsEnabledIT_ARRM(LPTIM));
}

static void lptim_set_autoreload(uint32_t arr)
{
	/* Update autoreload register */
	autoreload_next = arr;

	if (!autoreload_ready) {
		return;
	}

	/* The ARR register ready, we could set it directly */
	if ((arr > 0) && (arr != LL_LPTIM_GetAutoReload(LPTIM))) {
		/* The new autoreload value change, we set it */
		autoreload_ready = false;
		LL_LPTIM_ClearFlag_ARROK(LPTIM);
		LL_LPTIM_SetAutoReload(LPTIM, arr);
	}
}

static inline uint32_t z_clock_lptim_getcounter(void)
{
	uint32_t lp_time;
	uint32_t lp_time_prev_read;

	/* It should be noted that to read reliably the content
	 * of the LPTIM_CNT register, two successive read accesses
	 * must be performed and compared
	 */
	lp_time = LL_LPTIM_GetCounter(LPTIM);
	do {
		lp_time_prev_read = lp_time;
		lp_time = LL_LPTIM_GetCounter(LPTIM);
	} while (lp_time != lp_time_prev_read);
	return lp_time;
}

static uint32_t sys_clock_lp_time_get(void)
{
	uint32_t lp_time;

	do {
		/* In case of counter roll-over, add the autoreload value,
		 * because the irq has not yet been handled
		 */
		if (arrm_state_get()) {
			lp_time = LL_LPTIM_GetAutoReload(LPTIM) + 1;
			lp_time += z_clock_lptim_getcounter();
			break;
		}

		lp_time = z_clock_lptim_getcounter();

		/* Check if the flag ARRM wasn't be set during the process */
	} while (arrm_state_get());

	return lp_time;
}

/*
 * The counter is auto-reloading, so a RELOAD backend. ARR is not a plain
 * interval though: writing it does not restart the counter, so an interval of
 * `rel` from now is programmed as ARR = CNT + rel - 1, the hardware matching on
 * equality and wrapping to zero. The period the hardware then repeats on its
 * own is ARR + 1, which is exactly one tick when timer_core_init() arms from a
 * counter still at zero, so a ticked kernel needs no re-arm.
 *
 * The counter domain is the LPTIM clock, whose rate is only known once the
 * source and the prescaler have been resolved at init, and the arm range is
 * the 16 bit ARR, narrowed further by the st,timeout property.
 *
 * sys_clock_cycle_get_32() stays the driver's own, as it must for a counter
 * running at a rate of its own.
 */
#define TIMER_CORE_BACKEND_RELOAD
#define TIMER_CORE_CYCLES_PER_SEC   lptim_clock_freq
#define TIMER_CORE_CYCLES_PER_SEC_RUNTIME
#define TIMER_CORE_ALARM_MAX_CYCLES lptim_time_base
#define TIMER_CORE_ALARM_MIN_CYCLES (LPTIM_GUARD_VALUE + 1)
#define TIMER_CORE_HAVE_CYCLE_GET_32

static inline uint32_t timer_driver_cycle_get(void)
{
	return accumulated_lptim_cnt + sys_clock_lp_time_get();
}

/* Undo the gating done by sys_clock_idle_enter(). Both the wake path and the
 * next arming call this: the core programs a reload only when the deadline
 * actually moves, so a wake that re-arms the deadline it went to sleep on would
 * otherwise leave the timer unclocked.
 */
static void lptim_clock_restore(void)
{
	if (lptim_clock_gated) {
		lptim_clock_gated = false;
		clock_control_on(clk_ctrl, (clock_control_subsys_t)&lptim_clk[0]);
	}
}

static void timer_driver_set_reload(uint32_t rel)
{
	uint32_t lp_time, autoreload, next_arr;

	lptim_clock_restore();

	lp_time = z_clock_lptim_getcounter();
	autoreload = LL_LPTIM_GetAutoReload(LPTIM);

	if (LL_LPTIM_IsActiveFlag_ARRM(LPTIM) ||
	    ((autoreload - lp_time) < LPTIM_GUARD_VALUE)) {
		/* The period is ending; ARR can no longer be moved. The
		 * announce that follows re-arms with the deadline still due.
		 */
		return;
	}

	/* The core floors rel at LPTIM_GUARD_VALUE + 1, so the match lands at
	 * least LPTIM_GUARD_VALUE ahead of the counter and cannot be missed.
	 */
	next_arr = MIN(lp_time + rel - 1, lptim_time_base);

	lptim_set_autoreload(next_arr);
}

#include "system_timer_generic.h"

void sys_clock_idle_enter(uint32_t ticks)
{
	if (ticks == SYS_CLOCK_IDLE_FOREVER) {
		/* Nothing to wake up for and the uptime may drift: gate the
		 * LPTIM off. It is unclocked from here and cannot wake the CPU.
		 * This timer serves a single CPU, so nothing else can observe
		 * sys_clock_cycle_get_32() standing still.
		 */
		lptim_clock_gated = true;
		clock_control_off(clk_ctrl, (clock_control_subsys_t)&lptim_clk[0]);
		return;
	}

#ifdef CONFIG_STM32_LPTIM_STDBY_TIMER
	const struct pm_state_info *next;

	next = pm_policy_next_state(_current_cpu->id, ticks);

	/* Check if STANBY or STOP3 is requested */
	timeout_stdby = false;
	if (next != NULL) {
#ifdef CONFIG_PM_S2RAM
		if (next->state == PM_STATE_SUSPEND_TO_RAM) {
			timeout_stdby = true;
		}
#endif
#ifdef CONFIG_STM32_STOP3_LP_MODE
		if ((next->state == PM_STATE_SUSPEND_TO_IDLE) && (next->substate_id == 4)) {
			timeout_stdby = true;
		}
#endif
	}

	if (timeout_stdby) {
		uint64_t timeout_us = k_ticks_to_us_ceil64(ticks);

		struct counter_alarm_cfg cfg = {
			.callback = NULL,
			.ticks = counter_us_to_ticks(stdby_timer, timeout_us),
			.user_data = NULL,
			.flags = 0,
		};

		/* Set the alarm using timer that runs the standby.
		 * Needed rump-up/setting time, lower accurency etc. should be
		 * included in the exit-latency in the power state definition.
		 */
		counter_cancel_channel_alarm(stdby_timer, 0);
		counter_set_channel_alarm(stdby_timer, 0, &cfg);

		/* Store current values to calculate a difference in
		 * measurements after exiting the standby state.
		 */
		counter_get_value(stdby_timer, &stdby_timer_pre_stdby);
		lptim_cnt_pre_stdby = z_clock_lptim_getcounter();

		LL_LPTIM_DisableIT_ARROK(LPTIM);
		LL_LPTIM_ClearFlag_ARROK(LPTIM);
		NVIC_ClearPendingIRQ(DT_IRQN(LPTIM_SYSTIMER_NODE));
		/* Stop clocks for LPTIM, since RTC is used instead */
		clock_control_off(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);

		return;
	}
#endif /* CONFIG_STM32_LPTIM_STDBY_TIMER */

	sys_clock_set_timeout(ticks, false);
}

static void lptim_irq_handler(const struct device *unused)
{

	ARG_UNUSED(unused);

	uint32_t autoreload = LL_LPTIM_GetAutoReload(LPTIM);

	if ((LL_LPTIM_IsActiveFlag_ARROK(LPTIM) != 0)
		&& LL_LPTIM_IsEnabledIT_ARROK(LPTIM) != 0) {
		LL_LPTIM_ClearFlag_ARROK(LPTIM);
		if ((autoreload_next > 0) && (autoreload_next != autoreload)) {
			/* the new autoreload value change, we set it */
			autoreload_ready = false;
			LL_LPTIM_SetAutoReload(LPTIM, autoreload_next);
		} else {
			autoreload_ready = true;
		}
	}

	if (arrm_state_get()) {
		k_spinlock_key_t key = sys_clock_lock();

		/* Clear the flag first: sys_clock_lp_time_get() adds a period
		 * of its own while it is set, and the period is accounted here.
		 */
		LL_LPTIM_ClearFlag_ARRM(LPTIM);
		accumulated_lptim_cnt += autoreload + 1;

		timer_core_announce_from(key);
	}
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* timer_core_cycle_get() takes the clock lock, which the count needs:
	 * it is synthesized from two variables the ISR also writes. It is in
	 * LPTIM units; the kernel wants sys_clock_hw_cycles_per_sec() ones.
	 */
	return (uint32_t)((timer_core_cycle_get() * sys_clock_hw_cycles_per_sec()) /
			  lptim_clock_freq);
}

/* Wait for the IER register to be ready, after any bit write operation */
void stm32_lptim_wait_ready(void)
{
#if defined(LL_LPTIM_ISR_DIEROK)
	while (LL_LPTIM_IsActiveFlag_DIEROK(LPTIM) == 0) {
	}
	LL_LPTIM_ClearFlag_DIEROK(LPTIM);
#else
	/* Empty : not relevant */
#endif
}

static int sys_clock_driver_init(void)
{
	int err;

	/* Reset timer to default state using RCC */
	(void)reset_line_toggle_dt(&lptim_reset);

	/* Enable LPTIM bus clock */
	err = clock_control_on(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);
	if (err < 0) {
		return -EIO;
	}

	/* Enable autonomous mode for the LPTIM instance in use */
	lptim_enable_autonomous_mode();

	/* Enable LPTIM clock source */
	err = clock_control_configure(clk_ctrl,
				      (clock_control_subsys_t) &lptim_clk[1],
				      NULL);
	if (err < 0) {
		return -EIO;
	}

	/* Get LPTIM clock freq */
	err = clock_control_get_rate(clk_ctrl, (clock_control_subsys_t) &lptim_clk[1],
			       &lptim_clock_freq);

	if (err < 0) {
		return -EIO;
	}
#if defined(CONFIG_SOC_SERIES_STM32L0X)
	/* Driver only supports freqs up to 32768Hz. On L0, LSI freq is 37KHz,
	 * which will overflow the LPTIM counter.
	 * Previous LPTIM configuration using device tree was doing forcing this
	 * with a Kconfig default. Impact is that time is 1.13 faster than reality.
	 * Following lines reproduce this behavior in order not to change behavior.
	 * This issue will be fixed by implementation LPTIM prescaler support.
	 */
	if (lptim_clk[1].bus == STM32_SRC_LSI) {
		lptim_clock_freq = KHZ(32);
	}
#endif

#if DT_NODE_HAS_PROP(LPTIM_SYSTIMER_NODE, st_timeout)
	uint32_t timeout = DT_PROP(LPTIM_SYSTIMER_NODE, st_timeout);

	if (timeout > (lptim_clock_presc * 0xFFFF) / lptim_clock_freq) {
		__ASSERT(0,
			"st,timeout can't be higher than range defined by LPTIM presc and freq");
		return -EIO;
	}


	/*
	 * Define the lptim_time_base that should be set to expire at "timeout" seconds
	 * running counter at (lptim_clock_freq divided by lptim_clock_presc) Hz
	 */
	lptim_time_base = (lptim_clock_freq * timeout) / lptim_clock_presc;
#else
	/* Set LPTIM time base based on clock source freq */
	if (lptim_clock_freq == KHZ(32)) {
		lptim_time_base = 0xF9FF;
	} else if (lptim_clock_freq == 32768) {
		lptim_time_base = 0xFFFF;
	} else {
		return -EIO;
	}

#endif /* st_timeout */

#if !defined(CONFIG_STM32_LPTIM_TICK_FREQ_RATIO_OVERRIDE)
	/*
	 * Check coherency between CONFIG_SYS_CLOCK_TICKS_PER_SEC
	 * and the lptim_clock_freq which is the CONFIG_STM32_LPTIM_CLOCK reduced
	 * by the lptim_clock_presc
	 */
	if (lptim_clock_presc <= 8) {
		__ASSERT(CONFIG_STM32_LPTIM_CLOCK / 8 >= CONFIG_SYS_CLOCK_TICKS_PER_SEC,
		 "It is recommended to set SYS_CLOCK_TICKS_PER_SEC to CONFIG_STM32_LPTIM_CLOCK/8");
	} else {
		__ASSERT(CONFIG_STM32_LPTIM_CLOCK / lptim_clock_presc >=
			CONFIG_SYS_CLOCK_TICKS_PER_SEC,
		 "Set SYS_CLOCK_TICKS_PER_SEC to CONFIG_STM32_LPTIM_CLOCK/lptim_clock_presc");
	}
#endif /* !CONFIG_STM32_LPTIM_TICK_FREQ_RATIO_OVERRIDE */

	/* Actual lptim clock freq when the clock source is reduced by the prescaler */
	lptim_clock_freq = lptim_clock_freq / lptim_clock_presc;

	/* Clear the event flag and possible pending interrupt */
	IRQ_CONNECT(DT_IRQN(LPTIM_SYSTIMER_NODE),
		    DT_IRQ(LPTIM_SYSTIMER_NODE, priority),
		    lptim_irq_handler, 0, 0);
	irq_enable(DT_IRQN(LPTIM_SYSTIMER_NODE));

#ifdef CONFIG_SOC_SERIES_STM32WLX
	/* Enable the LPTIM wakeup EXTI line */
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_29);
#endif

	/* configure the LPTIM counter */
	LL_LPTIM_SetClockSource(LPTIM, LL_LPTIM_CLK_SOURCE_INTERNAL);
	/* the LPTIM clock freq is affected by the prescaler */
	LL_LPTIM_SetPrescaler(LPTIM, (__CLZ(__RBIT(lptim_clock_presc)) << LPTIM_CFGR_PRESC_Pos));

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_lptim)
	LL_LPTIM_OC_SetPolarity(LPTIM, LL_LPTIM_CHANNEL_CH1, STM32_LPTIM_OCPOLARITY_HIGH);
#else /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_lptim) */
	LL_LPTIM_SetPolarity(LPTIM, STM32_LPTIM_OCPOLARITY_HIGH);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_lptim) */
	LL_LPTIM_SetUpdateMode(LPTIM, STM32_LPTIM_PRELOAD_DISABLED);
	LL_LPTIM_SetCounterMode(LPTIM, LL_LPTIM_COUNTER_MODE_INTERNAL);
	LL_LPTIM_DisableTimeout(LPTIM);
	/* counting start is initiated by software */
	LL_LPTIM_TrigSw(LPTIM);

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_lptim)
	/* Enable the LPTIM before proceeding with configuration */
	LL_LPTIM_Enable(LPTIM);

	LL_LPTIM_DisableIT_CC1(LPTIM);
	stm32_lptim_wait_ready();
	LL_LPTIM_ClearFlag_CC1(LPTIM);
#else /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_lptim) */
	/* LPTIM interrupt set-up before enabling */
	/* no Compare match Interrupt */
	LL_LPTIM_DisableIT_CMPM(LPTIM);
	LL_LPTIM_ClearFlag_CMPM(LPTIM);
#endif /* DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_lptim) */

	/* Autoreload match Interrupt */
	LL_LPTIM_EnableIT_ARRM(LPTIM);
	stm32_lptim_wait_ready();
	LL_LPTIM_ClearFlag_ARRM(LPTIM);

	/* ARROK bit validates the write operation to ARR register */
	autoreload_ready = true;
	LL_LPTIM_EnableIT_ARROK(LPTIM);
	stm32_lptim_wait_ready();
	LL_LPTIM_ClearFlag_ARROK(LPTIM);

#if !DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_lptim)
	/* Enable the LPTIM counter */
	LL_LPTIM_Enable(LPTIM);
#endif /* !DT_HAS_COMPAT_STATUS_OKAY(st_stm32u5_lptim) */

	/* Program the one-tick period before the counter starts. ARR is at its
	 * reset value of zero here, which reads as a match about to happen, so
	 * timer_core_init() below could not move it. A ticked kernel keeps this
	 * period for good; a tickless one has it replaced at the first arming.
	 */
	lptim_set_autoreload((lptim_clock_freq / CONFIG_SYS_CLOCK_TICKS_PER_SEC) - 1);

	/* Start the LPTIM counter in continuous mode */
	LL_LPTIM_StartCounter(LPTIM, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

	/* Freeze LPTIM during debug */
	lptim_freeze_during_debug();

	timer_core_init();

	return 0;
}

void stm32_clock_control_standby_exit(void)
{
#ifdef CONFIG_STM32_LPTIM_STDBY_TIMER
	if (clock_control_get_status(clk_ctrl,
				     (clock_control_subsys_t) &lptim_clk[0])
				     != CLOCK_CONTROL_STATUS_ON) {
		sys_clock_driver_init();
	}
#endif /* CONFIG_STM32_LPTIM_STDBY_TIMER */
}

void sys_clock_idle_exit(void)
{
	lptim_clock_restore();

#ifdef CONFIG_STM32_LPTIM_STDBY_TIMER
	if (timeout_stdby) {
		uint64_t missed_lptim_cnt;
		uint32_t stdby_timer_diff, stdby_timer_post;
		uint64_t stdby_timer_us;

		/* Get current value for standby timer and reset LPTIM counter value
		 * to start anew.
		 */
		counter_get_value(stdby_timer, &stdby_timer_post);

		/* Calculate how much time has passed since last measurement for standby timer */
		/* Check IDLE timer overflow */
		if (stdby_timer_pre_stdby > stdby_timer_post) {
			stdby_timer_diff =
				(counter_get_top_value(stdby_timer) - stdby_timer_pre_stdby) +
				stdby_timer_post + 1;

		} else {
			stdby_timer_diff = stdby_timer_post - stdby_timer_pre_stdby;
		}
		stdby_timer_us = counter_ticks_to_us(stdby_timer, stdby_timer_diff);

		/* Convert standby time in LPTIM cnt */
		missed_lptim_cnt = (CONFIG_STM32_LPTIM_CLOCK * stdby_timer_us) /
				   USEC_PER_SEC;
		/* Add the LPTIM cnt pre standby */
		missed_lptim_cnt += lptim_cnt_pre_stdby;

		/* Hand the standby span to the core directly rather than folding
		 * it into the counter for the announce to find: the counter is 32
		 * bits and a standby can outrun what the masked delta resolves.
		 * The counter still takes the same span, so the two stay in step.
		 */
		k_spinlock_key_t key = sys_clock_lock();

		LL_LPTIM_ResetCounter(LPTIM);
		accumulated_lptim_cnt += (uint32_t)missed_lptim_cnt;

		timer_core_announce_cycles64_from(key, missed_lptim_cnt);

		/* We've already performed all needed operations */
		timeout_stdby = false;
	}
#endif /* CONFIG_STM32_LPTIM_STDBY_TIMER */
}

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
