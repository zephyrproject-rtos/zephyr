/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <net/http.h>
#include <misc/printk.h>
#include <net/nbuf.h>
#include <net/net_context.h>

#define HTTP_STATUS_200_OK	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"\r\n"

#define HTTP_STATUS_400_BR	"HTTP/1.1 400 Bad Request\r\n" \
				"\r\n"

#define HTTP_STATUS_403_FBD	"HTTP/1.1 403 Forbidden\r\n" \
				"\r\n"

#define HTTP_STATUS_404_NF	"HTTP/1.1 404 Not Found\r\n" \
				"\r\n"

static inline uint16_t http_strlen(const char *str)
{
	if (str) {
		return strlen(str);
	}

	return 0;
}

static int http_add_header(struct net_buf *tx, int32_t timeout, const char *str)
{
	if (net_nbuf_append(tx, strlen(str), (uint8_t *)str, timeout)) {
		return 0;
	}

	return -ENOMEM;
}

static int http_add_chunk(struct net_buf *tx, int32_t timeout, const char *str)
{
	char chunk_header[16];
	char *rn = "\r\n";
	uint16_t str_len;

	str_len = http_strlen(str);

	snprintk(chunk_header, sizeof(chunk_header), "%x\r\n", str_len);

	if (!net_nbuf_append(tx, strlen(chunk_header), chunk_header, timeout)) {
		return -ENOMEM;
	}

	if (str_len > 0) {
		if (!net_nbuf_append(tx, str_len, (uint8_t *)str, timeout)) {
			return -ENOMEM;
		}
	}

	if (!net_nbuf_append(tx, strlen(rn), rn, timeout)) {
		return -ENOMEM;
	}

	return 0;
}

int http_response(struct http_server_ctx *ctx, const char *http_header,
		  const char *html_payload)
{
	struct net_buf *tx;
	int rc = -EINVAL;

	tx = net_nbuf_get_tx(ctx->net_ctx, ctx->timeout);
	if (!tx) {
		goto exit_routine;
	}

	rc = http_add_header(tx, ctx->timeout, http_header);
	if (rc != 0) {
		goto exit_routine;
	}

	if (html_payload) {
		rc = http_add_chunk(tx, ctx->timeout, html_payload);
		if (rc != 0) {
			goto exit_routine;
		}

		/* like EOF */
		rc = http_add_chunk(tx, ctx->timeout, NULL);
		if (rc != 0) {
			goto exit_routine;
		}
	}

	rc = net_context_send(tx, NULL, 0, NULL, NULL);
	if (rc != 0) {
		goto exit_routine;
	}

	tx = NULL;

exit_routine:
	/* unref can handle NULL buffers, so we are covered */
	net_nbuf_unref(tx);

	return rc;
}

int http_response_400(struct http_server_ctx *ctx, const char *html_payload)
{
	return http_response(ctx, HTTP_STATUS_400_BR, html_payload);
}

int http_response_403(struct http_server_ctx *ctx, const char *html_payload)
{
	return http_response(ctx, HTTP_STATUS_403_FBD, html_payload);
}

int http_response_404(struct http_server_ctx *ctx, const char *html_payload)
{
	return http_response(ctx, HTTP_STATUS_404_NF, html_payload);
}
