/*
 * Copyright 2020,2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pit

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_pit.h>

#define LOG_MODULE_NAME counter_pit
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_COUNTER_LOG_LEVEL);

/* Device holds a pointer to pointer to data */
#define PIT_CHANNEL_DATA(dev)					\
	(*(struct nxp_pit_channel_data *const *const)dev->data)

/* Device config->data is an array of data pointers ordered by channel number,
 * dev->data is a pointer to one of these pointers in that array,
 * so the value of the dev->data - dev->config->data is the channel index
 */
#define PIT_CHANNEL_ID(dev)					\
	(((struct nxp_pit_channel_data *const *)dev->data) -	\
	((const struct nxp_pit_config *)dev->config)->data)


struct nxp_pit_channel_data {
	uint32_t top;
	counter_top_callback_t top_callback;
	void *top_user_data;
};


struct nxp_pit_config {
	struct counter_config_info info;
	PIT_Type *base;
	bool enableRunInDebug;
	int num_channels;
#if DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pit), interrupts)
	void (*irq_config_func)(const struct device *dev);
#else
	void (**irq_config_func)(const struct device *dev);
#endif
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct nxp_pit_channel_data *const *data;
	const struct device *const *channels;
};

static uint32_t nxp_pit_get_top_value(const struct device *dev)
{
	const struct nxp_pit_config *config = dev->config;
	pit_chnl_t channel = PIT_CHANNEL_ID(dev);

	/*
	 * According to RM, the LDVAL trigger = clock ticks -1
	 * The underlying HAL driver function PIT_SetTimerPeriod()
	 * automatically subtracted 1 from the value that ends up in
	 * LDVAL so for reporting purposes we need to add it back in
	 * here to by consistent.
	 */
	return (config->base->CHANNEL[channel].LDVAL + 1);
}

static int nxp_pit_start(const struct device *dev)
{
	const struct nxp_pit_config *config = dev->config;
	int channel_id = PIT_CHANNEL_ID(dev);

	LOG_DBG("period is %d", nxp_pit_get_top_value(dev));
	PIT_EnableInterrupts(config->base, channel_id,
			     kPIT_TimerInterruptEnable);
	PIT_StartTimer(config->base, channel_id);
	return 0;
}

static int nxp_pit_stop(const struct device *dev)
{
	const struct nxp_pit_config *config = dev->config;
	int channel_id = PIT_CHANNEL_ID(dev);

	PIT_DisableInterrupts(config->base, channel_id,
			      kPIT_TimerInterruptEnable);
	PIT_StopTimer(config->base, channel_id);

	return 0;
}

static int nxp_pit_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct nxp_pit_config *config = dev->config;
	int channel_id = PIT_CHANNEL_ID(dev);

	*ticks = PIT_GetCurrentTimerCount(config->base, channel_id);

	return 0;
}

static int nxp_pit_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct nxp_pit_config *config = dev->config;
	struct nxp_pit_channel_data *data = PIT_CHANNEL_DATA(dev);
	pit_chnl_t channel = PIT_CHANNEL_ID(dev);

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

static uint32_t nxp_pit_get_pending_int(const struct device *dev)
{
	const struct nxp_pit_config *config = dev->config;
	uint32_t mask = PIT_TFLG_TIF_MASK;
	uint32_t flags;
	int channel_id = PIT_CHANNEL_ID(dev);

	flags = PIT_GetStatusFlags(config->base, channel_id);

	return ((flags & mask) == mask);
}

static uint32_t nxp_pit_get_frequency(const struct device *dev)
{
	const struct nxp_pit_config *config = dev->config;
	uint32_t clock_rate;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_rate)) {
		LOG_ERR("Failed to get clock rate");
		return 0;
	}

	return clock_rate;
}

