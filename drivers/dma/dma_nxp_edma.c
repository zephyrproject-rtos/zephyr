/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "dma_nxp_edma.h"

/* TODO list:
 * 1) Support for requesting a specific channel.
 * 2) Support for checking if DMA transfer is pending when attempting config. (?)
 * 3) Support for error interrupt.
 * 4) Support for error if buffer overflow/underrun.
 * 5) Ideally, HALFMAJOR should be set on a per-channel basis not through a
 * config. If not possible, this should be done through a DTS property. Also,
 * maybe do the same for INTMAJOR IRQ.
 */

static void edma_isr(const void *parameter)
{
	const struct edma_config *cfg;
	struct edma_data *data;
	struct edma_channel *chan;
	int ret;
	uint32_t update_size;

	chan = (struct edma_channel *)parameter;
	cfg = chan->dev->config;
	data = chan->dev->data;

	if (!EDMA_ChannelRegRead(data->hal_cfg, chan->id, EDMA_TCD_CH_INT)) {
		/* skip, interrupt was probably triggered by another channel */
		return;
	}

	/* clear interrupt */
	EDMA_ChannelRegUpdate(data->hal_cfg, chan->id,
			      EDMA_TCD_CH_INT, EDMA_TCD_CH_INT_MASK, 0);

	if (chan->cyclic_buffer) {
		update_size = chan->bsize;

		if (IS_ENABLED(CONFIG_DMA_NXP_EDMA_ENABLE_HALFMAJOR_IRQ)) {
			update_size = chan->bsize / 2;
		} else {
			update_size = chan->bsize;
		}

		/* TODO: add support for error handling here */
		ret = EDMA_CHAN_PRODUCE_CONSUME_A(chan, update_size);
		if (ret < 0) {
			LOG_ERR("chan %d buffer overflow/underrun", chan->id);
		}
	}

	/* TODO: are there any sanity checks we have to perform before invoking
	 * the registered callback?
	 */
	if (chan->cb) {
		chan->cb(chan->dev, chan->arg, chan->id, DMA_STATUS_COMPLETE);
	}
}

static struct edma_channel *lookup_channel(const struct device *dev,
					   uint32_t chan_id)
{
	struct edma_data *data;
	const struct edma_config *cfg;
	int i;

	data = dev->data;
	cfg = dev->config;


	/* optimization: if dma-channels property is present then
	 * the channel data associated with the passed channel ID
	 * can be found at index chan_id in the array of channels.
	 */
	if (cfg->contiguous_channels) {
		/* check for index out of bounds */
		if (chan_id >= data->ctx.dma_channels) {
			return NULL;
		}

		return &data->channels[chan_id];
	}

	/* channels are passed through the valid-channels property.
	 * As such, since some channels may be missing we need to
	 * look through the entire channels array for an ID match.
	 */
	for (i = 0; i < data->ctx.dma_channels; i++) {
		if (data->channels[i].id == chan_id) {
			return &data->channels[i];
		}
	}

	return NULL;
}

static int edma_config(const struct device *dev, uint32_t chan_id,
		       struct dma_config *dma_cfg)
{
	struct edma_data *data;
	const struct edma_config *cfg;
	struct edma_channel *chan;
	uint32_t transfer_type;
	int ret;

	data = dev->data;
	cfg = dev->config;

	if (!dma_cfg->head_block) {
		LOG_ERR("head block shouldn't be NULL");
		return -EINVAL;
	}

	/* validate source data size (SSIZE) */
	if (!EDMA_TransferWidthIsValid(data->hal_cfg, dma_cfg->source_data_size)) {
		LOG_ERR("invalid source data size: %d",
			dma_cfg->source_data_size);
		return -EINVAL;
	}

	/* validate destination data size (DSIZE) */
	if (!EDMA_TransferWidthIsValid(data->hal_cfg, dma_cfg->dest_data_size)) {
		LOG_ERR("invalid destination data size: %d",
			dma_cfg->dest_data_size);
		return -EINVAL;
	}

