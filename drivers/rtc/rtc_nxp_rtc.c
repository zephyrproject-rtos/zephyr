/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rtc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/irq.h>
#include <fsl_rtc.h>
#include "rtc_utils.h"

struct nxp_rtc_config {
	RTC_Type *base;
	void (*irq_config_func)(const struct device *dev);
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clock_output)
	bool is_output_clock_enabled;
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(time_seconds_frequency)
	uint8_t time_seconds_frequency;
#endif
	bool is_lpo_clock_source;
	bool is_wakeup_enabled;
	bool is_update_mode;
	bool is_supervisor_access;
	uint8_t compensation_interval;
	uint8_t compensation_time;
};

struct nxp_rtc_data {
	bool is_dst_enabled;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	uint16_t alarm_mask;
	bool alarm_pending;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback;
	void *update_user_data;
#endif /* CONFIG_RTC_UPDATE */
};

#define SECONDS_IN_A_DAY    (86400U)
#define SECONDS_IN_A_HOUR   (3600U)
#define SECONDS_IN_A_MINUTE (60U)
#define DAYS_IN_A_YEAR      (365U)
#define YEAR_RANGE_START    (1970U)
#define YEAR_RANGE_END      (2099U)

static uint32_t nxp_rtc_convert_datetime_to_seconds(const struct rtc_time *timeptr)
{
	/* Number of days from begin of the non Leap-year*/
	uint16_t monthDays[] = {0U,   0U,   31U,  59U,  90U,  120U, 151U,
				181U, 212U, 243U, 273U, 304U, 334U};
	uint32_t seconds;
	uint16_t year = timeptr->tm_year + 1900;

	/* Compute number of days from 1970 till given year*/
	seconds = ((uint32_t)year - YEAR_RANGE_START) * DAYS_IN_A_YEAR;
	/* Add leap year days */
	seconds += (((uint32_t)year / 4U) - (YEAR_RANGE_START / 4U));
	/* Add number of days till given month*/
	seconds += monthDays[timeptr->tm_mon + 1];
	/*
	 * Add days in given month. We subtract the current day as it is
	 * represented in the hours, minutes and seconds field
	 */
	seconds += ((uint32_t)timeptr->tm_mday - 1U);
	/* For leap year if month less than or equal to Febraury, decrement day counter*/
	if ((0U == (year & 3U)) && ((timeptr->tm_mon + 1) <= 2U) && 0U != seconds) {
		seconds--;
	}

	assert(seconds < UINT32_MAX / SECONDS_IN_A_DAY);
	seconds = (seconds * SECONDS_IN_A_DAY) + ((uint32_t)timeptr->tm_hour * SECONDS_IN_A_HOUR) +
		  ((uint32_t)timeptr->tm_min * SECONDS_IN_A_MINUTE) + timeptr->tm_sec;

	return seconds;
}

static void nxp_rtc_convert_seconds_to_datetime(uint32_t seconds, struct rtc_time *timeptr)
{
	uint32_t secondsRemaining, days;
	uint16_t daysInYear;
	/*
	 * Table of days in a month for a non leap year. First entry in the table is not used,
	 * valid months start from 1
	 */
	uint8_t daysPerMonth[] = {0U, 31U, 28U, 31U, 30U, 31U, 30U, 31U, 31U, 30U, 31U, 30U, 31U};

	/* Start with the seconds value that is passed in to be converted to date time format */
	secondsRemaining = seconds;

	/*
	 * Calculate the number of days, we add 1 for the current day which is represented in the
	 * hours and seconds field
	 */
	days = secondsRemaining / SECONDS_IN_A_DAY + 1U;

	/* Update seconds left*/
	secondsRemaining = secondsRemaining % SECONDS_IN_A_DAY;

	/* Calculate the timeptr hour, minute and second fields */
	timeptr->tm_hour = (uint8_t)(secondsRemaining / SECONDS_IN_A_HOUR);
	secondsRemaining = secondsRemaining % SECONDS_IN_A_HOUR;
	timeptr->tm_min = (uint8_t)(secondsRemaining / SECONDS_IN_A_MINUTE);
	timeptr->tm_sec = (uint8_t)(secondsRemaining % SECONDS_IN_A_MINUTE);

	/* Calculate year */
	daysInYear = DAYS_IN_A_YEAR;
	timeptr->tm_year = YEAR_RANGE_START;
	while (days > daysInYear) {
		/* Decrease day count by a year and increment year by 1 */
		days -= daysInYear;
		timeptr->tm_year++;

		/* Adjust the number of days for a leap year */
		if (0U != (timeptr->tm_year & 3U)) {
			daysInYear = DAYS_IN_A_YEAR;
		} else {
			daysInYear = DAYS_IN_A_YEAR + 1U;
		}
	}

	/* Adjust the days in February for a leap year */
	if (0U == (timeptr->tm_year & 3U)) {
		daysPerMonth[2] = 29U;
	}

	timeptr->tm_year -= 1900;

	for (uint32_t x = 1U; x <= 12U; x++) {
		if (days <= daysPerMonth[x]) {
			timeptr->tm_mon = (uint8_t)x - 1;
			break;
		}
		days -= daysPerMonth[x];
	}

	timeptr->tm_mday = (uint8_t)days;
	/* Day of the week is not supported */
	timeptr->tm_wday = -1;
	/* Sub-second resolution is not supported */
	timeptr->tm_nsec = 0;
	/* Daylight saving time is not supported */
	timeptr->tm_isdst = -1;
}

