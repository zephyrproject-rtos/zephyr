/**
 * SPDX-FileCopyrightText: Copyright (c) 2026 AMD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_SOC_ACP_7_0
#include <acp70_fw_scratch_mem.h>
#include <acp70_chip_offsets.h>
#include <acp70_chip_reg.h>
#endif

/* Used for driver binding */
#define DT_DRV_COMPAT amd_acp_tdm_dma
#define ACP_TDM_INSTANCES 3

/* Per-instance FIFO address assignments (non-overlapping SRAM regions) */
#define ACP_TDM0_FIFO_SIZE		512U
#define ACP_TDM0_TX_FIFO_ADDR		(ACP_TDM0_FIFO_SIZE * 2)
#define ACP_TDM0_RX_FIFO_ADDR		(ACP_TDM0_TX_FIFO_ADDR + ACP_TDM0_FIFO_SIZE)

#define ACP_TDM1_FIFO_SIZE		512U
#define ACP_TDM1_TX_FIFO_ADDR		(ACP_TDM1_FIFO_SIZE * 3)
#define ACP_TDM1_RX_FIFO_ADDR		(ACP_TDM1_TX_FIFO_ADDR + ACP_TDM1_FIFO_SIZE)

#define ACP_TDM2_FIFO_SIZE		512U
#define ACP_TDM2_TX_FIFO_ADDR		0x000U
#define ACP_TDM2_RX_FIFO_ADDR		(ACP_TDM2_TX_FIFO_ADDR + ACP_TDM2_FIFO_SIZE)

/* ACP DMA transfer size */
#define TDM_IER_DISABLE		0x0U

/* Invalid/unassigned sentinel values */
#define ACP_TDM_INVALID_32	0xFFFFFFFFU
#define ACP_TDM_INVALID_16	0xFFFFU

LOG_MODULE_REGISTER(tdm_dma_acp, CONFIG_DMA_LOG_LEVEL);

static struct acp_tdm_context acp_tdm_ctx[ACP_TDM_INSTANCES][2];
/* TDM offsets array for checking all instances */
static const uint32_t tdm_offsets[ACP_TDM_INSTANCES] = {
	ACP_TDM0_OFFSET,
	ACP_TDM1_OFFSET,
	ACP_TDM2_OFFSET
};

/**
 * @brief Check if all TDM instances have stopped (no TX/RX running)
 *
 * @return true if all TDM streams are stopped, false if any stream is active
 */
static bool acp_tdm_all_streams_stopped(void)
{
	acp_tdm_iter_t tdm_iter = { .u32all = 0 };
	acp_tdm_irer_t tdm_irer = { .u32all = 0 };

	for (int inst = 0; inst < ACP_TDM_INSTANCES; inst++) {
		tdm_iter = (acp_tdm_iter_t)io_reg_read(
			PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offsets[inst]);
		tdm_irer = (acp_tdm_irer_t)io_reg_read(
			PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offsets[inst]);

		if ((tdm_iter.bits.tdm_txen != 0U) || (tdm_irer.bits.tdm_rx_en != 0U)) {
			return false;
		}
	}
	return true;
}

static bool acp_pdm_streams_stopped(void)
{
	uint32_t acp_pdm_en = (uint32_t)io_reg_read(PU_REGISTER_BASE + ACP_WOV_PDM_ENABLE);

	if (acp_pdm_en != 0U) {
		return false;
	}
	return true;
}

/**
 * @brief Check if all SDW instances have stopped (no TX/RX running)
 *
 * @return true if all SDW streams are stopped, false if any stream is active
 */