	/* validate configured alignment */
	if (!EDMA_TransferWidthIsValid(data->hal_cfg, CONFIG_DMA_NXP_EDMA_ALIGN)) {
		LOG_ERR("configured alignment %d is invalid",
			CONFIG_DMA_NXP_EDMA_ALIGN);
		return -EINVAL;
	}

	/* Scatter-Gather configurations currently not supported */
	if (dma_cfg->block_count != 1) {
		LOG_ERR("number of blocks %d not supported", dma_cfg->block_count);
		return -ENOTSUP;
	}

	/* source address shouldn't be NULL */
	if (!dma_cfg->head_block->source_address) {
		LOG_ERR("source address cannot be NULL");
		return -EINVAL;
	}

	/* destination address shouldn't be NULL */
	if (!dma_cfg->head_block->dest_address) {
		LOG_ERR("destination address cannot be NULL");
		return -EINVAL;
	}

	/* check source address's (SADDR) alignment with respect to the data size (SSIZE)
	 *
	 * Failing to meet this condition will lead to the assertion of the SAE
	 * bit (see CHn_ES register).
	 *
	 * TODO: this will also restrict scenarios such as the following:
	 *	SADDR is 8B aligned and SSIZE is 16B. I've tested this
	 *	scenario and seems to raise no hardware errors (I'm assuming
	 *	because this doesn't break the 8B boundary of the 64-bit system
	 *	I tested it on). Is there a need to allow such a scenario?
	 */
	if (dma_cfg->head_block->source_address % dma_cfg->source_data_size) {
		LOG_ERR("source address 0x%x alignment doesn't match data size %d",
			dma_cfg->head_block->source_address,
			dma_cfg->source_data_size);
		return -EINVAL;
	}

	/* check destination address's (DADDR) alignment with respect to the data size (DSIZE)
	 * Failing to meet this condition will lead to the assertion of the DAE
	 * bit (see CHn_ES register).
	 */
	if (dma_cfg->head_block->dest_address % dma_cfg->dest_data_size) {
		LOG_ERR("destination address 0x%x alignment doesn't match data size %d",
			dma_cfg->head_block->dest_address,
			dma_cfg->dest_data_size);
		return -EINVAL;
	}

	/* source burst length should match destination burst length.
	 * This is because the burst length is the equivalent of NBYTES which
	 * is used for both the destination and the source.
	 */
	if (dma_cfg->source_burst_length !=
	    dma_cfg->dest_burst_length) {
		LOG_ERR("source burst length %d doesn't match destination burst length %d",
			dma_cfg->source_burst_length,
			dma_cfg->dest_burst_length);
		return -EINVAL;
	}

	/* total number of bytes should be a multiple of NBYTES.
	 *
	 * This is needed because the EDMA engine performs transfers based
	 * on CITER (integer value) and NBYTES, thus it has no knowledge of
	 * the total transfer size. If the total transfer size is not a
	 * multiple of NBYTES then we'll end up with copying a wrong number
	 * of bytes (CITER = TOTAL_SIZE / BITER). This, of course, raises
	 * no error in the hardware but it's still wrong.
	 */
	if (dma_cfg->head_block->block_size % dma_cfg->source_burst_length) {
		LOG_ERR("block size %d should be a multiple of NBYTES %d",
			dma_cfg->head_block->block_size,
			dma_cfg->source_burst_length);
		return -EINVAL;
	}

	/* check if NBYTES is a multiple of MAX(SSIZE, DSIZE).
	 *
	 * This stems from the fact that NBYTES needs to be a multiple
	 * of SSIZE AND DSIZE. If NBYTES is a multiple of MAX(SSIZE, DSIZE)
	 * then it will for sure satisfy the aforementioned condition (since
	 * SSIZE and DSIZE are powers of 2).
	 *
	 * Failing to meet this condition will lead to the assertion of the
	 * NCE bit (see CHn_ES register).
	 */
	if (dma_cfg->source_burst_length %
	    MAX(dma_cfg->source_data_size, dma_cfg->dest_data_size)) {
		LOG_ERR("NBYTES %d should be a multiple of MAX(SSIZE(%d), DSIZE(%d))",
			dma_cfg->source_burst_length,
			dma_cfg->source_data_size,
			dma_cfg->dest_data_size);
		return -EINVAL;
	}

