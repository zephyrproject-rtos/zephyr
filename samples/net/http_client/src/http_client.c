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

static
int http_send_request(struct http_client_ctx *http_ctx, const char *method,
		      const char *url, const char *protocol,
		      const char *content_type_value, const char *payload)
{
	const char *content_type = "Content-Type: ";
	const char *sep = "\r\n\r\n";
	struct net_buf *tx;
	int rc;

	tx = net_nbuf_get_tx(http_ctx->tcp_ctx.net_ctx, K_FOREVER);
	if (tx == NULL) {
		return -ENOMEM;
	}

	if (!net_nbuf_append(tx, strlen(method), (uint8_t *)method,
			     K_FOREVER)) {
		goto lb_exit;
	}

	if (!net_nbuf_append(tx, strlen(url), (uint8_t *)url, K_FOREVER)) {
		goto lb_exit;
	}

	if (!net_nbuf_append(tx, strlen(protocol), (uint8_t *)protocol,
			     K_FOREVER)) {
		goto lb_exit;
	}

	if (!net_nbuf_append(tx, strlen(HEADER_FIELDS),
			     (uint8_t *)HEADER_FIELDS, K_FOREVER)) {
		goto lb_exit;
	}

	if (content_type_value && payload) {
		char content_len_str[CON_LEN_SIZE];

		if (!net_nbuf_append(tx, strlen(content_type),
				     (uint8_t *)content_type, K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		if (!net_nbuf_append(tx, strlen(content_type_value),
				     (uint8_t *)content_type_value,
				     K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		rc = snprintk(content_len_str, sizeof(content_len_str),
			      "\r\nContent-Length: %u\r\n\r\n",
			      (unsigned int)strlen(payload));
		if (rc <= 0 || rc >= sizeof(content_len_str)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		if (!net_nbuf_append(tx, strlen(content_len_str),
				     (uint8_t *)content_len_str, K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

		if (!net_nbuf_append(tx, strlen(payload), (uint8_t *)payload,
				     K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}

	} else {
		if (!net_nbuf_append(tx, strlen(sep), (uint8_t *)sep,
				     K_FOREVER)) {
			rc = -ENOMEM;
			goto lb_exit;
		}
	}

	return net_context_send(tx, NULL, http_ctx->tcp_ctx.timeout,
				NULL, NULL);

lb_exit:
	net_buf_unref(tx);

	return rc;
}

int http_send_get(struct http_client_ctx *http_ctx, const char *url)
{
	return http_send_request(http_ctx, "GET ", url, " HTTP/1.1\r\n",
				 NULL, NULL);
}

int http_send_head(struct http_client_ctx *http_ctx, const char *url)
{
	return http_send_request(http_ctx, "HEAD ", url, " HTTP/1.1\r\n",
				 NULL, NULL);
}

int http_send_options(struct http_client_ctx *http_ctx, const char *url,
		      const char *content_type_value, const char *payload)
{
	return http_send_request(http_ctx, "OPTIONS ", url, " HTTP/1.1\r\n",
				 content_type_value, payload);
}

int http_send_post(struct http_client_ctx *http_ctx, const char *url,
		   const char *content_type_value, const char *payload)
{
	return http_send_request(http_ctx, "POST ", url, " HTTP/1.1\r\n",
				 content_type_value, payload);
}
