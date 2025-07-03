/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. (AMD)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/timeutil.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/device.h>
#include "rtc_xlnx.h"

#define DT_DRV_COMPAT xlnx_zynqmp_rtc

LOG_MODULE_REGISTER(xlnx, CONFIG_RTC_LOG_LEVEL);

/**
 * @brief Holds configuration settings for the RTC
 */
struct xlnx_rtc_config {
	mm_reg_t regbase;
	void (*config_func)(const struct device *dev);
};

/**
 * @brief Holds internal state and alarm data for the RTC
 */
struct xlnx_rtc_data {
	struct k_spinlock lock;
	uint32_t rtc_clock_freq;
#ifdef CONFIG_RTC_ALARM
	bool alarm_pending;
	void *alarm_user_data;
	uint16_t alarm_set_mask;
	rtc_alarm_callback alarm_user_callback;
#endif
};

static inline uint32_t rtc_read32(const struct device *dev, mm_reg_t offset)
{
	const struct xlnx_rtc_config *config = dev->config;

	return sys_read32(config->regbase + offset);
}

static inline void rtc_write32(const struct device *dev, uint32_t value, mm_reg_t offset)
{
	const struct xlnx_rtc_config *config = dev->config;

	sys_write32(value, (config->regbase + offset));
}

static void rtc_xlnx_isr(const struct device *dev)
{
	uint32_t status;

	status = rtc_read32(dev, XLNX_RTC_INT_STS_OFFSET);

	if (!(status & (XLNX_RTC_SECS_MASK | XLNX_RTC_ALARM_MASK))) {
		return;
	}

#if defined(CONFIG_RTC_ALARM)
	struct xlnx_rtc_data *data = dev->data;

	if (status & XLNX_RTC_ALARM_MASK) {
		/* Clear the RTC Alarm Interrupt */
		rtc_write32(dev, XLNX_RTC_ALARM_MASK, XLNX_RTC_INT_STS_OFFSET);
		if (data->alarm_user_callback) {
			data->alarm_user_callback(dev, 0, data->alarm_user_data);
			data->alarm_pending = false;
		} else {
			data->alarm_pending = true;
		}
	}
#endif
}

/**
 * @brief Get the time from RTC
 */
static int xlnx_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint32_t status;
	time_t readtime;

	status = rtc_read32(dev, XLNX_RTC_INT_STS_OFFSET);

	if (status & XLNX_RTC_SECS_MASK) {
		/*
		 *  RTC has updated the CURRENT_TIME with the time written into
		 *  SET_TIME_WRITE register
		 */
		readtime = rtc_read32(dev, XLNX_RTC_CUR_TIM_OFFSET);
	} else {
		/*
		 * Time written in SET_TIME_WRITE has not yet updated into
		 * the seconds read register, so read the time from the
		 * SET_TIME_WRITE instead of CURRENT_TIME register.
		 * Since we add +1 sec while writing, we need to -1 sec while
		 * reading.
		 */
		readtime = rtc_read32(dev, XLNX_RTC_SET_TIM_READ_OFFSET) - 1;
	}

	gmtime_r(&readtime, (struct tm *)timeptr);
	timeptr->tm_nsec = 0;
	timeptr->tm_isdst = -1;

	return 0;
}

/**
 * @brief Set the time to RTC
 */
static int xlnx_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	time_t seconds = timeutil_timegm((struct tm *)timeptr);

	/*
	 * The value written will be updated after 1 sec into the
	 * seconds read register, so we need to program time +1 sec
	 * to get the correct time on read.
	 */

	rtc_write32(dev, seconds + 1, XLNX_RTC_SET_TIM_OFFSET);
	rtc_write32(dev, (XLNX_RTC_SECS_MASK), XLNX_RTC_INT_STS_OFFSET);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
/**
 * @brief Check if an RTC alarm is pending
 */
static int xlnx_rtc_alarm_pending(const struct device *dev, uint16_t id)
{
	struct xlnx_rtc_data *data = dev->data;
	int ret = 0;

	K_SPINLOCK(&data->lock) {
		ret = data->alarm_pending ? 1 : 0;
		data->alarm_pending = false;
	}

	return ret;
}

/**
 * @brief Get supported alarm fields
 */
static int xlnx_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					       uint16_t *mask)
{
	ARG_UNUSED(dev);

	*mask = XLNX_RTC_ALARM_TIME_MASK;
	return 0;
}

