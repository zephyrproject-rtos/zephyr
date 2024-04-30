/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MRT (Multirate timer) is a lightweight timer with multiple independent channels, each capable
 * of signalling the shared interrupt with a different period. This driver treats all the channels
 * as separate devices adhering to the counter API. The parent device is responsible for  the
 * initialization, interrupt handling, and any other module-wide tasks. The current implementation
 * of this driver prioritizes minimizing image size over speed, because it is not expected for the
 * functions to be called very often, and this IP is mostly present on low memory devices.
 */

#define DT_DRV_COMPAT nxp_mrt

#include <zephyr/drivers/counter.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/devicetree.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <soc.h>

#define LOG_MODULE_NAME counter_mrt
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_COUNTER_LOG_LEVEL);

/* Device holds a pointer to pointer to data */
#define MRT_CHANNEL_DATA(dev)	\
	(*(struct nxp_mrt_channel_data *const *const)dev->data)

/* Device config->data is an array of data pointers ordered by channel number,
 * dev->data is a pointer to one of these pointers in that array,
 * so the value of the dev->data - dev->config->data is the channel index
 */
#define MRT_CHANNEL_ID(dev)	\
	(((struct nxp_mrt_channel_data *const *)dev->data) - \
	((const struct nxp_mrt_config *)dev->config)->data)

/* Specific for each channel */
struct nxp_mrt_channel_data {
	uint32_t top;
	counter_top_callback_t cb;
	void *user_data;
};

/* Shared between all channels */
struct nxp_mrt_config {
	struct counter_config_info info;
	MRT_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	void (*irq_config_func)(const struct device *dev);
	struct nxp_mrt_channel_data *const *data;
	const struct device *const *channels;
};

static int nxp_mrt_stop(const struct device *dev)
{
	const struct nxp_mrt_config *config = dev->config;
	MRT_Type *base = config->base;
	int channel_id = MRT_CHANNEL_ID(dev);

	LOG_DBG("MRT@%p channel %d stopped", base, channel_id);
	LOG_WRN("MRT channel resets upon stopping");

	/* LOAD bit and 0 ivalue allows us to forcibly stop the timer */
	base->CHANNEL[channel_id].INTVAL = MRT_CHANNEL_INTVAL_LOAD(1);

	return 0;
}

static int nxp_mrt_start(const struct device *dev)
{
	const struct nxp_mrt_config *config = dev->config;
	MRT_Type *base = config->base;
	struct nxp_mrt_channel_data *data = MRT_CHANNEL_DATA(dev);
	int channel_id = MRT_CHANNEL_ID(dev);

	if (data->top <= 1) {
		/* Zephyr API says default should be max top value */
		LOG_INF("\"Started\" MRT@%p channel %d with default value %d",
				base, channel_id, config->info.max_top_value);
		data->top = config->info.max_top_value;
	}

	/* Start with previously configured top value (if already running this has no effect) */
	base->CHANNEL[channel_id].INTVAL = data->top;

	LOG_DBG("MRT@%p channel %d started with top value %d", base, channel_id, data->top);

	return 0;
}

static int nxp_mrt_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct nxp_mrt_config *config = dev->config;
	MRT_Type *base = config->base;
	int channel_id = MRT_CHANNEL_ID(dev);

	*ticks = base->CHANNEL[channel_id].TIMER & MRT_CHANNEL_TIMER_VALUE_MASK;

	return 0;
}


