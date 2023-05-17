/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_SHELL_MGMT_
#define H_SHELL_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for shell management group.
 */
#define SHELL_MGMT_ID_EXEC   0

/**
 * Command result codes for shell management group.
 */
enum shell_mgmt_ret_code_t {
	/** No error, this is implied if there is no ret value in the response */
	SHELL_MGMT_RET_RC_OK = 0,

	/** Unknown error occurred. */
	SHELL_MGMT_RET_RC_UNKNOWN,

	/** The provided command to execute is too long. */
	SHELL_MGMT_RET_RC_COMMAND_TOO_LONG,

	/** No command to execute was provided. */
	SHELL_MGMT_RET_RC_EMPTY_COMMAND,
};

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/*
 * @brief	Translate shell mgmt group error code into MCUmgr error code
 *
 * @param ret	#shell_mgmt_ret_code_t error code
 *
 * @return	#mcumgr_err_t error code
 */
int shell_mgmt_translate_error_code(uint16_t ret);
#endif

#ifdef __cplusplus
}
#endif

#endif /* H_SHELL_MGMT_ */