#if DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pit), interrupts)
static void nxp_pit_isr(const struct device *dev)
{
	const struct nxp_pit_config *config = dev->config;
	uint32_t flags;

	LOG_DBG("pit counter isr");

	for (int channel_index = 0;
			channel_index < config->num_channels;
			channel_index++) {
		flags = PIT_GetStatusFlags(config->base, channel_index);
		if (flags) {
			struct nxp_pit_channel_data *data =
				PIT_CHANNEL_DATA(config->channels[channel_index]);
			PIT_ClearStatusFlags(config->base, channel_index, flags);
			data->top_callback(dev, data->top_user_data);
		}
	}
}
#else
static void nxp_pit_isr(const struct device *dev)
{
	const struct nxp_pit_config *config = dev->config;
	struct nxp_pit_channel_data *data = PIT_CHANNEL_DATA(dev);
	pit_chnl_t channel = PIT_CHANNEL_ID(dev);
	uint32_t flags;

	LOG_DBG("pit counter isr");

	flags = PIT_GetStatusFlags(config->base, channel);
	if (flags) {
		PIT_ClearStatusFlags(config->base, channel, flags);
		if (data->top_callback) {
			data->top_callback(dev, data->top_user_data);
		}
	}
}
#endif /* DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pit), interrupts) */

static int nxp_pit_init(const struct device *dev)
{
	const struct nxp_pit_config *config = dev->config;
	pit_config_t pit_config;
	uint32_t clock_rate;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	PIT_GetDefaultConfig(&pit_config);
	pit_config.enableRunInDebug = config->enableRunInDebug;

	PIT_Init(config->base, &pit_config);

	clock_rate = nxp_pit_get_frequency(dev);

#if DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pit), interrupts)
	config->irq_config_func(dev);
	for (int channel_index = 0;
		channel_index < config->num_channels;
		channel_index++) {
		PIT_SetTimerPeriod(config->base, channel_index,
			USEC_TO_COUNT(config->info.max_top_value, clock_rate));
	}
#else
	for (int channel_index = 0;
		channel_index < config->num_channels;
		channel_index++) {
		if (config->irq_config_func[channel_index]) {
			config->irq_config_func[channel_index](dev);
			PIT_SetTimerPeriod(config->base, channel_index,
				USEC_TO_COUNT(config->info.max_top_value, clock_rate));
		}
	}
#endif /* DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pit), interrupts) */
	return 0;
}

static const struct counter_driver_api nxp_pit_driver_api = {
	.start = nxp_pit_start,
	.stop = nxp_pit_stop,
	.get_value = nxp_pit_get_value,
	.set_top_value = nxp_pit_set_top_value,
	.get_pending_int = nxp_pit_get_pending_int,
	.get_top_value = nxp_pit_get_top_value,
	.get_freq = nxp_pit_get_frequency,
};


/* Creates a device for a channel (needed for counter API) */
#define NXP_PIT_CHANNEL_DEV_INIT(node, pit_inst)						\
	DEVICE_DT_DEFINE(node, NULL, NULL,							\
		(void *)									\
		&nxp_pit_##pit_inst##_channel_datas[DT_REG_ADDR(node)],				\
		&nxp_pit_##pit_inst##_config,							\
		POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,					\
		&nxp_pit_driver_api);

/* Creates a decleration for each pit channel */
#define NXP_PIT_CHANNEL_DECLARATIONS(node)  static struct nxp_pit_channel_data			\
	nxp_pit_channel_data_##node;

/* Initializes an element of the channel data pointer array */
#define NXP_PIT_INSERT_CHANNEL_INTO_ARRAY(node)							\
	[DT_REG_ADDR(node)] =									\
		&nxp_pit_channel_data_##node,

#define NXP_PIT_INSERT_CHANNEL_DEVICE_INTO_ARRAY(node)						\
	[DT_REG_ADDR(node)] = DEVICE_DT_GET(node),


#if DT_NODE_HAS_PROP(DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pit), interrupts)
#define NXP_PIT_IRQ_CONFIG_DECLARATIONS(n)							\
	static void nxp_pit_irq_config_func_##n(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq),					\
			DT_INST_IRQ_BY_IDX(n, 0, priority),					\
			nxp_pit_isr,								\
			DEVICE_DT_INST_GET(n), 0);						\
		irq_enable(DT_INST_IRQN(n));							\
	};

#define NXP_PIT_SETUP_IRQ_CONFIG(n) NXP_PIT_IRQ_CONFIG_DECLARATIONS(n);
#define NXP_PIT_SETUP_IRQ_ARRAY(ignored)

#else
#define NXP_PIT_IRQ_CONFIG_DECLARATIONS(n)							\
	static void nxp_pit_irq_config_func_##n(const struct device *dev)			\
	{											\
		IRQ_CONNECT(DT_IRQN(n),								\
			DT_IRQ(n, priority),							\
			nxp_pit_isr,								\
			DEVICE_DT_GET(n), 0);							\
		irq_enable(DT_IRQN(n));								\
	};

