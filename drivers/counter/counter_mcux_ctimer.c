/*
 * Copyright (c) 2021, Toby Firth.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT nxp_lpc_ctimer

#include <zephyr/drivers/counter/counter_mcux_ctimer.h>
#include <fsl_ctimer.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/mcux_lpc_syscon_clock.h>
#include <fsl_inputmux.h>
#if defined(CONFIG_COUNTER_MCUX_CTIMER_CAPTURE)
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/devicetree/pinctrl.h>
#endif

LOG_MODULE_REGISTER(mcux_ctimer, CONFIG_COUNTER_LOG_LEVEL);

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
/* One of the CTimer channels is reserved to implement set_top_value API */
#define NUM_CHANNELS 3
#else
#define NUM_CHANNELS 4
#endif

struct mcux_lpc_ctimer_channel_data {
	counter_alarm_callback_t alarm_callback;
	void *alarm_user_data;
};

#if defined(CONFIG_COUNTER_MCUX_CTIMER_DMA)
#define NUM_DMA_CHANNELS 2
struct mcux_lpc_ctimer_dma_stream {
	const struct device *dma_dev;
	uint32_t dma_channel; /* stores the channel for dma */
	struct dma_config dma_cfg;
	struct dma_block_config dma_blk_cfg[CONFIG_DMA_LINK_QUEUE_SIZE];
	struct mcux_counter_dma_cfg priv_cfg;
	counter_dma_callback_t counter_dma_callback;
	void *counter_dma_user_data;
	const struct device *timer_dev;
};
#endif

struct mcux_lpc_ctimer_data {
	struct mcux_lpc_ctimer_channel_data channels[NUM_CHANNELS];
	counter_top_callback_t top_callback;
	void *top_user_data;
#if defined(CONFIG_COUNTER_MCUX_CTIMER_DMA)
	struct mcux_lpc_ctimer_dma_stream stream[NUM_DMA_CHANNELS];
#endif
};

struct mcux_lpc_ctimer_config {
	struct counter_config_info info;
	CTIMER_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	ctimer_timer_mode_t mode;
	ctimer_capture_channel_t input;
	uint32_t prescale;
	uint8_t timer_no;
	void (*irq_config_func)(const struct device *dev);

#if defined(CONFIG_COUNTER_MCUX_CTIMER_CAPTURE)
	uint8_t cap_pin;;
	uint8_t cap_edge;
	const struct pinctrl_dev_config *pincfg;
#endif
};

static int mcux_lpc_ctimer_start(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

	CTIMER_StartTimer(config->base);

	return 0;
}

static int mcux_lpc_ctimer_stop(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

	CTIMER_StopTimer(config->base);

	return 0;
}

static uint32_t mcux_lpc_ctimer_read(CTIMER_Type *base)
{
	return CTIMER_GetTimerCountValue(base);
}

static int mcux_lpc_ctimer_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	*ticks = mcux_lpc_ctimer_read(config->base);
	return 0;
}

static uint32_t mcux_lpc_ctimer_get_top_value(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	CTIMER_Type *base = config->base;

	/* Return the top value if it has been set, else return the max top value */
	if (base->MR[NUM_CHANNELS] != 0) {
		return base->MR[NUM_CHANNELS];
	} else {
		return config->info.max_top_value;
	}
#else
	return config->info.max_top_value;
#endif
}

static int mcux_lpc_ctimer_set_alarm(const struct device *dev, uint8_t chan_id,
				     const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	uint32_t ticks = alarm_cfg->ticks;
	uint32_t current = mcux_lpc_ctimer_read(config->base);
	uint32_t top = mcux_lpc_ctimer_get_top_value(dev);
	bool enable_stop = false;
	bool enable_reset = false;
	bool enable_interrupt = true;

	if (alarm_cfg->ticks > top) {
		return -EINVAL;
	}

	if (data->channels[chan_id].alarm_callback != NULL) {
		LOG_ERR("match channel already in use");
		return -EBUSY;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
		if (ticks > top) {
			ticks %= top;
		}
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_AUTO_STOP)) {
		enable_stop = true;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_AUTO_RESET)) {
		enable_reset = true;
	}

	if (alarm_cfg->callback == NULL) {
		enable_interrupt = false;
	}

	data->channels[chan_id].alarm_callback = alarm_cfg->callback;
	data->channels[chan_id].alarm_user_data = alarm_cfg->user_data;

	ctimer_match_config_t match_config = { .matchValue = ticks,
					       .enableCounterReset = enable_reset,
					       .enableCounterStop = enable_stop,
					       .outControl = kCTIMER_Output_NoAction,
					       .outPinInitState = false,
					       .enableInterrupt = enable_interrupt };

	CTIMER_SetupMatch(config->base, chan_id, &match_config);

	return 0;
}

