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
#include <stm32_bitops.h>
#include <stm32_ll_cortex.h>
#include <stm32_ll_exti.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_rtc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#include <stm32_backup_domain.h>
#include <stm32_hsem.h>

LOG_MODULE_REGISTER(counter_rtc_stm32, CONFIG_COUNTER_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32F2X) || \
	(defined(CONFIG_SOC_SERIES_STM32L1X) && !defined(RTC_SUBSECOND_SUPPORT))
/* subsecond counting is not supported by some STM32L1x MCUs (Cat.1) & by STM32F1x/2x SoC series */
#define HW_SUBSECOND_SUPPORT 0
#else
#define HW_SUBSECOND_SUPPORT 1
#endif

/* Seconds from 1970-01-01T00:00:00 to 2000-01-01T00:00:00 */
#define T_TIME_OFFSET 946684800

#if defined(CONFIG_SOC_SERIES_STM32L4X)
#define RTC_EXTI_LINE	LL_EXTI_LINE_18
#elif defined(CONFIG_SOC_SERIES_STM32C0X) \
	|| defined(CONFIG_SOC_SERIES_STM32G0X)
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
	|| defined(CONFIG_SOC_SERIES_STM32H5X) \
	|| defined(CONFIG_SOC_SERIES_STM32WLX)
#define RTC_EXTI_LINE	LL_EXTI_LINE_17
#endif

#if defined(CONFIG_SOC_SERIES_STM32F1X)
#define COUNTER_NO_DATE
#endif

#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_LSI
/* LSI */
#define RTCCLK_FREQ STM32_LSI_FREQ
#else
/* LSE */
#define RTCCLK_FREQ STM32_LSE_FREQ
#endif /* DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_LSI */

#if !defined(CONFIG_SOC_SERIES_STM32F1X)
#ifndef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
#define RTC_ASYNCPRE BIT_MASK(7)
#else /* !CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
/* Get the highest possible clock for the subsecond register */
#define RTC_ASYNCPRE 1
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
#else /* CONFIG_SOC_SERIES_STM32F1X */
#define RTC_ASYNCPRE (RTCCLK_FREQ - 1)
#endif /* CONFIG_SOC_SERIES_STM32F1X */

/* Timeout in microseconds used to wait for flags */
#define RTC_TIMEOUT 1000

/* Adjust the second sync prescaler to get 1Hz on ck_spre */
#define RTC_SYNCPRE ((RTCCLK_FREQ / (1 + RTC_ASYNCPRE)) - 1)

#ifndef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
typedef uint32_t tick_t;
#else
typedef uint64_t tick_t;
#endif

struct rtc_stm32_config {
	struct counter_config_info counter_info;
	uint32_t async_prescaler;
#if !defined(CONFIG_SOC_SERIES_STM32F1X)
	uint32_t sync_prescaler;
#endif  /* !CONFIG_SOC_SERIES_STM32F1X */
	const struct stm32_pclken *pclken;
#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	uint32_t hse_prescaler;
#endif
};

struct rtc_stm32_data {
	counter_alarm_callback_t callback;
	uint32_t ticks;
	void *user_data;
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	bool irq_on_late;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
};

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

#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
static inline uint32_t ll_func_isenabled_interrupt_alarm(RTC_TypeDef *rtc)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	return LL_RTC_IsEnabledIT_ALR(rtc);
#else
	return LL_RTC_IsEnabledIT_ALRA(rtc);
#endif
}
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

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

/* When no error occurs, this function disables the RTC write protection and should be balanced
 * with a call to rtc_stm32_exit_init_mode (which enables RTC write protection).
 * In case of error, the write protection is enabled when leaving this function, so nothing more
 * needs to be made.
 */
static int rtc_stm32_enter_init_mode(void)
{
#if defined(CONFIG_SOC_SERIES_STM32F1X)
	/* Wait for RTC to be ready */
	if (!WAIT_FOR(LL_RTC_IsActiveFlag_RTOF(RTC), RTC_TIMEOUT, NULL)) {
		return -ETIMEDOUT;
	}

	LL_RTC_DisableWriteProtection(RTC);
#else
	LL_RTC_DisableWriteProtection(RTC);

	/* Check if the Initialization mode is set */
	if (LL_RTC_IsActiveFlag_INIT(RTC) == 0U) {
		/* Set the Initialization mode */
		LL_RTC_EnableInitMode(RTC);
		if (!WAIT_FOR(LL_RTC_IsActiveFlag_INIT(RTC), RTC_TIMEOUT, NULL)) {
			LL_RTC_DisableInitMode(RTC);
			LL_RTC_EnableWriteProtection(RTC);
			return -ETIMEDOUT;
		}
	}
#endif

	return 0;
}

static int rtc_stm32_exit_init_mode(void)
{
	int status = 0;

#if defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_EnableWriteProtection(RTC);

	/* Wait for RTC to be ready */
	if (!WAIT_FOR(LL_RTC_IsActiveFlag_RTOF(RTC), RTC_TIMEOUT, NULL)) {
		status = -ETIMEDOUT;
	}
#else
	LL_RTC_DisableInitMode(RTC);

	LL_RTC_EnableWriteProtection(RTC);
#endif

	return status;
}

#if !defined(CONFIG_COUNTER_RTC_STM32_SAVE_VALUE_BETWEEN_RESETS)
static int rtc_stm32_wait_for_synchro(void)
{
	int status = 0;

	/* Clear RSF flag */
	LL_RTC_ClearFlag_RS(RTC);

	if (!WAIT_FOR(LL_RTC_IsActiveFlag_RS(RTC), RTC_TIMEOUT, NULL)) {
		status = -ETIMEDOUT;
	}

	return status;
}

static int rtc_stm32_deinit(void)
{
	int ret;

	/* Set Initialization mode */
	ret = rtc_stm32_enter_init_mode();
	if (ret < 0) {
		LOG_ERR("Failed to enter RTC init mode");
		return ret;
	}

#if defined(CONFIG_SOC_SERIES_STM32F1X)
	stm32_reg_write(&RTC->CNTL, 0U);
	stm32_reg_write(&RTC->CNTH, 0U);
	stm32_reg_write(&RTC->PRLH, 0U);
	stm32_reg_write(&RTC->PRLL, 0x8000U);
	stm32_reg_write(&RTC->CRH, 0U);
	stm32_reg_write(&RTC->CRL, 0x20U);
#else /* CONFIG_SOC_SERIES_STM32F1X */
	stm32_reg_write(&RTC->CR, 0U);
	stm32_reg_write(&RTC->TR, 0U);
#ifdef RTC_WUTR_WUT
	stm32_reg_write(&RTC->WUTR, RTC_WUTR_WUT);
#endif /* RTC_WUTR_WUT */
	stm32_reg_write(&RTC->DR, RTC_DR_WDU_0 | RTC_DR_MU_0 | RTC_DR_DU_0);
	stm32_reg_write(&RTC->PRER, RTC_PRER_PREDIV_A | 0xFFU);
	stm32_reg_write(&RTC->ALRMAR, 0U);
#ifdef RTC_CR_ALRBE
	stm32_reg_write(&RTC->ALRMBR, 0U);
#endif /* RTC_CR_ALRBE */

#if HW_SUBSECOND_SUPPORT
	stm32_reg_write(&RTC->CALR, 0U);
	stm32_reg_write(&RTC->SHIFTR, 0U);
	stm32_reg_write(&RTC->ALRMASSR, 0U);
#ifdef RTC_CR_ALRBE
	stm32_reg_write(&RTC->ALRMBSSR, 0U);
#endif /* RTC_CR_ALRBE */
#endif /* HW_SUBSECOND_SUPPORT */

#if defined(RTC_PRIVCFGR_PRIV)
	stm32_reg_write(&RTC->PRIVCFGR, 0U);
#endif /* RTC_PRIVCFGR_PRIV */
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
	stm32_reg_write(&RTC->SECCFGR,  0U);
#endif /* (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U) */

	/* Reset I(C)SR register and exit initialization mode */
#ifdef RTC_ICSR_INIT
	stm32_reg_write(&RTC->ICSR, 0U);
#else
	stm32_reg_write(&RTC->ISR, 0U);
#endif

#endif /* CONFIG_SOC_SERIES_STM32F1X */

	/* Exit Initialization mode */
	ret = rtc_stm32_exit_init_mode();
	if (ret < 0) {
		LOG_ERR("Failed to exit RTC init mode");
		return ret;
	}

	return rtc_stm32_wait_for_synchro();
}
#endif

