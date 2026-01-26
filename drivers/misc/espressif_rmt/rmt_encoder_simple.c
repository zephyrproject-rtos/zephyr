/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rmt_private.h"

#include <zephyr/drivers/dma.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espressif_rmt_encoder_simple, CONFIG_ESPRESSIF_RMT_LOG_LEVEL);

#include <zephyr/drivers/misc/espressif_rmt/rmt_encoder.h>

typedef struct rmt_simple_encoder_t {
	/* !< Encoder base class */
	rmt_encoder_t base;
	/* !< Index of symbol position in the primary stream */
	size_t last_symbol_index;
	/* !< Callback to call to encode */
	rmt_encode_simple_cb_t callback;
	/* !< Opaque callback argument */
	void *arg;
	/* !< Overflow buffer */
	rmt_symbol_word_t *ovf_buf;
	/* !< Size, in elements, of overflow buffer */
	size_t ovf_buf_size;
	/* !< How much actual info the overflow buffer has */
	size_t ovf_buf_fill_len;
	/* !< Up to where we moved info from the ovf buf to the rmt */
	size_t ovf_buf_parsed_pos;
	/* !< True if we can't call the callback for more data anymore */
	bool callback_done;
} rmt_simple_encoder_t;

static int rmt_simple_encoder_reset(rmt_encoder_t *encoder)
{
	rmt_simple_encoder_t *simple_encoder = __containerof(encoder, rmt_simple_encoder_t, base);

	simple_encoder->last_symbol_index = 0;
	simple_encoder->ovf_buf_fill_len = 0;
	simple_encoder->ovf_buf_parsed_pos = 0;
	simple_encoder->callback_done = false;

	return 0;
}

static size_t rmt_encode_simple(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
	const void *data, size_t data_size, rmt_encode_state_t *ret_state)
{
	rmt_simple_encoder_t *simple_encoder = __containerof(encoder, rmt_simple_encoder_t, base);
	rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);
	rmt_encode_state_t state = RMT_ENCODING_RESET;
	size_t encode_len = 0; /* Total amount of symbols written to RMT memory */
	bool is_done = false;
	/* where to put the encoded symbols? DMA buffer or RMT HW memory */
#if SOC_RMT_SUPPORT_DMA
	rmt_symbol_word_t *mem_to = channel->dma_dev ? channel->dma_mem_base : channel->hw_mem_base;
#else
	rmt_symbol_word_t *mem_to = channel->hw_mem_base;
#endif
#if SOC_RMT_SUPPORT_DMA
	int rc;
#endif

	/*
	 * While we're not done, we need to use the callback to fill the RMT memory until it is
	 * exactly entirely full. We cannot do that if the RMT memory still has N free spaces
	 * but the encoder callback needs more than N spaces to properly encode a symbol.
	 * In order to work around that, if we detect that situation we let the encoder
	 * encode into an overflow buffer, then we use the contents of that buffer to fill
	 * those last N spaces. On the next call, we will first output the rest of the
	 * overflow buffer before again using the callback to continue filling the RMT
	 * buffer.
	 */
	/*
	 * Note the next code is in a while loop to properly handle 'unsure' callbacks that
	 * e.g. return 0 with a free buffer size of M, but then return less than M symbols
	 * when then called with a larger buffer.
	 */
	while (tx_chan->mem_off < tx_chan->mem_end) {
		if (simple_encoder->ovf_buf_parsed_pos < simple_encoder->ovf_buf_fill_len) {
			/*
			 * Overflow buffer has data from the previous encoding call.
			 * Copy one entry from that.
			 */
			mem_to[tx_chan->mem_off++] =
				simple_encoder->ovf_buf[simple_encoder->ovf_buf_parsed_pos++];
			encode_len++;
		} else {
			/* Overflow buffer is empty, so we don't need to empty that first. */
			if (simple_encoder->callback_done) {
				/*
				 * We cannot call the callback anymore and the overflow buffer
				 * is empty, so we're done with the transaction.
				 */
				is_done = true;
				break;
			}
			/* Try to have the callback write the data directly into RMT memory. */
			size_t enc_size = simple_encoder->callback(data, data_size,
				simple_encoder->last_symbol_index,
				tx_chan->mem_end - tx_chan->mem_off, &mem_to[tx_chan->mem_off],
				&is_done, simple_encoder->arg);
			encode_len += enc_size;
			tx_chan->mem_off += enc_size;
			simple_encoder->last_symbol_index += enc_size;
			if (is_done) {
				/* we're done, no more data to write to RMT memory. */
				break;
			}
			if (enc_size == 0) {
				/*
				 * The encoder does not have enough space in RMT memory to encode
				 * its thing, but the RMT memory is not filled out entirely. Encode
				 * into the overflow buffer so the next iterations of the loop can
				 * fill out the RMT buffer from that.
				 */
				enc_size = simple_encoder->callback(data, data_size,
					simple_encoder->last_symbol_index,
					simple_encoder->ovf_buf_size, simple_encoder->ovf_buf,
					&is_done, simple_encoder->arg);
				simple_encoder->last_symbol_index += enc_size;
				/*
				 * Note we do *not* update encode_len here as the data isn't going
				 * to the RMT yet.
				 */
				simple_encoder->ovf_buf_fill_len = enc_size;
				simple_encoder->ovf_buf_parsed_pos = 0;
				if (is_done) {
					/*
					 * If the encoder is done, we cannot call the callback
					 * anymore, but we still need to handle any data in the
					 * overflow buffer.
					 */
					simple_encoder->callback_done = true;
				} else {
					if (enc_size == 0) {
						/*
						 * According to the callback docs, this is illegal.
						 * Report this. EARLY_LOGE as we're running from an
						 * ISR.
						 */
						LOG_ERR("rmt_encoder_simple: encoder callback "
							"returned 0 when fed a buffer of "
							"config::min_chunk_size!");
						/* Then abort the transaction. */
						is_done = true;
						break;
					}
				}
			}
		}
	}

