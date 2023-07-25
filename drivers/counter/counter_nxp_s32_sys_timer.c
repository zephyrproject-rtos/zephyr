/*
 * Copyright 2022 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <soc.h>

#include <Stm_Ip.h>

LOG_MODULE_REGISTER(nxp_s32_sys_timer, CONFIG_COUNTER_LOG_LEVEL);

#define SYS_TIMER_NODE(n)		DT_NODELABEL(stm##n)
#define SYS_TIMER_MAX_VALUE		0xFFFFFFFFU
#define SYS_TIMER_NUM_CHANNELS		4
#define SYS_TIMER_INSTANCE_ID(n)	(n + 3 + CONFIG_NXP_S32_RTU_INDEX * 4)

#define _SYS_TIMER_ISR(r, n)	RTU##r##_STM_##n##_ISR
#define SYS_TIMER_ISR(r, n)	_SYS_TIMER_ISR(r, n)

struct nxp_s32_sys_timer_chan_data {
	counter_alarm_callback_t callback;
	void *user_data;
};

struct nxp_s32_sys_timer_data {
	struct nxp_s32_sys_timer_chan_data ch_data[SYS_TIMER_NUM_CHANNELS];
};

struct nxp_s32_sys_timer_config {
	struct counter_config_info info;
	Stm_Ip_InstanceConfigType hw_cfg;
	Stm_Ip_ChannelConfigType ch_cfg[SYS_TIMER_NUM_CHANNELS];
	uint8_t instance;
};

static int nxp_s32_sys_timer_start(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	Stm_Ip_StartTimer(config->instance, 0);

	return 0;
}

static int nxp_s32_sys_timer_stop(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	Stm_Ip_StopTimer(config->instance);

	return 0;
}

static int nxp_s32_sys_timer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	*ticks = Stm_Ip_GetCounterValue(config->instance);

	return 0;
}

static int nxp_s32_sys_timer_set_alarm(const struct device *dev, uint8_t chan_id,
				       const struct counter_alarm_cfg *alarm_cfg)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	struct nxp_s32_sys_timer_data *data = dev->data;
	struct nxp_s32_sys_timer_chan_data *ch_data = &data->ch_data[chan_id];
	uint32_t ticks = alarm_cfg->ticks;

	if (ch_data->callback) {
		return -EBUSY;
	}

	if (ticks > config->info.max_top_value) {
		LOG_ERR("Invalid ticks value %d", ticks);
		return -EINVAL;
	}

	ch_data->callback = alarm_cfg->callback;
	ch_data->user_data = alarm_cfg->user_data;

	/* Disable the channel before loading the new value so that it takes effect immediately */
	Stm_Ip_DisableChannel(config->instance, chan_id);

	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		Stm_Ip_StartCountingAbsolute(config->instance, chan_id, ticks);
	} else {
		Stm_Ip_StartCounting(config->instance, chan_id, ticks);
	}

	return 0;
}

static int nxp_s32_sys_timer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	struct nxp_s32_sys_timer_data *data = dev->data;
	struct nxp_s32_sys_timer_chan_data *ch_data = &data->ch_data[chan_id];

	Stm_Ip_DisableChannel(config->instance, chan_id);
	ch_data->callback = NULL;
	ch_data->user_data = NULL;

	return 0;
}

static uint32_t nxp_s32_sys_timer_get_pending_int(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	uint32_t flags = 0;
	uint8_t i;

	for (i = 0; i < counter_get_num_of_channels(dev); i++) {
		flags = Stm_Ip_GetInterruptFlag(config->instance, i);
		if (flags) {
			break;
		}
	}

	return flags;
}

static int nxp_s32_sys_timer_set_top_value(const struct device *dev,
					   const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	/* Overflow is fixed and cannot be changed */
	return -ENOTSUP;
}

static uint32_t nxp_s32_sys_timer_get_top_value(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	return config->info.max_top_value;
}

static uint32_t nxp_s32_sys_timer_get_frequency(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;

	return config->info.freq / (config->hw_cfg.clockPrescaler + 1U);
}

static int nxp_s32_sys_timer_init(const struct device *dev)
{
	const struct nxp_s32_sys_timer_config *config = dev->config;
	struct nxp_s32_sys_timer_data *data = dev->data;
	struct nxp_s32_sys_timer_chan_data *ch_data;
	int i;

	Stm_Ip_Init(config->instance, &config->hw_cfg);

	for (i = 0; i < counter_get_num_of_channels(dev); i++) {
		ch_data = &data->ch_data[i];
		ch_data->callback = NULL;
		ch_data->user_data = NULL;
		Stm_Ip_InitChannel(config->instance, &config->ch_cfg[i]);
	}

	return 0;
}

