/*
 * Copyright (c) 2026 ITE Corporation. All Rights Reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it8xxx2_rtc

#include <soc.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "rtc_utils.h"

LOG_MODULE_REGISTER(rtc_ite_it8xxx2, CONFIG_RTC_LOG_LEVEL);

#define RTC_ALARM_COUNT DT_INST_PROP(0, alarms_count)

#ifdef CONFIG_RTC_ALARM
struct rtc_alarm_config {
	int alarm_irq;
	int alarm_flag;
};
#endif /* CONFIG_RTC_ALARM */

struct rtc_ite_config {
	mm_reg_t reg_ec2i;
	mm_reg_t reg_gpiogcr;
	mm_reg_t reg_gctrl;
#ifdef CONFIG_RTC_ALARM
	/* RTC alarm irq */
	void (*irq_config_func)(const struct device *dev);
	struct rtc_alarm_config alarm_cfgs[RTC_ALARM_COUNT];
	uint8_t alarms_count;
#endif /* CONFIG_RTC_ALARM */
};

#ifdef CONFIG_RTC_ALARM
/* This structure holds the callback information for RTC alarm events. */
struct rtc_ite_data_cb {
	bool is_alarm_pending;
	rtc_alarm_callback alarm_cb;
	void *alarm_user_data;
};
#endif /* CONFIG_RTC_ALARM */

struct rtc_ite_data {
	struct k_sem sem;
#ifdef CONFIG_RTC_ALARM
	struct rtc_ite_data_cb alarm_callback[RTC_ALARM_COUNT];
	uint16_t alarm_mask[RTC_ALARM_COUNT];

#endif /* CONFIG_RTC_ALARM */
	bool daylight_saving;
};

/* EC2I access index/data port */
enum rtc_ec2i_access {
	/* Bank0 */
	RTC_EC2I_ACCESS_B0_INDEX = 0,
	RTC_EC2I_ACCESS_B0_DATA = 1,
	/* Bank1 */
	RTC_EC2I_ACCESS_B1_INDEX = 2,
	RTC_EC2I_ACCESS_B1_DATA = 3,
};

enum rtc_bank_sel {
	B0,
	B1,
};

/* EC access to host controlled modules (EC2I Bridge) */
/* 0x00: Indirect host I/O address register */
#define RTC_EC2I_IHIOA       0x00
/* 0x01: Indirect host data register */
#define RTC_EC2I_IHD         0x01
/* 0x02: Lock super I/O host access register */
#define RTC_EC2I_LSIOHA      0x02
/* 0x03: Super I/O access lock violation register */
#define RTC_EC2I_SIOLV       0x03
/* 0x04: EC to I-Bus modules access enable register */
#define RTC_EC2I_IBMAE       0x04
/* Real-time clock (RTC) EC access enable */
#define RTC_EC2I_IBMAE_RTCAE BIT(1)
/* 0x05: I-Bus control register */
#define RTC_EC2I_IBCTL       0x05
/* EC write to I-Bus */
#define RTC_EC2I_IBCTL_CWIB  BIT(2)
/* EC read from I-Bus */
#define RTC_EC2I_IBCTL_CRIB  BIT(1)
#define RTC_EC2I_IBCTL_CRWIB (RTC_EC2I_IBCTL_CRIB | RTC_EC2I_IBCTL_CWIB)
/* EC to I-Bus access enabled */
#define RTC_EC2I_IBCTL_CSAE  BIT(0)

#define RTC_EC2I_WAIT_STATUS_TIMEOUT (USEC_PER_MSEC * 50)

/* General Purpose I/O Port (GPIO) registers */
#define RTC_GPIOGCR_CLKSRC    0xfa
#define RTC_CLKSRC_CRYSTAL    BIT(7)
#define RTC_CLKSRC_OSCILLATOR BIT(3)
#define RTC_CLKSRC_SEL        BIT(0)

/* General Control (GCTRL) registers */
#define RTC_GCTRL_RSTS    0x06
#define RTC_RSTS_VCCDO(n) FIELD_PREP(GENMASK(7, 6), n)
/* The VCC power status is treated as power-off */
#define RTC_VCC_OFF       0
/* The VCC power status is treated as power-on */
#define RTC_VCC_ON        1
#define RTC_RSTS_HGRST    BIT(3)

