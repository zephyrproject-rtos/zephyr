/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_HTTP)
#if defined(CONFIG_HTTPS)
#define SYS_LOG_DOMAIN "https/client"
#else
#define SYS_LOG_DOMAIN "http/client"
#endif
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <version.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/http.h>

#include "../../ip/net_private.h"

#define BUF_ALLOC_TIMEOUT 100

#define RC_STR(rc)	(rc == 0 ? "OK" : "ERROR")

#define HTTP_EOF           "\r\n\r\n"

#define HTTP_HOST          "Host"
#define HTTP_CONTENT_TYPE  "Content-Type"
#define HTTP_CONTENT_LEN   "Content-Length"
#define HTTP_CONT_LEN_SIZE 6

/* Default network activity timeout in seconds */
#define HTTP_NETWORK_TIMEOUT	K_SECONDS(CONFIG_HTTP_CLIENT_NETWORK_TIMEOUT)

int client_reset(struct http_ctx *ctx)
{
	http_parser_init(&ctx->http.parser, HTTP_RESPONSE);

	memset(ctx->http.rsp.http_status, 0,
	       sizeof(ctx->http.rsp.http_status));

	ctx->http.rsp.cl_present = 0;
	ctx->http.rsp.content_length = 0;
	ctx->http.rsp.processed = 0;
	ctx->http.rsp.body_found = 0;
	ctx->http.rsp.message_complete = 0;
	ctx->http.rsp.body_start = NULL;

	memset(ctx->http.rsp.response_buf, 0, ctx->http.rsp.response_buf_len);
	ctx->http.rsp.data_len = 0;

	return 0;
}

int http_request(struct http_ctx *ctx, struct http_request *req, s32_t timeout,
		 void *user_data)
{
	const char *method = http_method_str(req->method);
	int ret;

	if (ctx->pending) {
		net_pkt_unref(ctx->pending);
		ctx->pending = NULL;
	}

	ret = http_add_header(ctx, method, user_data);
	if (ret < 0) {
		goto out;
	}

	ret = http_add_header(ctx, " ", user_data);
	if (ret < 0) {
		goto out;
	}

	ret = http_add_header(ctx, req->url, user_data);
	if (ret < 0) {
		goto out;
	}

	ret = http_add_header(ctx, req->protocol, user_data);
	if (ret < 0) {
		goto out;
	}

	ret = http_add_header(ctx, HTTP_CRLF, user_data);
	if (ret < 0) {
		goto out;
	}

	if (req->host) {
		ret = http_add_header_field(ctx, HTTP_HOST, req->host,
					    user_data);
		if (ret < 0) {
			goto out;
		}
	}

	if (req->header_fields) {
		ret = http_add_header(ctx, req->header_fields, user_data);
		if (ret < 0) {
			goto out;
		}
	}

	if (req->content_type_value) {
		ret = http_add_header_field(ctx, HTTP_CONTENT_TYPE,
					    req->content_type_value,
					    user_data);
		if (ret < 0) {
			goto out;
		}
	}

	if (req->payload && req->payload_size) {
		char content_len_str[HTTP_CONT_LEN_SIZE];

		ret = snprintk(content_len_str, HTTP_CONT_LEN_SIZE,
			       "%u", req->payload_size);
		if (ret <= 0 || ret >= HTTP_CONT_LEN_SIZE) {
			ret = -ENOMEM;
			goto out;
		}

		ret = http_add_header_field(ctx, HTTP_CONTENT_LEN,
					    content_len_str, user_data);
		if (ret < 0) {
			goto out;
		}

		ret = http_add_header(ctx, HTTP_CRLF, user_data);
		if (ret < 0) {
			goto out;
		}

		ret = http_prepare_and_send(ctx, req->payload,
					    req->payload_size, user_data);
		if (ret < 0) {
			goto out;
		}
	} else {
		ret = http_add_header(ctx, HTTP_EOF, user_data);
		if (ret < 0) {
			goto out;
		}
	}

	http_send_flush(ctx, user_data);

out:
	if (ctx->pending) {
		net_pkt_unref(ctx->pending);
		ctx->pending = NULL;
	}

	return ret;
}

