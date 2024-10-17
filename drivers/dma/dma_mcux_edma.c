/*
 * Copyright 2020-23 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Common part of DMA drivers for imx rt series.
 */

#define DT_DRV_COMPAT nxp_mcux_edma

#include <zephyr/drivers/dma.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/irq.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>
#include <errno.h>
#include <soc.h>

#include <fsl_edma.h>

#if defined(FSL_FEATURE_SOC_DMAMUX_COUNT) && FSL_FEATURE_SOC_DMAMUX_COUNT
#include <fsl_dmamux.h>
#define SOC_HAS_DMAMUX 1
#endif

#define DEV_CFG(dev) \
	((const struct dma_mcux_edma_config *const)dev->config)
#define DEV_DATA(dev) ((struct dma_mcux_edma_data *)dev->data)
#define DEV_BASE(dev) ((DMA_Type *)DEV_CFG(dev)->base)

#define DEV_CHANNEL_DATA(dev, ch) \
	((struct call_back *)(&(DEV_DATA(dev)->data_cb[ch])))

#define DEV_EDMA_HANDLE(dev, ch) \
	((edma_handle_t *)(&(DEV_CHANNEL_DATA(dev, ch)->edma_handle)))

#ifdef SOC_HAS_DMAMUX
#define DEV_DMAMUX_BASE(dev, idx) ((DMAMUX_Type *)DEV_CFG(dev)->dmamux_base[idx])
#define DEV_DMAMUX_IDX(dev, ch)	(ch / DEV_CFG(dev)->channels_per_mux)
#define DEV_DMAMUX_CHANNEL(dev, ch) \
	(ch % DEV_CFG(dev)->channels_per_mux) ^ (DEV_CFG(dev)->dmamux_reg_offset)
#endif

#define HAS_CHANNEL_GAP(n)		DT_INST_NODE_HAS_PROP(n, channel_gap) ||
#define DMA_MCUX_HAS_CHANNEL_GAP	(DT_INST_FOREACH_STATUS_OKAY(HAS_CHANNEL_GAP) 0)

#define HAS_ERROR_IRQ UTIL_NOT(DT_INST_PROP(0, no_error_irq))

#ifndef CONFIG_HAS_MCUX_CACHE
/* if no cache on part, no need to worry about noncache for TCD */
#define EDMA_TCDPOOL_CACHE_ATTR
#elif defined(CONFIG_DMA_MCUX_USE_DTCM_FOR_DMA_DESCRIPTORS)
#define EDMA_TCDPOOL_CACHE_ATTR __dtcm_noinit_section
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
#warning EDMA TCD pool might not be in noncacheable memory
#define EDMA_TCDPOOL_CACHE_ATTR
#endif /* CONFIG_HAS_MCUX_CACHE */

LOG_MODULE_REGISTER(dma_mcux_edma, CONFIG_DMA_LOG_LEVEL);

struct dma_mcux_edma_config {
	DMA_Type *base;
#ifdef SOC_HAS_DMAMUX
	DMAMUX_Type **dmamux_base;
#endif
	void (*irq_config_func)(const struct device *dev);
#if DMA_MCUX_HAS_CHANNEL_GAP
	uint32_t channel_gap[2];
#endif
	uint16_t dma_channels; /* number of channels */
#if SOC_HAS_DMAMUX
	uint8_t channels_per_mux;
	uint8_t dmamux_reg_offset;
#endif
};

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

#if DMA_MCUX_HAS_CHANNEL_GAP
/*
 * The hardware channel (takes the gap into account) is used when access DMA registers.
 * For data structures in the shim driver still use the primitive channel.
 */
static ALWAYS_INLINE uint32_t dma_mcux_edma_add_channel_gap(const struct device *dev,
							    uint32_t channel)
{
	const struct dma_mcux_edma_config *config = DEV_CFG(dev);

	return (channel < config->channel_gap[0]) ? channel :
		(channel + 1 + config->channel_gap[1] - config->channel_gap[0]);
}

