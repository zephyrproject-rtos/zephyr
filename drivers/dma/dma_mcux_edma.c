/*
 * Copyright (c) 2020 NXP Semiconductor INC.
 * All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common part of DMA drivers for imx rt series.
 */

#include <errno.h>
#include <soc.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>

#include "dma_mcux_edma.h"

#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#define DT_DRV_COMPAT nxp_mcux_edma

LOG_MODULE_REGISTER(dma_mcux_edma, CONFIG_DMA_LOG_LEVEL);

struct dma_mcux_edma_config {
	DMA_Type *base;
	DMAMUX_Type *dmamux_base;
	int dma_channels; /* number of channels */
	void (*irq_config_func)(const struct device *dev);
};


#ifdef CONFIG_HAS_MCUX_CACHE

#ifdef CONFIG_DMA_MCUX_USE_DTCM_FOR_DMA_DESCRIPTORS

#if DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay)
#define EDMA_TCDPOOL_CACHE_ATTR __dtcm_noinit_section
#else /* DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay) */
#error Selected DTCM for MCUX DMA descriptors but no DTCM section.
#endif /* DT_NODE_HAS_STATUS(DT_CHOSEN(zephyr_dtcm), okay) */

#elif defined(CONFIG_NOCACHE_MEMORY)
#define EDMA_TCDPOOL_CACHE_ATTR __nocache
#else
/*
 * Note: the TCD pool *must* be in non cacheable memory. All of the NXP SOCs
 * that support caching memory have their default SRAM regions defined as a
 * non cached memory region, but if the default SRAM region is changed EDMA
 * TCD pools would be moved to cacheable memory, resulting in DMA cache
 * coherency issues.
 */

#define EDMA_TCDPOOL_CACHE_ATTR

#endif /* CONFIG_DMA_MCUX_USE_DTCM_FOR_DMA_DESCRIPTORS */

#else /* CONFIG_HAS_MCUX_CACHE */

#define EDMA_TCDPOOL_CACHE_ATTR

#endif /* CONFIG_HAS_MCUX_CACHE */

static __aligned(32) EDMA_TCDPOOL_CACHE_ATTR edma_tcd_t
tcdpool[DT_INST_PROP(0, dma_channels)][CONFIG_DMA_TCD_QUEUE_SIZE];

struct dma_mcux_channel_transfer_edma_settings {
	uint32_t source_data_size;
	uint32_t dest_data_size;
	uint32_t source_burst_length;
	uint32_t dest_burst_length;
	enum dma_channel_direction direction;
	edma_transfer_type_t transfer_type;
	bool valid;
};


struct call_back {
	edma_transfer_config_t transferConfig;
	edma_handle_t edma_handle;
	const struct device *dev;
	void *user_data;
	dma_callback_t dma_callback;
	struct dma_mcux_channel_transfer_edma_settings transfer_settings;
	bool busy;
};

struct dma_mcux_edma_data {
	struct dma_context dma_ctx;
	struct call_back data_cb[DT_INST_PROP(0, dma_channels)];
	ATOMIC_DEFINE(channels_atomic, DT_INST_PROP(0, dma_channels));
};

#define DEV_CFG(dev) \
	((const struct dma_mcux_edma_config *const)dev->config)
#define DEV_DATA(dev) ((struct dma_mcux_edma_data *)dev->data)
#define DEV_BASE(dev) ((DMA_Type *)DEV_CFG(dev)->base)

#define DEV_DMAMUX_BASE(dev) ((DMAMUX_Type *)DEV_CFG(dev)->dmamux_base)

#define DEV_CHANNEL_DATA(dev, ch) \
	((struct call_back *)(&(DEV_DATA(dev)->data_cb[ch])))

#define DEV_EDMA_HANDLE(dev, ch) \
	((edma_handle_t *)(&(DEV_CHANNEL_DATA(dev, ch)->edma_handle)))

static bool data_size_valid(const size_t data_size)
{
	return (data_size == 4U || data_size == 2U ||
		data_size == 1U || data_size == 8U ||
		data_size == 16U ||
		data_size == 32U);
}

