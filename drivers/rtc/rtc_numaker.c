/*
 * Copyright (c) 2024 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nuvoton_numaker_rtc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_numaker.h>
#include "rtc_utils.h"

LOG_MODULE_REGISTER(rtc_numaker, CONFIG_RTC_LOG_LEVEL);

/* RTC support 2000 ~ 2099 */
#define NVT_RTC_YEAR_MIN 2000U
#define NVT_RTC_YEAR_MAX 2099U
/* struct tm start time:   1st, Jan, 1900 */
#define TM_YEAR_REF 1900U

#define NVT_TIME_SCALE RTC_CLOCK_24
#define NVT_ALARM_MSK 0x3fU
#define NVT_ALARM_UNIT_MSK 0x03U

struct rtc_numaker_config {
	RTC_T *rtc_base;
	uint32_t clk_modidx;
	const struct device *clk_dev;
	uint32_t oscillator;
};

struct rtc_numaker_data {
	struct k_spinlock lock;
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
	bool alarm_pending;
#endif /* CONFIG_RTC_ALARM */
};

struct rtc_numaker_time {
	uint32_t year;		/* Year value */
	uint32_t month;		/* Month value */
	uint32_t day;		/* Day value */
	uint32_t day_of_week;	/* Day of week value */
	uint32_t hour;		/* Hour value */
	uint32_t minute;	/* Minute value */
	uint32_t second;	/* Second value */
	uint32_t time_scale;	/* 12-Hour, 24-Hour */
	uint32_t am_pm;		/* Only Time Scale select 12-hr used */
};

static int rtc_numaker_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	struct rtc_numaker_time curr_time;
	struct rtc_numaker_data *data = dev->data;
	uint32_t real_year = timeptr->tm_year + TM_YEAR_REF;
#ifdef CONFIG_RTC_ALARM
	const struct rtc_numaker_config *config = dev->config;
	RTC_T *rtc_base = config->rtc_base;
#endif

	if (real_year < NVT_RTC_YEAR_MIN || real_year > NVT_RTC_YEAR_MAX) {
		/* RTC can't support years out of 2000 ~ 2099 */
		return -EINVAL;
	}

	if (timeptr->tm_wday == -1) {
		return -EINVAL;
	}

	curr_time.year = real_year;
	curr_time.month = timeptr->tm_mon + 1;
	curr_time.day = timeptr->tm_mday;
	curr_time.hour = timeptr->tm_hour;
	curr_time.minute = timeptr->tm_min;
	curr_time.second = timeptr->tm_sec;
	curr_time.day_of_week = timeptr->tm_wday;
	curr_time.time_scale = NVT_TIME_SCALE;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	RTC_SetDateAndTime((S_RTC_TIME_DATA_T *)&curr_time);

#ifdef CONFIG_RTC_ALARM
	/* Restore RTC alarm mask */
	rtc_base->CAMSK = rtc_base->SPR[1];
	rtc_base->TAMSK = rtc_base->SPR[2];
#endif
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int rtc_numaker_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	struct rtc_numaker_data *data = dev->data;
	struct rtc_numaker_time curr_time;

	curr_time.time_scale = NVT_TIME_SCALE;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	RTC_GetDateAndTime((S_RTC_TIME_DATA_T *)&curr_time);
	k_spin_unlock(&data->lock, key);

	timeptr->tm_year = curr_time.year - TM_YEAR_REF;
	timeptr->tm_mon = curr_time.month - 1;
	timeptr->tm_mday = curr_time.day;
	timeptr->tm_wday = curr_time.day_of_week;

	timeptr->tm_hour = curr_time.hour;
	timeptr->tm_min = curr_time.minute;
	timeptr->tm_sec = curr_time.second;
	timeptr->tm_nsec = 0;

	/* unknown values */
	timeptr->tm_yday = -1;
	timeptr->tm_isdst = -1;

	return 0;
}

static void rtc_numaker_isr(const struct device *dev)
{
	const struct rtc_numaker_config *config = dev->config;
	RTC_T *rtc_base = config->rtc_base;
	uint32_t int_status;
#ifdef CONFIG_RTC_ALARM
	struct rtc_numaker_data *data = dev->data;
#endif

	int_status = rtc_base->INTSTS;
	if (int_status & RTC_INTSTS_TICKIF_Msk) {
		/* Clear RTC Tick interrupt flag */
		rtc_base->INTSTS = RTC_INTSTS_TICKIF_Msk;
	}

#ifdef CONFIG_RTC_ALARM
	if (int_status & RTC_INTSTS_ALMIF_Msk) {
		rtc_alarm_callback callback;
		void *user_data;

		/* Clear RTC Alarm interrupt flag */
		rtc_base->INTSTS = RTC_INTSTS_ALMIF_Msk;
		rtc_base->CAMSK = 0x00;
		rtc_base->TAMSK = 0x00;
		callback = data->alarm_callback;
		user_data = data->alarm_user_data;
		data->alarm_pending = callback ? false : true;

		if (callback != NULL) {
			callback(dev, 0, user_data);
		}
	}
#endif /* CONFIG_RTC_ALARM */
}

