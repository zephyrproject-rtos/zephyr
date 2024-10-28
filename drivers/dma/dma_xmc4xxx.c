/*
 * Copyright (c) 2022 Andriy Gelman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT infineon_xmc4xxx_dma

#include <soc.h>
#include <stdint.h>
#include <xmc_dma.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/dt-bindings/dma/infineon-xmc4xxx-dma.h>
#include <zephyr/irq.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_xmc4xxx, CONFIG_DMA_LOG_LEVEL);

#define MAX_PRIORITY	  7
#define DMA_MAX_BLOCK_LEN 4095
#define DLR_LINE_UNSET	  0xff

#define DLR_SRSEL_RS_BITSIZE 4
#define DLR_SRSEL_RS_MSK     0xf

#define ALL_EVENTS                                                                                 \
	(XMC_DMA_CH_EVENT_TRANSFER_COMPLETE | XMC_DMA_CH_EVENT_BLOCK_TRANSFER_COMPLETE |           \
	 XMC_DMA_CH_EVENT_SRC_TRANSACTION_COMPLETE | XMC_DMA_CH_EVENT_DST_TRANSACTION_COMPLETE |   \
	 XMC_DMA_CH_EVENT_ERROR)

struct dma_xmc4xxx_channel {
	dma_callback_t cb;
	void *user_data;
	uint16_t block_ts;
	uint8_t source_data_size;
	uint8_t dlr_line;
};

struct dma_xmc4xxx_config {
	XMC_DMA_t *dma;
	void (*irq_configure)(void);
};

struct dma_xmc4xxx_data {
	struct dma_context ctx;
	struct dma_xmc4xxx_channel *channels;
};

#define HANDLE_EVENT(event_test, get_channels_event, ret)                                  \
do {                                                                                       \
	if (event & (XMC_DMA_CH_##event_test)) {                                           \
		uint32_t channels_event = get_channels_event(dma);                         \
		int channel = find_lsb_set(channels_event) - 1;                            \
		struct dma_xmc4xxx_channel *dma_channel;                                   \
											   \
		__ASSERT_NO_MSG(channel >= 0);                                             \
		dma_channel = &dev_data->channels[channel];                                \
		/* Event has to be cleared before callback. The callback may call */       \
		/* dma_start() and re-enable the event */                                  \
		XMC_DMA_CH_ClearEventStatus(dma, channel, XMC_DMA_CH_##event_test);        \
		if (dma_channel->cb) {                                                     \
			dma_channel->cb(dev, dma_channel->user_data, channel, (ret));      \
		}                                                                          \
}                                                                                          \
} while (0)

/* Isr is level triggered, so we don't have to loop over all the channels */
/* in a single call */
static void dma_xmc4xxx_isr(const struct device *dev)
{
	struct dma_xmc4xxx_data *dev_data = dev->data;
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;
	int num_dma_channels = dev_data->ctx.dma_channels;
	XMC_DMA_t *dma = dev_cfg->dma;
	uint32_t event;
	uint32_t sr_overruns;

	/* There are two types of possible DMA error events: */
	/* 1. Error response from AHB slave on the HRESP bus during DMA transfer. */
	/*    Treat this as EPERM error. */
	/* 2. Service request overruns on the DLR line. */
	/*    Treat this EIO error. */

	event = XMC_DMA_GetEventStatus(dma);
	HANDLE_EVENT(EVENT_ERROR, XMC_DMA_GetChannelsErrorStatus, -EPERM);
	HANDLE_EVENT(EVENT_BLOCK_TRANSFER_COMPLETE, XMC_DMA_GetChannelsBlockCompleteStatus, 0);
	HANDLE_EVENT(EVENT_TRANSFER_COMPLETE, XMC_DMA_GetChannelsTransferCompleteStatus, 0);

	sr_overruns = DLR->OVRSTAT;

	if (sr_overruns == 0) {
		return;
	}

	/* clear the overruns */
	DLR->OVRCLR = sr_overruns;

	/* notify about overruns */
	for (int i = 0; i < num_dma_channels; i++) {
		struct dma_xmc4xxx_channel *dma_channel;

		dma_channel = &dev_data->channels[i];
		if (dma_channel->dlr_line != DLR_LINE_UNSET &&
		    sr_overruns & BIT(dma_channel->dlr_line)) {

			/* From XMC4700/4800 reference documentation - Section 4.4.1 */
			/* Once the overrun condition is entered the user can clear the */
			/* overrun status bits by writing to the DLR_OVRCLR register. */
			/* Additionally the pending request must be reset by successively */
			/* disabling and enabling the respective line. */
			DLR->LNEN &= ~BIT(dma_channel->dlr_line);
			DLR->LNEN |= BIT(dma_channel->dlr_line);

			LOG_ERR("Overruns detected on channel %d", i);
			if (dma_channel->cb != NULL) {
				dma_channel->cb(dev, dma_channel->user_data, i, -EIO);
			}
		}
	}
}

