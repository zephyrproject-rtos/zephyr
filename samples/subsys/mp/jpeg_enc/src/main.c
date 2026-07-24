/*
 * Copyright 2026 Meta Platforms, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * A pipeline showing how to use a jpeg software encoder
 * pipeline
 *  filesrc -> jpeg_parser -> caps_filter -> jpeg_dec -> jpeg_enc -> filesink
 *
 */

#include <errno.h>
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#include <ff.h>
#endif

#include <zephyr/drivers/video.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/core/mp.h>
#include <zephyr/mp/zbase/mp_capsfilter.h>
#include <zephyr/mp/zfs/mp_zfilesink.h>
#include <zephyr/mp/zfs/mp_zfilesrc.h>
#include <zephyr/mp/zjpeg/mp_zjpeg_decoder.h>
#include <zephyr/mp/zjpeg/mp_zjpeg_encoder.h>
#include <zephyr/mp/zjpeg/mp_zjpeg_parser.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

#define PIPE_ID        0
#define FILE_SRC_ID    1
#define JPEG_PARSER_ID 2
#define CAPS_FILTER_ID 3
#define JPEG_DEC_ID    4
#define JPEG_ENC_ID    5
#define FILE_SINK_ID   6

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
static struct mp_zjpeg_decoder jpeg_dec;
static struct mp_zjpeg_encoder jpeg_enc;
static struct mp_zfilesink filesink;

int main(void)
{
	/* (1) mount the filesystem (and embed the test MJPEG on first run) */
	int ret = mount_sd();

	if (ret != 0) {
		goto err;
	}

	/* (2) create the pipeline and its 6 elements */
	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&filesrc, mp_zfilesrc_init, FILE_SRC_ID);
	MP_ELEMENT_INIT(&jpeg_parser, mp_zjpeg_parser_init, JPEG_PARSER_ID);
	MP_ELEMENT_INIT(&caps_filter, mp_caps_filter_init, CAPS_FILTER_ID);
	MP_ELEMENT_INIT(&jpeg_dec, mp_zjpeg_decoder_init, JPEG_DEC_ID);
	MP_ELEMENT_INIT(&jpeg_enc, mp_zjpeg_encoder_init, JPEG_ENC_ID);
	MP_ELEMENT_INIT(&filesink, mp_zfilesink_init, FILE_SINK_ID);

	/* (3) configure elements via properties, in pipeline order */
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

	/*
	 * no need to set properties in jpeg_parser or jpeg_dec, as their behavior is
	 * fully determined by caps negotiation + defaults:
	 * - jpeg_parser just reframes the JPEG byte stream into SOI/EOI frames
	 * - jpeg_dec just decodes to RGB565, and that output format comes from
	 *   negotiation (it advertises RGB565; the encoder's sink wants RGB565;
	 *   they agree).
	 */

	ret = mp_object_set_properties((struct mp_object *)&jpeg_enc, PROP_ZJPEG_ENC_QUALITY,
				       CONFIG_JPEG_ENC_QUALITY, PROP_ZJPEG_ENC_SUBSAMPLE,
				       CONFIG_JPEG_ENC_SUBSAMPLE, PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	ret = mp_object_set_properties((struct mp_object *)&filesink, PROP_ZFILESINK_PATH,
				       CONFIG_FILE_OUTPUT_PATH, PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	/* (4) add the elements to the pipeline (bin) */
	ret = mp_bin_add((struct mp_bin *)&pipe, (struct mp_element *)&filesrc, (struct mp_element *)&jpeg_parser,
			 (struct mp_element *)&caps_filter, (struct mp_element *)&jpeg_dec, (struct mp_element *)&jpeg_enc,
			 (struct mp_element *)&filesink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* (5) link the elements into the processing chain */
	ret = mp_element_link((struct mp_element *)&filesrc, (struct mp_element *)&jpeg_parser,
			      (struct mp_element *)&caps_filter, (struct mp_element *)&jpeg_dec, (struct mp_element *)&jpeg_enc,
			      (struct mp_element *)&filesink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}

	/* (6) start the pipeline (set state to PLAYING) */
	if (mp_element_set_state((struct mp_element *)&pipe, MP_STATE_PLAYING) != MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err;
	}

	/* (7) wait on the bus for EOS or ERROR */
	struct mp_bus *bus = mp_element_get_bus((struct mp_element *)&pipe);
	struct mp_message msg;

	mp_bus_pop_msg(bus, MP_MESSAGE_ERROR | MP_MESSAGE_EOS, &msg);

	switch (msg.type) {
	case MP_MESSAGE_ERROR:
		LOG_INF("ERROR message from element %d", msg.origin->object.id);
		break;
	case MP_MESSAGE_EOS:
		LOG_INF("EOS message from element %d", msg.origin->object.id);
		break;
	default:
		LOG_ERR("Unexpected message from element %d", msg.origin->object.id);
		break;
	}

	/*
	 * (8) Bring the whole pipeline down to READY before unmounting. After EOS the
	 * pipeline is left in PLAYING with its thread parked, so this is the
	 * explicit teardown: it joins the pipeline thread and propagates
	 * PAUSED_TO_READY to every element, closing both the filesrc input file
	 * and the filesink output file (committing it to the FAT).
	 */
	(void)mp_element_set_state((struct mp_element *)&pipe, MP_STATE_READY);

	/* (9) unmount the filesystem */
	ret = fs_unmount(&mp);
	if (ret != 0) {
		LOG_ERR("fs_unmount failed (%d)", ret);
	}

	LOG_INF("Done; output written to %s", CONFIG_FILE_OUTPUT_PATH);

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
