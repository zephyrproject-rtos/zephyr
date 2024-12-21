/*
 * Copyright (c) 2023 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/sys/util.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/smartbond_clock_control.h>
#include <DA1469xAB.h>
#include <da1469x_config.h>
#include <da1469x_pdc.h>
#include "rtc_utils.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rtc_smartbond, CONFIG_RTC_LOG_LEVEL);

#define DT_DRV_COMPAT  renesas_smartbond_rtc

#define SMARTBOND_IRQN       DT_INST_IRQN(0)
#define SMARTBOND_IRQ_PRIO   DT_INST_IRQ(0, priority)

#define RTC_ALARMS_COUNT     DT_PROP(DT_NODELABEL(rtc), alarms_count)

#define TM_YEAR_REF          1900
#define RTC_DIV_DENOM_1000   0
#define RTC_DIV_DENOM_1024   1

#define RTC_SMARTBOND_SUPPORTED_ALARM_FIELDS \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | \
	 RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY)

#define RTC_TIME_REG_SET_FIELD(_field, _var, _val) \
	((_var) = \
	((_var) & ~(RTC_RTC_TIME_REG_RTC_TIME_ ## _field ## _T_Msk | \
	RTC_RTC_TIME_REG_RTC_TIME_ ## _field ## _U_Msk)) | \
	(((_val) << RTC_RTC_TIME_REG_RTC_TIME_ ## _field ## _U_Pos) & \
	(RTC_RTC_TIME_REG_RTC_TIME_ ## _field ## _T_Msk | \
	 RTC_RTC_TIME_REG_RTC_TIME_ ## _field ## _U_Msk)))

#define RTC_CALENDAR_REG_SET_FIELD(_field, _var, _val) \
	((_var) = \
	((_var) & ~(RTC_RTC_CALENDAR_REG_RTC_CAL_ ## _field ## _T_Msk | \
	RTC_RTC_CALENDAR_REG_RTC_CAL_ ## _field ## _U_Msk)) | \
	(((_val) << RTC_RTC_CALENDAR_REG_RTC_CAL_ ## _field ## _U_Pos) & \
	(RTC_RTC_CALENDAR_REG_RTC_CAL_ ## _field ## _T_Msk | \
	 RTC_RTC_CALENDAR_REG_RTC_CAL_ ## _field ## _U_Msk)))

#define RTC_CALENDAR_ALARM_REG_SET_FIELD(_field, _var, _val) \
	((_var) = \
	((_var) & ~(RTC_RTC_CALENDAR_ALARM_REG_RTC_CAL_ ## _field ## _T_Msk | \
	RTC_RTC_CALENDAR_ALARM_REG_RTC_CAL_ ## _field ## _U_Msk)) | \
	(((_val) << RTC_RTC_CALENDAR_ALARM_REG_RTC_CAL_ ## _field ## _U_Pos) & \
	(RTC_RTC_CALENDAR_ALARM_REG_RTC_CAL_ ## _field ## _T_Msk | \
	 RTC_RTC_CALENDAR_ALARM_REG_RTC_CAL_ ## _field ## _U_Msk)))

#define RTC_TIME_ALARM_REG_SET_FIELD(_field, _var, _val) \
	((_var) = \
	((_var) & ~(RTC_RTC_TIME_ALARM_REG_RTC_TIME_ ## _field ## _T_Msk | \
	RTC_RTC_TIME_ALARM_REG_RTC_TIME_ ## _field ## _U_Msk)) | \
	(((_val) << RTC_RTC_TIME_ALARM_REG_RTC_TIME_ ## _field ## _U_Pos) & \
	(RTC_RTC_TIME_ALARM_REG_RTC_TIME_ ## _field ## _T_Msk | \
	 RTC_RTC_TIME_ALARM_REG_RTC_TIME_ ## _field ## _U_Msk)))

#define RTC_TIME_REG_GET_FIELD(_field, _var) \
	(((_var) & (RTC_RTC_TIME_REG_RTC_TIME_ ## _field ## _T_Msk | \
	RTC_RTC_TIME_REG_RTC_TIME_ ## _field ## _U_Msk)) >> \
	RTC_RTC_TIME_REG_RTC_TIME_ ## _field ## _U_Pos)

#define RTC_CALENDAR_REG_GET_FIELD(_field, _var) \
	(((_var) & (RTC_RTC_CALENDAR_REG_RTC_CAL_ ## _field ## _T_Msk | \
	RTC_RTC_CALENDAR_REG_RTC_CAL_ ## _field ## _U_Msk)) >> \
	RTC_RTC_CALENDAR_REG_RTC_CAL_ ## _field ## _U_Pos)

#define RTC_CALENDAR_ALARM_REG_GET_FIELD(_field, _var) \
	(((_var) & (RTC_RTC_CALENDAR_ALARM_REG_RTC_CAL_ ## _field ## _T_Msk | \
	RTC_RTC_CALENDAR_ALARM_REG_RTC_CAL_ ## _field ## _U_Msk)) >> \
	RTC_RTC_CALENDAR_ALARM_REG_RTC_CAL_ ## _field ## _U_Pos)

#define RTC_TIME_ALARM_REG_GET_FIELD(_field, _var) \
	(((_var) & (RTC_RTC_TIME_ALARM_REG_RTC_TIME_ ## _field ## _T_Msk | \
	RTC_RTC_TIME_ALARM_REG_RTC_TIME_ ## _field ## _U_Msk)) >> \
	RTC_RTC_TIME_ALARM_REG_RTC_TIME_ ## _field ## _U_Pos)

#define CLK_RTCDIV_REG_SET_FIELD(_field, _var, _val) \
	((_var) = \
	((_var) & ~CRG_TOP_CLK_RTCDIV_REG_RTC_DIV_ ## _field ## _Msk) | \
	(((_val) << CRG_TOP_CLK_RTCDIV_REG_RTC_DIV_ ## _field ## _Pos) & \
	CRG_TOP_CLK_RTCDIV_REG_RTC_DIV_ ## _field ## _Msk))

struct rtc_smartbond_data {
	struct k_mutex lock;
	bool is_rtc_configured;
#if defined(CONFIG_RTC_ALARM)
	volatile bool is_alarm_pending;
	rtc_alarm_callback alarm_cb;
	void *alarm_user_data;
#endif
#if defined(CONFIG_RTC_UPDATE)
	rtc_update_callback update_cb;
	void *update_user_data;
#endif
};

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
static void smartbond_rtc_isr(const struct device *dev)
{
	struct rtc_smartbond_data *data = dev->data;
	/* Exercise which events asserted the RTC IRQ line. Register is cleared upon read. */
	uint32_t rtc_event_flags_reg = RTC->RTC_EVENT_FLAGS_REG;
	/* RTC_EVENT_FLASH_REG will be updated regardless of the interrupt mask. */
	uint32_t rtc_interrupt_mask_reg = RTC->RTC_INTERRUPT_MASK_REG;