static bool acp_sdw_all_streams_stopped(void)
{
	uint32_t sdw_en = 0;

	static const uint32_t sdw_en_regs[] = {
		ACP_SW_AUDIO_TX_EN, ACP_SW_BT_TX_EN, ACP_SW_HS_TX_EN,
		ACP_SW_AUDIO_RX_EN, ACP_SW_BT_RX_EN, ACP_SW_HS_RX_EN,
		ACP_P1_SW_AUDIO_TX_EN, ACP_P1_SW_BT_TX_EN, ACP_P1_SW_HEADSET_TX_EN,
		ACP_P1_SW_AUDIO_RX_EN, ACP_P1_SW_BT_RX_EN, ACP_P1_SW_HEADSET_RX_EN,
	};

	for (int i = 0; i < ARRAY_SIZE(sdw_en_regs); i++) {
		sdw_en |= io_reg_read(PU_REGISTER_BASE + sdw_en_regs[i]);
	}
	return (sdw_en == 0U);
}

static int acp_tdm_dma_config(const struct device *dev, uint32_t channel,
				     struct dma_config *cfg)
{
	LOG_DBG("config: ch=%d dir=%d blk_sz=%d blk_cnt=%d", channel,
		cfg->channel_direction, cfg->head_block->block_size,
		cfg->block_count);

	struct acp_dma_dev_data *dev_data = dev->data;
	uint32_t tdm_ringbuff_addr = 0, dir = ACP_TDM_INVALID_32;
	uint32_t tdm_fifo_addr_tx = 0, tdm_fifo_addr_rx = 0, tdm_fifosize = 0, tdm_offset = 0;
	uint32_t tdm_inst = ACP_TDM_INVALID_32;
	struct acp_tdm_context *ctx = NULL;
	struct {
		uint32_t tdm_instance;
		uint32_t pin_dir;
		uint32_t dma_channel;
		uint32_t index;
	} *tdm_data = NULL;

	dev_data->chan_data[channel].direction = cfg->channel_direction;
	tdm_data = dev_data->dai_index_ptr;
	if (cfg->channel_direction < 1 || cfg->channel_direction > 2) {
		LOG_ERR("config: invalid channel_direction %d", cfg->channel_direction);
		return -EINVAL;
	}

	dir = cfg->channel_direction - 1;
	LOG_DBG("config: valid dir=%d", cfg->channel_direction);
	for (int inst = 0; inst < ACP_TDM_INSTANCES; inst++) {
		if ((acp_tdm_ctx[inst][dir].dma_channel == ACP_TDM_INVALID_16) &&
		(inst == tdm_data->index)) {
			LOG_DBG("ctx assign: inst=%d dir=%d", inst, dir);
			acp_tdm_ctx[inst][dir].dma_channel = channel;
			acp_tdm_ctx[inst][dir].pin_dir = cfg->channel_direction;
			acp_tdm_ctx[inst][dir].index = tdm_data->index;
			break;
		}
	}

	for (int inst = 0; inst < ACP_TDM_INSTANCES; inst++) {
		if (acp_tdm_ctx[inst][dir].dma_channel == channel &&
		acp_tdm_ctx[inst][dir].pin_dir == cfg->channel_direction) {
			LOG_DBG("config: found ctx inst=%d dir=%d", inst, dir);
			ctx = &acp_tdm_ctx[inst][dir];
			tdm_inst = acp_tdm_ctx[inst][dir].index;
			break;
		}
	}
	if (!ctx) {
		LOG_ERR("config: ctx not found for ch=%d dir=%d", channel, cfg->channel_direction);
		return -EINVAL;
	}

	switch (tdm_inst) {
	case 0: /* HS */
		tdm_offset = ACP_I2S_TDM0_OFFSET;
		tdm_fifo_addr_tx = ACP_TDM0_TX_FIFO_ADDR;
		tdm_fifo_addr_rx = ACP_TDM0_RX_FIFO_ADDR;
		tdm_fifosize = ACP_TDM0_FIFO_SIZE;
		break;
	case 1: /* SP */
		tdm_offset = ACP_I2S_TDM1_OFFSET;
		tdm_fifo_addr_tx = ACP_TDM1_TX_FIFO_ADDR;
		tdm_fifo_addr_rx = ACP_TDM1_RX_FIFO_ADDR;
		tdm_fifosize = ACP_TDM1_FIFO_SIZE;
		break;
	case 2: /* BT */
		tdm_offset = ACP_I2S_TDM2_OFFSET;
		tdm_fifo_addr_tx = ACP_TDM2_TX_FIFO_ADDR;
		tdm_fifo_addr_rx = ACP_TDM2_RX_FIFO_ADDR;
		tdm_fifosize = ACP_TDM2_FIFO_SIZE;
		break;
	default:
		LOG_ERR("config: invalid idx=%d", tdm_inst);
		return -EINVAL;
	}

