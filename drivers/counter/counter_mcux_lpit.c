/*
 * Copyright 2023, 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpit

#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>
#include <fsl_lpit.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(counter_lpit, CONFIG_COUNTER_LOG_LEVEL);

/* Device holds a pointer to pointer to data */
#define LPIT_CHANNEL_DATA(dev) (*(struct mcux_lpit_channel_data *const *const)dev->data)

/* dev->config->data is an array of data pointers ordered by channel number,
 * dev->data is a pointer to one of these pointers in that array,
 * so the value of the dev->data - dev->config->data is the channel index
 */
#define LPIT_CHANNEL_ID(dev)                                                                       \
	(((struct mcux_lpit_channel_data *const *)dev->data) -                                     \
	 ((const struct mcux_lpit_config *)dev->config)->data)

struct mcux_lpit_channel_data {
	uint32_t top;                        /* Top value of the counter */
	counter_top_callback_t top_callback; /* Callback when counter turns around */
	void *top_user_data;                 /* User data for top callback */
};

struct mcux_lpit_config {
	struct counter_config_info info;
	LPIT_Type *base;           /* Poniter to LPIT peripheral instance base address*/
	lpit_config_t lpit_config; /* LPIT configuration structure */
	int num_channels;          /* Number of channels for one LPIT instance*/
	void (*irq_config_func)(const struct device *dev);
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	/* Point to array of channels data for this LPIT instance */
	struct mcux_lpit_channel_data *const *data;
	/* Point to array of channel devices for this LPIT instance */
	const struct device *const *channels;
};

static uint32_t mcux_lpit_get_top_value(const struct device *dev)
{
	const struct mcux_lpit_config *config = dev->config;
	uint8_t channel_id = LPIT_CHANNEL_ID(dev);

	/*
	 * The underlying HAL driver function LPIT_SetTimerPeriod()
	 * automatically subtracted 1 from the value that ends up in
	 * TVAL so for reporting purposes we need to add it back in
	 * here to by consistent.
	 */
	return (config->base->CHANNEL[channel_id].TVAL + 1U);
}

static int mcux_lpit_start(const struct device *dev)
{
	const struct mcux_lpit_config *config = dev->config;
	uint8_t channel_id = LPIT_CHANNEL_ID(dev);

	LOG_DBG("period is %d", mcux_lpit_get_top_value(dev));
	LPIT_EnableInterrupts(config->base, BIT(channel_id));
	LPIT_StartTimer(config->base, channel_id);
	return 0;
}

static int mcux_lpit_stop(const struct device *dev)
{
	const struct mcux_lpit_config *config = dev->config;
	uint8_t channel_id = LPIT_CHANNEL_ID(dev);

	LPIT_DisableInterrupts(config->base, BIT(channel_id));
	LPIT_StopTimer(config->base, channel_id);
	return 0;
}

static int mcux_lpit_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_lpit_config *config = dev->config;
	uint8_t channel_id = LPIT_CHANNEL_ID(dev);

	*ticks = LPIT_GetCurrentTimerCount(config->base, channel_id);

	return 0;
}

static int mcux_lpit_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct mcux_lpit_config *config = dev->config;
	struct mcux_lpit_channel_data *data = LPIT_CHANNEL_DATA(dev);
	uint8_t channel_id = LPIT_CHANNEL_ID(dev);

	/* Note: The underlying HAL driver function LPIT_SetTimerPeriod() assert(ticks > 2U) */
	if (cfg->ticks < 2U || cfg->ticks > config->info.max_top_value) {
		return -EINVAL;
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	if ((config->base->CHANNEL[channel_id].TCTRL & LPIT_TCTRL_T_EN_MASK) != 0) {
		/* Timer already enabled, check flags before resetting */
		if ((cfg->flags & COUNTER_TOP_CFG_DONT_RESET) != 0) {
			return -ENOTSUP;
		}
		mcux_lpit_stop(dev);
		LPIT_SetTimerPeriod(config->base, channel_id, cfg->ticks);
		mcux_lpit_start(dev);
	} else {
		LPIT_SetTimerPeriod(config->base, channel_id, cfg->ticks);
	}

	return 0;
}

