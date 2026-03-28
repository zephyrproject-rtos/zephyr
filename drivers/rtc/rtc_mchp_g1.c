/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/mchp_clock_control.h>
#include "rtc_utils.h"

#define DT_DRV_COMPAT microchip_rtc_g1

LOG_MODULE_REGISTER(rtc_mchp_g1, CONFIG_RTC_LOG_LEVEL);

#define RTC_MCHP_ALARM_1            (0)
#define RTC_MCHP_ALARM_2            (1)
#define RTC_TM_REFERENCE_YEAR       (1900U)
#define RTC_REFERENCE_YEAR          (1996U)
#define RTC_ADJUST_MONTH(month)     (month + 1U)
#define RTC_ALARM_COUNT             DT_PROP(DT_NODELABEL(rtc), alarms_count)
#define RTC_CALIB_PARTS_PER_BILLION (1000000000)
#define RTC_CALIBRATE_PPB_MAX       (127)
#define RTC_ALARM_PENDING           (1)

/* Timeout values for WAIT_FOR macro */
#define TIMEOUT_REG_SYNC 5000
#define DELAY_US         1

#ifdef CONFIG_RTC_ALARM
#define RTC_ALARM_SUPPORTED_MASK                                                                   \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR)

#define RTC_SUPPORTED_ALARM_INT_FLAGS (RTC_MODE2_INTFLAG_ALARM0_Msk | RTC_MODE2_INTFLAG_ALARM1_Msk)
#endif /* CONFIG_RTC_ALARM */

/* Structure defining various time parameters for Microchip RTC driver. */
struct rtc_mchp_time {
	uint32_t second;
	uint32_t minute;
	uint32_t hour;
	uint32_t date_of_month;
	uint32_t month;
	uint32_t year;
};

/* Do the peripheral interrupt related configuration */
#ifdef CONFIG_RTC_ALARM
#define RTC_MCHP_IRQ_HANDLER(n)                                                                    \
	static void rtc_mchp_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		RTC_MCHP_IRQ_CONNECT(n, 0);                                                        \
	}
#endif /* CONFIG_RTC_ALARM */

/* Clock configuration structure for RTC. */
struct rtc_mchp_clock {
	const struct device *clock_dev;
	clock_control_subsys_t mclk_sys;
	clock_control_subsys_t rtcclk_sys;
};

/* Enumeration for RTC (Real-Time Clock) alarm mask selection */
enum rtc_mchp_alarm_mask_sel {
	RTC_MCHP_ALARM_MASK_SEL_OFF = 0x0,
	RTC_MCHP_ALARM_MASK_SEL_SS = 0x1,
	RTC_MCHP_ALARM_MASK_SEL_MMSS = 0x2,
	RTC_MCHP_ALARM_MASK_SEL_HHMMSS = 0x3,
	RTC_MCHP_ALARM_MASK_SEL_DDHHMMSS = 0x4,
	RTC_MCHP_ALARM_MASK_SEL_MMDDHHMMSS = 0x5,
	RTC_MCHP_ALARM_MASK_SEL_YYMMDDHHMMSS = 0x6,
};

struct rtc_mchp_dev_config {
	rtc_registers_t *regs;
	struct rtc_mchp_clock rtc_clock;
	uint16_t prescaler;
	void (*irq_config_func)(const struct device *dev);

#ifdef CONFIG_RTC_CALIBRATION
	int32_t cal_constant;
#endif /* CONFIG_RTC_CALIBRATION */

#ifdef CONFIG_RTC_ALARM
	uint8_t alarms_count;
#endif /* CONFIG_RTC_ALARM */
};

#ifdef CONFIG_RTC_ALARM
/* This structure holds the callback information for RTC alarm events. */
struct rtc_mchp_data_cb {
	bool is_alarm_pending;
	rtc_alarm_callback alarm_cb;
	void *alarm_user_data;
};
#endif /* CONFIG_RTC_ALARM */

struct rtc_mchp_dev_data {
	/* Semaphore for protecting access to the RTC driver data. */
	struct k_sem lock;

#ifdef CONFIG_RTC_ALARM
	struct rtc_mchp_data_cb alarms[RTC_ALARM_COUNT];
#endif /* CONFIG_RTC_ALARM */
};

/* Wait for RTC synchronization to complete. */
static inline void rtc_sync_busy(const rtc_registers_t *regs, uint32_t sync_flag)
{
	if (WAIT_FOR(((regs->MODE2.RTC_SYNCBUSY & sync_flag) == 0), TIMEOUT_REG_SYNC,
		     k_busy_wait(DELAY_US)) == false) {
		LOG_ERR("RTC reset timed out");
	}
}

/* Enable or disable the RTC module based on the given flag. */
static inline void rtc_enable(rtc_registers_t *regs, bool enable)
{
	if (enable == true) {
		regs->MODE2.RTC_CTRLA |= RTC_MODE2_CTRLA_ENABLE(1);
	} else {
		regs->MODE2.RTC_CTRLA &= ~RTC_MODE2_CTRLA_ENABLE(1);
	}

	rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_ENABLE_Msk);
}