static int rtc_stm32_configure(const struct device *dev)
{
	const struct rtc_stm32_config *cfg = dev->config;
	int ret;

	/* Set Initialization mode */
	ret = rtc_stm32_enter_init_mode();
	if (ret < 0) {
		LOG_ERR("Failed to enter RTC init mode");
		return ret;
	}

#if defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_SetAsynchPrescaler(RTC, cfg->async_prescaler);
	LL_RTC_SetOutputSource(BKP, LL_RTC_CALIB_OUTPUT_NONE);
#else
	LL_RTC_SetHourFormat(RTC, LL_RTC_HOURFORMAT_24HOUR);
	LL_RTC_SetAsynchPrescaler(RTC, cfg->async_prescaler);
	LL_RTC_SetSynchPrescaler(RTC, cfg->sync_prescaler);
#endif

	/* Exit Initialization mode */
	ret = rtc_stm32_exit_init_mode();
	if (ret < 0) {
		LOG_ERR("Failed to exit RTC init mode");
	}

	return ret;
}

static int rtc_stm32_start(const struct device *dev)
{
#if defined(CONFIG_SOC_SERIES_STM32WBAX) || defined(CONFIG_SOC_SERIES_STM32U5X)
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct rtc_stm32_config *cfg = dev->config;

	/* Enable RTC bus clock */
	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken[0]) != 0) {
		LOG_ERR("RTC clock enabling failed");
		return -EIO;
	}
#else
	ARG_UNUSED(dev);

	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	stm32_backup_domain_enable_access();
	LL_RCC_EnableRTC();
	stm32_backup_domain_disable_access();
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
#endif

	return 0;
}


static int rtc_stm32_stop(const struct device *dev)
{
#if defined(CONFIG_SOC_SERIES_STM32WBAX) || defined(CONFIG_SOC_SERIES_STM32U5X)
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct rtc_stm32_config *cfg = dev->config;

	/* Disable RTC bus clock */
	if (clock_control_off(clk, (clock_control_subsys_t) &cfg->pclken[0]) != 0) {
		LOG_ERR("RTC clock disabling failed");
		return -EIO;
	}
#else
	ARG_UNUSED(dev);

	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	stm32_backup_domain_enable_access();
	LL_RCC_DisableRTC();
	stm32_backup_domain_disable_access();
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
#endif

	return 0;
}

#if !defined(COUNTER_NO_DATE)
tick_t rtc_stm32_read(const struct device *dev)
{
	struct tm now = { 0 };
	time_t ts;
	uint32_t rtc_date, rtc_time;
	tick_t ticks;
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	uint32_t rtc_subseconds;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
	ARG_UNUSED(dev);

	/* Read time and date registers. Make sure value of the previous register
	 * hasn't been changed while reading the next one.
	 */
	do {
		rtc_date = LL_RTC_DATE_Get(RTC);

#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
		do {
			rtc_time = LL_RTC_TIME_Get(RTC);
			rtc_subseconds = LL_RTC_TIME_GetSubSecond(RTC);
		} while (rtc_time != LL_RTC_TIME_Get(RTC));
#else /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
		rtc_time = LL_RTC_TIME_Get(RTC);
#endif

	} while (rtc_date != LL_RTC_DATE_Get(RTC));

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

	ticks = ts * counter_get_frequency(dev);
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	/* The RTC counts up, except for the subsecond register which counts
	 * down starting from the sync prescaler value. Add already counted
	 * ticks.
	 */
	ticks += RTC_SYNCPRE - rtc_subseconds;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

	return ticks;
}
#else /* defined(COUNTER_NO_DATE) */
tick_t rtc_stm32_read(const struct device *dev)
{
	uint32_t ticks;

	ARG_UNUSED(dev);

	ticks = LL_RTC_TIME_Get(RTC);

	return ticks;
}
#endif /* !defined(COUNTER_NO_DATE) */

