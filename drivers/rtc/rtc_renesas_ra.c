/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_rtc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <cold_start.h>

#include <r_rtc.h>
#include <soc.h>

#include "rtc_utils.h"
#include <stdlib.h>

LOG_MODULE_REGISTER(renesas_ra_rtc, CONFIG_RTC_LOG_LEVEL);

/* Zephyr mask supported by RTC Renesas RA device, values from RTC_ALARM_TIME_MASK */
#define RTC_RENESAS_RA_SUPPORTED_ALARM_FIELDS                                                      \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH |  \
	 RTC_ALARM_TIME_MASK_YEAR)

#define RTC_RENESAS_RA_MAX_ERROR_ADJUSTMENT_VALUE (63)

/* RTC Renesas RA start year: 2000 */
#define RTC_RENESAS_RA_YEAR_REF 2000

/* struct tm start year: 1900 */
#define TM_YEAR_REF 1900

struct rtc_renesas_ra_config {
	void (*irq_config_func)(const struct device *dev);
	const struct device *clock_dev;
#ifdef CONFIG_RTC_ALARM
	uint16_t alarms_count;
#endif
};

struct rtc_renesas_ra_data {
	rtc_instance_ctrl_t fsp_ctrl;
	rtc_cfg_t fsp_cfg;
	rtc_error_adjustment_cfg_t fsp_err_cfg;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_cb;
	void *alarm_cb_data;
	bool is_alarm_pending;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_cb;
	void *update_cb_data;
#endif /* CONFIG_RTC_UPDATE */
};

/* FSP ISR */
extern void rtc_alarm_periodic_isr(void);
extern void rtc_carry_isr(void);

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
static void renesas_ra_rtc_callback(rtc_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	__maybe_unused struct rtc_renesas_ra_data *data = dev->data;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_cb = data->alarm_cb;
	void *alarm_cb_data = data->alarm_cb_data;
#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_cb = data->update_cb;
	void *update_cb_data = data->update_cb_data;
#endif /* CONFIG_RTC_UPDATE */

	if (RTC_EVENT_ALARM_IRQ == p_args->event) {
#ifdef CONFIG_RTC_ALARM
		if (alarm_cb) {
			alarm_cb(dev, 0, alarm_cb_data);
			data->is_alarm_pending = false;
		} else {
			data->is_alarm_pending = true;
		}
#endif /* CONFIG_RTC_ALARM */
	}
#ifdef CONFIG_RTC_UPDATE
	else if (RTC_EVENT_PERIODIC_IRQ == p_args->event) {
		if (update_cb) {
			update_cb(dev, update_cb_data);
		}
	}
#endif /* CONFIG_RTC_UPDATE */
	else {
		LOG_ERR("Invalid callback event");
	}
}
#endif /* CONFIG_RTC_ALARM || CONFIG_RTC_UPDATE */

static int rtc_renesas_ra_init(const struct device *dev)
{
	struct rtc_renesas_ra_data *data = dev->data;
	const struct rtc_renesas_ra_config *config = dev->config;
	const char *clock_dev_name = config->clock_dev->name;
	fsp_err_t fsp_err;
	uint32_t rate;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	if (strcmp(clock_dev_name, "clock-loco") == 0) {
		data->fsp_cfg.clock_source = RTC_CLOCK_SOURCE_LOCO;
		ret = clock_control_get_rate(config->clock_dev, (clock_control_subsys_t)0, &rate);
		if (ret) {
			return ret;
		}

		/* The RTC time counter operates on a 128-Hz clock signal as the base clock.
		 * Therefore, when LOCO is selected, LOCO is divided by the prescaler to
		 * generate a 128-Hz clock signal. Calculation method of frequency
		 * comparison value: (LOCO clock frequency) / 128 - 1
		 */
		data->fsp_cfg.freq_compare_value = (rate / 128) - 1;
	} else {
		data->fsp_cfg.clock_source = RTC_CLOCK_SOURCE_SUBCLK;
	}

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
	data->fsp_cfg.p_callback = renesas_ra_rtc_callback;

#if defined(CONFIG_RTC_ALARM)
	data->alarm_cb = NULL;
	data->alarm_cb_data = NULL;
	data->is_alarm_pending = false;
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE)
	data->update_cb = NULL;
	data->update_cb_data = NULL;
