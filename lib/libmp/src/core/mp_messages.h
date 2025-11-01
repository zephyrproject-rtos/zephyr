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
 * @defgroup MpMessage
 * @brief Message used to notify application
 *
 * @{
 */

#define MP_MESSAGE_TYPE(msg) (msg->type)

/**
 * @enum MpMessageType
 * Supported message types to notify the application
 */
typedef enum {
	MP_MESSAGE_UNKNOWN = 0,               /**< Unknown message type */
	MP_MESSAGE_EOS = (1 << 0),            /**< End-of-stream message */
	MP_MESSAGE_ERROR = (1 << 1),          /**< Error message */
	MP_MESSAGE_ANY = (uint32_t)0xFFFFFFFF /**< Wildcard for any message type */
} MpMessageType;

/**
 * @struct _MpMessage
 * @brief Message structure
 *
 * Message structure used to notify application
 */
typedef struct {
	MpMessageType type; /**< type of message */
	MpObject *src;      /**< (nullable) object originating message */
	uint32_t timestamp; /**< Creation time of message */
	uint32_t seq_id;    /**< sequence id of message */
	MpStructure *data;  /**< (nullable) data to be sent with message */
} MpMessage;

/**
 * Create new message.
 *
 * @param type message type to distinguish between different messages
 * @param src (nullable) object originating message
 * @param data (nullable) data to be sent with message
 * @return (nullable) new message
 */
MpMessage *mp_message_new(MpMessageType type, MpObject *src, MpStructure *data);

/**
 * Destroys message and its data
 *
 * @param msg Pointer to the message to be destroyed.
 */
void mp_message_destroy(MpMessage *msg);

/** @} */

#endif /*__MP_MESSAGE_H__*/
