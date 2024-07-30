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
enum zephyr_basic_group_err_code_t {
	/** No error, this is implied if there is no ret value in the response */
	ZEPHYRBASIC_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	ZEPHYRBASIC_MGMT_ERR_UNKNOWN,

	/** Opening of the flash area has failed. */
	ZEPHYRBASIC_MGMT_ERR_FLASH_OPEN_FAILED,

	/** Querying the flash area parameters has failed. */
	ZEPHYRBASIC_MGMT_ERR_FLASH_CONFIG_QUERY_FAIL,

	/** Erasing the flash area has failed. */
	ZEPHYRBASIC_MGMT_ERR_FLASH_ERASE_FAILED,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZEPHYR_MCUMGR_GRP_ZBASIC_H_ */
