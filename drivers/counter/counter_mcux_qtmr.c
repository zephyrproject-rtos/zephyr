/*
 * Copyright (c) 2022 KT-Elektronik, Klaucke und Partner GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 *
 * Counter driver for the Quad Timer through the MCUxpresso SDK. Based mainly on counter_mcux_gpt.c
 *
 * Each quad timer module has four channels (0-3) that can operate independently, but the Zephyr
 * counter-API does not support starting or stopping different channels independently. Hence, each
 * channel is represented as an independent counter device.
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <fsl_qtmr.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mcux_qtmr, CONFIG_COUNTER_LOG_LEVEL);

struct mcux_qtmr_config {
	/* info must be first element */
	struct counter_config_info info;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	TMR_Type *base;
	clock_name_t clock_source;
	qtmr_channel_selection_t channel;
	qtmr_config_t qtmr_config;
	qtmr_counting_mode_t mode;
};

struct mcux_qtmr_data {
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
	qtmr_status_flags_t interrupt_mask;
	uint32_t freq;
};

/* Only one interrupt per QTMR module. Each of which has four timers. */
#define DT_DRV_COMPAT nxp_imx_qtmr

/**
 * @brief ISR for a specific timer channel
 *
 * @param dev	timer channel device
 */
void mcux_qtmr_timer_handler(const struct device *dev, uint32_t status)
{
	const struct mcux_qtmr_config *config = dev->config;
	struct mcux_qtmr_data *data = dev->data;
	uint32_t current = QTMR_GetCurrentTimerCount(config->base, config->channel);

	QTMR_ClearStatusFlags(config->base, config->channel, status);
	__DSB();

	if ((status & kQTMR_Compare1Flag) && data->alarm_callback) {
		QTMR_DisableInterrupts(config->base, config->channel,
			kQTMR_Compare1InterruptEnable);
		data->interrupt_mask &= ~kQTMR_Compare1InterruptEnable;
		counter_alarm_callback_t alarm_cb = data->alarm_callback;

		data->alarm_callback = NULL;
		alarm_cb(dev, config->channel, current, data->alarm_user_data);
	}

	if ((status & kQTMR_OverflowFlag) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
}

/**
 * @brief ISR for the QTMR
 *
 * @param timers	array containing the counter devices for each channel of the timer module
 */
static void mcux_qtmr_isr(const struct device *timers[])
{
	/* the interrupt can be triggered by any of the four channels of the QTMR. Check status
	 * of all channels and trigger the ISR for the channel(s) that has/have triggered the
	 * interrupt.
	 */
	for (qtmr_channel_selection_t ch = kQTMR_Channel_0; ch <= kQTMR_Channel_3 ; ch++) {
		if (timers[ch] != NULL) {
			const struct mcux_qtmr_config *config = timers[ch]->config;
			struct mcux_qtmr_data *data = timers[ch]->data;

			uint32_t channel_status = QTMR_GetStatus(config->base, ch);

			if ((channel_status & data->interrupt_mask) != 0) {
				mcux_qtmr_timer_handler(timers[ch], channel_status);
			}
		}
	}
}

#define INIT_TIMER(node_id) [DT_PROP(node_id, channel)] = DEVICE_DT_GET(node_id),

#define QTMR_DEVICE_INIT_MCUX(n)							\
	static const struct device *const timers_##n[4] = {				\
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(n), INIT_TIMER)		\
	};										\
	static int init_irq_##n(const struct device *dev)				\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mcux_qtmr_isr,	\
				timers_##n, 0);						\
		irq_enable(DT_INST_IRQN(n));						\
		return 0;								\
	}										\
											\
	SYS_INIT(init_irq_##n, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY);

DT_INST_FOREACH_STATUS_OKAY(QTMR_DEVICE_INIT_MCUX)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nxp_imx_tmr

static int mcux_qtmr_start(const struct device *dev)
{
	const struct mcux_qtmr_config *config = dev->config;

	QTMR_StartTimer(config->base, config->channel, config->mode);

	return 0;
}

static int mcux_qtmr_stop(const struct device *dev)
{
	const struct mcux_qtmr_config *config = dev->config;

	QTMR_StopTimer(config->base, config->channel);

	return 0;
}

static int mcux_qtmr_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_qtmr_config *config = dev->config;

	*ticks = QTMR_GetCurrentTimerCount(config->base, config->channel);
	return 0;
}

static int mcux_qtmr_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_qtmr_config *config = dev->config;
	struct mcux_qtmr_data *data = dev->data;
	uint32_t current;
	uint32_t ticks;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (data->alarm_callback) {
		return -EBUSY;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	current = QTMR_GetCurrentTimerCount(config->base, config->channel);
	ticks = alarm_cfg->ticks;

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
	}

	/* this timer always counts up. */
	config->base->CHANNEL[config->channel].COMP1 = ticks;

	data->interrupt_mask |= kQTMR_Compare1InterruptEnable;
	QTMR_EnableInterrupts(config->base, config->channel, data->interrupt_mask);

	return 0;
}

static int mcux_qtmr_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_qtmr_config *config = dev->config;
	struct mcux_qtmr_data *data = dev->data;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	QTMR_DisableInterrupts(config->base, config->channel, data->interrupt_mask);
	data->interrupt_mask &= ~kQTMR_Compare1InterruptEnable;
	data->alarm_callback = NULL;

	return 0;
}

