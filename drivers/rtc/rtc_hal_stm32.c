/*
 * Copyright (c) 2018 Workaround GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Source file for the STM32 RTC driver
 *
 */

#include <errno.h>

#include <device.h>
#include <init.h>
#include <kernel.h>
#include <rtc.h>
#include <power.h>
#include <soc.h>
#include <interrupt_controller/exti_stm32.h>

#ifdef CONFIG_SOC_SERIES_STM32F4X
#define STM32F4_EXTI_RTC_ALARM  17
#endif

/* Configuration data */
struct rtc_stm32_config {
	u32_t rtc_base;
};

/* Runtime driver data */
struct rtc_stm32_data {
	/* RTC peripheral handler */
	RTC_HandleTypeDef hrtc;
	void (*cb_fn)(struct device *dev);
	struct k_sem sem;
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
#endif
};

#define GET_SEM(dev) (&((struct rtc_stm32_data *)(dev->driver_data))->sem)

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

static void rtc_stm32_set_power_state(struct device *dev, u32_t power_state)
{
	struct rtc_stm32_data *rtc_data = dev->driver_data;

	rtc_data->device_power_state = power_state;
}

static u32_t rtc_stm32_get_power_state(struct device *dev)
{
	struct rtc_stm32_data *rtc_data = dev->driver_data;

	return rtc_data->device_power_state;
}
#else
#define rtc_stm32_set_power_state(...)
#endif
static int rtc_stm32_set_alarm(struct device *dev, const u32_t alarm_val);

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
}

static void rtc_stm32_enable(struct device *dev)
{
	__HAL_RCC_RTC_ENABLE();
}

static void rtc_stm32_disable(struct device *dev)
{
	__HAL_RCC_RTC_DISABLE();
}

static int rtc_stm32_set_config(struct device *dev, struct rtc_config *cfg)
{
	struct rtc_stm32_data *rtc_data = dev->driver_data;
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	int result = 0;

	if (cfg->cb_fn != NULL) {
		rtc_data->cb_fn = cfg->cb_fn;
	}

	sDate.Year    = 0;
	sDate.Month   = 0;
	sDate.Date    = 0;
	sDate.WeekDay = 0;

	k_sem_take(GET_SEM(dev), K_FOREVER);

	result = HAL_RTC_SetDate(&rtc_data->hrtc, &sDate, RTC_FORMAT_BIN);
	if (result) {
		result = -EIO;
	}

	sTime.Hours   = ((cfg->init_val / 1000) / 3600) % 24;
	sTime.Minutes = ((cfg->init_val / 1000) % 3600) / 60;
	sTime.Seconds = (cfg->init_val / 1000) % 60;

	result = HAL_RTC_SetTime(&rtc_data->hrtc, &sTime, RTC_FORMAT_BIN);
	if (result) {
		result = -EIO;
	}

	k_sem_give(GET_SEM(dev));

	return result;
}

static int rtc_stm32_set_alarm(struct device *dev, const u32_t alarm_val)
{
	struct rtc_stm32_data *rtc_data = dev->driver_data;
	RTC_AlarmTypeDef RTC_AlarmStructure;
	int result = 0;

	/* Set the alarm X+alarm_val */
	RTC_AlarmStructure.Alarm                = RTC_ALARM_A;
	RTC_AlarmStructure.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;
	RTC_AlarmStructure.AlarmTime.Hours      = ((alarm_val/1000)/3600)%24;
	RTC_AlarmStructure.AlarmTime.Minutes    = ((alarm_val/1000)%3600)/60;
	RTC_AlarmStructure.AlarmTime.Seconds    = (alarm_val/1000)%60;
	RTC_AlarmStructure.AlarmDateWeekDay     = ((alarm_val/1000)/3600)/24;
	RTC_AlarmStructure.AlarmDateWeekDaySel  = RTC_ALARMDATEWEEKDAYSEL_DATE;
	RTC_AlarmStructure.AlarmMask
		= RTC_ALARMMASK_DATEWEEKDAY|RTC_ALARMMASK_HOURS;
	RTC_AlarmStructure.AlarmSubSecondMask   = RTC_ALARMSUBSECONDMASK_NONE;
	result = HAL_RTC_SetAlarm_IT(&rtc_data->hrtc,
				     &RTC_AlarmStructure,
				     RTC_FORMAT_BIN);
	if (result) {
		result = -EIO;
	}
	return result;
}