#endif /* CONFIG_RTC_UPDATE */

#else
	data->fsp_cfg.p_callback = NULL;
#endif

	fsp_err = R_RTC_Open(&data->fsp_ctrl, &data->fsp_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to initialize the device");
		return -EIO;
	}

#if defined(CONFIG_RENESAS_RA_BATTERY_BACKUP_MANUAL_CONFIGURE)
	if (is_backup_domain_reset_happen()) {
		R_RTC_ClockSourceSet(&data->fsp_ctrl);
	}
#else
	if (is_power_on_reset_happen()) {
		R_RTC_ClockSourceSet(&data->fsp_ctrl);
	}
#endif /* CONFIG_RENESAS_RA_BATTERY_BACKUP_MANUAL_CONFIGURE */

#ifdef CONFIG_RTC_UPDATE
	fsp_err = R_RTC_PeriodicIrqRateSet(&data->fsp_ctrl, RTC_PERIODIC_IRQ_SELECT_1_SECOND);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Failed to configure update interrupt");
		return -EIO;
	}
#endif /* CONFIG_RTC_UPDATE */

	config->irq_config_func(dev);

	return 0;
}

static int rtc_renesas_ra_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err;

	if (timeptr == NULL) {
		LOG_ERR("No pointer is provided to set time");
		return -EINVAL;
	}

	if (timeptr->tm_year + TM_YEAR_REF < RTC_RENESAS_RA_YEAR_REF) {
		LOG_ERR("RTC time exceeds HW capabilities. Year must be 2000-2099");
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, RTC_RENESAS_RA_SUPPORTED_ALARM_FIELDS)) {
		LOG_ERR("RTC time is invalid");
		return -EINVAL;
	}

	fsp_err = R_RTC_CalendarTimeSet(&data->fsp_ctrl, (struct tm *const)timeptr);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Time set operation was not successful.");
		return -EIO;
	}

	return 0;
}

static int rtc_renesas_ra_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err;
	rtc_info_t rtc_info;

	if (timeptr == NULL) {
		LOG_ERR("Pointer provided to store the requested time is NULL");
		return -EINVAL;
	}

	fsp_err = R_RTC_InfoGet(&data->fsp_ctrl, &rtc_info);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	if (rtc_info.status != RTC_STATUS_RUNNING) {
		LOG_ERR("RTC time has not been set");
		return -ENODATA;
	}

	fsp_err = R_RTC_CalendarTimeGet(&data->fsp_ctrl, rtc_time_to_tm(timeptr));
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	/* value for unsupported fields */
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int rtc_renesas_ra_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						     uint16_t *mask)
{
	const struct rtc_renesas_ra_config *config = dev->config;

	if (mask == NULL) {
		LOG_ERR("Mask pointer is NULL");
		return -EINVAL;
	}

	if (id > config->alarms_count) {
		LOG_ERR("Invalid alarm ID %d", id);
		return -EINVAL;
	}

	*mask = (uint16_t)RTC_RENESAS_RA_SUPPORTED_ALARM_FIELDS;

	return 0;
}

#define ALARM_FIELD_CHECK_ENABLE(mask, field) (mask & field)