	/* fetch channel data */
	chan = lookup_channel(dev, chan_id);
	if (!chan) {
		LOG_ERR("channel ID %u is not valid", chan_id);
		return -EINVAL;
	}

	/* save the block size for later usage in edma_reload */
	chan->bsize = dma_cfg->head_block->block_size;

	if (dma_cfg->cyclic) {
		chan->cyclic_buffer = true;

		chan->stat.read_position = 0;
		chan->stat.write_position = 0;

		/* ASSUMPTION: for CONSUMER-type channels, the buffer from
		 * which the engine consumes should be full, while in the
		 * case of PRODUCER-type channels it should be empty.
		 */
		switch (dma_cfg->channel_direction) {
		case MEMORY_TO_PERIPHERAL:
			chan->type = CHAN_TYPE_CONSUMER;
			chan->stat.free = 0;
			chan->stat.pending_length = chan->bsize;
			break;
		case PERIPHERAL_TO_MEMORY:
			chan->type = CHAN_TYPE_PRODUCER;
			chan->stat.pending_length = 0;
			chan->stat.free = chan->bsize;
			break;
		default:
			LOG_ERR("unsupported transfer dir %d for cyclic mode",
				dma_cfg->channel_direction);
			return -ENOTSUP;
		}
	} else {
		chan->cyclic_buffer = false;
	}

	/* change channel's state to CONFIGURED */
	ret = channel_change_state(chan, CHAN_STATE_CONFIGURED);
	if (ret < 0) {
		LOG_ERR("failed to change channel %d state to CONFIGURED", chan_id);
		return ret;
	}

	ret = get_transfer_type(dma_cfg->channel_direction, &transfer_type);
	if (ret < 0) {
		return ret;
	}

	chan->cb = dma_cfg->dma_callback;
	chan->arg = dma_cfg->user_data;

	/* warning: this sets SOFF and DOFF to SSIZE and DSIZE which are POSITIVE. */
	ret = EDMA_ConfigureTransfer(data->hal_cfg, chan_id,
				     dma_cfg->head_block->source_address,
				     dma_cfg->head_block->dest_address,
				     dma_cfg->source_data_size,
				     dma_cfg->dest_data_size,
				     dma_cfg->source_burst_length,
				     dma_cfg->head_block->block_size,
				     transfer_type);
	if (ret < 0) {
		LOG_ERR("failed to configure transfer");
		return to_std_error(ret);
	}

	/* TODO: channel MUX should be forced to 0 based on the previous state */
	if (EDMA_HAS_MUX(data->hal_cfg)) {
		ret = EDMA_SetChannelMux(data->hal_cfg, chan_id, dma_cfg->dma_slot);
		if (ret < 0) {
			LOG_ERR("failed to set channel MUX");
			return to_std_error(ret);
		}
	}

	/* set SLAST and DLAST */
	ret = set_slast_dlast(dma_cfg, transfer_type, data, chan_id);
	if (ret < 0) {
		return ret;
	}

	/* allow interrupting the CPU when a major cycle is completed.
	 *
	 * interesting note: only 1 major loop is performed per slave peripheral
	 * DMA request. For instance, if block_size = 768 and burst_size = 192
	 * we're going to get 4 transfers of 192 bytes. Each of these transfers
	 * translates to a DMA request made by the slave peripheral.
	 */
	EDMA_ChannelRegUpdate(data->hal_cfg, chan_id,
			      EDMA_TCD_CSR, EDMA_TCD_CSR_INTMAJOR_MASK, 0);

	if (IS_ENABLED(CONFIG_DMA_NXP_EDMA_ENABLE_HALFMAJOR_IRQ)) {
		/* if enabled through the above configuration, also
		 * allow the CPU to be interrupted when CITER = BITER / 2.
		 */
		EDMA_ChannelRegUpdate(data->hal_cfg, chan_id, EDMA_TCD_CSR,
				      EDMA_TCD_CSR_INTHALF_MASK, 0);
	}

