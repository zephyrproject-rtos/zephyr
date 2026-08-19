/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/zbus/zbus.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/aud/mpipe_aud.h>
#include <zephyr/mpipe/aud/mpipe_aud_src.h>
#include <zephyr/mpipe/aud/mpipe_aud_i2s_codec_sink.h>
#include <zephyr/mpipe/aud/mpipe_aud_gain.h>
#include <zephyr/mpipe/aud/mpipe_aud_dmic_src.h>
#include <zephyr/mpipe/aud/mpipe_aud_buffer_pool.h>
#include <zephyr/mpipe/base/mpipe_caps_filter.h>

LOG_MODULE_REGISTER(main);

#define LOG_LEVEL LOG_LEVEL_DBG

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	DMIC_SRC_ID,
	CAPS_FILTER_ID,
	AUD_GAIN_ID,
	I2S_SINK_ID,
};

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

ZBUS_MSG_SUBSCRIBER_DEFINE(main_sub);

static struct mpipe pipe;

static struct mpipe_aud_dmic_src source;
static struct mpipe_aud_gain gain;
static struct mpipe_aud_i2s_codec_sink sink;
static struct mpipe_caps_filter caps_filter;

int main(void)
{
	int gain_val = 90; /* Set gain to 90% (0.9x amplification) */
	struct zbus_channel *bus = NULL;
	int ret = 0;

	ret = mpipe_pipeline_init(&pipe, PIPE_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_aud_dmic_src_init(&source, DMIC_SRC_ID);
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
		LOG_ERR("Failed to set properties for source element");
		goto err;
	}

	ret = mpipe_object_set_properties(
		(struct mpipe_object *)&sink, MPIPE_PROP_AUD_SINK_SLAB_PTR, &mem_slab,
#if (defined(CONFIG_USE_I2S_TARGET_CODEC_CONTROLLER) && CONFIG_USE_I2S_TARGET_CODEC_CONTROLLER == 1)
		MPIPE_PROP_AUD_SINK_CLK_ROLE, MPIPE_AUD_I2S_TARGET_CODEC_CONTROLLER,
#endif
		MPIPE_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for sink element");
		goto err;
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&gain,
					  MPIPE_PROP_AUD_TRANSFORM_GAIN, &gain_val,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for gain element");
		goto err;
	}

	/* Caps filter element */
	ret = mpipe_caps_filter_init(&caps_filter, CAPS_FILTER_ID);
	if (ret < 0) {
		goto err;
	}

	uint32_t frame_interval = 10000; /* 10ms */
	struct mpipe_structure caps;

	ret = mpipe_structure_init_fields(
		&caps, MPIPE_MEDIA_AUDIO_PCM, MPIPE_CAPS_FRAME_INTERVAL, MPIPE_TYPE_UINT,
		frame_interval, MPIPE_CAPS_NUM_OF_CHANNEL, MPIPE_TYPE_UINT, 2, MPIPE_CAPS_END);

	if (ret != 0) {
		LOG_ERR("Failed to create a caps");
		goto err;
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&caps_filter,
					  MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for caps filter element");
		goto err;
	}

	/* clang-format off */
	/* Add elements to the pipeline - order does not matter */
	ret = mpipe_bin_add((struct mpipe_bin *)&pipe,
			(struct mpipe_element *)&source,
			IF_ENABLED(CONFIG_MPIPE_BASE_CAPS_FILTER,
				   ((struct mpipe_element *)&caps_filter,))
			(struct mpipe_element *)&gain,
			(struct mpipe_element *)&sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* Link elements together - order does matter */
	ret = mpipe_element_link((struct mpipe_element *)&source,
			IF_ENABLED(CONFIG_MPIPE_BASE_CAPS_FILTER,
				   ((struct mpipe_element *)&caps_filter,))
			(struct mpipe_element *)&gain,
			(struct mpipe_element *)&sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}
	/* clang-format on */

	bus = mpipe_element_get_bus_chan((struct mpipe_element *)&pipe);

	ret = zbus_chan_add_obs(bus, &main_sub, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to attach observer to pipeline channel (%d)", ret);
		goto err;
	}

	/* Start playing */
	if (mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_PLAYING) !=
	    MPIPE_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err_set_state;
	}

	/* Handle message from the pipeline */
	const struct zbus_channel *chan;
	struct mpipe_message msg;

	/* Wait until an Error or an EOS */
	int sub_ret;

	do {
		sub_ret = zbus_sub_wait_msg(&main_sub, &chan, &msg, K_FOREVER);
		if (sub_ret != 0) {
			LOG_ERR("zbus_sub_wait_msg failed (%d)", sub_ret);
			goto err_set_state;
		}
	} while ((msg.type & (MPIPE_MESSAGE_ERROR | MPIPE_MESSAGE_EOS)) == 0);

	int origin_id = (msg.origin != NULL) ? msg.origin->object.id : -1;

	switch (msg.type) {
	case MPIPE_MESSAGE_ERROR:
		LOG_ERR("ERROR message from element %d", origin_id);
		break;
	case MPIPE_MESSAGE_EOS:
		LOG_INF("EOS message from element %d", origin_id);
		break;
	default:
		LOG_ERR("Unexpected message from element %d", origin_id);
		break;
	}

	(void)zbus_chan_rm_obs(bus, &main_sub, K_FOREVER);

	/* Stop/Deinit the pipeline */
	(void)mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_READY);

	return 0;

err_set_state:
	zbus_chan_rm_obs(bus, &main_sub, K_FOREVER);
err:
	LOG_ERR("Aborting sample");
	return ret != 0 ? ret : -EIO;
}
