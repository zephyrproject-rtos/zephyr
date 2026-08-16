/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_buffer_pool.h>

LOG_MODULE_REGISTER(mpipe_aud_buffer_pool, CONFIG_MPIPE_LOG_LEVEL);

#define AUD_BUFFER_POOL_BASE_ALIGN sizeof(void *)
#define AUD_BUFFER_POOL_SIZE                                                                       \
	(CONFIG_MPIPE_AUD_BUFFER_POOL_SZ_MAX * CONFIG_MPIPE_AUD_BUFFER_POOL_NUM_MAX)

static __nocache __aligned(AUD_BUFFER_POOL_BASE_ALIGN)
uint8_t aud_buffer_pool_buf[AUD_BUFFER_POOL_SIZE];

static void mpipe_aud_buffer_pool_release(struct mpipe_aud_buffer_pool *aud_pool,
					  bool clear_mem_slab)
{
	memset(aud_pool->blocks, 0, sizeof(aud_pool->blocks));

	if ((aud_pool->mem_slab != NULL) && clear_mem_slab) {
		aud_pool->mem_slab->buffer = NULL;
		aud_pool->mem_slab = NULL;
	}
}

static int mpipe_aud_buffer_pool_config(struct mpipe_buffer_pool *pool,
					struct mpipe_structure *config)
{
	struct mpipe_aud_buffer_pool *aud_pool = (struct mpipe_aud_buffer_pool *)pool;
	int align = 0;
	uint32_t required_align = 0;
	uint8_t *base;
	int ret;

	uint32_t sample_rate, bit_width, num_of_channel, frame_interval;

	if (mpipe_aud_caps_get_uint(config, MPIPE_CAPS_SAMPLE_RATE, &sample_rate) != 0 ||
	    mpipe_aud_caps_get_uint(config, MPIPE_CAPS_BITWIDTH, &bit_width) != 0 ||
	    mpipe_aud_caps_get_uint(config, MPIPE_CAPS_NUM_OF_CHANNEL, &num_of_channel) != 0 ||
	    mpipe_aud_caps_get_uint(config, MPIPE_CAPS_FRAME_INTERVAL, &frame_interval) != 0) {
		return -EINVAL;
	}

	/*
	 * The buffer count was negotiated through the buffer pool query and left
	 * in pool->config.min_buffers by mpipe_aud_src_decide_buffer_pool().
	 *
	 * TEMPORARY WORKAROUND: Adding 2 extra buffers beyond the negotiated count
	 * because the current buffer management system requires additional buffers.
	 *
	 * TODO: Remove this hardcoded +2 offset when:
	 * - Buffer lifecycle management is properly implemented
	 * - Proper flow control prevents buffer starvation
	 */
	pool->config.min_buffers += MPIPE_AUD_BUFFER_POOL_EXTRA_BUFS;
	pool->config.size = (bit_width / BITS_PER_BYTE) * (sample_rate * frame_interval / 1000000) *
			    num_of_channel;
	/* The address needs to be aligned to the size of the DMA transfer */
	required_align = bit_width >> 3;

	/* Decide alignment using LCM */
	align = sys_lcm(AUD_BUFFER_POOL_BASE_ALIGN, required_align);

	if (align == -1) {
		LOG_ERR("Incompatible alignment requirements");
		return -EINVAL;
	} else if (align == 0 && required_align != 0) {
		pool->config.align = required_align;
	} else if (align != 0) {
		pool->config.align = align;
	} else {
		/* Both are 0, use base alignment */
		pool->config.align = AUD_BUFFER_POOL_BASE_ALIGN;
	}

	if (pool->config.size * pool->config.min_buffers > AUD_BUFFER_POOL_SIZE) {
		LOG_ERR("aud_buffer_pool_buf hos not enough space for requested buffers");
		return -EINVAL;
	}

	if (pool->config.min_buffers > MPIPE_AUD_BUFFER_POOL_BLOCKS_MAX) {
		LOG_ERR("%u blocks needed, MPIPE_AUD_BUFFER_POOL_NUM_MAX allows %d",
			pool->config.min_buffers, MPIPE_AUD_BUFFER_POOL_BLOCKS_MAX);
		return -EINVAL;
	}

	if (aud_pool->mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return -EINVAL;
	}

	mpipe_aud_buffer_pool_release(aud_pool, false);

	ret = k_mem_slab_init(aud_pool->mem_slab, (void *)aud_buffer_pool_buf, pool->config.size,
			      pool->config.min_buffers);
	if (ret != 0) {
		LOG_ERR("Unable to initialize memory slab (%d)", ret);
		return ret;
	}

	base = (uint8_t *)aud_pool->mem_slab->buffer;

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		/* Keep the mem_slab chunk mapping used by the audio drivers */
		aud_pool->blocks[i] = &base[pool->config.size * i];
	}

	return 0;
}

static int mpipe_aud_buffer_pool_stop(struct mpipe_buffer_pool *pool)
{
	struct mpipe_aud_buffer_pool *aud_pool = (struct mpipe_aud_buffer_pool *)pool;

	mpipe_aud_buffer_pool_release(aud_pool, true);

	return 0;
}

void mpipe_aud_buffer_pool_init(struct mpipe_buffer_pool *pool)
{
	__ASSERT_NO_MSG(pool != NULL);

	struct mpipe_aud_buffer_pool *aud_pool = (struct mpipe_aud_buffer_pool *)pool;

	aud_pool->aud_dev = NULL;
	aud_pool->mem_slab = NULL;
	memset(aud_pool->blocks, 0, sizeof(aud_pool->blocks));

	mpipe_buffer_pool_init(pool);

	pool->configure = mpipe_aud_buffer_pool_config;
	pool->stop = mpipe_aud_buffer_pool_stop;
}