static int mcux_lpc_ctimer_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;

	CTIMER_DisableInterrupts(config->base, (1 << chan_id));

	data->channels[chan_id].alarm_callback = NULL;
	data->channels[chan_id].alarm_user_data = NULL;

	return 0;
}

static int mcux_lpc_ctimer_set_top_value(const struct device *dev,
					 const struct counter_top_cfg *cfg)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;

#ifndef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	/* Only allow max value when we do not reserve a ctimer channel for setting top value */
	if (cfg->ticks != config->info.max_top_value) {
		LOG_ERR("Wrap can only be set to 0x%x",
			config->info.max_top_value);
		return -ENOTSUP;
	}
#endif

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		CTIMER_Reset(config->base);
	} else if (mcux_lpc_ctimer_read(config->base) >= cfg->ticks) {
		if (cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE) {
			CTIMER_Reset(config->base);
		}
		return -ETIME;
	}

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	ctimer_match_config_t match_config = { .matchValue = cfg->ticks,
					       .enableCounterReset = true,
					       .enableCounterStop = false,
					       .outControl = kCTIMER_Output_NoAction,
					       .outPinInitState = false,
					       .enableInterrupt = true };

	CTIMER_SetupMatch(config->base, NUM_CHANNELS, &match_config);
#endif

	return 0;
}

static uint32_t mcux_lpc_ctimer_get_pending_int(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

	return (CTIMER_GetStatusFlags(config->base) & 0xF) != 0;
}

static uint32_t mcux_lpc_ctimer_get_freq(const struct device *dev)
{
	/*
	 * The frequency of the timer is not known at compile time so we need to
	 * calculate at runtime when the frequency is known.
	 */
	const struct mcux_lpc_ctimer_config *config = dev->config;

	uint32_t clk_freq = 0;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
					&clk_freq)) {
		LOG_ERR("unable to get clock frequency");
		return 0;
	}

	/* prescale increments when the prescale counter is 0 so if prescale is 1
	 * the counter is incremented every 2 cycles of the clock so will actually
	 * divide by 2 hence the addition of 1 to the value here.
	 */
	return (clk_freq / (config->prescale + 1));
}

#if defined(CONFIG_COUNTER_MCUX_CTIMER_CAPTURE)
/* This function is executed in the interrupt context */
static void mcux_lpc_ctimer_dma_callback(const struct device *dev, void *arg,
			 uint32_t dma_channel, int status)
{
	/* arg directly holds the ctimer device */
	struct mcux_lpc_ctimer_data *data = arg;

	if (status != 0) {
		LOG_ERR("DMA callback error with channel %d.", dma_channel);
	} else {
		for (uint8_t i = 0; i < NUM_DMA_CHANNELS; i++) {
			if (dma_channel == data->stream[i].dma_channel) {
				if (data->stream[i].counter_dma_callback) {
					data->stream[i].counter_dma_callback(data->stream[i].timer_dev, data->stream[i].counter_dma_user_data,
							i, status);
				}
				break;
			}
		}
	}
}

