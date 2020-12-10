/*
 * Copyright (c) 2020 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <time.h>

#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/timer/system_timer.h>
#include <soc.h>

#include <stm32_ll_rtc.h>
#include <stm32_ll_exti.h>
#include <stm32_ll_pwr.h>

#include <sys/_timeval.h>
#include <sys/timeutil.h>

/*
 * assumptions/notes/limitations:
 * - (currently) only for stm32l1
 * - only for LSI / LSE (prescalers set for 1hz rtc > (see RTC application note table 7)
 * 		LSI 32kHz async 127, sync 249
 * 		LSE 32.768kHz async 127, sync 255
 * - max resolution (CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC) = ((synchronous prescaler + 1)/2) > alarm mask SS [0] bit (see RTC application note table 10)
 * - max granularity = 1/resolution sec = 1000/resolution ms
 * 		for LSI 1/125 sec (8ms)
 * 		for LSE 1/128 sec (7,8125ms)
 * - highly advised to choose CONFIG_SYS_CLOCK_TICKS_PER_SEC that is CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC or /2 of it
 *
 * - in zephyr/soc/arm/st_stm32/common/Kconfig.defconfig.series:
 *     SYS_CLOCK_TICKS_PER_SEC=((RTC_SYNCH_PREDIV+1)/2)
 */


/*
 * questions:
 * - problems with also having the rtc counter driver / having a rtc device (devicetree)?
 * - for CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC should i use the max ss alarm resolution, or max ss resolution?
 * 		the actual granularity of the rtc (so also for read) is (RTC_SYNCH_PREDIV+1), so should use that probably
 */

#define RTC_EXTI_LINE	LL_EXTI_LINE_17
static struct k_spinlock lock;


#ifdef CONFIG_STM32_RTC_TIMER_LSI
/* ck_apre=LSIFreq/(ASYNC prediv + 1) with LSIFreq=32 kHz RC */
#define RTC_ASYNCH_PREDIV          ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV           ((uint32_t)0x00F9)
#endif

#ifdef CONFIG_STM32_RTC_TIMER_LSE
/* ck_apre=LSEFreq/(ASYNC prediv + 1) = 256Hz with LSEFreq=32768Hz */
#define RTC_ASYNCH_PREDIV          ((uint32_t)0x7F)
/* ck_spre=ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV           ((uint32_t)0x00FF)
#endif

#define RTC_CLOCK_HW_CYCLES_PER_SEC ((RTC_SYNCH_PREDIV+1)/ 2U)

#define CYCLES_PER_TICK (RTC_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* Seconds from 1970-01-01T00:00:00 to 2000-01-01T00:00:00 */
#define T_TIME_OFFSET 946684800
//#define NSEC_PER_SEC (NSEC_PER_USEC * USEC_PER_MSEC * MSEC_PER_SEC)

/* Tick/cycle count of the last announce call. */
static volatile uint32_t rtc_last;

/* TODO: check whether also right calculation for our impl */
/* Maximum number of ticks. */
#define MAX_TICKS (UINT32_MAX / CYCLES_PER_TICK - 2U)

/* TODO: check what tick threshold applicable to our rtc */
#define TICK_THRESHOLD 7

static bool nonidle_alarm_set = false;


////////// VALUES FROM STM32CUBE'S STM32L1XX_LL_RTC.C /////
/* Default values used for prescaler */
//#define RTC_ASYNCH_PRESC_DEFAULT     0x0000007FU
#define RTC_SYNCH_PRESC_DEFAULT      0x000000FFU

/* Values used for timeout */
#define RTC_INITMODE_TIMEOUT         1000U /* 1s when tick set to 1ms */
//#define RTC_SYNCHRO_TIMEOUT          1000U /* 1s when tick set to 1ms */

//static struct stm32_pclken pclken = {
//	.enr = DT_INST_CLOCKS_CELL(0, bits),
//	.bus = DT_INST_CLOCKS_CELL(0, bus)
//};


