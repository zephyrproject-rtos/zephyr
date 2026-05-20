/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>

#include <fsl_gdma.h>

#define DT_DRV_COMPAT nxp_gdma

LOG_MODULE_REGISTER(dma_nxp_gdma, CONFIG_DMA_LOG_LEVEL);

/* Maximum transfer length supported by GDMA hardware (8KB - 1) */
#define NXP_GDMA_MAX_XFER_LEN (8 * 1024 - 1)

/* Channel data structure */
struct dma_nxp_gdma_channel {
	uint32_t direction;
	void *user_data;
	dma_callback_t callback;
	bool busy;
	uint32_t transfer_length;
};

/* Device configuration */
struct dma_nxp_gdma_config {
	GDMA_Type *base;
	void (*irq_config_func)(const struct device *dev);
	uint8_t num_channels;
};

/* Device data */
struct dma_nxp_gdma_data {
	struct dma_context ctx;
	struct dma_nxp_gdma_channel *channels;
};

/**
 * @brief Convert Zephyr DMA transfer width to GDMA transfer width
 */
static gdma_transfer_width_t dma_nxp_gdma_width_convert(uint32_t width)
{
	switch (width) {
	case 1:
		return kGDMA_TransferWidth1Byte;
	case 2:
		return kGDMA_TransferWidth2Byte;
	case 4:
		return kGDMA_TransferWidth4Byte;
	default:
		return kGDMA_TransferWidth1Byte;
	}
}

/**
 * @brief Convert Zephyr DMA burst size to GDMA burst size
 */
static gdma_burst_size_t dma_nxp_gdma_burst_convert(uint32_t burst)
{
	switch (burst) {
	case 1:
		return kGDMA_BurstSize1;
	case 4:
		return kGDMA_BurstSize4;
	case 8:
		return kGDMA_BurstSize8;
	case 16:
		return kGDMA_BurstSize16;
	default:
		return kGDMA_BurstSize1;
	}
}

/**
 * @brief Configure a DMA channel
 */
static int dma_nxp_gdma_configure(const struct device *dev, uint32_t channel,
				  struct dma_config *cfg)
{
	const struct dma_nxp_gdma_config *config = dev->config;
	struct dma_nxp_gdma_data *data = dev->data;
	struct dma_nxp_gdma_channel *ch_data;
	struct dma_block_config *block;

	if (channel >= config->num_channels) {
		LOG_ERR("Invalid channel %d (max %d)", channel, config->num_channels - 1);
		return -EINVAL;
	}

	ch_data = &data->channels[channel];

	if (ch_data->busy) {
		LOG_ERR("Channel %d is busy", channel);
		return -EBUSY;
	}

	if (cfg->block_count != 1) {
		LOG_ERR("Only single block transfers supported");
		return -ENOTSUP;
	}

	block = cfg->head_block;
	if (block == NULL) {
		LOG_ERR("No block configuration provided");
		return -EINVAL;
	}

	if (block->block_size > NXP_GDMA_MAX_XFER_LEN) {
		LOG_ERR("Block size %d exceeds max %d", block->block_size, NXP_GDMA_MAX_XFER_LEN);
		return -EINVAL;
	}

