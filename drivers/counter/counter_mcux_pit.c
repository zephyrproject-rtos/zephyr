/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_kinetis_pit

#include <drivers/counter.h>
#include <fsl_pit.h>

#define LOG_MODULE_NAME counter_pit
#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_COUNTER_LOG_LEVEL);

struct mcux_pit_config {
	struct counter_config_info info;
	PIT_Type *base;
	bool enableRunInDebug;
	pit_chnl_t pit_channel;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_pit_data {
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
};

static uint32_t mcux_pit_get_top_value(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;
	pit_chnl_t channel = config->pit_channel;

	return config->base->CHANNEL[channel].LDVAL;
}

static int mcux_pit_start(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;

	LOG_DBG("period is %d", mcux_pit_get_top_value(dev));
	PIT_EnableInterrupts(config->base, config->pit_channel,
			     kPIT_TimerInterruptEnable);
	PIT_StartTimer(config->base, config->pit_channel);
	return 0;
}

static int mcux_pit_stop(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;

	PIT_DisableInterrupts(config->base, config->pit_channel,
			      kPIT_TimerInterruptEnable);
	PIT_StopTimer(config->base, config->pit_channel);

	return 0;
}

static int mcux_pit_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_pit_config *config = dev->config;

	*ticks = PIT_GetCurrentTimerCount(config->base, config->pit_channel);

	return 0;
}

static int mcux_pit_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_pit_config *config = dev->config;
	struct mcux_pit_data *data = dev->data;
	pit_chnl_t channel = config->pit_channel;

	if (cfg->ticks == 0) {
		return -EINVAL;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (config->base->CHANNEL[channel].TCTRL & PIT_TCTRL_TEN_MASK) {
		/* Timer already enabled, check flags before resetting */
		if (cfg->flags & COUNTER_TOP_CFG_DONT_RESET) {
			return -ENOTSUP;
		}
		PIT_StopTimer(config->base, channel);
		PIT_SetTimerPeriod(config->base, channel, cfg->ticks);
		PIT_StartTimer(config->base, channel);
	} else {
		PIT_SetTimerPeriod(config->base, channel, cfg->ticks);
	}

	return 0;
}

static uint32_t mcux_pit_get_pending_int(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;
	uint32_t mask = PIT_TFLG_TIF_MASK;
	uint32_t flags;

	flags = PIT_GetStatusFlags(config->base, config->pit_channel);

	return ((flags & mask) == mask);
}

static void mcux_pit_isr(const struct device *dev)
{
	const struct mcux_pit_config *config = dev->config;
	struct mcux_pit_data *data = dev->data;
	uint32_t flags;
	uint32_t current = 0;

	LOG_DBG("pit counter isr");
	flags = PIT_GetStatusFlags(config->base, config->pit_channel);
	PIT_ClearStatusFlags(config->base, config->pit_channel, flags);
	if (data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
	if (data->alarm_callback) {
		PIT_StopTimer(config->base, config->pit_channel);
		mcux_pit_get_value(dev, &current);
		data->alarm_callback(dev, config->pit_channel, current,
				     data->alarm_user_data);
	}
}

static int mcux_pit_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_pit_config *config = dev->config;
	struct mcux_pit_data *data = dev->data;

	uint32_t ticks = alarm_cfg->ticks;

	if (chan_id != DT_PROP(DT_DRV_INST(0), pit_channel)) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (ticks > mcux_pit_get_top_value(dev)) {
		LOG_ERR("Invalid tciks");
		return -EINVAL;
	}

	PIT_StopTimer(config->base, chan_id);
	PIT_SetTimerPeriod(config->base, chan_id, ticks);
	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;
	LOG_DBG("set alarm to %d", ticks);
	PIT_StartTimer(config->base, chan_id);
	return 0;
}

static int mcux_pit_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_pit_config *config = dev->config;
	struct mcux_pit_data *data = dev->data;

	if (chan_id != DT_PROP(DT_DRV_INST(0), pit_channel)) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	PIT_DisableInterrupts(config->base, chan_id, kPIT_TimerInterruptEnable);
	PIT_StopTimer(config->base, chan_id);
	data->alarm_callback = NULL;

	return 0;
}

static int mcux_pit_init(const struct device *dev)
{
	const struct mcux_pit_config *config =
		(struct mcux_pit_config *)dev->config;
	pit_config_t pit_config;

	PIT_GetDefaultConfig(&pit_config);
	pit_config.enableRunInDebug = config->enableRunInDebug;

	PIT_Init(config->base, &pit_config);

	config->irq_config_func(dev);

	PIT_SetTimerPeriod(config->base, config->pit_channel,
			   USEC_TO_COUNT(DT_PROP(DT_DRV_INST(0), pit_period),
					 CLOCK_GetFreq(kCLOCK_BusClk)));

	return 0;
}

static const struct counter_driver_api mcux_pit_driver_api = {
	.start = mcux_pit_start,
	.stop = mcux_pit_stop,
	.get_value = mcux_pit_get_value,
	.set_top_value = mcux_pit_set_top_value,
	.set_alarm = mcux_pit_set_alarm,
	.cancel_alarm = mcux_pit_cancel_alarm,
	.get_pending_int = mcux_pit_get_pending_int,
	.get_top_value = mcux_pit_get_top_value,
};

/*
 * This driver is single-instance. If the devicetree contains multiple
 * instances, this will fail and the driver needs to be revisited.
 */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "unsupported pit instance");

static struct mcux_pit_data mcux_pit_data_0;

static void mcux_pit_irq_config_0(const struct device *dev);

static const struct mcux_pit_config mcux_pit_config_0 = {
	.info = {
		.max_top_value = UINT32_MAX,
		.channels = 1,
		.freq = DT_PROP(DT_DRV_INST(0), clock_frequency),
	},
	.base = (PIT_Type *)DT_INST_REG_ADDR(0),
	.pit_channel = DT_PROP(DT_DRV_INST(0), pit_channel),
	.irq_config_func = mcux_pit_irq_config_0,
};

DEVICE_DT_INST_DEFINE(0, &mcux_pit_init, NULL,
		    &mcux_pit_data_0, &mcux_pit_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &mcux_pit_driver_api);

static void mcux_pit_irq_config_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 0, irq),
		    DT_INST_IRQ_BY_IDX(0, 0, priority), mcux_pit_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 0, irq));
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 1, irq),
		    DT_INST_IRQ_BY_IDX(0, 1, priority), mcux_pit_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 1, irq));
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 2, irq),
		    DT_INST_IRQ_BY_IDX(0, 2, priority), mcux_pit_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 2, irq));
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(0, 3, irq),
		    DT_INST_IRQ_BY_IDX(0, 3, priority), mcux_pit_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQ_BY_IDX(0, 3, irq));
}
