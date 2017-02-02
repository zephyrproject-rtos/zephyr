/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __DMA_H_
#define __DMA_H_

#include <errno.h>
#include "clk.h"
#include "qm_dma.h"

/* Timeout definitions */
#define STANDARD_TIMEOUT_MICROSECOND (1000)
#define ONE_MICROSECOND (1)

/* Set specific register bits */
#define UPDATE_REG_BITS(reg, value, offset, mask)                              \
	{                                                                      \
		reg &= ~mask;                                                  \
		reg |= (value << offset);                                      \
	}                                                                      \
	while (0)

/* Mask for all supported channels */
#define CHANNEL_MASK_ALL (BIT(QM_DMA_CHANNEL_NUM) - 1)

/*
 * DMA address increment type.
 */
typedef enum {
	QM_DMA_ADDRESS_INCREMENT = 0x0, /**< Increment address. */
	QM_DMA_ADDRESS_DECREMENT = 0x1, /**< Decrement address. */
	QM_DMA_ADDRESS_NO_CHANGE = 0x2  /**< Don't modify address. */
} qm_dma_address_increment_t;

/*
 * DMA channel private structure.
 */
typedef struct dma_cfg_prv_t {
	/* DMA client context to be passed back with callbacks */
	void *callback_context;

	/* DMA channel transfer callback */
	void (*client_callback)(void *callback_context, uint32_t len,
				int error_code);

	/* Pointer to latest configured LLI (multiblock linked list). */
	qm_dma_linked_list_item_t *lli_tail;

	/*
	 * Number of contiguous blocks per buffer (multiblock mode). This is
	 * needed to calculate the total transfer length which is communicated
	 * to the client with the complete callback. In linked list mode, where
	 * more than one buffer (scatter/gather) are used, this is also used to
	 * count single block transfer callbacks so that we know when to invoke
	 * the client callback corresponding to a whole transfered buffer.
	 */
	uint16_t num_blocks_per_buffer;

	/*
	 * Number of block interrupts pending on the buffer currently being
	 * transfered. Used in multiblock continuous mode as well as multiblock
	 * link list mode when more than one buffer is set up. This counter is
	 * decremented on each block interrupt.
	 */
	uint16_t num_blocks_int_pending;

	/*
	 * In multiblock linked list mode, indicates whether transfer is linear
	 * or circular. This information cannot be extracted from the DMA regs.
	 */
	bool transfer_type_ll_circular;
} dma_cfg_prv_t;

/*
 * The length of the transfer at the time that this function is called is
 * returned. The value returned is defined in bytes.
 */
static __inline__ uint32_t
get_transfer_length(const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
		    const dma_cfg_prv_t *prv_cfg)
{
	uint32_t source_transfer_width;
	uint32_t transfer_length;
	uint32_t ctrl_low;
	uint32_t ctrl_high;

	QM_ASSERT(prv_cfg != NULL);
	if (NULL == prv_cfg->lli_tail) {
		/* Single block or contiguous multiblock. */
		volatile qm_dma_chan_reg_t *chan_reg =
		    &QM_DMA[dma]->chan_reg[channel_id];
		ctrl_low = chan_reg->ctrl_low;
		ctrl_high = chan_reg->ctrl_high;
	} else {
		/* Linked list multiblock. */
		ctrl_low = prv_cfg->lli_tail->ctrl_low;
		ctrl_high = prv_cfg->lli_tail->ctrl_high;
	}

	/* Read the source transfer width register value. */
	source_transfer_width = ((ctrl_low & QM_DMA_CTL_L_SRC_TR_WIDTH_MASK) >>
				 QM_DMA_CTL_L_SRC_TR_WIDTH_OFFSET);

	/* Read the length from the block_ts field. The units of this field
	 * are dependent on the source transfer width. */
	transfer_length = ((ctrl_high & QM_DMA_CTL_H_BLOCK_TS_MASK) >>
			   QM_DMA_CTL_H_BLOCK_TS_OFFSET) *
			  prv_cfg->num_blocks_per_buffer;

	/* To convert this to bytes the transfer length can be shifted using
	 * the source transfer width value. This value correspond to the
	 * shifts required and so this can be done as an optimization. */
	return (transfer_length << source_transfer_width);
}