	switch (cfg->channel_direction) {
	case MEMORY_TO_PERIPHERAL:
		LOG_DBG("config: TX path (MEM->PERIPH)");
		ctx->buff_size = cfg->head_block->block_size * cfg->block_count;

		/* TDM Transmit FIFO Address and FIFO Size */
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_TX_FIFOADDR + tdm_offset),
			tdm_fifo_addr_tx);
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_TX_FIFOSIZE + tdm_offset),
			(uint32_t)(tdm_fifosize));

		/* Transmit RINGBUFFER Address and size */
		cfg->head_block->source_address =
		(cfg->head_block->source_address & ACP_DRAM_ADDRESS_MASK);
		tdm_ringbuff_addr = (cfg->head_block->source_address | ACP_SRAM);
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_TX_RINGBUFADDR + tdm_offset),
			tdm_ringbuff_addr);
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_TX_RINGBUFSIZE + tdm_offset),
			ctx->buff_size);

		/* Transmit DMA transfer size in bytes */
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_TX_DMA_SIZE + tdm_offset),
			(uint32_t)(ACP_DMA_TRANS_SIZE));

		/* Watermark size for TDM transfer fifo */
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_TX_INTR_WATERMARK_SIZE + tdm_offset),
			(ctx->buff_size >> 1));
		LOG_DBG("config: TX buf_sz=%d ring_addr=0x%x", ctx->buff_size, tdm_ringbuff_addr);
		break;

	case PERIPHERAL_TO_MEMORY:
		LOG_DBG("config: RX path (PERIPH->MEM)");
		ctx->buff_size = cfg->head_block->block_size * cfg->block_count;

		/* TDM Receive FIFO Address and FIFO Size */
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_RX_FIFOADDR + tdm_offset),
			tdm_fifo_addr_rx);
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_RX_FIFOSIZE + tdm_offset),
			tdm_fifosize);

		/* Receive RINGBUFFER Address and size */
		cfg->head_block->dest_address =
		(cfg->head_block->dest_address & ACP_DRAM_ADDRESS_MASK);
		tdm_ringbuff_addr = (cfg->head_block->dest_address | ACP_SRAM);
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_RX_RINGBUFADDR + tdm_offset),
			tdm_ringbuff_addr);
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_RX_RINGBUFSIZE + tdm_offset),
			ctx->buff_size);

		/* Receive DMA transfer size in bytes */
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_RX_DMA_SIZE + tdm_offset),
			(uint32_t)(ACP_DMA_TRANS_SIZE));

		/* Watermark size for TDM receive fifo */
		io_reg_write((PU_REGISTER_BASE + ACP_I2S_TDM_RX_INTR_WATERMARK_SIZE + tdm_offset),
			(ctx->buff_size >> 1));
		LOG_DBG("config: RX buf_sz=%d ring_addr=0x%x", ctx->buff_size, tdm_ringbuff_addr);
		break;

	default:
		LOG_ERR("config: invalid dir=%d", cfg->channel_direction);
		return -EINVAL;
	}

	return 0;
}

