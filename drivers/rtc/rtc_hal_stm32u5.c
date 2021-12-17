/*
 * Copyright (c) 2021 LACROIX Group
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * RTC driver for stm32u5 target using internal RTC ressources
 *
 * Use RTC Wakeup timer to wakeup application periodicaly from (TBD) ms to (TBD) seconds
 * Use Alarm A and B to wake up application on specifique time (+ weekday[0-6] or date[1-31])
 * Each Alarm is independant and can also be use to wake up application periodically on specific time every
 * 	- seconds
 *  - or minutes
 *  - or hours
 *  - or weekday [0-6]
 *  - or date [1-31]
 * 	=> for that use mask defined in rtc_alarm_mask enum
 */

#define DT_DRV_COMPAT st_stm32_rtc

#include <drivers/rtc.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <zephyr/types.h>
#include <logging/log.h>

/* DEV_DATA() macro obtains a properly typed pointer to the driver’s dev_data struct */
#define DEV_DATA(dev) ((struct rtc_stm32_data *)(dev)->data)
/* DEV_CFG() macro obtains a properly typed pointer to the driver’s config_info struct */
#define DEV_CFG(dev) ((const struct rtc_stm32_config *const)(dev)->config)

/* stm32 rtc config structure */
struct rtc_stm32_config {
	struct stm32_pclken pclken;
};

/* stm32 rtc data structure */
struct rtc_stm32_data {
	struct rtc_wakeup wut[RTC_WUT_NUM];
	struct rtc_alarm alarm[RTC_ALARM_NUM];
};

/* Select RTC clock source: LSI or LSE*/
/* todo: improve by using Kconfig or dts */
#define RTC_CLOCK_SOURCE_LSI    0
#define RTC_CLOCK_SOURCE_LSE    1
#define RTC_CLOCK_SOURCE                RTC_CLOCK_SOURCE_LSE

#if (RTC_CLOCK_SOURCE == RTC_CLOCK_SOURCE_LSI)
	#define RTC_ASYNCH_PREDIV    0x7FU
	#define RTC_SYNCH_PREDIV     0x00F9U
#elif (RTC_CLOCK_SOURCE == RTC_CLOCK_SOURCE_LSE)
	#define RTC_ASYNCH_PREDIV  0x7FU
	#define RTC_SYNCH_PREDIV   0x00FFU
#endif /* RTC_CLOCK_SOURCE */

/* Enumeration of RTC error handler code */
enum rtc_error_handler {
	RTC_E_ERR_UNKNOWN               = 0U,   /* Unknown error */
	RTC_E_ERR_INIT                  = 1U,   /* Error during HAL_RTC_Init() call */
	RTC_E_ERR_WU_TIMER_SET          = 2U,   /* Error during HAL_RTCEx_SetWakeUpTimer_IT() call */
	RTC_E_ERR_WU_TIMER_START        = 3U,   /* Error during ActivateWakeUpTimer() call */
	RTC_E_ERR_WU_TIMER_STOP         = 4U,   /* Error during HAL_RTCEx_DeactivateWakeUpTimer() call */
	RTC_E_ERR_ALARM_SET             = 5U,
	RTC_E_ERR_ALARM_START           = 6U,
	RTC_E_ERR_ALARM_STOP            = 7U,
	RTC_E_ERR_INIT_OSC              = 8U,   /* Error during HAL_RCC_OscConfig() call during RTC init */
	RTC_E_ERR_INIT_PERIPH_CLK       = 9U,   /* Error during HAL_RCCEx_PeriphCLKConfig() call during RTC init */
	RTC_E_ERR_INIT_SET_TIME         = 10U,  /* Error during time setting */
	RTC_E_ERR_INIT_SET_DATE         = 11U,  /* Error during date setting */
	RTC_E_ERR_INIT_GET_TIME         = 12U,  /* Error during time getting */
	RTC_E_ERR_INIT_GET_DATE         = 12U,  /* Error during date getting */
};

/* Wake up timer autoreload parameter */
#define RTC_STM32_WUT_AUTORELOAD_NB_BITS 16
#define RTC_MAX_AUTORELOAD      ((1 << RTC_STM32_WUT_AUTORELOAD_NB_BITS) - 1)

/* Mapping between a wakeup timer clock configuration symbol and the actual division performed */
static const uint32_t ck_div_to_val[][2] = {
	{ RTC_WAKEUPCLOCK_RTCCLK_DIV2,  2 },
	{ RTC_WAKEUPCLOCK_RTCCLK_DIV4,  4 },
	{ RTC_WAKEUPCLOCK_RTCCLK_DIV8,  8 },
	{ RTC_WAKEUPCLOCK_RTCCLK_DIV16, 16 },
	{ RTC_WAKEUPCLOCK_CK_SPRE_16BITS, 32768 },
};

/*	RTC handler declaration */
RTC_HandleTypeDef rtc_handle;

/* Register module to use logging */
LOG_MODULE_REGISTER(rtc_stm32, CONFIG_RTC_LOG_LEVEL);

static void rtc_stm32_irq_config(const struct device *dev);

// -------------------------------------------------------------------------------------------------------- //
//
//                            Local functions
//
// -------------------------------------------------------------------------------------------------------- //

/**
 * @brief RTC_ErrorHandler function is executed in case of RTC error occurrence
 *
 * @param e_ErrorHandler Error handler
 */
static void RTC_ErrorHandler(enum rtc_error_handler e_ErrorHandler)
{
	(void) e_ErrorHandler;

	/* todo: manage error */
	printk("RTC_ErrorHandler: %i \n", e_ErrorHandler);
}

/**
 * @brief RTC MSP Initialization
 *
 * @param hrtc RTC handler
 */
void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc)
{
	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	if (hrtc->Instance == RTC) {
		/*
		 * To change the source clock of the RTC feature (LSE, LSI), You have to:
		 * - Enable the power clock using __HAL_RCC_PWR_CLK_ENABLE()
		 * - Enable write access using HAL_PWR_EnableBkUpAccess() function before to
		 * 	configure the RTC clock source (to be done once after reset).
		 * - Reset the Back up Domain using __HAL_RCC_BACKUPRESET_FORCE() and
		 * 	__HAL_RCC_BACKUPRESET_RELEASE().
		 * - Configure the needed RTC clock source
		 */
		__HAL_RCC_PWR_CLK_ENABLE();
		HAL_PWR_EnableBkUpAccess();

		/* Set LSE drive capability configuration */
		__HAL_RCC_LSEDRIVE_CONFIG(RCC_BDCR_LSEDRV_1);

		/*
		 * Reset the whole backup domain, RTC included
		 * __HAL_RCC_BACKUPRESET_FORCE();
		 * __HAL_RCC_BACKUPRESET_RELEASE();
		 */

#if (RTC_CLOCK_SOURCE == RTC_CLOCK_SOURCE_LSE)
		/* Enable LSE oscillator */
		RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSE;
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
		RCC_OscInitStruct.LSEState = RCC_LSE_ON;
		if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
			RTC_ErrorHandler(RTC_E_ERR_INIT_OSC);
		}

		/* Set kernel clock source for RTC */
		PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
		PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
			RTC_ErrorHandler(RTC_E_ERR_INIT_PERIPH_CLK);
		}
#elif (RTC_CLOCK_SOURCE == RTC_CLOCK_SOURCE_LSI)
		(void) RCC_OscInitStruct;
		RCC_OscInitStruct.OscillatorType =  RCC_OSCILLATORTYPE_LSI;
		RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
		RCC_OscInitStruct.LSIState = RCC_LSI_ON;
		if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
			RTC_ErrorHandler(RTC_E_ERR_INIT_OSC);
		}

		PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
		PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
		if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
			RTC_ErrorHandler(RTC_E_ERR_INIT_PERIPH_CLK);
		}
 #else
 #error Please select the RTC Clock source
 #endif /* RTC_CLOCK_SOURCE */

		/* Enable RTC Clock */
		__HAL_RCC_RTC_ENABLE();
		__HAL_RCC_RTCAPB_CLK_ENABLE();

		/*
		 * for STOP3_MODE || STANDBY_MODE || SHUTDOWN_MODE
		 * Enable WakeUp line functionality for the RTC ALARMA
		 * HAL_PWR_EnableWakeUpPin(PWR_WAKEUP_PIN7_HIGH_3);
		 */

		/*
		 * STOP0_MODE || STOP1_MODE || STOP2_MODE
		 * Enable the Autonomous Mode for the RTC
		 */
		__HAL_RCC_RTCAPB_CLKAM_ENABLE();
	}
}

/**
 * @brief Set wake up timer
 *
 * @param hrtc RTC handler
 * @param WakeUpCounter Wake up timer value
 * @param WakeUpClock Wake up timer source clock
 * @param WakeUpAutoClr Wake up timer auto clear
 *
 * @retval HAL status.
 */
static HAL_StatusTypeDef set_wakeup_timer(RTC_HandleTypeDef *hrtc, uint32_t WakeUpCounter, uint32_t WakeUpClock, uint32_t WakeUpAutoClr)
{
	uint32_t tickstart;

	/* Check the parameters */
	assert_param(IS_RTC_WAKEUP_CLOCK(WakeUpClock));
	assert_param(IS_RTC_WAKEUP_COUNTER(WakeUpCounter));
	/* (0x0000<=WUTOCLR<=WUT) */
	assert_param(WakeUpAutoClr <= WakeUpCounter);

	/* Process Locked */
	__HAL_LOCK(hrtc);

	hrtc->State = HAL_RTC_STATE_BUSY;

	/* Disable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

	/* Clear WUTE in RTC_CR to disable the wakeup timer */
	CLEAR_BIT(RTC->CR, RTC_CR_WUTE);

	/* Clear flag Wake-Up */
	WRITE_REG(RTC->SCR, RTC_SCR_CWUTF);

	/*
	 * Poll WUTWF until it is set in RTC_ICSR to make sure the access to wakeup autoreload
	 * counter and to WUCKSEL[2:0] bits is allowed. This step must be skipped in
	 * calendar initialization mode.
	 */
	if (READ_BIT(RTC->ICSR, RTC_ICSR_INITF) == 0U) {
		tickstart = HAL_GetTick();
		while (READ_BIT(RTC->ICSR, RTC_ICSR_WUTWF) == 0U) {
			if ((HAL_GetTick() - tickstart) > RTC_TIMEOUT_VALUE) {
				/* Enable the write protection for RTC registers */
				__HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

				hrtc->State = HAL_RTC_STATE_TIMEOUT;

				/* Process Unlocked */
				__HAL_UNLOCK(hrtc);

				return HAL_TIMEOUT;
			}
		}
	}

	/* Configure the Wakeup Timer counter and auto clear value */
	WRITE_REG(RTC->WUTR, (uint32_t)(WakeUpCounter | (WakeUpAutoClr << RTC_WUTR_WUTOCLR_Pos)));

	/* Configure the clock source */
	MODIFY_REG(RTC->CR, RTC_CR_WUCKSEL, (uint32_t)WakeUpClock);

	/* Configure the Interrupt in the RTC_CR register and Enable the Wakeup Timer */
	/*  SET_BIT(RTC->CR, (RTC_CR_WUTIE | RTC_CR_WUTE)); */

	/* Enable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

	hrtc->State = HAL_RTC_STATE_READY;

	/* Process Unlocked */
	__HAL_UNLOCK(hrtc);

	return HAL_OK;
}

