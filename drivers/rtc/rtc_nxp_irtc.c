/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_irtc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/irq.h>

struct nxp_irtc_config {
	RTC_Type *base;
	void (*irq_config_func)(const struct device *dev);
	bool is_output_clock_enabled;
	uint8_t clock_src;
	uint8_t alarm_match_flag;
};

struct nxp_irtc_data {
	bool is_dst_enabled;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	uint16_t alarm_mask;
	bool is_alarm_pending;
#endif /* CONFIG_RTC_ALARM */
};

/* The IRTC Offset is 2112 instead of 1990 [2112 - 1990] -> [212] */
#define RTC_NXP_IRTC_YEAR_OFFSET 212

/* Utility function for testing if bit for alarm mask is enabled. */
#define RTC_NXP_RTCTIME_COMPARE_FLAG(mask, flag) (((mask) == 0) || (((mask) & (flag)) == (flag)))

/* Utility function to check the bounds of passed in rtc_time */
static uint8_t is_rtc_time_in_bounds(const struct rtc_time *time_value, uint16_t mask)
{
	if ((RTC_NXP_RTCTIME_COMPARE_FLAG(mask, RTC_ALARM_TIME_MASK_SECOND)) &&
	    (time_value->tm_sec < 0 || time_value->tm_sec > 59)) {
		return 0;
	}

	if ((RTC_NXP_RTCTIME_COMPARE_FLAG(mask, RTC_ALARM_TIME_MASK_MINUTE)) &&
	    (time_value->tm_min < 0 || time_value->tm_min > 59)) {
		return 0;
	}

	if ((RTC_NXP_RTCTIME_COMPARE_FLAG(mask, RTC_ALARM_TIME_MASK_HOUR)) &&
	    (time_value->tm_hour < 0 || time_value->tm_hour > 23)) {
		return 0;
	}

	if ((RTC_NXP_RTCTIME_COMPARE_FLAG(mask, RTC_ALARM_TIME_MASK_MONTHDAY)) &&
	    (time_value->tm_mday < 1 || time_value->tm_mday > 31)) {
		return 0;
	}

	if ((RTC_NXP_RTCTIME_COMPARE_FLAG(mask, RTC_ALARM_TIME_MASK_MONTH)) &&
	    (time_value->tm_mon < 0 || time_value->tm_mon > 11)) {
		return 0;
	}

	if ((RTC_NXP_RTCTIME_COMPARE_FLAG(mask, RTC_ALARM_TIME_MASK_YEAR)) &&
	    ((time_value->tm_year - RTC_NXP_IRTC_YEAR_OFFSET) < -128 ||
	     (time_value->tm_year - RTC_NXP_IRTC_YEAR_OFFSET) > 127)) {
		return 0;
	}

	if ((RTC_NXP_RTCTIME_COMPARE_FLAG(mask, RTC_ALARM_TIME_MASK_WEEKDAY)) &&
	    (time_value->tm_wday < -1 || time_value->tm_wday > 6)) {
		return 0;
	}

	if ((mask == 0) && (time_value->tm_isdst < -1 || time_value->tm_isdst > 1)) {
		return 0;
	}

	return 1;
}

/*
 * The runtime where this is accessed is unknown so we force a lock on the registers then force an
 * unlock to guarantee 2 seconds of write time
 */
static void nxp_irtc_unlock_registers(RTC_Type *reg)
{
	/* Lock the regsiters */
	while ((reg->STATUS & (uint16_t)RTC_STATUS_WRITE_PROT_EN_MASK) == 0) {
		*(uint8_t *)(&reg->STATUS) |= RTC_STATUS_WE(0x2);
	}
	/* Unlock the registers */
	while ((reg->STATUS & (int16_t)RTC_STATUS_WRITE_PROT_EN_MASK) != 0) {
		/*
		 * The casting is required for writing only a single Byte to the STATUS Register,
		 * the pattern here unlocks all RTC registers for writing. When unlocked if a 0x20
		 * if written to the STATUS register the RTC registers will lock again and the next
		 * write will lead to a fault.
		 */
		*(volatile uint8_t *)(&reg->STATUS) = 0x00;
		*(volatile uint8_t *)(&reg->STATUS) = 0x40;
		*(volatile uint8_t *)(&reg->STATUS) = 0xC0;
		*(volatile uint8_t *)(&reg->STATUS) = 0x80;
	}
}

