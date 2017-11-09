/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_HTTP)
#define SYS_LOG_DOMAIN "http"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>
#include <version.h>
#include <misc/printk.h>

#include <net/net_core.h>
#include <net/net_ip.h>
#include <net/http.h>

int http_set_cb(struct http_ctx *ctx,
		http_connect_cb_t connect_cb,
		http_recv_cb_t recv_cb,
		http_send_cb_t send_cb,
		http_close_cb_t close_cb)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	ctx->cb.connect = connect_cb;
	ctx->cb.recv = recv_cb;
	ctx->cb.send = send_cb;
	ctx->cb.close = close_cb;

	return 0;
}

int http_close(struct http_ctx *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	http_send_flush(ctx, NULL);
	if (ctx->pending) {
		net_pkt_unref(ctx->pending);
		ctx->pending = NULL;
	}

#if defined(CONFIG_HTTP_SERVER) && defined(CONFIG_NET_DEBUG_HTTP_CONN)
	if (!ctx->is_client) {
		http_server_conn_del(ctx);
	}
#endif

	return net_app_close(&ctx->app_ctx);
}

int http_release(struct http_ctx *ctx)
{
	if (!ctx) {
		return -EINVAL;
	}

	if (!ctx->is_init) {
		return -ENOENT;
	}

	ctx->is_tls = false;

#if defined(CONFIG_HTTP_SERVER) && defined(CONFIG_NET_DEBUG_HTTP_CONN)
	if (!ctx->is_client) {
		http_server_conn_del(ctx);
		http_server_disable(ctx);
	}
#endif

	if (ctx->pending) {
		net_pkt_unref(ctx->pending);
		ctx->pending = NULL;
	}

	ctx->is_init = false;

	return net_app_release(&ctx->app_ctx);
}

int http_send_msg_raw(struct http_ctx *ctx, struct net_pkt *pkt,
		      void *user_send_data)
{
	int ret;

	NET_DBG("[%p] Sending %zd bytes data", ctx, net_pkt_get_len(pkt));

	ret = net_app_send_pkt(&ctx->app_ctx, pkt, NULL, 0, 0,
			       user_send_data);
	if (!ret) {
		/* We must let the system to send the packet, otherwise TCP
		 * might timeout before the packet is actually sent. This is
		 * easily seen if the application calls this functions many
		 * times in a row.
		 */
		k_yield();
	}

	return ret;
}

int http_prepare_and_send(struct http_ctx *ctx,
			  const char *payload,
			  size_t payload_len,
			  void *user_send_data)
{
	size_t added;
	int ret;

	do {
		if (!ctx->pending) {
			ctx->pending = net_app_get_net_pkt(&ctx->app_ctx,
							   AF_UNSPEC,
							   ctx->timeout);
			if (!ctx->pending) {
				return -ENOMEM;
			}
		}

		ret = net_pkt_append(ctx->pending, payload_len, payload,
				     ctx->timeout);

		added = ret;

		if (added < payload_len) {
			/* Not all data could be added, send what we have now
			 * and allocate new stuff to be sent.
			 */
			ret = http_send_flush(ctx, user_send_data);
			if (ret < 0) {
				return ret;
			}

			payload_len -= added;
			payload += added;
		}
	} while (added < payload_len);

	return 0;
}

int http_send_flush(struct http_ctx *ctx, void *user_send_data)
{
	int ret;

	if (!ctx->pending) {
		return 0;
	}

	ret = http_send_msg_raw(ctx, ctx->pending, user_send_data);
	if (ret < 0) {
		return ret;
	}

	ctx->pending = NULL;

	return ret;
}

int http_send_chunk(struct http_ctx *ctx, const char *buf, size_t len,
		    void *user_send_data)
{
	char chunk_header[16];
	int ret;

	if (!buf) {
		len = 0;
	}

	snprintk(chunk_header, sizeof(chunk_header), "%x" HTTP_CRLF,
		 (unsigned int)len);

	ret = http_prepare_and_send(ctx, chunk_header, strlen(chunk_header),
				    user_send_data);
	if (ret < 0) {
		return ret;
	}

	if (len) {
		ret = http_prepare_and_send(ctx, buf, len, user_send_data);
		if (ret < 0) {
			return ret;
		}
	}

	ret = http_prepare_and_send(ctx, HTTP_CRLF, sizeof(HTTP_CRLF),
				    user_send_data);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int _http_add_header(struct http_ctx *ctx, s32_t timeout,
			    const char *name, const char *value,
			    void *user_send_data)
{
	int ret;

	ret = http_prepare_and_send(ctx, name, strlen(name), user_send_data);
	if (value && ret >= 0) {
		ret = http_prepare_and_send(ctx, ": ", strlen(": "),
					    user_send_data);
		if (ret < 0) {
			goto out;
		}

		ret = http_prepare_and_send(ctx, value, strlen(value),
					    user_send_data);
		if (ret < 0) {
			goto out;
		}

		ret = http_prepare_and_send(ctx, HTTP_CRLF, strlen(HTTP_CRLF),
					    user_send_data);
		if (ret < 0) {
			goto out;
		}
	}

out:
	return ret;
}

int http_add_header(struct http_ctx *ctx, const char *field,
		    void *user_send_data)
{
	return _http_add_header(ctx, ctx->timeout, field, NULL, user_send_data);
}

int http_add_header_field(struct http_ctx *ctx, const char *name,
			  const char *value, void *user_send_data)
{
	return _http_add_header(ctx, ctx->timeout, name, value, user_send_data);
}
