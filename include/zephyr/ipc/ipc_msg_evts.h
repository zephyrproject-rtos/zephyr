/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_IPC_IPC_MSG_EVTS_H_
#define ZEPHYR_INCLUDE_IPC_IPC_MSG_EVTS_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief IPC Message Service Event Types
 * @defgroup ipc_msg_service_evt_types IPC Message Service Event Types
 * @ingroup ipc_msg_service_api
 * @{
 */

/**
 * Enum for IPC message service event types.
 */
enum ipc_msg_evts {
	/**
	 * Start event type number for common event types.
	 *
	 * @internal
	 * Starting at 1 to avoid having a type with value 0.
	 * @endinternal
	 */
	IPC_MSG_EVT_COMMON_START = 1,

	/** Remote has acknowleged the message. */
	IPC_MSG_EVT_REMOTE_ACK = IPC_MSG_EVT_COMMON_START,

	/** Remote has done processing the message. */
	IPC_MSG_EVT_REMOTE_DONE,

	/**
	 * @cond INTERNAL_HIDDEN
	 *
	 * Module specific event types are defined below.
	 *
	 * - When adding types for a new module, leave some spaces
	 *   for the previous module to grow. A good practice would be
	 *   to round up the last entry of previous module to the next
	 *   hundreds and add another hundred.
	 *
	 *   For example:
	 *     IPC_MSG_EVT_MODULE_A_TYPE_LATEST = 5038,
	 *       <... rounding up to next hundred and add 100 ...>
	 *     IPC_MSG_EVT_MODULE_B_TYPE_NEW = 5200,
	 *
	 * - Removing old types should follow the deprecation process
	 *   to avoid breaking any users of those types.
	 *
	 * @endcond
	 */

	/** Starting number to define custom event types. */
	IPC_MSG_EVT_CUSTOM_START = 50000,

	/** Maximum number of event types. */
	IPC_MSG_EVT_NUM_MAX = 65536
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_MSG_EVTS_H_ */