static void nxp_edma_callback(edma_handle_t *handle, void *param, bool transferDone,
			      uint32_t tcds)
{
	int ret = 1;
	struct call_back *data = (struct call_back *)param;
	uint32_t channel = handle->channel;

	if (transferDone) {
		/* DMA is no longer busy when there are no remaining TCDs to transfer */
		data->busy = (handle->tcdPool != NULL) && (handle->tcdUsed > 0);
		ret = 0;
	}
	LOG_DBG("transfer %d", tcds);
	data->dma_callback(data->dev, data->user_data, channel, ret);
}


static void dma_mcux_edma_irq_handler(const struct device *dev)
{
	int i = 0;

	LOG_DBG("IRQ CALLED");
	for (i = 0; i < DT_INST_PROP(0, dma_channels); i++) {
		uint32_t flag = EDMA_GetChannelStatusFlags(DEV_BASE(dev), i);

		if ((flag & (uint32_t)kEDMA_InterruptFlag) != 0U) {
			LOG_DBG("IRQ OCCURRED");
			EDMA_HandleIRQ(DEV_EDMA_HANDLE(dev, i));
			LOG_DBG("IRQ DONE");
#if defined __CORTEX_M && (__CORTEX_M == 4U)
			__DSB();
#endif
		}
	}
}

static void dma_mcux_edma_error_irq_handler(const struct device *dev)
{
	int i = 0;
	uint32_t flag = 0;

	for (i = 0; i < DT_INST_PROP(0, dma_channels); i++) {
		if (DEV_CHANNEL_DATA(dev, i)->busy) {
			flag = EDMA_GetChannelStatusFlags(DEV_BASE(dev), i);
			LOG_INF("channel %d error status is 0x%x", i, flag);
			EDMA_ClearChannelStatusFlags(DEV_BASE(dev), i,
						     0xFFFFFFFF);
			EDMA_AbortTransfer(DEV_EDMA_HANDLE(dev, i));
			DEV_CHANNEL_DATA(dev, i)->busy = false;
		}
	}

#if defined __CORTEX_M && (__CORTEX_M == 4U)
	__DSB();
#endif
}

/* Configure a channel */
static int dma_mcux_edma_configure(const struct device *dev, uint32_t channel,
				   struct dma_config *config)
{
	/* Check for invalid parameters before dereferencing them. */
	if (NULL == dev || NULL == config) {
		return -EINVAL;
	}

	edma_handle_t *p_handle = DEV_EDMA_HANDLE(dev, channel);
	struct call_back *data = DEV_CHANNEL_DATA(dev, channel);
	struct dma_block_config *block_config = config->head_block;
	uint32_t slot = config->dma_slot;
	edma_transfer_type_t transfer_type;
	unsigned int key;
	int ret = 0;

	if (slot > DT_INST_PROP(0, dma_requests)) {
		LOG_ERR("source number is outof scope %d", slot);
		return -ENOTSUP;
	}

	if (channel > DT_INST_PROP(0, dma_channels)) {
		LOG_ERR("out of DMA channel %d", channel);
		return -EINVAL;
	}

	data->transfer_settings.valid = false;

	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
		transfer_type = kEDMA_MemoryToMemory;
		break;
	case MEMORY_TO_PERIPHERAL:
		transfer_type = kEDMA_MemoryToPeripheral;
		break;
	case PERIPHERAL_TO_MEMORY:
		transfer_type = kEDMA_PeripheralToMemory;
		break;
	case PERIPHERAL_TO_PERIPHERAL:
		transfer_type = kEDMA_PeripheralToPeripheral;
		break;
	default:
		LOG_ERR("not support transfer direction");
		return -EINVAL;
	}

	if (!data_size_valid(config->source_data_size)) {
		LOG_ERR("Source unit size error, %d", config->source_data_size);
		return -EINVAL;
	}

	if (!data_size_valid(config->dest_data_size)) {
		LOG_ERR("Dest unit size error, %d", config->dest_data_size);
		return -EINVAL;
	}

	if (block_config->source_gather_en || block_config->dest_scatter_en) {
		if (config->block_count > CONFIG_DMA_TCD_QUEUE_SIZE) {
			LOG_ERR("please config DMA_TCD_QUEUE_SIZE as %d", config->block_count);
			return -EINVAL;
		}
	}

	data->transfer_settings.source_data_size = config->source_data_size;
	data->transfer_settings.dest_data_size = config->dest_data_size;
	data->transfer_settings.source_burst_length = config->source_burst_length;
	data->transfer_settings.dest_burst_length = config->dest_burst_length;
	data->transfer_settings.direction = config->channel_direction;
	data->transfer_settings.transfer_type = transfer_type;
	data->transfer_settings.valid = true;


	/* Lock and page in the channel configuration */
	key = irq_lock();

