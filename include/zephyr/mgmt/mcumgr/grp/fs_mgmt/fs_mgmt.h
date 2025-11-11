/*
 * Copyright (c) 2018-2022 mcumgr authors
 * Copyright (c) 2022 Laird Connectivity
 * Copyright (c) 2022-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_FS_MGMT_
#define H_FS_MGMT_

/**
 * @brief MCUmgr File System Management API
 * @defgroup mcumgr_fs_mgmt File System Management
 * @ingroup mcumgr_mgmt_api
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Command IDs for File System Management group.
 * @{
 */
#define FS_MGMT_ID_FILE                    0 /**< File download/upload */
#define FS_MGMT_ID_STAT                    1 /**< File status */
#define FS_MGMT_ID_HASH_CHECKSUM           2 /**< File hash/checksum */
#define FS_MGMT_ID_SUPPORTED_HASH_CHECKSUM 3 /**< Supported file hash/checksum types */
#define FS_MGMT_ID_OPENED_FILE             4 /**< File close */
/**
 * @}
 */

/**
 * Command result codes for file system management group.
 */
enum fs_mgmt_err_code_t {
	/** No error, this is implied if there is no ret value in the response */
	FS_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	FS_MGMT_ERR_UNKNOWN,

	/** The specified file name is not valid. */
	FS_MGMT_ERR_FILE_INVALID_NAME,

	/** The specified file does not exist. */
	FS_MGMT_ERR_FILE_NOT_FOUND,

	/** The specified file is a directory, not a file. */
	FS_MGMT_ERR_FILE_IS_DIRECTORY,

	/** Error occurred whilst attempting to open a file. */
	FS_MGMT_ERR_FILE_OPEN_FAILED,

	/** Error occurred whilst attempting to seek to an offset in a file. */
	FS_MGMT_ERR_FILE_SEEK_FAILED,

	/** Error occurred whilst attempting to read data from a file. */
	FS_MGMT_ERR_FILE_READ_FAILED,

	/** Error occurred whilst trying to truncate file. */
	FS_MGMT_ERR_FILE_TRUNCATE_FAILED,

	/** Error occurred whilst trying to delete file. */
	FS_MGMT_ERR_FILE_DELETE_FAILED,

	/** Error occurred whilst attempting to write data to a file. */
	FS_MGMT_ERR_FILE_WRITE_FAILED,

	/**
	 * The specified data offset is not valid, this could indicate that the file on the device
	 * has changed since the previous command. The length of the current file on the device is
	 * returned as "len", the user application needs to decide how to handle this (e.g. the
	 * hash of the file could be requested and compared with the hash of the length of the
	 * file being uploaded to see if they match or not).
	 */
	FS_MGMT_ERR_FILE_OFFSET_NOT_VALID,

	/** The requested offset is larger than the size of the file on the device. */
	FS_MGMT_ERR_FILE_OFFSET_LARGER_THAN_FILE,

	/** The requested checksum or hash type was not found or is not supported by this build. */
	FS_MGMT_ERR_CHECKSUM_HASH_NOT_FOUND,

	/** The specified mount point was not found or is not mounted. */
	FS_MGMT_ERR_MOUNT_POINT_NOT_FOUND,

	/** The specified mount point is that of a read-only filesystem. */
	FS_MGMT_ERR_READ_ONLY_FILESYSTEM,

	/** The operation cannot be performed because the file is empty with no contents. */
	FS_MGMT_ERR_FILE_EMPTY,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