static int rtc_renesas_ra_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
					 const struct rtc_time *timeptr)
{
	struct rtc_renesas_ra_data *data = dev->data;
	const struct rtc_renesas_ra_config *config = dev->config;
	fsp_err_t fsp_err;
	rtc_alarm_time_t fsp_alarm_cfg;

	if ((timeptr == NULL) && (mask != 0)) {
		LOG_ERR("No pointer is provided to set alarm");
		return -EINVAL;
	}

	if (id > config->alarms_count) {
		LOG_ERR("Invalid alarm ID %d", id);
		return -EINVAL;
	}

	if (mask & ~RTC_RENESAS_RA_SUPPORTED_ALARM_FIELDS) {
		LOG_ERR("Invalid alarm mask");
		return -EINVAL;
	}

	if (mask > 0) {
		if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
			LOG_ERR("Invalid alarm fields values");
			return -EINVAL;
		}

		fsp_alarm_cfg.time.tm_sec = timeptr->tm_sec;
		fsp_alarm_cfg.time.tm_min = timeptr->tm_min;
		fsp_alarm_cfg.time.tm_hour = timeptr->tm_hour;
		fsp_alarm_cfg.time.tm_mday = timeptr->tm_mday;
		fsp_alarm_cfg.time.tm_mon = timeptr->tm_mon;
		fsp_alarm_cfg.time.tm_year = timeptr->tm_year;
		fsp_alarm_cfg.time.tm_wday = timeptr->tm_wday;
	}

	fsp_alarm_cfg.channel = id;
	fsp_alarm_cfg.sec_match = ALARM_FIELD_CHECK_ENABLE(mask, RTC_ALARM_TIME_MASK_SECOND);
	fsp_alarm_cfg.min_match = ALARM_FIELD_CHECK_ENABLE(mask, RTC_ALARM_TIME_MASK_MINUTE);
	fsp_alarm_cfg.hour_match = ALARM_FIELD_CHECK_ENABLE(mask, RTC_ALARM_TIME_MASK_HOUR);
	fsp_alarm_cfg.mday_match = ALARM_FIELD_CHECK_ENABLE(mask, RTC_ALARM_TIME_MASK_MONTHDAY);
	fsp_alarm_cfg.mon_match = ALARM_FIELD_CHECK_ENABLE(mask, RTC_ALARM_TIME_MASK_MONTH);
	fsp_alarm_cfg.year_match = ALARM_FIELD_CHECK_ENABLE(mask, RTC_ALARM_TIME_MASK_YEAR);
	fsp_alarm_cfg.dayofweek_match = ALARM_FIELD_CHECK_ENABLE(mask, RTC_ALARM_TIME_MASK_WEEKDAY);

	fsp_err = R_RTC_CalendarAlarmSet(&data->fsp_ctrl, &fsp_alarm_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Alarm time set is not successful!");
		return -EIO;
	}

	return 0;
}

static int rtc_renesas_ra_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
					 struct rtc_time *timeptr)
{
	struct rtc_renesas_ra_data *data = dev->data;
	const struct rtc_renesas_ra_config *config = dev->config;
	fsp_err_t fsp_err;
	rtc_alarm_time_t fsp_alarm_cfg;

	if ((mask == NULL) || (timeptr == NULL)) {
		LOG_ERR("No pointer is provided to store the requested alarm time/mask");
		return -EINVAL;
	}

	if (id > config->alarms_count) {
		LOG_ERR("Invalid alarm ID %d", id);
		return -EINVAL;
	}

	fsp_err = R_RTC_CalendarAlarmGet(&data->fsp_ctrl, &fsp_alarm_cfg);
	if (fsp_err != FSP_SUCCESS) {
		LOG_ERR("Alarm time get is not successful!");
		return -EIO;
	}

	timeptr->tm_sec = fsp_alarm_cfg.time.tm_sec;
	timeptr->tm_min = fsp_alarm_cfg.time.tm_min;
	timeptr->tm_hour = fsp_alarm_cfg.time.tm_hour;
	timeptr->tm_mday = fsp_alarm_cfg.time.tm_mday;
	timeptr->tm_mon = fsp_alarm_cfg.time.tm_mon;
	timeptr->tm_year = fsp_alarm_cfg.time.tm_year;
	timeptr->tm_wday = fsp_alarm_cfg.time.tm_wday;

	/* value for unsupported fields */
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;

	*mask = 0;