#if defined(CONFIG_RTC_ALARM)
	if ((rtc_event_flags_reg & RTC_RTC_EVENT_FLAGS_REG_RTC_EVENT_ALRM_Msk) &&
		!(rtc_interrupt_mask_reg & RTC_RTC_INTERRUPT_MASK_REG_RTC_ALRM_INT_MSK_Msk)) {
		if (data->alarm_cb) {
			data->alarm_cb(dev, 0, data->alarm_user_data);
			data->is_alarm_pending = false;
		} else {
			data->is_alarm_pending = true;
		}
	}
#endif

#if defined(CONFIG_RTC_UPDATE)
	if ((rtc_event_flags_reg & RTC_RTC_EVENT_FLAGS_REG_RTC_EVENT_SEC_Msk) &&
		!(rtc_interrupt_mask_reg & RTC_RTC_INTERRUPT_MASK_REG_RTC_SEC_INT_MSK_Msk)) {
		if (data->update_cb) {
			data->update_cb(dev, data->update_user_data);
		}
	}
#endif
}
#endif

static inline void rtc_smartbond_set_status(bool status)
{
	if (status) {
		CRG_TOP->CLK_RTCDIV_REG |= CRG_TOP_CLK_RTCDIV_REG_RTC_DIV_ENABLE_Msk;
		RTC->RTC_CONTROL_REG = 0;
	} else {
		RTC->RTC_CONTROL_REG = (RTC_RTC_CONTROL_REG_RTC_CAL_DISABLE_Msk |
							RTC_RTC_CONTROL_REG_RTC_TIME_DISABLE_Msk);
		CRG_TOP->CLK_RTCDIV_REG &= ~CRG_TOP_CLK_RTCDIV_REG_RTC_DIV_ENABLE_Msk;
	}
}

