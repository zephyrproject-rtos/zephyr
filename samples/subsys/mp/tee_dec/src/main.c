/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tee pipeline sample demonstrating multi-branch pipelines:
 *
 *   [filesrc] → [jpeg_parser] → [caps_filter] → [tee] → [queue1] → [jpeg_dec] → [disp_sink]
 *                                                     → [queue2] → [filesink]
 */

#include <errno.h>
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#include <ff.h>
#endif

#include <zephyr/drivers/video.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/mp.h>
#include <zephyr/mp/base/mp_capsfilter.h>
#include <zephyr/mp/base/mp_queue.h>
#include <zephyr/mp/base/mp_tee.h>
#include <zephyr/mp/disp/mp_disp_sink.h>
#include <zephyr/mp/fs/mp_filesink.h>
#include <zephyr/mp/fs/mp_filesrc.h>
#include <zephyr/mp/img/mp_img_jpeg_decoder.h>
#include <zephyr/mp/img/mp_img_jpeg_parser.h>
#include <zephyr/mp/utils/mp_player.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	FILE_SRC_ID,
	JPEG_PARSER_ID,
	CAPS_FILTER_ID,
	TEE_ID,
	QUEUE1_ID,
	JPEG_DEC_ID,
	DISP_SINK_ID,
	QUEUE2_ID,
	FILE_SINK_ID,
};

#define MNT_POINT "/SD:"

#if defined(CONFIG_FAT_FILESYSTEM_ELM)
static FATFS fat_fs;
#endif

static struct fs_mount_t mnt = {
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
	LOG_INF("Created %s (%u bytes)", CONFIG_FILE_INPUT_PATH, (unsigned int)mjpeg_sz);
	return 0;
}

static int mount_sd(void)
{
	int ret = fs_mount(&mnt);

	if (ret != 0) {
		LOG_ERR("fs_mount failed (%d)", ret);
		return ret;
	}

	LOG_INF("SDCard mounted at %s", MNT_POINT);
	return embed_test_file();
}

static struct mp_pipeline pipe;
static struct mp_filesrc filesrc;
static struct mp_img_jpeg_parser jpeg_parser;
static struct mp_caps_filter caps_filter;
static struct mp_tee tee;
static struct mp_queue queue1;
static struct mp_img_jpeg_decoder jpeg_dec;
static struct mp_disp_sink disp_sink;
static struct mp_queue queue2;
static struct mp_filesink filesink;
static struct mp_player player;

int main(void)
{
	int ret;

	ret = mount_sd();
	if (ret != 0) {
		goto err;
	}

	/* Initialize all elements */
	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&filesrc, mp_filesrc_init, FILE_SRC_ID);
	MP_ELEMENT_INIT(&jpeg_parser, mp_img_jpeg_parser_init, JPEG_PARSER_ID);
	MP_ELEMENT_INIT(&caps_filter, mp_caps_filter_init, CAPS_FILTER_ID);
	MP_ELEMENT_INIT(&tee, mp_tee_init, TEE_ID);
	MP_ELEMENT_INIT(&queue1, mp_queue_init, QUEUE1_ID);
	MP_ELEMENT_INIT(&jpeg_dec, mp_img_jpeg_decoder_init, JPEG_DEC_ID);
	MP_ELEMENT_INIT(&disp_sink, mp_disp_sink_init, DISP_SINK_ID);
	MP_ELEMENT_INIT(&queue2, mp_queue_init, QUEUE2_ID);
	MP_ELEMENT_INIT(&filesink, mp_filesink_init, FILE_SINK_ID);

	/* Set properties */
	ret = mp_object_set_properties((struct mp_object *)&filesrc, MP_PROP_FS_SRC_PATH,
				       CONFIG_FILE_INPUT_PATH, MP_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set filesrc properties (%d)", ret);
		goto err;
	}

	ret = mp_object_set_properties((struct mp_object *)&filesink, MP_PROP_FS_SINK_PATH,
				       CONFIG_FILE_OUTPUT_PATH, MP_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set filesink properties (%d)", ret);
		goto err;
	}

	/* Set caps filter to constrain negotiation with JPEG format + resolution */
	{
		struct mp_caps *caps = mp_caps_new(
			MP_MEDIA_VIDEO, MP_CAPS_PIXEL_FORMAT, MP_TYPE_UINT, VIDEO_PIX_FMT_JPEG,
			MP_CAPS_IMAGE_WIDTH, MP_TYPE_UINT, CONFIG_JPEG_IMAGE_WIDTH,
			MP_CAPS_IMAGE_HEIGHT, MP_TYPE_UINT, CONFIG_JPEG_IMAGE_HEIGHT, MP_CAPS_END);

		if (caps == NULL) {
			LOG_ERR("Failed to create caps");
			goto err;
		}

		ret = mp_object_set_properties((struct mp_object *)&caps_filter, MP_PROP_BASE_CAPSFILTER_CAPS, caps,
					       MP_PROP_LIST_END);
		mp_caps_unref(caps);
		if (ret < 0) {
			LOG_ERR("Failed to set caps_filter properties (%d)", ret);
			goto err;
		}
	}

	/* Add all elements to the pipeline bin */
	ret = mp_bin_add((struct mp_bin *)&pipe, (struct mp_element *)&filesrc,
			 (struct mp_element *)&jpeg_parser, (struct mp_element *)&caps_filter,
			 (struct mp_element *)&tee, (struct mp_element *)&queue1,
			 (struct mp_element *)&jpeg_dec, (struct mp_element *)&disp_sink,
			 (struct mp_element *)&queue2, (struct mp_element *)&filesink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* Branch 1: filesrc → jpeg_parser → caps_filter → tee → queue1 → jpeg_dec → disp_sink */
	ret = mp_element_link((struct mp_element *)&filesrc, (struct mp_element *)&jpeg_parser,
			      (struct mp_element *)&caps_filter, (struct mp_element *)&tee,
			      (struct mp_element *)&queue1, (struct mp_element *)&jpeg_dec,
			      (struct mp_element *)&disp_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link branch 1 (%d)", ret);
		goto err;
	}

	/* Branch 2: tee (2nd srcpad) → queue2 → filesink */
	ret = mp_element_link((struct mp_element *)&tee, (struct mp_element *)&queue2,
			      (struct mp_element *)&filesink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link branch 2 (%d)", ret);
		goto err;
	}

	LOG_INF("Pipeline linked.");

	ret = mp_player_init(&player, &pipe);
	if (ret != 0) {
		LOG_ERR("Failed to init player (%d)", ret);
		goto err;
	}

	(void)mp_player_play(&player);
	(void)mp_player_wait_quit(&player);
	(void)mp_player_deinit(&player);

	ret = fs_unmount(&mnt);
	if (ret != 0) {
		LOG_ERR("fs_unmount failed (%d)", ret);
	}

	LOG_INF("Done.");

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
