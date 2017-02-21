/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __HTTP_H__
#define __HTTP_H__

#if defined(CONFIG_HTTP_CLIENT)

#include <net/net_context.h>

struct http_client_request {
	/** The HTTP method: GET, HEAD, OPTIONS, POST */
	char *method;
	/** The URL for this request, for example: /index.html */
	char *url;
	/** The HTTP protocol: HTTP/1.1 */
	char *protocol;
	/** The HTTP header fields (application specific)
	 * The Content-Type may be specified here or in the next field.
	 * Depending on your application, the Content-Type may vary, however
	 * some header fields may remain constant through the application's
	 * life cycle.
	 */
	char *header_fields;
	/** The value of the Content-Type header field, may be NULL */
	char *content_type_value;
	/** Payload, may be NULL */
	char *payload;
	/** Payload size, may be 0 */
	uint16_t payload_size;
};

int http_request(struct net_context *net_ctx, int32_t timeout,
		 struct http_client_request *req);

int http_request_get(struct net_context *net_ctx, int32_t timeout, char *url,
		     char *header_fields);

int http_request_head(struct net_context *net_ctx, int32_t timeout, char *url,
		      char *header_fields);

int http_request_options(struct net_context *net_ctx, int32_t timeout,
			 char *url, char *header_fields);

int http_request_post(struct net_context *net_ctx, int32_t timeout, char *url,
		      char *header_fields, char *content_type_value,
		      char *payload);
#endif

#if defined(CONFIG_HTTP_SERVER)

#include <net/net_context.h>

#if defined(CONFIG_HTTP_PARSER)
#include <net/http_parser.h>
#endif

/* HTTP server context state */
enum HTTP_CTX_STATE {
	HTTP_CTX_FREE = 0,
	HTTP_CTX_IN_USE
};

/* HTTP header fields struct */
struct http_field_value {
	/** Field name, this variable will point to the beginning of the string
	 *  containing the HTTP field name
	 */
	const char *key;
	/** Length of the field name */
	uint16_t key_len;

	/** Value, this variable will point to the beginning of the string
	 *  containing the field value
	 */
	const char *value;
	/** Length of the field value */
	uint16_t value_len;
};

/* The HTTP server context struct */
struct http_server_ctx {
	uint8_t state;

	/** Collection of header fields */
	struct http_field_value field_values[CONFIG_HTTP_HEADER_FIELD_ITEMS];
	/** Number of header field elements */
	uint16_t field_values_ctr;

	/** HTTP Request URL */
	const char *url;
	/** URL's length */
	uint16_t url_len;

	/**IP stack network context */
	struct net_context *net_ctx;

	/** Network timeout */
	int32_t timeout;

#if defined(CONFIG_HTTP_PARSER)
	/** HTTP parser */
	struct http_parser parser;
	/** HTTP parser settings */
	struct http_parser_settings parser_settings;
#endif
};

int http_response(struct http_server_ctx *ctx, const char *http_header,
		  const char *html_payload);

int http_response_400(struct http_server_ctx *ctx, const char *html_payload);

int http_response_403(struct http_server_ctx *ctx, const char *html_payload);

int http_response_404(struct http_server_ctx *ctx, const char *html_payload);

#endif

#endif