static int nxp_mrt_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct nxp_mrt_config *config = dev->config;
	MRT_Type *base = config->base;
	struct nxp_mrt_channel_data *data = MRT_CHANNEL_DATA(dev);
	int channel_id = MRT_CHANNEL_ID(dev);
	/* By default in Zephyr API, the counter resets on changing top value */
	bool reset = !(cfg->flags & COUNTER_TOP_CFG_DONT_RESET);
	bool active = base->CHANNEL[channel_id].STAT & MRT_CHANNEL_STAT_RUN_MASK;
	uint32_t current_val = base->CHANNEL[channel_id].TIMER & MRT_CHANNEL_TIMER_VALUE_MASK;
	int ret = 0;

	/* Store for use by counter_start */
	data->top = cfg->ticks;

	/* Used by ISR */
	data->cb = cfg->callback;
	data->user_data = cfg->user_data;


	/* If not yet started, wait for counter_start because setting reg value starts timer */
	if (!active) {
		LOG_DBG("Set MRT@%p channel %d top value to %d", base, channel_id, data->top);
		return ret;
	}

	/* Otherwise if currently running, need to check for lateness */
	if (cfg->ticks < current_val) {
		LOG_WRN("MRT@%p channel %d received requested top value %d which is "
			"smaller than current count %d",
			base, channel_id, cfg->ticks, current_val);
		/* Zephyr API says return this error in case of lateness
		 * when COUNTER_TOP_CFG_DONT_RESET is set but can still set period
		 */
		ret = reset ? 0 : -ETIME;
		/* If user said not to reset, they can also clarify exception for lateness */
		reset |= cfg->flags & COUNTER_TOP_CFG_RESET_WHEN_LATE;
	}

	/* Sets the top value. If we need to reset, LOAD bit does this */
	base->CHANNEL[channel_id].INTVAL = MRT_CHANNEL_INTVAL_IVALUE(cfg->ticks) |
					   MRT_CHANNEL_INTVAL_LOAD(reset ? 1 : 0);

	LOG_DBG("Changed MRT@%p channel %d top value while active to %d",
			base, channel_id,
			base->CHANNEL[channel_id].INTVAL & MRT_CHANNEL_INTVAL_IVALUE_MASK);

	return ret;
}

static uint32_t nxp_mrt_get_top_value(const struct device *dev)
{
	const struct nxp_mrt_config *config = dev->config;
	MRT_Type *base = config->base;
	int channel_id = MRT_CHANNEL_ID(dev);

	return base->CHANNEL[channel_id].INTVAL & MRT_CHANNEL_INTVAL_IVALUE_MASK;
}

static uint32_t nxp_mrt_get_pending_int(const struct device *dev)
{
	const struct nxp_mrt_config *config = dev->config;
	MRT_Type *base = config->base;
	int channel_id = MRT_CHANNEL_ID(dev);

	return base->CHANNEL[channel_id].STAT & MRT_CHANNEL_STAT_INTFLAG_MASK;
}

static inline int nxp_mrt_set_alarm(const struct device *dev,
				uint8_t chan_id,
				const struct counter_alarm_cfg *alarm_cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);
	ARG_UNUSED(alarm_cfg);

	LOG_ERR("MRT does not support alarms");
	return -ENOTSUP;
}

static inline int nxp_mrt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(chan_id);

	LOG_ERR("MRT does not support alarms");
	return -ENOTSUP;
}

uint32_t nxp_mrt_get_freq(const struct device *dev)
{
	const struct nxp_mrt_config *config = dev->config;
	uint32_t freq;

	clock_control_get_rate(config->clock_dev, config->clock_subsys, &freq);

	return freq;
}

static int nxp_mrt_init(const struct device *dev)
{
	const struct nxp_mrt_config *config = dev->config;
	MRT_Type *base = config->base;
	uint32_t num_channels = (base->MODCFG & MRT_MODCFG_NOC_MASK) >> MRT_MODCFG_NOC_SHIFT;

	clock_control_on(config->clock_dev, config->clock_subsys);

	config->irq_config_func(dev);

	/* Enable interrupts for all the channels that have devices */
	for (int i = 0; i < num_channels; i++) {
		if (config->channels[i]) {
			base->CHANNEL[i].CTRL = MRT_CHANNEL_CTRL_INTEN_MASK;
		}
	}

	return 0;
}

static void nxp_mrt_isr(const struct device *dev)
{
	const struct nxp_mrt_config *config = dev->config;
	MRT_Type *base = config->base;
	uint32_t irq_pends = base->IRQ_FLAG;
	uint32_t num_channels = (base->MODCFG & MRT_MODCFG_NOC_MASK) >> MRT_MODCFG_NOC_SHIFT;

	for (int i = 0; i < num_channels; i++) {
		/* Channel IRQ pending flags lowest order bits in IRQ_FLAG register */
		if (!(irq_pends & (0x1 << i))) {
			continue;
		}

		LOG_DBG("Handling interrupt for MRT%p channel %d", base, i);

		/* W1C interrupt flag */
		base->CHANNEL[i].STAT |= MRT_CHANNEL_STAT_INTFLAG_MASK;

		/* Channel devs & pointer path to channel cbs is in shared config */
		if (config->data[i]->cb) {
			config->data[i]->cb(config->channels[i], config->data[i]->user_data);
		}
	}
}

