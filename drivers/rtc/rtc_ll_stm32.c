/*
 * Copyright (c) 2023 Prevas A/S
 * Copyright (c) 2023 Syslinbit
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

#include <zephyr/logging/log.h>

#include <stdbool.h>

LOG_MODULE_REGISTER(rtc_stm32, CONFIG_RTC_LOG_LEVEL);

#if defined(CONFIG_SOC_SERIES_STM32L1X) && !defined(RTC_SUBSECOND_SUPPORT)
/* subsecond counting is not supported by some STM32L1x MCUs */
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

struct rtc_stm32_config {
	uint32_t async_prescaler;
	uint32_t sync_prescaler;
	const struct stm32_pclken *pclken;
#if DT_INST_NODE_HAS_PROP(0, calib_out_freq)
	uint32_t cal_out_freq;
#endif
};

struct rtc_stm32_data {
	struct k_mutex lock;
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
#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
	LL_PWR_EnableBkUpAccess();
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPR_DBP */

	/* Enable RTC clock source */
	if (clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL) != 0) {
		LOG_ERR("clock configure failed\n");
		return -EIO;
	}

	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);

	LL_RCC_EnableRTC();

	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

	err = rtc_stm32_configure(dev);

#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
	LL_PWR_DisableBkUpAccess();
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPR_DBP */

	return err;
}

static int rtc_stm32_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_stm32_data *data = dev->data;

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

	LOG_INF("Setting clock");

#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
	LL_PWR_EnableBkUpAccess();
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPR_DBP */

	LL_RTC_DisableWriteProtection(RTC);

	ErrorStatus status = LL_RTC_EnterInitMode(RTC);

	if (status != SUCCESS) {
#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
		LL_PWR_DisableBkUpAccess();
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPR_DBP */
		k_mutex_unlock(&data->lock);
		return -EIO;
	}

	LL_RTC_DATE_SetYear(RTC, bin2bcd(real_year - RTC_YEAR_REF));
	LL_RTC_DATE_SetMonth(RTC, bin2bcd(timeptr->tm_mon + 1));
	LL_RTC_DATE_SetDay(RTC, bin2bcd(timeptr->tm_mday));

	if (timeptr->tm_wday == 0) {
		/* sunday (tm_wday = 0) is not represented by the same value in hardware */
		LL_RTC_DATE_SetWeekDay(RTC, LL_RTC_WEEKDAY_SUNDAY);
	} else {
		/* all the other values are consistent with what is expected by hardware */
		LL_RTC_DATE_SetWeekDay(RTC, timeptr->tm_wday);
	}


	LL_RTC_TIME_SetHour(RTC, bin2bcd(timeptr->tm_hour));
	LL_RTC_TIME_SetMinute(RTC, bin2bcd(timeptr->tm_min));
	LL_RTC_TIME_SetSecond(RTC, bin2bcd(timeptr->tm_sec));

	LL_RTC_DisableInitMode(RTC);

	LL_RTC_EnableWriteProtection(RTC);

#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
	LL_PWR_DisableBkUpAccess();
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPR_DBP */

	k_mutex_unlock(&data->lock);

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

	timeptr->tm_year = bcd2bin(__LL_RTC_GET_YEAR(rtc_date)) + RTC_YEAR_REF - TM_YEAR_REF;
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

	timeptr->tm_nsec = DIV_ROUND_CLOSEST(temp, cfg->sync_prescaler + 1);
#else
	timeptr->tm_nsec = 0;
#endif

	/* unknown values */
	timeptr->tm_yday  = -1;
	timeptr->tm_isdst = -1;

	return 0;
}

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

#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
	LL_PWR_EnableBkUpAccess();
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPR_DBP */

	LL_RTC_DisableWriteProtection(RTC);

	MODIFY_REG(RTC->CALR, RTC_CALR_CALP | RTC_CALR_CALM, calp | calm);

	LL_RTC_EnableWriteProtection(RTC);

#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
	LL_PWR_DisableBkUpAccess();
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPR_DBP */

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
	/* RTC_ALARM not supported */
	/* RTC_UPDATE not supported */
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

static const struct rtc_stm32_config rtc_config = {
#if DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_LSI
	/* prescaler values for LSI @ 32 KHz */
	.async_prescaler = 0x7F,
	.sync_prescaler = 0x00F9,
#else /* DT_INST_CLOCKS_CELL_BY_IDX(0, 1, bus) == STM32_SRC_LSE */
	/* prescaler values for LSE @ 32768 Hz */
	.async_prescaler = 0x7F,
	.sync_prescaler = 0x00FF,
#endif
	.pclken = rtc_clk,
#if DT_INST_NODE_HAS_PROP(0, calib_out_freq)
	.cal_out_freq = _CONCAT(_CONCAT(LL_RTC_CALIB_OUTPUT_, DT_INST_PROP(0, calib_out_freq)), HZ),
#endif
};

static struct rtc_stm32_data rtc_data;

DEVICE_DT_INST_DEFINE(0, &rtc_stm32_init, NULL, &rtc_data, &rtc_config, PRE_KERNEL_1,
		      CONFIG_RTC_INIT_PRIORITY, &rtc_stm32_driver_api);