static const struct counter_driver_api nxp_s32_sys_timer_driver_api = {
	.start = nxp_s32_sys_timer_start,
	.stop = nxp_s32_sys_timer_stop,
	.get_value = nxp_s32_sys_timer_get_value,
	.set_alarm = nxp_s32_sys_timer_set_alarm,
	.cancel_alarm = nxp_s32_sys_timer_cancel_alarm,
	.set_top_value = nxp_s32_sys_timer_set_top_value,
	.get_pending_int = nxp_s32_sys_timer_get_pending_int,
	.get_top_value = nxp_s32_sys_timer_get_top_value,
	.get_freq = nxp_s32_sys_timer_get_frequency
};

#define SYS_TIMER_CHANNEL_CFG(i, n)				\
	{							\
		.hwChannel = i,					\
		.callback = &nxp_s32_sys_timer_##n##_callback,	\
		.callbackParam = i,				\
		.channelMode = STM_IP_CH_MODE_ONESHOT,		\
	}

#define SYS_TIMER_ISR_DECLARE(n)	\
	extern void SYS_TIMER_ISR(CONFIG_NXP_S32_RTU_INDEX, n)(void)

#define SYS_TIMER_INIT_DEVICE(n)							\
	SYS_TIMER_ISR_DECLARE(n);							\
											\
	void nxp_s32_sys_timer_##n##_callback(uint8_t chan_id)				\
	{										\
		const struct device *dev = DEVICE_DT_GET(SYS_TIMER_NODE(n));		\
		const struct nxp_s32_sys_timer_config *config = dev->config;		\
		struct nxp_s32_sys_timer_data *data = dev->data;			\
		struct nxp_s32_sys_timer_chan_data *ch_data = &data->ch_data[chan_id];	\
		counter_alarm_callback_t cb = ch_data->callback;			\
		uint32_t val;								\
											\
		ch_data->callback = NULL;						\
		if (cb) {								\
			val = Stm_Ip_GetCounterValue(config->instance);			\
			cb(dev, chan_id, val, ch_data->user_data);			\
		}									\
	}										\
											\
	static int nxp_s32_sys_timer_##n##_init(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_IRQN(SYS_TIMER_NODE(n)),					\
			    DT_IRQ(SYS_TIMER_NODE(n), priority),			\
			    SYS_TIMER_ISR(CONFIG_NXP_S32_RTU_INDEX, n),			\
			    DEVICE_DT_GET(SYS_TIMER_NODE(n)),				\
			    DT_IRQ(SYS_TIMER_NODE(n), flags));				\
		irq_enable(DT_IRQN(SYS_TIMER_NODE(n)));					\
											\
		return nxp_s32_sys_timer_init(dev);					\
	}										\
											\
	static struct nxp_s32_sys_timer_data nxp_s32_sys_timer_data_##n;		\
											\
	static const struct nxp_s32_sys_timer_config nxp_s32_sys_timer_config_##n = {	\
		.info = {								\
			.max_top_value = SYS_TIMER_MAX_VALUE,				\
			.freq = (DT_PROP(SYS_TIMER_NODE(n), clock_frequency)),		\
			.channels = SYS_TIMER_NUM_CHANNELS,				\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,				\
		},									\
		.hw_cfg = {								\
			.stopInDebugMode = DT_PROP(SYS_TIMER_NODE(n), freeze),		\
			.clockPrescaler = DT_PROP(SYS_TIMER_NODE(n), prescaler) - 1,	\
		},									\
		.ch_cfg = {								\
			LISTIFY(SYS_TIMER_NUM_CHANNELS, SYS_TIMER_CHANNEL_CFG, (,), n)	\
		},									\
		.instance = SYS_TIMER_INSTANCE_ID(n),					\
	};										\
											\
	DEVICE_DT_DEFINE(SYS_TIMER_NODE(n),						\
			 nxp_s32_sys_timer_##n##_init,					\
			 NULL,								\
			 &nxp_s32_sys_timer_data_##n,					\
			 &nxp_s32_sys_timer_config_##n,					\
			 POST_KERNEL,							\
			 CONFIG_COUNTER_INIT_PRIORITY,					\
			 &nxp_s32_sys_timer_driver_api);

#if DT_NODE_HAS_STATUS(SYS_TIMER_NODE(0), okay)
SYS_TIMER_INIT_DEVICE(0)
#endif

#if DT_NODE_HAS_STATUS(SYS_TIMER_NODE(1), okay)
SYS_TIMER_INIT_DEVICE(1)
#endif

#if DT_NODE_HAS_STATUS(SYS_TIMER_NODE(2), okay)
SYS_TIMER_INIT_DEVICE(2)
#endif

#if DT_NODE_HAS_STATUS(SYS_TIMER_NODE(3), okay)
SYS_TIMER_INIT_DEVICE(3)
#endif
