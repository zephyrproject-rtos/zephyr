/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_caps.h>
#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zaud/mp_zaud.h>
#include <zephyr/mp/zaud/mp_zaud_buffer_pool.h>

LOG_MODULE_REGISTER(mp_zaud_buffer_pool, CONFIG_MP_LOG_LEVEL);

#define ZAUD_BUFFER_POOL_BASE_ALIGN sizeof(void *)
#define ZAUD_BUFFER_POOL_SIZE                                                                     \
	(CONFIG_MP_ZAUD_BUFFER_POOL_SZ_MAX * CONFIG_MP_ZAUD_BUFFER_POOL_NUM_MAX)

static __nocache __aligned(ZAUD_BUFFER_POOL_BASE_ALIGN)
uint8_t zaud_buffer_pool_buf[ZAUD_BUFFER_POOL_SIZE];

static void mp_zaud_buffer_pool_release_allocations(struct mp_zaud_buffer_pool *zaud_pool,
						    bool clear_mem_slab)
{
	if (zaud_pool->blocks != NULL) {
		k_free(zaud_pool->blocks);
		zaud_pool->blocks = NULL;
	}

	if ((zaud_pool->mem_slab != NULL) && clear_mem_slab) {
		zaud_pool->mem_slab->buffer = NULL;
		zaud_pool->mem_slab = NULL;
	}
}

static int mp_zaud_buffer_pool_config(struct mp_buffer_pool *pool, struct mp_structure *config)
{
	struct mp_zaud_buffer_pool *zaud_pool = (struct mp_zaud_buffer_pool *)pool;
	int align = 0;
	uint32_t required_align = 0;
	uint8_t *base;
	int ret;

	uint32_t sample_rate =
		mp_value_get_uint(mp_structure_get_value(config, MP_CAPS_SAMPLE_RATE));
	uint32_t bit_width = mp_value_get_uint(mp_structure_get_value(config, MP_CAPS_BITWIDTH));
	uint32_t num_of_channel =
		mp_value_get_uint(mp_structure_get_value(config, MP_CAPS_NUM_OF_CHANNEL));
	uint32_t frame_interval =
		mp_value_get_uint(mp_structure_get_value(config, MP_CAPS_FRAME_INTERVAL));
	uint32_t buffer_count =
		mp_value_get_uint(mp_structure_get_value(config, MP_CAPS_BUFFER_COUNT));

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
	required_align = bit_width >> 3;

	/* Decide alignment using LCM */
	align = sys_lcm(ZAUD_BUFFER_POOL_BASE_ALIGN, required_align);

	if (align == -1) {
		LOG_ERR("Incompatible alignment requirements");
		return -EINVAL;
	} else if (align == 0 && required_align != 0) {
		pool->config.align = required_align;
	} else if (align != 0) {
		pool->config.align = align;
	} else {
		/* Both are 0, use base alignment */
		pool->config.align = ZAUD_BUFFER_POOL_BASE_ALIGN;
	}

	if (pool->config.size * pool->config.min_buffers > ZAUD_BUFFER_POOL_SIZE) {
		LOG_ERR("zaud_buffer_pool_buf hos not enough space for requested buffers");
		return -EINVAL;
	}

	if (zaud_pool->mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return -EINVAL;
	}

	mp_zaud_buffer_pool_release_allocations(zaud_pool, false);

	zaud_pool->blocks = k_calloc(pool->config.min_buffers, sizeof(*zaud_pool->blocks));
	if (zaud_pool->blocks == NULL) {
		LOG_ERR("Unable to allocate pool block table");
		return -ENOMEM;
	}

	ret = k_mem_slab_init(zaud_pool->mem_slab, (void *)zaud_buffer_pool_buf, pool->config.size,
			      pool->config.min_buffers);
	if (ret != 0) {
		LOG_ERR("Unable to initialize memory slab (%d)", ret);
		k_free(zaud_pool->blocks);
		zaud_pool->blocks = NULL;
		return ret;
	}

	base = (uint8_t *)zaud_pool->mem_slab->buffer;

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		/* Keep the mem_slab chunk mapping used by the audio drivers */
		zaud_pool->blocks[i] = &base[pool->config.size * i];
	}

	return 0;
}

static int mp_zaud_buffer_pool_stop(struct mp_buffer_pool *pool)
{
	struct mp_zaud_buffer_pool *zaud_pool = (struct mp_zaud_buffer_pool *)pool;

	mp_zaud_buffer_pool_release_allocations(zaud_pool, true);

	return 0;
}

void mp_zaud_buffer_pool_init(struct mp_buffer_pool *pool)
{
	struct mp_zaud_buffer_pool *zaud_pool = (struct mp_zaud_buffer_pool *)pool;

	zaud_pool->zaud_dev = NULL;
	zaud_pool->mem_slab = NULL;
	zaud_pool->blocks = NULL;

	mp_buffer_pool_init(pool);

	pool->configure = mp_zaud_buffer_pool_config;
	pool->stop = mp_zaud_buffer_pool_stop;
}
