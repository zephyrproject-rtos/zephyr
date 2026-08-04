/*
 * Copyright (c) 2025 Linumiz GmbH
 * Copyright (c) 2026 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define DT_DRV_COMPAT ti_mspm0_rtc

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/sys/__assert.h>
#include "rtc_utils.h"

#if defined(CONFIG_RTC_ALARM)
#define RTC_TI_ALARM_1		0
#define RTC_TI_ALARM_2		1
#define RTC_TI_MAX_ALARM	DT_INST_PROP(0, alarms_count)

BUILD_ASSERT((RTC_TI_MAX_ALARM != 0),
	     "CONFIG_RTC_ALARM is enabled, without setting alarms-count property");
#endif

#define RTC_MSPM0_PWREN_MASK			BIT(0)
#define RTC_MSPM0_PWREN_KEY_MASK		GENMASK(31, 24)
#define RTC_MSPM0_PWREN_KEY			0x26

#define RTC_MSPM0_CLKCTL_ENABLE			BIT(31)

#define RTC_MSPM0_CTL_BIN			0x0

#define RTC_MSPM0_SEC_BIN_MASK			BIT_MASK(6)
#define RTC_MSPM0_MIN_BIN_MASK			BIT_MASK(6)
#define RTC_MSPM0_HOUR_BIN_MASK			BIT_MASK(5)
#define RTC_MSPM0_MDAY_BIN_MASK			GENMASK(12, 8)
#define RTC_MSPM0_WDAY_BIN_MASK			BIT_MASK(3)
#define RTC_MSPM0_MON_BIN_MASK			BIT_MASK(4)
#define RTC_MSPM0_YEARLOW_BIN_MASK		BIT_MASK(8)
#define RTC_MSPM0_YEARHIGH_BIN_MASK		GENMASK(11, 8)

#define RTC_MSPM0_IMASK_ALARM1			BIT(2)
#define RTC_MSPM0_IMASK_ALARM2			BIT(3)
#define RTC_MSPM0_AMIN_BIN_MASK			BIT_MASK(6)
#define RTC_MSPM0_AMIN_BIN_ENABLE		BIT(7)
#define RTC_MSPM0_AHOUR_BIN_MASK		BIT_MASK(5)
#define RTC_MSPM0_AHOUR_BIN_ENABLE		BIT(7)
#define RTC_MSPM0_ADAY_MON_BIN_MASK		GENMASK(12, 8)
#define RTC_MSPM0_ADAY_MON_BIN_ENABLE		BIT(15)
#define RTC_MSPM0_ADAY_WEEK_BIN_MASK		BIT_MASK(3)
#define RTC_MSPM0_ADAY_WEEK_BIN_ENABLE		BIT(7)

#define RTC_MSPM0_IIDX_ALARM1			0x3
#define RTC_MSPM0_IIDX_ALARM2			0x4

typedef struct {
	volatile uint32_t a_min;	/* RTC Alarm Minutes	*/
	volatile uint32_t a_hour;	/* RTC Alarm Hours	*/
	volatile uint32_t a_day;	/* RTC Alarm Day	*/
} rtc_ti_mspm0_alarm_reg_t;

