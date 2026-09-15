/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tee pipeline sample demonstrating multi-branch pipelines:
 *
 *   [file_src] -> [jpeg_parser] -> [caps_filter] -> [tee] -> [queue1] -> [jpeg_dec] -> [disp_sink]
 *                                                     -> [queue2] -> [file_sink]
 */

#include <errno.h>
#if defined(CONFIG_FAT_FILESYSTEM_ELM)
#include <ff.h>
#endif

#include <zephyr/drivers/video.h>
#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/base/mpipe_caps_filter.h>
#include <zephyr/mpipe/base/mpipe_queue.h>
#include <zephyr/mpipe/base/mpipe_tee.h>
#include <zephyr/mpipe/disp/mpipe_disp_sink.h>
#include <zephyr/mpipe/fs/mpipe_file_sink.h>
#include <zephyr/mpipe/fs/mpipe_file_src.h>
#include <zephyr/mpipe/img/mpipe_img_jpeg_decoder.h>
#include <zephyr/mpipe/img/mpipe_img_jpeg_parser.h>
#include <zephyr/mpipe/utils/mpipe_player.h>

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

static struct mpipe pipe;
static struct mpipe_file_src file_src;
static struct mpipe_img_jpeg_parser jpeg_parser;
static struct mpipe_caps_filter caps_filter;
static struct mpipe_tee tee;
static struct mpipe_queue queue1;
static struct mpipe_img_jpeg_decoder jpeg_dec;
static struct mpipe_disp_sink disp_sink;
static struct mpipe_queue queue2;
static struct mpipe_file_sink file_sink;
static struct mpipe_player player;

int main(void)
{
	int ret;

	ret = mount_sd();
	if (ret != 0) {
		goto err;
	}

	/* Initialize all elements */
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
	ret = mpipe_tee_init(&tee, TEE_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_queue_init(&queue1, QUEUE1_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_img_jpeg_decoder_init(&jpeg_dec, JPEG_DEC_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_disp_sink_init(&disp_sink, DISP_SINK_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_queue_init(&queue2, QUEUE2_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_file_sink_init(&file_sink, FILE_SINK_ID);
	if (ret < 0) {
		goto err;
	}

	/* Set properties */
	ret = mpipe_object_set_properties((struct mpipe_object *)&file_src, MPIPE_PROP_FS_SRC_PATH,
					  CONFIG_FILE_INPUT_PATH, MPIPE_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set file_src properties (%d)", ret);
		goto err;
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&file_sink,
					  MPIPE_PROP_FS_SINK_PATH, CONFIG_FILE_OUTPUT_PATH,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		LOG_ERR("Failed to set file_sink properties (%d)", ret);
		goto err;
	}

	/* Set caps filter to constrain negotiation with JPEG format + resolution */
	{
		struct mpipe_structure caps;

		ret = mpipe_structure_init_fields(
			&caps, MPIPE_MEDIA_VIDEO, MPIPE_CAPS_PIXEL_FORMAT, MPIPE_TYPE_UINT,
			VIDEO_PIX_FMT_JPEG, MPIPE_CAPS_IMAGE_WIDTH, MPIPE_TYPE_UINT,
			CONFIG_JPEG_IMAGE_WIDTH, MPIPE_CAPS_IMAGE_HEIGHT, MPIPE_TYPE_UINT,
			CONFIG_JPEG_IMAGE_HEIGHT, MPIPE_CAPS_END);

		if (ret != 0) {
			LOG_ERR("Failed to create caps");
			goto err;
		}

		ret = mpipe_object_set_properties((struct mpipe_object *)&caps_filter,
						  MPIPE_PROP_BASE_CAPS_FILTER_CAPS, &caps,
						  MPIPE_PROP_LIST_END);
		if (ret < 0) {
			LOG_ERR("Failed to set caps_filter properties (%d)", ret);
			goto err;
		}
	}

	/* Add all elements to the pipeline bin */
	ret = mpipe_bin_add((struct mpipe_bin *)&pipe, (struct mpipe_element *)&file_src,
			    (struct mpipe_element *)&jpeg_parser,
			    (struct mpipe_element *)&caps_filter, (struct mpipe_element *)&tee,
			    (struct mpipe_element *)&queue1, (struct mpipe_element *)&jpeg_dec,
			    (struct mpipe_element *)&disp_sink, (struct mpipe_element *)&queue2,
			    (struct mpipe_element *)&file_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* Branch 1: file_src -> jpeg_parser -> caps_filter -> tee -> queue1 ->
	 * jpeg_dec -> disp_sink
	 */
	ret = mpipe_element_link((struct mpipe_element *)&file_src,
				 (struct mpipe_element *)&jpeg_parser,
				 (struct mpipe_element *)&caps_filter, (struct mpipe_element *)&tee,
				 (struct mpipe_element *)&queue1, (struct mpipe_element *)&jpeg_dec,
				 (struct mpipe_element *)&disp_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link branch 1 (%d)", ret);
		goto err;
	}

	/* Branch 2: tee (2nd src_pad) -> queue2 -> file_sink */
	ret = mpipe_element_link((struct mpipe_element *)&tee, (struct mpipe_element *)&queue2,
				 (struct mpipe_element *)&file_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link branch 2 (%d)", ret);
		goto err;
	}

	LOG_INF("Pipeline linked.");

	ret = mpipe_player_init(&player, &pipe);
	if (ret != 0) {
		LOG_ERR("Failed to init player (%d)", ret);
		goto err;
	}

	(void)mpipe_player_play(&player);
	(void)mpipe_player_wait_quit(&player);
	(void)mpipe_player_deinit(&player);

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