#if DT_INST_PROP(0, nxp_a_on)
	if (config->source_handshake || config->dest_handshake ||
	    transfer_type == kEDMA_MemoryToMemory) {
		/*software trigger make the channel always on*/
		LOG_DBG("ALWAYS ON");
		DMAMUX_EnableAlwaysOn(DEV_DMAMUX_BASE(dev), channel, true);
	} else {
		DMAMUX_SetSource(DEV_DMAMUX_BASE(dev), channel, slot);
	}
#else
	DMAMUX_SetSource(DEV_DMAMUX_BASE(dev), channel, slot);
#endif

	/* dam_imx_rt_set_channel_priority(dev, channel, config); */
	DMAMUX_EnableChannel(DEV_DMAMUX_BASE(dev), channel);

	if (data->busy) {
		EDMA_AbortTransfer(p_handle);
	}
	EDMA_ResetChannel(DEV_BASE(dev), channel);
	EDMA_CreateHandle(p_handle, DEV_BASE(dev), channel);
	EDMA_SetCallback(p_handle, nxp_edma_callback, (void *)data);

	LOG_DBG("channel is %d", p_handle->channel);
	EDMA_EnableChannelInterrupts(DEV_BASE(dev), channel, kEDMA_ErrorInterruptEnable);

	if (block_config->source_gather_en || block_config->dest_scatter_en) {
		EDMA_InstallTCDMemory(p_handle, tcdpool[channel], CONFIG_DMA_TCD_QUEUE_SIZE);
		while (block_config != NULL) {
			EDMA_PrepareTransfer(
				&(data->transferConfig),
				(void *)block_config->source_address,
				config->source_data_size,
				(void *)block_config->dest_address,
				config->dest_data_size,
				config->source_burst_length,
				block_config->block_size, transfer_type);

			const status_t submit_status =
				EDMA_SubmitTransfer(p_handle, &(data->transferConfig));
			if (submit_status != kStatus_Success) {
				LOG_ERR("Error submitting EDMA Transfer: 0x%x", submit_status);
				ret = -EFAULT;
			}
			block_config = block_config->next_block;
		}
	} else {
		/* block_count shall be 1 */
		LOG_DBG("block size is: %d", block_config->block_size);
		EDMA_PrepareTransfer(&(data->transferConfig),
				     (void *)block_config->source_address,
				     config->source_data_size,
				     (void *)block_config->dest_address,
				     config->dest_data_size,
				     config->source_burst_length,
				     block_config->block_size, transfer_type);

		const status_t submit_status =
			EDMA_SubmitTransfer(p_handle, &(data->transferConfig));
		if (submit_status != kStatus_Success) {
			LOG_ERR("Error submitting EDMA Transfer: 0x%x", submit_status);
			ret = -EFAULT;
		}
		edma_tcd_t *tcdRegs = (edma_tcd_t *)(uint32_t)&p_handle->base->TCD[channel];
		LOG_DBG("data csr is 0x%x", tcdRegs->CSR);
	}

	if (config->dest_chaining_en) {
		LOG_DBG("link major channel %d", config->linked_channel);
		EDMA_SetChannelLink(DEV_BASE(dev), channel, kEDMA_MajorLink,
				    config->linked_channel);
	}
	if (config->source_chaining_en) {
		LOG_DBG("link minor channel %d", config->linked_channel);
		EDMA_SetChannelLink(DEV_BASE(dev), channel, kEDMA_MinorLink,
				    config->linked_channel);
	}

	data->busy = false;
	if (config->dma_callback) {
		LOG_DBG("INSTALL call back on channel %d", channel);
		data->user_data = config->user_data;
		data->dma_callback = config->dma_callback;
		data->dev = dev;
	}

	irq_unlock(key);

	return ret;
}