static int rtc_stm32_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = (uint32_t)rtc_stm32_read(dev);
	return 0;
}

#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
static int rtc_stm32_get_value_64(const struct device *dev, uint64_t *ticks)
{
	*ticks = rtc_stm32_read(dev);
	return 0;
}
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
static void rtc_stm32_set_int_pending(void)
{
	NVIC_SetPendingIRQ(DT_INST_IRQN(0));
}
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

static int rtc_stm32_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
#if !defined(COUNTER_NO_DATE)
	struct tm alarm_tm;
	time_t alarm_val_s;
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	uint32_t alarm_val_ss;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
#else
	uint32_t remain;
#endif
	struct rtc_stm32_data *data = dev->data;
	int ret = 0;

	tick_t now = rtc_stm32_read(dev);
	tick_t ticks = alarm_cfg->ticks;

	if (data->callback != NULL) {
		LOG_DBG("Alarm busy");
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
		alarm_val_s = (time_t)(ticks / counter_get_frequency(dev)) + T_TIME_OFFSET;
	} else {
		alarm_val_s = (time_t)(ticks / counter_get_frequency(dev));
	}

	gmtime_r(&alarm_val_s, &alarm_tm);

#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	alarm_val_ss = ticks % counter_get_frequency(dev);
	LOG_DBG("Set Alarm: %llu", ticks);
#else /* !CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
	LOG_DBG("Set Alarm: %d", ticks);
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

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

	stm32_backup_domain_enable_access();

#if !defined(COUNTER_NO_DATE)
	LL_RTC_DisableWriteProtection(RTC);

	ll_func_disable_alarm(RTC);

	/* Configure the Alarm registers */
	LL_RTC_ALMA_DisableWeekday(RTC);
	LL_RTC_ALMA_SetDay(RTC, __LL_RTC_CONVERT_BIN2BCD(alarm_tm.tm_mday));
	LL_RTC_ALMA_ConfigTime(RTC, LL_RTC_TIME_FORMAT_AM_OR_24,
				__LL_RTC_CONVERT_BIN2BCD(alarm_tm.tm_hour),
				__LL_RTC_CONVERT_BIN2BCD(alarm_tm.tm_min),
				__LL_RTC_CONVERT_BIN2BCD(alarm_tm.tm_sec));
	LL_RTC_ALMA_SetMask(RTC, LL_RTC_ALMA_MASK_NONE);

	LL_RTC_EnableWriteProtection(RTC);
#else
	/* Set Initialization mode */
	ret = rtc_stm32_enter_init_mode();
	if (ret < 0) {
		goto out_disable_bkup_access;
	}

	/* Set the alarm */
	LL_RTC_ALARM_Set(RTC, remain);

	ret = rtc_stm32_exit_init_mode();
	if (ret < 0) {
		goto out_disable_bkup_access;
	}
#endif

	LL_RTC_DisableWriteProtection(RTC);
#if HW_SUBSECOND_SUPPORT
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	/* Care about all bits of the subsecond register */
	LL_RTC_ALMA_SetSubSecondMask(RTC, 0xF);
	LL_RTC_ALMA_SetSubSecond(RTC, RTC_SYNCPRE - alarm_val_ss);
#else
	LL_RTC_ALMA_SetSubSecondMask(RTC, 0);
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
#endif /* HW_SUBSECOND_SUPPORT */
	ll_func_enable_alarm(RTC);
	ll_func_clear_alarm_flag(RTC);
	ll_func_enable_interrupt_alarm(RTC);
	LL_RTC_EnableWriteProtection(RTC);

