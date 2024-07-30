/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_SETTINGS_MGMT_CALLBACKS_
#define H_MCUMGR_SETTINGS_MGMT_CALLBACKS_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr settings_mgmt callback API
 * @defgroup mcumgr_callback_api_settings_mgmt MCUmgr settings_mgmt callback API
 * @ingroup mcumgr_callback_api
 * @{
 */

enum settings_mgmt_access_types {
	SETTINGS_ACCESS_READ,
	SETTINGS_ACCESS_WRITE,
	SETTINGS_ACCESS_DELETE,
	SETTINGS_ACCESS_COMMIT,
	SETTINGS_ACCESS_LOAD,
	SETTINGS_ACCESS_SAVE,
};

/**
 * Structure provided in the #MGMT_EVT_OP_SETTINGS_MGMT_ACCESS notification callback: This
 * callback function is used to notify the application about a pending setting
 * read/write/delete/load/save/commit request and to authorise or deny it. Access will be allowed
 * so long as no handlers return an error, if one returns an error then access will be denied.
 */
struct settings_mgmt_access {
	/** Type of access */
	enum settings_mgmt_access_types access;

	/**
	 * Key name for accesses (only set for SETTINGS_ACCESS_READ, SETTINGS_ACCESS_WRITE and
	 * SETTINGS_ACCESS_DELETE). Note that this can be changed by handlers to redirect settings
	 * access if needed (as long as it does not exceed the maximum setting string size) if
	 * CONFIG_MCUMGR_GRP_SETTINGS_BUFFER_TYPE_STACK is selected, of maximum size
	 * CONFIG_MCUMGR_GRP_SETTINGS_NAME_LEN.
	 *
	 * Note: This string *must* be NULL terminated.
	 */
#ifdef CONFIG_MCUMGR_GRP_SETTINGS_BUFFER_TYPE_HEAP
	const char *name;
#else
	char *name;
#endif

	/** Data provided by the user (only set for SETTINGS_ACCESS_WRITE) */
	const uint8_t *val;

	/** Length of data provided by the user (only set for SETTINGS_ACCESS_WRITE) */
	const size_t *val_length;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