static void rtc_enable_clock_calendar_mode(rtc_registers_t *regs)
{
	regs->MODE2.RTC_CTRLA = ((regs->MODE2.RTC_CTRLA &
				  ~(RTC_MODE2_CTRLA_MODE_Msk | RTC_MODE2_CTRLA_CLOCKSYNC_Msk)) |
				 (RTC_MODE2_CTRLA_MODE(2UL) | RTC_MODE2_CTRLA_CLOCKSYNC(1)));

	rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk);
}

static inline void rtc_set_prescaler(rtc_registers_t *regs, uint16_t prescaler_value)
{
	regs->MODE2.RTC_CTRLA = ((regs->MODE2.RTC_CTRLA & ~(RTC_MODE2_CTRLA_PRESCALER_Msk)) |
				 RTC_MODE2_CTRLA_PRESCALER(prescaler_value + 1));
}

static void rtc_set_clock_time(rtc_registers_t *regs, struct rtc_mchp_time *rtc_set_time)
{
	regs->MODE2.RTC_CLOCK =
		(uint32_t)((((RTC_TM_REFERENCE_YEAR + rtc_set_time->year) - RTC_REFERENCE_YEAR)
			    << RTC_MODE2_CLOCK_YEAR_Pos) |
			   ((RTC_ADJUST_MONTH(rtc_set_time->month)) << RTC_MODE2_CLOCK_MONTH_Pos) |
			   (rtc_set_time->date_of_month << RTC_MODE2_CLOCK_DAY_Pos) |
			   (rtc_set_time->hour << RTC_MODE2_CLOCK_HOUR_Pos) |
			   (rtc_set_time->minute << RTC_MODE2_CLOCK_MINUTE_Pos) |
			   (rtc_set_time->second << RTC_MODE2_CLOCK_SECOND_Pos));

	rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk);
}

static void rtc_get_clock_time(const rtc_registers_t *regs, struct rtc_mchp_time *rtc_get_time)
{
	uint32_t dataClockCalendar = 0U;

	/* Synchronization before reading value from CLOCK Register */
	rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk);
	dataClockCalendar = regs->MODE2.RTC_CLOCK;

	rtc_get_time->hour =
		(dataClockCalendar & RTC_MODE2_CLOCK_HOUR_Msk) >> RTC_MODE2_CLOCK_HOUR_Pos;

	rtc_get_time->minute =
		(dataClockCalendar & RTC_MODE2_CLOCK_MINUTE_Msk) >> RTC_MODE2_CLOCK_MINUTE_Pos;

	rtc_get_time->second =
		(dataClockCalendar & RTC_MODE2_CLOCK_SECOND_Msk) >> RTC_MODE2_CLOCK_SECOND_Pos;

	rtc_get_time->month =
		(((dataClockCalendar & RTC_MODE2_CLOCK_MONTH_Msk) >> RTC_MODE2_CLOCK_MONTH_Pos) -
		 1);

	rtc_get_time->year =
		(((dataClockCalendar & RTC_MODE2_CLOCK_YEAR_Msk) >> RTC_MODE2_CLOCK_YEAR_Pos) +
		 RTC_REFERENCE_YEAR) -
		RTC_TM_REFERENCE_YEAR;

	rtc_get_time->date_of_month =
		(dataClockCalendar & RTC_MODE2_CLOCK_DAY_Msk) >> RTC_MODE2_CLOCK_DAY_Pos;
}

#ifdef CONFIG_RTC_ALARM
static void rtc_set_alarm_mask(rtc_registers_t *regs, uint16_t alarm_id, uint16_t alarm_mask)
{
	uint16_t set_mask = alarm_mask;

	if (alarm_id == RTC_MCHP_ALARM_1) {
		regs->MODE2.RTC_MASK0 = (uint8_t)((regs->MODE2.RTC_MASK0 & ~RTC_MODE2_MASK0_Msk) |
						  RTC_MODE2_MASK0_SEL(set_mask));
		rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_MASK0_Msk);
	} else if (alarm_id == RTC_MCHP_ALARM_2) {
		regs->MODE2.RTC_MASK1 = (uint8_t)((regs->MODE2.RTC_MASK1 & ~RTC_MODE2_MASK1_Msk) |
						  RTC_MODE2_MASK1_SEL(set_mask));
		rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_MASK1_Msk);
	} else {
		LOG_ERR("Invalid alarm_id: %u", alarm_id);
	}
}

static uint16_t rtc_get_alarm_mask(const rtc_registers_t *regs, uint16_t alarm_id)
{
	uint16_t get_mask = 0;

	if (alarm_id == RTC_MCHP_ALARM_1) {
		get_mask = (uint16_t)regs->MODE2.RTC_MASK0;
		rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_MASK0_Msk);
	} else if (alarm_id == RTC_MCHP_ALARM_2) {
		get_mask = (uint16_t)regs->MODE2.RTC_MASK1;
		rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_MASK1_Msk);
	} else {
		LOG_ERR("Invalid alarm_id: %u", alarm_id);
	}

	return get_mask;
}

static void rtc_set_alarm_time(rtc_registers_t *regs, uint16_t alarm_id,
			       struct rtc_mchp_time *rtc_set_alarm)
{
	if (alarm_id == RTC_MCHP_ALARM_1) {
		regs->MODE2.RTC_ALARM0 =
			(uint32_t)((((RTC_TM_REFERENCE_YEAR + rtc_set_alarm->year) -
				     RTC_REFERENCE_YEAR)
				    << RTC_MODE2_CLOCK_YEAR_Pos) |
				   ((RTC_ADJUST_MONTH(rtc_set_alarm->month))
				    << RTC_MODE2_CLOCK_MONTH_Pos) |
				   (rtc_set_alarm->date_of_month << RTC_MODE2_CLOCK_DAY_Pos) |
				   (rtc_set_alarm->hour << RTC_MODE2_CLOCK_HOUR_Pos) |
				   (rtc_set_alarm->minute << RTC_MODE2_CLOCK_MINUTE_Pos) |
				   (rtc_set_alarm->second << RTC_MODE2_CLOCK_SECOND_Pos));
	} else if (alarm_id == RTC_MCHP_ALARM_2) {
		regs->MODE2.RTC_ALARM1 =
			(uint32_t)((((RTC_TM_REFERENCE_YEAR + rtc_set_alarm->year) -
				     RTC_REFERENCE_YEAR)
				    << RTC_MODE2_CLOCK_YEAR_Pos) |
				   ((RTC_ADJUST_MONTH(rtc_set_alarm->month))
				    << RTC_MODE2_CLOCK_MONTH_Pos) |
				   (rtc_set_alarm->date_of_month << RTC_MODE2_CLOCK_DAY_Pos) |
				   (rtc_set_alarm->hour << RTC_MODE2_CLOCK_HOUR_Pos) |
				   (rtc_set_alarm->minute << RTC_MODE2_CLOCK_MINUTE_Pos) |
				   (rtc_set_alarm->second << RTC_MODE2_CLOCK_SECOND_Pos));
	} else {
		LOG_ERR("Invalid alarm_id: %u", alarm_id);
	}

	rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk);
}

static void rtc_get_alarm_time(const rtc_registers_t *regs, uint16_t alarm_id,
			       struct rtc_mchp_time *rtc_get_time)
{
	uint32_t dataClockCalendar = 0U;

	/* Synchronization before reading value from CLOCK Register */
	rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_CLOCKSYNC_Msk);

	if (alarm_id == RTC_MCHP_ALARM_1) {
		dataClockCalendar = regs->MODE2.RTC_ALARM0;
	} else if (alarm_id == RTC_MCHP_ALARM_2) {
		dataClockCalendar = regs->MODE2.RTC_ALARM1;
	} else {
		LOG_ERR("Invalid alarm_id: %u", alarm_id);
	}
	rtc_get_time->hour =
		(dataClockCalendar & RTC_MODE2_CLOCK_HOUR_Msk) >> RTC_MODE2_CLOCK_HOUR_Pos;

	rtc_get_time->minute =
		(dataClockCalendar & RTC_MODE2_CLOCK_MINUTE_Msk) >> RTC_MODE2_CLOCK_MINUTE_Pos;

	rtc_get_time->second =
		(dataClockCalendar & RTC_MODE2_CLOCK_SECOND_Msk) >> RTC_MODE2_CLOCK_SECOND_Pos;

	rtc_get_time->month =
		(((dataClockCalendar & RTC_MODE2_CLOCK_MONTH_Msk) >> RTC_MODE2_CLOCK_MONTH_Pos) -
		 1);

	rtc_get_time->year =
		(((dataClockCalendar & RTC_MODE2_CLOCK_YEAR_Msk) >> RTC_MODE2_CLOCK_YEAR_Pos) +
		 RTC_REFERENCE_YEAR) -
		RTC_TM_REFERENCE_YEAR;

	rtc_get_time->date_of_month =
		(dataClockCalendar & RTC_MODE2_CLOCK_DAY_Msk) >> RTC_MODE2_CLOCK_DAY_Pos;
}

static void rtc_enable_interrupt(rtc_registers_t *regs, uint16_t alarm_id)
{
	uint16_t alarm_int = 0;

	if (alarm_id == RTC_MCHP_ALARM_1) {
		alarm_int = RTC_MODE2_INTENSET_ALARM0(1);
	} else if (alarm_id == RTC_MCHP_ALARM_2) {
		alarm_int = RTC_MODE2_INTENSET_ALARM1(1);
	} else {
		LOG_ERR("Invalid alarm_id: %u", alarm_id);
		return;
	}

	regs->MODE2.RTC_INTENSET = alarm_int;
}

