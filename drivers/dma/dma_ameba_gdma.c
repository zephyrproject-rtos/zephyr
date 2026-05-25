/*
 * Copyright (c) 2026 Realtek Semiconductor Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT realtek_ameba_gdma

/* Include <soc.h> before <ameba_soc.h> to avoid redefining unlikely() macro */
#include <soc.h>
#include <ameba_soc.h>

#include "dma_ameba_gdma.h"
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dma_ameba_gdma, CONFIG_DMA_LOG_LEVEL);

enum dma_reload_type {
	GDMA_single = 0,
	GDMA_reload_dst,
	GDMA_reload_src,
	GDMA_reload_src_dst,
};

struct dma_ameba_channel {
	uint32_t block_size;
	uint32_t block_num;
	uint32_t block_id;
	uint32_t src_trans_width;
	uint32_t dst_trans_width;
	uint32_t src_addr;
	uint32_t dst_addr;
	uint32_t chnl_priority;
	GDMA_InitTypeDef gdma_struct;
	struct dma_block_config *cur_dma_blk_cfg;
	dma_callback_t callback;
	void *user_data;
	struct GDMA_CH_LLI *link_node;
	uint8_t reload_type;
	uint8_t chnl_direction;
	bool busy;
};

struct dma_ameba_data {
	struct dma_context dma_ctx;
	/* define Bitmap variable for channel request */
	ATOMIC_DEFINE(channels_atomic, DT_INST_PROP(0, dma_channels));
	struct dma_ameba_channel *channel_status;
#ifdef CONFIG_DMA_AMEBA_LLI
	struct GDMA_CH_LLI *lli_pool;
#endif
};

struct dma_ameba_config {
	uint32_t base;
	uint8_t channel_num;
	uint8_t instance_id;
	void (*config_irq)(const struct device *dev);
	struct device *src_dev;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

static inline int dma_ameba_data_size_convert(uint32_t data_size)
{
	switch (data_size) {
	case 1:
		return TrWidthOneByte;
	case 2:
		return TrWidthTwoBytes;
	case 4:
		return TrWidthFourBytes;
	default:
		LOG_ERR("data_size must be 1, 2, or 4 (size = %d)", data_size);
		return -EINVAL;
	}
}

static inline int dma_ameba_burst_size_convert(uint32_t burst_size)
{
	switch (burst_size) {
	case 1:
		return MsizeOne;
	case 4:
		return MsizeFour;
	case 8:
		return MsizeEight;
	case 16:
		return MsizeSixteen;
	default:
		LOG_ERR("burst size must be 1, 4, 8, 16 (size = %d)", burst_size);
		return -EINVAL;
	}
}

static inline int dma_ameba_reload_type_get(struct dma_config *config_dma)
{
	enum dma_reload_type type;

	if (config_dma->head_block->source_reload_en && config_dma->head_block->dest_reload_en) {
		type = GDMA_reload_src_dst;
	} else if (config_dma->head_block->source_reload_en) {
		type = GDMA_reload_src;
	} else if (config_dma->head_block->dest_reload_en) {
		type = GDMA_reload_dst;
	} else {
		type = GDMA_single;
	}
	return type;
}

static int dma_ameba_blockcfg_update(const struct device *dev, uint32_t channel)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;

	/* 1. config channel width/burst length/addr change mode/block size */
	data->channel_status[channel].gdma_struct.GDMA_SrcAddr =
		data->channel_status[channel].cur_dma_blk_cfg->source_address;
	data->channel_status[channel].gdma_struct.GDMA_DstAddr =
		data->channel_status[channel].cur_dma_blk_cfg->dest_address;
	if (data->channel_status[channel].cur_dma_blk_cfg->source_addr_adj ==
		    DMA_ADDR_ADJ_DECREMENT ||
	    data->channel_status[channel].cur_dma_blk_cfg->dest_addr_adj ==
		    DMA_ADDR_ADJ_DECREMENT) {
		LOG_ERR("dma does not support addr decrement mode");
		return -ENOTSUP;
	}
	data->channel_status[channel].gdma_struct.GDMA_SrcInc =
		data->channel_status[channel].cur_dma_blk_cfg->source_addr_adj;
	data->channel_status[channel].gdma_struct.GDMA_DstInc =
		data->channel_status[channel].cur_dma_blk_cfg->dest_addr_adj;

