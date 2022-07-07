/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>
#include <errno.h>
#include <zephyr/mgmt/mcumgr/buf.h>
#include <mgmt/mgmt.h>
#include <fs_mgmt/fs_mgmt_impl.h>

int
fs_mgmt_impl_filelen(const char *path, size_t *out_len)
{
	struct fs_dirent dirent;
	int rc;

	rc = fs_stat(path, &dirent);

	if (rc == -EINVAL) {
		return MGMT_ERR_EINVAL;
	} else if (rc == -ENOENT) {
		return MGMT_ERR_ENOENT;
	} else if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	if (dirent.type != FS_DIR_ENTRY_FILE) {
		return MGMT_ERR_EUNKNOWN;
	}

	*out_len = dirent.size;

	return 0;
}

int
fs_mgmt_impl_read(const char *path, size_t offset, size_t len,
				  void *out_data, size_t *out_len)
{
	struct fs_file_t file;
	ssize_t bytes_read;
	int rc;

	fs_file_t_init(&file);
	rc = fs_open(&file, path, FS_O_READ);
	if (rc != 0) {
		return MGMT_ERR_ENOENT;
	}

	rc = fs_seek(&file, offset, FS_SEEK_SET);
	if (rc != 0) {
		goto done;
	}

	bytes_read = fs_read(&file, out_data, len);
	if (bytes_read < 0) {
		goto done;
	}

	*out_len = bytes_read;

done:
	fs_close(&file);

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return 0;
}

int
fs_mgmt_impl_write(const char *path, size_t offset, const void *data,
				   size_t len)
{
	struct fs_file_t file;
	int rc;

	/* Truncate the file before writing the first chunk.  This is done to
	 * properly handle an overwrite of an existing file
	 */
	if (offset == 0) {
		/* Try to truncate file; this will return -ENOENT if file
		 * does not exist so ignore it. Fs may log error -2 here,
		 * just ignore it.
		 */
		rc = fs_unlink(path);
		if (rc < 0 && rc != -ENOENT) {
			return rc;
		}
	}

	fs_file_t_init(&file);
	rc = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	rc = fs_seek(&file, offset, FS_SEEK_SET);
	if (rc != 0) {
		goto done;
	}

	rc = fs_write(&file, data, len);
	if (rc < 0) {
		goto done;
	}

done:
	fs_close(&file);

	if (rc < 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	return 0;
}