#ifdef CONFIG_RTC_ALARM
static int rtc_numaker_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						  uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	*mask =  RTC_ALARM_TIME_MASK_SECOND
		 | RTC_ALARM_TIME_MASK_MINUTE
		 | RTC_ALARM_TIME_MASK_HOUR
		 | RTC_ALARM_TIME_MASK_MONTHDAY
		 | RTC_ALARM_TIME_MASK_MONTH
		 | RTC_ALARM_TIME_MASK_YEAR;

	return 0;
}

static int rtc_numaker_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				      const struct rtc_time *timeptr)
{
	struct rtc_numaker_data *data = dev->data;
	const struct rtc_numaker_config *config = dev->config;
	RTC_T *rtc_base = config->rtc_base;
	uint16_t mask_capable;
	struct rtc_numaker_time alarm_time;

	rtc_numaker_alarm_get_supported_fields(dev, 0, &mask_capable);

	if ((id != 0)) {
		return -EINVAL;
	}

	if ((mask != 0) && (timeptr == NULL)) {
		return -EINVAL;
	}

	if (mask & ~mask_capable) {
		return -EINVAL;
	}

	if (rtc_utils_validate_rtc_time(timeptr, mask) == false) {
		return -EINVAL;
	}

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	irq_disable(DT_INST_IRQN(0));
	if ((mask == 0) || (timeptr == NULL)) {
		/* Disable the alarm */
		rtc_base->SPR[0] = mask;
		rtc_base->SPR[1] = 0x00;
		rtc_base->SPR[2] = 0x00;
		irq_enable(DT_INST_IRQN(0));
		k_spin_unlock(&data->lock, key);
		rtc_base->CAMSK = rtc_base->SPR[1];
		rtc_base->TAMSK = rtc_base->SPR[2];
		/* Disable RTC Alarm Interrupt */
		RTC_DisableInt(RTC_INTEN_ALMIEN_Msk);
		return 0;
	}

	alarm_time.time_scale = NVT_TIME_SCALE;
	RTC_GetDateAndTime((S_RTC_TIME_DATA_T *)&alarm_time);

	/* Reset RTC alarm mask of camsk & tamsk */
	uint32_t camsk = NVT_ALARM_MSK;
	uint32_t tamsk = NVT_ALARM_MSK;

	/* Set H/W care to match bits */
	if (mask & RTC_ALARM_TIME_MASK_YEAR) {
		alarm_time.year = timeptr->tm_year + TM_YEAR_REF;
		camsk &= ~(NVT_ALARM_UNIT_MSK << RTC_CAMSK_MYEAR_Pos);
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTH) {
		alarm_time.month = timeptr->tm_mon + 1;
		camsk &= ~(NVT_ALARM_UNIT_MSK << RTC_CAMSK_MMON_Pos);
	}
	if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		alarm_time.day = timeptr->tm_mday;
		camsk &= ~(NVT_ALARM_UNIT_MSK << RTC_CAMSK_MDAY_Pos);
	}
	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		alarm_time.hour = timeptr->tm_hour;
		tamsk &= ~(NVT_ALARM_UNIT_MSK << RTC_TAMSK_MHR_Pos);
	}
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		alarm_time.minute = timeptr->tm_min;
		tamsk &= ~(NVT_ALARM_UNIT_MSK << RTC_TAMSK_MMIN_Pos);
	}
	if (mask & RTC_ALARM_TIME_MASK_SECOND) {
		alarm_time.second = timeptr->tm_sec;
		tamsk &= ~(NVT_ALARM_UNIT_MSK << RTC_TAMSK_MSEC_Pos);
	}

	/* Disable RTC Alarm Interrupt */
	RTC_DisableInt(RTC_INTEN_ALMIEN_Msk);

	/* Set the alarm time */
	RTC_SetAlarmDateAndTime((S_RTC_TIME_DATA_T *)&alarm_time);

	/* Clear RTC alarm interrupt flag */
	RTC_CLEAR_ALARM_INT_FLAG();

	rtc_base->SPR[0] = mask;
	rtc_base->SPR[1] = camsk;
	rtc_base->SPR[2] = tamsk;

	rtc_base->CAMSK = rtc_base->SPR[1];
	rtc_base->TAMSK = rtc_base->SPR[2];

	k_spin_unlock(&data->lock, key);
	irq_enable(DT_INST_IRQN(0));

	/* Enable RTC Alarm Interrupt */
	RTC_EnableInt(RTC_INTEN_ALMIEN_Msk);

	return 0;
}