	/* set single block size = data block size / src_data_width */
	data->channel_status[channel].gdma_struct.GDMA_BlockSize =
		data->channel_status[channel].cur_dma_blk_cfg->block_size >>
		data->channel_status[channel].gdma_struct.GDMA_SrcDataWidth;

	/* 2. Continuous block(Adjacent address spaces) mode*/
	if (data->channel_status[channel].cur_dma_blk_cfg->dest_reload_en ||
	    data->channel_status[channel].cur_dma_blk_cfg->source_reload_en) {
		data->channel_status[channel].gdma_struct.GDMA_ReloadSrc =
			data->channel_status[channel].cur_dma_blk_cfg->source_reload_en;
		data->channel_status[channel].gdma_struct.GDMA_ReloadDst =
			data->channel_status[channel].cur_dma_blk_cfg->dest_reload_en;
	}

#ifdef CONFIG_DMA_AMEBA_SCATTER_GATHER
	/* 3. scatter and gather */
	if (data->channel_status[channel].cur_dma_blk_cfg->source_gather_en &&
	    data->channel_status[channel].cur_dma_blk_cfg->dest_scatter_en) {
		LOG_ERR("The same channel cannot enable source gather and dest scatter at the same "
			"time");
		return -EPERM;
	} else if (data->channel_status[channel].cur_dma_blk_cfg->source_gather_en) {
		GDMA_SourceGather(
			config->instance_id, channel,
			data->channel_status[channel].cur_dma_blk_cfg->source_gather_count,
			data->channel_status[channel].cur_dma_blk_cfg->source_gather_interval);
	} else if (data->channel_status[channel].cur_dma_blk_cfg->dest_scatter_en) {
		GDMA_DestinationScatter(
			config->instance_id, channel,
			data->channel_status[channel].cur_dma_blk_cfg->dest_scatter_count,
			data->channel_status[channel].cur_dma_blk_cfg->dest_scatter_interval);
	}
	/* else: normal DMA mode without scatter/gather - do nothing */
#endif
	/* 3. Initialization. */
	GDMA_SetChnlPriority(config->instance_id, channel,
			     data->channel_status[channel].chnl_priority);
	GDMA_Init(config->instance_id, channel, &data->channel_status[channel].gdma_struct);

	data->channel_status[channel].block_size =
		data->channel_status[channel].gdma_struct.GDMA_BlockSize;
	data->channel_status[channel].dst_addr =
		data->channel_status[channel].gdma_struct.GDMA_DstAddr;
	data->channel_status[channel].src_addr =
		data->channel_status[channel].gdma_struct.GDMA_SrcAddr;

	return 0;
}

static void dma_ameba_isr_handler(const struct device *dev, uint32_t channel)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;
	int err = 0;
	uint32_t isr_type;

	/* If multiple blocks perform reload transmission, the reload bit needs to be cleared
	 * when transmitting to the second block in reverse order.
	 */
	if (data->channel_status[channel].block_num == data->channel_status[channel].block_id + 2) {
		GDMA_ChCleanAutoReload(config->instance_id, channel, CLEAN_RELOAD_SRC_DST);
	}
	/* 1. DMA interrupt type.*/
	isr_type = GDMA_ClearINT(config->instance_id, channel);

	if (isr_type & BlockType) {
		LOG_DBG("dma block %d transfer complete.", data->channel_status[channel].block_id);
		data->channel_status[channel].block_id++;
	}
	/* transfer complete*/
	if (isr_type == TransferType) {
		data->channel_status[channel].busy = false;
	}
	/* transfer error(protocol error)*/
	if (isr_type == ErrType) {
		err = -EIO;
	}
	/* 2. Configure dma transfer if there are multiple DMA configuration blocks */
	if (data->channel_status[channel].cur_dma_blk_cfg == NULL) {
		LOG_ERR("cur_dma_blk_cfg is NULL");
		return;
	}
	if (data->channel_status[channel].cur_dma_blk_cfg->next_block != NULL) {
		/*update dma configuration and re-init dma*/
		data->channel_status[channel].cur_dma_blk_cfg =
			data->channel_status[channel].cur_dma_blk_cfg->next_block;
		GDMA_Cmd(config->instance_id, channel, DISABLE);
		dma_ameba_blockcfg_update(dev, channel);
		GDMA_Cmd(config->instance_id, channel, ENABLE);
	} else {
		/* 3. Execute user call back function.*/
		if (data->channel_status[channel].callback) {
			data->channel_status[channel].callback(
				dev, data->channel_status[channel].user_data, channel, err);
		}
	}
}