static __inline__ int dma_controller_disable(const qm_dma_t dma)
{
	volatile qm_dma_misc_reg_t *misc_reg = &QM_DMA[dma]->misc_reg;

	misc_reg->cfg_low = 0;
	if (misc_reg->cfg_low) {
		return -EIO;
	}

	return 0;
}

static __inline__ void dma_controller_enable(const qm_dma_t dma)
{
	QM_DMA[dma]->misc_reg.cfg_low = QM_DMA_MISC_CFG_DMA_EN;
}

static int dma_channel_disable(const qm_dma_t dma,
			       const qm_dma_channel_id_t channel_id)
{
	uint8_t channel_mask = BIT(channel_id);
	uint16_t timeout_us;
	volatile qm_dma_misc_reg_t *misc_reg = &QM_DMA[dma]->misc_reg;
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	/* If the channel is already disabled return */
	if (!(misc_reg->chan_en_low & channel_mask)) {
		return 0;
	}

	/* Suspend the channel */
	chan_reg->cfg_low |= QM_DMA_CFG_L_CH_SUSP_MASK;

	/* Ensure that the channel has been suspended */
	timeout_us = STANDARD_TIMEOUT_MICROSECOND;
	while ((!(chan_reg->cfg_low & QM_DMA_CFG_L_CH_SUSP_MASK)) &&
	       timeout_us) {
		clk_sys_udelay(ONE_MICROSECOND);
		timeout_us--;
	}

	if (!(chan_reg->cfg_low & QM_DMA_CFG_L_CH_SUSP_MASK)) {
		return -EIO;
	}

	/* Wait until the fifo is empty */
	timeout_us = STANDARD_TIMEOUT_MICROSECOND;
	while ((!(chan_reg->cfg_low & QM_DMA_CFG_L_FIFO_EMPTY_MASK)) &&
	       timeout_us) {
		clk_sys_udelay(ONE_MICROSECOND);
		timeout_us--;
	}

	/* Disable the channel and wait to confirm that it has been disabled. */
	misc_reg->chan_en_low = (channel_mask << QM_DMA_MISC_CHAN_EN_WE_OFFSET);

	timeout_us = STANDARD_TIMEOUT_MICROSECOND;
	while ((misc_reg->chan_en_low & channel_mask) && timeout_us) {
		clk_sys_udelay(ONE_MICROSECOND);
		timeout_us--;
	}

	if (misc_reg->chan_en_low & channel_mask) {
		return -EIO;
	}

	/* Set the channel to resume */
	chan_reg->cfg_low &= ~QM_DMA_CFG_L_CH_SUSP_MASK;

	return 0;
}

static __inline__ void dma_channel_enable(const qm_dma_t dma,
					  const qm_dma_channel_id_t channel_id)
{
	uint8_t channel_mask = BIT(channel_id);

	QM_DMA[dma]->misc_reg.chan_en_low =
	    (channel_mask << QM_DMA_MISC_CHAN_EN_WE_OFFSET) | channel_mask;
}

static __inline__ void
dma_interrupt_disable(const qm_dma_t dma, const qm_dma_channel_id_t channel_id)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	chan_reg->ctrl_low &= ~QM_DMA_CTL_L_INT_EN_MASK;
}

static __inline__ void
dma_interrupt_enable(const qm_dma_t dma, const qm_dma_channel_id_t channel_id)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	chan_reg->ctrl_low |= QM_DMA_CTL_L_INT_EN_MASK;
}

