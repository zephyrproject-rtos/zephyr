/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video-controls.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <mp.h>

LOG_MODULE_REGISTER(main);

#define LOG_LEVEL LOG_LEVEL_DBG

int main()
{
	int ret;

	/* Initialize built-in elements and plugins */
	mp_init();

	/* Create elements */
	MpElement *source = mp_element_factory_create("zvid_src", "camsrc");
	if (source == NULL) {
		LOG_ERR("Failed to create camsrc element");
		return 0;
	}

	MpElement *transform = mp_element_factory_create("zvid_transform", "vtransform");
	if (transform == NULL) {
		LOG_ERR("Failed to create vtransform element");
		return 0;
	}

	MpElement *sink = mp_element_factory_create("zdisp_sink", "dispsink");
	if (sink == NULL) {
		LOG_ERR("Failed to create dispsink element");
		return 0;
	}

	/* Set elements' properties */
	ret = mp_object_set_properties(MP_OBJECT(source), PROP_NUM_BUFS, 3, VIDEO_CID_HFLIP, 1,
				       PROP_LIST_END);
	if (ret < 0) {
		return ret;
	}

	static const struct device *const pxp_dev = DEVICE_DT_GET(DT_NODELABEL(pxp));

	if (!device_is_ready(pxp_dev)) {
		LOG_ERR("%s: pxp device is not ready", pxp_dev->name);
		return -ENODEV;
	}

	ret = mp_object_set_properties(MP_OBJECT(transform), PROP_DEVICE, pxp_dev, VIDEO_CID_ROTATE,
				       90, PROP_LIST_END);
	if (ret < 0) {
		return ret;
	}

	/* Create a new pipeline */
	MpElement *pipeline = mp_pipeline_new("cam_transform_disp");
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
			LOG_INF("Received ERROR from %s", msg->src->name);
			break;
		case MP_MESSAGE_EOS:
			LOG_INF("Received EOS from %s", msg->src->name);
			break;
		default:
			LOG_ERR("Unexpected message received from %s", msg->src->name);
			break;
		}
	}

	/* TODO: Stop pipeline and free allocated ressources */

	return 0;
}