static void rtc_disable_interrupt(rtc_registers_t *regs, uint16_t alarm_id)
{
	uint16_t alarm_int = 0;

	if (alarm_id == RTC_MCHP_ALARM_1) {
		alarm_int = RTC_MODE2_INTENCLR_ALARM0(1);
	} else if (alarm_id == RTC_MCHP_ALARM_2) {
		alarm_int = RTC_MODE2_INTENCLR_ALARM1(1);
	} else {
		LOG_ERR("Invalid alarm_id: %u", alarm_id);
		return;
	}

	regs->MODE2.RTC_INTENCLR = alarm_int;
}

static uint16_t rtc_get_interrupt_flags(const rtc_registers_t *regs, uint16_t *alarm_id)
{
	uint16_t int_status = regs->MODE2.RTC_INTFLAG;

	if ((int_status & RTC_MODE2_INTFLAG_ALARM0_Msk) == RTC_MODE2_INTFLAG_ALARM0_Msk) {
		*alarm_id = RTC_MCHP_ALARM_1;
	} else if ((int_status & RTC_MODE2_INTFLAG_ALARM1_Msk) == RTC_MODE2_INTFLAG_ALARM1_Msk) {
		*alarm_id = RTC_MCHP_ALARM_2;
	} else {
		LOG_ERR("Invalid alarm interrupt flag detected");
	}

	return int_status;
}

static void rtc_clear_interrupt_flags(rtc_registers_t *regs, uint16_t alarm_id)
{
	uint16_t alarm_status =
		((alarm_id == RTC_MCHP_ALARM_1)
			 ? RTC_MODE2_INTFLAG_ALARM0_Msk
			 : ((alarm_id == RTC_MCHP_ALARM_2) ? RTC_MODE2_INTFLAG_ALARM1_Msk : 0));

	regs->MODE2.RTC_INTFLAG = alarm_status;
}

/*
 * Get the supported alarm interrupt flags for the RTC.
 *
 * This function retrieves the supported alarm interrupt flags for the Real-Time Clock (RTC)
 * hardware. The flags indicate which alarm interrupt features are supported by the RTC.
 */
static inline uint16_t rtc_supported_alarm_int_flags(const rtc_registers_t *regs)
{
	return (RTC_MODE2_INTFLAG_ALARM0_Msk | RTC_MODE2_INTFLAG_ALARM1_Msk);
}

/*
 * Convert the provided alarm mask to the hardware-specific format.
 *
 * This function converts the provided alarm mask into a format that the hardware RTC can
 * understand.
 *
 * returns The hardware-specific alarm mask to write to the register.
 */
static uint16_t rtc_alarm_mask(uint16_t alarm_mask)
{
	uint16_t rtc_alarm_enable_mask = 0;

	if ((alarm_mask & RTC_ALARM_TIME_MASK_SECOND) != 0) {
		rtc_alarm_enable_mask = RTC_MCHP_ALARM_MASK_SEL_SS;
	}
	if ((alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) != 0) {
		rtc_alarm_enable_mask = RTC_MCHP_ALARM_MASK_SEL_MMSS;
	}
	if ((alarm_mask & RTC_ALARM_TIME_MASK_HOUR) != 0) {
		rtc_alarm_enable_mask = RTC_MCHP_ALARM_MASK_SEL_HHMMSS;
	}
	if ((alarm_mask & RTC_ALARM_TIME_MASK_MONTHDAY) != 0) {
		rtc_alarm_enable_mask = RTC_MCHP_ALARM_MASK_SEL_DDHHMMSS;
	}
	if ((alarm_mask & RTC_ALARM_TIME_MASK_MONTH) != 0) {
		rtc_alarm_enable_mask = RTC_MCHP_ALARM_MASK_SEL_MMDDHHMMSS;
	}
	if ((alarm_mask & RTC_ALARM_TIME_MASK_YEAR) != 0) {
		rtc_alarm_enable_mask = RTC_MCHP_ALARM_MASK_SEL_YYMMDDHHMMSS;
	}

	return rtc_alarm_enable_mask;
}

/*
 * Convert the mask value from hardware-specific format to expected zephyr mask format.
 *
 * This function converts the read mask from rtc registers into a format that zephyr
 * expects.
 *
 * returns The converted alarm mask.
 */
