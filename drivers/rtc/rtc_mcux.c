/*
 * Copyright (c) 2017 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SYS_LOG_DOMAIN "dev/rtc_mcux"
#define SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#include <logging/sys_log.h>

#include <device.h>
#include <rtc.h>
#include <fsl_rtc.h>

#define DEV_CFG(dev) (dev->config->config_info)
#define DEV_DATA(dev) (dev->driver_data)

#define SECONDS_IN_A_DAY (86400U)
#define SECONDS_IN_A_HOUR (3600U)
#define SECONDS_IN_A_MINUTE (60U)
#define DAYS_IN_A_YEAR (365U)
#define YEAR_RANGE_START (1970U)
#define YEAR_RANGE_END (2099U)

#define TICKS_PER_SEC 1

struct mcux_rtc_config {
	RTC_Type     *base;
	clock_name_t clock_source;
	u32_t        clock_frequency;
	void         (*irq_config_func)(void);
};

struct mcux_rtc_data {
	void (*alarm_callback)(struct device *dev);
};

static void dump_regs(RTC_Type *base)
{
	SYS_LOG_DBG("RTC_TSR: %lx", base->TSR);
	SYS_LOG_DBG("RTC_TPR: %lx", base->TPR);
	SYS_LOG_DBG("RTC_TAR: %lx", base->TAR);
	SYS_LOG_DBG("RTC_TCR: %lx", base->TCR);
	SYS_LOG_DBG("RTC_CR:  %lx", base->CR);
	SYS_LOG_DBG("RTC_SR:  %lx", base->SR);
	SYS_LOG_DBG("RTC_LR:  %lx", base->LR);
	SYS_LOG_DBG("RTC_IER: %lx", base->IER);
	SYS_LOG_DBG("RTC_WAR: %lx", base->WAR);
	SYS_LOG_DBG("RTC_RAR: %lx", base->RAR);
}

/*
 * This function is a clone of RTC_ConvertDatetimeToSeconds() in fsl_rtc.c
 * RTC_ConvertDatetimeToSeconds() is private. make a copy here to prevent
 * modifying mcux sdk. It will be easier to upgrade mcux sdk to upstream.
 */
static uint32_t convert_datetime_to_seconds(const rtc_datetime_t *datetime)
{
	assert(datetime);

	/* Number of days from begin of the non Leap-year*/
	/* Number of days from begin of the non Leap-year*/
	u16_t monthDays[] = {0U, 0U, 31U, 59U, 90U, 120U, 151U, 181U, 212U,
			     243U, 273U, 304U, 334U};
	u32_t seconds;

	/* Compute number of days from 1970 till given year*/
	seconds = (datetime->year - 1970U) * DAYS_IN_A_YEAR;
	/* Add leap year days */
	seconds += ((datetime->year / 4) - (1970U / 4));
	/* Add number of days till given month*/
	seconds += monthDays[datetime->month];
	/* Add days in given month. We subtract the current day as it is
	 * represented in the hours, minutes and seconds field*/
	seconds += (datetime->day - 1);
	/* For leap year if month less than or equal to Febraury
	 * decrement day counter
	 */
	if ((!(datetime->year & 3U)) && (datetime->month <= 2U)) {
		seconds--;
	}

	seconds = (seconds * SECONDS_IN_A_DAY) +
		  (datetime->hour * SECONDS_IN_A_HOUR) +
		  (datetime->minute * SECONDS_IN_A_MINUTE) + datetime->second;

	return seconds;
}

/*
 * This function is a clone of RTC_ConvertSecondsToDatetime() in fsl_rtc.c
 * RTC_ConvertSecondsToDatetime() is private. make a copy here to prevent
 * modifying mcux sdk. It will be easier to upgrade mcux sdk to upstream.
 */