/* Host view register map via index-data I/O pair, RTC bank 0 */
/* 0x00: Seconds register */
#define RTC_SECREG   0x00
/* 0x01: Seconds alarm 1 register */
#define RTC_SECA1REG 0x01
/* 0x02: Minutes register */
#define RTC_MINREG   0x02
/* 0x03: Minutes alarm 1 register */
#define RTC_MINA1REG 0x03
/* 0x04: Hours register */
#define RTC_HRREG    0x04
/* 0x05: Hours alarm 1 register */
#define RTC_HRA1REG  0x05
/* 0x06: Day of week register */
#define RTC_DOWREG   0x06
/* 0x07: Day of month register */
#define RTC_DOMREG   0x07
/* 0x08: Month register */
#define RTC_MONREG   0x08
/* 0x09: Year register */
#define RTC_YRREG    0x09

/* 0x0a: RTC control register A */
#define RTC_CTLREGA           0x0a
#define RTC_CTLREGA_DICCTL(n) FIELD_PREP(GENMASK(6, 4), n)
/* Normal operation */
#define RTC_NORMAL_OP         2
/* Divider chain reset */
#define RTC_DIV_CHAIN_RST     6

/* 0x0b: RTC control register B */
#define RTC_CTLREGB      0x0b
#define RTC_CTLREGB_SM   BIT(7)
#define RTC_CTLREGB_PIE  BIT(6)
#define RTC_CTLREGB_A1IE BIT(5)
#define RTC_CTLREGB_UEIE BIT(4)
#define RTC_CTLREGB_A2IE BIT(3)
#define RTC_CTLREGB_DAM  BIT(2)
#define RTC_CTLREGB_HRM  BIT(1)
#define RTC_CTLREGB_DS   BIT(0)
/* 0x0c: RTC control register C */
#define RTC_CTLREGC      0x0c
#define RTC_CTLREGC_A1IF BIT(5)
#define RTC_CTLREGC_A2IF BIT(3)
/* 0x0e: Day of week alarm 1 register */
#define RTC_DOWA1REG     0x0e
/* 0x49: Day of month alarm 1 register */
#define RTC_DOMA1REG     0x49
/* 0x4a: Month alarm 1 register */
#define RTC_MONA1REG     0x4a
/* Host view register map via index-data I/O pair, RTC bank 1 */
/* 0x00: Seconds alarm 2 register */
#define RTC_SECA2REG     0x00
/* 0x01: Minutes alarm 2 register */
#define RTC_MINA2REG     0x01
/* 0x02: Hours alarm 2 register */
#define RTC_HRA2REG      0x02
/* 0x03: Day of month alarm 2 register */
#define RTC_DOMA2REG     0x03
/* 0x04: Month alarm 2 register */
#define RTC_MONA2REG     0x04
/* 0x05: Day of week alarm 2 register */
#define RTC_DOWA2REG     0x05

#define RTC_SECAREG(sel) ((sel) == B0 ? RTC_SECA1REG : RTC_SECA2REG)
#define RTC_MINAREG(sel) ((sel) == B0 ? RTC_MINA1REG : RTC_MINA2REG)
#define RTC_HRAREG(sel)  ((sel) == B0 ? RTC_HRA1REG : RTC_HRA2REG)
#define RTC_DOMAREG(sel) ((sel) == B0 ? RTC_DOMA1REG : RTC_DOMA2REG)
#define RTC_MONAREG(sel) ((sel) == B0 ? RTC_MONA1REG : RTC_MONA2REG)
#define RTC_DOWAREG(sel) ((sel) == B0 ? RTC_DOWA1REG : RTC_DOWA2REG)

#define RTC_CTLREGC_AIF(sel) ((sel) == B0 ? RTC_CTLREGC_A1IF : RTC_CTLREGC_A2IF)

#define RTC_ITE_SUPPORTED_TIME_FIELDS                                                              \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_YEAR |     \
	 RTC_ALARM_TIME_MASK_WEEKDAY)
/* Zephyr mask supported by RTC ITE device, values from RTC_ALARM_TIME_MASK */
#define RTC_ITE_SUPPORTED_ALARM_FIELDS                                                             \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH | RTC_ALARM_TIME_MASK_WEEKDAY)

/* struct tm start year: 1900 */
#define TM_YEAR_REF      1900U
/* RTC support 2000 ~ 2099 */
#define RTC_ITE_MIN_YEAR 2000U
#define RTC_ITE_MAX_YEAR 2099U

static void rtc_it8xxx2_update_gctrl(mm_reg_t addr, uint8_t mask, uint8_t val)
{
	uint8_t reg = sys_read8(addr);

	reg = (reg & ~mask) | (val & mask);

	sys_write8(reg, addr);
}

static inline void ec2i_ite_wait_status_cleared(const struct device *dev, uint8_t mask)
{
	const struct rtc_ite_config *const config = dev->config;

	/*
	 * Though the internal bus normally completes immediately, WAIT_FOR with a timeout
	 * ensures the code cannot hang if unexpected hardware issues occur.
	 */
	if (!WAIT_FOR(!(sys_read8(config->reg_ec2i + RTC_EC2I_IBCTL) & mask),
		      RTC_EC2I_WAIT_STATUS_TIMEOUT, NULL)) {
		LOG_ERR("%s: Timeout waiting for status", __func__);
		return;
	}
}

static inline void ec2i_ite_write_rtc_bank(const struct device *dev, enum rtc_ec2i_access sel,
					   uint8_t data)
{
	const struct rtc_ite_config *config = dev->config;

	/* Wait that both CWIB and CRIB bits in IBCTL register are cleared. */
	ec2i_ite_wait_status_cleared(dev, RTC_EC2I_IBCTL_CRWIB);
	/* Set indirect host I/O offset. */
	sys_write8(sel, config->reg_ec2i + RTC_EC2I_IHIOA);
	/* Write the data to IHD register */
	sys_write8(data, config->reg_ec2i + RTC_EC2I_IHD);
	/* EC access to the RTC registers is enabled */
	sys_write8(RTC_EC2I_IBMAE_RTCAE, config->reg_ec2i + RTC_EC2I_IBMAE);
	/* bit0: EC to I-Bus access enabled. */
	sys_write8(sys_read8(config->reg_ec2i + RTC_EC2I_IBCTL) | RTC_EC2I_IBCTL_CSAE,
		   config->reg_ec2i + RTC_EC2I_IBCTL);
	/* Wait the CWIB bit in IBCTL cleared. */
	ec2i_ite_wait_status_cleared(dev, RTC_EC2I_IBCTL_CWIB);

	/* EC access to the RTC registers is disabled */
	sys_write8(sys_read8(config->reg_ec2i + RTC_EC2I_IBMAE) & ~RTC_EC2I_IBMAE_RTCAE,
		   config->reg_ec2i + RTC_EC2I_IBMAE);
	/* Disable EC to I-Bus access. */
	sys_write8(sys_read8(config->reg_ec2i + RTC_EC2I_IBCTL) & ~RTC_EC2I_IBCTL_CSAE,
		   config->reg_ec2i + RTC_EC2I_IBCTL);
}

static inline uint8_t ec2i_ite_read_rtc_bank(const struct device *dev, enum rtc_ec2i_access sel)
{
	const struct rtc_ite_config *config = dev->config;
	uint8_t data;

	/* Wait that both CWIB and CRIB bits in IBCTL register are cleared. */
	ec2i_ite_wait_status_cleared(dev, RTC_EC2I_IBCTL_CRWIB);
	/* Set indirect host I/O offset. */
	sys_write8(sel, config->reg_ec2i + RTC_EC2I_IHIOA);
	/* EC access to the RTC registers is enabled */
	sys_write8(RTC_EC2I_IBMAE_RTCAE, config->reg_ec2i + RTC_EC2I_IBMAE);
	/* bit0: EC to I-Bus access enabled. */
	sys_write8(RTC_EC2I_IBCTL_CRIB | RTC_EC2I_IBCTL_CSAE, config->reg_ec2i + RTC_EC2I_IBCTL);
	/* Wait the CWIB bit in IBCTL cleared. */
	ec2i_ite_wait_status_cleared(dev, RTC_EC2I_IBCTL_CRIB);
	/* Read the data to IHD register */
	data = sys_read8(config->reg_ec2i + RTC_EC2I_IHD);

	/* EC access to the RTC registers is disabled */
	sys_write8(sys_read8(config->reg_ec2i + RTC_EC2I_IBMAE) & ~RTC_EC2I_IBMAE_RTCAE,
		   config->reg_ec2i + RTC_EC2I_IBMAE);
	/* Disable EC to I-Bus access. */
	sys_write8(sys_read8(config->reg_ec2i + RTC_EC2I_IBCTL) & ~RTC_EC2I_IBCTL_CSAE,
		   config->reg_ec2i + RTC_EC2I_IBCTL);

	return data;
}

