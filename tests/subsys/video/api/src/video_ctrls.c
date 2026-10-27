/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/video.h>
#include <zephyr/video/controls.h>
#include <zephyr/video/video.h>
#include <zephyr/ztest.h>

#include "video_common.h"

struct test_codec_ctrls {
	struct video_ctrl gop_size;
	struct video_ctrl bitrate_mode;
	struct video_ctrl bitrate;
	struct video_ctrl frame_rc_enable;
	struct video_ctrl header_mode;
	struct video_ctrl mb_rc_enable;
	struct video_ctrl force_key_frame;
	struct video_ctrl i_frame_qp;
	struct video_ctrl p_frame_qp;
	struct video_ctrl min_qp;
	struct video_ctrl max_qp;
	struct video_ctrl h264_level;
	struct video_ctrl h264_profile;
	struct video_ctrl prepend_spspps;
	struct video_ctrl constant_quality;
};

struct test_ctrl_data {
	struct test_codec_ctrls ctrls;
	int force_key_frame_count;
	uint32_t reject_id;
};

static struct test_ctrl_data test_ctrl_data_0 = {
	.ctrls.gop_size = {.id = VIDEO_CID_CODEC_GOP_SIZE,
			   .range = {.min = 0, .max = 300, .step = 1, .def = 30}},
	.ctrls.bitrate_mode = {.id = VIDEO_CID_CODEC_BITRATE_MODE,
			       .range = {.def = VIDEO_CODEC_BITRATE_MODE_VBR}},
	.ctrls.bitrate = {.id = VIDEO_CID_CODEC_BITRATE,
			  .range = {.min = 10000, .max = 10000000, .step = 1, .def = 1000000}},
	.ctrls.frame_rc_enable = {.id = VIDEO_CID_CODEC_FRAME_RC_ENABLE, .range = {.def = 1}},
	.ctrls.header_mode = {.id = VIDEO_CID_CODEC_HEADER_MODE,
			      .range = {.def = VIDEO_CODEC_HEADER_MODE_JOINED_WITH_1ST_FRAME}},
	.ctrls.mb_rc_enable = {.id = VIDEO_CID_CODEC_MB_RC_ENABLE, .range = {.def = 0}},
	.ctrls.force_key_frame = {.id = VIDEO_CID_CODEC_FORCE_KEY_FRAME},
	.ctrls.i_frame_qp = {.id = VIDEO_CID_H264_I_FRAME_QP,
			     .range = {.min = 0, .max = 51, .step = 1, .def = 26}},
	.ctrls.p_frame_qp = {.id = VIDEO_CID_H264_P_FRAME_QP,
			     .range = {.min = 0, .max = 51, .step = 1, .def = 26}},
	.ctrls.min_qp = {.id = VIDEO_CID_H264_MIN_QP,
			 .range = {.min = 0, .max = 51, .step = 1, .def = 10}},
	.ctrls.max_qp = {.id = VIDEO_CID_H264_MAX_QP,
			 .range = {.min = 0, .max = 51, .step = 1, .def = 51}},
	.ctrls.h264_level = {.id = VIDEO_CID_H264_LEVEL, .range = {.def = VIDEO_H264_LEVEL_4_0}},
	.ctrls.h264_profile = {.id = VIDEO_CID_H264_PROFILE,
			       .range = {.def = VIDEO_H264_PROFILE_CONSTRAINED_BASELINE}},
	.ctrls.prepend_spspps = {.id = VIDEO_CID_CODEC_PREPEND_SPSPPS_TO_IDR, .range = {.def = 0}},
	.ctrls.constant_quality = {.id = VIDEO_CID_CODEC_CONSTANT_QUALITY,
				   .range = {.min = 1, .max = 100, .step = 1, .def = 75}},
};

static int test_ctrl_set_ctrl(const struct device *dev, uint32_t id)
{
	struct test_ctrl_data *data = dev->data;

	if (id == data->reject_id) {
		return -EINVAL;
	}

	if (id == VIDEO_CID_CODEC_FORCE_KEY_FRAME) {
		data->force_key_frame_count++;
	}

	return 0;
}

static DEVICE_API(video, test_ctrl_api) = {
	.set_ctrl = test_ctrl_set_ctrl,
};

static int test_ctrl_init(const struct device *dev)
{
	struct test_ctrl_data *data = dev->data;

	return video_register_ctrls((struct video_ctrl *)&data->ctrls,
				    sizeof(data->ctrls) / sizeof(struct video_ctrl), dev);
}

DEVICE_DEFINE(test_video_ctrls, "test_video_ctrls", test_ctrl_init, NULL, &test_ctrl_data_0, NULL,
	      POST_KERNEL, CONFIG_VIDEO_INIT_PRIORITY, &test_ctrl_api);

