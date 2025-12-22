/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "rmt_private.h"

#include <zephyr/drivers/dma.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(espressif_rmt_encoder_copy, CONFIG_ESPRESSIF_RMT_LOG_LEVEL);

#include <zephyr/drivers/misc/espressif_rmt/rmt_encoder.h>

typedef struct rmt_copy_encoder_t {
	/* !< Encoder base class */
	rmt_encoder_t base;
	/* !< Index of symbol position in the primary stream */
	size_t last_symbol_index;
} rmt_copy_encoder_t;

static int rmt_copy_encoder_reset(rmt_encoder_t *encoder)
{
	rmt_copy_encoder_t *copy_encoder = __containerof(encoder, rmt_copy_encoder_t, base);

	copy_encoder->last_symbol_index = 0;

	return 0;
}

static size_t IRAM_ATTR rmt_encode_copy(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
	const void *primary_data, size_t data_size, rmt_encode_state_t *ret_state)
{
	rmt_copy_encoder_t *copy_encoder = __containerof(encoder, rmt_copy_encoder_t, base);
	rmt_tx_channel_t *tx_chan = __containerof(channel, rmt_tx_channel_t, base);
	rmt_symbol_word_t *symbols = (rmt_symbol_word_t *)primary_data;
	rmt_encode_state_t state = RMT_ENCODING_RESET;
	size_t symbol_index = copy_encoder->last_symbol_index;
	/* how many symbols will be copied by the encoder */
	size_t mem_want = (data_size / 4 - symbol_index);
	/* how many symbols we can save for this round */
	size_t mem_have = tx_chan->mem_end - tx_chan->mem_off;
	/* where to put the encoded symbols? DMA buffer or RMT HW memory */
#if SOC_RMT_SUPPORT_DMA
	rmt_symbol_word_t *mem_to = channel->dma_dev ? channel->dma_mem_base : channel->hw_mem_base;
#else
	rmt_symbol_word_t *mem_to = channel->hw_mem_base;
#endif
	/* how many symbols will be encoded in this round */
	size_t encode_len = MIN(mem_want, mem_have);
	bool encoding_truncated = mem_have < mem_want;
	bool encoding_space_free = mem_have > mem_want;
	size_t len;
#if SOC_RMT_SUPPORT_DMA
	int rc;
#endif

	len = encode_len;
	while (len > 0) {
		mem_to[tx_chan->mem_off++] = symbols[symbol_index++];
		len--;
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

	if (encoding_truncated) {
		/* this encoding has not finished yet, save the truncated position */
		copy_encoder->last_symbol_index = symbol_index;
	} else {
		/* reset internal index if encoding session has finished */
		copy_encoder->last_symbol_index = 0;
		state |= RMT_ENCODING_COMPLETE;
	}

	if (!encoding_space_free) {
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

static int rmt_del_copy_encoder(rmt_encoder_t *encoder)
{
	rmt_copy_encoder_t *copy_encoder;

	copy_encoder = __containerof(encoder, rmt_copy_encoder_t, base);
	free(copy_encoder);

	return 0;
}

int rmt_new_copy_encoder(const rmt_copy_encoder_config_t *config,
	rmt_encoder_handle_t *ret_encoder)
{
	rmt_copy_encoder_t *encoder;

	if (!(config && ret_encoder)) {
		LOG_ERR("Invalid argument");
		return -EINVAL;
	}
	encoder = rmt_alloc_encoder_mem(sizeof(rmt_copy_encoder_t));
	if (!encoder) {
		LOG_ERR("Unable to allocate memory for encoder");
		return -ENOMEM;
	}
	encoder->base.encode = rmt_encode_copy;
	encoder->base.del = rmt_del_copy_encoder;
	encoder->base.reset = rmt_copy_encoder_reset;

	/* return general encoder handle */
	*ret_encoder = &encoder->base;
	LOG_DBG("new copy encoder @%p", encoder);

	return 0;
}
