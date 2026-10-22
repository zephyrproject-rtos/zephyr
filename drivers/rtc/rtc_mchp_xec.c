/* rtc_mchp_xec.c - Microchip XEC Real Time Clock driver */

/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/dt-bindings/interrupt-controller/mchp-xec-ecia.h>
#include <soc.h>
#include "rtc_utils.h"

#define DT_DRV_COMPAT microchip_xec_rtc

LOG_MODULE_REGISTER(rtc_mchp_xec, CONFIG_RTC_LOG_LEVEL);

/* RTC HOURS register */
#define MCHP_RTC_HOURS_MASK       0x7F
#define MCHP_RTC_HOURS_AM_PM_MASK 0x80

/* RTC REGA register */
#define MCHP_RTC_REGA_UPDT_IN_PRGRSS_POS 7
#define MCHP_RTC_REGA_UPDT_IN_PRGRSS     BIT(MCHP_RTC_REGA_UPDT_IN_PRGRSS_POS)

/* RTC REGB register */
#define MCHP_RTC_REGB_UPDT_CYC_INHBT_POS   7
#define MCHP_RTC_REGB_UPDT_CYC_INHBT       BIT(MCHP_RTC_REGB_UPDT_CYC_INHBT_POS)
#define MCHP_RTC_REGB_PRDIC_INTR_EN_POS    6
#define MCHP_RTC_REGB_PRDIC_INTR_EN        BIT(MCHP_RTC_REGB_PRDIC_INTR_EN_POS)
#define MCHP_RTC_REGB_ALRM_INTR_EN_POS     5
#define MCHP_RTC_REGB_ALRM_INTR_EN         BIT(MCHP_RTC_REGB_ALRM_INTR_EN_POS)
#define MCHP_RTC_REGB_UPDT_END_INTR_EN_POS 4
#define MCHP_RTC_REGB_UPDT_END_INTR_EN     BIT(MCHP_RTC_REGB_UPDT_END_INTR_EN_POS)
#define MCHP_RTC_REGB_DATA_MODE_POS        2
#define MCHP_RTC_REGB_DATA_MODE            BIT(MCHP_RTC_REGB_DATA_MODE_POS)
#define MCHP_RTC_REGB_HOUR_FRMT_POS        1
#define MCHP_RTC_REGB_HOUR_FRMT            BIT(MCHP_RTC_REGB_HOUR_FRMT_POS)

/* RTC REGC register */
#define MCHP_RTC_REGC_OFFSET 0x0C

/* RTC Control Register */
#define MCHP_RTC_CTL_ALRM_EN_POS 3
#define MCHP_RTC_CTL_ALRM_EN     BIT(MCHP_RTC_CTL_ALRM_EN_POS)
#define MCHP_RTC_CTL_VCI_EN_POS  2
#define MCHP_RTC_CTL_VCI_EN      BIT(MCHP_RTC_CTL_VCI_EN_POS)
#define MCHP_RTC_CTL_SFT_RST_POS 1
#define MCHP_RTC_CTL_SFT_RST_EN  BIT(MCHP_RTC_CTL_SFT_RST_POS)
#define MCHP_RTC_CTL_BLCK_EN_POS 0
#define MCHP_RTC_CTL_BLCK_EN     BIT(MCHP_RTC_CTL_BLCK_EN_POS)

/* Alarm masks */
#ifdef CONFIG_RTC_ALARM
#define RTC_MCHP_ALARM_SUPPORTED_MASK                                                              \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_WEEKDAY)

#define MCHP_RTC_ALRM_DONT_CARE 0xC0
#endif /* CONFIG_RTC_ALARM */

/**
 * Structure type to access Real Time Clock (RTC).
 */