static int acp_tdm_dma_start(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];

	acp_tdm_ier_t tdm_ier;
	acp_tdm_iter_t tdm_iter = { .u32all = 0 };
	acp_tdm_irer_t tdm_irer = { .u32all = 0 };
	uint32_t tdm_offset;
	int dir = chan->direction - 1;
	int tdm_inst = ACP_TDM_INVALID_32;

	for (int inst = 0; inst < ACP_TDM_INSTANCES; inst++) {
		if (acp_tdm_ctx[inst][dir].dma_channel == channel &&
		acp_tdm_ctx[inst][dir].pin_dir == chan->direction) {
			tdm_inst = acp_tdm_ctx[inst][dir].index;
			acp_tdm_ctx[inst][dir].prev_pos = 0;
			break;
		}
	}

	LOG_DBG("start: idx=%d dir=%d", tdm_inst, chan->direction);

	switch (tdm_inst) {
	case 0: /* HS */
		tdm_offset = ACP_TDM0_OFFSET;
		break;
	case 1: /* SP */
		tdm_offset = ACP_TDM1_OFFSET;
		break;
	case 2: /* BT */
		tdm_offset = ACP_TDM2_OFFSET;
		break;
	default:
		LOG_ERR("start: invalid idx=%d ch=%d", tdm_inst, channel);
		return -EINVAL;
	}
	LOG_DBG("start: tdm_offset=0x%x", tdm_offset);

	if (acp_tdm_all_streams_stopped() && acp_sdw_all_streams_stopped() &&
	    acp_pdm_streams_stopped()) {
		acp_change_clock_notify(600000000);
		io_reg_write(PU_REGISTER_BASE + ACP_CLKMUX_SEL, ACP_ACLK_CLK_SEL);
	}
	if (chan->direction == MEMORY_TO_PERIPHERAL) {
		LOG_DBG("start: TX (MEM->PERIPH)");
		chan->state = ACP_DMA_ACTIVE;
		tdm_ier = (acp_tdm_ier_t)io_reg_read(PU_REGISTER_BASE + ACP_TDM_IER + tdm_offset);
		tdm_ier.bits.tdm_ien = 1U;
		io_reg_write((PU_REGISTER_BASE + ACP_TDM_IER + tdm_offset), tdm_ier.u32all);

		/* Re-read ITER to preserve protocol_mode set by DAI */
		tdm_iter.u32all = io_reg_read(PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offset);
		tdm_iter.bits.tdm_txen = 1U;
		tdm_iter.bits.tdm_tx_protocol_mode = 0U;
		tdm_iter.bits.tdm_tx_data_path_mode = 1U;
		tdm_iter.bits.tdm_tx_samp_len = 2U;
		io_reg_write((PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offset), tdm_iter.u32all);
	} else if (chan->direction == PERIPHERAL_TO_MEMORY) {
		LOG_DBG("start: RX (PERIPH->MEM)");
		chan->state = ACP_DMA_ACTIVE;
		tdm_ier = (acp_tdm_ier_t)io_reg_read(PU_REGISTER_BASE + ACP_TDM_IER + tdm_offset);
		tdm_ier.bits.tdm_ien = 1U;
		io_reg_write((PU_REGISTER_BASE + ACP_TDM_IER + tdm_offset), tdm_ier.u32all);

		/* Re-read IRER to preserve protocol_mode set by DAI */
		tdm_irer.u32all = io_reg_read(PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offset);
		tdm_irer.bits.tdm_rx_en = 1U;
		tdm_irer.bits.tdm_rx_protocol_mode = 0U;
		tdm_irer.bits.tdm_rx_data_path_mode = 1U;
		tdm_irer.bits.tdm_rx_samplen = 2U;
		io_reg_write((PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offset), tdm_irer.u32all);
	} else {
		LOG_ERR("start: invalid dir=%d ch=%d", chan->direction, channel);
		return -EINVAL;
	}

	return 0;
}

static bool acp_tdm_dma_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	uint32_t req_chan;

	if (!filter_param) {
		return true;
	}
	req_chan = *(uint32_t *)filter_param;
	LOG_DBG("filter: ch=%d req=%d", channel, req_chan);

	if (channel == req_chan) {
		return true;
	}
	return false;
}

static void acp_tdm_dma_chan_release(const struct device *dev, uint32_t channel)
{
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan_data = &dev_data->chan_data[channel];

	LOG_DBG("release: ch=%d", channel);
	chan_data->state = ACP_DMA_READY;
}

