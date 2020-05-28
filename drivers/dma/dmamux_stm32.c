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
#include <init.h>
#include <drivers/dma.h>
#include <drivers/clock_control.h>
#include <drivers/clock_control/stm32_clock_control.h>

#include "dmamux_stm32.h"
#include "dma_stm32.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(dmamux_stm32, CONFIG_DMA_LOG_LEVEL);

#define DT_DRV_COMPAT st_stm32_dmamux

int dmamux_stm32_configure(struct device *dev, uint32_t id,
				struct dma_config *config)
{
	/* device is the dmamux, id is the dmamux channel from 0 */
	struct dmamux_stm32_data *data = dev->data;
	const struct dmamux_stm32_config *dev_config = dev->config;

	/*
	 * request line ID for this mux channel is stored
	 * in the dma_slot parameter
	 */
	int request_id = config->dma_slot;

	if (request_id >= dev_config->req_nb + dev_config->gen_nb) {
		LOG_ERR("request ID %d is too big.", request_id);
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
	if (dma_stm32_configure(data->mux_channels[id].dev_dma,
			data->mux_channels[id].dma_id, config) != 0) {
		LOG_ERR("cannot configure the dmamux.");
		return -EINVAL;
	}

	/* set the Request Line ID to this dmamux channel i */
	DMAMUX_Channel_TypeDef *dmamux =
			(DMAMUX_Channel_TypeDef *)dev_config->base;

	LL_DMAMUX_SetRequestID(dmamux, id, request_id);

	return 0;
}

int dmamux_stm32_start(struct device *dev, uint32_t id)
{
	const struct dmamux_stm32_config *dev_config = dev->config;
	struct dmamux_stm32_data *data = dev->data;

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_stm32_start(data->mux_channels[id].dev_dma,
		data->mux_channels[id].dma_id) != 0) {
		LOG_ERR("cannot start the dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

int dmamux_stm32_stop(struct device *dev, uint32_t id)
{
	const struct dmamux_stm32_config *dev_config = dev->config;
	struct dmamux_stm32_data *data = dev->data;

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_stm32_stop(data->mux_channels[id].dev_dma,
		data->mux_channels[id].dma_id) != 0) {
		LOG_ERR("cannot stop the dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

int dmamux_stm32_reload(struct device *dev, uint32_t id,
			    uint32_t src, uint32_t dst, size_t size)
{
	const struct dmamux_stm32_config *dev_config = dev->config;
	struct dmamux_stm32_data *data = dev->data;

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_stm32_reload(data->mux_channels[id].dev_dma,
		data->mux_channels[id].dma_id,
		src, dst, size) != 0) {
		LOG_ERR("cannot reload the dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

static int dmamux_stm32_init(struct device *dev)
{
	struct dmamux_stm32_data *data = dev->data;
	const struct dmamux_stm32_config *config = dev->config;
	struct device *clk =
		device_get_binding(STM32_CLOCK_CONTROL_NAME);

	if (clock_control_on(clk,
		(clock_control_subsys_t *) &config->pclken) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}

	int size_stream =
		sizeof(struct dmamux_stm32_channel) * config->channel_nb;
	data->mux_channels = k_malloc(size_stream);
	if (!data->mux_channels) {
		LOG_ERR("HEAP_MEM_POOL_SIZE is too small");
		return -ENOMEM;
	}
	for (int i = 0; i < config->channel_nb; i++) {
		/*
		 * associates the dmamux channel
		 * to the corresponding dma stream
		 */
		if (i < config->channel_nb / 2) {
			data->mux_channels[i].dev_dma =
				device_get_binding((const char *)"DMA_1");
			/* dma 1 channels from 1 to N */
			data->mux_channels[i].dma_id = i + 1;
		} else {
			data->mux_channels[i].dev_dma =
				device_get_binding((const char *)"DMA_2");
			data->mux_channels[i].dma_id =
			/* dma 2 channels from 1 to N */
				i - config->channel_nb / 2 + 1;
		}
	}

	return 0;
}

static const struct dma_driver_api dma_funcs = {
	.reload		 = dmamux_stm32_reload,
	.config		 = dmamux_stm32_configure,
	.start		 = dmamux_stm32_start,
	.stop		 = dmamux_stm32_stop,
};

#define DMAMUX_INIT(index) \
								\
const struct dmamux_stm32_config dmamux_stm32_config_##index = {\
	.pclken = { .bus = DT_INST_CLOCKS_CELL(index, bus),	\
		    .enr = DT_INST_CLOCKS_CELL(index, bits) },	\
	.base = DT_INST_REG_ADDR(index),			\
	.channel_nb = DT_INST_PROP(index, dma_channels),	\
	.gen_nb = DT_INST_PROP(index, dma_generators),		\
	.req_nb = DT_INST_PROP(index, dma_requests),		\
};								\
								\
static struct dmamux_stm32_data dmamux_stm32_data_##index = {	\
	.mux_channels = NULL,					\
};								\
								\
DEVICE_AND_API_INIT(dmamux_##index, DT_INST_LABEL(index),	\
		    &dmamux_stm32_init,				\
		    &dmamux_stm32_data_##index, &dmamux_stm32_config_##index,\
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,\
		    &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMAMUX_INIT)
