/*
 * Copyright (c) 2025 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_rtc

#include "rtc_utils.h"

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include <register.h>

LOG_MODULE_REGISTER(rtc_sf32lb, CONFIG_RTC_LOG_LEVEL);

#define RTC_TIMER      offsetof(RTC_TypeDef, TR)
#define RTC_DATER      offsetof(RTC_TypeDef, DR)
#define RTC_CR         offsetof(RTC_TypeDef, CR)
#define RTC_ISR        offsetof(RTC_TypeDef, ISR)
#define RTC_PSCLR      offsetof(RTC_TypeDef, PSCLR)
#define RTC_WUTR       offsetof(RTC_TypeDef, WUTR)
#define RTC_ALRMTR     offsetof(RTC_TypeDef, ALRMTR)
#define RTC_ALRMDR     offsetof(RTC_TypeDef, ALRMDR)

#define SYS_CFG_RTC_TR offsetof(HPSYS_CFG_TypeDef, RTC_TR)
#define SYS_CFG_RTC_DR offsetof(HPSYS_CFG_TypeDef, RTC_DR)

/*
 * The RTC clock, CLK_RTC, can be configured to use the LXT32 (32.768 kHz) or
 * LRC10 (9.8 kHz). The prescaler values need to be set such that the CLK1S
 * event runs at 1 Hz. The formula that relates prescaler values with the
 * clock frequency is as follows:
 *  F(CLK1S) = CLK_RTC / (DIV_A_INT + DIV_A_FRAC / 2^14) / DIV_B
 */
#define RC10K_DIVA_INT 38U
#define RC10K_DIVA_FRAC 4608U
#define RC10K_DIVB 256U

#ifdef CONFIG_RTC_ALARM
#define RTC_SF32LB_ALRM_MASK_ALL                                                                   \
	(RTC_ALRMDR_MSKS | RTC_ALRMDR_MSKMN | RTC_ALRMDR_MSKH |                                    \
		RTC_ALRMDR_MSKD | RTC_ALRMDR_MSKM | RTC_ALRMDR_MSKWD)

#define RTC_SF32LB_SUPPORTED_ALARM_FIELDS                                                          \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_MONTHDAY)
#endif

#ifdef CONFIG_RTC_ALARM
struct rtc_sf32lb_alarm_cb {
	rtc_alarm_callback cb;
	void *user_data;
};
#endif

struct rtc_sf32lb_data {
#ifdef CONFIG_RTC_ALARM
	struct rtc_sf32lb_alarm_cb alarm_cb;
	ATOMIC_DEFINE(is_pending, 1);
#endif
};

struct rtc_sf32lb_config {
	uintptr_t base;
	uintptr_t cfg;
#ifdef CONFIG_RTC_ALARM
	void (*irq_config_func)(void);
	uint16_t alarms_count;
#endif
};

#ifdef CONFIG_RTC_ALARM
static void rtc_irq_handler(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;
	struct rtc_sf32lb_data *data = dev->data;
	uint32_t isr = sys_read32(config->base + RTC_ISR);

	if (isr & RTC_ISR_ALRMF) {
		sys_clear_bit(config->base + RTC_ISR, RTC_ISR_ALRMF_Pos);

		atomic_set_bit(data->is_pending, 0);

		if (data->alarm_cb.cb) {
			data->alarm_cb.cb(dev, 0, data->alarm_cb.user_data);
		}
	}
}
#endif

static inline int rtc_sf32lb_enter_init_mode(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;

	sys_set_bit(config->base + RTC_ISR, RTC_ISR_INIT_Pos);

	while (!sys_test_bit(config->base + RTC_ISR, RTC_ISR_INITF_Pos)) {
	}

	return 0;
}

static inline int rtc_sf32lb_exit_init_mode(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;

	sys_clear_bit(config->base + RTC_ISR, RTC_ISR_INIT_Pos);

	return 0;
}

static inline void rtc_sf32lb_wait_for_sync(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;

	sys_clear_bit(config->base + RTC_ISR, RTC_ISR_RSF_Pos);
}