static int nxp_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	if (!timeptr || !rtc_utils_validate_rtc_time(timeptr, 0)) {
		return -EINVAL;
	}

	const struct nxp_rtc_config *config = dev->config;
	RTC_Type *rtc_reg = config->base;

	RTC_StopTimer(rtc_reg);
	rtc_reg->TSR = nxp_rtc_convert_datetime_to_seconds(timeptr);
	RTC_StartTimer(rtc_reg);

	return 0;
}

static int nxp_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	if (!timeptr) {
		return -EINVAL;
	}

	const struct nxp_rtc_config *config = dev->config;
	RTC_Type *rtc_reg = config->base;
	uint32_t seconds = rtc_reg->TSR;

	memset(timeptr, 0, sizeof(struct rtc_time));
	nxp_rtc_convert_seconds_to_datetime(seconds, timeptr);

	return 0;
}

#if defined(CONFIG_RTC_ALARM)
static int nxp_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		return -EINVAL;
	}

	*mask = (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
		 RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY |
		 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR);

	return 0;
}

static int nxp_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				  const struct rtc_time *timeptr)
{
	if (id != 0 || (mask && (timeptr == 0)) ||
	    (timeptr && !rtc_utils_validate_rtc_time(timeptr, mask))) {
		return -EINVAL;
	}

	const struct nxp_rtc_config *config = dev->config;
	struct nxp_rtc_data *data = dev->data;
	RTC_Type *rtc_reg = config->base;
	uint32_t alarmSeconds = 0;
	uint32_t currSeconds = 0;
#if defined(FSL_FEATURE_RTC_HAS_ERRATA_010716) && (FSL_FEATURE_RTC_HAS_ERRATA_010716)
	bool restartCounter = false;
#endif /* FSL_FEATURE_RTC_HAS_ERRATA_010716 */

	data->alarm_pending = false;

	uint32_t key = irq_lock();

	alarmSeconds = nxp_rtc_convert_datetime_to_seconds(timeptr);
	/* Get the current time */
	currSeconds = rtc_reg->TSR;

	/* Return error if the alarm time has passed */
	if (alarmSeconds < currSeconds) {
		return -ETIME;
	}

#if defined(FSL_FEATURE_RTC_HAS_ERRATA_010716) && (FSL_FEATURE_RTC_HAS_ERRATA_010716)
	/* Save current TCE state */
	restartCounter = ((rtc_reg->SR & RTC_SR_TCE_MASK) != 0U);
	/* Disable time counter */
	rtc_reg->SR &= ~RTC_SR_TCE_MASK;
#endif /* FSL_FEATURE_RTC_HAS_ERRATA_010716 */

	/* Set alarm in seconds*/
	rtc_reg->TAR = alarmSeconds;

#if defined(FSL_FEATURE_RTC_HAS_ERRATA_010716) && (FSL_FEATURE_RTC_HAS_ERRATA_010716)
	/* Restore TCE if it was enabled */
	if (restartCounter) {
		rtc_reg->SR |= RTC_SR_TCE_MASK;
	}
#endif /* FSL_FEATURE_RTC_HAS_ERRATA_010716 */

	/* Enabling Alarm Interrupts */
	RTC_EnableInterrupts(rtc_reg, kRTC_AlarmInterruptEnable);
	data->alarm_mask = mask;

	irq_unlock(key);

	return 0;
}

