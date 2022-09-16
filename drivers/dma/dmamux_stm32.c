/*
 * Copyright (c) 2020 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common part of DMAMUX drivers for stm32.
 * @note  api functions named dmamux_stm32_
 *        are calling the dma_stm32 corresponding function
 *        implemented in dma_stm32.c
 */

#include <soc.h>
#include <stm32_ll_dmamux.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>

#include "dma_stm32.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dmamux_stm32, CONFIG_DMA_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_dmamux

/* this is the configuration of one dmamux channel */
struct dmamux_stm32_channel {
	/* pointer to the associated dma instance */
	const struct device *dev_dma;
	/* ref of the associated dma stream for this instance */
	uint8_t dma_id;
};

/* the table of all the dmamux channel */
struct dmamux_stm32_data {
	void *callback_arg;
	void (*dmamux_callback)(void *arg, uint32_t id,
				int error_code);
};

/* this is the configuration of the dmamux IP */
struct dmamux_stm32_config {
#if DT_INST_NODE_HAS_PROP(0, clocks)
	struct stm32_pclken pclken;
#endif
	uint32_t base;
	uint8_t channel_nb;	/* total nb of channels */
	uint8_t gen_nb;	/* total nb of Request generator */
	uint8_t req_nb;	/* total nb of Peripheral Request inputs */
	const struct dmamux_stm32_channel *mux_channels;
};

/*
 * LISTIFY is used to generate arrays with function pointers to check
 * and clear interrupt flags using LL functions
 */
#define DMAMUX_CHANNEL(i, _)		LL_DMAMUX_CHANNEL_ ## i
#define IS_ACTIVE_FLAG_SOX(i, _)	LL_DMAMUX_IsActiveFlag_SO  ## i
#define CLEAR_FLAG_SOX(i, _)		LL_DMAMUX_ClearFlag_SO ## i
#define IS_ACTIVE_FLAG_RGOX(i, _)	LL_DMAMUX_IsActiveFlag_RGO  ## i
#define CLEAR_FLAG_RGOX(i, _)		LL_DMAMUX_ClearFlag_RGO ## i

uint32_t table_ll_channel[] = {
	LISTIFY(DT_INST_PROP(0, dma_channels), DMAMUX_CHANNEL, (,))
};

uint32_t (*func_ll_is_active_so[])(DMAMUX_Channel_TypeDef *DMAMUXx) = {
	LISTIFY(DT_INST_PROP(0, dma_channels), IS_ACTIVE_FLAG_SOX, (,))
};

void (*func_ll_clear_so[])(DMAMUX_Channel_TypeDef *DMAMUXx) = {
	LISTIFY(DT_INST_PROP(0, dma_channels), CLEAR_FLAG_SOX, (,))
};

uint32_t (*func_ll_is_active_rgo[])(DMAMUX_Channel_TypeDef *DMAMUXx) = {
	LISTIFY(DT_INST_PROP(0, dma_generators), IS_ACTIVE_FLAG_RGOX, (,))
};

void (*func_ll_clear_rgo[])(DMAMUX_Channel_TypeDef *DMAMUXx) = {
	LISTIFY(DT_INST_PROP(0, dma_generators), CLEAR_FLAG_RGOX, (,))
};

int dmamux_stm32_configure(const struct device *dev, uint32_t id,
				struct dma_config *config)
{
	/* device is the dmamux, id is the dmamux channel from 0 */
	const struct dmamux_stm32_config *dev_config = dev->config;

	/*
	 * request line ID for this mux channel is stored
	 * in the dma_slot parameter
	 */
	int request_id = config->dma_slot;

	if (request_id > dev_config->req_nb + dev_config->gen_nb) {
		LOG_ERR("request ID %d is not valid.", request_id);
		return -EINVAL;
	}

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	/*
	 * Also configures the corresponding dma channel
	 * instance is given by the dev_dma
	 * stream is given by the index i
	 * config is directly this dma_config
	 */

	/*
	 * This dmamux channel 'id' is now used for this peripheral request
	 * It gives this mux request ID to the dma through the config.dma_slot
	 */
	if (dma_stm32_configure(dev_config->mux_channels[id].dev_dma,
			dev_config->mux_channels[id].dma_id, config) != 0) {
		LOG_ERR("cannot configure the dmamux.");
		return -EINVAL;
	}

	/* set the Request Line ID to this dmamux channel i */
	DMAMUX_Channel_TypeDef *dmamux =
			(DMAMUX_Channel_TypeDef *)dev_config->base;

	LL_DMAMUX_SetRequestID(dmamux, id, request_id);

	return 0;
}

