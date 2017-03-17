/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_TYPES_H_
#define _HTTP_TYPES_H_

#include <net/net_context.h>
#include <net/http.h>

/* Max number of HTTP header field-value pairs */
#define HTTP_PARSER_MAX_FIELD_VALUES	CONFIG_HTTP_HEADER_FIELD_ITEMS

/* Max number of HTTP server context available to this app */
#define HTTP_MAX_NUMBER_SERVER_CTX	CONFIG_NET_MAX_CONTEXTS

/* Max number of URLs this server will handle */
#define HTTP_MAX_NUMBER_URL		16

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
