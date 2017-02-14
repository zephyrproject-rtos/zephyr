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
#include <net/nbuf.h>
#include <stdio.h>

#define URL_DEFAULT_HANDLER_INDEX 0

#define HTTP_BUF_CTR	HTTP_MAX_NUMBER_SERVER_CTX
#define HTTP_BUF_SIZE	1024

NET_BUF_POOL_DEFINE(http_msg_pool, HTTP_BUF_CTR, HTTP_BUF_SIZE, 0, NULL);

void http_accept_cb(struct net_context *net_ctx, struct sockaddr *addr,
		    socklen_t addr_len, int status, void *data)
{
	struct http_server_ctx *http_ctx = NULL;

	ARG_UNUSED(addr_len);
	ARG_UNUSED(data);

	if (status != 0) {
		net_context_put(net_ctx);
		return;
	}

	print_client_banner(addr);

	http_ctx = http_ctx_get();
	if (!http_ctx) {
		net_context_put(net_ctx);
		return;
	}

	http_ctx_set(http_ctx, net_ctx);

	net_context_recv(net_ctx, http_rx_tx, K_NO_WAIT, http_ctx);
}

/**
 * @brief http_ctx_release	Releases an HTTP context
 * @return			0, future versions may return error codes
 */
static
int http_ctx_release(struct http_server_ctx *http_ctx);

/**
 * @brief parser_init	Initializes some parser-related fields at the
 *			http_server_ctx structure
 * @param ctx		HTTP server context
 * @param net_ctx	Network context
 * @return		0 on success
 * @return		-EINVAL on error
 */
static
int parser_init(struct http_server_ctx *ctx);

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

static int http_url_cmp(const char *url, uint16_t url_len,
			const char *root_url, uint16_t root_url_len);

static void http_tx(struct http_server_ctx *http_ctx);

void http_rx_tx(struct net_context *net_ctx, struct net_buf *rx, int status,
		void *user_data)
{
	struct http_server_ctx *http_ctx = NULL;
	struct net_buf *data = NULL;
	uint16_t rcv_len;
	uint16_t offset;
	int parsed_len;
	int rc;

	if (status) {
		printf("[%s:%d] Status code: %d, <%s>\n",
			__func__, __LINE__, status, RC_STR(status));
		goto lb_exit;
	}

	if (!user_data) {
		printf("[%s:%d] User data is null\n", __func__, __LINE__);
		goto lb_exit;
	}

	http_ctx = (struct http_server_ctx *)user_data;
	if (http_ctx->net_ctx != net_ctx) {
		printf("[%s:%d] Wrong network context received\n",
		       __func__, __LINE__);
		goto lb_exit;
	}

	if (!rx) {
		printf("[%s:%d] Connection closed by peer\n",
		       __func__, __LINE__);
		http_ctx_release(http_ctx);
		goto lb_exit;
	}

	rcv_len = net_nbuf_appdatalen(rx);
	if (rcv_len == 0) {
		/* don't print info about zero-length app data buffers */
		goto lb_exit;
	}

	data = net_buf_alloc(&http_msg_pool, APP_SLEEP_MSECS);
	if (data == NULL) {
		printf("[%s:%d] Data buffer alloc error\n", __func__, __LINE__);
		goto lb_exit;
	}

	offset = net_buf_frags_len(rx) - rcv_len;
	rc = net_nbuf_linear_copy(data, rx, offset, rcv_len);
	if (rc != 0) {
		printf("[%s:%d] Linear copy error\n", __func__, __LINE__);
		goto lb_exit;
	}

	parser_init(http_ctx);
	parsed_len = parser_parse_request(http_ctx, data);
	if (parsed_len <= 0) {
		printf("[%s:%d] Received: %u bytes, only parsed: %d bytes\n",
		       __func__, __LINE__, rcv_len, parsed_len);
	}

	if (http_ctx->parser.http_errno != HPE_OK) {
		http_write_400_bad_request(http_ctx);
	} else {
		http_tx(http_ctx);
	}

lb_exit:
	net_buf_unref(data);
	net_buf_unref(rx);
}