static int acp_tdm_dma_stop(const struct device *dev, uint32_t channel)
{
	acp_tdm_iter_t tdm_iter = { .u32all = 0 };
	acp_tdm_irer_t tdm_irer = { .u32all = 0 };
	uint32_t tdm_offset = ACP_TDM_INVALID_32;
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	int dir;
	int tdm_inst = ACP_TDM_INVALID_32;

	LOG_DBG("stop: state=%d dir=%d ch=%d", chan->state, chan->direction, channel);

	switch (chan->state) {
	case ACP_DMA_READY:
	case ACP_DMA_PREPARED:
		LOG_DBG("stop: already stopped");
		return 0;
	case ACP_DMA_SUSPENDED:
	case ACP_DMA_ACTIVE:
		break;
	default:
		LOG_ERR("stop: invalid DMA state %d ch=%d", chan->state, channel);
		return -EINVAL;
	}

	dir = chan->direction - 1;

	for (int inst = 0; inst < ACP_TDM_INSTANCES; inst++) {
		if (acp_tdm_ctx[inst][dir].dma_channel == channel &&
		acp_tdm_ctx[inst][dir].pin_dir == chan->direction) {
			tdm_inst = acp_tdm_ctx[inst][dir].index;
			break;
		}
	}

	LOG_DBG("stop: idx=%d dir=%d", tdm_inst, chan->direction);

	switch (tdm_inst) {
	case 0: /* HS */
		tdm_offset = ACP_TDM0_OFFSET;
		break;
	case 1: /* SP */
		tdm_offset = ACP_TDM1_OFFSET;
		break;
	case 2: /* BT */
		tdm_offset = ACP_TDM2_OFFSET;
		break;
	default:
		LOG_ERR("stop: invalid idx=%d ch=%d", tdm_inst, channel);
		return -EINVAL;
	}

	chan->state = ACP_DMA_READY;

	if (chan->direction == MEMORY_TO_PERIPHERAL) {
		LOG_DBG("stop: TX");
		tdm_iter = (acp_tdm_iter_t)io_reg_read(
			PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offset);
		tdm_iter.bits.tdm_txen = 0U;
		io_reg_write((PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offset), tdm_iter.u32all);
	} else if (chan->direction == PERIPHERAL_TO_MEMORY) {
		LOG_DBG("stop: RX");
		tdm_irer = (acp_tdm_irer_t)io_reg_read(
			PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offset);
		tdm_irer.bits.tdm_rx_en = 0U;
		io_reg_write((PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offset), tdm_irer.u32all);
	} else {
		LOG_ERR("stop: invalid dir=%d ch=%d", chan->direction, channel);
		return -EINVAL;
	}

	/* Re-read both to check if TX and RX are both off for this instance */
	tdm_iter = (acp_tdm_iter_t)io_reg_read(
		PU_REGISTER_BASE + ACP_TDM_ITER + tdm_offset);
	tdm_irer = (acp_tdm_irer_t)io_reg_read(
		PU_REGISTER_BASE + ACP_TDM_IRER + tdm_offset);
	if (!tdm_iter.bits.tdm_txen && !tdm_irer.bits.tdm_rx_en) {
		LOG_DBG("stop: TX/RX disabled, disabling IRQ");
		io_reg_write((PU_REGISTER_BASE + ACP_TDM_IER + tdm_offset), TDM_IER_DISABLE);
	}
	if (acp_tdm_all_streams_stopped() && acp_sdw_all_streams_stopped() &&
	    acp_pdm_streams_stopped()) {
		io_reg_write(PU_REGISTER_BASE + ACP_CLKMUX_SEL, ACP_INTERNAL_CLK_SEL);
		acp_change_clock_notify(0);
	}
	dir = chan->direction - 1;
	for (int inst = 0; inst < ACP_TDM_INSTANCES; inst++) {
		if (acp_tdm_ctx[inst][dir].dma_channel == channel &&
			acp_tdm_ctx[inst][dir].pin_dir == chan->direction) {
			acp_tdm_ctx[inst][dir].index = ACP_TDM_INVALID_16;
			acp_tdm_ctx[inst][dir].pin_dir = ACP_TDM_INVALID_16;
			acp_tdm_ctx[inst][dir].dma_channel = ACP_TDM_INVALID_16;
			acp_tdm_ctx[inst][dir].prev_pos = 0;
			break;
		}
	}
	return 0;
}

