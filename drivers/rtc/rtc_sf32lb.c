/*
 * SPDX-FileCopyrightText: 2025 SiFli Technologies(Nanjing) Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sifli_sf32lb_rtc

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <soc.h>

#include "bf0_hal.h"

LOG_MODULE_REGISTER(rtc_sf32lb, CONFIG_RTC_LOG_LEVEL);

struct rtc_sf32lb_data {
	RTC_HandleTypeDef hrtc;
};

struct rtc_sf32lb_config {
	RTC_TypeDef *reg;
	void (*irq_config_func)(const struct device *dev);
};

static int rtc_sf32lb_enter_init_mode(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;

	config->reg->ISR |= RTC_ISR_INIT;
	while ((config->reg->ISR & RTC_ISR_INITF) == (uint32_t)0U);

	return 0;
}

static int rtc_sf32lb_exit_init_mode(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;

	config->reg->ISR &= (uint32_t)~RTC_ISR_INIT;

	return 0;
}

static int rtc_sf32lb_set_time(const struct device *dev, const struct rtc_time *time)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t tr = 0;
	uint32_t dr = 0;

	if (!time) {
		return -EINVAL;
	}

	tr = (uint32_t)(((uint32_t)bin2bcd(time->tm_hour) << RTC_TR_HU_Pos) | \
		             ((uint32_t)bin2bcd(time->tm_min) << RTC_TR_MNU_Pos) | \
			     ((uint32_t)bin2bcd(time->tm_sec) << RTC_TR_SU_Pos) | \
			     (((uint32_t)RTC_HOURFORMAT_24) << RTC_TR_PM_Pos));

	rtc_sf32lb_enter_init_mode(dev);
	config->reg->TR = tr;
	rtc_sf32lb_exit_init_mode(dev);

	if (time->tm_year < 100) {
		dr |= RTC_DR_CB;
		dr |= (((uint32_t)bin2bcd(time->tm_year) << RTC_DR_YU_Pos) | \
			((uint32_t)bin2bcd(time->tm_mon + 1) << RTC_DR_MU_Pos) | \
			((uint32_t)bin2bcd(time->tm_mday)) | \
			((uint32_t)time->tm_wday << RTC_DR_WD_Pos));
	} else {
		dr |= (((uint32_t)bin2bcd(time->tm_year - 100) << RTC_DR_YU_Pos) | \
			((uint32_t)bin2bcd(time->tm_mon + 1) << RTC_DR_MU_Pos) | \
			((uint32_t)bin2bcd(time->tm_mday)) | \
			((uint32_t)time->tm_wday << RTC_DR_WD_Pos));
	}

	rtc_sf32lb_enter_init_mode(dev);
	config->reg->DR = dr;
	rtc_sf32lb_exit_init_mode(dev);

	return 0;
}

static int rtc_sf32lb_get_time(const struct device *dev, struct rtc_time *time)
{
	const struct rtc_sf32lb_config *config = dev->config;
	uint32_t reg;

	if (!time) {
		return -EINVAL;
	}

	reg = config->reg->TR;

	time->tm_hour = bcd2bin((uint8_t)((reg & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos));
	time->tm_min = bcd2bin((uint8_t)((reg & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos));
	time->tm_sec = bcd2bin((uint8_t)((reg & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_SU_Pos));

	reg = config->reg->DR;

	time->tm_year = bcd2bin((uint8_t)((reg & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos)) + 100;
	time->tm_mon = bcd2bin((uint8_t)((reg & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos)) - 1;
	time->tm_mday = bcd2bin((uint8_t)(reg & (RTC_DR_DT | RTC_DR_DU)));
	time->tm_wday = bcd2bin((uint8_t)((reg & (RTC_DR_WD)) >> RTC_DR_WD_Pos));

	return 0;
}

static int rtc_sf32lb_init(const struct device *dev)
{
	const struct rtc_sf32lb_config *config = dev->config;
	struct rtc_sf32lb_data *data = dev->data;

	HAL_StatusTypeDef status;

	// config->reg->CR |= RTC_CR_LPCKSEL;


	/* Initialize RTC */
	data->hrtc.Instance = (RTC_TypeDef *)config->reg;
	data->hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
	data->hrtc.Init.DivAInt = 0;
	data->hrtc.Init.DivAFrac = 0;
	data->hrtc.Init.DivB = RC10K_SUB_SEC_DIVB;

	status = HAL_RTC_Init(&data->hrtc, 0);
	if (status != HAL_OK) {
		LOG_ERR("Failed to initialize RTC");
		return -EIO;
	}

	/* Configure RTC interrupts if needed */
	if (config->irq_config_func) {
		config->irq_config_func(dev);
	}

	return 0;
}

static const struct rtc_driver_api rtc_sf32lb_driver_api = {
	.set_time = rtc_sf32lb_set_time,
	.get_time = rtc_sf32lb_get_time,
};

#define RTC_SF32LB_INIT(n) \
	static void rtc_sf32lb_irq_config_func_##n(const struct device *dev); \
	static struct rtc_sf32lb_data rtc_sf32lb_data_##n; \
	static const struct rtc_sf32lb_config rtc_sf32lb_config_##n = { \
		.reg = (RTC_TypeDef *)DT_INST_REG_ADDR(n), \
		.irq_config_func = rtc_sf32lb_irq_config_func_##n, \
	}; \
	DEVICE_DT_INST_DEFINE(n, \
		rtc_sf32lb_init, \
		NULL, \
		&rtc_sf32lb_data_##n, \
		&rtc_sf32lb_config_##n, \
		POST_KERNEL, \
		CONFIG_RTC_INIT_PRIORITY, \
		&rtc_sf32lb_driver_api); \
	static void rtc_sf32lb_irq_config_func_##n(const struct device *dev) \
	{ \
		IRQ_CONNECT(DT_INST_IRQN(n), \
			DT_INST_IRQ(n, priority), \
			HAL_RTC_IRQHandler, \
			DEVICE_DT_INST_GET(n), \
			0); \
		irq_enable(DT_INST_IRQN(n)); \
	}

DT_INST_FOREACH_STATUS_OKAY(RTC_SF32LB_INIT)
