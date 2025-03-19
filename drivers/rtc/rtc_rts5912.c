/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2025 Realtek Semiconductor Corporation, SIBG-SD7
 * Author: Chia-Yang Lin <cylin0708@realtek.com>
 */

#define DT_DRV_COMPAT realtek_rts5912_rtc

#include <soc.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_rts5912.h>
#include "rtc_utils.h"

#include "reg/reg_system.h"
#include "reg/reg_rtc.h"

LOG_MODULE_REGISTER(rtc_rts5912, CONFIG_RTC_LOG_LEVEL);

#define RTS5912_RTC_TIME_MASK                                                                      \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_MONTH |  \
	 RTC_ALARM_TIME_MASK_YEAR)

#define RTS5912_RTC_DIVCTL_NORMAL_OPERATION BIT(1)

#define RTS5912_RTC_DAYWEEK_OFFSET 1
#define RTS5912_RTC_MONTH_OFFSET   1
#define RTS5912_RTC_YEAR_OFFSET    100

struct rtc_rts5912_config {
	RTC_Type *regs;
	uint32_t rtc_base;
	uint32_t rtc_clk_grp;
	uint32_t rtc_clk_idx;
	const struct device *clk_dev;
};

struct rtc_rts5912_data {
	struct k_spinlock lock;
};

static void rtc_rts5912_reset_rtc_time(const struct device *dev)
{
	const struct rtc_rts5912_config *const dev_cfg = dev->config;
	RTC_Type *rtc_regs = dev_cfg->regs;

	rtc_regs->CTRL1 |= RTC_CTRL1_SETMODE_Msk;
	rtc_regs->CTRL0 &= ~RTC_CTRL0_DIVCTL_Msk;
	rtc_regs->CTRL0 |= (RTS5912_RTC_DIVCTL_NORMAL_OPERATION << RTC_CTRL0_DIVCTL_Pos);
	rtc_regs->CTRL1 |= RTC_CTRL1_DATEMODE_Msk;
	rtc_regs->CTRL1 |= RTC_CTRL1_HRMODE_Msk;
	rtc_regs->SEC = 0;
	rtc_regs->MIN = 0;
	rtc_regs->HR &= ~(RTC_HR_AMPM_Msk | RTC_HR_VAL_Msk);
	rtc_regs->DAYWEEK = BIT(0);
	rtc_regs->DAYMONTH = BIT(0);
	rtc_regs->MONTH = BIT(0);
	rtc_regs->YEAR = 0;
	rtc_regs->WEEK &= ~RTC_WEEK_NUM_Msk;
}

static int rtc_rts5912_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_rts5912_config *const dev_cfg = dev->config;
	RTC_Type *rtc_regs = dev_cfg->regs;

	if (!rtc_utils_validate_rtc_time(timeptr, RTS5912_RTC_TIME_MASK)) {
		rtc_rts5912_reset_rtc_time(dev);
		k_msleep(1);
		return -EINVAL;
	}

	rtc_regs->CTRL1 |= RTC_CTRL1_SETMODE_Msk;
	rtc_regs->SEC = timeptr->tm_sec;
	rtc_regs->MIN = timeptr->tm_min;
	rtc_regs->HR = timeptr->tm_hour;
	rtc_regs->DAYWEEK = timeptr->tm_wday + RTS5912_RTC_DAYWEEK_OFFSET;
	rtc_regs->DAYMONTH = timeptr->tm_mday;
	rtc_regs->MONTH = timeptr->tm_mon + RTS5912_RTC_MONTH_OFFSET;
	rtc_regs->YEAR = timeptr->tm_year % RTS5912_RTC_YEAR_OFFSET;
	/* Need to delay in order to update register after setting RTC time */
	k_msleep(1);
	rtc_regs->CTRL1 &= ~RTC_CTRL1_SETMODE_Msk;

	return 0;
}

static int rtc_rts5912_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_rts5912_config *const dev_cfg = dev->config;
	RTC_Type *rtc_regs = dev_cfg->regs;

	timeptr->tm_sec = rtc_regs->SEC;
	timeptr->tm_min = rtc_regs->MIN;
	timeptr->tm_hour = rtc_regs->HR;
	timeptr->tm_wday = rtc_regs->DAYWEEK - RTS5912_RTC_DAYWEEK_OFFSET;
	timeptr->tm_mday = rtc_regs->DAYMONTH;
	timeptr->tm_mon = rtc_regs->MONTH - RTS5912_RTC_MONTH_OFFSET;
	timeptr->tm_year = rtc_regs->YEAR + RTS5912_RTC_YEAR_OFFSET;

	/* No support for daylight saving time flag and nanoseconds in rts5912 RTC */
	timeptr->tm_isdst = -1;
	timeptr->tm_nsec = 0;

	return 0;
}

static DEVICE_API(rtc, rtc_rts5912_driver_api) = {
	.set_time = rtc_rts5912_set_time,
	.get_time = rtc_rts5912_get_time,
};

static int rtc_rts5912_init(const struct device *dev)
{
	const struct rtc_rts5912_config *const rtc_config = dev->config;
	struct rts5912_sccon_subsys sccon;

	int rc;

	if (!device_is_ready(rtc_config->clk_dev)) {
		LOG_ERR("RTC device not ready");
		return -ENODEV;
	}

	sccon.clk_grp = rtc_config->rtc_clk_grp;
	sccon.clk_idx = rtc_config->rtc_clk_idx;
	rc = clock_control_on(rtc_config->clk_dev, (clock_control_subsys_t)&sccon);
	if (rc < 0) {
		LOG_ERR("Failed to turn on RTC clock (%d)", rc);
		return rc;
	}

	rtc_rts5912_reset_rtc_time(dev);

	return rc;
}

#define RTC_RTS5912_CONFIG(inst)                                                                   \
	static struct rtc_rts5912_config rtc_rts5912_config_##inst = {                             \
		.regs = (RTC_Type *)(DT_INST_REG_ADDR(inst)),                                      \
		.rtc_base = DT_INST_REG_ADDR(inst),                                                \
		.rtc_clk_grp = DT_INST_CLOCKS_CELL_BY_NAME(inst, rtc, clk_grp),                    \
		.rtc_clk_idx = DT_INST_CLOCKS_CELL_BY_NAME(inst, rtc, clk_idx),                    \
		.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                               \
	};

#define RTC_RTS5912_DEVICE_INIT(index)                                                             \
	RTC_RTS5912_CONFIG(index)                                                                  \
	DEVICE_DT_INST_DEFINE(index, &rtc_rts5912_init, NULL, NULL, &rtc_rts5912_config_##index,   \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, &rtc_rts5912_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_RTS5912_DEVICE_INIT)
