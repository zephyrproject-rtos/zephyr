/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_IPC_MSG_TYPES_H_
#define ZEPHYR_INCLUDE_IPC_IPC_MSG_TYPES_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Message Service Message Types
 * @defgroup ipc_msg_service_msg_types IPC Message Service Message Types
 * @ingroup ipc_msg_service_api
 * @{
 */

/**
 * Enum for IPC message types.
 */
enum ipc_msg_types {
	/**
	 * Start message type number for common message types.
	 *
	 * @internal
	 * Starting at 1 to avoid having a type with value 0.
	 * @endinternal
	 */
	IPC_MSG_TYPE_COMMON_START = 1,

	/** IPC message type with only a uint32_t command. */
	IPC_MSG_TYPE_CMD = IPC_MSG_TYPE_COMMON_START,

	/** Starting number to module specific IPC message types. */
	IPC_MSG_TYPE_MODULE_START = 1000,

	/**
	 * @cond INTERNAL_HIDDEN
	 *
	 * Module specific IPC message types are defined below.
	 *
	 * - When adding types for a new module, leave some spaces
	 *   for the previous module to grow. A good practice would be
	 *   to round up the last entry of previous module to the next
	 *   hundreds and add another hundred.
	 *
	 *   For example:
	 *     IPC_MSG_TYPE_MODULE_A_TYPE_LATEST = 5038,
	 *       <... rounding up to next hundred and add 100 ...>
	 *     IPC_MSG_TYPE_MODULE_B_TYPE_NEW = 5200,
	 *
	 * - Removing old types should follow the deprecation process
	 *   to avoid breaking any users of those types.
	 *
	 * @endcond
	 */

	/** Starting number to define custom IPC message types. */
	IPC_MSG_TYPE_CUSTOM_START = 50000,

	/** Maximum number of IPC message types. */
	IPC_MSG_TYPE_NUM_MAX = 65536
};

/**
 * IPC_MSG_TYPE_CMD struct
 */
struct ipc_msg_type_cmd {
	/** IPC command. */
	uint32_t cmd;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_MSG_TYPES_H_ */