	/* dump register status - for debugging purposes */
	edma_dump_channel_registers(data, chan_id);

	return 0;
}

static int edma_get_status(const struct device *dev, uint32_t chan_id,
			   struct dma_status *stat)
{
	struct edma_data *data;
	struct edma_channel *chan;
	uint32_t citer, biter, done;
	unsigned int key;

	data = dev->data;

	/* fetch channel data */
	chan = lookup_channel(dev, chan_id);
	if (!chan) {
		LOG_ERR("channel ID %u is not valid", chan_id);
		return -EINVAL;
	}

	if (chan->cyclic_buffer) {
		key = irq_lock();

		stat->free = chan->stat.free;
		stat->pending_length = chan->stat.pending_length;

		irq_unlock(key);
	} else {
		/* note: no locking required here. The DMA interrupts
		 * have no effect over CITER and BITER.
		 */
		citer = EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CITER);
		biter = EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_BITER);
		done = EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CH_CSR) &
			EDMA_TCD_CH_CSR_DONE_MASK;
		if (done) {
			stat->free = chan->bsize;
			stat->pending_length = 0;
		} else {
			stat->free = (biter - citer) * (chan->bsize / biter);
			stat->pending_length = chan->bsize - stat->free;
		}
	}

	LOG_DBG("free: %d, pending: %d", stat->free, stat->pending_length);

	return 0;
}

static int edma_suspend(const struct device *dev, uint32_t chan_id)
{
	struct edma_data *data;
	const struct edma_config *cfg;
	struct edma_channel *chan;
	int ret;

	data = dev->data;
	cfg = dev->config;

	/* fetch channel data */
	chan = lookup_channel(dev, chan_id);
	if (!chan) {
		LOG_ERR("channel ID %u is not valid", chan_id);
		return -EINVAL;
	}

	edma_dump_channel_registers(data, chan_id);

	/* change channel's state to SUSPENDED */
	ret = channel_change_state(chan, CHAN_STATE_SUSPENDED);
	if (ret < 0) {
		LOG_ERR("failed to change channel %d state to SUSPENDED", chan_id);
		return ret;
	}

	LOG_DBG("suspending channel %u", chan_id);

	/* disable HW requests */
	EDMA_ChannelRegUpdate(data->hal_cfg, chan_id,
			      EDMA_TCD_CH_CSR, 0, EDMA_TCD_CH_CSR_ERQ_MASK);

	return 0;
}

static int edma_stop(const struct device *dev, uint32_t chan_id)
{
	struct edma_data *data;
	const struct edma_config *cfg;
	struct edma_channel *chan;
	enum channel_state prev_state;
	int ret;

	data = dev->data;
	cfg = dev->config;

	/* fetch channel data */
	chan = lookup_channel(dev, chan_id);
	if (!chan) {
		LOG_ERR("channel ID %u is not valid", chan_id);
		return -EINVAL;
	}

	prev_state = chan->state;

	/* change channel's state to STOPPED */
	ret = channel_change_state(chan, CHAN_STATE_STOPPED);
	if (ret < 0) {
		LOG_ERR("failed to change channel %d state to STOPPED", chan_id);
		return ret;
	}

	LOG_DBG("stopping channel %u", chan_id);

	if (prev_state == CHAN_STATE_SUSPENDED) {
		/* if the channel has been suspended then there's
		 * no point in disabling the HW requests again. Just
		 * jump to the channel release operation.
		 */
		goto out_release_channel;
	}

	/* disable HW requests */
	EDMA_ChannelRegUpdate(data->hal_cfg, chan_id, EDMA_TCD_CH_CSR, 0,
			      EDMA_TCD_CH_CSR_ERQ_MASK);

out_release_channel:

	irq_disable(chan->irq);

	/* clear the channel MUX so that it can used by a different peripheral.
	 *
	 * note: because the channel is released during dma_stop() that means
	 * dma_start() can no longer be immediately called. This is because
	 * one needs to re-configure the channel MUX which can only be done
	 * through dma_config(). As such, if one intends to reuse the current
	 * configuration then please call dma_suspend() instead of dma_stop().
	 */
	if (EDMA_HAS_MUX(data->hal_cfg)) {
		ret = EDMA_SetChannelMux(data->hal_cfg, chan_id, 0);
		if (ret < 0) {
			LOG_ERR("failed to set channel MUX");
			return to_std_error(ret);
		}
	}

	edma_dump_channel_registers(data, chan_id);

	return 0;
}

