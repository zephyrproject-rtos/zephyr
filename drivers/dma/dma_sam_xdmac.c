/*
 * Copyright (c) 2017 comsuisse AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Atmel SAM MCU family Direct Memory Access (XDMAC) driver.
 */

#include <errno.h>
#include <misc/__assert.h>
#include <device.h>
#include <init.h>
#include <string.h>
#include <soc.h>
#include <dma.h>
#include "dma_sam_xdmac.h"

#define SYS_LOG_DOMAIN "dev/dma_sam_xdmac"
#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DMA_LEVEL
#include <logging/sys_log.h>

#define XDMAC_INT_ERR (XDMAC_CIE_RBIE | XDMAC_CIE_WBIE | XDMAC_CIE_ROIE)
#define DMA_CHANNELS_NO  XDMACCHID_NUMBER

/* DMA channel configuration */
struct sam_xdmac_channel_cfg {
	dma_callback callback;
};

/* Device constant configuration parameters */
struct sam_xdmac_dev_cfg {
	Xdmac *regs;
	void (*irq_config)(void);
	u8_t periph_id;
	u8_t irq_id;
};

/* Device run time data */
struct sam_xdmac_dev_data {
	struct sam_xdmac_channel_cfg dma_channels[DMA_CHANNELS_NO];
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CFG(dev) \
	((const struct sam_xdmac_dev_cfg *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct sam_xdmac_dev_data *const)(dev)->driver_data)

static void sam_xdmac_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct sam_xdmac_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct sam_xdmac_dev_data *const dev_data = DEV_DATA(dev);
	Xdmac *const xdmac = dev_cfg->regs;
	struct sam_xdmac_channel_cfg *channel_cfg;
	u32_t isr_status;
	u32_t err;

	/* Get global interrupt status */
	isr_status = xdmac->XDMAC_GIS;

	for (int channel = 0; channel < DMA_CHANNELS_NO; channel++) {
		if (!(isr_status & (1 << channel))) {
			continue;
		}

		channel_cfg = &dev_data->dma_channels[channel];

		/* Get channel errors */
		err = xdmac->XDMAC_CHID[channel].XDMAC_CIS & XDMAC_INT_ERR;

		/* Execute callback */
		if (channel_cfg->callback) {
			channel_cfg->callback(dev, channel, err);
		}
	}
}

int sam_xdmac_channel_configure(struct device *dev, u32_t channel,
				struct sam_xdmac_channel_config *param)
{
	const struct sam_xdmac_dev_cfg *const dev_cfg = DEV_CFG(dev);
	Xdmac *const xdmac = dev_cfg->regs;

