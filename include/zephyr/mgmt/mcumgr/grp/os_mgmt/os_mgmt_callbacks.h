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
 * @brief MCUmgr os_mgmt callback API
 * @defgroup mcumgr_callback_api_os_mgmt MCUmgr os_mgmt callback API
 * @ingroup mcumgr_callback_api
 * @{
 */

/**
 * Structure provided in the #MGMT_EVT_OP_OS_MGMT_RESET notification callback: This callback
 * function is used to notify the application about a pending device reboot request and to
 * authorise or deny it.
 */
struct os_mgmt_reset_data {
	/** Contains the value of the force parameter. */
	bool force;
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