static inline void rtc_ec2i_ite_write(const struct device *dev, uint8_t index, uint8_t data,
				      enum rtc_bank_sel bank)
{
	enum rtc_ec2i_access ec2i_index, ec2i_data;

	ec2i_index = (bank == B0) ? RTC_EC2I_ACCESS_B0_INDEX : RTC_EC2I_ACCESS_B1_INDEX;
	ec2i_data = (bank == B0) ? RTC_EC2I_ACCESS_B0_DATA : RTC_EC2I_ACCESS_B1_DATA;

	/* Set index */
	ec2i_ite_write_rtc_bank(dev, ec2i_index, index);
	/* Write data */
	ec2i_ite_write_rtc_bank(dev, ec2i_data, data);
}

static inline uint8_t rtc_ec2i_ite_read(const struct device *dev, uint8_t index,
					enum rtc_bank_sel bank)
{
	enum rtc_ec2i_access ec2i_index, ec2i_data;

	ec2i_index = (bank == B0) ? RTC_EC2I_ACCESS_B0_INDEX : RTC_EC2I_ACCESS_B1_INDEX;
	ec2i_data = (bank == B0) ? RTC_EC2I_ACCESS_B0_DATA : RTC_EC2I_ACCESS_B1_DATA;

	/* Set index */
	ec2i_ite_write_rtc_bank(dev, ec2i_index, index);
	/* Read data */
	return ec2i_ite_read_rtc_bank(dev, ec2i_data);
}

static int rtc_it8xxx2_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_ite_data *data = dev->data;
	int year;
	uint8_t ctlregb;

	if (timeptr == NULL) {
		LOG_ERR("RTC set time failed: time pointer is NULL");
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, RTC_ITE_SUPPORTED_TIME_FIELDS)) {
		LOG_ERR("Invalid RTC time provided");
		return -EINVAL;
	}

	year = timeptr->tm_year + TM_YEAR_REF;
	/* RTC supports only years 2000-2099 */
	if (year < RTC_ITE_MIN_YEAR || year > RTC_ITE_MAX_YEAR) {
		LOG_ERR("RTC year %d out of HW range (%d-%d)", year, RTC_ITE_MIN_YEAR,
			RTC_ITE_MAX_YEAR);
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* Configure RTC control register B (data/hour/DST mode) */
	ctlregb = rtc_ec2i_ite_read(dev, RTC_CTLREGB, B0);
	/* Daylight saving control */
	if (timeptr->tm_isdst > 0) {
		ctlregb |= timeptr->tm_isdst ? RTC_CTLREGB_DS : 0;
		data->daylight_saving = true;
	}
	/* Update cycles will not occur until set mode (SM) bit is 0 */
	rtc_ec2i_ite_write(dev, RTC_CTLREGB, RTC_CTLREGB_SM | ctlregb, B0);

	/* Seconds (0-59) */
	rtc_ec2i_ite_write(dev, RTC_SECREG, bin2bcd(timeptr->tm_sec), B0);
	/* Minutes (0-59) */
	rtc_ec2i_ite_write(dev, RTC_MINREG, bin2bcd(timeptr->tm_min), B0);
	/* Hours (0-23, 12 or 24-hour mode) */
	rtc_ec2i_ite_write(dev, RTC_HRREG, bin2bcd(timeptr->tm_hour), B0);
	/* Day of month (1-31) */
	rtc_ec2i_ite_write(dev, RTC_DOMREG, bin2bcd(timeptr->tm_mday), B0);
	/* Month: tm_mon (0-11), RTC stores (1-12) */
	rtc_ec2i_ite_write(dev, RTC_MONREG, bin2bcd(timeptr->tm_mon + 1), B0);
	/* Year: tm_year since 1900, RTC stores (0-99) for 2000-2099 */
	rtc_ec2i_ite_write(dev, RTC_YRREG, bin2bcd(timeptr->tm_year - 100), B0);
	/* Day of week: tm_wday (0-6, Sunday=0), RTC stores (1-7) */
	rtc_ec2i_ite_write(dev, RTC_DOWREG, bin2bcd(timeptr->tm_wday + 1), B0);

	/* Timing updates occur normally */
	ctlregb &= ~RTC_CTLREGB_SM;
	rtc_ec2i_ite_write(dev, RTC_CTLREGB, ctlregb, B0);

	k_sem_give(&data->sem);

	LOG_DBG("RTC set time (UTC): %04d-%02d-%02d %02d:%02d:%02d (wday=%d)",
		timeptr->tm_year + TM_YEAR_REF, timeptr->tm_mon + 1, timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, timeptr->tm_wday);

	return 0;
}