static int acp_tdm_dma_resume(const struct device *dev, uint32_t channel)
{
	return 0;
}

static int acp_tdm_dma_suspend(const struct device *dev, uint32_t channel)
{
	return 0;
}

static int acp_tdm_dma_reload(const struct device *dev, uint32_t channel,
				     uint32_t src, uint32_t dst, size_t bytes)
{
	return 0;
}

static int acp_tdm_dma_get_status(const struct device *dev, uint32_t channel,
					  struct dma_status *stat)
{
	uint32_t tdm_offset = ACP_TDM_INVALID_32;
	uint32_t tdm_inst = ACP_TDM_INVALID_32;
	struct acp_tdm_context *ctx = NULL;
	struct acp_dma_dev_data *const dev_data = dev->data;
	struct acp_dma_chan_data *chan = &dev_data->chan_data[channel];
	uint32_t dir;

	LOG_DBG("get_status: ch=%d", channel);

	if (channel >= dev_data->dma_ctx.dma_channels) {
		LOG_ERR("get_status: invalid channel %d (max %d)",
			channel, dev_data->dma_ctx.dma_channels);
		return -EINVAL;
	}

	dir = chan->direction - 1;

	for (int inst = 0; inst < ACP_TDM_INSTANCES; inst++) {
		if (acp_tdm_ctx[inst][dir].dma_channel == channel &&
		acp_tdm_ctx[inst][dir].pin_dir == chan->direction) {
			ctx = &acp_tdm_ctx[inst][dir];
			tdm_inst = acp_tdm_ctx[inst][dir].index;
			break;
		}
	}
	if (!ctx) {
		LOG_ERR("get_status: ctx not found ch=%d", channel);
		return -EINVAL;
	}

	LOG_DBG("get_status: ctx found idx=%d dir=%d", tdm_inst, chan->direction);

	switch (tdm_inst) {
	case 0: /* HS */
		tdm_offset = ACP_I2S_TDM0_OFFSET;
		break;
	case 1: /* SP */
		tdm_offset = ACP_I2S_TDM1_OFFSET;
		break;
	case 2: /* BT */
		tdm_offset = ACP_I2S_TDM2_OFFSET;
		break;
	default:
		LOG_ERR("get_status: invalid idx=%d ch=%d", tdm_inst, channel);
		return -EINVAL;
	}

	if (chan->direction == MEMORY_TO_PERIPHERAL) {
		LOG_DBG("get_status: tx idx=%d", tdm_inst);
		uint64_t curr_tx_pos = 0;
		uint32_t tx_low = 0, tx_high = 0;

		tx_low = (uint32_t)io_reg_read(PU_REGISTER_BASE +
		ACP_I2S_TDM_TX_LINEARPOSITIONCNTR_LOW + tdm_offset);
		tx_high = (uint32_t)io_reg_read(PU_REGISTER_BASE +
		ACP_I2S_TDM_TX_LINEARPOSITIONCNTR_HIGH + tdm_offset);
		curr_tx_pos = (uint64_t)(((uint64_t)tx_high << 32) | tx_low);

		stat->free = (int32_t)(curr_tx_pos - ctx->prev_pos) > 0 ?
		(curr_tx_pos - ctx->prev_pos) :
		(curr_tx_pos) + (ctx->buff_size - ctx->prev_pos);
		stat->pending_length = ctx->buff_size - stat->free;

		if (stat->free >= (ctx->buff_size >> 1)) {
			ctx->prev_pos += ctx->buff_size >> 1;
		} else {
			stat->free = 0;
		}
	} else if (chan->direction == PERIPHERAL_TO_MEMORY) {
		LOG_DBG("get_status: rx idx=%d", tdm_inst);
		uint64_t curr_rx_pos = 0;
		uint32_t rx_low = 0, rx_high = 0;

		rx_low = (uint32_t)io_reg_read(PU_REGISTER_BASE +
		ACP_I2S_TDM_RX_LINEARPOSITIONCNTR_LOW + tdm_offset);
		rx_high = (uint32_t)io_reg_read(PU_REGISTER_BASE +
		ACP_I2S_TDM_RX_LINEARPOSITIONCNTR_HIGH + tdm_offset);
		curr_rx_pos = (uint64_t)(((uint64_t)rx_high << 32) | rx_low);
		stat->pending_length = (int32_t)(curr_rx_pos - ctx->prev_pos);

		if (stat->pending_length >= (ctx->buff_size >> 1)) {
			ctx->prev_pos += (ctx->buff_size >> 1);
		} else {
			stat->pending_length = 0;
		}
		stat->free = ctx->buff_size - stat->pending_length;
	} else {
		LOG_ERR("get_status: invalid dir=%d ch=%d", chan->direction, channel);
		return -EINVAL;
	}

