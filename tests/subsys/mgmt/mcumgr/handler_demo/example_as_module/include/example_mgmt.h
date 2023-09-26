/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef H_EXAMPLE_MGMT_
#define H_EXAMPLE_MGMT_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Group ID for example management group.
 */
#define MGMT_GROUP_ID_EXAMPLE  MGMT_GROUP_ID_PERUSER

/**
 * Command IDs for example management group.
 */
#define EXAMPLE_MGMT_ID_TEST   0
#define EXAMPLE_MGMT_ID_OTHER  1

/**
 * Command result codes for example management group.
 */
enum example_mgmt_err_code_t {
	/** No error, this is implied if there is no ret value in the response */
	EXAMPLE_MGMT_ERR_OK = 0,

	/** Unknown error occurred. */
	EXAMPLE_MGMT_ERR_UNKNOWN,

	/** The provided value is not wanted at this time. */
	EXAMPLE_MGMT_ERR_NOT_WANTED,

	/** The provided value was rejected by a hook. */
	EXAMPLE_MGMT_ERR_REJECTED_BY_HOOK,
};

#ifdef __cplusplus
}
#endif

#endif
