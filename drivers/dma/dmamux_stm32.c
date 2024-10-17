/*
 * Copyright (c) 2020 STMicroelectronics
 * Copyright (c) 2023 Jeroen van Dooren, Nobleo Technology
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
#ifdef CONFIG_DMA_STM32_BDMA
#include "dma_stm32_bdma.h"
#endif

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

#if !defined(CONFIG_SOC_SERIES_STM32G0X) && \
	!defined(CONFIG_SOC_SERIES_STM32C0X)
#define dmamux_channel_typedef DMAMUX_Channel_TypeDef
#else
#define dmamux_channel_typedef const DMAMUX_Channel_TypeDef
#endif
uint32_t (*func_ll_is_active_so[])(dmamux_channel_typedef * DMAMUXx) = {
	LISTIFY(DT_INST_PROP(0, dma_channels), IS_ACTIVE_FLAG_SOX, (,))
};

void (*func_ll_clear_so[])(dmamux_channel_typedef * DMAMUXx) = {
	LISTIFY(DT_INST_PROP(0, dma_channels), CLEAR_FLAG_SOX, (,))
};

uint32_t (*func_ll_is_active_rgo[])(dmamux_channel_typedef * DMAMUXx) = {
	LISTIFY(DT_INST_PROP(0, dma_generators), IS_ACTIVE_FLAG_RGOX, (,))
};

void (*func_ll_clear_rgo[])(dmamux_channel_typedef * DMAMUXx) = {
	LISTIFY(DT_INST_PROP(0, dma_generators), CLEAR_FLAG_RGOX, (,))
};

typedef int (*dma_configure_fn)(const struct device *dev, uint32_t id, struct dma_config *config);
typedef int (*dma_start_fn)(const struct device *dev, uint32_t id);
typedef int (*dma_stop_fn)(const struct device *dev, uint32_t id);
typedef int (*dma_reload_fn)(const struct device *dev, uint32_t id,
			uint32_t src, uint32_t dst, size_t size);
typedef int (*dma_status_fn)(const struct device *dev, uint32_t id,
				struct dma_status *stat);

struct dmamux_stm32_dma_fops {
	dma_configure_fn configure;
	dma_start_fn start;
	dma_stop_fn stop;
	dma_reload_fn reload;
	dma_status_fn get_status;
};

#if (defined(CONFIG_DMA_STM32_V1) || defined(CONFIG_DMA_STM32_V2)) && \
	DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux1))
static const struct dmamux_stm32_dma_fops dmamux1 = {
	dma_stm32_configure,
	dma_stm32_start,
	dma_stm32_stop,
	dma_stm32_reload,
	dma_stm32_get_status,
};
#endif

#if defined(CONFIG_DMA_STM32_BDMA) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux2))
static const struct dmamux_stm32_dma_fops dmamux2 = {
	bdma_stm32_configure,
	bdma_stm32_start,
	bdma_stm32_stop,
	bdma_stm32_reload,
	bdma_stm32_get_status
};
#endif /* CONFIG_DMA_STM32_BDMA */

const struct dmamux_stm32_dma_fops *get_dma_fops(const struct dmamux_stm32_config *dev_config)
{
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux1))
	if (dev_config->base == DT_REG_ADDR(DT_NODELABEL(dmamux1))) {
		return &dmamux1;
	}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux1)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux2))
	if (dev_config->base == DT_REG_ADDR(DT_NODELABEL(dmamux2))) {
		return &dmamux2;
	}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux2)) */

	__ASSERT(false, "Unknown dma base address %x", dev_config->base);
	return (void *)0;
}

int dmamux_stm32_configure(const struct device *dev, uint32_t id,
				struct dma_config *config)
{
	/* device is the dmamux, id is the dmamux channel from 0 */
	const struct dmamux_stm32_config *dev_config = dev->config;
	const struct dmamux_stm32_dma_fops *dma_device = get_dma_fops(dev_config);

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
	if (dma_device->configure(dev_config->mux_channels[id].dev_dma,
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
	const struct dmamux_stm32_dma_fops *dma_device = get_dma_fops(dev_config);

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_device->start(dev_config->mux_channels[id].dev_dma,
		dev_config->mux_channels[id].dma_id) != 0) {
		LOG_ERR("cannot start the dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

int dmamux_stm32_stop(const struct device *dev, uint32_t id)
{
	const struct dmamux_stm32_config *dev_config = dev->config;
	const struct dmamux_stm32_dma_fops *dma_device = get_dma_fops(dev_config);

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_device->stop(dev_config->mux_channels[id].dev_dma,
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
	const struct dmamux_stm32_dma_fops *dma_device = get_dma_fops(dev_config);

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_device->reload(dev_config->mux_channels[id].dev_dma,
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
	const struct dmamux_stm32_dma_fops *dma_device = get_dma_fops(dev_config);

	/* check if this channel is valid */
	if (id >= dev_config->channel_nb) {
		LOG_ERR("channel ID %d is too big.", id);
		return -EINVAL;
	}

	if (dma_device->get_status(dev_config->mux_channels[id].dev_dma,
		dev_config->mux_channels[id].dma_id, stat) != 0) {
		LOG_ERR("cannot get the status of dmamux channel %d.", id);
		return -EINVAL;
	}

	return 0;
}

static int dmamux_stm32_init(const struct device *dev)
{
	const struct dmamux_stm32_config *config = dev->config;
#if DT_INST_NODE_HAS_PROP(0, clocks)
	const struct device *const clk = DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE);

	if (!device_is_ready(clk)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(clk,
		(clock_control_subsys_t) &config->pclken) != 0) {
		LOG_ERR("clock op failed\n");
		return -EIO;
	}
#endif /* DT_INST_NODE_HAS_PROP(0, clocks) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux1))
	/* DMA 1 and DMA2 for DMAMUX1, BDMA for DMAMUX2 */
	if (config->base == DT_REG_ADDR(DT_NODELABEL(dmamux1))) {
		/* DMAs assigned to DMAMUX channels at build time might not be ready. */
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dma1))
		if (device_is_ready(DEVICE_DT_GET(DT_NODELABEL(dma1))) == false) {
			return -ENODEV;
		}
#endif
#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dma2))
		if (device_is_ready(DEVICE_DT_GET(DT_NODELABEL(dma2))) == false) {
			return -ENODEV;
		}
#endif
	}
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux1)) */

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dmamux2)) && DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(bdma1))
	if (config->base == DT_REG_ADDR(DT_NODELABEL(dmamux2))) {
		if (device_is_ready(DEVICE_DT_GET(DT_NODELABEL(bdma1))) == false) {
			return -ENODEV;
		}
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
 * up to 2 dmamuxes and up to 3 dmas.
 */

#define DMA_1_BEGIN_DMAMUX_CHANNEL DT_PROP_OR(DT_NODELABEL(dma1), dma_offset, 0)
#define DMA_1_END_DMAMUX_CHANNEL (DMA_1_BEGIN_DMAMUX_CHANNEL + \
				DT_PROP_OR(DT_NODELABEL(dma1), dma_requests, 0))
#define DEV_DMA1 COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dma1)), \
			     DEVICE_DT_GET(DT_NODELABEL(dma1)), NULL)

#define DMA_2_BEGIN_DMAMUX_CHANNEL DT_PROP_OR(DT_NODELABEL(dma2), dma_offset, 0)
#define DMA_2_END_DMAMUX_CHANNEL (DMA_2_BEGIN_DMAMUX_CHANNEL + \
				DT_PROP_OR(DT_NODELABEL(dma2), dma_requests, 0))
#define DEV_DMA2 COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(dma2)), \
			     DEVICE_DT_GET(DT_NODELABEL(dma2)), NULL)

#define BDMA_1_BEGIN_DMAMUX_CHANNEL DT_PROP_OR(DT_NODELABEL(bdma1), dma_offset, 0)
#define BDMA_1_END_DMAMUX_CHANNEL (BDMA_1_BEGIN_DMAMUX_CHANNEL + \
				DT_PROP_OR(DT_NODELABEL(bdma1), dma_requests, 0))
#define DEV_BDMA COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(bdma1)), \
			     DEVICE_DT_GET(DT_NODELABEL(bdma1)), NULL)

#define DEV_DMA_BINDING(mux_channel) \
	((mux_channel < DMA_1_END_DMAMUX_CHANNEL) ? DEV_DMA1 : DEV_DMA2)

#define DEV_BDMA_BINDING(mux_channel) \
	(DEV_BDMA)


#define DMA_CHANNEL(mux_channel) \
	((mux_channel < DMA_1_END_DMAMUX_CHANNEL) ? \
	 (mux_channel + 1) : (mux_channel - DMA_2_BEGIN_DMAMUX_CHANNEL + 1))

#define BDMA_CHANNEL(mux_channel) \
	((mux_channel < BDMA_1_END_DMAMUX_CHANNEL) ? \
	 (mux_channel) : 0 /* not supported */)

/*
 * H7 series implements DMAMUX1 and DMAMUX2
 * DMAMUX1 is used by DMA1 and DMA2
 * DMAMUX2 is used by BDMA
 *
 * Note: Instance Number (or index) has no guarantee to which dmamux it refers
 */

#define INIT_DMAMUX1_CHANNEL(x, ...) \
	{ .dev_dma = DEV_DMA_BINDING(x), .dma_id = DMA_CHANNEL(x), }
#define INIT_DMAMUX2_CHANNEL(x, ...) \
	{ .dev_dma = DEV_BDMA_BINDING(x), .dma_id = BDMA_CHANNEL(x), }

#if DT_SAME_NODE(DT_DRV_INST(0), DT_NODELABEL(dmamux1))
#define INIT_INST0_CHANNEL(x, ...) INIT_DMAMUX1_CHANNEL(x, ...)
#define INIT_INST1_CHANNEL(x, ...) INIT_DMAMUX2_CHANNEL(x, ...)
#else
#define INIT_INST0_CHANNEL(x, ...) INIT_DMAMUX2_CHANNEL(x, ...)
#define INIT_INST1_CHANNEL(x, ...) INIT_DMAMUX1_CHANNEL(x, ...)
#endif

#define DMAMUX_CHANNELS_INIT(index, count)                \
	LISTIFY(count, INIT_INST##index##_CHANNEL, (,))

#define DMAMUX_CLOCK_INIT(index) \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, clocks),		\
	(.pclken = {	.bus = DT_INST_CLOCKS_CELL(index, bus),		\
			.enr = DT_INST_CLOCKS_CELL(index, bits)},),	\
	())

#define DMAMUX_INIT(index)						\
static const struct dmamux_stm32_channel				\
	dmamux_stm32_channels_##index[DT_INST_PROP(index, dma_channels)] = {   \
		DMAMUX_CHANNELS_INIT(index, DT_INST_PROP(index, dma_channels))\
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
		    PRE_KERNEL_1, CONFIG_DMAMUX_STM32_INIT_PRIORITY,	\
		    &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMAMUX_INIT)

/*
 * Make sure that this driver is initialized after the DMA driver (higher priority)
 */
BUILD_ASSERT(CONFIG_DMAMUX_STM32_INIT_PRIORITY >= CONFIG_DMA_INIT_PRIORITY,
	     "CONFIG_DMAMUX_STM32_INIT_PRIORITY must be higher than CONFIG_DMA_INIT_PRIORITY");