	LOG_DBG("get_status: OK ch=%d free=%d pend=%d", channel, stat->free, stat->pending_length);
	return 0;
}

static int acp_tdm_dma_get_attribute(const struct device *dev, uint32_t type,
					 uint32_t *value)
{
	LOG_DBG("TDM get_attr: type=%d", type);

	switch (type) {
	case DMA_ATTR_COPY_ALIGNMENT:
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
		*value = CONFIG_DMA_AMD_ACP_TDM_BUFFER_ALIGN;
		break;
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*value = CONFIG_DMA_AMD_ACP_TDM_BUFFER_ADDRESS_ALIGN;
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		*value = 2U;
		break;
	default:
		LOG_ERR("get_attribute: unsupported type %d", type);
		return -EINVAL;
	}
	return 0;
}

static int acp_tdm_dma_init(const struct device *dev)
{
	struct acp_dma_dev_data *const dev_data = dev->data;

	LOG_DBG("init: TDM DMA driver");
	dev_data->dma_ctx.magic = DMA_MAGIC;
	dev_data->dma_ctx.dma_channels = CONFIG_DMA_AMD_ACP_TDM_CHANNEL_COUNT;
	dev_data->dma_ctx.atomic = dev_data->atomic;
	for (int dir = 0; dir < 2; dir++) {
		for (int inst = 0; inst < ACP_TDM_INSTANCES; inst++) {
			acp_tdm_ctx[inst][dir].index = ACP_TDM_INVALID_16;
			acp_tdm_ctx[inst][dir].pin_dir = ACP_TDM_INVALID_16;
			acp_tdm_ctx[inst][dir].dma_channel = ACP_TDM_INVALID_16;
			acp_tdm_ctx[inst][dir].prev_pos = 0;
		}
	}
	return 0;
}

static DEVICE_API(dma, acptdm_dma_ops) = {
	.config = acp_tdm_dma_config,
	.start = acp_tdm_dma_start,
	.stop = acp_tdm_dma_stop,
	.chan_filter = acp_tdm_dma_chan_filter,
	.chan_release = acp_tdm_dma_chan_release,
	.reload = acp_tdm_dma_reload,
	.suspend = acp_tdm_dma_suspend,
	.resume = acp_tdm_dma_resume,
	.get_status = acp_tdm_dma_get_status,
	.get_attribute = acp_tdm_dma_get_attribute,
};

static struct acp_dma_dev_data acp_tdm_dma_data;

DEVICE_DT_DEFINE(DT_NODELABEL(acp_tdm_dma),
		 &acp_tdm_dma_init,
		 NULL,
		 &acp_tdm_dma_data,
		 NULL, POST_KERNEL,
		 CONFIG_DMA_INIT_PRIORITY,
		 &acptdm_dma_ops);
