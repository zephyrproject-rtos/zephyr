/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_buffer_pool.h>
#include <zephyr/mpipe/aud/mpipe_aud_dmic_src.h>
#include <zephyr/mpipe/aud/mpipe_aud_gain.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_codec_sink.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_src.h>
#include <zephyr/mpipe/base/mpipe_caps_filter.h>
#include <zephyr/mpipe/utils/mpipe_player.h>

LOG_MODULE_REGISTER(main);

enum {
	PIPE_ID,
	AUD_SRC_ID,
	CAPS_FILTER_ID,
	AUD_GAIN_ID,
	I2S_SINK_ID,
};

__nocache struct k_mem_slab mem_slab;

static struct mpipe pipe;
#ifdef CONFIG_SAMPLE_AUDIO_SOURCE_I2S
static struct mpipe_aud_i2s_src source;
#define audio_src_init(src, id)                                                                    \
	mpipe_aud_i2s_src_init((src), (id), DEVICE_DT_GET(DT_ALIAS(i2s_codec_rx)))
#else
static struct mpipe_aud_dmic_src source;
#define audio_src_init(src, id) mpipe_aud_dmic_src_init((src), (id))
#endif
static struct mpipe_caps_filter caps_filter;
static struct mpipe_aud_gain gain;
static struct mpipe_aud_i2s_codec_sink sink;
static struct mpipe_player player;

int main(void)
{
	int gain_val = CONFIG_SAMPLE_AUDIO_GAIN_PERCENT;
	struct mpipe_structure caps;
	int ret;

	ret = mpipe_pipeline_init(&pipe, PIPE_ID);
	if (ret < 0) {
		goto err;
	}
	ret = audio_src_init(&source, AUD_SRC_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_caps_filter_init(&caps_filter, CAPS_FILTER_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_aud_gain_init(&gain, AUD_GAIN_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_aud_i2s_codec_sink_init(&sink, I2S_SINK_ID);
	if (ret < 0) {
		goto err;
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&source,
					  MPIPE_PROP_AUD_SRC_SLAB_PTR, &mem_slab,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_object_set_properties((struct mpipe_object *)&sink,
					  MPIPE_PROP_AUD_SINK_SLAB_PTR, &mem_slab,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_object_set_properties((struct mpipe_object *)&gain,
					  MPIPE_PROP_AUD_TRANSFORM_GAIN, &gain_val,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	ret = mpipe_structure_init_fields(&caps, MPIPE_MEDIA_AUDIO_PCM,
					  MPIPE_CAPS_FRAME_INTERVAL, MPIPE_TYPE_UINT, 10000,
					  MPIPE_CAPS_NUM_OF_CHANNEL, MPIPE_TYPE_UINT, 2,
					  MPIPE_CAPS_END);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_object_set_properties((struct mpipe_object *)&caps_filter,
					  MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	ret = mpipe_bin_add((struct mpipe_bin *)&pipe, (struct mpipe_element *)&source,
			    (struct mpipe_element *)&caps_filter, (struct mpipe_element *)&gain,
			    (struct mpipe_element *)&sink, NULL);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_element_link((struct mpipe_element *)&source,
				 (struct mpipe_element *)&caps_filter,
				 (struct mpipe_element *)&gain,
				 (struct mpipe_element *)&sink, NULL);
	if (ret < 0) {
		goto err;
	}

	ret = mpipe_player_init(&player, &pipe);
	if (ret < 0) {
		goto err;
	}

	LOG_INF("player ready -- controls: player play | pause | stop | replay | status");
	mpipe_player_play(&player);

	return 0;

err:
	LOG_ERR("Aborting sample (%d)", ret);
	return ret != 0 ? ret : -EIO;
}