static uint32_t mcux_lpit_get_pending_int(const struct device *dev)
{
	const struct mcux_lpit_config *config = dev->config;
	uint8_t channel_id = LPIT_CHANNEL_ID(dev);

	return (LPIT_GetStatusFlags(config->base) >> channel_id) & 0x1U;
}

static uint32_t mcux_lpit_get_frequency(const struct device *dev)
{
	const struct mcux_lpit_config *config = dev->config;
	uint32_t clock_rate;

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys, &clock_rate)) {
		LOG_ERR("Failed to get clock rate");
		return 0;
	}

	return clock_rate;
}

static void mcux_lpit_isr(const struct device *dev)
{
	const struct mcux_lpit_config *config = dev->config;

	LOG_DBG("lpit counter isr");

	uint32_t flags = LPIT_GetStatusFlags(config->base);

	for (int channel_index = 0; channel_index < config->num_channels; channel_index++) {

		if ((flags & BIT(channel_index)) == 0) {
			continue;
		}

		/* Clear interrupt flag */
		LPIT_ClearStatusFlags(config->base, BIT(channel_index));

		struct mcux_lpit_channel_data *data =
			LPIT_CHANNEL_DATA(config->channels[channel_index]);

		if (data->top_callback != NULL) {
			data->top_callback(dev, data->top_user_data);
		}
	}
}

static int mcux_lpit_init(const struct device *dev)
{
	const struct mcux_lpit_config *config = dev->config;
	lpit_config_t lpit_config;
	uint32_t clock_rate;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock control device not ready");
		return -ENODEV;
	}

	LPIT_GetDefaultConfig(&lpit_config);
	lpit_config.enableRunInDebug = config->lpit_config.enableRunInDebug;
	lpit_config.enableRunInDoze = config->lpit_config.enableRunInDoze;

	LPIT_Init(config->base, &lpit_config);

	clock_rate = mcux_lpit_get_frequency(dev);

	config->irq_config_func(dev);

	for (int channel_index = 0U; channel_index < config->num_channels; channel_index++) {
		LPIT_SetTimerPeriod(config->base, channel_index, config->info.max_top_value);
	}

	return 0;
}

static DEVICE_API(counter, mcux_lpit_driver_api) = {
	.start = mcux_lpit_start,
	.stop = mcux_lpit_stop,
	.get_value = mcux_lpit_get_value,
	.set_top_value = mcux_lpit_set_top_value,
	.get_pending_int = mcux_lpit_get_pending_int,
	.get_top_value = mcux_lpit_get_top_value,
	.get_freq = mcux_lpit_get_frequency,
};

/* Creates a device for a channel (needed for counter API) */
#define MCUX_LPIT_CHANNEL_DEV_INIT(node, lpit_inst)                                                \
	DEVICE_DT_DEFINE(node, NULL, NULL,                                                         \
			 (void *)&mcux_lpit_##lpit_inst##_channel_datas[DT_REG_ADDR(node)],        \
			 &mcux_lpit_##lpit_inst##_config, POST_KERNEL,                             \
			 CONFIG_COUNTER_INIT_PRIORITY, &mcux_lpit_driver_api);

/* Creates a decleration for each lpit channel */
#define MCUX_LPIT_CHANNEL_DECLARATIONS(node)                                                       \
	static struct mcux_lpit_channel_data mcux_lpit_channel_data_##node;

/* Initializes an element of the channel data pointer array */
#define MCUX_LPIT_INSERT_CHANNEL_INTO_ARRAY(node)                                                  \
	[DT_REG_ADDR(node)] = &mcux_lpit_channel_data_##node,

#define MCUX_LPIT_INSERT_CHANNEL_DEVICE_INTO_ARRAY(node) [DT_REG_ADDR(node)] = DEVICE_DT_GET(node),

