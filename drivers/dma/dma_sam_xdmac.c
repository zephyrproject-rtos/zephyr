/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT atmel_sam_xdmac

/** @file
 * @brief Atmel SAM MCU family Direct Memory Access (XDMAC) driver.
 */

#include <errno.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <string.h>
#include <soc.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control/atmel_sam_pmc.h>
#include "dma_sam_xdmac.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
LOG_MODULE_REGISTER(dma_sam_xdmac);

#define XDMAC_INT_ERR (XDMAC_CIE_RBIE | XDMAC_CIE_WBIE | XDMAC_CIE_ROIE)
#define DMA_CHANNELS_MAX 31

enum dma_state {
	DMA_STATE_INIT = 0,
	DMA_STATE_CONFIGURED,
	DMA_STATE_RUNNING,
	DMA_STATE_SUSPENDED,
};

/* DMA channel configuration */
struct sam_xdmac_channel_cfg {
	void *user_data;
	dma_callback_t callback;
	uint32_t data_size;
	enum dma_state state;
};

/* Device constant configuration parameters */
struct sam_xdmac_dev_cfg {
	Xdmac *regs;
	void (*irq_config)(void);
	const struct atmel_sam_pmc_config clock_cfg;
	uint8_t irq_id;
};

/* Device run time data */
struct sam_xdmac_dev_data {
	struct dma_context dma_ctx;
	struct sam_xdmac_channel_cfg dma_channels[DMA_CHANNELS_MAX + 1];
};

static void sam_xdmac_isr(const struct device *dev)
{
	const struct sam_xdmac_dev_cfg *const dev_cfg = dev->config;
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	uint32_t channel_num = dev_data->dma_ctx.dma_channels;

	Xdmac * const xdmac = dev_cfg->regs;
	struct sam_xdmac_channel_cfg *channel_cfg;
	uint32_t isr_status;
	uint32_t err;

	/* Get global interrupt status */
	isr_status = xdmac->XDMAC_GIS;

	for (int channel = 0; channel < channel_num; channel++) {
		if (!(isr_status & (1 << channel))) {
			continue;
		}

		dev_data->dma_channels[channel].state = DMA_STATE_CONFIGURED;
		channel_cfg = &dev_data->dma_channels[channel];

		/* Get channel errors */
		err = xdmac->XDMAC_CHID[channel].XDMAC_CIS & XDMAC_INT_ERR;

		/* Execute callback */
		if (channel_cfg->callback) {
			channel_cfg->callback(dev, channel_cfg->user_data,
					      channel, err);
		}
	}
}

int sam_xdmac_channel_configure(const struct device *dev, uint32_t channel,
				struct sam_xdmac_channel_config *param)
{
	const struct sam_xdmac_dev_cfg *const dev_cfg = dev->config;
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	uint32_t channel_num = dev_data->dma_ctx.dma_channels;

	Xdmac * const xdmac = dev_cfg->regs;

	if (channel >= channel_num) {
		LOG_ERR("Channel %d out of range", channel);
		return -EINVAL;
	}

	/* Check if the channel is enabled */
	if (xdmac->XDMAC_GS & (XDMAC_GS_ST0 << channel)) {
		return -EBUSY;
	}

	/* Disable all channel interrupts */
	xdmac->XDMAC_CHID[channel].XDMAC_CID = 0xFF;
	/* Clear pending Interrupt Status bit(s) */
	(void)xdmac->XDMAC_CHID[channel].XDMAC_CIS;

	/* NOTE:
	 * Setting channel configuration is not required for linked list view 2
	 * to 3 modes. It is done anyway to keep the code simple. It has no
	 * negative impact on the DMA functionality.
	 */

	/* Set channel configuration */
	xdmac->XDMAC_CHID[channel].XDMAC_CC = param->cfg;

	/* Set data stride memory pattern */
	xdmac->XDMAC_CHID[channel].XDMAC_CDS_MSP = param->ds_msp;
	/* Set source microblock stride */
	xdmac->XDMAC_CHID[channel].XDMAC_CSUS = param->sus;
	/* Set destination microblock stride */
	xdmac->XDMAC_CHID[channel].XDMAC_CDUS = param->dus;

	/* Enable selected channel interrupts */
	xdmac->XDMAC_CHID[channel].XDMAC_CIE = param->cie;