/* 1. Extract parameter validation */
static int dma_ameba_validate_config(const struct dma_ameba_config *config, uint32_t channel,
				     struct dma_config *config_dma)
{
	if (!config_dma) {
		LOG_ERR("invalid dma config parameters");
		return -EINVAL;
	}

	if (channel >= config->channel_num) {
		LOG_ERR("channel id must be < (%d)", config->channel_num);
		return -EINVAL;
	}

	if (config_dma->channel_direction > PERIPHERAL_TO_MEMORY) {
		LOG_ERR("channel_direction (%d) is not supported", config_dma->channel_direction);
		return -ENOTSUP;
	}

	if (config_dma->head_block->fifo_mode_control) {
		LOG_ERR("fifo_mode_control settings is not supported");
		return -ENOTSUP;
	}

	if (config_dma->channel_priority >= config->channel_num) {
		LOG_ERR("channel_priority must be < (%d)", config->channel_num);
		return -EINVAL;
	}

	if (config_dma->head_block->source_addr_adj == DMA_ADDR_ADJ_DECREMENT ||
	    config_dma->head_block->dest_addr_adj == DMA_ADDR_ADJ_DECREMENT) {
		LOG_ERR("dma does not support addr decrement mode");
		return -ENOTSUP;
	}

	return 0;
}

/* 2. Extract basic channel configurations (direction, width, burst) */
static int dma_ameba_config_basic(struct dma_ameba_data *data, uint32_t channel,
				  struct dma_config *config_dma)
{
	int ret;

	/* Configure direction and handshake interface */
	if (config_dma->channel_direction == MEMORY_TO_PERIPHERAL) {
		if (config_dma->head_block->flow_control_mode) {
			data->channel_status[channel].gdma_struct.GDMA_DIR = TTFCMemToPeri_PerCtrl;
		}
		if (!config_dma->dest_handshake) {
			data->channel_status[channel].gdma_struct.GDMA_DstHandshakeInterface =
				config_dma->dma_slot;
		} else {
			LOG_ERR("dma does not support software handshake");
			return -ENOTSUP;
		}
	} else if (config_dma->channel_direction == PERIPHERAL_TO_MEMORY) {
		if (config_dma->head_block->flow_control_mode) {
			data->channel_status[channel].gdma_struct.GDMA_DIR = TTFCPeriToMem_PerCtrl;
		}
		if (!config_dma->source_handshake) {
			data->channel_status[channel].gdma_struct.GDMA_SrcHandshakeInterface =
				config_dma->dma_slot;
		} else {
			LOG_ERR("dma does not support software handshake");
			return -ENOTSUP;
		}
	} else {
		/* Do nothing */
	}

	/* Configure width and burst size */
	ret = dma_ameba_data_size_convert(config_dma->source_data_size);
	if (ret != -EINVAL) {
		data->channel_status[channel].gdma_struct.GDMA_SrcDataWidth = ret;
	} else {
		return ret;
	}

	ret = dma_ameba_data_size_convert(config_dma->dest_data_size);
	if (ret != -EINVAL) {
		data->channel_status[channel].gdma_struct.GDMA_DstDataWidth = ret;
	} else {
		return ret;
	}