static int dma_xmc4xxx_config(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	struct dma_xmc4xxx_data *dev_data = dev->data;
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;
	struct dma_block_config *block = config->head_block;
	XMC_DMA_t *dma = dev_cfg->dma;
	uint8_t dlr_line = DLR_LINE_UNSET;

	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("Invalid channel number");
		return -EINVAL;
	}

	if (config->channel_priority > MAX_PRIORITY) {
		LOG_ERR("Invalid priority");
		return -EINVAL;
	}

	if (config->source_chaining_en || config->dest_chaining_en) {
		LOG_ERR("Channel chaining is not supported");
		return -EINVAL;
	}

	if (config->channel_direction != MEMORY_TO_MEMORY &&
	    config->channel_direction != MEMORY_TO_PERIPHERAL &&
	    config->channel_direction != PERIPHERAL_TO_MEMORY) {
		LOG_ERR("Unsupported channel direction");
		return -EINVAL;
	}

	if (config->block_count != 1) {
		LOG_ERR("Invalid block count");
		return -EINVAL;
	}

	if (block->source_gather_en || block->dest_scatter_en) {
		if (dma != XMC_DMA0 || channel >= 2) {
			LOG_ERR("Gather/scatter only supported on DMA0 on ch0 and ch1");
			return -EINVAL;
		}
	}

	if (config->dest_data_size != 1 && config->dest_data_size != 2 &&
	    config->dest_data_size != 4) {
		LOG_ERR("Invalid dest size, Only 1,2,4 bytes supported");
		return -EINVAL;
	}

	if (config->source_data_size != 1 && config->source_data_size != 2 &&
	    config->source_data_size != 4) {
		LOG_ERR("Invalid source size, Only 1,2,4 bytes supported");
		return -EINVAL;
	}

	if (config->source_burst_length != 1 && config->source_burst_length != 4 &&
	    config->source_burst_length != 8) {
		LOG_ERR("Invalid src burst length (data size units). Only 1,4,8 units supported");
		return -EINVAL;
	}

	if (config->dest_burst_length != 1 && config->dest_burst_length != 4 &&
	    config->dest_burst_length != 8) {
		LOG_ERR("Invalid dest burst length (data size units). Only 1,4,8 units supported");
		return -EINVAL;
	}

	if (block->block_size / config->source_data_size > DMA_MAX_BLOCK_LEN) {
		LOG_ERR("Block transactions must be <= 4095");
		return -EINVAL;
	}

	if (XMC_DMA_CH_IsEnabled(dma, channel)) {
		LOG_ERR("Channel is still active");
		return -EINVAL;
	}

	XMC_DMA_CH_ClearEventStatus(dma, channel, ALL_EVENTS);

	/* check dma slot number */
	dma->CH[channel].SAR = block->source_address;
	dma->CH[channel].DAR = block->dest_address;
	dma->CH[channel].LLP = 0;

	/* set number of transactions */
	dma->CH[channel].CTLH = block->block_size / config->source_data_size;
	/* set priority and software handshaking for src/dst. if hardware hankshaking is used */
	/* it will be enabled later in the code */
	dma->CH[channel].CFGL = (config->channel_priority << GPDMA0_CH_CFGL_CH_PRIOR_Pos) |
				GPDMA0_CH_CFGL_HS_SEL_SRC_Msk | GPDMA0_CH_CFGL_HS_SEL_DST_Msk;

	dma->CH[channel].CTLL = config->dest_data_size / 2 << GPDMA0_CH_CTLL_DST_TR_WIDTH_Pos |
				config->source_data_size / 2 << GPDMA0_CH_CTLL_SRC_TR_WIDTH_Pos |
				block->dest_addr_adj << GPDMA0_CH_CTLL_DINC_Pos |
				block->source_addr_adj << GPDMA0_CH_CTLL_SINC_Pos |
				config->dest_burst_length / 4 << GPDMA0_CH_CTLL_DEST_MSIZE_Pos |
				config->source_burst_length / 4 << GPDMA0_CH_CTLL_SRC_MSIZE_Pos |
				BIT(GPDMA0_CH_CTLL_INT_EN_Pos);

	if (config->channel_direction == MEMORY_TO_PERIPHERAL ||
	    config->channel_direction == PERIPHERAL_TO_MEMORY) {
		uint8_t request_source = XMC4XXX_DMA_GET_REQUEST_SOURCE(config->dma_slot);
		uint8_t dlr_line_reg = XMC4XXX_DMA_GET_LINE(config->dma_slot);

		dlr_line = dlr_line_reg;
		if (dma == XMC_DMA0 && dlr_line > 7) {
			LOG_ERR("Unsupported request line %d for DMA0."
					"Should be in range [0,7]", dlr_line);
			return -EINVAL;
		}

		if (dma == XMC_DMA1 && (dlr_line < 8 || dlr_line > 11)) {
			LOG_ERR("Unsupported request line %d for DMA1."
					"Should be in range [8,11]", dlr_line);
			return -EINVAL;
		}

		/* clear any overruns */
		DLR->OVRCLR = BIT(dlr_line);
		/* enable the dma line */
		DLR->LNEN &= ~BIT(dlr_line);
		DLR->LNEN |= BIT(dlr_line);

		/* connect DMA Line to SR */
		if (dma == XMC_DMA0) {
			DLR->SRSEL0 &= ~(DLR_SRSEL_RS_MSK << (dlr_line_reg * DLR_SRSEL_RS_BITSIZE));
			DLR->SRSEL0 |= request_source << (dlr_line_reg * DLR_SRSEL_RS_BITSIZE);
		}

		if (dma == XMC_DMA1) {
			dlr_line_reg -= 8;
			DLR->SRSEL1 &= ~(DLR_SRSEL_RS_MSK << (dlr_line_reg * DLR_SRSEL_RS_BITSIZE));
			DLR->SRSEL1 |= request_source << (dlr_line_reg * DLR_SRSEL_RS_BITSIZE);
		}

		/* connect DMA channel to DMA line */
		if (config->channel_direction == MEMORY_TO_PERIPHERAL) {
			dma->CH[channel].CFGH = (dlr_line_reg << GPDMA0_CH_CFGH_DEST_PER_Pos) | 4;
			dma->CH[channel].CFGL &= ~BIT(GPDMA0_CH_CFGL_HS_SEL_DST_Pos);
			dma->CH[channel].CTLL |= 1 << GPDMA0_CH_CTLL_TT_FC_Pos;
		}

		if (config->channel_direction == PERIPHERAL_TO_MEMORY) {
			dma->CH[channel].CFGH = (dlr_line_reg << GPDMA0_CH_CFGH_SRC_PER_Pos) | 4;
			dma->CH[channel].CFGL &= ~BIT(GPDMA0_CH_CFGL_HS_SEL_SRC_Pos);
			dma->CH[channel].CTLL |= 2 << GPDMA0_CH_CTLL_TT_FC_Pos;
		}
	}

	if (block->source_gather_en) {
		dma->CH[channel].CTLL |= BIT(GPDMA0_CH_CTLL_SRC_GATHER_EN_Pos);
		/* truncate if we are out of range */
		dma->CH[channel].SGR = (block->source_gather_interval & GPDMA0_CH_SGR_SGI_Msk) |
				       block->source_gather_count << GPDMA0_CH_SGR_SGC_Pos;
	}

	if (block->dest_scatter_en) {
		dma->CH[channel].CTLL |= BIT(GPDMA0_CH_CTLL_DST_SCATTER_EN_Pos);
		/* truncate if we are out of range */
		dma->CH[channel].DSR = (block->dest_scatter_interval & GPDMA0_CH_DSR_DSI_Msk) |
				       block->dest_scatter_count << GPDMA0_CH_DSR_DSC_Pos;
	}

	dev_data->channels[channel].cb = config->dma_callback;
	dev_data->channels[channel].user_data = config->user_data;
	dev_data->channels[channel].block_ts = block->block_size / config->source_data_size;
	dev_data->channels[channel].source_data_size = config->source_data_size;
	dev_data->channels[channel].dlr_line = dlr_line;

	XMC_DMA_CH_DisableEvent(dma, channel, ALL_EVENTS);
	XMC_DMA_CH_EnableEvent(dma, channel, XMC_DMA_CH_EVENT_TRANSFER_COMPLETE);

	/* trigger enable on block transfer complete */
	if (config->complete_callback_en) {
		XMC_DMA_CH_EnableEvent(dma, channel, XMC_DMA_CH_EVENT_BLOCK_TRANSFER_COMPLETE);
	}

	if (!config->error_callback_dis) {
		XMC_DMA_CH_EnableEvent(dma, channel, XMC_DMA_CH_EVENT_ERROR);
	}

	LOG_DBG("Configured channel %d for %08X to %08X (%u)", channel, block->source_address,
		block->dest_address, block->block_size);

	return 0;
}