	return 0;
}

int sam_xdmac_transfer_configure(const struct device *dev, uint32_t channel,
				 struct sam_xdmac_transfer_config *param)
{
	const struct sam_xdmac_dev_cfg *const dev_cfg = dev->config;
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	uint32_t channel_num = dev_data->dma_ctx.dma_channels;

	Xdmac * const xdmac = dev_cfg->regs;

	if (channel >= channel_num) {
		LOG_ERR("Channel %d out of range", channel);
		return -EINVAL;
	}

	/* Check if the channel is enabled */
	if (xdmac->XDMAC_GS & (XDMAC_GS_ST0 << channel)) {
		return -EBUSY;
	}

	/* NOTE:
	 * Setting source, destination address is not required for linked list
	 * view 1 to 3 modes. It is done anyway to keep the code simple. It has
	 * no negative impact on the DMA functionality.
	 */

	/* Set source address */
	xdmac->XDMAC_CHID[channel].XDMAC_CSA = param->sa;
	/* Set destination address */
	xdmac->XDMAC_CHID[channel].XDMAC_CDA = param->da;

	if ((param->ndc & XDMAC_CNDC_NDE) == XDMAC_CNDC_NDE_DSCR_FETCH_DIS) {
		/*
		 * Linked List is disabled, configure additional transfer
		 * parameters.
		 */

		/* Set length of data in the microblock */
		xdmac->XDMAC_CHID[channel].XDMAC_CUBC = param->ublen;
		/* Set block length: block length is (blen+1) microblocks */
		xdmac->XDMAC_CHID[channel].XDMAC_CBC = param->blen;
	} else {
		/*
		 * Linked List is enabled, configure additional transfer
		 * parameters.
		 */

		/* Set next descriptor address */
		xdmac->XDMAC_CHID[channel].XDMAC_CNDA = param->nda;
	}

	/* Set next descriptor configuration */
	xdmac->XDMAC_CHID[channel].XDMAC_CNDC = param->ndc;

	dev_data->dma_channels[channel].state = DMA_STATE_CONFIGURED;

	return 0;
}

static int sam_xdmac_config(const struct device *dev, uint32_t channel,
			    struct dma_config *cfg)
{
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	uint32_t channel_num = dev_data->dma_ctx.dma_channels;
	struct sam_xdmac_channel_config channel_cfg;
	struct sam_xdmac_transfer_config transfer_cfg;
	uint32_t burst_size;
	uint32_t data_size;
	int ret;

	if (channel >= channel_num) {
		LOG_ERR("Channel %d out of range", channel);
		return -EINVAL;
	}

	if (dev_data->dma_channels[channel].state != DMA_STATE_INIT &&
	    dev_data->dma_channels[channel].state != DMA_STATE_CONFIGURED) {
		LOG_ERR("Config Channel %d at invalidate state", channel);
		return -EINVAL;
	}

	__ASSERT_NO_MSG(cfg->source_data_size == cfg->dest_data_size);
	__ASSERT_NO_MSG(cfg->source_burst_length == cfg->dest_burst_length);

	if (cfg->source_data_size != 1U && cfg->source_data_size != 2U &&
	    cfg->source_data_size != 4U) {
		LOG_ERR("Invalid 'source_data_size' value");
		return -EINVAL;
	}

	if (cfg->block_count != 1U) {
		LOG_ERR("Only single block transfer is currently supported."
			    " Please submit a patch.");
		return -EINVAL;
	}

	burst_size = find_msb_set(cfg->source_burst_length) - 1;
	LOG_DBG("burst_size=%d", burst_size);
	data_size = find_msb_set(cfg->source_data_size) - 1;
	dev_data->dma_channels[channel].data_size = data_size;
	LOG_DBG("data_size=%d", data_size);

	uint32_t xdmac_inc_cfg = 0;

	if (cfg->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT
		&& cfg->channel_direction == MEMORY_TO_PERIPHERAL) {
		xdmac_inc_cfg |= XDMAC_CC_SAM_INCREMENTED_AM;
	} else {
		xdmac_inc_cfg |= XDMAC_CC_SAM_FIXED_AM;
	}

