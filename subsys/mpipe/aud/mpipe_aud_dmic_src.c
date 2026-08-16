/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/logging/log.h>
#include <zephyr/audio/dmic.h>

#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/mpipe_value.h>

#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_buffer_pool.h>
#include <zephyr/mpipe/aud/mpipe_aud_dmic_src.h>

LOG_MODULE_REGISTER(mpipe_aud_dmic_src, CONFIG_MPIPE_LOG_LEVEL);

static int mpipe_aud_dmic_src_set_caps(struct mpipe_src *src, const struct mpipe_structure *caps)
{
	struct mpipe_aud_dmic_src *aud_dmic_src = (struct mpipe_aud_dmic_src *)src;

	uint32_t sample_rate, bit_width, num_of_channel, frame_interval;

	if (mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_SAMPLE_RATE, &sample_rate) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_BITWIDTH, &bit_width) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_NUM_OF_CHANNEL, &num_of_channel) != 0 ||
	    mpipe_aud_caps_get_uint(caps, MPIPE_CAPS_FRAME_INTERVAL, &frame_interval) != 0) {
		return -EINVAL;
	}

	struct pcm_stream_cfg stream = {
		.pcm_width = bit_width,
		.mem_slab = aud_dmic_src->pool.mem_slab,
	};

	struct dmic_cfg cfg = {0};

	/* These fields can be used to limit the PDM clock
	 * configurations that the driver is allowed to use
	 * to those supported by the microphone.
	 */
	/* TODO: Move to DT and driver init */
	cfg.io.min_pdm_clk_freq = 1000000;
	cfg.io.max_pdm_clk_freq = 3500000;
	cfg.io.min_pdm_clk_dc = 40;
	cfg.io.max_pdm_clk_dc = 60;

	cfg.streams = &stream;

	cfg.channel.req_num_streams = 1;

	if (aud_dmic_src->pool.mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return -EINVAL;
	}

	cfg.channel.req_num_chan = num_of_channel;
	cfg.channel.req_chan_map_lo = 0;
	cfg.channel.req_chan_map_hi = 0;
	for (uint32_t i = 0; i < num_of_channel; i++) {
		if (i < 8) {
			/* TODO: i%2 is hardcodded */
			cfg.channel.req_chan_map_lo |= dmic_build_channel_map(
				i, i / 2, (i % 2 ? PDM_CHAN_RIGHT : PDM_CHAN_LEFT));
		} else {
			/* TODO: i%2 is hardcodded */
			cfg.channel.req_chan_map_hi |= dmic_build_channel_map(
				i, i / 2, (i % 2 ? PDM_CHAN_RIGHT : PDM_CHAN_LEFT));
		}
	}

	cfg.streams[0].pcm_rate = sample_rate;
	cfg.streams[0].block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);

	LOG_DBG("PCM output rate: %u, channels: %u\n", cfg.streams[0].pcm_rate,
		cfg.channel.req_num_chan);

	if (dmic_configure(aud_dmic_src->pool.aud_dev, &cfg) < 0) {
		LOG_DBG("Failed to configure the driver");
		return -EIO;
	}

	mpipe_pad_set_caps(&src->src_pad, caps);

	return 0;
}

static int mpipe_aud_dmic_src_acquire_buffer(struct mpipe_buffer_pool *pool,
					     struct net_buf **buffer)
{
	struct mpipe_aud_buffer_pool *aud_pool =
		CONTAINER_OF(pool, struct mpipe_aud_buffer_pool, pool);
	struct mpipe_buffer_meta *meta;
	void *mem_block = NULL;
	size_t bytes_used = pool->config.size;
	int err = -1;

	err = dmic_read(aud_pool->aud_dev, 0, &mem_block, &bytes_used, INT32_MAX);
	if (err < 0) {
		LOG_ERR("Unable to read a DMIC buffer: %d", err);
		return err;
	}

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		if (mem_block == aud_pool->blocks[i]) {
			*buffer = net_buf_alloc_with_data(pool->nb_pool, aud_pool->blocks[i],
							  pool->config.size, K_NO_WAIT);
			if (*buffer == NULL) {
				LOG_ERR("Unable to allocate net_buf wrapper for DMIC buffer");
				k_mem_slab_free(aud_pool->mem_slab, mem_block);
				return -ENOBUFS;
			}

			(*buffer)->len = bytes_used;

			meta = mpipe_buffer_get_meta(*buffer);
			meta->pool = pool;
			meta->bytes_used = bytes_used;
			meta->timestamp = 0U;
			meta->driver_buf = NULL;
			meta->priv = NULL;

			return 0;
		}
	}

	LOG_ERR("Unable to match DMIC buffer %p with mem_slab backing store", mem_block);
	k_mem_slab_free(aud_pool->mem_slab, mem_block);
	return -ENOENT;
}

static int mpipe_aud_dmic_src_start(struct mpipe_buffer_pool *pool)
{
	struct mpipe_aud_buffer_pool *aud_pool =
		CONTAINER_OF(pool, struct mpipe_aud_buffer_pool, pool);

	/* Stream on */
	if (dmic_trigger(aud_pool->aud_dev, DMIC_TRIGGER_START) < 0) {
		LOG_ERR("Unable to start capture (interface)");
		return -EIO;
	}

	LOG_INF("Capture started now");

	return 0;
}

int mpipe_aud_dmic_src_init(struct mpipe_aud_dmic_src *aud_dmic_src, uint8_t id)
{
	__ASSERT_NO_MSG(aud_dmic_src != NULL);

	struct mpipe_element *self = &aud_dmic_src->aud_src.src.element;
	struct mpipe_src *src = &aud_dmic_src->aud_src.src;
	int ret = mpipe_aud_src_init(&aud_dmic_src->aud_src, id);

	if (ret != 0) {
		return ret;
	}

	mpipe_element_set_name(self, "aud_dmic_src");

	/* Initialize buffer pool */
	src->pool = &(aud_dmic_src->pool.pool);
	mpipe_aud_buffer_pool_init(src->pool);

	aud_dmic_src->pool.aud_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(dmic_dev));

	aud_dmic_src->aud_src.get_audio_caps = dmic_get_caps;

	src->set_caps = mpipe_aud_dmic_src_set_caps;
	src->pool->acquire_buffer = mpipe_aud_dmic_src_acquire_buffer;
	src->pool->start = mpipe_aud_dmic_src_start;

	mpipe_aud_src_update_caps(src);

	return 0;
}
