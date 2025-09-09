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
 * @brief MCUmgr Settings Management Callbacks API
 * @defgroup mcumgr_callback_api_settings_mgmt Settings Management Callbacks
 * @ingroup mcumgr_settings_mgmt
 * @ingroup mcumgr_callback_api
 * @{
 */

/**
 * @name Settings access types
 * @{
 */
enum settings_mgmt_access_types {
	SETTINGS_ACCESS_READ,   /**< Setting is being read */
	SETTINGS_ACCESS_WRITE,  /**< Setting is being written */
	SETTINGS_ACCESS_DELETE, /**< Setting is being deleted */
	SETTINGS_ACCESS_COMMIT, /**< Setting is being committed */
	SETTINGS_ACCESS_LOAD,   /**< Setting is being loaded */
	SETTINGS_ACCESS_SAVE,   /**< Setting is being saved */
};
/** @} */

/**
 * Structure provided in the #MGMT_EVT_OP_SETTINGS_MGMT_ACCESS notification callback.
 * This callback function is used to notify the application about a pending setting
 * read/write/delete/load/save/commit request and to authorise or deny it. Access will be allowed
 * so long as no handlers return an error, if one returns an error then access will be denied.
 */
struct settings_mgmt_access {
	/** Type of access */
	enum settings_mgmt_access_types access;

	/**
	 * Key name for accesses (only set for #SETTINGS_ACCESS_READ, #SETTINGS_ACCESS_WRITE and
	 * #SETTINGS_ACCESS_DELETE). Note that this can be changed by handlers to redirect settings
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
