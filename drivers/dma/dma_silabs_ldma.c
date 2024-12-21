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
	enum dma_channel_direction dir;
	uint32_t complete_callback_en;
	atomic_t busy;
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

static int dma_silabs_get_blocksize(uint32_t src_blen, uint32_t dst_blen, uint32_t src_dsize)
{
	const static struct {
		int native;
		int efr;
	} ldma_blocksize_map[] = {
		{ 0x0001, ldmaCtrlBlockSizeUnit1 },
		{ 0x0002, ldmaCtrlBlockSizeUnit2 },
		{ 0x0003, ldmaCtrlBlockSizeUnit3 },
		{ 0x0004, ldmaCtrlBlockSizeUnit4 },
		{ 0x0006, ldmaCtrlBlockSizeUnit6 },
		{ 0x0008, ldmaCtrlBlockSizeUnit8 },
		{ 0x0010, ldmaCtrlBlockSizeUnit16 },
		{ 0x0020, ldmaCtrlBlockSizeUnit32 },
		{ 0x0040, ldmaCtrlBlockSizeUnit64 },
		{ 0x0080, ldmaCtrlBlockSizeUnit128 },
		{ 0x0100, ldmaCtrlBlockSizeUnit256 },
		{ 0x0200, ldmaCtrlBlockSizeUnit512 },
		{ 0x0400, ldmaCtrlBlockSizeUnit1024 }
	};
	uint32_t arb_unit;

	if (src_blen != dst_blen) {
		LOG_ERR("Source burst length (%u) and destination burst length(%u) must be equal",
			src_blen, dst_blen);
		return -ENOTSUP;
	}

	if (src_blen % src_dsize) {
		LOG_ERR("burst length (%u) and data size (%u) mismatch", src_blen, dst_blen);
		return -EINVAL;
	}

	arb_unit = src_blen / src_dsize;

	for (int i = 0; i < ARRAY_SIZE(ldma_blocksize_map); i++) {
		if (ldma_blocksize_map[i].native == arb_unit) {
			return ldma_blocksize_map[i].efr;
		}
	}
	return -EINVAL;
}

static int dma_silabs_block_to_descriptor(struct dma_config *config,
					  struct dma_silabs_channel *chan_conf,
					  struct dma_block_config *block, LDMA_Descriptor_t *desc)
{
	int ret, src_size, xfer_count;

	if (block->dest_scatter_count || block->source_gather_count ||
	    block->source_gather_interval || block->dest_scatter_interval ||
	    block->dest_reload_en || block->source_reload_en) {
		return -ENOTSUP;
	}

	if ((block->source_gather_en || block->dest_scatter_en) && config->block_count == 1) {
		LOG_WRN("DMA scatter_gather enabled but there is only one descriptor "
			"configured");
	}

	memset(desc, 0, sizeof(*desc));

	desc->xfer.structReq = 1;

	if (config->source_data_size != config->dest_data_size) {
		LOG_ERR("Source data size(%u) and destination data size(%u) must be equal",
			config->source_data_size, config->dest_data_size);
		return -ENOTSUP;
	}

	if (config->source_data_size < 1 || config->source_data_size > 4) {
		return -ENOTSUP;
	}

	src_size = config->source_data_size;
	desc->xfer.size = LOG2(src_size);

	if (block->block_size % config->source_data_size) {
		xfer_count = block->block_size / config->source_data_size;
	} else {
		xfer_count = block->block_size / config->source_data_size - 1;
	}

	if (xfer_count > LDMA_DESCRIPTOR_MAX_XFER_SIZE) {
		return -ENOTSUP;
	}

	desc->xfer.xferCnt = xfer_count;

	/* Warning : High LDMA blockSize (high burst) mean a large transfer
	 *           without LDMA controller re-arbitration.
	 */
	ret = dma_silabs_get_blocksize(config->source_burst_length, config->dest_burst_length,
				       config->source_data_size);
	if (ret < 0) {
		return ret;
	}

	desc->xfer.blockSize = ret;

	/* if complete_callbacks_enabled, callback is called at then end of each descriptor
	 * in the list (block for zephyr)
	 */
	desc->xfer.doneIfs = config->complete_callback_en;
	desc->xfer.reqMode = ldmaCtrlReqModeAll;
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

	if (block->source_address == 0) {
		LOG_WRN("source_buffer address is null.");
	}
	if (block->dest_address == 0) {
		LOG_WRN("dest_buffer address is null.");
	}

	desc->xfer.srcAddr = block->source_address;
	desc->xfer.dstAddr = block->dest_address;

	return 0;
}

static int dma_silabs_release_descriptor(struct dma_silabs_data *data, LDMA_Descriptor_t *desc)
{
	LDMA_Descriptor_t *head_desc, *next_desc;
	int ret;

	head_desc = desc;
	while (desc) {
		next_desc = LDMA_DESCRIPTOR_LINKABS_LINKADDR_TO_ADDR(desc->xfer.linkAddr);
		ret = sys_mem_blocks_free(data->dma_desc_pool, 1, (void **)&desc);
		if (ret) {
			return ret;
		}
		desc = next_desc;
		/* Protection against descriptor loop*/
		if (desc == head_desc) {
			break;
		}
	}

	return 0;
}

static int dma_silabs_configure_descriptor(struct dma_config *config, struct dma_silabs_data *data,
					   struct dma_silabs_channel *chan_conf)
{
	struct dma_block_config *head_block = config->head_block;
	struct dma_block_config *block = config->head_block;
	LDMA_Descriptor_t *desc, *prev_desc;
	int ret;

	/* Descriptors configuration
	 * block refers to user configured block (dma_block_config structure from dma.h)
	 * desc refers to driver configured block (LDMA_Descriptor_t structure from silabs
	 * hal)
	 */
	prev_desc = NULL;
	while (block) {
		ret = sys_mem_blocks_alloc(data->dma_desc_pool, 1, (void **)&desc);
		if (ret) {
			goto err;
		}

		ret = dma_silabs_block_to_descriptor(config, chan_conf, block, desc);
		if (ret) {
			goto err;
		}

		if (!prev_desc) {
			chan_conf->desc = desc;
		} else {
			prev_desc->xfer.linkAddr = LDMA_DESCRIPTOR_LINKABS_ADDR_TO_LINKADDR(desc);
			prev_desc->xfer.linkMode = ldmaLinkModeAbs;
			prev_desc->xfer.link = 1;
		}

		prev_desc = desc;
		block = block->next_block;
		if (block == head_block) {
			block = NULL;
			prev_desc->xfer.linkAddr =
				LDMA_DESCRIPTOR_LINKABS_ADDR_TO_LINKADDR(chan_conf->desc);
			prev_desc->xfer.linkMode = ldmaLinkModeAbs;
			prev_desc->xfer.link = 1;
		}
	}

	return 0;
err:
	/* Free all eventually allocated descriptor */
	(void)dma_silabs_release_descriptor(data, chan_conf->desc);

	return ret;

}

