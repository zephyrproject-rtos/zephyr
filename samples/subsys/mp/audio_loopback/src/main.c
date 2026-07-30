/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/mp.h>
#include <zephyr/mp/aud/mp_aud.h>
#include <zephyr/mp/aud/mp_aud_src.h>
#include <zephyr/mp/aud/mp_aud_i2s_codec_sink.h>
#include <zephyr/mp/aud/mp_aud_gain.h>
#include <zephyr/mp/aud/mp_aud_dmic_src.h>
#include <zephyr/mp/aud/mp_aud_i2s_src.h>
#include <zephyr/mp/aud/mp_aud_buffer_pool.h>
#include <zephyr/mp/base/mp_capsfilter.h>
#include <zephyr/mp/utils/mp_player.h>

LOG_MODULE_REGISTER(main);

#define LOG_LEVEL LOG_LEVEL_DBG

#define PIPE_ID        0
#define AUD_SRC_ID     1
#define CAPS_FILTER_ID 2
#define AUD_GAIN_ID    3
#define I2S_SINK_ID    4

/*
 * WORKAROUND: Direct memory slab management in application code
 *
 * TODO: Normally, applications should not set this because they do not
 * need to know about the memory slab audio buffers implementation.
 *
 * The __nocache attribute ensures this memory is not cached, which is
 * required for DMA operations used by audio hardware.
 */
__nocache struct k_mem_slab mem_slab;

static struct mp_pipeline pipe;
#ifdef CONFIG_SAMPLE_AUDIO_SOURCE_I2S
static struct mp_aud_i2s_src source;
#define audio_src_init mp_aud_i2s_src_init
#else
static struct mp_aud_dmic_src source;
#define audio_src_init mp_aud_dmic_src_init
#endif
static struct mp_aud_gain gain;
static struct mp_aud_i2s_codec_sink sink;
static struct mp_caps_filter caps_filter;
static struct mp_player player;

int main(void)
{
	int gain_val = CONFIG_SAMPLE_AUDIO_GAIN_PERCENT;
	int ret = 0;

	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&source, audio_src_init, AUD_SRC_ID);
	MP_ELEMENT_INIT(&gain, mp_aud_gain_init, AUD_GAIN_ID);
	MP_ELEMENT_INIT(&sink, mp_aud_i2s_codec_sink_init, I2S_SINK_ID);

	ret = mp_object_set_properties((struct mp_object *)&source, MP_PROP_AUD_SRC_SLAB_PTR,
				       &mem_slab, MP_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for source element");
		goto err;
	}

	ret = mp_object_set_properties(
		(struct mp_object *)&sink, MP_PROP_AUD_SINK_SLAB_PTR, &mem_slab,
#if (defined(CONFIG_USE_I2S_TARGET_CODEC_CONTROLLER) && CONFIG_USE_I2S_TARGET_CODEC_CONTROLLER == 1)
		MP_PROP_AUD_SINK_CLK_ROLE, MP_AUD_I2S_TARGET_CODEC_CONTROLLER,
#endif
		MP_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for sink element");
		goto err;
	}

	ret = mp_object_set_properties((struct mp_object *)&gain, MP_PROP_AUD_TRANSFORM_GAIN,
				       &gain_val, MP_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for gain element");
		goto err;
	}

	/* Caps filter element */
	MP_ELEMENT_INIT(&caps_filter, mp_caps_filter_init, CAPS_FILTER_ID);

	uint32_t frame_interval = 10000; /* 10ms */
	struct mp_caps *caps =
		mp_caps_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_FRAME_INTERVAL, MP_TYPE_UINT,
			    frame_interval, MP_CAPS_NUM_OF_CHANNEL, MP_TYPE_UINT, 2, MP_CAPS_END);

	if (caps == NULL) {
		LOG_ERR("Failed to create a caps");
		goto err;
	}

	ret = mp_object_set_properties((struct mp_object *)&caps_filter,
				       MP_PROP_BASE_CAPSFILTER_CAPS, caps, MP_PROP_LIST_END);
	mp_caps_unref(caps);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for caps filter element");
		goto err;
	}

	/* clang-format off */
	/* Add elements to the pipeline - order does not matter */
	ret = mp_bin_add((struct mp_bin *)&pipe,
			(struct mp_element *)&source,
			IF_ENABLED(CONFIG_MP_BASE_CAPSFILTER, ((struct mp_element *)&caps_filter,))
			(struct mp_element *)&gain,
			(struct mp_element *)&sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* Link elements together - order does matter */
	ret = mp_element_link((struct mp_element *)&source,
			IF_ENABLED(CONFIG_MP_BASE_CAPSFILTER, ((struct mp_element *)&caps_filter,))
			(struct mp_element *)&gain,
			(struct mp_element *)&sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}
	/* clang-format on */

	/*
	 * Hand the pipeline to the mp_player helper, which runs its lifecycle on
	 * its own worker and exposes "mp player play/pause/stop/replay/quit"
	 * shell commands for control and debugging. mp_player_wait_quit() blocks
	 * until an error, EOS, or a "mp player quit"; mp_player_deinit() then
	 * stops and tears the pipeline down.
	 */
	ret = mp_player_init(&player, &pipe);
	if (ret < 0) {
		LOG_ERR("Failed to init player (%d)", ret);
		goto err;
	}

	(void)mp_player_play(&player);
	(void)mp_player_wait_quit(&player);
	(void)mp_player_deinit(&player);

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
