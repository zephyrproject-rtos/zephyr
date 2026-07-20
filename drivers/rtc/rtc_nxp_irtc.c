/*
 * Copyright 2024-2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_irtc

#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/irq.h>
#include "rtc_utils.h"

struct nxp_irtc_config {
	RTC_Type *base;
	void (*irq_config_func)(const struct device *dev);
	bool is_output_clock_enabled;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clock_src)
	uint8_t clock_src;
#endif
#if defined(CONFIG_RTC_CALIBRATION)
	uint32_t clock_frequency;
#endif
	bool share_counter;
	uint8_t alarm_match_flag;
};

struct nxp_irtc_data {
	bool is_dst_enabled;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	uint16_t alarm_mask;
	bool alarm_pending;
#endif /* CONFIG_RTC_ALARM */
};

/* The IRTC Offset is 2112 instead of 1900 [2112 - 1900] -> [212] */
#define RTC_NXP_IRTC_YEAR_OFFSET 212

#define RTC_NXP_GET_REG_FIELD(_reg, _name, _field)                                                 \
	((_reg->_name & RTC_##_name##_##_field##_MASK) >> RTC_##_name##_##_field##_SHIFT)

/*
 * The runtime where this is accessed is unknown so we force a lock on the registers then force an
 * unlock to guarantee 2 seconds of write time
 */
static void nxp_irtc_unlock_registers(RTC_Type *reg)
{
	/* Lock the registers */
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

	if (config->share_counter) {
		ARG_UNUSED(timeptr);

		return -EPERM;
	}

	struct nxp_irtc_data *data = dev->data;
	RTC_Type *irtc_reg = config->base;

	if (!timeptr || !rtc_utils_validate_rtc_time(timeptr, 0)) {
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

	/* 1 is valid for rtc_time.tm_wday property but is out of bounds for IRTC registers
	 */
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

	__ASSERT(timeptr != 0, "timeptr has not been set");

	timeptr->tm_sec = RTC_NXP_GET_REG_FIELD(irtc_reg, SECONDS, SEC_CNT);
	timeptr->tm_min = RTC_NXP_GET_REG_FIELD(irtc_reg, HOURMIN, MIN_CNT);
	timeptr->tm_hour = RTC_NXP_GET_REG_FIELD(irtc_reg, HOURMIN, HOUR_CNT);
	timeptr->tm_wday = RTC_NXP_GET_REG_FIELD(irtc_reg, DAYS, DOW);
	timeptr->tm_mday = RTC_NXP_GET_REG_FIELD(irtc_reg, DAYS, DAY_CNT);
	timeptr->tm_mon = RTC_NXP_GET_REG_FIELD(irtc_reg, YEARMON, MON_CNT) - 1;
	timeptr->tm_year =
		(int8_t)RTC_NXP_GET_REG_FIELD(irtc_reg, YEARMON, YROFST) + RTC_NXP_IRTC_YEAR_OFFSET;
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

#if defined(CONFIG_RTC_ALARM)
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
	    (timeptr && !rtc_utils_validate_rtc_time(timeptr, mask))) {
		return -EINVAL;
	}

	data->alarm_pending = false;

	uint32_t key = irq_lock();

	nxp_irtc_unlock_registers(irtc_reg);

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		irtc_reg->ALM_SECONDS = RTC_ALM_SECONDS_ALM_SEC(timeptr->tm_sec);
	}

	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		irtc_reg->ALM_HOURMIN = RTC_ALM_HOURMIN_ALM_MIN(timeptr->tm_min);
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		irtc_reg->ALM_HOURMIN |= RTC_ALM_HOURMIN_ALM_HOUR(timeptr->tm_hour);
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		irtc_reg->ALM_DAYS = RTC_ALM_DAYS_ALM_DAY(timeptr->tm_mday);
	}

	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		irtc_reg->ALM_YEARMON = RTC_ALM_YEARMON_ALM_MON(timeptr->tm_mon + 1);
	}

	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		irtc_reg->ALM_YEARMON |=
			RTC_ALM_YEARMON_ALM_YEAR(timeptr->tm_year - RTC_NXP_IRTC_YEAR_OFFSET);
	}

	/* Clearing out the ALARM Flag Field then setting the correct value */
	irtc_reg->CTRL &= ~((uint16_t)RTC_CTRL_ALM_MATCH_MASK);
	uint8_t year_month_day_mask = (mask & (BIT(3) | BIT(4) | BIT(5))) >> 3;

	switch (year_month_day_mask) {
	case 0x00:
		irtc_reg->CTRL |= RTC_CTRL_ALM_MATCH(0x0);
		break;
	case 0x01:
		irtc_reg->CTRL |= RTC_CTRL_ALM_MATCH(0x1);
		break;
	case 0x03:
		irtc_reg->CTRL |= RTC_CTRL_ALM_MATCH(0x2);
		break;
	case 0x07:
		irtc_reg->CTRL |= RTC_CTRL_ALM_MATCH(0x3);
		break;
	default:
		irq_unlock(key);
		return -EINVAL;
	}

	/* Enabling Alarm Interrupts */
	irtc_reg->IER |= RTC_ISR_ALM_IS_MASK;
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
	uint16_t tmp_hourmin = irtc_reg->ALM_HOURMIN;
	uint16_t tmp_yearmon = irtc_reg->ALM_YEARMON;

	if (id != 0 || !timeptr) {
		return -EINVAL;
	}

	memset(timeptr, 0, sizeof(struct rtc_time));

	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_SECOND) {
		timeptr->tm_sec = (irtc_reg->ALM_SECONDS) & RTC_ALM_SECONDS_ALM_SEC_MASK;
		return_mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) {
		timeptr->tm_min = tmp_hourmin & RTC_ALM_HOURMIN_ALM_MIN_MASK;
		return_mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_HOUR) {
		timeptr->tm_hour = ((tmp_hourmin & RTC_ALM_HOURMIN_ALM_HOUR_MASK) >>
				    RTC_ALM_HOURMIN_ALM_HOUR_SHIFT);
		return_mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		timeptr->tm_mday = (irtc_reg->ALM_DAYS) & RTC_ALM_DAYS_ALM_DAY_MASK;
		return_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}

	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_MONTH) {
		timeptr->tm_mon = (tmp_yearmon & RTC_ALM_YEARMON_ALM_MON_MASK) - 1;
		return_mask |= RTC_ALARM_TIME_MASK_MONTH;
	}

	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_YEAR) {
		timeptr->tm_year = (int8_t)((tmp_yearmon & RTC_ALM_YEARMON_ALM_YEAR_MASK) >>
					    RTC_ALM_YEARMON_ALM_YEAR_SHIFT) +
				   RTC_NXP_IRTC_YEAR_OFFSET;
		return_mask |= RTC_ALARM_TIME_MASK_YEAR;
	}

	*mask = return_mask;

	return 0;
}

