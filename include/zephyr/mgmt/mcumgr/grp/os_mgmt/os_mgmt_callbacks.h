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
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif
