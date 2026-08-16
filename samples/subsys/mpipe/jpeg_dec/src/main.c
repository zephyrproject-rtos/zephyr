/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#include <ff.h>
#endif

#include <zephyr/drivers/video.h>
#include <zephyr/video/controls.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/base/mpipe_caps_filter.h>
#include <zephyr/mpipe/disp/mpipe_disp_sink.h>
#include <zephyr/mpipe/fs/mpipe_file_src.h>
#include <zephyr/mpipe/img/mpipe_img_jpeg_decoder.h>
#include <zephyr/mpipe/img/mpipe_img_jpeg_parser.h>
#include <zephyr/mpipe/utils/mpipe_player.h>
#if DT_HAS_CHOSEN(zephyr_jpegdec) || DT_HAS_CHOSEN(zephyr_videotrans)
#include <zephyr/mpipe/vid/mpipe_vid_transform.h>
#endif
#if DT_HAS_CHOSEN(zephyr_jpegdec)
#include <zephyr/mpipe/vid/mpipe_vid_convert.h>
#endif

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	FILE_SRC_ID,
	JPEG_PARSER_ID,
	CAPS_FILTER_ID,
	JPEG_DEC_ID,
	VID_CONV_ID,
	VID_TRANS_ID,
	DISP_SINK_ID,
};

#define MNT_POINT "/SD:"

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
static FATFS fat_fs;
#endif

static struct fs_mount_t mp = {
	.type = FS_FATFS,
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
	.fs_data = &fat_fs,
#endif
	.mnt_point = MNT_POINT,
};

static const uint8_t mjpeg[] = {
#include "mjpeg.inc"
};
static const size_t mjpeg_sz = sizeof(mjpeg);

static int embed_test_file(void)
{
	struct fs_dirent ent;
	int ret = fs_stat(CONFIG_FILE_INPUT_PATH, &ent);

	if (ret == 0) {
		return 0;
	}

	if (ret != -ENOENT) {
		LOG_ERR("fs_stat(%s) failed (%d)", CONFIG_FILE_INPUT_PATH, ret);
		return ret;
	}

	if (mjpeg_sz == 0) {
		LOG_ERR("MJPEG test file not available");
		return -ENOENT;
	}

	struct fs_file_t f;

	fs_file_t_init(&f);

	ret = fs_open(&f, CONFIG_FILE_INPUT_PATH, FS_O_CREATE | FS_O_WRITE);
	if (ret != 0) {
		LOG_ERR("fs_open(%s) failed (%d)", CONFIG_FILE_INPUT_PATH, ret);
		return ret;
	}

	ssize_t w = fs_write(&f, mjpeg, mjpeg_sz);

	if (w < 0 || (size_t)w != mjpeg_sz) {
		LOG_ERR("fs_write failed (%d)", (int)w);
		(void)fs_close(&f);
		return (w < 0) ? (int)w : -EIO;
	}

	(void)fs_close(&f);

	LOG_INF("Created %s file for test (%u bytes)", CONFIG_FILE_INPUT_PATH,
		(unsigned int)mjpeg_sz);

	return 0;
}

static int mount_sd(void)
{
	int ret;

	ret = fs_mount(&mp);
	if (ret != 0) {
		LOG_ERR("fs_mount failed (%d)", ret);
		return ret;
	}

	LOG_INF("SDCard mounted at %s", MNT_POINT);

	ret = embed_test_file();
	if (ret != 0) {
		return ret;
	}

	return 0;
}

static struct mpipe pipe;
static struct mpipe_file_src file_src;
static struct mpipe_img_jpeg_parser jpeg_parser;
static struct mpipe_caps_filter caps_filter;
static struct mpipe_disp_sink disp_sink;
#if DT_HAS_CHOSEN(zephyr_jpegdec)
static struct mpipe_vid_transform jpeg_dec;
static struct mpipe_vid_convert vid_conv;
#else
static struct mpipe_img_jpeg_decoder jpeg_dec;
#endif
#if DT_HAS_CHOSEN(zephyr_videotrans)
static struct mpipe_vid_transform vid_trans;
#endif
static struct mpipe_player player;