struct rtc_regs {
	volatile uint8_t SECS;
	volatile uint8_t SECS_ALRM;
	volatile uint8_t MINS;
	volatile uint8_t MINS_ALRM;
	volatile uint8_t HRS;
	volatile uint8_t HRS_ALRM;
	volatile uint8_t DAY_WK;
	volatile uint8_t DAY_MNTH;
	volatile uint8_t MNTH;
	volatile uint8_t YEAR;
	volatile uint8_t REGA;
	volatile uint8_t REGB;
	volatile uint8_t REGC;
	volatile uint8_t REGD;
	volatile uint8_t Reserved1;
	volatile uint8_t Reserved2;
	volatile uint32_t CTL;
	volatile uint32_t WK_ALRM;
	volatile uint32_t DAY_SV_FWRD;
	volatile uint32_t DAY_SV_BCK;
	volatile uint32_t TEST_MODE;
};

struct rtc_xec_config {
	mem_addr_t base;
	struct rtc_regs *regs;
	uint8_t girq_rtc;
	uint8_t girq_pos_rtc;
	uint8_t girq_rtc_alrm;
	uint8_t girq_pos_rtc_alrm;
	uint8_t enc_pcr;
};

struct rtc_xec_data {
	bool alrm_pending;
	bool is_bcd;
	bool is_24;
	struct k_sem sem;
	rtc_alarm_callback cb;
	void *cb_user_data;
};

static bool is_valid_bcd8(uint8_t v)
{
	return ((v & 0x0F) <= 9) && (((v >> 4) & 0x0F) <= 9);
}

static bool validate_rtc_alrm_time(const struct rtc_time *time_data, struct rtc_xec_data *data)
{
	uint8_t hrs = ((uint8_t)time_data->tm_hour) & MCHP_RTC_HOURS_MASK;
	uint8_t max_hrs = data->is_24 ? 23 : 12;

	if (data->is_bcd) {
		if (!is_valid_bcd8((uint8_t)time_data->tm_sec) ||
		    !is_valid_bcd8((uint8_t)time_data->tm_min) || !is_valid_bcd8(hrs)) {
			return false;
		}
	} else {
		if (((uint8_t)time_data->tm_sec > 59) || ((uint8_t)time_data->tm_min > 59) ||
		    (hrs > max_hrs)) {
			return false;
		}
	}

	return true;
}

static bool validate_rtc_time(const struct rtc_time *time_data, struct rtc_xec_data *data)
{
	uint8_t hrs = ((uint8_t)time_data->tm_hour) & MCHP_RTC_HOURS_MASK;
	uint8_t max_hrs = data->is_24 ? 23 : 12;

	if (data->is_bcd) {
		if (!is_valid_bcd8((uint8_t)time_data->tm_sec) ||
		    !is_valid_bcd8((uint8_t)time_data->tm_min) || !is_valid_bcd8(hrs) ||
		    !is_valid_bcd8((uint8_t)time_data->tm_wday) ||
		    !is_valid_bcd8((uint8_t)time_data->tm_mday) ||
		    !is_valid_bcd8((uint8_t)time_data->tm_mon) ||
		    !is_valid_bcd8((uint8_t)time_data->tm_year)) {
			return false;
		}
	} else {
		if (((uint8_t)time_data->tm_sec > 59) || ((uint8_t)time_data->tm_min > 59) ||
		    (hrs > max_hrs) || ((uint8_t)time_data->tm_wday > 12) ||
		    ((uint8_t)time_data->tm_mday > 31) || ((uint8_t)time_data->tm_mon > 12) ||
		    ((uint8_t)time_data->tm_year > 99)) {
			return false;
		}
	}

	return true;
}

static void rtc_enable(struct rtc_regs *regs, bool en)
{
	if (en) {
		regs->CTL |= MCHP_RTC_CTL_BLCK_EN;
	} else {
		regs->CTL &= ~MCHP_RTC_CTL_BLCK_EN;
	}
}

static void rtc_config_data_mode(struct rtc_regs *regs, struct rtc_xec_data *data)
{
#ifdef CONFIG_RTC_XEC_DATA_MODE_BCD
	regs->REGB &= ~MCHP_RTC_REGB_DATA_MODE;
	data->is_bcd = true;
#else
	regs->REGB |= MCHP_RTC_REGB_DATA_MODE;
	data->is_bcd = false;
#endif
}

static void rtc_enable_global_alrms(struct rtc_regs *regs, bool en)
{
	if (en) {
		regs->CTL |= MCHP_RTC_CTL_ALRM_EN;
	} else {
		regs->CTL &= ~MCHP_RTC_CTL_ALRM_EN;
	}
}

static void rtc_dis_alrm_intr(struct rtc_regs *regs, uint8_t flags)
{
	regs->REGB &= ~flags;
}

static void rtc_en_alrm_intr(struct rtc_regs *regs, uint8_t flags)
{
	regs->REGB |= flags;
}

static void rtc_enable_vci(struct rtc_regs *regs, bool en)
{
	if (en) {
		regs->CTL |= MCHP_RTC_CTL_VCI_EN;
	} else {
		regs->CTL &= ~MCHP_RTC_CTL_VCI_EN;
	}
}

static void rtc_config_hour_frmt(struct rtc_regs *regs, struct rtc_xec_data *data)
{
#ifdef CONFIG_RTC_XEC_HOUR_FORMAT_24
	regs->REGB |= MCHP_RTC_REGB_HOUR_FRMT;
	data->is_24 = true;
#else
	regs->REGB &= ~MCHP_RTC_REGB_HOUR_FRMT;
	data->is_24 = false;
#endif
}

static inline void rtc_sync_busy(struct rtc_regs *regs)
{
	while (regs->REGA & MCHP_RTC_REGA_UPDT_IN_PRGRSS) {
	}
}

static void rtc_xec_isr(const struct device *dev)
{
	struct rtc_xec_data *data = dev->data;
	struct rtc_xec_config const *cfg = dev->config;

	sys_read8(cfg->base + MCHP_RTC_REGC_OFFSET);

	soc_ecia_girq_status_clear(cfg->girq_rtc, cfg->girq_pos_rtc);

	if (data->cb) {
		data->cb(dev, 0, data->cb_user_data);
		data->alrm_pending = false;
	}

	rtc_dis_alrm_intr(cfg->regs, MCHP_RTC_REGB_ALRM_INTR_EN);
}