static int rtc_numaker_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				      struct rtc_time *timeptr)
{
	struct rtc_numaker_data *data = dev->data;
	const struct rtc_numaker_config *config = dev->config;
	RTC_T *rtc_base = config->rtc_base;
	struct rtc_numaker_time alarm_time;

	if ((id != 0) || (mask == NULL) || (timeptr == NULL)) {
		return -EINVAL;
	}

	alarm_time.time_scale = NVT_TIME_SCALE;

	K_SPINLOCK(&data->lock) {
		RTC_GetAlarmDateAndTime((S_RTC_TIME_DATA_T *)&alarm_time);
	}

	*mask = rtc_base->SPR[0];
	if (*mask & RTC_ALARM_TIME_MASK_YEAR) {
		timeptr->tm_year = alarm_time.year - TM_YEAR_REF;
	}
	if (*mask & RTC_ALARM_TIME_MASK_MONTH) {
		timeptr->tm_mon = alarm_time.month - 1;
	}
	if (*mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		timeptr->tm_mday = alarm_time.day;
	}
	if (*mask & RTC_ALARM_TIME_MASK_HOUR) {
		timeptr->tm_hour = alarm_time.hour;
	}
	if (*mask & RTC_ALARM_TIME_MASK_MINUTE) {
		timeptr->tm_min = alarm_time.minute;
	}
	if (*mask & RTC_ALARM_TIME_MASK_SECOND) {
		timeptr->tm_sec = alarm_time.second;
	}

	return 0;
}

static int rtc_numaker_alarm_is_pending(const struct device *dev, uint16_t id)
{
	struct rtc_numaker_data *data = dev->data;
	int ret;

	if (id != 0) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		ret = data->alarm_pending ? 1 : 0;
		data->alarm_pending = false;
	}

	return ret;
}

static int rtc_numaker_alarm_set_callback(const struct device *dev, uint16_t id,
					  rtc_alarm_callback callback, void *user_data)
{
	struct rtc_numaker_data *data = dev->data;

	if (id != 0) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock) {
		irq_disable(DT_INST_IRQN(0));
		data->alarm_callback = callback;
		data->alarm_user_data = user_data;
		if ((callback == NULL) && (user_data == NULL)) {
			/* Disable RTC Alarm Interrupt */
			RTC_DisableInt(RTC_INTEN_ALMIEN_Msk);
		}
		irq_enable(DT_INST_IRQN(0));
	}

	return 0;
}
#endif /* CONFIG_RTC_ALARM */

static const struct rtc_driver_api rtc_numaker_driver_api = {
	.set_time = rtc_numaker_set_time,
	.get_time = rtc_numaker_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rtc_numaker_alarm_get_supported_fields,
	.alarm_set_time = rtc_numaker_alarm_set_time,
	.alarm_get_time = rtc_numaker_alarm_get_time,
	.alarm_is_pending = rtc_numaker_alarm_is_pending,
	.alarm_set_callback = rtc_numaker_alarm_set_callback,
#endif /* CONFIG_RTC_ALARM */
};

static int rtc_numaker_init(const struct device *dev)
{
	const struct rtc_numaker_config *cfg = dev->config;
	struct numaker_scc_subsys scc_subsys;
	RTC_T *rtc_base = cfg->rtc_base;
	int err;

	/* CLK controller */
	memset(&scc_subsys, 0x00, sizeof(scc_subsys));
	scc_subsys.subsys_id = NUMAKER_SCC_SUBSYS_ID_PCC;
	scc_subsys.pcc.clk_modidx = cfg->clk_modidx;

	SYS_UnlockReg();

	/* CLK_EnableModuleClock */
	err = clock_control_on(cfg->clk_dev, (clock_control_subsys_t)&scc_subsys);
	if (err != 0) {
		goto done;
	}

	RTC_SetClockSource(cfg->oscillator);
	/* Enable spare registers */
	rtc_base->SPRCTL = RTC_SPRCTL_SPRRWEN_Msk;

	irq_disable(DT_INST_IRQN(0));

	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), rtc_numaker_isr,
		    DEVICE_DT_INST_GET(0), 0);

	irq_enable(DT_INST_IRQN(0));
	err = RTC_Open(0);

done:
	SYS_LockReg();
	return err;
}

static struct rtc_numaker_data rtc_data;

/* Set config based on DTS */
static const struct rtc_numaker_config rtc_config = {
	.rtc_base = (RTC_T *)DT_INST_REG_ADDR(0),
	.clk_modidx = DT_INST_CLOCKS_CELL(0, clock_module_index),
	.clk_dev = DEVICE_DT_GET(DT_PARENT(DT_INST_CLOCKS_CTLR(0))),
	.oscillator = DT_ENUM_IDX(DT_NODELABEL(rtc), oscillator),
};

DEVICE_DT_INST_DEFINE(0, &rtc_numaker_init, NULL, &rtc_data, &rtc_config, PRE_KERNEL_1,
		      CONFIG_RTC_INIT_PRIORITY, &rtc_numaker_driver_api);