static int rtc_it8xxx2_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_ite_data *data = dev->data;

	if (timeptr == NULL) {
		LOG_ERR("RTC get time failed: time pointer is NULL");
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	/* Seconds (0-59) */
	timeptr->tm_sec = bcd2bin(rtc_ec2i_ite_read(dev, RTC_SECREG, B0));
	/* Minutes (0-59) */
	timeptr->tm_min = bcd2bin(rtc_ec2i_ite_read(dev, RTC_MINREG, B0));
	/* Hours (0-23, 12 or 24-hour mode) */
	timeptr->tm_hour = bcd2bin(rtc_ec2i_ite_read(dev, RTC_HRREG, B0));
	/* Day of month (1-31) */
	timeptr->tm_mday = bcd2bin(rtc_ec2i_ite_read(dev, RTC_DOMREG, B0));
	/* Month: tm_mon is 0-11, RTC stores 1-12 */
	timeptr->tm_mon = bcd2bin(rtc_ec2i_ite_read(dev, RTC_MONREG, B0)) - 1;
	/* Year: tm_year since 1900, RTC stores (0-99) for 2000-2099  */
	timeptr->tm_year = bcd2bin(rtc_ec2i_ite_read(dev, RTC_YRREG, B0)) + 100;
	/* Day of week: tm_wday (0-6, Sunday=0), RTC stores (1-7) */
	timeptr->tm_wday = bcd2bin(rtc_ec2i_ite_read(dev, RTC_DOWREG, B0)) - 1;
	/* Set tm_isdst based on RTC daylight saving configuration */
	if (data->daylight_saving) {
		timeptr->tm_isdst = 1;
	}

	/* There is no nano second support for RTC */
	timeptr->tm_nsec = 0;
	/* There is no day of the year support for RTC */
	timeptr->tm_yday = -1;

	k_sem_give(&data->sem);

	LOG_DBG("RTC get time: %04d-%02d-%02d %02d:%02d:%02d (wday=%d, DST=%sable)",
		timeptr->tm_year + TM_YEAR_REF, timeptr->tm_mon + 1, timeptr->tm_mday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec, timeptr->tm_wday,
		data->daylight_saving ? "en" : "dis");

	return 0;
}

#ifdef CONFIG_RTC_ALARM
static void rtc_it8xxx2_enable_alarm_irq(const struct device *dev, enum rtc_bank_sel sel)
{
	const struct rtc_ite_config *const config = dev->config;

	/* Configure interrupt polarity */
	ite_intc_irq_polarity_set(config->alarm_cfgs[sel].alarm_irq,
				  config->alarm_cfgs[sel].alarm_flag);
	/* Clear pending interrupt status (W/C) */
	ite_intc_isr_clear(config->alarm_cfgs[sel].alarm_irq);
	/* Enable IRQ */
	irq_enable(config->alarm_cfgs[sel].alarm_irq);
}

static inline int rtc_it8xxx2_validate_alarm_id(const struct device *dev, uint16_t id,
						const char *func)
{
	const struct rtc_ite_config *const config = dev->config;

	if (id >= config->alarms_count) {
		LOG_ERR("%s: RTC alarm ID %d is out of range (max %d)", func, id,
			config->alarms_count - 1);
		return -EINVAL;
	}

	return 0;
}

static int rtc_it8xxx2_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						  uint16_t *mask)
{
	if (rtc_it8xxx2_validate_alarm_id(dev, id, __func__)) {
		return -EINVAL;
	}

	*mask = RTC_ITE_SUPPORTED_ALARM_FIELDS;

	return 0;
}

