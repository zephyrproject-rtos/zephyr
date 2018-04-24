/*
 * Copyright (c) 2018 blik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/sys_log.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <kernel.h>
#include <rtc.h>
#include <power.h>
#include <soc.h>
#include <misc/util.h>
#include <fsl_rtc.h>

struct mcux_rtc_config {
	RTC_Type *base;
	void (*irq_config_func)(struct device *dev);
};

struct mcux_rtc_data {
	struct k_sem sync;
	rtc_config_t config;

	/* callback information */
	void (*callback)(void *data);
	void *callback_data;
};

static void mcux_rtc_enable(struct device *dev)
{
	const struct mcux_rtc_config *config = dev->config->config_info;

	RTC_StartTimer(config->base);
	RTC_EnableInterrupts(config->base,
		kRTC_AlarmInterruptEnable |
		kRTC_TimeOverflowInterruptEnable |
		kRTC_TimeInvalidInterruptEnable);
}

static void mcux_rtc_disable(struct device *dev)
{
	const struct mcux_rtc_config *config = dev->config->config_info;

	RTC_DisableInterrupts(config->base,
			      kRTC_AlarmInterruptEnable |
			      kRTC_TimeOverflowInterruptEnable |
			      kRTC_TimeInvalidInterruptEnable);
	RTC_StopTimer(config->base);

	/* clear out any set alarms */
	config->base->TAR = 0;
}

static int mcux_rtc_set_alarm(struct device *dev, const u32_t alarm_val)
{
	const struct mcux_rtc_config *config = dev->config->config_info;

	if (alarm_val < config->base->TSR) {
		SYS_LOG_ERR("alarm cannot be earlier than current time\n");
		return -EINVAL;
	}

	config->base->TAR = alarm_val;
	return 0;
}

static int mcux_rtc_set_config(struct device *dev, struct rtc_config *cfg)
{
	const struct mcux_rtc_config *config = dev->config->config_info;
	struct mcux_rtc_data *data = dev->driver_data;
	int ret = 0;

	/* only allow one modifier at a time */
	k_sem_take(&data->sync, K_FOREVER);

	if (cfg->alarm_enable) {
		/* set up callback information */
		data->callback = (void *) cfg->cb_fn;
		data->callback_data = dev;

		RTC_StopTimer(config->base);
		config->base->TSR = cfg->init_val;
		RTC_StartTimer(config->base);
		ret = mcux_rtc_set_alarm(dev, cfg->alarm_val);
	} else {
		/* clear any existing alarm setting */
		config->base->TAR = 0;

		/* clear callbacks */
		data->callback = NULL;
		data->callback_data = NULL;
	}

	k_sem_give(&data->sync);

	return ret;
}

static u32_t mcux_rtc_read(struct device *dev)
{
	const struct mcux_rtc_config *config = dev->config->config_info;
	u32_t val = config->base->TSR;

	/*
	 * Read TSR seconds twice in case it glitches during an update.
	 * This can happen when a read occurs at the time the register is
	 * incrementing.
	 */
	if (config->base->TSR == val) {
		return val;
	}

	val = config->base->TSR;

	return val;
}

static u32_t mcux_rtc_get_pending_int(struct device *dev)
{
	const struct mcux_rtc_config *config = dev->config->config_info;

	return RTC_GetStatusFlags(config->base) & RTC_SR_TAF_MASK;
}

static const struct rtc_driver_api mcux_rtc_driver_api = {
	.enable = mcux_rtc_enable,
	.disable = mcux_rtc_disable,
	.read = mcux_rtc_read,
	.set_config = mcux_rtc_set_config,
	.set_alarm = mcux_rtc_set_alarm,
	.get_pending_int = mcux_rtc_get_pending_int,
};

static void mcux_rtc_isr(void *arg)
{
	struct device *dev = arg;
	const struct mcux_rtc_config *config = dev->config->config_info;
	struct mcux_rtc_data *data = dev->driver_data;

	/* perform any registered callbacks */
	if (data->callback) {
		data->callback(data->callback_data);
	}

	/*
	 * Clear any conditions to ack the IRQ
	 *
	 * callback may have already reset the alarm flag if a new
	 * alarm value was programmed to the TAR
	 */
	RTC_StopTimer(config->base);
	if (RTC_GetStatusFlags(config->base) & RTC_SR_TAF_MASK) {
		RTC_ClearStatusFlags(config->base, kRTC_AlarmFlag);
	} else if (RTC_GetStatusFlags(config->base) & RTC_SR_TIF_MASK) {
		RTC_ClearStatusFlags(config->base, kRTC_TimeInvalidFlag);
	} else if (RTC_GetStatusFlags(config->base) & RTC_SR_TOF_MASK) {
		RTC_ClearStatusFlags(config->base, kRTC_TimeOverflowFlag);
	}
	RTC_StartTimer(config->base);
}

static int mcux_rtc_init(struct device *dev)
{
	const struct mcux_rtc_config *config = dev->config->config_info;
	struct mcux_rtc_data *data = dev->driver_data;

	k_sem_init(&data->sync, 1, UINT_MAX);

	/* Create default configuration and store it off */
	RTC_GetDefaultConfig(&data->config);
	RTC_Init(config->base, &data->config);

	/* Enable 32kHz oscillator and wait for 1ms to settle */
	config->base->CR |= 0x100;
	k_busy_wait(USEC_PER_MSEC);

	/* connect and enable the IRQ line */
	config->irq_config_func(dev);
	return 0;
}

static struct mcux_rtc_data rtc_mcux_data_0;

static void rtc_mcux_irq_config_0(struct device *dev);

static struct mcux_rtc_config rtc_mcux_config_0 = {
	.base = (RTC_Type *)CONFIG_RTC_MCUX_0_BASE_ADDRESS,
	.irq_config_func = rtc_mcux_irq_config_0,
};

DEVICE_DEFINE(rtc, CONFIG_RTC_MCUX_0_NAME,
	      &mcux_rtc_init, NULL, &rtc_mcux_data_0, &rtc_mcux_config_0,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &mcux_rtc_driver_api);

static void rtc_mcux_irq_config_0(struct device *dev)
{
	IRQ_CONNECT(CONFIG_RTC_MCUX_0_IRQ, CONFIG_RTC_MCUX_0_IRQ_PRI,
		    mcux_rtc_isr, DEVICE_GET(rtc), 0);
	irq_enable(CONFIG_RTC_MCUX_0_IRQ);
}