static int nxp_irtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct nxp_irtc_data *data = dev->data;
	int ret = 0;

	if (id != 0) {
		return -EINVAL;
	}

	__disable_irq();
	ret = data->alarm_pending ? 1 : 0;
	data->alarm_pending = false;
	__enable_irq();

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

	irq_unlock(key);

	return 0;
}

#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE)

static int nxp_irtc_update_set_callback(const struct device *dev, rtc_update_callback callback,
					void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(callback);
	ARG_UNUSED(user_data);
	return -ENOTSUP;
}

#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION)

/*
 * Coarse compensation maps the signed calibration (in parts-per-billion) onto
 * the IRTC COMPEN register. With fine compensation disabled (CTRL[FINEEN] = 0):
 *   - COMPEN[7:0]  = correction value, a two's-complement count of RTC source
 *                    clock cycles added to / removed from the cycles used to
 *                    generate one 1 Hz tick, applied once per interval.
 *   - COMPEN[15:8] = compensation interval in seconds.
 * The interval is fixed at 1 s here, so the fractional frequency correction is
 * value / cycles-per-second, i.e. ppb = value * 1e9 / cycles. The source clock
 * frequency is taken from the node's clock-frequency property (16.384 kHz or
 * 32.768 kHz), giving an LSB of ~61036 or ~30518 ppb and a range of roughly
 * -7.8M..+7.8M or -3.9M..+3.9M ppb (value in -128..+127) respectively, which
 * covers the RTC calibration API's tested range. clock-frequency is used
 * rather than CTRL[CLK_SEL] because that field is not readable / configured on
 * every IRTC platform (e.g. RT700 does not set clock-src).
 *
 * A positive COMPEN correction value adds source clock cycles to the 1 Hz
 * generation, lengthening the second and slowing the RTC down (measured on
 * MCXN236 hardware). The RTC API defines positive calibration as increasing
 * the RTC frequency, so the value is negated in both directions.
 */
