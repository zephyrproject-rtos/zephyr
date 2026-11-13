/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief HTTP2 frame information.
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_FRAME_H_
#define ZEPHYR_INCLUDE_NET_HTTP_FRAME_H_

#include <stdint.h>

#include <zephyr/toolchain.h>

#ifdef __cplusplus
extern "C" {
#endif

/** HTTP2 frame types */
enum http2_frame_type {
	/** Data frame */
	HTTP2_DATA_FRAME = 0x00,
	/** Headers frame */
	HTTP2_HEADERS_FRAME = 0x01,
	/** Priority frame */
	HTTP2_PRIORITY_FRAME = 0x02,
	/** Reset stream frame */
	HTTP2_RST_STREAM_FRAME = 0x03,
	/** Settings frame */
	HTTP2_SETTINGS_FRAME = 0x04,
	/** Push promise frame */
	HTTP2_PUSH_PROMISE_FRAME = 0x05,
	/** Ping frame */
	HTTP2_PING_FRAME = 0x06,
	/** Goaway frame */
	HTTP2_GOAWAY_FRAME = 0x07,
	/** Window update frame */
	HTTP2_WINDOW_UPDATE_FRAME = 0x08,
	/** Continuation frame */
	HTTP2_CONTINUATION_FRAME = 0x09
};

/** @cond INTERNAL_HIDDEN */

enum http2_error {
	HTTP2_ERROR_NO_ERROR = 0x00,
	HTTP2_ERROR_PROTOCOL_ERROR = 0x01,
	HTTP2_ERROR_INTERNAL_ERROR = 0x02,
	HTTP2_ERROR_FLOW_CONTROL_ERROR = 0x03,
	HTTP2_ERROR_SETTINGS_TIMEOUT = 0x04,
	HTTP2_ERROR_STREAM_CLOSED = 0x05,
	HTTP2_ERROR_FRAME_SIZE_ERROR = 0x06,
	HTTP2_ERROR_REFUSED_STREAM = 0x07,
	HTTP2_ERROR_CANCEL = 0x08,
	HTTP2_ERROR_COMPRESSION_ERROR = 0x09,
	HTTP2_ERROR_CONNECT_ERROR = 0x0a,
	HTTP2_ERROR_ENHANCE_YOUR_CALM = 0x0b,
	HTTP2_ERROR_INADEQUATE_SECURITY = 0x0c,
	HTTP2_ERROR_HTTP_1_1_REQUIRED = 0x0d,
};

#define HTTP2_FLAG_ACK          0x01
#define HTTP2_FLAG_SETTINGS_ACK 0x01
#define HTTP2_FLAG_END_HEADERS  0x04
#define HTTP2_FLAG_END_STREAM   0x01
#define HTTP2_FLAG_PADDED       0x08
#define HTTP2_FLAG_PRIORITY     0x20

#define HTTP2_FRAME_HEADER_SIZE      9
#define HTTP2_FRAME_LENGTH_OFFSET    0
#define HTTP2_FRAME_TYPE_OFFSET      3
#define HTTP2_FRAME_FLAGS_OFFSET     4
#define HTTP2_FRAME_STREAM_ID_OFFSET 5
#define HTTP2_FRAME_STREAM_ID_MASK   0x7FFFFFFF

#define HTTP2_HEADERS_FRAME_PRIORITY_LEN 5
#define HTTP2_PRIORITY_FRAME_LEN 5
#define HTTP2_RST_STREAM_FRAME_LEN 4
#define HTTP2_GOAWAY_FRAME_LEN 8
#define HTTP2_PING_FRAME_LEN 8
#define HTTP2_WINDOW_UPDATE_FRAME_LEN 4

/** @endcond */

/** @brief HTTP/2 frame representation. */
struct http2_frame {
	uint32_t length; /**< Frame payload length. */
	uint32_t stream_identifier; /**< Stream ID the frame belongs to. */
	uint8_t type; /**< Frame type. */
	uint8_t flags; /**< Frame flags. */
	uint8_t padding_len; /**< Frame padding length. */
};

/** HTTP2 settings field */
struct http2_settings_field {
	uint16_t id;    /**< Field id */
	uint32_t value; /**< Field value */
} __packed;

/** @brief HTTP2 settings */
enum http2_settings {
	/** Header table size */
	HTTP2_SETTINGS_HEADER_TABLE_SIZE = 1,
	/** Enable push */
	HTTP2_SETTINGS_ENABLE_PUSH = 2,
	/** Maximum number of concurrent streams */
	HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS = 3,
	/** Initial window size */
	HTTP2_SETTINGS_INITIAL_WINDOW_SIZE = 4,
	/** Max frame size */
	HTTP2_SETTINGS_MAX_FRAME_SIZE = 5,
	/** Max header list size */
	HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE = 6,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NET_HTTP_FRAME_H_ */
