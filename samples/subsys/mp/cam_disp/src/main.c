/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/video.h>
#include <zephyr/drivers/video-controls.h>
#include <zephyr/logging/log.h>
#include <zephyr/mp/mp.h>
#include <zephyr/mp/base/mp_capsfilter.h>
#include <zephyr/mp/disp/mp_disp_sink.h>
#include <zephyr/mp/vid/mp_vid_src.h>
#include <zephyr/mp/vid/mp_vid_property.h>
#include <zephyr/mp/utils/mp_player.h>
#if DT_HAS_CHOSEN(zephyr_jpegdec) || DT_HAS_CHOSEN(zephyr_videotrans)
#include <zephyr/mp/vid/mp_vid_transform.h>
#endif
#if DT_HAS_CHOSEN(zephyr_jpegdec)
#include <zephyr/mp/vid/mp_vid_convert.h>
#endif
#if defined(CONFIG_MP_BASE_CAPSFILTER)
#include <zephyr/mp/base/mp_capsfilter.h>
#endif
#include <zephyr/sys/util_macro.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	VID_SRC_ID,
	CAPS_FILTER_ID,
	JPEG_DEC_ID,
	VID_CONV_ID,
	VID_TRANS_ID,
	DISP_SINK_ID,
};

static struct mp_pipeline pipe;
static struct mp_vid_src vid_src;
static struct mp_disp_sink disp_sink;
#if defined(CONFIG_MP_BASE_CAPSFILTER)
static struct mp_caps_filter caps_filter;
#endif
#if (DT_HAS_CHOSEN(zephyr_jpegdec))
static struct mp_vid_transform jpeg_dec;
static struct mp_vid_convert vid_conv;
#endif
#if (DT_HAS_CHOSEN(zephyr_videotrans))
static struct mp_vid_transform vid_trans;
#endif
static struct mp_player player;

int main(void)
{
	int ret;

	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&vid_src, mp_vid_src_init, VID_SRC_ID);
	MP_ELEMENT_INIT(&disp_sink, mp_disp_sink_init, DISP_SINK_ID);

	struct video_rect __maybe_unused crop = {
		CONFIG_VIDEO_SOURCE_CROP_LEFT, CONFIG_VIDEO_SOURCE_CROP_TOP,
		CONFIG_VIDEO_SOURCE_CROP_WIDTH, CONFIG_VIDEO_SOURCE_CROP_HEIGHT};

	/* clang-format off */
	ret = mp_object_set_properties((struct mp_object *)&vid_src,
		COND_CODE_0(CONFIG_PROP_NUM_BUFS, (), (MP_PROP_SRC_NUM_BUFS, CONFIG_PROP_NUM_BUFS,))
		COND_CODE_0(CONFIG_VIDEO_SOURCE_CROP_WIDTH, (), (MP_PROP_VID_CROP, &crop,))
		IF_ENABLED(CONFIG_VIDEO_CTRL_HFLIP, (VIDEO_CID_HFLIP, CONFIG_VIDEO_CTRL_HFLIP,))
		IF_ENABLED(CONFIG_VIDEO_CTRL_VFLIP, (VIDEO_CID_VFLIP, CONFIG_VIDEO_CTRL_VFLIP,))
		MP_PROP_LIST_END);
	/* clang-format on */
	if (ret < 0) {
		goto err;
	}

	/* Caps filter element */
#if defined(CONFIG_MP_BASE_CAPSFILTER)
	MP_ELEMENT_INIT(&caps_filter, mp_caps_filter_init, CAPS_FILTER_ID);

	/* clang-format off */
	struct mp_caps *caps = mp_caps_new(MP_MEDIA_VIDEO,
		COND_CODE_0(CONFIG_VIDEO_FRAME_WIDTH,
			(), (MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT, CONFIG_VIDEO_FRAME_WIDTH,))
		COND_CODE_0(CONFIG_VIDEO_FRAME_HEIGHT,
			(), (MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT, CONFIG_VIDEO_FRAME_HEIGHT,))
		COND_CODE_0(CONFIG_VIDEO_FRAME_RATE, (),
			(MP_CAPS_FRAME_RATE, MP_TYPE_UINT_FRACTION,
			 CONFIG_VIDEO_FRAME_RATE, 1,))
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

	ret = mp_object_set_properties((struct mp_object *)&caps_filter, MP_PROP_BASE_CAPSFILTER_CAPS, caps,
				       MP_PROP_LIST_END);
	mp_caps_unref(caps);
	if (ret < 0) {
		goto err;
	}
#endif

	/* JPEG decoder element */
#if (DT_HAS_CHOSEN(zephyr_jpegdec))
	MP_ELEMENT_INIT(&jpeg_dec, mp_vid_transform_init, JPEG_DEC_ID);
	MP_ELEMENT_INIT(&vid_conv, mp_vid_convert_init, VID_CONV_ID);

	ret = mp_object_set_properties((struct mp_object *)&jpeg_dec, MP_PROP_VID_DEVICE,
				       DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_jpegdec)),
				       MP_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#endif

	/* Video transform element */
#if (DT_HAS_CHOSEN(zephyr_videotrans))
	MP_ELEMENT_INIT(&vid_trans, mp_vid_transform_init, VID_TRANS_ID);

	/* clang-format off */
	ret = mp_object_set_properties((struct mp_object *)&vid_trans,
		COND_CODE_0(CONFIG_VIDEO_ROTATION_ANGLE,
			(), (VIDEO_CID_ROTATE, CONFIG_VIDEO_ROTATION_ANGLE,)) MP_PROP_LIST_END);
	/* clang-format on */
	if (ret < 0) {
		goto err;
	}
#endif

	/* clang-format off */
	/* Add elements to the pipeline - order does not matter */
	ret = mp_bin_add((struct mp_bin *)&pipe,
			(struct mp_element *)&vid_src,
			IF_ENABLED(CONFIG_MP_BASE_CAPSFILTER, ((struct mp_element *)&caps_filter,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_jpegdec), ((struct mp_element *)&jpeg_dec,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_jpegdec), ((struct mp_element *)&vid_conv,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans), ((struct mp_element *)&vid_trans,))
			(struct mp_element *)&disp_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}
	/* Link elements together - order does matter */
	ret = mp_element_link((struct mp_element *)&vid_src,
			IF_ENABLED(CONFIG_MP_BASE_CAPSFILTER, ((struct mp_element *)&caps_filter,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_jpegdec), ((struct mp_element *)&jpeg_dec,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_jpegdec), ((struct mp_element *)&vid_conv,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans), ((struct mp_element *)&vid_trans,))
			(struct mp_element *)&disp_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}
	/* clang-format on */

	LOG_INF("Pipeline linked.");

	ret = mp_player_init(&player, &pipe);
	if (ret != 0) {
		LOG_ERR("Failed to init player (%d)", ret);
		goto err;
	}

	(void)mp_player_play(&player);
	(void)mp_player_wait_quit(&player);
	(void)mp_player_deinit(&player);

	LOG_INF("Done.");

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
