/*
 * Copyright (c) 2023 Prevas A/S
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
#include <soc.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_rcc.h>
#include <stm32_ll_rtc.h>
#include <stm32_hsem.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rtc_stm32, CONFIG_RTC_LOG_LEVEL);

/* Convert calendar start time */
/* RTC start time: 1st, Jan, 2000 */
/* struct tm start:   1st, Jan, 1900 */
#define TM_TO_RTC_OFFSET 100

struct rtc_stm32_config {
	LL_RTC_InitTypeDef ll_rtc_config;
	const struct stm32_pclken *pclken;
};

struct rtc_stm32_data {
	/* Currently empty */
};

static int rtc_stm32_init(const struct device *dev)
{
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);
	const struct rtc_stm32_config *cfg = dev->config;

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Enable RTC bus clock */
	if (clock_control_on(clk, (clock_control_subsys_t)&cfg->pclken[0]) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	/* Enable Backup access */
	z_stm32_hsem_lock(CFG_HW_RCC_SEMID, HSEM_LOCK_DEFAULT_RETRY);
#if defined(PWR_CR_DBP) || defined(PWR_CR1_DBP) || defined(PWR_DBPCR_DBP) || defined(PWR_DBPR_DBP)
	LL_PWR_EnableBkUpAccess();
#endif /* PWR_CR_DBP || PWR_CR1_DBP || PWR_DBPR_DBP */

	/* Enable RTC clock source */
	if (clock_control_configure(clk, (clock_control_subsys_t)&cfg->pclken[1], NULL) != 0) {
		LOG_ERR("clock configure failed\n");
		return -EIO;
	}

	LL_RCC_EnableRTC();

	z_stm32_hsem_unlock(CFG_HW_RCC_SEMID);

	if (LL_RTC_Init(RTC, ((LL_RTC_InitTypeDef *)&cfg->ll_rtc_config)) != SUCCESS) {
		return -EIO;
	}

#ifdef RTC_CR_BYPSHAD
	LL_RTC_DisableWriteProtection(RTC);
	LL_RTC_EnableShadowRegBypass(RTC);
	LL_RTC_EnableWriteProtection(RTC);
#endif /* RTC_CR_BYPSHAD */

	return 0;
}

static const struct stm32_pclken rtc_clk[] = STM32_DT_INST_CLOCKS(0);

static const struct rtc_stm32_config rtc_config = {
	.ll_rtc_config = {
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
		},
	.pclken = rtc_clk,
};

static int rtc_stm32_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	LOG_INF("Setting clock");
	LL_RTC_DisableWriteProtection(RTC);

	LL_RTC_EnableInitMode(RTC);

	while (!LL_RTC_IsActiveFlag_INIT(RTC)) {
	};

	LL_RTC_DATE_SetYear(RTC, __LL_RTC_CONVERT_BIN2BCD(timeptr->tm_year - TM_TO_RTC_OFFSET));
	LL_RTC_DATE_SetMonth(RTC, __LL_RTC_CONVERT_BIN2BCD(timeptr->tm_mon + 1));
	LL_RTC_DATE_SetDay(RTC, __LL_RTC_CONVERT_BIN2BCD(timeptr->tm_mday));

	LL_RTC_DATE_SetWeekDay(RTC, __LL_RTC_CONVERT_BIN2BCD(timeptr->tm_wday) + 1);

	LL_RTC_TIME_SetHour(RTC, __LL_RTC_CONVERT_BIN2BCD(timeptr->tm_hour));
	LL_RTC_TIME_SetMinute(RTC, __LL_RTC_CONVERT_BIN2BCD(timeptr->tm_min));
	LL_RTC_TIME_SetSecond(RTC, __LL_RTC_CONVERT_BIN2BCD(timeptr->tm_sec));

	LL_RTC_DisableInitMode(RTC);

	LL_RTC_EnableWriteProtection(RTC);

	return 0;
}

static int rtc_stm32_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	ARG_UNUSED(dev);

	uint32_t rtc_date, rtc_time;

	/* Read time and date registers */
	rtc_time = LL_RTC_TIME_Get(RTC);
	rtc_date = LL_RTC_DATE_Get(RTC);

	timeptr->tm_year = TM_TO_RTC_OFFSET + __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_YEAR(rtc_date));
	/* tm_mon allowed values are 0-11 */
	timeptr->tm_mon = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MONTH(rtc_date)) - 1;
	timeptr->tm_mday = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_DAY(rtc_date));
	timeptr->tm_wday = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_WEEKDAY(rtc_date)) - 1;

	timeptr->tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(rtc_time));
	timeptr->tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(rtc_time));
	timeptr->tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(rtc_time));

	timeptr->tm_nsec = LL_RTC_TIME_GetSubSecond(RTC);

	return 0;
}

struct rtc_driver_api rtc_stm32_driver_api = {
	.set_time = rtc_stm32_set_time,
	.get_time = rtc_stm32_get_time,
	/* RTC_ALARM not supported */
	/* RTC_UPDATE not supported */
};

#define RTC_STM32_DEV_CFG(n)                                                                       \
	static struct rtc_stm32_data rtc_data_##n = {};                                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &rtc_stm32_init, NULL, &rtc_data_##n, &rtc_config, POST_KERNEL,   \
			      CONFIG_RTC_INIT_PRIORITY, &rtc_stm32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_STM32_DEV_CFG);