static int nxp_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				  struct rtc_time *timeptr)
{
	if (id != 0 || !timeptr) {
		return -EINVAL;
	}

	const struct nxp_rtc_config *config = dev->config;
	struct nxp_rtc_data *data = dev->data;
	RTC_Type *rtc_reg = config->base;
	uint32_t alarmSeconds = rtc_reg->TAR;

	memset(timeptr, 0, sizeof(struct rtc_time));
	nxp_rtc_convert_seconds_to_datetime(alarmSeconds, timeptr);

	*mask = data->alarm_mask;

	return 0;
}

static int nxp_rtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	if (id != 0) {
		return -EINVAL;
	}

	struct nxp_rtc_data *data = dev->data;
	int ret = 0;

	__disable_irq();
	ret = data->alarm_pending ? 1 : 0;
	data->alarm_pending = false;
	__enable_irq();

	return ret;
}

static int nxp_rtc_alarm_set_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
	if (id != 0) {
		return -EINVAL;
	}

	struct nxp_rtc_data *data = dev->data;
	uint32_t key = irq_lock();

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;

	irq_unlock(key);

	return 0;
}

#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE)

static int nxp_rtc_update_set_callback(const struct device *dev, rtc_update_callback callback,
				       void *user_data)
{
	struct nxp_rtc_data *data = dev->data;
	uint32_t key = irq_lock();

	data->update_callback = callback;
	data->update_user_data = user_data;

	irq_unlock(key);

	return 0;
}

#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION)

static int nxp_rtc_calc_tcr(int32_t calibration, uint64_t cycles, int min, int max, uint32_t *cir,
			    int32_t *tcr)
{
	int64_t tcr_calc;

	if (calibration >= 0) {
		tcr_calc = (calibration * cycles + 500000000LL) / 1000000000LL;
	} else {
		tcr_calc = (calibration * cycles - 500000000LL) / 1000000000LL;
	}
	if (tcr_calc >= min && tcr_calc <= max) {
		*tcr = (int32_t)tcr_calc;
		*cir = 0;
		return 0;
	}
	for (int interval = 2; interval <= 256; interval++) {
		int64_t adjusted_tcr;

		if (calibration >= 0) {
			adjusted_tcr =
				(calibration * cycles * interval + 500000000LL) / 1000000000LL;
		} else {
			adjusted_tcr =
				(calibration * cycles * interval - 500000000LL) / 1000000000LL;
		}
		if (adjusted_tcr >= min && adjusted_tcr <= max) {
			*tcr = (int32_t)adjusted_tcr;
			*cir = interval - 1;
			return 0;
		}
	}
	return -EINVAL;
}

static int nxp_rtc_set_calibration(const struct device *dev, int32_t calibration)
{
	const struct nxp_rtc_config *config = dev->config;
	RTC_Type *rtc_reg = config->base;
	bool is_run_under_lpo = false;
	int32_t tcr_value;
	uint32_t cir_value;
	uint32_t tcr_reg_value;

#if (defined(FSL_FEATURE_RTC_HAS_LPO_ADJUST) && FSL_FEATURE_RTC_HAS_LPO_ADJUST)
	if (0U != (rtc_reg->CR & RTC_CR_LPOS_MASK)) {
		is_run_under_lpo = true;
	}
#endif

	int ret;

	if (is_run_under_lpo) {
		ret = nxp_rtc_calc_tcr(calibration, 1024LL, -4, 3, &cir_value, &tcr_value);
		if (ret) {
			return ret;
		}
		/* Only TCR[7:5] used, shift left by 5 */
		tcr_reg_value = (tcr_value >= 0) ? ((tcr_value & 0x07) << 5)
						 : (((8 + tcr_value) & 0x07) << 5);
		rtc_reg->TCR = (cir_value << 8) | tcr_reg_value;
	} else {
		ret = nxp_rtc_calc_tcr(calibration, 32768LL, -128, 127, &cir_value, &tcr_value);
		if (ret) {
			return ret;
		}
		tcr_reg_value = (tcr_value >= 0) ? (tcr_value & 0xFF) : ((256 + tcr_value) & 0xFF);
		rtc_reg->TCR = (cir_value << 8) | tcr_reg_value;
	}
	return 0;
}