#if defined(CONFIG_NET_DEBUG_HTTP)
static void sprint_addr(char *buf, int len,
			sa_family_t family,
			struct sockaddr *addr)
{
	if (family == AF_INET6) {
		net_addr_ntop(AF_INET6, &net_sin6(addr)->sin6_addr, buf, len);
	} else if (family == AF_INET) {
		net_addr_ntop(AF_INET, &net_sin(addr)->sin_addr, buf, len);
	} else {
		NET_DBG("Invalid protocol family");
	}
}
#endif

static inline void print_info(struct http_ctx *ctx,
			      enum http_method method)
{
#if defined(CONFIG_NET_DEBUG_HTTP)
	char local[NET_IPV6_ADDR_LEN];
	char remote[NET_IPV6_ADDR_LEN];

	sprint_addr(local, NET_IPV6_ADDR_LEN,
		    ctx->app_ctx.default_ctx->local.sa_family,
		    &ctx->app_ctx.default_ctx->local);

	sprint_addr(remote, NET_IPV6_ADDR_LEN,
		    ctx->app_ctx.default_ctx->remote.sa_family,
		    &ctx->app_ctx.default_ctx->remote);

	NET_DBG("HTTP %s (%s) %s -> %s port %d",
		http_method_str(method), ctx->http.req.host, local, remote,
		ntohs(net_sin(&ctx->app_ctx.default_ctx->remote)->sin_port));
#endif
}

int http_client_send_req(struct http_ctx *ctx,
			 struct http_request *req,
			 http_response_cb_t cb,
			 u8_t *response_buf,
			 size_t response_buf_len,
			 void *user_data,
			 s32_t timeout)
{
	int ret;

	if (!response_buf || response_buf_len == 0) {
		return -EINVAL;
	}

	ctx->http.rsp.response_buf = response_buf;
	ctx->http.rsp.response_buf_len = response_buf_len;

	client_reset(ctx);

	if (!req->host) {
		req->host = ctx->server;
	}

	ctx->http.req.host = req->host;
	ctx->http.req.method = req->method;
	ctx->http.req.user_data = user_data;

	ctx->http.rsp.cb = cb;

	ret = net_app_connect(&ctx->app_ctx, HTTP_NETWORK_TIMEOUT);
	if (ret < 0) {
		NET_DBG("Cannot connect to server (%d)", ret);
		return ret;
	}

	/* We might wait longer than timeout if the first connection
	 * establishment takes long time (like with HTTPS)
	 */
	if (k_sem_take(&ctx->http.connect_wait, HTTP_NETWORK_TIMEOUT)) {
		NET_DBG("Connection timed out");
		ret = -ETIMEDOUT;
		goto out;
	}

	print_info(ctx, ctx->http.req.method);

	ret = http_request(ctx, req, timeout, user_data);
	if (ret < 0) {
		NET_DBG("Send error (%d)", ret);
		goto out;
	}

	if (timeout != 0 && k_sem_take(&ctx->http.req.wait, timeout)) {
		ret = -ETIMEDOUT;
		goto out;
	}

	if (timeout == 0) {
		return -EINPROGRESS;
	}

	return 0;

out:
	return ret;
}

static void print_header_field(size_t len, const char *str)
{
#if defined(CONFIG_NET_DEBUG_HTTP)
#define MAX_OUTPUT_LEN 128
	char output[MAX_OUTPUT_LEN];

	/* The value of len does not count \0 so we need to increase it
	 * by one.
	 */
	if ((len + 1) > sizeof(output)) {
		len = sizeof(output) - 1;
	}

	snprintk(output, len + 1, "%s", str);

	NET_DBG("[%zd] %s", len, output);
#endif
}

