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
#include <zephyr/drivers/interrupt_controller/intc_exti_stm32.h>
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

#if CONFIG_STM32_HAL2
#define STM32_RTC_HOUR_FORMAT_24HOUR		LL_RTC_HOUR_FORMAT_24HOUR
#define STM32_RTC_HOUR_FORMAT_AMPM		LL_RTC_HOUR_FORMAT_AMPM
#define STM32_RTC_TIME_FORMAT_AM_24H		LL_RTC_TIME_FORMAT_AM_24H
#define STM32_RTC_GET_SECOND			LL_RTC_GET_SECOND
#define STM32_RTC_GET_MINUTE			LL_RTC_GET_MINUTE
#define STM32_RTC_GET_HOUR			LL_RTC_GET_HOUR
#define STM32_RTC_GET_DAY			LL_RTC_GET_DAY
#define STM32_RTC_GET_WEEKDAY			LL_RTC_GET_WEEKDAY
#define STM32_RTC_GET_YEAR			LL_RTC_GET_YEAR
#define STM32_RTC_GET_MONTH			LL_RTC_GET_MONTH
#define STM32_RTC_EnableBypassShadowReg		LL_RTC_EnableBypassShadowReg
#define STM32_RTC_DisableBypassShadowReg	LL_RTC_DisableBypassShadowReg

/* On HAL2 this macro skips the RTC instance as the first argument,
 * only the potential following arguments are passed.
 */
#define STM32_ARG(dev, ...)	__VA_ARGS__
#else /* CONFIG_STM32_HAL2 */
#define STM32_RTC_HOUR_FORMAT_24HOUR		LL_RTC_HOURFORMAT_24HOUR
#define STM32_RTC_HOUR_FORMAT_AMPM		LL_RTC_HOURFORMAT_AMPM
#define STM32_RTC_TIME_FORMAT_AM_24H		LL_RTC_TIME_FORMAT_AM_OR_24
#define STM32_RTC_GET_SECOND			__LL_RTC_GET_SECOND
#define STM32_RTC_GET_MINUTE			__LL_RTC_GET_MINUTE
#define STM32_RTC_GET_HOUR			__LL_RTC_GET_HOUR
#define STM32_RTC_GET_DAY			__LL_RTC_GET_DAY
#define STM32_RTC_GET_WEEKDAY			__LL_RTC_GET_WEEKDAY
#define STM32_RTC_GET_MONTH			__LL_RTC_GET_MONTH
#define STM32_RTC_GET_YEAR			__LL_RTC_GET_YEAR
#define STM32_RTC_EnableBypassShadowReg		LL_RTC_EnableShadowRegBypass
#define STM32_RTC_DisableBypassShadowReg	LL_RTC_DisableShadowRegBypass

/* On HAL1 this macro adds the RTC instance as the first argument,
 * with or without a comma depending on number of arguments.
 */
#define STM32_ARG(dev, ...)	COND_CODE_1(IS_EMPTY(__VA_ARGS__), (dev), (dev, __VA_ARGS__))
#endif /* CONFIG_STM32_HAL2 */

#if defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32F2X) || \
	(defined(CONFIG_SOC_SERIES_STM32L1X) && !defined(RTC_SUBSECOND_SUPPORT))
/* subsecond counting is not supported by some STM32L1x MCUs (Cat.1) & by STM32F1x/2x SoC series */
#define HW_SUBSECOND_SUPPORT 0
#else
#define HW_SUBSECOND_SUPPORT 1
#endif

/* Seconds from 1970-01-01T00:00:00 to 2000-01-01T00:00:00 */
#define T_TIME_OFFSET 946684800

#if DT_INST_NODE_HAS_PROP(0, alrm_exti_line)
#define RTC_EXTI_LINE_NUM DT_INST_PROP(0, alrm_exti_line)
#endif /* DT_INST_NODE_HAS_PROP(0, alrm_exti_line) */

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
	size_t pclken_count;
#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	uint32_t hse_prescaler;
#endif
};

struct rtc_stm32_data {
	counter_alarm_callback_t callback;
#if defined(CONFIG_COUNTER_64BITS_TICKS)
	counter_alarm_callback_64_t callback_64;
#endif
	uint32_t ticks;
	void *user_data;
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	uint32_t guard_period;
#endif
#if defined(CONFIG_SOC_SERIES_STM32N6X) || defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)
	bool irq_on_late;
#endif
};

