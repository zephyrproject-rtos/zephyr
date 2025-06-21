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
 * @brief IPC Message Types
 * @ingroup ipc_msg_service_api
 * @{
 */

enum ipc_msg_types {
	/**
	 * Start message type number for common message types.
	 *
	 * @note Starting at 1 to avoid having a type with value 0.
	 */
	IPC_MSG_TYPE_COMMON_START = 1,

	/** IPC message type with only a uint32_t command. */
	IPC_MSG_TYPE_CMD = IPC_MSG_TYPE_COMMON_START,

	/** Max number for common IPC message types. */
	IPC_MSG_TYPE_COMMON_MAX,

	/** Starting number to define custom IPC message types. */
	IPC_MSG_TYPE_CUSTOM_START = 128,

	/** Maximum number of IPC message types. */
	IPC_MSG_TYPE_MAX = 255
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