static int rtc_xec_set_time(const struct device *dev, const struct rtc_time *time_data)
{
	struct rtc_xec_data *data = dev->data;
	struct rtc_xec_config const *cfg = dev->config;
	struct rtc_regs *regs = cfg->regs;

	if (time_data == NULL || !validate_rtc_time(time_data, data)) {
		LOG_ERR("RTC set time failed: time_data pointer is NULL");
		return -EINVAL;
	}

	if (!validate_rtc_time(time_data, data)) {
		LOG_ERR("RTC time validation fail");
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	regs->REGB |= MCHP_RTC_REGB_UPDT_CYC_INHBT;

	regs->SECS = (uint8_t)time_data->tm_sec;
	regs->MINS = (uint8_t)time_data->tm_min;
	regs->HRS = ((uint8_t)time_data->tm_hour) & MCHP_RTC_HOURS_MASK;
	if (!data->is_24) {
		regs->HRS &= ~MCHP_RTC_HOURS_AM_PM_MASK;
		regs->HRS |= ((uint8_t)time_data->tm_hour) & MCHP_RTC_HOURS_AM_PM_MASK;
	}
	regs->DAY_WK = (uint8_t)time_data->tm_wday;
	regs->DAY_MNTH = (uint8_t)time_data->tm_mday;
	regs->MNTH = (uint8_t)time_data->tm_mon;
	regs->YEAR = (uint8_t)time_data->tm_year;

	regs->REGB &= ~MCHP_RTC_REGB_UPDT_CYC_INHBT;

	k_sem_give(&data->sem);

	return 0;
}

static int rtc_xec_get_time(const struct device *dev, struct rtc_time *time_data)
{
	struct rtc_xec_data *data = dev->data;
	struct rtc_xec_config const *cfg = dev->config;
	struct rtc_regs *regs = cfg->regs;

	k_sem_take(&data->sem, K_FOREVER);

	rtc_sync_busy(cfg->regs);

	time_data->tm_sec = regs->SECS;
	time_data->tm_min = regs->MINS;
	time_data->tm_hour = regs->HRS;
	time_data->tm_wday = regs->DAY_WK;
	time_data->tm_mday = regs->DAY_MNTH;
	time_data->tm_mon = regs->MNTH;
	time_data->tm_year = regs->YEAR;

	k_sem_give(&data->sem);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static int rtc_xec_set_alarm_time(const struct device *dev, uint16_t alarm_id, uint16_t alarm_mask,
				  const struct rtc_time *time_data)
{
	struct rtc_xec_data *data = dev->data;
	struct rtc_xec_config const *cfg = dev->config;
	struct rtc_regs *regs = cfg->regs;

	if (time_data == NULL) {
		LOG_ERR("RTC time_data pointer is null");
		return -EINVAL;
	}

	if ((alarm_mask & ~RTC_MCHP_ALARM_SUPPORTED_MASK) != 0) {
		LOG_ERR("Invalid RTC alarm mask");
		return -EINVAL;
	}

	if (!validate_rtc_alrm_time(time_data, data)) {
		LOG_ERR("RTC time validation fail");
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	regs->SECS_ALRM = (alarm_mask & RTC_ALARM_TIME_MASK_SECOND) ? (uint8_t)time_data->tm_sec
								    : MCHP_RTC_ALRM_DONT_CARE;
	regs->MINS_ALRM = (alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) ? (uint8_t)time_data->tm_min
								    : MCHP_RTC_ALRM_DONT_CARE;
	regs->WK_ALRM = (alarm_mask & RTC_ALARM_TIME_MASK_WEEKDAY) ? (uint8_t)time_data->tm_wday
								   : MCHP_RTC_ALRM_DONT_CARE;

	if (alarm_mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs->HRS_ALRM = ((uint8_t)time_data->tm_hour) & MCHP_RTC_HOURS_MASK;
		if (!data->is_24) {
			regs->HRS_ALRM &= ~MCHP_RTC_HOURS_AM_PM_MASK;
			regs->HRS_ALRM |= ((uint8_t)time_data->tm_hour) & MCHP_RTC_HOURS_AM_PM_MASK;
		}
	} else {
		regs->HRS_ALRM = MCHP_RTC_ALRM_DONT_CARE;
	}

	rtc_en_alrm_intr(regs, MCHP_RTC_REGB_ALRM_INTR_EN);
	data->alrm_pending = true;

	k_sem_give(&data->sem);

	return 0;
}

static int rtc_xec_get_alarm_time(const struct device *dev, uint16_t alarm_id, uint16_t *alarm_mask,
				  struct rtc_time *time_data)
{
	struct rtc_xec_data *data = dev->data;
	struct rtc_xec_config const *cfg = dev->config;
	struct rtc_regs *regs = cfg->regs;
	uint8_t secs, mins, hrs, wka;

	k_sem_take(&data->sem, K_FOREVER);

	secs = regs->SECS_ALRM;
	mins = regs->MINS_ALRM;
	hrs = regs->HRS_ALRM;
	wka = (uint8_t)regs->WK_ALRM;

	*alarm_mask = 0;

	if ((secs & MCHP_RTC_ALRM_DONT_CARE) != MCHP_RTC_ALRM_DONT_CARE) {
		time_data->tm_sec = secs;
		*alarm_mask |= RTC_ALARM_TIME_MASK_SECOND;
	}

	if ((mins & MCHP_RTC_ALRM_DONT_CARE) != MCHP_RTC_ALRM_DONT_CARE) {
		time_data->tm_min = mins;
		*alarm_mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}

	if ((wka & MCHP_RTC_ALRM_DONT_CARE) != MCHP_RTC_ALRM_DONT_CARE) {
		time_data->tm_wday = wka;
		*alarm_mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	if ((hrs & MCHP_RTC_ALRM_DONT_CARE) != MCHP_RTC_ALRM_DONT_CARE) {
#ifdef CONFIG_RTC_XEC_HOUR_FORMAT_24
		time_data->tm_hour = hrs & MCHP_RTC_HOURS_MASK;
#else
		time_data->tm_hour = hrs;
#endif
		*alarm_mask |= RTC_ALARM_TIME_MASK_HOUR;
	}

	k_sem_give(&data->sem);

	return 0;
}

static int rtc_xec_get_alarm_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	*mask = (RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE |
		 RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_WEEKDAY);

	return 0;
}

static int rtc_xec_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_xec_data *data = dev->data;
	int ret;

	k_sem_take(&data->sem, K_FOREVER);

	ret = data->alrm_pending ? 1 : 0;
	k_sem_give(&data->sem);

	return ret;
}

static int rtc_xec_set_alarm_callback(const struct device *dev, uint16_t id,
				      rtc_alarm_callback callback, void *user_data)
{
	struct rtc_xec_data *data = dev->data;

	if (callback == NULL) {
		LOG_ERR("Callback function not assigned");
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	data->cb = callback;
	data->cb_user_data = user_data;
	k_sem_give(&data->sem);

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

static int rtc_xec_init(const struct device *dev)
{
	struct rtc_xec_config const *cfg = dev->config;
	struct rtc_xec_data *data = dev->data;

	k_sem_init(&data->sem, 1, 1);
	soc_xec_pcr_sleep_en_clear(cfg->enc_pcr);
	rtc_config_hour_frmt(cfg->regs, data);
	rtc_config_data_mode(cfg->regs, data);
	rtc_enable_global_alrms(cfg->regs, true);
	uint8_t flags = (MCHP_RTC_REGB_PRDIC_INTR_EN | MCHP_RTC_REGB_ALRM_INTR_EN |
			 MCHP_RTC_REGB_UPDT_END_INTR_EN);
	rtc_dis_alrm_intr(cfg->regs, flags);
	rtc_enable_vci(cfg->regs, true);
	rtc_enable(cfg->regs, true);

	soc_ecia_girq_ctrl(cfg->girq_rtc, cfg->girq_pos_rtc, 1);

	/* RTC Interrupt */
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(0, 0), DT_INST_IRQ_BY_IDX(0, 0, priority), rtc_xec_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN_BY_IDX(0, 0));

	return 0;
}

static DEVICE_API(rtc, rtc_xec_api) = {
	.set_time = rtc_xec_set_time,
	.get_time = rtc_xec_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_xec_get_alarm_supported_fields,
	.alarm_is_pending = rtc_xec_alarm_is_pending,
	.alarm_set_time = rtc_xec_set_alarm_time,
	.alarm_get_time = rtc_xec_get_alarm_time,
	.alarm_set_callback = rtc_xec_set_alarm_callback,
#endif /* CONFIG_RTC_ALARM */
};

#define DEV_CFG_GIRQ(inst, idx)     MCHP_XEC_ECIA_GIRQ(DT_INST_PROP_BY_IDX(inst, girqs, idx))
#define DEV_CFG_GIRQ_POS(inst, idx) MCHP_XEC_ECIA_GIRQ_POS(DT_INST_PROP_BY_IDX(inst, girqs, idx))

static const struct rtc_xec_config rtc_xec_config_0 = {
	.base = DT_INST_REG_ADDR(0),
	.regs = (struct rtc_regs *)(DT_INST_REG_ADDR(0)),
	.girq_rtc = DEV_CFG_GIRQ(0, 0),
	.girq_pos_rtc = DEV_CFG_GIRQ_POS(0, 0),
	.girq_rtc_alrm = DEV_CFG_GIRQ(0, 1),
	.girq_pos_rtc_alrm = DEV_CFG_GIRQ_POS(0, 1),
	.enc_pcr = DT_INST_PROP(0, pcr_scr),
};

static struct rtc_xec_data rtc_xec_dev_data;

DEVICE_DT_INST_DEFINE(0, rtc_xec_init, NULL, &rtc_xec_dev_data, &rtc_xec_config_0, POST_KERNEL,
		      CONFIG_RTC_INIT_PRIORITY, &rtc_xec_api);