static int dma_xmc4xxx_start(const struct device *dev, uint32_t channel)
{
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;

	LOG_DBG("Starting channel %d", channel);
	XMC_DMA_CH_Enable(dev_cfg->dma, channel);
	return 0;
}

static int dma_xmc4xxx_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;
	struct dma_xmc4xxx_data *dev_data = dev->data;
	struct dma_xmc4xxx_channel *dma_channel;
	XMC_DMA_t *dma = dev_cfg->dma;

	dma_channel = &dev_data->channels[channel];
	XMC_DMA_CH_Suspend(dma, channel);

	/* wait until ongoing transfer finishes */
	while (XMC_DMA_CH_IsEnabled(dma, channel) &&
	      (dma->CH[channel].CFGL & GPDMA0_CH_CFGL_FIFO_EMPTY_Msk) == 0) {
	}

	/* disconnect DLR line to stop overuns */
	if (dma_channel->dlr_line != DLR_LINE_UNSET) {
		DLR->LNEN &= ~BIT(dma_channel->dlr_line);
	}

	dma_channel->dlr_line = DLR_LINE_UNSET;
	dma_channel->cb = NULL;

	XMC_DMA_CH_Disable(dma, channel);
	return 0;
}

static int dma_xmc4xxx_reload(const struct device *dev, uint32_t channel, uint32_t src,
			      uint32_t dst, size_t size)
{
	struct dma_xmc4xxx_data *dev_data = dev->data;
	size_t block_ts;
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;
	XMC_DMA_t *dma = dev_cfg->dma;
	struct dma_xmc4xxx_channel *dma_channel;

	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("Invalid channel number");
		return -EINVAL;
	}

	if (XMC_DMA_CH_IsEnabled(dma, channel)) {
		LOG_ERR("Channel is still active");
		return -EINVAL;
	}

	dma_channel = &dev_data->channels[channel];
	block_ts = size / dma_channel->source_data_size;
	if (block_ts > DMA_MAX_BLOCK_LEN) {
		LOG_ERR("Block transactions must be <= 4095");
		return -EINVAL;
	}
	dma_channel->block_ts = block_ts;

	/* do we need to clear any errors */
	dma->CH[channel].SAR = src;
	dma->CH[channel].DAR = dst;
	dma->CH[channel].CTLH = block_ts;

	return 0;
}

