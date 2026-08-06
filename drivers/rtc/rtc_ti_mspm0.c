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

#define RTC_MSPM0_PWREN_MASK     BIT(0)
#define RTC_MSPM0_PWREN_KEY_MASK GENMASK(31, 24)
#define RTC_MSPM0_PWREN_KEY      0x26

#define RTC_MSPM0_CLKCTL_ENABLE BIT(31)

#define RTC_MSPM0_CTL_BIN 0x0

#define RTC_MSPM0_SEC_BIN_MASK			BIT_MASK(6)
#define RTC_MSPM0_MIN_BIN_MASK			BIT_MASK(6)
#define RTC_MSPM0_HOUR_BIN_MASK		BIT_MASK(5)
#define RTC_MSPM0_MDAY_BIN_MASK		GENMASK(12, 8)
#define RTC_MSPM0_WDAY_BIN_MASK		BIT_MASK(3)
#define RTC_MSPM0_MON_BIN_MASK			BIT_MASK(4)
#define RTC_MSPM0_YEARLOW_BIN_MASK		BIT_MASK(8)
#define RTC_MSPM0_YEARHIGH_BIN_MASK		GENMASK(11, 8)

#define RTC_MSPM0_IMASK_ALARM1			BIT(2)
#define RTC_MSPM0_IMASK_ALARM2			BIT(3)
#define RTC_MSPM0_AMIN_BIN_MASK		BIT_MASK(6)
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
	volatile uint32_t A_MIN;	/* RTC Alarm Minutes	*/
	volatile uint32_t A_HOUR;	/* RTC Alarm Hours	*/
	volatile uint32_t A_DAY;	/* RTC Alarm Day	*/
} rtc_ti_mspm0_alarm_reg_t;

typedef struct {
	uint32_t RESERVED0[0x111];
	volatile uint32_t FPUB_0;	/* Publisher Port 0			@0x444h */
	uint32_t RESERVED1[0xEE];
	volatile uint32_t PWREN;	/* Power Enable  @0x800h */
	volatile uint32_t RSTCTL;	/* Reset Control			@0x804h */
	volatile uint32_t CLKCFG;	/* Peripheral Clock Configuration	@0x808h */
	uint32_t RESERVED2[2];
	volatile uint32_t STAT;		/* Status Register			@0x814h */
	uint32_t RESERVED3[0x1FB];
	volatile uint32_t CLKSEL;	/* Clock Select @0x1004h */
	uint32_t RESERVED4[6];
	volatile uint32_t IIDX;		/* Interrupt Index Register		@0x1020h */
	uint32_t RESERVED5[1];
	volatile uint32_t IMASK;	/* Interrupt Mask			@0x1028h */
	uint32_t RESERVED6[1];
	volatile uint32_t RIS;		/* Raw Interrupt Status			@0x1030h */
	uint32_t RESERVED7[1];
	volatile uint32_t MIS;		/* Masked Interrupt Status		@0x1038h */
	uint32_t RESERVED8[1];
	volatile uint32_t ISET;		/* Interrupt Set			@0x1040h */
	uint32_t RESERVED9[1];
	volatile uint32_t ICLR;		/* Interrupt Clear			@0x1048h */
	uint32_t RESERVED10[1];
	volatile uint32_t IIDX1;	/* Interrupt Index Register 1		@0x1050h */
	uint32_t RESERVED11[1];
	volatile uint32_t IMASK1;	/* Interrupt Mask 1			@0x1058h */
	uint32_t RESERVED12[1];
	volatile uint32_t RIS1;		/* Raw Interrupt Status 1		@0x1060h */
	uint32_t RESERVED13[1];
	volatile uint32_t MIS1;		/* Masked Interrupt Status 1		@0x1068h */
	uint32_t RESERVED14[1];
	volatile uint32_t ISET1;	/* Interrupt Set 1			@0x1070h */
	uint32_t RESERVED15[1];
	volatile uint32_t ICLR1;	/* Interrupt Clear 1			@0x1078h */
	uint32_t RESERVED16[0x19];
	volatile uint32_t EVT_MODE;	/* Event Mode @0x10E0h */
	uint32_t RESERVED17[6];
	volatile uint32_t DESC;		/* RTC Descriptor Register		@0x10FCh */
	volatile uint32_t CLKCTL;	/* RTC Clock Control			@0x1100h */
	volatile uint32_t DBGCTL;	/* RTC Debug Control			@0x1104h */
	volatile uint32_t CTL;		/* RTC Control				@0x1108h */
	volatile uint32_t STA;		/* RTC Status				@0x110Ch */
	volatile uint32_t CAL;		/* RTC Clock Offset Calibration		@0x1110h */
	volatile uint32_t TCMP;		/* RTC Temperature Compensation		@0x1114h */
	volatile uint32_t SEC;		/* RTC Seconds				@0x1118h */
	volatile uint32_t MIN;		/* RTC Minutes				@0x111Ch */
	volatile uint32_t HOUR;		/* RTC Hours				@0x1120h */
	volatile uint32_t DAY;		/* RTC Day of Week/Month		@0x1124h */
	volatile uint32_t MON;		/* RTC Month				@0x1128h */
	volatile uint32_t YEAR;		/* RTC Year				@0x112Ch */
	/*
	 * Alarm 1 @0x1130h
	 * Alarm 2 @0x113Ch
	 */
	rtc_ti_mspm0_alarm_reg_t ALARM[2];
	volatile uint32_t PSCTL;	/* RTC Prescale Timer 0/1 Control	@0x1148h */
} rtc_ti_mspm0_reg_t;

