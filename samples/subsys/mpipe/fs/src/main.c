/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ff.h>

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>
#include <zephyr/zbus/zbus.h>

#include <zephyr/mpipe/mpipe.h>
#include <zephyr/mpipe/mpipe_message.h>
#include <zephyr/mpipe/fs/mpipe_file_sink.h>
#include <zephyr/mpipe/fs/mpipe_file_src.h>

LOG_MODULE_REGISTER(main, CONFIG_LOG_DEFAULT_LEVEL);

/* Element IDs (values are arbitrary; only uniqueness within the pipeline matters) */
enum {
	PIPE_ID,
	FILE_SRC_ID,
	FILE_SINK_ID,
};

#define MNT_POINT "/SD:"

#define INPUT_FILE  "test_in.txt"
#define OUTPUT_FILE "test_out.txt"

static FATFS fat_fs;

static struct fs_mount_t mp = {
	.type = FS_FATFS,
	.fs_data = &fat_fs,
	.mnt_point = MNT_POINT,
};

ZBUS_MSG_SUBSCRIBER_DEFINE(main_sub);

static struct mpipe pipe;
static struct mpipe_file_src file_src;
static struct mpipe_file_sink file_sink;

int main(void)
{
	struct zbus_channel *bus = NULL;
	int ret;

	/* Mount the disk */
	ret = fs_mount(&mp);
	if (ret != 0) {
		LOG_ERR("Failed to mount disk (%d)", ret);
		goto err;
	}

	/* Create a file for test */
	{
		struct fs_file_t fh;
		static const char *path = MNT_POINT "/" INPUT_FILE;
		static const char data[] = "test\n";

		fs_file_t_init(&fh);

		ret = fs_open(&fh, path, FS_O_CREATE | FS_O_WRITE);
		if (ret != 0) {
			LOG_ERR("fs_open failed for %s (%d)", path, ret);
			goto err;
		}

		ssize_t wr = fs_write(&fh, data, sizeof(data) - 1);

		if (wr <= 0) {
			LOG_ERR("fs_write returned %u", wr);
			(void)fs_close(&fh);
			goto err;
		}

		ret = fs_close(&fh);
		if (ret != 0) {
			LOG_ERR("fs_close failed (%d)", ret);
			goto err;
		}

		LOG_INF("Wrote %u bytes to %s", wr, path);
	}

	/* Build the pipeline */
	ret = mpipe_pipeline_init(&pipe, PIPE_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_file_src_init(&file_src, FILE_SRC_ID);
	if (ret < 0) {
		goto err;
	}
	ret = mpipe_file_sink_init(&file_sink, FILE_SINK_ID);
	if (ret < 0) {
		goto err;
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&file_src, MPIPE_PROP_FS_SRC_PATH,
					  MNT_POINT "/" INPUT_FILE, MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	ret = mpipe_object_set_properties((struct mpipe_object *)&file_sink,
					  MPIPE_PROP_FS_SINK_PATH, MNT_POINT "/" OUTPUT_FILE,
					  MPIPE_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	/* Add elements to the pipeline - order does not matter */
	ret = mpipe_bin_add((struct mpipe_bin *)&pipe, (struct mpipe_element *)&file_src,
			    (struct mpipe_element *)&file_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* Link elements together - order does matter */
	ret = mpipe_element_link((struct mpipe_element *)&file_src,
				 (struct mpipe_element *)&file_sink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}

	LOG_INF("Pipeline linked.");

	bus = mpipe_element_get_bus_chan((struct mpipe_element *)&pipe);

	ret = zbus_chan_add_obs(bus, &main_sub, K_FOREVER);
	if (ret != 0) {
		LOG_ERR("Failed to attach observer to pipeline channel (%d)", ret);
		goto err;
	}

	/* Start the pipeline */
	if (mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_PLAYING) !=
	    MPIPE_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err_set_state;
	}

	/* Handle message from the pipeline */
	const struct zbus_channel *chan;
	struct mpipe_message msg;

	/* Wait until an Error or an EOS */
	int sub_ret;

	do {
		sub_ret = zbus_sub_wait_msg(&main_sub, &chan, &msg, K_FOREVER);
		if (sub_ret != 0) {
			LOG_ERR("zbus_sub_wait_msg failed (%d)", sub_ret);
			goto err_set_state;
		}
	} while ((msg.type & (MPIPE_MESSAGE_ERROR | MPIPE_MESSAGE_EOS)) == 0);

	int origin_id = (msg.origin != NULL) ? msg.origin->object.id : -1;

	switch (msg.type) {
	case MPIPE_MESSAGE_ERROR:
		LOG_ERR("ERROR message from element %d", origin_id);
		break;
	case MPIPE_MESSAGE_EOS:
		LOG_INF("EOS message from element %d", origin_id);
		break;
	default:
		LOG_ERR("Unexpected message from element %d", origin_id);
		break;
	}

	(void)zbus_chan_rm_obs(bus, &main_sub, K_FOREVER);

	/* Stop/Deinit the pipeline */
	(void)mpipe_element_set_state((struct mpipe_element *)&pipe, MPIPE_STATE_READY);

	/* Unmount the disk */
	ret = fs_unmount(&mp);
	if (ret != 0) {
		LOG_ERR("Failed to unmount disk (%d)", ret);
	}

	LOG_INF("Done.");

	return 0;

err_set_state:
	zbus_chan_rm_obs(bus, &main_sub, K_FOREVER);
err:
	LOG_ERR("Aborting sample");
	return 0;
}
