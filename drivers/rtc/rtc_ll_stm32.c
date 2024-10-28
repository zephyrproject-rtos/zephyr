/*
 * Copyright (c) 2023 Prevas A/S
 * Copyright (c) 2023 Syslinbit
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT st_stm32_rtc

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>
#include <soc.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_rtc.h>
#include <stm32_hsem.h>
#ifdef CONFIG_RTC_ALARM
#include <stm32_ll_exti.h>
#endif /* CONFIG_RTC_ALARM */

#include <zephyr/logging/log.h>
#ifdef CONFIG_RTC_ALARM
#include <zephyr/irq.h>
#endif /* CONFIG_RTC_ALARM */

#include <stdbool.h>
#include "rtc_utils.h"

#include "rtc_ll_stm32.h"

LOG_MODULE_REGISTER(rtc_stm32, CONFIG_RTC_LOG_LEVEL);

#if (defined(CONFIG_SOC_SERIES_STM32L1X) && !defined(RTC_SUBSECOND_SUPPORT)) \
	|| defined(CONFIG_SOC_SERIES_STM32F2X)
/* subsecond counting is not supported by some STM32L1x MCUs (Cat.1) & by STM32F2x SoC series */
#define HW_SUBSECOND_SUPPORT (0)
#else
#define HW_SUBSECOND_SUPPORT (1)
#endif

/* RTC start time: 1st, Jan, 2000 */
#define RTC_YEAR_REF 2000
/* struct tm start time:   1st, Jan, 1900 */
#define TM_YEAR_REF 1900

/* Convert part per billion calibration value to a number of clock pulses added or removed each
 * 2^20 clock cycles so it is suitable for the CALR register fields
 *
 * nb_pulses = ppb * 2^20 / 10^9 = ppb * 2^11 / 5^9 = ppb * 2048 / 1953125
 */
#define PPB_TO_NB_PULSES(ppb) DIV_ROUND_CLOSEST((ppb) * 2048, 1953125)

/* Convert CALR register value (number of clock pulses added or removed each 2^20 clock cycles)
 * to part ber billion calibration value
 *
 * ppb = nb_pulses * 10^9 / 2^20 = nb_pulses * 5^9 / 2^11 = nb_pulses * 1953125 / 2048
 */
#define NB_PULSES_TO_PPB(pulses) DIV_ROUND_CLOSEST((pulses) * 1953125, 2048)

/* CALP field can only be 512 or 0 as in reality CALP is a single bit field representing 512 pulses
 * added every 2^20 clock cycles
 */
#define MAX_CALP (512)
#define MAX_CALM (511)

#define MAX_PPB NB_PULSES_TO_PPB(MAX_CALP)
#define MIN_PPB -NB_PULSES_TO_PPB(MAX_CALM)

/* Timeout in microseconds used to wait for flags */
#define RTC_TIMEOUT 1000000

#ifdef CONFIG_RTC_ALARM
#define RTC_STM32_ALARMS_COUNT	DT_INST_PROP(0, alarms_count)

#define RTC_STM32_ALRM_A	0U
#define RTC_STM32_ALRM_B	1U

/* Zephyr mask supported by RTC device, values from RTC_ALARM_TIME_MASK */
#define RTC_STM32_SUPPORTED_ALARM_FIELDS				\
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE	\
	| RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_WEEKDAY	\
	| RTC_ALARM_TIME_MASK_MONTHDAY)

#if DT_INST_NODE_HAS_PROP(0, alrm_exti_line)
#define RTC_STM32_EXTI_LINE	CONCAT(LL_EXTI_LINE_, DT_INST_PROP(0, alrm_exti_line))
#else
#define RTC_STM32_EXTI_LINE	0
#endif /* DT_INST_NODE_HAS_PROP(0, alrm_exti_line) */
#endif /* CONFIG_RTC_ALARM */

#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
/*
 * After system reset, the RTC registers are protected against parasitic write access by the
 * DBP bit in the power control peripheral (PWR).
 * Hence, DBP bit must be set in order to enable RTC registers write access.
 */
#define RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION	(1)
#else
#define RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION	(0)
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPCR_DBP || PWR_DBPR_DBP */

struct rtc_stm32_config {
	uint32_t async_prescaler;
	uint32_t sync_prescaler;
	const struct stm32_pclken *pclken;
#if DT_INST_NODE_HAS_PROP(0, calib_out_freq)
	uint32_t cal_out_freq;
#endif
#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	uint32_t hse_prescaler;
#endif
};

#ifdef CONFIG_RTC_ALARM
struct rtc_stm32_alrm {
	LL_RTC_AlarmTypeDef ll_rtc_alrm;
	/* user-defined alarm mask, values from RTC_ALARM_TIME_MASK */
	uint16_t user_mask;
	rtc_alarm_callback user_callback;
	void *user_data;
	bool is_pending;
};
#endif /* CONFIG_RTC_ALARM */

struct rtc_stm32_data {
	struct k_mutex lock;
#ifdef CONFIG_RTC_ALARM
	struct rtc_stm32_alrm rtc_alrm_a;
	struct rtc_stm32_alrm rtc_alrm_b;
#endif /* CONFIG_RTC_ALARM */
};

