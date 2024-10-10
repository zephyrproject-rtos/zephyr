/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SCMI protocol generic functions and structures
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PROTOCOL_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PROTOCOL_H_

#include <zephyr/device.h>
#include <zephyr/drivers/firmware/scmi/util.h>
#include <stdint.h>
#include <errno.h>

#define SCMI_MAX_STR_SIZE        64
#define SCMI_SHORT_NAME_MAX_SIZE 16

/**
 * @brief SCMI message header definitions
 */
#define SCMI_MSG_ID_MASK             GENMASK(7, 0)
#define SCMI_MSG_XTRACT_ID(hdr)      (uint8_t)FIELD_GET(SCMI_MSG_ID_MASK, (hdr))
#define SCMI_MSG_TYPE_MASK           GENMASK(9, 8)
#define SCMI_MSG_XTRACT_TYPE(hdr)    (uint8_t)FIELD_GET(SCMI_MSG_TYPE_MASK, (hdr))
#define SCMI_MSG_TYPE_COMMAND        0
#define SCMI_MSG_TYPE_DELAYED_RESP   2
#define SCMI_MSG_TYPE_NOTIFICATION   3
#define SCMI_MSG_PROTOCOL_ID_MASK    GENMASK(17, 10)
#define SCMI_MSG_XTRACT_PROT_ID(hdr) (uint8_t)FIELD_GET(SCMI_MSG_PROTOCOL_ID_MASK, (hdr))
#define SCMI_MSG_TOKEN_ID_MASK       GENMASK(27, 18)
#define SCMI_MSG_XTRACT_TOKEN(hdr)   (uint16_t)FIELD_GET(SCMI_MSG_TOKEN_ID_MASK, (hdr))
#define SCMI_MSG_TOKEN_MAX           (SCMI_MSG_XTRACT_TOKEN(SCMI_MSG_TOKEN_ID_MASK) + 1)

/**
 * @brief Build an SCMI message header
 *
 * Builds an SCMI message header based on the
 * fields that make it up.
 *
 * @param id message ID
 * @param type message type
 * @param proto protocol ID
 * @param token message token
 */
#define SCMI_MESSAGE_HDR_MAKE(id, type, proto, token)                                              \
	(FIELD_PREP(SCMI_MSG_ID_MASK, id) | FIELD_PREP(SCMI_MSG_TYPE_MASK, type) |                 \
	 FIELD_PREP(SCMI_MSG_PROTOCOL_ID_MASK, proto) | FIELD_PREP(SCMI_MSG_TOKEN_ID_MASK, token))

struct scmi_channel;

/**
 * @brief SCMI message type
 */
enum scmi_message_type {
	/** command message */
	SCMI_COMMAND = 0x0,
	/** delayed reply message */
	SCMI_DELAYED_REPLY = 0x2,
	/** notification message */
	SCMI_NOTIFICATION = 0x3,
};

/**
 * @brief SCMI status codes
 */
enum scmi_status_code {
	SCMI_SUCCESS = 0,
	SCMI_NOT_SUPPORTED = -1,
	SCMI_INVALID_PARAMETERS = -2,
	SCMI_DENIED = -3,
	SCMI_NOT_FOUND = -4,
	SCMI_OUT_OF_RANGE = -5,
	SCMI_BUSY = -6,
	SCMI_COMMS_ERROR = -7,
	SCMI_GENERIC_ERROR = -8,
	SCMI_HARDWARE_ERROR = -9,
	SCMI_PROTOCOL_ERROR = -10,
	SCMI_IN_USE = -11,
};

/**
 * @brief SCMI protocol version
 *
 * Protocol versioning uses a 32-bit unsigned integer, where
 * - the upper 16 bits are the major revision;
 * - the lower 16 bits are the minor revision.
 *
 */
struct scmi_protocol_version {
	/** major protocol revision */
	uint16_t minor;
	/** minor protocol revision */
	uint16_t major;
};

/**
 * @struct scmi_protocol
 *
 * @brief SCMI protocol structure
 */
struct scmi_protocol {
	/** protocol ID */
	uint32_t id;
	/** TX channel */
	struct scmi_channel *tx;
	/** transport layer device */
	const struct device *transport;
	/** protocol private data */
	void *data;
};

/**
 * @struct scmi_message
 *
 * @brief SCMI message structure
 */
struct scmi_message {
	uint32_t hdr;
	uint32_t len;
	int32_t status;
	void *content;
};

/**
 * @brief Convert an SCMI status code to its Linux equivalent (if possible)
 *
 * @param scmi_status SCMI status code as shown in `enum scmi_status_code`
 *
 * @retval Linux equivalent status code
 */
int scmi_status_to_errno(int scmi_status);

/**
 * @brief Send an SCMI message and wait for its reply
 *
 * Blocking function used to send an SCMI message over
 * a given channel and wait for its reply
 *
 * @param proto pointer to SCMI protocol
 * @param msg pointer to SCMI message to send
 * @param reply pointer to SCMI message in which the reply is to be
 * written
 *
 * @retval 0 if successful
 * @retval negative errno if failure
 */
int scmi_send_message(struct scmi_protocol *proto,
		      struct scmi_message *msg, struct scmi_message *reply);

/**
 * @brief Get SCMI protocol version
 *
 * Generic function to get SCMI protocol version which is common operation for all protocols.
 *
 * @param proto pointer to SCMI protocol
 * @param ver pointer to SCMI protocol version structure
 *
 * @retval 0 if successful
 * @retval negative errno on failure
 */
int scmi_core_get_version(struct scmi_protocol *proto, struct scmi_protocol_version *ver);

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PROTOCOL_H_ */
