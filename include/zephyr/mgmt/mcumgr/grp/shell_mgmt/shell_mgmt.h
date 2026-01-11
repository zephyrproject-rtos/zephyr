/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SHELL_MGMT_
#define H_SHELL_MGMT_

/**
 * @brief MCUmgr Shell Management API
 * @defgroup mcumgr_shell_mgmt Shell Management
 * @ingroup mcumgr_mgmt_api
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Command IDs for Shell Management group.
 * @{
 */
#define SHELL_MGMT_ID_EXEC 0 /**< Shell command line execute */
/** @} */

/**
 * Command result codes for shell management group.
 */
enum shell_mgmt_err_code_t {
	/** No error, this is implied if there is no ret value in the response */
	SHELL_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	SHELL_MGMT_ERR_UNKNOWN,

	/** The provided command to execute is too long. */
	SHELL_MGMT_ERR_COMMAND_TOO_LONG,

	/** No command to execute was provided. */
	SHELL_MGMT_ERR_EMPTY_COMMAND,
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* H_SHELL_MGMT_ */