static ALWAYS_INLINE uint32_t dma_mcux_edma_remove_channel_gap(const struct device *dev,
								uint32_t channel)
{
	const struct dma_mcux_edma_config *config = DEV_CFG(dev);

	return (channel < config->channel_gap[0]) ? channel :
		(channel + config->channel_gap[0] - config->channel_gap[1] - 1);
}
#else
#define dma_mcux_edma_add_channel_gap(dev, channel) channel
#define dma_mcux_edma_remove_channel_gap(dev, channel) channel
#endif /* DMA_MCUX_HAS_CHANNEL_GAP */

static bool data_size_valid(const size_t data_size)
{
	bool valid = (data_size == 4U || data_size == 2U ||
		data_size == 1U || data_size == 8U ||
		data_size == 16U || data_size == 32U);

	if (data_size == 64U &&
	    (IS_ENABLED(CONFIG_DMA_MCUX_EDMA_V3) ||
	    IS_ENABLED(CONFIG_DMA_MCUX_EDMA_V4))) {
		valid = true;
	}

	return valid;
}

static void nxp_edma_callback(edma_handle_t *handle, void *param, bool transferDone,
			      uint32_t tcds)
{
	int ret = -EIO;
	struct call_back *data = (struct call_back *)param;

	uint32_t channel = dma_mcux_edma_remove_channel_gap(data->dev, handle->channel);

	if (transferDone) {
		/* DMA is no longer busy when there are no remaining TCDs to transfer */
		data->busy = (handle->tcdPool != NULL) && (handle->tcdUsed > 0);
		ret = DMA_STATUS_COMPLETE;
	}
	LOG_DBG("transfer %d", tcds);
	data->dma_callback(data->dev, data->user_data, channel, ret);
}

static void dma_mcux_edma_irq_handler(const struct device *dev, uint32_t channel)
{
	uint32_t hw_channel = dma_mcux_edma_add_channel_gap(dev, channel);
	uint32_t flag = EDMA_GetChannelStatusFlags(DEV_BASE(dev), hw_channel);

	if (flag & kEDMA_InterruptFlag) {
		LOG_DBG("IRQ OCCURRED");
		/* EDMA interrupt flag is cleared here */
		EDMA_HandleIRQ(DEV_EDMA_HANDLE(dev, channel));
		LOG_DBG("IRQ DONE");
	}

	else if (!HAS_ERROR_IRQ && (flag & kEDMA_ErrorFlag)) {
	/* Channel shares the same irq for error and transfer complete */
		EDMA_ClearChannelStatusFlags(DEV_BASE(dev), channel, 0xFFFFFFFF);
		EDMA_AbortTransfer(DEV_EDMA_HANDLE(dev, channel));
		DEV_CHANNEL_DATA(dev, channel)->busy = false;
		LOG_INF("channel %d error status is 0x%x", channel, flag);
	}
}

#if HAS_ERROR_IRQ
static void dma_mcux_edma_error_irq_handler(const struct device *dev)
{
	int i = 0;
	uint32_t flag = 0;
	uint32_t hw_channel;

	for (i = 0; i < DEV_CFG(dev)->dma_channels; i++) {
		if (DEV_CHANNEL_DATA(dev, i)->busy) {
			hw_channel = dma_mcux_edma_add_channel_gap(dev, i);
			flag = EDMA_GetChannelStatusFlags(DEV_BASE(dev), hw_channel);
			EDMA_ClearChannelStatusFlags(DEV_BASE(dev), hw_channel, 0xFFFFFFFF);
			EDMA_AbortTransfer(DEV_EDMA_HANDLE(dev, i));
			DEV_CHANNEL_DATA(dev, i)->busy = false;
			LOG_INF("channel %d error status is 0x%x", hw_channel, flag);
		}
	}

	if (IS_ENABLED(CONFIG_CPU_CORTEX_M4)) {
		barrier_dsync_fence_full();
	}
}
#endif /* HAS_ERROR_IRQ */

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
	uint32_t hw_channel;
	edma_transfer_type_t transfer_type;
	unsigned int key;
	int ret = 0;

	if (slot >= DT_INST_PROP(0, dma_requests)) {
		LOG_ERR("source number is out of scope %d", slot);
		return -ENOTSUP;
	}

	if (channel >= DT_INST_PROP(0, dma_channels)) {
		LOG_ERR("out of DMA channel %d", channel);
		return -EINVAL;
	}

	hw_channel = dma_mcux_edma_add_channel_gap(dev, channel);

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