static int nxp_rtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	if (!calibration) {
		return -EINVAL;
	}

	const struct nxp_rtc_config *config = dev->config;
	RTC_Type *rtc_reg = config->base;
	bool is_run_under_lpo = false;
	uint32_t tcr_register;
	uint8_t tcr_field, cir_field;
	int32_t tcr_value;
	uint32_t interval;

#if (defined(FSL_FEATURE_RTC_HAS_LPO_ADJUST) && FSL_FEATURE_RTC_HAS_LPO_ADJUST)
	if (0U != (rtc_reg->CR & RTC_CR_LPOS_MASK)) {
		is_run_under_lpo = true;
	}
#endif

	/* Read the Time Compensation Register */
	tcr_register = rtc_reg->TCR;

	/* Extract CIR field (bits 15:8) and TCR field (bits 7:0) */
	cir_field = (tcr_register >> 8) & 0xFF;
	tcr_field = tcr_register & 0xFF;

	/* Convert CIR to actual interval (CIR + 1) */
	interval = cir_field + 1;

	if (is_run_under_lpo) {
		/* LPO mode: only TCR[7:5] are used, TCR[4:0] are ignored */
		/* Extract TCR[7:5] and convert from 3-bit two's complement */
		uint8_t lpo_tcr_field = (tcr_field >> 5) & 0x07;

		if (lpo_tcr_field & 0x04) {
			/* Negative value - 3-bit two's complement */
			tcr_value = (int32_t)(lpo_tcr_field | 0xFFFFFFF8U);
		} else {
			/* Positive value */
			tcr_value = (int32_t)lpo_tcr_field;
		}

		/* Convert TCR value back to parts per billion for LPO mode */
		if (tcr_value == 0) {
			*calibration = 0;
		} else {
			/*
			 * Reverse the LPO formula: calibration = (tcr_value * 1000000000) / (1024 *
			 * interval) Use 64-bit arithmetic to avoid overflow
			 */
			int64_t cal_calc =
				((int64_t)tcr_value * 1000000000LL) / (1024LL * interval);

			/* Check if result fits in int32_t */
			if (cal_calc > INT32_MAX || cal_calc < INT32_MIN) {
				return -EINVAL;
			}

			*calibration = (int32_t)cal_calc;
		}
	} else {
		/* Convert TCR field from 8-bit two's complement to signed integer */
		if (tcr_field & 0x80) {
			/* Negative value - sign extend */
			tcr_value = (int32_t)(tcr_field | 0xFFFFFF00U);
		} else {
			/* Positive value */
			tcr_value = (int32_t)tcr_field;
		}

		/* Convert TCR value back to parts per billion */
		if (tcr_value == 0) {
			*calibration = 0;
		} else {
			/*
			 * Reverse the formula: calibration = (tcr_value * 1000000000) / (32768 *
			 * interval) Use 64-bit arithmetic to avoid overflow
			 */
			int64_t cal_calc =
				((int64_t)tcr_value * 1000000000LL) / (32768LL * interval);

			/* Check if result fits in int32_t */
			if (cal_calc > INT32_MAX || cal_calc < INT32_MIN) {
				return -EINVAL;
			}

			*calibration = (int32_t)cal_calc;
		}
	}

	return 0;
}

#endif /* CONFIG_RTC_CALIBRATION */

