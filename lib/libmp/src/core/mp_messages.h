/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Message header file.
 */

#ifndef __MP_MESSAGE_H__
#define __MP_MESSAGE_H__

#include <stdint.h>

#include "mp_object.h"
#include "mp_structure.h"

/**
 * @defgroup mp_message
 * @brief Message used to notify application
 *
 * @{
 */

/**
 * @enum mp_message_type
 * Supported message types to notify the application
 */
enum mp_message_type {
	MP_MESSAGE_UNKNOWN = 0,      /**< Unknown message type */
	MP_MESSAGE_EOS = (1 << 0),   /**< End-of-stream message */
	MP_MESSAGE_ERROR = (1 << 1), /**< Error message */
	MP_MESSAGE_ANY = 0xFFFFFFFF  /**< Wildcard for any message type */
};

/**
 * @struct mp_message
 * @brief Message structure
 *
 * Message structure used to notify application
 */
struct mp_message {
	enum mp_message_type type; /**< type of message */
	struct mp_object *src;     /**< (nullable) object originating message */
	uint32_t timestamp;        /**< Creation time of message */
	uint32_t seq_id;           /**< sequence id of message */
	struct mp_structure *data; /**< (nullable) data to be sent with message */
};

/**
 * Create new message.
 *
 * @param type message type to distinguish between different messages
 * @param src (nullable) object originating message
 * @param data (nullable) data to be sent with message
 * @return (nullable) new message
 */
struct mp_message *mp_message_new(enum mp_message_type type, struct mp_object *src,
				  struct mp_structure *data);

/**
 * Destroys message and its data
 *
 * @param msg Pointer to the message to be destroyed.
 */
void mp_message_destroy(struct mp_message *msg);

/** @} */

#endif /*__MP_MESSAGE_H__*/
