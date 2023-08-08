/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/pm/device_runtime.h>
#include <soc.h>
#include "dma_dw_common.h"

#define LOG_LEVEL CONFIG_DMA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_dw_common);

/* number of tries to wait for reset */
#define DW_DMA_CFG_TRIES	10000

void dw_dma_isr(const struct device *dev)
{
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;
	struct dw_dma_dev_data *const dev_data = dev->data;
	struct dw_dma_chan_data *chan_data;

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
			LOG_DBG("Dispatching block complete callback");

			/* Ensure the linked list (chan_data->lli) is
			 * freed in the user callback function once
			 * all the blocks are transferred.
			 */
			chan_data->dma_blkcallback(dev,
						   chan_data->blkuser_data,
						   channel, DMA_STATUS_BLOCK);
		}
	}

	while (status_tfr) {
		channel = find_lsb_set(status_tfr) - 1;
		status_tfr &= ~(1 << channel);
		chan_data = &dev_data->chan[channel];

		/* Transfer complete, channel now idle, a reload
		 * could safely occur in the callback via dma_config
		 * and dma_start
		 */
		chan_data->state = DW_DMA_IDLE;

		if (chan_data->dma_tfrcallback) {
			LOG_DBG("Dispatching transfer callback");
			chan_data->dma_tfrcallback(dev,
						   chan_data->tfruser_data,
						   channel, DMA_STATUS_COMPLETE);
		}
	}
}


/* mask address for dma to identify memory space. */
static void dw_dma_mask_address(struct dma_block_config *block_cfg,
				struct dw_lli *lli_desc, uint32_t direction)
{
	lli_desc->sar = block_cfg->source_address;
	lli_desc->dar = block_cfg->dest_address;

	switch (direction) {
	case MEMORY_TO_PERIPHERAL:
		lli_desc->sar |= CONFIG_DMA_DW_HOST_MASK;
		break;
	case PERIPHERAL_TO_MEMORY:
		lli_desc->dar |= CONFIG_DMA_DW_HOST_MASK;
		break;
	case MEMORY_TO_MEMORY:
		lli_desc->sar |= CONFIG_DMA_DW_HOST_MASK;
		lli_desc->dar |= CONFIG_DMA_DW_HOST_MASK;
		break;
	default:
		break;
	}
}

int dw_dma_config(const struct device *dev, uint32_t channel,
			 struct dma_config *cfg)
{
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;
	struct dw_dma_dev_data *const dev_data = dev->data;
	struct dma_block_config *block_cfg;


	struct dw_lli *lli_desc;
	struct dw_lli *lli_desc_head;
	struct dw_lli *lli_desc_tail;
	uint32_t msize = 3;/* default msize, 8 bytes */
	int ret = 0;

	if (channel >= DW_CHAN_COUNT) {
		LOG_ERR("%s: invalid dma channel %d", __func__, channel);
		ret = -EINVAL;
		goto out;
	}

	struct dw_dma_chan_data *chan_data = &dev_data->chan[channel];

	if (chan_data->state != DW_DMA_IDLE && chan_data->state != DW_DMA_PREPARED) {
		LOG_ERR("%s: dma %s channel %d must be inactive to "
			"reconfigure, currently %d", __func__, dev->name,
			channel, chan_data->state);
		ret = -EBUSY;
		goto out;
	}

	LOG_DBG("%s: dma %s channel %d config",
	       __func__, dev->name, channel);

	__ASSERT_NO_MSG(cfg->source_data_size == cfg->dest_data_size);
	__ASSERT_NO_MSG(cfg->source_burst_length == cfg->dest_burst_length);
	__ASSERT_NO_MSG(cfg->block_count > 0);
	__ASSERT_NO_MSG(cfg->head_block != NULL);

	if (cfg->source_data_size != 1 && cfg->source_data_size != 2 &&
	    cfg->source_data_size != 4 && cfg->source_data_size != 8 &&
	    cfg->source_data_size != 16) {
		LOG_ERR("%s: dma %s channel %d 'invalid source_data_size' value %d",
			__func__, dev->name, channel, cfg->source_data_size);
		ret = -EINVAL;
		goto out;
	}

	if (cfg->block_count > CONFIG_DMA_DW_LLI_POOL_SIZE) {
		LOG_ERR("%s: dma %s channel %d scatter gather list larger than"
			" descriptors in pool, consider increasing CONFIG_DMA_DW_LLI_POOL_SIZE",
			__func__, dev->name, channel);
		ret = -EINVAL;
		goto out;
	}

	/* burst_size = (2 ^ msize) */
	msize = find_msb_set(cfg->source_burst_length) - 1;
	LOG_DBG("%s: dma %s channel %d m_size=%d", __func__, dev->name, channel, msize);
	__ASSERT_NO_MSG(msize < 5);

	/* default channel config */
	chan_data->direction = cfg->channel_direction;
	chan_data->cfg_lo = 0;
	chan_data->cfg_hi = 0;

	/* setup a list of lli structs. we don't need to allocate */
	chan_data->lli = &dev_data->lli_pool[channel][0]; /* TODO allocate here */
	chan_data->lli_count = cfg->block_count;

	/* zero the scatter gather list */
	memset(chan_data->lli, 0, sizeof(struct dw_lli) * chan_data->lli_count);
	lli_desc = chan_data->lli;
	lli_desc_head = &chan_data->lli[0];
	lli_desc_tail = &chan_data->lli[chan_data->lli_count - 1];

	chan_data->ptr_data.buffer_bytes = 0;

