/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/clock_control_silabs.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/dma/dma_silabs_ldma.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/mem_blocks.h>

#include <sl_device_peripheral.h>
#include <sl_dma_manager.h>
#include <sl_hal_ldma.h>

#define DT_DRV_COMPAT silabs_ldma

#define DMA_IRQ_PRIORITY 3

LOG_MODULE_REGISTER(silabs_dma, CONFIG_DMA_LOG_LEVEL);

struct dma_silabs_channel {
	enum dma_channel_direction dir;
	uint32_t complete_callback_en;
	atomic_t busy;
	void *user_data;
	dma_callback_t cb;
	sl_hal_ldma_transfer_init_t xfer_config;
	sl_hal_ldma_descriptor_t *desc;
};

struct dma_silabs_config {
	struct sl_dma_handle *handle;
	LDMA_TypeDef *ldma;
	void (*config)(const struct device *dev);
	const struct device *clock_dev;
	const struct silabs_clock_control_cmu_config clock_cfg;
	const struct silabs_clock_control_cmu_config clock_cfg_xbar;
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
		{ 0x0001, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_1 },
		{ 0x0002, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_2 },
		{ 0x0003, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_3 },
		{ 0x0004, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_4 },
		{ 0x0006, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_6 },
		{ 0x0008, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_8 },
		{ 0x0010, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_16 },
		{ 0x0020, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_32 },
		{ 0x0040, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_64 },
		{ 0x0080, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_128 },
		{ 0x0100, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_256 },
		{ 0x0200, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_512 },
		{ 0x0400, SL_HAL_LDMA_CTRL_BLOCK_SIZE_UNIT_1024 }
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
					  struct dma_block_config *block,
					  sl_hal_ldma_descriptor_t *desc, int *offset)
{
	int ret, src_size, xfer_count, loc_offset, mod, rem_bsize;

	loc_offset = *offset;

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

	if (config->channel_direction == MEMORY_TO_MEMORY) {
		desc->xfer.struct_req = 1;
	}

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

	if (loc_offset) {
		rem_bsize = block->block_size - loc_offset * config->source_data_size;
	} else {
		rem_bsize = block->block_size;
	}

	xfer_count = rem_bsize / config->source_data_size;
	mod = rem_bsize % config->source_data_size;

	if (xfer_count > SL_HAL_LDMA_DESCRIPTOR_MAX_XFER_SIZE) {
		desc->xfer.xfer_count = SL_HAL_LDMA_DESCRIPTOR_MAX_XFER_SIZE - 1;
		*offset = loc_offset + SL_HAL_LDMA_DESCRIPTOR_MAX_XFER_SIZE;

	} else {
		if (!mod || xfer_count == SL_HAL_LDMA_DESCRIPTOR_MAX_XFER_SIZE) {
			xfer_count--;
		}

		desc->xfer.xfer_count = xfer_count;
		*offset = 0;
	}

	/* Warning : High LDMA block size (high burst) means a large transfer
	 *           without LDMA controller re-arbitration.
	 */
	ret = dma_silabs_get_blocksize(config->source_burst_length, config->dest_burst_length,
				       config->source_data_size);
	if (ret < 0) {
		return ret;
	}

	desc->xfer.block_size = ret;

	/* if complete_callbacks_enabled, callback is called at then end of each descriptor
	 * in the list (block for zephyr)
	 */
	desc->xfer.done_ifs = config->complete_callback_en;

	if (config->channel_direction == PERIPHERAL_TO_MEMORY ||
	    config->channel_direction == MEMORY_TO_PERIPHERAL) {
		if (block->flow_control_mode) {
			desc->xfer.req_mode = SL_HAL_LDMA_CTRL_REQ_MODE_ALL;
		} else {
			desc->xfer.req_mode = SL_HAL_LDMA_CTRL_REQ_MODE_BLOCK;
		}
	} else {
		desc->xfer.req_mode = SL_HAL_LDMA_CTRL_REQ_MODE_ALL;
	}

	/* In silabs LDMA, increment sign is managed with the transfer configuration
	 * which is common for all descs of the channel. Zephyr DMA API allows
	 * to manage increment sign for each block desc which can't be done with
	 * silabs LDMA. If increment sign is different in 2 block desc, then an
	 * error is returned.
	 */
	if (block->source_addr_adj != DMA_ADDR_ADJ_NO_CHANGE &&
	    block->source_addr_adj != chan_conf->xfer_config.src_inc_sign) {
		return -ENOTSUP;
	}

	if (block->source_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		desc->xfer.src_inc = SL_HAL_LDMA_CTRL_SRC_INC_NONE;
	} else {
		desc->xfer.src_inc = SL_HAL_LDMA_CTRL_SRC_INC_ONE;
	}

	if (block->dest_addr_adj == DMA_ADDR_ADJ_NO_CHANGE) {
		desc->xfer.dst_inc = SL_HAL_LDMA_CTRL_DST_INC_NONE;
	} else {
		desc->xfer.dst_inc = SL_HAL_LDMA_CTRL_DST_INC_ONE;
	}

	desc->xfer.src_addr_mode = SL_HAL_LDMA_CTRL_SRC_ADDR_MODE_ABS;
	desc->xfer.dst_addr_mode = SL_HAL_LDMA_CTRL_DST_ADDR_MODE_ABS;

	if (block->source_address == 0) {
		LOG_WRN("source_buffer address is null.");
	}
	if (block->dest_address == 0) {
		LOG_WRN("dest_buffer address is null.");
	}

	desc->xfer.src_addr = block->source_address + loc_offset * config->source_data_size;
	desc->xfer.dst_addr = block->dest_address + loc_offset * config->dest_data_size;

	return 0;
}

