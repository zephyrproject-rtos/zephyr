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
 * - only for stm32l1 atm
 * - only for LSI / LSE (prescalers set for 1hz rtc > (see RTC application note table 7)
 * 		LSI 32kHz async 127, sync 249
 * 		LSE 32.768kHz async 127, sync 255
 * - max resolution (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) = ((synchronous prescaler + 1)/2) > alarm mask SS [0] bit (see RTC application note table 10)
 * - max granularity = 1/resolution sec = 1000/resolution ms
 * 		for LSI 1/125 sec (8ms)
 * 		for LSE 1/128 sec (7,8125ms)
 * - highly advised to choose CONFIG_SYS_CLOCK_TICKS_PER_SEC that is CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC or /2 of it
 */

/* TODO config to add
 * - CONFIG_TIMER_STM32_RTC_SRC
 * - CONFIG_TIMER_STM32_RTC_LSI
 * - CONFIG_TIMER_STM32_RTC_LSE
 * - CONFIG_TIMER_STM32_RTC_LSE_BYPASS
 */

/*
 * questions:
 * - problems with also having the rtc counter driver / having a rtc device (devicetree)?
 * - for CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC should i use the max ss alarm resolution, or max ss resolution?
 * 		the actual granularity of the rtc (so also for read) is (RTC_SYNCH_PREDIV+1), so should use that probably
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

#define RTC_CLOCK_HW_CYCLES_PER_SEC (RTC_SYNCH_PREDIV+1)

#define TEMPORARY_CONFIG_SYS_CLOCK_TICKS_PER_SEC ((RTC_SYNCH_PREDIV+1)/2)
/* TODO: prob should also do the LSI/LSE config options */

#define CYCLES_PER_TICK RTC_CLOCK_HW_CYCLES_PER_SEC / \
			TEMPORARY_CONFIG_SYS_CLOCK_TICKS_PER_SEC

//#define CYCLES_PER_TICK (RTC_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

#define T_TIME_OFFSET 946684800
#define NSEC_PER_SEC (NSEC_PER_USEC * USEC_PER_MSEC * MSEC_PER_SEC)

/* Tick/cycle count of the last announce call. */
static volatile uint32_t rtc_last;

/* TODO: check whether also right calculation for our impl
/* Maximum number of ticks. */
#define MAX_TICKS (UINT32_MAX / CYCLES_PER_TICK - 2)

/* TODO: check what tick threshold applicable to our rtc */
#define TICK_THRESHOLD 7

static bool nonidle_alarm_set = false;





/* TODO:
 * - change to timeval!!
 * - check whether problem with offsetting
 * - check whether problem using posix function
 * - check ms>ticks conversion formula
 */
static uint32_t rtc_stm32_read(const void *arg)
{
	struct tm now = { 0 };
	/* timespec struct? timesecs dan tv_sec, subseconds dan tv_nsec */
	struct timespec ts = { 0 };
	int32_t tms = 0;
	//time_t timesecs;

	uint32_t rtc_date, rtc_time, rtc_subsec, ticks;

	/* Read time and date registers */
	/* needs to be done in this order to unlock shadow registers afterwards */
	rtc_subsec = LL_RTC_TIME_GetSubSecond(RTC);
	rtc_time = LL_RTC_TIME_Get(RTC);
	rtc_date = LL_RTC_DATE_Get(RTC);

	/* TODO check whether this is correct offsetting etc */

	/* Convert calendar datetime to UNIX timestamp */
	/* RTC start time: 1st, Jan, 2000 */
	/* according to counter_ll_stm32_rtc: time_t start:   1st, Jan, 1900 */
	/* according to documentation + pabigot: time_t start:   1st, Jan, 1970 */
	now.tm_year = 100 +
			__LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_YEAR(rtc_date));
	/* tm_mon allowed values are 0-11 */
	now.tm_mon = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MONTH(rtc_date)) - 1;
	now.tm_mday = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_DAY(rtc_date));

	now.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(rtc_time));
	now.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(rtc_time));
	now.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(rtc_time));

	ts.tv_sec = mktime(&now);

	/* Subtract offset of RTC (2000>1970), back to UNIX epoch */
	ts.tv_sec -= T_TIME_OFFSET;

	/*convert subseconds value into ns */
	/* ns required for timespec struct
	 *
	 * formula based on stm32l1 ref manual (RM0038, 16) pg 537:
	 * Second fraction = ( PREDIV_S - SS ) / ( PREDIV_S + 1 )
	 *
	 * also see LL_RTC_TIME_GetSubSecond in stm32cube's stm32l1xx_ll_rtc.h:
	 * formula for seconds, but for timespec tv_nsec nanosecs required, so * time_unit
	 */
	ts.tv_nsec = (RTC_SYNCH_PREDIV - rtc_subsec) / (RTC_SYNCH_PREDIV+1) * NSEC_PER_SEC;

	/* convert timespec	 */
	/* NOTE: use of zephyr/include/posix/time.h function, problem?	 */
	tms = _ts_to_ms(ts);

	/* TODO: check if okay, still not fully sure it covers everything */
	ticks = tms * CYCLES_PER_TICK / TEMPORARY_CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	return ticks;
}

