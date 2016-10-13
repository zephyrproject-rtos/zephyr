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

#include "dma.h"

#ifndef UNIT_TEST
qm_dma_reg_t *qm_dma[QM_DMA_NUM] = {(qm_dma_reg_t *)QM_DMA_BASE};
#endif

/* DMA driver private data structures */
dma_cfg_prv_t dma_channel_config[QM_DMA_NUM][QM_DMA_CHANNEL_NUM] = {{{0}}};

/*
 * Transfer interrupt handler.
 * - Single block: TFR triggers an user callback invocation
 * - Multiblock (contiguous): TFR triggers an user callback invocation, block
 *   interrupts are silent
 * - Multiblock (linked list): Last block interrupt on each buffer triggers an
 *   user callback invocation, TFR is silent
 */
static void qm_dma_isr_handler(const qm_dma_t dma,
			       const qm_dma_channel_id_t channel_id)
{
	dma_cfg_prv_t *prv_cfg = &dma_channel_config[dma][channel_id];
	volatile qm_dma_int_reg_t *int_reg = &QM_DMA[dma]->int_reg;
	volatile qm_dma_chan_reg_t *chan_reg =
	    &QM_DMA[dma]->chan_reg[channel_id];
	uint32_t transfer_length =
	    get_transfer_length(dma, channel_id, prv_cfg);

	QM_ASSERT(int_reg->status_int_low &
		  (QM_DMA_INT_STATUS_TFR | QM_DMA_INT_STATUS_BLOCK));

	if (0 != prv_cfg->num_blocks_per_buffer) {
		/* Multiblock transfer. */
		transfer_length *= prv_cfg->num_blocks_per_buffer;
	}

	if (int_reg->status_int_low & QM_DMA_INT_STATUS_TFR) {
		QM_ASSERT(int_reg->status_tfr_low & BIT(channel_id));
		/* Transfer completed, clear interrupt */
		int_reg->clear_tfr_low = BIT(channel_id);

		/* Mask interrupts for this channel */
		int_reg->mask_tfr_low = BIT(channel_id) << 8;
		int_reg->mask_err_low = BIT(channel_id) << 8;

		/*
		 * Call the callback if registered and pass the transfer length.
		 */
		if (prv_cfg->client_callback && NULL == prv_cfg->lli_tail) {
			/* Single block or contiguous multiblock. */
			prv_cfg->client_callback(prv_cfg->callback_context,
						 transfer_length, 0);
		}
	} else {
		/* Block interrupts are only unmasked in multiblock mode. */
		QM_ASSERT(int_reg->status_block_low & BIT(channel_id));

		/* Block completed, clear interrupt. */
		int_reg->clear_block_low = BIT(channel_id);

		prv_cfg->num_blocks_remaining--;

		if (NULL != prv_cfg->lli_tail &&
		    0 == prv_cfg->num_blocks_remaining) {
			/*
			 * Linked list mode, invoke callback if this is last
			 * block of buffer.
			 */
			if (prv_cfg->client_callback) {
				prv_cfg->client_callback(
				    prv_cfg->callback_context, transfer_length,
				    0);
			}

			/* Buffer done, set for next buffer. */
			prv_cfg->num_blocks_remaining =
			    prv_cfg->num_blocks_per_buffer;

		} else if (NULL == prv_cfg->lli_tail) {
			QM_ASSERT(prv_cfg->num_blocks_remaining <
				  prv_cfg->num_blocks_per_buffer);
			if (1 == prv_cfg->num_blocks_remaining) {
				/*
				 * Contiguous mode. We have just processed the
				 * next to last block, clear CFG.RELOAD so
				 * that the next block is the last one to be
				 * transfered.
				 */
				chan_reg->cfg_low &=
				    ~QM_DMA_CFG_L_RELOAD_SRC_MASK;
				chan_reg->cfg_low &=
				    ~QM_DMA_CFG_L_RELOAD_DST_MASK;
			}
		}
	}
}

/*
 * Error interrupt handler.
 */
