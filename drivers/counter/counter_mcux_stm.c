/*
 * Copyright (c) 2023-2025 NXP.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_stm

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>

#include <fsl_stm.h>

LOG_MODULE_REGISTER(mcux_stm, CONFIG_COUNTER_LOG_LEVEL);

struct mcux_stm_channel_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
};

struct mcux_stm_config {
	struct counter_config_info info;
	STM_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint8_t prescale;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_stm_data {
	uint32_t freq;
	struct mcux_stm_channel_data channels[STM_CHANNEL_COUNT];
};

static int mcux_stm_start(const struct device *dev)
{
	const struct mcux_stm_config *config = dev->config;

	STM_StartTimer(config->base);

	return 0;
}

static int mcux_stm_stop(const struct device *dev)
{
	const struct mcux_stm_config *config = dev->config;

	STM_StopTimer(config->base);

	return 0;
}

static int mcux_stm_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_stm_config *config = dev->config;

	*ticks = STM_GetTimerCount(config->base);

	return 0;
}

static uint32_t mcux_stm_get_top_value(const struct device *dev)
{
	const struct mcux_stm_config *config = dev->config;

	return config->info.max_top_value;
}

static int mcux_stm_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct mcux_stm_config *config = dev->config;

	if (cfg->ticks != config->info.max_top_value) {
		return -ENOTSUP;
	} else {
		return 0;
	}
}

static int mcux_stm_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_stm_config *config = dev->config;
	struct mcux_stm_data *data = dev->data;
	uint32_t current = STM_GetTimerCount(config->base);
	uint32_t top_value = mcux_stm_get_top_value(dev);
	uint32_t ticks = alarm_cfg->ticks;

	if (chan_id >= config->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (data->channels[chan_id].alarm_callback != NULL) {
		LOG_ERR("channel already in use");
		return -EBUSY;
	}

	if (ticks > top_value) {
		return -EINVAL;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		if (top_value - current >= ticks) {
			ticks += current;
		} else {
			ticks -= top_value - current;
		}
	}

	data->channels[chan_id].alarm_callback = alarm_cfg->callback;
	data->channels[chan_id].alarm_user_data = alarm_cfg->user_data;

	STM_SetCompare(config->base, (stm_channel_t)chan_id, ticks);

	return 0;
}

static int mcux_stm_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_stm_config *config = dev->config;
	struct mcux_stm_data *data = dev->data;

	if (chan_id >= config->info.channels) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	STM_DisableCompareChannel(config->base, (stm_channel_t)chan_id);

	data->channels[chan_id].alarm_callback = NULL;
	data->channels[chan_id].alarm_user_data = NULL;

	return 0;
}

void mcux_stm_isr(const struct device *dev)
{
	const struct mcux_stm_config *config = dev->config;
	struct mcux_stm_data *data = dev->data;
	uint32_t current = STM_GetTimerCount(config->base);
	uint32_t status;

	/* Check the status flags for each channel */
	for (int chan_id = 0; chan_id < config->info.channels; chan_id++) {
		status = STM_GetStatusFlags(config->base, (stm_channel_t)chan_id);

		if ((status & STM_CIR_CIF_MASK) != 0) {
			STM_ClearStatusFlags(config->base, (stm_channel_t)chan_id);
		} else {
			continue;
		}

		if (data->channels[chan_id].alarm_callback != NULL) {

			counter_alarm_callback_t alarm_callback =
				data->channels[chan_id].alarm_callback;
			void *alarm_user_data = data->channels[chan_id].alarm_user_data;

			STM_DisableCompareChannel(config->base, (stm_channel_t)chan_id);
			data->channels[chan_id].alarm_callback = NULL;
			data->channels[chan_id].alarm_user_data = NULL;

			alarm_callback(dev, chan_id, current, alarm_user_data);
		}
	}
}

static uint32_t mcux_stm_get_pending_int(const struct device *dev)
{
	const struct mcux_stm_config *config = dev->config;
	uint32_t pending = 0;

	for (int i = 0; i < config->info.channels; i++) {
		pending |= STM_GetStatusFlags(config->base, (stm_channel_t)i);
	}

	return pending != 0 ? 1 : 0;
}

static uint32_t mcux_stm_get_freq(const struct device *dev)
{
	struct mcux_stm_data *data = dev->data;

	return data->freq;
}

static int mcux_stm_init(const struct device *dev)
{
	const struct mcux_stm_config *config = dev->config;
	struct mcux_stm_data *data = dev->data;
	stm_config_t stmConfig;
	uint32_t clock_freq;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	for (uint8_t chan = 0; chan < config->info.channels; chan++) {
		data->channels[chan].alarm_callback = NULL;
		data->channels[chan].alarm_user_data = NULL;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	data->freq = clock_freq / (config->prescale + 1);

	STM_GetDefaultConfig(&stmConfig);
	stmConfig.prescale = config->prescale;
	STM_Init(config->base, &stmConfig);

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(counter, mcux_stm_driver_api) = {
	.start = mcux_stm_start,
	.stop = mcux_stm_stop,
	.get_value = mcux_stm_get_value,
	.set_alarm = mcux_stm_set_alarm,
	.cancel_alarm = mcux_stm_cancel_alarm,
	.set_top_value = mcux_stm_set_top_value,
	.get_pending_int = mcux_stm_get_pending_int,
	.get_top_value = mcux_stm_get_top_value,
	.get_freq = mcux_stm_get_freq,
};

#define COUNTER_MCUX_STM_DEVICE_INIT(n)     \
	static struct mcux_stm_data mcux_stm_data_##n;      \
	static void mcux_stm_irq_config_##n(const struct device *dev);      \
                                                \
	static const struct mcux_stm_config mcux_stm_config_##n = {     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),     \
		.clock_subsys =	(clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),   \
		.info = {	\
			.max_top_value = UINT32_MAX,	\
			.channels = STM_CHANNEL_COUNT,	\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,	\
		},	\
		.base = (STM_Type *)DT_INST_REG_ADDR(n),	\
		.prescale = DT_INST_PROP(n, prescaler),	\
		.irq_config_func = mcux_stm_irq_config_##n,	\
	};                                                                          \
                                                                                \
	DEVICE_DT_INST_DEFINE(n, mcux_stm_init, NULL, &mcux_stm_data_##n,	\
						&mcux_stm_config_##n, POST_KERNEL,	\
						CONFIG_COUNTER_INIT_PRIORITY,   \
						&mcux_stm_driver_api);	\
												\
	static void mcux_stm_irq_config_##n(const struct device *dev)	\
	{	\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_stm_isr,	\
					DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));		\
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_STM_DEVICE_INIT)