static int mcux_lpc_ctimer_set_dma_cfg(const struct device *dev, uint8_t chan_id,
				     struct counter_dma_cfg *counter_dma_cfg)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	bool src_addr_wrap = false;
	bool dest_addr_wrap = false;

	if (chan_id > NUM_DMA_CHANNELS) {
		return -EINVAL;
	}
	const struct device *dma = data->stream[chan_id].dma_dev;
	if (dma == NULL) {
		LOG_ERR("Only match channel 0 ~ 1 support DMA");
		return -EINVAL;
	}
	/* DMA0 input trigger configure
	 * Note: only support DMA0
	 */
	inputmux_connection_t conn = kINPUTMUX_Ctimer0M0ToDma0 +
						config->timer_no * NUM_DMA_CHANNELS + chan_id;
	INPUTMUX_AttachSignal(INPUTMUX, data->stream[chan_id].dma_channel, conn);

	inputmux_signal_t signal = kINPUTMUX_Dmac0InputTriggerCtimer0M0Ena + 
						config->timer_no * NUM_DMA_CHANNELS + chan_id;
	INPUTMUX_EnableSignal(INPUTMUX, signal, true);
	// INPUTMUX->DMAC0_ITRIG_ENA0_SET = (1 << 
	// 			(4 + config->mux_config.timer_no * NUM_DMA_CHANNELS + chan_id));

	/* preprare DMA configuration structure */
	struct dma_config *dma_cfg = &data->stream[chan_id].dma_cfg;
	memset(dma_cfg, 0, sizeof(struct dma_config ));

	dma_cfg->channel_direction = counter_dma_cfg->channel_direction;
	dma_cfg->channel_priority = counter_dma_cfg->channel_priority;
	dma_cfg->source_data_size = counter_dma_cfg->source_data_size;
	dma_cfg->dest_data_size = counter_dma_cfg->dest_data_size;
	dma_cfg->source_burst_length = counter_dma_cfg->source_burst_length;
	dma_cfg->dest_burst_length = counter_dma_cfg->dest_burst_length;
	if (counter_dma_cfg->callback) {
		dma_cfg->dma_callback = mcux_lpc_ctimer_dma_callback;
		dma_cfg->user_data = (void *)dev->data;
	}
	dma_cfg->priv_dma_config = NULL;

	if (counter_dma_cfg->priv_config) {
		struct mcux_counter_dma_cfg *priv_dma_cfg = &data->stream[chan_id].priv_cfg;
		struct mcux_counter_dma_cfg *mcux_counter_dma_cfg = (struct mcux_counter_dma_cfg *)counter_dma_cfg->priv_config;

		memcpy(priv_dma_cfg, mcux_counter_dma_cfg, sizeof(struct mcux_counter_dma_cfg));

		dma_cfg->priv_dma_config = priv_dma_cfg;

		src_addr_wrap = (priv_dma_cfg->mcux_dma_cfg.channel_trigger.wrap & DMA_CHANNEL_CFG_SRCBURSTWRAP_MASK) ? true : false;
		dest_addr_wrap = (priv_dma_cfg->mcux_dma_cfg.channel_trigger.wrap & DMA_CHANNEL_CFG_DSTBURSTWRAP_MASK) ? true : false;
	}

	// total bytes / dest_data_size
	uint32_t dest_data_num = counter_dma_cfg->length / counter_dma_cfg->dest_data_size;

	dma_cfg->block_count = (dest_data_num & 0x3FF) ? 
			((dest_data_num >> 10) + 1) : 
			(dest_data_num >> 10);

	if (dma_cfg->block_count > CONFIG_DMA_LINK_QUEUE_SIZE) {
		LOG_ERR("please config DMA_LINK_QUEUE_SIZE as %d",
				dma_cfg->block_count);
		return -EINVAL;
	}

	memset(&data->stream[chan_id].dma_blk_cfg[0], 0, sizeof(struct dma_block_config) * CONFIG_DMA_LINK_QUEUE_SIZE);
	struct dma_block_config *dma_blk_cfg = &data->stream[chan_id].dma_blk_cfg[0];

	dma_cfg->head_block = dma_blk_cfg;
	if (dma_cfg->block_count > 1) {
		dma_blk_cfg->source_gather_en = 1;
	}

	for (uint8_t i = 0; i < dma_cfg->block_count; i++) {
		dma_blk_cfg->block_size = ((dest_data_num - i * DMA_MAX_TRANS_NUM) > DMA_MAX_TRANS_NUM) ?
						counter_dma_cfg->dest_data_size * DMA_MAX_TRANS_NUM :
						counter_dma_cfg->dest_data_size * (dest_data_num - i * DMA_MAX_TRANS_NUM);
		dma_blk_cfg->source_addr_adj = counter_dma_cfg->source_addr_adj;
		dma_blk_cfg->dest_addr_adj = counter_dma_cfg->dest_addr_adj;

		if (counter_dma_cfg->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE || src_addr_wrap) {
			dma_blk_cfg->source_address = counter_dma_cfg->src_addr;
		} else {
			dma_blk_cfg->source_address = counter_dma_cfg->src_addr + (i * DMA_MAX_TRANS_NUM) * counter_dma_cfg->source_data_size;
		}

		if (counter_dma_cfg->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE || dest_addr_wrap) {
			dma_blk_cfg->dest_address = counter_dma_cfg->dest_addr;
		} else {
			dma_blk_cfg->dest_address = counter_dma_cfg->dest_addr + (i * DMA_MAX_TRANS_NUM) * counter_dma_cfg->dest_data_size;
		}

		dma_blk_cfg->next_block = dma_blk_cfg + 1;
		dma_blk_cfg = dma_blk_cfg->next_block;
	}

	data->stream[chan_id].counter_dma_callback = counter_dma_cfg->callback;
	data->stream[chan_id].counter_dma_user_data = counter_dma_cfg->user_data;
	data->stream[chan_id].timer_dev = dev;

	mcux_lpc_ctimer_start(dev);
	return dma_config(dma, data->stream[chan_id].dma_channel, dma_cfg);
}
#endif