#ifdef SOC_HAS_DMAMUX
	uint8_t dmamux_idx, dmamux_channel;

	dmamux_idx = DEV_DMAMUX_IDX(dev, channel);
	dmamux_channel = DEV_DMAMUX_CHANNEL(dev, channel);

#if DT_INST_PROP(0, nxp_a_on)
	if (config->source_handshake || config->dest_handshake ||
	    transfer_type == kEDMA_MemoryToMemory) {
		/*software trigger make the channel always on*/
		LOG_DBG("ALWAYS ON");
		DMAMUX_EnableAlwaysOn(DEV_DMAMUX_BASE(dev, dmamux_idx), dmamux_channel, true);
	}
#endif /* DT_INST_HAS_PROP(0, nxp_a_on) */

	DMAMUX_SetSource(DEV_DMAMUX_BASE(dev, dmamux_idx), dmamux_channel, slot);

	/* dam_imx_rt_set_channel_priority(dev, channel, config); */
	DMAMUX_EnableChannel(DEV_DMAMUX_BASE(dev, dmamux_idx), dmamux_channel);
#endif /* SOC_HAS_DMAMUX */

	if (data->busy) {
		EDMA_AbortTransfer(p_handle);
	}
	EDMA_ResetChannel(DEV_BASE(dev), hw_channel);
	EDMA_CreateHandle(p_handle, DEV_BASE(dev), hw_channel);
	EDMA_SetCallback(p_handle, nxp_edma_callback, (void *)data);

#if defined(FSL_FEATURE_EDMA_HAS_CHANNEL_MUX) && FSL_FEATURE_EDMA_HAS_CHANNEL_MUX
	/* First release any peripheral previously associated with this channel */
	EDMA_SetChannelMux(DEV_BASE(dev), hw_channel, 0);
	EDMA_SetChannelMux(DEV_BASE(dev), hw_channel, slot);
#endif

	LOG_DBG("channel is %d", channel);
	EDMA_EnableChannelInterrupts(DEV_BASE(dev), hw_channel, kEDMA_ErrorInterruptEnable);

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
#if defined(CONFIG_DMA_MCUX_EDMA_V3) || defined(CONFIG_DMA_MCUX_EDMA_V4)
		LOG_DBG("DMA TCD_CSR 0x%x", DEV_BASE(dev)->CH[hw_channel].TCD_CSR);
#else
		LOG_DBG("data csr is 0x%x", DEV_BASE(dev)->TCD[hw_channel].CSR);
#endif
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

#if !defined(CONFIG_DMA_MCUX_EDMA_V3) && !defined(CONFIG_DMA_MCUX_EDMA_V4)
	LOG_DBG("DMA CR 0x%x", DEV_BASE(dev)->CR);
#endif
	data->busy = true;
	EDMA_StartTransfer(DEV_EDMA_HANDLE(dev, channel));
	return 0;
}