	ret = dma_ameba_burst_size_convert(config_dma->source_burst_length);
	if (ret != -EINVAL) {
		data->channel_status[channel].gdma_struct.GDMA_SrcMsize = ret;
	} else {
		return ret;
	}

	ret = dma_ameba_burst_size_convert(config_dma->dest_burst_length);
	if (ret != -EINVAL) {
		data->channel_status[channel].gdma_struct.GDMA_DstMsize = ret;
	} else {
		return ret;
	}

	return 0;
}

#ifdef CONFIG_DMA_AMEBA_LLI
/* 3. Configure multi-block (LLI) transfer */
static int dma_ameba_config_lli(struct dma_ameba_data *data, uint32_t channel,
				struct dma_config *config_dma, struct GDMA_CH_LLI *lli_pool)
{
	struct dma_block_config *cur_block = config_dma->head_block;
	struct dma_ameba_channel *ch = &data->channel_status[channel];

	if (config_dma->head_block->next_block && (config_dma->block_count <= 1)) {
		LOG_ERR("The block_count does not match the dma_block_cfg configuration.");
		return -EINVAL;
	}

	if (config_dma->block_count > CONFIG_DMA_AMEBA_LLI_MAX_DESC) {
		LOG_ERR("block_count %d exceeds max descriptor %d",
			config_dma->block_count, CONFIG_DMA_AMEBA_LLI_MAX_DESC);
		return -EINVAL;
	}

	ch->gdma_struct.GDMA_LlpSrcEn =
		(config_dma->head_block->source_addr_adj == DMA_ADDR_ADJ_INCREMENT);
	ch->gdma_struct.GDMA_LlpDstEn =
		(config_dma->head_block->dest_addr_adj == DMA_ADDR_ADJ_INCREMENT);

	ch->link_node = lli_pool;

	for (int i = 0; i < config_dma->block_count && cur_block; i++) {
		if (ch->gdma_struct.GDMA_LlpSrcEn) {
			ch->link_node[i].LliEle.Sarx = cur_block->source_address;
		}

		if (ch->gdma_struct.GDMA_LlpDstEn) {
			ch->link_node[i].LliEle.Darx = cur_block->dest_address;
		}

		/* Link list concatenation */
		if ((i == config_dma->block_count - 1) && (config_dma->cyclic == 1)) {
			ch->link_node[i].pNextLli = &ch->link_node[0];
		} else if ((i == config_dma->block_count - 1) && (config_dma->cyclic == 0)) {
			ch->link_node[i].pNextLli = NULL;
		} else {
			ch->link_node[i].pNextLli = &ch->link_node[i + 1];
		}

		ch->link_node[i].BlockSize =
			cur_block->block_size >> ch->gdma_struct.GDMA_SrcDataWidth;

		cur_block = cur_block->next_block;
	}

	return 0;
}
#endif

static int dma_ameba_configure(const struct device *dev, uint32_t channel,
			       struct dma_config *config_dma)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;
	int ret;

	/* 1. Check parameters */
	ret = dma_ameba_validate_config(config, channel, config_dma);
	if (ret != 0) {
		return ret;
	}

	/* 2. Init GDMA struct and generic properties */
	GDMA_StructInit(&data->channel_status[channel].gdma_struct);
	data->channel_status[channel].gdma_struct.GDMA_Index = config->instance_id;
	data->channel_status[channel].gdma_struct.GDMA_ChNum = channel;
	data->channel_status[channel].gdma_struct.GDMA_DIR = config_dma->channel_direction;
	data->channel_status[channel].gdma_struct.GDMA_SrcAddr =
		config_dma->head_block->source_address;
	data->channel_status[channel].gdma_struct.GDMA_DstAddr =
		config_dma->head_block->dest_address;
	data->channel_status[channel].gdma_struct.GDMA_SrcInc =
		config_dma->head_block->source_addr_adj;
	data->channel_status[channel].gdma_struct.GDMA_DstInc =
		config_dma->head_block->dest_addr_adj;

	/* 3. Configure channel basic settings */
	ret = dma_ameba_config_basic(data, channel, config_dma);
	if (ret != 0) {
		return ret;
	}

	data->channel_status[channel].gdma_struct.GDMA_BlockSize =
		config_dma->head_block->block_size >>
		data->channel_status[channel].gdma_struct.GDMA_SrcDataWidth;
	if (config_dma->head_block->dest_reload_en || config_dma->head_block->source_reload_en) {
		data->channel_status[channel].gdma_struct.GDMA_ReloadSrc =
			config_dma->head_block->source_reload_en;
		data->channel_status[channel].gdma_struct.GDMA_ReloadDst =
			config_dma->head_block->dest_reload_en;
	}

	/* 4. Multi-block (LLI) transfer mode */
