/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DMA_DMA_NXP_EDMA_H_
#define ZEPHYR_DRIVERS_DMA_DMA_NXP_EDMA_H_

#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/logging/log.h>

#include "fsl_edma_soc_rev2.h"

LOG_MODULE_REGISTER(nxp_edma);

/* used for driver binding */
#define DT_DRV_COMPAT nxp_edma

/* workaround the fact that device_map() is not defined for SoCs with no MMU */
#ifndef DEVICE_MMIO_IS_IN_RAM
#define device_map(virt, phys, size, flags) *(virt) = (phys)
#endif /* DEVICE_MMIO_IS_IN_RAM */

/* macros used to parse DTS properties */

/* used in conjunction with LISTIFY which expects F to also take a variable
 * number of arguments. Since IDENTITY doesn't do that we need to use a version
 * of it which also takes a variable number of arguments.
 */
#define IDENTITY_VARGS(V, ...) IDENTITY(V)

/* used to generate an array of indexes for the channels */
#define _EDMA_CHANNEL_INDEX_ARRAY(inst)\
	LISTIFY(DT_INST_PROP_LEN_OR(inst, valid_channels, 0), IDENTITY_VARGS, (,))

/* used to generate an array of indexes for the channels - this is different
 * from _EDMA_CHANNEL_INDEX_ARRAY because the number of channels is passed
 * explicitly through dma-channels so no need to deduce it from the length
 * of the valid-channels property.
 */
#define _EDMA_CHANNEL_INDEX_ARRAY_EXPLICIT(inst)\
	LISTIFY(DT_INST_PROP_OR(inst, dma_channels, 0), IDENTITY_VARGS, (,))

/* used to generate an array of indexes for the interrupt */
#define _EDMA_INT_INDEX_ARRAY(inst)\
	LISTIFY(DT_NUM_IRQS(DT_INST(inst, DT_DRV_COMPAT)), IDENTITY_VARGS, (,))

/* used to register an ISR/arg pair. TODO: should we also use the priority? */
#define _EDMA_INT_CONNECT(idx, inst)				\
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(inst, idx),		\
		    0, edma_isr,				\
		    &channels_##inst[idx], 0)

/* used to declare a struct edma_channel by the non-explicit macro suite */
#define _EDMA_CHANNEL_DECLARE(idx, inst)				\
{									\
	.id = DT_INST_PROP_BY_IDX(inst, valid_channels, idx),		\
	.dev = DEVICE_DT_INST_GET(inst),				\
	.irq = DT_INST_IRQN_BY_IDX(inst, idx),				\
}

/* used to declare a struct edma_channel by the explicit macro suite */
#define _EDMA_CHANNEL_DECLARE_EXPLICIT(idx, inst)			\
{									\
	.id = idx,							\
	.dev = DEVICE_DT_INST_GET(inst),				\
	.irq = DT_INST_IRQN_BY_IDX(inst, idx),				\
}

/* used to create an array of channel IDs via the valid-channels property */
#define _EDMA_CHANNEL_ARRAY(inst)					\
	{ FOR_EACH_FIXED_ARG(_EDMA_CHANNEL_DECLARE, (,),		\
			     inst, _EDMA_CHANNEL_INDEX_ARRAY(inst)) }

/* used to create an array of channel IDs via the dma-channels property */
#define _EDMA_CHANNEL_ARRAY_EXPLICIT(inst)				\
	{ FOR_EACH_FIXED_ARG(_EDMA_CHANNEL_DECLARE_EXPLICIT, (,), inst,	\
			     _EDMA_CHANNEL_INDEX_ARRAY_EXPLICIT(inst)) }

/* used to construct the channel array based on the specified property:
 * dma-channels or valid-channels.
 */
#define EDMA_CHANNEL_ARRAY_GET(inst)							\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_INST(inst, DT_DRV_COMPAT), dma_channels),	\
		    (_EDMA_CHANNEL_ARRAY_EXPLICIT(inst)),				\
		    (_EDMA_CHANNEL_ARRAY(inst)))

#define EDMA_HAL_CFG_GET(inst)								\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_INST(inst, DT_DRV_COMPAT), hal_cfg_index),	\
		    (s_edmaConfigs[DT_INST_PROP(inst, hal_cfg_index)]),			\
		    (s_edmaConfigs[0]))

/* used to register edma_isr for all specified interrupts */
#define EDMA_CONNECT_INTERRUPTS(inst)				\
	FOR_EACH_FIXED_ARG(_EDMA_INT_CONNECT, (;),		\
			   inst, _EDMA_INT_INDEX_ARRAY(inst))

#define EDMA_CHANS_ARE_CONTIGUOUS(inst)\
	DT_NODE_HAS_PROP(DT_INST(inst, DT_DRV_COMPAT), dma_channels)

