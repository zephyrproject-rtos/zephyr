/*
 * Copyright (c) 2020 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_rtc.h>
#include <stm32_ll_exti.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_system.h>

#include <drivers/timer/system_timer.h>

#include <sys/_timeval.h>
#include <sys/timeutil.h>

/*
 * assumptions/notes/limitations:
 * - (currently) only for stm32l1 (LSI/LSE frequency dependent + only tested on L1)
 * - only for LSI / LSE (prescalers set for 1hz rtc > (see RTC application note table 7)
 * 		LSI 37kHz async 1, sync 18499
 * 		LSE 32.768kHz async 1, sync 16383
 * 		(other combinations possible, but async=>1, sync as high as possible for max granularity)
 * - max resolution = ((synchronous prescaler + 1)/2) > alarm mask SS [0] bit (see RTC application note table 10)
 * 	> subsecond granularity in read is 2x the subsec interrupt granularity (due to subsec alarm masks)
 * - max granularity = 1/resolution sec = 1000/resolution ms
 * 		= 1/sync prescaler
 * - choose a CONFIG_SYS_CLOCK_TICKS_PER_SEC that is 'max resolution' or /2, /4 etc of it
 *
 * - (currently not implemented:) in zephyr/soc/arm/st_stm32/common/Kconfig.defconfig.series:
 *     SYS_CLOCK_TICKS_PER_SEC=((RTC_SYNCH_PREDIV+1)/2) > this way dependent on LSI/LSE frequency per SoC series
 * - CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC currently is still 'attached' to the stm32 clock_control driver, this reflects the
 * 		hardware system clock (HSI/HSE/MSI/PLL), not the system timer (in the case of using this driver, the RTC). Hence
 * 		why not used in this driver.
 *
 *
 */

/* dependencies / limitations:
 * - max time timeout/sleep (calculated in z_clock_set_timeout()) depends on:
 * 		- int32_t ticks & CONFIG_SYS_CLOCK_TICKS_PER_SEC
 * 			Fe LSE 32.768kHz async 1, sync 16383 with CONFIG_SYS_CLOCK_TICKS_PER_SEC = 8192,
 * 			max time asleep (max timeout) +- 72 hours
 * 		- vars used during calculations:
 * 			- uint32_t timeout (in ticks, vartype larger than ticks, so no problems)
 * 			- uint64_t timeout_us (*1000000 larger than timeout, but vartype enough space with 64bits)
 * 			- (timeval) ts.tv_usec = suseconds_t = int32_t (more than enough, bc max usec in 1 sec)
 */

/*
 * questions:
 * - for CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC should i use the max ss alarm resolution, or max ss resolution?
 * 		the actual granularity of the rtc (so also for read) is (RTC_SYNCH_PREDIV+1), so should use that probably
 */

/*
 * TODO:
 * - find proper place for LSI/LSE config (in board config?) > dependent on SoC's LSI freq, but also application preferred config
 * 		(especially if consumption during sleep dependent on prescalers > need to run tests)
 * - add something for CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC ?
 * - figure out if tick threshold
 * - figure out if there is a way to not have to 'overwrite' the LL functions, due to the disabling of SysTick
 * - calculate if variables are large enough if controller would run for 10 years :)
 */

#define RTC_EXTI_LINE	LL_EXTI_LINE_17
static struct k_spinlock lock;

/* new configuration: */
#ifdef CONFIG_STM32_RTC_TIMER_LSI
/* ck_apre=LSIFreq/(ASYNC prediv + 1)=18500 with LSIFreq=37 kHz RC */
#define RTC_ASYNCH_PREDIV          ((uint32_t)0x01)
/* sync presc = 18499, ck_spre= ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV           ((uint32_t)0x4843)
#endif

#ifdef CONFIG_STM32_RTC_TIMER_LSE
/* ck_apre=LSEFreq/(ASYNC prediv + 1) = 16384 with LSEFreq=32768Hz */
#define RTC_ASYNCH_PREDIV          ((uint32_t)0x01)
/* sync presc = 16384, ck_spre= ck_apre/(SYNC prediv + 1) = 1 Hz */
#define RTC_SYNCH_PREDIV           ((uint32_t)0x3FFF)
#endif