static int on_url(struct http_parser *parser, const char *at, size_t length)
{
	ARG_UNUSED(parser);

	print_header_field(length, at);

	return 0;
}

static int on_status(struct http_parser *parser, const char *at, size_t length)
{
	u16_t len;
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    http.parser);

	len = min(length, sizeof(ctx->http.rsp.http_status) - 1);
	memcpy(ctx->http.rsp.http_status, at, len);
	ctx->http.rsp.http_status[len] = 0;

	NET_DBG("HTTP response status %s", ctx->http.rsp.http_status);

	return 0;
}

static int on_header_field(struct http_parser *parser, const char *at,
			   size_t length)
{
	const char *content_len = HTTP_CONTENT_LEN;
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    http.parser);
	u16_t len;

	len = strlen(content_len);
	if (length >= len && memcmp(at, content_len, len) == 0) {
		ctx->http.rsp.cl_present = true;
	}

	print_header_field(length, at);

	return 0;
}

#define MAX_NUM_DIGITS	16

static int on_header_value(struct http_parser *parser, const char *at,
			   size_t length)
{
	char str[MAX_NUM_DIGITS];
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    http.parser);

	if (ctx->http.rsp.cl_present) {
		if (length <= MAX_NUM_DIGITS - 1) {
			long int num;

			memcpy(str, at, length);
			str[length] = 0;

			num = strtol(str, NULL, 10);
			if (num == LONG_MIN || num == LONG_MAX) {
				return -EINVAL;
			}

			ctx->http.rsp.content_length = num;
		}

		ctx->http.rsp.cl_present = false;
	}

	print_header_field(length, at);

	return 0;
}

static int on_body(struct http_parser *parser, const char *at, size_t length)
{
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    http.parser);

	ctx->http.rsp.body_found = 1;
	ctx->http.rsp.processed += length;

	NET_DBG("Processed %zd length %zd", ctx->http.rsp.processed, length);

	if (!ctx->http.rsp.body_start &&
	    (u8_t *)at != (u8_t *)ctx->http.rsp.response_buf) {
		ctx->http.rsp.body_start = (u8_t *)at;
	}

	if (ctx->http.rsp.cb) {
		NET_DBG("Calling callback for partitioned %zd len data",
			ctx->http.rsp.data_len);

		ctx->http.rsp.cb(ctx,
				 ctx->http.rsp.response_buf,
				 ctx->http.rsp.response_buf_len,
				 ctx->http.rsp.data_len,
				 HTTP_DATA_MORE,
				 ctx->http.req.user_data);

		/* Re-use the result buffer and start to fill it again */
		ctx->http.rsp.data_len = 0;
		ctx->http.rsp.body_start = NULL;
	}

	return 0;
}

static int on_headers_complete(struct http_parser *parser)
{
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    http.parser);

	if (parser->status_code >= 500 && parser->status_code < 600) {
		NET_DBG("Status %d, skipping body", parser->status_code);

		return 1;
	}

	if ((ctx->http.req.method == HTTP_HEAD ||
	     ctx->http.req.method == HTTP_OPTIONS)
	    && ctx->http.rsp.content_length > 0) {
		NET_DBG("No body expected");
		return 1;
	}

	NET_DBG("Headers complete");

	return 0;
}

static int on_message_begin(struct http_parser *parser)
{
#if defined(CONFIG_NET_DEBUG_HTTP) && (CONFIG_SYS_LOG_NET_LEVEL > 2)
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    http.parser);

	NET_DBG("-- HTTP %s response (headers) --",
		http_method_str(ctx->http.req.method));
#else
	ARG_UNUSED(parser);
#endif
	return 0;
}

