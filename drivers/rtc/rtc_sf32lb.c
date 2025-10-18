/*
 * Copyright (c) 2025 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_rtc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include <register.h>

LOG_MODULE_REGISTER(rtc_sf32lb, CONFIG_RTC_LOG_LEVEL);

#ifndef RTC_RSF_MASK
#define RTC_RSF_MASK            0xFFFFFF5FU
#endif

struct rtc_sf32lb_config {
	RTC_TypeDef *reg;
};

static int rtc_sf32lb_enter_init_mode(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;

	config->reg->ISR |= RTC_ISR_INIT;
	while ((config->reg->ISR & RTC_ISR_INITF) == (uint32_t)0U) {
	}

	return 0;
}

static int rtc_sf32lb_exit_init_mode(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;

	config->reg->ISR &= (uint32_t)~RTC_ISR_INIT;

	return 0;
}

static void rtc_sf32lb_wait_for_sync(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;

	config->reg->ISR &= RTC_RSF_MASK;

	while ((config->reg->ISR & RTC_ISR_RSF) == 0U) {  /* Wait the registers to be synchronised */
	}
}

static int rtc_sf32lb_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t tr = 0;
	uint32_t dr = 0;

	__ASSERT_NO_MSG(timeptr != NULL);

	tr = (uint32_t)(((uint32_t)bin2bcd(timeptr->tm_hour) << RTC_TR_HU_Pos) |
			((uint32_t)bin2bcd(timeptr->tm_min) << RTC_TR_MNU_Pos) |
			((uint32_t)bin2bcd(timeptr->tm_sec) << RTC_TR_SU_Pos));

	rtc_sf32lb_enter_init_mode(dev);
	config->reg->TR = tr;
	rtc_sf32lb_exit_init_mode(dev);

	if (timeptr->tm_year < 100) {
		dr |= RTC_DR_CB;
		dr |= (((uint32_t)bin2bcd(timeptr->tm_year) << RTC_DR_YU_Pos) |
		       ((uint32_t)bin2bcd(timeptr->tm_mon + 1) << RTC_DR_MU_Pos) |
		       ((uint32_t)bin2bcd(timeptr->tm_mday)) |
		       ((uint32_t)timeptr->tm_wday << RTC_DR_WD_Pos));
	} else {
		dr |= (((uint32_t)bin2bcd(timeptr->tm_year - 100) << RTC_DR_YU_Pos) |
		       ((uint32_t)bin2bcd(timeptr->tm_mon + 1) << RTC_DR_MU_Pos) |
		       ((uint32_t)bin2bcd(timeptr->tm_mday)) |
		       ((uint32_t)timeptr->tm_wday << RTC_DR_WD_Pos));
	}

	rtc_sf32lb_enter_init_mode(dev);
	config->reg->DR = dr;
	rtc_sf32lb_exit_init_mode(dev);

	if ((config->reg->CR & RTC_CR_BYPSHAD) == 0U) {
		rtc_sf32lb_wait_for_sync(dev);
	}

	return 0;
}

static int rtc_sf32lb_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t reg;

	__ASSERT_NO_MSG(timeptr != NULL);

	reg = config->reg->TR;

	timeptr->tm_hour = bcd2bin((uint8_t)((reg & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos));
	timeptr->tm_min = bcd2bin((uint8_t)((reg & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos));
	timeptr->tm_sec = bcd2bin((uint8_t)((reg & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_SU_Pos));

	reg = config->reg->DR;

	if (reg & RTC_DR_CB) {
		timeptr->tm_year =
			bcd2bin((uint8_t)((reg & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos));
	} else {
		timeptr->tm_year =
			bcd2bin((uint8_t)((reg & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos)) + 100;
	}
	timeptr->tm_mon = bcd2bin((uint8_t)((reg & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos)) - 1;
	timeptr->tm_mday = bcd2bin((uint8_t)(reg & (RTC_DR_DT | RTC_DR_DU)));
	timeptr->tm_wday = bcd2bin((uint8_t)((reg & (RTC_DR_WD)) >> RTC_DR_WD_Pos));

	return 0;
}

static int rtc_sf32lb_init(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t psclr = 0;

	psclr |= (38U << RTC_PSCLR_DIVA_INT_Pos) & RTC_PSCLR_DIVA_INT_Msk;
	psclr |= (4608U << RTC_PSCLR_DIVA_FRAC_Pos) & RTC_PSCLR_DIVA_FRAC_Msk;
	psclr |= (256U << RTC_PSCLR_DIVB_Pos) & RTC_PSCLR_DIVB_Msk;
	config->reg->PSCLR = psclr;

	if ((config->reg->CR & RTC_CR_BYPSHAD) == 0U) {
		rtc_sf32lb_wait_for_sync(dev);
	}

	return 0;
}

static const struct rtc_driver_api rtc_sf32lb_driver_api = {
	.set_time = rtc_sf32lb_set_time,
	.get_time = rtc_sf32lb_get_time,
};

#define RTC_SF32LB_INIT(n)                                                                         \
	static const struct rtc_sf32lb_config rtc_sf32lb_config_##n = {                            \
		.reg = (RTC_TypeDef *)DT_INST_REG_ADDR(n),                                         \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, rtc_sf32lb_init, NULL, NULL, &rtc_sf32lb_config_##n, POST_KERNEL, \
			      CONFIG_RTC_INIT_PRIORITY, &rtc_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_SF32LB_INIT)
