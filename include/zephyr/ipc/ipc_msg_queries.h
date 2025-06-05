/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_IPC_MSG_QUERIES_H_
#define ZEPHYR_INCLUDE_IPC_IPC_MSG_QUERIES_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Message Service Query Types
 * @defgroup ipc_msg_service_query_types IPC Message Service Query Types
 * @ingroup ipc_msg_service_api
 * @{
 */

/**
 * Enum for IPC message service query types.
 */
enum ipc_msg_queries {
	/**
	 * Start event type number for common query types.
	 *
	 * @internal
	 * Starting at 1 to avoid having a type with value 0.
	 * @endinternal
	 */
	IPC_MSG_QUERY_COMMON_START = 1,

	/**
	 * Ask if the backend is ready.
	 *
	 * Returns 0 if backend is ready. Negative value if not.
	 */
	IPC_MSG_QUERY_IS_READY = IPC_MSG_QUERY_COMMON_START,

	/**
	 * @cond INTERNAL_HIDDEN
	 *
	 * Module specific query types are defined below.
	 *
	 * - When adding types for a new module, leave some spaces
	 *   for the previous module to grow. A good practice would be
	 *   to round up the last entry of previous module to the next
	 *   hundreds and add another hundred.
	 *
	 *   For example:
	 *     IPC_MSG_QUERY_MODULE_A_TYPE_LATEST = 5038,
	 *       <... rounding up to next hundred and add 100 ...>
	 *     IPC_MSG_QUERY_MODULE_B_TYPE_NEW = 5200,
	 *
	 * - Removing old types should follow the deprecation process
	 *   to avoid breaking any users of those types.
	 *
	 * @endcond
	 */

	/** Starting number to define custom query types. */
	IPC_MSG_QUERY_CUSTOM_START = 50000,

	/** Maximum number of query types. */
	IPC_MSG_QUERY_NUM_MAX = 65536
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_MSG_QUERIES_H_ */