#ifdef CONFIG_DMA_AMEBA_LLI
	if (config_dma->block_count > 1) {
		ret = dma_ameba_config_lli(data, channel, config_dma, data->lli_pool);
		if (ret != 0) {
			return ret;
		}
	}
#endif

	/* 5. Scatter and gather mode */
#ifdef CONFIG_DMA_AMEBA_SCATTER_GATHER
	if (config_dma->head_block->source_gather_en && config_dma->head_block->dest_scatter_en) {
		LOG_ERR("Cannot enable source gather and dest scatter at the same time");
		return -EPERM;
	} else if (config_dma->head_block->source_gather_en) {
		GDMA_SourceGather(config->instance_id, channel,
				  config_dma->head_block->source_gather_count,
				  config_dma->head_block->source_gather_interval);
	} else if (config_dma->head_block->dest_scatter_en) {
		GDMA_DestinationScatter(config->instance_id, channel,
					config_dma->head_block->dest_scatter_count,
					config_dma->head_block->dest_scatter_interval);
	}
	/* else: normal DMA mode without scatter/gather - do nothing */
#endif

	/* 6. Initialization and channel status recording */
	data->channel_status[channel].gdma_struct.GDMA_IsrType =
		(TransferType | BlockType | ErrType);
	GDMA_Init(config->instance_id, channel, &data->channel_status[channel].gdma_struct);
	GDMA_SetChnlPriority(config->instance_id, channel, config_dma->channel_priority);
	if (data->channel_status[channel].link_node != NULL) {
		GDMA_SetLLP(config->instance_id, channel, config_dma->block_count,
			    data->channel_status[channel].link_node, config_dma->cyclic);
	}

	data->channel_status[channel].block_num = config_dma->block_count;
	data->channel_status[channel].block_size = config_dma->head_block->block_size;
	data->channel_status[channel].chnl_direction = config_dma->channel_direction;
	data->channel_status[channel].chnl_priority = config_dma->channel_priority;
	data->channel_status[channel].src_trans_width =
		data->channel_status[channel].gdma_struct.GDMA_SrcDataWidth;
	data->channel_status[channel].dst_trans_width =
		data->channel_status[channel].gdma_struct.GDMA_DstDataWidth;
	data->channel_status[channel].dst_addr = config_dma->head_block->dest_address;
	data->channel_status[channel].src_addr = config_dma->head_block->source_address;
	data->channel_status[channel].callback = config_dma->dma_callback;
	data->channel_status[channel].user_data = config_dma->user_data;
	data->channel_status[channel].reload_type = dma_ameba_reload_type_get(config_dma);
	data->channel_status[channel].cur_dma_blk_cfg = config_dma->head_block;

	return 0;
}

static int dma_ameba_start(const struct device *dev, uint32_t channel)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;

	if (channel >= config->channel_num) {
		LOG_ERR("channel id must be < (%d)", config->channel_num);
		return -EINVAL;
	}
	/* channel enable */
	GDMA_Cmd(config->instance_id, channel, ENABLE);

	data->channel_status[channel].busy = true;

	return 0;
}