static int rtc_it8xxx2_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr)
{
	struct rtc_ite_data *data = dev->data;
	uint16_t mask_capable;
	uint8_t ctlregb;

	rtc_it8xxx2_alarm_get_supported_fields(dev, id, &mask_capable);

	if (rtc_it8xxx2_validate_alarm_id(dev, id, __func__)) {
		return -EINVAL;
	}

	if ((mask != 0) && (timeptr == NULL)) {
		LOG_ERR("No pointer is provided to set alarm");
		return -EINVAL;
	}

	if (mask & ~mask_capable) {
		LOG_ERR("Invalid RTC alarm mask");
		return -EINVAL;
	}

	if (!rtc_utils_validate_rtc_time(timeptr, mask)) {
		LOG_ERR("Invalid RTC alarm time provided");
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		/* Alarm seconds (0-59) */
		rtc_ec2i_ite_write(dev, RTC_SECAREG(id), bin2bcd(timeptr->tm_sec), id);
	}
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		/* Alarm minutes (0-59) */
		rtc_ec2i_ite_write(dev, RTC_MINAREG(id), bin2bcd(timeptr->tm_min), id);
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		/* Alarm hours (0-23, 12 or 24-hour mode) */
		rtc_ec2i_ite_write(dev, RTC_HRAREG(id), bin2bcd(timeptr->tm_hour), id);
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		/* Alarm day of month (1-31) */
		rtc_ec2i_ite_write(dev, RTC_DOMAREG(id), bin2bcd(timeptr->tm_mday), id);
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		/* Alarm month: RTC: 1-12 struct tm_mon: 0-11 */
		rtc_ec2i_ite_write(dev, RTC_MONAREG(id), bin2bcd(timeptr->tm_mon + 1), id);
	}
	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		/* Alarm weekday: tm_wday 0-6 Sunday = 0 */
		rtc_ec2i_ite_write(dev, RTC_DOWAREG(id), bin2bcd(timeptr->tm_wday + 1), id);
	}

	/* Setting interrupt */
	rtc_it8xxx2_enable_alarm_irq(dev, id);

	/* Setting alarm1 or alarm2 interrupt */
	ctlregb = rtc_ec2i_ite_read(dev, RTC_CTLREGB, B0);
	ctlregb |= (id == B0) ? RTC_CTLREGB_A1IE : RTC_CTLREGB_A2IE;
	/* Configure RTC control register B */
	rtc_ec2i_ite_write(dev, RTC_CTLREGB, ctlregb, B0);

	data->alarm_mask[id] = mask;

	k_sem_give(&data->sem);

	LOG_DBG("RTC set alarm%d time (UTC): %02d-%02d %02d:%02d:%02d (wday=%d)", id + 1,
		timeptr->tm_mon + 1, timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min,
		timeptr->tm_sec, timeptr->tm_wday);

	return 0;
}

static int rtc_it8xxx2_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr)
{
	struct rtc_ite_data *data = dev->data;
	uint16_t curr_alarm_mask = data->alarm_mask[id];
	uint16_t return_mask = 0;

	if (rtc_it8xxx2_validate_alarm_id(dev, id, __func__)) {
		return -EINVAL;
	}

	if ((mask == NULL) || (timeptr == NULL)) {
		LOG_ERR("No pointer is provided to get alarm");
		return -EINVAL;
	}

	k_sem_take(&data->sem, K_FOREVER);

	memset(timeptr, 0, sizeof(struct rtc_time));

	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_SECOND) {
		/* Alarm seconds (0-59) */
		timeptr->tm_sec = bcd2bin(rtc_ec2i_ite_read(dev, RTC_SECAREG(id), id));
		return_mask |= RTC_ALARM_TIME_MASK_SECOND;
	}
	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_MINUTE) {
		/* Alarm minutes (0-59) */
		timeptr->tm_min = bcd2bin(rtc_ec2i_ite_read(dev, RTC_MINAREG(id), id));
		return_mask |= RTC_ALARM_TIME_MASK_MINUTE;
	}
	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_HOUR) {
		/* Alarm hours (0-23, 12 or 24-hour mode) */
		timeptr->tm_hour = bcd2bin(rtc_ec2i_ite_read(dev, RTC_HRAREG(id), id));
		return_mask |= RTC_ALARM_TIME_MASK_HOUR;
	}
	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		/* Alarm day of month (1-31) */
		timeptr->tm_mday = bcd2bin(rtc_ec2i_ite_read(dev, RTC_DOMAREG(id), id));
		return_mask |= RTC_ALARM_TIME_MASK_MONTHDAY;
	}
	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_MONTH) {
		/* Alarm month: tm_mon (0-11), RTC stores (1-12) */
		timeptr->tm_mon = bcd2bin(rtc_ec2i_ite_read(dev, RTC_MONAREG(id), id)) - 1;
		return_mask |= RTC_ALARM_TIME_MASK_MONTH;
	}
	if (curr_alarm_mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		/* Alarm day of week: tm_wday (0-6, Sunday=0), RTC stores (1-7) */
		timeptr->tm_wday = bcd2bin(rtc_ec2i_ite_read(dev, RTC_DOWAREG(id), id)) - 1;
		return_mask |= RTC_ALARM_TIME_MASK_WEEKDAY;
	}

	*mask = return_mask;

	k_sem_give(&data->sem);

	LOG_DBG("RTC get alarm%d time (UTC): %02d-%02d %02d:%02d:%02d (wday=%d)", id + 1,
		timeptr->tm_mon + 1, timeptr->tm_mday, timeptr->tm_hour, timeptr->tm_min,
		timeptr->tm_sec, timeptr->tm_wday);

	return 0;
}

