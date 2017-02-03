/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_TYPES_H_
#define _HTTP_TYPES_H_

#include <net/net_context.h>
#include <net/http_parser.h>

/* Max number of HTTP header field-value pairs */
#define HTTP_PARSER_MAX_FIELD_VALUES	10

/* Max number of HTTP server context available to this app */
#define HTTP_MAX_NUMBER_SERVER_CTX	16

/* Max number of URLs this server will handle */
#define HTTP_MAX_NUMBER_URL		16

/**
 * @brief HTTP_CTX_STATE enum	HTTP context state
 */
enum HTTP_CTX_STATE {
	HTTP_CTX_FREE = 0,
	HTTP_CTX_IN_USE
};

/**
 * @brief http_field_value	HTTP header fields struct
 */
struct http_field_value {
	/** Field name, this variable will point to the beginning of the string
	 *  containing the HTTP field name
	 */
	const char *key;
	/** Value, this variable will point to the beginning of the string
	 *  containing the field's value
	 */
	const char *value;

	/** Length of the field name */
	uint16_t key_len;
	/** Length of the field's value */
	uint16_t value_len;
};

/**
 * @brief http_server_ctx	The HTTP server context struct
 */
struct http_server_ctx {

	uint8_t used;

	/** Collection of header fields */
	struct http_field_value field_values[HTTP_PARSER_MAX_FIELD_VALUES];
	/** Number of header field elements */
	uint16_t field_values_ctr;

	/** HTTP Request URL */
	const char *url;
	/** URL's length */
	uint16_t url_len;

	/** HTTP Parser settings */
	struct http_parser_settings parser_settings;
	/** The HTTP Parser struct */
	struct http_parser parser;

	/**IP stack network context */
	struct net_context *net_ctx;
};

enum HTTP_URL_FLAGS {
	HTTP_URL_STANDARD = 0
};

/**
 * @brief http_root_url		HTTP root URL struct, used for pattern matching
 */
struct http_root_url {
	const char *root;
	uint16_t root_len;

	uint8_t flags;
	int (*write_cb)(struct http_server_ctx *http_ctx);
};

/**
 * @brief http_url_ctx		Collection of URLs that this server will handle
 */
struct http_url_ctx {
	struct http_root_url urls[HTTP_MAX_NUMBER_URL];
	uint8_t urls_ctr;
};

#endif