static int rtc_stm32_configure(const struct device *dev)
{
	const struct rtc_stm32_config *cfg = dev->config;

	int err = 0;

	uint32_t hour_format     = LL_RTC_GetHourFormat(RTC);
	uint32_t sync_prescaler  = LL_RTC_GetSynchPrescaler(RTC);
	uint32_t async_prescaler = LL_RTC_GetAsynchPrescaler(RTC);

	LL_RTC_DisableWriteProtection(RTC);

	/* configuration process requires to stop the RTC counter so do it
	 * only if needed to avoid inducing time drift at each reset
	 */
	if ((hour_format != LL_RTC_HOURFORMAT_24HOUR) ||
	    (sync_prescaler != cfg->sync_prescaler) ||
	    (async_prescaler != cfg->async_prescaler)) {
		ErrorStatus status = LL_RTC_EnterInitMode(RTC);

		if (status == SUCCESS) {
			LL_RTC_SetHourFormat(RTC, LL_RTC_HOURFORMAT_24HOUR);
			LL_RTC_SetSynchPrescaler(RTC, cfg->sync_prescaler);
			LL_RTC_SetAsynchPrescaler(RTC, cfg->async_prescaler);
		} else {
			err = -EIO;
		}

		LL_RTC_DisableInitMode(RTC);
	}

#if DT_INST_NODE_HAS_PROP(0, calib_out_freq)
	LL_RTC_CAL_SetOutputFreq(RTC, cfg->cal_out_freq);
#else
	LL_RTC_CAL_SetOutputFreq(RTC, LL_RTC_CALIB_OUTPUT_NONE);
#endif

#ifdef RTC_CR_BYPSHAD
	LL_RTC_EnableShadowRegBypass(RTC);
#endif /* RTC_CR_BYPSHAD */

	LL_RTC_EnableWriteProtection(RTC);

	return err;
}

#ifdef CONFIG_RTC_ALARM
static inline ErrorStatus rtc_stm32_init_alarm(RTC_TypeDef *rtc, uint32_t format,
					LL_RTC_AlarmTypeDef *ll_alarm_struct, uint16_t id)
{
	ll_alarm_struct->AlarmDateWeekDaySel = RTC_STM32_ALRM_DATEWEEKDAYSEL_DATE;
	/*
	 * RTC write protection is disabled & enabled again inside LL_RTC_ALMx_Init functions
	 * The LL_RTC_ALMx_Init does convert bin2bcd by itself
	 */
	if (id == RTC_STM32_ALRM_A) {
		return LL_RTC_ALMA_Init(rtc, format, ll_alarm_struct);
	}
#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		return LL_RTC_ALMB_Init(rtc, format, ll_alarm_struct);
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */

	return 0;
}

static inline void rtc_stm32_clear_alarm_flag(RTC_TypeDef *rtc, uint16_t id)
{
	if (id == RTC_STM32_ALRM_A) {
		LL_RTC_ClearFlag_ALRA(rtc);
		return;
	}
#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		LL_RTC_ClearFlag_ALRB(rtc);
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */
}

static inline uint32_t rtc_stm32_is_active_alarm(RTC_TypeDef *rtc, uint16_t id)
{
	if (id == RTC_STM32_ALRM_A) {
		return LL_RTC_IsActiveFlag_ALRA(rtc);
	}
#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		return LL_RTC_IsActiveFlag_ALRB(rtc);
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */

	return 0;
}

static inline void rtc_stm32_enable_interrupt_alarm(RTC_TypeDef *rtc, uint16_t id)
{
	if (id == RTC_STM32_ALRM_A) {
		LL_RTC_EnableIT_ALRA(rtc);
		return;
	}
#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		LL_RTC_EnableIT_ALRB(rtc);
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */
}

static inline void rtc_stm32_disable_interrupt_alarm(RTC_TypeDef *rtc, uint16_t id)
{
	if (id == RTC_STM32_ALRM_A) {
		LL_RTC_DisableIT_ALRA(rtc);
		return;
	}
#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		LL_RTC_DisableIT_ALRB(rtc);
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */
}

static inline void rtc_stm32_enable_alarm(RTC_TypeDef *rtc, uint16_t id)
{
	if (id == RTC_STM32_ALRM_A) {
		LL_RTC_ALMA_Enable(rtc);
		return;
	}
#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		LL_RTC_ALMB_Enable(rtc);
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */
}

static inline void rtc_stm32_disable_alarm(RTC_TypeDef *rtc, uint16_t id)
{
	if (id == RTC_STM32_ALRM_A) {
		LL_RTC_ALMA_Disable(rtc);
		return;
	}
#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		LL_RTC_ALMB_Disable(rtc);
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */
}

void rtc_stm32_isr(const struct device *dev)
{
	struct rtc_stm32_data *data = dev->data;
	struct rtc_stm32_alrm *p_rtc_alrm;
	int id = 0;

#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_EnableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

	for (id = 0; id < RTC_STM32_ALARMS_COUNT; id++) {
		if (rtc_stm32_is_active_alarm(RTC, (uint16_t)id) != 0) {
			LL_RTC_DisableWriteProtection(RTC);
			rtc_stm32_clear_alarm_flag(RTC, (uint16_t)id);
			LL_RTC_EnableWriteProtection(RTC);

			if (id == RTC_STM32_ALRM_A) {
				p_rtc_alrm = &(data->rtc_alrm_a);
			} else {
				p_rtc_alrm = &(data->rtc_alrm_b);
			}

			p_rtc_alrm->is_pending = true;

			if (p_rtc_alrm->user_callback != NULL) {
				p_rtc_alrm->user_callback(dev, (uint16_t)id, p_rtc_alrm->user_data);
			}
		}
	}

#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_DisableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

	ll_func_exti_clear_rtc_alarm_flag(RTC_STM32_EXTI_LINE);
}

static void rtc_stm32_irq_config(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    rtc_stm32_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}
#endif /* CONFIG_RTC_ALARM */

static int rtc_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct rtc_stm32_config *cfg = dev->config;
	struct rtc_stm32_data *data = dev->data;

	int err = 0;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable RTC bus clock */
	if (clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	k_mutex_init(&data->lock);

	/* Enable Backup access */
#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_EnableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	/* Must be configured before selecting the RTC clock source */
	LL_RCC_SetRTC_HSEPrescaler(cfg->hse_prescaler);
#endif
	/* Enable RTC clock source */
	if (clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL) != 0) {
		LOG_ERR("clock configure failed\n");
		return -EIO;
	}

/*
 * On STM32WBAX series, there is no bit in BCDR register to enable RTC.
 * Enabling RTC is done directly via the RCC APB register bit.
 */
#ifndef CONFIG_SOC_SERIES_STM32WBAX
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	LL_RCC_EnableRTC();

	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
#endif /* CONFIG_SOC_SERIES_STM32WBAX */

	err = rtc_stm32_configure(dev);

#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_DisableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

#ifdef CONFIG_RTC_ALARM
	rtc_stm32_irq_config(dev);

	ll_func_exti_enable_rtc_alarm_it(RTC_STM32_EXTI_LINE);

	k_mutex_lock(&data->lock, K_FOREVER);
	memset(&(data->rtc_alrm_a), 0, sizeof(struct rtc_stm32_alrm));
	memset(&(data->rtc_alrm_b), 0, sizeof(struct rtc_stm32_alrm));
	k_mutex_unlock(&data->lock);
#endif /* CONFIG_RTC_ALARM */

	return err;
}

static int rtc_stm32_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_stm32_data *data = dev->data;
	LL_RTC_TimeTypeDef rtc_time;
	LL_RTC_DateTypeDef rtc_date;
	uint32_t real_year = timeptr->tm_year + TM_YEAR_REF;
	int err = 0;

	if (real_year < RTC_YEAR_REF) {
		/* RTC does not support years before 2000 */
		return -EINVAL;
	}

	if (timeptr->tm_wday == -1) {
		/* day of the week is expected */
		return -EINVAL;
	}

	err = k_mutex_lock(&data->lock, K_NO_WAIT);
	if (err) {
		return err;
	}

	LOG_DBG("Setting clock");

#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_EnableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

	/* Enter Init mode inside the LL_RTC_Time and Date Init functions */
	rtc_time.Hours = bin2bcd(timeptr->tm_hour);
	rtc_time.Minutes = bin2bcd(timeptr->tm_min);
	rtc_time.Seconds = bin2bcd(timeptr->tm_sec);
	LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BCD, &rtc_time);

	/* Set Date after Time to be sure the DR is correctly updated on stm32F2 serie. */
	rtc_date.Year = bin2bcd((real_year - RTC_YEAR_REF));
	rtc_date.Month = bin2bcd((timeptr->tm_mon + 1));
	rtc_date.Day = bin2bcd(timeptr->tm_mday);
	rtc_date.WeekDay = ((timeptr->tm_wday == 0) ? (LL_RTC_WEEKDAY_SUNDAY) : (timeptr->tm_wday));
	/* WeekDay sunday (tm_wday = 0) is not represented by the same value in hardware,
	 * all the other values are consistent with what is expected by hardware.
	 */
	LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BCD, &rtc_date);

#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_DisableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

#ifdef CONFIG_SOC_SERIES_STM32F2X
	/*
	 * Because stm32F2 serie has no shadow registers,
	 * wait until TR and DR registers are synchronised : flag RS
	 */
	while (LL_RTC_IsActiveFlag_RS(RTC) != 1) {
		;
	}
#endif /* CONFIG_SOC_SERIES_STM32F2X */

	k_mutex_unlock(&data->lock);

	LOG_DBG("Calendar set : %d/%d/%d - %dh%dm%ds",
			LL_RTC_DATE_GetDay(RTC),
			LL_RTC_DATE_GetMonth(RTC),
			LL_RTC_DATE_GetYear(RTC),
			LL_RTC_TIME_GetHour(RTC),
			LL_RTC_TIME_GetMinute(RTC),
			LL_RTC_TIME_GetSecond(RTC)
	);

	return err;
}

static int rtc_stm32_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_stm32_data *data = dev->data;

	uint32_t rtc_date, rtc_time;

#if HW_SUBSECOND_SUPPORT
	const struct rtc_stm32_config *cfg = dev->config;
	uint32_t rtc_subsecond;
#endif /* HW_SUBSECOND_SUPPORT */

	if (timeptr == NULL) {
		LOG_ERR("NULL rtc_time pointer");
		return -EINVAL;
	}

	int err = k_mutex_lock(&data->lock, K_NO_WAIT);

	if (err) {
		return err;
	}

	if (!LL_RTC_IsActiveFlag_INITS(RTC)) {
		/* INITS flag is set when the calendar has been initialiazed. This flag is
		 * reset only on backup domain reset, so it can be read after a system
		 * reset to check if the calendar has been initialized.
		 */
		k_mutex_unlock(&data->lock);
		return -ENODATA;
	}

	do {
		/* read date, time and subseconds and relaunch if a day increment occurred
		 * while doing so as it will result in an erroneous result otherwise
		 */
		rtc_date = LL_RTC_DATE_Get(RTC);
		do {
			/* read time and subseconds and relaunch if a second increment occurred
			 * while doing so as it will result in an erroneous result otherwise
			 */
			rtc_time      = LL_RTC_TIME_Get(RTC);
#if HW_SUBSECOND_SUPPORT
			rtc_subsecond = LL_RTC_TIME_GetSubSecond(RTC);
#endif /* HW_SUBSECOND_SUPPORT */
		} while (rtc_time != LL_RTC_TIME_Get(RTC));
	} while (rtc_date != LL_RTC_DATE_Get(RTC));

	k_mutex_unlock(&data->lock);

	/* tm_year is the value since 1900 and Rtc year is from 2000 */
	timeptr->tm_year = bcd2bin(__LL_RTC_GET_YEAR(rtc_date)) + (RTC_YEAR_REF - TM_YEAR_REF);
	/* tm_mon allowed values are 0-11 */
	timeptr->tm_mon = bcd2bin(__LL_RTC_GET_MONTH(rtc_date)) - 1;
	timeptr->tm_mday = bcd2bin(__LL_RTC_GET_DAY(rtc_date));

	int hw_wday = __LL_RTC_GET_WEEKDAY(rtc_date);

	if (hw_wday == LL_RTC_WEEKDAY_SUNDAY) {
		/* LL_RTC_WEEKDAY_SUNDAY = 7 but a 0 is expected in tm_wday for sunday */
		timeptr->tm_wday = 0;
	} else {
		/* all other values are consistent between hardware and rtc_time structure */
		timeptr->tm_wday = hw_wday;
	}

	timeptr->tm_hour = bcd2bin(__LL_RTC_GET_HOUR(rtc_time));
	timeptr->tm_min = bcd2bin(__LL_RTC_GET_MINUTE(rtc_time));
	timeptr->tm_sec = bcd2bin(__LL_RTC_GET_SECOND(rtc_time));

#if HW_SUBSECOND_SUPPORT
	uint64_t temp = ((uint64_t)(cfg->sync_prescaler - rtc_subsecond)) * 1000000000L;

	timeptr->tm_nsec = temp / (cfg->sync_prescaler + 1);
#else
	timeptr->tm_nsec = 0;
#endif
	/* unknown values */
	timeptr->tm_yday  = -1;
	timeptr->tm_isdst = -1;

	/* __LL_RTC_GET_YEAR(rtc_date)is the real year (from 2000) */
	LOG_DBG("Calendar get : %d/%d/%d - %dh%dm%ds",
		timeptr->tm_mday,
		timeptr->tm_mon,
		__LL_RTC_GET_YEAR(rtc_date),
		timeptr->tm_hour,
		timeptr->tm_min,
		timeptr->tm_sec);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static void rtc_stm32_init_ll_alrm_struct(LL_RTC_AlarmTypeDef *p_rtc_alarm,
					const struct rtc_time *timeptr, uint16_t mask)
{
	LL_RTC_TimeTypeDef *p_rtc_alrm_time = &(p_rtc_alarm->AlarmTime);
	uint32_t ll_mask = 0;

	/*
	 * STM32 RTC Alarm LL mask should be set for all fields beyond the broadest one
	 * that's being matched with RTC calendar to trigger alarm periodically,
	 * the opposite of Zephyr RTC Alarm mask which is set for active fields.
	 */
	ll_mask = RTC_STM32_ALRM_MASK_ALL;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		ll_mask &= ~RTC_STM32_ALRM_MASK_SECONDS;
		p_rtc_alrm_time->Seconds = bin2bcd(timeptr->tm_sec);
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		ll_mask &= ~RTC_STM32_ALRM_MASK_MINUTES;
		p_rtc_alrm_time->Minutes = bin2bcd(timeptr->tm_min);
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		ll_mask &= ~RTC_STM32_ALRM_MASK_HOURS;
		p_rtc_alrm_time->Hours = bin2bcd(timeptr->tm_hour);
	}

	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		/* the Alarm Mask field compares with the day of the week */
		ll_mask &= ~RTC_STM32_ALRM_MASK_DATEWEEKDAY;
		p_rtc_alarm->AlarmDateWeekDaySel = RTC_STM32_ALRM_DATEWEEKDAYSEL_WEEKDAY;

		if (timeptr->tm_wday == 0) {
			/* sunday (tm_wday = 0) is not represented by the same value in hardware */
			p_rtc_alarm->AlarmDateWeekDay = LL_RTC_WEEKDAY_SUNDAY;
		} else {
			/* all the other values are consistent with what is expected by hardware */
			p_rtc_alarm->AlarmDateWeekDay = bin2bcd(timeptr->tm_wday);
		}

	} else if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		/* the Alarm compares with the day number & ignores the day of the week */
		ll_mask &= ~RTC_STM32_ALRM_MASK_DATEWEEKDAY;
		p_rtc_alarm->AlarmDateWeekDaySel = RTC_STM32_ALRM_DATEWEEKDAYSEL_DATE;
		p_rtc_alarm->AlarmDateWeekDay = bin2bcd(timeptr->tm_mday);
	}

	p_rtc_alrm_time->TimeFormat = LL_RTC_TIME_FORMAT_AM_OR_24;

	p_rtc_alarm->AlarmMask = ll_mask;
}

static inline void rtc_stm32_get_ll_alrm_time(uint16_t id, struct rtc_time *timeptr)
{
	if (id == RTC_STM32_ALRM_A) {
		timeptr->tm_sec = bcd2bin(LL_RTC_ALMA_GetSecond(RTC));
		timeptr->tm_min = bcd2bin(LL_RTC_ALMA_GetMinute(RTC));
		timeptr->tm_hour = bcd2bin(LL_RTC_ALMA_GetHour(RTC));
		timeptr->tm_wday = bcd2bin(LL_RTC_ALMA_GetWeekDay(RTC));
		timeptr->tm_mday = bcd2bin(LL_RTC_ALMA_GetDay(RTC));
		return;
	}
#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		timeptr->tm_sec = bcd2bin(LL_RTC_ALMB_GetSecond(RTC));
		timeptr->tm_min = bcd2bin(LL_RTC_ALMB_GetMinute(RTC));
		timeptr->tm_hour = bcd2bin(LL_RTC_ALMB_GetHour(RTC));
		timeptr->tm_wday = bcd2bin(LL_RTC_ALMB_GetWeekDay(RTC));
		timeptr->tm_mday = bcd2bin(LL_RTC_ALMB_GetDay(RTC));
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */
}

static inline uint16_t rtc_stm32_get_ll_alrm_mask(uint16_t id)
{
	uint32_t ll_alarm_mask = 0;
	uint16_t zephyr_alarm_mask = 0;
	uint32_t week_day = 0;

	/*
	 * STM32 RTC Alarm LL mask is set for all fields beyond the broadest one
	 * that's being matched with RTC calendar to trigger alarm periodically,
	 * the opposite of Zephyr RTC Alarm mask which is set for active fields.
	 */

	if (id == RTC_STM32_ALRM_A) {
		ll_alarm_mask = LL_RTC_ALMA_GetMask(RTC);
	}

#if RTC_STM32_ALARMS_COUNT > 1
	if (id == RTC_STM32_ALRM_B) {
		ll_alarm_mask = LL_RTC_ALMB_GetMask(RTC);
	}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */

	if ((ll_alarm_mask & RTC_STM32_ALRM_MASK_SECONDS) == 0x0) {
		zephyr_alarm_mask = RTC_ALARM_TIME_MASK_SECOND;
	}
	if ((ll_alarm_mask & RTC_STM32_ALRM_MASK_MINUTES) == 0x0) {
		zephyr_alarm_mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if ((ll_alarm_mask & RTC_STM32_ALRM_MASK_HOURS) == 0x0) {
		zephyr_alarm_mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if ((ll_alarm_mask & RTC_STM32_ALRM_MASK_DATEWEEKDAY) == 0x0) {
		if (id == RTC_STM32_ALRM_A) {
			week_day = LL_RTC_ALMA_GetWeekDay(RTC);
		}
#if RTC_STM32_ALARMS_COUNT > 1
		if (id == RTC_STM32_ALRM_B) {
			week_day = LL_RTC_ALMB_GetWeekDay(RTC);
		}
#endif /* RTC_STM32_ALARMS_COUNT > 1 */
		if (week_day) {
			zephyr_alarm_mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		} else {
			zephyr_alarm_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		}
	}

	return zephyr_alarm_mask;
}

static int rtc_stm32_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					uint16_t *mask)
{
	if (mask == NULL) {
		LOG_ERR("NULL mask pointer");
		return -EINVAL;
	}

	if ((id != RTC_STM32_ALRM_A) && (id != RTC_STM32_ALRM_B)) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	*mask = (uint16_t)RTC_STM32_SUPPORTED_ALARM_FIELDS;

	return 0;
}

static int rtc_stm32_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
			struct rtc_time *timeptr)
{
	struct rtc_stm32_data *data = dev->data;
	struct rtc_stm32_alrm *p_rtc_alrm;
	LL_RTC_AlarmTypeDef *p_ll_rtc_alarm;
	LL_RTC_TimeTypeDef *p_ll_rtc_alrm_time;
	int err = 0;

	if ((mask == NULL) || (timeptr == NULL)) {
		LOG_ERR("NULL pointer");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	if (id == RTC_STM32_ALRM_A) {
		p_rtc_alrm = &(data->rtc_alrm_a);
	} else if (id == RTC_STM32_ALRM_B) {
		p_rtc_alrm = &(data->rtc_alrm_b);
	} else {
		LOG_ERR("invalid alarm ID %d", id);
		err = -EINVAL;
		goto unlock;
	}

	p_ll_rtc_alarm = &(p_rtc_alrm->ll_rtc_alrm);
	p_ll_rtc_alrm_time = &(p_ll_rtc_alarm->AlarmTime);

	memset(timeptr, -1, sizeof(struct rtc_time));

	rtc_stm32_get_ll_alrm_time(id, timeptr);

	p_rtc_alrm->user_mask = rtc_stm32_get_ll_alrm_mask(id);

	*mask = p_rtc_alrm->user_mask;

	LOG_DBG("get alarm: mday = %d, wday = %d, hour = %d, min = %d, sec = %d, "
		"mask = 0x%04x", timeptr->tm_mday, timeptr->tm_wday, timeptr->tm_hour,
		timeptr->tm_min, timeptr->tm_sec, *mask);

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int rtc_stm32_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
			const struct rtc_time *timeptr)
{
	struct rtc_stm32_data *data = dev->data;
	struct rtc_stm32_alrm *p_rtc_alrm;
	LL_RTC_AlarmTypeDef *p_ll_rtc_alarm;
	LL_RTC_TimeTypeDef *p_ll_rtc_alrm_time;
	int err = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (id == RTC_STM32_ALRM_A) {
		p_rtc_alrm = &(data->rtc_alrm_a);
	} else if (id == RTC_STM32_ALRM_B) {
		p_rtc_alrm = &(data->rtc_alrm_b);
	} else {
		LOG_ERR("invalid alarm ID %d", id);
		err = -EINVAL;
		goto unlock;
	}

	if ((mask == 0) && (timeptr == NULL)) {
		memset(&(p_rtc_alrm->ll_rtc_alrm), 0, sizeof(LL_RTC_AlarmTypeDef));
		p_rtc_alrm->user_callback = NULL;
		p_rtc_alrm->user_data = NULL;
		p_rtc_alrm->is_pending = false;
#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
		LL_PWR_EnableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */
		if (rtc_stm32_is_active_alarm(RTC, id)) {
			LL_RTC_DisableWriteProtection(RTC);
			rtc_stm32_disable_alarm(RTC, id);
			rtc_stm32_disable_interrupt_alarm(RTC, id);
			LL_RTC_EnableWriteProtection(RTC);
		}
		LOG_DBG("Alarm %d has been disabled", id);
		goto disable_bkup_access;
	}

	if ((mask & ~RTC_STM32_SUPPORTED_ALARM_FIELDS) != 0) {
		LOG_ERR("unsupported alarm %d field mask 0x%04x", id, mask);
		err = -EINVAL;
		goto unlock;
	}

	if (timeptr == NULL) {
		LOG_ERR("timeptr is invalid");
		err = -EINVAL;
		goto unlock;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_DBG("One or multiple time values are invalid");
		err = -EINVAL;
		goto unlock;
	}

	p_ll_rtc_alarm = &(p_rtc_alrm->ll_rtc_alrm);
	p_ll_rtc_alrm_time = &(p_ll_rtc_alarm->AlarmTime);

	memset(p_ll_rtc_alrm_time, 0, sizeof(LL_RTC_TimeTypeDef));
	rtc_stm32_init_ll_alrm_struct(p_ll_rtc_alarm, timeptr, mask);

	p_rtc_alrm->user_mask = mask;

	LOG_DBG("set alarm %d : second = %d, min = %d, hour = %d,"
			" wday = %d, mday = %d, mask = 0x%04x",
			id, timeptr->tm_sec, timeptr->tm_min, timeptr->tm_hour,
			timeptr->tm_wday, timeptr->tm_mday, mask);

#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_EnableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

	/* Disable the write protection for RTC registers */
	LL_RTC_DisableWriteProtection(RTC);

	/* Disable ALARM so that the RTC_ISR_ALRAWF/RTC_ISR_ALRBWF is 0 */
	rtc_stm32_disable_alarm(RTC, id);
	rtc_stm32_disable_interrupt_alarm(RTC, id);

#ifdef RTC_ISR_ALRAWF
	if (id == RTC_STM32_ALRM_A) {
		/* Wait till RTC ALRAWF flag is set before writing to RTC registers */
		while (!LL_RTC_IsActiveFlag_ALRAW(RTC)) {
			;
		}
	}
#endif /* RTC_ISR_ALRAWF */

#ifdef RTC_ISR_ALRBWF
	if (id == RTC_STM32_ALRM_B) {
		/* Wait till RTC ALRBWF flag is set before writing to RTC registers */
		while (!LL_RTC_IsActiveFlag_ALRBW(RTC)) {
			;
		}
	}
#endif /* RTC_ISR_ALRBWF */

	/* init Alarm */
	/* write protection is disabled & enabled again inside the LL_RTC_ALMx_Init function */
	if (rtc_stm32_init_alarm(RTC, LL_RTC_FORMAT_BCD, p_ll_rtc_alarm, id) != SUCCESS) {
		LOG_ERR("Could not initialize Alarm %d", id);
		err = -ECANCELED;
		goto disable_bkup_access;
	}

	/* Disable the write protection for RTC registers */
	LL_RTC_DisableWriteProtection(RTC);

	/* Enable Alarm */
	rtc_stm32_enable_alarm(RTC, id);
	/* Clear Alarm flag */
	rtc_stm32_clear_alarm_flag(RTC, id);
	/* Enable Alarm IT */
	rtc_stm32_enable_interrupt_alarm(RTC, id);

	ll_func_exti_enable_rtc_alarm_it(RTC_STM32_EXTI_LINE);

	/* Enable the write protection for RTC registers */
	LL_RTC_EnableWriteProtection(RTC);

disable_bkup_access:
#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_DisableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

unlock:
	k_mutex_unlock(&data->lock);

	if (id == RTC_STM32_ALRM_A) {
		LOG_DBG("Alarm A : %dh%dm%ds   mask = 0x%x",
			LL_RTC_ALMA_GetHour(RTC),
			LL_RTC_ALMA_GetMinute(RTC),
			LL_RTC_ALMA_GetSecond(RTC),
			LL_RTC_ALMA_GetMask(RTC));
	}
#ifdef RTC_ALARM_B
	if (id == RTC_STM32_ALRM_B) {
		LOG_DBG("Alarm B : %dh%dm%ds   mask = 0x%x",
			LL_RTC_ALMB_GetHour(RTC),
			LL_RTC_ALMB_GetMinute(RTC),
			LL_RTC_ALMB_GetSecond(RTC),
			LL_RTC_ALMB_GetMask(RTC));
	}
#endif /* #ifdef RTC_ALARM_B */
	return err;
}

static int rtc_stm32_alarm_set_callback(const struct device *dev, uint16_t id,
			rtc_alarm_callback callback, void *user_data)
{
	struct rtc_stm32_data *data = dev->data;
	struct rtc_stm32_alrm *p_rtc_alrm;
	int err = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (id == RTC_STM32_ALRM_A) {
		p_rtc_alrm = &(data->rtc_alrm_a);
	} else if (id == RTC_STM32_ALRM_B) {
		p_rtc_alrm = &(data->rtc_alrm_b);
	} else {
		LOG_ERR("invalid alarm ID %d", id);
		err = -EINVAL;
		goto unlock;
	}

	/* Passing the callback function and userdata filled by the user */
	p_rtc_alrm->user_callback = callback;
	p_rtc_alrm->user_data = user_data;

unlock:
	k_mutex_unlock(&data->lock);

	return err;
}

static int rtc_stm32_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_stm32_data *data = dev->data;
	struct rtc_stm32_alrm *p_rtc_alrm;
	int ret = 0;

	k_mutex_lock(&data->lock, K_FOREVER);

	if (id == RTC_STM32_ALRM_A) {
		p_rtc_alrm = &(data->rtc_alrm_a);
	} else if (id == RTC_STM32_ALRM_B) {
		p_rtc_alrm = &(data->rtc_alrm_b);
	} else {
		LOG_ERR("invalid alarm ID %d", id);
		ret = -EINVAL;
		goto unlock;
	}

	__disable_irq();
	ret = p_rtc_alrm->is_pending ? 1 : 0;
	p_rtc_alrm->is_pending = false;
	__enable_irq();

unlock:
	k_mutex_unlock(&data->lock);
	return ret;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_CALIBRATION
#if !defined(CONFIG_SOC_SERIES_STM32F2X) && \
	!(defined(CONFIG_SOC_SERIES_STM32L1X) && !defined(RTC_SMOOTHCALIB_SUPPORT))
static int rtc_stm32_set_calibration(const struct device *dev, int32_t calibration)
{
	ARG_UNUSED(dev);

	/* Note : calibration is considered here to be ppb value to apply
	 *        on clock period (not frequency) but with an opposite sign
	 */

	if ((calibration > MAX_PPB) || (calibration < MIN_PPB)) {
		/* out of supported range */
		return -EINVAL;
	}

	int32_t nb_pulses = PPB_TO_NB_PULSES(calibration);

	/* we tested calibration against supported range
	 * so theoretically nb_pulses is also within range
	 */
	__ASSERT_NO_MSG(nb_pulses <= MAX_CALP);
	__ASSERT_NO_MSG(nb_pulses >= -MAX_CALM);

	uint32_t calp, calm;

	if (nb_pulses > 0) {
		calp = LL_RTC_CALIB_INSERTPULSE_SET;
		calm = MAX_CALP - nb_pulses;
	} else {
		calp = LL_RTC_CALIB_INSERTPULSE_NONE;
		calm = -nb_pulses;
	}

	/* wait for recalibration to be ok if a previous recalibration occurred */
	if (!WAIT_FOR(LL_RTC_IsActiveFlag_RECALP(RTC) == 0, 100000, k_msleep(1))) {
		return -EIO;
	}

#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_EnableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

	LL_RTC_DisableWriteProtection(RTC);

	MODIFY_REG(RTC->CALR, RTC_CALR_CALP | RTC_CALR_CALM, calp | calm);

	LL_RTC_EnableWriteProtection(RTC);

#if RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION
	LL_PWR_DisableBkUpAccess();
#endif /* RTC_STM32_BACKUP_DOMAIN_WRITE_PROTECTION */

	return 0;
}

static int rtc_stm32_get_calibration(const struct device *dev, int32_t *calibration)
{
	ARG_UNUSED(dev);

	uint32_t calr = sys_read32((mem_addr_t) &RTC->CALR);

	bool calp_enabled = READ_BIT(calr, RTC_CALR_CALP);
	uint32_t calm = READ_BIT(calr, RTC_CALR_CALM);

	int32_t nb_pulses = -((int32_t) calm);

	if (calp_enabled) {
		nb_pulses += MAX_CALP;
	}

	*calibration = NB_PULSES_TO_PPB(nb_pulses);

	return 0;
}
#endif
#endif /* CONFIG_RTC_CALIBRATION */

static const struct rtc_driver_api rtc_stm32_driver_api = {
	.set_time = rtc_stm32_set_time,
	.get_time = rtc_stm32_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_stm32_alarm_get_supported_fields,
	.alarm_set_time = rtc_stm32_alarm_set_time,
	.alarm_get_time = rtc_stm32_alarm_get_time,
	.alarm_set_callback = rtc_stm32_alarm_set_callback,
	.alarm_is_pending = rtc_stm32_alarm_is_pending,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_CALIBRATION
#if !defined(CONFIG_SOC_SERIES_STM32F2X) && \
	!(defined(CONFIG_SOC_SERIES_STM32L1X) && !defined(RTC_SMOOTHCALIB_SUPPORT))
	.set_calibration = rtc_stm32_set_calibration,
	.get_calibration = rtc_stm32_get_calibration,
#else
#error RTC calibration for devices without smooth calibration feature is not supported yet
#endif
#endif /* CONFIG_RTC_CALIBRATION */
};

static const struct stm32_pclken rtc_clk[] = STM32_DT_INST_CLOCKS(0);

BUILD_ASSERT(DT_INST_CLOCKS_HAS_IDX(0, 1), "RTC source clock not defined in the device tree");

#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
#if STM32_HSE_FREQ % MHZ(1) != 0
#error RTC clock source HSE frequency should be whole MHz
#elif STM32_HSE_FREQ < MHZ(16) && defined(LL_RCC_RTC_HSE_DIV_16)
#define RTC_HSE_PRESCALER LL_RCC_RTC_HSE_DIV_16
#define RTC_HSE_FREQUENCY (STM32_HSE_FREQ / 16)
#elif STM32_HSE_FREQ < MHZ(32) && defined(LL_RCC_RTC_HSE_DIV_32)
#define RTC_HSE_PRESCALER LL_RCC_RTC_HSE_DIV_32
#define RTC_HSE_FREQUENCY (STM32_HSE_FREQ / 32)
#elif STM32_HSE_FREQ < MHZ(64) && defined(LL_RCC_RTC_HSE_DIV_64)
#define RTC_HSE_PRESCALER LL_RCC_RTC_HSE_DIV_64
#define RTC_HSE_FREQUENCY (STM32_HSE_FREQ / 64)
#else
#error RTC does not support HSE frequency
#endif
#define RTC_HSE_ASYNC_PRESCALER 125
#define RTC_HSE_SYNC_PRESCALER  (RTC_HSE_FREQUENCY / RTC_HSE_ASYNC_PRESCALER)
#endif /* DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE */

static const struct rtc_stm32_config rtc_config = {
#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_LSI
	/* prescaler values for LSI @ 32 KHz */
	.async_prescaler = 0x7F,
	.sync_prescaler = 0x00F9,
#elif DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_LSE
	/* prescaler values for LSE @ 32768 Hz */
	.async_prescaler = 0x7F,
	.sync_prescaler = 0x00FF,
#elif DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	/* prescaler values for HSE */
	.async_prescaler = RTC_HSE_ASYNC_PRESCALER - 1,
	.sync_prescaler = RTC_HSE_SYNC_PRESCALER - 1,
	.hse_prescaler = RTC_HSE_PRESCALER,
#else
#error Invalid RTC SRC
#endif
	.pclken = rtc_clk,
#if DT_INST_NODE_HAS_PROP(0, calib_out_freq)
	.cal_out_freq = _CONCAT(_CONCAT(LL_RTC_CALIB_OUTPUT_, DT_INST_PROP(0, calib_out_freq)), HZ),
#endif
};

static struct rtc_stm32_data rtc_data;

DEVICE_DT_INST_DEFINE(0, &rtc_stm32_init, NULL, &rtc_data, &rtc_config, PRE_KERNEL_1,
		      CONFIG_RTC_INIT_PRIORITY, &rtc_stm32_driver_api);