static int dma_ameba_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;

	if (channel >= config->channel_num) {
		LOG_ERR("channel id must be < (%d)", config->channel_num);
		return -EINVAL;
	}
	/* channel disable */
	GDMA_Cmd(config->instance_id, channel, DISABLE);

	data->channel_status[channel].busy = false;

#ifdef CONFIG_DMA_AMEBA_LLI
	/* Reset link_node to indicate no active LLI transfer */
	data->channel_status[channel].link_node = NULL;
#endif

	return 0;
}

static int dma_ameba_get_status(const struct device *dev, uint32_t channel,
				struct dma_status *status)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;

	if (channel >= config->channel_num) {
		LOG_ERR("channel id must be < (%d)", config->channel_num);
		return -EINVAL;
	}

	if (!status) {
		LOG_ERR("status is NULL");
		return -EINVAL;
	}

	status->busy = data->channel_status[channel].busy;
	/* In the actual test, only the total amount of data can be read
	 * if it is greater than 2048 Bytes.
	 */
	status->pending_length = data->channel_status[channel].block_size -
				 GDMA_GetBlkSize(config->instance_id, channel);
	status->dir = data->channel_status[channel].chnl_direction;

	return 0;
}

static int dma_ameba_reload(const struct device *dev, uint32_t channel, uint32_t src, uint32_t dst,
			    size_t size)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;

	if (channel >= config->channel_num) {
		LOG_ERR("channel id must be < (%d)", config->channel_num);
		return -EINVAL;
	}
	/* update current channel's parameters. */
	data->channel_status[channel].src_addr = src;
	data->channel_status[channel].dst_addr = dst;
	data->channel_status[channel].block_size = size;

	GDMA_Cmd(config->instance_id, channel, DISABLE);
	GDMA_SetSrcAddr(config->instance_id, channel, src);
	GDMA_SetDstAddr(config->instance_id, channel, dst);
	/* The size required by the gdma hardware must be aligned
	 * with the source transmission width.
	 */
	size = size >> data->channel_status[channel].src_trans_width;
	GDMA_SetBlkSize(config->instance_id, channel, (uint32_t)size);

	return 0;
}

static int dma_ameba_suspend(const struct device *dev, uint32_t channel)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;

	if (channel >= config->channel_num) {
		LOG_ERR("channel id must be < (%d)", config->channel_num);
		return -EINVAL;
	}

	if (data->channel_status[channel].busy == false) {
		LOG_ERR("suspend channel not busy");
		return -EINVAL;
	}

	GDMA_Suspend(config->instance_id, channel);

	data->channel_status[channel].busy = false;

	return 0;
}

static int dma_ameba_resume(const struct device *dev, uint32_t channel)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;

	if (channel >= config->channel_num) {
		LOG_ERR("channel id must be < (%d)", config->channel_num);
		return -EINVAL;
	}

	if (data->channel_status[channel].busy == true) {
		LOG_ERR("resume channel is busy");
		return -EINVAL;
	}

	GDMA_Resume(config->instance_id, channel);

	data->channel_status[channel].busy = true;

	return 0;
}

static bool dma_ameba_chan_filter(const struct device *dev, int channel, void *filter_param)
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

static int dma_ameba_init(const struct device *dev)
{
	const struct dma_ameba_config *config = (const struct dma_ameba_config *)dev->config;
	struct dma_ameba_data *data = (struct dma_ameba_data *)dev->data;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Could not initialize clock (%d)", ret);
		return ret;
	}

	for (uint32_t i = 0; i < config->channel_num; i++) {
		data->channel_status[i].reload_type = GDMA_single;
	}

	data->dma_ctx.dma_channels = config->channel_num;
	data->dma_ctx.magic = DMA_MAGIC;
	data->dma_ctx.atomic = data->channels_atomic;
	config->config_irq(dev);

	return 0;
}

