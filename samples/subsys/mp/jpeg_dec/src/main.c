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
#include <zephyr/drivers/video-controls.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/mp.h>
#include <zephyr/mp/base/mp_capsfilter.h>
#include <zephyr/mp/disp/mp_disp_sink.h>
#include <zephyr/mp/fs/mp_filesrc.h>
#include <zephyr/mp/img/mp_img_jpeg_decoder.h>
#include <zephyr/mp/img/mp_img_jpeg_parser.h>
#include <zephyr/mp/utils/mp_player.h>
#if DT_HAS_CHOSEN(zephyr_jpegdec) || DT_HAS_CHOSEN(zephyr_videotrans)
#include <zephyr/mp/vid/mp_vid_transform.h>
#endif
#if DT_HAS_CHOSEN(zephyr_jpegdec)
#include <zephyr/mp/vid/mp_vid_convert.h>
#include <zephyr/mp/vid/mp_vid_property.h>
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

static struct mp_pipeline pipe;
static struct mp_filesrc filesrc;
static struct mp_img_jpeg_parser jpeg_parser;
static struct mp_caps_filter caps_filter;
static struct mp_disp_sink disp_sink;
#if DT_HAS_CHOSEN(zephyr_jpegdec)
static struct mp_vid_transform jpeg_dec;
static struct mp_vid_convert vid_conv;
#else
static struct mp_img_jpeg_decoder jpeg_dec;
#endif
#if DT_HAS_CHOSEN(zephyr_videotrans)
static struct mp_vid_transform vid_trans;
#endif
static struct mp_player player;

int main(void)
{
	int ret = mount_sd();

	if (ret != 0) {
		goto err;
	}

	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&filesrc, mp_filesrc_init, FILE_SRC_ID);
	MP_ELEMENT_INIT(&jpeg_parser, mp_img_jpeg_parser_init, JPEG_PARSER_ID);
	MP_ELEMENT_INIT(&caps_filter, mp_caps_filter_init, CAPS_FILTER_ID);
	MP_ELEMENT_INIT(&disp_sink, mp_disp_sink_init, DISP_SINK_ID);

#if DT_HAS_CHOSEN(zephyr_jpegdec)
	MP_ELEMENT_INIT(&jpeg_dec, mp_vid_transform_init, JPEG_DEC_ID);
	MP_ELEMENT_INIT(&vid_conv, mp_vid_convert_init, VID_CONV_ID);

	ret = mp_object_set_properties((struct mp_object *)&jpeg_dec, MP_PROP_VID_DEVICE,
				       DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_jpegdec)),
				       MP_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#else
	MP_ELEMENT_INIT(&jpeg_dec, mp_img_jpeg_decoder_init, JPEG_DEC_ID);
#endif
#if DT_HAS_CHOSEN(zephyr_videotrans)
	MP_ELEMENT_INIT(&vid_trans, mp_vid_transform_init, VID_TRANS_ID);
	ret = mp_object_set_properties(
		(struct mp_object *)&vid_trans,
		COND_CODE_0(CONFIG_VIDEO_ROTATION_ANGLE,
			(), (VIDEO_CID_ROTATE, CONFIG_VIDEO_ROTATION_ANGLE,)) MP_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#endif

	ret = mp_object_set_properties((struct mp_object *)&filesrc, MP_PROP_FS_SRC_PATH,
				       CONFIG_FILE_INPUT_PATH, MP_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	{
		struct mp_caps *caps = mp_caps_new(
			MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT, VIDEO_PIX_FMT_JPEG,
			MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT, CONFIG_JPEG_IMAGE_WIDTH,
			MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT, CONFIG_JPEG_IMAGE_HEIGHT, MP_CAPS_END);

		if (caps == NULL) {
			goto err;
		}

		ret = mp_object_set_properties((struct mp_object *)&caps_filter, MP_PROP_BASE_CAPSFILTER_CAPS, caps,
					       MP_PROP_LIST_END);
		mp_caps_unref(caps);
		if (ret < 0) {
			goto err;
		}
	}

	/* clang-format off */
	ret = mp_bin_add((struct mp_bin *)&pipe,
			(struct mp_element *)&filesrc,
			(struct mp_element *)&jpeg_parser,
			(struct mp_element *)&caps_filter,
			(struct mp_element *)&jpeg_dec,
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_jpegdec), ((struct mp_element *)&vid_conv,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans), ((struct mp_element *)&vid_trans,))
			(struct mp_element *)&disp_sink,
			NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	ret = mp_element_link((struct mp_element *)&filesrc,
			(struct mp_element *)&jpeg_parser,
			(struct mp_element *)&caps_filter,
			(struct mp_element *)&jpeg_dec,
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_jpegdec), ((struct mp_element *)&vid_conv,))
			IF_ENABLED(DT_HAS_CHOSEN(zephyr_videotrans), ((struct mp_element *)&vid_trans,))
			(struct mp_element *)&disp_sink,
			NULL);
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