/* own implementation of LL_RTC_EnterInitMode(RTC_TypeDef *RTCx),
 * since in that function use of LL_SYSTICK_IsActiveCounterFlag(),
 * which causes a infinite while loop (since SYSTICK not used)
 */
/**
  * @brief  Enters the RTC Initialization mode.
  * @note   The RTC Initialization mode is write protected, use the
  *         @ref LL_RTC_DisableWriteProtection before calling this function.
  * @param  RTCx RTC Instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RTC is in Init mode
  *          - ERROR: RTC is not in Init mode
  */
ErrorStatus rtc_EnterInitMode(RTC_TypeDef *RTCx)
{
  __IO uint32_t timeout = RTC_INITMODE_TIMEOUT;
  ErrorStatus status = SUCCESS;
  uint32_t tmp;

  /* Check the parameter */
  assert_param(IS_RTC_ALL_INSTANCE(RTCx));

  /* Check if the Initialization mode is set */
  if (LL_RTC_IsActiveFlag_INIT(RTCx) == 0U)
  {
    /* Set the Initialization mode */
    LL_RTC_EnableInitMode(RTCx);

    /* Wait till RTC is in INIT state and if Time out is reached exit */
    tmp = LL_RTC_IsActiveFlag_INIT(RTCx);
    while ((timeout != 0U) && (tmp != 1U))
    {
      timeout --;
      tmp = LL_RTC_IsActiveFlag_INIT(RTCx);
      if (timeout == 0U)
      {
        status = ERROR;
      }
    }
  }
  return status;
}

/* own implementation of LL_RTC_Init(RTC_TypeDef *RTCx,
 *          LL_RTC_InitTypeDef *RTC_InitStruct),
 * to avoid use of LL_RTC_EnterInitMode(RTC_TypeDef *RTCx),
 * since in that function use of LL_SYSTICK_IsActiveCounterFlag(),
 * which causes a infinite while loop (since SYSTICK not used)
 */

/**
  * @brief  Initializes the RTC registers according to the specified parameters
  *         in RTC_InitStruct.
  * @param  RTCx RTC Instance
  * @param  RTC_InitStruct pointer to a @ref LL_RTC_InitTypeDef structure that contains
  *         the configuration information for the RTC peripheral.
  * @note   The RTC Prescaler register is write protected and can be written in
  *         initialization mode only.
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RTC registers are initialized
  *          - ERROR: RTC registers are not initialized
  */
ErrorStatus rtc_Init(RTC_TypeDef *RTCx, LL_RTC_InitTypeDef *RTC_InitStruct)
{
  ErrorStatus status = ERROR;

  /* Check the parameters */
  assert_param(IS_RTC_ALL_INSTANCE(RTCx));
  assert_param(IS_LL_RTC_HOURFORMAT(RTC_InitStruct->HourFormat));
  assert_param(IS_LL_RTC_ASYNCH_PREDIV(RTC_InitStruct->AsynchPrescaler));
  assert_param(IS_LL_RTC_SYNCH_PREDIV(RTC_InitStruct->SynchPrescaler));

  /* Disable the write protection for RTC registers */
  LL_RTC_DisableWriteProtection(RTCx);

  /* Set Initialization mode */
  if (rtc_EnterInitMode(RTCx) != ERROR)
  {
    /* Set Hour Format */
    LL_RTC_SetHourFormat(RTCx, RTC_InitStruct->HourFormat);

    /* Configure Synchronous and Asynchronous prescaler factor */
    LL_RTC_SetSynchPrescaler(RTCx, RTC_InitStruct->SynchPrescaler);
    LL_RTC_SetAsynchPrescaler(RTCx, RTC_InitStruct->AsynchPrescaler);

    /* Exit Initialization mode */
    LL_RTC_DisableInitMode(RTCx);

    status = SUCCESS;
  }
  /* Enable the write protection for RTC registers */
  LL_RTC_EnableWriteProtection(RTCx);

  return status;
}


/* own implementation of LL_RTC_DeInit(RTC_TypeDef *RTCx),
 * to avoid use of LL_RTC_EnterInitMode(RTC_TypeDef *RTCx),
 * since in that function use of LL_SYSTICK_IsActiveCounterFlag(),
 * which causes a infinite while loop (since SYSTICK not used)
 */