static void rtc_stm32_clear_callback(struct rtc_stm32_data *data)
{
	data->callback = NULL;
#if defined(CONFIG_COUNTER_64BITS_TICKS)
	data->callback_64 = NULL;
#endif
}

#if defined(CONFIG_SOC_SERIES_STM32N6X)
#if defined(RTC_EXTI_LINE_NUM)
#define RTC_N6_EXTI_LINE BIT(RTC_EXTI_LINE_NUM)

static void rtc_stm32_n6_clear_exti_pending(void)
{
	LL_EXTI_ClearRisingFlag_0_31(RTC_N6_EXTI_LINE);
	LL_EXTI_ClearFallingFlag_0_31(RTC_N6_EXTI_LINE);
}

static void rtc_stm32_n6_configure_exti(void)
{
	rtc_stm32_n6_clear_exti_pending();
	CLEAR_BIT(EXTI->RTSR1, RTC_N6_EXTI_LINE);
	CLEAR_BIT(EXTI->FTSR1, RTC_N6_EXTI_LINE);
	LL_EXTI_DisableSecure_0_31(RTC_N6_EXTI_LINE);
	LL_EXTI_DisableEvent_0_31(RTC_N6_EXTI_LINE);
	LL_EXTI_EnableIT_0_31(RTC_N6_EXTI_LINE);
}
#endif /* RTC_EXTI_LINE_NUM */

static bool rtc_stm32_alarm_is_late(uint32_t now, uint32_t target, uint32_t guard_period)
{
	return guard_period != 0U && (uint32_t)(now - target) < guard_period;
}
#endif

static inline void ll_clear_alarm_flag(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	LL_RTC_ClearFlag_WUT(RTC);
#elif defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_ClearFlag_ALR(STM32_ARG(RTC));
#else
	LL_RTC_ClearFlag_ALRA(STM32_ARG(RTC));
#endif
}

static inline uint32_t ll_is_active_alarm(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	/* As in ST HAL, secure images read the masked status in SMISR. */
	return (RTC->SMISR & RTC_SMISR_WUTMF) != 0U;
#elif defined(CONFIG_SOC_SERIES_STM32F1X)
	return LL_RTC_IsActiveFlag_ALR(STM32_ARG(RTC));
#else
	return LL_RTC_IsActiveFlag_ALRA(STM32_ARG(RTC));
#endif
}

static inline void ll_enable_interrupt_alarm(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	LL_RTC_EnableIT_WUT(RTC);
#elif defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_EnableIT_ALR(STM32_ARG(RTC));
#else
	LL_RTC_EnableIT_ALRA(STM32_ARG(RTC));
#endif
}

static inline void ll_disable_interrupt_alarm(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	LL_RTC_DisableIT_WUT(RTC);
#elif defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_DisableIT_ALR(STM32_ARG(RTC));
#else
	LL_RTC_DisableIT_ALRA(STM32_ARG(RTC));
#endif
}

#if defined(CONFIG_SOC_SERIES_STM32N6X) || defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)
static inline uint32_t ll_isenabled_interrupt_alarm(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	return LL_RTC_IsEnabledIT_WUT(RTC);
#elif defined(CONFIG_SOC_SERIES_STM32F1X)
	return LL_RTC_IsEnabledIT_ALR(STM32_ARG(RTC));
#else
	return LL_RTC_IsEnabledIT_ALRA(STM32_ARG(RTC));
#endif
}
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

static inline void ll_enable_alarm(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	LL_RTC_WAKEUP_Enable(RTC);
#elif !defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_ALMA_Enable(STM32_ARG(RTC));
#endif
}