static void qm_dma_isr_err_handler(const qm_dma_t dma)
{
	uint32_t interrupt_channel_mask;
	dma_cfg_prv_t *chan_cfg;
	qm_dma_channel_id_t channel_id = 0;
	volatile qm_dma_int_reg_t *int_reg = &QM_DMA[dma]->int_reg;

	QM_ASSERT(int_reg->status_int_low & QM_DMA_INT_STATUS_ERR);
	QM_ASSERT(int_reg->status_err_low);

	interrupt_channel_mask = int_reg->status_err_low;
	while (interrupt_channel_mask) {

		/* Find the channel that the interrupt is for */
		if (!(interrupt_channel_mask & 0x1)) {
			interrupt_channel_mask >>= 1;
			channel_id++;
			continue;
		}

		/* Clear the error interrupt for this channel */
		int_reg->clear_err_low = BIT(channel_id);

		/* Mask interrupts for this channel */
		int_reg->mask_tfr_low = BIT(channel_id) << 8;
		int_reg->mask_err_low = BIT(channel_id) << 8;

		/* Call the callback if registered and pass the
		 * transfer error code */
		chan_cfg = &dma_channel_config[dma][channel_id];
		if (chan_cfg->client_callback) {
			chan_cfg->client_callback(chan_cfg->callback_context, 0,
						  -EIO);
		}

		interrupt_channel_mask >>= 1;
		channel_id++;
	}
}

QM_ISR_DECLARE(qm_dma_0_isr_err)
{
	qm_dma_isr_err_handler(QM_DMA_0);
	QM_ISR_EOI(QM_IRQ_DMA_ERR_VECTOR);
}

QM_ISR_DECLARE(qm_dma_0_isr_0)
{
	qm_dma_isr_handler(QM_DMA_0, QM_DMA_CHANNEL_0);
	QM_ISR_EOI(QM_IRQ_DMA_0_VECTOR);
}

QM_ISR_DECLARE(qm_dma_0_isr_1)
{
	qm_dma_isr_handler(QM_DMA_0, QM_DMA_CHANNEL_1);
	QM_ISR_EOI(QM_IRQ_DMA_1_VECTOR);
}

#if (QUARK_SE)
QM_ISR_DECLARE(qm_dma_0_isr_2)
{
	qm_dma_isr_handler(QM_DMA_0, QM_DMA_CHANNEL_2);
	QM_ISR_EOI(QM_IRQ_DMA_2_VECTOR);
}

QM_ISR_DECLARE(qm_dma_0_isr_3)
{
	qm_dma_isr_handler(QM_DMA_0, QM_DMA_CHANNEL_3);
	QM_ISR_EOI(QM_IRQ_DMA_3_VECTOR);
}

QM_ISR_DECLARE(qm_dma_0_isr_4)
{
	qm_dma_isr_handler(QM_DMA_0, QM_DMA_CHANNEL_4);
	QM_ISR_EOI(QM_IRQ_DMA_4_VECTOR);
}

QM_ISR_DECLARE(qm_dma_0_isr_5)
{
	qm_dma_isr_handler(QM_DMA_0, QM_DMA_CHANNEL_5);
	QM_ISR_EOI(QM_IRQ_DMA_5_VECTOR);
}

QM_ISR_DECLARE(qm_dma_0_isr_6)
{
	qm_dma_isr_handler(QM_DMA_0, QM_DMA_CHANNEL_6);
	QM_ISR_EOI(QM_IRQ_DMA_6_VECTOR);
}

QM_ISR_DECLARE(qm_dma_0_isr_7)
{
	qm_dma_isr_handler(QM_DMA_0, QM_DMA_CHANNEL_7);
	QM_ISR_EOI(QM_IRQ_DMA_7_VECTOR);
}
#endif /* QUARK_SE */

