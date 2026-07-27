/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <ff.h>

#include <zephyr/fs/fs.h>
#include <zephyr/logging/log.h>

#include <zephyr/mp/mp.h>
#include <zephyr/mp/fs/mp_filesink.h>
#include <zephyr/mp/fs/mp_filesrc.h>

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

static struct mp_pipeline pipe;
static struct mp_filesrc filesrc;
static struct mp_filesink filesink;

int main(void)
{
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
	MP_ELEMENT_INIT(&pipe, mp_pipeline_init, PIPE_ID);
	MP_ELEMENT_INIT(&filesrc, mp_filesrc_init, FILE_SRC_ID);
	MP_ELEMENT_INIT(&filesink, mp_filesink_init, FILE_SINK_ID);

	ret = mp_object_set_properties((struct mp_object *)&filesrc, MP_PROP_FS_SRC_PATH,
				       MNT_POINT "/" INPUT_FILE, MP_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	ret = mp_object_set_properties((struct mp_object *)&filesink, MP_PROP_FS_SINK_PATH,
				       MNT_POINT "/" OUTPUT_FILE, MP_PROP_LIST_END);
	if (ret < 0) {
		goto err;
	}

	/* Add elements to the pipeline - order does not matter */
	ret = mp_bin_add((struct mp_bin *)&pipe, (struct mp_element *)&filesrc,
			 (struct mp_element *)&filesink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to add elements (%d)", ret);
		goto err;
	}

	/* Link elements together - order does matter */
	ret = mp_element_link((struct mp_element *)&filesrc, (struct mp_element *)&filesink, NULL);
	if (ret < 0) {
		LOG_ERR("Failed to link elements (%d)", ret);
		goto err;
	}

	LOG_INF("Pipeline linked.");

	/* Start the pipeline */
	if (mp_element_set_state((struct mp_element *)&pipe, MP_STATE_PLAYING) !=
	    MP_STATE_CHANGE_SUCCESS) {
		LOG_ERR("Failed to start pipeline");
		goto err;
	}

	/* Handle message from the pipeline */
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

	/* Unmount the disk */
	ret = fs_unmount(&mp);
	if (ret != 0) {
		LOG_ERR("Failed to unmount disk (%d)", ret);
	}

	LOG_INF("Done.");

	return 0;

err:
	LOG_ERR("Aborting sample");
	return 0;
}