	/* copy the scatter gather list from dma_cfg to dw_lli */
	block_cfg = cfg->head_block;
	for (int i = 0; i < cfg->block_count; i++) {
		__ASSERT_NO_MSG(block_cfg != NULL);
		LOG_DBG("copying block_cfg %p to lli_desc %p", block_cfg, lli_desc);

		/* write CTL_LO for each lli */
		switch (cfg->source_data_size) {
		case 1:
			/* byte at a time transfer */
			lli_desc->ctrl_lo |= DW_CTLL_SRC_WIDTH(0);
			break;
		case 2:
			/* non peripheral copies are optimal using words */
			switch (cfg->channel_direction) {
			case MEMORY_TO_MEMORY:
				/* config the src tr width for 32 bit words */
				lli_desc->ctrl_lo |= DW_CTLL_SRC_WIDTH(2);
				break;
			default:
				/* config the src width for 16 bit samples */
				lli_desc->ctrl_lo |= DW_CTLL_SRC_WIDTH(1);
				break;
			}
			break;
		case 4:
			/* config the src tr width for 24, 32 bit samples */
			lli_desc->ctrl_lo |= DW_CTLL_SRC_WIDTH(2);
			break;
		default:
			LOG_ERR("%s: dma %s channel %d invalid src width %d",
			       __func__, dev->name, channel, cfg->source_data_size);
			ret = -EINVAL;
			goto out;
		}

		LOG_DBG("source data size: lli_desc %p, ctrl_lo %x",
			lli_desc, lli_desc->ctrl_lo);

		switch (cfg->dest_data_size) {
		case 1:
			/* byte at a time transfer */
			lli_desc->ctrl_lo |= DW_CTLL_DST_WIDTH(0);
			break;
		case 2:
			/* non peripheral copies are optimal using words */
			switch (cfg->channel_direction) {
			case MEMORY_TO_MEMORY:
				/* config the dest tr width for 32 bit words */
				lli_desc->ctrl_lo |= DW_CTLL_DST_WIDTH(2);
				break;
			default:
				/* config the dest width for 16 bit samples */
				lli_desc->ctrl_lo |= DW_CTLL_DST_WIDTH(1);
				break;
			}
			break;
		case 4:
			/* config the dest tr width for 24, 32 bit samples */
			lli_desc->ctrl_lo |= DW_CTLL_DST_WIDTH(2);
			break;
		default:
			LOG_ERR("%s: dma %s channel %d invalid dest width %d",
				__func__, dev->name, channel, cfg->dest_data_size);
			ret = -EINVAL;
			goto out;
		}

		LOG_DBG("dest data size: lli_desc %p, ctrl_lo %x", lli_desc, lli_desc->ctrl_lo);

		lli_desc->ctrl_lo |= DW_CTLL_SRC_MSIZE(msize) |
			DW_CTLL_DST_MSIZE(msize);

		if (cfg->dma_callback) {
			lli_desc->ctrl_lo |= DW_CTLL_INT_EN; /* enable interrupt */
		}

		LOG_DBG("msize, int_en: lli_desc %p, ctrl_lo %x", lli_desc, lli_desc->ctrl_lo);

		/* config the SINC and DINC fields of CTL_LO,
		 * SRC/DST_PER fields of CFG_HI
		 */
		switch (cfg->channel_direction) {
		case MEMORY_TO_MEMORY:
			lli_desc->ctrl_lo |= DW_CTLL_FC_M2M | DW_CTLL_SRC_INC |
				DW_CTLL_DST_INC;
#if CONFIG_DMA_DW_HW_LLI
			LOG_DBG("setting LLP_D_EN, LLP_S_EN in lli_desc->ctrl_lo %x",
				lli_desc->ctrl_lo);
			lli_desc->ctrl_lo |=
				DW_CTLL_LLP_S_EN | DW_CTLL_LLP_D_EN;
			LOG_DBG("lli_desc->ctrl_lo %x", lli_desc->ctrl_lo);
#endif
#if CONFIG_DMA_DW
			chan_data->cfg_lo |= DW_CFGL_SRC_SW_HS;
			chan_data->cfg_lo |= DW_CFGL_DST_SW_HS;
#endif
			break;
		case MEMORY_TO_PERIPHERAL:
			lli_desc->ctrl_lo |= DW_CTLL_FC_M2P | DW_CTLL_SRC_INC |
				DW_CTLL_DST_FIX;
#if CONFIG_DMA_DW_HW_LLI
			lli_desc->ctrl_lo |= DW_CTLL_LLP_S_EN;
			chan_data->cfg_lo |= DW_CFGL_RELOAD_DST;
#endif
			/* Assign a hardware handshake interface (0-15) to the
			 * destination of the channel
			 */
			chan_data->cfg_hi |= DW_CFGH_DST(cfg->dma_slot);
#if CONFIG_DMA_DW
			chan_data->cfg_lo |= DW_CFGL_SRC_SW_HS;
#endif
			break;
		case PERIPHERAL_TO_MEMORY:
			lli_desc->ctrl_lo |= DW_CTLL_FC_P2M | DW_CTLL_SRC_FIX |
				DW_CTLL_DST_INC;
#if CONFIG_DMA_DW_HW_LLI
			if (!block_cfg->dest_scatter_en) {
				lli_desc->ctrl_lo |= DW_CTLL_LLP_D_EN;
			} else {
				/* Use contiguous auto-reload. Line 3 in
				 * table 3-3
				 */
				lli_desc->ctrl_lo |= DW_CTLL_D_SCAT_EN;
			}
			chan_data->cfg_lo |= DW_CFGL_RELOAD_SRC;
#endif
			/* Assign a hardware handshake interface (0-15) to the
			 * source of the channel
			 */
			chan_data->cfg_hi |= DW_CFGH_SRC(cfg->dma_slot);
#if CONFIG_DMA_DW
			chan_data->cfg_lo |= DW_CFGL_DST_SW_HS;
#endif
			break;
		default:
			LOG_ERR("%s: dma %s channel %d invalid direction %d",
			       __func__, dev->name, channel, cfg->channel_direction);
			ret = -EINVAL;
			goto out;
		}

		LOG_DBG("direction: lli_desc %p, ctrl_lo %x, cfg_hi %x, cfg_lo %x",
			lli_desc, lli_desc->ctrl_lo, chan_data->cfg_hi, chan_data->cfg_lo);

		dw_dma_mask_address(block_cfg, lli_desc, cfg->channel_direction);

		LOG_DBG("mask address: lli_desc %p, ctrl_lo %x, cfg_hi %x, cfg_lo %x",
			lli_desc, lli_desc->ctrl_lo, chan_data->cfg_hi, chan_data->cfg_lo);

		if (block_cfg->block_size > DW_CTLH_BLOCK_TS_MASK) {
			LOG_ERR("%s: dma %s channel %d block size too big %d",
			       __func__, dev->name, channel, block_cfg->block_size);
			ret = -EINVAL;
			goto out;
		}

		/* Set class and transfer size */
		lli_desc->ctrl_hi |= DW_CTLH_CLASS(dev_data->channel_data->chan[channel].class) |
			(block_cfg->block_size & DW_CTLH_BLOCK_TS_MASK);

		LOG_DBG("block_size, class: lli_desc %p, ctrl_lo %x, cfg_hi %x, cfg_lo %x",
			lli_desc, lli_desc->ctrl_lo, chan_data->cfg_hi, chan_data->cfg_lo);

		chan_data->ptr_data.buffer_bytes += block_cfg->block_size;

		/* set next descriptor in list */
		lli_desc->llp = (uintptr_t)(lli_desc + 1);

		LOG_DBG("lli_desc llp %x", lli_desc->llp);

		/* next descriptor */
		lli_desc++;

		block_cfg = block_cfg->next_block;
	}

#if CONFIG_DMA_DW_HW_LLI
	chan_data->cfg_lo |= DW_CFGL_CTL_HI_UPD_EN;
#endif

