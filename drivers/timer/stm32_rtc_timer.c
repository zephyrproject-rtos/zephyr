/*
 * Copyright (c) 2020 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/timer/system_timer.h>

/*
 * assumptions/notes/limitations:
 * - only for stm32l1
 * - only for LSE (1hz rtc, sync prescaler 1/128 largest resolution for subseconds
 */

/* config to add
 * - CONFIG_TIMER_STM32_RTC_SRC
 * - CONFIG_TIMER_STM32_RTC_LSI
 * - CONFIG_TIMER_STM32_RTC_LSE
 * - CONFIG_TIMER_STM32_RTC_LSE_BYPASS
 */

/*
 * questions:
 * - problems with also having the rtc counter driver / having a rtc device (devicetree)?
 *
 */

#ifdef CONFIG_TIMER_STM32_RTC_LSI
/* ck_apre=LSIFreq/(ASYNC prediv + 1) with LSIFreq=32 kHz RC */
#define RTC_ASYNCH_PREDIV          ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV           ((uint32_t)0x00F9)
#endif

#ifdef CONFIG_TIMER_STM32_RTC_LSE
/* ck_apre=LSEFreq/(ASYNC prediv + 1) = 256Hz with LSEFreq=32768Hz */
#define RTC_ASYNCH_PREDIV          ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV           ((uint32_t)0x00FF)
#endif


static uint32_t rtc_stm32_read(const void *arg)
{
	struct tm now = { 0 };
	time_t ts;
	uint32_t rtc_date, rtc_time, ticks;

	/* Read time and date registers */
	rtc_time = LL_RTC_TIME_Get(RTC);
	rtc_date = LL_RTC_DATE_Get(RTC);
	/* TODO: add subseconds */

	/* Convert calendar datetime to UNIX timestamp */
	/* RTC start time: 1st, Jan, 2000 */
	/* time_t start:   1st, Jan, 1900 */
	now.tm_year = 100 +
			__LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_YEAR(rtc_date));
	/* tm_mon allowed values are 0-11 */
	now.tm_mon = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MONTH(rtc_date)) - 1;
	now.tm_mday = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_DAY(rtc_date));

	now.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(rtc_time));
	now.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(rtc_time));
	now.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(rtc_time));

	ts = mktime(&now);

	/* Return number of seconds since RTC init */
	ts -= T_TIME_OFFSET;

	__ASSERT(sizeof(time_t) == 8, "unexpected time_t definition");
	ticks = counter_us_to_ticks(dev, ts * USEC_PER_SEC);

	return ticks;
}


static void rtc_stm32_isr(const void *arg)
{
	ARG_UNUSED(arg);

	//uint32_t now = rtc_stm32_read(dev);

	if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {

		k_spinlock_key_t key = k_spin_lock(&lock);

		LL_RTC_DisableWriteProtection(RTC);
		LL_RTC_ClearFlag_ALRA(RTC);
		LL_RTC_DisableIT_ALRA(RTC);
		LL_RTC_ALMA_Disable(RTC);
		LL_RTC_EnableWriteProtection(RTC);

		/* get rtc value */

		k_spin_unlock(&lock, key);

		/* announce the elapsed time in ms  */
		uint32_t dticks = counter_sub(t, last_count) / CYC_PER_TICK;

		last_count += dticks * CYC_PER_TICK;

		z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL)
				? dticks : (dticks > 0));

		}
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_ClearFlag_0_31(RTC_EXTI_LINE);
#else
	LL_EXTI_ClearFlag_0_31(RTC_EXTI_LINE);
#endif
}


int z_clock_driver_init(const struct device *device){
	ARG_UNUSED(device);

	LL_RTC_InitTypeDef rtc_initstruct;

	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	LL_PWR_EnableBkUpAccess();

#if defined(CONFIG_TIMER_STM32_RTC_CLOCK_LSI)
	LL_RCC_LSI_Enable();
	while (LL_RCC_LSI_IsReady() != 1) {
	}
	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);

#else /* CONFIG_TIMER_STM32_RTC_CLOCK_LSE */

#if defined(CONFIG_TIMER_STM32_RTC_CLOCK_LSE_BYPASS)
	LL_RCC_LSE_EnableBypass();
#endif /* CONFIG_COUNTER_RTC_STM32_LSE_BYPASS */

	LL_RCC_LSE_Enable();

	/* Wait until LSE is ready */
	while (LL_RCC_LSE_IsReady() != 1) {
	}

	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

#endif /* CONFIG_TIMER_STM32_RTC_CLOCK_SRC */

	LL_RCC_EnableRTC();

	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

	if (LL_RTC_DeInit(RTC) != SUCCESS) {
		return -EIO;
	}

	/* RTC configuration */
	rtc_initstruct.HourFormat      = LL_RTC_HOURFORMAT_24HOUR;
	rtc_initstruct.AsynchPrescaler = RTC_ASYNCH_PREDIV;
	rtc_initstruct.SynchPrescaler  = RTC_SYNCH_PREDIV;

	if (LL_RTC_Init(RTC, &rtc_initstruct) != SUCCESS) {
		return -EIO;
	}

/* TODO: bypass shadow registers control, where enabled?
 * important because has an influence on correctness of calendar registers
 * 	(if enabled, time and date registers are frozen when subsecond register is read)
 * but also synchronization delay after exiting stop,standby or shutdown mode
 * 	(if enabled a delay of up to two RTC clock periods can be experienced)
 */
#ifdef RTC_CR_BYPSHAD
	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_EnableShadowRegBypass(RTC);
	LL_RTC_EnableWriteProtection(RTC);
#endif /* RTC_CR_BYPSHAD */

	LL_EXTI_EnableIT_0_31(RTC_EXTI_LINE);

	LL_EXTI_EnableRisingTrig_0_31(RTC_EXTI_LINE);

/* same as stm32 rtc counter driver, problem? */
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    rtc_stm32_isr, DEVICE_GET(rtc_stm32), 0);
	irq_enable(DT_INST_IRQN(0));

	return 0;
}


void z_clock_set_timeout(int32_t ticks, bool idle){


}

u32_t z_clock_elapsed(void){


}

u32_t z_timer_cycle_get_32(void){


}

