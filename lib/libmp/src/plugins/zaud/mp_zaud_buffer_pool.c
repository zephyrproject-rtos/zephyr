/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include <src/core/mp_caps.h>

#include "mp_zaud.h"
#include "mp_zaud_buffer_pool.h"

LOG_MODULE_REGISTER(mp_zaud_buffer_pool, CONFIG_LIBMP_LOG_LEVEL);

static __nocache uint8_t
	zaud_buffer_pool_buf[CONFIG_ZAUD_BUFFER_POOL_SZ_MAX * CONFIG_ZAUD_BUFFER_POOL_NUM_MAX];

static struct k_heap zaud_buffer_pool_heap;
static bool zaud_buffer_pool_heap_initialized;

#define ZAUD_BUFFER_HEAP_ALLOC(align, size)                                                        \
	k_heap_aligned_alloc(&zaud_buffer_pool_heap, align, size, K_NO_WAIT)
#define ZAUD_BUFFER_FREE(block) k_heap_free(&zaud_buffer_pool_heap, block)

static bool mp_zaud_buffer_pool_config(struct mp_buffer_pool *pool, struct mp_structure *config)
{
	struct mp_zaud_buffer_pool *zaud_pool = MP_ZAUD_BUFFER_POOL(pool);

	int sample_rate = mp_value_get_int(mp_structure_get_value(config, MP_CAPS_SAMPLE_RATE));
	int bit_width = mp_value_get_int(mp_structure_get_value(config, MP_CAPS_BITWIDTH));
	int num_of_channel =
		mp_value_get_int(mp_structure_get_value(config, MP_CAPS_NUM_OF_CHANNEL));
	uint32_t frame_interval =
		mp_value_get_uint(mp_structure_get_value(config, MP_CAPS_FRAME_INTERVAL));
	int buffer_count = mp_value_get_int(mp_structure_get_value(config, MP_CAPS_BUFFER_COUNT));

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

	zaud_pool->buffer = (void *)ZAUD_BUFFER_HEAP_ALLOC(
		pool->config.align, (pool->config.size * pool->config.min_buffers));

	if (zaud_pool->buffer == NULL) {
		LOG_ERR("Unable to allocate mem_slab buffer");
		return false;
	}

	k_mem_slab_init(zaud_pool->mem_slab, zaud_pool->buffer, pool->config.size,
			pool->config.min_buffers);

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		/* Wrap mem_slab buffer to generic libMP buffer */
		pool->buffers[i].pool = pool;
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
		ZAUD_BUFFER_FREE(zaud_pool->buffer);
		zaud_pool->buffer = NULL;
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
	zaud_pool->buffer = NULL;

	mp_buffer_pool_init(pool);

	if (zaud_buffer_pool_heap_initialized != true) {
		k_heap_init(&zaud_buffer_pool_heap, zaud_buffer_pool_buf,
			    sizeof(zaud_buffer_pool_buf));
		zaud_buffer_pool_heap_initialized = true;
	}

	pool->configure = mp_zaud_buffer_pool_config;
	pool->stop = mp_zaud_buffer_pool_stop;
}