/**
 * @brief on_header_field	HTTP Parser callback for header fields
 * @param parser		HTTP Parser
 * @param at			Points to where the field begins
 * @param length		Field's length
 * @return			0 (always)
 */
static
int on_header_field(struct http_parser *parser, const char *at, size_t length)
{
	struct http_server_ctx *ctx = (struct http_server_ctx *)parser->data;

	if (ctx->field_values_ctr >= HTTP_PARSER_MAX_FIELD_VALUES) {
		return 0;
	}

	ctx->field_values[ctx->field_values_ctr].key = at;
	ctx->field_values[ctx->field_values_ctr].key_len = length;

	return 0;
}

/**
 * @brief on_header_value	HTTP Parser callback for header values
 * @param parser		HTTP Parser
 * @param at			Points to where the value begins
 * @param length		Value's length
 * @return			0 (always)
 */
static
int on_header_value(struct http_parser *parser, const char *at, size_t length)
{
	struct http_server_ctx *ctx = (struct http_server_ctx *)parser->data;

	if (ctx->field_values_ctr >= HTTP_PARSER_MAX_FIELD_VALUES) {
		return 0;
	}

	ctx->field_values[ctx->field_values_ctr].value = at;
	ctx->field_values[ctx->field_values_ctr].value_len = length;

	ctx->field_values_ctr++;

	return 0;
}

/**
 * @brief on_url	HTTP Parser callback for URLs
 * @param parser	HTTP Parser
 * @param at		Points to where the value begins
 * @param length	Value's length
 * @return		0 (always)
 */
static
int on_url(struct http_parser *parser, const char *at, size_t length)
{
	struct http_server_ctx *ctx = (struct http_server_ctx *)parser->data;

	ctx->url = at;
	ctx->url_len = length;

	return 0;
}

static
int parser_init(struct http_server_ctx *ctx)
{
	memset(ctx->field_values, 0x00, sizeof(ctx->field_values));

	ctx->parser_settings.on_header_field = on_header_field;
	ctx->parser_settings.on_header_value = on_header_value;
	ctx->parser_settings.on_url = on_url;

	http_parser_init(&ctx->parser, HTTP_REQUEST);

	/* This circular reference is useful when some parser callbacks
	 * want to access some internal data structures
	 */
	ctx->parser.data = ctx;

	return 0;
}

static
int parser_parse_request(struct http_server_ctx *ctx, struct net_buf *rx)
{
	int rc;

	ctx->field_values_ctr = 0;
	rc = http_parser_execute(&ctx->parser, &ctx->parser_settings,
				 rx->data, rx->len);
	if (rc < 0) {
		printf("[%s:%d] http_parser_execute: %s\n\t%s\n",
		       __func__, __LINE__,
		       http_errno_name(ctx->parser.http_errno),
		       http_errno_description(ctx->parser.http_errno));

		rc = -EINVAL;
		goto exit_routine;
	}

exit_routine:
	return rc;
}

/**
 * @brief server_collection	This is a collection of server ctx structs
 */
static
struct http_server_ctx server_collection[HTTP_MAX_NUMBER_SERVER_CTX];

/**
 * @brief http_url_ctx	Just one URL context per application
 */
static struct http_url_ctx url_ctx;

int http_ctx_init(void)
{
	memset(server_collection, 0x00, sizeof(server_collection));

	memset(&url_ctx, 0x00, sizeof(url_ctx));

	/* 0 is reserved for the default handler */
	url_ctx.urls_ctr = 1;

	return 0;
}

struct http_server_ctx *http_ctx_get(void)
{
	int i;