static u32_t rtc_stm32_read(struct device *dev)
{
	struct rtc_stm32_data *rtc_data = dev->driver_data;
	RTC_TimeTypeDef sTime;
	RTC_DateTypeDef sDate;
	u32_t TimeMs, fromMinutes, fromSeconds, fromSubSeconds;
	int result;

	result = HAL_RTC_GetDate(&rtc_data->hrtc, &sDate, RTC_FORMAT_BIN);
	if (result) {
		return -EIO;
	}
	result = HAL_RTC_GetTime(&rtc_data->hrtc, &sTime, RTC_FORMAT_BIN);
	if (result) {
		return -EIO;
	}

	/* Calculate number of milli-seconds from each sTime attributes */
	fromMinutes    = 60 * 1000 * sTime.Minutes;
	fromSeconds    = 1000 * sTime.Seconds;
	fromSubSeconds = 1000 * sTime.SubSeconds / (sTime.SecondFraction + 1);

	TimeMs = fromMinutes + fromSeconds + fromSubSeconds;

	return TimeMs;
}

static u32_t rtc_stm32_get_pending_int(struct device *dev)
{
	/* Not available for STM32 RTC */
	return 0;
}

#if defined(CONFIG_SOC_SERIES_STM32F4X) || defined(CONFIG_SOC_SERIES_STM32L4X)
void rtc_stm32_isr(int line, void *userdata)
{
	struct device * const dev = (struct device *)userdata;
	struct rtc_stm32_data *rtc_data = dev->driver_data;

	HAL_RTC_AlarmIRQHandler(&rtc_data->hrtc);
	rtc_data->cb_fn(dev);
}
#endif

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT

#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
static int rtc_suspend_device(struct device *dev)
{
	__HAL_RCC_RTC_DISABLE();
	rtc_stm32_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);
	return 0;
}

static int rtc_resume_device(struct device *dev)
{
	__HAL_RCC_RTC_ENABLE();
	rtc_stm32_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}
#endif

/*
 * Implements the driver control management functionality
 * the *context may include IN data or/and OUT data
 */
static int rtc_stm32_device_ctrl(struct device *dev, u32_t ctrl_command,
				 void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
#ifdef CONFIG_SYS_POWER_DEEP_SLEEP
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return rtc_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return rtc_resume_device(dev);
		}
#endif
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = rtc_stm32_get_power_state(dev);
		return 0;
	}

	return 0;
}
#endif

static void rtc_stm32_config_irq(void);

static int rtc_stm32_init(struct device *dev)
{
	RCC_OscInitTypeDef        RCC_OscInitStruct;
	RCC_PeriphCLKInitTypeDef  PeriphClkInitStruct;

	struct rtc_stm32_data *context = dev->driver_data;

	/* Initialize device semaphore */
	k_sem_init(GET_SEM(dev), 0, UINT_MAX);
	k_sem_give(GET_SEM(dev));

	rtc_stm32_config_irq();

	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_PWR_EnableBkUpAccess();


	/* Configure LSI as RTC clock source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
	RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_NONE;
	RCC_OscInitStruct.LSIState       = RCC_LSI_ON;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		return -EIO;
	}

	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
	PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
		return -EIO;
	}

	__HAL_RCC_RTC_ENABLE();

	context->hrtc.Instance = (RTC_TypeDef *)CONFIG_RTC_0_BASE_ADDRESS;

	context->hrtc.Init.HourFormat   = RTC_HOURFORMAT_24;
	context->hrtc.Init.AsynchPrediv = 0x7F;  /* LSI */
	context->hrtc.Init.SynchPrediv  = 0x00FF; /* LSI */

	if (HAL_RTC_DeInit(&context->hrtc) == HAL_ERROR) {
		return -EIO;
	}

	if (HAL_RTC_Init(&context->hrtc) == HAL_ERROR) {
		return -EIO;
	}

	if (HAL_RTCEx_EnableBypassShadow(&context->hrtc) == HAL_ERROR) {
		return -EIO;
	}

	/* Unmask RTC interrupt */
	irq_enable(CONFIG_RTC_0_IRQ);

	rtc_stm32_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

static struct rtc_stm32_data rtc_data;

static const struct rtc_driver_api rtc_api = {
		.enable = rtc_stm32_enable,
		.disable = rtc_stm32_disable,
		.read = rtc_stm32_read,
		.set_config = rtc_stm32_set_config,
		.set_alarm = rtc_stm32_set_alarm,
		.get_pending_int = rtc_stm32_get_pending_int,
};

DEVICE_DEFINE(rtc_stm32, CONFIG_RTC_0_NAME,
	      &rtc_stm32_init, rtc_stm32_device_ctrl,
	      &rtc_data, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &rtc_api);

static void rtc_stm32_config_irq(void)
{
#ifdef CONFIG_SOC_SERIES_STM32F4X
	stm32_exti_set_callback(STM32F4_EXTI_RTC_ALARM,
				rtc_stm32_isr, DEVICE_GET(rtc_stm32));
#endif
#ifdef CONFIG_SOC_SERIES_STM32L4X
	IRQ_CONNECT(CONFIG_RTC_0_IRQ, CONFIG_RTC_0_IRQ_PRI,
		    rtc_stm32_isr, DEVICE_GET(rtc_stm32), 0);
#endif
}