static int mcux_lpc_ctimer_reset(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;

	CTIMER_Reset(config->base);

	return 0;
}

static void mcux_lpc_ctimer_isr(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;

	uint32_t interrupt_stat = CTIMER_GetStatusFlags(config->base);

	CTIMER_ClearStatusFlags(config->base, interrupt_stat);

	uint32_t ticks = mcux_lpc_ctimer_read(config->base);

	for (uint8_t chan = 0; chan < NUM_CHANNELS; chan++) {
		uint8_t channel_mask = 0x01 << chan;

		if (((interrupt_stat & channel_mask) != 0) &&
		    (data->channels[chan].alarm_callback != NULL)) {
			counter_alarm_callback_t alarm_callback =
				data->channels[chan].alarm_callback;
			void *alarm_user_data = data->channels[chan].alarm_user_data;

			data->channels[chan].alarm_callback = NULL;
			data->channels[chan].alarm_user_data = NULL;
			alarm_callback(dev, chan, ticks, alarm_user_data);
		}
	}

#ifdef CONFIG_COUNTER_MCUX_CTIMER_RESERVE_CHANNEL_FOR_SETTOP
	if (((interrupt_stat & (0x01 << NUM_CHANNELS)) != 0) && data->top_callback) {
		data->top_callback(dev, data->top_user_data);
	}
#endif
}

static int mcux_lpc_ctimer_init(const struct device *dev)
{
	const struct mcux_lpc_ctimer_config *config = dev->config;
	struct mcux_lpc_ctimer_data *data = dev->data;
	ctimer_config_t ctimer_config;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	for (uint8_t chan = 0; chan < NUM_CHANNELS; chan++) {
		data->channels[chan].alarm_callback = NULL;
		data->channels[chan].alarm_user_data = NULL;
	}

	CTIMER_GetDefaultConfig(&ctimer_config);

	ctimer_config.mode = config->mode;
	ctimer_config.input = config->input;
	ctimer_config.prescale = config->prescale;

	/* clear all interrupt flags */
	CTIMER_ClearStatusFlags(config->base, 0xFF);

	CTIMER_Init(config->base, &ctimer_config);

#if defined(CONFIG_COUNTER_MCUX_CTIMER_CAPTURE)
	if (config->pincfg) {
		int err = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
		if (err) {
			return err;
		}
		/* INPUTMUX init in DMA */
		INPUTMUX->CT32BIT_CAP_SEL[config->timer_no][config->input] = 
					config->cap_pin;
		CTIMER_SetupCapture(config->base, config->input, config->cap_edge, false);
		LOG_DBG("\ntimer %d capture config input %d cap_edge %d\n", config->timer_no, config->input, config->cap_edge);
	}
#endif

	config->irq_config_func(dev);

	return 0;
}

static const struct counter_driver_api mcux_ctimer_driver_api = {
	.start = mcux_lpc_ctimer_start,
	.stop = mcux_lpc_ctimer_stop,
	.get_value = mcux_lpc_ctimer_get_value,
	.set_alarm = mcux_lpc_ctimer_set_alarm,
	.cancel_alarm = mcux_lpc_ctimer_cancel_alarm,
	.set_top_value = mcux_lpc_ctimer_set_top_value,
	.get_pending_int = mcux_lpc_ctimer_get_pending_int,
	.get_top_value = mcux_lpc_ctimer_get_top_value,
	.get_freq = mcux_lpc_ctimer_get_freq,
#if defined(CONFIG_COUNTER_MCUX_CTIMER_DMA)
	.set_dma_cfg = mcux_lpc_ctimer_set_dma_cfg,
#endif
	.reset = mcux_lpc_ctimer_reset,
};