/* utility macros */

/* a few words about EDMA_CHAN_PRODUCE_CONSUME_{A/B}:
 *	- in the context of cyclic buffers we introduce
 *	the concepts of consumer and producer channels.
 *
 *	- a consumer channel is a channel for which the
 *	DMA copies data from a buffer, thus leading to
 *	less data in said buffer (data is consumed with
 *	each transfer).
 *
 *	- a producer channel is a channel for which the
 *	DMA copies data into a buffer, thus leading to
 *	more data in said buffer (data is produced with
 *	each transfer).
 *
 *	- for consumer channels, each DMA interrupt will
 *	signal that an amount of data has been consumed
 *	from the buffer (half of the buffer size if
 *	HALFMAJOR is enabled, the whole buffer otherwise).
 *
 *	- for producer channels, each DMA interrupt will
 *	signal that an amount of data has been added
 *	to the buffer.
 *
 *	- to signal this, the ISR uses EDMA_CHAN_PRODUCE_CONSUME_A
 *	which will "consume" data from the buffer for
 *	consumer channels and "produce" data for
 *	producer channels.
 *
 *	- since the upper layers using this driver need
 *	to let the EDMA driver know whenever they've produced
 *	(in the case of consumer channels) or consumed
 *	data (in the case of producer channels) they can
 *	do so through the reload() function.
 *
 *	- reload() uses EDMA_CHAN_PRODUCE_CONSUME_B which
 *	for consumer channels will "produce" data and
 *	"consume" data for producer channels, thus letting
 *	the driver know what action the upper layer has
 *	performed (if the channel is a consumer it's only
 *	natural that the upper layer will write/produce more
 *	data to the buffer. The same rationale applies to
 *	producer channels).
 *
 *	- EDMA_CHAN_PRODUCE_CONSUME_B is just the opposite
 *	of EDMA_CHAN_PRODUCE_CONSUME_A. If one produces
 *	data, the other will consume and vice-versa.
 *
 *	- all of this information is valid only in the
 *	context of cyclic buffers. If this behaviour is
 *	not enabled, querying the status will simply
 *	resolve to querying CITER and BITER.
 */
#define EDMA_CHAN_PRODUCE_CONSUME_A(chan, size)\
	((chan)->type == CHAN_TYPE_CONSUMER ?\
	 edma_chan_cyclic_consume(chan, size) :\
	 edma_chan_cyclic_produce(chan, size))

#define EDMA_CHAN_PRODUCE_CONSUME_B(chan, size)\
	((chan)->type == CHAN_TYPE_CONSUMER ?\
	 edma_chan_cyclic_produce(chan, size) :\
	 edma_chan_cyclic_consume(chan, size))

enum channel_type {
	CHAN_TYPE_CONSUMER = 0,
	CHAN_TYPE_PRODUCER,
};

enum channel_state {
	CHAN_STATE_INIT = 0,
	CHAN_STATE_CONFIGURED,
	CHAN_STATE_STARTED,
	CHAN_STATE_STOPPED,
	CHAN_STATE_SUSPENDED,
};

struct edma_channel {
	/* channel ID, needs to be the same as the hardware channel ID */
	uint32_t id;
	/* pointer to device representing the EDMA instance, used by edma_isr */
	const struct device *dev;
	/* current state of the channel */
	enum channel_state state;
	/* type of the channel (PRODUCER/CONSUMER) - only applicable to cyclic
	 * buffer configurations.
	 */
	enum channel_type type;
	/* argument passed to the user-defined DMA callback */
	void *arg;
	/* user-defined callback, called at the end of a channel's interrupt
	 * handling.
	 */
	dma_callback_t cb;
	/* INTID associated with the channel */
	int irq;
	/* the channel's status */
	struct dma_status stat;
	/* cyclic buffer size - currently, this is set to head_block's size */
	uint32_t bsize;
	/* set to true if the channel uses a cyclic buffer configuration */
	bool cyclic_buffer;
};

struct edma_data {
	/* this needs to be the first member */
	struct dma_context ctx;
	mm_reg_t regmap;
	struct edma_channel *channels;
	atomic_t channel_flags;
	edma_config_t *hal_cfg;
};

struct edma_config {
	uint32_t regmap_phys;
	uint32_t regmap_size;
	void (*irq_config)(void);
	/* true if channels are contiguous. The channels may not be contiguous
	 * if the valid-channels property is used instead of dma-channels. This
	 * is used to improve the time complexity of the channel lookup
	 * function.
	 */
	bool contiguous_channels;
};

static inline int channel_change_state(struct edma_channel *chan,
				       enum channel_state next)
{
	enum channel_state prev = chan->state;

	LOG_DBG("attempting to change state from %d to %d for channel %d", prev, next, chan->id);