static uint16_t rtc_mask_from_alarm_msk(uint16_t mask)
{
	uint16_t alarm_mask = 0;

	switch (mask) {
	case RTC_MCHP_ALARM_MASK_SEL_YYMMDDHHMMSS:
		alarm_mask |= RTC_ALARM_TIME_MASK_YEAR;
		__fallthrough;
	case RTC_MCHP_ALARM_MASK_SEL_MMDDHHMMSS:
		alarm_mask |= RTC_ALARM_TIME_MASK_MONTH;
		__fallthrough;
	case RTC_MCHP_ALARM_MASK_SEL_DDHHMMSS:
		alarm_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		__fallthrough;
	case RTC_MCHP_ALARM_MASK_SEL_HHMMSS:
		alarm_mask |= RTC_ALARM_TIME_MASK_HOUR;
		__fallthrough;
	case RTC_MCHP_ALARM_MASK_SEL_MMSS:
		alarm_mask |= RTC_ALARM_TIME_MASK_MINUTE;
		__fallthrough;
	case RTC_MCHP_ALARM_MASK_SEL_SS:
		alarm_mask |= RTC_ALARM_TIME_MASK_SECOND;
		break;
	default:
		break;
	}

	return alarm_mask;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_CALIBRATION
static void rtc_set_calibration_value(rtc_registers_t *regs, uint32_t calib, uint32_t correction)
{
	/* Combine the calibration value and correction sign */
	uint32_t freqcorr_value =
		((calib & RTC_FREQCORR_VALUE_Msk) | (correction & RTC_FREQCORR_SIGN_Msk));

	/* Write the combined value to the RTC_FREQCORR register */
	regs->MODE2.RTC_FREQCORR = (uint8_t)freqcorr_value;

	rtc_sync_busy(regs, RTC_MODE2_SYNCBUSY_FREQCORR_Msk);
}

static int rtc_get_calibration_value(const rtc_registers_t *regs, uint32_t *calib,
				     uint8_t *corr_sign)
{
	if ((calib == NULL) || (corr_sign == NULL)) {
		LOG_ERR("Invalid argument: calib or corr_sign is NULL");
		return -EINVAL;
	}

	/* Read the calibration value from the RTC_FREQCORR register */
	*calib = (uint32_t)(regs->MODE2.RTC_FREQCORR & RTC_FREQCORR_VALUE_Msk);

	/* Read the correction sign from the RTC_FREQCORR register */
	if ((regs->MODE2.RTC_FREQCORR & RTC_FREQCORR_SIGN_Msk) == RTC_FREQCORR_SIGN_Msk) {
		*corr_sign = 1;
	}

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

#ifdef CONFIG_RTC_ALARM
static void rtc_mchp_isr(const struct device *dev)
{
	struct rtc_mchp_dev_data *data = dev->data;
	const struct rtc_mchp_dev_config *const cfg = dev->config;
	uint16_t alarm_id = -1;

	/* Get the interrupt flags and the alarm ID and clear the interrupt flags */
	uint16_t rtc_int_flag = rtc_get_interrupt_flags(cfg->regs, &alarm_id);

	rtc_clear_interrupt_flags(cfg->regs, alarm_id);

	for (uint8_t alarm = 0; alarm <= alarm_id; alarm++) {
		if ((rtc_int_flag & RTC_SUPPORTED_ALARM_INT_FLAGS) != 0) {
			if (data->alarms[alarm].alarm_cb != NULL) {
				data->alarms[alarm].alarm_cb(dev, alarm,
							     data->alarms[alarm].alarm_user_data);
				data->alarms[alarm].is_alarm_pending = false;
			} else {
				data->alarms[alarm].is_alarm_pending = true;
			}
		}
	}
}

static int rtc_mchp_get_alarm_supported_fields(const struct device *dev, uint16_t id,
					       uint16_t *mask)
{
	ARG_UNUSED(id);

	*mask = RTC_ALARM_SUPPORTED_MASK;

	return 0;
}

static int rtc_mchp_alarm_is_pending(const struct device *dev, uint16_t alarm_id)
{
	struct rtc_mchp_dev_data *data = dev->data;
	const struct rtc_mchp_dev_config *const cfg = dev->config;
	int retval = 0;

	/* Check if the alarm ID is within the valid range */
	if (alarm_id >= cfg->alarms_count) {
		LOG_ERR("RTC Alarm id is out of range");
		retval = -EINVAL;
	} else {
		/* Lock interrupts to ensure setting of callback*/
		unsigned int key = irq_lock();

		if (data->alarms[alarm_id].is_alarm_pending == true) {
			retval = RTC_ALARM_PENDING;

			/* Clear the pending status of the alarm. */
			data->alarms[alarm_id].is_alarm_pending = false;
		}

		/* Unlock interrupts after alarm pending check */
		irq_unlock(key);
	}

	return retval;
}

static int rtc_mchp_set_alarm_time(const struct device *dev, uint16_t alarm_id, uint16_t alarm_mask,
				   const struct rtc_time *timeptr)
{
	const struct rtc_mchp_dev_config *const cfg = dev->config;
	struct rtc_mchp_time rtc_time;
	struct rtc_mchp_dev_data *data = dev->data;
	uint16_t supported_mask;
	uint16_t set_mask = 0;

	/* Get the supported alarm fields */
	rtc_mchp_get_alarm_supported_fields(dev, 0, &supported_mask);

	/* Check if the provided alarm mask is valid */
	if ((alarm_mask & ~supported_mask) != 0) {
		LOG_ERR("Invalid RTC alarm mask");
		return -EINVAL;
	}

	/* Check if the alarm ID is within the valid range */
	if (alarm_id >= cfg->alarms_count) {
		LOG_ERR("RTC Alarm id is out of range");
		return -EINVAL;
	}

	/* Check if the time pointer is provided when the alarm mask is not zero */
	if ((timeptr == NULL) && (alarm_mask != 0)) {
		LOG_ERR("No pointer is provided to set RTC alarm");
		return -EINVAL;
	}

	/* Validate the provided RTC time */
	if ((timeptr != NULL) && (rtc_utils_validate_rtc_time(timeptr, supported_mask) == false)) {
		LOG_ERR("Invalid RTC time provided");
		return -EINVAL;
	}

	/* If validation passed, set the RTC ALARM time */
	rtc_time.second = timeptr->tm_sec;
	rtc_time.minute = timeptr->tm_min;
	rtc_time.hour = timeptr->tm_hour;
	rtc_time.month = timeptr->tm_mon;
	rtc_time.date_of_month = timeptr->tm_mday;
	rtc_time.year = timeptr->tm_year;

	/* Lock the semaphore before accessing the RTC. */
	k_sem_take(&data->lock, K_FOREVER);

	/* Disable the interrupt for the specified alarm ID */
	rtc_disable_interrupt(cfg->regs, alarm_id);

	if (alarm_mask == 0) {
		/* If the alarm mask is zero, turn off the alarm */
		rtc_set_alarm_mask(cfg->regs, alarm_id, RTC_MCHP_ALARM_MASK_SEL_OFF);
	} else {

		/* Set the alarm time */
		rtc_set_alarm_time(cfg->regs, alarm_id, &rtc_time);

		/* Enable the interrupt for the specified alarm ID */
		set_mask = rtc_alarm_mask(alarm_mask);

		rtc_set_alarm_mask(cfg->regs, alarm_id, set_mask);

		/* Enable the interrupt for the specified alarm ID */
		rtc_enable_interrupt(cfg->regs, alarm_id);
	}

	/* Unlock the semaphore before returning. */
	k_sem_give(&data->lock);

	return 0;
}

static int rtc_mchp_get_alarm_time(const struct device *dev, uint16_t alarm_id,
				   uint16_t *alarm_mask, struct rtc_time *timeptr)
{
	struct rtc_mchp_dev_data *data = dev->data;
	const struct rtc_mchp_dev_config *const cfg = dev->config;
	struct rtc_mchp_time rtc_alarm_time = {0};
	uint16_t mask;

	/* Check if the alarm ID is within the valid range */
	if (alarm_id >= cfg->alarms_count) {
		LOG_ERR("RTC Alarm id is out of range");
		return -EINVAL;
	}

	/* Check if the time pointer is provided when the alarm mask is not zero */
	if (timeptr == NULL) {
		LOG_ERR("No pointer is provided to get RTC alarm");
		return -EINVAL;
	}

	/* Lock the semaphore before accessing the RTC. */
	k_sem_take(&data->lock, K_FOREVER);

	/* get the rtc alarm time from id */
	rtc_get_alarm_time(cfg->regs, alarm_id, &rtc_alarm_time);

	/* Get the mask of fields which are enabled in the alarm time */
	mask = rtc_get_alarm_mask(cfg->regs, alarm_id);
	*alarm_mask = rtc_mask_from_alarm_msk(mask);

	/* Unlock the semaphore before returning. */
	k_sem_give(&data->lock);

	/* Populate the rtc_time structure with the retrieved values. */
	timeptr->tm_sec = (int)rtc_alarm_time.second;
	timeptr->tm_min = (int)rtc_alarm_time.minute;
	timeptr->tm_hour = (int)rtc_alarm_time.hour;
	timeptr->tm_mon = (int)rtc_alarm_time.month;
	timeptr->tm_mday = (int)rtc_alarm_time.date_of_month;
	timeptr->tm_year = (int)rtc_alarm_time.year;

	return 0;
}

static int rtc_mchp_set_alarm_callback(const struct device *dev, uint16_t alarm_id,
				       rtc_alarm_callback callback, void *user_data)
{
	struct rtc_mchp_dev_data *data = dev->data;
	const struct rtc_mchp_dev_config *const cfg = dev->config;

	/* Check if the alarm ID is within the valid range */
	if (alarm_id >= cfg->alarms_count) {
		LOG_ERR("RTC Alarm id is out of range");
		return -EINVAL;
	}
	/* Lock interrupts to ensure setting of callback */
	unsigned int key = irq_lock();

	/* Set the callback function for the alarm and user data. */
	data->alarms[alarm_id].alarm_cb = callback;
	data->alarms[alarm_id].alarm_user_data = user_data;

	/* Unlock the IRQ after completion of setting callback */
	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

static int rtc_mchp_set_clock_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_mchp_dev_data *data = dev->data;
	const struct rtc_mchp_dev_config *const cfg = dev->config;
	struct rtc_mchp_time rtc_time;

	/* Check if rtc_time structure not null */
	if (timeptr == NULL) {
		LOG_ERR("RTC set time failed: time pointer is NULL");
		return -EINVAL;

	} else {
#ifdef CONFIG_RTC_ALARM
		/* Validate the provided RTC time parameters */
		if (rtc_utils_validate_rtc_time(timeptr, RTC_ALARM_SUPPORTED_MASK) == false) {
			LOG_ERR("RTC time parameters are invalid");
			return -EINVAL;
		}
#endif /* CONFIG_RTC_ALARM */

		/* If validation passed, set the RTC time */
		rtc_time.second = timeptr->tm_sec;
		rtc_time.minute = timeptr->tm_min;
		rtc_time.hour = timeptr->tm_hour;
		rtc_time.month = timeptr->tm_mon;
		rtc_time.date_of_month = timeptr->tm_mday;
		rtc_time.year = timeptr->tm_year;

		/* lock the semaphore before setting the RTC. */
		k_sem_take(&data->lock, K_FOREVER);

		rtc_set_clock_time(cfg->regs, &rtc_time);

		/* Unlock the semaphore before returning. */
		k_sem_give(&data->lock);
	}

	return 0;
}

static int rtc_mchp_get_clock_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_mchp_dev_data *data = dev->data;
	const struct rtc_mchp_dev_config *const cfg = dev->config;
	struct rtc_mchp_time rtc_current_time;

	/* Lock the semaphore before accessing the RTC. */
	k_sem_take(&data->lock, K_FOREVER);

	/* Retrieve the current time from the RTC. */
	rtc_get_clock_time(cfg->regs, &rtc_current_time);

	/* Unlock the semaphore before returning. */
	k_sem_give(&data->lock);

	/* Populate the rtc_time structure with the retrieved values. */
	timeptr->tm_sec = (int)rtc_current_time.second;
	timeptr->tm_min = (int)rtc_current_time.minute;
	timeptr->tm_hour = (int)rtc_current_time.hour;
	timeptr->tm_mon = (int)rtc_current_time.month;
	timeptr->tm_mday = (int)rtc_current_time.date_of_month;
	timeptr->tm_year = (int)rtc_current_time.year;

	return 0;
}

#ifdef CONFIG_RTC_CALIBRATION
static int rtc_mchp_set_calibration(const struct device *dev, int32_t calibration)
{
	const struct rtc_mchp_dev_config *const cfg = dev->config;

	/* Calculate the calibration value in parts per billion */
	int32_t correction = calibration / (RTC_CALIB_PARTS_PER_BILLION / cfg->cal_constant);
	uint32_t abs_correction = abs(correction);

	LOG_DBG("Correction: %d, Absolute: %d, Calibration: %d", correction, abs_correction,
		calibration);

	/* Check if correction value is 0 */
	if (abs_correction == 0) {
		rtc_set_calibration_value(cfg->regs, 0, 0);
	}

	/* Check if correction value out of range */
	if (abs_correction > RTC_CALIBRATE_PPB_MAX) {
		LOG_ERR("The RTC calibration %d result in an out of range value %d", calibration,
			abs_correction);
		return -EINVAL;
	}

	/* Set the correction value into RTC calib register */
	rtc_set_calibration_value(cfg->regs, abs_correction, correction);

	return 0;
}

static int rtc_mchp_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct rtc_mchp_dev_config *const cfg = dev->config;
	int32_t correction;
	uint8_t correction_sign = 0;
	int status;

	/* Check if the calibration pointer is NULL */
	if (calibration == NULL) {
		LOG_ERR("Invalid input: calibration pointer is NULL");
		return -EINVAL;
	}

	/* Retrieve the correction value from the hardware register */
	status = rtc_get_calibration_value(cfg->regs, &correction, &correction_sign);
	if (status < 0) {
		LOG_ERR("Unable to read RTC calibration value");
		return -EINVAL;
	}

	/* Calculate the calibration value based on the correction value */
	if (correction == 0) {
		*calibration = 0;
	} else {
		*calibration =
			((int64_t)correction * RTC_CALIB_PARTS_PER_BILLION) / cfg->cal_constant;
	}

	/* Adjust the calibration value based on the sign bit */
	if (correction_sign == 1) {
		*calibration *= -1;
	}

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

/*
 * Initialize the RTC (Real-Time Clock) for the Microchip device.
 *
 * This function initializes the RTC hardware by enabling the clock, performing a software
 * reset, setting the prescaler, enabling the clock calendar mode, and enabling the RTC. If
 * RTC alarm configuration is enabled, it also configures the IRQ for the RTC peripheral.
 */
static int rtc_mchp_init(const struct device *dev)
{
	struct rtc_mchp_dev_data *data = dev->data;
	const struct rtc_mchp_dev_config *const cfg = dev->config;
	int ret = 0;

	/* On Oscillator clock for RTC */
	ret = clock_control_on(cfg->rtc_clock.clock_dev, cfg->rtc_clock.rtcclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the osc32k clock for RTC: %d", ret);
		return ret;
	}

	/* On Main clock for RTC */
	ret = clock_control_on(cfg->rtc_clock.clock_dev, cfg->rtc_clock.mclk_sys);
	if ((ret != 0) && (ret != -EALREADY)) {
		LOG_ERR("Failed to enable the MCLK for RTC: %d", ret);
		return ret;
	}

	/* Initialize semaphore for RTC data structure */
	k_sem_init(&data->lock, 1, 1);

	/* Set the prescaler for the RTC peripheral */
	rtc_set_prescaler(cfg->regs, cfg->prescaler);

	/* Enable the clock calendar mode for the RTC peripheral */
	rtc_enable_clock_calendar_mode(cfg->regs);

	/* Enable the RTC peripheral */
	rtc_enable(cfg->regs, true);

#ifdef CONFIG_RTC_ALARM
	/* Configure the IRQ for the RTC peripheral */
	cfg->irq_config_func(dev);
#endif /* CONFIG_RTC_ALARM */

	return 0;
}

static DEVICE_API(rtc, rtc_mchp_api) = {
	.set_time = rtc_mchp_set_clock_time,
	.get_time = rtc_mchp_get_clock_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_mchp_get_alarm_supported_fields,
	.alarm_is_pending = rtc_mchp_alarm_is_pending,
	.alarm_set_time = rtc_mchp_set_alarm_time,
	.alarm_get_time = rtc_mchp_get_alarm_time,
	.alarm_set_callback = rtc_mchp_set_alarm_callback,
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rtc_mchp_set_calibration,
	.get_calibration = rtc_mchp_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

/* Defines the RTC interrupt configurations. */
#ifdef CONFIG_RTC_ALARM
#define RTC_MCHP_IRQ_CONNECT(n, m)                                                                 \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    rtc_mchp_isr, DEVICE_DT_INST_GET(n), 0);                               \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)
#endif /* CONFIG_RTC_ALARM */

/* RTC driver clock configuration for instance n */
#define RTC_MCHP_CLOCK_DEFN(n)                                                                     \
	.rtc_clock.clock_dev = DEVICE_DT_GET(DT_NODELABEL(clock)),                                 \
	.rtc_clock.mclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, mclk, subsystem)),           \
	.rtc_clock.rtcclk_sys = (void *)(DT_INST_CLOCKS_CELL_BY_NAME(n, rtcclk, subsystem)),