static inline void ll_disable_alarm(void)
{
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	LL_RTC_WAKEUP_Disable(RTC);
#elif !defined(CONFIG_SOC_SERIES_STM32F1X)
	LL_RTC_ALMA_Disable(STM32_ARG(RTC));
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
	if (!WAIT_FOR(LL_RTC_IsActiveFlag_RTOF(STM32_ARG(RTC)), RTC_TIMEOUT, NULL)) {
		return -ETIMEDOUT;
	}

	LL_RTC_DisableWriteProtection(STM32_ARG(RTC));
#else
	LL_RTC_DisableWriteProtection(STM32_ARG(RTC));

	/* Check if the Initialization mode is set */
	if (LL_RTC_IsActiveFlag_INIT(STM32_ARG(RTC)) == 0U) {
		/* Set the Initialization mode */
		LL_RTC_EnableInitMode(STM32_ARG(RTC));
		if (!WAIT_FOR(LL_RTC_IsActiveFlag_INIT(STM32_ARG(RTC)), RTC_TIMEOUT, NULL)) {
			LL_RTC_DisableInitMode(STM32_ARG(RTC));
			LL_RTC_EnableWriteProtection(STM32_ARG(RTC));
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
	LL_RTC_EnableWriteProtection(STM32_ARG(RTC));

	/* Wait for RTC to be ready */
	if (!WAIT_FOR(LL_RTC_IsActiveFlag_RTOF(STM32_ARG(RTC)), RTC_TIMEOUT, NULL)) {
		status = -ETIMEDOUT;
	}
#else
	LL_RTC_DisableInitMode(STM32_ARG(RTC));

	LL_RTC_EnableWriteProtection(STM32_ARG(RTC));
#endif

	return status;
}

#if !defined(CONFIG_COUNTER_RTC_STM32_SAVE_VALUE_BETWEEN_RESETS) || \
	defined(CONFIG_SOC_SERIES_STM32N6X)
static int rtc_stm32_wait_for_synchro(void)
{
	int status = 0;

	/* Clear RSF flag */
	LL_RTC_ClearFlag_RS(STM32_ARG(RTC));

	if (!WAIT_FOR(LL_RTC_IsActiveFlag_RS(STM32_ARG(RTC)), RTC_TIMEOUT, NULL)) {
		status = -ETIMEDOUT;
	}

	return status;
}

#endif

#if !defined(CONFIG_COUNTER_RTC_STM32_SAVE_VALUE_BETWEEN_RESETS)
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
	LL_RTC_SetAsynchPrescaler(STM32_ARG(RTC, cfg->async_prescaler));
	LL_RTC_SetOutputSource(BKP, LL_RTC_CALIB_OUTPUT_NONE);
#else
	LL_RTC_SetHourFormat(STM32_ARG(RTC, STM32_RTC_HOUR_FORMAT_24HOUR));
	LL_RTC_SetAsynchPrescaler(STM32_ARG(RTC, cfg->async_prescaler));
	LL_RTC_SetSynchPrescaler(STM32_ARG(RTC, cfg->sync_prescaler));
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
#endif /* CONFIG_SOC_SERIES_STM32WBAX || CONFIG_SOC_SERIES_STM32U5X */

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
#endif /* CONFIG_SOC_SERIES_STM32WBAX || CONFIG_SOC_SERIES_STM32U5X */

	return 0;
}

#if !defined(COUNTER_NO_DATE)
tick_t rtc_stm32_read(const struct device *dev)
{
	struct tm now = { 0 };
	int64_t ts;
	uint32_t rtc_date, rtc_time;
	tick_t ticks;
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	uint32_t rtc_subsecond;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
	ARG_UNUSED(dev);

	do {
		/* read date, time and subseconds and relaunch if a day increment occurred
		 * while doing so as it will result in an erroneous result otherwise
		 */
		rtc_date = LL_RTC_DATE_Get(STM32_ARG(RTC));
		do {
			/* read time and subseconds and relaunch if a second increment occurred
			 * while doing so as it will result in an erroneous result otherwise
			 */
			rtc_time = LL_RTC_TIME_Get(STM32_ARG(RTC));
#if CONFIG_COUNTER_RTC_STM32_SUBSECONDS
			do {
				/* read subseconds and relaunch if a second increment occurred
				 * while doing so as it will result in an erroneous result otherwise
				 */
				rtc_subsecond = LL_RTC_TIME_GetSubSecond(STM32_ARG(RTC));
			} while (rtc_subsecond != LL_RTC_TIME_GetSubSecond(STM32_ARG(RTC)));
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
		} while (rtc_time != LL_RTC_TIME_Get(STM32_ARG(RTC)));
	} while (rtc_date != LL_RTC_DATE_Get(STM32_ARG(RTC)));

	/* Convert calendar datetime to UNIX timestamp */
	/* RTC start time: 1st, Jan, 2000 */
	/* time_t start:   1st, Jan, 1970 */
	now.tm_year = 100 + bcd2bin(STM32_RTC_GET_YEAR(rtc_date));
	/* tm_mon allowed values are 0-11 */
	now.tm_mon = bcd2bin(STM32_RTC_GET_MONTH(rtc_date)) - 1;
	now.tm_mday = bcd2bin(STM32_RTC_GET_DAY(rtc_date));

	now.tm_hour = bcd2bin(STM32_RTC_GET_HOUR(rtc_time));
	now.tm_min = bcd2bin(STM32_RTC_GET_MINUTE(rtc_time));
	now.tm_sec = bcd2bin(STM32_RTC_GET_SECOND(rtc_time));

	ts = timeutil_timegm64(&now);

	/* Return number of seconds since RTC init */
	ts -= T_TIME_OFFSET;

	ticks = ts * counter_get_frequency(dev);
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	/* The RTC counts up, except for the subsecond register which counts
	 * down starting from the sync prescaler value. Add already counted
	 * ticks.
	 */
	ticks += RTC_SYNCPRE - rtc_subsecond;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

	return ticks;
}
#else /* defined(COUNTER_NO_DATE) */
tick_t rtc_stm32_read(const struct device *dev)
{
	uint32_t ticks;

	ARG_UNUSED(dev);

	ticks = LL_RTC_TIME_Get(STM32_ARG(RTC));

	return ticks;
}
#endif /* !defined(COUNTER_NO_DATE) */

static int rtc_stm32_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = (uint32_t)rtc_stm32_read(dev);
	return 0;
}