/**
 * @brief Start wake up timer with interrupt
 *
 * @param hrtc RTC handler
 *
 * @retval HAL status.
 */
static HAL_StatusTypeDef ActivateWakeUpTimer_IT(RTC_HandleTypeDef *hrtc)
{
	/* Process Locked */
	__HAL_LOCK(hrtc);

	hrtc->State = HAL_RTC_STATE_BUSY;

	/* Disable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

	/* Configure the Interrupt in the RTC_CR register and Enable the Wakeup Timer */
	SET_BIT(RTC->CR, (RTC_CR_WUTIE | RTC_CR_WUTE));

	/* Enable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

	hrtc->State = HAL_RTC_STATE_READY;

	/* Process Unlocked */
	__HAL_UNLOCK(hrtc);

	return HAL_OK;
}

/**
 * @brief Get stm32 alarm mask from RTC alarm mask using Zephyr APIs
 *
 * @param alarm_mask Alarm mask using Zephyr APIs
 *
 * @retval Stm32 alarm mask.
 */
static uint32_t get_stm32_alarm_mask(enum rtc_alarm_mask alarm_mask)
{
	uint32_t stm32_alarm_mask;

	switch (alarm_mask) {
	case RTC_ALARM_MASK_NONE: stm32_alarm_mask = RTC_ALARMMASK_NONE; break;
	case RTC_ALARM_MASK_DATE_WEEKDAY: stm32_alarm_mask = RTC_ALARMMASK_DATEWEEKDAY; break;
	case RTC_ALARM_MASK_HOURS: stm32_alarm_mask = RTC_ALARMMASK_HOURS; break;
	case RTC_ALARM_MASK_MIN: stm32_alarm_mask = RTC_ALARMMASK_MINUTES; break;
	case RTC_ALARM_MASK_SEC: stm32_alarm_mask = RTC_ALARMMASK_SECONDS; break;
	case RTC_ALARM_MASK_ALL: stm32_alarm_mask = RTC_ALARMMASK_ALL; break;
	default: stm32_alarm_mask = RTC_ALARMMASK_NONE; break;         // *Todo: add unknow mask message */
	}
	return stm32_alarm_mask;
}

/**
 * @brief Get stm32 alarm date or weekday selection using Zephyr APIs
 *
 * @param alarm_date_weekday_sel Alarm date or weekday selection using Zephyr APIs
 *
 * @retval Stm32 date or weekday selection.
 */
static uint32_t get_stm32_alarm_date_wday_sel(enum rtc_alarm_date_weekday alarm_date_weekday_sel)
{
	uint32_t stm32_date_weekday_sel;

	switch (alarm_date_weekday_sel) {
	case RTC_ALARM_DATE_SEL: stm32_date_weekday_sel = RTC_ALARMDATEWEEKDAYSEL_DATE; break;
	case RTC_ALARM_WEEKDAY_SEL: stm32_date_weekday_sel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY; break;
	default: stm32_date_weekday_sel = RTC_ALARMDATEWEEKDAYSEL_WEEKDAY; break;         /* Todo: add unknow selection message */
	}
	return stm32_date_weekday_sel;
}

/**
 * @brief Get stm32 weekday value using Zephyr APIs
 *
 * @param tm_wday Weekday value using Zephyr APIs
 *
 * @retval Stm32 weekday value.
 */
static uint8_t get_stm32_wday_val(int tm_wday)
{
	uint8_t stm32_weekdayVal;

	/* Todo : check range of tm_wday => from 0 to 6 (Zephyr APIs) */
	if (tm_wday == 0) {
		stm32_weekdayVal = 7;   /* Sunday (STM32 APIs) */
	} else {
		stm32_weekdayVal = (uint8_t)tm_wday;
	}

	return stm32_weekdayVal;
}

/**
 * @brief Get stm32 alarm date or weekday value  using Zephyr APIs
 *
 * @param alarm_date_weekday_sel Alarm date or Weekday value using Zephyr APIs
 *
 * @retval Stm32 alarm date or Weekday value.
 */
static uint8_t GetStm32AlarmDateOrWeekdayVal(struct tm alarm_time, enum rtc_alarm_date_weekday alarm_date_weekday_sel)
{
	uint8_t stm32_date_weekdayVal = 0U;

	switch (alarm_date_weekday_sel) {
	case RTC_ALARM_DATE_SEL:
		// Todo : check range of alarm_time.tm_mday => from 1 to 31
		stm32_date_weekdayVal = alarm_time.tm_mday;
		break;
	case RTC_ALARM_WEEKDAY_SEL:
		stm32_date_weekdayVal = get_stm32_wday_val(alarm_time.tm_wday);
		break;
	default:
		// Todo: add unknow selection message
		break;
	}

	return stm32_date_weekdayVal;
}