static int nxp_irtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct nxp_irtc_config *config = dev->config;
	struct nxp_irtc_data *data = dev->data;
	RTC_Type *irtc_reg = config->base;

	if (!timeptr || !is_rtc_time_in_bounds(timeptr, 0)) {
		return -EINVAL;
	}

	int calc_year = timeptr->tm_year - RTC_NXP_IRTC_YEAR_OFFSET;
	/* The IRTC Month Index starts at 1 instead of 0 */
	int calc_month = timeptr->tm_mon + 1;

	uint32_t key = irq_lock();

	nxp_irtc_unlock_registers(irtc_reg);
	irtc_reg->SECONDS = RTC_SECONDS_SEC_CNT(timeptr->tm_sec);

	irtc_reg->HOURMIN = RTC_HOURMIN_MIN_CNT(timeptr->tm_min) |
		RTC_HOURMIN_HOUR_CNT(timeptr->tm_hour);

	/* 1 is valid for rtc_time.tm_wday property but is out of bounds for IRTC registers */
	irtc_reg->DAYS = RTC_DAYS_DAY_CNT(timeptr->tm_mday) |
		      (timeptr->tm_wday == -1 ? 0 : RTC_DAYS_DOW(timeptr->tm_wday));

	irtc_reg->YEARMON = RTC_YEARMON_MON_CNT(calc_month) | RTC_YEARMON_YROFST(calc_year);

	if (timeptr->tm_isdst != -1) {
		irtc_reg->CTRL |= RTC_CTRL_DST_EN(timeptr->tm_isdst);
		data->is_dst_enabled = true;
	}

	irq_unlock(key);

	return 0;
}

static int nxp_irtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct nxp_irtc_config *config = dev->config;
	struct nxp_irtc_data *data = dev->data;
	RTC_Type *irtc_reg = config->base;

	if (!timeptr) {
		return -ENODATA;
	}

	timeptr->tm_sec =
		((irtc_reg->SECONDS & RTC_SECONDS_SEC_CNT_MASK) >> RTC_SECONDS_SEC_CNT_SHIFT);
	timeptr->tm_min =
		((irtc_reg->HOURMIN & RTC_HOURMIN_MIN_CNT_MASK) >> RTC_HOURMIN_MIN_CNT_SHIFT);
	timeptr->tm_hour =
		((irtc_reg->HOURMIN & RTC_HOURMIN_HOUR_CNT_MASK) >> RTC_HOURMIN_HOUR_CNT_SHIFT);
	timeptr->tm_wday = ((irtc_reg->DAYS & RTC_DAYS_DOW_MASK) >> RTC_DAYS_DOW_SHIFT);
	timeptr->tm_mday = ((irtc_reg->DAYS & RTC_DAYS_DAY_CNT_MASK) >> RTC_DAYS_DAY_CNT_SHIFT);
	timeptr->tm_mon =
		((irtc_reg->YEARMON & RTC_YEARMON_MON_CNT_MASK) >> RTC_YEARMON_MON_CNT_SHIFT) - 1;
	timeptr->tm_year = (int8_t)((irtc_reg->YEARMON & RTC_YEARMON_YROFST_MASK) >>
				    RTC_YEARMON_YROFST_SHIFT) +
			   RTC_NXP_IRTC_YEAR_OFFSET;
	if (data->is_dst_enabled) {
		timeptr->tm_isdst =
			((irtc_reg->CTRL & RTC_CTRL_DST_EN_MASK) >> RTC_CTRL_DST_EN_SHIFT);
	}

	/* There is no nano second support for IRTC */
	timeptr->tm_nsec = 0;
	/* There is no day of the year support for IRTC */
	timeptr->tm_yday = -1;

	return 0;
}

#if defined(CONFIG_RTC_ALARM) || defined(__DOXYGEN__)
static int nxp_irtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
					       uint16_t *mask)
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

