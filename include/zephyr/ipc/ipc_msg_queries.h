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

enum ipc_msg_queries {
	/**
	 * Start event type number for common query types.
	 *
	 * @note Starting at 1 to avoid having a type with value 0.
	 */
	IPC_MSG_QUERY_COMMON_START = 1,

	/**
	 * Ask if the backend is ready.
	 *
	 * Returns 0 if backend is ready. Negative value if not.
	 */
	IPC_MSG_QUERY_IS_READY = IPC_MSG_QUERY_COMMON_START,

	/** Max number for common query types. */
	IPC_MSG_QUERY_COMMON_MAX,

	/** Starting number to define custom query types. */
	IPC_MSG_QUERY_CUSTOM_START = 128,

	/** Maximum number of query types. */
	IPC_MSG_QUERY_MAX = 255
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_MSG_QUERIES_H_ */