/**
 * @brief  Set the specified RTC Alarm.
 *
 * @param  hrtc RTC handle
 * @param  sAlarm Pointer to Alarm structure
 *          if Binary mode is RTC_BINARY_ONLY, 3 fields only are used
 *             sAlarm->AlarmTime.SubSeconds
 *             sAlarm->AlarmSubSecondMask
 *             sAlarm->BinaryAutoClr
 * @param  Format Specifies the format of the entered parameters.
 *          if Binary mode is RTC_BINARY_ONLY, this parameter is not used
 *          else this parameter can be one of the following values
 *             @arg RTC_FORMAT_BIN: Binary format
 *             @arg RTC_FORMAT_BCD: BCD format
 *
 * @retval HAL status
 */
static HAL_StatusTypeDef set_alarm(RTC_HandleTypeDef *hrtc, RTC_AlarmTypeDef *sAlarm, uint32_t Format)
{
	uint32_t tmpreg = 0, binaryMode;

	/* Process Locked */
	__HAL_LOCK(hrtc);

	hrtc->State = HAL_RTC_STATE_BUSY;

#ifdef  USE_FULL_ASSERT
	/* Check the parameters depending of the Binary mode (32-bit free-running counter configuration). */
	if (READ_BIT(RTC->ICSR, RTC_ICSR_BIN) == RTC_BINARY_NONE) {
		assert_param(IS_RTC_FORMAT(Format));
		assert_param(IS_RTC_ALARM(sAlarm->Alarm));
		assert_param(IS_RTC_ALARM_MASK(sAlarm->AlarmMask));
		assert_param(IS_RTC_ALARM_DATE_WEEKDAY_SEL(sAlarm->AlarmDateWeekDaySel));
		assert_param(IS_RTC_ALARM_SUB_SECOND_VALUE(sAlarm->AlarmTime.SubSeconds));
		assert_param(IS_RTC_ALARM_SUB_SECOND_MASK(sAlarm->AlarmSubSecondMask));
	} else if (READ_BIT(RTC->ICSR, RTC_ICSR_BIN) == RTC_BINARY_ONLY) {
		assert_param(IS_RTC_ALARM_SUB_SECOND_BINARY_MASK(sAlarm->AlarmSubSecondMask));
		assert_param(IS_RTC_ALARMSUBSECONDBIN_AUTOCLR(sAlarm->BinaryAutoClr));
	} else {  /* RTC_BINARY_MIX */
		assert_param(IS_RTC_FORMAT(Format));
		assert_param(IS_RTC_ALARM(sAlarm->Alarm));
		assert_param(IS_RTC_ALARM_MASK(sAlarm->AlarmMask));
		assert_param(IS_RTC_ALARM_DATE_WEEKDAY_SEL(sAlarm->AlarmDateWeekDaySel));
		/* In Binary Mix Mode, the RTC can not generate an alarm on a match
		   involving all calendar items + the upper SSR bits */
		assert_param((sAlarm->AlarmSubSecondMask >> RTC_ALRMASSR_MASKSS_Pos) <= (8U + (READ_BIT(RTC->ICSR, RTC_ICSR_BCDU) >> RTC_ICSR_BCDU_Pos)));
	}
#endif /* USE_FULL_ASSERT */

	/* Get Binary mode (32-bit free-running counter configuration) */
	binaryMode = READ_BIT(RTC->ICSR, RTC_ICSR_BIN);

	if (binaryMode != RTC_BINARY_ONLY) {
		if (Format == RTC_FORMAT_BIN) {
			if (READ_BIT(RTC->CR, RTC_CR_FMT) != 0U) {
				assert_param(IS_RTC_HOUR12(sAlarm->AlarmTime.Hours));
				assert_param(IS_RTC_HOURFORMAT12(sAlarm->AlarmTime.TimeFormat));
			} else {
				sAlarm->AlarmTime.TimeFormat = 0x00U;
				assert_param(IS_RTC_HOUR24(sAlarm->AlarmTime.Hours));
			}
			assert_param(IS_RTC_MINUTES(sAlarm->AlarmTime.Minutes));
			assert_param(IS_RTC_SECONDS(sAlarm->AlarmTime.Seconds));

			if (sAlarm->AlarmDateWeekDaySel == RTC_ALARMDATEWEEKDAYSEL_DATE) {
				assert_param(IS_RTC_ALARM_DATE_WEEKDAY_DATE(sAlarm->AlarmDateWeekDay));
			} else {
				assert_param(IS_RTC_ALARM_DATE_WEEKDAY_WEEKDAY(sAlarm->AlarmDateWeekDay));
			}
			tmpreg = (((uint32_t)RTC_ByteToBcd2(sAlarm->AlarmTime.Hours) << RTC_ALRMAR_HU_Pos) |	\
				  ((uint32_t)RTC_ByteToBcd2(sAlarm->AlarmTime.Minutes) << RTC_ALRMAR_MNU_Pos) |	\
				  ((uint32_t)RTC_ByteToBcd2(sAlarm->AlarmTime.Seconds) << RTC_ALRMAR_SU_Pos) |	\
				  ((uint32_t)(sAlarm->AlarmTime.TimeFormat) << RTC_ALRMAR_PM_Pos) |		\
				  ((uint32_t)RTC_ByteToBcd2(sAlarm->AlarmDateWeekDay) << RTC_ALRMAR_DU_Pos) |	\
				  ((uint32_t)sAlarm->AlarmDateWeekDaySel) |					\
				  ((uint32_t)sAlarm->AlarmMask));
		} else {  /* Format BCD */
			if (READ_BIT(RTC->CR, RTC_CR_FMT) != 0U) {
				assert_param(IS_RTC_HOUR12(RTC_Bcd2ToByte(sAlarm->AlarmTime.Hours)));
				assert_param(IS_RTC_HOURFORMAT12(sAlarm->AlarmTime.TimeFormat));
			} else {
				sAlarm->AlarmTime.TimeFormat = 0x00U;
				assert_param(IS_RTC_HOUR24(RTC_Bcd2ToByte(sAlarm->AlarmTime.Hours)));
			}

			assert_param(IS_RTC_MINUTES(RTC_Bcd2ToByte(sAlarm->AlarmTime.Minutes)));
			assert_param(IS_RTC_SECONDS(RTC_Bcd2ToByte(sAlarm->AlarmTime.Seconds)));

#ifdef  USE_FULL_ASSERT
			if (sAlarm->AlarmDateWeekDaySel == RTC_ALARMDATEWEEKDAYSEL_DATE) {
				assert_param(IS_RTC_ALARM_DATE_WEEKDAY_DATE(RTC_Bcd2ToByte(sAlarm->AlarmDateWeekDay)));
			} else {
				assert_param(IS_RTC_ALARM_DATE_WEEKDAY_WEEKDAY(RTC_Bcd2ToByte(sAlarm->AlarmDateWeekDay)));
			}

#endif /* USE_FULL_ASSERT */
			tmpreg = (((uint32_t)(sAlarm->AlarmTime.Hours) << RTC_ALRMAR_HU_Pos) |	    \
				  ((uint32_t)(sAlarm->AlarmTime.Minutes) << RTC_ALRMAR_MNU_Pos) |   \
				  ((uint32_t)(sAlarm->AlarmTime.Seconds) << RTC_ALRMAR_SU_Pos) |    \
				  ((uint32_t)(sAlarm->AlarmTime.TimeFormat) << RTC_ALRMAR_PM_Pos) | \
				  ((uint32_t)(sAlarm->AlarmDateWeekDay) << RTC_ALRMAR_DU_Pos) |	    \
				  ((uint32_t)sAlarm->AlarmDateWeekDaySel) |			    \
				  ((uint32_t)sAlarm->AlarmMask));

		}
	}

	/* Disable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

	/* Configure the Alarm registers */
	if (sAlarm->Alarm == RTC_ALARM_A) {
		/* Disable the Alarm A interrupt */
		CLEAR_BIT(RTC->CR, RTC_CR_ALRAE | RTC_CR_ALRAIE);
		/* Clear flag alarm A */
		WRITE_REG(RTC->SCR, RTC_SCR_CALRAF);

		if (binaryMode == RTC_BINARY_ONLY) {
			RTC->ALRMASSR = sAlarm->AlarmSubSecondMask | sAlarm->BinaryAutoClr;
		} else {
			WRITE_REG(RTC->ALRMAR, tmpreg);
			WRITE_REG(RTC->ALRMASSR, sAlarm->AlarmSubSecondMask);
		}

		WRITE_REG(RTC->ALRABINR, sAlarm->AlarmTime.SubSeconds);

		if (sAlarm->FlagAutoClr == ALARM_FLAG_AUTOCLR_ENABLE) {
			/* Configure the  Alarm A output clear */
			SET_BIT(RTC->CR, RTC_CR_ALRAOCLR);
		} else {
			/* Disable the  Alarm A output clear*/
			CLEAR_BIT(RTC->CR, RTC_CR_ALRAOCLR);
		}

		/* Configure the Alarm interrupt */
		/*  SET_BIT(RTC->CR, RTC_CR_ALRAE | RTC_CR_ALRAIE); */
	} else {
		/* Disable the Alarm B interrupt */
		CLEAR_BIT(RTC->CR, RTC_CR_ALRBE | RTC_CR_ALRBIE);
		/* Clear flag alarm B */
		WRITE_REG(RTC->SCR, RTC_SCR_CALRBF);

		if (binaryMode == RTC_BINARY_ONLY) {
			WRITE_REG(RTC->ALRMBSSR, sAlarm->AlarmSubSecondMask | sAlarm->BinaryAutoClr);
		} else {
			WRITE_REG(RTC->ALRMBR, tmpreg);
			WRITE_REG(RTC->ALRMBSSR, sAlarm->AlarmSubSecondMask);
		}

		WRITE_REG(RTC->ALRBBINR, sAlarm->AlarmTime.SubSeconds);

		if (sAlarm->FlagAutoClr == ALARM_FLAG_AUTOCLR_ENABLE) {
			/* Configure the  Alarm B Output clear */
			SET_BIT(RTC->CR, RTC_CR_ALRBOCLR);
		} else {
			/* Disable the  Alarm B Output clear */
			CLEAR_BIT(RTC->CR, RTC_CR_ALRBOCLR);
		}

		/* Configure the Alarm interrupt */
		/* SET_BIT(RTC->CR, RTC_CR_ALRBE | RTC_CR_ALRBIE); */
	}

	/* Enable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

	hrtc->State = HAL_RTC_STATE_READY;

	/* Process Unlocked */
	__HAL_UNLOCK(hrtc);

	return HAL_OK;
}