static int nxp_irtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				   const struct rtc_time *timeptr)
{
	const struct nxp_irtc_config *config = dev->config;
	struct nxp_irtc_data *data = dev->data;
	RTC_Type *irtc_reg = config->base;

	if (id != 0 || (mask && (timeptr == 0)) ||
	    (timeptr && !is_rtc_time_in_bounds(timeptr, mask))) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	nxp_irtc_unlock_registers(irtc_reg);

	if ((mask & RTC_ALARM_TIME_MASK_SECOND) == RTC_ALARM_TIME_MASK_SECOND) {
		irtc_reg->ALM_SECONDS = RTC_ALM_SECONDS_ALM_SEC(timeptr->tm_sec);
	}

	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) == RTC_ALARM_TIME_MASK_MINUTE) {
		irtc_reg->ALM_HOURMIN = RTC_ALM_HOURMIN_ALM_MIN(timeptr->tm_min);
	}

	if ((mask & RTC_ALARM_TIME_MASK_HOUR) == RTC_ALARM_TIME_MASK_HOUR) {
		irtc_reg->ALM_HOURMIN |= RTC_ALM_HOURMIN_ALM_HOUR(timeptr->tm_hour);
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) == RTC_ALARM_TIME_MASK_MONTHDAY) {
		irtc_reg->ALM_DAYS = RTC_ALM_DAYS_ALM_DAY(timeptr->tm_mday);
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTH) == RTC_ALARM_TIME_MASK_MONTH) {
		irtc_reg->ALM_YEARMON = RTC_ALM_YEARMON_ALM_MON(timeptr->tm_mon + 1);
	}

	if ((mask & RTC_ALARM_TIME_MASK_YEAR) == RTC_ALARM_TIME_MASK_YEAR) {
		irtc_reg->ALM_YEARMON |=
			RTC_ALM_YEARMON_ALM_YEAR(timeptr->tm_year - RTC_NXP_IRTC_YEAR_OFFSET);
	}

	/* Clearing out the ALARM Flag Field then setting the correct value */
	irtc_reg->CTRL &= ~(0xC);
	switch (mask) {
	case 0x0F: {
		irtc_reg->CTRL |= RTC_CTRL_ALM_MATCH(0x4);
	} break;
	case 0x1F: {
		irtc_reg->CTRL |= RTC_CTRL_ALM_MATCH(0x8);
	} break;
	case 0x3F: {
		irtc_reg->CTRL |= RTC_CTRL_ALM_MATCH(0xC);
	} break;
	default: {
		irtc_reg->CTRL |= RTC_CTRL_ALM_MATCH(0x0);
	}
	}

	/* Enabling Alarm Interrupts */
	irtc_reg->IER |= 0x4;
	data->alarm_mask = mask;

	irq_unlock(key);

	return 0;
}

static int nxp_irtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				   struct rtc_time *timeptr)
{
	const struct nxp_irtc_config *config = dev->config;
	struct nxp_irtc_data *data = dev->data;
	RTC_Type *irtc_reg = config->base;
	uint16_t curr_alarm_mask = data->alarm_mask;
	uint16_t return_mask = 0;

	if (id != 0 || !timeptr) {
		return -EINVAL;
	}

	if ((curr_alarm_mask & RTC_ALARM_TIME_MASK_SECOND) == RTC_ALARM_TIME_MASK_SECOND) {
		timeptr->tm_sec = ((irtc_reg->ALM_SECONDS & RTC_ALM_SECONDS_ALM_SEC_MASK) >>
				   RTC_ALM_SECONDS_ALM_SEC_SHIFT);
		return_mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if ((curr_alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) == RTC_ALARM_TIME_MASK_MINUTE) {
		timeptr->tm_min = ((irtc_reg->ALM_HOURMIN & RTC_ALM_HOURMIN_ALM_MIN_MASK) >>
				   RTC_ALM_HOURMIN_ALM_MIN_SHIFT);
		return_mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if ((curr_alarm_mask & RTC_ALARM_TIME_MASK_HOUR) == RTC_ALARM_TIME_MASK_HOUR) {
		timeptr->tm_hour = ((irtc_reg->ALM_HOURMIN & RTC_ALM_HOURMIN_ALM_HOUR_MASK) >>
				    RTC_ALM_HOURMIN_ALM_HOUR_SHIFT);
		return_mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if ((curr_alarm_mask & RTC_ALARM_TIME_MASK_MONTHDAY) == RTC_ALARM_TIME_MASK_MONTHDAY) {
		timeptr->tm_mday = ((irtc_reg->ALM_DAYS & RTC_ALM_DAYS_ALM_DAY_MASK) >>
				    RTC_ALM_DAYS_ALM_DAY_SHIFT);
		return_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	if ((curr_alarm_mask & RTC_ALARM_TIME_MASK_MONTH) == RTC_ALARM_TIME_MASK_MONTH) {
		timeptr->tm_mon = ((irtc_reg->ALM_YEARMON & RTC_ALM_YEARMON_ALM_MON_MASK) >>
				   RTC_ALM_YEARMON_ALM_MON_SHIFT) - 1;
		return_mask |= RTC_ALARM_TIME_MASK_MONTH;
	}

	if ((curr_alarm_mask & RTC_ALARM_TIME_MASK_YEAR) == RTC_ALARM_TIME_MASK_YEAR) {
		timeptr->tm_year =
			(int8_t)((irtc_reg->ALM_YEARMON & RTC_ALM_YEARMON_ALM_YEAR_MASK) >>
				 RTC_ALM_YEARMON_ALM_YEAR_SHIFT) + RTC_NXP_IRTC_YEAR_OFFSET;
		return_mask |= RTC_ALARM_TIME_MASK_YEAR;
	}

	*mask = return_mask;

	return 0;
}

static int nxp_irtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct nxp_irtc_data *data = dev->data;
	int ret = data->is_alarm_pending;

	if (id != 0) {
		return -EINVAL;
	}

	data->is_alarm_pending = 0;

	return ret;
}

static int nxp_irtc_alarm_set_callback(const struct device *dev, uint16_t id,
				       rtc_alarm_callback callback, void *user_data)
{
	struct nxp_irtc_data *data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	data->alarm_callback = callback;
	data->alarm_user_data = user_data;
	data->is_alarm_pending = 1;

	irq_unlock(key);

	return 0;
}

#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE) || defined(__DOXYGEN__)

static int nxp_irtc_update_set_callback(const struct device *dev, rtc_update_callback callback,
					void *user_data)
{
	return -ENOTSUP;
}

#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION) || defined(__DOXYGEN__)

static int nxp_irtc_set_calibration(const struct device *dev, int32_t calibration)
{
	return -ENOTSUP;
}

static int nxp_irtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	return -ENOTSUP;
}

#endif /* CONFIG_RTC_CALIBRATION */

static int nxp_irtc_init(const struct device *dev)
{
	const struct nxp_irtc_config *config = dev->config;
	RTC_Type *irtc_reg = config->base;

	nxp_irtc_unlock_registers(irtc_reg);

	/* set the control register bits */
	irtc_reg->CTRL = RTC_CTRL_CLK_SEL(config->clock_src) |
			       RTC_CTRL_CLKO_DIS(!config->is_output_clock_enabled);

	config->irq_config_func(dev);

	return 0;
}

static void nxp_irtc_isr(const struct device *dev)
{
#ifdef CONFIG_RTC_ALARM
	const struct nxp_irtc_config *config = dev->config;
	RTC_Type *irtc_reg = config->base;
	struct nxp_irtc_data *data = dev->data;
	uint32_t key = irq_lock();

	nxp_irtc_unlock_registers(irtc_reg);
	irtc_reg->ISR = irtc_reg->ISR;

	if (data->alarm_callback) {
		data->alarm_callback(dev, 0, data->alarm_user_data);
		data->is_alarm_pending = 0;
	} else {
		data->is_alarm_pending = 1;
	}
	irq_unlock(key);
#endif /* CONFIG_RTC_ALARM */
}

static const struct rtc_driver_api rtc_nxp_irtc_driver_api = {
	.set_time = nxp_irtc_set_time,
	.get_time = nxp_irtc_get_time,
#if defined(CONFIG_RTC_ALARM) || defined(__DOXYGEN__)
	.alarm_get_supported_fields = nxp_irtc_alarm_get_supported_fields,
	.alarm_set_time = nxp_irtc_alarm_set_time,
	.alarm_get_time = nxp_irtc_alarm_get_time,
	.alarm_is_pending = nxp_irtc_alarm_is_pending,
	.alarm_set_callback = nxp_irtc_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE) || defined(__DOXYGEN__)
	.update_set_callback = nxp_irtc_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION) || defined(__DOXYGEN__)
	.set_calibration = nxp_irtc_set_calibration,
	.get_calibration = nxp_irtc_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

#define RTC_NXP_IRTC_DEVICE_INIT(n)                                                                \
	static void nxp_irtc_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), nxp_irtc_isr,               \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}                                                                                          \
	static const struct nxp_irtc_config nxp_irtc_config_##n = {                                \
		.base = (RTC_Type *)DT_INST_REG_ADDR(n),                                           \
		.clock_src = DT_INST_PROP(n, clock_src),					   \
		.is_output_clock_enabled = DT_INST_PROP(n, output_clk_en),	                   \
		.irq_config_func = nxp_irtc_config_func_##n,                                       \
	};                                                                                         \
                                                                                                   \
	static struct nxp_irtc_data nxp_irtc_data_##n;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, nxp_irtc_init, NULL, &nxp_irtc_data_##n, &nxp_irtc_config_##n,    \
			      PRE_KERNEL_1, CONFIG_RTC_INIT_PRIORITY, &rtc_nxp_irtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_NXP_IRTC_DEVICE_INIT)