static __inline__ int
dma_set_transfer_type(const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
		      const qm_dma_transfer_type_t transfer_type,
		      const qm_dma_channel_direction_t channel_direction)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	/*
	 * Valid for single block and contiguous multiblock, will be later
	 * updated if using linked list multiblock.
	 */
	chan_reg->llp_low = 0x0;

	/* Currently only single block is supported */
	switch (transfer_type) {
	case QM_DMA_TYPE_SINGLE:
		chan_reg->ctrl_low &= ~QM_DMA_CTL_L_LLP_SRC_EN_MASK;
		chan_reg->ctrl_low &= ~QM_DMA_CTL_L_LLP_DST_EN_MASK;
		chan_reg->cfg_low &= ~QM_DMA_CFG_L_RELOAD_SRC_MASK;
		chan_reg->cfg_low &= ~QM_DMA_CFG_L_RELOAD_DST_MASK;
		break;

	case QM_DMA_TYPE_MULTI_CONT:
		if (QM_DMA_MEMORY_TO_MEMORY == channel_direction) {
			/*
			 * The DMA core cannot handle memory to memory
			 * multiblock contiguous transactions.
			 */
			return -EINVAL;
		} else if (QM_DMA_PERIPHERAL_TO_MEMORY == channel_direction) {
			/* Reload source. */
			chan_reg->cfg_low |= QM_DMA_CFG_L_RELOAD_SRC_MASK;
			chan_reg->cfg_low &= ~QM_DMA_CFG_L_RELOAD_DST_MASK;
		} else {
			/* Reload destination. */
			chan_reg->cfg_low |= QM_DMA_CFG_L_RELOAD_DST_MASK;
			chan_reg->cfg_low &= ~QM_DMA_CFG_L_RELOAD_SRC_MASK;
		}

		/* Disable block chaining. */
		chan_reg->ctrl_low &= ~QM_DMA_CTL_L_LLP_SRC_EN_MASK;
		chan_reg->ctrl_low &= ~QM_DMA_CTL_L_LLP_DST_EN_MASK;
		break;

	case QM_DMA_TYPE_MULTI_LL:
	case QM_DMA_TYPE_MULTI_LL_CIRCULAR:
		/* Destination status update disable. */
		chan_reg->cfg_high &= ~QM_DMA_CFG_H_DS_UPD_EN_MASK;

		/* Source status update disable. */
		chan_reg->cfg_high &= ~QM_DMA_CFG_H_SS_UPD_EN_MASK;

		/* Enable linked lists for source. */
		chan_reg->ctrl_low |= QM_DMA_CTL_L_LLP_SRC_EN_MASK;
		chan_reg->cfg_low &= ~QM_DMA_CFG_L_RELOAD_SRC_MASK;

		/* Enable linked lists for destination. */
		chan_reg->ctrl_low |= QM_DMA_CTL_L_LLP_DST_EN_MASK;
		chan_reg->cfg_low &= ~QM_DMA_CFG_L_RELOAD_DST_MASK;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static __inline__ qm_dma_transfer_type_t
dma_get_transfer_type(const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
		      const dma_cfg_prv_t *prv_cfg)
{
	qm_dma_transfer_type_t transfer_type;
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	if (0 == (chan_reg->ctrl_low & (QM_DMA_CTL_L_LLP_SRC_EN_MASK |
					QM_DMA_CTL_L_LLP_DST_EN_MASK))) {
		/* Block chaining disabled */
		if (0 == (chan_reg->cfg_low & (QM_DMA_CFG_L_RELOAD_SRC_MASK |
					       QM_DMA_CFG_L_RELOAD_DST_MASK))) {
			/* Single block transfer */
			transfer_type = QM_DMA_TYPE_SINGLE;
		} else {
			/* Contiguous multiblock */
			transfer_type = QM_DMA_TYPE_MULTI_CONT;
		}
	} else {
		/* LLP enabled, linked list multiblock */
		transfer_type = (prv_cfg->transfer_type_ll_circular)
				    ? QM_DMA_TYPE_MULTI_LL_CIRCULAR
				    : QM_DMA_TYPE_MULTI_LL;
	}

	return transfer_type;
}

static __inline__ void
dma_set_source_transfer_width(const qm_dma_t dma,
			      const qm_dma_channel_id_t channel_id,
			      const qm_dma_transfer_width_t transfer_width)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->ctrl_low, transfer_width,
			QM_DMA_CTL_L_SRC_TR_WIDTH_OFFSET,
			QM_DMA_CTL_L_SRC_TR_WIDTH_MASK);
}

static __inline__ void
dma_set_destination_transfer_width(const qm_dma_t dma,
				   const qm_dma_channel_id_t channel_id,
				   const qm_dma_transfer_width_t transfer_width)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->ctrl_low, transfer_width,
			QM_DMA_CTL_L_DST_TR_WIDTH_OFFSET,
			QM_DMA_CTL_L_DST_TR_WIDTH_MASK);
}