#if defined(CONFIG_COUNTER_64BITS_TICKS)
static int rtc_stm32_get_value_64(const struct device *dev, uint64_t *ticks)
{
	*ticks = rtc_stm32_read(dev);
	return 0;
}
#endif /* CONFIG_COUNTER_64BITS_TICKS */

#if defined(CONFIG_SOC_SERIES_STM32N6X) || defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)
static void rtc_stm32_set_int_pending(void)
{
	k_irq_set_pending(DT_INST_IRQN(0));
}
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

#if defined(CONFIG_SOC_SERIES_STM32N6X)
static bool rtc_stm32_wait_wutw(const struct device *dev)
{
	const struct rtc_stm32_config *cfg = dev->config;
	const uint32_t subsecond_period = cfg->sync_prescaler + 1U;
	/*
	 * AN4759 Table 11: RTC3 needs one previous ck_wut (WUCKSEL[2] = 0)
	 * or ck_apre (WUCKSEL[2] = 1), plus one RTCCLK cycle. Budget the
	 * slowest divided clock, or the configured asynchronous prescaler,
	 * and retain 1 ms of margin. CKSPRE with PREDIV_A = 127 and LSE
	 * needs up to 129 / 32768 s (3.94 ms), before that margin.
	 */
	const uint32_t wake_div = (RTC->CR & RTC_CR_WUCKSEL_2) != 0U ?
		cfg->async_prescaler + 1U : 16U;
	const uint32_t timeout_us = RTC_TIMEOUT +
		(uint32_t)DIV_ROUND_UP((uint64_t)(wake_div + 1U) * 1000000U, RTCCLK_FREQ);
	/* Include the partially elapsed subsecond tick in the timeout budget. */
	const uint32_t timeout_ticks = 1U +
		(uint32_t)DIV_ROUND_UP((uint64_t)RTCCLK_FREQ * timeout_us,
				     (uint64_t)(cfg->async_prescaler + 1U) * 1000000U);
	const uint32_t start = LL_RTC_TIME_GetSubSecond(RTC);
	const uint32_t cpu_timeout_cycles =
		MAX(1U, (uint32_t)DIV_ROUND_UP((uint64_t)SystemCoreClock * timeout_us, 1000000U));
	uint32_t start_cycles;

	CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
	DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
	start_cycles = DWT->CYCCNT;

	/*
	 * The system-timer companion holds timeout_lock. WAIT_FOR() would
	 * recurse into that lock through the SysTick cycle getter. Use RTC
	 * ticks instead; DWT also bounds the wait if the RTC clock has stopped.
	 * This runs before STOP entry, with SystemCoreClock still valid.
	 */
	for (;;) {
		uint32_t current;
		uint32_t elapsed;

		if (LL_RTC_IsActiveFlag_WUTW(RTC)) {
			return true;
		}

		current = LL_RTC_TIME_GetSubSecond(RTC);
		elapsed = start >= current ? start - current : start + subsecond_period - current;
		if (elapsed >= timeout_ticks ||
		    (uint32_t)(DWT->CYCCNT - start_cycles) >= cpu_timeout_cycles) {
			return false;
		}
	}
}

static int rtc_stm32_n6_set_alarm(const struct device *dev,
				  const struct counter_alarm_cfg *alarm_cfg)
{
	struct rtc_stm32_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;
	uint32_t initial_now = 0U;
	uint32_t initial_ticks = 0U;
	uint32_t counter_frequency = counter_get_frequency(dev);
	uint64_t wake_ticks;
	uint32_t wake_clock;
	uint32_t old_cr;
	bool absolute = (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) != 0U;
	bool late = false;
	int ret = 0;

	if (absolute) {
		initial_now = (uint32_t)rtc_stm32_read(dev);
		ticks = alarm_cfg->ticks - initial_now;
		initial_ticks = ticks;
		late = rtc_stm32_alarm_is_late(initial_now, alarm_cfg->ticks, data->guard_period);
		if (late && (alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) == 0U) {
			rtc_stm32_clear_callback(data);
			return -ETIME;
		}
	}

	old_cr = RTC->CR;
	stm32_backup_domain_enable_access();
	LL_RTC_DisableWriteProtection(RTC);
	ll_disable_interrupt_alarm();
	ll_disable_alarm();

	/* A software late expiry does not write WUTR or WUCKSEL. */
	if (!late && !rtc_stm32_wait_wutw(dev)) {
		if ((old_cr & RTC_CR_WUTIE) != 0U) {
			ll_enable_interrupt_alarm();
		}
		if ((old_cr & RTC_CR_WUTE) != 0U) {
			ll_enable_alarm();
		}
		ret = -ETIMEDOUT;
		goto out;
	}

	ll_clear_alarm_flag();
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
#if defined(RTC_EXTI_LINE_NUM)
	rtc_stm32_n6_configure_exti();
#endif
	data->irq_on_late = false;

	if (absolute && !late) {
		uint32_t now = (uint32_t)rtc_stm32_read(dev);

		/* Detect crossing during setup even if the guard period is zero. */
		late = rtc_stm32_alarm_is_late(now, alarm_cfg->ticks, data->guard_period) ||
			(uint32_t)(now - initial_now) > ticks ||
			(ticks != 0U && (uint32_t)(now - initial_now) == ticks);
		ticks = alarm_cfg->ticks - now;
	}

	if (late) {
		goto late_alarm;
	}

	wake_ticks = DIV_ROUND_UP((uint64_t)ticks * (RTCCLK_FREQ / 16U), counter_frequency);
	wake_clock = LL_RTC_WAKEUPCLOCK_DIV_16;
	wake_ticks = MAX(wake_ticks, 1U);

	/* WUTR stores the number of wakeup clock cycles minus one. */
	if (wake_ticks > (uint32_t)UINT16_MAX + 1U) {
		wake_clock = LL_RTC_WAKEUPCLOCK_CKSPRE;
		wake_ticks = DIV_ROUND_UP((uint64_t)ticks, counter_frequency);
	}

	if (wake_ticks > (uint32_t)UINT16_MAX + 1U) {
		ret = -EINVAL;
		goto out;
	}

	LL_RTC_WAKEUP_SetAutoReload(RTC, (uint32_t)wake_ticks - 1U);
	LL_RTC_WAKEUP_SetClock(RTC, wake_clock);
	if (absolute) {
		uint32_t now = (uint32_t)rtc_stm32_read(dev);
		uint32_t elapsed = now - initial_now;

		/* Do not activate a full-wrap delay if the target passed while programming. */
		if (rtc_stm32_alarm_is_late(now, alarm_cfg->ticks, data->guard_period) ||
		    elapsed > initial_ticks || (initial_ticks != 0U && elapsed == initial_ticks)) {
			goto late_alarm;
		}
	}

	ll_enable_interrupt_alarm();
	ll_enable_alarm();
	goto out;

late_alarm:
	ret = -ETIME;
	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_EXPIRE_WHEN_LATE) != 0U) {
		data->irq_on_late = true;
		ll_enable_interrupt_alarm();
	}

out:
	/* Drop ownership before a pending callback can run and rearm the channel. */
	if (ret != 0 && !data->irq_on_late) {
		rtc_stm32_clear_callback(data);
	}
	LL_RTC_EnableWriteProtection(RTC);
	stm32_backup_domain_disable_access();
	if (data->irq_on_late) {
		rtc_stm32_set_int_pending();
	}

	return ret;
}
#endif

