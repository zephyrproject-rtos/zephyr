/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_client_rcv.h"
#include "http_client_types.h"
#include "config.h"

#include <net/net_pkt.h>

#ifdef LINEARIZE_BUFFER

NET_BUF_POOL_DEFINE(http_pool, HTTP_POOL_BUF_CTR, HTTP_POOL_BUF_SIZE, 0, NULL);

void http_receive_cb(struct tcp_client_ctx *tcp_ctx, struct net_pkt *rx)
{
	struct http_client_ctx *http_ctx;
	struct net_buf *data_buf = NULL;
	u16_t data_len;
	u16_t offset;
	int rc;

	if (!rx) {
		return;
	}

	data_buf = net_buf_alloc(&http_pool, tcp_ctx->timeout);
	if (data_buf == NULL) {
		goto lb_exit;
	}

	data_len = min(net_pkt_appdatalen(rx), HTTP_POOL_BUF_SIZE);
	offset = net_pkt_get_len(rx) - data_len;

	rc = net_frag_linear_copy(data_buf, rx->frags, offset, data_len);
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
	net_pkt_unref(rx);
}

#else

void http_receive_cb(struct tcp_client_ctx *tcp_ctx, struct net_pkt *rx)
{
	struct http_client_ctx *http_ctx;
	struct net_buf *frag = rx->frags;
	u16_t offset;

	if (!rx) {
		return;
	}

	http_ctx = CONTAINER_OF(tcp_ctx, struct http_client_ctx, tcp_ctx);

	offset = net_pkt_frags_len(rx) - net_pkt_appdatalen(rx);

	/* find the fragment */
	while (frag && offset >= frag->len) {
		offset -= frag->len;
		frag = frag->frags;
	}

	while (frag) {
		(void)http_parser_execute(&http_ctx->parser,
					  &http_ctx->settings,
					  frag->data + offset,
					  frag->len - offset);

		/* after the first iteration, we set offset to 0 */
		offset = 0;

		/* The parser's error can be catched outside, reading the
		 * http_errno struct member
		 */
		if (http_ctx->parser.http_errno != HPE_OK) {
			goto lb_exit;
		}

		frag = frag->frags;
	}

lb_exit:
	net_pkt_unref(rx);
}

#endif