typedef struct {
	uint32_t reserved0[0x111];
	volatile uint32_t fpub_0;	/* Publisher Port 0			@0x444h */
	uint32_t reserved1[0xEE];
	volatile uint32_t pwren;	/* Power Enable				@0x800h */
	volatile uint32_t rstctl;	/* Reset Control			@0x804h */
	volatile uint32_t clkcfg;	/* Peripheral Clock Configuration	@0x808h */
	uint32_t reserved2[2];
	volatile uint32_t stat;		/* Status Register			@0x814h */
	uint32_t reserved3[0x1FB];
	volatile uint32_t clksel;	/* Clock Select				@0x1004h */
	uint32_t reserved4[6];
	volatile uint32_t iidx;		/* Interrupt Index Register		@0x1020h */
	uint32_t reserved5[1];
	volatile uint32_t imask;	/* Interrupt Mask			@0x1028h */
	uint32_t reserved6[1];
	volatile uint32_t ris;		/* Raw Interrupt Status			@0x1030h */
	uint32_t reserved7[1];
	volatile uint32_t mis;		/* Masked Interrupt Status		@0x1038h */
	uint32_t reserved8[1];
	volatile uint32_t iset;		/* Interrupt Set			@0x1040h */
	uint32_t reserved9[1];
	volatile uint32_t iclr;		/* Interrupt Clear			@0x1048h */
	uint32_t reserved10[1];
	volatile uint32_t iidx1;	/* Interrupt Index Register 1		@0x1050h */
	uint32_t reserved11[1];
	volatile uint32_t imask1;	/* Interrupt Mask 1			@0x1058h */
	uint32_t reserved12[1];
	volatile uint32_t ris1;		/* Raw Interrupt Status 1		@0x1060h */
	uint32_t reserved13[1];
	volatile uint32_t mis1;		/* Masked Interrupt Status 1		@0x1068h */
	uint32_t reserved14[1];
	volatile uint32_t iset1;	/* Interrupt Set 1			@0x1070h */
	uint32_t reserved15[1];
	volatile uint32_t iclr1;	/* Interrupt Clear 1			@0x1078h */
	uint32_t reserved16[0x19];
	volatile uint32_t evt_mode;	/* Event Mode				0x10E0h */
	uint32_t reserved17[6];
	volatile uint32_t desc;		/* RTC Descriptor Register		@0x10FCh */
	volatile uint32_t clkctl;	/* RTC Clock Control			@0x1100h */
	volatile uint32_t dbgctl;	/* RTC Debug Control			@0x1104h */
	volatile uint32_t ctl;		/* RTC Control				@0x1108h */
	volatile uint32_t sta;		/* RTC Status				@0x110Ch */
	volatile uint32_t cal;		/* RTC Clock Offset Calibration		@0x1110h */
	volatile uint32_t tcmp;		/* RTC Temperature Compensation		@0x1114h */
	volatile uint32_t sec;		/* RTC Seconds				@0x1118h */
	volatile uint32_t min;		/* RTC Minutes				@0x111Ch */
	volatile uint32_t hour;		/* RTC Hours				@0x1120h */
	volatile uint32_t day;		/* RTC Day of Week/Month		@0x1124h */
	volatile uint32_t mon;		/* RTC Month				@0x1128h */
	volatile uint32_t year;		/* RTC Year				@0x112Ch */
	/*
	 * Alarm 1 @0x1130h
	 * Alarm 2 @0x113Ch
	 */
	rtc_ti_mspm0_alarm_reg_t alarm[2];
	volatile uint32_t psctl;	/* RTC Prescale Timer 0/1 Control	@0x1148h */
	volatile uint32_t extpsctl;     /* RTC Prescale Timer 2 Control		@0x114Ch */
	volatile uint32_t tssec;	/* Time Stamp Seconds			@0x1150h */
	volatile uint32_t tsmin;	/* Time Stamp Minutes			@0x1154h */
	volatile uint32_t tshour;	/* Time Stamp Hours			@0x1158h */
	volatile uint32_t tsday;	/* Time Stamp Day Of Week/Month		@0x115Ch */
	volatile uint32_t tsmon;	/* Time Stamp Month			@0x1160h */
	volatile uint32_t tsyear;	/* Time Stamp Years			@0x1164h */
	volatile uint32_t tsstat;	/* Time Stamp Status			@0x1168h */
	volatile uint32_t tsctl;	/* Time Stamp Control			@0x116Ch */
	volatile uint32_t tsclr;	/* Time Stamp Clear			@0x1170h */
	volatile uint32_t lfssrst;	/* Low Frequency Subsystem Reset	@0x1174h */
	volatile uint32_t rtclock;	/* Real Time Clock Lock			@0x1178h */
} rtc_ti_mspm0_reg_t;

