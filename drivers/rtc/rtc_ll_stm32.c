/*
 * Copyright (c) 2018 Workaround GmbH
 * Copyright (c) 2018 Allterco Robotics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Source file for the STM32 RTC driver
 *
 */

#include <time.h>

#include <clock_control/stm32_clock_control.h>
#include <clock_control.h>
#include <misc/util.h>
#include <kernel.h>
#include <board.h>
#include <rtc.h>

#if defined(CONFIG_SOC_SERIES_STM32L4X)
#define EXTI_LINE	LL_EXTI_LINE_18
#elif defined(CONFIG_SOC_SERIES_STM32F4X) || defined(CONFIG_SOC_SERIES_STM32F3X)
#define EXTI_LINE	LL_EXTI_LINE_17
#endif

#define EPOCH_OFFSET 946684800

struct rtc_stm32_config {
	struct stm32_pclken pclken;
	LL_RTC_InitTypeDef ll_rtc_config;
};

struct rtc_stm32_data {
	void (*cb_fn)(struct device *dev);
	struct k_sem sem;
};


#define DEV_DATA(dev) ((struct rtc_stm32_data *const)(dev)->driver_data)
#define DEV_SEM(dev) (&DEV_DATA(dev)->sem)
#define DEV_CFG(dev)	\
((const struct rtc_stm32_config * const)(dev)->config->config_info)

static int rtc_stm32_set_alarm(struct device *dev, const u32_t alarm_val);
static void rtc_stm32_irq_config(struct device *dev);

static void rtc_stm32_enable(struct device *dev)
{
	LL_RCC_EnableRTC();
}

static void rtc_stm32_disable(struct device *dev)
{
	LL_RCC_DisableRTC();
}

static u32_t rtc_stm32_read(struct device *dev)
{
	struct tm now = { 0 };
	time_t ts;
	u32_t rtc_date, rtc_time;

	/* Read time and date registers */
	rtc_time = LL_RTC_TIME_Get(RTC);
	rtc_date = LL_RTC_DATE_Get(RTC);

	/* Convert calendar datetime to UNIX timestamp */
	now.tm_year = 100 + __LL_RTC_CONVERT_BCD2BIN(
					__LL_RTC_GET_YEAR(rtc_date));
	now.tm_mon = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MONTH(rtc_date));
	now.tm_mday = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_DAY(rtc_date));

	now.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(rtc_time));
	now.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(rtc_time));
	now.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(rtc_time));

	ts = mktime(&now);

	/* Return number of seconds since 2000-01-01 00:00:00 */
	ts -= EPOCH_OFFSET;

	return (u32_t)ts;
}

static int rtc_stm32_set_alarm(struct device *dev, const u32_t alarm_val)
{
	struct tm alarm_tm;
	time_t alarm_ts;
	LL_RTC_AlarmTypeDef rtc_alarm;

	u32_t now = rtc_stm32_read(dev);

	/* The longest period we can match for universally is the
	   duration of the shortest month */
	if ((alarm_val - now) > (RTC_ALARM_DAY * 28)) {
		return -ENOTSUP;
	}

	/* Convert seconds since 2000-01-01 00:00:00 to calendar datetime */
	alarm_ts = alarm_val;
	alarm_ts += EPOCH_OFFSET;
	gmtime_r(&alarm_ts, &alarm_tm);

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

static int rtc_stm32_set_config(struct device *dev, struct rtc_config *cfg)
{
	int result = 0;
	time_t init_ts = 0;
	struct tm init_tm = { 0 };

	LL_RTC_DateTypeDef rtc_date = { 0 };
	LL_RTC_TimeTypeDef rtc_time = { 0 };

	/* Convert seconds since 2000-01-01 00:00:00 to calendar datetime */
	init_ts = cfg->init_val;
	init_ts += EPOCH_OFFSET;

	gmtime_r(&init_ts, &init_tm);

	rtc_date.Year = init_tm.tm_year % 100;
	rtc_date.Month = init_tm.tm_mon;
	rtc_date.Day = init_tm.tm_mday;
	rtc_date.WeekDay = init_tm.tm_wday + 1;

	rtc_time.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
	rtc_time.Hours = init_tm.tm_hour;
	rtc_time.Minutes = init_tm.tm_min;
	rtc_time.Seconds = init_tm.tm_sec;

	k_sem_take(DEV_SEM(dev), K_FOREVER);

	if (cfg->cb_fn != NULL) {
		DEV_DATA(dev)->cb_fn = cfg->cb_fn;
	}

	if (LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_date) != SUCCESS) {
		result = -EIO;
		goto fin;
	}

	if (LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &rtc_time) != SUCCESS) {
		result = -EIO;
		goto fin;
	}

	if (cfg->alarm_enable) {
		rtc_stm32_set_alarm(dev, cfg->alarm_val);
	}