/**
 * @brief Start Alarm A or B with interrupt
 *
 * @param hrtc: RTC handle pointer
 * @param stm32_alarm_id: Alarm id (Stm32 format)
 *
 * @retval HAL status
 */
static HAL_StatusTypeDef activate_alarm_it(RTC_HandleTypeDef *hrtc, uint32_t stm32_alarm_id)
{
	assert_param(IS_RTC_ALARM(stm32_alarm_id));

	/* Process Locked */
	__HAL_LOCK(hrtc);

	hrtc->State = HAL_RTC_STATE_BUSY;

	/* Disable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_DISABLE(hrtc);

	/* Configure the Alarm interrupt */
	if (stm32_alarm_id == RTC_ALARM_A) {
		SET_BIT(RTC->CR, RTC_CR_ALRAE | RTC_CR_ALRAIE);
	} else {   /* RTC_ALARM_B */
		SET_BIT(RTC->CR, RTC_CR_ALRBE | RTC_CR_ALRBIE);
	}

	/* Enable the write protection for RTC registers */
	__HAL_RTC_WRITEPROTECTION_ENABLE(hrtc);

	hrtc->State = HAL_RTC_STATE_READY;

	/* Process Unlocked */
	__HAL_UNLOCK(hrtc);

	return HAL_OK;
}

