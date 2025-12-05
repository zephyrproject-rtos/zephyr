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

#define PIPELINE_ID    0
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
	int gain = 90; /* Set gain to 90% (0.9x amplification) */
	int ret = 0;

	/* Create a new pipeline */
	struct mp_element *pipeline = mp_pipeline_new(PIPELINE_ID);

	if (pipeline == NULL) {
		LOG_ERR("Failed to create pipeline");
		goto err;
	}

	/* Create elements */
	struct mp_element *source = mp_element_factory_create(MP_ZAUD_DMIC_SRC_ELEM, DMIC_SRC_ID);

	if (source == NULL) {
		LOG_ERR("Failed to create dmic element");
		goto err;
	}

	struct mp_element *caps_filter =
		mp_element_factory_create(MP_CAPS_FILTER_ELEM, CAPS_FILTER_ID);

	if (caps_filter == NULL) {
		LOG_ERR("Failed to create cap filter element");
		goto err;
	}

	struct mp_element *transform = mp_element_factory_create(MP_ZAUD_GAIN_ELEM, AUD_GAIN_ID);

	if (transform == NULL) {
		LOG_ERR("Failed to create gain element");
		goto err;
	}

	struct mp_element *sink =
		mp_element_factory_create(MP_ZAUD_I2S_CODEC_SINK_ELEM, I2S_SINK_ID);
	if (sink == NULL) {
		LOG_ERR("Failed to create speaker element");
		goto err;
	}

	/* Set element's properties */
	ret = mp_object_set_properties(MP_OBJECT(source), PROP_ZAUD_SRC_SLAB_PTR, &mem_slab,
				       PROP_LIST_END);

	if (ret < 0) {
		LOG_ERR("Failed to set properties for dmic element");
		goto err;
	}

	uint32_t frame_interval = 10000; /* 10ms */
	struct mp_caps *filtered_caps = mp_caps_new(MP_MEDIA_AUDIO_PCM, MP_CAPS_FRAME_INTERVAL,
						    MP_TYPE_UINT, frame_interval, MP_CAPS_END);

	if (filtered_caps == NULL) {
		LOG_ERR("Failed to create a filtered caps");
		goto err;
	}

	ret = mp_object_set_properties(MP_OBJECT(caps_filter), PROP_CAPS, filtered_caps,
				       PROP_LIST_END);
	mp_caps_unref(filtered_caps);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for caps filter element");
		goto err;
	}

	ret = mp_object_set_properties(MP_OBJECT(transform), PROP_GAIN, &gain, PROP_LIST_END);

	if (ret < 0) {
		LOG_ERR("Failed to set properties for transform element");
		goto err;
	}

	ret = mp_object_set_properties(MP_OBJECT(sink), PROP_ZAUD_SINK_SLAB_PTR, &mem_slab,
				       PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for speaker element");
		goto err;
	}

	/* Add elements to the pipeline - order does not matter */
	if (mp_bin_add(MP_BIN(pipeline), source, caps_filter, transform, sink, NULL) == false) {
		LOG_ERR("Failed to add elements");
		goto err;
	}

	/* Link elements together - order does matter */
	if (mp_element_link(source, caps_filter, transform, sink, NULL) == false) {
		LOG_ERR("Failed to link elements");
		goto err;
	}

	/* Start playing */
	if (mp_element_set_state(pipeline, MP_STATE_PLAYING) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err;
	}

	/* Handle message from the pipeline */
	struct mp_bus *bus = mp_element_get_bus(pipeline);
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