	for (i = 0; i < HTTP_MAX_NUMBER_SERVER_CTX; i++) {

		if (server_collection[i].used == HTTP_CTX_FREE) {

			printf("[%s:%d] Free ctx found, index: %d\n",
			       __func__, __LINE__, i);

			memset(server_collection + i, 0x00,
			       sizeof(struct http_server_ctx));

			return server_collection + i;
		}
	}

	return NULL;
}

int http_ctx_set(struct http_server_ctx *http_ctx, struct net_context *net_ctx)
{
	if (http_ctx == NULL || net_ctx == NULL) {
		return -EINVAL;
	}

	http_ctx->used = HTTP_CTX_IN_USE;
	http_ctx->net_ctx = net_ctx;

	return 0;
}

static
int http_ctx_release(struct http_server_ctx *http_ctx)
{
	if (http_ctx == NULL) {
		return 0;
	}

	http_ctx->used = HTTP_CTX_FREE;

	return 0;
}

int http_url_default_handler(int (*write_cb)(struct http_server_ctx *))
{
	if (write_cb == NULL) {
		return -EINVAL;
	}

	url_ctx.urls[URL_DEFAULT_HANDLER_INDEX].flags = 0x00;
	url_ctx.urls[URL_DEFAULT_HANDLER_INDEX].root = NULL;
	url_ctx.urls[URL_DEFAULT_HANDLER_INDEX].root_len = 0;
	url_ctx.urls[URL_DEFAULT_HANDLER_INDEX].write_cb = write_cb;

	return 0;
}

int http_url_add(const char *url, uint8_t flags,
		 int (*write_cb)(struct http_server_ctx *http_ctx))
{
	struct http_root_url *root = NULL;

	if (url_ctx.urls_ctr >= HTTP_MAX_NUMBER_URL) {
		return -ENOMEM;
	}

	root = &url_ctx.urls[url_ctx.urls_ctr];

	root->root = url;
	/* this will speed-up some future operations */
	root->root_len = strlen(url);
	root->flags = flags;
	root->write_cb = write_cb;

	url_ctx.urls_ctr++;

	return 0;
}

static int http_url_cmp(const char *url, uint16_t url_len,
			const char *root_url, uint16_t root_url_len)
{
	if (url_len < root_url_len) {
		return -EINVAL;
	}

	if (memcmp(url, root_url, root_url_len) == 0) {
		if (url_len == root_url_len) {
			return 0;
		}

		/* Here we evlaute the following conditions:
		 * root_url = /images, url = /images/ -> OK
		 * root_url = /images/, url = /images/img.png -> OK
		 * root_url = /images/, url = /images_and_docs -> ERROR
		 */
		if (url_len > root_url_len) {
			if (root_url[root_url_len - 1] == '/') {
				return 0;
			}

			if (url[root_url_len] == '/') {
				return 0;
			}
		}
	}

	return -EINVAL;
}

static struct http_root_url *http_url_find(struct http_server_ctx *http_ctx)
{
	uint16_t url_len = http_ctx->url_len;
	const char *url = http_ctx->url;
	struct http_root_url *root_url;
	uint8_t i;
	int rc;

	/* at some point we must come up with something better */
	for (i = 1; i < url_ctx.urls_ctr; i++) {
		root_url = &url_ctx.urls[i];

		rc = http_url_cmp(url, url_len,
				  root_url->root, root_url->root_len);
		if (rc == 0) {
			return root_url;
		}
	}

	return NULL;
}

static void http_tx(struct http_server_ctx *http_ctx)
{
	struct http_root_url *root_url;

	root_url = http_url_find(http_ctx);
	if (!root_url) {
		root_url = &url_ctx.urls[URL_DEFAULT_HANDLER_INDEX];
	}

	if (root_url->write_cb) {
		root_url->write_cb(http_ctx);
	} else {
		printf("[%s:%d] No default handler for %.*s\n",
		       __func__, __LINE__,
		       http_ctx->url_len, http_ctx->url);
	}
}