struct rtc_ti_mspm0_config {
	rtc_ti_mspm0_reg_t *registers;
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

static int rtc_ti_mspm0_set_time(const struct device *dev, const struct rtc_time *timeptr)
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
		/* Set calendar seconds in binary */
		cfg->registers->SEC = timeptr->tm_sec & RTC_MSPM0_SEC_BIN_MASK;
		/* Set calendar minutes in binary */
		cfg->registers->MIN = timeptr->tm_min & RTC_MSPM0_MIN_BIN_MASK;
		/* Set calendar hours in binary */
		cfg->registers->HOUR = timeptr->tm_hour & RTC_MSPM0_HOUR_BIN_MASK;
		/* Set calendar day in binary */
		/* Month of the day: bits 12-8
		 * Month of the week: bits 2-0
		 */
		cfg->registers->DAY = FIELD_PREP(RTC_MSPM0_MDAY_BIN_MASK, timeptr->tm_mday) |
				      FIELD_PREP(RTC_MSPM0_WDAY_BIN_MASK, timeptr->tm_wday);
		/* Set calendar month in binary */
		cfg->registers->MON = mon & RTC_MSPM0_MON_BIN_MASK;
		cfg->registers->YEAR =
			year & (RTC_MSPM0_YEARLOW_BIN_MASK | RTC_MSPM0_YEARHIGH_BIN_MASK);
	}

	return 0;
}

static int rtc_ti_mspm0_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		timeptr->tm_sec = cfg->registers->SEC & RTC_MSPM0_SEC_BIN_MASK;
		timeptr->tm_min = cfg->registers->MIN & RTC_MSPM0_MIN_BIN_MASK;
		timeptr->tm_hour = cfg->registers->HOUR & RTC_MSPM0_HOUR_BIN_MASK;
		timeptr->tm_mday = FIELD_GET(RTC_MSPM0_MDAY_BIN_MASK, cfg->registers->DAY);
		timeptr->tm_mon = (cfg->registers->MON & RTC_MSPM0_MON_BIN_MASK) - 1;
		timeptr->tm_year = (cfg->registers->YEAR &
				    (RTC_MSPM0_YEARLOW_BIN_MASK | RTC_MSPM0_YEARHIGH_BIN_MASK)) -
				   1900;
		timeptr->tm_wday = FIELD_GET(RTC_MSPM0_WDAY_BIN_MASK, cfg->registers->DAY);
	}

	timeptr->tm_yday = -1;
	timeptr->tm_nsec = 0;
	timeptr->tm_isdst = -1;

	return 0;
}

#if defined(CONFIG_RTC_ALARM)
static int rtc_ti_mspm0_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						   uint16_t *mask)
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

	cfg->registers->ALARM[id].A_MIN = 0x00;
	cfg->registers->ALARM[id].A_HOUR = 0x00;
	cfg->registers->ALARM[id].A_DAY = 0x00;
}

static int rtc_ti_mspm0_alarm_set_time(const struct device *dev, uint16_t id,
						uint16_t mask, const struct rtc_time *timeptr)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;
	uint32_t imask_bit;

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	if (mask == 0) {
		rtc_ti_mspm0_clear_alarm(dev, id);
		return 0;
	}

	if (timeptr == NULL) {
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		return -EINVAL;
	}

	imask_bit = (id == RTC_TI_ALARM_1) ? RTC_MSPM0_IMASK_ALARM1 : RTC_MSPM0_IMASK_ALARM2;

	K_SPINLOCK(&data->lock) {
		rtc_ti_mspm0_clear_alarm(dev, id);
		cfg->registers->IMASK &= ~imask_bit;

		if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
			cfg->registers->ALARM[id].A_MIN =
				RTC_MSPM0_AMIN_BIN_ENABLE |
				(timeptr->tm_min & RTC_MSPM0_AMIN_BIN_MASK);
		}

		if (mask & RTC_ALARM_TIME_MASK_HOUR) {
			cfg->registers->ALARM[id].A_HOUR =
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
				       (timeptr->tm_mday & RTC_MSPM0_ADAY_MON_BIN_MASK);
			}
			cfg->registers->ALARM[id].A_DAY = day;
		}

		cfg->registers->IMASK |= imask_bit;
		data->rtc_alarm[id].mask = mask;
		data->rtc_alarm[id].is_pending = false;
	}

	return 0;
}