/* Do the peripheral interrupt related configuration */
#ifdef CONFIG_RTC_ALARM
#define RTC_MCHP_IRQ_HANDLER(n)                                                                    \
	static void rtc_mchp_irq_config_##n(const struct device *dev)                              \
	{                                                                                          \
		RTC_MCHP_IRQ_CONNECT(n, 0);                                                        \
	}
#endif /* CONFIG_RTC_ALARM */

/* RTC driver configuration structure for instance n */
/* clang-format off */
#define RTC_MCHP_CONFIG_DEFN(n)								\
	static const struct rtc_mchp_dev_config rtc_mchp_dev_config_##n = {		\
		.regs = (rtc_registers_t *)DT_INST_REG_ADDR(n),				\
		.prescaler = DT_INST_ENUM_IDX(n, prescaler),				\
		IF_ENABLED(CONFIG_RTC_ALARM, (						\
			.alarms_count = DT_INST_PROP(n, alarms_count),			\
			.irq_config_func = &rtc_mchp_irq_config_##n,			\
		))									\
		IF_ENABLED(CONFIG_RTC_CALIBRATION, (					\
			.cal_constant = DT_INST_PROP(n, cal_constant),			\
		))									\
		RTC_MCHP_CLOCK_DEFN(n)							\
	}
/* Define and initialize the RTC device. */
#define RTC_MCHP_DEVICE_INIT(n)								\
	IF_ENABLED(CONFIG_RTC_ALARM, (							\
		static void rtc_mchp_irq_config_##n(const struct device *dev);		\
	))										\
	RTC_MCHP_CONFIG_DEFN(n);							\
	static struct rtc_mchp_dev_data rtc_mchp_dev_data_##n;				\
	DEVICE_DT_INST_DEFINE(n, rtc_mchp_init, NULL, &rtc_mchp_dev_data_##n,		\
			      &rtc_mchp_dev_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,	\
			      &rtc_mchp_api);							\
	IF_ENABLED(CONFIG_RTC_ALARM, (								\
			RTC_MCHP_IRQ_HANDLER(n)							\
		))

/* clang-format on */
DT_INST_FOREACH_STATUS_OKAY(RTC_MCHP_DEVICE_INIT);