static uint32_t rtc_time_to_bcd(const struct rtc_time *timeptr)
{
	uint32_t rtc_time_reg = 0;

	RTC_TIME_REG_SET_FIELD(S, rtc_time_reg, bin2bcd(timeptr->tm_sec)); /*[0, 59]*/
	RTC_TIME_REG_SET_FIELD(M, rtc_time_reg, bin2bcd(timeptr->tm_min)); /*[0, 59]*/
	RTC_TIME_REG_SET_FIELD(HR, rtc_time_reg, bin2bcd(timeptr->tm_hour)); /*[0, 23]*/

	return rtc_time_reg;
}

static uint32_t rtc_calendar_to_bcd(const struct rtc_time *timeptr)
{
	uint32_t rtc_calendar_reg = 0;

	RTC_CALENDAR_REG_SET_FIELD(D, rtc_calendar_reg, bin2bcd(timeptr->tm_mday)); /*[1, 31]*/
	RTC_CALENDAR_REG_SET_FIELD(Y, rtc_calendar_reg,
		bin2bcd((timeptr->tm_year + TM_YEAR_REF) % 100)); /*[year - 1900]*/
	RTC_CALENDAR_REG_SET_FIELD(C, rtc_calendar_reg,
		bin2bcd((timeptr->tm_year + TM_YEAR_REF) / 100));
	RTC_CALENDAR_REG_SET_FIELD(M, rtc_calendar_reg, bin2bcd(timeptr->tm_mon + 1)); /*[0, 11]*/

	if (timeptr->tm_wday != -1) {
		rtc_calendar_reg |= ((timeptr->tm_wday + 1) &
			RTC_RTC_CALENDAR_REG_RTC_DAY_Msk); /*[0, 6]*/
	}

	return rtc_calendar_reg;
}

static void bcd_to_rtc_time(struct rtc_time *timeptr)
{
	uint32_t rtc_time_reg = RTC->RTC_TIME_REG;

	timeptr->tm_sec = bcd2bin(RTC_TIME_REG_GET_FIELD(S, rtc_time_reg));
	timeptr->tm_min = bcd2bin(RTC_TIME_REG_GET_FIELD(M, rtc_time_reg));
	timeptr->tm_hour = bcd2bin(RTC_TIME_REG_GET_FIELD(HR, rtc_time_reg));

	timeptr->tm_nsec = 0; /*Unknown*/
}

static void bcd_to_rtc_calendar(struct rtc_time *timeptr)
{
	uint32_t rtc_calendar_reg = RTC->RTC_CALENDAR_REG;

	timeptr->tm_mday = bcd2bin(RTC_CALENDAR_REG_GET_FIELD(D, rtc_calendar_reg));
	timeptr->tm_mon = bcd2bin(RTC_CALENDAR_REG_GET_FIELD(M, rtc_calendar_reg)) - 1;
	timeptr->tm_year = bcd2bin(RTC_CALENDAR_REG_GET_FIELD(Y, rtc_calendar_reg)) +
	    (bcd2bin(RTC_CALENDAR_REG_GET_FIELD(C, rtc_calendar_reg)) * 100) - TM_YEAR_REF;
	timeptr->tm_wday = (rtc_calendar_reg & RTC_RTC_CALENDAR_REG_RTC_DAY_Msk) - 1;

	timeptr->tm_yday = timeptr->tm_isdst = -1; /*Unknown*/
}