static void convert_seconds_to_datetime(u32_t seconds, rtc_datetime_t *datetime)
{
	assert(datetime);

	u32_t x;
	u32_t secondsRemaining, days;
	u16_t daysInYear;
	/* Table of days in a month for a non leap year. First entry in the
	 * table is not used, valid months start from 1
	 */
	u8_t daysPerMonth[] = {0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U,
			       30U, 31U, 30U, 31U};

	/* Start with the seconds value that is passed in to be converted to
	 * date time format
	 */
	secondsRemaining = seconds;

	/* Calcuate the number of days, we add 1 for the current day which is
	 * represented in the hours and seconds field
	 */
	days = secondsRemaining / SECONDS_IN_A_DAY + 1;

	/* Update seconds left*/
	secondsRemaining = secondsRemaining % SECONDS_IN_A_DAY;

	/* Calculate the datetime hour, minute and second fields */
	datetime->hour = secondsRemaining / SECONDS_IN_A_HOUR;
	secondsRemaining = secondsRemaining % SECONDS_IN_A_HOUR;
	datetime->minute = secondsRemaining / 60U;
	datetime->second = secondsRemaining % SECONDS_IN_A_MINUTE;

	/* Calculate year */
	daysInYear = DAYS_IN_A_YEAR;
	datetime->year = YEAR_RANGE_START;
	while (days > daysInYear) {
		/* Decrease day count by a year and increment year by 1 */
		days -= daysInYear;
		datetime->year++;

		/* Adjust the number of days for a leap year */
		if (datetime->year & 3U) {
			daysInYear = DAYS_IN_A_YEAR;
		} else {
			daysInYear = DAYS_IN_A_YEAR + 1;
		}
	}

	/* Adjust the days in February for a leap year */
	if (!(datetime->year & 3U)) {
		daysPerMonth[2] = 29U;
	}

	for (x = 1U; x <= 12U; x++) {
		if (days <= daysPerMonth[x]) {
			datetime->month = x;
			break;
		} else {
			days -= daysPerMonth[x];
		}
	}

	datetime->day = days;
}

static void rtc_mcux_isr(void *arg)
{
	struct device *dev = (struct device *)arg;

	if (dev) {
		const struct mcux_rtc_config *dev_config = DEV_CFG(dev);
		struct mcux_rtc_data *dev_data = DEV_DATA(dev);

		/* Clear interrupt flag */
		if (dev_config) {
			dev_config->base->TAR = dev_config->base->TAR;
		}

		/* Notify user callback */
		if (dev_data && dev_data->alarm_callback) {
			dev_data->alarm_callback(dev);
		}
	}
}

static void enable_rtc_osc(struct device *dev)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);

	/* Enable RTC software access */
	CLOCK_EnableClock(kCLOCK_Rtc0);

	if ((dev_config->base->CR & RTC_CR_OSCE_MASK) == 0) {
		/* Enable the RTC 32KHz oscillator */
		dev_config->base->CR |= RTC_CR_OSCE_MASK;
	}

	/* Set the XTAL32/RTC_CLKIN frequency based on board setting. */
	CLOCK_SetXtal32Freq(dev_config->clock_frequency);
	/* Set RTC_TSR if there is fault value in RTC */
	if (dev_config->base->SR & RTC_SR_TIF_MASK) {
		dev_config->base->TSR = dev_config->base->TSR;
	}

	/* Disable RTC software access */
	CLOCK_DisableClock(kCLOCK_Rtc0);
}

static int rtc_mcux_init(struct device *dev)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);
	rtc_config_t config;

	enable_rtc_osc(dev);

	RTC_GetDefaultConfig(&config);
	config.wakeupSelect = true;
	config.updateMode = true;
	config.supervisorAccess = true;
	RTC_Init(dev_config->base, &config);
	RTC_StartTimer(dev_config->base);

	dev_config->irq_config_func();

	return 0;
}

static void rtc_mcux_enable(struct device *dev)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);

	RTC_StartTimer(dev_config->base);
}

static void rtc_mcux_disable(struct device *dev)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);

	RTC_StopTimer(dev_config->base);
}

static u32_t rtc_mcux_read(struct device *dev)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);
	rtc_datetime_t datetime;

	dump_regs(dev_config->base);

	RTC_GetDatetime(dev_config->base, &datetime);

	return convert_datetime_to_seconds(&datetime);
}