/**
 * @brief Identify Wake up timer set-up parameters
 *
 * According wut period in milliseconds, identify RTC divider and autoreload values
 *
 * @param period_ms: Wake up timer period in milliseconds
 * @param clk_source_freq: RTC clock source frequency (32768Hz...)
 * @param clk_divider: Pointer to calculated RTC clock divider
 * @param autoreload: Pointer to calculated RTC autoreload value
 *
 * @retval 0		On success.
 * @retval -EINVAL	If setup is invalid.
 */
static int identify_wut_parameters(uint32_t period_ms, uint32_t clk_source_freq,
				   uint32_t *clk_divider, uint32_t *autoreload)
{
	int i, ret = 0;
	uint32_t divider = 0U;
	uint64_t max_period_ms = 0U;
	bool setup_found = false;

	/* Start with lowest division for maximum resolution */
	for (i = 0, setup_found = false; i < ARRAY_SIZE(ck_div_to_val) && !setup_found; i++) {
		divider = ck_div_to_val[i][1];
		max_period_ms = divider * RTC_MAX_AUTORELOAD * 1000 / clk_source_freq;

		if (max_period_ms >= period_ms) {
			*clk_divider = ck_div_to_val[i][0];
			*autoreload = period_ms * clk_source_freq / divider / 1000 - 1;
			setup_found = true;
		}
	}

	if (!setup_found) {
		ret = -EINVAL;
	}

	return ret;
}

// -------------------------------------------------------------------------------------------------------- //
//
//                            ZEPHYR APIs for RTC driver
//
// -------------------------------------------------------------------------------------------------------- //

static int rtc_stm32_set_current_time(const struct device *dev, const struct tm date_time)
{
	RTC_TimeTypeDef sTime = { 0 };
	RTC_DateTypeDef sDate = { 0 };
	uint32_t Format = RTC_FORMAT_BIN;       /* versus RTC_FORMAT_BCD */

	/* Check date_time limits */
	/* todo: return -EINVAL if date or time settings are invalid */

	/* Set the Time */
	sTime.Hours = (uint8_t)date_time.tm_hour;
	sTime.Minutes = (uint8_t)date_time.tm_min;
	sTime.Seconds = (uint8_t)date_time.tm_sec;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	if (HAL_RTC_SetTime(&rtc_handle, &sTime, Format) != HAL_OK) {
		RTC_ErrorHandler(RTC_E_ERR_INIT_SET_TIME);
		return -ENOTSUP;
	}
	/* Set the Date */
	sDate.WeekDay = get_stm32_wday_val(date_time.tm_wday);
	sDate.Year = (uint8_t)date_time.tm_year;
	sDate.Month = (uint8_t)date_time.tm_mon;
	sDate.Date = (uint8_t)date_time.tm_mday;

	if (HAL_RTC_SetDate(&rtc_handle, &sDate, Format) != HAL_OK) {
		RTC_ErrorHandler(RTC_E_ERR_INIT_SET_DATE);
		return -ENOTSUP;
	}

	return 0;
}

static int rtc_stm32_get_current_time(const struct device *dev, struct tm *date_time)
{
	RTC_DateTypeDef date = { 0 };
	RTC_TimeTypeDef time = { 0 };
	uint32_t Format = RTC_FORMAT_BIN;       /* versus RTC_FORMAT_BCD */

	/* Get the RTC current Time */
	if (HAL_RTC_GetTime(&rtc_handle, &time, Format) != HAL_OK) {
		RTC_ErrorHandler(RTC_E_ERR_INIT_GET_TIME);
		return -ENOTSUP;
	}
	/* Get the RTC current Date */
	if (HAL_RTC_GetDate(&rtc_handle, &date, Format) != HAL_OK) {
		RTC_ErrorHandler(RTC_E_ERR_INIT_GET_DATE);
		return -ENOTSUP;
	}

	date_time->tm_year = (int)date.Year;
	date_time->tm_mon = (int)date.Month;
	date_time->tm_mday = (int)date.Date;
	date_time->tm_wday = (int)date.WeekDay;

	date_time->tm_hour = (int)time.Hours;
	date_time->tm_min = (int)time.Minutes;
	date_time->tm_sec = (int)time.Seconds;

	return 0;
}