struct rtc_ti_mspm0_config {
	rtc_ti_mspm0_reg_t *regs;
#if defined(CONFIG_RTC_ALARM)
	void (*irq_config_func)(void);
#endif
	bool rtc_x;
};

#if defined(CONFIG_RTC_ALARM)
struct rtc_ti_mspm0_alarm {
	rtc_alarm_callback callback;
	void *user_data;
	uint16_t mask;
	bool is_pending;
};
#endif

struct rtc_ti_mspm0_data {
	struct k_spinlock lock;
#if defined(CONFIG_RTC_ALARM)
	struct rtc_ti_mspm0_alarm rtc_alarm[RTC_TI_MAX_ALARM];
#endif
};

static int rtc_ti_mspm0_set_time(const struct device *dev,
				 const struct rtc_time *timeptr)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;
	int mon, year;

	if ((timeptr == NULL) || !rtc_utils_validate_rtc_time(timeptr, 0)) {
		return -EINVAL;
	}

	mon = timeptr->tm_mon + 1;
	year = timeptr->tm_year + 1900;

	K_SPINLOCK(&data->lock) {
		/*
		 * Back to back writes to counter/calendar registers such as
		 * SEC, MIN, HOUR, DAY, MON, YEAR need to be avoided since writes
		 * to calendar registers take 2 to 3 RTCCLK cycles to take effect.
		 */

		/* Set calendar seconds in binary */
		cfg->regs->sec = timeptr->tm_sec & RTC_MSPM0_SEC_BIN_MASK;
		/* Set calendar minutes in binary */
		cfg->regs->min = timeptr->tm_min & RTC_MSPM0_MIN_BIN_MASK;
		/* Set calendar hours in binary */
		cfg->regs->hour = timeptr->tm_hour & RTC_MSPM0_HOUR_BIN_MASK;
		/* Set calendar day in binary */
		/* Month of the day: bits 12-8
		 * Month of the week: bits 2-0
		 */
		cfg->regs->day = FIELD_PREP(RTC_MSPM0_MDAY_BIN_MASK, timeptr->tm_mday) |
				      FIELD_PREP(RTC_MSPM0_WDAY_BIN_MASK, timeptr->tm_wday);
		/* Set calendar month in binary */
		cfg->regs->mon = mon & RTC_MSPM0_MON_BIN_MASK;
		cfg->regs->year =
			year & (RTC_MSPM0_YEARLOW_BIN_MASK | RTC_MSPM0_YEARHIGH_BIN_MASK);
	}

	return 0;
}

static int rtc_ti_mspm0_get_time(const struct device *dev,
				 struct rtc_time *timeptr)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		timeptr->tm_sec = cfg->regs->sec & RTC_MSPM0_SEC_BIN_MASK;
		timeptr->tm_min = cfg->regs->min & RTC_MSPM0_MIN_BIN_MASK;
		timeptr->tm_hour = cfg->regs->hour & RTC_MSPM0_HOUR_BIN_MASK;
		timeptr->tm_mday = FIELD_GET(RTC_MSPM0_MDAY_BIN_MASK, cfg->regs->day);
		timeptr->tm_mon = (cfg->regs->mon & RTC_MSPM0_MON_BIN_MASK) - 1;
		timeptr->tm_year = (cfg->regs->year &
				    (RTC_MSPM0_YEARLOW_BIN_MASK | RTC_MSPM0_YEARHIGH_BIN_MASK)) -
				   1900;
		timeptr->tm_wday = FIELD_GET(RTC_MSPM0_WDAY_BIN_MASK, cfg->regs->day);
	}

	timeptr->tm_yday = -1;
	timeptr->tm_nsec = 0;
	timeptr->tm_isdst = -1;

	return 0;
}

#if defined(CONFIG_RTC_ALARM)
static int rtc_ti_mspm0_alarm_get_supported_fields(const struct device *dev,
						   uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	__ASSERT(mask != NULL, "Invalid argument mask");
	*mask = (RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |
		 RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_MONTHDAY);

	return 0;
}