int qm_dma_init(const qm_dma_t dma)
{
	QM_CHECK(dma < QM_DMA_NUM, -EINVAL);

	qm_dma_channel_id_t channel_id;
	volatile qm_dma_int_reg_t *int_reg = &QM_DMA[dma]->int_reg;
	int return_code;

	/* Enable the DMA Clock */
	QM_SCSS_CCU->ccu_mlayer_ahb_ctl |= QM_CCU_DMA_CLK_EN;

	/* Disable the controller */
	return_code = dma_controller_disable(dma);
	if (return_code) {
		return return_code;
	}

	/* Disable the channels and interrupts */
	for (channel_id = 0; channel_id < QM_DMA_CHANNEL_NUM; channel_id++) {
		return_code = dma_channel_disable(dma, channel_id);
		if (return_code) {
			return return_code;
		}
		dma_interrupt_disable(dma, channel_id);
	}

	/* Mask all interrupts */
	int_reg->mask_tfr_low = CHANNEL_MASK_ALL << 8;
	int_reg->mask_block_low = CHANNEL_MASK_ALL << 8;
	int_reg->mask_src_trans_low = CHANNEL_MASK_ALL << 8;
	int_reg->mask_dst_trans_low = CHANNEL_MASK_ALL << 8;
	int_reg->mask_err_low = CHANNEL_MASK_ALL << 8;

	/* Clear all interrupts */
	int_reg->clear_tfr_low = CHANNEL_MASK_ALL;
	int_reg->clear_block_low = CHANNEL_MASK_ALL;
	int_reg->clear_src_trans_low = CHANNEL_MASK_ALL;
	int_reg->clear_dst_trans_low = CHANNEL_MASK_ALL;
	int_reg->clear_err_low = CHANNEL_MASK_ALL;

	/* Enable the controller */
	dma_controller_enable(dma);

	return 0;
}

int qm_dma_channel_set_config(const qm_dma_t dma,
			      const qm_dma_channel_id_t channel_id,
			      qm_dma_channel_config_t *const channel_config)
{
	QM_CHECK(dma < QM_DMA_NUM, -EINVAL);
	QM_CHECK(channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);
	QM_CHECK(channel_config != NULL, -EINVAL);

	dma_cfg_prv_t *chan_cfg = &dma_channel_config[dma][channel_id];
	int return_code;

	/* Set the transfer type. Only one currently supported */
	return_code = dma_set_transfer_type(dma, channel_id,
					    channel_config->transfer_type,
					    channel_config->channel_direction);
	if (return_code) {
		return return_code;
	}

	/* Set the source and destination transfer width. */
	dma_set_source_transfer_width(dma, channel_id,
				      channel_config->source_transfer_width);
	dma_set_destination_transfer_width(
	    dma, channel_id, channel_config->destination_transfer_width);

	/* Set the source and destination burst transfer length. */
	dma_set_source_burst_length(dma, channel_id,
				    channel_config->source_burst_length);
	dma_set_destination_burst_length(
	    dma, channel_id, channel_config->destination_burst_length);

	/* Set channel direction */
	dma_set_transfer_direction(dma, channel_id,
				   channel_config->channel_direction);

	/* Set the increment type depending on direction */
	switch (channel_config->channel_direction) {
	case QM_DMA_PERIPHERAL_TO_MEMORY:
		dma_set_source_increment(dma, channel_id,
					 QM_DMA_ADDRESS_NO_CHANGE);
		dma_set_destination_increment(dma, channel_id,
					      QM_DMA_ADDRESS_INCREMENT);
		break;
	case QM_DMA_MEMORY_TO_PERIPHERAL:
		dma_set_source_increment(dma, channel_id,
					 QM_DMA_ADDRESS_INCREMENT);
		dma_set_destination_increment(dma, channel_id,
					      QM_DMA_ADDRESS_NO_CHANGE);
		break;
	case QM_DMA_MEMORY_TO_MEMORY:
		dma_set_source_increment(dma, channel_id,
					 QM_DMA_ADDRESS_INCREMENT);
		dma_set_destination_increment(dma, channel_id,
					      QM_DMA_ADDRESS_INCREMENT);
		break;
	}

	if (channel_config->channel_direction != QM_DMA_MEMORY_TO_MEMORY) {
		/* Set the handshake interface. */
		dma_set_handshake_interface(
		    dma, channel_id, channel_config->handshake_interface);

		/* Set the handshake type. This is hardcoded to hardware */
		dma_set_handshake_type(dma, channel_id, 0);

		/* Set the handshake polarity. */
		dma_set_handshake_polarity(dma, channel_id,
					   channel_config->handshake_polarity);
	}

	/* Save the client ID */
	chan_cfg->callback_context = channel_config->callback_context;

	/* Save the callback provided by DMA client */
	chan_cfg->client_callback = channel_config->client_callback;

	/* Multiblock linked list not configured. */
	chan_cfg->lli_tail = NULL;

	/* Multiblock number of blocks per buffer (LL and contiguous modes). */
	chan_cfg->num_blocks_per_buffer = 0;

	return 0;
}