static void dma_silabs_irq_handler(const struct device *dev, uint32_t id)
{
	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan;
	int status;
	uint32_t pending, chnum;

	pending = LDMA_IntGetEnabled();

	for (chnum = 0; chnum < data->dma_ctx.dma_channels; chnum++) {
		chan = &data->dma_chan_table[chnum];
		status = DMA_STATUS_COMPLETE;

		if (pending & LDMA_IF_ERROR) {
			if (chan->cb) {
				chan->cb(dev, chan->user_data, chnum, -EIO);
			}
		} else if (pending & BIT(chnum)) {
			LDMA_IntClear(BIT(chnum));

			/* Is it only an interrupt for the end of a descriptor and not a complete
			 * transfer.
			 */
			if (chan->complete_callback_en) {
				status = DMA_STATUS_BLOCK;
			} else {
				atomic_clear(&chan->busy);
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
	int ret;

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!config) {
		return -EINVAL;
	}

	if (atomic_get(&chan_conf->busy)) {
		LOG_ERR("DMA channel %u is busy", channel);
		return -EBUSY;
	}

	/* Release previously owned descriptor for this channel*/
	ret = dma_silabs_release_descriptor(data, chan_conf->desc);
	if (ret) {
		return ret;
	}

	if (config->dest_data_size != config->source_data_size) {
		LOG_ERR("source and dest data size differ");
		return -ENOTSUP;
	}

	if (config->source_handshake || config->dest_handshake || config->source_chaining_en ||
	    config->dest_chaining_en || config->linked_channel) {
		return -ENOTSUP;
	}

	LDMA_StopTransfer(channel);

	chan_conf->user_data = config->user_data;
	chan_conf->cb = config->dma_callback;
	chan_conf->dir = config->channel_direction;
	chan_conf->complete_callback_en = config->complete_callback_en;

	memset(xfer_config, 0, sizeof(*xfer_config));

	switch (config->channel_direction) {
	case MEMORY_TO_MEMORY:
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

	/* Directly transform channel_priority into efr priority */
	if (config->channel_priority < ldmaCfgArbSlotsAs1 ||
	    config->channel_priority > ldmaCfgArbSlotsAs8) {
		return -EINVAL;
	}
	xfer_config->ldmaCfgArbSlots = config->channel_priority;

	switch (config->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		xfer_config->ldmaCfgSrcIncSign = ldmaCfgSrcIncSignPos;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		xfer_config->ldmaCfgSrcIncSign = ldmaCfgSrcIncSignNeg;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		xfer_config->ldmaCfgSrcIncSign = ldmaCfgSrcIncSignPos;
		break;
	default:
		LOG_ERR("Addr Adjustement error %d", config->head_block->source_addr_adj);
		break;
	}

	switch (config->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		xfer_config->ldmaCfgDstIncSign = ldmaCfgDstIncSignPos;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		xfer_config->ldmaCfgDstIncSign = ldmaCfgDstIncSignNeg;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		xfer_config->ldmaCfgDstIncSign = ldmaCfgDstIncSignPos;
		break;
	default:
		break;
	}

	ret = dma_silabs_configure_descriptor(config, data, chan_conf);
	if (ret) {
		return ret;
	}

	atomic_set_bit(data->dma_ctx.atomic, channel);

	return 0;
}

static int dma_silabs_start(const struct device *dev, uint32_t channel)
{

	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan = &data->dma_chan_table[channel];

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	atomic_inc(&chan->busy);

	LDMA_StartTransfer(channel, &chan->xfer_config, chan->desc);

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

	atomic_clear(&chan->busy);

	LDMA_IntClear(BIT(channel));

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
	const struct dma_silabs_config *config = dev->config;
	LDMA_Init_t dmaInit = {
		/*  0x7 indicate that the 8 channels have round robin priority. */
		.ldmaInitCtrlNumFixed = 0x7,
		.ldmaInitIrqPriority = DMA_IRQ_PRIORITY,
	};

	/* Clock is managed by em_ldma */

	LDMA_Init(&dmaInit);

	/* LDMA_Init configure IRQ but we want IRQ to match with configured one in the dts*/
	config->config_irq(dev);

	return 0;
}

static DEVICE_API(dma, dma_funcs) = {
	.config = dma_silabs_configure,
	.start = dma_silabs_start,
	.stop = dma_silabs_stop,
	.get_status = dma_silabs_get_status
};

#define SILABS_DMA_IRQ_CONNECT(n, inst)                                                            \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    dma_silabs_irq_handler, DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, SILABS_DMA_IRQ_CONNECT, (), inst)

#define DMA_SILABS_LDMA_INIT(inst)                                                                 \
                                                                                                   \
	static void silabs_dma_irq_configure_##inst(const struct device *dev)                      \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		CONFIGURE_ALL_IRQS(inst, DT_NUM_IRQS(DT_DRV_INST(inst)));                          \
	};                                                                                         \
                                                                                                   \
	const struct dma_silabs_config dma_silabs_config_##inst = {                                \
		.config_irq = silabs_dma_irq_configure_##inst                                      \
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
		.dma_desc_pool = &desc_pool_##inst                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, &dma_silabs_init, NULL, &dma_silabs_data_##inst,               \
			      &dma_silabs_config_##inst, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,   \
			      &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMA_SILABS_LDMA_INIT);