static int dma_mcux_edma_stop(const struct device *dev, uint32_t channel)
{
	struct dma_mcux_edma_data *data = DEV_DATA(dev);
	uint32_t hw_channel;

	hw_channel = dma_mcux_edma_add_channel_gap(dev, channel);

	data->data_cb[channel].transfer_settings.valid = false;

	if (!data->data_cb[channel].busy) {
		return 0;
	}

	EDMA_AbortTransfer(DEV_EDMA_HANDLE(dev, channel));
	EDMA_ClearChannelStatusFlags(DEV_BASE(dev), hw_channel,
				     kEDMA_DoneFlag | kEDMA_ErrorFlag |
				     kEDMA_InterruptFlag);
	EDMA_ResetChannel(DEV_BASE(dev), hw_channel);
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
	uint32_t hw_channel = dma_mcux_edma_add_channel_gap(dev, channel);

	if (DEV_CHANNEL_DATA(dev, channel)->busy) {
		status->busy = true;
		status->pending_length =
			EDMA_GetRemainingMajorLoopCount(DEV_BASE(dev), hw_channel);
	} else {
		status->busy = false;
		status->pending_length = 0;
	}
	status->dir = DEV_CHANNEL_DATA(dev, channel)->transfer_settings.direction;

#if defined(CONFIG_DMA_MCUX_EDMA_V3) || defined(CONFIG_DMA_MCUX_EDMA_V4)
	LOG_DBG("DMA MP_CSR 0x%x",  DEV_BASE(dev)->MP_CSR);
	LOG_DBG("DMA MP_ES 0x%x",   DEV_BASE(dev)->MP_ES);
	LOG_DBG("DMA CHx_ES 0x%x",  DEV_BASE(dev)->CH[hw_channel].CH_ES);
	LOG_DBG("DMA CHx_CSR 0x%x", DEV_BASE(dev)->CH[hw_channel].CH_CSR);
	LOG_DBG("DMA CHx_ES 0x%x",  DEV_BASE(dev)->CH[hw_channel].CH_ES);
	LOG_DBG("DMA CHx_INT 0x%x", DEV_BASE(dev)->CH[hw_channel].CH_INT);
	LOG_DBG("DMA TCD_CSR 0x%x", DEV_BASE(dev)->CH[hw_channel].TCD_CSR);
#else
	LOG_DBG("DMA CR 0x%x", DEV_BASE(dev)->CR);
	LOG_DBG("DMA INT 0x%x", DEV_BASE(dev)->INT);
	LOG_DBG("DMA ERQ 0x%x", DEV_BASE(dev)->ERQ);
	LOG_DBG("DMA ES 0x%x", DEV_BASE(dev)->ES);
	LOG_DBG("DMA ERR 0x%x", DEV_BASE(dev)->ERR);
	LOG_DBG("DMA HRS 0x%x", DEV_BASE(dev)->HRS);
	LOG_DBG("data csr is 0x%x", DEV_BASE(dev)->TCD[hw_channel].CSR);
#endif
	return 0;
}

static bool dma_mcux_edma_channel_filter(const struct device *dev,
					 int channel_id, void *param)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(channel_id);
	ARG_UNUSED(param);

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

#ifdef SOC_HAS_DMAMUX
	for (uint8_t i = 0; i < config->dma_channels / config->channels_per_mux; i++) {
		DMAMUX_Init(DEV_DMAMUX_BASE(dev, i));
	}
#endif

	EDMA_GetDefaultConfig(&userConfig);
	EDMA_Init(DEV_BASE(dev), &userConfig);
#ifdef CONFIG_DMA_MCUX_EDMA_V3
	/* Channel linking available and will be controlled by each channel's link settings */
	EDMA_EnableAllChannelLink(DEV_BASE(dev), true);
#endif
	config->irq_config_func(dev);
	memset(dev->data, 0, sizeof(struct dma_mcux_edma_data));
	memset(tcdpool, 0, sizeof(tcdpool));
	data->dma_ctx.magic = DMA_MAGIC;
	data->dma_ctx.dma_channels = config->dma_channels;
	data->dma_ctx.atomic = data->channels_atomic;
	return 0;
}

/* The shared error interrupt (if have) must be declared as the last element in devicetree */
#if HAS_ERROR_IRQ
#define NUM_IRQS_WITHOUT_ERROR_IRQ(n)	UTIL_DEC(DT_NUM_IRQS(DT_DRV_INST(n)))
#else
#define NUM_IRQS_WITHOUT_ERROR_IRQ(n)	DT_NUM_IRQS(DT_DRV_INST(n))
#endif

#define IRQ_CONFIG(n, idx, fn)							\
	{									\
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(n, idx, irq),			\
			    DT_INST_IRQ_BY_IDX(n, idx, priority),		\
			    fn,							\
			    DEVICE_DT_INST_GET(n), 0);				\
			    irq_enable(DT_INST_IRQ_BY_IDX(n, idx, irq));	\
	}