static int dma_xmc4xxx_get_status(const struct device *dev, uint32_t channel,
				  struct dma_status *stat)
{
	struct dma_xmc4xxx_data *dev_data = dev->data;
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;
	XMC_DMA_t *dma = dev_cfg->dma;
	struct dma_xmc4xxx_channel *dma_channel;

	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("Invalid channel number");
		return -EINVAL;
	}
	dma_channel = &dev_data->channels[channel];

	stat->busy = XMC_DMA_CH_IsEnabled(dma, channel);

	stat->pending_length  = dma_channel->block_ts - XMC_DMA_CH_GetTransferredData(dma, channel);
	stat->pending_length *= dma_channel->source_data_size;
	/* stat->dir and other remaining fields are not set. They are not */
	/* useful for xmc4xxx peripheral drivers. */

	return 0;
}

static bool dma_xmc4xxx_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t requested_channel;

	if (!filter_param) {
		return true;
	}

	requested_channel = *(uint32_t *)filter_param;

	if (channel == requested_channel) {
		return true;
	}

	return false;
}

static int dma_xmc4xxx_suspend(const struct device *dev, uint32_t channel)
{
	struct dma_xmc4xxx_data *dev_data = dev->data;
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;
	XMC_DMA_t *dma = dev_cfg->dma;

	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("Invalid channel number");
		return -EINVAL;
	}

	XMC_DMA_CH_Suspend(dma, channel);
	return 0;
}

