/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SETTINGS_MGMT_
#define H_SETTINGS_MGMT_

/**
 * @brief MCUmgr Settings Management API
 * @defgroup mcumgr_settings_mgmt Settings Management
 * @ingroup mcumgr_mgmt_api
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Command IDs for Settings Management group.
 * @{
 */
#define SETTINGS_MGMT_ID_READ_WRITE 0 /**< Read/write setting */
#define SETTINGS_MGMT_ID_DELETE     1 /**< Delete setting */
#define SETTINGS_MGMT_ID_COMMIT     2 /**< Commit settings */
#define SETTINGS_MGMT_ID_LOAD_SAVE  3 /**< Load/save settings */
/** @} */

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

/**
 * @}
 */

#endif
