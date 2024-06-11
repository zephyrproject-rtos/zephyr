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
#include <zephyr/sys_clock.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/pm/policy.h>

#include <zephyr/spinlock.h>

#define DT_DRV_COMPAT st_stm32_lptim

#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) > 1
#error Only one LPTIM instance should be enabled
#endif

#define LPTIM (LPTIM_TypeDef *) DT_INST_REG_ADDR(0)

#if DT_INST_NUM_CLOCKS(0) == 1
#warning Kconfig for LPTIM source clock (LSI/LSE) is deprecated, use device tree.
static const struct stm32_pclken lptim_clk[] = {
	STM32_CLOCK_INFO(0, DT_DRV_INST(0)),
	/* Use Kconfig to configure source clocks fields */
	/* Fortunately, values are consistent across enabled series */
#ifdef CONFIG_STM32_LPTIM_CLOCK_LSI
	{.bus = STM32_SRC_LSI, .enr = LPTIM1_SEL(1)}
#else
	{.bus = STM32_SRC_LSE, .enr = LPTIM1_SEL(3)}
#endif
};
#else
static const struct stm32_pclken lptim_clk[] = STM32_DT_INST_CLOCKS(0);
#endif

static const struct device *const clk_ctrl = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

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

static int32_t lptim_time_base;
static uint32_t lptim_clock_freq = CONFIG_STM32_LPTIM_CLOCK;
/* The prescaler given by the DTS and to apply to the lptim_clock_freq */
static uint32_t lptim_clock_presc = DT_PROP(DT_DRV_INST(0), st_prescaler);

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

static struct k_spinlock lock;

#ifdef CONFIG_STM32_LPTIM_STDBY_TIMER

#define CURRENT_CPU \
	(COND_CODE_1(CONFIG_SMP, (arch_curr_cpu()->id), (_current_cpu->id)))

#define cycle_t uint32_t

/* This local variable indicates that the timeout was set right before
 * entering standby state.
 *
 * It is used for chips that has to use a separate standby timer in such
 * case because the LPTIM is not clocked in some low power mode state.
 */
static bool timeout_stdby;

/* Cycle counter before entering the standby state. */
static cycle_t lptim_cnt_pre_stdby;

/* Standby timer value before entering the standby state. */
static uint32_t stdby_timer_pre_stdby;

/* Standby timer used for timer while entering the standby state */
static const struct device *stdby_timer = DEVICE_DT_GET(DT_CHOSEN(st_lptim_stdby_timer));

#endif /* CONFIG_STM32_LPTIM_STDBY_TIMER */

static inline bool arrm_state_get(void)
{
	return (LL_LPTIM_IsActiveFlag_ARRM(LPTIM) && LL_LPTIM_IsEnabledIT_ARRM(LPTIM));
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
		k_spinlock_key_t key = k_spin_lock(&lock);

		/* do not change ARR yet, sys_clock_announce will do */
		LL_LPTIM_ClearFLAG_ARRM(LPTIM);

		/* increase the total nb of autoreload count
		 * used in the sys_clock_cycle_get_32() function.
		 */
		autoreload++;

		accumulated_lptim_cnt += autoreload;

		k_spin_unlock(&lock, key);

		/* announce the elapsed time in ms (count register is 16bit) */
		uint32_t dticks = (autoreload
				* CONFIG_SYS_CLOCK_TICKS_PER_SEC)
				/ lptim_clock_freq;

		sys_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL)
				? dticks : (dticks > 0));
	}
}