static DEVICE_API(dma, dma_ameba_api) = {
	.config = dma_ameba_configure,
	.start = dma_ameba_start,
	.stop = dma_ameba_stop,
	.suspend = dma_ameba_suspend,
	.resume = dma_ameba_resume,
	.get_status = dma_ameba_get_status,
	.reload = dma_ameba_reload,
	.chan_filter = dma_ameba_chan_filter,
};

/*
 * Macro to CONNECT and enable each irq (order is given by the 'listify')
 * chan: channel of the DMA instance (assuming one irq per channel)
 * dma : dma instance (one GDMA instance on ameba)
 */
#define DMA_AMEBA_IRQ_CONNECT_CHANNEL(chan, dma)                                                   \
	do {                                                                                       \
		IRQ_CONNECT(DT_INST_IRQ_BY_IDX(dma, chan, irq),                                    \
			    DT_INST_IRQ_BY_IDX(dma, chan, priority), dma_ameba_irq_##dma##_##chan, \
			    DEVICE_DT_INST_GET(dma), 0);                                           \
		irq_enable(DT_INST_IRQ_BY_IDX(dma, chan, irq));                                    \
	} while (0)

/*
 * Macro to configure the irq for each dma instance (n)
 * Loop to CONNECT and enable each irq for each channel
 * Expecting as many irq as property <dma_channels>
 */
#define DMA_AMEBA_IRQ_CONNECT(n)                                                                   \
	static void dma_ameba_config_irq_##n(const struct device *dev)                             \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		LISTIFY(DT_INST_PROP(n, dma_channels),                                             \
				DMA_AMEBA_IRQ_CONNECT_CHANNEL,                                     \
				(;),                                                               \
				n);                                                                \
	}

/*
 * Macro to instantiate the irq handler (order is given by the 'listify')
 * chan: channel of the DMA instance (assuming one irq per channel)
 * dma : dma instance (one GDMA instance on ameba)
 */
#define DMA_AMEBA_DEFINE_IRQ_HANDLER(chan, dma)                                                    \
	static void dma_ameba_irq_##dma##_##chan(const struct device *dev)                         \
	{                                                                                          \
		dma_ameba_isr_handler(dev, chan);                                                  \
	}

#define DMA_AMEBA_INIT(n)                                                                          \
	BUILD_ASSERT(DT_INST_PROP(n, dma_channels) == DT_NUM_IRQS(DT_DRV_INST(n)),                 \
		     "Nb of Channels and IRQ mismatch");                                           \
                                                                                                   \
	LISTIFY(DT_INST_PROP(n, dma_channels),                                                     \
			DMA_AMEBA_DEFINE_IRQ_HANDLER,                                              \
			(;),                                                                       \
			n);                                                                        \
                                                                                                   \
	DMA_AMEBA_IRQ_CONNECT(n);                                                                  \
                                                                                                   \
	static const struct dma_ameba_config dma_config_##n = {                                    \
		.instance_id = n,                                                                  \
		.base = DT_INST_REG_ADDR(n),                                                       \
		.channel_num = DT_INST_PROP(n, dma_channels),                                      \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (void *)DT_INST_CLOCKS_CELL(n, idx),                               \
		.config_irq = dma_ameba_config_irq_##n,                                            \
	};                                                                                         \
	static struct dma_ameba_channel dma_ameba_##n##_channels[DT_INST_PROP(n, dma_channels)];   \
	IF_ENABLED(CONFIG_DMA_AMEBA_LLI,                                                           \
		(static struct GDMA_CH_LLI lli_pool_##n[DT_INST_PROP(n, dma_channels) *            \
						CONFIG_DMA_AMEBA_LLI_MAX_DESC];))                  \
	static struct dma_ameba_data dma_data_##n = {                                              \
		.channel_status = dma_ameba_##n##_channels,                                        \
		IF_ENABLED(CONFIG_DMA_AMEBA_LLI, (.lli_pool = lli_pool_##n,))                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &dma_ameba_init, NULL, &dma_data_##n, &dma_config_##n,            \
			      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY, &dma_ameba_api);

DT_INST_FOREACH_STATUS_OKAY(DMA_AMEBA_INIT);
