/*
 * Copyright (c) 2018-2021 mcumgr authors
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_STAT_MGMT_
#define H_STAT_MGMT_

/**
 * @brief MCUmgr Statistics Management API
 * @defgroup mcumgr_stat_mgmt Statistics Management
 * @ingroup mcumgr_mgmt_api
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Command IDs for Statistics Management group.
 * @{
 */
#define STAT_MGMT_ID_SHOW 0 /**< Group data */
#define STAT_MGMT_ID_LIST 1 /**< List groups */
/** @} */

/**
 * Command result codes for statistics management group.
 */
enum stat_mgmt_err_code_t {
	/** No error, this is implied if there is no ret value in the response */
	STAT_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	STAT_MGMT_ERR_UNKNOWN,

	/** The provided statistic group name was not found. */
	STAT_MGMT_ERR_INVALID_GROUP,

	/** The provided statistic name was not found. */
	STAT_MGMT_ERR_INVALID_STAT_NAME,

	/** The size of the statistic cannot be handled. */
	STAT_MGMT_ERR_INVALID_STAT_SIZE,

	/** Walk through of statistics was aborted. */
	STAT_MGMT_ERR_WALK_ABORTED,
};

/**
 * @brief Represents a single value in a statistics group.
 */
struct stat_mgmt_entry {
	/** Name of the statistic */
	const char *name;

	/** Value of the statistic */
	uint64_t value;
};

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* H_STAT_MGMT_ */
