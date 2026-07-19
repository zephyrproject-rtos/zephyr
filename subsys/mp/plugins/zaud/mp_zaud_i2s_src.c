/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/drivers/i2s.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp_structure.h>
#include <zephyr/mp/core/mp_value.h>

#include <zephyr/mp/zaud/mp_zaud.h>
#include <zephyr/mp/zaud/mp_zaud_buffer_pool.h>
#include <zephyr/mp/zaud/mp_zaud_i2s_src.h>
#include <zephyr/mp/zaud/mp_zaud_property.h>

LOG_MODULE_REGISTER(mp_zaud_i2s_src, CONFIG_MP_LOG_LEVEL);

/*
 * The base source builds its capabilities through a get_audio_caps() callback
 * that takes no direction, so bind i2s_get_caps() to the receive direction.
 */
static int mp_zaud_i2s_src_get_audio_caps(const struct device *dev, struct audio_caps *caps)
{
	return i2s_get_caps(dev, caps, I2S_DIR_RX);
}

static int mp_zaud_i2s_src_set_caps(struct mp_src *src, struct mp_caps *caps)
{
	struct mp_zaud_buffer_pool *pool =
		CONTAINER_OF(src->pool, struct mp_zaud_buffer_pool, pool);
	struct i2s_config config;
	int ret;

	struct mp_structure *first_structure = mp_caps_get_structure(caps, 0);

	uint32_t sample_rate =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_SAMPLE_RATE));
	uint32_t bit_width =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_BITWIDTH));
	uint32_t num_of_channel =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_NUM_OF_CHANNEL));
	uint32_t frame_interval =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_FRAME_INTERVAL));

	if (pool->mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return -EINVAL;
	}

	config.word_size = bit_width;
	config.channels = num_of_channel;
	config.format = I2S_FMT_DATA_FORMAT_I2S;
	/* The I2S receiver drives the clocks for a microphone that has none. */
	config.options = I2S_OPT_BIT_CLK_CONTROLLER | I2S_OPT_FRAME_CLK_CONTROLLER;
	config.frame_clk_freq = sample_rate;
	config.mem_slab = pool->mem_slab;
	config.block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);
	/*
	 * i2s_config.timeout is in milliseconds while frame_interval is in
	 * microseconds. Wait a few frame periods for a captured block before
	 * reporting a timeout.
	 */
	config.timeout = MAX(100U, (frame_interval / 1000U) * 4U);

	ret = i2s_configure(pool->zaud_dev, I2S_DIR_RX, &config);
	if (ret < 0) {
		LOG_ERR("Failed to configure I2S stream: %d", ret);
		return ret;
	}

	mp_caps_replace(&(src->srcpad.caps), caps);

	return 0;
}

static int mp_zaud_i2s_src_acquire_buffer(struct mp_buffer_pool *pool, struct net_buf **buffer)
{
	struct mp_zaud_buffer_pool *zaud_pool =
		CONTAINER_OF(pool, struct mp_zaud_buffer_pool, pool);
	struct mp_buffer_meta *meta;
	void *mem_block = NULL;
	size_t bytes_used = pool->config.size;
	int err;

	err = i2s_read(zaud_pool->zaud_dev, &mem_block, &bytes_used);
	if (err < 0) {
		LOG_ERR("Unable to read an I2S buffer: %d", err);
		return err;
	}

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		if (mem_block == zaud_pool->blocks[i]) {
			*buffer = net_buf_alloc_with_data(pool->nb_pool, zaud_pool->blocks[i],
							  pool->config.size, K_NO_WAIT);
			if (*buffer == NULL) {
				LOG_ERR("Unable to allocate net_buf wrapper for I2S buffer");
				k_mem_slab_free(zaud_pool->mem_slab, mem_block);
				return -ENOBUFS;
			}

			(*buffer)->len = bytes_used;

			meta = mp_buffer_get_meta(*buffer);
			meta->pool = pool;
			meta->bytes_used = bytes_used;
			meta->timestamp = 0U;
			meta->driver_buf = NULL;
			meta->priv = NULL;

			return 0;
		}
	}

	LOG_ERR("Unable to match I2S buffer %p with mem_slab backing store", mem_block);
	k_mem_slab_free(zaud_pool->mem_slab, mem_block);
	return -ENOENT;
}

static int mp_zaud_i2s_src_start(struct mp_buffer_pool *pool)
{
	struct mp_zaud_buffer_pool *zaud_pool =
		CONTAINER_OF(pool, struct mp_zaud_buffer_pool, pool);

	if (i2s_trigger(zaud_pool->zaud_dev, I2S_DIR_RX, I2S_TRIGGER_START) < 0) {
		LOG_ERR("Unable to start I2S capture");
		return -EIO;
	}

	LOG_INF("Capture started now");
	return 0;
}

void mp_zaud_i2s_src_init(struct mp_element *self)
{
	struct mp_src *src = (struct mp_src *)self;
	struct mp_zaud_i2s_src *zaud_i2s_src = (struct mp_zaud_i2s_src *)src;

	/* Init base class */
	mp_zaud_src_init(&zaud_i2s_src->zaud_src.src.element);

	/* Initialize buffer pool */
	src->pool = &(zaud_i2s_src->pool.pool);
	mp_zaud_buffer_pool_init(src->pool);

	zaud_i2s_src->pool.zaud_dev = DEVICE_DT_GET_OR_NULL(DT_ALIAS(i2s_codec_rx));

	zaud_i2s_src->zaud_src.get_audio_caps = mp_zaud_i2s_src_get_audio_caps;

	src->set_caps = mp_zaud_i2s_src_set_caps;
	src->pool->acquire_buffer = mp_zaud_i2s_src_acquire_buffer;
	src->pool->start = mp_zaud_i2s_src_start;

	mp_zaud_src_update_caps(src);
}