	if (fsp_alarm_cfg.sec_match) {
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
	}
	if (fsp_alarm_cfg.min_match) {
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if (fsp_alarm_cfg.hour_match) {
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if (fsp_alarm_cfg.mday_match) {
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}
	if (fsp_alarm_cfg.mon_match) {
		*mask |= RTC_ALARM_TIME_MASK_MONTH;
	}
	if (fsp_alarm_cfg.year_match) {
		*mask |= RTC_ALARM_TIME_MASK_YEAR;
	}
	if (fsp_alarm_cfg.dayofweek_match) {
		*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	return 0;
}

static int rtc_renesas_ra_alarm_set_callback(const struct device *dev, uint16_t id,
					     rtc_alarm_callback callback, void *user_data)
{
	struct rtc_renesas_ra_data *data = dev->data;
	const struct rtc_renesas_ra_config *config = dev->config;
	unsigned int key;

	if (id > config->alarms_count) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	key = irq_lock();
	data->alarm_cb = callback;
	data->alarm_cb_data = user_data;
	irq_unlock(key);

	return 0;
}

static int rtc_renesas_ra_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_renesas_ra_data *data = dev->data;
	const struct rtc_renesas_ra_config *config = dev->config;
	unsigned int key;
	int ret;

	if (!(id < config->alarms_count)) {
		LOG_ERR("invalid alarm ID %d", id);
		return -EINVAL;
	}

	key = irq_lock();
	ret = data->is_alarm_pending ? 1 : 0;
	data->is_alarm_pending = false;
	irq_unlock(key);

	return ret;
}

#endif /* CONFIG_RTC_ALARM */

#ifdef CONFIG_RTC_UPDATE
static int rtc_renesas_ra_update_set_callback(const struct device *dev,
					      rtc_update_callback callback, void *user_data)
{
	struct rtc_renesas_ra_data *data = dev->data;
	unsigned int key;

	key = irq_lock();
	data->update_cb = callback;
	data->update_cb_data = user_data;
	irq_unlock(key);

	return 0;
}
#endif /* CONFIG_RTC_UPDATE */

#ifdef CONFIG_RTC_CALIBRATION

/* Convert number of clock cycles added or removed in 10 seconds or 1 minute
 * For each 10 second (total 327,680 clock cycles):
 * ppb = cycles * 10^9 / total_cycles = cycles * 10^9  / 327,680
 *     = cycles * 390625 / 128
 */
#define CYCLES_TO_PPB_EACH_10_SECOND(cycles) DIV_ROUND_CLOSEST((cycles) * 390625, 128)

/* For each 1 minute (total 1,966,080 clock cycles):
 * ppb = cycles * 10^9 / total_cycles = cycles * 10^9 / 1,966,080
 *     = cycles * 390625 / 768
 */
#define CYCLES_TO_PPB_EACH_1_MINUTE(cycles) DIV_ROUND_CLOSEST((cycles) * 390625, 768)

/* Convert part per billion calibration value to a number of clock cycles added or removed
 * to part per billion calibration value.
 * For each 10 second (total 327,680 clock cycles):
 * cycles = ppb * total_cycles / 10^9 = ppb * 327,680 / 10^9
 *           = ppb * 128 / 390625
 */
#define PPB_TO_CYCLES_PER_10_SECOND(ppb) DIV_ROUND_CLOSEST((ppb) * 128, 390625)
/*
 * For each 1 minute (total 1,966,080 clock cycles):
 * cycles = ppb * total_cycles / 10^9 = ppb * 1,966,080 / 10^9
 *           = ppb * 768 / 390625
 */
#define PPB_TO_CYCLES_PER_1_MINUTE(ppb)  DIV_ROUND_CLOSEST((ppb) * 768, 390625)

static int rtc_renesas_ra_set_calibration(const struct device *dev, int32_t calibration)
{
	struct rtc_renesas_ra_data *data = dev->data;
	fsp_err_t fsp_err;
	uint32_t adjustment_cycles = 0;
	uint32_t adjustment_cycles_ten_seconds = 0;
	uint32_t adjustment_cycles_one_minute = 0;
	int32_t abs_calibration = abs(calibration);

	/* Calibration is not available while using the LOCO clock */
	if (data->fsp_cfg.clock_source == RTC_CLOCK_SOURCE_LOCO) {
		LOG_DBG("Calibration is not available while using the LOCO clock");
		return -ENOTSUP;
	}

	if (calibration == 0) {
		data->fsp_err_cfg.adjustment_type = RTC_ERROR_ADJUSTMENT_NONE;
		data->fsp_err_cfg.adjustment_value = 0;
	} else {
		adjustment_cycles_ten_seconds = PPB_TO_CYCLES_PER_10_SECOND(abs_calibration);
		adjustment_cycles_one_minute = PPB_TO_CYCLES_PER_1_MINUTE(abs_calibration);

		if ((adjustment_cycles_ten_seconds > RTC_RENESAS_RA_MAX_ERROR_ADJUSTMENT_VALUE) &&
		    (adjustment_cycles_one_minute > RTC_RENESAS_RA_MAX_ERROR_ADJUSTMENT_VALUE)) {
			LOG_ERR("Calibration out of HW range");
			return -EINVAL;
		}

		/* 1 minute period has low range part per bilion than 10 minute period when transfer
		 * from ppb to cycles. So check it first.
		 */
		if (adjustment_cycles_one_minute > RTC_RENESAS_RA_MAX_ERROR_ADJUSTMENT_VALUE) {
			adjustment_cycles = adjustment_cycles_ten_seconds;
			data->fsp_err_cfg.adjustment_period = RTC_ERROR_ADJUSTMENT_PERIOD_10_SECOND;
		} else {
			int32_t err_ten_seconds =
				abs(CYCLES_TO_PPB_EACH_10_SECOND(adjustment_cycles_ten_seconds) -
				    abs_calibration);
			int32_t err_one_minute =
				abs(CYCLES_TO_PPB_EACH_1_MINUTE(adjustment_cycles_one_minute) -
				    abs_calibration);
			LOG_DBG("10 seconds error: %d; 1 minute error: %d", err_ten_seconds,
				err_one_minute);

			if (err_one_minute < err_ten_seconds) {
				data->fsp_err_cfg.adjustment_period =
					RTC_ERROR_ADJUSTMENT_PERIOD_1_MINUTE;
				adjustment_cycles = adjustment_cycles_one_minute;
			} else {
				data->fsp_err_cfg.adjustment_period =
					RTC_ERROR_ADJUSTMENT_PERIOD_10_SECOND;
				adjustment_cycles = adjustment_cycles_ten_seconds;
			}
		}

		data->fsp_err_cfg.adjustment_type =
			(calibration > 0) ? RTC_ERROR_ADJUSTMENT_ADD_PRESCALER
					  : RTC_ERROR_ADJUSTMENT_SUBTRACT_PRESCALER;
		data->fsp_err_cfg.adjustment_value = adjustment_cycles;
	}

	data->fsp_err_cfg.adjustment_mode = RTC_ERROR_ADJUSTMENT_MODE_AUTOMATIC;
	fsp_err = R_RTC_ErrorAdjustmentSet(&data->fsp_ctrl, &data->fsp_err_cfg);
	if (fsp_err != FSP_SUCCESS) {
		return -EIO;
	}

	return 0;
}

#endif /* CONFIG_RTC_CALIBRATION */

static DEVICE_API(rtc, rtc_renesas_ra_driver_api) = {
	.set_time = rtc_renesas_ra_set_time,
	.get_time = rtc_renesas_ra_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_renesas_ra_alarm_get_supported_fields,
	.alarm_set_time = rtc_renesas_ra_alarm_set_time,
	.alarm_get_time = rtc_renesas_ra_alarm_get_time,
	.alarm_set_callback = rtc_renesas_ra_alarm_set_callback,
	.alarm_is_pending = rtc_renesas_ra_alarm_is_pending,
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	.update_set_callback = rtc_renesas_ra_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#ifdef CONFIG_RTC_CALIBRATION
	.set_calibration = rtc_renesas_ra_set_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

#define RTC_RENESAS_RA_ALARM_COUNT_GET(index)                                                      \
	IF_ENABLED(CONFIG_RTC_ALARM,								   \
		(.alarms_count = DT_INST_PROP(index, alarms_count),))

#define RTC_RENESAS_RA_CALIBRATION_MODE                                                            \
	COND_CODE_1(CONFIG_RTC_CALIBRATION, (RTC_ERROR_ADJUSTMENT_MODE_AUTOMATIC),		   \
		    (RTC_ERROR_ADJUSTMENT_MODE_MANUAL))

#define RTC_RENESAS_RA_CALIBRATION_PERIOD                                                          \
	COND_CODE_1(CONFIG_RTC_CALIBRATION,  (RTC_ERROR_ADJUSTMENT_PERIOD_1_MINUTE),		   \
		(RTC_ERROR_ADJUSTMENT_PERIOD_NONE))

#define RTC_RENESAS_RA_IRQ_GET(id, name, cell)                                                     \
	COND_CODE_1(DT_INST_IRQ_HAS_NAME(id, name),                                                \
		(DT_INST_IRQ_BY_NAME(id, name, cell)),                                             \
		((IRQn_Type) BSP_IRQ_DISABLED))

#define ALARM_IRQ_ENABLE(index)                                                                    \
	R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, alm, irq)] = ELC_EVENT_RTC_ALARM;                  \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, alm, irq),                                          \
		    DT_INST_IRQ_BY_NAME(index, alm, priority), rtc_alarm_periodic_isr, (NULL), 0);

