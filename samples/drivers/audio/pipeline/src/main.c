/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <mp.h>

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

int main(void)
{
	int gain_val = 90; /* Set gain to 90% (0.9x amplification) */
	int ret = 0;

	struct mp_pipeline pipe = {0};
	struct mp_zaud_dmic_src source = {0};
	struct mp_zaud_gain gain = {0};
	struct mp_zaud_i2s_codec_sink sink = {0};

	MP_ELEMENT_INIT(&pipe, pipeline, PIPE_ID);
	MP_ELEMENT_INIT(&source, zaud_dmic_src, DMIC_SRC_ID);
	MP_ELEMENT_INIT(&gain, zaud_gain, AUD_GAIN_ID);
	MP_ELEMENT_INIT(&sink, zaud_i2s_codec_sink, I2S_SINK_ID);

	ret = mp_object_set_properties(MP_OBJECT(&source), PROP_ZAUD_SRC_SLAB_PTR, &mem_slab,
				       PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for source element");
		goto err;
	}

	ret = mp_object_set_properties(MP_OBJECT(&sink), PROP_ZAUD_SINK_SLAB_PTR, &mem_slab,
				       PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for sink element");
		goto err;
	}

	ret = mp_object_set_properties(MP_OBJECT(&gain), PROP_GAIN, &gain_val, PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for gain element");
		goto err;
	}

	/* Caps filter element */
#if (CONFIG_MP_CAPSFILTER)
	struct mp_caps_filter caps_filter = {0};

	MP_ELEMENT_INIT(&caps_filter, caps_filter, CAPS_FILTER_ID);

	uint32_t frame_interval = 10000; /* 10ms */
	struct mp_caps *caps = mp_caps_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_FRAME_INTERVAL, MP_TYPE_UINT,
					   frame_interval, MP_CAPS_END);

	if (caps == NULL) {
		LOG_ERR("Failed to create a caps");
		goto err;
	}

	ret = mp_object_set_properties(MP_OBJECT(&caps_filter), PROP_CAPS, caps, PROP_LIST_END);
	mp_caps_unref(caps);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for caps filter element");
		goto err;
	}
#endif

	/* clang-format off */
	/* Add elements to the pipeline - order does not matter */
	if (!mp_bin_add(MP_BIN(&pipe),
			MP_ELEMENT(&source),
			IF_ENABLED(CONFIG_MP_CAPSFILTER, (MP_ELEMENT(&caps_filter),))
			MP_ELEMENT(&gain),
			MP_ELEMENT(&sink), NULL)) {
		LOG_ERR("Failed to add elements");
		goto err;
	}

	/* Link elements together - order does matter */
	if (!mp_element_link(MP_ELEMENT(&source),
			IF_ENABLED(CONFIG_MP_CAPSFILTER, (MP_ELEMENT(&caps_filter),))
			MP_ELEMENT(&gain),
			MP_ELEMENT(&sink), NULL)) {
		LOG_ERR("Failed to link elements");
		goto err;
	}
	/* clang-format on */

	/* Start playing */
	if (mp_element_set_state(MP_ELEMENT(&pipe), MP_STATE_PLAYING) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err;
	}

	/* Handle message from the pipeline */
	struct mp_bus *bus = mp_element_get_bus(MP_ELEMENT(&pipe));
	/* Wait until an Error or an EOS - blocking */
	struct mp_message *msg = mp_bus_pop_msg(bus, MP_MESSAGE_ERROR | MP_MESSAGE_EOS);

	if (msg != NULL) {
		switch (msg->type) {
		case MP_MESSAGE_ERROR:
			LOG_INF("ERROR message from element %d", msg->src->id);
			break;
		case MP_MESSAGE_EOS:
			LOG_INF("EOS message from element %d", msg->src->id);
			break;
		default:
			LOG_ERR("Unexpected message from element %d", msg->src->id);
			break;
		}
	}

	/* TODO: Stop pipeline and free allocated resources */
	mp_message_destroy(msg);

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
