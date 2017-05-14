/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "http_types.h"
#include "http_server.h"
#include "http_utils.h"
#include "http_write_utils.h"
#include "config.h"

#include <net/http_parser.h>
#include <net/net_pkt.h>
#include <stdio.h>


#define HTTP_BUF_CTR	HTTP_MAX_NUMBER_SERVER_CTX
#define HTTP_BUF_SIZE	1024

/**
 * @brief parser_parse_request	Parses an HTTP REQUEST
 * @param ctx		HTTP server context
 * @param rx		Input buffer
 * @return		0 on success
 * @return		-EINVAL on error
 */
static
int parser_parse_request(struct http_server_ctx *ctx, struct net_buf *rx);

static struct http_root_url *http_url_find(struct http_server_ctx *http_ctx);

static int http_url_cmp(const char *url, u16_t url_len,
			const char *root_url, u16_t root_url_len);

static void http_tx(struct http_server_ctx *http_ctx);


/**
 * @brief server_collection	This is a collection of server ctx structs
 */
static struct http_server_ctx server_ctx[CONFIG_HTTP_SERVER_CONNECTIONS];

/**
 * @brief http_url_ctx	Just one URL context per application
 */
static struct http_url_ctx url_ctx;

int http_auth(struct http_server_ctx *ctx)
{
	const char *auth_str = "Authorization: Basic "HTTP_AUTH_CREDENTIALS;

	if (strstr(ctx->field_values[0].key, auth_str)) {
		return http_response_auth(ctx);
	}

	return http_response_401(ctx);
}