int main(void)
{
	int ret = mount_sd();

	if (ret != 0) {
		goto err;
	}

	ret = mpipe_pipeline_init(&pipe, PIPE_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_file_src_init(&file_src, FILE_SRC_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_img_jpeg_parser_init(&jpeg_parser, JPEG_PARSER_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_caps_filter_init(&caps_filter, CAPS_FILTER_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_disp_sink_init(&disp_sink, DISP_SINK_ID);
	if (ret < 0) {
		goto err;
	}

#if DT_HAS_CHOSEN(zephyr_jpegdec)
	ret = mpipe_vid_transform_init(&jpeg_dec, JPEG_DEC_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_vid_convert_init(&vid_conv, VID_CONV_ID);
	if (ret < 0) {
		goto err;
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&jpeg_dec, MPIPE_PROP_VID_DEVICE,
					  DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_jpegdec)),
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#else
	ret = mpipe_img_jpeg_decoder_init(&jpeg_dec, JPEG_DEC_ID);
	if (ret < 0) {
		goto err;
	}
#endif
#if DT_HAS_CHOSEN(zephyr_videotrans)
	ret = mpipe_vid_transform_init(&vid_trans, VID_TRANS_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_object_set_properties(
		(struct mpipe_object *)&vid_trans,
		COND_CODE_0(CONFIG_VIDEO_ROTATION_ANGLE,
			(), (VIDEO_CID_ROTATE, CONFIG_VIDEO_ROTATION_ANGLE,))
								   MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#endif

	ret = mpipe_object_set_properties((struct mpipe_object *)&file_src, MPIPE_PROP_FS_SRC_PATH,
					  CONFIG_FILE_INPUT_PATH, MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	{
		struct mpipe_structure caps;

		ret = mpipe_structure_init_fields(
			&caps, MPIPE_MEDIA_VIDEO, MPIPE_CAPS_PIXEL_FORMAT, MPIPE_TYPE_UINT,
			VIDEO_PIX_FMT_JPEG, MPIPE_CAPS_IMAGE_WIDTH, MPIPE_TYPE_UINT,
			CONFIG_JPEG_IMAGE_WIDTH, MPIPE_CAPS_IMAGE_HEIGHT, MPIPE_TYPE_UINT,
			CONFIG_JPEG_IMAGE_HEIGHT, MPIPE_CAPS_END);

		if (ret != 0) {
			goto err;
		}

		ret = mpipe_object_set_properties((struct mpipe_object *)&caps_filter,
						  MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
						  MPIPE_PROP_LIST_END);
		if (ret < 0) {
			goto err;
		}
	}

	/* clang-format off */
	ret = mpipe_bin_add((struct mpipe_bin *)&pipe,
			(struct mpipe_element *)&file_src,
			(struct mpipe_element *)&jpeg_parser,
			(struct mpipe_element *)&caps_filter,
			(struct mpipe_element *)&jpeg_dec,
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_jpegdec),
				   ((struct mpipe_element *)&vid_conv,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans),
				   ((struct mpipe_element *)&vid_trans,))
			(struct mpipe_element *)&disp_sink,
			NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	ret = mpipe_element_link((struct mpipe_element *)&file_src,
			(struct mpipe_element *)&jpeg_parser,
			(struct mpipe_element *)&caps_filter,
			(struct mpipe_element *)&jpeg_dec,
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_jpegdec),
				   ((struct mpipe_element *)&vid_conv,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans),
				   ((struct mpipe_element *)&vid_trans,))
			(struct mpipe_element *)&disp_sink,
			NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}
	/* clang-format on */

	LOG_INF("Pipeline linked.");

	ret = mpipe_player_init(&player, &pipe);
	if (ret != 0) {
		LOG_ERR("Failed to init player (%d)", ret);
		goto err;
	}

	(void)mpipe_player_play(&player);
	(void)mpipe_player_wait_quit(&player);
	(void)mpipe_player_deinit(&player);

	ret = fs_unmount(&mp);
	if (ret != 0) {
		LOG_ERR("fs_unmount failed (%d)", ret);
	}

	LOG_INF("Done.");

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
