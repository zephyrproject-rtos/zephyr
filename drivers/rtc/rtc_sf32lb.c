/*
 * Copyright (c) 2025 Qingsong Gou <gouqs@hotmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_rtc

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>

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

struct rtc_sf32lb_config {
	uintptr_t base;
	uintptr_t cfg;
};

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
	     FIELD_PREP(RTC_TR_ST_Msk | RTC_TR_SU_Msk, bin2bcd(timeptr->tm_sec));

	rtc_sf32lb_enter_init_mode(dev);
	sys_write32(tr, config->base + RTC_TIMER);
	rtc_sf32lb_exit_init_mode(dev);

	if (!sys_test_bit(config->base + RTC_CR, RTC_CR_BYPSHAD_Pos)) {
		rtc_sf32lb_wait_for_sync(dev);
	}

	if (timeptr->tm_year < 100) { /* 20th century */
		dr |= RTC_DR_CB;
	}
	dr |= FIELD_PREP(RTC_DR_YT_Msk | RTC_DR_YU_Msk, bin2bcd(timeptr->tm_year)) |
	      FIELD_PREP(RTC_DR_MT_Msk | RTC_DR_MU_Msk, bin2bcd(timeptr->tm_mon)) |
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

	reg = sys_read32(config->cfg + SYS_CFG_RTC_DR);

	timeptr->tm_year = bcd2bin(FIELD_GET(RTC_DR_YT_Msk | RTC_DR_YU_Msk, reg));
	timeptr->tm_mon = bcd2bin(FIELD_GET(RTC_DR_MT_Msk | RTC_DR_MU_Msk, reg));
	timeptr->tm_mday = bcd2bin(FIELD_GET(RTC_DR_DT_Msk | RTC_DR_DU_Msk, reg));
	timeptr->tm_wday = FIELD_GET(RTC_DR_WD_Msk, reg);

	return 0;
}

static DEVICE_API(rtc, rtc_sf32lb_driver_api) = {
	.set_time = rtc_sf32lb_set_time,
	.get_time = rtc_sf32lb_get_time,
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

	return 0;
}

#define RTC_SF32LB_DEFINE(n)                                                                       \
	static const struct rtc_sf32lb_config rtc_sf32lb_config_##n = {                            \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.cfg = DT_REG_ADDR(DT_INST_PHANDLE(n, sifli_cfg)),                                 \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, rtc_sf32lb_init, NULL, NULL, &rtc_sf32lb_config_##n, POST_KERNEL, \
			      CONFIG_RTC_INIT_PRIORITY, &rtc_sf32lb_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RTC_SF32LB_DEFINE)