#if defined(COUNTER_NO_DATE)
out_disable_bkup_access:
#endif
	stm32_backup_domain_disable_access();

#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	/* The reference manual says:
	 * "Each change of the RTC_CR register is taken into account after
	 * 1 to 2 RTCCLK clock cycles due to clock synchronization."
	 * It means we need at least two cycles after programming the CR
	 * register. It is confirmed experimentally.
	 *
	 * It should happen only if one tick alarm is requested and a tick
	 * occurs while processing the function. Trigger the irq manually in
	 * this case.
	 */
	now = rtc_stm32_read(dev);
	if ((ticks - now < 2) || (now > ticks)) {
		data->irq_on_late = 1;
		rtc_stm32_set_int_pending();
	}
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

	return ret;
}


static int rtc_stm32_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct rtc_stm32_data *data = dev->data;

	stm32_backup_domain_enable_access();
	LL_RTC_DisableWriteProtection(RTC);
	ll_func_clear_alarm_flag(RTC);
	ll_func_disable_interrupt_alarm(RTC);
	ll_func_disable_alarm(RTC);
	LL_RTC_EnableWriteProtection(RTC);
	stm32_backup_domain_disable_access();

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

	if (ll_func_is_active_alarm(RTC) != 0
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	    || (data->irq_on_late && ll_func_isenabled_interrupt_alarm(RTC))
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
	) {

		stm32_backup_domain_enable_access();
		LL_RTC_DisableWriteProtection(RTC);
		ll_func_clear_alarm_flag(RTC);
		ll_func_disable_interrupt_alarm(RTC);
		ll_func_disable_alarm(RTC);
		LL_RTC_EnableWriteProtection(RTC);
		stm32_backup_domain_disable_access();
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
		data->irq_on_late = 0;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

		if (alarm_callback != NULL) {
			data->callback = NULL;
			alarm_callback(dev, 0, now, data->user_data);
		}
	}

#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_ClearFlag_0_31(RTC_EXTI_LINE);
#elif defined(CONFIG_SOC_SERIES_STM32C0X) \
	|| defined(CONFIG_SOC_SERIES_STM32G0X) \
	|| defined(CONFIG_SOC_SERIES_STM32L5X) \
	|| defined(CONFIG_SOC_SERIES_STM32H5X)
	LL_EXTI_ClearRisingFlag_0_31(RTC_EXTI_LINE);
#elif defined(CONFIG_SOC_SERIES_STM32U5X) || defined(CONFIG_SOC_SERIES_STM32WBAX)
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
	int ret = -EIO;

	data->callback = NULL;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable RTC bus clock */
	if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken[0]) != 0) {
		LOG_ERR("clock op failed");
		return -EIO;
	}

	/* Enable Backup access */
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	stm32_backup_domain_enable_access();

#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	/* Must be configured before selecting the RTC clock source */
	LL_RCC_SetRTC_HSEPrescaler(cfg->hse_prescaler);
#endif

	/* Enable RTC clock source */
	if (clock_control_configure(clk,
				    (clock_control_subsys_t) &cfg->pclken[1],
				    NULL) != 0) {
		LOG_ERR("clock configure failed");
		goto out_disable_bkup_access;
	}

#if !defined(CONFIG_SOC_SERIES_STM32WBAX)
	LL_RCC_EnableRTC();
#endif

	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

#if !defined(CONFIG_COUNTER_RTC_STM32_SAVE_VALUE_BETWEEN_RESETS)
	ret = rtc_stm32_deinit();
	if (ret < 0) {
		LOG_ERR("Failed to deinit RTC");
		goto out_disable_bkup_access;
	}
#endif

	ret = rtc_stm32_configure(dev);
	if (ret < 0) {
		LOG_ERR("Failed to init RTC");
		goto out_disable_bkup_access;
	}

#ifdef RTC_CR_BYPSHAD
	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_EnableShadowRegBypass(RTC);
	LL_RTC_EnableWriteProtection(RTC);
