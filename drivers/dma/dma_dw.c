/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_dma

#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <kernel.h>
#include <device.h>
#include <init.h>
#include <drivers/dma.h>
#include <soc.h>
#include "dma_dw.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(dma_dw);

#define BYTE				(1)
#define WORD				(2)
#define DWORD				(4)

/* CFG_LO */
#define DW_CFG_CLASS(x)			(x << 29)
/* CFG_HI */
#define DW_CFGH_SRC_PER(x)		((x & 0xf) | ((x & 0x30) << 24))
#define DW_CFGH_DST_PER(x)		(((x & 0xf) << 4) | ((x & 0x30) << 26))

/* default initial setup register values */
#define DW_CFG_LOW_DEF			0x0

#define DEV_NAME(dev) ((dev)->name)
#define DEV_DATA(dev) ((struct dw_dma_dev_data *const)(dev)->data)
#define DEV_CFG(dev) \
	((const struct dw_dma_dev_cfg *const)(dev)->config)

/* number of tries to wait for reset */
#define DW_DMA_CFG_TRIES	10000
#define INT_MASK_ALL		0xFF00

static ALWAYS_INLINE void dw_write(uint32_t dma_base, uint32_t reg, uint32_t value)
{
	*((volatile uint32_t*)(dma_base + reg)) = value;
}

static ALWAYS_INLINE uint32_t dw_read(uint32_t dma_base, uint32_t reg)
{
	return *((volatile uint32_t*)(dma_base + reg));
}

static void dw_dma_isr(const struct device *dev)
{
	const struct dw_dma_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct dw_dma_dev_data *const dev_data = DEV_DATA(dev);
	struct dma_chan_data *chan_data;

	uint32_t status_tfr = 0U;
	uint32_t status_block = 0U;
	uint32_t status_err = 0U;
	uint32_t status_intr;
	uint32_t channel;

	status_intr = dw_read(dev_cfg->base, DW_INTR_STATUS);
	if (!status_intr) {
		LOG_ERR("status_intr = %d", status_intr);
	}

	/* get the source of our IRQ. */
	status_block = dw_read(dev_cfg->base, DW_STATUS_BLOCK);
	status_tfr = dw_read(dev_cfg->base, DW_STATUS_TFR);

	/* TODO: handle errors, just clear them atm */
	status_err = dw_read(dev_cfg->base, DW_STATUS_ERR);
	if (status_err) {
		LOG_ERR("status_err = %d\n", status_err);
		dw_write(dev_cfg->base, DW_CLEAR_ERR, status_err);
	}

	/* clear interrupts */
	dw_write(dev_cfg->base, DW_CLEAR_BLOCK, status_block);
	dw_write(dev_cfg->base, DW_CLEAR_TFR, status_tfr);

	/* Dispatch callbacks for channels depending upon the bit set */
	while (status_block) {
		channel = find_lsb_set(status_block) - 1;
		status_block &= ~(1 << channel);
		chan_data = &dev_data->chan[channel];

		if (chan_data->dma_blkcallback) {

			/* Ensure the linked list (chan_data->lli) is
			 * freed in the user callback function once
			 * all the blocks are transferred.
			 */
			chan_data->dma_blkcallback(dev,
						   chan_data->blkuser_data,
						   channel, 0);
		}
	}

	while (status_tfr) {
		channel = find_lsb_set(status_tfr) - 1;
		status_tfr &= ~(1 << channel);
		chan_data = &dev_data->chan[channel];
		if (chan_data->dma_tfrcallback) {
			chan_data->dma_tfrcallback(dev,
						   chan_data->tfruser_data,
						   channel, 0);
		}
	}
}

static int dw_dma_config(const struct device *dev, uint32_t channel,
			 struct dma_config *cfg)
{
	struct dw_dma_dev_data *const dev_data = DEV_DATA(dev);
	const struct dw_dma_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct dma_chan_data *chan_data;
	struct dma_block_config *cfg_blocks;
	uint32_t m_size;
	uint32_t tr_width;
	uint32_t ctrl_lo;

	if (channel >= DW_MAX_CHAN) {
		return -EINVAL;
	}

	__ASSERT_NO_MSG(cfg->source_data_size == cfg->dest_data_size);
	__ASSERT_NO_MSG(cfg->source_burst_length == cfg->dest_burst_length);

	if (cfg->source_data_size != BYTE && cfg->source_data_size != WORD &&
	    cfg->source_data_size != DWORD) {
		LOG_ERR("Invalid 'source_data_size' value");
		return -EINVAL;
	}

	cfg_blocks = cfg->head_block;

	if ((cfg_blocks->next_block) || (cfg->block_count > 1)) {
		/*
		 * return error since the application may have allocated
		 * memory for the buffers that may be lost when the DMA
		 * driver discards the buffers provided in the linked blocks
		 */
		LOG_ERR("block_count > 1 not supported");
		return -EINVAL;
	}

	chan_data = &dev_data->chan[channel];

