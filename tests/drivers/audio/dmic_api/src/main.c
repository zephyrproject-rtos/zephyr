/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Based on DMIC driver sample, which is:
 * Copyright (c) 2021 Nordic Semiconductor ASA
 */

#include <zephyr/kernel.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/ztest.h>

static const struct device *dmic_dev = DEVICE_DT_GET(DT_ALIAS(dmic_dev));

#if DT_HAS_COMPAT_STATUS_OKAY(nxp_dmic)
#define PDM_CHANNELS 4 /* Two L/R pairs of channels */
#define SAMPLE_BIT_WIDTH 16
#define BYTES_PER_SAMPLE sizeof(int16_t)
#define SLAB_ALIGN 4
#define MAX_SAMPLE_RATE  48000
/* Milliseconds to wait for a block to be read. */
#define READ_TIMEOUT 1000
/* Size of a block for 100 ms of audio data. */
#define BLOCK_SIZE(_sample_rate, _number_of_channels) \
	(BYTES_PER_SAMPLE * (_sample_rate / 10) * _number_of_channels)
#else
#error "Unsupported DMIC device"
#endif

/* Driver will allocate blocks from this slab to receive audio data into them.
 * Application, after getting a given block from the driver and processing its
 * data, needs to free that block.
 */
#define MAX_BLOCK_SIZE   BLOCK_SIZE(MAX_SAMPLE_RATE, PDM_CHANNELS)
#define BLOCK_COUNT      8
K_MEM_SLAB_DEFINE_STATIC(mem_slab, MAX_BLOCK_SIZE, BLOCK_COUNT, SLAB_ALIGN);

static struct pcm_stream_cfg pcm_stream = {
	.pcm_width = SAMPLE_BIT_WIDTH,
	.mem_slab = &mem_slab,
};
static struct dmic_cfg dmic_cfg = {
	.io = {
		/* These fields can be used to limit the PDM clock
		 * configurations that the driver is allowed to use
		 * to those supported by the microphone.
		 */
		.min_pdm_clk_freq = 1000000,
		.max_pdm_clk_freq = 3500000,
		.min_pdm_clk_dc   = 40,
		.max_pdm_clk_dc   = 60,
	},
	.streams = &pcm_stream,
	.channel = {
		.req_num_streams = 1,
	},
};

/* Verify that dmic_trigger fails when DMIC is not configured
 * this test must run first, before DMIC has been configured
 */
ZTEST(dmic, test_0_start_fail)
{
	int ret;

	zassert_true(device_is_ready(dmic_dev), "DMIC device is not ready");
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	zassert_not_equal(ret, 0, "DMIC trigger should fail when DMIC is not configured");
}

static int do_pdm_transfer(const struct device *dmic,
			    struct dmic_cfg *cfg)
{
	int ret;
	void *buffer;
	uint32_t size;

	TC_PRINT("PCM output rate: %u, channels: %u\n",
		 cfg->streams[0].pcm_rate, cfg->channel.req_num_chan);

	ret = dmic_configure(dmic, cfg);
	if (ret < 0) {
		TC_PRINT("DMIC configuration failed: %d\n", ret);
		return ret;
	}

	/* Check that the driver is properly populating the "act*" fields */
	zassert_equal(cfg->channel.act_num_chan,
		      cfg->channel.req_num_chan,
		      "DMIC configure should populate act_num_chan field");
	zassert_equal(cfg->channel.act_chan_map_lo,
		      cfg->channel.req_chan_map_lo,
		      "DMIC configure should populate act_chan_map_lo field");
	zassert_equal(cfg->channel.act_chan_map_hi,
		      cfg->channel.req_chan_map_hi,
		      "DMIC configure should populate act_chan_map_hi field");
	ret = dmic_trigger(dmic, DMIC_TRIGGER_START);
	if (ret < 0) {
		TC_PRINT("DMIC start trigger failed: %d\n", ret);
		return ret;
	}

	/* We read more than the total BLOCK_COUNT to insure the DMIC
	 * driver correctly reallocates memory slabs as it exhausts existing
	 * ones.
	 */
	for (int i = 0; i < (2 * BLOCK_COUNT); i++) {
		ret = dmic_read(dmic, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			TC_PRINT("DMIC read failed: %d\n", ret);
			return ret;
		}

		TC_PRINT("%d - got buffer %p of %u bytes\n", i, buffer, size);
		k_mem_slab_free(&mem_slab, buffer);
	}

	ret = dmic_trigger(dmic, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		TC_PRINT("DMIC stop trigger failed: %d\n", ret);
		return ret;
	}
	return 0;
}


/* Verify that the DMIC can transfer from a single channel */
ZTEST(dmic, test_single_channel)
{
	dmic_cfg.channel.req_num_chan = 1;
	dmic_cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
	dmic_cfg.streams[0].pcm_rate = MAX_SAMPLE_RATE;
	dmic_cfg.streams[0].block_size =
		BLOCK_SIZE(dmic_cfg.streams[0].pcm_rate,
			   dmic_cfg.channel.req_num_chan);
	zassert_equal(do_pdm_transfer(dmic_dev, &dmic_cfg), 0,
		      "Single channel transfer failed");
}

/* Verify that the DMIC can transfer from a L/R channel pair */
ZTEST(dmic, test_stereo_channel)
{
	dmic_cfg.channel.req_num_chan = 2;
	dmic_cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_LEFT) |
		dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
	dmic_cfg.streams[0].pcm_rate = MAX_SAMPLE_RATE;
	dmic_cfg.streams[0].block_size =
		BLOCK_SIZE(dmic_cfg.streams[0].pcm_rate,
			   dmic_cfg.channel.req_num_chan);
	zassert_equal(do_pdm_transfer(dmic_dev, &dmic_cfg), 0,
		      "L/R channel transfer failed");
	dmic_cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT) |
		dmic_build_channel_map(1, 0, PDM_CHAN_LEFT);
	zassert_equal(do_pdm_transfer(dmic_dev, &dmic_cfg), 0,
		      "R/L channel transfer failed");
}