	/* Validate direction */
	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
	case MEMORY_TO_PERIPHERAL:
	case PERIPHERAL_TO_MEMORY:
		break;
	default:
		LOG_ERR("Unsupported direction %d", cfg->channel_direction);
		return -ENOTSUP;
	}

	/* Store channel configuration */
	ch_data->direction = cfg->channel_direction;
	ch_data->callback = cfg->dma_callback;
	ch_data->user_data = cfg->user_data;
	ch_data->transfer_length = block->block_size;

	/* Set channel priority */
	GDMA_SetChannelPriority(config->base, channel, (gdma_priority_t)cfg->channel_priority);

	/* Prepare transfer configuration */
	gdma_channel_xfer_config_t xfer_cfg = {
		.srcAddr = block->source_address,
		.destAddr = block->dest_address,
		.ahbProt = kGDMA_ProtPrevilegedMode,
		.srcBurstSize = dma_nxp_gdma_burst_convert(cfg->source_burst_length),
		.destBurstSize = dma_nxp_gdma_burst_convert(cfg->dest_burst_length),
		.srcWidth = dma_nxp_gdma_width_convert(cfg->source_data_size),
		.destWidth = dma_nxp_gdma_width_convert(cfg->dest_data_size),
		.srcAddrInc = (block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT),
		.destAddrInc = (block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT),
		.transferLen = block->block_size,
		.enableLinkList = false,
		.enableDescInterrupt = false,
		.stopAfterDescFinished = true,
		.linkListAddr = 0,
	};

	/* Set transfer configuration directly */
	GDMA_SetChannelTransferConfig(config->base, channel, &xfer_cfg);

	/* Enable transfer done and error interrupts */
	GDMA_EnableChannelInterrupts(config->base, channel,
				     kGDMA_TransferDoneInterruptEnable |
				     kGDMA_AddressErrorInterruptEnable |
				     kGDMA_BusErrorInterruptEnable);

	return 0;
}

/**
 * @brief Start a DMA transfer
 */
static int dma_nxp_gdma_start(const struct device *dev, uint32_t channel)
{
	const struct dma_nxp_gdma_config *config = dev->config;
	struct dma_nxp_gdma_data *data = dev->data;
	struct dma_nxp_gdma_channel *ch_data;

	if (channel >= config->num_channels) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	ch_data = &data->channels[channel];

	if (ch_data->busy) {
		LOG_ERR("Channel %d is already busy", channel);
		return -EBUSY;
	}

	ch_data->busy = true;

	/* Start the transfer */
	GDMA_StartChannel(config->base, channel);

	return 0;
}

/**
 * @brief Stop a DMA transfer
 */
static int dma_nxp_gdma_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_nxp_gdma_config *config = dev->config;
	struct dma_nxp_gdma_data *data = dev->data;
	struct dma_nxp_gdma_channel *ch_data;

	if (channel >= config->num_channels) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	ch_data = &data->channels[channel];

	/* Stop the channel */
	GDMA_StopChannel(config->base, channel);

	/* Wait for channel to stop */
	if (!WAIT_FOR(!GDMA_IsChannelBusy(config->base, channel), 1000, k_busy_wait(1))) {
		LOG_WRN("Channel %d did not stop in time", channel);
	}

	ch_data->busy = false;

	return 0;
}

/**
 * @brief Get DMA channel status
 */
static int dma_nxp_gdma_get_status(const struct device *dev, uint32_t channel,
				   struct dma_status *status)
{
	const struct dma_nxp_gdma_config *config = dev->config;
	struct dma_nxp_gdma_data *data = dev->data;
	struct dma_nxp_gdma_channel *ch_data;

	if (channel >= config->num_channels) {
		LOG_ERR("Invalid channel %d", channel);
		return -EINVAL;
	}

	ch_data = &data->channels[channel];

	status->busy = GDMA_IsChannelBusy(config->base, channel);
	status->dir = ch_data->direction;
	/* If channel is busy, pending_length is the total transfer size;
	 * if not busy, pending_length is 0
	 */
	status->pending_length = status->busy ? ch_data->transfer_length : 0;
	status->total_copied = 0;   /* Not easily available from hardware */

	return 0;
}

/**
 * @brief Channel filter function for dma_request_channel
 */
static bool dma_nxp_gdma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	const struct dma_nxp_gdma_config *config = dev->config;
	struct dma_nxp_gdma_data *data = dev->data;

	if (channel >= config->num_channels) {
		return false;
	}

	if (data->channels[channel].busy) {
		return false;
	}

	if (filter_param != NULL && *((uint32_t *)filter_param) != channel) {
		return false;
	}

	return true;
}

