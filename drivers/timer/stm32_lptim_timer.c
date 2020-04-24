/*
 * Copyright (c) 2018 Foundries.io Ltd
 * Copyright (c) 2019 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/timer/system_timer.h>
#include <sys_clock.h>

#include <spinlock.h>

/*
 * Assumptions and limitations:
 *
 * - system clock based on an LPTIM1 instance, clocked by LSI or LSE
 * - prescaler is set to 1 (LL_LPTIM_PRESCALER_DIV1 in the related register)
 * - using LPTIM1 AutoReload capability to trig the IRQ (timeout irq)
 * - when timeout irq occurs the counter is already reset
 * - the maximum timeout duration is reached with the LPTIM_TIMEBASE value
 * - with prescaler of 1, the max timeout (LPTIM_TIMEBASE) is 2seconds
 */

#define LPTIM_CLOCK CONFIG_STM32_LPTIM_CLOCK
#define LPTIM_TIMEBASE CONFIG_STM32_LPTIM_TIMEBASE

/* nb of LPTIM counter unit per kernel tick  */
#define COUNT_PER_TICK (LPTIM_CLOCK / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* A 32bit value cannot exceed 0xFFFFFFFF/LPTIM_TIMEBASE counting cycles.
 * This is for example about of 65000 x 2000ms when clocked by LSI
 */
static u32_t accumulated_lptim_cnt;

static struct k_spinlock lock;

static void lptim_irq_handler(struct device *unused)
{

	ARG_UNUSED(unused);

	if ((LL_LPTIM_IsActiveFlag_ARRM(LPTIM1) != 0)
		&& LL_LPTIM_IsEnabledIT_ARRM(LPTIM1) != 0) {

		k_spinlock_key_t key = k_spin_lock(&lock);

		/* LPTIM1 CNT register is already reset after one autoreload */
		volatile u32_t lp_time = LL_LPTIM_GetCounter(LPTIM1);

		/* It should be noted that to read reliably the content
		 * of the LPTIM_CNT register, two successive read accesses
		 * must be performed and compared
		 */

		while (lp_time != LL_LPTIM_GetCounter(LPTIM1)) {
			lp_time = LL_LPTIM_GetCounter(LPTIM1);
		}
		lp_time += LL_LPTIM_GetAutoReload(LPTIM1) + 1;

		/* do not change ARR yet, z_clock_announce will do */
		LL_LPTIM_ClearFLAG_ARRM(LPTIM1);

		/* increase the total nb of lptim count
		 * used in the z_timer_cycle_get_32() function.
		 * Reading the CNT register gives a reliable value
		 */
		accumulated_lptim_cnt += lp_time;

		k_spin_unlock(&lock, key);

		/* announce the elapsed time in ms (count register is 16bit) */
		u32_t dticks = (lp_time
				* CONFIG_SYS_CLOCK_TICKS_PER_SEC)
				/ LPTIM_CLOCK;

		z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL)
				? dticks : 1);
	}
}

int z_clock_driver_init(struct device *device)
{
	ARG_UNUSED(device);

	/* enable LPTIM clock source */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_LPTIM1);
	LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_LPTIM1);

#if defined(CONFIG_STM32_LPTIM_CLOCK_LSI)
	/* enable LSI clock */
#ifdef CONFIG_SOC_SERIES_STM32WBX
	LL_RCC_LSI1_Enable();
	while (!LL_RCC_LSI1_IsReady()) {
#else
	LL_RCC_LSI_Enable();
	while (!LL_RCC_LSI_IsReady()) {
#endif /* CONFIG_SOC_SERIES_STM32WBX */
		/* Wait for LSI ready */
	}

	LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSI);