int qm_dma_transfer_set_config(const qm_dma_t dma,
			       const qm_dma_channel_id_t channel_id,
			       qm_dma_transfer_t *const transfer_config)
{
	QM_CHECK(dma < QM_DMA_NUM, -EINVAL);
	QM_CHECK(channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);
	QM_CHECK(transfer_config != NULL, -EINVAL);
	QM_CHECK(transfer_config->source_address != NULL, -EINVAL);
	QM_CHECK(transfer_config->destination_address != NULL, -EINVAL);
	QM_CHECK(transfer_config->block_size >= QM_DMA_CTL_H_BLOCK_TS_MIN,
		 -EINVAL);
	QM_CHECK(transfer_config->block_size <= QM_DMA_CTL_H_BLOCK_TS_MAX,
		 -EINVAL);

	/* Set the source and destination addresses. */
	dma_set_source_address(dma, channel_id,
			       (uint32_t)transfer_config->source_address);
	dma_set_destination_address(
	    dma, channel_id, (uint32_t)transfer_config->destination_address);

	/* Set the block size for the transfer. */
	dma_set_block_size(dma, channel_id, transfer_config->block_size);

	return 0;
}

/* Populate a linked list. */
static qm_dma_linked_list_item_t *
dma_linked_list_init(const qm_dma_multi_transfer_t *multi_transfer,
		     uint32_t ctrl_low, uint32_t tail_pointing_lli)
{
	uint32_t source_address = (uint32_t)multi_transfer->source_address;
	uint32_t destination_address =
	    (uint32_t)multi_transfer->destination_address;
	/*
	 * Extracted source/destination increment type, used to calculate
	 * address increment between consecutive blocks.
	 */
	qm_dma_address_increment_t source_address_inc_type =
	    (ctrl_low & QM_DMA_CTL_L_SINC_MASK) >> QM_DMA_CTL_L_SINC_OFFSET;
	qm_dma_address_increment_t destination_address_inc_type =
	    (ctrl_low & QM_DMA_CTL_L_DINC_MASK) >> QM_DMA_CTL_L_DINC_OFFSET;
	QM_ASSERT(source_address_inc_type == QM_DMA_ADDRESS_INCREMENT ||
		  source_address_inc_type == QM_DMA_ADDRESS_NO_CHANGE);
	QM_ASSERT(destination_address_inc_type == QM_DMA_ADDRESS_INCREMENT ||
		  destination_address_inc_type == QM_DMA_ADDRESS_NO_CHANGE);
	/* Source/destination address increment between consecutive blocks. */
	uint32_t source_inc =
	    (source_address_inc_type == QM_DMA_ADDRESS_INCREMENT)
		? multi_transfer->block_size
		: 0;
	uint32_t destination_inc =
	    (destination_address_inc_type == QM_DMA_ADDRESS_INCREMENT)
		? multi_transfer->block_size
		: 0;
	/* Linked list node iteration variable. */
	qm_dma_linked_list_item_t *lli = multi_transfer->linked_list_first;
	uint32_t i;

	for (i = 0; i < multi_transfer->num_blocks; i++) {
		lli->source_address = source_address;
		lli->destination_address = destination_address;
		lli->ctrl_low = ctrl_low;
		lli->ctrl_high = multi_transfer->block_size;
		if (i < (uint32_t)(multi_transfer->num_blocks - 1)) {
			lli->linked_list_address =
			    (uint32_t)(qm_dma_linked_list_item_t *)(lli + 1);
			lli++;
			source_address += source_inc;
			destination_address += destination_inc;
		} else {
			/* Last node. */
			lli->linked_list_address = tail_pointing_lli;
		}
	}

	/* Last node of the populated linked list. */
	return lli;
}