#define PERODIC_IRQ_ENABLE(index)                                                                  \
	R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, prd, irq)] = ELC_EVENT_RTC_PERIOD;                 \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, prd, irq),                                          \
		    DT_INST_IRQ_BY_NAME(index, prd, priority), rtc_alarm_periodic_isr, (NULL), 0);

#define ALARM_IRQ_INIT(index)   IF_ENABLED(CONFIG_RTC_ALARM, (ALARM_IRQ_ENABLE(index)))
#define PERODIC_IRQ_INIT(index) IF_ENABLED(CONFIG_RTC_UPDATE, (PERODIC_IRQ_ENABLE(index)))

#define RTC_RENESAS_RA_INIT(index)                                                                 \
	static void rtc_renesas_ra_irq_config_func##index(const struct device *dev)                \
	{                                                                                          \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(index, cup, irq)] = ELC_EVENT_RTC_CARRY;          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(index, cup, irq),                                  \
			    DT_INST_IRQ_BY_NAME(index, cup, priority), rtc_carry_isr, (NULL), 0);  \
		irq_enable(DT_INST_IRQ_BY_NAME(index, cup, irq));                                  \
                                                                                                   \
		ALARM_IRQ_INIT(index)                                                              \
		PERODIC_IRQ_INIT(index)                                                            \
	}                                                                                          \
	static const struct rtc_renesas_ra_config rtc_renesas_ra_config_##index = {                \
		.irq_config_func = rtc_renesas_ra_irq_config_func##index,                          \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)),                            \
		RTC_RENESAS_RA_ALARM_COUNT_GET(index)};                                            \
	static struct rtc_renesas_ra_data rtc_renesas_ra_data_##index = {                          \
		.fsp_err_cfg =                                                                     \
			{                                                                          \
				.adjustment_mode = RTC_RENESAS_RA_CALIBRATION_MODE,                \
				.adjustment_period = RTC_RENESAS_RA_CALIBRATION_PERIOD,            \
				.adjustment_type = RTC_ERROR_ADJUSTMENT_NONE,                      \
				.adjustment_value = 0x00,                                          \
			},                                                                         \
		.fsp_cfg =                                                                         \
			{                                                                          \
				.p_err_cfg = &rtc_renesas_ra_data_##index.fsp_err_cfg,             \
				.alarm_irq = RTC_RENESAS_RA_IRQ_GET(index, alm, irq),              \
				.alarm_ipl = RTC_RENESAS_RA_IRQ_GET(index, alm, priority),         \
				.periodic_irq = RTC_RENESAS_RA_IRQ_GET(index, prd, irq),           \
				.periodic_ipl = RTC_RENESAS_RA_IRQ_GET(index, prd, priority),      \
				.carry_irq = RTC_RENESAS_RA_IRQ_GET(index, cup, irq),              \
				.carry_ipl = RTC_RENESAS_RA_IRQ_GET(index, cup, priority),         \
				.p_context = (void *)DEVICE_DT_INST_GET(index),                    \
				.p_extend = NULL,                                                  \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(index, &rtc_renesas_ra_init, NULL, &rtc_renesas_ra_data_##index,     \
			      &rtc_renesas_ra_config_##index, PRE_KERNEL_1,                        \
			      CONFIG_RTC_INIT_PRIORITY, &rtc_renesas_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_RENESAS_RA_INIT)