/**
  * @brief  De-Initializes the RTC registers to their default reset values.
  * @note   This function does not reset the RTC Clock source and RTC Backup Data
  *         registers.
  * @param  RTCx RTC Instance
  * @retval An ErrorStatus enumeration value:
  *          - SUCCESS: RTC registers are de-initialized
  *          - ERROR: RTC registers are not de-initialized
  */
ErrorStatus rtc_DeInit(RTC_TypeDef *RTCx)
{
  ErrorStatus status = ERROR;

  /* Check the parameter */
  assert_param(IS_RTC_ALL_INSTANCE(RTCx));

  /* Disable the write protection for RTC registers */
  LL_RTC_DisableWriteProtection(RTCx);

  /* Set Initialization mode */
  if (rtc_EnterInitMode(RTCx) != ERROR)
  {
    /* Reset TR, DR and CR registers */
    LL_RTC_WriteReg(RTCx, TR,       0x00000000U);
#if defined(RTC_WAKEUP_SUPPORT)
    LL_RTC_WriteReg(RTCx, WUTR,     RTC_WUTR_WUT);
#endif /* RTC_WAKEUP_SUPPORT */
    LL_RTC_WriteReg(RTCx, DR, (RTC_DR_WDU_0 | RTC_DR_MU_0 | RTC_DR_DU_0));
    /* Reset All CR bits except CR[2:0] */
#if defined(RTC_WAKEUP_SUPPORT)
    LL_RTC_WriteReg(RTCx, CR, (LL_RTC_ReadReg(RTCx, CR) & RTC_CR_WUCKSEL));
#else
    LL_RTC_WriteReg(RTCx, CR, 0x00000000U);
#endif /* RTC_WAKEUP_SUPPORT */
    LL_RTC_WriteReg(RTCx, PRER, (RTC_PRER_PREDIV_A | RTC_SYNCH_PRESC_DEFAULT));
    LL_RTC_WriteReg(RTCx, ALRMAR,   0x00000000U);
    LL_RTC_WriteReg(RTCx, ALRMBR,   0x00000000U);
#if defined(RTC_SHIFTR_ADD1S)
    LL_RTC_WriteReg(RTCx, SHIFTR,   0x00000000U);
#endif /* RTC_SHIFTR_ADD1S */
#if defined(RTC_SMOOTHCALIB_SUPPORT)
    LL_RTC_WriteReg(RTCx, CALR,     0x00000000U);
#endif /* RTC_SMOOTHCALIB_SUPPORT */
#if defined(RTC_SUBSECOND_SUPPORT)
    LL_RTC_WriteReg(RTCx, ALRMASSR, 0x00000000U);
    LL_RTC_WriteReg(RTCx, ALRMBSSR, 0x00000000U);
#endif /* RTC_SUBSECOND_SUPPORT */

    /* Reset ISR register and exit initialization mode */
    LL_RTC_WriteReg(RTCx, ISR,      0x00000000U);

    /* Reset Tamper and alternate functions configuration register */
    LL_RTC_WriteReg(RTCx, TAFCR, 0x00000000U);

    /* Wait till the RTC RSF flag is set */
    status = LL_RTC_WaitForSynchro(RTCx);
  }

  /* Enable the write protection for RTC registers */
  LL_RTC_EnableWriteProtection(RTCx);

  return status;
}




static uint32_t _tv_to_ms(const struct timeval *to)
{
	return (to->tv_sec * MSEC_PER_SEC) + (to->tv_usec / USEC_PER_MSEC);
}


/* TODO:
 * - check if okay changed to timeval (from timespec) > okay range with usec vs nsec?
 * - check whether problem with offsetting
 * - check ms>ticks conversion formula
 */
