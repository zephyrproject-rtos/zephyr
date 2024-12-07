/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/mem_blocks.h>

#include "em_ldma.h"

#define DT_DRV_COMPAT silabs_ldma

#define DMA_IRQ_PRIORITY 3

LOG_MODULE_REGISTER(silabs_dma, CONFIG_DMA_LOG_LEVEL);

struct dma_silabs_channel {
	uint8_t id;
	enum dma_channel_direction dir;
	uint32_t complete_callback_en;
	volatile bool busy;
	void *user_data;
	dma_callback_t cb;
	LDMA_TransferCfg_t xfer_config;
	LDMA_Descriptor_t *desc;
};

struct dma_silabs_config {
	void (*config_irq)(const struct device *dev);
	const struct device *clock_dev;
};

struct dma_silabs_data {
	struct dma_context dma_ctx;
	struct dma_silabs_channel *dma_chan_table;
	struct sys_mem_blocks *dma_desc_pool;
};

struct table {
	int native;
	int efr;
};

const static struct table ldma_blocksize_map[] = {
	{0x1, ldmaCtrlBlockSizeUnit1},     {0x2, ldmaCtrlBlockSizeUnit2},
	{0x3, ldmaCtrlBlockSizeUnit3},     {0x4, ldmaCtrlBlockSizeUnit4},
	{0x6, ldmaCtrlBlockSizeUnit6},     {0x8, ldmaCtrlBlockSizeUnit8},
	{0x10, ldmaCtrlBlockSizeUnit16},   {0x20, ldmaCtrlBlockSizeUnit32},
	{0x40, ldmaCtrlBlockSizeUnit64},   {0x80, ldmaCtrlBlockSizeUnit128},
	{0x100, ldmaCtrlBlockSizeUnit256}, {0x200, ldmaCtrlBlockSizeUnit512},
	{0x400, ldmaCtrlBlockSizeUnit1024}};

const static struct table ldma_prio_map[] = {{0x0, ldmaCfgArbSlotsAs1},
					     {0x1, ldmaCfgArbSlotsAs2},
					     {0x2, ldmaCfgArbSlotsAs4},
					     {0x3, ldmaCfgArbSlotsAs8}};

static bool dma_silabs_get_priority(uint32_t prio, LDMA_CfgArbSlots_t *efr_prio)
{
	for (int i = 0; i < ARRAY_SIZE(ldma_prio_map); i++) {
		if (ldma_prio_map[i].native == prio) {
			*efr_prio = ldma_prio_map[i].efr;
			return true;
		}
	}
	return false;
}

static int dma_silabs_get_transfer_parameter(uint32_t src_dsize, uint32_t dst_dsize,
					     uint32_t block_size, LDMA_Descriptor_t *desc)
{
	if (src_dsize != dst_dsize) {
		LOG_ERR("Source data size(%u) and destination data size(%u) must be equal",
			src_dsize, dst_dsize);
		return -ENOTSUP;
	}

	switch (src_dsize) {
	case 0x1:
		desc->xfer.size = ldmaCtrlSizeByte;
		desc->xfer.xferCnt = block_size - 1;
		break;
	case 0x2:
		desc->xfer.size = ldmaCtrlSizeHalf;
		desc->xfer.xferCnt = block_size % src_dsize ? block_size / src_dsize
							    : block_size / src_dsize - 1;
		break;
	case 0x4:
		desc->xfer.size = ldmaCtrlSizeWord;
		desc->xfer.xferCnt = block_size % src_dsize ? block_size / src_dsize
							    : block_size / src_dsize - 1;
		break;
	default:
		LOG_ERR("data size(%u) not supported : must be BYTE(1), HALF_WORD(2) or WORD(4)",
			src_dsize);
		return -ENOTSUP;
	}

	if (desc->xfer.xferCnt > LDMA_DESCRIPTOR_MAX_XFER_SIZE) {
		return -ENOTSUP;
	}

	return 0;
}

static int dma_silabs_get_blocksize(uint32_t src_blen, uint32_t dst_blen, uint32_t src_dsize,
				    LDMA_Descriptor_t *desc)
{
	uint32_t arb_unit;

	if (src_blen != dst_blen) {
		LOG_ERR("Source burst length (%u) and destination burst length(%u) must be equal",
			src_blen, dst_blen);
		return -ENOTSUP;
	}

	if (src_blen % src_dsize) {
		LOG_ERR("burst length (%u) and data size (%u) mismatch ", src_blen, dst_blen);
		return -EINVAL;
	}

	arb_unit = src_blen / src_dsize;

	for (int i = 0; i < ARRAY_SIZE(ldma_blocksize_map); i++) {
		if (ldma_blocksize_map[i].native == arb_unit) {
			desc->xfer.blockSize = ldma_blocksize_map[i].efr;
			return 0;
		}
	}
	return -EINVAL;
}

