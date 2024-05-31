/** @file
 * @brief HTTP HPACK
 */

/*
 * Copyright (c) 2023 Emna Rekik
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NET_HTTP_SERVER_HPACK_H_
#define ZEPHYR_INCLUDE_NET_HTTP_SERVER_HPACK_H_

#include <stddef.h>
#include <stdint.h>

/**
 * @brief HTTP HPACK
 * @defgroup http_hpack HTTP HPACK
 * @ingroup networking
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** @cond INTERNAL_HIDDEN */

enum http_hpack_static_key {
	HTTP_SERVER_HPACK_INVALID = 0,
	HTTP_SERVER_HPACK_AUTHORITY = 1,
	HTTP_SERVER_HPACK_METHOD_GET = 2,
	HTTP_SERVER_HPACK_METHOD_POST = 3,
	HTTP_SERVER_HPACK_PATH_ROOT = 4,
	HTTP_SERVER_HPACK_PATH_INDEX = 5,
	HTTP_SERVER_HPACK_SCHEME_HTTP = 6,
	HTTP_SERVER_HPACK_SCHEME_HTTPS = 7,
	HTTP_SERVER_HPACK_STATUS_200 = 8,
	HTTP_SERVER_HPACK_STATUS_204 = 9,
	HTTP_SERVER_HPACK_STATUS_206 = 10,
	HTTP_SERVER_HPACK_STATUS_304 = 11,
	HTTP_SERVER_HPACK_STATUS_400 = 12,
	HTTP_SERVER_HPACK_STATUS_404 = 13,
	HTTP_SERVER_HPACK_STATUS_500 = 14,
	HTTP_SERVER_HPACK_ACCEPT_CHARSET = 15,
	HTTP_SERVER_HPACK_ACCEPT_ENCODING = 16,
	HTTP_SERVER_HPACK_ACCEPT_LANGUAGE = 17,
	HTTP_SERVER_HPACK_ACCEPT_RANGES = 18,
	HTTP_SERVER_HPACK_ACCEPT = 19,
	HTTP_SERVER_HPACK_ACCESS_CONTROL_ALLOW_ORIGIN = 20,
	HTTP_SERVER_HPACK_AGE = 21,
	HTTP_SERVER_HPACK_ALLOW = 22,
	HTTP_SERVER_HPACK_AUTHORIZATION = 23,
	HTTP_SERVER_HPACK_CACHE_CONTROL = 24,
	HTTP_SERVER_HPACK_CONTENT_DISPOSITION = 25,
	HTTP_SERVER_HPACK_CONTENT_ENCODING = 26,
	HTTP_SERVER_HPACK_CONTENT_LANGUAGE = 27,
	HTTP_SERVER_HPACK_CONTENT_LENGTH = 28,
	HTTP_SERVER_HPACK_CONTENT_LOCATION = 29,
	HTTP_SERVER_HPACK_CONTENT_RANGE = 30,
	HTTP_SERVER_HPACK_CONTENT_TYPE = 31,
	HTTP_SERVER_HPACK_COOKIE = 32,
	HTTP_SERVER_HPACK_DATE = 33,
	HTTP_SERVER_HPACK_ETAG = 34,
	HTTP_SERVER_HPACK_EXPECT = 35,
	HTTP_SERVER_HPACK_EXPIRES = 36,
	HTTP_SERVER_HPACK_FROM = 37,
	HTTP_SERVER_HPACK_HOST = 38,
	HTTP_SERVER_HPACK_IF_MATCH = 39,
	HTTP_SERVER_HPACK_IF_MODIFIED_SINCE = 40,
	HTTP_SERVER_HPACK_IF_NONE_MATCH = 41,
	HTTP_SERVER_HPACK_IF_RANGE = 42,
	HTTP_SERVER_HPACK_IF_UNMODIFIED_SINCE = 43,
	HTTP_SERVER_HPACK_LAST_MODIFIED = 44,
	HTTP_SERVER_HPACK_LINK = 45,
	HTTP_SERVER_HPACK_LOCATION = 46,
	HTTP_SERVER_HPACK_MAX_FORWARDS = 47,
	HTTP_SERVER_HPACK_PROXY_AUTHENTICATE = 48,
	HTTP_SERVER_HPACK_PROXY_AUTHORIZATION = 49,
	HTTP_SERVER_HPACK_RANGE = 50,
	HTTP_SERVER_HPACK_REFERER = 51,
	HTTP_SERVER_HPACK_REFRESH = 52,
	HTTP_SERVER_HPACK_RETRY_AFTER = 53,
	HTTP_SERVER_HPACK_SERVER = 54,
	HTTP_SERVER_HPACK_SET_COOKIE = 55,
	HTTP_SERVER_HPACK_STRICT_TRANSPORT_SECURITY = 56,
	HTTP_SERVER_HPACK_TRANSFER_ENCODING = 57,
	HTTP_SERVER_HPACK_USER_AGENT = 58,
	HTTP_SERVER_HPACK_VARY = 59,
	HTTP_SERVER_HPACK_VIA = 60,
	HTTP_SERVER_HPACK_WWW_AUTHENTICATE = 61,
};

/* TODO Kconfig */
#define HTTP2_HEADER_FIELD_MAX_LEN 256

#if defined(CONFIG_HTTP_SERVER)
#define HTTP_SERVER_HUFFMAN_DECODE_BUFFER_SIZE CONFIG_HTTP_SERVER_HUFFMAN_DECODE_BUFFER_SIZE
#else
#define HTTP_SERVER_HUFFMAN_DECODE_BUFFER_SIZE 0
#endif

/** @endcond */

/** HTTP2 header field with decoding buffer. */
struct http_hpack_header_buf {
	/** A pointer to the decoded header field name. */
	const char *name;

	/** A pointer to the decoded header field value. */
	const char *value;

	/** Length of the decoded header field name. */
	size_t name_len;

	/** Length of the decoded header field value. */
	size_t value_len;

	/** Encoding/Decoding buffer. Used with Huffman encoding/decoding. */
	uint8_t buf[HTTP_SERVER_HUFFMAN_DECODE_BUFFER_SIZE];

	/** Length of the data in the decoding buffer. */
	size_t datalen;
};

/** @cond INTERNAL_HIDDEN */

int http_hpack_huffman_decode(const uint8_t *encoded_buf, size_t encoded_len,
			      uint8_t *buf, size_t buflen);
int http_hpack_huffman_encode(const uint8_t *str, size_t str_len,
			      uint8_t *buf, size_t buflen);
int http_hpack_decode_header(const uint8_t *buf, size_t datalen,
			     struct http_hpack_header_buf *header);
int http_hpack_encode_header(uint8_t *buf, size_t buflen,
			     struct http_hpack_header_buf *header);

/** @endcond */

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif
