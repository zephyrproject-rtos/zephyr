/*
 * Copyright (c) 2018-2021 mcumgr authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Declares implementation-specific functions required by file system
 *	  management.  The default stubs can be overridden with functions that
 *	  are compatible with the host OS.
 */

#ifndef H_FS_MGMT_IMPL_
#define H_FS_MGMT_IMPL_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Retrieves the length of the file at the specified path.
 *
 * @param path		The path of the file to query.
 * @param out_len	On success, the file length gets written here.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int fs_mgmt_impl_filelen(const char *path, size_t *out_len);

/**
 * @brief Reads the specified chunk of file data.
 *
 * @param path		The path of the file to read from.
 * @param offset	The byte offset to read from.
 * @param len		The number of bytes to read.
 * @param out_data	On success, the file data gets written here.
 * @param out_len	On success, the number of bytes actually read
 *			gets written here.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int fs_mgmt_impl_read(const char *path, size_t offset, size_t len,
					  void *out_data, size_t *out_len);

/**
 * @brief Writes the specified chunk of file data.  A write to offset 0 must
 * truncate the file; other offsets must append.
 *
 * @param path		The path of the file to write to.
 * @param offset	The byte offset to write to.
 * @param data		The data to write to the file.
 * @param len		The number of bytes to write.
 *
 * @return 0 on success, MGMT_ERR_[...] code on failure.
 */
int fs_mgmt_impl_write(const char *path, size_t offset, const void *data,
					   size_t len);

#ifdef __cplusplus
}
#endif

#endif
