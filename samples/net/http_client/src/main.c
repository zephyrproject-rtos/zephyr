/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "http-client"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/net_ip.h>

#include <net/http.h>

#include <net_sample_app.h>

#include "config.h"

#define MAX_ITERATIONS	20
#define WAIT_TIME (APP_REQ_TIMEOUT * 2)

#define RESULT_BUF_SIZE 1024
static u8_t result[RESULT_BUF_SIZE];

/*
 * Note that the http_client_ctx is quite large so be careful if that is
 * allocated from stack.
 */
static struct http_client_ctx http_ctx;

struct waiter {
	struct http_client_ctx *ctx;
	struct k_sem wait;
	size_t total_len;
	size_t header_len;
};

void panic(const char *msg)
{
	if (msg) {
		NET_ERR("%s", msg);
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

static int do_sync_http_req(struct http_client_ctx *ctx,
			    enum http_method method,
			    const char *url,
			    const char *content_type,
			    const char *payload)
{
	struct http_client_request req = {};
	int ret;

	req.method = method;
	req.url = url;
	req.protocol = " " HTTP_PROTOCOL HTTP_CRLF;

	ret = http_client_send_req(ctx, &req, NULL, result, sizeof(result),
				   NULL, APP_REQ_TIMEOUT);
	if (ret < 0) {
		NET_ERR("Cannot send %s request (%d)", http_method_str(method),
			ret);
		goto out;
	}

	if (ctx->rsp.data_len > sizeof(result)) {
		NET_ERR("Result buffer overflow by %zd bytes",
		       ctx->rsp.data_len - sizeof(result));

		ret = -E2BIG;
	} else {
		NET_INFO("HTTP server response status: %s",
			 ctx->rsp.http_status);

		if (ctx->parser.http_errno) {
			if (method == HTTP_OPTIONS) {
				/* Ignore error if OPTIONS is not found */
				goto out;
			}

			NET_INFO("HTTP parser status: %s",
			       http_errno_description(ctx->parser.http_errno));
			ret = -EINVAL;
			goto out;
		}

		if (method != HTTP_HEAD) {
			if (ctx->rsp.body_found) {
				NET_INFO("HTTP body: %zd bytes, "
					 "expected: %zd bytes",
					 ctx->rsp.processed,
					 ctx->rsp.content_length);
			} else {
				NET_ERR("Error detected during HTTP msg "
					"processing");
			}
		}
	}

out:
	return ret;
}

void response(struct http_client_ctx *ctx,
	      u8_t *data, size_t buflen,
	      size_t datalen,
	      enum http_final_call data_end,
	      void *user_data)
{
	struct waiter *waiter = user_data;
	int ret;

	if (data_end == HTTP_DATA_MORE) {
		NET_INFO("Received %zd bytes piece of data", datalen);

		/* Do something with the data here. For this example
		 * we just ignore the received data.
		 */
		waiter->total_len += datalen;

		if (ctx->rsp.body_start) {
			/* This fragment contains the start of the body
			 * Note that the header length is not proper if
			 * the header is spanning over multiple recv
			 * fragments.
			 */
			waiter->header_len = ctx->rsp.body_start -
				ctx->rsp.response_buf;
		}

		return;
	}

	waiter->total_len += datalen;

	NET_INFO("HTTP server response status: %s", ctx->rsp.http_status);

	if (ctx->parser.http_errno) {
		if (ctx->req.method == HTTP_OPTIONS) {
			/* Ignore error if OPTIONS is not found */
			goto out;
		}

		NET_INFO("HTTP parser status: %s",
			 http_errno_description(ctx->parser.http_errno));
		ret = -EINVAL;
		goto out;
	}

	if (ctx->req.method != HTTP_HEAD && ctx->req.method != HTTP_OPTIONS) {
		if (ctx->rsp.body_found) {
			NET_INFO("HTTP body: %zd bytes, expected: %zd bytes",
				 ctx->rsp.processed, ctx->rsp.content_length);
		} else {
			NET_ERR("Error detected during HTTP msg processing");
		}

		if (waiter->total_len !=
		    waiter->header_len + ctx->rsp.content_length) {
			NET_ERR("Error while receiving data, "
				"received %zd expected %zd bytes",
				waiter->total_len, waiter->header_len +
				ctx->rsp.content_length);
		}
	}

out:
	k_sem_give(&waiter->wait);
}

static int do_async_http_req(struct http_client_ctx *ctx,
			     enum http_method method,
			     const char *url,
			     const char *content_type,
			     const char *payload)
{
	struct http_client_request req = {};
	struct waiter waiter;
	int ret;

	req.method = method;
	req.url = url;
	req.protocol = " " HTTP_PROTOCOL HTTP_CRLF;

	k_sem_init(&waiter.wait, 0, 1);

	waiter.total_len = 0;

	ret = http_client_send_req(ctx, &req, response, result, sizeof(result),
				   &waiter, APP_REQ_TIMEOUT);
	if (ret < 0 && ret != -EINPROGRESS) {
		NET_ERR("Cannot send %s request (%d)", http_method_str(method),
			ret);
		goto out;
	}

	if (k_sem_take(&waiter.wait, WAIT_TIME)) {
		NET_ERR("Timeout while waiting HTTP response");
		http_client_release(ctx);
		ret = -ETIMEDOUT;
		goto out;
	}

	ret = 0;

out:
	return ret;
}

static inline int do_sync_reqs(struct http_client_ctx *ctx, int count)
{
	int ret;

	/* These examples use the HTTP client API synchronously so they
	 * do not set the callback parameter.
	 */
	while (count--) {
		ret = do_sync_http_req(&http_ctx, HTTP_GET, "/index.html",
				       NULL, NULL);
		if (ret < 0) {
			goto out;
		}

		ret = do_sync_http_req(&http_ctx, HTTP_HEAD, "/",
				       NULL, NULL);
		if (ret < 0) {
			goto out;
		}

		ret = do_sync_http_req(&http_ctx, HTTP_OPTIONS, "/index.html",
				       NULL, NULL);
		if (ret < 0) {
			goto out;
		}

		ret = do_sync_http_req(&http_ctx, HTTP_POST, "/post_test.php",
				       POST_CONTENT_TYPE, POST_PAYLOAD);
		if (ret < 0) {
			goto out;
		}

		/* Note that we cannot receive data bigger than RESULT_BUF_SIZE
		 * if we wait the buffer synchronously. If you want to receive
		 * bigger data, then you need to set the callback when sending
		 * the HTTP request using http_client_send_req()
		 */
	}

out:
	return ret;
}

static inline int do_async_reqs(struct http_client_ctx *ctx, int count)
{
	int ret;

	/* These examples use the HTTP client API asynchronously so they
	 * do set the callback parameter.
	 */
	while (count--) {
		ret = do_async_http_req(&http_ctx, HTTP_GET, "/index.html",
					NULL, NULL);
		if (ret < 0) {
			goto out;
		}

		ret = do_async_http_req(&http_ctx, HTTP_HEAD, "/",
					NULL, NULL);
		if (ret < 0) {
			goto out;
		}

		ret = do_async_http_req(&http_ctx, HTTP_OPTIONS, "/index.html",
					NULL, NULL);
		if (ret < 0) {
			goto out;
		}

		ret = do_async_http_req(&http_ctx, HTTP_POST, "/post_test.php",
					POST_CONTENT_TYPE, POST_PAYLOAD);
		if (ret < 0) {
			goto out;
		}

		ret = do_async_http_req(&http_ctx, HTTP_GET, "/big-file.html",
					NULL, NULL);
		if (ret < 0) {
			goto out;
		}
	}

out:
	return ret;
}

void main(void)
{
	int ret;

	ret = net_sample_app_init("Run HTTP client", 0, APP_STARTUP_TIME);
	if (ret < 0) {
		panic("Application init failed");
	}

	ret = http_client_init(&http_ctx, SERVER_ADDR, SERVER_PORT);
	if (ret < 0) {
		NET_ERR("HTTP init failed (%d)", ret);
		panic(NULL);
	}

	ret = do_sync_reqs(&http_ctx, MAX_ITERATIONS);
	if (ret < 0) {
		goto out;
	}

	ret = do_async_reqs(&http_ctx, MAX_ITERATIONS);
	if (ret < 0) {
		goto out;
	}

out:
	http_client_release(&http_ctx);

	NET_INFO("Done!");
}
