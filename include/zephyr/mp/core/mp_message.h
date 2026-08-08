/*
 * Copyright 2025-2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Bus message
 */

#ifndef ZEPHYR_INCLUDE_MP_CORE_MP_MESSAGE_H_
#define ZEPHYR_INCLUDE_MP_CORE_MP_MESSAGE_H_

/**
 * @defgroup mp_message Messages
 * @ingroup mp_core
 * @brief Messages exchanged through the bus.
 * @{
 */

#include <stdint.h>

#include <zephyr/kernel.h>

#include <zephyr/mp/core/mp_element.h>

/** @brief Filter mask matching any message type. */
#define MP_MESSAGE_ANY UINT32_MAX

/** @brief Message types, usable as bitmask filters. */
enum mp_message_type {
	MP_MESSAGE_UNKNOWN = 0,        /**< Uninitialized */
	MP_MESSAGE_EOS = (1 << 0),     /**< End of stream */
	MP_MESSAGE_ERROR = (1 << 1),   /**< Error */
	MP_MESSAGE_WARNING = (1 << 2), /**< Warning */
};

/**
 * @brief Message structure carrying type and origin of the message.
 */
struct mp_message {
	struct mp_element *origin; /**< Origin of message */
	uint32_t type;             /**< Message type (see @ref mp_message_type) */
};

/**
 * @brief Initialize a mp_message structure.
 *
 * @param _msg    Pointer to the mp_message to initialize.
 * @param _origin Pointer to the source mp_element.
 * @param _type   Message type (see @ref mp_message_type).
 */
#define MP_MESSAGE_INIT(_msg, _origin, _type)                                                      \
	{                                                                                          \
		(_msg)->origin = (_origin);                                                        \
		(_msg)->type = (_type);                                                            \
	}

/** @} */

#endif /* ZEPHYR_INCLUDE_MP_CORE_MP_MESSAGE_H_ */