struct counter_driver_api nxp_mrt_api = {
	.get_value = nxp_mrt_get_value,
	.start = nxp_mrt_start,
	.stop = nxp_mrt_stop,
	.set_top_value = nxp_mrt_set_top_value,
	.get_top_value = nxp_mrt_get_top_value,
	.get_pending_int = nxp_mrt_get_pending_int,
	.set_alarm = nxp_mrt_set_alarm,
	.cancel_alarm = nxp_mrt_cancel_alarm,
	.get_freq = nxp_mrt_get_freq,
};

/* Creates a device for a channel (needed for counter API) */
#define NXP_MRT_CHANNEL_DEV_INIT(node, mrt_inst)				\
	DEVICE_DT_DEFINE(node, NULL, NULL,					\
			(void *)						\
			&nxp_mrt_##mrt_inst##_channel_datas[DT_REG_ADDR(node)],	\
			&nxp_mrt_##mrt_inst##_config,				\
			POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,		\
			&nxp_mrt_api);						\

/* Creates a data struct for a channel device */
#define NXP_MRT_CHANNEL_DATA_INIT(node)						\
	static struct nxp_mrt_channel_data					\
		nxp_mrt_channel_data_##node;					\

/* Initializes an element of the channel data pointer array */
#define NXP_MRT_CHANNEL_DATA_ARRAY_INIT(node)					\
	[DT_REG_ADDR(node)] =							\
		&nxp_mrt_channel_data_##node,

/* Initializes an element of the channel device pointer array */
#define NXP_MRT_CHANNEL_DEV_ARRAY_INIT(node)					\
	[DT_REG_ADDR(node)] = DEVICE_DT_GET(node),

#define NXP_MRT_INIT(n)								\
	/* ISR is shared between all channels */				\
	static void nxp_mrt_##n##_irq_config_func(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
				nxp_mrt_isr, DEVICE_DT_INST_GET(n), 0);		\
		irq_enable(DT_INST_IRQN(n));					\
	}									\
										\
	/* Initialize all the data structs for active channels */		\
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, NXP_MRT_CHANNEL_DATA_INIT)		\
										\
	/* Create an array of const pointers to the data structs */		\
	static struct nxp_mrt_channel_data *const nxp_mrt_##n##_channel_datas	\
			[DT_INST_PROP(n, num_channels)] = {			\
			DT_INST_FOREACH_CHILD_STATUS_OKAY(n,			\
				NXP_MRT_CHANNEL_DATA_ARRAY_INIT)		\
	};									\
										\
	/* Forward declaration */						\
	const static struct nxp_mrt_config nxp_mrt_##n##_config;		\
										\
	/* Create all the channel/counter devices */				\
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, NXP_MRT_CHANNEL_DEV_INIT, n)	\
										\
	/* This channel device array is needed by the module device ISR */	\
	const struct device *const nxp_mrt_##n##_channels			\
				[DT_INST_PROP(n, num_channels)] = {		\
			DT_INST_FOREACH_CHILD_STATUS_OKAY(n,			\
				NXP_MRT_CHANNEL_DEV_ARRAY_INIT)			\
	};									\
										\
	/* This config struct is shared by all the channels and parent device */\
	const static struct nxp_mrt_config nxp_mrt_##n##_config = {		\
		.info =	{							\
			.max_top_value =					\
			GENMASK(DT_INST_PROP(n, num_bits) - 1, 0),		\
			.channels = 0,						\
		},								\
		.base = (MRT_Type *)DT_INST_REG_ADDR(n),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),		\
		.clock_subsys = (clock_control_subsys_t)			\
				DT_INST_CLOCKS_CELL(n, name),			\
		.irq_config_func = nxp_mrt_##n##_irq_config_func,		\
		.data = nxp_mrt_##n##_channel_datas,				\
		.channels = nxp_mrt_##n##_channels,				\
	};									\
										\
	/* Init parent device in order to handle ISR and init. */		\
	DEVICE_DT_INST_DEFINE(n, &nxp_mrt_init, NULL, NULL,			\
				&nxp_mrt_##n##_config,				\
				POST_KERNEL,					\
				CONFIG_COUNTER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NXP_MRT_INIT)