/* Test DMIC with maximum number of channels */
ZTEST(dmic, test_max_channel)
{
	enum pdm_lr lr;
	uint8_t pdm_hw_chan;

	dmic_cfg.channel.req_num_chan = PDM_CHANNELS;
	dmic_cfg.channel.req_chan_map_lo = 0;
	dmic_cfg.channel.req_chan_map_hi = 0;
	for (uint8_t i = 0; i < PDM_CHANNELS; i++) {
		lr = ((i % 2) == 0) ? PDM_CHAN_LEFT : PDM_CHAN_RIGHT;
		pdm_hw_chan = i >> 1;
		if (i < 4) {
			dmic_cfg.channel.req_chan_map_lo |=
				dmic_build_channel_map(i, pdm_hw_chan, lr);
		} else {
			dmic_cfg.channel.req_chan_map_hi |=
				dmic_build_channel_map(i, pdm_hw_chan, lr);
		}
	}

	dmic_cfg.streams[0].pcm_rate = MAX_SAMPLE_RATE;
	dmic_cfg.streams[0].block_size =
		BLOCK_SIZE(dmic_cfg.streams[0].pcm_rate,
			   dmic_cfg.channel.req_num_chan);
	zassert_equal(do_pdm_transfer(dmic_dev, &dmic_cfg), 0,
		      "Maximum channel transfer failed");
}

/* Test pausing and restarting a channel */
ZTEST(dmic, test_pause_restart)
{
	int ret, i;
	void *buffer;
	uint32_t size;

	dmic_cfg.channel.req_num_chan = 1;
	dmic_cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_LEFT);
	dmic_cfg.streams[0].pcm_rate = MAX_SAMPLE_RATE;
	dmic_cfg.streams[0].block_size =
		BLOCK_SIZE(dmic_cfg.streams[0].pcm_rate,
			   dmic_cfg.channel.req_num_chan);
	ret = dmic_configure(dmic_dev, &dmic_cfg);
	zassert_equal(ret, 0, "DMIC configure failed");

	/* Start the DMIC, and pause it immediately */
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	zassert_equal(ret, 0, "DMIC start failed");
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_PAUSE);
	zassert_equal(ret, 0, "DMIC pause failed");
	/* There may be some buffers in the DMIC queue, but a read
	 * should eventually time out while it is paused
	 */
	for (i = 0; i < (2 * BLOCK_COUNT); i++) {
		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			break;
		}
		k_mem_slab_free(&mem_slab, buffer);
	}
	zassert_not_equal(ret, 0, "DMIC is paused, reads should timeout");
	TC_PRINT("Queue drained after %d reads\n", i);
	/* Unpause the DMIC */
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_RELEASE);
	zassert_equal(ret, 0, "DMIC release failed");
	/* Reads should not timeout now */
	for (i = 0; i < (2 * BLOCK_COUNT); i++) {
		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			break;
		}
		k_mem_slab_free(&mem_slab, buffer);
	}
	zassert_equal(ret, 0, "DMIC is active, reads should succeed");
	TC_PRINT("%d reads completed\n", (2 * BLOCK_COUNT));
	/* Stop the DMIC, and repeat the same tests */
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	zassert_equal(ret, 0, "DMIC stop failed");
	/* Versus a pause, DMIC reads should immediately stop once DMIC times
	 * out
	 */
	ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
	zassert_not_equal(ret, 0, "DMIC read should timeout when DMIC is stopped");
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_START);
	zassert_equal(ret, 0, "DMIC restart failed");
	/* Reads should not timeout now */
	for (i = 0; i < (2 * BLOCK_COUNT); i++) {
		ret = dmic_read(dmic_dev, 0, &buffer, &size, READ_TIMEOUT);
		if (ret < 0) {
			break;
		}
		k_mem_slab_free(&mem_slab, buffer);
	}
	zassert_equal(ret, 0, "DMIC is active, reads should succeed");
	TC_PRINT("%d reads completed\n", (2 * BLOCK_COUNT));
	/* Test is over. Stop the DMIC */
	ret = dmic_trigger(dmic_dev, DMIC_TRIGGER_STOP);
	zassert_equal(ret, 0, "DMIC stop failed");
}

/* Verify that channel map without adjacent L/R pairs fails */
ZTEST(dmic, test_bad_pair)
{
	int ret;

	dmic_cfg.channel.req_num_chan = 2;
	dmic_cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT) |
		dmic_build_channel_map(1, 0, PDM_CHAN_RIGHT);
	dmic_cfg.streams[0].pcm_rate = MAX_SAMPLE_RATE;
	dmic_cfg.streams[0].block_size =
		BLOCK_SIZE(dmic_cfg.streams[0].pcm_rate,
			   dmic_cfg.channel.req_num_chan);
	ret = dmic_configure(dmic_dev, &dmic_cfg);
	zassert_not_equal(ret, 0, "DMIC configure should fail with "
			  "two of same channel in map");

	dmic_cfg.channel.req_num_chan = 2;
	dmic_cfg.channel.req_chan_map_lo =
		dmic_build_channel_map(0, 0, PDM_CHAN_RIGHT) |
		dmic_build_channel_map(1, 1, PDM_CHAN_LEFT);
	ret = dmic_configure(dmic_dev, &dmic_cfg);
	zassert_not_equal(ret, 0, "DMIC configure should fail with "
			  "non adjacent channels in map");
}

ZTEST_SUITE(dmic, NULL, NULL, NULL, NULL, NULL);