	/* default channel config */
	chan_data->direction = cfg->channel_direction;

	/* data_size = (2 ^ tr_width) */
	tr_width = find_msb_set(cfg->source_data_size) - 1;
	LOG_DBG("Ch%u: tr_width=%d", channel, tr_width);

	/* burst_size = (2 ^ msize) */
	m_size = find_msb_set(cfg->source_burst_length) - 1;
	LOG_DBG("Ch%u: m_size=%d", channel, m_size);

	ctrl_lo = DW_CTLL_SRC_WIDTH(tr_width) | DW_CTLL_DST_WIDTH(tr_width);
	ctrl_lo |= DW_CTLL_SRC_MSIZE(m_size) | DW_CTLL_DST_MSIZE(m_size);

	/* enable interrupt */
	ctrl_lo |= DW_CTLL_INT_EN;

	switch (cfg->channel_direction) {

		case MEMORY_TO_MEMORY:
			ctrl_lo |= DW_CTLL_FC_M2M;
			ctrl_lo |= DW_CTLL_SRC_INC | DW_CTLL_DST_INC;
			break;

		case MEMORY_TO_PERIPHERAL:
			ctrl_lo |= DW_CTLL_FC_M2P;
			ctrl_lo |= DW_CTLL_SRC_INC | DW_CTLL_DST_FIX;

			/* Assign a hardware handshaking interface (0-15) to the
			 * destination of channel
			 */
			dw_write(dev_cfg->base, DW_CFG_HIGH(channel),
					DW_CFGH_DST_PER(cfg->dma_slot));
			break;

		case PERIPHERAL_TO_MEMORY:
			ctrl_lo |= DW_CTLL_FC_P2M;
			ctrl_lo |= DW_CTLL_SRC_FIX | DW_CTLL_DST_INC;

			/* Assign a hardware handshaking interface (0-15) to the
			 * source of channel
			 */
			dw_write(dev_cfg->base, DW_CFG_HIGH(channel),
					DW_CFGH_SRC_PER(cfg->dma_slot));
			break;

		default:
			LOG_ERR("channel_direction %d is not supported",
				    cfg->channel_direction);
			return -EINVAL;
	}

	/* channel needs started from scratch, so write SARn, DARn */
	dw_write(dev_cfg->base, DW_SAR(channel), cfg_blocks->source_address);
	dw_write(dev_cfg->base, DW_DAR(channel), cfg_blocks->dest_address);

	/* Configure a callback appropriately depending on whether the
	 * interrupt is requested at the end of transaction completion or
	 * at the end of each block.
	 */
	if (cfg->complete_callback_en) {
		chan_data->dma_blkcallback = cfg->dma_callback;
		chan_data->blkuser_data = cfg->user_data;
		dw_write(dev_cfg->base, DW_MASK_BLOCK, INT_UNMASK(channel));
	} else {
		chan_data->dma_tfrcallback = cfg->dma_callback;
		chan_data->tfruser_data = cfg->user_data;
		dw_write(dev_cfg->base, DW_MASK_TFR, INT_UNMASK(channel));
	}

	dw_write(dev_cfg->base, DW_MASK_ERR, INT_UNMASK(channel));

	/* write interrupt clear registers for the channel
	 * ClearTfr, ClearBlock, ClearSrcTran, ClearDstTran, ClearErr
	 */
	dw_write(dev_cfg->base, DW_CLEAR_TFR, 0x1 << channel);
	dw_write(dev_cfg->base, DW_CLEAR_BLOCK, 0x1 << channel);
	dw_write(dev_cfg->base, DW_CLEAR_SRC_TRAN, 0x1 << channel);
	dw_write(dev_cfg->base, DW_CLEAR_DST_TRAN, 0x1 << channel);
	dw_write(dev_cfg->base, DW_CLEAR_ERR, 0x1 << channel);

	/* single transfer, must set zero */
	dw_write(dev_cfg->base, DW_LLP(channel), 0);

	/* program CTLn */
	dw_write(dev_cfg->base, DW_CTRL_LOW(channel), ctrl_lo);
	dw_write(dev_cfg->base, DW_CTRL_HIGH(channel),
		DW_CFG_CLASS(dev_data->channel_data->chan[channel].class) |
		cfg_blocks->block_size);

	/* write channel config */
	dw_write(dev_cfg->base, DW_CFG_LOW(channel), DW_CFG_LOW_DEF);

	return 0;
}

static int dw_dma_reload(const struct device *dev, uint32_t channel,
			 uint32_t src, uint32_t dst, size_t size)
{
	struct dw_dma_dev_data *const dev_data = DEV_DATA(dev);
	const struct dw_dma_dev_cfg *const dev_cfg = DEV_CFG(dev);

	if (channel >= DW_MAX_CHAN) {
		return -EINVAL;
	}

	dw_write(dev_cfg->base, DW_SAR(channel), src);
	dw_write(dev_cfg->base, DW_DAR(channel), dst);
	dw_write(dev_cfg->base, DW_CTRL_HIGH(channel),
		DW_CFG_CLASS(dev_data->channel_data->chan[channel].class) |
		size);

	return 0;
}