static int rtc_it8xxx2_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_ite_data *data = dev->data;
	int ret = 0;

	if (rtc_it8xxx2_validate_alarm_id(dev, id, __func__)) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	if (data->alarm_callback[id].is_alarm_pending == true) {
		ret = 1;

		/* Clear the pending status of the alarm. */
		data->alarm_callback[id].is_alarm_pending = false;
	}

	irq_unlock(key);

	return ret;
}

static int rtc_it8xxx2_alarm_set_callback(const struct device *dev, uint16_t id,
					  rtc_alarm_callback callback, void *user_data)
{
	struct rtc_ite_data *data = dev->data;

	if (rtc_it8xxx2_validate_alarm_id(dev, id, __func__)) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	/* Set the callback function for the alarm and user data. */
	data->alarm_callback[id].alarm_cb = callback;
	data->alarm_callback[id].alarm_user_data = user_data;

	irq_unlock(key);

	return 0;
}

static void rtc_alarm_handle(const struct device *dev, int id)
{
	struct rtc_ite_data *data = dev->data;

	if (data->alarm_callback[id].alarm_cb) {
		data->alarm_callback[id].alarm_cb(dev, id,
						  data->alarm_callback[id].alarm_user_data);
		data->alarm_callback[id].is_alarm_pending = false;
	} else {
		data->alarm_callback[id].is_alarm_pending = true;
	}
}

static void rtc_it8xxx2_isr(const void *arg)
{
	const struct device *dev = arg;
	const struct rtc_ite_config *config = dev->config;
	uint8_t ctlregc = rtc_ec2i_ite_read(dev, RTC_CTLREGC, B0);

	for (int i = 0; i < config->alarms_count; i++) {
		if (ctlregc & RTC_CTLREGC_AIF(i)) {
			LOG_DBG("Alarm%d %s", i + 1, __func__);
			rtc_alarm_handle(dev, i);
		}
	}
}
#endif /* CONFIG_RTC_ALARM */

static DEVICE_API(rtc, rtc_it8xxx2_driver_api) = {
	.set_time = rtc_it8xxx2_set_time,
	.get_time = rtc_it8xxx2_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_it8xxx2_alarm_get_supported_fields,
	.alarm_set_time = rtc_it8xxx2_alarm_set_time,
	.alarm_get_time = rtc_it8xxx2_alarm_get_time,
	.alarm_is_pending = rtc_it8xxx2_alarm_is_pending,
	.alarm_set_callback = rtc_it8xxx2_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
};