	if (cfg->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT
		&& cfg->channel_direction == PERIPHERAL_TO_MEMORY) {
		xdmac_inc_cfg |= XDMAC_CC_DAM_INCREMENTED_AM;
	} else {
		xdmac_inc_cfg |= XDMAC_CC_DAM_FIXED_AM;
	}

	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		channel_cfg.cfg =
			  XDMAC_CC_TYPE_MEM_TRAN
			| XDMAC_CC_MBSIZE(burst_size == 0U ? 0 : burst_size - 1)
			| XDMAC_CC_SAM_INCREMENTED_AM
			| XDMAC_CC_DAM_INCREMENTED_AM;
#if defined(CONFIG_SOC_SERIES_SAMA7G5)
		/* When a memory-to-memory transfer is performed, configure PERID to 0x7F. */
		cfg->dma_slot = 0x7F;
#endif
		break;
	case MEMORY_TO_PERIPHERAL:
		channel_cfg.cfg =
			  XDMAC_CC_TYPE_PER_TRAN
			| XDMAC_CC_CSIZE(burst_size)
			| XDMAC_CC_DSYNC_MEM2PER
			| xdmac_inc_cfg;
		break;
	case PERIPHERAL_TO_MEMORY:
		channel_cfg.cfg =
			  XDMAC_CC_TYPE_PER_TRAN
			| XDMAC_CC_CSIZE(burst_size)
			| XDMAC_CC_DSYNC_PER2MEM
			| xdmac_inc_cfg;
		break;
	default:
		LOG_ERR("'channel_direction' value %d is not supported",
			    cfg->channel_direction);
		return -EINVAL;
	}

	channel_cfg.cfg |= XDMAC_CC_DWIDTH(data_size) |
#ifdef XDMAC_CC_SIF_AHB_IF1
			   XDMAC_CC_SIF_AHB_IF1 |
#endif
#ifdef XDMAC_CC_DIF_AHB_IF1
			   XDMAC_CC_DIF_AHB_IF1 |
#endif
			   XDMAC_CC_PERID(cfg->dma_slot);
	channel_cfg.ds_msp = 0U;
	channel_cfg.sus = 0U;
	channel_cfg.dus = 0U;
	channel_cfg.cie =
		  (cfg->complete_callback_en ? XDMAC_CIE_BIE : XDMAC_CIE_LIE)
		| (cfg->error_callback_dis ? 0 : XDMAC_INT_ERR);

	ret = sam_xdmac_channel_configure(dev, channel, &channel_cfg);
	if (ret < 0) {
		return ret;
	}

	dev_data->dma_channels[channel].callback = cfg->dma_callback;
	dev_data->dma_channels[channel].user_data = cfg->user_data;

	(void)memset(&transfer_cfg, 0, sizeof(transfer_cfg));
	transfer_cfg.sa = cfg->head_block->source_address;
	transfer_cfg.da = cfg->head_block->dest_address;
	transfer_cfg.ublen = cfg->head_block->block_size >> data_size;

	ret = sam_xdmac_transfer_configure(dev, channel, &transfer_cfg);

	return ret;
}

static int sam_xdmac_transfer_reload(const struct device *dev, uint32_t channel,
				     uint32_t src, uint32_t dst, size_t size)
{
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	struct sam_xdmac_transfer_config transfer_cfg = {
		.sa = src,
		.da = dst,
		.ublen = size >> dev_data->dma_channels[channel].data_size,
	};

	return sam_xdmac_transfer_configure(dev, channel, &transfer_cfg);
}

int sam_xdmac_transfer_start(const struct device *dev, uint32_t channel)
{
	const struct sam_xdmac_dev_cfg *config = dev->config;
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	uint32_t channel_num = dev_data->dma_ctx.dma_channels;

	Xdmac * const xdmac = config->regs;

	if (channel >= channel_num) {
		LOG_ERR("Channel %d out of range", channel);
		return -EINVAL;
	}

	if (dev_data->dma_channels[channel].state != DMA_STATE_CONFIGURED &&
	    dev_data->dma_channels[channel].state != DMA_STATE_RUNNING) {
		LOG_ERR("Start Channel %d at invalidate state", channel);
		return -EINVAL;
	}

	/* Check if the channel is enabled */
	if (xdmac->XDMAC_GS & (XDMAC_GS_ST0 << channel)) {
		LOG_DBG("Channel %d already enabled", channel);
		return -EBUSY;
	}

	/* Enable channel interrupt */
	xdmac->XDMAC_GIE = XDMAC_GIE_IE0 << channel;
	/* Enable channel */
	xdmac->XDMAC_GE = XDMAC_GE_EN0 << channel;

	dev_data->dma_channels[channel].state = DMA_STATE_RUNNING;

	return 0;
}

