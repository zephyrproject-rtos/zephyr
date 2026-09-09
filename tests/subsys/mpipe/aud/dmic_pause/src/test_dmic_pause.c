/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <limits.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <zephyr/audio/dmic.h>
#include <zephyr/device.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_bin.h>
#include <zephyr/mpipe/mpipe_element.h>
#include <zephyr/mpipe/mpipe_pipeline.h>
#include <zephyr/mpipe/mpipe_structure.h>
#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_dmic_src.h>
#include <zephyr/mpipe/aud/mpipe_aud_gain.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_codec_sink.h>
#include <zephyr/mpipe/base/mpipe_caps_filter.h>
#include <zephyr/mpipe/utils/mpipe_player.h>

struct test_graph {
	struct mpipe pipe;
	struct mpipe_aud_dmic_src src;
	struct mpipe_caps_filter caps_filter;
	struct mpipe_aud_gain gain;
	struct mpipe_aud_i2s_codec_sink sink;
	struct mpipe_player player;
	struct mpipe_structure caps;
};

__nocache struct k_mem_slab native_slab;
__nocache struct k_mem_slab interrupting_slab;

static struct test_graph native_graph;
static struct test_graph interrupting_graph;

struct interrupting_dmic_data {
	struct k_sem pause_sem;
	struct k_mem_slab *mem_slab;
	size_t block_size;
	atomic_t interrupt_read;
};

static int interrupting_dmic_configure(const struct device *dev, struct dmic_cfg *config)
{
	struct interrupting_dmic_data *data = dev->data;

	data->mem_slab = config->streams[0].mem_slab;
	data->block_size = config->streams[0].block_size;
	return 0;
}

static int interrupting_dmic_trigger(const struct device *dev, enum dmic_trigger cmd)
{
	struct interrupting_dmic_data *data = dev->data;

	switch (cmd) {
	case DMIC_TRIGGER_START:
		atomic_set(&data->interrupt_read, 1);
		break;
	case DMIC_TRIGGER_PAUSE:
		k_sem_give(&data->pause_sem);
		break;
	case DMIC_TRIGGER_RELEASE:
		atomic_clear(&data->interrupt_read);
		break;
	case DMIC_TRIGGER_STOP:
	case DMIC_TRIGGER_RESET:
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int interrupting_dmic_read(const struct device *dev, uint8_t stream, void **buffer,
				  size_t *size, int32_t timeout)
{
	struct interrupting_dmic_data *data = dev->data;
	int ret;

	ARG_UNUSED(stream);
	ARG_UNUSED(timeout);

	if (atomic_cas(&data->interrupt_read, 1, 0)) {
		k_sem_take(&data->pause_sem, K_FOREVER);
		return -EAGAIN;
	}

	ret = k_mem_slab_alloc(data->mem_slab, buffer, K_NO_WAIT);
	if (ret != 0) {
		return ret;
	}

	*size = data->block_size;
	return 0;
}

static int interrupting_dmic_get_caps(const struct device *dev, struct audio_caps *caps)
{
	ARG_UNUSED(dev);

	memset(caps, 0, sizeof(*caps));
	caps->min_total_channels = 1U;
	caps->max_total_channels = 2U;
	caps->supported_sample_rates = AUDIO_SAMPLE_RATE_48000;
	caps->supported_bit_widths = AUDIO_BIT_WIDTH_16;
	caps->min_num_buffers = 1U;
	caps->min_frame_interval = 1U;
	caps->max_frame_interval = UINT32_MAX;
	caps->interleaved = true;
	return 0;
}

static DEVICE_API(dmic, interrupting_dmic_api) = {
	.configure = interrupting_dmic_configure,
	.trigger = interrupting_dmic_trigger,
	.read = interrupting_dmic_read,
	.get_caps = interrupting_dmic_get_caps,
};

static int interrupting_dmic_init(const struct device *dev)
{
	struct interrupting_dmic_data *data = dev->data;

	k_sem_init(&data->pause_sem, 0, 1);
	return 0;
}

static struct interrupting_dmic_data interrupting_dmic_data;

DEVICE_DEFINE(interrupting_dmic, "interrupting_dmic", interrupting_dmic_init, NULL,
	      &interrupting_dmic_data, NULL, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &interrupting_dmic_api);

static bool wait_for_state(struct mpipe_player *player, enum mpipe_player_state state)
{
	for (int i = 0; i < 300; i++) {
		if (player->state == state) {
			return true;
		}
		k_sleep(K_MSEC(10));
	}

	return false;
}

static void setup_graph(struct test_graph *graph, struct k_mem_slab *slab,
			const struct device *dmic_dev)
{
	zassert_ok(mpipe_pipeline_init(&graph->pipe, 0));
	zassert_ok(mpipe_aud_dmic_src_init(&graph->src, 1));
	if (dmic_dev != NULL) {
		graph->src.pool.aud_dev = dmic_dev;
	}
	zassert_ok(mpipe_caps_filter_init(&graph->caps_filter, 2));
	zassert_ok(mpipe_aud_gain_init(&graph->gain, 3));
	zassert_ok(mpipe_aud_i2s_codec_sink_init(&graph->sink, 4));

	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&graph->src,
					       MPIPE_PROP_AUD_SRC_SLAB_PTR, slab,
					       MPIPE_PROP_LIST_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&graph->sink,
					       MPIPE_PROP_AUD_SINK_SLAB_PTR, slab,
					       MPIPE_PROP_LIST_END));
	zassert_ok(mpipe_structure_init_fields(
		&graph->caps, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_FRAME_INTERVAL, MPIPE_TYPE_UINT,
		10000, MPIPE_CAPS_NUM_OF_CHANNEL, MPIPE_TYPE_UINT, 2, MPIPE_CAPS_END));
	zassert_ok(mpipe_object_set_properties((struct mpipe_object *)&graph->caps_filter,
					       MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &graph->caps,
					       MPIPE_PROP_LIST_END));
	zassert_ok(mpipe_element_link(
		(struct mpipe_element *)&graph->src, (struct mpipe_element *)&graph->caps_filter,
		(struct mpipe_element *)&graph->gain, (struct mpipe_element *)&graph->sink, NULL));
	zassert_ok(mpipe_bin_add(
		(struct mpipe_bin *)&graph->pipe, (struct mpipe_element *)&graph->src,
		(struct mpipe_element *)&graph->caps_filter, (struct mpipe_element *)&graph->gain,
		(struct mpipe_element *)&graph->sink, NULL));
	zassert_ok(mpipe_player_init(&graph->player, &graph->pipe));
}