#define RTC_NXP_IRTC_COMP_INTERVAL  1U
#define RTC_NXP_IRTC_PPB_PER_HZ     1000000000LL
#define RTC_NXP_IRTC_COMP_VALUE_MIN (-128)
#define RTC_NXP_IRTC_COMP_VALUE_MAX (127)

static int nxp_irtc_set_calibration(const struct device *dev, int32_t calibration)
{
	const struct nxp_irtc_config *config = dev->config;
	RTC_Type *irtc_reg = config->base;
	int64_t cycles_per_sec = config->clock_frequency;
	int64_t value;
	uint16_t reg;

	if (config->share_counter) {
		/* The time counters are owned by the primary IRTC instance. */
		return -EPERM;
	}

	/*
	 * value = -round(calibration * cycles / 1e9); negated because a positive
	 * COMPEN value slows the RTC down while a positive calibration must speed
	 * it up. Round to nearest to minimize quantization error;
	 * get_calibration() reports the resulting programmed value so a
	 * set()/get() round-trips at the hardware resolution.
	 */
	value = -(((int64_t)calibration * cycles_per_sec +
		   ((calibration >= 0 ? RTC_NXP_IRTC_PPB_PER_HZ : -RTC_NXP_IRTC_PPB_PER_HZ) / 2)) /
		  RTC_NXP_IRTC_PPB_PER_HZ);

	if (value < RTC_NXP_IRTC_COMP_VALUE_MIN || value > RTC_NXP_IRTC_COMP_VALUE_MAX) {
		return -EINVAL;
	}

	uint32_t key = irq_lock();

	nxp_irtc_unlock_registers(irtc_reg);

	/*
	 * With CTRL[FINEEN] = 0 the RM requires compensation to be disabled
	 * through CTRL[COMP_EN] before COMPEN is changed for the first time,
	 * so always disable it around the COMPEN update.
	 */
	reg = irtc_reg->CTRL & (uint16_t)~(RTC_CTRL_FINEEN_MASK | RTC_CTRL_COMP_EN_MASK);
	irtc_reg->CTRL = reg;

	/* COMPEN[7:0] = value (two's complement), COMPEN[15:8] = interval. */
	irtc_reg->COMPEN = RTC_COMPEN_COMPEN_VAL(((uint16_t)((uint8_t)value)) |
						 ((uint16_t)RTC_NXP_IRTC_COMP_INTERVAL << 8U));

	/* Enable coarse compensation. */
	irtc_reg->CTRL = reg | RTC_CTRL_COMP_EN_MASK;

	irq_unlock(key);

	return 0;
}

static int nxp_irtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct nxp_irtc_config *config = dev->config;
	RTC_Type *irtc_reg = config->base;
	uint16_t compen;
	int8_t value;
	uint8_t interval;

	if (!calibration) {
		return -EINVAL;
	}

	if (config->share_counter) {
		/* The time counters are owned by the primary IRTC instance. */
		return -EPERM;
	}

	if ((irtc_reg->CTRL & RTC_CTRL_COMP_EN_MASK) == 0U) {
		/* Compensation disabled. */
		*calibration = 0;
		return 0;
	}

	compen = RTC_NXP_GET_REG_FIELD(irtc_reg, COMPEN, COMPEN_VAL);

	/* COMPEN[7:0] is a signed correction value; COMPEN[15:8] is the interval. */
	value = (int8_t)(compen & 0xFFU);
	interval = (uint8_t)(compen >> 8U);

	if (interval == 0U) {
		/* A zero compensation interval also disables compensation. */
		*calibration = 0;
		return 0;
	}

	/* ppb = -value * 1e9 / (cycles-per-second * interval), negated as in set(). */
	*calibration = (int32_t)(((int64_t)-value * RTC_NXP_IRTC_PPB_PER_HZ) /
				 ((int64_t)config->clock_frequency * interval));

	return 0;
}

#endif /* CONFIG_RTC_CALIBRATION */

static int nxp_irtc_init(const struct device *dev)
{
	const struct nxp_irtc_config *config = dev->config;
	RTC_Type *irtc_reg = config->base;
	uint16_t reg = 0;
#ifdef CONFIG_RTC_ALARM
	struct nxp_irtc_data *data = dev->data;
#endif

	nxp_irtc_unlock_registers(irtc_reg);

	reg = irtc_reg->CTRL;
	reg &= ~(uint16_t)RTC_CTRL_CLKO_DIS_MASK;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clock_src)
	reg &= ~(uint16_t)RTC_CTRL_CLK_SEL_MASK;
