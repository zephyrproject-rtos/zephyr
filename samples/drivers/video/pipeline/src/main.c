/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util_macro.h>

#include <mp.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define PIPELINE_ID    0
#define CAM_SRC_ID     1
#define CAPS_FILTER_ID 2
#define VID_TRANS_ID   3
#define DISP_SINK_ID   4

int main(void)
{
	int ret;
	struct mp_element *pipeline, *source, *sink;
	__maybe_unused struct mp_element *caps_filter = NULL, *transform = NULL;

	/* Create a new pipeline */
	pipeline = mp_pipeline_new(PIPELINE_ID);
	if (pipeline == NULL) {
		goto err;
	}

	/* Camera source element */
	source = mp_element_factory_create(MP_ZVID_SRC_ELEM, CAM_SRC_ID);
	if (source == NULL) {
		goto err;
	}

	struct video_rect __maybe_unused crop = {
		CONFIG_VIDEO_SOURCE_CROP_LEFT, CONFIG_VIDEO_SOURCE_CROP_TOP,
		CONFIG_VIDEO_SOURCE_CROP_WIDTH, CONFIG_VIDEO_SOURCE_CROP_HEIGHT};

	ret = mp_object_set_properties(
		MP_OBJECT(source),
		COND_CODE_0(CONFIG_PROP_NUM_BUFS, (), (PROP_NUM_BUFS, CONFIG_PROP_NUM_BUFS,))
				 COND_CODE_0(CONFIG_VIDEO_SOURCE_CROP_WIDTH, (), (PROP_ZVID_CROP, &crop,))
						  IF_ENABLED(CONFIG_VIDEO_CTRL_HFLIP, (VIDEO_CID_HFLIP, CONFIG_VIDEO_CTRL_HFLIP,))
								  IF_ENABLED(CONFIG_VIDEO_CTRL_VFLIP, (VIDEO_CID_VFLIP, CONFIG_VIDEO_CTRL_VFLIP,))
										  PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	/* Caps filter element */
#if (CONFIG_MP_CAPSFILTER)
	caps_filter = mp_element_factory_create(MP_CAPS_FILTER_ELEM, CAPS_FILTER_ID);
	if (caps_filter == NULL) {
		goto err;
	}

	struct mp_caps *caps = mp_caps_new(
		MP_MEDIA_VIDEO,
		COND_CODE_0(CONFIG_VIDEO_FRAME_WIDTH, (),
		(MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT, CONFIG_VIDEO_FRAME_WIDTH,))
				       COND_CODE_0(CONFIG_VIDEO_FRAME_HEIGHT, (),
		(MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT, CONFIG_VIDEO_FRAME_HEIGHT,)) MP_CAPS_END);

	if (caps == NULL) {
		goto err;
	}

	if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "") != 0) {
		mp_structure_append(mp_caps_get_structure(caps, 0), MP_CAPS_PIXEL_FORMAT,
				    mp_value_new(MP_TYPE_UINT,
						 VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_PIXEL_FORMAT)));
	}

	ret = mp_object_set_properties(MP_OBJECT(caps_filter), PROP_CAPS, caps, PROP_LIST_END);
	mp_caps_unref(caps);
	if (ret < 0) {
		goto err;
	}
#endif

	/* Video transform element */
#if (DT_HAS_CHOSEN(zephyr_videotrans))
	transform = mp_element_factory_create(MP_ZVID_TRANSFORM_ELEM, VID_TRANS_ID);
	if (transform == NULL) {
		goto err;
	}

	ret = mp_object_set_properties(
		MP_OBJECT(transform),
		COND_CODE_0(CONFIG_VIDEO_ROTATION_ANGLE, (),
			    (VIDEO_CID_ROTATE, CONFIG_VIDEO_ROTATION_ANGLE,)) PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#endif

	/* Display sink element */
	sink = mp_element_factory_create(MP_ZDISP_SINK_ELEM, DISP_SINK_ID);
	if (sink == NULL) {
		goto err;
	}

	/* Add elements to the pipeline - order does not matter */
	if (!mp_bin_add(MP_BIN(pipeline), source,
			IF_ENABLED(CONFIG_MP_CAPSFILTER, (caps_filter,)) IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans), (transform,)) sink, NULL)) {
		LOG_ERR("Failed to add elements");
		goto err;
	}

	/* Link elements together - order does matter */
	if (!mp_element_link(source, IF_ENABLED(CONFIG_MP_CAPSFILTER, (caps_filter,))
							IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans), (transform,)) sink,
										 NULL)) {
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