/**
 * @brief GDMA interrupt service routine
 */
static void dma_nxp_gdma_isr(const struct device *dev)
{
	const struct dma_nxp_gdma_config *config = dev->config;
	struct dma_nxp_gdma_data *data = dev->data;

	for (uint8_t channel = 0; channel < config->num_channels; channel++) {
		uint32_t flags = GDMA_GetChannelInterruptFlags(config->base, channel);

		if (flags == 0) {
			continue;
		}

		/* Clear interrupt flags */
		GDMA_ClearChannelInterruptFlags(config->base, channel, flags);

		/* Handle the interrupt directly */
		struct dma_nxp_gdma_channel *ch_data = &data->channels[channel];
		int status = DMA_STATUS_COMPLETE;

		/* Check for errors */
		if (flags & (kGDMA_AddressErrorFlag | kGDMA_BusErrorFlag)) {
			LOG_ERR("GDMA channel %d transfer error, flags: 0x%08x", channel, flags);
			status = -EIO;
		}

		ch_data->busy = false;

		if (ch_data->callback) {
			ch_data->callback(dev, ch_data->user_data, channel, status);
		}
	}
}

/**
 * @brief Initialize the DMA controller
 */
static int dma_nxp_gdma_init(const struct device *dev)
{
	const struct dma_nxp_gdma_config *config = dev->config;

	/* Initialize GDMA hardware */
	GDMA_Init(config->base);

	/* Configure and enable interrupts */
	config->irq_config_func(dev);

	LOG_INF("NXP GDMA initialized with %d channels", config->num_channels);

	return 0;
}

static DEVICE_API(dma, dma_nxp_gdma_api) = {
	.config = dma_nxp_gdma_configure,
	.start = dma_nxp_gdma_start,
	.stop = dma_nxp_gdma_stop,
	.get_status = dma_nxp_gdma_get_status,
	.chan_filter = dma_nxp_gdma_chan_filter,
};

#define DMA_NXP_GDMA_IRQ_CONFIG(n)                                                                 \
	static void dma_nxp_gdma_irq_config_##n(const struct device *dev)                          \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),                             \
			    dma_nxp_gdma_isr,                                                      \
			    DEVICE_DT_INST_GET(n), 0);                                             \
		irq_enable(DT_INST_IRQN(n));                                                       \
	}

#define DMA_NXP_GDMA_INIT(n)                                                                       \
	DMA_NXP_GDMA_IRQ_CONFIG(n)                                                                 \
	static struct dma_nxp_gdma_channel                                                         \
		dma_nxp_gdma_channels_##n[DT_INST_PROP(n, dma_channels)];                          \
	ATOMIC_DEFINE(dma_nxp_gdma_atomic_##n, DT_INST_PROP(n, dma_channels));                     \
	static struct dma_nxp_gdma_data dma_nxp_gdma_data_##n = {                                  \
		.ctx =                                                                             \
			{                                                                          \
				.magic = DMA_MAGIC,                                                \
				.dma_channels = DT_INST_PROP(n, dma_channels),                     \
				.atomic = dma_nxp_gdma_atomic_##n,                                 \
			},                                                                         \
		.channels = dma_nxp_gdma_channels_##n,                                             \
	};                                                                                         \
	static const struct dma_nxp_gdma_config dma_nxp_gdma_config_##n = {                        \
		.base = (GDMA_Type *)DT_INST_REG_ADDR(n),                                          \
		.irq_config_func = dma_nxp_gdma_irq_config_##n,                                    \
		.num_channels = DT_INST_PROP(n, dma_channels),                                     \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, dma_nxp_gdma_init, NULL, &dma_nxp_gdma_data_##n,                  \
			      &dma_nxp_gdma_config_##n, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,    \
			      &dma_nxp_gdma_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_NXP_GDMA_INIT)