static int rtc_stm32_set_wakeup_timer(const struct device *dev, struct rtc_wakeup wut, enum rtc_wakeup_id wut_id)
{
	struct rtc_stm32_data *data = DEV_DATA(dev);
	uint32_t wut_autoreload = 0U;           /* Wake up timer autoreload value */
	uint32_t wut_source_clock = 0U;         /* Wake up timer source clock */
	uint32_t wut_auto_clear = 0U;           /* Wake up timer auto clear value */

	/* Check wake up timer id: only 1 rtc wake up timer for stm32u5 */
	if (wut_id == RTC_WUT0) {
		/* Calculated wut parameters */
		if (0 == identify_wut_parameters(wut.period, 32768U, &wut_source_clock, &wut_autoreload)) {
			data->wut[wut_id].period = wut.period;
			data->wut[wut_id].callback = wut.callback;
			wut_auto_clear = 0U;
		} else {
			return -EINVAL;
		}

		if (set_wakeup_timer(&rtc_handle, wut_autoreload, wut_source_clock, wut_auto_clear) != HAL_OK) {
			RTC_ErrorHandler(RTC_E_ERR_WU_TIMER_SET);
			return -ENOTSUP;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int rtc_stm32_start_wakeup_timer(const struct device *dev, enum rtc_wakeup_id wut_id)
{
	/* Check wake up timer id: only 1 rtc wake up timer for stm32u5 */
	if (wut_id == RTC_WUT0) {
		if (ActivateWakeUpTimer_IT(&rtc_handle) != HAL_OK) {
			RTC_ErrorHandler(RTC_E_ERR_WU_TIMER_START);
			return -ENOTSUP;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int rtc_stm32_stop_wakeup_timer(const struct device *dev, enum rtc_wakeup_id wut_id)
{
	/* Check wake up timer id: only 1 rtc wake up timer for stm32u5 */
	if (wut_id == RTC_WUT0) {
		if (HAL_RTCEx_DeactivateWakeUpTimer(&rtc_handle) != HAL_OK) {
			RTC_ErrorHandler(RTC_E_ERR_WU_TIMER_STOP);
			return -ENOTSUP;
		}
	} else {
		return -EINVAL;
	}

	return 0;
}

static int rtc_stm32_set_alarm(const struct device *dev, struct rtc_alarm alarm, enum rtc_alarm_id alarm_id)
{
	struct rtc_stm32_data *data = DEV_DATA(dev);
	RTC_AlarmTypeDef sAlarm = { 0 };
	uint32_t Format = RTC_FORMAT_BIN;               /* versus RTC_FORMAT_BCD */

	/* Check alarm id */
	if (alarm_id == RTC_ALARMA) {
		sAlarm.Alarm = RTC_ALARM_A;
	} else if (alarm_id == RTC_ALARMB) {
		sAlarm.Alarm = RTC_ALARM_B;
	} else {
		return -EINVAL;
	}

	/* Record alarm to the rtc data */
	data->alarm[alarm_id].alarm_time.tm_year = alarm.alarm_time.tm_year;    /* Year since 1900 */
	data->alarm[alarm_id].alarm_time.tm_mon = alarm.alarm_time.tm_mon;      /* month of year [0,11], where 0 = jan */
	data->alarm[alarm_id].alarm_time.tm_mday = alarm.alarm_time.tm_mday;    /* Day of month [1,31] */
	data->alarm[alarm_id].alarm_time.tm_hour = alarm.alarm_time.tm_hour;    /* Hour [0,23] */
	data->alarm[alarm_id].alarm_time.tm_min = alarm.alarm_time.tm_min;      /* Minutes [0,59] */
	data->alarm[alarm_id].alarm_time.tm_sec = alarm.alarm_time.tm_sec;      /* Seconds [0,61] */
	data->alarm[alarm_id].alarm_time.tm_isdst = alarm.alarm_time.tm_isdst;  /* Daylight savings flag: Is DST on? 1 = yes, 0 = no, -1 = unknown */
	data->alarm[alarm_id].alarm_time.tm_wday = alarm.alarm_time.tm_wday;    /* Day of week [0,6] (Sunday = 0) */
	data->alarm[alarm_id].alarm_time.tm_yday = alarm.alarm_time.tm_yday;    /* Day of year [0,365] */
	data->alarm[alarm_id].alarm_date_weekday_sel = alarm.alarm_date_weekday_sel;
	data->alarm[alarm_id].alarm_mask = alarm.alarm_mask;
	data->alarm[alarm_id].callback = alarm.callback;

	/* Prepare alarm to the stm32 format */
	sAlarm.AlarmTime.Hours = (uint8_t)alarm.alarm_time.tm_hour;
	sAlarm.AlarmTime.Minutes = (uint8_t)alarm.alarm_time.tm_min;
	sAlarm.AlarmTime.Seconds = (uint8_t)alarm.alarm_time.tm_sec;
	sAlarm.AlarmTime.SubSeconds = 0U;
	sAlarm.AlarmMask = get_stm32_alarm_mask(alarm.alarm_mask);
	sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
	sAlarm.AlarmDateWeekDaySel = get_stm32_alarm_date_wday_sel(alarm.alarm_date_weekday_sel);
	sAlarm.AlarmDateWeekDay =  GetStm32AlarmDateOrWeekdayVal(alarm.alarm_time, alarm.alarm_date_weekday_sel);

	if (set_alarm(&rtc_handle, &sAlarm, Format) != HAL_OK) {
		RTC_ErrorHandler(RTC_E_ERR_ALARM_SET);
		return -ENOTSUP;
	}

	return 0;
}

static int rtc_stm32_start_alarm(const struct device *dev, enum rtc_alarm_id alarm_id)
{
	uint32_t stm32_alarm_id = 0;

	/* Check alarm id */
	if (alarm_id == RTC_ALARMA) {
		stm32_alarm_id = RTC_ALARM_A;

	} else if (alarm_id == RTC_ALARMB) {
		stm32_alarm_id = RTC_ALARM_B;
	} else {
		return -EINVAL;
	}

	if (activate_alarm_it(&rtc_handle, stm32_alarm_id) != HAL_OK) {
		RTC_ErrorHandler(RTC_E_ERR_ALARM_START);
		return -ENOTSUP;
	}

	return 0;
}

static int rtc_stm32_stop_alarm(const struct device *dev, enum rtc_alarm_id alarm_id)
{
	uint32_t stm32_alarm_id = 0;

	/** Check alarm id */
	if (alarm_id == RTC_ALARMA) {
		stm32_alarm_id = RTC_ALARM_A;
	} else if (alarm_id == RTC_ALARMB) {
		stm32_alarm_id = RTC_ALARM_B;
	} else {
		return -EINVAL;
	}

	if (HAL_RTC_DeactivateAlarm(&rtc_handle, stm32_alarm_id) != HAL_OK) {
		RTC_ErrorHandler(RTC_E_ERR_ALARM_STOP);
		return -ENOTSUP;
	}

	return 0;
}

/**
 * @brief rtc interrupt
 * @param dev Pointeur to the device instance
 */
void rtc_stm32_isr(const struct device *dev)
{
	struct rtc_stm32_data *data = DEV_DATA(dev);
	uint32_t misr_reg = READ_REG(RTC->MISR);

	/* Get the pending status of the WAKEUPTIMER Interrupt */
	if (READ_BIT(RTC->MISR, RTC_MISR_WUTMF) != 0U) {
		/* Clear the WAKEUPTIMER interrupt pending bit */
		WRITE_REG(RTC->SCR, RTC_SCR_CWUTF);

		/* WAKEUPTIMER callback */
		data->wut[RTC_WUT0].callback(dev);
	}
	/* Get the pending status of the AlarmA Interrupt */
	else if ((misr_reg & RTC_MISR_ALRAMF) != 0U) {
		/* Clear the AlarmA interrupt pending bit */
		WRITE_REG(RTC->SCR, RTC_SCR_CALRAF);

		/* AlarmA callback */
		data->alarm[RTC_ALARMA].callback(dev);
	}
	/* Get the pending status of the AlarmB Interrupt */
	else if ((misr_reg & RTC_MISR_ALRBMF) != 0U) {
		/* Clear the AlarmA interrupt pending bit */
		WRITE_REG(RTC->SCR, RTC_SCR_CALRBF);

		/* AlarmB callback */
		data->alarm[RTC_ALARMB].callback(dev);
	}

	/* Change RTC state */
	rtc_handle.State = HAL_RTC_STATE_READY;
}

/**
 * @brief Init specified rtc device
 * @param dev Pointeur to the device instance
 * @retval 0 if successful
 */
static int rtc_stm32_init(const struct device *dev)
{
	int ret = 0;

	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct rtc_stm32_config *cfg = DEV_CFG(dev);

	if (clock_control_on(clk, (clock_control_subsys_t *) &cfg->pclken) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	HAL_Init();

	/* Set RTC Instance */
	rtc_handle.Instance = RTC;

	/* Set parameter to be configured */
	rtc_handle.Init.HourFormat = RTC_HOURFORMAT_24;
	rtc_handle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
	rtc_handle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
	rtc_handle.Init.OutPut = RTC_OUTPUT_DISABLE; /* versus RTC_OUTPUT_WAKEUP; */
	rtc_handle.Init.OutPutRemap = RTC_OUTPUT_REMAP_NONE;
	rtc_handle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	rtc_handle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN; /* versus RTC_OUTPUT_TYPE_PUSHPULL */
	rtc_handle.Init.OutPutPullUp = RTC_OUTPUT_PULLUP_NONE;  /* versus RTC_OUTPUT_PULLUP_ON */

	/* Initialize the RTC peripher */
	if (HAL_RTC_Init(&rtc_handle) != HAL_OK) {
		RTC_ErrorHandler(RTC_E_ERR_INIT);
	}

	rtc_stm32_irq_config(dev);

	return ret;
}

// -------------------------------------------------------------------------------------------------------- //
// Device driver conveniences
// -------------------------------------------------------------------------------------------------------- //
static const struct rtc_driver_api rtc_stm32_driver_api = {
	.set_time = rtc_stm32_set_current_time,
	.get_time = rtc_stm32_get_current_time,
	.set_wakeup_timer = rtc_stm32_set_wakeup_timer,
	.start_wakeup_timer = rtc_stm32_start_wakeup_timer,
	.stop_wakeup_timer = rtc_stm32_stop_wakeup_timer,
	.set_alarm = rtc_stm32_set_alarm,
	.start_alarm = rtc_stm32_start_alarm,
	.stop_alarm = rtc_stm32_stop_alarm,
};

static struct rtc_stm32_data rtc_data;

static const struct rtc_stm32_config rtc_config = {
	.pclken = {
		.enr = DT_INST_CLOCKS_CELL(0, bits),
		.bus = DT_INST_CLOCKS_CELL(0, bus),
	},
};

DEVICE_DT_INST_DEFINE(0, &rtc_stm32_init, NULL,
		      &rtc_data, &rtc_config, PRE_KERNEL_1,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &rtc_stm32_driver_api);


static void rtc_stm32_irq_config(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    rtc_stm32_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}