VIDEO_DEVICE_DEFINE(test_video_ctrls_vdev, DEVICE_GET(test_video_ctrls), NULL);

DEVICE_DEFINE(test_video_menus, "test_video_menus", NULL, NULL, NULL, NULL, POST_KERNEL,
	      CONFIG_VIDEO_INIT_PRIORITY, &test_ctrl_api);

VIDEO_DEVICE_DEFINE(test_video_menus_vdev, DEVICE_GET(test_video_menus), NULL);

ZTEST(video_common, test_video_codec_ctrls_metadata)
{
	const struct device *dev = DEVICE_GET(test_video_ctrls);
	static const struct {
		uint32_t id;
		enum video_ctrl_type type;
		uint32_t flags;
		const char *name;
		int32_t max;
	} expected[] = {
		{VIDEO_CID_CODEC_GOP_SIZE, VIDEO_CTRL_TYPE_INTEGER, 0, "Video GOP Size", 300},
		{VIDEO_CID_CODEC_BITRATE_MODE, VIDEO_CTRL_TYPE_MENU, 0, "Video Bitrate Mode",
		 VIDEO_CODEC_BITRATE_MODE_CQ},
		{VIDEO_CID_CODEC_BITRATE, VIDEO_CTRL_TYPE_INTEGER, 0, "Video Bitrate", 10000000},
		{VIDEO_CID_CODEC_FRAME_RC_ENABLE, VIDEO_CTRL_TYPE_BOOLEAN, 0,
		 "Frame Level Rate Control Enable", 1},
		{VIDEO_CID_CODEC_HEADER_MODE, VIDEO_CTRL_TYPE_MENU, 0, "Sequence Header Mode",
		 VIDEO_CODEC_HEADER_MODE_JOINED_WITH_1ST_FRAME},
		{VIDEO_CID_CODEC_MB_RC_ENABLE, VIDEO_CTRL_TYPE_BOOLEAN, 0,
		 "MB Level Rate Control Enable", 1},
		{VIDEO_CID_CODEC_FORCE_KEY_FRAME, VIDEO_CTRL_TYPE_BUTTON,
		 VIDEO_CTRL_FLAG_WRITE_ONLY | VIDEO_CTRL_FLAG_EXECUTE_ON_WRITE, "Force Key Frame",
		 0},
		{VIDEO_CID_H264_I_FRAME_QP, VIDEO_CTRL_TYPE_INTEGER, 0, "H264 I-Frame QP Value",
		 51},
		{VIDEO_CID_H264_P_FRAME_QP, VIDEO_CTRL_TYPE_INTEGER, 0, "H264 P-Frame QP Value",
		 51},
		{VIDEO_CID_H264_MIN_QP, VIDEO_CTRL_TYPE_INTEGER, 0, "H264 Minimum QP Value", 51},
		{VIDEO_CID_H264_MAX_QP, VIDEO_CTRL_TYPE_INTEGER, 0, "H264 Maximum QP Value", 51},
		{VIDEO_CID_H264_LEVEL, VIDEO_CTRL_TYPE_MENU, 0, "H264 Level", VIDEO_H264_LEVEL_6_2},
		{VIDEO_CID_H264_PROFILE, VIDEO_CTRL_TYPE_MENU, 0, "H264 Profile",
		 VIDEO_H264_PROFILE_CONSTRAINED_HIGH},
		{VIDEO_CID_CODEC_PREPEND_SPSPPS_TO_IDR, VIDEO_CTRL_TYPE_BOOLEAN, 0,
		 "Prepend SPS and PPS to IDR", 1},
		{VIDEO_CID_CODEC_CONSTANT_QUALITY, VIDEO_CTRL_TYPE_INTEGER, 0, "Constant Quality",
		 100},
	};
	struct video_ctrl_query cq = {.dev = dev, .id = VIDEO_CTRL_FLAG_NEXT_CTRL};
	size_t idx = 0;

	zassert_true(device_is_ready(dev));

	while (video_query_ctrl(&cq) == 0) {
		zassert_true(idx < ARRAY_SIZE(expected), "unexpected extra control 0x%08x", cq.id);
		zassert_equal(cq.id, expected[idx].id);
		zassert_equal(cq.type, expected[idx].type);
		zassert_equal(cq.flags, expected[idx].flags);
		zassert_not_null(cq.name);
		zassert_equal(strcmp(cq.name, expected[idx].name), 0);
		zassert_equal(cq.range.max, expected[idx].max);
		if (cq.type == VIDEO_CTRL_TYPE_MENU) {
			zassert_not_null(cq.menu);
			zassert_not_null(cq.menu[cq.range.max]);
			zassert_is_null(cq.menu[cq.range.max + 1]);
		}
		video_print_ctrl(&cq);
		cq.id |= VIDEO_CTRL_FLAG_NEXT_CTRL;
		idx++;
	}

	zassert_equal(idx, ARRAY_SIZE(expected));
}