#define NXP_PIT_SETUP_IRQ_CONFIG(n)								\
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, NXP_PIT_IRQ_CONFIG_DECLARATIONS);

#define NXP_PIT_INSERT_IRQ_CONFIG_INTO_ARRAY(n)							\
	[DT_REG_ADDR(n)] = &nxp_pit_irq_config_func_##n,

#define NXP_PIT_SETUP_IRQ_ARRAY(n)								\
	/* Create Array of IRQs -> 1 irq func per channel */					\
	void (*nxp_pit_irq_config_array[DT_INST_FOREACH_CHILD_SEP_VARGS(n,			\
					DT_NODE_HAS_COMPAT, (+), nxp_pit_channel)])		\
				(const struct device *dev) = {					\
					DT_INST_FOREACH_CHILD_STATUS_OKAY(n,			\
					NXP_PIT_INSERT_IRQ_CONFIG_INTO_ARRAY)			\
	};
#endif

#define COUNTER_NXP_PIT_DEVICE_INIT(n)								\
												\
	/* Setup the IRQ either for parent irq or per channel irq */				\
	NXP_PIT_SETUP_IRQ_CONFIG(n)								\
												\
	/* Create channel declarations */							\
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n,							\
		NXP_PIT_CHANNEL_DECLARATIONS)							\
												\
	/* Array of channel devices */								\
	static struct nxp_pit_channel_data *const						\
		nxp_pit_##n##_channel_datas							\
			[DT_INST_FOREACH_CHILD_SEP_VARGS(					\
				n, DT_NODE_HAS_COMPAT, (+), nxp_pit_channel)] = {		\
				DT_INST_FOREACH_CHILD_STATUS_OKAY(n,				\
					NXP_PIT_INSERT_CHANNEL_INTO_ARRAY)			\
	};											\
												\
	/* forward declaration */								\
	static const struct nxp_pit_config nxp_pit_##n##_config;				\
												\
	/* Create all the channel/counter devices */						\
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n,						\
		NXP_PIT_CHANNEL_DEV_INIT, n)							\
												\
	/* This channel device array is needed by the module device ISR */			\
	const struct device *const nxp_pit_##n##_channels					\
				[DT_INST_FOREACH_CHILD_SEP_VARGS(				\
					n, DT_NODE_HAS_COMPAT, (+), nxp_pit_channel)] = {	\
				DT_INST_FOREACH_CHILD_STATUS_OKAY(n,				\
				NXP_PIT_INSERT_CHANNEL_DEVICE_INTO_ARRAY)			\
	};											\
												\
												\
	NXP_PIT_SETUP_IRQ_ARRAY(n)								\
												\
	/* This config struct is shared by all the channels and parent device */		\
	static const struct nxp_pit_config nxp_pit_##n##_config = {				\
		.info = {									\
			.max_top_value =							\
				DT_INST_PROP(n, max_load_value),				\
			.channels = 0,								\
		},										\
		.base = (PIT_Type *)DT_INST_REG_ADDR(n),					\
		.irq_config_func = COND_CODE_1(DT_NODE_HAS_PROP(				\
					DT_COMPAT_GET_ANY_STATUS_OKAY(nxp_pit), interrupts),	\
					(nxp_pit_irq_config_func_##n),				\
					(&nxp_pit_irq_config_array[0])),			\
		.num_channels = DT_INST_FOREACH_CHILD_SEP_VARGS(				\
			n, DT_NODE_HAS_COMPAT, (+), nxp_pit_channel),				\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),				\
		.clock_subsys = (clock_control_subsys_t)					\
				DT_INST_CLOCKS_CELL(n, name),					\
		.data = nxp_pit_##n##_channel_datas,						\
		.channels = nxp_pit_##n##_channels,						\
	};											\
												\
	/* Init parent device in order to handle ISR and init. */				\
	DEVICE_DT_INST_DEFINE(n, &nxp_pit_init, NULL,						\
			NULL, &nxp_pit_##n##_config, POST_KERNEL,				\
			CONFIG_COUNTER_INIT_PRIORITY, NULL);


DT_INST_FOREACH_STATUS_OKAY(COUNTER_NXP_PIT_DEVICE_INIT)