/**
 * @brief Read the alarm time set in the RTC
 */
static int xlnx_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				   struct rtc_time *timeptr)
{
	struct xlnx_rtc_data *data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	time_t readtime = rtc_read32(dev, XLNX_RTC_ALARM_OFFSET);
	*mask = data->alarm_set_mask;
	k_spin_unlock(&data->lock, key);

	gmtime_r(&readtime, (struct tm *)timeptr);
	LOG_DBG("Get alarm seconds is:%d minute is:%d hour is:%d month is:%d mday is:%d year is:%d",
		timeptr->tm_sec, timeptr->tm_min, timeptr->tm_hour, timeptr->tm_mon,
		timeptr->tm_mday, timeptr->tm_year);

	return 0;
}

/**
 * @brief Set the RTC alarm time
 */
static int xlnx_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	struct xlnx_rtc_data *data = dev->data;
	uint16_t mask_available;
	k_spinlock_key_t key;
	time_t seconds;

	if (mask == 0 || !timeptr) {
		key = k_spin_lock(&data->lock);
		rtc_write32(dev, XLNX_RTC_ALARM_MASK, XLNX_RTC_INT_STS_OFFSET);
		rtc_write32(dev, XLNX_RTC_ALARM_MASK, XLNX_RTC_INT_DIS_OFFSET);
		LOG_DBG("Alarm %d has been cleared and disabled", id);
		goto unlock;
	}

	if (rtc_utils_validate_rtc_time(timeptr, mask) == false) {
		LOG_DBG("Invalid Input Value");
		return -EINVAL;
	}

	(void)xlnx_rtc_alarm_get_supported_fields(NULL, 0, &mask_available);

	if (mask & ~(mask_available)) {
		return -EINVAL;
	}

	data->alarm_set_mask = mask;

	/* Clear and disable the alarm */
	key = k_spin_lock(&data->lock);
	rtc_write32(dev, XLNX_RTC_ALARM_MASK, XLNX_RTC_INT_STS_OFFSET);
	rtc_write32(dev, XLNX_RTC_ALARM_MASK, XLNX_RTC_INT_DIS_OFFSET);

	/* Convert date to seconds */
	LOG_DBG("Set alarm: seconds:%d, minute:%d hour:%d month:%d mday:%d year:%d",
		timeptr->tm_sec, timeptr->tm_min, timeptr->tm_hour, timeptr->tm_mon,
		timeptr->tm_mday, timeptr->tm_year);
	seconds = timeutil_timegm((struct tm *)timeptr);

	rtc_write32(dev, seconds, XLNX_RTC_ALARM_OFFSET);
	rtc_write32(dev, XLNX_RTC_ALARM_MASK, XLNX_RTC_INT_ENA_OFFSET);

unlock:
	k_spin_unlock(&data->lock, key);
	return 0;
}

/**
 * @brief Registers a callback function for the RTC alarm event
 */
static int xlnx_rtc_alarm_callback(const struct device *dev, uint16_t id,
				   rtc_alarm_callback callback, void *user_data)
{
	struct xlnx_rtc_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->alarm_user_callback = callback;
		data->alarm_user_data = user_data;
	}

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_CALIBRATION
/**
 * @brief Sets the RTC calibration offset to adjust clock drift
 */
static int xlnx_rtc_set_offset(const struct device *dev, int32_t offset)
{
	struct xlnx_rtc_data *data = dev->data;
	uint32_t tick_mult = RTC_PPB % data->rtc_clock_freq;
	uint8_t fract_tick = 0;
	uint16_t max_tick;
	uint32_t calibval;
	int fract_offset;

	if (offset < RTC_MIN_OFFSET || offset > RTC_MAX_OFFSET) {
		return -EINVAL;
	}

	max_tick = offset / tick_mult;
	fract_offset = offset % tick_mult;

	if (fract_offset) {
		if (fract_offset < 0) {
			fract_offset += tick_mult;
			max_tick--;
		}
		if (fract_offset > (tick_mult / RTC_FR_MAX_TICKS)) {
			for (fract_tick = 1; fract_tick < 16; fract_tick++) {
				if (fract_offset <= (fract_tick * (tick_mult / RTC_FR_MAX_TICKS))) {
					break;
				}
			}
		}
	}

	calibval = max_tick + RTC_CALIB_DEF;

	if (fract_tick) {
		calibval |= RTC_FR_EN;
	}
	calibval |= (fract_tick << RTC_FR_DATSHIFT);
	rtc_write32(dev, calibval, XLNX_RTC_CALIB_WR_OFFSET);

	return 0;
}