#if SOC_RMT_SUPPORT_DMA
	if (channel->dma_dev) {
		rc = dma_reload(channel->dma_dev, channel->dma_channel,
			(uint32_t)channel->dma_mem_base, 0, channel->dma_mem_size);
		if (rc) {
			LOG_ERR("Reloading DMA channel failed");
			return 0;
		}
	}
#endif

	if (is_done) {
		/* reset internal index if encoding session has finished */
		simple_encoder->last_symbol_index = 0;
		state |= RMT_ENCODING_COMPLETE;
	} else {
		/* no more free memory, the caller should yield */
		state |= RMT_ENCODING_MEM_FULL;
	}

	/* reset offset pointer when exceeds maximum range */
	if (tx_chan->mem_off >= tx_chan->ping_pong_symbols * 2) {
#if SOC_RMT_SUPPORT_DMA
		if (channel->dma_dev) {
			rc = dma_reload(channel->dma_dev, channel->dma_channel,
				(uint32_t)channel->dma_mem_base, 0, channel->dma_mem_size);
			if (rc) {
				LOG_ERR("Reloading DMA channel failed");
				return 0;
			}
		}
#endif
		tx_chan->mem_off = 0;
	}

	*ret_state = state;
	return encode_len;
}

static int rmt_del_simple_encoder(rmt_encoder_t *encoder)
{
	rmt_simple_encoder_t *simple_encoder = __containerof(encoder, rmt_simple_encoder_t, base);

	if (simple_encoder->ovf_buf) {
		free(simple_encoder->ovf_buf);
	}
	free(simple_encoder);

	return 0;
}

int rmt_new_simple_encoder(const rmt_simple_encoder_config_t *config,
	rmt_encoder_handle_t *ret_encoder)
{
	rmt_simple_encoder_t *encoder = NULL;

	if (!(config && ret_encoder)) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	encoder = rmt_alloc_encoder_mem(sizeof(rmt_simple_encoder_t));
	if (!encoder) {
		LOG_ERR("Unable to allocate memory for encoder");
		return -ENOMEM;
	}
	encoder->base.encode = rmt_encode_simple;
	encoder->base.del = rmt_del_simple_encoder;
	encoder->base.reset = rmt_simple_encoder_reset;
	encoder->callback = config->callback;
	encoder->arg = config->arg;

	size_t min_chunk_size = config->min_chunk_size;

	if (min_chunk_size == 0) {
		min_chunk_size = 64;
	}
	encoder->ovf_buf = rmt_alloc_encoder_mem(min_chunk_size * sizeof(rmt_symbol_word_t));
	if (!encoder->ovf_buf) {
		LOG_ERR("Unable to allocate memory for overflow buffer");
		if (encoder) {
			free(encoder);
		}
		return -ENOMEM;
	}
	encoder->ovf_buf_size = min_chunk_size;
	encoder->ovf_buf_fill_len = 0;
	encoder->ovf_buf_parsed_pos = 0;

	/* return general encoder handle */
	*ret_encoder = &encoder->base;
	LOG_DBG("new simple encoder @%p", encoder);

	return 0;
}
