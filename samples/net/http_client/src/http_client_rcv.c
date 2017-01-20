/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_client_rcv.h"
#include "http_client_types.h"
#include "config.h"

#include <net/nbuf.h>

#ifdef LINEARIZE_BUFFER

NET_BUF_POOL_DEFINE(http_pool, HTTP_POOL_BUF_CTR, HTTP_POOL_BUF_SIZE, 0, NULL);

void http_receive_cb(struct tcp_client_ctx *tcp_ctx, struct net_buf *rx)
{
	struct http_client_ctx *http_ctx;
	struct net_buf *data_buf = NULL;
	uint16_t data_len;
	uint16_t offset;
	int rc;

	if (!rx) {
		return;
	}

	data_buf = net_buf_alloc(&http_pool, tcp_ctx->timeout);
	if (data_buf == NULL) {
		goto lb_exit;
	}

	data_len = min(net_nbuf_appdatalen(rx), HTTP_POOL_BUF_SIZE);
	offset = net_buf_frags_len(rx) - data_len;

	rc = net_nbuf_linear_copy(data_buf, rx, offset, data_len);
	if (rc != 0) {
		rc = -ENOMEM;
		goto lb_exit;
	}

	http_ctx = CONTAINER_OF(tcp_ctx, struct http_client_ctx, tcp_ctx);

	/* The parser's error can be catched outside, reading the
	 * http_errno struct member
	 */
	http_parser_execute(&http_ctx->parser, &http_ctx->settings,
			    data_buf->data, data_buf->len);

lb_exit:
	net_buf_unref(data_buf);
	net_buf_unref(rx);
}

#else

void http_receive_cb(struct tcp_client_ctx *tcp_ctx, struct net_buf *rx)
{
	struct http_client_ctx *http_ctx;
	struct net_buf *buf = rx;
	uint16_t offset;

	if (!rx) {
		return;
	}

	http_ctx = CONTAINER_OF(tcp_ctx, struct http_client_ctx, tcp_ctx);

	offset = net_buf_frags_len(buf) - net_nbuf_appdatalen(buf);

	/* find the fragment */
	while (buf && offset >= buf->len) {
		offset -= buf->len;
		buf = buf->frags;
	}

	while (buf) {
		(void)http_parser_execute(&http_ctx->parser,
					  &http_ctx->settings,
					  buf->data + offset,
					  buf->len - offset);

		/* after the first iteration, we set offset to 0 */
		offset = 0;

		/* The parser's error can be catched outside, reading the
		 * http_errno struct member
		 */
		if (http_ctx->parser.http_errno != HPE_OK) {
			goto lb_exit;
		}

		buf = buf->frags;
	}

lb_exit:
	net_buf_unref(rx);
}

#endif
