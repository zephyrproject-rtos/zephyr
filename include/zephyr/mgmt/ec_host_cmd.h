/*
 * Copyright (c) 2020 Google LLC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_H_
#define ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_H_

/**
 * @brief EC Host Command Interface
 * @defgroup ec_host_cmd_periph_interface EC Host Command Interface
 * @ingroup io_interfaces
 * @{
 */

#include <stdint.h>

/**
 * @brief Arguments passed into every installed host command handler
 */
struct ec_host_cmd_handler_args {
	/** The incoming data that can be cast to the handlers request type. */
	const void *const input_buf;
	/** The number of valid bytes that can be read from @a input_buf. */
	const uint16_t input_buf_size;
	/** The data written to this buffer will be send to the host. */
	void *const output_buf;
	/** [in/out] Upon entry, this is the maximum number of bytes that can
	 *  be written to the @a output_buf. Upon exit, this should be
	 *  the number of bytes of @a output_buf to send to the host.
	 */
	uint16_t output_buf_size;
	/** The version of the host command that is being requested. This will
	 *  be a value that has been static registered as valid for the handler.
	 */
	const uint8_t version;
};

typedef enum ec_host_cmd_status (*ec_host_cmd_handler_cb)(
	struct ec_host_cmd_handler_args *args);
/**
 * @brief Structure use for statically registering host command handlers
 */
struct ec_host_cmd_handler {
	/** Callback routine to process commands that match @a id. */
	ec_host_cmd_handler_cb handler;
	/** The numerical command id used as the lookup for commands. */
	uint16_t id;
	/** The bitfield of all versions that the @a handler supports, where
	 *  each bit value represents that the @a handler supports that version.
	 *  E.g. BIT(0) corresponds to version 0.
	 */
	uint16_t version_mask;
	/** The minimum @a input_buf_size enforced by the framework before
	 *  passing to the handler.
	 */
	uint16_t min_rqt_size;
	/** The minimum @a output_buf_size enforced by the framework before
	 *  passing to the handler.
	 */
	uint16_t min_rsp_size;
};

/**
 *
 * @brief Statically define and register a host command handler.
 *
 * Helper macro to statically define and register a host command handler that
 * has a compile-time-fixed sizes for its both request and response structures.
 *
 * @param _function Name of handler function.
 * @param _id Id of host command to handle request for.
 * @param _version_mask The bitfield of all versions that the @a _function
 *        supports. E.g. BIT(0) corresponds to version 0.
 * @param _request_type The datatype of the request parameters for @a _function.
 * @param _response_type The datatype of the response parameters for
 *        @a _function.
 */
#define EC_HOST_CMD_HANDLER(_function, _id, _version_mask, _request_type,      \
			    _response_type)                                    \
	const STRUCT_SECTION_ITERABLE(ec_host_cmd_handler, __cmd##_id) = {     \
		.id = _id,                                                     \
		.handler = _function,                                          \
		.version_mask = _version_mask,                                 \
		.min_rqt_size = sizeof(_request_type),                         \
		.min_rsp_size = sizeof(_response_type),                        \
	}

/**
 *
 * @brief Statically define and register a host command handler without sizes.
 *
 * Helper macro to statically define and register a host command handler whose
 * request or response structure size is not known as compile time.
 *
 * @param _function Name of handler function.
 * @param _id Id of host command to handle request for.
 * @param _version_mask The bitfield of all versions that the @a _function
 *        supports. E.g. BIT(0) corresponds to version 0.
 */
#define EC_HOST_CMD_HANDLER_UNBOUND(_function, _id, _version_mask)             \
	const STRUCT_SECTION_ITERABLE(ec_host_cmd_handler, __cmd##_id) = {     \
		.id = _id,                                                     \
		.handler = _function,                                          \
		.version_mask = _version_mask,                                 \
		.min_rqt_size = 0,                                             \
		.min_rsp_size = 0,                                             \
	}

/**
 * @brief Header for requests from host to embedded controller
 *
 * Represent the over-the-wire header in LE format for host command requests.
 * This represent version 3 of the host command header. The requests are always
 * sent from host to embedded controller.
 */
struct ec_host_cmd_request_header {
	/** Should be 3. The EC will return EC_HOST_CMD_INVALID_HEADER if it
	 *  receives a header with a version it doesn't know how to parse.
	 */
	uint8_t prtcl_ver;
	/** Checksum of response and data; sum of all bytes including checksum.
	 *  Should total to 0.
	 */
	uint8_t checksum;
	/** Id of command that is being sent. */
	uint16_t cmd_id;
	/** Version of the specific @a cmd_id being requested. Valid
	 *  versions start at 0.
	 */
	uint8_t cmd_ver;
	/** Unused byte in current protocol version; set to 0. */
	uint8_t reserved;
	/** Length of data which follows this header. */
	uint16_t data_len;
} __packed;

/**
 * @brief Header for responses from embedded controller to host
 *
 * Represent the over-the-wire header in LE format for host command responses.
 * This represent version 3 of the host command header. Responses are always
 * sent from embedded controller to host.
 */
struct ec_host_cmd_response_header {
	/** Should be 3. */
	uint8_t prtcl_ver;
	/** Checksum of response and data; sum of all bytes including checksum.
	 *  Should total to 0.
	 */
	uint8_t checksum;
	/** A @a ec_host_cmd_status response code for specific command. */
	uint16_t result;
	/** Length of data which follows this header. */
	uint16_t data_len;
	/** Unused bytes in current protocol version; set to 0. */
	uint16_t reserved;
} __packed;

/*
 * Host command response codes (16-bit).
 */
enum ec_host_cmd_status {
	/** Host command was successful. */
	EC_HOST_CMD_SUCCESS = 0,
	/** The specified command id is not recognized or supported. */
	EC_HOST_CMD_INVALID_COMMAND = 1,
	/** Generic Error. */
	EC_HOST_CMD_ERROR = 2,
	/** One of more of the input request parameters is invalid. */
	EC_HOST_CMD_INVALID_PARAM = 3,
	/** Host command is not permitted. */
	EC_HOST_CMD_ACCESS_DENIED = 4,
	/** Response was invalid (e.g. not version 3 of header). */
	EC_HOST_CMD_INVALID_RESPONSE = 5,
	/** Host command id version unsupported. */
	EC_HOST_CMD_INVALID_VERSION = 6,
	/** Checksum did not match */
	EC_HOST_CMD_INVALID_CHECKSUM = 7,
	/** A host command is currently being processed. */
	EC_HOST_CMD_IN_PROGRESS = 8,
	/** Requested information is currently unavailable. */
	EC_HOST_CMD_UNAVAILABLE = 9,
	/** Timeout during processing. */
	EC_HOST_CMD_TIMEOUT = 10,
	/** Data or table overflow. */
	EC_HOST_CMD_OVERFLOW = 11,
	/** Header is invalid or unsupported (e.g. not version 3 of header). */
	EC_HOST_CMD_INVALID_HEADER = 12,
	/** Did not receive all expected request data. */
	EC_HOST_CMD_REQUEST_TRUNCATED = 13,
	/** Response was too big to send within one response packet. */
	EC_HOST_CMD_RESPONSE_TOO_BIG = 14,
	/** Error on underlying communication bus. */
	EC_HOST_CMD_BUS_ERROR = 15,
	/** System busy. Should retry later. */
	EC_HOST_CMD_BUSY = 16,

	EC_HOST_CMD_MAX = UINT16_MAX /* Force enum to be 16 bits */
} __packed;

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_MGMT_EC_HOST_CMD_H_ */
