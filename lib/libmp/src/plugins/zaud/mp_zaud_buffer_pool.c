/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "mp_zaud.h"
#include "mp_zaud_buffer_pool.h"

LOG_MODULE_REGISTER(mp_zaud_buffer_pool, CONFIG_LIBMP_LOG_LEVEL);

static bool mp_zaud_buffer_pool_config(struct mp_buffer_pool *pool, struct mp_structure *config)
{
	struct mp_zaud_buffer_pool *zaud_pool = MP_ZAUD_BUFFER_POOL(pool);
	void *aligned_buffer = NULL;

	int sample_rate = mp_value_get_int(mp_structure_get_value(config, "samplerate"));
	int bit_width = mp_value_get_int(mp_structure_get_value(config, "bitwidth"));
	int num_of_channel = mp_value_get_int(mp_structure_get_value(config, "numOfchannel"));
	uint32_t frame_interval =
		mp_value_get_uint(mp_structure_get_value(config, "frameinterval"));
	int buffer_count = mp_value_get_int(mp_structure_get_value(config, "buffercount"));

	/*
	 * TEMPORARY WORKAROUND: Adding 2 extra buffers to the minimum count
	 *
	 * Currently adding +2 buffers beyond the requested buffer_count because
	 * the current buffer management system requires additional buffers
	 *
	 * This is a temporary solution
	 *
	 * TODO: Remove this hardcoded +2 offset when:
	 * - Buffer lifecycle management is properly implemented
	 * - Proper flow control prevents buffer starvation
	 */
	pool->config.min_buffers = buffer_count + 2;
	pool->config.size = (bit_width / BITS_PER_BYTE) * (sample_rate * frame_interval / 1000000) *
			    num_of_channel;
	/* The address needs to be aligned to the size of the DMA transfer */
	pool->config.align = bit_width >> 3;

	/* Allocate just the pool's buffers structure */
	pool->buffers = k_calloc(pool->config.min_buffers, sizeof(struct mp_buffer));
	if (pool->buffers == NULL) {
		LOG_ERR("Unable to allocate pool buffer");
		return false;
	}

	/* TODO: Need to allocate in non-cachable memory */
	zaud_pool->unaligned_buffer = (void *)k_calloc(
		1, (pool->config.size * pool->config.min_buffers) + (pool->config.align - 1));
	if (zaud_pool->unaligned_buffer == NULL) {
		LOG_ERR("Unable to allocate mem_slab buffer");
		return false;
	}

	/* Align buffer to required alignment */
	uintptr_t unaligned_addr = (uintptr_t)zaud_pool->unaligned_buffer;
	uintptr_t aligned_addr =
		(unaligned_addr + pool->config.align - 1) & ~(pool->config.align - 1);
	aligned_buffer = (void *)aligned_addr;

	k_mem_slab_init(zaud_pool->mem_slab, aligned_buffer, pool->config.size,
			pool->config.min_buffers);

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		/* Wrap mem_slab buffer to generic libMP buffer */
		pool->buffers[i].pool = pool;
		pool->buffers[i].size = pool->config.size;
		pool->buffers[i].data = &(zaud_pool->mem_slab->buffer[pool->config.size * i]);
		pool->buffers[i].index = i;
	}

	return true;
}

static bool mp_zaud_buffer_pool_stop(struct mp_buffer_pool *pool)
{
	struct mp_zaud_buffer_pool *zaud_pool = MP_ZAUD_BUFFER_POOL(pool);

	if (pool->buffers != NULL) {
		k_free(pool->buffers);
		pool->buffers = NULL;
	}

	if (zaud_pool->mem_slab->buffer != NULL) {
		k_free(zaud_pool->unaligned_buffer);
		zaud_pool->unaligned_buffer = NULL;
		zaud_pool->mem_slab->buffer = NULL;
	}

	zaud_pool->mem_slab = NULL;

	return true;
}

void mp_zaud_buffer_pool_init(struct mp_buffer_pool *pool)
{
	struct mp_zaud_buffer_pool *zaud_pool = MP_ZAUD_BUFFER_POOL(pool);

	zaud_pool->zaud_dev = NULL;
	zaud_pool->mem_slab = NULL;

	mp_buffer_pool_init(pool);

	pool->configure = mp_zaud_buffer_pool_config;
	pool->stop = mp_zaud_buffer_pool_stop;
}
