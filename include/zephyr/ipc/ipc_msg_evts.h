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

enum ipc_msg_evts {
	/**
	 * Start event type number for common event types.
	 *
	 * @note Starting at 1 to avoid having a type with value 0.
	 */
	IPC_MSG_EVT_COMMON_START = 1,

	/** Remote has acknowleged the message. */
	IPC_MSG_EVT_REMOTE_ACK = IPC_MSG_EVT_COMMON_START,

	/** Remote has done processing the message. */
	IPC_MSG_EVT_REMOTE_DONE,

	/** Max number for common event types. */
	IPC_MSG_EVT_COMMON_MAX,

	/** Starting number to define custom event types. */
	IPC_MSG_EVT_CUSTOM_START = 128,

	/** Maximum number of event types. */
	IPC_MSG_EVT_MAX = 255
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_IPC_IPC_MSG_EVTS_H_ */