static inline void rtc_ti_mspm0_clear_alarm(const struct device *dev, uint16_t id)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;

	cfg->regs->alarm[id].a_min = 0x00;
	cfg->regs->alarm[id].a_hour = 0x00;
	cfg->regs->alarm[id].a_day = 0x00;
}

static int rtc_ti_mspm0_alarm_set_time(const struct device *dev, uint16_t id,
				       uint16_t mask,
				       const struct rtc_time *timeptr)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;
	uint32_t imask_bit;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		return -EINVAL;
	}

	imask_bit = (id == RTC_TI_ALARM_1) ? RTC_MSPM0_IMASK_ALARM1 : RTC_MSPM0_IMASK_ALARM2;

	K_SPINLOCK(&data->lock) {
		rtc_ti_mspm0_clear_alarm(dev, id);
		cfg->regs->imask &= ~imask_bit;

		if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
			cfg->regs->alarm[id].a_min = RTC_MSPM0_AMIN_BIN_ENABLE |
						     (timeptr->tm_min & RTC_MSPM0_AMIN_BIN_MASK);
		}

		if (mask & RTC_ALARM_TIME_MASK_HOUR) {
			cfg->regs->alarm[id].a_hour =
				RTC_MSPM0_AHOUR_BIN_ENABLE |
				(timeptr->tm_hour & RTC_MSPM0_AHOUR_BIN_MASK);
		}

		if (mask & (RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_MONTHDAY)) {
			uint32_t day = 0;

			if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
				day |= RTC_MSPM0_ADAY_WEEK_BIN_ENABLE |
				       (timeptr->tm_wday & RTC_MSPM0_ADAY_WEEK_BIN_MASK);
			}
			if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
				day |= RTC_MSPM0_ADAY_MON_BIN_ENABLE |
				       FIELD_PREP(RTC_MSPM0_ADAY_MON_BIN_MASK, timeptr->tm_mday);
			}
			cfg->regs->alarm[id].a_day = day;
		}

		cfg->regs->imask |= imask_bit;
		data->rtc_alarm[id].mask = mask;
		data->rtc_alarm[id].is_pending = false;
	}

	return 0;
}

static int rtc_ti_mspm0_alarm_get_time(const struct device *dev, uint16_t id,
				       uint16_t *mask, struct rtc_time *timeptr)
{
	uint16_t return_mask = 0;
	uint16_t alarm_mask = 0;
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		alarm_mask = data->rtc_alarm[id].mask;

		if (alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) {
			timeptr->tm_min = cfg->regs->alarm[id].a_min & RTC_MSPM0_AMIN_BIN_MASK;
			return_mask |= RTC_ALARM_TIME_MASK_MINUTE;
		}
		if (alarm_mask & RTC_ALARM_TIME_MASK_HOUR) {
			timeptr->tm_hour =
				cfg->regs->alarm[id].a_hour & RTC_MSPM0_AHOUR_BIN_MASK;
			return_mask |= RTC_ALARM_TIME_MASK_HOUR;
		}
		if (alarm_mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
			timeptr->tm_wday = FIELD_GET(RTC_MSPM0_ADAY_WEEK_BIN_MASK,
						     cfg->regs->alarm[id].a_day);
			return_mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		}
		if (alarm_mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
			timeptr->tm_mday = FIELD_GET(RTC_MSPM0_ADAY_MON_BIN_MASK,
						     cfg->regs->alarm[id].a_day);
			return_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		}

		*mask = return_mask;
	}

	return 0;
}

static int rtc_ti_mspm0_alarm_set_callback(const struct device *dev, uint16_t id,
					   rtc_alarm_callback callback,
					   void *user_data)
{
	struct rtc_ti_mspm0_data *data = dev->data;

	if (callback == NULL) {
		return -EINVAL;
	}

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		data->rtc_alarm[id].callback = callback;
		data->rtc_alarm[id].user_data = user_data;
	}

	return 0;
}