#endif

	reg |= RTC_CTRL_CLKO_DIS(!config->is_output_clock_enabled);
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clock_src)
	reg |= RTC_CTRL_CLK_SEL(config->clock_src);
#endif

	irtc_reg->CTRL = reg;

#ifdef CONFIG_RTC_ALARM
	data->alarm_callback = NULL;
	data->alarm_user_data = NULL;
	data->alarm_mask = 0;
	data->alarm_pending = false;
#endif

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
	/* Clearing ISR Register since w1c */
	irtc_reg->ISR = irtc_reg->ISR;
	if (data->alarm_callback) {
		data->alarm_callback(dev, 0, data->alarm_user_data);
		data->alarm_pending = false;
	} else {
		data->alarm_pending = true;
	}
	irq_unlock(key);
#endif /* CONFIG_RTC_ALARM */
}

static DEVICE_API(rtc, rtc_nxp_irtc_driver_api) = {
	.set_time = nxp_irtc_set_time,
	.get_time = nxp_irtc_get_time,
#if defined(CONFIG_RTC_ALARM)
	.alarm_get_supported_fields = nxp_irtc_alarm_get_supported_fields,
	.alarm_set_time = nxp_irtc_alarm_set_time,
	.alarm_get_time = nxp_irtc_alarm_get_time,
	.alarm_is_pending = nxp_irtc_alarm_is_pending,
	.alarm_set_callback = nxp_irtc_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE)
	.update_set_callback = nxp_irtc_update_set_callback,
#endif /* CONFIG_RTC_UPDATE */
#if defined(CONFIG_RTC_CALIBRATION)
	.set_calibration = nxp_irtc_set_calibration,
	.get_calibration = nxp_irtc_get_calibration,
#endif /* CONFIG_RTC_CALIBRATION */
};

#define RTC_NXP_IRTC_IRQ_CONNECT(n, m)                                                             \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, m, irq), DT_INST_IRQ_BY_IDX(n, m, priority),     \
			    nxp_irtc_isr, DEVICE_DT_INST_GET(n), 0);                               \
		irq_enable(DT_INST_IRQ_BY_IDX(n, m, irq));                                         \
	} while (false)

#if DT_INST_IRQ_HAS_IDX(0, 1)
#define NXP_IRTC_CONFIG_FUNC(n)                                                                    \
	static void nxp_irtc_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		RTC_NXP_IRTC_IRQ_CONNECT(n, 0);                                                    \
		RTC_NXP_IRTC_IRQ_CONNECT(n, 1);                                                    \
	}
#else
#define NXP_IRTC_CONFIG_FUNC(n)                                                                    \
	static void nxp_irtc_config_func_##n(const struct device *dev)                             \
	{                                                                                          \
		RTC_NXP_IRTC_IRQ_CONNECT(n, 0);                                                    \
	}
#endif

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clock_src)
#define NXP_IRTC_CLOCK_SELECTION_INIT(n) .clock_src = DT_INST_PROP(n, clock_src),
#else
#define NXP_IRTC_CLOCK_SELECTION_INIT(n)
#endif

#if defined(CONFIG_RTC_CALIBRATION)
#define NXP_IRTC_CLOCK_FREQUENCY_INIT(n) .clock_frequency = DT_INST_PROP(n, clock_frequency),
#else
#define NXP_IRTC_CLOCK_FREQUENCY_INIT(n)
#endif

#define RTC_NXP_IRTC_DEVICE_INIT(n)                                                                \
	NXP_IRTC_CONFIG_FUNC(n)                                                                    \
	static const struct nxp_irtc_config nxp_irtc_config_##n = {                                \
		.base = (RTC_Type *)DT_INST_REG_ADDR(n),                                           \
		NXP_IRTC_CLOCK_SELECTION_INIT(n)                                \
		NXP_IRTC_CLOCK_FREQUENCY_INIT(n)                                \
		.share_counter = DT_INST_PROP(n, share_counter),                \
		.is_output_clock_enabled = DT_INST_PROP(n, output_clk_en),                         \
		.irq_config_func = nxp_irtc_config_func_##n,                                       \
	};                                                                                         \
                                                                                                   \
	static struct nxp_irtc_data nxp_irtc_data_##n;                                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, nxp_irtc_init, NULL, &nxp_irtc_data_##n, &nxp_irtc_config_##n,    \
			      PRE_KERNEL_1, CONFIG_RTC_INIT_PRIORITY, &rtc_nxp_irtc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_NXP_IRTC_DEVICE_INIT)