static int rtc_it8xxx2_init(const struct device *dev)
{
	const struct rtc_ite_config *config = dev->config;
	struct rtc_ite_data *data = dev->data;
	uint8_t gpiogcr_clksrc, ctlregb;

	/*
	 * The VCC power status is treated as power-on.
	 * The reset source of PNPCFG is RSTPNP bit in RSTCH register and WRST#.
	 */
	rtc_it8xxx2_update_gctrl(config->reg_gctrl + RTC_GCTRL_RSTS, GENMASK(7, 6) | RTC_RSTS_HGRST,
				 RTC_RSTS_VCCDO(RTC_VCC_ON));

	/* Select RTC clock source */
	gpiogcr_clksrc = sys_read8(config->reg_gpiogcr + RTC_GPIOGCR_CLKSRC);
#ifdef CONFIG_SOC_IT8XXX2_EXT_32K
	/* Enable external crystal oscillator for RTC */
	gpiogcr_clksrc &= ~(RTC_CLKSRC_OSCILLATOR | RTC_CLKSRC_SEL);
#else
	/* Enable the ring oscillator for RTC */
	gpiogcr_clksrc &= ~RTC_CLKSRC_CRYSTAL;
#endif
	sys_write8(gpiogcr_clksrc, config->reg_gpiogcr + RTC_GPIOGCR_CLKSRC);

	/* Divider chain control: [6:4]=010b: normal operation */
	rtc_ec2i_ite_write(dev, RTC_CTLREGA, RTC_CTLREGA_DICCTL(RTC_NORMAL_OP), B0);

	/* Update cycles will not occur until set mode (SM) bit is 0 */
	ctlregb = rtc_ec2i_ite_read(dev, RTC_CTLREGB, B0);
	rtc_ec2i_ite_write(dev, RTC_CTLREGB, RTC_CTLREGB_SM | ctlregb, B0);
	/* Timing updates occur normally */
	rtc_ec2i_ite_write(dev, RTC_CTLREGB, ctlregb, B0);

#ifdef CONFIG_RTC_ALARM
	/* Configure RTC timer interrupt for this instance */
	config->irq_config_func(dev);
#endif /* CONFIG_RTC_ALARM */

	/* Initialize mutex for RTC */
	k_sem_init(&data->sem, 1, 1);

	return 0;
}

#ifdef CONFIG_RTC_ALARM
#define RTC_ITE_IRQ_FUN_ELEM(idx, inst)                                                            \
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(inst, idx), 0, rtc_it8xxx2_isr, DEVICE_DT_INST_GET(inst),  \
		    DT_INST_IRQ_BY_IDX(inst, idx, flags));

#define RTC_ITE_IRQN_ELEM(idx, inst)                                                               \
	{                                                                                          \
		.alarm_irq = DT_INST_IRQN_BY_IDX(inst, idx),                                       \
		.alarm_flag = DT_INST_IRQ_BY_IDX(inst, idx, flags),                                \
	}
#endif /* CONFIG_RTC_ALARM */

/* clang-format off */
#define RTC_ITE_INIT(inst)                                                                         \
	IF_ENABLED(CONFIG_RTC_ALARM, (                                                             \
		BUILD_ASSERT(DT_INST_PROP(inst, alarms_count) <= 2,                                \
			     "RTC driver only supports 2 alarms. Check alarms-count in DTS");      \
		static void rtc_ite_irq_config_func_##inst(const struct device *dev);              \
	))                                                                                         \
                                                                                                   \
	static const struct rtc_ite_config rtc_ite_cfg_##inst = {                                  \
		.reg_ec2i = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                      \
		.reg_gpiogcr = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                   \
		.reg_gctrl = DT_INST_REG_ADDR_BY_IDX(inst, 2),                                     \
		IF_ENABLED(CONFIG_RTC_ALARM, (                                                     \
			.irq_config_func = rtc_ite_irq_config_func_##inst,                         \
			.alarm_cfgs = {                                                            \
				LISTIFY(DT_INST_PROP(inst, alarms_count), RTC_ITE_IRQN_ELEM, (,),  \
					inst)                                                      \
			},                                                                         \
			.alarms_count = DT_INST_PROP(inst, alarms_count),                          \
		))                                                                                 \
	};                                                                                         \
                                                                                                   \
	static struct rtc_ite_data rtc_ite_data_##inst;                                            \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, rtc_it8xxx2_init, NULL, &rtc_ite_data_##inst,                  \
			      &rtc_ite_cfg_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,          \
			      &rtc_it8xxx2_driver_api);                                            \
                                                                                                   \
	IF_ENABLED(CONFIG_RTC_ALARM, (                                                             \
		static void rtc_ite_irq_config_func_##inst(const struct device *dev)               \
		{                                                                                  \
			LISTIFY(DT_INST_PROP(inst, alarms_count), RTC_ITE_IRQ_FUN_ELEM, (;), inst) \
		}                                                                                  \
	))
/* clang-format on */
DT_INST_FOREACH_STATUS_OKAY(RTC_ITE_INIT)
