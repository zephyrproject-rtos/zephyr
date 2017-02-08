/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_CLIENT_H_
#define _HTTP_CLIENT_H_

#include "http_client_types.h"

int http_init(struct http_client_ctx *http_ctx);

int http_reset_ctx(struct http_client_ctx *http_ctx);

/* Reception callback executed by the IP stack */
void http_receive_cb(struct tcp_client_ctx *tcp_ctx, struct net_buf *rx);

/* Sends an HTTP GET request for URL url */
int http_send_get(struct http_client_ctx *ctx, const char *url);

/* Sends an HTTP HEAD request for URL url */
int http_send_head(struct http_client_ctx *ctx, const char *url);

/* Sends an HTTP OPTIONS request for URL url. From RFC 2616:
 * If the OPTIONS request includes an entity-body (as indicated by the
 * presence of Content-Length or Transfer-Encoding), then the media type
 * MUST be indicated by a Content-Type field.
 * Note: Transfer-Encoding is not yet supported.
 */
int http_send_options(struct http_client_ctx *http_ctx, const char *url,
		      const char *content_type_value, const char *payload);

/* Sends an HTTP POST request for URL url with payload as content */
int http_send_post(struct http_client_ctx *http_ctx, const char *url,
		   const char *content_type_value, const char *payload);

#endif