static int rtc_ti_mspm0_alarm_is_pending(const struct device *dev, uint16_t id)
{
	int ret;
	struct rtc_ti_mspm0_alarm *alarm = NULL;
	struct rtc_ti_mspm0_data *data = dev->data;

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	alarm = &data->rtc_alarm[id];
	ret = alarm->is_pending ? 1 : 0;
	alarm->is_pending = false;

	k_spin_unlock(&data->lock, key);
	return ret;
}

static void rtc_ti_mspm0_isr(const struct device *dev)
{
	uint8_t id;
	struct rtc_ti_mspm0_alarm *alarm = NULL;
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	switch (cfg->regs->iidx) {
	case RTC_MSPM0_IIDX_ALARM1:
		id = RTC_TI_ALARM_1;
		alarm = &data->rtc_alarm[RTC_TI_ALARM_1];
		break;
	case RTC_MSPM0_IIDX_ALARM2:
		id = RTC_TI_ALARM_2;
		alarm = &data->rtc_alarm[RTC_TI_ALARM_2];
		break;
	default:
		goto out;
	}

	if (alarm != NULL) {
		alarm->is_pending = true;
		if (alarm->callback) {
			alarm->callback(dev, id, alarm->user_data);
		}
	}

out:
	k_spin_unlock(&data->lock, key);
}
#endif

static int rtc_ti_mspm0_init(const struct device *dev)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;

	if (!cfg->rtc_x) {
		if (!(cfg->regs->pwren & RTC_MSPM0_PWREN_MASK)) {
			/* Write Power enable key and set Power enable bit simultaneously */
			cfg->regs->pwren =
				FIELD_PREP(RTC_MSPM0_PWREN_KEY_MASK, RTC_MSPM0_PWREN_KEY) |
				RTC_MSPM0_PWREN_MASK;
		}
	}
	/* Enable 32k clock supply */
	cfg->regs->clkctl = RTC_MSPM0_CLKCTL_ENABLE;
	/* Set clock format to binary */
	cfg->regs->ctl = RTC_MSPM0_CTL_BIN;

#if defined(CONFIG_RTC_ALARM)
	cfg->irq_config_func();
#endif

	return 0;
}

static DEVICE_API(rtc, rtc_ti_mspm0_driver_api) = {
	.set_time		= rtc_ti_mspm0_set_time,
	.get_time		= rtc_ti_mspm0_get_time,
#if defined(CONFIG_RTC_ALARM)
	.alarm_set_time		= rtc_ti_mspm0_alarm_set_time,
	.alarm_get_time		= rtc_ti_mspm0_alarm_get_time,
	.alarm_is_pending	= rtc_ti_mspm0_alarm_is_pending,
	.alarm_set_callback	= rtc_ti_mspm0_alarm_set_callback,
	.alarm_get_supported_fields = rtc_ti_mspm0_alarm_get_supported_fields,
#endif /* CONFIG_RTC_ALARM */
};

#define RTC_TI_MSPM0_DEVICE_INIT(n)						\
	IF_ENABLED(CONFIG_RTC_ALARM,						\
	(static void ti_mspm0_config_irq_##n(void)				\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    rtc_ti_mspm0_isr, DEVICE_DT_INST_GET(n), 0);	\
		irq_enable(DT_INST_IRQN(n));					\
	}))									\
										\
	static struct rtc_ti_mspm0_data rtc_data_##n;				\
										\
	static struct rtc_ti_mspm0_config rtc_config_##n = {			\
		.regs = (rtc_ti_mspm0_reg_t *)DT_INST_REG_ADDR(n),		\
		.rtc_x		 = DT_INST_PROP(n, ti_rtc_x),			\
		IF_ENABLED(CONFIG_RTC_ALARM,					\
		(.irq_config_func = ti_mspm0_config_irq_##n,))			\
	};									\
										\
DEVICE_DT_INST_DEFINE(n, &rtc_ti_mspm0_init, NULL, &rtc_data_##n,		\
		      &rtc_config_##n, PRE_KERNEL_1,				\
		      CONFIG_RTC_INIT_PRIORITY, &rtc_ti_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_TI_MSPM0_DEVICE_INIT);