static uint32_t rtc_stm32_read(void)
{
	struct tm now;
	struct timeval ts;
	uint32_t tms = 0;

	uint32_t rtc_date, rtc_time, rtc_subsec, ticks;

	/* Read time and date registers */
	/* needs to be done in this order to unlock shadow registers afterwards */
	rtc_subsec = LL_RTC_TIME_GetSubSecond(RTC);
	rtc_time = LL_RTC_TIME_Get(RTC);
	rtc_date = LL_RTC_DATE_Get(RTC);

	/* TODO check whether this is correct offsetting etc */

	/* Convert calendar datetime to UNIX timestamp */
	/* RTC start time: 1st, Jan, 2000 */
	/* time_t start:   1st, Jan, 1970 */
	now.tm_year = 100 +
			__LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_YEAR(rtc_date));
	/* tm_mon allowed values are 0-11 */
	now.tm_mon = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MONTH(rtc_date)) - 1;
	now.tm_mday = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_DAY(rtc_date));

	now.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(rtc_time));
	now.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(rtc_time));
	now.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(rtc_time));

	ts.tv_sec = timeutil_timegm(&now);

	/* Subtract offset of RTC (2000>1970), back to UNIX epoch */
	ts.tv_sec -= T_TIME_OFFSET;

	//printk("rtc_stm32_read: ts.tv_sec = %llu\n", ts.tv_sec);

	/*convert subseconds value into us */
	/* us required for timeval struct
	 *
	 * formula based on stm32l1 ref manual (RM0038, 16) pg 537:
	 * Second fraction = ( PREDIV_S - SS ) / ( PREDIV_S + 1 )
	 *
	 * also see LL_RTC_TIME_GetSubSecond in stm32cube's stm32l1xx_ll_rtc.h:
	 * formula for seconds, but for timespec tv_usec microsecs required, so * time_unit
	 */
	ts.tv_usec = (RTC_SYNCH_PREDIV - rtc_subsec) * USEC_PER_SEC / (RTC_SYNCH_PREDIV+1);

	//printk("rtc_stm32_read: ts.tv_usec = %u\n", ts.tv_usec);

	/* convert timeval	 */
	tms = _tv_to_ms(&ts);

	//printk("rtc_stm32_read: tms = %u\n", tms);

	/* TODO: check whether cycles_per_tick used correctly here */
	ticks = tms / (MSEC_PER_SEC * (uint32_t)(CYCLES_PER_TICK) / (uint32_t)(CONFIG_SYS_CLOCK_TICKS_PER_SEC));

	//printk("rtc_stm32_read: ticks = %u\n", ticks);

	return ticks;
}


//static int rtc_stm32_set_calendar_alarm(time_t tv_sec)
//{
//	struct tm alarm_tm;
//	LL_RTC_AlarmTypeDef rtc_alarm;
//
//	gmtime_r(&tv_sec, &alarm_tm);
//
//	/* Apply ALARM_A */
//	rtc_alarm.AlarmTime.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
//	rtc_alarm.AlarmTime.Hours = alarm_tm.tm_hour;
//	rtc_alarm.AlarmTime.Minutes = alarm_tm.tm_min;
//	rtc_alarm.AlarmTime.Seconds = alarm_tm.tm_sec;
//
//	rtc_alarm.AlarmMask = LL_RTC_ALMA_MASK_NONE;
//	rtc_alarm.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_DATE;
//	rtc_alarm.AlarmDateWeekDay = alarm_tm.tm_mday;
//
//	LL_RTC_DisableWriteProtection(RTC);
//	LL_RTC_ALMA_Disable(RTC);
//	LL_RTC_EnableWriteProtection(RTC);
//
//	if (LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_alarm) != SUCCESS) {
//		return -EIO;
//	}
//
//	LL_RTC_DisableWriteProtection(RTC);
//	LL_RTC_ALMA_Enable(RTC);
//	LL_RTC_ClearFlag_ALRA(RTC);
//	LL_RTC_EnableIT_ALRA(RTC);
//	LL_RTC_EnableWriteProtection(RTC);
//
//	return 0;
//}


/*
 * TODO:
 * - add error returns
 * - check whether this enough to safely set ss alarm
 * - check whether this will clear the calendar alarm
 */