ZTEST(video_common, test_video_ctrl_button_and_rollback)
{
	const struct device *dev = DEVICE_GET(test_video_ctrls);
	struct test_ctrl_data *data = dev->data;
	struct video_control ctrl = {.id = VIDEO_CID_CODEC_FORCE_KEY_FRAME, .val = 0};

	data->force_key_frame_count = 0;
	data->reject_id = 0;

	/* Buttons are write-only and execute on every write regardless of value */
	zassert_equal(video_get_ctrl(dev, &ctrl), -EACCES);
	zassert_ok(video_set_ctrl(dev, &ctrl));
	zassert_ok(video_set_ctrl(dev, &ctrl));
	ctrl.val = 99;
	zassert_ok(video_set_ctrl(dev, &ctrl));
	zassert_equal(data->force_key_frame_count, 3);

	/* Out-of-range integer/menu values fail with -ERANGE; failed set_ctrl rolls back */
	ctrl.id = VIDEO_CID_CODEC_GOP_SIZE;
	ctrl.val = 301;
	zassert_equal(video_set_ctrl(dev, &ctrl), -ERANGE);

	ctrl.val = 60;
	zassert_ok(video_set_ctrl(dev, &ctrl));
	data->reject_id = VIDEO_CID_CODEC_GOP_SIZE;
	ctrl.val = 120;
	zassert_equal(video_set_ctrl(dev, &ctrl), -EINVAL);
	data->reject_id = 0;
	zassert_ok(video_get_ctrl(dev, &ctrl));
	zassert_equal(ctrl.val, 60);
}

ZTEST(video_common, test_video_ctrl_menu_variants)
{
	const struct device *dev = DEVICE_GET(test_video_menus);
	static const char *const single_menu[] = {"Separate Buffer", NULL};
	struct video_ctrl narrowed_std = {
		.id = VIDEO_CID_CODEC_BITRATE_MODE,
		.range = {.max = VIDEO_CODEC_BITRATE_MODE_CBR, .def = VIDEO_CODEC_BITRATE_MODE_VBR},
	};
	struct video_ctrl oversized_std = {
		.id = VIDEO_CID_CODEC_BITRATE_MODE,
		.range = {.max = 10, .def = VIDEO_CODEC_BITRATE_MODE_VBR},
	};
	struct video_ctrl single_item = {
		.id = VIDEO_CID_CODEC_HEADER_MODE,
		.menu = single_menu,
	};
	struct video_ctrl unnamed_priv = {
		.id = VIDEO_CID_PRIVATE_BASE,
		.type = VIDEO_CTRL_TYPE_MENU,
		.name = "Private Menu",
		.range = {.max = 2, .def = 1},
	};
	struct video_ctrl_query cq = {.dev = dev};
	struct video_control vc;

	sys_dlist_init(&test_video_menus_vdev.ctrls);

	/* Standard menu narrowed by driver still populates standard menu strings */
	zassert_ok(video_register_ctrl(&narrowed_std, dev));
	cq.id = VIDEO_CID_CODEC_BITRATE_MODE;
	zassert_ok(video_query_ctrl(&cq));
	zassert_equal(cq.range.max, VIDEO_CODEC_BITRATE_MODE_CBR);
	zassert_not_null(cq.menu);
	zassert_equal(strcmp(cq.menu[0], "Variable Bitrate"), 0);
	zassert_equal(strcmp(cq.menu[1], "Constant Bitrate"), 0);
	vc.id = VIDEO_CID_CODEC_BITRATE_MODE;
	vc.val = VIDEO_CODEC_BITRATE_MODE_CQ;
	zassert_equal(video_set_ctrl(dev, &vc), -ERANGE);

	/* Standard menu with range.max beyond standard entries is rejected */
	zassert_equal(video_register_ctrl(&oversized_std, dev), -ERANGE);

	/* Single-item menu (range.max == 0) via explicit 1-entry menu array */
	zassert_ok(video_register_ctrl(&single_item, dev));
	cq.id = VIDEO_CID_CODEC_HEADER_MODE;
	zassert_ok(video_query_ctrl(&cq));
	zassert_equal(cq.range.max, 0);
	zassert_not_null(cq.menu);
	zassert_equal(strcmp(cq.menu[0], "Separate Buffer"), 0);

	/* Private menu sized by driver without menu strings remains allowed */
	zassert_ok(video_register_ctrl(&unnamed_priv, dev));
	cq.id = VIDEO_CID_PRIVATE_BASE;
	zassert_ok(video_query_ctrl(&cq));
	zassert_equal(cq.range.max, 2);
	zassert_is_null(cq.menu);
	zassert_equal(strcmp(cq.name, "Private Menu"), 0);

	sys_dlist_init(&test_video_menus_vdev.ctrls);
}
