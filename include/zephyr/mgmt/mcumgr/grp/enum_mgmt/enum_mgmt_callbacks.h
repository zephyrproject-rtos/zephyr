/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_MCUMGR_ENUM_MGMT_CALLBACKS_
#define H_MCUMGR_ENUM_MGMT_CALLBACKS_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MCUmgr Enumeration Management Callbacks API
 * @defgroup mcumgr_callback_api_enum_mgmt Enumeration Management Callbacks
 * @ingroup mcumgr_enum_mgmt
 * @ingroup mcumgr_callback_api
 * @{
 */

/**
 * Structure provided in the #MGMT_EVT_OP_ENUM_MGMT_DETAILS notification callback: This callback
 * function is called once per command group when the detail command is used, it can be used to
 * return additional information/fields in the response.
 */
struct enum_mgmt_detail_output {
	/** The group that is currently being enumerated. */
	const struct mgmt_group *group;

	/**
	 * The zcbor encoder which is currently being used to output group information, additional
	 * fields to the group can be added using this.
	 */
	zcbor_state_t *zse;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
