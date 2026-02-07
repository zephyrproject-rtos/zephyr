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

#define PIPE_ID        0
#define CAM_SRC_ID     1
#define CAPS_FILTER_ID 2
#define VID_TRANS_ID   3
#define DISP_SINK_ID   4

int main(void)
{
	int ret;
	struct mp_pipeline pipe = {0};
	struct mp_zvid_src source = {0};
	struct mp_zdisp_sink sink = {0};

	MP_ELEMENT_INIT(&pipe, pipeline, PIPE_ID);
	MP_ELEMENT_INIT(&source, zvid_src, CAM_SRC_ID);
	MP_ELEMENT_INIT(&sink, zdisp_sink, DISP_SINK_ID);

	struct video_rect __maybe_unused crop = {
		CONFIG_VIDEO_SOURCE_CROP_LEFT, CONFIG_VIDEO_SOURCE_CROP_TOP,
		CONFIG_VIDEO_SOURCE_CROP_WIDTH, CONFIG_VIDEO_SOURCE_CROP_HEIGHT};

	/* clang-format off */
	ret = mp_object_set_properties(MP_OBJECT(&source),
		COND_CODE_0(CONFIG_PROP_NUM_BUFS, (), (PROP_NUM_BUFS, CONFIG_PROP_NUM_BUFS,))
		COND_CODE_0(CONFIG_VIDEO_SOURCE_CROP_WIDTH, (), (PROP_ZVID_CROP, &crop,))
		IF_ENABLED(CONFIG_VIDEO_CTRL_HFLIP, (VIDEO_CID_HFLIP, CONFIG_VIDEO_CTRL_HFLIP,))
		IF_ENABLED(CONFIG_VIDEO_CTRL_VFLIP, (VIDEO_CID_VFLIP, CONFIG_VIDEO_CTRL_VFLIP,))
		PROP_LIST_END);
	/* clang-format on */
	if (ret < 0) {
		goto err;
	}

	/* Caps filter element */
#if (CONFIG_MP_CAPSFILTER)
	struct mp_caps_filter caps_filter = {0};

	MP_ELEMENT_INIT(&caps_filter, caps_filter, CAPS_FILTER_ID);

	/* clang-format off */
	struct mp_caps *caps = mp_caps_new(MP_MEDIA_VIDEO,
		COND_CODE_0(CONFIG_VIDEO_FRAME_WIDTH,
			(), (MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT, CONFIG_VIDEO_FRAME_WIDTH,))
		COND_CODE_0(CONFIG_VIDEO_FRAME_HEIGHT,
			(), (MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT, CONFIG_VIDEO_FRAME_HEIGHT,))
		MP_CAPS_END);
	/* clang-format on */

	if (caps == NULL) {
		goto err;
	}

	if (strcmp(CONFIG_VIDEO_PIXEL_FORMAT, "") != 0) {
		mp_structure_append(mp_caps_get_structure(caps, 0), MP_CAPS_PIXEL_FORMAT,
				    mp_value_new(MP_TYPE_UINT,
						 VIDEO_FOURCC_FROM_STR(CONFIG_VIDEO_PIXEL_FORMAT)));
	}

	ret = mp_object_set_properties(MP_OBJECT(&caps_filter), PROP_CAPS, caps, PROP_LIST_END);
	mp_caps_unref(caps);
	if (ret < 0) {
		goto err;
	}
#endif

	/* Video transform element */
#if (DT_HAS_CHOSEN(zephyr_videotrans))
	struct mp_zvid_transform transform = {0};

	MP_ELEMENT_INIT(&transform, zvid_transform, VID_TRANS_ID);

	/* clang-format off */
	ret = mp_object_set_properties(MP_OBJECT(&transform),
		COND_CODE_0(CONFIG_VIDEO_ROTATION_ANGLE,
			(), (VIDEO_CID_ROTATE, CONFIG_VIDEO_ROTATION_ANGLE,)) PROP_LIST_END);
	/* clang-format on */
	if (ret < 0) {
		goto err;
	}
#endif

	/* clang-format off */
	/* Add elements to the pipeline - order does not matter */
	if (!mp_bin_add(MP_BIN(&pipe),
			MP_ELEMENT(&source),
			IF_ENABLED(CONFIG_MP_CAPSFILTER, (MP_ELEMENT(&caps_filter),))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans), (MP_ELEMENT(&transform),))
			MP_ELEMENT(&sink), NULL)) {
		LOG_ERR("Failed to add elements");
		goto err;
	}
	/* Link elements together - order does matter */
	if (!mp_element_link(MP_ELEMENT(&source),
			IF_ENABLED(CONFIG_MP_CAPSFILTER, (MP_ELEMENT(&caps_filter),))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans), (MP_ELEMENT(&transform),))
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