#else /* CONFIG_STM32_LPTIM_CLOCK_LSI */
#if defined(LL_APB1_GRP1_PERIPH_PWR)
	/* Enable the power interface clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
#endif /* LL_APB1_GRP1_PERIPH_PWR */
	/* enable backup domain */
	LL_PWR_EnableBkUpAccess();
	LL_RCC_ForceBackupDomainReset();
	LL_RCC_ReleaseBackupDomainReset();

	/* enable LSE clock */
	LL_RCC_LSE_DisableBypass();
	LL_RCC_LSE_Enable();
	while (!LL_RCC_LSE_IsReady()) {
		/* Wait for LSE ready */
	}
	LL_RCC_SetLPTIMClockSource(LL_RCC_LPTIM1_CLKSOURCE_LSE);

#endif /* CONFIG_STM32_LPTIM_CLOCK_LSI */

	/* Clear the event flag and possible pending interrupt */
	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(lptim1)),
		    DT_IRQ(DT_NODELABEL(lptim1), priority),
		    lptim_irq_handler, 0, 0);
	irq_enable(DT_IRQN(DT_NODELABEL(lptim1)));

	/* configure the LPTIM1 counter */
	LL_LPTIM_SetClockSource(LPTIM1, LL_LPTIM_CLK_SOURCE_INTERNAL);
	/* configure the LPTIM1 prescaler with 1 */
	LL_LPTIM_SetPrescaler(LPTIM1, LL_LPTIM_PRESCALER_DIV1);
	LL_LPTIM_SetPolarity(LPTIM1, LL_LPTIM_OUTPUT_POLARITY_REGULAR);
	LL_LPTIM_SetUpdateMode(LPTIM1, LL_LPTIM_UPDATE_MODE_IMMEDIATE);
	LL_LPTIM_SetCounterMode(LPTIM1, LL_LPTIM_COUNTER_MODE_INTERNAL);
	LL_LPTIM_DisableTimeout(LPTIM1);
	/* counting start is initiated by software */
	LL_LPTIM_TrigSw(LPTIM1);

	/* LPTIM1 interrupt set-up before enabling */
	/* no Compare match Interrupt */
	LL_LPTIM_DisableIT_CMPM(LPTIM1);
	LL_LPTIM_ClearFLAG_CMPM(LPTIM1);

	/* Autoreload match Interrupt */
	LL_LPTIM_EnableIT_ARRM(LPTIM1);
	LL_LPTIM_ClearFLAG_ARRM(LPTIM1);
	/* ARROK bit validates the write operation to ARR register */
	LL_LPTIM_ClearFlag_ARROK(LPTIM1);

	accumulated_lptim_cnt = 0;

	/* Enable the LPTIM1 counter */
	LL_LPTIM_Enable(LPTIM1);

	/* Set the Autoreload value once the timer is enabled */
	if (IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		/* LPTIM1 is triggered on a LPTIM_TIMEBASE period */
		LL_LPTIM_SetAutoReload(LPTIM1, LPTIM_TIMEBASE);

	} else {
		/* LPTIM1 is triggered on a Tick period */
		LL_LPTIM_SetAutoReload(LPTIM1, COUNT_PER_TICK);
	}

	/* Start the LPTIM counter in continuous mode */
	LL_LPTIM_StartCounter(LPTIM1, LL_LPTIM_OPERATING_MODE_CONTINUOUS);

#ifdef CONFIG_DEBUG
	/* stop LPTIM1 during DEBUG */
	LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_LPTIM1_STOP);
#endif
	return 0;
}

