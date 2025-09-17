/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ENUM_MGMT_
#define H_ENUM_MGMT_

/**
 * @brief MCUmgr Enumeration Management API
 * @defgroup mcumgr_enum_mgmt Enumeration Management
 * @ingroup mcumgr_mgmt_api
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Command IDs for enumeration management group.
 * @{
 */
#define ENUM_MGMT_ID_COUNT   0 /**< Count of supported groups */
#define ENUM_MGMT_ID_LIST    1 /**< List supported groups */
#define ENUM_MGMT_ID_SINGLE  2 /**< Fetch single group ID */
#define ENUM_MGMT_ID_DETAILS 3 /**< Details on supported groups */
/**
 * @}
 */

/**
 * Command result codes for enumeration management group.
 */
enum enum_mgmt_err_code_t {
	/** No error, this is implied if there is no ret value in the response */
	ENUM_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	ENUM_MGMT_ERR_UNKNOWN,

	/** Too many entries were provided. */
	ENUM_MGMT_ERR_TOO_MANY_GROUP_ENTRIES,

	/** Insufficient heap memory to store entry data. */
	ENUM_MGMT_ERR_INSUFFICIENT_HEAP_FOR_ENTRIES,

	/** Provided index is larger than the number of supported grouped. */
	ENUM_MGMT_ERR_INDEX_TOO_LARGE,
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* H_ENUM_MGMT_ */