static int rtc_ti_mspm0_alarm_get_time(const struct device *dev, uint16_t id,
						uint16_t *mask, struct rtc_time *timeptr)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;
	uint16_t return_mask = 0;
	uint16_t alarm_mask;

	if (timeptr == NULL) {
		return -EINVAL;
	}

	if (id != RTC_TI_ALARM_1 && id != RTC_TI_ALARM_2) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		alarm_mask = data->rtc_alarm[id].mask;

		if (alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) {
			timeptr->tm_min = cfg->registers->ALARM[id].A_MIN & RTC_MSPM0_AMIN_BIN_MASK;
			return_mask |= RTC_ALARM_TIME_MASK_MINUTE;
		}
		if (alarm_mask & RTC_ALARM_TIME_MASK_HOUR) {
			timeptr->tm_hour =
				cfg->registers->ALARM[id].A_HOUR & RTC_MSPM0_AHOUR_BIN_MASK;
			return_mask |= RTC_ALARM_TIME_MASK_HOUR;
		}
		if (alarm_mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
			timeptr->tm_wday = FIELD_GET(RTC_MSPM0_ADAY_WEEK_BIN_MASK,
						     cfg->registers->ALARM[id].A_DAY);
			return_mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
		}
		if (alarm_mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
			timeptr->tm_mday = FIELD_GET(RTC_MSPM0_ADAY_MON_BIN_MASK,
						     cfg->registers->ALARM[id].A_DAY);
			return_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
		}

		*mask = return_mask;
	}

	return 0;
}

static int rtc_ti_mspm0_alarm_set_callback(const struct device *dev, uint16_t id,
					   rtc_alarm_callback callback, void *user_data)
{
	struct rtc_ti_mspm0_data *data = dev->data;

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

	K_SPINLOCK(&data->lock) {
		alarm = &data->rtc_alarm[id];
		ret = alarm->is_pending ? 1 : 0;
		alarm->is_pending = false;
	}

	return ret;
}

static void rtc_ti_mspm0_isr(const struct device *dev)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;
	struct rtc_ti_mspm0_data *data = dev->data;
	struct rtc_ti_mspm0_alarm alarm = {0};
	uint8_t alarm_id = 0;
	bool has_cb = false;

	K_SPINLOCK(&data->lock) {
		switch (cfg->registers->IIDX) {
			case RTC_MSPM0_IIDX_ALARM1:
				alarm_id = RTC_TI_ALARM_1;
				break;
			case RTC_MSPM0_IIDX_ALARM2:
				alarm_id = RTC_TI_ALARM_2;
				break;
			default:
			K_SPINLOCK_BREAK;
		}

		alarm = data->rtc_alarm[alarm_id];
		if (!alarm.callback) {
			data->rtc_alarm[alarm_id].is_pending = true;
		} else {
			has_cb = true;
		}
	}

	if (has_cb) {
		alarm.callback(dev, alarm_id, alarm.user_data);
	}
}
#endif

static int rtc_ti_mspm0_init(const struct device *dev)
{
	const struct rtc_ti_mspm0_config *cfg = dev->config;

	if (!cfg->rtc_x) {
		if (!(cfg->registers->PWREN & RTC_MSPM0_PWREN_MASK)) {
			/* Write Power enable key and set Power enable bit simultaneously */
			cfg->registers->PWREN =
				FIELD_PREP(RTC_MSPM0_PWREN_KEY_MASK, RTC_MSPM0_PWREN_KEY) |
				RTC_MSPM0_PWREN_MASK;
		}
	}
	/* Enable 32k clock supply */
	cfg->registers->CLKCTL = RTC_MSPM0_CLKCTL_ENABLE;
	/* Set clock format to binary */
	cfg->registers->CTL = RTC_MSPM0_CTL_BIN;

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
		.registers = (rtc_ti_mspm0_reg_t *)DT_INST_REG_ADDR(n),		\
		.rtc_x		 = DT_INST_PROP(n, ti_rtc_x),			\
		IF_ENABLED(CONFIG_RTC_ALARM,					\
		(.irq_config_func = ti_mspm0_config_irq_##n,))			\
	};									\
										\
DEVICE_DT_INST_DEFINE(n, &rtc_ti_mspm0_init, NULL, &rtc_data_##n,		\
		      &rtc_config_##n, PRE_KERNEL_1,				\
		      CONFIG_RTC_INIT_PRIORITY, &rtc_ti_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_TI_MSPM0_DEVICE_INIT);