int qm_dma_multi_transfer_set_config(
    const qm_dma_t dma, const qm_dma_channel_id_t channel_id,
    qm_dma_multi_transfer_t *const multi_transfer_config)
{
	QM_CHECK(dma < QM_DMA_NUM, -EINVAL);
	QM_CHECK(channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);
	QM_CHECK(multi_transfer_config != NULL, -EINVAL);
	QM_CHECK(multi_transfer_config->source_address != NULL, -EINVAL);
	QM_CHECK(multi_transfer_config->destination_address != NULL, -EINVAL);
	QM_CHECK(multi_transfer_config->block_size >= QM_DMA_CTL_H_BLOCK_TS_MIN,
		 -EINVAL);
	QM_CHECK(multi_transfer_config->block_size <= QM_DMA_CTL_H_BLOCK_TS_MAX,
		 -EINVAL);
	QM_CHECK(multi_transfer_config->num_blocks > 0, -EINVAL);

	dma_cfg_prv_t *prv_cfg = &dma_channel_config[dma][channel_id];
	/*
	 * Node to which last node points to, 0 on linear linked lists or first
	 * node on circular linked lists.
	 */
	uint32_t tail_pointing_lli;

	if (NULL == prv_cfg->lli_tail) {
		/* First call to this function after DMA channel config. */
		volatile qm_dma_chan_reg_t *chan_reg =
		    &QM_DMA[dma]->chan_reg[channel_id];

		if (0 == chan_reg->llp_low) {
			/* Contiguous multiblock transfer. */
			dma_set_source_address(
			    dma, channel_id,
			    (uint32_t)multi_transfer_config->source_address);
			dma_set_destination_address(
			    dma, channel_id, (uint32_t)multi_transfer_config
						 ->destination_address);
			dma_set_block_size(dma, channel_id,
					   multi_transfer_config->block_size);

		} else if (multi_transfer_config->linked_list_first != NULL &&
			   ((uint32_t)multi_transfer_config->linked_list_first &
			    0x3) == 0) {
			/*
			 * Linked list multiblock tranfer (uninitialized). User
			 * allocated linked list memory needs to be 4-byte
			 * alligned.
			 */
			QM_ASSERT((uint32_t)chan_reg->llp_low &
				  LLP_LL_TO_BE_SET_MULTI_LL);
			QM_ASSERT(0 == ((uint32_t)chan_reg->llp_low &
					~(LLP_LL_TO_BE_SET_MULTI_LL |
					  LLP_LL_TO_BE_SET_MULTI_LL_CIRCULAR)));
			/*
			 * With circular operation, the last LLI node should
			 * point to the first one (this node).
			 */
			tail_pointing_lli =
			    (chan_reg->llp_low &
			     LLP_LL_TO_BE_SET_MULTI_LL_CIRCULAR)
				? (uint32_t)
				      multi_transfer_config->linked_list_first
				: 0;

			/* Initialize LLIs using CTL drom DMA register (plus
			 * INT_EN bit). */
			prv_cfg->lli_tail = dma_linked_list_init(
			    multi_transfer_config,
			    chan_reg->ctrl_low | QM_DMA_CTL_L_INT_EN_MASK,
			    tail_pointing_lli);

			/* Point DMA LLP register to this LLI. */
			chan_reg->llp_low =
			    (uint32_t)multi_transfer_config->linked_list_first;
		} else {
			return -EINVAL;
		}

		/*
		 * Initialize block counting internal variables, needed in ISR
		 * to manage client callback invocations.
		 */
		prv_cfg->num_blocks_per_buffer =
		    multi_transfer_config->num_blocks;
		prv_cfg->num_blocks_remaining =
		    multi_transfer_config->num_blocks;

	} else {
		/*
		 * Linked list multiblock transfer (additional appended LLIs).
		 * The number of blocks needs to match the number of blocks on
		 * previous calls to this function (we only allow scatter/gather
		 * buffers of same size).
		 */
		if (prv_cfg->num_blocks_per_buffer !=
		    multi_transfer_config->num_blocks) {
			return -EINVAL;
		}

		/*
		 * Reference to NULL (linear LL) or the first LLI node (circular
		 * LL), extracted from previously configured linked list.
		 */
		tail_pointing_lli = prv_cfg->lli_tail->linked_list_address;

		/* Point last previously configured linked list to this node. */
		prv_cfg->lli_tail->linked_list_address =
		    (uint32_t)multi_transfer_config->linked_list_first;

		/*
		 * Initialize LLI using CTL from last previously configured LLI,
		 * returning a pointer to the new tail node.
		 */
		prv_cfg->lli_tail = dma_linked_list_init(
		    multi_transfer_config, prv_cfg->lli_tail->ctrl_low,
		    tail_pointing_lli);

		QM_ASSERT(prv_cfg->lli_tail->linked_list_address ==
			  tail_pointing_lli);
	}

	return 0;
}