static int dma_silabs_configure_descriptor(struct dma_config *config, struct dma_silabs_data *data,
					   struct dma_silabs_channel *chan_conf)
{
	struct dma_block_config *head_block = config->head_block;
	struct dma_block_config *block = config->head_block;
	LDMA_Descriptor_t *desc = NULL;
	LDMA_Descriptor_t *temp_desc = NULL;
	int blockcount = 0;
	int ret = 0;

	/* Descriptors configuration
	 * block refers to user configured block (dma_block_config structure from dma.h)
	 * desc refers to driver configured block (LDMA_Descriptor_t structure from silabs
	 * hal)
	 */

	ret = sys_mem_blocks_alloc(data->dma_desc_pool, 1, (void **)&desc);
	if (ret) {
		return -EXFULL;
	}

	/* Attach the first description to the chan_conf*/
	chan_conf->desc = desc;

	do {
		blockcount++;
		if (blockcount > config->block_count) {
			return -EINVAL;
		}

		if (block->dest_scatter_count || block->source_gather_count ||
		    block->source_gather_interval || block->dest_scatter_interval ||
		    block->dest_reload_en || block->source_reload_en) {
			return -ENOTSUP;
		}

		if ((block->source_gather_en || block->dest_scatter_en) &&
		    config->block_count == 1) {
			LOG_WRN("DMA scatter_gather enabled but there is only one descriptor "
				"configured in this channel(%u)",
				chan_conf->id);
		}

		desc->xfer.structType = ldmaCtrlStructTypeXfer;
		desc->xfer.structReq = 1;
		ret = dma_silabs_get_transfer_parameter(
			config->source_data_size, config->dest_data_size, block->block_size, desc);
		if (ret) {
			return ret;
		}

		desc->xfer.byteSwap = 0;

		/* Warning : High LDMA blockSize (high burst) mean a large transfer
		 *           without LDMA controller re-arbitration.
		 */
		ret = dma_silabs_get_blocksize(config->source_burst_length,
					       config->dest_burst_length, config->source_data_size,
					       desc);
		if (ret) {
			return ret;
		}

		/* if complete_callbacks_enabled, callback is called at then end of each descriptors
		 * list (block for zephyr)
		 */
		desc->xfer.doneIfs = config->complete_callback_en ? 1 : 0;
		desc->xfer.reqMode = ldmaCtrlReqModeAll;
		desc->xfer.decLoopCnt = 0;
		desc->xfer.ignoreSrec = block->flow_control_mode;

		/* In silabs LDMA, increment sign is managed with the transfer configuration
		 * which is common for all descs of the channel. Zephyr DMA API allows
		 * to manage increment sign for each block desc which can't be done with
		 * silabs LDMA. If increment sign is different in 2 block desc, then an
		 * error is returned.
		 */
		if (block->source_addr_adj != DMA_ADDR_ADJ_NO_CHANGE &&
		    block->source_addr_adj != chan_conf->xfer_config.ldmaCfgSrcIncSign) {
			return -ENOTSUP;
		}

		if (block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			desc->xfer.srcInc = ldmaCtrlSrcIncNone;
		} else {
			desc->xfer.srcInc = ldmaCtrlSrcIncOne;
		}
		if (block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
			desc->xfer.dstInc = ldmaCtrlDstIncNone;
		} else {
			desc->xfer.dstInc = ldmaCtrlDstIncOne;
		}

		desc->xfer.srcAddrMode = ldmaCtrlSrcAddrModeAbs;
		desc->xfer.dstAddrMode = ldmaCtrlDstAddrModeAbs;

		if (config->head_block->source_address == 0) {
			LOG_WRN("source_buffer address is null.");
		}

		if (config->head_block->dest_address == 0) {
			LOG_WRN("dest_buffer address is null.");
		}

		desc->xfer.srcAddr = block->source_address;
		desc->xfer.dstAddr = block->dest_address;

		if (block->next_block) {
			desc->xfer.linkMode = ldmaLinkModeAbs;
			desc->xfer.link = 1;

			ret = sys_mem_blocks_alloc(data->dma_desc_pool, 1, (void **)&temp_desc);
			if (ret) {
				return -EXFULL;
			}

			/* Link to the next descriptor and configure it in the next iteration */
			desc->xfer.linkAddr = LDMA_DESCRIPTOR_LINKABS_ADDR_TO_LINKADDR(temp_desc);
			desc = temp_desc;
			block = block->next_block;
			/* Protection against descriptor loop*/
			if (block == head_block) {
				block = 0;
			}
		} else {
			desc->xfer.linkMode = 0;
			desc->xfer.link = 0;
			desc->xfer.linkAddr = 0;
			block = 0;
		}
	} while (block);

	return 0;
}

static void dma_silabs_irq_handler(const struct device *dev, uint32_t id)
{
	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan;
	int status = DMA_STATUS_COMPLETE;
	uint32_t pending, chnum, chmask;

	/* Get all pending and enabled interrupts. */
	pending = LDMA->IF;
	pending &= LDMA->IEN;

	/* Iterate over all LDMA channels. */
	for (chnum = 0, chmask = 1; chnum < data->dma_ctx.dma_channels; chnum++, chmask <<= 1) {
		chan = &data->dma_chan_table[chnum];

		/* Check for LDMA error. */
		if (pending & LDMA_IF_ERROR) {
			if (chan->cb != NULL) {
				chan->cb(dev, chan->user_data, chnum, -EIO);
			}
		} else if (pending & chmask) {
			/* Clear the interrupt flag. */
			LDMA->IF_CLR = chmask;

			/* Is it only an interrupt for the end of a descriptor and not a complete
			 * transfer 
			 */
			if (!chan->complete_callback_en) {
				status = DMA_STATUS_BLOCK;
			} else {
				chan->busy = false;
			}

			if (chan->cb) {
				chan->cb(dev, chan->user_data, chnum, status);
			}
		}
	}
}

static int dma_silabs_configure(const struct device *dev, uint32_t channel,
				struct dma_config *config)
{
	struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan_conf = &data->dma_chan_table[channel];
	LDMA_TransferCfg_t *xfer_config = &chan_conf->xfer_config;
	LDMA_Descriptor_t *temp_desc, *head_desc;
	int ret = 0;

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!config) {
		return -EINVAL;
	}

	if (chan_conf->busy) {
		LOG_ERR("DMA channel %u is busy.", channel);
		return -EBUSY;
	}

	/* Release previously owned descriptor for this channel*/
	head_desc = chan_conf->desc;
	while (chan_conf->desc) {
		temp_desc =
			LDMA_DESCRIPTOR_LINKABS_LINKADDR_TO_ADDR(chan_conf->desc->xfer.linkAddr);
		ret = sys_mem_blocks_free(data->dma_desc_pool, 1, (void **)&chan_conf->desc);
		if (ret) {
			return -EXFULL;
		}
		chan_conf->desc = temp_desc;
		/* Protection against descriptor loop*/
		if (chan_conf->desc == head_desc) {
			break;
		}
	}

	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("source and dest data size differ.");
		return -ENOTSUP;
	}

	if (config->source_handshake || config->dest_handshake || config->source_chaining_en ||
	    config->dest_chaining_en || config->linked_channel) {
		return -ENOTSUP;
	}

	LDMA_StopTransfer(channel);

	chan_conf->user_data = config->user_data;
	chan_conf->cb = config->dma_callback;
	chan_conf->id = channel;
	chan_conf->dir = config->channel_direction;

	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
		xfer_config->ldmaReqSel = 0;
		break;
	case PERIPHERAL_TO_MEMORY:
	case MEMORY_TO_PERIPHERAL:
		xfer_config->ldmaReqSel = config->dma_slot;
		break;
	case PERIPHERAL_TO_PERIPHERAL:
	case HOST_TO_MEMORY:
	case MEMORY_TO_HOST:
	default:
		return -ENOTSUP;
	}

	xfer_config->ldmaReqDis = false;
	xfer_config->ldmaDbgHalt = false;

	if (!dma_silabs_get_priority(config->channel_priority, &xfer_config->ldmaCfgArbSlots)) {
		return -EINVAL;
	}

	switch (config->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		xfer_config->ldmaCfgDstIncSign = ldmaCfgSrcIncSignPos;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		xfer_config->ldmaCfgDstIncSign = ldmaCfgSrcIncSignNeg;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		xfer_config->ldmaCfgDstIncSign = ldmaCfgSrcIncSignPos;
		break;
	default:
		LOG_ERR("Addr Adjustement error. %d", config->head_block->source_addr_adj);
		break;
	}

	switch (config->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		xfer_config->ldmaCfgSrcIncSign = ldmaCfgDstIncSignPos;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		xfer_config->ldmaCfgSrcIncSign = ldmaCfgDstIncSignNeg;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		xfer_config->ldmaCfgSrcIncSign = ldmaCfgDstIncSignPos;
		break;
	default:
		break;
	}

	xfer_config->ldmaLoopCnt = 0;

	ret = dma_silabs_configure_descriptor(config, data, chan_conf);
	if (ret) {
		return ret;
	}

	atomic_set_bit(data->dma_ctx.atomic, channel);

	return ret;
}