static int rtc_sf32lb_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t tr = 0;
	uint32_t dr = 0;

	tr = FIELD_PREP(RTC_TR_HT_Msk | RTC_TR_HU_Msk, bin2bcd(timeptr->tm_hour)) |
	     FIELD_PREP(RTC_TR_MNT_Msk | RTC_TR_MNU_Msk, bin2bcd(timeptr->tm_min)) |
	     FIELD_PREP(RTC_TR_ST_Msk | RTC_TR_SU_Msk, bin2bcd(timeptr->tm_sec)) |
	     FIELD_PREP(RTC_TR_SS_Msk, timeptr->tm_nsec * RC10K_DIVA_FRAC / 1000000000U);

	rtc_sf32lb_enter_init_mode(dev);
	sys_write32(tr, config->base + RTC_TIMER);
	rtc_sf32lb_exit_init_mode(dev);

	if (!sys_test_bit(config->base + RTC_CR, RTC_CR_BYPSHAD_Pos)) {
		rtc_sf32lb_wait_for_sync(dev);
	}

	if (timeptr->tm_year < 100) { /* 20th century */
		dr |= RTC_DR_CB;
		dr |= FIELD_PREP(RTC_DR_YT_Msk | RTC_DR_YU_Msk, bin2bcd(timeptr->tm_year));
	} else {
		dr |= FIELD_PREP(RTC_DR_YT_Msk | RTC_DR_YU_Msk, bin2bcd(timeptr->tm_year - 100));
	}
	dr |= FIELD_PREP(RTC_DR_MT_Msk | RTC_DR_MU_Msk, bin2bcd(timeptr->tm_mon + 1)) |
	      FIELD_PREP(RTC_DR_DT_Msk | RTC_DR_DU_Msk, bin2bcd(timeptr->tm_mday)) |
	      FIELD_PREP(RTC_DR_WD_Msk, timeptr->tm_wday);

	rtc_sf32lb_enter_init_mode(dev);
	sys_write32(dr, config->base + RTC_DATER);
	rtc_sf32lb_exit_init_mode(dev);

	if (!sys_test_bit(config->base + RTC_CR, RTC_CR_BYPSHAD_Pos)) {
		rtc_sf32lb_wait_for_sync(dev);
	}

	return 0;
}

static int rtc_sf32lb_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t reg;

	reg = sys_read32(config->cfg + SYS_CFG_RTC_TR);

	timeptr->tm_hour = bcd2bin(FIELD_GET(RTC_TR_HT_Msk | RTC_TR_HU_Msk, reg));
	timeptr->tm_min = bcd2bin(FIELD_GET(RTC_TR_MNT_Msk | RTC_TR_MNU_Msk, reg));
	timeptr->tm_sec = bcd2bin(FIELD_GET(RTC_TR_ST_Msk | RTC_TR_SU_Msk, reg));
	timeptr->tm_nsec = FIELD_GET(RTC_TR_SS_Msk, reg) * 1000000000U / RC10K_DIVA_FRAC;

	reg = sys_read32(config->cfg + SYS_CFG_RTC_DR);

	if (reg & RTC_DR_CB) { /* 20th century */
		uint8_t year = bcd2bin(FIELD_GET(RTC_DR_YT_Msk | RTC_DR_YU_Msk, reg));

		if (year < 70) {
			/* Year is 00-69, which should be 2000-2069. Clear CB bit */
			sys_clear_bit(config->base + RTC_DATER, RTC_DR_CB_Pos);

			timeptr->tm_year = year + 100;
		} else {
			/* Year is 70-99, which is 1970-1999. Keep CB bit set */
			timeptr->tm_year = year;
		}
	} else {
		timeptr->tm_year = bcd2bin(FIELD_GET(RTC_DR_YT_Msk | RTC_DR_YU_Msk, reg)) + 100;
	}
	timeptr->tm_mon = bcd2bin(FIELD_GET(RTC_DR_MT_Msk | RTC_DR_MU_Msk, reg)) - 1;
	timeptr->tm_mday = bcd2bin(FIELD_GET(RTC_DR_DT_Msk | RTC_DR_DU_Msk, reg));
	timeptr->tm_wday = FIELD_GET(RTC_DR_WD_Msk, reg);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int rtc_sf32lb_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						 uint16_t *mask)
{
	ARG_UNUSED(dev);

	if (id != 0) {
		return -EINVAL;
	}

	*mask = RTC_SF32LB_SUPPORTED_ALARM_FIELDS;

	return 0;
}