int qm_dma_transfer_start(const qm_dma_t dma,
			  const qm_dma_channel_id_t channel_id)
{
	QM_CHECK(dma < QM_DMA_NUM, -EINVAL);
	QM_CHECK(channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);

	volatile qm_dma_int_reg_t *int_reg = &QM_DMA[dma]->int_reg;
	dma_cfg_prv_t *prv_cfg = &dma_channel_config[dma][channel_id];

	/* Unmask Interrupts */
	int_reg->mask_tfr_low = ((BIT(channel_id) << 8) | BIT(channel_id));
	int_reg->mask_err_low = ((BIT(channel_id) << 8) | BIT(channel_id));

	if (prv_cfg->num_blocks_per_buffer != 0) {
		/* Block interrupts are only unmasked in multiblock mode. */
		int_reg->mask_block_low =
		    ((BIT(channel_id) << 8) | BIT(channel_id));
	}

	/* Enable interrupts and the channel */
	dma_interrupt_enable(dma, channel_id);
	dma_channel_enable(dma, channel_id);

	return 0;
}

int qm_dma_transfer_terminate(const qm_dma_t dma,
			      const qm_dma_channel_id_t channel_id)
{
	QM_CHECK(dma < QM_DMA_NUM, -EINVAL);
	QM_CHECK(channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);

	int return_code;
	volatile qm_dma_int_reg_t *int_reg = &QM_DMA[dma]->int_reg;

	/* Disable interrupts for the channel */
	dma_interrupt_disable(dma, channel_id);

	/* Mask Interrupts */
	int_reg->mask_tfr_low = (BIT(channel_id) << 8);
	int_reg->mask_block_low = (BIT(channel_id) << 8);
	int_reg->mask_err_low = (BIT(channel_id) << 8);

	/* The channel is disabled and the transfer complete callback is
	 * triggered. This callback provides the client with the data length
	 * transferred before the transfer was stopped. */
	return_code = dma_channel_disable(dma, channel_id);
	if (!return_code) {
		dma_cfg_prv_t *prv_cfg = &dma_channel_config[dma][channel_id];
		if (prv_cfg->client_callback) {
			uint32_t transfer_length =
			    get_transfer_length(dma, channel_id, prv_cfg);
			if (0 != prv_cfg->num_blocks_per_buffer) {
				/* Multiblock transfer. */
				transfer_length *=
				    prv_cfg->num_blocks_per_buffer;
			}
			prv_cfg->client_callback(prv_cfg->callback_context,
						 transfer_length, 0);
		}
	}

	return return_code;
}

int qm_dma_transfer_mem_to_mem(const qm_dma_t dma,
			       const qm_dma_channel_id_t channel_id,
			       qm_dma_transfer_t *const transfer_config)
{
	QM_CHECK(dma < QM_DMA_NUM, -EINVAL);
	QM_CHECK(channel_id < QM_DMA_CHANNEL_NUM, -EINVAL);
	QM_CHECK(transfer_config != NULL, -EINVAL);
	QM_CHECK(transfer_config->source_address != NULL, -EINVAL);
	QM_CHECK(transfer_config->destination_address != NULL, -EINVAL);
	QM_CHECK(transfer_config->block_size <= QM_DMA_CTL_H_BLOCK_TS_MAX,
		 -EINVAL);

	int return_code;

	/* Set the transfer configuration and start the transfer */
	return_code =
	    qm_dma_transfer_set_config(dma, channel_id, transfer_config);
	if (!return_code) {
		return_code = qm_dma_transfer_start(dma, channel_id);
	}

	return return_code;
}
