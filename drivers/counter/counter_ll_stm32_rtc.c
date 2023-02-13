/*
 * Copyright (c) 2018 Workaround GmbH
 * Copyright (c) 2018 Allterco Robotics
 * Copyright (c) 2018 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Source file for the STM32 RTC driver
 *
 */

#define DT_DRV_COMPAT st_stm32_rtc

#include <time.h>

#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <soc.h>
#include <stm32_ll_exti.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_rtc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/timeutil.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include "stm32_hsem.h"

LOG_MODULE_REGISTER(counter_rtc_stm32, CONFIG_COUNTER_LOG_LEVEL);

/* Seconds from 1970-01-01T00:00:00 to 2000-01-01T00:00:00 */
#define T_TIME_OFFSET 946684800

#if defined(CONFIG_SOC_SERIES_STM32L4X)
#define RTC_EXTI_LINE	LL_EXTI_LINE_18
#elif defined(CONFIG_SOC_SERIES_STM32G0X)
#define RTC_EXTI_LINE	LL_EXTI_LINE_19
#elif defined(CONFIG_SOC_SERIES_STM32F4X) \
	|| defined(CONFIG_SOC_SERIES_STM32F0X) \
	|| defined(CONFIG_SOC_SERIES_STM32F1X) \
	|| defined(CONFIG_SOC_SERIES_STM32F2X) \
	|| defined(CONFIG_SOC_SERIES_STM32F3X) \
	|| defined(CONFIG_SOC_SERIES_STM32F7X) \
	|| defined(CONFIG_SOC_SERIES_STM32WBX) \
	|| defined(CONFIG_SOC_SERIES_STM32G4X) \
	|| defined(CONFIG_SOC_SERIES_STM32L0X) \
	|| defined(CONFIG_SOC_SERIES_STM32L1X) \
	|| defined(CONFIG_SOC_SERIES_STM32L5X) \
	|| defined(CONFIG_SOC_SERIES_STM32H7X) \
	|| defined(CONFIG_SOC_SERIES_STM32WLX)
#define RTC_EXTI_LINE	LL_EXTI_LINE_17
#endif

#if defined(CONFIG_SOC_SERIES_STM32F1X)
#define COUNTER_NO_DATE
#endif

struct rtc_stm32_config {
	struct counter_config_info counter_info;
	LL_RTC_InitTypeDef ll_rtc_config;
	const struct stm32_pclken *pclken;
};

struct rtc_stm32_data {
	counter_alarm_callback_t callback;
	uint32_t ticks;
	void *user_data;
};

static inline ErrorStatus ll_func_init_alarm(RTC_TypeDef *rtc, uint32_t format,
					     LL_RTC_AlarmTypeDef *alarmStruct)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	return LL_RTC_ALARM_Init(rtc, format, alarmStruct);
#else
	return LL_RTC_ALMA_Init(rtc, format, alarmStruct);
#endif
}

static inline void ll_func_clear_alarm_flag(RTC_TypeDef *rtc)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_ClearFlag_ALR(rtc);
#else
	LL_RTC_ClearFlag_ALRA(rtc);
#endif
}

static inline uint32_t ll_func_is_active_alarm(RTC_TypeDef *rtc)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	return LL_RTC_IsActiveFlag_ALR(rtc);
#else
	return LL_RTC_IsActiveFlag_ALRA(rtc);
#endif
}

static inline void ll_func_enable_interrupt_alarm(RTC_TypeDef *rtc)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_EnableIT_ALR(rtc);
#else
	LL_RTC_EnableIT_ALRA(rtc);
#endif
}

static inline void ll_func_disable_interrupt_alarm(RTC_TypeDef *rtc)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_DisableIT_ALR(rtc);
#else
	LL_RTC_DisableIT_ALRA(rtc);
#endif
}

static inline void ll_func_enable_alarm(RTC_TypeDef *rtc)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	ARG_UNUSED(rtc);
#else
	LL_RTC_ALMA_Enable(rtc);
#endif
}

static inline void ll_func_disable_alarm(RTC_TypeDef *rtc)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	ARG_UNUSED(rtc);
#else
	LL_RTC_ALMA_Disable(rtc);
#endif
}

static void rtc_stm32_irq_config(const struct device *dev);


static int rtc_stm32_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	LL_RCC_EnableRTC();
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

	return 0;
}


static int rtc_stm32_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	LL_RCC_DisableRTC();
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

	return 0;
}


static uint32_t rtc_stm32_read(const struct device *dev)
{
#if !defined(COUNTER_NO_DATE)
	struct tm now = { 0 };
	time_t ts;
	uint32_t rtc_date, rtc_time, ticks;
#else
	uint32_t rtc_time, ticks;
#endif
	ARG_UNUSED(dev);

	/* Read time and date registers */
	rtc_time = LL_RTC_TIME_Get(RTC);
#if !defined(COUNTER_NO_DATE)
	rtc_date = LL_RTC_DATE_Get(RTC);
#endif

#if !defined(COUNTER_NO_DATE)
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

	ts = timeutil_timegm(&now);

	/* Return number of seconds since RTC init */
	ts -= T_TIME_OFFSET;

	__ASSERT(sizeof(time_t) == 8, "unexpected time_t definition");
	ticks = counter_us_to_ticks(dev, ts * USEC_PER_SEC);
#else
	ticks = rtc_time;
#endif

	return ticks;
}

static int rtc_stm32_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = rtc_stm32_read(dev);
	return 0;
}

static int rtc_stm32_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
#if !defined(COUNTER_NO_DATE)
	struct tm alarm_tm;
	time_t alarm_val;
#else
	uint32_t remain;
#endif
	LL_RTC_AlarmTypeDef rtc_alarm;
	struct rtc_stm32_data *data = dev->data;

	uint32_t now = rtc_stm32_read(dev);
	uint32_t ticks = alarm_cfg->ticks;

	if (data->callback != NULL) {
		LOG_DBG("Alarm busy\n");
		return -EBUSY;
	}


	data->callback = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

#if !defined(COUNTER_NO_DATE)
	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		/* Add +1 in order to compensate the partially started tick.
		 * Alarm will expire between requested ticks and ticks+1.
		 * In case only 1 tick is requested, it will avoid
		 * that tick+1 event occurs before alarm setting is finished.
		 */
		ticks += now + 1;
		alarm_val = (time_t)(counter_ticks_to_us(dev, ticks) / USEC_PER_SEC)
			+ T_TIME_OFFSET;
	} else {
		alarm_val = (time_t)(counter_ticks_to_us(dev, ticks) / USEC_PER_SEC);
	}
#else
	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		remain = ticks + now + 1;
	} else {
		remain = ticks;
	}

	/* In F1X, an interrupt occurs when the counter expires,
	 * not when the counter matches, so set -1
	 */
	remain--;
#endif

#if !defined(COUNTER_NO_DATE)
	LOG_DBG("Set Alarm: %d\n", ticks);

	gmtime_r(&alarm_val, &alarm_tm);

	/* Apply ALARM_A */
	rtc_alarm.AlarmTime.TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;
	rtc_alarm.AlarmTime.Hours = alarm_tm.tm_hour;
	rtc_alarm.AlarmTime.Minutes = alarm_tm.tm_min;
	rtc_alarm.AlarmTime.Seconds = alarm_tm.tm_sec;

	rtc_alarm.AlarmMask = LL_RTC_ALMA_MASK_NONE;
	rtc_alarm.AlarmDateWeekDaySel = LL_RTC_ALMA_DATEWEEKDAYSEL_DATE;
	rtc_alarm.AlarmDateWeekDay = alarm_tm.tm_mday;
#else
	rtc_alarm.AlarmTime.Hours = remain / 3600;
	remain -= rtc_alarm.AlarmTime.Hours * 3600;
	rtc_alarm.AlarmTime.Minutes = remain / 60;
	remain -= rtc_alarm.AlarmTime.Minutes * 60;
	rtc_alarm.AlarmTime.Seconds = remain;
#endif

	LL_RTC_DisableWriteProtection(RTC);
	ll_func_disable_alarm(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	if (ll_func_init_alarm(RTC, LL_RTC_FORMAT_BIN, &rtc_alarm) != SUCCESS) {
		return -EIO;
	}

	LL_RTC_DisableWriteProtection(RTC);
	ll_func_enable_alarm(RTC);
	ll_func_clear_alarm_flag(RTC);
	ll_func_enable_interrupt_alarm(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	return 0;
}


static int rtc_stm32_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct rtc_stm32_data *data = dev->data;

	LL_RTC_DisableWriteProtection(RTC);
	ll_func_clear_alarm_flag(RTC);
	ll_func_disable_interrupt_alarm(RTC);
	ll_func_disable_alarm(RTC);
	LL_RTC_EnableWriteProtection(RTC);

	data->callback = NULL;

	return 0;
}


static uint32_t rtc_stm32_get_pending_int(const struct device *dev)
{
	return ll_func_is_active_alarm(RTC) != 0;
}


static uint32_t rtc_stm32_get_top_value(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value;
}


static int rtc_stm32_set_top_value(const struct device *dev,
				   const struct counter_top_cfg *cfg)
{
	const struct counter_config_info *info = dev->config;

	if ((cfg->ticks != info->max_top_value) ||
		!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		return -ENOTSUP;
	} else {
		return 0;
	}


}

void rtc_stm32_isr(const struct device *dev)
{
	struct rtc_stm32_data *data = dev->data;
	counter_alarm_callback_t alarm_callback = data->callback;

	uint32_t now = rtc_stm32_read(dev);

	if (ll_func_is_active_alarm(RTC) != 0) {

		LL_RTC_DisableWriteProtection(RTC);
		ll_func_clear_alarm_flag(RTC);
		ll_func_disable_interrupt_alarm(RTC);
		ll_func_disable_alarm(RTC);
		LL_RTC_EnableWriteProtection(RTC);

		if (alarm_callback != NULL) {
			data->callback = NULL;
			alarm_callback(dev, 0, now, data->user_data);
		}
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_ClearFlag_0_31(RTC_EXTI_LINE);
#elif defined(CONFIG_SOC_SERIES_STM32G0X) || defined(CONFIG_SOC_SERIES_STM32L5X)
	LL_EXTI_ClearRisingFlag_0_31(RTC_EXTI_LINE);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	/* in STM32U5 family RTC is not connected to EXTI */
#else
	LL_EXTI_ClearFlag_0_31(RTC_EXTI_LINE);
#endif
}


static int rtc_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct rtc_stm32_config *cfg = dev->config;
	struct rtc_stm32_data *data = dev->data;

	data->callback = NULL;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable RTC bus clock */
	if (clock_control_on(clk, (clock_control_subsys_t *) &cfg->pclken[0]) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	/* Enable Backup access */
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	LL_PWR_EnableBkUpAccess();

	/* Enable RTC clock source */
	if (clock_control_configure(clk,
				    (clock_control_subsys_t *) &cfg->pclken[1],
				    NULL) != 0) {
		LOG_ERR("clock configure failed\n");
		return -EIO;
	}

	LL_RCC_EnableRTC();

	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

#if !defined(CONFIG_COUNTER_RTC_STM32_SAVE_VALUE_BETWEEN_RESETS)
	if (LL_RTC_DeInit(RTC) != SUCCESS) {
		return -EIO;
	}
#endif

	if (LL_RTC_Init(RTC, ((LL_RTC_InitTypeDef *)
			      &cfg->ll_rtc_config)) != SUCCESS) {
		return -EIO;
	}

#ifdef RTC_CR_BYPSHAD
	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_EnableShadowRegBypass(RTC);
	LL_RTC_EnableWriteProtection(RTC);
#endif /* RTC_CR_BYPSHAD */

#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_EnableIT_0_31(RTC_EXTI_LINE);
	LL_EXTI_EnableRisingTrig_0_31(RTC_EXTI_LINE);
#elif defined(CONFIG_SOC_SERIES_STM32U5X)
	/* in STM32U5 family RTC is not connected to EXTI */
#else
	LL_EXTI_EnableIT_0_31(RTC_EXTI_LINE);
	LL_EXTI_EnableRisingTrig_0_31(RTC_EXTI_LINE);
#endif

	rtc_stm32_irq_config(dev);

	return 0;
}

static struct rtc_stm32_data rtc_data;

#if DT_INST_NUM_CLOCKS(0) == 1
#warning STM32 RTC needs a kernel source clock. Please define it in dts file
static const struct stm32_pclken rtc_clk[] = {
	STM32_CLOCK_INFO(0, DT_DRV_INST(0)),
	/* Use Kconfig to configure source clocks fields (Deprecated) */
	/* Fortunately, values are consistent across enabled series */
#ifdef CONFIG_COUNTER_RTC_STM32_CLOCK_LSE
	{.bus = STM32_SRC_LSE, .enr = RTC_SEL(1)}
#else
	{.bus = STM32_SRC_LSI, .enr = RTC_SEL(2)}
#endif
};
#else
static const struct stm32_pclken rtc_clk[] = STM32_DT_INST_CLOCKS(0);
#endif

static const struct rtc_stm32_config rtc_config = {
	.counter_info = {
		.max_top_value = UINT32_MAX,
		.freq = 1,
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 1,
	},
	.ll_rtc_config = {
#if !defined(CONFIG_SOC_SERIES_STM32F1X)
		.HourFormat = LL_RTC_HOURFORMAT_24HOUR,
#if DT_INST_CLOCKS_CELL(1, bus) == STM32_SRC_LSI
		/* prescaler values for LSI @ 32 KHz */
		.AsynchPrescaler = 0x7F,
		.SynchPrescaler = 0x00F9,
#else /* DT_INST_CLOCKS_CELL(1, bus) == STM32_SRC_LSE */
		/* prescaler values for LSE @ 32768 Hz */
		.AsynchPrescaler = 0x7F,
		.SynchPrescaler = 0x00FF,
#endif
#else /* CONFIG_SOC_SERIES_STM32F1X */
#if DT_INST_CLOCKS_CELL(1, bus) == STM32_SRC_LSI
		/* prescaler values for LSI @ 40 KHz */
		.AsynchPrescaler = 0x9C3F,
#else /* DT_INST_CLOCKS_CELL(1, bus) == STM32_SRC_LSE */
		/* prescaler values for LSE @ 32768 Hz */
		.AsynchPrescaler = 0x7FFF,
#endif /* DT_INST_CLOCKS_CELL(1, bus) == STM32_SRC_LSE */
		.OutPutSource = LL_RTC_CALIB_OUTPUT_NONE,
#endif /* CONFIG_SOC_SERIES_STM32F1X */
	},
	.pclken = rtc_clk,
};


static const struct counter_driver_api rtc_stm32_driver_api = {
		.start = rtc_stm32_start,
		.stop = rtc_stm32_stop,
		.get_value = rtc_stm32_get_value,
		.set_alarm = rtc_stm32_set_alarm,
		.cancel_alarm = rtc_stm32_cancel_alarm,
		.set_top_value = rtc_stm32_set_top_value,
		.get_pending_int = rtc_stm32_get_pending_int,
		.get_top_value = rtc_stm32_get_top_value,
};

DEVICE_DT_INST_DEFINE(0, &rtc_stm32_init, NULL,
		    &rtc_data, &rtc_config, PRE_KERNEL_1,
		    CONFIG_COUNTER_INIT_PRIORITY, &rtc_stm32_driver_api);

static void rtc_stm32_irq_config(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    rtc_stm32_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}