static int rtc_smartbond_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_smartbond_data *data = dev->data;
	int ret = 0;
	uint32_t rtc_time_reg, rtc_calendar_reg, rtc_status_reg;

	if (timeptr == NULL) {
		LOG_ERR("No pointer is provided to set time");
		return -EINVAL;
	}

	if (timeptr->tm_year + TM_YEAR_REF < TM_YEAR_REF) {
		LOG_ERR("RTC time exceeds HW capabilities");
		return -EINVAL;
	}

	if ((timeptr->tm_yday != -1) || (timeptr->tm_isdst != -1) || (timeptr->tm_nsec != 0)) {
		LOG_WRN("Unsupported RTC sub-values");
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	rtc_smartbond_set_status(false);

	/* Store current counter values as it might happen that the requested time is not valid */
	rtc_time_reg = RTC->RTC_TIME_REG;
	rtc_calendar_reg = RTC->RTC_CALENDAR_REG;

	RTC->RTC_TIME_REG = rtc_time_to_bcd(timeptr);
	RTC->RTC_CALENDAR_REG = rtc_calendar_to_bcd(timeptr);

	/* Check if the new values were valid, otherwise reset back to the previous ones. */
	rtc_status_reg = RTC->RTC_STATUS_REG;
	if (!(rtc_status_reg & RTC_RTC_STATUS_REG_RTC_VALID_CAL_Msk) ||
		 !(rtc_status_reg & RTC_RTC_STATUS_REG_RTC_VALID_TIME_Msk)) {
		RTC->RTC_TIME_REG = rtc_time_reg;
		RTC->RTC_CALENDAR_REG = rtc_calendar_reg;
		ret = -EINVAL;
	}

	/* Mark the very first valid RTC configuration; used to check if RTC contains valid data. */
	if (!data->is_rtc_configured && (ret == 0)) {
		data->is_rtc_configured = true;
	}

	/* It might happen that the very first time RTC is not configured correctly; do not care. */
	rtc_smartbond_set_status(true);
	k_mutex_unlock(&data->lock);

	return ret;
}

static int rtc_smartbond_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_smartbond_data *data = dev->data;

	if (timeptr == NULL) {
		LOG_ERR("No pointer is provided to store the requested time");
		return -EINVAL;
	}

	if (!data->is_rtc_configured) {
		LOG_WRN("RTC is not initialized yet");
	}

	k_mutex_lock(&data->lock, K_FOREVER);
	/* Stop RTC counters to obtain coherent data. */
	rtc_smartbond_set_status(false);

	bcd_to_rtc_time(timeptr);
	bcd_to_rtc_calendar(timeptr);

	rtc_smartbond_set_status(true);
	k_mutex_unlock(&data->lock);

	return 0;
}

#if defined(CONFIG_RTC_ALARM)
BUILD_ASSERT(RTC_ALARMS_COUNT, "At least one alarm event should be supported");

/*
 * Parse only the alarm fields indicated by the mask. Default valid values should be assigned
 * to unused fields as it might happen that application has provided with invalid values.
 */
static uint32_t alarm_calendar_to_bcd(const struct rtc_time *timeptr, uint16_t mask)
{
	uint32_t rtc_calendar_alarm_reg = 0x0108;

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		RTC_CALENDAR_ALARM_REG_SET_FIELD(D, rtc_calendar_alarm_reg,
					bin2bcd(timeptr->tm_mday));
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		RTC_CALENDAR_ALARM_REG_SET_FIELD(M, rtc_calendar_alarm_reg,
					bin2bcd(timeptr->tm_mon + 1));
	}

	return rtc_calendar_alarm_reg;
}

/*
 * Parse only the alarm fields indicated by the mask. Default valid values should be assigned
 * to unused fields as it might happen that application has provided with invalid values.
 */
static inline uint32_t alarm_time_to_bcd(const struct rtc_time *timeptr, uint16_t mask)
{
	uint32_t rtc_time_alarm_reg = 0;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		/*[0, 59]*/
		RTC_TIME_ALARM_REG_SET_FIELD(S, rtc_time_alarm_reg, bin2bcd(timeptr->tm_sec));
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		/*[0, 59]*/
		RTC_TIME_ALARM_REG_SET_FIELD(M, rtc_time_alarm_reg, bin2bcd(timeptr->tm_min));
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		/*[0, 23]*/
		RTC_TIME_ALARM_REG_SET_FIELD(HR, rtc_time_alarm_reg, bin2bcd(timeptr->tm_hour));
	}

	return rtc_time_alarm_reg;
}