static int rtc_stm32_set_alarm_common(const struct device *dev,
				    const struct counter_alarm_cfg *alarm_cfg,
				    counter_alarm_callback_64_t callback_64)
{
#if !defined(CONFIG_SOC_SERIES_STM32N6X)
#if !defined(COUNTER_NO_DATE)
	struct tm alarm_tm;
	time_t alarm_val_s;
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	uint32_t alarm_val_ss;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
#else
	uint32_t remain;
#endif
	int ret = 0;

	tick_t now = rtc_stm32_read(dev);
	tick_t ticks = alarm_cfg->ticks;
#endif
	struct rtc_stm32_data *data = dev->data;

	if (data->callback != NULL
#if defined(CONFIG_COUNTER_64BITS_TICKS)
	    || data->callback_64 != NULL
#endif
	) {
		LOG_DBG("Alarm busy");
		return -EBUSY;
	}

	data->callback = alarm_cfg->callback;
#if defined(CONFIG_COUNTER_64BITS_TICKS)
	data->callback_64 = callback_64;
#else
	ARG_UNUSED(callback_64);
#endif
	data->user_data = alarm_cfg->user_data;

#if defined(CONFIG_SOC_SERIES_STM32N6X)
	return rtc_stm32_n6_set_alarm(dev, alarm_cfg);
#else
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
	LL_RTC_DisableWriteProtection(STM32_ARG(RTC));

	ll_disable_alarm();

	/* Configure the Alarm registers */
	LL_RTC_ALMA_DisableWeekday(STM32_ARG(RTC));
	LL_RTC_ALMA_SetDay(STM32_ARG(RTC, bin2bcd(alarm_tm.tm_mday)));
	LL_RTC_ALMA_ConfigTime(STM32_ARG(RTC, STM32_RTC_TIME_FORMAT_AM_24H,
				bin2bcd(alarm_tm.tm_hour),
				bin2bcd(alarm_tm.tm_min),
				bin2bcd(alarm_tm.tm_sec)));
	LL_RTC_ALMA_SetMask(STM32_ARG(RTC, LL_RTC_ALMA_MASK_NONE));

	LL_RTC_EnableWriteProtection(STM32_ARG(RTC));
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

	LL_RTC_DisableWriteProtection(STM32_ARG(RTC));
#if HW_SUBSECOND_SUPPORT
#ifdef CONFIG_COUNTER_RTC_STM32_SUBSECONDS
	/* Care about all bits of the subsecond register */
	LL_RTC_ALMA_SetSubSecondMask(STM32_ARG(RTC, 0xF));
	LL_RTC_ALMA_SetSubSecond(STM32_ARG(RTC, RTC_SYNCPRE - alarm_val_ss));
#else
	LL_RTC_ALMA_SetSubSecondMask(STM32_ARG(RTC, 0));
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
#endif /* HW_SUBSECOND_SUPPORT */
	ll_enable_alarm();
	ll_clear_alarm_flag();
	ll_enable_interrupt_alarm();
	LL_RTC_EnableWriteProtection(STM32_ARG(RTC));

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
		data->irq_on_late = true;
		rtc_stm32_set_int_pending();
	}
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

	if (ret < 0) {
		rtc_stm32_clear_callback(data);
	}

	return ret;
#endif /* CONFIG_SOC_SERIES_STM32N6X */
}

static int rtc_stm32_set_alarm(const struct device *dev, uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(chan_id);

	return rtc_stm32_set_alarm_common(dev, alarm_cfg, NULL);
}

#if defined(CONFIG_COUNTER_64BITS_TICKS)
static int rtc_stm32_set_alarm_64(const struct device *dev, uint8_t chan_id,
				   const struct counter_alarm_cfg_64 *alarm_cfg)
{
	const struct counter_config_info *info = dev->config;
	struct counter_alarm_cfg cfg = {
		.ticks = (uint32_t)alarm_cfg->ticks,
		.user_data = alarm_cfg->user_data,
		.flags = alarm_cfg->flags,
	};

	ARG_UNUSED(chan_id);

	/* The 64-bit API does not extend this driver's advertised alarm range. */
	if (alarm_cfg->ticks > info->max_top_value) {
		return -EINVAL;
	}

	return rtc_stm32_set_alarm_common(dev, &cfg, alarm_cfg->callback);
}
#endif

static int rtc_stm32_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct rtc_stm32_data *data = dev->data;

	stm32_backup_domain_enable_access();
	LL_RTC_DisableWriteProtection(STM32_ARG(RTC));
	ll_clear_alarm_flag();
	ll_disable_interrupt_alarm();
	ll_disable_alarm();
	LL_RTC_EnableWriteProtection(STM32_ARG(RTC));
	stm32_backup_domain_disable_access();

	rtc_stm32_clear_callback(data);
#if defined(CONFIG_SOC_SERIES_STM32N6X) || defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)
	data->irq_on_late = false;
