/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_write_utils.h"
#include "http_types.h"
#include "http_utils.h"
#include "config.h"

#include <net/nbuf.h>
#include <stdio.h>

#define HTTP_STATUS_200_OK	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"\r\n"

#define HTTP_401_STATUS_US	"HTTP/1.1 401 Unauthorized status\r\n" \
				"WWW-Authenticate: Basic realm=" \
				"\""HTTP_AUTH_REALM"\"\r\n\r\n"

#define HTML_HEADER		"<html><head>" \
				"<title>Zephyr HTTP Server</title>" \
				"</head><body><h1>" \
				"<center>Zephyr HTTP server</center></h1>\r\n"

#define HTML_FOOTER		"</body></html>\r\n"

/* Prints the received HTTP header fields as an HTML list */
static void print_http_headers(struct http_server_ctx *ctx,
			       char *str, uint16_t size)
{
	struct http_parser *parser = &ctx->parser;
	uint16_t offset = 0;

	offset = snprintf(str, size,
			  HTML_HEADER
			  "<h2>HTTP Header Fields</h2>\r\n<ul>\r\n");
	if (offset >= size) {
		return;
	}

	for (uint8_t i = 0; i < ctx->field_values_ctr; i++) {
		struct http_field_value *kv = &ctx->field_values[i];

		offset += snprintf(str + offset, size - offset,
				   "<li>%.*s: %.*s</li>\r\n",
				   kv->key_len, kv->key,
				   kv->value_len, kv->value);
		if (offset >= size) {
			return;
		}
	}

	offset += snprintf(str + offset, size - offset, "</ul>\r\n");
	if (offset >= size) {
		return;
	}

	offset += snprintf(str + offset, size - offset,
			   "<h2>HTTP Method: %s</h2>\r\n",
			   http_method_str(parser->method));
	if (offset >= size) {
		return;
	}

	offset += snprintf(str + offset, size - offset,
			   "<h2>URL: %.*s</h2>\r\n",
			   ctx->url_len, ctx->url);
	if (offset >= size) {
		return;
	}

	snprintf(str + offset, size - offset,
		 "<h2>Server: %s</h2>"HTML_FOOTER, CONFIG_ARCH);
}

#define HTTP_MAX_BODY_STR_SIZE		512
static char html_body[HTTP_MAX_BODY_STR_SIZE];

int http_response_header_fields(struct http_server_ctx *ctx)
{
	print_http_headers(ctx, html_body, HTTP_MAX_BODY_STR_SIZE);

	return http_response(ctx, HTTP_STATUS_200_OK, html_body);
}

int http_response_it_works(struct http_server_ctx *ctx)
{
	return http_response(ctx, HTTP_STATUS_200_OK, HTML_HEADER
			     "<body><h2><center>It Works!</center></h2>"
			     HTML_FOOTER);
}

int http_response_401(struct http_server_ctx *ctx)
{
	return http_response(ctx, HTTP_401_STATUS_US, NULL);
}

int http_response_soft_404(struct http_server_ctx *ctx)
{
	return http_response(ctx, HTTP_STATUS_200_OK, HTML_HEADER
			     "<h2><center>404 Not Found</center></h2>"
			     HTML_FOOTER);
}

int http_response_auth(struct http_server_ctx *ctx)
{
	return http_response(ctx, HTTP_STATUS_200_OK,
			     HTML_HEADER"<h2><center>user: "HTTP_AUTH_USERNAME
			     ", password: "HTTP_AUTH_PASSWORD"</center></h2>"
			     HTML_FOOTER);
}
