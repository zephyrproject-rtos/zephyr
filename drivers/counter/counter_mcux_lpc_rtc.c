/*
 * Copyright 2021-23, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <fsl_rtc.h>
#include "fsl_power.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mcux_rtc, CONFIG_COUNTER_LOG_LEVEL);

struct mcux_lpc_rtc_data {
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
	uint32_t value;
};

struct mcux_lpc_rtc_config {
	struct counter_config_info info;
	RTC_Type *base;
	const struct device *rtc_dev;
	void (*irq_config_func)(const struct device *dev);
	/* Device defined as wake-up source */
	bool wakeup_source;
};

#if CONFIG_COUNTER_MCUX_LPC_RTC_HIGHRES
static int mcux_lpc_rtc_highres_start(const struct device *dev);
#endif

static void mcux_lpc_rtc_isr(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);
	struct mcux_lpc_rtc_data *data = dev->data;
	counter_alarm_callback_t cb;
	uint32_t current = RTC_GetSecondsTimerCount(config->base);

	LOG_DBG("Current time is %d ticks", current);

	if ((RTC_GetStatusFlags(config->base) & RTC_CTRL_ALARM1HZ_MASK) &&
	    (data->alarm_callback)) {
		cb = data->alarm_callback;
		data->alarm_callback = NULL;
		cb(dev, 0, current, data->alarm_user_data);
	}

	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}

	/*
	 * Clear any conditions to ack the IRQ
	 *
	 * callback may have already reset the alarm flag if a new
	 * alarm value was programmed to the TAR
	 */
	if (RTC_GetStatusFlags(config->base) & RTC_CTRL_ALARM1HZ_MASK) {
		RTC_ClearStatusFlags(config->base, kRTC_AlarmFlag);
	}

	/* Check if the Wake counter interrupt was set */
	if (RTC_GetStatusFlags(config->base) & RTC_CTRL_WAKE1KHZ_MASK) {
		RTC_ClearStatusFlags(config->base, kRTC_WakeupFlag);
#if CONFIG_COUNTER_MCUX_LPC_RTC_HIGHRES
		if (config->base->CTRL & RTC_CTRL_RTC1KHZ_EN_MASK) {
			mcux_lpc_rtc_highres_start(dev);
		}
#endif
	}
}

#if CONFIG_COUNTER_MCUX_LPC_RTC_1HZ

#define DT_DRV_COMPAT nxp_lpc_rtc

static int mcux_lpc_rtc_start(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	RTC_EnableTimer(config->base, true);

	return 0;
}

static int mcux_lpc_rtc_stop(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	RTC_EnableTimer(config->base, false);

	/* clear out any set alarms */
	RTC_SetSecondsTimerMatch(config->base, 0);

	return 0;
}

static uint32_t mcux_lpc_rtc_read(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	uint32_t ticks = RTC_GetSecondsTimerCount(config->base);

	return ticks;
}

static int mcux_lpc_rtc_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = mcux_lpc_rtc_read(dev);
	return 0;
}

static int mcux_lpc_rtc_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);
	struct mcux_lpc_rtc_data *data = dev->data;

	uint32_t ticks = alarm_cfg->ticks;
	uint32_t current = mcux_lpc_rtc_read(dev);

	LOG_DBG("Current time is %d ticks", current);

	if (chan_id != 0U) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (data->alarm_callback != NULL) {
		return -EBUSY;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
	}

	if (ticks < current) {
		LOG_ERR("Alarm cannot be earlier than current time");
		return -EINVAL;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	RTC_SetSecondsTimerMatch(config->base, ticks);
	LOG_DBG("Alarm set to %d ticks", ticks);

	return 0;
}

static int mcux_lpc_rtc_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct mcux_lpc_rtc_data *data = dev->data;

	if (chan_id != 0U) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	data->alarm_callback = NULL;

	return 0;
}

static int mcux_lpc_rtc_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	return -ENOTSUP;
}

static uint32_t mcux_lpc_rtc_get_pending_int(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	return RTC_GetStatusFlags(config->base) & RTC_CTRL_ALARM1HZ_MASK;
}

static uint32_t mcux_lpc_rtc_get_top_value(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value;
}

static int mcux_lpc_rtc_init(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	RTC_Init(config->base);

	/* Issue a software reset to set the registers to init state */
	RTC_Reset(config->base);

	config->irq_config_func(dev);

	if (config->wakeup_source) {
		/* Enable the bit to wakeup from Deep Power Down mode */
		RTC_EnableAlarmTimerInterruptFromDPD(config->base, true);
	}

	return 0;
}