static int rtc_sf32lb_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				     const struct rtc_time *timeptr)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t alarm_tr;
	uint32_t alarm_dr;
	uint32_t alarm_mask = RTC_SF32LB_ALRM_MASK_ALL;

	if (id != 0) {
		return -EINVAL;
	}

	if ((mask > 0) && (timeptr == NULL)) {
		return -EINVAL;
	}

	if ((mask & ~RTC_SF32LB_SUPPORTED_ALARM_FIELDS) != 0) {
		LOG_ERR("unsupported alarm %d field mask 0x%04x", id, mask);
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		return -EINVAL;
	}

	if (mask == 0) {
		sys_clear_bits(config->base + RTC_CR, RTC_CR_ALRME | RTC_CR_ALRMIE);
	} else {
		alarm_tr = FIELD_PREP(RTC_ALRMTR_HT | RTC_ALRMTR_HU, bin2bcd(timeptr->tm_hour)) |
			   FIELD_PREP(RTC_ALRMTR_MNT | RTC_ALRMTR_MNU, bin2bcd(timeptr->tm_min)) |
			   FIELD_PREP(RTC_ALRMTR_ST | RTC_ALRMTR_SU, bin2bcd(timeptr->tm_sec));

		alarm_dr = FIELD_PREP(RTC_ALRMDR_DT | RTC_ALRMDR_DU, bin2bcd(timeptr->tm_mday)) |
			   FIELD_PREP(RTC_ALRMDR_MT | RTC_ALRMDR_MU, bin2bcd(timeptr->tm_mon + 1)) |
			   FIELD_PREP(RTC_ALRMDR_WD, timeptr->tm_wday);

		sys_write32(alarm_tr, config->base + RTC_ALRMTR);

		if (mask & RTC_ALARM_TIME_MASK_HOUR) {
			alarm_mask &= ~RTC_ALRMDR_MSKH;
		}
		if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
			alarm_mask &= ~RTC_ALRMDR_MSKMN;
		}
		if (mask & RTC_ALARM_TIME_MASK_SECOND) {
			alarm_mask &= ~RTC_ALRMDR_MSKS;
		}
		if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
			alarm_mask &= ~RTC_ALRMDR_MSKWD;
		}
		if (mask & RTC_ALARM_TIME_MASK_MONTH) {
			alarm_mask &= ~RTC_ALRMDR_MSKM;
		}
		if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
			alarm_mask &= ~RTC_ALRMDR_MSKD;
		}

		sys_write32(alarm_dr | alarm_mask, config->base + RTC_ALRMDR);
		sys_set_bits(config->base + RTC_CR, RTC_CR_ALRME | RTC_CR_ALRMIE);
	}

	return 0;
}

