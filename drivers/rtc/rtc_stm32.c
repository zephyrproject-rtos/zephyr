/*
 * Copyright (c) 2020 Paratronic
 * SPDX-License-Identifier: Apache-2.0
 *
 * Source file for the STM32 RTC driver
 *
 */


#include <kernel.h>
#include <soc.h>
#include <drivers/clock_control/stm32_clock_control.h>
#include <drivers/rtc.h>
#include <sys/timeutil.h>

#define LOG_LEVEL CONFIG_RTC_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(rtc);

#if defined(CONFIG_RTC_STM32_CLOCK_LSI)
/* prescaler values for LSI @ 32 KHz */
#define RTC_PREDIV_ASYNC 31
#define RTC_PREDIV_SYNC  999
#else /* CONFIG_RTC_STM32_CLOCK_LSE */
/* prescaler values for LSE @ 32768 Hz */
#define RTC_PREDIV_ASYNC 127
#define RTC_PREDIV_SYNC  255
#endif

static int rtc_stm32_set_time(struct device *dev, const struct timespec *tp)
{
	int err = -EIO;
	struct tm tm;
	LL_RTC_TimeTypeDef timeDef;
	LL_RTC_DateTypeDef dateDef;
	const char *serr = NULL;
	int key;

	ARG_UNUSED(dev);

	key = irq_lock();

	LL_PWR_EnableBkUpAccess();

	gmtime_r(&tp->tv_sec, &tm);
	timeDef.Hours = tm.tm_hour;
	timeDef.Minutes = tm.tm_min;
	timeDef.Seconds = tm.tm_sec;
	if (LL_RTC_TIME_Init(RTC, LL_RTC_FORMAT_BIN, &timeDef) != SUCCESS) {
		serr = "failed to set time";
		goto out;
	}

	dateDef.WeekDay = ((tm.tm_wday + 6) % 7) + 1;
	dateDef.Month = tm.tm_mon + 1;
	dateDef.Day = tm.tm_mday;
	dateDef.Year = (tm.tm_year + 1900) - 2000;
	if (LL_RTC_DATE_Init(RTC, LL_RTC_FORMAT_BIN, &dateDef) != SUCCESS) {
		serr = "failed to set date";
		goto out;
	}

	err = 0;
out:
	LL_PWR_DisableBkUpAccess();

	irq_unlock(key);

	if (serr) {
		LOG_ERR("%s", serr);
	}

	return err;
}

static int rtc_stm32_get_time(struct device *dev, struct timespec *tp)
{
	struct tm tm;
	u32_t time;
	u32_t subSeconds;
	u32_t date;
	int key;

	ARG_UNUSED(dev);

	key = irq_lock();
	subSeconds = LL_RTC_TIME_GetSubSecond(RTC);
	time = LL_RTC_TIME_Get(RTC);
	date = LL_RTC_DATE_Get(RTC);
	irq_unlock(key);

	tm.tm_sec = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_SECOND(time));
	tm.tm_min = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MINUTE(time));
	tm.tm_hour = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_HOUR(time));

	tm.tm_mday = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_DAY(date));
	tm.tm_mon = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_MONTH(date)) - 1;
	tm.tm_year = __LL_RTC_CONVERT_BCD2BIN(__LL_RTC_GET_YEAR(date)) + 100;
	tp->tv_sec = timeutil_timegm(&tm);
	tp->tv_nsec = ((u64_t)(RTC_PREDIV_SYNC - subSeconds) * NSEC_PER_SEC) /
		(RTC_PREDIV_SYNC + 1);

	return 0;
}

static int rtc_stm32_init(struct device *dev)
{
	int err = -EIO;
	struct device *clk;
	struct stm32_pclken pclken;
	LL_RTC_InitTypeDef init;

	clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);
	__ASSERT_NO_MSG(clk);

	pclken.enr = DT_INST_0_ST_STM32_RTC_CLOCK_BITS;
	pclken.bus = DT_INST_0_ST_STM32_RTC_CLOCK_BUS;
	clock_control_on(clk, (clock_control_subsys_t *) &pclken);

	LL_PWR_EnableBkUpAccess();

#if defined(CONFIG_RTC_STM32_CLOCK_LSI)
	LL_RCC_LSI_Enable();

	/* Wait until LSI is ready */
	while (LL_RCC_LSI_IsReady() != 1) {
	}

	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSI);

#else /* CONFIG_RTC_STM32_CLOCK_LSE */
	LL_RCC_LSE_Enable();

	/* Wait until LSE is ready */
	while (LL_RCC_LSE_IsReady() != 1) {
	}

	LL_RCC_SetRTCClockSource(LL_RCC_RTC_CLKSOURCE_LSE);

#endif

	LL_RCC_EnableRTC();

	init.HourFormat = RTC_HOURFORMAT_24;
	init.AsynchPrescaler = RTC_PREDIV_ASYNC;
	init.SynchPrescaler = RTC_PREDIV_SYNC;
	if (LL_RTC_Init(RTC, &init) != SUCCESS) {
		LOG_ERR("failed to init");
		goto out;
	}
	/* check if calendar has been initialized */
	if (!LL_RTC_IsActiveFlag_INITS(RTC)) {
		LOG_INF("Datetime initialization");
		struct timespec tp;
		/* Init date time to 1 January 2001 00:00:00 */
		tp.tv_sec = 978307200;
		tp.tv_nsec = 0;
		err = rtc_stm32_set_time(dev, &tp);
		if (err < 0) {
			goto out;
		}
	}

	err = 0;
out:
	LL_PWR_DisableBkUpAccess();
	if (err == 0) {
		LOG_DBG("RTC initialised correctly");
	}
	return err;
}

static const struct rtc_driver_api rtc_stm32_driver_api = {
	.get_time = rtc_stm32_get_time,
	.set_time = rtc_stm32_set_time,
};

DEVICE_AND_API_INIT(rtc_stm32, DT_RTC_0_NAME, &rtc_stm32_init,
		    NULL, NULL, PRE_KERNEL_1,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &rtc_stm32_driver_api);
