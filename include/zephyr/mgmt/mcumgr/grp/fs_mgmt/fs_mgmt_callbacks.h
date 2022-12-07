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
 * @brief MCUmgr fs_mgmt callback API
 * @defgroup mcumgr_callback_api_fs_mgmt MCUmgr fs_mgmt callback API
 * @ingroup mcumgr_callback_api
 * @{
 */

/**
 * Structure provided in the #MGMT_EVT_OP_FS_MGMT_FILE_ACCESS notification callback: This callback
 * function is used to notify the application about a pending file read/write request and to
 * authorise or deny it. Access will be allowed so long as all notification handlers return
 * #MGMT_ERR_EOK, if one returns an error then access will be denied.
 */
struct fs_mgmt_file_access {
	/** True if the request is for uploading data to the file, otherwise false */
	bool upload;

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