static __inline__ void
dma_set_source_burst_length(const qm_dma_t dma,
			    const qm_dma_channel_id_t channel_id,
			    const qm_dma_burst_length_t burst_length)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->ctrl_low, burst_length,
			QM_DMA_CTL_L_SRC_MSIZE_OFFSET,
			QM_DMA_CTL_L_SRC_MSIZE_MASK);
}

static __inline__ void
dma_set_destination_burst_length(const qm_dma_t dma,
				 const qm_dma_channel_id_t channel_id,
				 const qm_dma_burst_length_t burst_length)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->ctrl_low, burst_length,
			QM_DMA_CTL_L_DEST_MSIZE_OFFSET,
			QM_DMA_CTL_L_DEST_MSIZE_MASK);
}

static __inline__ void
dma_set_transfer_direction(const qm_dma_t dma,
			   const qm_dma_channel_id_t channel_id,
			   const qm_dma_channel_direction_t transfer_direction)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->ctrl_low, transfer_direction,
			QM_DMA_CTL_L_TT_FC_OFFSET, QM_DMA_CTL_L_TT_FC_MASK);
}

static __inline__ void
dma_set_source_increment(const qm_dma_t dma,
			 const qm_dma_channel_id_t channel_id,
			 const qm_dma_address_increment_t address_increment)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->ctrl_low, address_increment,
			QM_DMA_CTL_L_SINC_OFFSET, QM_DMA_CTL_L_SINC_MASK);
}

static __inline__ void dma_set_destination_increment(
    const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
    const qm_dma_address_increment_t address_increment)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->ctrl_low, address_increment,
			QM_DMA_CTL_L_DINC_OFFSET, QM_DMA_CTL_L_DINC_MASK);
}

static __inline__ void dma_set_handshake_interface(
    const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
    const qm_dma_handshake_interface_t handshake_interface)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->cfg_high, handshake_interface,
			QM_DMA_CFG_H_SRC_PER_OFFSET, QM_DMA_CFG_H_SRC_PER_MASK);

	UPDATE_REG_BITS(chan_reg->cfg_high, handshake_interface,
			QM_DMA_CFG_H_DEST_PER_OFFSET,
			QM_DMA_CFG_H_DEST_PER_MASK);
}

static __inline__ void
dma_set_handshake_type(const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
		       const uint8_t handshake_type)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->cfg_low, handshake_type,
			QM_DMA_CFG_L_HS_SEL_SRC_OFFSET,
			QM_DMA_CFG_L_HS_SEL_SRC_MASK);

	UPDATE_REG_BITS(chan_reg->cfg_low, handshake_type,
			QM_DMA_CFG_L_HS_SEL_DST_OFFSET,
			QM_DMA_CFG_L_HS_SEL_DST_MASK);
}

static __inline__ void
dma_set_handshake_polarity(const qm_dma_t dma,
			   const qm_dma_channel_id_t channel_id,
			   const qm_dma_handshake_polarity_t handshake_polarity)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->cfg_low, handshake_polarity,
			QM_DMA_CFG_L_SRC_HS_POL_OFFSET,
			QM_DMA_CFG_L_SRC_HS_POL_MASK);

	UPDATE_REG_BITS(chan_reg->cfg_low, handshake_polarity,
			QM_DMA_CFG_L_DST_HS_POL_OFFSET,
			QM_DMA_CFG_L_DST_HS_POL_MASK);
}

static __inline__ void
dma_set_source_address(const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
		       const uint32_t source_address)
{
	QM_DMA[dma]->chan_reg[channel_id].sar_low = source_address;
}

static __inline__ void
dma_set_destination_address(const qm_dma_t dma,
			    const qm_dma_channel_id_t channel_id,
			    const uint32_t destination_address)
{
	QM_DMA[dma]->chan_reg[channel_id].dar_low = destination_address;
}

static __inline__ void dma_set_block_size(const qm_dma_t dma,
					  const qm_dma_channel_id_t channel_id,
					  const uint32_t block_size)
{
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];

	UPDATE_REG_BITS(chan_reg->ctrl_high, block_size,
			QM_DMA_CTL_H_BLOCK_TS_OFFSET,
			QM_DMA_CTL_H_BLOCK_TS_MASK);
}

#endif /* __DMA_H_ */
