/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SETTINGS_MGMT_
#define H_SETTINGS_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for settings management group.
 */
#define SETTINGS_MGMT_ID_READ_WRITE		0
#define SETTINGS_MGMT_ID_DELETE			1
#define SETTINGS_MGMT_ID_COMMIT			2
#define SETTINGS_MGMT_ID_LOAD_SAVE		3

/**
 * Command result codes for settings management group.
 */
enum settings_mgmt_ret_code_t {
	/** No error, this is implied if there is no ret value in the response. */
	SETTINGS_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	SETTINGS_MGMT_ERR_UNKNOWN,

	/** The provided key name is too long to be used. */
	SETTINGS_MGMT_ERR_KEY_TOO_LONG,

	/** The provided key name does not exist. */
	SETTINGS_MGMT_ERR_KEY_NOT_FOUND,

	/** The provided key name does not support being read. */
	SETTINGS_MGMT_ERR_READ_NOT_SUPPORTED,

	/** The provided root key name does not exist. */
	SETTINGS_MGMT_ERR_ROOT_KEY_NOT_FOUND,

	/** The provided key name does not support being written. */
	SETTINGS_MGMT_ERR_WRITE_NOT_SUPPORTED,

	/** The provided key name does not support being deleted. */
	SETTINGS_MGMT_ERR_DELETE_NOT_SUPPORTED,
};

#ifdef __cplusplus
}
#endif

#endif
