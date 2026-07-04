/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp.h>
#include <zephyr/mp/zaud/mp_zaud.h>
#include <zephyr/mp/zaud/mp_zaud_src.h>
#include <zephyr/mp/zaud/mp_zaud_property.h>
#include <zephyr/mp/zaud/mp_zaud_i2s_codec_sink.h>
#include <zephyr/mp/zaud/mp_zaud_gain.h>
#include <zephyr/mp/zaud/mp_zaud_dmic_src.h>
#include <zephyr/mp/zaud/mp_zaud_buffer_pool.h>
#include <zephyr/mp/zbase/mp_capsfilter.h>

LOG_MODULE_REGISTER(main);

#define LOG_LEVEL LOG_LEVEL_DBG

#define PIPE_ID        0
#define DMIC_SRC_ID    1
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
static struct mp_zaud_dmic_src source;
static struct mp_zaud_gain gain;
static struct mp_zaud_i2s_codec_sink sink;
static struct mp_caps_filter caps_filter;

int main(void)
{
	int gain_val = 90; /* Set gain to 90% (0.9x amplification) */
	int ret = 0;

	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&source, mp_zaud_dmic_src_init, DMIC_SRC_ID);
	MP_ELEMENT_INIT(&gain, mp_zaud_gain_init, AUD_GAIN_ID);
	MP_ELEMENT_INIT(&sink, mp_zaud_i2s_codec_sink_init, I2S_SINK_ID);

	ret = mp_object_set_properties((struct mp_object *)&source, PROP_ZAUD_SRC_SLAB_PTR,
				       &mem_slab, PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for source element");
		goto err;
	}

	ret = mp_object_set_properties((struct mp_object *)&sink, PROP_ZAUD_SINK_SLAB_PTR,
				       &mem_slab,
#if (defined(CONFIG_USE_I2S_TARGET_CODEC_CONTROLLER) && CONFIG_USE_I2S_TARGET_CODEC_CONTROLLER == 1)
				       PROP_ZAUD_SINK_CLK_ROLE, MP_ZAUD_I2S_TARGET_CODEC_CONTROLLER,
#endif
				       PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for sink element");
		goto err;
	}

	ret = mp_object_set_properties((struct mp_object *)&gain, PROP_GAIN, &gain_val,
				       PROP_LIST_END);
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

	ret = mp_object_set_properties((struct mp_object *)&caps_filter, PROP_CAPS, caps,
				       PROP_LIST_END);
	mp_caps_unref(caps);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for caps filter element");
		goto err;
	}

	/* clang-format off */
	/* Add elements to the pipeline - order does not matter */
	ret = mp_bin_add((struct mp_bin *)&pipe,
			(struct mp_element *)&source,
			IF_ENABLED(CONFIG_MP_CAPSFILTER, ((struct mp_element *)&caps_filter,))
			(struct mp_element *)&gain,
			(struct mp_element *)&sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* Link elements together - order does matter */
	ret = mp_element_link((struct mp_element *)&source,
			IF_ENABLED(CONFIG_MP_CAPSFILTER, ((struct mp_element *)&caps_filter,))
			(struct mp_element *)&gain,
			(struct mp_element *)&sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}
	/* clang-format on */

	/* Start playing */
	if (mp_element_set_state((struct mp_element *)&pipe, MP_STATE_PLAYING) !=
	    MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err;
	}

	/* Handle message from the pipeline */
	struct mp_bus *bus = mp_element_get_bus((struct mp_element *)&pipe);
	struct mp_message msg;

	/* Wait until an Error or an EOS - blocking */
	mp_bus_pop_msg(bus, MP_MESSAGE_ERROR | MP_MESSAGE_EOS, &msg);

	switch (msg.type) {
	case MP_MESSAGE_ERROR:
		LOG_ERR("ERROR message from element %d", msg.origin->object.id);
		break;
	case MP_MESSAGE_EOS:
		LOG_INF("EOS message from element %d", msg.origin->object.id);
		break;
	default:
		LOG_ERR("Unexpected message from element %d", msg.origin->object.id);
		break;
	}

	/* Stop/Deinit the pipeline */
	(void)mp_element_set_state((struct mp_element *)&pipe, MP_STATE_READY);

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
