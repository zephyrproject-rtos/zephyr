/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <net/net_context.h>

/* Callback executed when a new connection is accepted */
void http_accept_cb(struct net_context *net_ctx, struct sockaddr *addr,
		    socklen_t addr_len, int status, void *data);

/**
 * @brief http_rx_tx	Reads the HTTP request from the `rx` buffer
 *			and writes an HTTP 1.1 200 OK response with client
 *			header fields or an error message
 * @param ctx		The network context
 * @param rx		The reception buffer
 * @param status	Connection status, see `net_context_recv_cb_t`
 * @param user_data	User-provided data
 */
void http_rx_tx(struct net_context *ctx, struct net_buf *rx, int status,
		void *user_data);

/**
 * @brief http_ctx_init	Initializes the HTTP server contexts collection
 * @return		0, future versions may return error codes
 */
int http_ctx_init(void);

/**
 * @brief http_ctx_get	Gets an HTTP server context struct
 * @return		A valid HTTP server ctx on success
 * @return		NULL on error
 */
struct http_server_ctx *http_ctx_get(void);

/**
 * @brief http_ctx_set		Assigns the net_ctx pointer to the HTTP ctx
 * @param http_ctx		HTTP server context struct
 * @param net_ctx		Network context
 * @return			0 on success
 * @return			-EINVAL on error
 */
int http_ctx_set(struct http_server_ctx *http_ctx, struct net_context *net_ctx);

int http_url_default_handler(int (*write_cb)(struct http_server_ctx *));

int http_url_add(const char *url, uint8_t flags,
		 int (*write_cb)(struct http_server_ctx *http_ctx));

#endif