static int dma_xmc4xxx_resume(const struct device *dev, uint32_t channel)
{
	struct dma_xmc4xxx_data *dev_data = dev->data;
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;
	XMC_DMA_t *dma = dev_cfg->dma;

	if (channel >= dev_data->ctx.dma_channels) {
		LOG_ERR("Invalid channel number");
		return -EINVAL;
	}

	XMC_DMA_CH_Resume(dma, channel);
	return 0;
}

static int dma_xmc4xxx_init(const struct device *dev)
{
	const struct dma_xmc4xxx_config *dev_cfg = dev->config;

	XMC_DMA_Enable(dev_cfg->dma);
	dev_cfg->irq_configure();
	return 0;
}

static const struct dma_driver_api dma_xmc4xxx_driver_api = {
	.config = dma_xmc4xxx_config,
	.reload = dma_xmc4xxx_reload,
	.start = dma_xmc4xxx_start,
	.stop = dma_xmc4xxx_stop,
	.get_status = dma_xmc4xxx_get_status,
	.chan_filter = dma_xmc4xxx_chan_filter,
	.suspend = dma_xmc4xxx_suspend,
	.resume = dma_xmc4xxx_resume,
};

#define XMC4XXX_DMA_INIT(inst)                                                  \
	static void dma_xmc4xxx##inst##_irq_configure(void)                     \
	{                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, 0, irq),                   \
			    DT_INST_IRQ_BY_IDX(inst, 0, priority),              \
			    dma_xmc4xxx_isr,                                    \
			    DEVICE_DT_INST_GET(inst), 0);                       \
		irq_enable(DT_INST_IRQ_BY_IDX(inst, 0, irq));                   \
	}                                                                       \
	static const struct dma_xmc4xxx_config dma_xmc4xxx##inst##_config = {   \
		.dma = (XMC_DMA_t *)DT_INST_REG_ADDR(inst),                     \
		.irq_configure = dma_xmc4xxx##inst##_irq_configure,             \
	};                                                                      \
										\
	static struct dma_xmc4xxx_channel                                       \
		dma_xmc4xxx##inst##_channels[DT_INST_PROP(inst, dma_channels)]; \
	ATOMIC_DEFINE(dma_xmc4xxx_atomic##inst,                                 \
		      DT_INST_PROP(inst, dma_channels));                        \
	static struct dma_xmc4xxx_data dma_xmc4xxx##inst##_data = {             \
		.ctx =  {                                                       \
			.magic = DMA_MAGIC,                                     \
			.atomic = dma_xmc4xxx_atomic##inst,                     \
			.dma_channels = DT_INST_PROP(inst, dma_channels),       \
		},                                                              \
		.channels = dma_xmc4xxx##inst##_channels,                       \
	};                                                                      \
										\
	DEVICE_DT_INST_DEFINE(inst, &dma_xmc4xxx_init, NULL,                    \
			      &dma_xmc4xxx##inst##_data,                        \
			      &dma_xmc4xxx##inst##_config, PRE_KERNEL_1,        \
			      CONFIG_DMA_INIT_PRIORITY, &dma_xmc4xxx_driver_api);

DT_INST_FOREACH_STATUS_OKAY(XMC4XXX_DMA_INIT)