static uint32_t mcux_qtmr_get_pending_int(const struct device *dev)
{
	const struct mcux_qtmr_config *config = dev->config;

	return QTMR_GetStatus(config->base, config->channel);
}

static int mcux_qtmr_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct mcux_qtmr_config *config = dev->config;
	struct mcux_qtmr_data *data = dev->data;

	if (cfg->ticks != config->info.max_top_value) {
		LOG_ERR("Wrap can only be set to 0x%x",
			config->info.max_top_value);
		return -ENOTSUP;
	}

	if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) == 0) {
		if ((config->base->CHANNEL[config->channel].CTRL & TMR_CTRL_DIR_MASK) != 0U) {
			/* counting down, reset to UINT16MAX */
			config->base->CHANNEL[config->channel].CNTR = UINT16_MAX;
		} else {
			/* counting up, reset to 0 */
			config->base->CHANNEL[config->channel].CNTR = 0;
		}
	}

	if (cfg->callback != NULL) {
		data->top_callback = cfg->callback;
		data->top_user_data = cfg->user_data;

		data->interrupt_mask |= kQTMR_OverflowInterruptEnable;
		QTMR_EnableInterrupts(config->base, config->channel, kQTMR_OverflowInterruptEnable);
	}

	return 0;
}

static uint32_t mcux_qtmr_get_top_value(const struct device *dev)
{
	const struct mcux_qtmr_config *config = dev->config;

	return config->info.max_top_value;
}

static uint32_t mcux_qtmr_get_freq(const struct device *dev)
{
	struct mcux_qtmr_data *data = dev->data;

	return data->freq;
}

/**
 * @brief look up table for dividers when using internal clock sources kQTMR_ClockDivide_1 to
 * kQTMR_ClockDivide_128
 */
static const uint8_t qtmr_primary_source_divider[] = {1, 2, 4, 8, 16, 32, 64, 128};

static int mcux_qtmr_init(const struct device *dev)
{
	const struct mcux_qtmr_config *config = dev->config;
	struct mcux_qtmr_data *data = dev->data;

	if (config->qtmr_config.primarySource < kQTMR_ClockDivide_1) {
		/* for external sources, use the value from the dts (if given) */
		data->freq = config->info.freq;
	} else {
		/* bus clock with divider */
		if (!device_is_ready(config->clock_dev)) {
			LOG_ERR("clock control device not ready");
			return -ENODEV;
		}

		if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
					&data->freq)) {
			return -EINVAL;
		}

		data->freq /= qtmr_primary_source_divider[config->qtmr_config.primarySource -
								kQTMR_ClockDivide_1];
	}

	QTMR_Init(config->base, config->channel, &config->qtmr_config);

	return 0;
}

static const struct counter_driver_api mcux_qtmr_driver_api = {
	.start = mcux_qtmr_start,
	.stop = mcux_qtmr_stop,
	.get_value = mcux_qtmr_get_value,
	.set_alarm = mcux_qtmr_set_alarm,
	.cancel_alarm = mcux_qtmr_cancel_alarm,
	.set_top_value = mcux_qtmr_set_top_value,
	.get_pending_int = mcux_qtmr_get_pending_int,
	.get_top_value = mcux_qtmr_get_top_value,
	.get_freq = mcux_qtmr_get_freq,
};

#define TMR_DEVICE_INIT_MCUX(n)									\
	static struct mcux_qtmr_data mcux_qtmr_data_ ## n;					\
												\
	static const struct mcux_qtmr_config mcux_qtmr_config_ ## n = {				\
		.base = (void *)DT_REG_ADDR(DT_INST_PARENT(n)),					\
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(n))),			\
		.clock_subsys =									\
			(clock_control_subsys_t)DT_CLOCKS_CELL(DT_INST_PARENT(n), name),	\
		.info = {									\
			.max_top_value = UINT16_MAX,						\
			.freq = DT_INST_PROP_OR(n, freq, 0),					\
			.channels = 1,								\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,					\
		},										\
		.channel = DT_INST_PROP(n, channel),						\
		.qtmr_config = {								\
			.debugMode = kQTMR_RunNormalInDebug,					\
			.enableExternalForce = false,						\
			.enableMasterMode = false,						\
			.faultFilterCount = DT_INST_PROP_OR(n, filter_count, 0),		\
			.faultFilterPeriod = DT_INST_PROP_OR(n, filter_count, 0),		\
			.primarySource = DT_INST_ENUM_IDX(n, primary_source),			\
			.secondarySource = DT_INST_ENUM_IDX_OR(n, secondary_source, 0),		\
		},										\
		.mode = DT_INST_ENUM_IDX(n, mode),						\
	};											\
												\
	DEVICE_DT_INST_DEFINE(n,								\
			    mcux_qtmr_init,							\
			    NULL,								\
			    &mcux_qtmr_data_ ## n,						\
			    &mcux_qtmr_config_ ## n,						\
			    POST_KERNEL,							\
			    CONFIG_COUNTER_INIT_PRIORITY,					\
			    &mcux_qtmr_driver_api);						\

DT_INST_FOREACH_STATUS_OKAY(TMR_DEVICE_INIT_MCUX)