void z_clock_set_timeout(s32_t ticks, bool idle)
{
	/* new LPTIM1 AutoReload value to set (aligned on Kernel ticks) */
	u32_t next_arr = 0;

	ARG_UNUSED(idle);

	/* ARROK bit validates previous write operation to ARR register */
	while (LL_LPTIM_IsActiveFlag_ARROK(LPTIM1) == 0) {
	}
	LL_LPTIM_ClearFlag_ARROK(LPTIM1);

#ifdef CONFIG_TICKLESS_KERNEL
	if (ticks == K_TICKS_FOREVER) {
		/* disable LPTIM */
		LL_APB1_GRP1_ForceReset(LL_APB1_GRP1_PERIPH_LPTIM1);
		LL_APB1_GRP1_DisableClock(LL_APB1_GRP1_PERIPH_LPTIM1);
		return;
	}

	/* passing ticks==1 means "announce the next tick",
	 * ticks value of zero (or even negative) is legal and
	 * treated identically: it simply indicates the kernel would like the
	 * next tick announcement as soon as possible.
	 */
	if (ticks <= (s32_t)1) {
		ticks = 1;
	} else {
		ticks = (ticks - 1);
	}
	/* maximise Tick to keep next_arr on 32bit values,
	 * in anycase the ARR cannot exceed LPTIM_TIMEBASE
	 */
	if (ticks > (s32_t)0xFFFF) {
		ticks = 0xFFFF;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	/* read current counter value (cannot exceed 16bit) */

	volatile u32_t lp_time = LL_LPTIM_GetCounter(LPTIM1);

	/* It should be noted that to read reliably the content
	 * of the LPTIM_CNT register, two successive read accesses
	 * must be performed and compared
	 */

	while (lp_time != LL_LPTIM_GetCounter(LPTIM1)) {
		lp_time = LL_LPTIM_GetCounter(LPTIM1);
	}

	/* calculate the next arr value (cannot exceed 16bit)
	 * adjust the next ARR match value to align on Ticks
	 * from the current counter value to first next Tick
	 */
	next_arr = (((lp_time * CONFIG_SYS_CLOCK_TICKS_PER_SEC)
			/ LPTIM_CLOCK) + 1) * LPTIM_CLOCK
			/ (CONFIG_SYS_CLOCK_TICKS_PER_SEC);
	/* add count unit from the expected nb of Ticks */
	next_arr = next_arr + ((u32_t)(ticks) * LPTIM_CLOCK)
			/ CONFIG_SYS_CLOCK_TICKS_PER_SEC + 1;

	/* maximise to TIMEBASE */
	if (next_arr > LPTIM_TIMEBASE) {
		next_arr = LPTIM_TIMEBASE;
	}
	/* run timer and wait for the reload match */
	LL_LPTIM_SetAutoReload(LPTIM1, next_arr);

	k_spin_unlock(&lock, key);

#endif /* CONFIG_TICKLESS_KERNEL */
}

u32_t z_clock_elapsed(void)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return 0;
	}

	k_spinlock_key_t key = k_spin_lock(&lock);

	volatile u32_t lp_time = LL_LPTIM_GetCounter(LPTIM1);

	/* It should be noted that to read reliably the content
	 * of the LPTIM_CNT register, two successive read accesses
	 * must be performed and compared
	 */

	while (lp_time != LL_LPTIM_GetCounter(LPTIM1)) {
		lp_time = LL_LPTIM_GetCounter(LPTIM1);
	}

	k_spin_unlock(&lock, key);

	/* gives the value of LPTIM1 counter (ms)
	 * since the previous 'announce'
	 */
	u32_t ret = ((lp_time + 1) * 1000) / LPTIM_CLOCK;

	/* convert to ticks */
	return z_ms_to_ticks(ret);
}

u32_t z_timer_cycle_get_32(void)
{
	/* just gives the accumulated count in a number of hw cycles */

	k_spinlock_key_t key = k_spin_lock(&lock);

	volatile u32_t lp_time = LL_LPTIM_GetCounter(LPTIM1);

	/* It should be noted that to read reliably the content
	 * of the LPTIM_CNT register, two successive read accesses
	 * must be performed and compared
	 */

	while (lp_time != LL_LPTIM_GetCounter(LPTIM1)) {
		lp_time = LL_LPTIM_GetCounter(LPTIM1);
	}
	lp_time += accumulated_lptim_cnt;

	k_spin_unlock(&lock, key);

	/* convert in hw cycles (keeping 32bit value) */
	return ((lp_time / (LPTIM_CLOCK / 1000))
		* (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC / 1000));
}
