/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/audio/dmic.h>

#include "dmic_capture.h"

/* Bounded drain of buffers the driver frees asynchronously after STOP, before freeing the pool. */
#define DMIC_CAPTURE_STOP_DRAIN_ATTEMPTS 20
#define DMIC_CAPTURE_STOP_DRAIN_DELAY_MS 5

/* Carve the capture pool out of the system heap and init a slab over it. */
static int dmic_capture_pool_alloc(struct dmic_capture *cap, uint32_t block_size,
				   uint8_t block_count)
{
	size_t slab_block = ROUND_UP(block_size, sizeof(void *));

	cap->pool = k_malloc(slab_block * block_count);
	if (cap->pool == NULL) {
		return -ENOMEM;
	}

	return k_mem_slab_init(&cap->slab, cap->pool, slab_block, block_count);
}

static void dmic_capture_pool_free(struct dmic_capture *cap)
{
	k_free(cap->pool);
	cap->pool = NULL;
}

int dmic_capture_start(struct dmic_capture *cap, const struct device *dev,
		       const struct dmic_capture_cfg *cfg)
{
	struct pcm_stream_cfg stream = {0};
	struct dmic_cfg dmic = {0};
	uint32_t block_size;
	int ret;

	if (cfg->sample_rate == 0U) {
		return -EINVAL;
	}

	block_size = cfg->sample_rate * cfg->channels * (cfg->pcm_width / 8U) *
		     cfg->block_duration_ms / 1000U;
	/* block_size is stored as a uint16_t; reject anything that would truncate. */
	if (block_size == 0U || block_size > UINT16_MAX) {
		return -EINVAL;
	}

	cap->dev = dev;
	cap->block_size = block_size;

	ret = dmic_capture_pool_alloc(cap, block_size, cfg->block_count);
	if (ret < 0) {
		return ret;
	}

	stream.pcm_rate = cfg->sample_rate;
	stream.pcm_width = cfg->pcm_width;
	stream.block_size = block_size;
	stream.mem_slab = &cap->slab;

	dmic.io.min_pdm_clk_freq = cfg->min_pdm_clk_freq;
	dmic.io.max_pdm_clk_freq = cfg->max_pdm_clk_freq;
	dmic.io.min_pdm_clk_dc = cfg->min_pdm_clk_dc;
	dmic.io.max_pdm_clk_dc = cfg->max_pdm_clk_dc;

	dmic.streams = &stream;
	dmic.channel.req_num_streams = 1U;
	dmic.channel.req_num_chan = cfg->channels;
	dmic.channel.req_chan_map_lo = dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
	if (cfg->channels == 2U) {
		dmic.channel.req_chan_map_lo |= dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
	}

	ret = dmic_configure(dev, &dmic);
	if (ret < 0) {
		dmic_capture_pool_free(cap);
		return ret;
	}

	ret = dmic_trigger(dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		dmic_capture_pool_free(cap);
		return ret;
	}

	return 0;
}

int dmic_capture_read(struct dmic_capture *cap, void **buf, size_t *size, int32_t timeout_ms)
{
	return dmic_read(cap->dev, 0, buf, size, timeout_ms);
}

void dmic_capture_free_block(struct dmic_capture *cap, void *buf)
{
	k_mem_slab_free(&cap->slab, buf);
}

void dmic_capture_stop(struct dmic_capture *cap)
{
	(void)dmic_trigger(cap->dev, DMIC_TRIGGER_STOP);

	/* STOP is async; drain queued blocks and wait for the slab to empty before freeing it. */
	for (int i = 0; i < DMIC_CAPTURE_STOP_DRAIN_ATTEMPTS; i++) {
		void *buf;
		size_t size;

		while (dmic_read(cap->dev, 0, &buf, &size, 0) == 0) {
			k_mem_slab_free(&cap->slab, buf);
		}
		if (k_mem_slab_num_used_get(&cap->slab) == 0U) {
			break;
		}
		k_msleep(DMIC_CAPTURE_STOP_DRAIN_DELAY_MS);
	}

	uint32_t in_use = k_mem_slab_num_used_get(&cap->slab);

	__ASSERT(in_use == 0U, "%u slab block(s) still in use after STOP drain", in_use);
	if (in_use != 0U) {
		/* Leak the pool: the driver still owns blocks, freeing now risks a UAF. */
		return;
	}

	dmic_capture_pool_free(cap);
}
