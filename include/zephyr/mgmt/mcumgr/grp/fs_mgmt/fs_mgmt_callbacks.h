/*
 * Copyright (c) 2022 Laird Connectivity
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_FS_MGMT_CALLBACKS_
#define H_MCUMGR_FS_MGMT_CALLBACKS_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr File System Management Callbacks API
 * @defgroup mcumgr_callback_api_fs_mgmt File System Management Callbacks
 * @ingroup mcumgr_fs_mgmt
 * @ingroup mcumgr_callback_api
 * @{
 */

/** The type of operation that is being requested for a given file access callback. */
enum fs_mgmt_file_access_types {
	/** Access to read file (file download). */
	FS_MGMT_FILE_ACCESS_READ,

	/** Access to write file (file upload). */
	FS_MGMT_FILE_ACCESS_WRITE,

	/** Access to get status of file. */
	FS_MGMT_FILE_ACCESS_STATUS,

	/** Access to calculate hash or checksum of file. */
	FS_MGMT_FILE_ACCESS_HASH_CHECKSUM,
};

/**
 * Structure provided in the #MGMT_EVT_OP_FS_MGMT_FILE_ACCESS notification callback: This callback
 * function is used to notify the application about a pending file read/write request and to
 * authorise or deny it. Access will be allowed so long as all notification handlers return
 * #MGMT_ERR_EOK, if one returns an error then access will be denied.
 */
struct fs_mgmt_file_access {
	/** Specifies the type of the operation that is being requested. */
	enum fs_mgmt_file_access_types access;

	/**
	 * Path and filename of file be accesses, note that this can be changed by handlers to
	 * redirect file access if needed (as long as it does not exceed the maximum path string
	 * size).
	 */
	char *filename;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
