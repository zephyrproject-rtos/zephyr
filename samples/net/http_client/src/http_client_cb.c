/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_client_cb.h"
#include "http_client_types.h"

#include <stdlib.h>
#include <stdio.h>

#define MAX_NUM_DIGITS	16

int on_url(struct http_parser *parser, const char *at, size_t length)
{
	ARG_UNUSED(parser);

	printf("URL: %.*s\n", (int)length, at);

	return 0;
}

int on_status(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx;
	uint16_t len;

	ARG_UNUSED(parser);

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);
	len = min(length, sizeof(ctx->http_status) - 1);
	memcpy(ctx->http_status, at, len);
	ctx->http_status[len] = 0;

	return 0;
}

int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	char *content_len = "Content-Length";
	struct http_client_ctx *ctx;
	uint16_t len;

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);

	len = strlen(content_len);
	if (length >= len && memcmp(at, content_len, len) == 0) {
		ctx->cl_present = 1;
	}

	printf("%.*s: ", (int)length, at);

	return 0;
}

int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx;
	char str[MAX_NUM_DIGITS];

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);

	if (ctx->cl_present) {
		if (length <= MAX_NUM_DIGITS - 1) {
			long int num;

			memcpy(str, at, length);
			str[length] = 0;
			num = strtol(str, NULL, 10);
			if (num == LONG_MIN || num == LONG_MAX) {
				return -EINVAL;
			}

			ctx->content_length = num;
		}

		ctx->cl_present = 0;
	}

	printf("%.*s\n", (int)length, at);

	return 0;
}

int on_body(struct http_parser *parser, const char *at, size_t length)
{
	struct http_client_ctx *ctx;

	ctx = CONTAINER_OF(parser, struct http_client_ctx, parser);

	ctx->body_found = 1;
	ctx->processed += length;

	return 0;
}

int on_headers_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

int on_message_begin(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	printf("\n--------- HTTP response (headers) ---------\n");

	return 0;
}

int on_message_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

int on_chunk_header(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

int on_chunk_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}