static int nxp_rtc_init(const struct device *dev)
{
	const struct nxp_rtc_config *config = dev->config;
	RTC_Type *rtc_reg = config->base;
#if (defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE))
	struct nxp_rtc_data *data = dev->data;
#endif
	rtc_config_t rtc_config = {
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clock_output)
		.clockOutput = config->is_output_clock_enabled,
#endif
		.wakeupSelect = config->is_wakeup_enabled,
		.updateMode = config->is_update_mode,
		.supervisorAccess = config->is_supervisor_access,
		.compensationInterval = config->compensation_interval - 1,
		.compensationTime = config->compensation_time,
	};

	RTC_Init(rtc_reg, &rtc_config);

	if (config->is_lpo_clock_source) {
#if (defined(FSL_FEATURE_RTC_HAS_LPO_ADJUST) && FSL_FEATURE_RTC_HAS_LPO_ADJUST)
		RTC_EnableLPOClock(rtc_reg, true);
#else
		return -ENOTSUP;
#endif
	}

#ifdef CONFIG_RTC_ALARM
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_mask = 0;
	data->alarm_pending = false;
#endif

#ifdef CONFIG_RTC_UPDATE
	RTC_EnableInterrupts(rtc_reg, kRTC_SecondsInterruptEnable);
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(time_seconds_frequency)
#if defined(FSL_FEATURE_RTC_HAS_TSIC) && FSL_FEATURE_RTC_HAS_TSIC
	RTC_SetTimerSecondsInterruptFrequency(rtc_reg, config->time_seconds_frequency);
#else
	return -ENOTSUP;
#endif
#endif
	data->update_callback = NULL;
	data->update_user_data = NULL;
#endif

	config->irq_config_func(dev);

	return 0;
}

/* Combined ISR RTC alarm and RTC second interrupt */
#if DT_INST_IRQ_HAS_IDX(0, 0) && !DT_INST_IRQ_HAS_IDX(0, 1)
static void nxp_rtc_isr(const struct device *dev)
{
#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
	const struct nxp_rtc_config *config = dev->config;
	RTC_Type *rtc_reg = config->base;
	struct nxp_rtc_data *data = dev->data;
	uint32_t key = irq_lock();
	uint32_t status_flags = RTC_GetStatusFlags(rtc_reg);

#ifdef CONFIG_RTC_ALARM
	/* Check for alarm interrupt */
	if (status_flags & kRTC_AlarmFlag) {
		RTC_ClearStatusFlags(rtc_reg, kRTC_AlarmFlag);
		if (data->alarm_callback) {
			data->alarm_callback(dev, 0, data->alarm_user_data);
			data->alarm_pending = false;
		} else {
			data->alarm_pending = true;
		}
	}
#endif

#ifdef CONFIG_RTC_UPDATE
	/*
	 * There is no status flag for RTC second interrupt, if the status flags are 0, then it is
	 * time second interrupt
	 */
	if (0U == status_flags) {
		if (data->update_callback) {
			data->update_callback(dev, data->update_user_data);
		}
	}
#endif

	irq_unlock(key);
#endif
}
#endif

/* Separate ISRs RTC alarm and RTC second interrupts */
#if DT_INST_IRQ_HAS_IDX(0, 0) && DT_INST_IRQ_HAS_IDX(0, 1)
static void nxp_rtc_alarm_isr(const struct device *dev)
{
#ifdef CONFIG_RTC_ALARM
	const struct nxp_rtc_config *config = dev->config;
	RTC_Type *rtc_reg = config->base;
	struct nxp_rtc_data *data = dev->data;
	uint32_t status_flags = 0U;
	uint32_t key = irq_lock();

	status_flags = RTC_GetStatusFlags(rtc_reg);
	if (status_flags & kRTC_AlarmFlag) {
		RTC_ClearStatusFlags(rtc_reg, kRTC_AlarmFlag);
		if (data->alarm_callback) {
			data->alarm_callback(dev, 0, data->alarm_user_data);
			data->alarm_pending = false;
		} else {
			data->alarm_pending = true;
		}
	}
	irq_unlock(key);
#endif
}

static void nxp_rtc_second_isr(const struct device *dev)
{
#ifdef CONFIG_RTC_UPDATE
	struct nxp_rtc_data *data = dev->data;
	uint32_t key = irq_lock();

	/* There is no status flag for RTC second interrupt */
	if (data->update_callback) {
		data->update_callback(dev, data->update_user_data);
	}
	irq_unlock(key);
#endif
}
#endif

static DEVICE_API(rtc, rtc_nxp_rtc_driver_api) = {
	.set_time = nxp_rtc_set_time,
	.get_time = nxp_rtc_get_time,
#if defined(CONFIG_RTC_ALARM)
	.alarm_get_supported_fields = nxp_rtc_alarm_get_supported_fields,
	.alarm_set_time = nxp_rtc_alarm_set_time,
	.alarm_get_time = nxp_rtc_alarm_get_time,
	.alarm_is_pending = nxp_rtc_alarm_is_pending,
	.alarm_set_callback = nxp_rtc_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE)
	.update_set_callback = nxp_rtc_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION)
	.set_calibration = nxp_rtc_set_calibration,
	.get_calibration = nxp_rtc_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

/* IRQ connection macros */
#define RTC_NXP_RTC_SINGLE_IRQ_CONNECT(n)                                                         \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), nxp_rtc_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	} while (false)

#define RTC_NXP_RTC_ALARM_IRQ_CONNECT(n)                                                          \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    nxp_rtc_alarm_isr, DEVICE_DT_INST_GET(n), 0);                         \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 0, irq));                                         \
	} while (false)

#define RTC_NXP_RTC_SECOND_IRQ_CONNECT(n)                                                         \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 1, irq), DT_INST_IRQ_BY_IDX(n, 1, priority),     \
			    nxp_rtc_second_isr, DEVICE_DT_INST_GET(n), 0);                        \
		irq_enable(DT_INST_IRQ_BY_IDX(n, 1, irq));                                         \
	} while (false)

#define NXP_RTC_CONFIG_FUNC(n)                                                               \
	static void nxp_rtc_config_func_##n(const struct device *dev)                            \
	{                                                                                         \
		IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, 1),                                              \
				(RTC_NXP_RTC_ALARM_IRQ_CONNECT(n);   \
				RTC_NXP_RTC_SECOND_IRQ_CONNECT(n);)) \
		IF_ENABLED(UTIL_NOT(DT_INST_IRQ_HAS_IDX(n, 1)),\
				(RTC_NXP_RTC_SINGLE_IRQ_CONNECT(n);)) \
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clock_output)
#define NXP_RTC_CLOCK_OUTPUT_ENABLE(n) .is_output_clock_enabled = DT_INST_PROP(n, clock_output),
#else
#define NXP_RTC_CLOCK_OUTPUT_ENABLE(n)
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(time_seconds_frequency)
#define NXP_RTC_TIME_SECONDS_FREQUENCY(n)                                                     \
	.time_seconds_frequency = DT_INST_PROP(n, time_seconds_frequency),
#else
#define NXP_RTC_TIME_SECONDS_FREQUENCY(n)
#endif

#define RTC_NXP_RTC_DEVICE_INIT(n)                                                            \
	NXP_RTC_CONFIG_FUNC(n)                                                                    \
	static const struct nxp_rtc_config nxp_rtc_config_##n = {                                \
		.base = (RTC_Type *)DT_INST_REG_ADDR(n),                                           \
		.is_lpo_clock_source = (strcmp(DT_INST_PROP(n, clock_source), "LPO") == 0),        \
		NXP_RTC_CLOCK_OUTPUT_ENABLE(n)                                                    \
		NXP_RTC_TIME_SECONDS_FREQUENCY(n)                                                 \
		.is_wakeup_enabled = DT_INST_PROP(n, enable_wakeup),                               \
		.is_update_mode = DT_INST_PROP(n, enable_update_mode),                             \
		.is_supervisor_access = DT_INST_PROP(n, supervisor_access),                        \
		.compensation_interval = DT_INST_PROP(n, compensation_interval),                   \
		.compensation_time = DT_INST_PROP(n, compensation_time),                           \
		.irq_config_func = nxp_rtc_config_func_##n,                                       \
	};                                                                                         \
                                                                                               \
	static struct nxp_rtc_data nxp_rtc_data_##n;                                             \
                                                                                               \
	DEVICE_DT_INST_DEFINE(n, nxp_rtc_init, NULL, &nxp_rtc_data_##n, &nxp_rtc_config_##n,    \
			      PRE_KERNEL_1, CONFIG_RTC_INIT_PRIORITY, &rtc_nxp_rtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_NXP_RTC_DEVICE_INIT)