static int dw_dma_transfer_start(const struct device *dev, uint32_t channel)
{
	const struct dw_dma_dev_cfg *const dev_cfg = DEV_CFG(dev);

	if (channel >= DW_MAX_CHAN) {
		return -EINVAL;
	}

	/* enable the channel */
	dw_write(dev_cfg->base, DW_DMA_CHAN_EN, CHAN_ENABLE(channel));

	return 0;
}

static int dw_dma_transfer_stop(const struct device *dev, uint32_t channel)
{
	const struct dw_dma_dev_cfg *const dev_cfg = DEV_CFG(dev);

	if (channel >= DW_MAX_CHAN) {
		return -EINVAL;
	}

	/* disable the channel */
	dw_write(dev_cfg->base, DW_DMA_CHAN_EN, CHAN_DISABLE(channel));
	return 0;
}

static void dw_dma_setup(const struct device *dev)
{
	const struct dw_dma_dev_cfg *const dev_cfg = DEV_CFG(dev);
	struct dw_dma_dev_data *const dev_data = DEV_DATA(dev);
	struct dw_drv_plat_data *dp = dev_data->channel_data;
	int i;

	/* we cannot config DMAC if DMAC has been already enabled by host */
	if (dw_read(dev_cfg->base, DW_DMA_CFG) != 0) {
		dw_write(dev_cfg->base, DW_DMA_CFG, 0x0);
	}

	/* now check that it's 0 */
	for (i = DW_DMA_CFG_TRIES; i > 0; i--) {
		if (dw_read(dev_cfg->base, DW_DMA_CFG) == 0) {
			goto found;
		}
	}
	LOG_ERR("DW_DMA_CFG is non-zero\n");
	return;

found:
	for (i = 0; i <  DW_MAX_CHAN; i++) {
		dw_read(dev_cfg->base, DW_DMA_CHAN_EN);
	}

	/* enable the DMA controller */
	dw_write(dev_cfg->base, DW_DMA_CFG, 1);

	/* mask all interrupts for all 8 channels */
	dw_write(dev_cfg->base, DW_MASK_TFR, INT_MASK_ALL);
	dw_write(dev_cfg->base, DW_MASK_BLOCK, INT_MASK_ALL);
	dw_write(dev_cfg->base, DW_MASK_SRC_TRAN, INT_MASK_ALL);
	dw_write(dev_cfg->base, DW_MASK_DST_TRAN, INT_MASK_ALL);
	dw_write(dev_cfg->base, DW_MASK_ERR, INT_MASK_ALL);

	/* set channel priorities */
	for (i = 0; i <  DW_MAX_CHAN; i++) {
		dw_write(dev_cfg->base, DW_CTRL_HIGH(i),
		DW_CFG_CLASS(dp->chan[i].class));
	}
}

static int dw_dma_init(const struct device *dev)
{
	const struct dw_dma_dev_cfg *const dev_cfg = DEV_CFG(dev);

	/* Disable all channels and Channel interrupts */
	dw_dma_setup(dev);

	/* Configure interrupts */
	dev_cfg->irq_config();

	LOG_INF("Device %s initialized", DEV_NAME(dev));

	return 0;
}

static const struct dma_driver_api dw_dma_driver_api = {
	.config = dw_dma_config,
	.reload = dw_dma_reload,
	.start = dw_dma_transfer_start,
	.stop = dw_dma_transfer_stop,
};

#define DW_DMAC_INIT(inst)						\
									\
	static struct dw_drv_plat_data dmac##inst = {			\
		.chan[0] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[1] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[2] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[3] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[4] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[5] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[6] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
		.chan[7] = {						\
			.class  = 6,					\
			.weight = 0,					\
		},							\
	};								\
									\
	static void dw_dma##inst##_irq_config(void);			\
									\
	static const struct dw_dma_dev_cfg dw_dma##inst##_config = {	\
		.base = DT_INST_REG_ADDR(inst),				\
		.irq_config = dw_dma##inst##_irq_config			\
	};								\
									\
	static struct dw_dma_dev_data dw_dma##inst##_data = {		\
		.channel_data = &dmac##inst,				\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			    &dw_dma_init,				\
			    device_pm_control_nop,			\
			    &dw_dma##inst##_data,			\
			    &dw_dma##inst##_config, POST_KERNEL,	\
			    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			    &dw_dma_driver_api);			\
									\
	static void dw_dma##inst##_irq_config(void)			\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(inst),				\
			    DT_INST_IRQ(inst, priority), dw_dma_isr,	\
			    DEVICE_DT_INST_GET(inst),			\
			    DT_INST_IRQ(inst, sense));			\
		irq_enable(DT_INST_IRQN(inst));				\
	}

DT_INST_FOREACH_STATUS_OKAY(DW_DMAC_INIT)