static int dma_mcux_edma_start(const struct device *dev, uint32_t channel)
{
	struct call_back *data = DEV_CHANNEL_DATA(dev, channel);

	LOG_DBG("START TRANSFER");
	LOG_DBG("DMAMUX CHCFG 0x%x", DEV_DMAMUX_BASE(dev)->CHCFG[channel]);
	LOG_DBG("DMA CR 0x%x", DEV_BASE(dev)->CR);
	data->busy = true;
	EDMA_StartTransfer(DEV_EDMA_HANDLE(dev, channel));
	return 0;
}

static int dma_mcux_edma_stop(const struct device *dev, uint32_t channel)
{
	struct dma_mcux_edma_data *data = DEV_DATA(dev);

	data->data_cb[channel].transfer_settings.valid = false;

	if (!data->data_cb[channel].busy) {
		return 0;
	}
	EDMA_AbortTransfer(DEV_EDMA_HANDLE(dev, channel));
	EDMA_ClearChannelStatusFlags(DEV_BASE(dev), channel,
				     kEDMA_DoneFlag | kEDMA_ErrorFlag |
				     kEDMA_InterruptFlag);
	EDMA_ResetChannel(DEV_BASE(dev), channel);
	data->data_cb[channel].busy = false;
	return 0;
}

static int dma_mcux_edma_suspend(const struct device *dev, uint32_t channel)
{
	struct call_back *data = DEV_CHANNEL_DATA(dev, channel);

	if (!data->busy) {
		return -EINVAL;
	}
	EDMA_StopTransfer(DEV_EDMA_HANDLE(dev, channel));
	return 0;
}

static int dma_mcux_edma_resume(const struct device *dev, uint32_t channel)
{
	struct call_back *data = DEV_CHANNEL_DATA(dev, channel);

	if (!data->busy) {
		return -EINVAL;
	}
	EDMA_StartTransfer(DEV_EDMA_HANDLE(dev, channel));
	return 0;
}


static int dma_mcux_edma_reload(const struct device *dev, uint32_t channel,
				uint32_t src, uint32_t dst, size_t size)
{
	struct call_back *data = DEV_CHANNEL_DATA(dev, channel);

	/* Lock the channel configuration */
	const unsigned int key = irq_lock();
	int ret = 0;

	if (!data->transfer_settings.valid) {
		LOG_ERR("Invalid EDMA settings on initial config. Configure DMA before reload.");
		ret = -EFAULT;
		goto cleanup;
	}

	/* If the tcdPool is not in use (no s/g) then only a single TCD can be active at once. */
	if (data->busy && data->edma_handle.tcdPool == NULL) {
		LOG_ERR("EDMA busy. Wait until the transfer completes before reloading.");
		ret = -EBUSY;
		goto cleanup;
	}

	EDMA_PrepareTransfer(
		&(data->transferConfig),
		(void *)src,
		data->transfer_settings.source_data_size,
		(void *)dst,
		data->transfer_settings.dest_data_size,
		data->transfer_settings.source_burst_length,
		size,
		data->transfer_settings.transfer_type);

	const status_t submit_status =
		EDMA_SubmitTransfer(DEV_EDMA_HANDLE(dev, channel), &(data->transferConfig));

	if (submit_status != kStatus_Success) {
		LOG_ERR("Error submitting EDMA Transfer: 0x%x", submit_status);
		ret = -EFAULT;
	}

cleanup:
	irq_unlock(key);
	return ret;
}

static int dma_mcux_edma_get_status(const struct device *dev, uint32_t channel,
				    struct dma_status *status)
{
	edma_tcd_t *tcdRegs;

	if (DEV_CHANNEL_DATA(dev, channel)->busy) {
		status->busy = true;
		status->pending_length =
			EDMA_GetRemainingMajorLoopCount(DEV_BASE(dev), channel);
	} else {
		status->busy = false;
		status->pending_length = 0;
	}
	status->dir = DEV_CHANNEL_DATA(dev, channel)->transfer_settings.direction;
	LOG_DBG("DMAMUX CHCFG 0x%x", DEV_DMAMUX_BASE(dev)->CHCFG[channel]);
	LOG_DBG("DMA CR 0x%x", DEV_BASE(dev)->CR);
	LOG_DBG("DMA INT 0x%x", DEV_BASE(dev)->INT);
	LOG_DBG("DMA ERQ 0x%x", DEV_BASE(dev)->ERQ);
	LOG_DBG("DMA ES 0x%x", DEV_BASE(dev)->ES);
	LOG_DBG("DMA ERR 0x%x", DEV_BASE(dev)->ERR);
	LOG_DBG("DMA HRS 0x%x", DEV_BASE(dev)->HRS);
	tcdRegs = (edma_tcd_t *)((uint32_t)&DEV_BASE(dev)->TCD[channel]);
	LOG_DBG("data csr is 0x%x", tcdRegs->CSR);
	return 0;
}