static void bcd_to_alarm_calendar(struct rtc_time *timeptr)
{
	uint32_t rtc_calendar_alarm_reg = RTC->RTC_CALENDAR_ALARM_REG;

	timeptr->tm_mday = bcd2bin(RTC_CALENDAR_ALARM_REG_GET_FIELD(D, rtc_calendar_alarm_reg));
	timeptr->tm_mon = bcd2bin(RTC_CALENDAR_ALARM_REG_GET_FIELD(M, rtc_calendar_alarm_reg)) - 1;

	timeptr->tm_yday = timeptr->tm_wday = timeptr->tm_isdst = timeptr->tm_year = -1;
}

static void bcd_to_alarm_time(struct rtc_time *timeptr)
{
	uint32_t rtc_time_alarm_reg = RTC->RTC_TIME_ALARM_REG;

	timeptr->tm_sec = bcd2bin(RTC_TIME_ALARM_REG_GET_FIELD(S, rtc_time_alarm_reg));
	timeptr->tm_min = bcd2bin(RTC_TIME_ALARM_REG_GET_FIELD(M, rtc_time_alarm_reg));
	timeptr->tm_hour = bcd2bin(RTC_TIME_ALARM_REG_GET_FIELD(HR, rtc_time_alarm_reg));

	timeptr->tm_nsec = 0;
}

static uint32_t tm_to_rtc_alarm_mask(uint16_t mask)
{
	uint32_t rtc_alarm_enable_reg = 0;

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		rtc_alarm_enable_reg |= RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_SEC_EN_Msk;
	}
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		rtc_alarm_enable_reg |= RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_MIN_EN_Msk;
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		rtc_alarm_enable_reg |= RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_HOUR_EN_Msk;
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		rtc_alarm_enable_reg |= RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_MNTH_EN_Msk;
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		rtc_alarm_enable_reg |= RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_DATE_EN_Msk;
	}

	return rtc_alarm_enable_reg;
}

