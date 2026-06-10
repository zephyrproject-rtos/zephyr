/*
 * Copyright 2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @ingroup scmi_interface
 * @brief Header file for the SCMI (System Control and Management Interface) driver API.
 */

#ifndef _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PROTOCOL_H_
#define _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PROTOCOL_H_

/**
 * @brief Interfaces for ARM System Control and Management Interface (SCMI)
 * @defgroup scmi_interface SCMI
 * @ingroup io_interfaces
 * @{
 */

#include <zephyr/device.h>
#include <zephyr/drivers/firmware/scmi/util.h>
#include <stdint.h>
#include <errno.h>

/** @brief Maximum size of strings to describe SCMI names, including NUL terminator. */
#define SCMI_SHORT_NAME_MAX_SIZE 16

/**
 * @brief Build an SCMI message header
 *
 * Builds an SCMI message header based on the fields that make it up.
 * The resulting 32-bit header is laid out as follows:
 *
 * - bits [7:0]   message ID
 * - bits [9:8]   message type (see @ref scmi_message_type)
 * - bits [17:10] protocol ID
 * - bits [27:18] message token
 *
 * @param id message ID
 * @param type message type
 * @param proto protocol ID
 * @param token message token
 */
#define SCMI_MESSAGE_HDR_MAKE(id, type, proto, token)                                              \
	(SCMI_FIELD_MAKE(id, GENMASK(7, 0), 0) | SCMI_FIELD_MAKE(type, GENMASK(1, 0), 8) |         \
	 SCMI_FIELD_MAKE(proto, GENMASK(7, 0), 10) | SCMI_FIELD_MAKE(token, GENMASK(9, 0), 18))

struct scmi_channel;

/**
 * @brief SCMI message type
 */
enum scmi_message_type {
	/** Command message */
	SCMI_COMMAND = 0x0,
	/** Delayed reply message */
	SCMI_DELAYED_REPLY = 0x2,
	/** Notification message */
	SCMI_NOTIFICATION = 0x3,
};

/**
 * @brief SCMI status codes
 */
enum scmi_status_code {
	/** Successful completion */
	SCMI_SUCCESS = 0,
	/** The command is not supported */
	SCMI_NOT_SUPPORTED = -1,
	/** One or more parameters are invalid */
	SCMI_INVALID_PARAMETERS = -2,
	/** The caller is not permitted to perform the operation */
	SCMI_DENIED = -3,
	/** The entity referenced by the command was not found */
	SCMI_NOT_FOUND = -4,
	/** A parameter or value is out of the supported range */
	SCMI_OUT_OF_RANGE = -5,
	/** The platform is busy and cannot service the command */
	SCMI_BUSY = -6,
	/** A communication error occurred */
	SCMI_COMMS_ERROR = -7,
	/** An unspecified error occurred */
	SCMI_GENERIC_ERROR = -8,
	/** A hardware error occurred */
	SCMI_HARDWARE_ERROR = -9,
	/** A protocol error (e.g. malformed message) occurred */
	SCMI_PROTOCOL_ERROR = -10,
	/** The resource is already in use */
	SCMI_IN_USE = -11,
};

/**
 * @struct scmi_protocol
 *
 * @brief SCMI protocol structure
 */
struct scmi_protocol {
	/** Protocol ID */
	uint32_t id;
	/** TX channel */
	struct scmi_channel *tx;
	/** Transport layer device */
	const struct device *transport;
	/** Protocol private data */
	void *data;
	/** Protocol supported version */
	uint32_t version;
};

/**
 * @struct scmi_protocol_version
 *
 * @brief SCMI protocol version
 *
 * Protocol versioning uses a 32-bit unsigned integer, where
 * - the upper 16 bits are the major revision;
 * - the lower 16 bits are the minor revision.
 *
 */
struct scmi_protocol_version {
	/** @brief Access to protocol version bits. */
	union {
		/** Raw 32-bit protocol version value */
		uint32_t raw;
		struct {
			/** Minor protocol revision */
			uint16_t minor;
			/** Major protocol revision */
			uint16_t major;
		};
	};
};

/**
 * @struct scmi_message
 *
 * @brief SCMI message structure
 */
struct scmi_message {
	/** Message header (see @ref SCMI_MESSAGE_HDR_MAKE) */
	uint32_t hdr;
	/** Size in bytes of the data pointed to by @ref content */
	uint32_t len;
	/** Pointer to the message payload (parameters or return values) */
	void *content;
};

/**
 * @brief Convert an SCMI status code to its Linux equivalent (if possible)
 *
 * @param scmi_status SCMI status code as shown in `enum scmi_status_code`
 *
 * @return Linux (errno) equivalent of the given SCMI status code
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
 * @param use_polling Specifies the communication mechanism used by the scmi
 * platform to interact with agents.
 *
 * - true: Polling mode — the platform actively checks the message status
 *   to determine if it has been processed
 * - false: Interrupt mode — the platform relies on SCMI interrupts to
 *   detect when a message has been handled.
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_send_message(struct scmi_protocol *proto, struct scmi_message *msg,
		      struct scmi_message *reply, bool use_polling);

/**
 * @brief Get protocol version
 *
 * @param proto Protocol instance
 * @param version Pointer to store protocol version
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_protocol_get_version(struct scmi_protocol *proto, uint32_t *version);

/**
 * @brief Get protocol attributes
 *
 * @param proto Protocol instance
 * @param attributes Pointer to store protocol attributes
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_protocol_attributes_get(struct scmi_protocol *proto, uint32_t *attributes);

/**
 * @brief Get protocol message attributes
 *
 * @param proto Protocol instance
 * @param message_id Message ID to query
 * @param attributes Pointer to store message attributes
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_protocol_message_attributes_get(struct scmi_protocol *proto, uint32_t message_id,
					 uint32_t *attributes);

/**
 * @brief Negotiate protocol version
 *
 * @param proto Protocol instance
 * @param version Desired protocol version
 *
 * @return 0 on success, negative errno value on failure.
 */
int scmi_protocol_version_negotiate(struct scmi_protocol *proto, uint32_t version);

/**
 * @}
 */

/**
 * @brief Standard SCMI Protocol definitions
 * @defgroup scmi_protocols Protocols
 * @ingroup scmi_interface
 */

#endif /* _INCLUDE_ZEPHYR_DRIVERS_FIRMWARE_SCMI_PROTOCOL_H_ */
