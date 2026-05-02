/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_OS_MGMT_CALLBACKS_
#define H_MCUMGR_OS_MGMT_CALLBACKS_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr OS Management Callbacks API
 * @defgroup mcumgr_callback_api_os_mgmt OS Management Callbacks
 * @ingroup mcumgr_os_mgmt
 * @ingroup mcumgr_callback_api
 * @{
 */

/**
 * MGMT event opcodes for operating system management group.
 */
enum os_mgmt_group_events {
	/** Callback when a reset command has been received, data is @ref os_mgmt_reset_data. */
	MGMT_EVT_OP_OS_MGMT_RESET		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 0),

	/** Callback when an info command is processed, data is @ref os_mgmt_info_check. */
	MGMT_EVT_OP_OS_MGMT_INFO_CHECK		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 1),

	/** Callback when an info command needs to output data, data is @ref os_mgmt_info_append. */
	MGMT_EVT_OP_OS_MGMT_INFO_APPEND		= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 2),

	/** Callback when a datetime get command has been received. */
	MGMT_EVT_OP_OS_MGMT_DATETIME_GET	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 3),

	/** Callback when a datetime set command has been received, data is @ref rtc_time. */
	MGMT_EVT_OP_OS_MGMT_DATETIME_SET	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 4),

	/**
	 * Callback when a bootloader info command has been received, data is
	 * os_mgmt_bootloader_info_data().
	 */
	MGMT_EVT_OP_OS_MGMT_BOOTLOADER_INFO	= MGMT_DEF_EVT_OP_ID(MGMT_EVT_GRP_OS, 5),

	/** Used to enable all os_mgmt_group events. */
	MGMT_EVT_OP_OS_MGMT_ALL			= MGMT_DEF_EVT_OP_ALL(MGMT_EVT_GRP_OS),
};

/**
 * Structure provided in the #MGMT_EVT_OP_OS_MGMT_RESET notification callback: This callback
 * function is used to notify the application about a pending device reboot request and to
 * authorise or deny it.
 */
struct os_mgmt_reset_data {
	/** Contains the value of the force parameter. */
	bool force;

#if defined(CONFIG_MCUMGR_GRP_OS_RESET_BOOT_MODE) || defined(__DOXYGEN__)
	/** Contains the value of the boot_mode parameter. */
	uint8_t boot_mode;
#endif
};

/**
 * Structure provided in the #MGMT_EVT_OP_OS_MGMT_BOOTLOADER_INFO notification callback: This
 * callback function is used to add new fields to the bootloader info response.
 */
struct os_mgmt_bootloader_info_data {
	/**
	 * The zcbor encoder which is currently being used to output group information, additional
	 * fields to the group can be added using this.
	 */
	zcbor_state_t *zse;

	/** Contains the number of decoded parameters. */
	const size_t *decoded;

	/** Contains the value of the query parameter. */
	struct zcbor_string *query;

	/**
	 * Must be set to true to indicate a response has been added, otherwise will return the
	 * #OS_MGMT_ERR_QUERY_YIELDS_NO_ANSWER error.
	 */
	bool *has_output;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
