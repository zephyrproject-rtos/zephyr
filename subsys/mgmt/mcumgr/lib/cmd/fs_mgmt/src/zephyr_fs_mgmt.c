/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fs/fs.h>
#include <errno.h>
#include <mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/buf.h>
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
	size_t file_size = 0;

	if (offset == 0) {
		rc = fs_mgmt_impl_filelen(path, &file_size);
	}

	fs_file_t_init(&file);
	rc = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

	if (offset == 0 && file_size > 0) {
		/* Offset is 0 and existing file exists with data, attempt to truncate the file
		 * size to 0
		 */
		rc = fs_truncate(&file, 0);

		if (rc == -ENOTSUP) {
			/* Truncation not supported by filesystem, therefore close file, delete
			 * it then re-open it
			 */
			fs_close(&file);

			rc = fs_unlink(path);
			if (rc < 0 && rc != -ENOENT) {
				return rc;
			}

			rc = fs_open(&file, path, FS_O_CREATE | FS_O_WRITE);
		}

		if (rc < 0) {
			/* Failed to truncate file */
			return MGMT_ERR_EUNKNOWN;
		}
	} else if (offset > 0) {
		rc = fs_seek(&file, offset, FS_SEEK_SET);
		if (rc != 0) {
			goto done;
		}
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