static int edma_start(const struct device *dev, uint32_t chan_id)
{
	struct edma_data *data;
	const struct edma_config *cfg;
	struct edma_channel *chan;
	int ret;

	data = dev->data;
	cfg = dev->config;

	/* fetch channel data */
	chan = lookup_channel(dev, chan_id);
	if (!chan) {
		LOG_ERR("channel ID %u is not valid", chan_id);
		return -EINVAL;
	}

	/* change channel's state to STARTED */
	ret = channel_change_state(chan, CHAN_STATE_STARTED);
	if (ret < 0) {
		LOG_ERR("failed to change channel %d state to STARTED", chan_id);
		return ret;
	}

	LOG_DBG("starting channel %u", chan_id);

	irq_enable(chan->irq);

	/* enable HW requests */
	EDMA_ChannelRegUpdate(data->hal_cfg, chan_id,
			      EDMA_TCD_CH_CSR, EDMA_TCD_CH_CSR_ERQ_MASK, 0);

	return 0;
}

static int edma_reload(const struct device *dev, uint32_t chan_id, uint32_t src,
		       uint32_t dst, size_t size)
{
	struct edma_data *data;
	struct edma_channel *chan;
	int ret;
	unsigned int key;

	data = dev->data;

	/* fetch channel data */
	chan = lookup_channel(dev, chan_id);
	if (!chan) {
		LOG_ERR("channel ID %u is not valid", chan_id);
		return -EINVAL;
	}

	/* channel needs to be started to allow reloading */
	if (chan->state != CHAN_STATE_STARTED) {
		LOG_ERR("reload is only supported on started channels");
		return -EINVAL;
	}

	if (chan->cyclic_buffer) {
		key = irq_lock();
		ret = EDMA_CHAN_PRODUCE_CONSUME_B(chan, size);
		irq_unlock(key);
		if (ret < 0) {
			LOG_ERR("chan %d buffer overflow/underrun", chan_id);
			return ret;
		}
	}

	return 0;
}

static int edma_get_attribute(const struct device *dev, uint32_t type, uint32_t *val)
{
	switch (type) {
	case DMA_ATTR_BUFFER_SIZE_ALIGNMENT:
	case DMA_ATTR_BUFFER_ADDRESS_ALIGNMENT:
		*val = CONFIG_DMA_NXP_EDMA_ALIGN;
		break;
	case DMA_ATTR_MAX_BLOCK_COUNT:
		/* this is restricted to 1 because SG configurations are not supported */
		*val = 1;
		break;
	default:
		LOG_ERR("invalid attribute type: %d", type);
		return -EINVAL;
	}

	return 0;
}

static bool edma_channel_filter(const struct device *dev, int chan_id, void *param)
{
	int *requested_channel;

	if (!param) {
		return false;
	}

	requested_channel = param;

	if (*requested_channel == chan_id && lookup_channel(dev, chan_id)) {
		return true;
	}

	return false;
}

static const struct dma_driver_api edma_api = {
	.reload = edma_reload,
	.config = edma_config,
	.start = edma_start,
	.stop = edma_stop,
	.suspend = edma_suspend,
	.resume = edma_start,
	.get_status = edma_get_status,
	.get_attribute = edma_get_attribute,
	.chan_filter = edma_channel_filter,
};

static edma_config_t *edma_hal_cfg_get(const struct edma_config *cfg)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(s_edmaConfigs); i++) {
		if (cfg->regmap_phys == s_edmaConfigs[i].regmap) {
			return s_edmaConfigs + i;
		}
	}

	return NULL;
}