	/* end of list or cyclic buffer */
	if (cfg->cyclic) {
		lli_desc_tail->llp = (uintptr_t)lli_desc_head;
	} else {
		lli_desc_tail->llp = 0;
#if CONFIG_DMA_DW_HW_LLI
		LOG_DBG("Clearing LLP_S_EN, LLP_D_EN from tail LLI %x", lli_desc_tail->ctrl_lo);
		lli_desc_tail->ctrl_lo &= ~(DW_CTLL_LLP_S_EN | DW_CTLL_LLP_D_EN);
		LOG_DBG("ctrl_lo %x", lli_desc_tail->ctrl_lo);
#endif
	}

	/* set the initial lli, mark the channel as prepared (ready to be started) */
	chan_data->state = DW_DMA_PREPARED;
	chan_data->lli_current = chan_data->lli;

	/* initialize pointers */
	chan_data->ptr_data.start_ptr = DW_DMA_LLI_ADDRESS(chan_data->lli,
							 chan_data->direction);
	chan_data->ptr_data.end_ptr = chan_data->ptr_data.start_ptr +
				    chan_data->ptr_data.buffer_bytes;
	chan_data->ptr_data.current_ptr = chan_data->ptr_data.start_ptr;
	chan_data->ptr_data.hw_ptr = chan_data->ptr_data.start_ptr;

	/* Configure a callback appropriately depending on whether the
	 * interrupt is requested at the end of transaction completion or
	 * at the end of each block.
	 */
	if (cfg->complete_callback_en) {
		chan_data->dma_blkcallback = cfg->dma_callback;
		chan_data->blkuser_data = cfg->user_data;
		dw_write(dev_cfg->base, DW_MASK_BLOCK, DW_CHAN_UNMASK(channel));
	} else {
		chan_data->dma_tfrcallback = cfg->dma_callback;
		chan_data->tfruser_data = cfg->user_data;
		dw_write(dev_cfg->base, DW_MASK_TFR, DW_CHAN_UNMASK(channel));
	}

	dw_write(dev_cfg->base, DW_MASK_ERR, DW_CHAN_UNMASK(channel));

	/* write interrupt clear registers for the channel
	 * ClearTfr, ClearBlock, ClearSrcTran, ClearDstTran, ClearErr
	 */
	dw_write(dev_cfg->base, DW_CLEAR_TFR, 0x1 << channel);
	dw_write(dev_cfg->base, DW_CLEAR_BLOCK, 0x1 << channel);
	dw_write(dev_cfg->base, DW_CLEAR_SRC_TRAN, 0x1 << channel);
	dw_write(dev_cfg->base, DW_CLEAR_DST_TRAN, 0x1 << channel);
	dw_write(dev_cfg->base, DW_CLEAR_ERR, 0x1 << channel);


out:
	return ret;
}

