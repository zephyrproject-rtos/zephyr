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
#define SCMI_MESSAGE_HDR_MAKE(id, type, proto, token)	\
	(SCMI_FIELD_MAKE(id, GENMASK(7, 0), 0)     |	\
	 SCMI_FIELD_MAKE(type, GENMASK(1, 0), 8)   |	\
	 SCMI_FIELD_MAKE(proto, GENMASK(7, 0), 10) |	\
	 SCMI_FIELD_MAKE(token, GENMASK(9, 0), 18))

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
 * @param use_polling Specifies the communication mechanism used by the scmi
 * platform to interact with agents.
 * - true: Polling mode — the platform actively checks the message status
 *   to determine if it has been processed
 * - false: Interrupt mode — the platform relies on SCMI interrupts to
 *   detect when a message has been handled.
 */
int scmi_send_message(struct scmi_protocol *proto,
		      struct scmi_message *msg, struct scmi_message *reply,
		      bool use_polling);

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PROTOCOL_H_ */