static int edma_init(const struct device *dev)
{
	const struct edma_config *cfg;
	struct edma_data *data;
	mm_reg_t regmap;

	data = dev->data;
	cfg = dev->config;

	data->hal_cfg = edma_hal_cfg_get(cfg);
	if (!data->hal_cfg) {
		return -ENODEV;
	}

	/* map instance MMIO */
	device_map(&regmap, cfg->regmap_phys, cfg->regmap_size, K_MEM_CACHE_NONE);

	/* overwrite physical address set in the HAL configuration.
	 * We can down-cast the virtual address to a 32-bit address because
	 * we know we're working with 32-bit addresses only.
	 */
	data->hal_cfg->regmap = (uint32_t)POINTER_TO_UINT(regmap);

	cfg->irq_config();

	/* dma_request_channel() uses this variable to keep track of the
	 * available channels. As such, it needs to be initialized with NULL
	 * which signifies that all channels are initially available.
	 */
	data->channel_flags = ATOMIC_INIT(0);
	data->ctx.atomic = &data->channel_flags;
	data->ctx.dma_channels = data->hal_cfg->channels;

	return 0;
}

/* a few comments about the BUILD_ASSERT statements:
 *	1) dma-channels and valid-channels should be mutually exclusive.
 *	This means that you specify the one or the other. There's no real
 *	need to have both of them.
 *	2) Number of channels should match the number of interrupts for
 *	said channels (TODO: what about error interrupts?)
 *	3) The channel-mux property shouldn't be specified unless
 *	the eDMA is MUX-capable (signaled via the EDMA_HAS_CHAN_MUX
 *	configuration).
 */
#define EDMA_INIT(inst)								\
										\
BUILD_ASSERT(!DT_NODE_HAS_PROP(DT_INST(inst, DT_DRV_COMPAT), dma_channels) ||	\
	     !DT_NODE_HAS_PROP(DT_INST(inst, DT_DRV_COMPAT), valid_channels),	\
	     "dma_channels and valid_channels are mutually exclusive");		\
										\
BUILD_ASSERT(DT_INST_PROP_OR(inst, dma_channels, 0) ==				\
	     DT_NUM_IRQS(DT_INST(inst, DT_DRV_COMPAT)) ||			\
	     DT_INST_PROP_LEN_OR(inst, valid_channels, 0) ==			\
	     DT_NUM_IRQS(DT_INST(inst, DT_DRV_COMPAT)),				\
	     "number of interrupts needs to match number of channels");		\
										\
BUILD_ASSERT(DT_PROP_OR(DT_INST(inst, DT_DRV_COMPAT), hal_cfg_index, 0) <	\
	     ARRAY_SIZE(s_edmaConfigs),						\
	     "HAL configuration index out of bounds");				\
										\
static struct edma_channel channels_##inst[] = EDMA_CHANNEL_ARRAY_GET(inst);	\
										\
static void interrupt_config_function_##inst(void)				\
{										\
	EDMA_CONNECT_INTERRUPTS(inst);						\
}										\
										\
static struct edma_config edma_config_##inst = {				\
	.regmap_phys = DT_INST_REG_ADDR(inst),					\
	.regmap_size = DT_INST_REG_SIZE(inst),					\
	.irq_config = interrupt_config_function_##inst,				\
	.contiguous_channels = EDMA_CHANS_ARE_CONTIGUOUS(inst),			\
};										\
										\
static struct edma_data edma_data_##inst = {					\
	.channels = channels_##inst,						\
	.ctx.magic = DMA_MAGIC,							\
};										\
										\
DEVICE_DT_INST_DEFINE(inst, &edma_init, NULL,					\
		      &edma_data_##inst, &edma_config_##inst,			\
		      PRE_KERNEL_1, CONFIG_DMA_INIT_PRIORITY,			\
		      &edma_api);						\

DT_INST_FOREACH_STATUS_OKAY(EDMA_INIT);
