/*
 * Copyright (c) 2021-2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_ZEPHYR_MCUMGR_GRP_ZBASIC_H_
#define ZEPHYR_INCLUDE_ZEPHYR_MCUMGR_GRP_ZBASIC_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Command IDs for zephyr basic management group.
 */
#define ZEPHYR_MGMT_GRP_BASIC_CMD_ERASE_STORAGE	0	/* Command to erase storage partition */

/**
 * Command result codes for statistics management group.
 */
enum zephyr_basic_group_ret_code_t {
	/** No error, this is implied if there is no ret value in the response */
	ZEPHYR_MGMT_GRP_CMD_RC_OK = 0,

	ZEPHYR_MGMT_GRP_CMD_RC_FLASH_OPEN_FAILED,
	ZEPHYR_MGMT_GRP_CMD_RC_FLASH_CONFIG_QUERY_FAIL,
	ZEPHYR_MGMT_GRP_CMD_RC_FLASH_ERASE_FAILED,
};

#ifdef CONFIG_MCUMGR_SMP_SUPPORT_ORIGINAL_PROTOCOL
/*
 * @brief	Translate zephyr basic group error code into MCUmgr error code
 *
 * @param ret	#zephyr_basic_group_ret_code_t error code
 *
 * @return	#mcumgr_err_t error code
 */
int zephyr_basic_group_translate_error_code(uint16_t ret);
#endif

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_MCUMGR_GRP_ZBASIC_H_ */