#endif
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	NVIC_ClearPendingIRQ(DT_INST_IRQN(0));
#if defined(RTC_EXTI_LINE_NUM)
	rtc_stm32_n6_clear_exti_pending();
#endif
#endif

	return 0;
}


static uint32_t rtc_stm32_get_pending_int(const struct device *dev)
{
	return ll_is_active_alarm() != 0;
}


static uint32_t rtc_stm32_get_top_value(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value;
}


#if defined(CONFIG_COUNTER_64BITS_TICKS)
static uint64_t rtc_stm32_get_top_value_64(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value_64;
}

static int rtc_stm32_set_top_value_64(const struct device *dev,
				    const struct counter_top_cfg_64 *cfg)
{
	const struct counter_config_info *info = dev->config;

	if (cfg->ticks != info->max_top_value_64 || cfg->callback != NULL ||
	    (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0U) {
		return -ENOTSUP;
	}

	return 0;
}
#endif

#if defined(CONFIG_SOC_SERIES_STM32N6X)
static uint32_t rtc_stm32_get_guard_period(const struct device *dev, uint32_t flags)
{
	const struct rtc_stm32_data *data = dev->data;

	ARG_UNUSED(flags);
	return data->guard_period;
}

static int rtc_stm32_set_guard_period(const struct device *dev, uint32_t guard, uint32_t flags)
{
	struct rtc_stm32_data *data = dev->data;
	const struct counter_config_info *info = dev->config;

	ARG_UNUSED(flags);
	if (guard >= info->max_top_value) {
		return -EINVAL;
	}

	data->guard_period = guard;
	return 0;
}

#if defined(CONFIG_COUNTER_64BITS_TICKS)
static uint64_t rtc_stm32_get_guard_period_64(const struct device *dev, uint32_t flags)
{
	return rtc_stm32_get_guard_period(dev, flags);
}

static int rtc_stm32_set_guard_period_64(const struct device *dev, uint64_t guard, uint32_t flags)
{
	if (guard >= UINT32_MAX) {
		return -EINVAL;
	}

	return rtc_stm32_set_guard_period(dev, (uint32_t)guard, flags);
}
#endif
#endif

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
#if defined(CONFIG_COUNTER_64BITS_TICKS)
	counter_alarm_callback_64_t alarm_callback_64 = data->callback_64;
#endif
	void *user_data = data->user_data;
	tick_t now = rtc_stm32_read(dev);

	if (ll_is_active_alarm() != 0
#if defined(CONFIG_SOC_SERIES_STM32N6X) || defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)
	    || (data->irq_on_late && ll_isenabled_interrupt_alarm())
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */
	) {

		stm32_backup_domain_enable_access();
		LL_RTC_DisableWriteProtection(STM32_ARG(RTC));
		ll_clear_alarm_flag();
		ll_disable_interrupt_alarm();
		ll_disable_alarm();
		LL_RTC_EnableWriteProtection(STM32_ARG(RTC));
		stm32_backup_domain_disable_access();
#if defined(CONFIG_SOC_SERIES_STM32N6X) || defined(CONFIG_COUNTER_RTC_STM32_SUBSECONDS)
		data->irq_on_late = false;
#endif /* CONFIG_COUNTER_RTC_STM32_SUBSECONDS */

		/* Release the channel before invoking a callback that may rearm it. */
		rtc_stm32_clear_callback(data);
		if (alarm_callback != NULL) {
			alarm_callback(dev, 0, (uint32_t)now, user_data);
#if defined(CONFIG_COUNTER_64BITS_TICKS)
		} else if (alarm_callback_64 != NULL) {
			alarm_callback_64(dev, 0, now, user_data);
#endif
		}
	}

#if defined(RTC_EXTI_LINE_NUM)
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	rtc_stm32_n6_clear_exti_pending();
#else
	stm32_exti_clear_pending(RTC_EXTI_LINE_NUM);
#endif
#endif /* defined(RTC_EXTI_LINE_NUM) */
}


static int rtc_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct rtc_stm32_config *cfg = dev->config;
	struct rtc_stm32_data *data = dev->data;
	int ret = -EIO;

	rtc_stm32_clear_callback(data);

	/* RTC gate bits may be protected by backup-domain write access. */
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
	stm32_backup_domain_enable_access();

	/* Enable the gate clocks; entry 1 selects the RTC source. */
	for (size_t i = 0; i < cfg->pclken_count; i++) {
		if (i == 1U) {
			continue;
		}

		if (clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[i]) != 0) {
			LOG_ERR("RTC clock enabling failed");
			goto out_unlock_hsem;
		}
	}

#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_HSE
	/* Must be configured before selecting the RTC clock source */
	LL_RCC_SetRTC_HSEPrescaler(cfg->hse_prescaler);
#endif

	/* Enable RTC clock source */
	if (clock_control_configure(clk,
				    (clock_control_subsys_t) &cfg->pclken[1],
				    NULL) != 0) {
		LOG_ERR("clock configure failed");
		goto out_unlock_hsem;
	}

#if !defined(CONFIG_SOC_SERIES_STM32WBAX)
	LL_RCC_EnableRTC();
#endif /* !CONFIG_SOC_SERIES_STM32WBAX */

	ret = 0;

out_unlock_hsem:
	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);
	if (ret < 0) {
		goto out_disable_bkup_access;
	}

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

#if defined(CONFIG_SOC_SERIES_STM32N6X)
	ret = rtc_stm32_wait_for_synchro();
	if (ret < 0) {
		LOG_ERR("Failed to synchronize RTC");
		goto out_disable_bkup_access;
	}
#endif

#ifdef RTC_CR_BYPSHAD
	LL_RTC_DisableWriteProtection(STM32_ARG(RTC));
	STM32_RTC_EnableBypassShadowReg(STM32_ARG(RTC));
	LL_RTC_EnableWriteProtection(STM32_ARG(RTC));
#endif /* RTC_CR_BYPSHAD */

#if defined(RTC_EXTI_LINE_NUM)
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	/* Route wakeup events through direct EXTI17 and the secure RTC IRQ. */
	LL_RTC_DisableWriteProtection(RTC);
	WRITE_REG(RTC->SECCFGR, RTC_SECCFGR_SEC | RTC_SECCFGR_INITSEC | RTC_SECCFGR_CALSEC |
				RTC_SECCFGR_TSSEC | RTC_SECCFGR_WUTSEC |
				RTC_SECCFGR_ALRASEC | RTC_SECCFGR_ALRBSEC);
	WRITE_REG(RTC->PRIVCFGR, 0U);
	LL_RTC_EnableWriteProtection(RTC);
	rtc_stm32_n6_configure_exti();
#else
	/* Trigger NVIC IRQ on RTC EXTI line rising edge */
	ret = stm32_exti_enable(RTC_EXTI_LINE_NUM,
				STM32_EXTI_TRIG_RISING,
				STM32_EXTI_MODE_IT);
	if (ret < 0) {
		LOG_ERR("Failed to enable RTC EXTI line");
		goto out_disable_bkup_access;
	}
#endif /* CONFIG_SOC_SERIES_STM32N6X */
#endif /* defined(RTC_EXTI_LINE_NUM) */

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
	.async_prescaler = DT_INST_PROP_OR(0, async_prescaler, RTC_HSE_ASYNC_PRESCALER - 1),
#if !defined(CONFIG_SOC_SERIES_STM32F1X)
	.sync_prescaler = DT_INST_PROP_OR(0, hse_prescaler, RTC_HSE_SYNC_PRESCALER - 1),
#endif /* !CONFIG_SOC_SERIES_STM32F1X */
#else
#error Invalid RTC SRC
#endif
	.pclken = rtc_clk,
	.pclken_count = ARRAY_SIZE(rtc_clk),
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
#if defined(CONFIG_COUNTER_64BITS_TICKS)
	.get_value_64 = rtc_stm32_get_value_64,
	.set_alarm_64 = rtc_stm32_set_alarm_64,
	.get_top_value_64 = rtc_stm32_get_top_value_64,
	.set_top_value_64 = rtc_stm32_set_top_value_64,
#endif
	.set_alarm = rtc_stm32_set_alarm,
	.cancel_alarm = rtc_stm32_cancel_alarm,
	.set_top_value = rtc_stm32_set_top_value,
	.get_pending_int = rtc_stm32_get_pending_int,
	.get_top_value = rtc_stm32_get_top_value,
#if defined(CONFIG_SOC_SERIES_STM32N6X)
	.get_guard_period = rtc_stm32_get_guard_period,
	.set_guard_period = rtc_stm32_set_guard_period,
#if defined(CONFIG_COUNTER_64BITS_TICKS)
	.get_guard_period_64 = rtc_stm32_get_guard_period_64,
	.set_guard_period_64 = rtc_stm32_set_guard_period_64,
#endif
#endif
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
