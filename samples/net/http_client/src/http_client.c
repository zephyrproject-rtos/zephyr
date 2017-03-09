/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_client.h"
#include "http_client_rcv.h"
#include "http_client_cb.h"
#include "config.h"

#include <misc/printk.h>
#include <net/nbuf.h>

int http_init(struct http_client_ctx *http_ctx)
{
	memset(http_ctx, 0, sizeof(struct http_client_ctx));

	http_ctx->settings.on_body = on_body;
	http_ctx->settings.on_chunk_complete = on_chunk_complete;
	http_ctx->settings.on_chunk_header = on_chunk_header;
	http_ctx->settings.on_headers_complete = on_headers_complete;
	http_ctx->settings.on_header_field = on_header_field;
	http_ctx->settings.on_header_value = on_header_value;
	http_ctx->settings.on_message_begin = on_message_begin;
	http_ctx->settings.on_message_complete = on_message_complete;
	http_ctx->settings.on_status = on_status;
	http_ctx->settings.on_url = on_url;

	return 0;
}

int http_reset_ctx(struct http_client_ctx *http_ctx)
{
	http_parser_init(&http_ctx->parser, HTTP_RESPONSE);

	memset(http_ctx->http_status, 0, sizeof(http_ctx->http_status));

	http_ctx->cl_present = 0;
	http_ctx->content_length = 0;
	http_ctx->processed = 0;
	http_ctx->body_found = 0;

	return 0;
}