#define DMA_MCUX_EDMA_IRQ_DEFINE(idx, n)					\
	static void dma_mcux_edma_##n##_irq_##idx(const struct device *dev)	\
	{									\
		dma_mcux_edma_irq_handler(dev, idx);				\
										\
		IF_ENABLED(UTIL_BOOL(DT_INST_PROP(n, irq_shared_offset)),	\
			  (dma_mcux_edma_irq_handler(dev,			\
			   idx + DT_INST_PROP(n, irq_shared_offset));))		\
										\
		IF_ENABLED(CONFIG_CPU_CORTEX_M4, (barrier_dsync_fence_full();))	\
	}

#define DMA_MCUX_EDMA_IRQ_CONFIG(idx, n)					\
	IRQ_CONFIG(n, idx, dma_mcux_edma_##n##_irq_##idx)

#define DMA_MCUX_EDMA_CONFIG_FUNC(n)						\
	LISTIFY(NUM_IRQS_WITHOUT_ERROR_IRQ(n), DMA_MCUX_EDMA_IRQ_DEFINE, (), n) \
	static void dma_imx_config_func_##n(const struct device *dev)		\
	{									\
		ARG_UNUSED(dev);						\
										\
		LISTIFY(NUM_IRQS_WITHOUT_ERROR_IRQ(n),				\
			DMA_MCUX_EDMA_IRQ_CONFIG, (;), n)			\
										\
		IF_ENABLED(HAS_ERROR_IRQ,					\
			   (IRQ_CONFIG(n, NUM_IRQS_WITHOUT_ERROR_IRQ(n),	\
			    dma_mcux_edma_error_irq_handler)))			\
										\
		LOG_DBG("install irq done");					\
	}

#if DMA_MCUX_HAS_CHANNEL_GAP
#define DMA_MCUX_EDMA_CHANNEL_GAP(n)						\
	.channel_gap = DT_INST_PROP_OR(n, channel_gap,				\
				{[0 ... 1] = DT_INST_PROP(n, dma_channels)}),
#else
#define DMA_MCUX_EDMA_CHANNEL_GAP(n)
#endif

#ifdef SOC_HAS_DMAMUX
#define DMA_MCUX_EDMA_MUX(idx, n)						\
	(DMAMUX_Type *)DT_INST_REG_ADDR_BY_IDX(n, UTIL_INC(idx))
#define DMAMUX_BASE_INIT_DEFINE(n)						\
	static DMAMUX_Type *dmamux_base_##n[] = {				\
		LISTIFY(UTIL_DEC(DT_NUM_REGS(DT_DRV_INST(n))),			\
			DMA_MCUX_EDMA_MUX, (,), n)				\
	};
#define DMAMUX_BASE_INIT(n) .dmamux_base = &dmamux_base_##n[0],
#define CHANNELS_PER_MUX(n) .channels_per_mux = DT_INST_PROP(n, dma_channels) /	\
						ARRAY_SIZE(dmamux_base_##n),
#define DMAMUX_REG_OFFSET(n) .dmamux_reg_offset = DT_INST_PROP(n, dmamux_reg_offset),
#else
#define DMAMUX_BASE_INIT_DEFINE(n)
#define DMAMUX_BASE_INIT(n)
#define CHANNELS_PER_MUX(n)
#define DMAMUX_REG_OFFSET(n)
#endif /* SOC_HAS_DMAMUX */

#define DMA_INIT(n)								\
	DMAMUX_BASE_INIT_DEFINE(n)						\
	DMA_MCUX_EDMA_CONFIG_FUNC(n)						\
	static const struct dma_mcux_edma_config dma_config_##n = {		\
		.base = (DMA_Type *)DT_INST_REG_ADDR(n),			\
		.dma_channels = (uint16_t)DT_INST_PROP(n, dma_channels),	\
		.irq_config_func = dma_imx_config_func_##n,			\
		DMAMUX_BASE_INIT(n)						\
		CHANNELS_PER_MUX(n)						\
		DMAMUX_REG_OFFSET(n)						\
		DMA_MCUX_EDMA_CHANNEL_GAP(n)					\
	};									\
										\
	struct dma_mcux_edma_data dma_data_##n;					\
										\
	DEVICE_DT_INST_DEFINE(n,						\
			      &dma_mcux_edma_init, NULL,			\
			      &dma_data_##n, &dma_config_##n,			\
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,		\
			      &dma_mcux_edma_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_INIT)
