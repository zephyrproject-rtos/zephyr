/*
 * Copyright (c) 2021 Voi.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <sys/util.h>
#include <fs/fs.h>

#include <debug/coredump.h>
#include "coredump_internal.h"

#include <logging/log.h>
LOG_MODULE_DECLARE(coredump, CONFIG_KERNEL_LOG_LEVEL);

static bool coredump_file_open;
static struct fs_file_t coredump_file;
static struct fs_dirent coredump_dirent;

static void coredump_fs_backend_start(void)
{
	int ret;

	coredump_file_open = false;

	ret = fs_stat(CONFIG_DEBUG_COREDUMP_BACKEND_FS_FILENAME, &coredump_dirent);
	if(ret >= 0) {
		ret = fs_unlink(CONFIG_DEBUG_COREDUMP_BACKEND_FS_FILENAME);
		if(ret < 0) {
			LOG_ERR("Failed to remove the coredump file");
			return;
		}
	}

	ret = fs_open(&coredump_file, CONFIG_DEBUG_COREDUMP_BACKEND_FS_FILENAME, FS_O_WRITE | FS_O_CREATE);
	if (ret < 0) {
		LOG_ERR("fs_open returned %d", ret);
		return;
	}

	coredump_file_open = true;
}

static void coredump_fs_backend_end(void)
{
	int ret;

	if(coredump_file_open) {
		LOG_ERR("core dumped");
		ret = fs_close(&coredump_file);
		if (ret < 0) {
			LOG_ERR("fs_close error");
		}
	}
}

static void coredump_fs_backend_error(void)
{
	LOG_ERR("Error");
}

static int coredump_fs_backend_buffer_output(uint8_t *buf, size_t buflen)
{
	size_t remaining = buflen;
	int ret = 0;

	if(!coredump_file_open) {
		return 0;
	}

	if ((buf == NULL) || (buflen == 0)) {
		ret = -EINVAL;
		remaining = 0;
	}

	while (remaining > 0) {
		ret = fs_write(&coredump_file, buf, buflen);
		if(ret < 0) {
			LOG_ERR("Failed to write to file, ret = %d", ret);
			return -EINVAL;
		}

		remaining = 0;
	}

	return 0;
}

struct z_coredump_backend_api z_coredump_backend_fs = {
	.start = coredump_fs_backend_start,
	.end = coredump_fs_backend_end,
	.error = coredump_fs_backend_error,
	.buffer_output = coredump_fs_backend_buffer_output,
};