int sam_xdmac_transfer_stop(const struct device *dev, uint32_t channel)
{
	const struct sam_xdmac_dev_cfg *config = dev->config;
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	uint32_t channel_num = dev_data->dma_ctx.dma_channels;

	Xdmac * const xdmac = config->regs;

	if (channel >= channel_num) {
		LOG_ERR("Channel %d out of range", channel);
		return -EINVAL;
	}

	if (dev_data->dma_channels[channel].state == DMA_STATE_INIT) {
		LOG_ERR("Channel %d not configured", channel);
		return -EINVAL;
	}

	/* Check if the channel is enabled */
	if (!(xdmac->XDMAC_GS & (XDMAC_GS_ST0 << channel))) {
		return 0;
	}

	/* Disable channel */
	xdmac->XDMAC_GD = XDMAC_GD_DI0 << channel;
	/* Disable channel interrupt */
	xdmac->XDMAC_GID = XDMAC_GID_ID0 << channel;
	/* Disable all channel interrupts */
	xdmac->XDMAC_CHID[channel].XDMAC_CID = 0xFF;
	/* Clear the pending Interrupt Status bit(s) */
	(void)xdmac->XDMAC_CHID[channel].XDMAC_CIS;

	dev_data->dma_channels[channel].state = DMA_STATE_CONFIGURED;

	return 0;
}

static int sam_xdmac_initialize(const struct device *dev)
{
	const struct sam_xdmac_dev_cfg *const dev_cfg = dev->config;
	struct sam_xdmac_dev_data *const dev_data = dev->data;

	Xdmac * const xdmac = dev_cfg->regs;

	dev_data->dma_ctx.dma_channels = FIELD_GET(XDMAC_GTYPE_NB_CH_Msk, xdmac->XDMAC_GTYPE) + 1;
	if (dev_data->dma_ctx.dma_channels > DMA_CHANNELS_MAX + 1) {
		LOG_ERR("Maximum supported channels is %d", DMA_CHANNELS_MAX + 1);
		return -EINVAL;
	}

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Enable XDMAC clock in PMC */
	(void)clock_control_on(SAM_DT_PMC_CONTROLLER,
			       (clock_control_subsys_t)&dev_cfg->clock_cfg);

	/* Disable all channels */
	xdmac->XDMAC_GD = UINT32_MAX;
	/* Disable all channel interrupts */
	xdmac->XDMAC_GID = UINT32_MAX;

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	LOG_INF("Device %s initialized", dev->name);

	return 0;
}