//static int rtc_stm32_set_subsecond_alarm(suseconds_t timeout_usec)
//{
//	/* convert usecs to subsecond register values
//	 * formula adapted from subsecond>us formula, see rtc_stm32_read()
//	*/
//	uint32_t subsecond = RTC_SYNCH_PREDIV - ( (timeout_usec * (RTC_SYNCH_PREDIV+1))
//			/ USEC_PER_SEC );
//
//	LL_RTC_DisableWriteProtection(RTC);
//	LL_RTC_ALMA_Disable(RTC);
//	LL_RTC_EnableWriteProtection(RTC);
//
//	/* check whether subsecond is between min and max data/prescaler
//	 * and set subsecond alarm a
//	 */
//	LL_RTC_ALMA_SetSubSecond(RTC, subsecond <= RTC_SYNCH_PREDIV ? subsecond : RTC_SYNCH_PREDIV);
//
//	/* set [1] mask so that smallest possible granularity is achieved */
//	LL_RTC_ALMA_SetSubSecondMask(RTC, 1);
//
//	LL_RTC_DisableWriteProtection(RTC);
//	LL_RTC_ALMA_Enable(RTC);
//	LL_RTC_ClearFlag_ALRA(RTC);
//	LL_RTC_EnableIT_ALRA(RTC);
//	LL_RTC_EnableWriteProtection(RTC);
//
//	return 0;
//}

/*
 * TODO:
 * - check if okay this way with the subsecond alarm setting separate
 *    (a lot of the protection things are done in the alarm init function)
 * - !! Change to add timeout/alarm value to current time, not absolute
 *       time
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

	LL_RTC_DisableWriteProtection(RTC);

	/* Clear subsecond alarm A / set mask to [0] */
	/* TODO: think whether to set the ss alarm register here: is it needed, smart, etc? */
	LL_RTC_ALMA_SetSubSecond(RTC, 0x00);
	LL_RTC_ALMA_SetSubSecondMask(RTC, 0);

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
 * - check whether mask [1] always okay, or causes problem with LSI,
 *     since that bit isn't 1 when 249
 */
static int rtc_stm32_set_nonidle_alarm(void)
{
	//rtc_DeInit(RTC);

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Disable(RTC);

	/* Set subsecond alarm A / set mask to [1] */
	LL_RTC_ALMA_SetSubSecond(RTC, RTC_SYNCH_PREDIV);
	LL_RTC_ALMA_SetSubSecondMask(RTC, 1);

	LL_RTC_ALMA_Enable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_EnableIT_ALRA(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	printk("rtc_stm32_set_nonidle_alarm: LL_RTC_ALMA_GetSubSecond = %u, LL_RTC_ALMA_GetSubSecondMask = %u, LL_RTC_ALMA_GetTime = %u\n", LL_RTC_ALMA_GetSubSecond(RTC), LL_RTC_ALMA_GetSubSecondMask(RTC), LL_RTC_ALMA_GetTime(RTC));

	printk("rtc_stm32_set_nonidle_alarm: RTC alarm interrupt has been enabled = %d\n", ((((RTC->CR) & RTC_IT_ALRA) != 0U) ? 1U : 0U));
	printk("rtc_stm32_set_nonidle_alarm: RTC alarm flag status = %d\n", ((((RTC->ISR) & RTC_FLAG_ALRAF) != 0U) ? 1U : 0U));
	printk("rtc_stm32_set_nonidle_alarm: RTC alarm interrupt has occurred = %d\n", (((((RTC->ISR) & (RTC_IT_ALRA >> 4U)) != 0U) ? 1U : 0U)));


	return 0;
}



/* TODO
 * - check if rtc value okay (that is usable for the subtraction)
 * - check whether order for read (before flags are cleared etc) is okay (fe with shadow registers etc)
 * - check edge cases like other rtc timers (if non-tickless, if z_clock_set_timeout invokes isr?)
 * - check whether to add the CYCLES_PER_TICK
 * - add idle/nonidle case for disable alarm
 */
static void rtc_stm32_isr_handler(const void *arg)
{
	ARG_UNUSED(arg);

	const struct device *clk;
	clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	if (stm32_clock_control_real_init(clk) == 0){
		/* clock reconfiguration successful */
		printk("Clock reconfig after rtc isr successful\n");
	}

	uint32_t now_ticks = rtc_stm32_read();
	//printk("rtc_stm32_isr_handler: now_ticks=%u\n", now_ticks);

	printk("rtc_stm32_isr_handler: LL_EXTI_IsActiveFlag_0_31 = %u, LL_RTC_IsActiveFlag_ALRA = %u\n", LL_EXTI_IsActiveFlag_0_31(RTC_EXTI_LINE), LL_RTC_IsActiveFlag_ALRA(RTC));


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

		/* TODO: this condition doesn't really apply anymore */
//		z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL)
//						? now_ticks : (now_ticks > 0));
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

	/* enable RTC clock source */
	/* replace with clock_control_on() at some point, like in counter driver */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);
	LL_APB1_GRP1_ReleaseReset(LL_APB1_GRP1_PERIPH_PWR);

	LL_PWR_EnableBkUpAccess();

#if defined(CONFIG_STM32_RTC_TIMER_BACKUP_DOMAIN_RESET)
	LL_RCC_ForceBackupDomainReset();
	LL_RCC_ReleaseBackupDomainReset();
#endif

#if defined(CONFIG_STM32_RTC_TIMER_LSI)
	LL_RCC_LSI_Enable();
	while (LL_RCC_LSI_IsReady() != 1) {
	}
	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);
	printk("z_clock_driver_init: LSI clock source set\n");