	/* validate transition */
	switch (prev) {
	case CHAN_STATE_INIT:
		if (next != CHAN_STATE_CONFIGURED) {
			return -EPERM;
		}
		break;
	case CHAN_STATE_CONFIGURED:
		if (next != CHAN_STATE_STARTED &&
		    next != CHAN_STATE_CONFIGURED) {
			return -EPERM;
		}
		break;
	case CHAN_STATE_STARTED:
		if (next != CHAN_STATE_STOPPED &&
		    next != CHAN_STATE_SUSPENDED) {
			return -EPERM;
		}
		break;
	case CHAN_STATE_STOPPED:
		if (next != CHAN_STATE_CONFIGURED) {
			return -EPERM;
		}
		break;
	case CHAN_STATE_SUSPENDED:
		if (next != CHAN_STATE_STARTED &&
		    next != CHAN_STATE_STOPPED) {
			return -EPERM;
		}
		break;
	default:
		LOG_ERR("invalid channel previous state: %d", prev);
		return -EINVAL;
	}

	/* transition OK, proceed */
	chan->state = next;

	return 0;
}

static inline int get_transfer_type(enum dma_channel_direction dir, uint32_t *type)
{
	switch (dir) {
	case MEMORY_TO_MEMORY:
		*type = kEDMA_TransferTypeM2M;
		break;
	case MEMORY_TO_PERIPHERAL:
		*type = kEDMA_TransferTypeM2P;
		break;
	case PERIPHERAL_TO_MEMORY:
		*type = kEDMA_TransferTypeP2M;
		break;
	default:
		LOG_ERR("invalid channel direction: %d", dir);
		return -EINVAL;
	}

	return 0;
}

static inline bool data_size_is_valid(uint16_t size)
{
	switch (size) {
	case 1:
	case 2:
	case 4:
	case 8:
	case 16:
	case 32:
	case 64:
		break;
	default:
		return false;
	}

	return true;
}

/* TODO: we may require setting the channel type through DTS
 * or through struct dma_config. For now, we'll only support
 * MEMORY_TO_PERIPHERAL and PERIPHERAL_TO_MEMORY directions
 * and assume that these are bound to a certain channel type.
 */
static inline int edma_set_channel_type(struct edma_channel *chan,
					enum dma_channel_direction dir)
{
	switch (dir) {
	case MEMORY_TO_PERIPHERAL:
		chan->type = CHAN_TYPE_CONSUMER;
		break;
	case PERIPHERAL_TO_MEMORY:
		chan->type = CHAN_TYPE_PRODUCER;
		break;
	default:
		LOG_ERR("unsupported transfer direction: %d", dir);
		return -ENOTSUP;
	}

	return 0;
}

/* this function is used in cyclic buffer configurations. What it does
 * is it updates the channel's read position based on the number of
 * bytes requested. If the number of bytes that's being read is higher
 * than the number of bytes available in the buffer (pending_length)
 * this will lead to an error. The main point of this check is to
 * provide a way for the user to determine if data is consumed at a
 * higher rate than it is being produced.
 *
 * This function is used in edma_isr() for CONSUMER channels to mark
 * that data has been consumed (i.e: data has been transferred to the
 * destination) (this is done via EDMA_CHAN_PRODUCE_CONSUME_A that's
 * called in edma_isr()). For producer channels, this function is used
 * in edma_reload() to mark the fact that the user of the EDMA driver
 * has consumed data.
 */
static inline int edma_chan_cyclic_consume(struct edma_channel *chan,
					   uint32_t bytes)
{
	if (bytes > chan->stat.pending_length) {
		return -EINVAL;
	}

	chan->stat.read_position =
		(chan->stat.read_position + bytes) % chan->bsize;

	if (chan->stat.read_position > chan->stat.write_position) {
		chan->stat.free = chan->stat.read_position -
			chan->stat.write_position;
	} else if (chan->stat.read_position == chan->stat.write_position) {
		chan->stat.free = chan->bsize;
	} else {
		chan->stat.free = chan->bsize -
			(chan->stat.write_position - chan->stat.read_position);
	}

	chan->stat.pending_length = chan->bsize - chan->stat.free;

	return 0;
}

/* this function is used in cyclic buffer configurations. What it does
 * is it updates the channel's write position based on the number of
 * bytes requested. If the number of bytes that's being written is higher
 * than the number of free bytes in the buffer this will lead to an error.
 * The main point of this check is to provide a way for the user to determine
 * if data is produced at a higher rate than it is being consumed.
 *
 * This function is used in edma_isr() for PRODUCER channels to mark
 * that data has been produced (i.e: data has been transferred to the
 * destination) (this is done via EDMA_CHAN_PRODUCE_CONSUME_A that's
 * called in edma_isr()). For consumer channels, this function is used
 * in edma_reload() to mark the fact that the user of the EDMA driver
 * has produced data.
 */