static int on_message_complete(struct http_parser *parser)
{
	struct http_ctx *ctx = CONTAINER_OF(parser,
					    struct http_ctx,
					    http.parser);

	NET_DBG("-- HTTP %s response (complete) --",
		http_method_str(ctx->http.req.method));

	if (ctx->http.rsp.cb) {
		ctx->http.rsp.cb(ctx,
				 ctx->http.rsp.response_buf,
				 ctx->http.rsp.response_buf_len,
				 ctx->http.rsp.data_len,
				 HTTP_DATA_FINAL,
				 ctx->http.req.user_data);
	}

	ctx->http.rsp.message_complete = 1;

	k_sem_give(&ctx->http.req.wait);

	return 0;
}

static int on_chunk_header(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

static int on_chunk_complete(struct http_parser *parser)
{
	ARG_UNUSED(parser);

	return 0;
}

static void http_received(struct net_app_ctx *app_ctx,
			  struct net_pkt *pkt,
			  int status,
			  void *user_data)
{
	struct http_ctx *ctx = user_data;
	size_t start = ctx->http.rsp.data_len;
	u16_t len = 0;
	struct net_buf *frag, *prev_frag = NULL;
	size_t recv_len;
	size_t pkt_len;

	recv_len = net_pkt_appdatalen(pkt);
	if (recv_len == 0) {
		/* don't print info about zero-length app data buffers */
		goto quit;
	}

	if (status) {
		NET_DBG("[%p] Status %d <%s>", ctx, status, RC_STR(status));
		goto out;
	}

	/* Get rid of possible IP headers in the first fragment. */
	frag = pkt->frags;

	pkt_len = net_pkt_get_len(pkt);

	if (recv_len < pkt_len) {
		net_buf_pull(frag, pkt_len - recv_len);
		net_pkt_set_appdata(pkt, frag->data);
	}

	NET_DBG("[%p] Received %zd bytes http data", ctx, recv_len);

	while (frag) {
		/* If this fragment cannot be copied to result buf,
		 * then parse what we have which will cause the callback to be
		 * called in function on_body(), and continue copying.
		 */
		if ((ctx->http.rsp.data_len + frag->len) >
		    ctx->http.rsp.response_buf_len) {

			/* If the caller has not supplied a callback, then
			 * we cannot really continue if the request buffer
			 * overflows. Set the data_len to mark how many bytes
			 * should be needed in the response_buf.
			 */
			if (!ctx->cb.recv) {
				ctx->http.rsp.data_len = recv_len;
				goto out;
			}

			http_parser_execute(&ctx->http.parser,
					    &ctx->http.parser_settings,
					    ctx->http.rsp.response_buf + start,
					    len);

			ctx->http.rsp.data_len = 0;
			len = 0;
			start = 0;
		}

		memcpy(ctx->http.rsp.response_buf + ctx->http.rsp.data_len,
		       frag->data, frag->len);

		ctx->http.rsp.data_len += frag->len;
		len += frag->len;

		prev_frag = frag;
		frag = frag->frags;
		pkt->frags = frag;

		prev_frag->frags = NULL;
		net_pkt_frag_unref(prev_frag);
	}

out:
	http_parser_execute(&ctx->http.parser,
			    &ctx->http.parser_settings,
			    ctx->http.rsp.response_buf + start,
			    len);

	net_pkt_unref(pkt);
	return;

quit:
	http_parser_init(&ctx->http.parser, HTTP_RESPONSE);
	ctx->http.rsp.data_len = 0;
	net_pkt_unref(pkt);
}

static void http_data_sent(struct net_app_ctx *app_ctx,
			   int status,
			   void *user_data_send,
			   void *user_data)
{
	struct http_ctx *ctx = user_data;

	if (!user_data_send) {
		/* This is the token field in the net_context_send().
		 * If this is not set, then it is TCP ACK messages
		 * that are generated by the stack. We just ignore those.
		 */
		return;
	}

	if (ctx->cb.send) {
		ctx->cb.send(ctx, status, user_data_send, ctx->user_data);
	}
}

static void http_connected(struct net_app_ctx *app_ctx,
			   int status,
			   void *user_data)
{
	struct http_ctx *ctx = user_data;

	if (status < 0) {
		return;
	}

	if (ctx->cb.connect) {
		ctx->cb.connect(ctx, HTTP_CONNECTION, ctx->user_data);
	}

	if (ctx->is_connected) {
		return;
	}

	ctx->is_connected = true;

	k_sem_give(&ctx->http.connect_wait);
}

static void http_closed(struct net_app_ctx *app_ctx,
			int status,
			void *user_data)
{
	struct http_ctx *ctx = user_data;

	ARG_UNUSED(app_ctx);
	ARG_UNUSED(status);

	NET_DBG("[%p] connection closed", ctx);

	ctx->is_connected = false;

	if (ctx->cb.close) {
		ctx->cb.close(ctx, 0, ctx->user_data);
	}
}

int http_client_init(struct http_ctx *ctx,
		     const char *server,
		     u16_t server_port,
		     struct sockaddr *server_addr,
		     s32_t timeout)
{
	int ret;

	memset(ctx, 0, sizeof(*ctx));

	ret = net_app_init_tcp_client(&ctx->app_ctx,
				      NULL,         /* use any local address */
				      server_addr,
				      server,
				      server_port,
				      timeout,
				      ctx);
	if (ret < 0) {
		NET_DBG("Cannot init HTTP client (%d)", ret);
		return ret;
	}

	ret = net_app_set_cb(&ctx->app_ctx, http_connected, http_received,
			     http_data_sent, http_closed);
	if (ret < 0) {
		NET_ERR("Cannot set callbacks (%d)", ret);
		return ret;
	}

	ctx->http.parser_settings.on_body = on_body;
	ctx->http.parser_settings.on_chunk_complete = on_chunk_complete;
	ctx->http.parser_settings.on_chunk_header = on_chunk_header;
	ctx->http.parser_settings.on_headers_complete = on_headers_complete;
	ctx->http.parser_settings.on_header_field = on_header_field;
	ctx->http.parser_settings.on_header_value = on_header_value;
	ctx->http.parser_settings.on_message_begin = on_message_begin;
	ctx->http.parser_settings.on_message_complete = on_message_complete;
	ctx->http.parser_settings.on_status = on_status;
	ctx->http.parser_settings.on_url = on_url;

	k_sem_init(&ctx->http.req.wait, 0, 1);
	k_sem_init(&ctx->http.connect_wait, 0, 1);

	ctx->server = server;
	ctx->is_init = true;
	ctx->is_client = true;

	return 0;
}

int http_request_cancel(struct http_ctx *ctx)
{
	if (!ctx->is_init) {
		return -EINVAL;
	}

	if (!ctx->is_client) {
		return -EINVAL;
	}

	client_reset(ctx);

	return 0;
}

#if defined(CONFIG_HTTPS)
int http_client_set_tls(struct http_ctx *ctx,
			u8_t *request_buf,
			size_t request_buf_len,
			u8_t *personalization_data,
			size_t personalization_data_len,
			net_app_ca_cert_cb_t cert_cb,
			const char *cert_host,
			net_app_entropy_src_cb_t entropy_src_cb,
			struct k_mem_pool *pool,
			k_thread_stack_t *https_stack,
			size_t https_stack_size)
{
	int ret;

	ret = net_app_client_tls(&ctx->app_ctx,
				 request_buf,
				 request_buf_len,
				 personalization_data,
				 personalization_data_len,
				 cert_cb,
				 cert_host,
				 entropy_src_cb,
				 pool,
				 https_stack,
				 https_stack_size);
	if (ret < 0) {
		NET_DBG("Cannot init TLS (%d)", ret);
		return ret;
	}

	ctx->is_tls = true;

	return 0;
}
#endif /* CONFIG_HTTPS */
