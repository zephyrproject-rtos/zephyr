/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <audio/dmic.h>

#define LOG_LEVEL LOG_LEVEL_INF
#include <logging/log.h>
LOG_MODULE_REGISTER(dmic_sample);

#define AUDIO_SAMPLE_FREQ	48000
#define AUDIO_SAMPLE_WIDTH	32
#define SAMPLES_PER_FRAME	(64)

#define NUM_MIC_CHANNELS	8
#define MIC_FRAME_SAMPLES	(SAMPLES_PER_FRAME * NUM_MIC_CHANNELS)
#define MIC_FRAME_BYTES		(MIC_FRAME_SAMPLES * AUDIO_SAMPLE_WIDTH / 8)

#define DMIC_DEV_NAME		"PDM"
#define MIC_IN_BUF_COUNT	2

#define FRAMES_PER_ITERATION	100
#define NUM_ITERATIONS		4
#define DELAY_BTW_ITERATIONS	K_MSEC(20)

static struct k_mem_slab dmic_mem_slab;
__attribute__((section(".dma_buffers")))
static char audio_buffers[MIC_FRAME_BYTES][MIC_IN_BUF_COUNT];
static struct device *dmic_device;

static void dmic_init(void)
{
	int ret;
	struct pcm_stream_cfg stream = {
		.pcm_rate		= AUDIO_SAMPLE_FREQ,
		.pcm_width		= AUDIO_SAMPLE_WIDTH,
		.block_size		= MIC_FRAME_BYTES,
		.mem_slab		= &dmic_mem_slab,
	};
	struct dmic_cfg cfg = {
		.io = {
			.min_pdm_clk_freq	= 1024000,
			.max_pdm_clk_freq	= 4800000,
			.min_pdm_clk_dc		= 48,
			.max_pdm_clk_dc		= 52,
			.pdm_clk_pol		= 0,
			.pdm_data_pol		= 0,
		},
		.streams			= &stream,
		.channel = {
			.req_chan_map_lo	=
				dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT) |
				dmic_build_channel_map(1, 0, PDM_CHAN_LEFT) |
				dmic_build_channel_map(2, 2, PDM_CHAN_RIGHT) |
				dmic_build_channel_map(3, 2, PDM_CHAN_LEFT) |
				dmic_build_channel_map(4, 1, PDM_CHAN_RIGHT) |
				dmic_build_channel_map(5, 1, PDM_CHAN_LEFT) |
				dmic_build_channel_map(6, 3, PDM_CHAN_RIGHT) |
				dmic_build_channel_map(7, 3, PDM_CHAN_LEFT),
			.req_num_chan		= NUM_MIC_CHANNELS,
			.req_num_streams	= 1,
		},
	};

	k_mem_slab_init(&dmic_mem_slab, audio_buffers, MIC_FRAME_BYTES,
			MIC_IN_BUF_COUNT);
	dmic_device = device_get_binding(DMIC_DEV_NAME);
	if (!dmic_device) {
		LOG_ERR("unable to find device %s", DMIC_DEV_NAME);
		return;
	}

	ret = dmic_configure(dmic_device, &cfg);
	if (ret != 0) {
		LOG_ERR("dmic_configure failed with %d error", ret);
	}
}

static void dmic_start(void)
{
	int ret;

	LOG_DBG("starting dmic");
	ret = dmic_trigger(dmic_device, DMIC_TRIGGER_START);
	if (ret) {
		LOG_ERR("dmic_trigger failed with code %d", ret);
	}
}

static void dmic_receive(void)
{
	int frame_counter = 0;
	s32_t *mic_in_buf;
	size_t size;
	int ret;

	while (frame_counter++ < FRAMES_PER_ITERATION) {
		ret = dmic_read(dmic_device, 0, (void **)&mic_in_buf, &size,
				K_FOREVER);
		if (ret) {
			LOG_ERR("dmic_read failed %d", ret);
		} else {
			LOG_DBG("dmic_read buffer %p size %u frame %d",
					mic_in_buf, size, frame_counter);
			k_mem_slab_free(&dmic_mem_slab, (void **)&mic_in_buf);
		}
	}
}

static void dmic_stop(void)
{
	int ret;

	ret = dmic_trigger(dmic_device, DMIC_TRIGGER_STOP);
	if (ret) {
		LOG_ERR("dmic_trigger failed with code %d", ret);
	} else {
		LOG_DBG("dmic stopped");
	}
}

static void dmic_sample_app(void *p1, void *p2, void *p3)
{
	int loop_count = 0;

	dmic_init();

	LOG_INF("Starting DMIC sample app ...");
	while (loop_count++ < NUM_ITERATIONS) {
		dmic_start();
		dmic_receive();
		dmic_stop();
		k_sleep(DELAY_BTW_ITERATIONS);
		LOG_INF("Iteration %d/%d complete, %d audio frames received.",
				loop_count, NUM_ITERATIONS,
				FRAMES_PER_ITERATION);
	}
	LOG_INF("Exiting DMIC sample app ...");
}

K_THREAD_DEFINE(dmic_sample, 1024, dmic_sample_app, NULL, NULL, NULL, 10, 0,
		K_NO_WAIT);