static inline int edma_chan_cyclic_produce(struct edma_channel *chan,
					   uint32_t bytes)
{
	if (bytes > chan->stat.free) {
		return -EINVAL;
	}

	chan->stat.write_position =
		(chan->stat.write_position + bytes) % chan->bsize;

	if (chan->stat.write_position > chan->stat.read_position) {
		chan->stat.pending_length = chan->stat.write_position -
			chan->stat.read_position;
	} else if (chan->stat.write_position == chan->stat.read_position) {
		chan->stat.pending_length = chan->bsize;
	} else {
		chan->stat.pending_length = chan->bsize -
			(chan->stat.read_position - chan->stat.write_position);
	}

	chan->stat.free = chan->bsize - chan->stat.pending_length;

	return 0;
}

static inline void edma_dump_channel_registers(struct edma_data *data,
					       uint32_t chan_id)
{
	LOG_DBG("dumping channel data for channel %d", chan_id);

	LOG_DBG("CH_CSR: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CH_CSR));
	LOG_DBG("CH_ES: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CH_ES));
	LOG_DBG("CH_INT: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CH_INT));
	LOG_DBG("CH_SBR: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CH_SBR));
	LOG_DBG("CH_PRI: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CH_PRI));

	if (EDMA_HAS_MUX(data->hal_cfg)) {
		LOG_DBG("CH_MUX: 0x%x",
			EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CH_MUX));
	}

	LOG_DBG("TCD_SADDR: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_SADDR));
	LOG_DBG("TCD_SOFF: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_SOFF));
	LOG_DBG("TCD_ATTR: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_ATTR));
	LOG_DBG("TCD_NBYTES: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_NBYTES));
	LOG_DBG("TCD_SLAST_SDA: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_SLAST_SDA));
	LOG_DBG("TCD_DADDR: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_DADDR));
	LOG_DBG("TCD_DOFF: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_DOFF));
	LOG_DBG("TCD_CITER: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CITER));
	LOG_DBG("TCD_DLAST_SGA: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_DLAST_SGA));
	LOG_DBG("TCD_CSR: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_CSR));
	LOG_DBG("TCD_BITER: 0x%x",
		EDMA_ChannelRegRead(data->hal_cfg, chan_id, EDMA_TCD_BITER));
}

static inline int set_slast_dlast(struct dma_config *dma_cfg,
				  uint32_t transfer_type,
				  struct edma_data *data,
				  uint32_t chan_id)
{
	int32_t slast, dlast;

	if (transfer_type == kEDMA_TransferTypeP2M) {
		slast = 0;
	} else {
		switch (dma_cfg->head_block->source_addr_adj) {
		case DMA_ADDR_ADJ_INCREMENT:
			slast = (int32_t)dma_cfg->head_block->block_size;
			break;
		case DMA_ADDR_ADJ_DECREMENT:
			slast = (-1) * (int32_t)dma_cfg->head_block->block_size;
			break;
		default:
			LOG_ERR("unsupported SADDR adjustment: %d",
				dma_cfg->head_block->source_addr_adj);
			return -EINVAL;
		}
	}

	if (transfer_type == kEDMA_TransferTypeM2P) {
		dlast = 0;
	} else {
		switch (dma_cfg->head_block->dest_addr_adj) {
		case DMA_ADDR_ADJ_INCREMENT:
			dlast = (int32_t)dma_cfg->head_block->block_size;
			break;
		case DMA_ADDR_ADJ_DECREMENT:
			dlast = (-1) * (int32_t)dma_cfg->head_block->block_size;
			break;
		default:
			LOG_ERR("unsupported DADDR adjustment: %d",
				dma_cfg->head_block->dest_addr_adj);
			return -EINVAL;
		}
	}

	LOG_DBG("attempting to commit SLAST %d", slast);
	LOG_DBG("attempting to commit DLAST %d", dlast);

	/* commit configuration */
	EDMA_ChannelRegWrite(data->hal_cfg, chan_id, EDMA_TCD_SLAST_SDA, slast);
	EDMA_ChannelRegWrite(data->hal_cfg, chan_id, EDMA_TCD_DLAST_SGA, dlast);

	return 0;
}

/* the NXP HAL EDMA driver uses some custom return values
 * that need to be converted to standard error codes. This function
 * performs exactly this translation.
 */
static inline int to_std_error(int edma_err)
{
	switch (edma_err) {
	case kStatus_EDMA_InvalidConfiguration:
	case kStatus_InvalidArgument:
		return -EINVAL;
	case kStatus_Busy:
		return -EBUSY;
	default:
		LOG_ERR("unknown EDMA error code: %d", edma_err);
		return -EINVAL;
	}
}

#endif /* ZEPHYR_DRIVERS_DMA_DMA_NXP_EDMA_H_ */