static int xdmac_suspend(const struct device *dev, uint32_t channel)
{
	const struct sam_xdmac_dev_cfg *config = dev->config;
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	uint32_t channel_num = dev_data->dma_ctx.dma_channels;

	Xdmac * const xdmac = config->regs;

	if (channel >= channel_num) {
		LOG_ERR("Channel %d out of range", channel);
		return -EINVAL;
	}

	switch (dev_data->dma_channels[channel].state) {
	case DMA_STATE_RUNNING:
		break;
	case DMA_STATE_SUSPENDED:
		return 0;
	default:
		LOG_ERR("Suspend Channel %d at invalidate state", channel);
		return -EINVAL;
	}

	if (!(xdmac->XDMAC_GS & BIT(channel))) {
		LOG_DBG("Channel %d not enabled", channel);
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_SAMX7X)
	if (xdmac->XDMAC_GRS & BIT(channel) || xdmac->XDMAC_GWS & BIT(channel)) {
#elif defined(CONFIG_SOC_SERIES_SAMA7G5)
	if (xdmac->XDMAC_GRSS & BIT(channel) || xdmac->XDMAC_GWSS & BIT(channel)) {
#else
#error Unsupported SoC family
#endif
		LOG_DBG("Channel %d already suspended", channel);
		return 0;
	}

	xdmac->XDMAC_GRWS |= BIT(channel);

	dev_data->dma_channels[channel].state = DMA_STATE_SUSPENDED;

	return 0;
}

static int xdmac_resume(const struct device *dev, uint32_t channel)
{
	const struct sam_xdmac_dev_cfg *config = dev->config;
	struct sam_xdmac_dev_data *const dev_data = dev->data;
	uint32_t channel_num = dev_data->dma_ctx.dma_channels;

	Xdmac * const xdmac = config->regs;

	if (channel >= channel_num) {
		LOG_ERR("Channel %d out of range", channel);
		return -EINVAL;
	}

	switch (dev_data->dma_channels[channel].state) {
	case DMA_STATE_SUSPENDED:
		break;
	case DMA_STATE_RUNNING:
		return 0;
	default:
		LOG_ERR("Resume Channel %d at invalidate state", channel);
		return -EINVAL;
	}

	if (!(xdmac->XDMAC_GS & BIT(channel))) {
		LOG_DBG("Channel %d not enabled", channel);
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_SAMX7X)
	if (!(xdmac->XDMAC_GRS & BIT(channel) || xdmac->XDMAC_GWS & BIT(channel))) {
#elif defined(CONFIG_SOC_SERIES_SAMA7G5)
	if (!(xdmac->XDMAC_GRSS & BIT(channel) || xdmac->XDMAC_GWSS & BIT(channel))) {
#else
#error Unsupported SoC family
#endif
		LOG_DBG("Channel %d not suspended", channel);
		return 0;
	}

	xdmac->XDMAC_GRWR |= BIT(channel);

	dev_data->dma_channels[channel].state = DMA_STATE_RUNNING;

	return 0;
}

static int sam_xdmac_get_status(const struct device *dev, uint32_t channel,
				struct dma_status *status)
{
	const struct sam_xdmac_dev_cfg *const dev_cfg = dev->config;

	Xdmac * const xdmac = dev_cfg->regs;
	uint32_t chan_cfg = xdmac->XDMAC_CHID[channel].XDMAC_CC;
	uint32_t ublen = xdmac->XDMAC_CHID[channel].XDMAC_CUBC;

	/* we need to check some of the XDMAC_CC registers to determine the DMA direction */
	if ((chan_cfg & XDMAC_CC_TYPE_Msk) == 0) {
		status->dir = MEMORY_TO_MEMORY;
	} else if ((chan_cfg & XDMAC_CC_DSYNC_Msk) == XDMAC_CC_DSYNC_MEM2PER) {
		status->dir = MEMORY_TO_PERIPHERAL;
	} else {
		status->dir = PERIPHERAL_TO_MEMORY;
	}

	status->busy = ((chan_cfg & XDMAC_CC_INITD_Msk) != 0) || (ublen > 0);
	status->pending_length = ublen;

	return 0;
}

static DEVICE_API(dma, sam_xdmac_driver_api) = {
	.config = sam_xdmac_config,
	.reload = sam_xdmac_transfer_reload,
	.start = sam_xdmac_transfer_start,
	.stop = sam_xdmac_transfer_stop,
	.suspend = xdmac_suspend,
	.resume = xdmac_resume,
	.get_status = sam_xdmac_get_status,
};

#define DMA_INIT(n)								\
	static void dma##n##_irq_config(void)					\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority),		\
			    sam_xdmac_isr, DEVICE_DT_INST_GET(n), 0);		\
	}									\
										\
	static const struct sam_xdmac_dev_cfg dma##n##_config = {		\
		.regs = (Xdmac *)DT_INST_REG_ADDR(n),				\
		.irq_config = dma##n##_irq_config,				\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),			\
		.irq_id = DT_INST_IRQN(n),					\
	};									\
										\
	static ATOMIC_DEFINE(dma_channels_atomic_##n, DMA_CHANNELS_MAX);	\
										\
	static struct sam_xdmac_dev_data dma##n##_data = {			\
		.dma_ctx.magic = DMA_MAGIC,					\
		.dma_ctx.atomic = dma_channels_atomic_##n,			\
	};									\
										\
	DEVICE_DT_INST_DEFINE(n, &sam_xdmac_initialize, NULL,			\
			      &dma##n##_data, &dma##n##_config, POST_KERNEL,	\
			      CONFIG_DMA_INIT_PRIORITY, &sam_xdmac_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_INIT)