bool dw_dma_is_enabled(const struct device *dev, uint32_t channel)
{
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;

	return dw_read(dev_cfg->base, DW_DMA_CHAN_EN) & DW_CHAN_MASK(channel);
}

int dw_dma_start(const struct device *dev, uint32_t channel)
{
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;
	struct dw_dma_dev_data *dev_data = dev->data;
	int ret = 0;

	/* validate channel */
	if (channel >= DW_CHAN_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	if (dw_dma_is_enabled(dev, channel)) {
		goto out;
	}

	struct dw_dma_chan_data *chan_data = &dev_data->chan[channel];

	/* validate channel state */
	if (chan_data->state != DW_DMA_PREPARED) {
		LOG_ERR("%s: dma %s channel %d not ready ena 0x%x status 0x%x",
		       __func__, dev->name, channel,
		       dw_read(dev_cfg->base, DW_DMA_CHAN_EN),
		       chan_data->state);
		ret = -EBUSY;
		goto out;
	}

	/* is valid stream */
	if (!chan_data->lli) {
		LOG_ERR("%s: dma %s channel %d invalid stream",
		       __func__, dev->name, channel);
		ret = -EINVAL;
		goto out;
	}

	struct dw_lli *lli = chan_data->lli_current;

#ifdef CONFIG_DMA_DW_HW_LLI
	/* LLP mode - write LLP pointer */

	uint32_t masked_ctrl_lo = lli->ctrl_lo & (DW_CTLL_LLP_D_EN | DW_CTLL_LLP_S_EN);
	uint32_t llp = 0;

	if (masked_ctrl_lo) {
		llp = (uint32_t)lli;
		LOG_DBG("Setting llp");
	}
	dw_write(dev_cfg->base, DW_LLP(channel), llp);
	LOG_DBG("ctrl_lo %x, masked ctrl_lo %x, LLP %x",
		lli->ctrl_lo, masked_ctrl_lo, dw_read(dev_cfg->base, DW_LLP(channel)));
#endif /* CONFIG_DMA_DW_HW_LLI */

	/* channel needs to start from scratch, so write SAR and DAR */
#ifdef CONFIG_DMA_64BIT
	dw_write(dev_cfg->base, DW_SAR(channel), (uint32_t)(lli->sar & DW_ADDR_MASK_32));
	dw_write(dev_cfg->base, DW_SAR_HI(channel), (uint32_t)(lli->sar >> DW_ADDR_RIGHT_SHIFT));
	dw_write(dev_cfg->base, DW_DAR(channel), (uint32_t)(lli->dar & DW_ADDR_MASK_32));
	dw_write(dev_cfg->base, DW_DAR_HI(channel), (uint32_t)(lli->dar >> DW_ADDR_RIGHT_SHIFT));
#else
	dw_write(dev_cfg->base, DW_SAR(channel), lli->sar);
	dw_write(dev_cfg->base, DW_DAR(channel), lli->dar);
#endif /* CONFIG_DMA_64BIT */

	/* program CTL_LO and CTL_HI */
	dw_write(dev_cfg->base, DW_CTRL_LOW(channel), lli->ctrl_lo);
	dw_write(dev_cfg->base, DW_CTRL_HIGH(channel), lli->ctrl_hi);

	/* program CFG_LO and CFG_HI */
	dw_write(dev_cfg->base, DW_CFG_LOW(channel), chan_data->cfg_lo);
	dw_write(dev_cfg->base, DW_CFG_HIGH(channel), chan_data->cfg_hi);

#ifdef CONFIG_DMA_64BIT
	LOG_DBG("start: sar %llx, dar %llx, ctrl_lo %x, ctrl_hi %x, cfg_lo %x, cfg_hi %x, llp %x",
		lli->sar, lli->dar, lli->ctrl_lo, lli->ctrl_hi, chan_data->cfg_lo,
		chan_data->cfg_hi, dw_read(dev_cfg->base, DW_LLP(channel))
		);
#else
	LOG_DBG("start: sar %x, dar %x, ctrl_lo %x, ctrl_hi %x, cfg_lo %x, cfg_hi %x, llp %x",
		lli->sar, lli->dar, lli->ctrl_lo, lli->ctrl_hi, chan_data->cfg_lo,
		chan_data->cfg_hi, dw_read(dev_cfg->base, DW_LLP(channel))
		);
#endif /* CONFIG_DMA_64BIT */

#ifdef CONFIG_DMA_DW_HW_LLI
	if (lli->ctrl_lo & DW_CTLL_D_SCAT_EN) {
		LOG_DBG("configuring DW_DSR");
		uint32_t words_per_tfr = (lli->ctrl_hi & DW_CTLH_BLOCK_TS_MASK) >>
			((lli->ctrl_lo & DW_CTLL_DST_WIDTH_MASK) >> DW_CTLL_DST_WIDTH_SHIFT);
		dw_write(dev_cfg->base, DW_DSR(channel),
			 DW_DSR_DSC(words_per_tfr) | DW_DSR_DSI(words_per_tfr));
	}
#endif /* CONFIG_DMA_DW_HW_LLI */

	chan_data->state = DW_DMA_ACTIVE;

	/* enable the channel */
	dw_write(dev_cfg->base, DW_DMA_CHAN_EN, DW_CHAN_UNMASK(channel));
	ret = pm_device_runtime_get(dev);

out:
	return ret;
}

int dw_dma_stop(const struct device *dev, uint32_t channel)
{
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;
	struct dw_dma_dev_data *dev_data = dev->data;
	struct dw_dma_chan_data *chan_data = &dev_data->chan[channel];
	int ret = 0;

	if (channel >= DW_CHAN_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	if (!dw_dma_is_enabled(dev, channel) && chan_data->state != DW_DMA_SUSPENDED) {
		goto out;
	}

#ifdef CONFIG_DMA_DW_HW_LLI
	struct dw_lli *lli = chan_data->lli;
	int i;
#endif

	LOG_DBG("%s: dma %s channel %d stop",
		__func__, dev->name, channel);

	/* Validate the channel state */
	if (chan_data->state != DW_DMA_ACTIVE &&
	    chan_data->state != DW_DMA_SUSPENDED) {
		ret = -EINVAL;
		goto out;
	}

#ifdef CONFIG_DMA_DW_SUSPEND_DRAIN
	/* channel cannot be disabled right away, so first we need to)
	 * suspend it and drain the FIFO
	 */
	dw_write(dev_cfg->base, DW_CFG_LOW(channel),
		 chan_data->cfg_lo | DW_CFGL_SUSPEND | DW_CFGL_DRAIN);

	/* now we wait for FIFO to be empty */
	bool fifo_empty = WAIT_FOR(dw_read(dev_cfg->base, DW_CFG_LOW(channel)) & DW_CFGL_FIFO_EMPTY,
				DW_DMA_TIMEOUT, k_busy_wait(DW_DMA_TIMEOUT/10));
	if (!fifo_empty) {
		LOG_ERR("%s: dma %d channel drain time out", __func__, channel);
		return -ETIMEDOUT;
	}
#endif

	dw_write(dev_cfg->base, DW_DMA_CHAN_EN, DW_CHAN_MASK(channel));

	/* now we wait for channel to be disabled */
	bool is_disabled = WAIT_FOR(!(dw_read(dev_cfg->base, DW_DMA_CHAN_EN) & DW_CHAN(channel)),
				    DW_DMA_TIMEOUT, k_busy_wait(DW_DMA_TIMEOUT/10));
	if (!is_disabled) {
		LOG_ERR("%s: dma %d channel disable timeout", __func__, channel);
		return -ETIMEDOUT;
	}

#if CONFIG_DMA_DW_HW_LLI
	for (i = 0; i < chan_data->lli_count; i++) {
		lli->ctrl_hi &= ~DW_CTLH_DONE(1);
		lli++;
	}
#endif
	chan_data->state = DW_DMA_IDLE;
	ret = pm_device_runtime_put(dev);
out:
	return ret;
}

int dw_dma_resume(const struct device *dev, uint32_t channel)
{
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;
	struct dw_dma_dev_data *dev_data = dev->data;
	int ret = 0;

	/* Validate channel index */
	if (channel >= DW_CHAN_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	struct dw_dma_chan_data *chan_data = &dev_data->chan[channel];

	/* Validate channel state */
	if (chan_data->state != DW_DMA_SUSPENDED) {
		ret = -EINVAL;
		goto out;
	}

	LOG_DBG("%s: dma %s channel %d resume",
		__func__, dev->name, channel);

	dw_write(dev_cfg->base, DW_CFG_LOW(channel), chan_data->cfg_lo);

	/* Channel is now active */
	chan_data->state = DW_DMA_ACTIVE;

out:
	return ret;
}

int dw_dma_suspend(const struct device *dev, uint32_t channel)
{
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;
	struct dw_dma_dev_data *dev_data = dev->data;
	int ret = 0;

	/* Validate channel index */
	if (channel >= DW_CHAN_COUNT) {
		ret = -EINVAL;
		goto out;
	}

	struct dw_dma_chan_data *chan_data = &dev_data->chan[channel];

	/* Validate channel state */
	if (chan_data->state != DW_DMA_ACTIVE) {
		ret = -EINVAL;
		goto out;
	}


	LOG_DBG("%s: dma %s channel %d suspend",
		__func__, dev->name, channel);

	dw_write(dev_cfg->base, DW_CFG_LOW(channel),
		      chan_data->cfg_lo | DW_CFGL_SUSPEND);

	/* Channel is now suspended */
	chan_data->state = DW_DMA_SUSPENDED;


out:
	return ret;
}


int dw_dma_setup(const struct device *dev)
{
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;

	int i, ret = 0;

	/* we cannot config DMAC if DMAC has been already enabled by host */
	if (dw_read(dev_cfg->base, DW_DMA_CFG) != 0) {
		dw_write(dev_cfg->base, DW_DMA_CFG, 0x0);
	}

	for (i = DW_DMA_CFG_TRIES; i > 0; i--) {
		if (!dw_read(dev_cfg->base, DW_DMA_CFG)) {
			break;
		}
	}

	if (!i) {
		LOG_ERR("%s: dma %s setup failed",
			__func__, dev->name);
		ret = -EIO;
		goto out;
	}

	LOG_DBG("%s: dma %s", __func__, dev->name);

	for (i = 0; i <  DW_CHAN_COUNT; i++) {
		dw_read(dev_cfg->base, DW_DMA_CHAN_EN);
	}


	/* enable the DMA controller */
	dw_write(dev_cfg->base, DW_DMA_CFG, 1);

	/* mask all interrupts for all 8 channels */
	dw_write(dev_cfg->base, DW_MASK_TFR, DW_CHAN_MASK_ALL);
	dw_write(dev_cfg->base, DW_MASK_BLOCK, DW_CHAN_MASK_ALL);
	dw_write(dev_cfg->base, DW_MASK_SRC_TRAN, DW_CHAN_MASK_ALL);
	dw_write(dev_cfg->base, DW_MASK_DST_TRAN, DW_CHAN_MASK_ALL);
	dw_write(dev_cfg->base, DW_MASK_ERR, DW_CHAN_MASK_ALL);

#ifdef CONFIG_DMA_DW_FIFO_PARTITION
	/* allocate FIFO partitions for each channel */
	dw_write(dev_cfg->base, DW_FIFO_PART1_HI,
		      DW_FIFO_CHx(DW_FIFO_SIZE) | DW_FIFO_CHy(DW_FIFO_SIZE));
	dw_write(dev_cfg->base, DW_FIFO_PART1_LO,
		      DW_FIFO_CHx(DW_FIFO_SIZE) | DW_FIFO_CHy(DW_FIFO_SIZE));
	dw_write(dev_cfg->base, DW_FIFO_PART0_HI,
		      DW_FIFO_CHx(DW_FIFO_SIZE) | DW_FIFO_CHy(DW_FIFO_SIZE));
	dw_write(dev_cfg->base, DW_FIFO_PART0_LO,
		      DW_FIFO_CHx(DW_FIFO_SIZE) | DW_FIFO_CHy(DW_FIFO_SIZE) |
		 DW_FIFO_UPD);
#endif /* CONFIG_DMA_DW_FIFO_PARTITION */

	/* TODO add baytrail/cherrytrail workaround */
out:
	return ret;
}

static int dw_dma_avail_data_size(uint32_t base,
				  struct dw_dma_chan_data *chan_data,
				  uint32_t channel)
{
	int32_t read_ptr = chan_data->ptr_data.current_ptr;
	int32_t write_ptr = dw_read(base, DW_DAR(channel));
	int32_t delta = write_ptr - chan_data->ptr_data.hw_ptr;
	int size;

	chan_data->ptr_data.hw_ptr = write_ptr;

	size = write_ptr - read_ptr;

	if (size < 0) {
		size += chan_data->ptr_data.buffer_bytes;
	} else if (!size) {
		/*
		 * Buffer is either full or empty. If the DMA pointer has
		 * changed, then the DMA has filled the buffer.
		 */
		if (delta) {
			size = chan_data->ptr_data.buffer_bytes;
		} else {
			LOG_DBG("%s size is 0!", __func__);
		}
	}

	LOG_DBG("DAR %x reader 0x%x free 0x%x avail 0x%x", write_ptr, read_ptr,
		chan_data->ptr_data.buffer_bytes - size, size);

	return size;
}

static int dw_dma_free_data_size(uint32_t base,
				 struct dw_dma_chan_data *chan_data,
				 uint32_t channel)
{
	int32_t read_ptr = dw_read(base, DW_SAR(channel));
	int32_t write_ptr = chan_data->ptr_data.current_ptr;
	int32_t delta = read_ptr - chan_data->ptr_data.hw_ptr;
	int size;

	chan_data->ptr_data.hw_ptr = read_ptr;

	size = read_ptr - write_ptr;
	if (size < 0) {
		size += chan_data->ptr_data.buffer_bytes;
	} else if (!size) {
		/*
		 * Buffer is either full or empty. If the DMA pointer has
		 * changed, then the DMA has emptied the buffer.
		 */
		if (delta) {
			size = chan_data->ptr_data.buffer_bytes;
		} else {
			LOG_DBG("%s size is 0!", __func__);
		}
	}

	LOG_DBG("SAR %x writer 0x%x free 0x%x avail 0x%x", read_ptr, write_ptr, size,
		chan_data->ptr_data.buffer_bytes - size);

	return size;
}

int dw_dma_get_status(const struct device *dev, uint32_t channel,
		      struct dma_status *stat)
{
	struct dw_dma_dev_data *const dev_data = dev->data;
	const struct dw_dma_dev_cfg *const dev_cfg = dev->config;
	struct dw_dma_chan_data *chan_data;

	if (channel >= DW_CHAN_COUNT) {
		return -EINVAL;
	}

	chan_data = &dev_data->chan[channel];

	if (chan_data->direction == MEMORY_TO_MEMORY ||
	    chan_data->direction == PERIPHERAL_TO_MEMORY) {
		stat->pending_length = dw_dma_avail_data_size(dev_cfg->base, chan_data, channel);
		stat->free = chan_data->ptr_data.buffer_bytes - stat->pending_length;

	} else {
		stat->free = dw_dma_free_data_size(dev_cfg->base, chan_data, channel);
		stat->pending_length = chan_data->ptr_data.buffer_bytes - stat->free;
	}
#if CONFIG_DMA_DW_HW_LLI
	if (!(dw_read(dev_cfg->base, DW_DMA_CHAN_EN) & DW_CHAN(channel))) {
		LOG_ERR("xrun detected");
		return -EPIPE;
	}
#endif
	return 0;
}