#define RTC_CLOCK_HW_CYCLES_PER_SEC ((RTC_SYNCH_PREDIV+1)/ 2U)

#define CYCLES_PER_TICK (RTC_CLOCK_HW_CYCLES_PER_SEC / CONFIG_SYS_CLOCK_TICKS_PER_SEC)

/* Seconds from 1970-01-01T00:00:00 to 2000-01-01T00:00:00 */
#define T_TIME_OFFSET 946684800

/* Tick/cycle count of the last announce call. */
static volatile uint32_t rtc_last = 0;

/* TODO: check whether also right calculation for our impl */
/* Maximum number of ticks. */
#define MAX_TICKS (UINT32_MAX / CYCLES_PER_TICK - 2U)

/* TODO: check what tick threshold applicable to our rtc */
#define TICK_THRESHOLD 7


////////// VALUES FROM STM32CUBE'S STM32L1XX_LL_RTC.C /////
/* Default values used for prescaler */
//#define RTC_ASYNCH_PRESC_DEFAULT     0x0000007FU
#define RTC_SYNCH_PRESC_DEFAULT      0x000000FFU

/* Values used for timeout */
#define RTC_INITMODE_TIMEOUT         1000U /* 1s when tick set to 1ms */
//#define RTC_SYNCHRO_TIMEOUT          1000U /* 1s when tick set to 1ms */


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

static struct timeval rtc_stm32_ticks_to_tv(uint32_t ticks)
{
	struct timeval ts;
	uint64_t ticks_us;

	/* ticks to us, some accuracy loss,
	 * but this is the best way with also long enough timeout length
	 *
	 * ticks_to_us = ticks * (1/CONFIG_SYS_CLOCK_TICKS_PER_SEC) * USEC_PER_SEC
	 */
	ticks_us = (uint64_t)ticks *
			(uint64_t)(USEC_PER_SEC) /
			(uint64_t)(CONFIG_SYS_CLOCK_TICKS_PER_SEC);

	/* check how many ticks for calendar, and how many for subseconds */
	ts.tv_usec = ticks_us % USEC_PER_SEC;
	ts.tv_sec = (ticks_us - ts.tv_usec) / USEC_PER_SEC;

	return ts;
}

/* TODO:
 * - check if okay changed to timeval (from timespec) > okay range with usec vs nsec?
 * - check whether problem with offsetting
 * - check ms>ticks conversion formula
 * - check whether the last ticks formula covers all combinations of tms & ticks_per_sec (lower than msec_per_sec)
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

	/* Convert calendar datetime to UNIX timestamp */

	/* RTC start time: 1st, Jan, 2000
	 * tm start: commonly 1st, Jan, 1900 (but just struct for civil time)
	 * time_t start: UNIX epoch: 1st, Jan, 1970
	 */

	/* add 100 before, since in timeutil_timegm() tm epoch considered
	 * 1900, so need to change to RTC epoch (2000)
	 */
	now.tm_year = 100 +
			__LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_YEAR(rtc_date));
	/* tm_mon allowed values are 0-11 */
	now.tm_mon = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MONTH(rtc_date)) - 1;
	now.tm_mday = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_DAY(rtc_date));

	now.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(rtc_time));
	now.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(rtc_time));
	now.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(rtc_time));

	/* Convert tm to a POSIX epoch offset in seconds.*/
	ts.tv_sec = timeutil_timegm(&now);

	/* Subtract offset of RTC (2000>1970), back to UNIX epoch */
	ts.tv_sec -= T_TIME_OFFSET;


	/* Convert subseconds value into us */
	/*
	 * formula based on stm32l1 ref manual (RM0038, 16) pg 537:
	 * Second fraction = ( PREDIV_S - SS ) / ( PREDIV_S + 1 )
	 *
	 * also see LL_RTC_TIME_GetSubSecond in stm32cube's stm32l1xx_ll_rtc.h:
	 * formula for seconds, but for timeval tv_usec microsecs required, so * time_unit
	 */
	ts.tv_usec = (RTC_SYNCH_PREDIV - rtc_subsec) * USEC_PER_SEC / (RTC_SYNCH_PREDIV+1);

	/* convert sec&usec to ms (so some accuracy loss) */
	tms = _tv_to_ms(&ts);

	/* convert ms to ticks */
	ticks = tms * CONFIG_SYS_CLOCK_TICKS_PER_SEC / MSEC_PER_SEC ;

	return ticks;
}