#define MCUX_LPIT_IRQ_CONFIG_DECLARATIONS(n)                                                       \
	static void mcux_lpit_irq_config_func_##n(const struct device *dev)                        \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, 0, irq), DT_INST_IRQ_BY_IDX(n, 0, priority),     \
			    mcux_lpit_isr, DEVICE_DT_INST_GET(n), 0);                              \
		irq_enable(DT_INST_IRQN(n));                                                       \
	};

#define MCUX_LPIT_SETUP_IRQ_CONFIG(n) MCUX_LPIT_IRQ_CONFIG_DECLARATIONS(n);
#define MCUX_LPIT_SETUP_IRQ_ARRAY(ignored)

#define COUNTER_MCUX_LPIT_DEVICE_INIT(n)                                                           \
                                                                                                   \
	/* Setup the IRQ either for parent irq or per channel irq */                               \
	MCUX_LPIT_SETUP_IRQ_CONFIG(n)                                                              \
                                                                                                   \
	/* Create channel declarations */                                                          \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, MCUX_LPIT_CHANNEL_DECLARATIONS)                       \
                                                                                                   \
	/* Array of channel devices */                                                             \
	static struct mcux_lpit_channel_data                                                       \
		*const mcux_lpit_##n##_channel_datas[DT_INST_FOREACH_CHILD_SEP_VARGS(              \
			n, DT_NODE_HAS_COMPAT, (+), nxp_lpit_channel)] = {                         \
			DT_INST_FOREACH_CHILD_STATUS_OKAY(n,                                       \
							  MCUX_LPIT_INSERT_CHANNEL_INTO_ARRAY)};   \
                                                                                                   \
	/* forward declaration */                                                                  \
	static const struct mcux_lpit_config mcux_lpit_##n##_config;                               \
                                                                                                   \
	/* Create all the channel/counter devices */                                               \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_VARGS(n, MCUX_LPIT_CHANNEL_DEV_INIT, n)                  \
                                                                                                   \
	/* This channel device array is needed by the module device ISR */                         \
	const struct device *const mcux_lpit_##n##_channels[DT_INST_FOREACH_CHILD_SEP_VARGS(       \
		n, DT_NODE_HAS_COMPAT, (+), nxp_lpit_channel)] = {                                 \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, MCUX_LPIT_INSERT_CHANNEL_DEVICE_INTO_ARRAY)}; \
                                                                                                   \
	MCUX_LPIT_SETUP_IRQ_ARRAY(n)                                                               \
                                                                                                   \
	/* This config struct is shared by all the channels and parent device */                   \
	static const struct mcux_lpit_config mcux_lpit_##n##_config = {                            \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = DT_INST_PROP(n, max_load_value),                  \
				.channels = 0U,                                                    \
			},                                                                         \
		.base = (LPIT_Type *)DT_INST_REG_ADDR(n),                                          \
		.lpit_config =                                                                     \
			{                                                                          \
				.enableRunInDebug = DT_INST_PROP(n, enable_run_in_debug),          \
				.enableRunInDoze = DT_INST_PROP(n, enable_run_in_doze),            \
			},                                                                         \
		.irq_config_func = mcux_lpit_irq_config_func_##n,                                  \
		.num_channels = DT_INST_FOREACH_CHILD_SEP_VARGS(n, DT_NODE_HAS_COMPAT, (+),        \
								nxp_lpit_channel),                 \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.data = mcux_lpit_##n##_channel_datas,                                             \
		.channels = mcux_lpit_##n##_channels,                                              \
	};                                                                                         \
                                                                                                   \
	/* Init parent device in order to handle ISR and init. */                                  \
	DEVICE_DT_INST_DEFINE(n, &mcux_lpit_init, NULL, NULL, &mcux_lpit_##n##_config,             \
			      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(COUNTER_MCUX_LPIT_DEVICE_INIT)