#define PINCTRL_DEFINE(id)				\
		COND_CODE_1(DT_INST_NUM_PINCTRL_STATES(id), 		\
			(PINCTRL_DT_INST_DEFINE(id)),	\
			())

#define PINCTRL_CONFIG(id)				\
		COND_CODE_1(DT_INST_NUM_PINCTRL_STATES(id), 		\
			(PINCTRL_DT_INST_DEV_CONFIG_GET(id)),	\
			(NULL))

#if defined(CONFIG_COUNTER_MCUX_CTIMER_CAPTURE)
#define CTIMER_CAPTURE_PINCTRL_DEFINE(id) PINCTRL_DEFINE(id);
#define CTIMER_CAPTURE_PINCTRL_INIT(id) .pincfg = PINCTRL_CONFIG(id),
#define CTIMER_CAPTURE_CONFIG_INIT(id) 		\
		.cap_pin = DT_INST_PROP_OR(id, capture_pin, 0),	\
		.cap_edge = DT_INST_PROP_OR(id, capture_edge, 1),	\
		CTIMER_CAPTURE_PINCTRL_INIT(id)			
#else
#define CTIMER_CAPTURE_PINCTRL_DEFINE(id)
#define CTIMER_CAPTURE_PINCTRL_INIT(id)
#define CTIMER_CAPTURE_CONFIG_INIT(id)
#endif

#if defined(CONFIG_COUNTER_MCUX_CTIMER_DMA)
#define DMA_CHANNEL_DATA(id, name)					\
	{						\
		.dma_dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(id, name)), \
		.dma_channel =						\
			DT_INST_DMAS_CELL_BY_NAME(id, name, channel),		\
		.dma_cfg = {					\
				.dma_callback = mcux_lpc_ctimer_dma_callback,	\
			}								\
	},

#define DMA_CHANNEL(id, name) 				\
	COND_CODE_1(DT_INST_DMAS_HAS_NAME(id, name), 	\
			(DMA_CHANNEL_DATA(id, name)),	\
			({NULL},))

#define TIMER_DMA_CHANNELS(id)				\
	.stream = {										\
		DMA_CHANNEL(id, m0)					\
		DMA_CHANNEL(id, m1)					\
	},
#else
#define TIMER_DMA_CHANNELS(id)
#endif

#define COUNTER_LPC_CTIMER_DEVICE(id)                                                          \
	CTIMER_CAPTURE_PINCTRL_DEFINE(id)														   \
	static void mcux_lpc_ctimer_irq_config_##id(const struct device *dev);                     \
	static struct mcux_lpc_ctimer_config mcux_lpc_ctimer_config_##id = { \
		.info = {						\
			.max_top_value = UINT32_MAX,			\
			.flags = COUNTER_CONFIG_INFO_COUNT_UP,		\
			.channels = NUM_CHANNELS,					\
		},\
		.base = (CTIMER_Type *)DT_INST_REG_ADDR(id),		\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(id)),	\
		.clock_subsys =				\
		(clock_control_subsys_t)(DT_INST_CLOCKS_CELL(id, name) + MCUX_CTIMER_CLK_OFFSET),\
		.mode = DT_INST_PROP(id, mode),						\
		.input = DT_INST_PROP(id, input),					\
		.prescale = DT_INST_PROP(id, prescale),				\
		.timer_no = DT_INST_PROP(id, timer_no),				\
		.irq_config_func = mcux_lpc_ctimer_irq_config_##id,	\
		CTIMER_CAPTURE_CONFIG_INIT(id)						\
	};                     \
	static struct mcux_lpc_ctimer_data mcux_lpc_ctimer_data_##id = {					\
		TIMER_DMA_CHANNELS(id)		\
	};                              \
	DEVICE_DT_INST_DEFINE(id, &mcux_lpc_ctimer_init, NULL, &mcux_lpc_ctimer_data_##id,         \
			      &mcux_lpc_ctimer_config_##id, POST_KERNEL,                           \
			      CONFIG_COUNTER_INIT_PRIORITY, &mcux_ctimer_driver_api);              \
	static void mcux_lpc_ctimer_irq_config_##id(const struct device *dev)                      \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(id), DT_INST_IRQ(id, priority), mcux_lpc_ctimer_isr,      \
			    DEVICE_DT_INST_GET(id), 0);                                            \
		irq_enable(DT_INST_IRQN(id));                                                      \
	}

DT_INST_FOREACH_STATUS_OKAY(COUNTER_LPC_CTIMER_DEVICE)