	if (channel >= DMA_CHANNELS_NO) {
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

int sam_xdmac_transfer_configure(struct device *dev, u32_t channel,
				 struct sam_xdmac_transfer_config *param)
{
	const struct sam_xdmac_dev_cfg *const dev_cfg = DEV_CFG(dev);
	Xdmac *const xdmac = dev_cfg->regs;

	if (channel >= DMA_CHANNELS_NO) {
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

	return 0;
}

static int sam_xdmac_config(struct device *dev, u32_t channel,
			    struct dma_config *cfg)
{
	struct sam_xdmac_dev_data *const dev_data = DEV_DATA(dev);
	struct sam_xdmac_channel_config channel_cfg;
	struct sam_xdmac_transfer_config transfer_cfg;
	u32_t burst_size;
	u32_t data_size;
	int ret;

	if (channel >= DMA_CHANNELS_NO) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(cfg->source_data_size == cfg->dest_data_size);
	__ASSERT_NO_MSG(cfg->source_burst_length == cfg->dest_burst_length);

	if (cfg->source_data_size != 1 && cfg->source_data_size != 2 &&
	    cfg->source_data_size != 4) {
		SYS_LOG_ERR("Invalid 'source_data_size' value");
		return -EINVAL;
	}

	if (cfg->block_count != 1) {
		SYS_LOG_ERR("Only single block transfer is currently supported."
			    " Please submit a patch.");
		return -EINVAL;
	}

	burst_size = find_msb_set(cfg->source_burst_length) - 1;
	SYS_LOG_DBG("burst_size=%d", burst_size);
	data_size = find_msb_set(cfg->source_data_size) - 1;
	SYS_LOG_DBG("data_size=%d", data_size);

	switch (cfg->channel_direction) {
	case MEMORY_TO_MEMORY:
		channel_cfg.cfg =
			  XDMAC_CC_TYPE_MEM_TRAN
			| XDMAC_CC_MBSIZE(burst_size == 0 ? 0 : burst_size - 1)
			| XDMAC_CC_SAM_INCREMENTED_AM
			| XDMAC_CC_DAM_INCREMENTED_AM;
		break;
	case MEMORY_TO_PERIPHERAL:
		channel_cfg.cfg =
			  XDMAC_CC_TYPE_PER_TRAN
			| XDMAC_CC_CSIZE(burst_size)
			| XDMAC_CC_DSYNC_MEM2PER
			| XDMAC_CC_SAM_INCREMENTED_AM
			| XDMAC_CC_DAM_FIXED_AM;
		break;
	case PERIPHERAL_TO_MEMORY:
		channel_cfg.cfg =
			  XDMAC_CC_TYPE_PER_TRAN
			| XDMAC_CC_CSIZE(burst_size)
			| XDMAC_CC_DSYNC_PER2MEM
			| XDMAC_CC_SAM_FIXED_AM
			| XDMAC_CC_DAM_INCREMENTED_AM;
		break;
	default:
		SYS_LOG_ERR("'channel_direction' value %d is not supported",
			    cfg->channel_direction);
		return -EINVAL;
	}

	channel_cfg.cfg |=
		  XDMAC_CC_DWIDTH(data_size)
		| XDMAC_CC_SIF_AHB_IF1
		| XDMAC_CC_DIF_AHB_IF1
		| XDMAC_CC_PERID(cfg->dma_slot);
	channel_cfg.ds_msp = 0;
	channel_cfg.sus = 0;
	channel_cfg.dus = 0;
	channel_cfg.cie =
		  (cfg->complete_callback_en ? XDMAC_CIE_BIE : XDMAC_CIE_LIE)
		| (cfg->error_callback_en ? XDMAC_INT_ERR : 0);

	ret = sam_xdmac_channel_configure(dev, channel, &channel_cfg);
	if (ret < 0) {
		return ret;
	}

	dev_data->dma_channels[channel].callback = cfg->dma_callback;

	(void)memset(&transfer_cfg, 0, sizeof(transfer_cfg));
	transfer_cfg.sa = cfg->head_block->source_address;
	transfer_cfg.da = cfg->head_block->dest_address;
	transfer_cfg.ublen = cfg->head_block->block_size >> data_size;

	ret = sam_xdmac_transfer_configure(dev, channel, &transfer_cfg);

	return ret;
}

int sam_xdmac_transfer_start(struct device *dev, u32_t channel)
{
	Xdmac *const xdmac = DEV_CFG(dev)->regs;

	if (channel >= DMA_CHANNELS_NO) {
		return -EINVAL;
	}

	/* Check if the channel is enabled */
	if (xdmac->XDMAC_GS & (XDMAC_GS_ST0 << channel)) {
		return -EBUSY;
	}

	/* Enable channel interrupt */
	xdmac->XDMAC_GIE = XDMAC_GIE_IE0 << channel;
	/* Enable channel */
	xdmac->XDMAC_GE = XDMAC_GE_EN0 << channel;

	return 0;
}

int sam_xdmac_transfer_stop(struct device *dev, u32_t channel)
{
	Xdmac *const xdmac = DEV_CFG(dev)->regs;

	if (channel >= DMA_CHANNELS_NO) {
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

	return 0;
}

static int sam_xdmac_initialize(struct device *dev)
{
	const struct sam_xdmac_dev_cfg *const dev_cfg = DEV_CFG(dev);
	Xdmac *const xdmac = dev_cfg->regs;

	/* Configure interrupts */
	dev_cfg->irq_config();

	/* Enable module's clock */
	soc_pmc_peripheral_enable(dev_cfg->periph_id);

	/* Disable all channels */
	xdmac->XDMAC_GD = UINT32_MAX;
	/* Disable all channel interrupts */
	xdmac->XDMAC_GID = UINT32_MAX;

	/* Enable module's IRQ */
	irq_enable(dev_cfg->irq_id);

	SYS_LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct dma_driver_api sam_xdmac_driver_api = {
	.config = sam_xdmac_config,
	.start = sam_xdmac_transfer_start,
	.stop = sam_xdmac_transfer_stop,
};

/* DMA0 */

static struct device DEVICE_NAME_GET(dma0_sam);

static void dma0_sam_irq_config(void)
{
	IRQ_CONNECT(XDMAC_IRQn, CONFIG_DMA_0_IRQ_PRI, sam_xdmac_isr,
		    DEVICE_GET(dma0_sam), 0);
}

static const struct sam_xdmac_dev_cfg dma0_sam_config = {
	.regs = XDMAC,
	.irq_config = dma0_sam_irq_config,
	.periph_id = ID_XDMAC,
	.irq_id = XDMAC_IRQn,
};

static struct sam_xdmac_dev_data dma0_sam_data;

DEVICE_AND_API_INIT(dma0_sam, CONFIG_DMA_0_NAME, &sam_xdmac_initialize,
		    &dma0_sam_data, &dma0_sam_config, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &sam_xdmac_driver_api);