static const struct counter_driver_api mcux_rtc_driver_api = {
	.start = mcux_lpc_rtc_start,
	.stop = mcux_lpc_rtc_stop,
	.get_value = mcux_lpc_rtc_get_value,
	.set_alarm = mcux_lpc_rtc_set_alarm,
	.cancel_alarm = mcux_lpc_rtc_cancel_alarm,
	.set_top_value = mcux_lpc_rtc_set_top_value,
	.get_pending_int = mcux_lpc_rtc_get_pending_int,
	.get_top_value = mcux_lpc_rtc_get_top_value,
};

#define COUNTER_LPC_RTC_DEVICE(id)						\
	static void mcux_lpc_rtc_irq_config_##id(const struct device *dev);	\
	static const struct mcux_lpc_rtc_config mcux_lpc_rtc_config_##id = {	\
		.base = (RTC_Type *)DT_INST_REG_ADDR(id),			\
		.irq_config_func = mcux_lpc_rtc_irq_config_##id,		\
		.rtc_dev = DEVICE_DT_GET_OR_NULL(DT_INST_CHILD(id, rtc_highres)),	\
		.info = {						\
			.max_top_value = UINT32_MAX,			\
			.freq = 1,					\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
			.channels = 1,					\
		},							\
		.wakeup_source = DT_INST_PROP(id, wakeup_source)	\
	};								\
	static struct mcux_lpc_rtc_data mcux_lpc_rtc_data_##id;		\
	DEVICE_DT_INST_DEFINE(id, &mcux_lpc_rtc_init, NULL,		\
				&mcux_lpc_rtc_data_##id, &mcux_lpc_rtc_config_##id.info,	\
				POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
				&mcux_rtc_driver_api);					\
	static void mcux_lpc_rtc_irq_config_##id(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(id),					\
			DT_INST_IRQ(id, priority),				\
			mcux_lpc_rtc_isr, DEVICE_DT_INST_GET(id), 0);		\
		irq_enable(DT_INST_IRQN(id));					\
		if (DT_INST_PROP(id, wakeup_source)) {				\
			EnableDeepSleepIRQ(DT_INST_IRQN(id));			\
		}								\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_LPC_RTC_DEVICE)
#endif

#if CONFIG_COUNTER_MCUX_LPC_RTC_HIGHRES

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_lpc_rtc_highres

static int mcux_lpc_rtc_highres_start(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);
	struct mcux_lpc_rtc_data *data = dev->data;

	if (config->rtc_dev) {
		/* We have another RTC driver enabled, check if RTC is enabled */
		if ((config->base->CTRL & RTC_CTRL_RTC_EN_MASK) == 0) {
			/* RTC is not enabled and we do not turn it on as it will effect
			 * the RTC counter value thereby affecting the RTC counter drivers
			 */
			LOG_ERR("RTC Wake counter cannot be started as RTC is not enabled.");
			return -EINVAL;
		}
	} else {
		if ((config->base->CTRL & RTC_CTRL_RTC_EN_MASK) == 0) {
			RTC_EnableTimer(config->base, true);
		}
	}

	if (data->value == 0) {
		/* Start from the max value */
		RTC_SetWakeupCount(config->base, counter_get_top_value(dev));
	} else {
		RTC_SetWakeupCount(config->base, data->value);
	}

	return 0;
}

static int mcux_lpc_rtc_highres_stop(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	config->base->CTRL &= ~RTC_CTRL_RTC1KHZ_EN_MASK;

	if (config->rtc_dev == NULL) {
		/* Disable RTC as no other driver is using it */
		RTC_EnableTimer(config->base, false);
	}

	return 0;
}

static uint32_t mcux_lpc_rtc_highres_read(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	uint32_t ticks = RTC_GetWakeupCount(config->base);

	return ticks;
}

static int mcux_lpc_rtc_highres_set_alarm(const struct device *dev, uint8_t chan_id,
					  const struct counter_alarm_cfg *alarm_cfg)
{
	return -ENOTSUP;
}

static int mcux_lpc_rtc_highres_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	return -ENOTSUP;
}


static int mcux_lpc_rtc_highres_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = mcux_lpc_rtc_highres_read(dev);
	return 0;
}