static void lptim_set_autoreload(uint32_t arr)
{
	/* Update autoreload register */
	autoreload_next = arr;

	if (!autoreload_ready)
		return;

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

void sys_clock_set_timeout(int32_t ticks, bool idle)
{
	/* new LPTIM AutoReload value to set (aligned on Kernel ticks) */
	uint32_t next_arr = 0;
	int err;

	ARG_UNUSED(idle);

#ifdef CONFIG_STM32_LPTIM_STDBY_TIMER
	const struct pm_state_info *next;

	next = pm_policy_next_state(CURRENT_CPU, ticks);

	/* Check if STANBY or STOP3 is requested */
	timeout_stdby = false;
	if ((next != NULL) && idle) {
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
		uint64_t timeout_us =
			((uint64_t)ticks * USEC_PER_SEC) / CONFIG_SYS_CLOCK_TICKS_PER_SEC;

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

		/* Stop clocks for LPTIM, since RTC is used instead */
		clock_control_off(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);

		return;
	}
#endif /* CONFIG_STM32_LPTIM_STDBY_TIMER */

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	/*
	 * When CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE = y, ticks equals to -1
	 * is treated as a lptim off ; never waking up ; lptim not clocked anymore
	 */
	if (ticks == K_TICKS_FOREVER) {
		clock_control_off(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);
		return;
	}
	/*
	 * When CONFIG_SYSTEM_CLOCK_SLOPPY_IDLE = n, ticks equals to INT_MAX
	 * is treated as a maximum possible value LPTIM_MAX_TIMEBASE (16bit counter)
	 */

	/* if LPTIM clock was previously stopped, it must now be restored */
	err = clock_control_on(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);

	if (err < 0) {
		return;
	}
	/* passing ticks==1 means "announce the next tick",
	 * ticks value of zero (or even negative) is legal and
	 * treated identically: it simply indicates the kernel would like the
	 * next tick announcement as soon as possible.
	 */
	ticks = CLAMP(ticks - 1, 1, lptim_time_base);

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* read current counter value (cannot exceed 16bit) */

	uint32_t lp_time = z_clock_lptim_getcounter();

	uint32_t autoreload = LL_LPTIM_GetAutoReload(LPTIM);

	if (LL_LPTIM_IsActiveFlag_ARRM(LPTIM)
	    || ((autoreload - lp_time) < LPTIM_GUARD_VALUE)) {
		/* interrupt happens or happens soon.
		 * It's impossible to set autoreload value.
		 */
		k_spin_unlock(&lock, key);
		return;
	}

	/* calculate the next arr value (cannot exceed 16bit)
	 * adjust the next ARR match value to align on Ticks
	 * from the current counter value to first next Tick
	 */
	next_arr = (((lp_time * CONFIG_SYS_CLOCK_TICKS_PER_SEC)
			/ lptim_clock_freq) + 1) * lptim_clock_freq
			/ (CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	next_arr = next_arr + ((uint32_t)(ticks) * lptim_clock_freq)
			/ CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	/* if the lptim_clock_freq <  one ticks/sec, then next_arr must be > 0 */

	/* maximise to TIMEBASE */
	if (next_arr > lptim_time_base) {
		next_arr = lptim_time_base;
	}
	/* The new autoreload value must be LPTIM_GUARD_VALUE clock cycles
	 * after current lptim to make sure we don't miss
	 * an autoreload interrupt
	 */
	else if (next_arr < (lp_time + LPTIM_GUARD_VALUE)) {
		next_arr = lp_time + LPTIM_GUARD_VALUE;
	}
	/* with slow lptim_clock_freq, LPTIM_GUARD_VALUE of 1 is enough */
	next_arr = next_arr - 1;

	/* Update autoreload register */
	lptim_set_autoreload(next_arr);

	k_spin_unlock(&lock, key);
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


uint32_t sys_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t lp_time = sys_clock_lp_time_get();

	k_spin_unlock(&lock, key);

	/* gives the value of LPTIM counter (ms)
	 * since the previous 'announce'
	 */
	uint64_t ret = ((uint64_t)lp_time * CONFIG_SYS_CLOCK_TICKS_PER_SEC) / lptim_clock_freq;

	return (uint32_t)(ret);
}

uint32_t sys_clock_cycle_get_32(void)
{
	/* just gives the accumulated count in a number of hw cycles */

	k_spinlock_key_t key = k_spin_lock(&lock);

	uint32_t lp_time = sys_clock_lp_time_get();

	lp_time += accumulated_lptim_cnt;

	/* convert lptim count in a nb of hw cycles with precision */
	uint64_t ret = ((uint64_t)lp_time * sys_clock_hw_cycles_per_sec()) / lptim_clock_freq;

	k_spin_unlock(&lock, key);

	/* convert in hw cycles (keeping 32bit value) */
	return (uint32_t)(ret);
}

/* Wait for the IER register of the stm32U5 ready, after any bit write operation */
void stm32_lptim_wait_ready(void)
{
#ifdef CONFIG_SOC_SERIES_STM32U5X
	while (LL_LPTIM_IsActiveFlag_DIEROK(LPTIM) == 0) {
	}
	LL_LPTIM_ClearFlag_DIEROK(LPTIM);
#else
	/* Empty : not relevant */
#endif
}

int init_sys_clock_driver(void)
{
	uint32_t count_per_tick;
	int err;

	if (!device_is_ready(clk_ctrl)) {
		return -ENODEV;
	}

	/* Enable LPTIM bus clock */
	err = clock_control_on(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);
	if (err < 0) {
		return -EIO;
	}

#if defined(LL_SRDAMR_GRP1_PERIPH_LPTIM1AMEN)
	LL_SRDAMR_GRP1_EnableAutonomousClock(LL_SRDAMR_GRP1_PERIPH_LPTIM1AMEN);
#endif

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

	/* Set LPTIM time base based on clock source freq */
	if (lptim_clock_freq == KHZ(32)) {
		lptim_time_base = 0xF9FF;
	} else if (lptim_clock_freq == 32768) {
		lptim_time_base = 0xFFFF;
	} else {
		return -EIO;
	}

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
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    lptim_irq_handler, 0, 0);
	irq_enable(DT_INST_IRQN(0));

#ifdef CONFIG_SOC_SERIES_STM32WLX
	/* Enable the LPTIM wakeup EXTI line */
	LL_EXTI_EnableIT_0_31(LL_EXTI_LINE_29);
#endif

	/* configure the LPTIM counter */
	LL_LPTIM_SetClockSource(LPTIM, LL_LPTIM_CLK_SOURCE_INTERNAL);
	/* the LPTIM clock freq is affected by the prescaler */
	LL_LPTIM_SetPrescaler(LPTIM, (__CLZ(__RBIT(lptim_clock_presc)) << LPTIM_CFGR_PRESC_Pos));

#if defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBAX)
	LL_LPTIM_OC_SetPolarity(LPTIM, LL_LPTIM_CHANNEL_CH1,
				LL_LPTIM_OUTPUT_POLARITY_REGULAR);
#else
	LL_LPTIM_SetPolarity(LPTIM, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
#endif
	LL_LPTIM_SetUpdateMode(LPTIM, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
	LL_LPTIM_SetCounterMode(LPTIM, LL_LPTIM_COUNTER_MODE_INTERNAL);
	LL_LPTIM_DisableTimeout(LPTIM);
	/* counting start is initiated by software */
	LL_LPTIM_TrigSw(LPTIM);

#if defined(CONFIG_SOC_SERIES_STM32U5X) || \
	defined(CONFIG_SOC_SERIES_STM32H5X) || \
	defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* Enable the LPTIM before proceeding with configuration */
	LL_LPTIM_Enable(LPTIM);

	LL_LPTIM_DisableIT_CC1(LPTIM);
	stm32_lptim_wait_ready();
	LL_LPTIM_ClearFLAG_CC1(LPTIM);
#else
	/* LPTIM interrupt set-up before enabling */
	/* no Compare match Interrupt */
	LL_LPTIM_DisableIT_CMPM(LPTIM);
	LL_LPTIM_ClearFLAG_CMPM(LPTIM);
#endif

	/* Autoreload match Interrupt */
	LL_LPTIM_EnableIT_ARRM(LPTIM);
	stm32_lptim_wait_ready();
	LL_LPTIM_ClearFLAG_ARRM(LPTIM);

	/* ARROK bit validates the write operation to ARR register */
	LL_LPTIM_EnableIT_ARROK(LPTIM);
	stm32_lptim_wait_ready();
	LL_LPTIM_ClearFlag_ARROK(LPTIM);

#if !defined(CONFIG_SOC_SERIES_STM32U5X) && \
	!defined(CONFIG_SOC_SERIES_STM32H5X) && \
	!defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* Enable the LPTIM counter */
	LL_LPTIM_Enable(LPTIM);
#endif

	/* Set the Autoreload value once the timer is enabled */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* LPTIM is triggered on a LPTIM_TIMEBASE period */
		lptim_set_autoreload(lptim_time_base);
	} else {
		/* nb of LPTIM counter unit per kernel tick (depends on lptim clock prescaler) */
		count_per_tick = (lptim_clock_freq / CONFIG_SYS_CLOCK_TICKS_PER_SEC);
		/* LPTIM is triggered on a Tick period */
		lptim_set_autoreload(count_per_tick - 1);
	}

	/* Start the LPTIM counter in continuous mode */
	LL_LPTIM_StartCounter(LPTIM, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

#ifdef CONFIG_DEBUG
	/* stop LPTIM during DEBUG */
#if defined(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP)
	LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP);
#elif defined(LL_DBGMCU_APB3_GRP1_LPTIM1_STOP)
	LL_DBGMCU_APB3_GRP1_FreezePeriph(LL_DBGMCU_APB3_GRP1_LPTIM1_STOP);
#endif

#endif
	return 0;
}

void stm32_clock_control_standby_exit(void)
{
#ifdef CONFIG_STM32_LPTIM_STDBY_TIMER
	if (clock_control_get_status(clk_ctrl,
				     (clock_control_subsys_t) &lptim_clk[0])
				     != CLOCK_CONTROL_STATUS_ON) {
		init_sys_clock_driver();
	}
#endif /* CONFIG_STM32_LPTIM_STDBY_TIMER */
}

void sys_clock_idle_exit(void)
{
#ifdef CONFIG_STM32_LPTIM_STDBY_TIMER
	if (timeout_stdby) {
		cycle_t missed_lptim_cnt;
		uint32_t stdby_timer_diff, stdby_timer_post, dticks;
		uint64_t stdby_timer_us;

		/* Get current value for standby timer and reset LPTIM counter value
		 * to start anew.
		 */
		LL_LPTIM_ResetCounter(LPTIM);
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
		missed_lptim_cnt = (sys_clock_hw_cycles_per_sec() * stdby_timer_us) /
				   USEC_PER_SEC;
		/* Add the LPTIM cnt pre standby */
		missed_lptim_cnt += lptim_cnt_pre_stdby;

		/* Update the cycle counter to include the cycles missed in standby */
		accumulated_lptim_cnt += missed_lptim_cnt;

		/* Announce the passed ticks to the kernel */
		dticks = (missed_lptim_cnt * CONFIG_SYS_CLOCK_TICKS_PER_SEC)
				/ lptim_clock_freq;
		sys_clock_announce(dticks);

		/* We've already performed all needed operations */
		timeout_stdby = false;
	}
#endif /* CONFIG_STM32_LPTIM_STDBY_TIMER */
}