/*
 * TODO:
 * - add error returns
 * - !! Check if need to change to add timeout/alarm value to current time,
 * 			not absolute time (variable type limitations)
 * - !! check if problem when >1 year
 */
static int rtc_stm32_set_calendar_alarm(struct timeval alarm_tv)
{
	LL_RTC_AlarmTypeDef rtc_alarm;
	struct tm alarm_tm;

	/* From UNIX epoch (1970) to RTC epoch (2000) */
	alarm_tv.tv_sec += T_TIME_OFFSET;

	/* Convert UNIX time to civil time*/
	gmtime_r(&alarm_tv.tv_sec , &alarm_tm);

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

	/* reset subsec alarm, otherwise will still
	 * generate interrupt
	 */
	LL_RTC_ALMA_SetSubSecond(RTC, 0);
	LL_RTC_ALMA_SetSubSecondMask(RTC, 0);

	LL_RTC_ALMA_Enable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_EnableIT_ALRA(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	return 0;

}

/* TODO:
 * - add error returns?
 * - check whether this best way to do subsecond alarm setting (based on tv_usec + mask)
 *     or whether it screws with granularity / tick boundaries
 * - check whether mask [1] always okay, or causes problem with LSI,
 *     since that bit isn't 1 when 249
 * - add something that bases the mask on the ticks_per_sec?
 *      for now just smallest possible granularity
 */
static int rtc_stm32_set_subsec_alarm(struct timeval alarm_tv)
{
	LL_RTC_AlarmTypeDef rtc_alarm;

	/* reset ALARM_A calendar */
	rtc_alarm.AlarmTime.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
	rtc_alarm.AlarmTime.Hours = 0;
	rtc_alarm.AlarmTime.Minutes = 0;
	rtc_alarm.AlarmTime.Seconds = 0;

	rtc_alarm.AlarmMask = LL_RTC_ALMA_MASK_ALL;
	rtc_alarm.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_DATE;
	rtc_alarm.AlarmDateWeekDay = 0;

	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_ALMA_Disable(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	/* Set calendar alarm A */
	if (LL_RTC_ALMA_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_alarm) != SUCCESS) {
		return -EIO;
	}

	LL_RTC_DisableWriteProtection(RTC);

	/* convert usecs to subsecond register values
	 * formula adapted from subsecond>us formula, see rtc_stm32_read()
	*/
	uint32_t subsecond = RTC_SYNCH_PREDIV - ( (alarm_tv.tv_usec * (RTC_SYNCH_PREDIV+1)) / USEC_PER_SEC );

	/* check whether subsecond is between min and max data/prescaler
	 * and set subsecond alarm a
	 */
	LL_RTC_ALMA_SetSubSecond(RTC, subsecond <= RTC_SYNCH_PREDIV ? subsecond : RTC_SYNCH_PREDIV);
	LL_RTC_ALMA_SetSubSecondMask(RTC, 1);

	LL_RTC_ALMA_Enable(RTC);
	LL_RTC_ClearFlag_ALRA(RTC);
	LL_RTC_EnableIT_ALRA(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	return 0;
}


/* TODO
 * - check edge cases like other rtc timers (if non-tickless, if z_clock_set_timeout invokes isr?)
 * - check whether to add the CYCLES_PER_TICK
 */
static void rtc_stm32_isr_handler(const void *arg)
{
	ARG_UNUSED(arg);

	if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {
		/* clear flags */

		k_spinlock_key_t key = k_spin_lock(&lock);

		uint32_t now_ticks = rtc_stm32_read();

		LL_RTC_DisableWriteProtection(RTC);
		LL_RTC_ClearFlag_ALRA(RTC);
		LL_RTC_DisableIT_ALRA(RTC);
		LL_RTC_ALMA_Disable(RTC);
		LL_RTC_EnableWriteProtection(RTC);

		k_spin_unlock(&lock, key);

		/* Announce the elapsed time in ticks  */

		uint32_t dticks = now_ticks - rtc_last;

		rtc_last += dticks;

		z_clock_announce(IS_ENABLED(CONFIG_TICKLESS_KERNEL)
				? dticks : (dticks > 0));

	}

	LL_EXTI_ClearFlag_0_31(RTC_EXTI_LINE);

}

/* TODO:
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

#else /* CONFIG_STM32_RTC_TIMER_LSE */

#if defined(CONFIG_STM32_RTC_TIMER_LSE_BYPASS)
	LL_RCC_LSE_EnableBypass();
#endif /* CONFIG_STM32_RTC_TIMER_LSE_BYPASS */

	LL_RCC_LSE_Enable();

	/* Wait until LSE is ready */
	while (LL_RCC_LSE_IsReady() != 1) {
	}

	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

#endif /* CONFIG_STM32_RTC_TIMER_SRC */

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

#ifdef CONFIG_DEBUG
	/* Freeze RTC during Debug breakpoint */
	LL_DBGMCU_APB1_GRP1_FreezePeriph(LL_DBGMCU_APB1_GRP1_RTC_STOP);
#endif /* CONFIG_DEBUG */

	return 0;
}

/* TODO:
 * - check whether to use differentiation of tickless/non-tickless
 * - calculate and add max_cyc, max_tick
 * - check whether okay this way with the -rtc_last and later +, for rounding
 * 		(especially since it's updated in the isr handler, and not here)
 * - if, and if so where, to implement CYCLES_PER_TICK?
 * - check if/how to account for partial ticks (should it be -1 or +1?)
 */
void z_clock_set_timeout(int32_t ticks, bool idle)
{
	struct timeval alarm_tv;

	if (!IS_ENABLED(CONFIG_TICKLESS_KERNEL)) {
		return;
	}

	ticks = (ticks == K_TICKS_FOREVER) ? MAX_TICKS : ticks;
	ticks = CLAMP(ticks - 1, 1, (uint32_t) MAX_TICKS);

	uint32_t now_ticks = rtc_stm32_read();

	/* Compute number of RTC cycles until the next timeout. */
	/* alarm_on_ticks is tick value of when timeout occurs, not amount until */

	// can't be uint32_t bc otherwise max a few days since boot
	uint64_t alarm_on_ticks = now_ticks + ticks;

	/**** rounding off ticks *****/

	/* Add +1 in order to compensate the partially started tick.
	 * Alarm will expire between requested ticks and ticks+1.
	 * In case only 1 tick is requested, it will avoid
	 * that tick+1 event occurs before alarm setting is finished.
	 */
	//alarm_on_ticks+=1;

	// ticks > timeval (there already split in sec & usec)
	alarm_tv = rtc_stm32_ticks_to_tv(alarm_on_ticks);

	if (ticks >= CONFIG_SYS_CLOCK_TICKS_PER_SEC){
		if (!rtc_stm32_set_calendar_alarm(alarm_tv)){
			/* log something? */
		}
		else{
			/* error */
			return;
		}
	}
	else {
		if(!rtc_stm32_set_subsec_alarm(alarm_tv)){
			/* log something? */
		}
		else{
			/* error */
			return;
		}
	}
}

/* TODO:
 * - check whether to add the tickless kernel ifs
 * - check whether to add the cycles_per_tick
 * - check if need to do additional rounding
 */
/* announce to kernel how many ticks have elapsed since
 * last call to z_clock_announce()
 */
uint32_t z_clock_elapsed(void)
{
	uint32_t now_ticks = rtc_stm32_read();

	return (now_ticks - rtc_last);
}

/* TODO:
 * - check when this used, and whether additional stuff is required
 */
uint32_t z_timer_cycle_get_32(void)
{
	return rtc_stm32_read();
}