static int dma_silabs_release_descriptor(struct dma_silabs_data *data,
					 sl_hal_ldma_descriptor_t *desc)
{
	sl_hal_ldma_descriptor_t *head_desc, *next_desc;
	int ret;

	head_desc = desc;
	while (desc) {
		next_desc = SL_HAL_LDMA_DESCRIPTOR_LINKABS_LINKADDR_TO_ADDR(desc->xfer.link_addr);
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
	sl_hal_ldma_descriptor_t *desc, *prev_desc;
	int ret, offset;

	/* Descriptors configuration
	 * block refers to user configured block (dma_block_config structure from dma.h)
	 * desc refers to driver configured block (sl_hal_ldma_descriptor_t structure from silabs
	 * hal)
	 */
	prev_desc = NULL;
	offset = 0;
	while (block) {
		ret = sys_mem_blocks_alloc(data->dma_desc_pool, 1, (void **)&desc);
		if (ret) {
			goto err;
		}

		ret = dma_silabs_block_to_descriptor(config, chan_conf, block, desc, &offset);
		if (ret) {
			goto err;
		}

		if (!prev_desc) {
			chan_conf->desc = desc;
		} else {
			prev_desc->xfer.link_addr =
				SL_HAL_LDMA_DESCRIPTOR_LINKABS_ADDR_TO_LINKADDR(desc);
			prev_desc->xfer.link_mode = SL_HAL_LDMA_LINK_MODE_ABS;
			prev_desc->xfer.link = 1;
		}

		prev_desc = desc;
		if (!offset) {
			block = block->next_block;
			if (block == head_block) {
				block = NULL;
				prev_desc->xfer.link_addr =
					SL_HAL_LDMA_DESCRIPTOR_LINKABS_ADDR_TO_LINKADDR(
						chan_conf->desc);
				prev_desc->xfer.link_mode = SL_HAL_LDMA_LINK_MODE_ABS;
				prev_desc->xfer.link = 1;
			}
		}
	}

	return 0;
err:
	/* Free all eventually allocated descriptor */
	dma_silabs_release_descriptor(data, chan_conf->desc);

	return ret;
}

static void dma_silabs_irq_handler(const struct device *dev, uint32_t id)
{
	const struct dma_silabs_config *config = dev->config;
	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan;
	int status;
	uint32_t pending, chnum, error_mask;

	pending = sl_hal_ldma_get_enabled_pending_interrupts(config->ldma);
	error_mask = LDMA_IF_ERROR;

	for (chnum = 0; chnum < data->dma_ctx.dma_channels; chnum++) {
		chan = &data->dma_chan_table[chnum];
		status = DMA_STATUS_COMPLETE;

		if (pending & error_mask) {
			if (chan->cb) {
				chan->cb(dev, chan->user_data, chnum, -EIO);
			}
		} else if (pending & BIT(chnum)) {
			sl_hal_ldma_clear_interrupts(config->ldma, BIT(chnum));

			/* Is it only an interrupt for the end of a descriptor and not a complete
			 * transfer.
			 */
			if (chan->complete_callback_en) {
				status = DMA_STATUS_BLOCK;
			} else {
				atomic_clear(&chan->busy);
			}

			/*
			 * In the case that the transfer is done but we have append a new
			 * descriptor, we need to manually load the next descriptor
			 */
			if (sl_hal_ldma_transfer_is_done(config->ldma, chnum) &&
			    config->ldma->CH[chnum].LINK & _LDMA_CH_LINK_LINK_MASK) {
				sys_clear_bit((mem_addr_t)&config->ldma->CHDONE, chnum);
				config->ldma->LINKLOAD = BIT(chnum);
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
	const struct dma_silabs_config *cfg = dev->config;
	struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan_conf = &data->dma_chan_table[channel];
	sl_hal_ldma_transfer_init_t *xfer_config = &chan_conf->xfer_config;
	int ret;

	if (channel >= data->dma_ctx.dma_channels) {
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

	sl_hal_ldma_stop_transfer(cfg->ldma, channel);

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
		xfer_config->request_sel = SILABS_LDMA_SLOT_TO_REQSEL(config->dma_slot);
		break;
	case PERIPHERAL_TO_PERIPHERAL:
	case HOST_TO_MEMORY:
	case MEMORY_TO_HOST:
	default:
		return -ENOTSUP;
	}

	/* Directly transform channel_priority into efr priority */
	if (config->channel_priority < SL_HAL_LDMA_CFG_ARBSLOTS_ONE ||
	    config->channel_priority > SL_HAL_LDMA_CFG_ARBSLOTS_EIGHT) {
		return -EINVAL;
	}
	xfer_config->arb_slots = config->channel_priority;

	switch (config->head_block->source_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		xfer_config->src_inc_sign = SL_HAL_LDMA_CFG_SRC_INC_SIGN_POS;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		xfer_config->src_inc_sign = SL_HAL_LDMA_CFG_SRC_INC_SIGN_NEG;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		xfer_config->src_inc_sign = SL_HAL_LDMA_CFG_SRC_INC_SIGN_POS;
		break;
	default:
		LOG_ERR("Addr Adjustment error %d", config->head_block->source_addr_adj);
		break;
	}

	switch (config->head_block->dest_addr_adj) {
	case DMA_ADDR_ADJ_INCREMENT:
		xfer_config->dst_inc_sign = SL_HAL_LDMA_CFG_DST_INC_SIGN_POS;
		break;
	case DMA_ADDR_ADJ_DECREMENT:
		xfer_config->dst_inc_sign = SL_HAL_LDMA_CFG_DST_INC_SIGN_NEG;
		break;
	case DMA_ADDR_ADJ_NO_CHANGE:
		xfer_config->dst_inc_sign = SL_HAL_LDMA_CFG_DST_INC_SIGN_POS;
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
	const struct dma_silabs_config *config = dev->config;
	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan = &data->dma_chan_table[channel];

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	atomic_inc(&chan->busy);

	sl_hal_ldma_init_transfer(config->ldma, channel, &chan->xfer_config, chan->desc);
	sl_hal_ldma_clear_interrupts(config->ldma, BIT(channel));
	sl_hal_ldma_enable_interrupts(config->ldma, BIT(channel));
	sl_hal_ldma_start_transfer(config->ldma, channel);

	return 0;
}

static int dma_silabs_stop(const struct device *dev, uint32_t channel)
{
	const struct dma_silabs_config *config = dev->config;
	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan = &data->dma_chan_table[channel];

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	sl_hal_ldma_stop_transfer(config->ldma, channel);

	atomic_clear(&chan->busy);

	sl_hal_ldma_clear_interrupts(config->ldma, BIT(channel));

	return 0;
}

static int dma_silabs_get_status(const struct device *dev, uint32_t channel,
				 struct dma_status *status)
{
	const struct dma_silabs_config *config = dev->config;
	const struct dma_silabs_data *data = dev->data;

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!atomic_test_bit(data->dma_ctx.atomic, channel)) {
		return -EINVAL;
	}

	status->pending_length = sl_hal_ldma_transfer_remaining_count(config->ldma, channel);
	status->busy = data->dma_chan_table[channel].busy;
	status->dir = data->dma_chan_table[channel].dir;

	return 0;
}

bool dma_silabs_chan_filter(const struct device *dev, int channel, void *filter_param)
{
	const struct dma_silabs_config *config = dev->config;
	sl_status_t status;

	ARG_UNUSED(filter_param);

	status = sl_dma_manager_reserve_channel(config->handle, (uint8_t)channel);

	return (status == SL_STATUS_OK);
}

void dma_silabs_chan_release(const struct device *dev, uint32_t channel)
{
	const struct dma_silabs_config *config = dev->config;
	sl_status_t status;

	status = sl_dma_manager_free_channel(config->handle, (uint8_t)channel);

	__ASSERT_NO_MSG(status == SL_STATUS_OK);
}

static int dma_silabs_init(const struct device *dev)
{
	const struct dma_silabs_config *config = dev->config;
	int err;

	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	err = clock_control_on(config->clock_dev, (clock_control_subsys_t)&config->clock_cfg_xbar);
	if (err < 0 && err != -EALREADY) {
		return err;
	}

	config->config(dev);

	return 0;
}

static DEVICE_API(dma, dma_funcs) = {
	.config = dma_silabs_configure,
	.start = dma_silabs_start,
	.stop = dma_silabs_stop,
	.get_status = dma_silabs_get_status,
	.chan_filter = dma_silabs_chan_filter,
	.chan_release = dma_silabs_chan_release
};

int silabs_ldma_append_block(const struct device *dev, uint32_t channel, struct dma_config *config)
{
	const struct dma_silabs_config *cfg = dev->config;
	const struct dma_silabs_data *data = dev->data;
	struct dma_silabs_channel *chan_conf = &data->dma_chan_table[channel];
	struct dma_block_config *block_config = config->head_block;
	sl_hal_ldma_descriptor_t *desc = data->dma_chan_table[channel].desc;
	unsigned int key;
	int ret, offset;

	offset = 0;

	__ASSERT(!((uintptr_t)desc & ~_LDMA_CH_LINK_LINKADDR_MASK),
		 "DMA Descriptor is not 32 bits aligned");

	if (channel > data->dma_ctx.dma_channels) {
		return -EINVAL;
	}

	if (!atomic_test_bit(data->dma_ctx.atomic, channel)) {
		return -EINVAL;
	}

	/* DMA Channel already have loaded a descriptor with a linkaddr
	 * so we can't append a new block just after the current transfer.
	 * You can't also append a descriptor list.
	 * This check is here to not use the function in a wrong way
	 */
	if (desc->xfer.link_addr || config->head_block->next_block) {
		return -EINVAL;
	}

	/* A link is already set by a previous call to the function */
	if (sys_test_bit((mem_addr_t)&cfg->ldma->CH[channel].LINK, _LDMA_CH_LINK_LINK_SHIFT)) {
		return -EINVAL;
	}

	ret = dma_silabs_block_to_descriptor(config, chan_conf, block_config, desc, &offset);
	if (ret) {
		return ret;
	} else if (offset) {
		/* If the offset is not 0, it means that the block size is larger than the transfer
		 * capacity of a single hardware LDMA descriptor. It is not supported with the
		 * append function.
		 */
		return -EINVAL;
	}

	key = irq_lock();
	if (sl_hal_ldma_channel_is_enabled(cfg->ldma, channel)) {
		/*
		 * It is voluntary to split this 2 lines in order to separate the write of the link
		 * addr and the write of the link bit. In this way, there is always a linkAddr when
		 * the link bit is set.
		 */
		sys_write32((uintptr_t)desc, (mem_addr_t)&cfg->ldma->CH[channel].LINK);
		sys_set_bit((mem_addr_t)&cfg->ldma->CH[channel].LINK, _LDMA_CH_LINK_LINK_SHIFT);
		irq_unlock(key);

	} else {
		irq_unlock(key);
		sl_hal_ldma_init_transfer(cfg->ldma, channel, &chan_conf->xfer_config, desc);
		sl_hal_ldma_clear_interrupts(cfg->ldma, BIT(channel));
		sl_hal_ldma_enable_interrupts(cfg->ldma, BIT(channel));
		sl_hal_ldma_start_transfer(cfg->ldma, channel);
	}

	return 0;
}

#define SILABS_DMA_IRQ_CONNECT(n, inst)                                                            \
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, n, irq), DT_INST_IRQ_BY_IDX(inst, n, priority),       \
		    dma_silabs_irq_handler, DEVICE_DT_INST_GET(inst), 0);                          \
	irq_enable(DT_INST_IRQ_BY_IDX(inst, n, irq));

#define CONFIGURE_ALL_IRQS(inst, n) LISTIFY(n, SILABS_DMA_IRQ_CONNECT, (), inst)

#define DMA_SILABS_LDMA_INIT(inst)                                                                 \
	static const uint32_t ldma_bus_clock_##inst = DT_INST_CLOCKS_CELL(inst, enable);           \
	static const sl_peripheral_dma_val_t ldma_peripheral_##inst = {                            \
		.base = DT_INST_REG_ADDR(inst),                                                    \
		.clk_branch = DT_INST_CLOCKS_CELL(inst, branch),                                   \
		.bus_clock =                                                                       \
			(DT_INST_CLOCKS_CELL(inst, enable) != 0xFFFFFFFFU ? &ldma_bus_clock_##inst \
									  : NULL),                 \
		.dual_destination_map = 0,                                                         \
		.rule_based_interleaving_map = 0,                                                  \
		.nbr_channel = DT_INST_PROP(inst, dma_channels),                                   \
		.nbr_sync = DT_INST_PROP(inst, dma_channels),                                      \
	};                                                                                         \
	static sl_dma_manager_channel_irq_callback_t                                               \
		ldma_cb_##inst[DT_INST_PROP(inst, dma_channels)];                                  \
	static void *ldma_user_data_##inst[DT_INST_PROP(inst, dma_channels)];                      \
	static struct sl_dma_handle ldma_handle_##inst;                                            \
                                                                                                   \
	static void silabs_dma_configure_##inst(const struct device *dev)                          \
	{                                                                                          \
		const struct dma_silabs_config *config = dev->config;                              \
                                                                                                   \
		sli_dma_manager_init_advanced(config->handle, &ldma_peripheral_##inst,             \
					      ldma_cb_##inst, ldma_user_data_##inst,               \
					      DT_INST_PROP(inst, dma_channels));                   \
                                                                                                   \
		CONFIGURE_ALL_IRQS(inst, DT_NUM_IRQS(DT_DRV_INST(inst)));                          \
	};                                                                                         \
                                                                                                   \
	const struct dma_silabs_config dma_silabs_config_##inst = {                                \
		.handle = &ldma_handle_##inst,                                                     \
		.ldma = (LDMA_TypeDef *)DT_INST_REG_ADDR(inst),                                    \
		.config = silabs_dma_configure_##inst,                                             \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_cfg = SILABS_DT_INST_CLOCK_CFG(inst),                                       \
		.clock_cfg_xbar = SILABS_DT_INST_CLOCK_CFG_BY_NAME(inst, xbar),                    \
	};                                                                                         \
                                                                                                   \
	static ATOMIC_DEFINE(dma_channels_atomic_##inst, DT_INST_PROP(inst, dma_channels));        \
                                                                                                   \
	static struct dma_silabs_channel                                                           \
		dma_silabs_channel_##inst[DT_INST_PROP(inst, dma_channels)];                       \
                                                                                                   \
	SYS_MEM_BLOCKS_DEFINE_STATIC_TYPE(desc_pool_##inst, sl_hal_ldma_descriptor_t,              \
					  CONFIG_DMA_MAX_DESCRIPTOR);                              \
                                                                                                   \
	static struct dma_silabs_data dma_silabs_data_##inst = {                                   \
		.dma_ctx.magic = DMA_MAGIC,                                                        \
		.dma_ctx.dma_channels = DT_INST_PROP(inst, dma_channels),                          \
		.dma_ctx.atomic = dma_channels_atomic_##inst,                                      \
		.dma_chan_table = dma_silabs_channel_##inst,                                       \
		.dma_desc_pool = &desc_pool_##inst                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, dma_silabs_init, NULL, &dma_silabs_data_##inst,                \
			      &dma_silabs_config_##inst, PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,   \
			      &dma_funcs);

DT_INST_FOREACH_STATUS_OKAY(DMA_SILABS_LDMA_INIT);