static int rtc_stm32_set_calendar_alarm(time_t tv_sec)
{
	struct tm alarm_tm;
	LL_RTC_AlarmTypeDef rtc_alarm;

	gmtime_r(&tv_sec, &alarm_tm);

	/* Apply ALARM_A */
	rtc_alarm.AlarmTime.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
	rtc_alarm.AlarmTime.Hours = alarm_tm.tm_hour;
	rtc_alarm.AlarmTime.Minutes = alarm_tm.tm_min;
	rtc_alarm.AlarmTime.Seconds = alarm_tm.tm_sec;

	rtc_alarm.AlarmMask = LL_RTC_ALMA_MASK_NONE;
	rtc_alarm.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_DATE;
	rtc_alarm.AlarmDateWeekDay = alarm_tm.tm_mday;

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Disable(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	if (LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_alarm) != SUCCESS) {
		return -EIO;
	}

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Enable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_EnableIT_ALRA(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	return 0;
}

/*
 * TODO:
 * - add error returns
 * - check whether this enough to safely set ss alarm
 * - check whether this will clear the calendar alarm
 */
static int rtc_stm32_set_subsecond_alarm(susecond_t timeout_usec)
{
	/* convert usecs to subsecond register values
	 * formula adapted from subsecond>us formula, see rtc_stm32_read()
	*/
	uint32_t subsecond = RTC_SYNCH_PREDIV - ( (timeout_usec * (RTC_SYNCH_PREDIV+1))
			/ USEC_PER_SEC );

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Disable(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	/* check whether subsecond is between min and max data/prescaler
	 * and set subsecond alarm a
	 */
	LL_RTC_ALMA_SetSubSecond(RTC, subsecond <= RTC_SYNCH_PREDIV ? subsecond : RTC_SYNCH_PREDIV);

	/* set [1] mask so that smallest possible granularity is achieved */
	LL_RTC_ALMA_SetSubSecondMask(RTC, 1);

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Enable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_EnableIT_ALRA(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	return 0;
}

/*
 * TODO:
 * - check if okay this way with the subsecond alarm setting separate
 *    (a lot of the protection things are done in the alarm init function)
 */
static int rtc_stm32_set_idle_alarm(time_t tv_sec)
{
	/* set calendar alarm
	* and clear subsecond mask, otherwise will still tick
	*/

	struct tm alarm_tm;
	LL_RTC_AlarmTypeDef rtc_alarm;

	gmtime_r(&tv_sec, &alarm_tm);

	/* Apply ALARM_A */
	rtc_alarm.AlarmTime.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
	rtc_alarm.AlarmTime.Hours = alarm_tm.tm_hour;
	rtc_alarm.AlarmTime.Minutes = alarm_tm.tm_min;
	rtc_alarm.AlarmTime.Seconds = alarm_tm.tm_sec;

	rtc_alarm.AlarmMask = LL_RTC_ALMA_MASK_NONE;
	rtc_alarm.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_DATE;
	rtc_alarm.AlarmDateWeekDay = alarm_tm.tm_mday;

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Disable(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	/* Set calendar alarm A */
	if (LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_alarm) != SUCCESS) {
		return -EIO;
	}

	/* Clear subsecond alarm A / set mask to [0] */
	/* TODO: think whether to set the ss alarm register here: is it needed, smart, etc? */
	LL_RTC_ALMA_SetSubSecond(RTC, 0x00);
	LL_RTC_ALMA_SetSubSecondMask(RTC, 0);

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Enable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_EnableIT_ALRA(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	return 0;
}

/*
 * TODO:
 * - check whether accuracy is good enough without setting the alarm value to a specific value,
 *     use rtc_stm32_set_subsecond_alarm() for comparison?
 * - check whether calendar alarm can still be generated this way,
 *      or need to also add clear alarm flag & disable IT?
 *      (see counter's rtc_stm32_cancel_alarm() )
 * - add something that bases the mask on the ticks_per_sec?
 *      for now just smallest possible granularity
 */
static int rtc_stm32_set_nonidle_alarm(const void *arg)
{
	ARG_UNUSED(arg);

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Disable(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	/* Set subsecond alarm A / set mask to [1] */
	LL_RTC_ALMA_SetSubSecond(RTC, RTC_SYNCH_PREDIV);
	LL_RTC_ALMA_SetSubSecondMask(RTC, 1);

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Enable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_EnableIT_ALRA(RTC);
	LL_RTC_EnableWriteProtection(RTC);
}



/* TODO
 * - check if rtc value okay (that is usable for the subtraction)
 * - check whether order for read (before flags are cleared etc) is okay (fe with shadow registers etc)
 * - check edge cases like other rtc timers (if non-tickless, if z_clock_set_timeout invokes isr?)
 * - check whether to add the CYCLES_PER_TICK
 */
static void rtc_stm32_isr(const void *arg)
{
	ARG_UNUSED(arg);

	uint32_t now_ticks = rtc_stm32_read(dev);

	if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {
		/* clear flags */
		k_spinlock_key_t key = k_spin_lock(&lock);

		LL_RTC_DisableWriteProtection(RTC);
		LL_RTC_ClearFlag_ALRA(RTC);
		LL_RTC_DisableIT_ALRA(RTC);
		LL_RTC_ALMA_Disable(RTC);
		LL_RTC_EnableWriteProtection(RTC);

		k_spin_unlock(&lock, key);

		/* Announce the elapsed time in ticks  */

		uint32_t dticks = (now_ticks - rtc_last);

		rtc_last += dticks;

		z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL)
				? dticks : (dticks > 0));

	}

	LL_EXTI_ClearFlag_0_31(RTC_EXTI_LINE);

}

/* TODO:
 * - subseconds support > seems like i don't have to add anything for it?
 * - bypass registers control enable?
 */
int z_clock_driver_init(const struct device *device)
{
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

/* TODO:
 * - check whether to use differentiation of tickless/non-tickless
 * - check whether calculations go alright (cycles_per_tick calc very important)
 */
void z_clock_set_timeout(int32_t ticks, bool idle)
{
	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	struct timeval ts = { 0 };

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (int32_t) MAX_TICKS);

	/* Compute number of RTC cycles until the next timeout. */
	uint32_t now_ticks = stm32_rtc_read();
	uint32_t timeout = ticks * CYCLES_PER_TICK + now_ticks % CYCLES_PER_TICK;

	/* Round to the nearest tick boundary. */
	timeout = (timeout + CYCLES_PER_TICK - 1) / CYCLES_PER_TICK
		  * CYCLES_PER_TICK;

	if (timeout < TICK_THRESHOLD) {
		timeout += CYCLES_PER_TICK;
	}

	/* ticks to secs & subseconds*/

	/* ticks to us, some accuracy loss,
	 * but this is the best way with also long enough timeout length
	 *
	 * ticks_to_us = timeout_ticks * (1/CONFIG_SYS_CLOCK_TICKS_PER_SEC) * USEC_PER_SEC
	 */
	uint64_t timeout_us = (timeout * USEC_PER_SEC)/CONFIG_SYS_CLOCK_TICKS_PER_SEC;

	/* check how many ticks for calendar, and how many for subseconds */
	ts.tv_usec = timeout_us % USEC_PER_SEC;
	ts.tv_sec = timeout_us - ts.tv_usec;

	if (idle){
		if (!rtc_stm32_set_idle_alarm(ts.tv_sec)){
			/* calendar alarm set, and clearing ss mask successful */
		}
	}
	else {
		/*!idle*/
		if (nonidle_alarm_set == 0){
			rtc_stm32_set_nonidle_alarm();
		}
		else {
			/* subsecond alarm mask already set: don't do anything
			 * subsecond alarm will generate ticks
			 */
			return;
		}
	}

}

/* TODO:
 * - check whether to add the tickless kernel ifs
 * - check whether to keep the cycles_per_tick
 */
u32_t z_clock_elapsed(void)
{
	return (rtc_stm32_read() - rtc_last) / CYCLES_PER_TICK;
}

/* TODO:
 * - check when this used, and whether additional stuff is required
 */
u32_t z_timer_cycle_get_32(void)
{
	return rtc_stm32_read();
}