fin:
	k_sem_give(DEV_SEM(dev));

	return result;
}

static u32_t rtc_stm32_get_pending_int(struct device *dev)
{
	return LL_RTC_IsActiveFlag_ALRA(RTC) != 0;
}

void rtc_stm32_isr(void *arg)
{
	struct device *const dev = (struct device *)arg;

	if (LL_RTC_IsActiveFlag_ALRA(RTC) != 0) {

		if (DEV_DATA(dev)->cb_fn != NULL) {
			DEV_DATA(dev)->cb_fn(dev);
		}

		LL_RTC_ClearFlag_ALRA(RTC);
		LL_RTC_DisableIT_ALRA(RTC);
	}

	LL_EXTI_ClearFlag_0_31(EXTI_LINE);
}

static int rtc_stm32_init(struct device *dev)
{
	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	const struct rtc_stm32_config *cfg = DEV_CFG(dev);

	__ASSERT_NO_MSG(clk);

	k_sem_init(DEV_SEM(dev), 1, UINT_MAX);
	DEV_DATA(dev)->cb_fn = NULL;

	clock_control_on(clk, (clock_control_subsys_t *) &cfg->pclken);

	LL_PWR_EnableBkUpAccess();
	LL_RCC_ForceBackupDomainReset();
	LL_RCC_ReleaseBackupDomainReset();

#if defined(CONFIG_RTC_STM32_CLOCK_LSI)

	LL_RCC_LSI_Enable();
	while (LL_RCC_LSI_IsReady() != 1)
		;
	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);

#else /* CONFIG_RTC_STM32_CLOCK_LSE */

	LL_RCC_LSE_SetDriveCapability(CONFIG_RTC_STM32_LSE_DRIVE_STRENGTH);
	LL_RCC_LSE_Enable();

	/* Wait untill LSE is ready */
	while (LL_RCC_LSE_IsReady() != 1)
		;

	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

#endif /* CONFIG_RTC_STM32_CLOCK_SRC */

	LL_RCC_EnableRTC();

	if (LL_RTC_DeInit(RTC) != SUCCESS) {
		return -EIO;
	}

	if (LL_RTC_Init(RTC, ((LL_RTC_InitTypeDef *)
			      &cfg->ll_rtc_config)) != SUCCESS) {
		return -EIO;
	}

	LL_RTC_EnableShadowRegBypass(RTC);

	LL_EXTI_EnableIT_0_31(EXTI_LINE);
	LL_EXTI_EnableRisingTrig_0_31(EXTI_LINE);

	rtc_stm32_irq_config(dev);

	return 0;
}

static struct rtc_stm32_data rtc_data;

static const struct rtc_stm32_config rtc_config = {
	.pclken = {
		.enr = LL_APB1_GRP1_PERIPH_PWR,
		.bus = STM32_CLOCK_BUS_APB1,
	},
	.ll_rtc_config = {
		.HourFormat = LL_RTC_HOURFORMAT_24HOUR,

#if defined(CONFIG_RTC_STM32_CLOCK_LSI)

		/* prescaler values for LSI @ 32 KHz */
		.AsynchPrescaler = 0x7F,
		.SynchPrescaler = 0x00F9,

#else /* CONFIG_RTC_STM32_CLOCK_LSE */

		/* prescaler values for LSE @ 32768 Hz */
		.AsynchPrescaler = 0x7F,
		.SynchPrescaler = 0x00FF,
#endif

	},
};

static const struct rtc_driver_api rtc_api = {
		.enable = rtc_stm32_enable,
		.disable = rtc_stm32_disable,
		.read = rtc_stm32_read,
		.set_config = rtc_stm32_set_config,
		.set_alarm = rtc_stm32_set_alarm,
		.get_pending_int = rtc_stm32_get_pending_int,
};

DEVICE_AND_API_INIT(rtc_stm32, CONFIG_RTC_0_NAME, &rtc_stm32_init,
		    &rtc_data, &rtc_config, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &rtc_api);

static void rtc_stm32_irq_config(struct device *dev)
{
	IRQ_CONNECT(CONFIG_RTC_0_IRQ, CONFIG_RTC_0_IRQ_PRI,
		    rtc_stm32_isr, DEVICE_GET(rtc_stm32), 0);
	irq_enable(CONFIG_RTC_0_IRQ);
}