#else /* CONFIG_STM32_RTC_TIMER_LSE */

#if defined(CONFIG_STM32_RTC_TIMER_LSE_BYPASS)
	LL_RCC_LSE_EnableBypass();
#endif /* CONFIG_STM32_RTC_TIMER_LSE_BYPASS */

	printk("z_clock_driver_init: before setting LSE things \n");
	LL_RCC_LSE_Enable();

	/* Wait until LSE is ready */
	while (LL_RCC_LSE_IsReady() != 1) {
	}


	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);
	printk("z_clock_driver_init: LSE clock source set\n");

#endif /* CONFIG_STM32_RTC_TIMER_SRC */

	printk("z_clock_driver_init: is LL_RCC_GetRTCClockSource LSI? = %d\n", LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSI ? 1 : 0);
	printk("z_clock_driver_init: is LL_RCC_GetRTCClockSource LSE? = %d\n", LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSE ? 1 : 0);

	LL_RCC_EnableRTC();

	if (rtc_DeInit(RTC) != SUCCESS) {
		return -EIO;
	}

	/* RTC configuration */
	rtc_initstruct.HourFormat      = LL_RTC_HOURFORMAT_24HOUR;
	rtc_initstruct.AsynchPrescaler = RTC_ASYNCH_PREDIV;
	rtc_initstruct.SynchPrescaler  = RTC_SYNCH_PREDIV;

	if (rtc_Init(RTC, &rtc_initstruct) != SUCCESS) {
		return -EIO;
	}

	printk("z_clock_driver_init: 2 is LL_RCC_GetRTCClockSource LSI? = %d\n", LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSI ? 1 : 0);
	printk("z_clock_driver_init: 2 is LL_RCC_GetRTCClockSource LSE? = %d\n", LL_RCC_GetRTCClockSource() == LL_RCC_RTC_CLKSOURCE_LSE ? 1 : 0);


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


	IRQ_CONNECT(DT_IRQN(DT_NODELABEL(rtc)),
			DT_IRQ(DT_NODELABEL(rtc), priority),
			rtc_stm32_isr_handler, 0, 0);
	irq_enable(DT_IRQN(DT_NODELABEL(rtc)));



	return 0;
}

/* TODO:
 * - check whether to use differentiation of tickless/non-tickless
 * - check whether calculations go alright (cycles_per_tick calc very important)
 */