static uint16_t rtc_to_tm_alarm_mask(uint32_t rtc_alarm_enable_reg)
{
	uint16_t mask = 0;

	if (rtc_alarm_enable_reg & RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_SEC_EN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_SECOND;
	}
	if (rtc_alarm_enable_reg & RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_MIN_EN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if (rtc_alarm_enable_reg & RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_HOUR_EN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if (rtc_alarm_enable_reg & RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_MNTH_EN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_MONTH;
	}
	if (rtc_alarm_enable_reg & RTC_RTC_ALARM_ENABLE_REG_RTC_ALARM_DATE_EN_Msk) {
		mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	return mask;
}

static int rtc_smartbond_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
						const struct rtc_time *timeptr)
{
	int ret = 0;
	struct rtc_smartbond_data *data = dev->data;
	uint32_t rtc_time_alarm_reg;
	uint32_t rtc_calendar_alarm_reg;
	uint32_t rtc_alarm_enable_reg;
	uint32_t rtc_status_reg;

	if (id >= RTC_ALARMS_COUNT) {
		LOG_ERR("Alarm id is out of range");
		return -EINVAL;
	}

	if (mask & ~RTC_SMARTBOND_SUPPORTED_ALARM_FIELDS) {
		LOG_ERR("Invalid alarm mask");
		return -EINVAL;
	}

	if ((timeptr == NULL) && (mask != 0)) {
		LOG_ERR("No pointer is provided to set alarm");
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("Invalid alarm fields values");
		return -EINVAL;
	}

	if (!data->is_rtc_configured) {
		LOG_WRN("RTC is not initialized yet");
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	rtc_alarm_enable_reg = RTC->RTC_ALARM_ENABLE_REG;

	/* Disable alarm to obtain coherency and/or when the alarm mask is empty */
	RTC->RTC_ALARM_ENABLE_REG = 0;
	RTC->RTC_INTERRUPT_DISABLE_REG = RTC_RTC_INTERRUPT_DISABLE_REG_RTC_ALRM_INT_DIS_Msk;

	if (mask) {
		/* Store current counter values as it might happen requested alrm is not valid */
		rtc_time_alarm_reg = RTC->RTC_TIME_ALARM_REG;
		rtc_calendar_alarm_reg = RTC->RTC_CALENDAR_ALARM_REG;

		RTC->RTC_TIME_ALARM_REG = alarm_time_to_bcd(timeptr, mask);
		RTC->RTC_CALENDAR_ALARM_REG = alarm_calendar_to_bcd(timeptr, mask);

		rtc_status_reg = RTC->RTC_STATUS_REG;
		if (!(rtc_status_reg & RTC_RTC_STATUS_REG_RTC_VALID_CAL_ALM_Msk) ||
			!(rtc_status_reg & RTC_RTC_STATUS_REG_RTC_VALID_TIME_ALM_Msk)) {
			RTC->RTC_TIME_ALARM_REG = rtc_time_alarm_reg;
			RTC->RTC_CALENDAR_ALARM_REG = rtc_calendar_alarm_reg;
			RTC->RTC_ALARM_ENABLE_REG = rtc_alarm_enable_reg;
			ret = -EINVAL;
		} else {
			RTC->RTC_ALARM_ENABLE_REG = tm_to_rtc_alarm_mask(mask);
		}

		RTC->RTC_INTERRUPT_ENABLE_REG = RTC_RTC_INTERRUPT_ENABLE_REG_RTC_ALRM_INT_EN_Msk;
	}

	k_mutex_unlock(&data->lock);

	return ret;
}

static int rtc_smartbond_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
						struct rtc_time *timeptr)
{
	struct rtc_smartbond_data *data = dev->data;

	if (id >= RTC_ALARMS_COUNT) {
		LOG_ERR("Alarm id is out of range");
		return -EINVAL;
	}

	if ((timeptr == NULL) || (mask == NULL)) {
		LOG_ERR("No pointer is provided to store the requested alarm time/mask");
		return -EINVAL;
	}

	if (!data->is_rtc_configured) {
		LOG_WRN("RTC is not initialized yet");
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	bcd_to_alarm_calendar(timeptr);
	bcd_to_alarm_time(timeptr);
	*mask = rtc_to_tm_alarm_mask(RTC->RTC_ALARM_ENABLE_REG);

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtc_smartbond_alarm_is_pending(const struct device *dev, uint16_t id)
{
	unsigned int key;
	int status;
	struct rtc_smartbond_data *data = dev->data;

	if (id >= RTC_ALARMS_COUNT) {
		LOG_ERR("Alarm id is out of range");
		return -EINVAL;
	}

	/* Globally disable interrupts as the status flag can be updated within ISR */
	key = DA1469X_IRQ_DISABLE();
	status = data->is_alarm_pending;
	/* After reading, the alarm status should be cleared. */
	data->is_alarm_pending = 0;
	DA1469X_IRQ_ENABLE(key);

	return status;
}

static int rtc_smartbond_alarm_set_callback(const struct device *dev, uint16_t id,
		rtc_alarm_callback callback, void *user_data)
{
	struct rtc_smartbond_data *data = dev->data;

	if (id >= RTC_ALARMS_COUNT) {
		LOG_ERR("Alarm id is out of range");
		return -EINVAL;
	}

	k_mutex_lock(&data->lock, K_FOREVER);

	data->alarm_cb = callback;
	data->alarm_user_data = user_data;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int rtc_smartbond_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						uint16_t *mask)
{
	if (id >= RTC_ALARMS_COUNT) {
		LOG_ERR("Alarm id is out of range");
		return -EINVAL;
	}

	if (mask == NULL) {
		LOG_ERR("Pointer to store the mask value is missed");
		return -EINVAL;
	}

	*mask = (uint16_t)RTC_SMARTBOND_SUPPORTED_ALARM_FIELDS;

	return 0;
}
#endif

#if defined(CONFIG_RTC_UPDATE)
static int rtc_smartbond_update_set_callback(const struct device *dev, rtc_update_callback callback,
						void *user_data)
{
	struct rtc_smartbond_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);

	data->update_cb = callback;
	data->update_user_data = user_data;

	if (data->update_cb) {
		/* Enable asserting the RTC interrupt line when the second counter rolls over. */
		RTC->RTC_INTERRUPT_ENABLE_REG = RTC_RTC_INTERRUPT_ENABLE_REG_RTC_SEC_INT_EN_Msk;
	} else {
		RTC->RTC_INTERRUPT_DISABLE_REG = RTC_RTC_INTERRUPT_DISABLE_REG_RTC_SEC_INT_DIS_Msk;
	}

	k_mutex_unlock(&data->lock);

	return 0;
}
#endif

static const struct rtc_driver_api rtc_smartbond_driver_api = {
	.get_time = rtc_smartbond_get_time,
	.set_time = rtc_smartbond_set_time,
#if defined(CONFIG_RTC_ALARM)
	.alarm_get_time = rtc_smartbond_alarm_get_time,
	.alarm_set_time = rtc_smartbond_alarm_set_time,
	.alarm_is_pending = rtc_smartbond_alarm_is_pending,
	.alarm_set_callback = rtc_smartbond_alarm_set_callback,
	.alarm_get_supported_fields = rtc_smartbond_alarm_get_supported_fields,
#endif
#if defined(CONFIG_RTC_UPDATE)
	.update_set_callback = rtc_smartbond_update_set_callback,
#endif
};

static void rtc_smartbond_100HZ_clock_cfg(void)
{
	const struct device * const dev = DEVICE_DT_GET(DT_NODELABEL(osc));
	uint32_t lp_clk_rate;
	uint32_t clk_rtcdiv_reg;

	if (!device_is_ready(dev)) {
		__ASSERT_MSG_INFO("Clock device is not ready");
	}

	if (clock_control_get_rate(dev, (clock_control_subsys_t)SMARTBOND_CLK_LP_CLK,
								&lp_clk_rate) < 0) {
		__ASSERT_MSG_INFO("Cannot extract LP clock rate");
	}

	clk_rtcdiv_reg = CRG_TOP->CLK_RTCDIV_REG;
	CLK_RTCDIV_REG_SET_FIELD(DENOM, clk_rtcdiv_reg, RTC_DIV_DENOM_1000);
	CLK_RTCDIV_REG_SET_FIELD(INT, clk_rtcdiv_reg, lp_clk_rate / 100);
	CLK_RTCDIV_REG_SET_FIELD(FRAC, clk_rtcdiv_reg, (lp_clk_rate % 100) * 10);
	CRG_TOP->CLK_RTCDIV_REG = clk_rtcdiv_reg;
}

static int rtc_smartbond_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	/* Wakeup device from RTC events (alarm/roll over) */
#if CONFIG_PM
	bool is_xtal32m_enabled = DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(xtal32m));
	int pdc_idx = da1469x_pdc_add(MCU_PDC_TRIGGER_RTC_ALARM, MCU_PDC_MASTER_M33,
					is_xtal32m_enabled ? MCU_PDC_EN_XTAL : 0);

	__ASSERT(pdc_idx >= 0, "Failed to add RTC PDC entry");
	da1469x_pdc_set(pdc_idx);
	da1469x_pdc_ack(pdc_idx);
#endif

	rtc_smartbond_100HZ_clock_cfg();

	/* Timer and calendar counters will not reset after SW reset */
	RTC->RTC_KEEP_RTC_REG |= RTC_RTC_KEEP_RTC_REG_RTC_KEEP_Msk;

#if defined(CONFIG_RTC_ALARM) || defined(CONFIG_RTC_UPDATE)
	IRQ_CONNECT(SMARTBOND_IRQN, SMARTBOND_IRQ_PRIO, smartbond_rtc_isr,
				DEVICE_DT_INST_GET(0), 0);
	irq_enable(SMARTBOND_IRQN);
#endif

	return 0;
}

#define SMARTBOND_RTC_INIT(inst) \
	BUILD_ASSERT((inst) == 0, "multiple instances are not supported"); \
	\
	static struct rtc_smartbond_data rtc_smartbond_data_ ## inst; \
	\
	DEVICE_DT_INST_DEFINE(0, rtc_smartbond_init, NULL, \
		&rtc_smartbond_data_ ## inst, NULL, \
		POST_KERNEL, \
		CONFIG_RTC_INIT_PRIORITY, \
		&rtc_smartbond_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SMARTBOND_RTC_INIT)
