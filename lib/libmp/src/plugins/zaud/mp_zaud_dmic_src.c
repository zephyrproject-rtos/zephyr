/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>

#include <zephyr/logging/log.h>
#include <zephyr/audio/dmic.h>

#include "mp_zaud.h"
#include "mp_zaud_buffer_pool.h"
#include "mp_zaud_property.h"
#include "mp_zaud_dmic_src.h"

LOG_MODULE_REGISTER(mp_zaud_dmic_src, CONFIG_LIBMP_LOG_LEVEL);

static bool mp_zaud_dmic_src_set_caps(struct mp_src *src, struct mp_caps *caps)
{
	struct mp_zaud_dmic_src *zaud_dmic_src = MP_ZAUD_DMIC_SRC(src);

	struct mp_structure *first_structure = mp_caps_get_structure(caps, 0);

	int sample_rate =
		mp_value_get_int(mp_structure_get_value(first_structure, MP_CAPS_SAMPLE_RATE));
	int bit_width = mp_value_get_int(mp_structure_get_value(first_structure, MP_CAPS_BITWIDTH));
	int num_of_channel =
		mp_value_get_int(mp_structure_get_value(first_structure, MP_CAPS_NUM_OF_CHANNEL));
	uint32_t frame_interval =
		mp_value_get_uint(mp_structure_get_value(first_structure, MP_CAPS_FRAME_INTERVAL));

	struct pcm_stream_cfg stream = {
		.pcm_width = bit_width,
		.mem_slab = zaud_dmic_src->pool.mem_slab,
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

	if (zaud_dmic_src->pool.mem_slab == NULL) {
		LOG_ERR("Memory slab not configured");
		return false;
	}

	cfg.channel.req_num_chan = num_of_channel;
	cfg.channel.req_chan_map_lo = 0;
	cfg.channel.req_chan_map_hi = 0;
	for (int i = 0; i < num_of_channel; i++) {
		if (i < 8) {
			/* TODO: i%2 is hardcodded */
			cfg.channel.req_chan_map_lo |= dmic_build_channel_map(
				i, i / 2, (i % 2 ? PDM_CHAN_LEFT : PDM_CHAN_RIGHT));
		} else {
			/* TODO: i%2 is hardcodded */
			cfg.channel.req_chan_map_hi |= dmic_build_channel_map(
				i, i / 2, (i % 2 ? PDM_CHAN_LEFT : PDM_CHAN_RIGHT));
		}
	}

	cfg.streams[0].pcm_rate = sample_rate;
	cfg.streams[0].block_size =
		(bit_width >> 3) * ((sample_rate * frame_interval / 1000000) * num_of_channel);

	LOG_DBG("PCM output rate: %u, channels: %u\n", cfg.streams[0].pcm_rate,
		cfg.channel.req_num_chan);

	if (dmic_configure(zaud_dmic_src->pool.zaud_dev, &cfg) < 0) {
		LOG_DBG("Failed to configure the driver");
		return false;
	}

	mp_caps_replace(&(src->srcpad.caps), caps);

	return true;
}

static bool mp_zaud_dmic_src_acquire_buffer(struct mp_buffer_pool *pool, struct mp_buffer **buffer)
{
	struct mp_zaud_buffer_pool *zaud_pool = MP_ZAUD_BUFFER_POOL(pool);
	void *mem_block = NULL;
	int err = -1;

	err = dmic_read(zaud_pool->zaud_dev, 0, &mem_block, &pool->config.size, INT32_MAX);
	if (err < 0) {
		LOG_ERR("Unable to read a DMIC buffer: %d", err);
		return false;
	}

	for (uint8_t i = 0; i < pool->config.min_buffers; i++) {
		if (mem_block == pool->buffers[i].data) {
			*buffer = &pool->buffers[i];
			break;
		}
	}

	return true;
}

static bool mp_zaud_dmic_src_start(struct mp_buffer_pool *pool)
{
	struct mp_zaud_buffer_pool *zaud_pool = MP_ZAUD_BUFFER_POOL(pool);

	/* Stream on */
	if (dmic_trigger(zaud_pool->zaud_dev, DMIC_TRIGGER_START) < 0) {
		LOG_ERR("Unable to start capture (interface)");
		return false;
	}

	LOG_INF("Capture started");
	return true;
}

void mp_zaud_dmic_src_init(struct mp_element *self)
{
	struct mp_src *src = MP_SRC(self);
	struct mp_zaud_dmic_src *zaud_dmic_src = MP_ZAUD_DMIC_SRC(self);

	/* Init base class */
	mp_zaud_src_init(MP_ELEMENT(&(zaud_dmic_src->zaud_src)));

	/* Initialize buffer pool */
	src->pool = &(zaud_dmic_src->pool.pool);
	mp_zaud_buffer_pool_init(src->pool);

#if DT_NODE_EXISTS(DT_NODELABEL(dmic_dev))
	zaud_dmic_src->pool.zaud_dev = DEVICE_DT_GET(DT_NODELABEL(dmic_dev));
#else
#error "dmic_dev node label not found in device tree."
#endif

	if (!device_is_ready(zaud_dmic_src->pool.zaud_dev)) {
		LOG_ERR("%s is not ready", zaud_dmic_src->pool.zaud_dev->name);
		return;
	}

	zaud_dmic_src->zaud_src.get_audio_caps = dmic_get_caps;

	src->set_caps = mp_zaud_dmic_src_set_caps;
	src->pool->acquire_buffer = mp_zaud_dmic_src_acquire_buffer;
	src->pool->start = mp_zaud_dmic_src_start;

	src->srcpad.caps = src->get_caps(src);
}