static bool dma_mcux_edma_channel_filter(const struct device *dev,
					 int channel_id, void *param)
{
	enum dma_channel_filter *filter = (enum dma_channel_filter *)param;

	if (filter && *filter == DMA_CHANNEL_PERIODIC) {
		if (channel_id > 3) {
			return false;
		}
	}
	return true;
}

static const struct dma_driver_api dma_mcux_edma_api = {
	.reload = dma_mcux_edma_reload,
	.config = dma_mcux_edma_configure,
	.start = dma_mcux_edma_start,
	.stop = dma_mcux_edma_stop,
	.suspend = dma_mcux_edma_suspend,
	.resume = dma_mcux_edma_resume,
	.get_status = dma_mcux_edma_get_status,
	.chan_filter = dma_mcux_edma_channel_filter,
};

static int dma_mcux_edma_init(const struct device *dev)
{
	const struct dma_mcux_edma_config *config = dev->config;
	struct dma_mcux_edma_data *data = dev->data;

	edma_config_t userConfig = { 0 };

	LOG_DBG("INIT NXP EDMA");
	DMAMUX_Init(DEV_DMAMUX_BASE(dev));
	EDMA_GetDefaultConfig(&userConfig);
	EDMA_Init(DEV_BASE(dev), &userConfig);
	config->irq_config_func(dev);
	memset(dev->data, 0, sizeof(struct dma_mcux_edma_data));
	memset(tcdpool, 0, sizeof(tcdpool));
	data->dma_ctx.magic = DMA_MAGIC;
	data->dma_ctx.dma_channels = config->dma_channels;
	data->dma_ctx.atomic = data->channels_atomic;
	return 0;
}

#define IRQ_CONFIG(n, idx, fn)						     \
	IF_ENABLED(DT_INST_IRQ_HAS_IDX(n, idx), (			     \
			   IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),	     \
				       DT_INST_IRQ_BY_IDX(n, idx, priority), \
				       fn,				     \
				       DEVICE_DT_INST_GET(n), 0);	     \
			   irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));	     \
			   ))

#define DMA_MCUX_EDMA_CONFIG_FUNC(n)				      \
	static void dma_imx_config_func_##n(const struct device *dev) \
	{							      \
		ARG_UNUSED(dev);				      \
								      \
		IRQ_CONFIG(n, 0, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 1, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 2, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 3, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 4, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 5, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 6, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 7, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 8, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 9, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 10, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 11, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 12, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 13, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 14, dma_mcux_edma_irq_handler);	      \
		IRQ_CONFIG(n, 15, dma_mcux_edma_irq_handler);	      \
								      \
		IRQ_CONFIG(n, 16, dma_mcux_edma_error_irq_handler);   \
								      \
		LOG_DBG("install irq done");			      \
	}

/*
 * define the dma
 */
#define DMA_INIT(n)						       \
	static void dma_imx_config_func_##n(const struct device *dev); \
	static const struct dma_mcux_edma_config dma_config_##n = {    \
		.base = (DMA_Type *)DT_INST_REG_ADDR(n),	       \
		.dmamux_base =					       \
			(DMAMUX_Type *)DT_INST_REG_ADDR_BY_IDX(n, 1),  \
		.dma_channels = DT_INST_PROP(n, dma_channels),	       \
		.irq_config_func = dma_imx_config_func_##n,	       \
	};							       \
								       \
	struct dma_mcux_edma_data dma_data_##n;			       \
								       \
	DEVICE_DT_INST_DEFINE(n,				       \
			      &dma_mcux_edma_init, NULL,	       \
			      &dma_data_##n, &dma_config_##n,	       \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,   \
			      &dma_mcux_edma_api);		       \
								       \
	DMA_MCUX_EDMA_CONFIG_FUNC(n);

DT_INST_FOREACH_STATUS_OKAY(DMA_INIT)