ZTEST(mpipe_aud_dmic_pause, test_resume_after_capture_settles_while_paused)
{
	struct mpipe_player *player = &native_graph.player;

	setup_graph(&native_graph, &native_slab, NULL);
	zassert_ok(mpipe_player_play(player));
	zassert_true(wait_for_state(player, MPIPE_PLAYER_PLAYING), "initial play failed");
	/* Reach steady-state capture before pausing; an immediate pause can hide
	 * a transient shared-pool shortage.
	 */
	k_sleep(K_SECONDS(20));

	zassert_ok(mpipe_player_pause(player));
	zassert_true(wait_for_state(player, MPIPE_PLAYER_PAUSED), "pause failed");
	k_sleep(K_SECONDS(1));
	zassert_not_equal(player->last_error.type, MPIPE_MESSAGE_ERROR,
			  "capture reported %d while paused", player->last_error.code);
	zassert_equal(player->state, MPIPE_PLAYER_PAUSED,
		      "an interrupted capture read stopped the paused pipeline");

	zassert_ok(mpipe_player_play(player));
	zassert_true(wait_for_state(player, MPIPE_PLAYER_PLAYING), "resume did not start");
	k_sleep(K_MSEC(100));
	zassert_not_equal(player->last_error.type, MPIPE_MESSAGE_ERROR,
			  "capture reported %d after resume", player->last_error.code);
	zassert_equal(player->state, MPIPE_PLAYER_PLAYING,
		      "capture exhausted the shared pool across pause");

	zassert_ok(mpipe_player_deinit(player));
}

ZTEST(mpipe_aud_dmic_pause, test_pause_flushes_interrupted_capture_read)
{
	struct mpipe_player *player = &interrupting_graph.player;

	setup_graph(&interrupting_graph, &interrupting_slab, DEVICE_GET(interrupting_dmic));
	zassert_ok(mpipe_player_play(player));
	zassert_true(wait_for_state(player, MPIPE_PLAYER_PLAYING), "initial play failed");

	zassert_ok(mpipe_player_pause(player));
	k_sleep(K_MSEC(100));
	zassert_not_equal(player->last_error.type, MPIPE_MESSAGE_ERROR,
			  "capture reported %d while paused", player->last_error.code);
	zassert_equal(player->state, MPIPE_PLAYER_PAUSED,
		      "an interrupted capture read stopped the paused pipeline");

	zassert_ok(mpipe_player_play(player));
	zassert_true(wait_for_state(player, MPIPE_PLAYER_PLAYING), "resume did not start");
	zassert_ok(mpipe_player_deinit(player));
}

ZTEST_SUITE(mpipe_aud_dmic_pause, NULL, NULL, NULL, NULL, NULL);
