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

#include <zephyr/mp/core/mp.h>
#include <zephyr/mp/zbase/mp_capsfilter.h>
#include <zephyr/mp/zdisp/mp_zdisp_sink.h>
#include <zephyr/mp/zfs/mp_zfilesrc.h>
#include <zephyr/mp/zjpeg/mp_zjpeg_decoder.h>
#include <zephyr/mp/zjpeg/mp_zjpeg_parser.h>
#if DT_HAS_CHOSEN(zephyr_jpegdec) || DT_HAS_CHOSEN(zephyr_videotrans)
#include <zephyr/mp/zvid/mp_zvid_transform.h>
#endif
#if DT_HAS_CHOSEN(zephyr_jpegdec)
#include <zephyr/mp/zvid/mp_zvid_convert.h>
#include <zephyr/mp/zvid/mp_zvid_property.h>
#endif

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define PIPE_ID        0
#define FILE_SRC_ID    1
#define JPEG_PARSER_ID 2
#define CAPS_FILTER_ID 3
#define JPEG_DEC_ID    4
#define VID_CONV_ID    5
#define VID_TRANS_ID   6
#define DISP_SINK_ID   7

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
static struct mp_zfilesrc filesrc;
static struct mp_zjpeg_parser jpeg_parser;
static struct mp_caps_filter caps_filter;
static struct mp_zdisp_sink disp_sink;

#if DT_HAS_CHOSEN(zephyr_jpegdec)
static struct mp_zvid_transform jpeg_dec;
static struct mp_zvid_convert vid_conv;
#else
static struct mp_zjpeg_decoder jpeg_dec;
#endif
#if DT_HAS_CHOSEN(zephyr_videotrans)
static struct mp_zvid_transform vid_trans;
#endif

int main(void)
{
	int ret = mount_sd();

	if (ret != 0) {
		goto err;
	}

	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&filesrc, mp_zfilesrc_init, FILE_SRC_ID);
	MP_ELEMENT_INIT(&jpeg_parser, mp_zjpeg_parser_init, JPEG_PARSER_ID);
	MP_ELEMENT_INIT(&caps_filter, mp_caps_filter_init, CAPS_FILTER_ID);
	MP_ELEMENT_INIT(&disp_sink, mp_zdisp_sink_init, DISP_SINK_ID);

#if DT_HAS_CHOSEN(zephyr_jpegdec)
	MP_ELEMENT_INIT(&jpeg_dec, mp_zvid_transform_init, JPEG_DEC_ID);
	MP_ELEMENT_INIT(&vid_conv, mp_zvid_convert_init, VID_CONV_ID);

	ret = mp_object_set_properties((struct mp_object *)&jpeg_dec, PROP_ZVID_DEVICE,
				       DEVICE_DT_GET_OR_NULL(DT_CHOSEN(zephyr_jpegdec)),
				       PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#else
	MP_ELEMENT_INIT(&jpeg_dec, mp_zjpeg_decoder_init, JPEG_DEC_ID);
#endif
#if DT_HAS_CHOSEN(zephyr_videotrans)
	MP_ELEMENT_INIT(&vid_trans, mp_zvid_transform_init, VID_TRANS_ID);
	ret = mp_object_set_properties(
		(struct mp_object *)&vid_trans,
		COND_CODE_0(CONFIG_VIDEO_ROTATION_ANGLE,
			(), (VIDEO_CID_ROTATE, CONFIG_VIDEO_ROTATION_ANGLE,)) PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}
#endif

	ret = mp_object_set_properties((struct mp_object *)&filesrc, PROP_ZFILESRC_PATH,
				       CONFIG_FILE_INPUT_PATH, PROP_LIST_END);
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

		ret = mp_object_set_properties((struct mp_object *)&caps_filter, PROP_CAPS, caps,
					       PROP_LIST_END);
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

	if (mp_element_set_state((struct mp_element *)&pipe, MP_STATE_PLAYING) !=
	    MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err;
	}

	struct mp_bus *bus = mp_element_get_bus((struct mp_element *)&pipe);
	struct mp_message msg;

	mp_bus_pop_msg(bus, MP_MESSAGE_ERROR | MP_MESSAGE_EOS, &msg);

	switch (msg.type) {
	case MP_MESSAGE_ERROR:
		LOG_ERR("ERROR message from element %d", msg.origin->object.id);
		break;
	case MP_MESSAGE_EOS:
		LOG_INF("EOS message from element %d", msg.origin->object.id);
		break;
	default:
		LOG_ERR("Unexpected message from element %d", msg.origin->object.id);
		break;
	}

	/* Stop/Deinit the pipeline */
	(void)mp_element_set_state((struct mp_element *)&pipe, MP_STATE_READY);

	ret = fs_unmount(&mp);
	if (ret != 0) {
		LOG_ERR("fs_unmount failed (%d)", ret);
	}

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