int dmamux_stm32_start(const struct device *dev, uint32_t id)
{
	const struct dmamux_stm32_config *dev_config = dev->config;

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_stm32_start(dev_config->mux_channels[id].dev_dma,
		dev_config->mux_channels[id].dma_id) != 0) {
		LOG_ERR("cannot start the dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

int dmamux_stm32_stop(const struct device *dev, uint32_t id)
{
	const struct dmamux_stm32_config *dev_config = dev->config;

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_stm32_stop(dev_config->mux_channels[id].dev_dma,
		dev_config->mux_channels[id].dma_id) != 0) {
		LOG_ERR("cannot stop the dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

int dmamux_stm32_reload(const struct device *dev, uint32_t id,
			    uint32_t src, uint32_t dst, size_t size)
{
	const struct dmamux_stm32_config *dev_config = dev->config;

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_stm32_reload(dev_config->mux_channels[id].dev_dma,
		dev_config->mux_channels[id].dma_id,
		src, dst, size) != 0) {
		LOG_ERR("cannot reload the dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

int dmamux_stm32_get_status(const struct device *dev, uint32_t id,
				struct dma_status *stat)
{
	const struct dmamux_stm32_config *dev_config = dev->config;

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_stm32_get_status(dev_config->mux_channels[id].dev_dma,
		dev_config->mux_channels[id].dma_id, stat) != 0) {
		LOG_ERR("cannot get the status of dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

static int dmamux_stm32_init(const struct device *dev)
{
#if DT_INST_NODE_HAS_PROP(0, clocks)
	const struct dmamux_stm32_config *config = dev->config;
	const struct device *clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (clock_control_on(clk,
		(clock_control_subsys_t *) &config->pclken) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}
#endif /* DT_INST_NODE_HAS_PROP(0, clocks) */

	/* DMAs assigned to DMAMUX channels at build time might not be ready. */
#if DT_NODE_HAS_STATUS(DT_NODELABEL(dma1), okay)
	if (device_is_ready(DEVICE_DT_GET(DT_NODELABEL(dma1))) == false) {
		return -ENODEV;
	}
#endif
#if DT_NODE_HAS_STATUS(DT_NODELABEL(dma2), okay)
	if (device_is_ready(DEVICE_DT_GET(DT_NODELABEL(dma2))) == false) {
		return -ENODEV;
	}
#endif

	return 0;
}

static const struct dma_driver_api dma_funcs = {
	.reload		 = dmamux_stm32_reload,
	.config		 = dmamux_stm32_configure,
	.start		 = dmamux_stm32_start,
	.stop		 = dmamux_stm32_stop,
	.get_status	 = dmamux_stm32_get_status,
};

/*
 * Each dmamux channel is hardwired to one dma controllers dma channel.
 * DMAMUX_CHANNEL_INIT_X macros resolve this mapping at build time for each
 * dmamux channel using the dma dt properties dma_offset and dma_requests,
 * such that it can be stored in dmamux_stm32_channels_X configuration.
 * The Macros to get the corresponding dma device binding and dma channel
 * for a given dmamux channel, are currently valid for series having
 * 1 dmamux and 1 or 2 dmas.
 */

#define DMA_1_BEGIN_DMAMUX_CHANNEL DT_PROP_OR(DT_NODELABEL(dma1), dma_offset, 0)
#define DMA_1_END_DMAMUX_CHANNEL (DMA_1_BEGIN_DMAMUX_CHANNEL + \
				DT_PROP_OR(DT_NODELABEL(dma1), dma_requests, 0))
#define DEV_DMA1 COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(dma1), okay), \
			     DEVICE_DT_GET(DT_NODELABEL(dma1)), NULL)

#define DMA_2_BEGIN_DMAMUX_CHANNEL DT_PROP_OR(DT_NODELABEL(dma2), dma_offset, 0)
#define DMA_2_END_DMAMUX_CHANNEL (DMA_2_BEGIN_DMAMUX_CHANNEL + \
				DT_PROP_OR(DT_NODELABEL(dma2), dma_requests, 0))
#define DEV_DMA2 COND_CODE_1(DT_NODE_HAS_STATUS(DT_NODELABEL(dma2), okay), \
			     DEVICE_DT_GET(DT_NODELABEL(dma2)), NULL)

#define DEV_DMA_BINDING(mux_channel) \
	((mux_channel < DMA_1_END_DMAMUX_CHANNEL) ? DEV_DMA1 : DEV_DMA2)
#define DMA_CHANNEL(mux_channel) \
	((mux_channel < DMA_1_END_DMAMUX_CHANNEL) ? \
	 (mux_channel + 1) : (mux_channel - DMA_2_BEGIN_DMAMUX_CHANNEL + 1))

/*
 * No series implements more than 1 dmamux yet, dummy define added for easier
 * future extension.
 */
#define INIT_DMAMUX_0_CHANNEL(x, ...) \
	{ .dev_dma = DEV_DMA_BINDING(x), .dma_id = DMA_CHANNEL(x), }
#define INIT_DMAMUX_1_CHANNEL(x, ...) \
	{ .dev_dma = 0, .dma_id = 0, }

#define DMAMUX_CHANNELS_INIT_0(count) \
	LISTIFY(count, INIT_DMAMUX_0_CHANNEL, (,))
#define DMAMUX_CHANNELS_INIT_1(count) \
	LISTIFY(count, INIT_DMAMUX_1_CHANNEL, (,))


#define DMAMUX_CLOCK_INIT(index) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, clocks),		\
	(.pclken = {	.bus = DT_INST_CLOCKS_CELL(index, bus),		\
			.enr = DT_INST_CLOCKS_CELL(index, bits)},),	\
	())

#define DMAMUX_INIT(index)						\
static const struct dmamux_stm32_channel				\
	dmamux_stm32_channels_##index[DT_INST_PROP(index, dma_channels)] = {   \
		DMAMUX_CHANNELS_INIT_##index(DT_INST_PROP(index, dma_channels))\
	};								       \
									\
const struct dmamux_stm32_config dmamux_stm32_config_##index = {	\
	DMAMUX_CLOCK_INIT(index)					\
	.base = DT_INST_REG_ADDR(index),				\
	.channel_nb = DT_INST_PROP(index, dma_channels),		\
	.gen_nb = DT_INST_PROP(index, dma_generators),			\
	.req_nb = DT_INST_PROP(index, dma_requests),			\
	.mux_channels = dmamux_stm32_channels_##index,			\
};									\
									\
static struct dmamux_stm32_data dmamux_stm32_data_##index;		\
									\
DEVICE_DT_INST_DEFINE(index,						\
		    &dmamux_stm32_init,					\
		    NULL,						\
		    &dmamux_stm32_data_##index, &dmamux_stm32_config_##index,\
		    PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,		\
		    &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMAMUX_INIT)