void z_clock_set_timeout(int32_t ticks, bool idle)
{
	struct timeval ts;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 0, (uint32_t) MAX_TICKS);

	/* Compute number of RTC cycles until the next timeout. */
	/* timeout is tick value of when timeout occurs, not amount until */
	uint32_t now_ticks = rtc_stm32_read();
	uint32_t timeout = ticks * CYCLES_PER_TICK + now_ticks % CYCLES_PER_TICK;
	printk("z_clock_set_timeout: ticks = %d, now_ticks = %u, timeout = %u, CYCLES_PER_TICK = %d\n", ticks, now_ticks, timeout, CYCLES_PER_TICK);
	//printk("z_clock_set_timeout: RTC_CLOCK_HW_CYCLES_PER_SEC = %d, CONFIG_SYS_CLOCK_TICKS_PER_SEC = %d\n", RTC_CLOCK_HW_CYCLES_PER_SEC, CONFIG_SYS_CLOCK_TICKS_PER_SEC);


	/* Round to the nearest tick boundary. */
	timeout = (timeout + CYCLES_PER_TICK - 1) / CYCLES_PER_TICK
		  * CYCLES_PER_TICK;

	if (timeout < TICK_THRESHOLD) {
		timeout += CYCLES_PER_TICK;
	}
	printk("z_clock_set_timeout: now_ticks = %u, timeout = %u\n", now_ticks, timeout);

	/* ticks to secs & subseconds*/

	/* ticks to us, some accuracy loss,
	 * but this is the best way with also long enough timeout length
	 *
	 * ticks_to_us = timeout_ticks * (1/CONFIG_SYS_CLOCK_TICKS_PER_SEC) * USEC_PER_SEC
	 */
	uint64_t timeout_us = (uint64_t)timeout *
			(uint64_t)USEC_PER_SEC /
			(uint64_t)CONFIG_SYS_CLOCK_TICKS_PER_SEC;
	printk("z_clock_set_timeout: timeout_us = %llu\n", timeout_us);


	/* check how many ticks for calendar, and how many for subseconds */
	ts.tv_usec = timeout_us % USEC_PER_SEC;
	ts.tv_sec = (timeout_us - ts.tv_usec) / USEC_PER_SEC;

	//ts.tv_sec = 20;

	printk("z_clock_set_timeout: ts.tv_sec=%llu, ts.tv_usec=%u, idle=%d\n", ts.tv_sec, ts.tv_usec, idle);

//	if (!rtc_stm32_set_idle_alarm(ts.tv_sec)){
//		printk("calendar alarm set, and clearing ss mask successful\n");
//		nonidle_alarm_set = 0;
//	}

	if (idle && ts.tv_sec > 1){
		if (!rtc_stm32_set_idle_alarm(ts.tv_sec)){
			printk("calendar alarm set, and clearing ss mask successful\n");
			nonidle_alarm_set = 0;
		}
	}
	else {
		/*!idle*/
		if (nonidle_alarm_set == 0){
			if(!rtc_stm32_set_nonidle_alarm()){
				printk("subsecond alarm & ss mask set\n");
			}
			nonidle_alarm_set = 1;
		}
		else {
			/* subsecond alarm mask already set: don't do anything
			 * subsecond alarm will generate ticks
			 */
			printk("subsecond alarm mask already set: don't do anything\n");
			return;
		}
	}

}

/* TODO:
 * - check whether to add the tickless kernel ifs
 * - check whether to keep the cycles_per_tick
 */
uint32_t z_clock_elapsed(void)
{
	uint32_t now_ticks = rtc_stm32_read();
	printk("z_clock_elapsed: now_ticks= %u, rtc_last= %u\n", now_ticks, rtc_last);

	uint32_t dticks = (now_ticks - rtc_last);

	//rtc_last += dticks;

	return (dticks / CYCLES_PER_TICK);

	//return (rtc_stm32_read() - rtc_last) / CYCLES_PER_TICK;
}

/* TODO:
 * - check when this used, and whether additional stuff is required
 */
uint32_t z_timer_cycle_get_32(void)
{
	return rtc_stm32_read();
}