/**
 * @brief Retrieves the current RTC calibration offset
 */
static int xlnx_rtc_get_offset(const struct device *dev, int32_t *offset)
{
	struct xlnx_rtc_data *data = dev->data;
	uint32_t tick_mult = RTC_PPB % data->rtc_clock_freq;
	int32_t offset_val;
	uint32_t calibval;

	calibval = rtc_read32(dev, XLNX_RTC_CALIB_RD_OFFSET);
	/* offset with seconds ticks */
	offset_val = calibval & RTC_TICK_MASK;
	offset_val -= RTC_CALIB_DEF;
	offset_val *= tick_mult;

	/* Offset with fractional ticks */
	if (calibval & RTC_FR_EN) {
		offset_val += ((calibval & RTC_FR_MASK) >> RTC_FR_DATSHIFT) *
				(tick_mult / RTC_FR_MAX_TICKS);
	}

	*offset = offset_val;

	return 0;
}
#endif /* CONFIG_RTC_CALIBRATION */

/**
 * @brief Performs early initialization of the RTC device
 */
static int xlnx_rtc_init(const struct device *dev)
{
	const struct xlnx_rtc_config *pdev = dev->config;
	struct xlnx_rtc_data *data = dev->data;
	uint32_t controlreg;

	pdev->config_func(dev);

	controlreg = rtc_read32(dev, XLNX_RTC_CTL_OFFSET);

	/* set the calibration value in calibration register */
	rtc_write32(dev, data->rtc_clock_freq, XLNX_RTC_CALIB_WR_OFFSET);

	/* set the oscillator and Battery switch enable in control register */
	rtc_write32(dev, (controlreg | XLNX_RTC_BATTERY_EN | XLNX_RTC_OSC_EN), XLNX_RTC_CTL_OFFSET);

	/* Clear the interrupt status */
	rtc_write32(dev, (XLNX_RTC_SECS_MASK | XLNX_RTC_ALARM_MASK), XLNX_RTC_INT_STS_OFFSET);

#if defined(CONFIG_RTC_ALARM)
	data->alarm_user_callback = NULL;
	data->alarm_pending = false;
#endif

	return 0;
}

static const struct rtc_driver_api xlnx_driver_api = {
	.get_time = xlnx_rtc_get_time,
	.set_time = xlnx_rtc_set_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = xlnx_rtc_alarm_get_supported_fields,
	.alarm_is_pending = xlnx_rtc_alarm_pending,
	.alarm_get_time = xlnx_rtc_alarm_get_time,
	.alarm_set_time = xlnx_rtc_alarm_set_time,
	.alarm_set_callback = xlnx_rtc_alarm_callback,
#endif
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = xlnx_rtc_set_offset,
	.get_calibration = xlnx_rtc_get_offset,
#endif
};

#define XLNX_RTC_INTR_CONFIG(inst)								\
	static void rtc_xlnx_irq_config_##inst(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, alarm, irq),				\
			    DT_INST_IRQ_BY_NAME(inst, alarm, priority), rtc_xlnx_isr,		\
			    DEVICE_DT_INST_GET(inst), 0);					\
		irq_enable(DT_INST_IRQ_BY_NAME(inst, alarm, irq));				\
	}

#define XLNX_RTC_INIT(inst)									\
	XLNX_RTC_INTR_CONFIG(inst)								\
	const static struct xlnx_rtc_config xlnx_rtc_config_##inst = {				\
		.regbase = DT_INST_REG_ADDR(inst),						\
		.config_func = &rtc_xlnx_irq_config_##inst					\
	};											\
												\
	static struct xlnx_rtc_data xlnx_rtc_data_##inst = {					\
		.rtc_clock_freq = DT_INST_PROP(inst, clock_frequency),				\
	};											\
												\
	DEVICE_DT_INST_DEFINE(inst, &xlnx_rtc_init, NULL, &xlnx_rtc_data_##inst,		\
			      &xlnx_rtc_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,	\
			      &xlnx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XLNX_RTC_INIT);