static int rtc_sf32lb_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				     struct rtc_time *timeptr)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t reg;

	if (id != 0 || mask == NULL || timeptr == NULL) {
		return -EINVAL;
	}

	reg = sys_read32(config->base + RTC_ALRMTR);

	timeptr->tm_hour = bcd2bin(FIELD_GET(RTC_ALRMTR_HT | RTC_ALRMTR_HU, reg));
	timeptr->tm_min = bcd2bin(FIELD_GET(RTC_ALRMTR_MNT | RTC_ALRMTR_MNU, reg));
	timeptr->tm_sec = bcd2bin(FIELD_GET(RTC_ALRMTR_ST | RTC_ALRMTR_SU, reg));

	reg = sys_read32(config->base + RTC_ALRMDR);

	timeptr->tm_mday = bcd2bin(FIELD_GET(RTC_ALRMDR_DT | RTC_ALRMDR_DU, reg));
	timeptr->tm_mon = bcd2bin(FIELD_GET(RTC_ALRMDR_MT | RTC_ALRMDR_MU, reg)) - 1;
	timeptr->tm_wday = FIELD_GET(RTC_ALRMDR_WD, reg);

	*mask = 0U;

	if ((reg & RTC_ALRMDR_MSKH) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if ((reg & RTC_ALRMDR_MSKMN) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if ((reg & RTC_ALRMDR_MSKS) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_SECOND;
	}
	if ((reg & RTC_ALRMDR_MSKD) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}
	if ((reg & RTC_ALRMDR_MSKM) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_MONTH;
	}
	if ((reg & RTC_ALRMDR_MSKWD) == 0) {
		*mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	return 0;
}

static int rtc_sf32lb_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_sf32lb_data *data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	return (int)atomic_test_and_clear_bit(data->is_pending, 0);
}

static int rtc_sf32lb_alarm_set_callback(const struct device *dev, uint16_t id,
					 rtc_alarm_callback callback, void *user_data)
{
	const struct rtc_sf32lb_config *config = dev->config;
	struct rtc_sf32lb_data *data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	data->alarm_cb.cb = callback;
	data->alarm_cb.user_data = user_data;

	if (callback == NULL) {
		sys_clear_bits(config->base + RTC_CR, RTC_CR_ALRME | RTC_CR_ALRMIE);
	}

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

static DEVICE_API(rtc, rtc_sf32lb_driver_api) = {
	.set_time = rtc_sf32lb_set_time,
	.get_time = rtc_sf32lb_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_sf32lb_alarm_get_supported_fields,
	.alarm_set_time = rtc_sf32lb_alarm_set_time,
	.alarm_get_time = rtc_sf32lb_alarm_get_time,
	.alarm_is_pending = rtc_sf32lb_alarm_is_pending,
	.alarm_set_callback = rtc_sf32lb_alarm_set_callback,
#endif
};

static int rtc_sf32lb_init(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t psclr = 0;

	psclr |= FIELD_PREP(RTC_PSCLR_DIVA_INT_Msk, RC10K_DIVA_INT);
	psclr |= FIELD_PREP(RTC_PSCLR_DIVA_FRAC_Msk, RC10K_DIVA_FRAC);
	psclr |= FIELD_PREP(RTC_PSCLR_DIVB_Msk, RC10K_DIVB);
	sys_write32(psclr, config->base + RTC_PSCLR);

	if (!sys_test_bit(config->base + RTC_CR, RTC_CR_BYPSHAD_Pos)) {
		rtc_sf32lb_wait_for_sync(dev);
	}

#ifdef CONFIG_RTC_ALARM
	if (config->irq_config_func) {
		config->irq_config_func();
	}
#endif
	return 0;
}

#define RTC_SF32LB_DEFINE(n)                                                                       \
	IF_ENABLED(CONFIG_RTC_ALARM, (                                                             \
		static void rtc_sf32lb_irq_config_func_##n(void);))                                \
	static const struct rtc_sf32lb_config rtc_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.cfg = DT_REG_ADDR(DT_INST_PHANDLE(n, sifli_cfg)),                                 \
		IF_ENABLED(CONFIG_RTC_ALARM,                                                       \
			(.irq_config_func = rtc_sf32lb_irq_config_func_##n,))                      \
				   IF_ENABLED(CONFIG_RTC_ALARM,                                    \
			(.alarms_count = DT_INST_PROP(n, alarms_count),)) };                       \
	static struct rtc_sf32lb_data rtc_sf32lb_data##n;                                          \
	DEVICE_DT_INST_DEFINE(n, rtc_sf32lb_init, NULL, &rtc_sf32lb_data##n,                       \
			      &rtc_sf32lb_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,       \
			      &rtc_sf32lb_driver_api);                                             \
	IF_ENABLED(CONFIG_RTC_ALARM,                                                               \
	(static void rtc_sf32lb_irq_config_func_##n(void)                                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), rtc_irq_handler,            \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}));

DT_INST_FOREACH_STATUS_OKAY(RTC_SF32LB_DEFINE)
