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

int main()
{
	int gain = 90; /* Set gain to 90% (0.9x amplification) */
	int ret = 0;

	/* Initialize built-in elements and plugins */
	mp_init();

	/* Create elements */
	MpElement *source = mp_element_factory_create("zaud_dmic_src", "dmic");
	if (source == NULL) {
		LOG_ERR("Failed to create dmic element");
		return 0;
	}

	MpElement *transform = mp_element_factory_create("zaud_gain", "gain");
	if (transform == NULL) {
		LOG_ERR("Failed to create gain element");
		return 0;
	}

	MpElement *sink = mp_element_factory_create("zaud_i2s_codec_sink", "speaker");
	if (sink == NULL) {
		LOG_ERR("Failed to create speaker element");
		return 0;
	}

	ret = mp_object_set_properties(MP_OBJECT(source), PROP_ZAUD_SRC_SLAB_PTR, &mem_slab,
				       PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for dmic element");
		return 0;
	}

	ret = mp_object_set_properties(MP_OBJECT(transform), PROP_GAIN, &gain, PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for transform element");
		return 0;
	}

	ret = mp_object_set_properties(MP_OBJECT(sink), PROP_ZAUD_SINK_SLAB_PTR, &mem_slab,
				       PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set properties for speaker element");
		return 0;
	}

	/* Create a new pipeline */
	MpElement *pipeline = mp_pipeline_new("dmic_gain_speaker_pipeline");
	if (pipeline == NULL) {
		LOG_ERR("Failed to create pipeline");
		return 0;
	}

	/* Add elements to the pipeline - order does not matter */
	if (mp_bin_add(MP_BIN(pipeline), source, transform, sink, NULL) == false) {
		LOG_ERR("Failed to add elements");
		return 0;
	}

	/* Link elements together - order does matter */
	if (mp_element_link(source, transform, sink, NULL) == false) {
		LOG_ERR("Failed to link elements");
		return 0;
	}

	/* Start playing */
	if (mp_element_set_state(pipeline, MP_STATE_PLAYING) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		return 0;
	}

	/* Handle message from the pipeline */
	MpBus *bus = mp_element_get_bus(pipeline);
	/* Wait until an Error or an EOS - blocking */
	MpMessage *msg = mp_bus_pop_msg(bus, MP_MESSAGE_ERROR | MP_MESSAGE_EOS);
	if (msg != NULL) {
		switch (MP_MESSAGE_TYPE(msg)) {
		case MP_MESSAGE_ERROR:
			LOG_INF("Received ERROR from %s\n", msg->src->name);
			break;
		case MP_MESSAGE_EOS:
			LOG_INF("Received EOS from %s\n", msg->src->name);
			break;
		default:
			LOG_ERR("Unexpected message received from %s\n", msg->src->name);
			break;
		}
	}

	/* TODO: Stop the pipeline */

	return 0;
}