#endif /* RTC_CR_BYPSHAD */

#if defined(CONFIG_SOC_SERIES_STM32H7X) && defined(CONFIG_CPU_CORTEX_M4)
	LL_C2_EXTI_EnableIT_0_31(RTC_EXTI_LINE);
	LL_EXTI_EnableRisingTrig_0_31(RTC_EXTI_LINE);
#elif defined(CONFIG_SOC_SERIES_STM32U5X) || defined(CONFIG_SOC_SERIES_STM32WBAX)
	/* in STM32U5 family RTC is not connected to EXTI */
#else
	LL_EXTI_EnableIT_0_31(RTC_EXTI_LINE);
	LL_EXTI_EnableRisingTrig_0_31(RTC_EXTI_LINE);
#endif

out_disable_bkup_access:
	stm32_backup_domain_disable_access();

	if (ret == 0) {
		rtc_stm32_irq_config(dev);
	}

	return ret;
}

static struct rtc_stm32_data rtc_data;

static const struct stm32_pclken rtc_clk[] = STM32_DT_INST_CLOCKS(0);

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
	.counter_info = {
		.max_top_value = UINT32_MAX,
#ifndef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
		/* freq = 1Hz for not subsec based driver */
		.freq = RTCCLK_FREQ / ((RTC_ASYNCPRE + 1) * (RTC_SYNCPRE + 1)),
#else /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
		.freq = RTCCLK_FREQ / (RTC_ASYNCPRE + 1),
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 1,
	},
#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_LSI ||                                      \
	DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_LSE
	.async_prescaler = DT_INST_PROP_OR(0, async_prescaler, RTC_ASYNCPRE),
#if !defined(CONFIG_SOC_SERIES_STM32F1X)
	.sync_prescaler = DT_INST_PROP_OR(0, sync_prescaler, RTC_SYNCPRE),
#endif /* !CONFIG_SOC_SERIES_STM32F1X */
#elif DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	.async_prescaler = DT_INST_PROP_OR(0, async_prescaler, _HSE_ASYNC_PRESCALER - 1),
#if !defined(CONFIG_SOC_SERIES_STM32F1X)
	.sync_prescaler = DT_INST_PROP_OR(0, hse_prescaler, RTC_HSE_SYNC_PRESCALER - 1),
#endif /* !CONFIG_SOC_SERIES_STM32F1X */
#else
#error Invalid RTC SRC
#endif
	.pclken = rtc_clk,
#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	.hse_prescaler = DT_INST_PROP_OR(0, hse_prescaler, RTC_HSE_PRESCALER),
#endif
};

#ifdef CONFIG_PM_DEVICE
static int rtc_stm32_pm_action(const struct device *dev,
			       enum pm_device_action action)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct rtc_stm32_config *cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Enable RTC bus clock */
		if (clock_control_on(clk, (clock_control_subsys_t) &cfg->pclken[0]) != 0) {
			LOG_ERR("clock op failed");
			return -EIO;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(counter, rtc_stm32_driver_api) = {
	.start = rtc_stm32_start,
	.stop = rtc_stm32_stop,
	.get_value = rtc_stm32_get_value,
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	.get_value_64 = rtc_stm32_get_value_64,
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
	.set_alarm = rtc_stm32_set_alarm,
	.cancel_alarm = rtc_stm32_cancel_alarm,
	.set_top_value = rtc_stm32_set_top_value,
	.get_pending_int = rtc_stm32_get_pending_int,
	.get_top_value = rtc_stm32_get_top_value,
};

PM_DEVICE_DT_INST_DEFINE(0, rtc_stm32_pm_action);

DEVICE_DT_INST_DEFINE(0, &rtc_stm32_init, PM_DEVICE_DT_INST_GET(0),
		    &rtc_data, &rtc_config, PRE_KERNEL_1,
		    CONFIG_COUNTER_INIT_PRIORITY, &rtc_stm32_driver_api);

static void rtc_stm32_irq_config(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    rtc_stm32_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}