static int dma_silabs_start(const struct device *dev, uint32_t channel)
{

	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan_conf = &data->dma_chan_table[channel];

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	chan_conf->busy = true;

	LDMA_StartTransfer(channel, &chan_conf->xfer_config, chan_conf->desc);

	return 0;
}

static int dma_silabs_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan = &data->dma_chan_table[channel];

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	LDMA_StopTransfer(channel);

	chan->busy = false;

	/* Clear the interrupt flag. */
	LDMA->IF_CLR = BIT(channel);

	return 0;
}

static int dma_silabs_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *status)
{
	const struct dma_silabs_data *data = dev->data;

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!atomic_test_bit(data->dma_ctx.atomic, channel)) {
		return -EINVAL;
	}

	status->busy = data->dma_chan_table[channel].busy;
	status->dir = data->dma_chan_table[channel].dir;

	return 0;
}

static int dma_silabs_init(const struct device *dev)
{
	struct dma_silabs_config *config = (struct dma_silabs_config *)dev->config;
	struct dma_silabs_data *data = (struct dma_silabs_data *)dev->data;
	struct dma_silabs_channel *dma_channel;
	LDMA_Init_t dmaInit = LDMA_INIT_DEFAULT;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	for (int i = 0; i < data->dma_ctx.dma_channels; i++) {
		dma_channel = &data->dma_chan_table[i];
		dma_channel->id = i;
		dma_channel->busy = false;
		dma_channel->dir = MEMORY_TO_MEMORY;
		dma_channel->complete_callback_en = 1;
		dma_channel->cb = NULL;
		dma_channel->user_data = NULL;
		memset(&dma_channel->xfer_config, 0, sizeof(dma_channel->xfer_config));
	}

	/*  0x7 indicate that the 8 channels have round robin priority. */
	dmaInit.ldmaInitCtrlNumFixed = 0x7;
	dmaInit.ldmaInitIrqPriority = DMA_IRQ_PRIORITY;

	LDMA_Init(&dmaInit);

	return 0;
}

static const struct dma_driver_api dma_funcs = {.config = dma_silabs_configure,
						.start = dma_silabs_start,
						.stop = dma_silabs_stop,
						.get_status = dma_silabs_get_status};

#define SILABS_DMA_IRQ_CONNECT(n, inst)                                                            \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    dma_silabs_irq_handler, DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, SILABS_DMA_IRQ_CONNECT, (), inst)

#define DMA_SILABS_INIT(inst)                                                                      \
                                                                                                   \
	static void silabs_dma_irq_configure_##inst(const struct device *dev)                      \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		CONFIGURE_ALL_IRQS(inst, DT_NUM_IRQS(DT_DRV_INST(inst)));                          \
	};                                                                                         \
                                                                                                   \
	const struct dma_silabs_config dma_silabs_config_##inst = {                                \
		.config_irq = silabs_dma_irq_configure_##inst,                                     \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
	};                                                                                         \
                                                                                                   \
	static ATOMIC_DEFINE(dma_channels_atomic_##inst, DT_INST_PROP(inst, dma_channels));        \
                                                                                                   \
	static struct dma_silabs_channel                                                           \
		dma_silabs_channel_##inst[DT_INST_PROP(inst, dma_channels)];                       \
                                                                                                   \
	SYS_MEM_BLOCKS_DEFINE_STATIC(desc_pool_##inst, sizeof(LDMA_Descriptor_t),                  \
				     CONFIG_DMA_MAX_DESCRIPTOR, 4);                                \
                                                                                                   \
	static struct dma_silabs_data dma_silabs_data_##inst = {                                   \
		.dma_ctx.magic = DMA_MAGIC,                                                        \
		.dma_ctx.dma_channels = DT_INST_PROP(inst, dma_channels),                          \
		.dma_ctx.atomic = dma_channels_atomic_##inst,                                      \
		.dma_chan_table = dma_silabs_channel_##inst,                                       \
		.dma_desc_pool = &desc_pool_##inst};                                               \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &dma_silabs_init, NULL, &dma_silabs_data_##inst,               \
			      &dma_silabs_config_##inst, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,   \
			      &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMA_SILABS_INIT);
