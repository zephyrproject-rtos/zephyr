/*
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
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
 * - prescaler is set to 1 (LL_LPTIM_PRESCALER_DIV1 in the related register)
 * - using LPTIM AutoReload capability to trig the IRQ (timeout irq)
 * - when timeout irq occurs the counter is already reset
 * - the maximum timeout duration is reached with the lptim_time_base value
 * - with prescaler of 1, the max timeout (lptim_time_base) is 2seconds
 */

static uint32_t lptim_clock_freq = 32000;
static int32_t lptim_time_base;


/* minimum nb of clock cycles to have to set autoreload register correctly */
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

/* For tick accuracy, a specific tick to freq ratio is expected */
/* This check assumes LSI@32KHz or LSE@32768Hz */
#if !defined(CONFIG_STM32_LPTIM_TICK_FREQ_RATIO_OVERRIDE)
#if (((DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(0), 1, bus) == STM32_SRC_LSI) &&	\
		(CONFIG_SYS_CLOCK_TICKS_PER_SEC != 4000)) ||			\
	((DT_CLOCKS_CELL_BY_IDX(DT_DRV_INST(0), 1, bus) == STM32_SRC_LSE) &&	\
		(CONFIG_SYS_CLOCK_TICKS_PER_SEC != 4096)))
#warning Advised tick freq is 4096 for LSE / 4000 for LSI
#endif
#endif /* !CONFIG_STM32_LPTIM_TICK_FREQ_RATIO_OVERRIDE */

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

	ARG_UNUSED(idle);

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	if (ticks == K_TICKS_FOREVER) {
		clock_control_off(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);
		return;
	}

	/* if LPTIM clock was previously stopped, it must now be restored */
	clock_control_on(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);

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
	/* add count unit from the expected nb of Ticks */
	next_arr = next_arr + ((uint32_t)(ticks) * lptim_clock_freq)
			/ CONFIG_SYS_CLOCK_TICKS_PER_SEC - 1;

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

static int sys_clock_driver_init(void)
{
	int err;


	if (!device_is_ready(clk_ctrl)) {
		return -ENODEV;
	}

	/* Enable LPTIM bus clock */
	err = clock_control_on(clk_ctrl, (clock_control_subsys_t) &lptim_clk[0]);
	if (err < 0) {
		return -EIO;
	}

#if defined(LL_APB1_GRP1_PERIPH_LPTIM1)
	LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_LPTIM1);
#elif defined(LL_APB3_GRP1_PERIPH_LPTIM1)
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

	if (IS_ENABLED(DT_PROP(DT_DRV_INST(0), st_static_prescaler))) {
		/*
		 * LPTIM of the stm32, like stm32U5, which has a clock source x2.
		 * A full 16bit LPTIM counter is counting 4s at 2 * 1/32768 (with LSE)
		 * Time base = (4s * freq) - 1
		 */
		lptim_clock_freq = lptim_clock_freq / 2;
	}
	/*
	 * Else, a full 16bit LPTIM counter is counting 2s at 1/32768 (with LSE)
	 * Time base = (2s * freq) - 1
	 */

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
	/* configure the LPTIM prescaler with 1 */
	LL_LPTIM_SetPrescaler(LPTIM, LL_LPTIM_PRESCALER_DIV1);
#ifdef CONFIG_SOC_SERIES_STM32U5X
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

#ifdef CONFIG_SOC_SERIES_STM32U5X
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
#ifdef CONFIG_SOC_SERIES_STM32U5X
	while (LL_LPTIM_IsActiveFlag_DIEROK(LPTIM) == 0) {
	}
	LL_LPTIM_ClearFlag_DIEROK(LPTIM);
#endif
	LL_LPTIM_ClearFlag_ARROK(LPTIM);

	accumulated_lptim_cnt = 0;

#ifndef CONFIG_SOC_SERIES_STM32U5X
	/* Enable the LPTIM counter */
	LL_LPTIM_Enable(LPTIM);
#endif

	/* Set the Autoreload value once the timer is enabled */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* LPTIM is triggered on a LPTIM_TIMEBASE period */
		lptim_set_autoreload(lptim_time_base);
	} else {
		/* LPTIM is triggered on a Tick period */
		lptim_set_autoreload((lptim_clock_freq / CONFIG_SYS_CLOCK_TICKS_PER_SEC) - 1);
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

SYS_INIT(sys_clock_driver_init, PRE_KERNEL_2,
	 CONFIG_SYSTEM_CLOCK_INIT_PRIORITY);
