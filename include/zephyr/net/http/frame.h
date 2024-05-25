/*
 * Copyright (c) 2023, Emna Rekik
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief HTTP2 frame information.
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_SERVER_FRAME_H_
#define ZEPHYR_INCLUDE_NET_HTTP_SERVER_FRAME_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** HTTP2 frame types */
enum http_frame_type {
	/** Data frame */
	HTTP_SERVER_DATA_FRAME = 0x00,
	/** Headers frame */
	HTTP_SERVER_HEADERS_FRAME = 0x01,
	/** Priority frame */
	HTTP_SERVER_PRIORITY_FRAME = 0x02,
	/** Reset stream frame */
	HTTP_SERVER_RST_STREAM_FRAME = 0x03,
	/** Settings frame */
	HTTP_SERVER_SETTINGS_FRAME = 0x04,
	/** Push promise frame */
	HTTP_SERVER_PUSH_PROMISE_FRAME = 0x05,
	/** Ping frame */
	HTTP_SERVER_PING_FRAME = 0x06,
	/** Goaway frame */
	HTTP_SERVER_GOAWAY_FRAME = 0x07,
	/** Window update frame */
	HTTP_SERVER_WINDOW_UPDATE_FRAME = 0x08,
	/** Continuation frame */
	HTTP_SERVER_CONTINUATION_FRAME = 0x09
};

/** @cond INTERNAL_HIDDEN */

#define HTTP_SERVER_HPACK_METHOD 0
#define HTTP_SERVER_HPACK_PATH   1

#define HTTP_SERVER_FLAG_SETTINGS_ACK 0x1
#define HTTP_SERVER_FLAG_END_HEADERS  0x4
#define HTTP_SERVER_FLAG_END_STREAM   0x1

#define HTTP_SERVER_FRAME_HEADER_SIZE      9
#define HTTP_SERVER_FRAME_LENGTH_OFFSET    0
#define HTTP_SERVER_FRAME_TYPE_OFFSET      3
#define HTTP_SERVER_FRAME_FLAGS_OFFSET     4
#define HTTP_SERVER_FRAME_STREAM_ID_OFFSET 5

/** @endcond */

/** HTTP2 settings field */
struct http_settings_field {
	uint16_t id;    /**< Field id */
	uint32_t value; /**< Field value */
} __packed;

/** @brief HTTP2 settings */
enum http_settings {
	/** Header table size */
	HTTP_SETTINGS_HEADER_TABLE_SIZE = 1,
	/** Enable push */
	HTTP_SETTINGS_ENABLE_PUSH = 2,
	/** Maximum number of concurrent streams */
	HTTP_SETTINGS_MAX_CONCURRENT_STREAMS = 3,
	/** Initial window size */
	HTTP_SETTINGS_INITIAL_WINDOW_SIZE = 4,
	/** Max frame size */
	HTTP_SETTINGS_MAX_FRAME_SIZE = 5,
	/** Max header list size */
	HTTP_SETTINGS_MAX_HEADER_LIST_SIZE = 6,
};

#ifdef __cplusplus
}
#endif

#endif