static int rtc_mcux_set_time(struct device *dev, const u32_t ticks)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);
	rtc_datetime_t datetime;
	status_t status;
	u32_t saved_sr;
	u32_t seconds;
	int rv = 0;

	/*
	 * TSR is read-only if time counter is running. stop time counter before
	 * updating TSR
	 */
	saved_sr = dev_config->base->SR;
	if (saved_sr & RTC_SR_TCE_MASK) {
		rtc_mcux_disable(dev);
	}

	seconds = ticks / TICKS_PER_SEC;
	convert_seconds_to_datetime(seconds, &datetime);
	status = RTC_SetDatetime(dev_config->base, &datetime);
	if (status != kStatus_Success) {
		rv = -EIO;
	}

	/* Restart timer counter if necessary */
	if (saved_sr & RTC_SR_TCE_MASK) {
		rtc_mcux_enable(dev);
	}

	return rv;
}

static int rtc_mcux_set_alarm(struct device *dev, const u32_t alarm_val)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);
	rtc_datetime_t datetime;
	status_t status;

	RTC_EnableInterrupts(dev_config->base, kRTC_AlarmInterruptEnable);
	convert_seconds_to_datetime(alarm_val, &datetime);
	status = RTC_SetAlarm(dev_config->base, &datetime);
	if (status != kStatus_Success) {
		return -EIO;
	}

	return 0;
}

static int rtc_mcux_set_config(struct device *dev, struct rtc_config *config)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);
	struct mcux_rtc_data *dev_data = DEV_DATA(dev);
	int rv = 0;

	if (!config || !dev_config || !dev_data) {
		return -EFAULT;
	}

	/* init value */
	rv = rtc_mcux_set_time(dev, config->init_val);
	if (rv < 0) {
		return rv;
	}

	/* alarm */
	dev_data->alarm_callback = config->cb_fn;
	if (config->alarm_enable) {
		rv = rtc_mcux_set_alarm(dev, config->alarm_val);
		if (rv < 0) {
			return rv;
		}
	} else {
		RTC_DisableInterrupts(dev_config->base, kRTC_AlarmInterruptEnable);
	}

	return 0;
}

static u32_t rtc_mcux_get_pending_int(struct device *dev)
{
	const struct mcux_rtc_config *dev_config = DEV_CFG(dev);
	u32_t flags;

	flags = RTC_GetStatusFlags(dev_config->base);
	if (flags & RTC_SR_TAF_MASK) {
		return 1;
	} else {
		return 0;
	}
}

static u32_t rtc_mcux_get_ticks_per_sec(struct device *dev)
{
	return TICKS_PER_SEC;
}

static void rtc_mcux_irq_config_func_0(void);

static const struct mcux_rtc_config mcux_rtc_config_0 = {
	.base               = (RTC_Type *)CONFIG_RTC_0_BASE_ADDRESS,
	.clock_source       = kCLOCK_Er32kClk,
	.clock_frequency    = CONFIG_RTC_0_CLOCK_FREQUENCY,
	.irq_config_func    = rtc_mcux_irq_config_func_0,
};

static struct mcux_rtc_data mcux_rtc_data_0 = {
	.alarm_callback = NULL,
};

static const struct rtc_driver_api api = {
	.enable = rtc_mcux_enable,
	.disable = rtc_mcux_disable,
	.read = rtc_mcux_read,
	.set_config = rtc_mcux_set_config,
	.set_time = rtc_mcux_set_time,
	.set_alarm = rtc_mcux_set_alarm,
	.get_pending_int = rtc_mcux_get_pending_int,
	.get_ticks_per_sec = rtc_mcux_get_ticks_per_sec,
};

DEVICE_AND_API_INIT(rtc_mcux_0, CONFIG_RTC_0_NAME, &rtc_mcux_init,
		    &mcux_rtc_data_0, &mcux_rtc_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &api);

static void rtc_mcux_irq_config_func_0(void)
{
	IRQ_CONNECT(CONFIG_RTC_0_IRQ_ALARM, CONFIG_RTC_0_IRQ_ALARM_PRI,
		    rtc_mcux_isr, DEVICE_GET(rtc_mcux_0), 0);
	irq_enable(CONFIG_RTC_0_IRQ_ALARM);
}