static int mcux_lpc_rtc_highres_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);
	struct mcux_lpc_rtc_data *data = dev->data;

	if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
		return -ENOTSUP;
	}

	data->value = cfg->ticks;
	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (config->base->CTRL & RTC_CTRL_RTC1KHZ_EN_MASK) {
		return mcux_lpc_rtc_highres_start(dev);
	}

	return 0;
}

static uint32_t mcux_lpc_rtc_highres_get_pending_int(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	return RTC_GetStatusFlags(config->base) & RTC_CTRL_WAKE1KHZ_MASK;
}

static uint32_t mcux_lpc_rtc_highres_get_top_value(const struct device *dev)
{
	struct mcux_lpc_rtc_data *data = dev->data;
	const struct counter_config_info *info = dev->config;

	if (data->value == 0) {
		return info->max_top_value;
	} else {
		return data->value;
	}
}

static int mcux_lpc_rtc_highres_init(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_lpc_rtc_config *config =
		CONTAINER_OF(info, struct mcux_lpc_rtc_config, info);

	/* Initialize the RTC if this is only driver using it */
	if (config->rtc_dev == NULL) {
		RTC_Init(config->base);

		/* Issue a software reset to set the registers to init state */
		RTC_Reset(config->base);

		config->irq_config_func(dev);
	}

	if (config->wakeup_source) {
		/* Enable the bit to wakeup from Deep Power Down mode */
		RTC_EnableWakeUpTimerInterruptFromDPD(config->base, true);
	}

	return 0;
}

static const struct counter_driver_api mcux_rtc_highres_driver_api = {
	.start = mcux_lpc_rtc_highres_start,
	.stop = mcux_lpc_rtc_highres_stop,
	.get_value = mcux_lpc_rtc_highres_get_value,
	.set_alarm = mcux_lpc_rtc_highres_set_alarm,
	.cancel_alarm = mcux_lpc_rtc_highres_cancel_alarm,
	.set_top_value = mcux_lpc_rtc_highres_set_top_value,
	.get_pending_int = mcux_lpc_rtc_highres_get_pending_int,
	.get_top_value = mcux_lpc_rtc_highres_get_top_value,
};

#define COUNTER_LPC_RTC_HIGHRES_IRQ_INIT(n)							\
	do {											\
		IRQ_CONNECT(DT_IRQN(DT_INST_PARENT(n)),						\
			DT_IRQ(DT_INST_PARENT(n), priority),					\
			mcux_lpc_rtc_isr,							\
			DEVICE_DT_INST_GET(n), 0);						\
		irq_enable(DT_IRQN(DT_INST_PARENT(n)));						\
		if (DT_INST_PROP(n, wakeup_source)) {						\
			EnableDeepSleepIRQ(DT_IRQN(DT_INST_PARENT(n)));				\
		}										\
	} while (false)

#define COUNTER_LPC_RTC_HIGHRES_DEVICE(id)							\
	static void mcux_lpc_rtc_highres_irq_config_##id(const struct device *dev);		\
	static const struct mcux_lpc_rtc_config mcux_lpc_rtc_highres_config_##id = {		\
		.base = (RTC_Type *)DT_REG_ADDR(DT_INST_PARENT(id)),				\
		.rtc_dev = DEVICE_DT_GET_OR_NULL(DT_INST_PARENT(id)),				\
		.irq_config_func = mcux_lpc_rtc_highres_irq_config_##id,			\
		.info = {									\
			.max_top_value = UINT16_MAX,						\
			.freq = 1000,								\
			.channels = 0,								\
		},										\
		.wakeup_source = DT_INST_PROP(id, wakeup_source)				\
	};											\
	static struct mcux_lpc_rtc_data mcux_lpc_rtc_highres_data_##id;				\
	DEVICE_DT_INST_DEFINE(id, &mcux_lpc_rtc_highres_init, NULL,				\
				&mcux_lpc_rtc_highres_data_##id,				\
				&mcux_lpc_rtc_highres_config_##id.info,				\
				POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,			\
				&mcux_rtc_highres_driver_api);					\
	static void mcux_lpc_rtc_highres_irq_config_##id(const struct device *dev)		\
	{											\
		COND_CODE_1(IS_ENABLED(DT_HAS_COMPAT_STATUS_OKAY(nxp_lpc_rtc)),			\
			 (), (COUNTER_LPC_RTC_HIGHRES_IRQ_INIT(id));)				\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_LPC_RTC_HIGHRES_DEVICE)

#endif
