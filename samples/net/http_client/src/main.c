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

#include <net/net_app.h>

#include "config.h"

#define MAX_ITERATIONS	20
#define WAIT_TIME (APP_REQ_TIMEOUT * 2)

/* The OPTIONS method is problematic as the HTTP server might not support
 * it so turn it off by default.
 */
#define SEND_OPTIONS 0

/*
 * Note that the http_ctx is quite large so be careful if that is
 * allocated from stack.
 */
static struct http_ctx http_ctx;

#if defined(CONFIG_HTTPS)
#define HOSTNAME "localhost"   /* for cert verification if that is enabled */

/* The result buf size is set to large enough so that we can receive max size
 * buf back. Note that mbedtls needs also be configured to have equal size
 * value for its buffer size. See MBEDTLS_SSL_MAX_CONTENT_LEN option in TLS
 * config file.
 */
#define RESULT_BUF_SIZE MBEDTLS_SSL_MAX_CONTENT_LEN
static u8_t https_result_buf[RESULT_BUF_SIZE];

#define INSTANCE_INFO "Zephyr HTTPS client #1"

#define HTTPS_STACK_SIZE (8 * 1024)

NET_STACK_DEFINE(HTTPS, https_stack, HTTPS_STACK_SIZE, HTTPS_STACK_SIZE);

NET_APP_TLS_POOL_DEFINE(ssl_pool, 10);
#endif /* CONFIG_HTTPS */

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(http_cli_tx, 15);
NET_PKT_DATA_POOL_DEFINE(http_cli_data, 30);

static struct k_mem_slab *tx_slab(void)
{
	return &http_cli_tx;
}

static struct net_buf_pool *data_pool(void)
{
	return &http_cli_data;
}
#else
#if defined(CONFIG_NET_L2_BT)
#error "TCP connections over Bluetooth need CONFIG_NET_CONTEXT_NET_PKT_POOL "\
	"defined."
#endif /* CONFIG_NET_L2_BT */

#define tx_slab NULL
#define data_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#if !defined(RESULT_BUF_SIZE)
#define RESULT_BUF_SIZE 1600
#endif

/* This will contain the returned HTTP response to a sent request */
static u8_t result[RESULT_BUF_SIZE];

struct waiter {
	struct http_ctx *ctx;
	struct k_sem wait;
	size_t total_len;
	size_t header_len;
};

#if defined(CONFIG_HTTPS)
/* Load the certificates etc. In this sample app, we verify that
 * the server is the test server we are communicating against to.
 */

static const char echo_apps_cert_der[] = {
#include "echo-apps-cert.der.inc"
};

#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
static const unsigned char client_psk[] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
	0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
};

static const char client_psk_id[] = "Client_identity";
#endif

static int setup_cert(struct net_app_ctx *ctx, void *cert)
{
#if defined(MBEDTLS_KEY_EXCHANGE__SOME__PSK_ENABLED)
	mbedtls_ssl_conf_psk(&ctx->tls.mbedtls.conf,
			     client_psk, sizeof(client_psk),
			     (const unsigned char *)client_psk_id,
			     sizeof(client_psk_id) - 1);
#endif

#if defined(MBEDTLS_X509_CRT_PARSE_C)
	{
		mbedtls_x509_crt *ca_cert = cert;
		int ret;

		ret = mbedtls_x509_crt_parse_der(ca_cert,
						 echo_apps_cert_der,
						 sizeof(echo_apps_cert_der));
		if (ret != 0) {
			NET_ERR("mbedtls_x509_crt_parse_der failed "
				"(-0x%x)", -ret);
			return ret;
		}

		mbedtls_ssl_conf_ca_chain(&ctx->tls.mbedtls.conf,
					  ca_cert, NULL);
		mbedtls_ssl_conf_authmode(&ctx->tls.mbedtls.conf,
					  MBEDTLS_SSL_VERIFY_REQUIRED);

		mbedtls_ssl_conf_cert_profile(&ctx->tls.mbedtls.conf,
					    &mbedtls_x509_crt_profile_default);
	}
#endif /* MBEDTLS_X509_CRT_PARSE_C */

	return 0;
}
#endif /* CONFIG_HTTPS */

static int do_sync_http_req(struct http_ctx *ctx,
			    enum http_method method,
			    const char *url,
			    const char *content_type,
			    const char *payload,
			    int count)
{
	struct http_request req = {};
	int ret;

	req.method = method;
	req.url = url;
	req.protocol = " " HTTP_PROTOCOL;

	NET_INFO("[%d] Send %s", count, url);

	ret = http_client_send_req(ctx, &req, NULL, result, sizeof(result),
				   NULL, APP_REQ_TIMEOUT);
	if (ret < 0) {
		NET_ERR("Cannot send %s request (%d)", http_method_str(method),
			ret);
		goto out;
	}

	if (ctx->http.rsp.data_len > sizeof(result)) {
		NET_ERR("Result buffer overflow by %zd bytes",
		       ctx->http.rsp.data_len - sizeof(result));

		ret = -E2BIG;
	} else {
		NET_INFO("HTTP server response status: %s",
			 ctx->http.rsp.http_status);

		if (ctx->http.parser.http_errno) {
			if (method == HTTP_OPTIONS) {
				/* Ignore error if OPTIONS is not found */
				goto out;
			}

			NET_INFO("HTTP parser status: %s",
			       http_errno_description(
				       ctx->http.parser.http_errno));
			ret = -EINVAL;
			goto out;
		}

		if (method != HTTP_HEAD) {
			if (ctx->http.rsp.body_found) {
				NET_INFO("HTTP body: %zd bytes, "
					 "expected: %zd bytes",
					 ctx->http.rsp.processed,
					 ctx->http.rsp.content_length);
			} else {
				NET_ERR("Error detected during HTTP msg "
					"processing");
			}
		}
	}

out:
	http_close(ctx);

	return ret;
}

void response(struct http_ctx *ctx,
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

		if (ctx->http.rsp.body_start) {
			/* This fragment contains the start of the body
			 * Note that the header length is not proper if
			 * the header is spanning over multiple recv
			 * fragments.
			 */
			waiter->header_len = ctx->http.rsp.body_start -
				ctx->http.rsp.response_buf;
		}

		return;
	}

	waiter->total_len += datalen;

	NET_INFO("HTTP server response status: %s", ctx->http.rsp.http_status);

	if (ctx->http.parser.http_errno) {
		if (ctx->http.req.method == HTTP_OPTIONS) {
			/* Ignore error if OPTIONS is not found */
			goto out;
		}

		NET_INFO("HTTP parser status: %s",
			 http_errno_description(ctx->http.parser.http_errno));
		ret = -EINVAL;
		goto out;
	}

	if (ctx->http.req.method != HTTP_HEAD &&
	    ctx->http.req.method != HTTP_OPTIONS) {
		if (ctx->http.rsp.body_found) {
			NET_INFO("HTTP body: %zd bytes, expected: %zd bytes",
				 ctx->http.rsp.processed,
				 ctx->http.rsp.content_length);
		} else {
			NET_ERR("Error detected during HTTP msg processing");
		}

		if (waiter->total_len !=
		    waiter->header_len + ctx->http.rsp.content_length) {
			NET_ERR("Error while receiving data, "
				"received %zd expected %zd bytes",
				waiter->total_len, waiter->header_len +
				ctx->http.rsp.content_length);
		}
	}

out:
	k_sem_give(&waiter->wait);
}

static int do_async_http_req(struct http_ctx *ctx,
			     enum http_method method,
			     const char *url,
			     const char *content_type,
			     const char *payload,
			     int count)
{
	struct http_request req = {};
	struct waiter waiter;
	int ret;

	req.method = method;
	req.url = url;
	req.protocol = " " HTTP_PROTOCOL;

	k_sem_init(&waiter.wait, 0, 1);

	waiter.total_len = 0;

	NET_INFO("[%d] Send %s", count, url);

	ret = http_client_send_req(ctx, &req, response, result, sizeof(result),
				   &waiter, APP_REQ_TIMEOUT);
	if (ret < 0 && ret != -EINPROGRESS) {
		NET_ERR("Cannot send %s request (%d)", http_method_str(method),
			ret);
		goto out;
	}

	if (k_sem_take(&waiter.wait, WAIT_TIME)) {
		NET_ERR("Timeout while waiting HTTP response");
		ret = -ETIMEDOUT;
		http_request_cancel(ctx);
		goto out;
	}

	ret = 0;

out:
	http_close(ctx);

	return ret;
}

static inline int do_sync_reqs(struct http_ctx *ctx, int count)
{
	int max_count = count;
	int ret;

	/* These examples use the HTTP client API synchronously so they
	 * do not set the callback parameter.
	 */
	while (count--) {
		ret = do_sync_http_req(&http_ctx, HTTP_GET, "/index.html",
				       NULL, NULL, max_count - count);
		if (ret < 0) {
			goto out;
		}

		ret = do_sync_http_req(&http_ctx, HTTP_HEAD, "/",
				       NULL, NULL, max_count - count);
		if (ret < 0) {
			goto out;
		}

#if SEND_OPTIONS
		ret = do_sync_http_req(&http_ctx, HTTP_OPTIONS, "/index.html",
				       NULL, NULL, max_count - count);
		if (ret < 0) {
			goto out;
		}
#endif

		ret = do_sync_http_req(&http_ctx, HTTP_POST, "/post_test.php",
				       POST_CONTENT_TYPE, POST_PAYLOAD,
				       max_count - count);
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

static inline int do_async_reqs(struct http_ctx *ctx, int count)
{
	int max_count = count;
	int ret;

	/* These examples use the HTTP client API asynchronously so they
	 * do set the callback parameter.
	 */
	while (count--) {
		ret = do_async_http_req(&http_ctx, HTTP_GET, "/index.html",
					NULL, NULL, max_count - count);
		if (ret < 0) {
			goto out;
		}

		ret = do_async_http_req(&http_ctx, HTTP_HEAD, "/",
					NULL, NULL, max_count - count);
		if (ret < 0) {
			goto out;
		}

#if SEND_OPTIONS
		ret = do_async_http_req(&http_ctx, HTTP_OPTIONS, "/index.html",
					NULL, NULL, max_count - count);
		if (ret < 0) {
			goto out;
		}
#endif

		ret = do_async_http_req(&http_ctx, HTTP_POST, "/post_test.php",
					POST_CONTENT_TYPE, POST_PAYLOAD,
					max_count - count);
		if (ret < 0) {
			goto out;
		}

		ret = do_async_http_req(&http_ctx, HTTP_GET, "/big-file.html",
					NULL, NULL, max_count - count);
		if (ret < 0) {
			goto out;
		}
	}

out:
	return ret;
}

static void http_received(struct http_ctx *ctx,
			  struct net_pkt *pkt,
			  int status,
			  u32_t flags,
			  void *user_data)
{
	if (!status) {
		if (pkt) {
			NET_DBG("Received %d bytes data",
				net_pkt_appdatalen(pkt));
			net_pkt_unref(pkt);
		}
	} else {
		NET_ERR("Receive error (%d)", status);

		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

void main(void)
{
	int ret;

	ret = http_client_init(&http_ctx, SERVER_ADDR, SERVER_PORT, NULL,
			       K_FOREVER);
	if (ret < 0) {
		NET_ERR("HTTP init failed (%d)", ret);
		goto out;
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&http_ctx.app_ctx, tx_slab, data_pool);
#endif

	http_set_cb(&http_ctx, NULL, http_received, NULL, NULL);

#if defined(CONFIG_HTTPS)
	ret = http_client_set_tls(&http_ctx,
				  https_result_buf,
				  sizeof(https_result_buf),
				  (u8_t *)INSTANCE_INFO,
				  strlen(INSTANCE_INFO),
				  setup_cert,
				  HOSTNAME,
				  NULL,
				  &ssl_pool,
				  https_stack,
				  K_THREAD_STACK_SIZEOF(https_stack));
	if (ret < 0) {
		NET_ERR("HTTPS init failed (%d)", ret);
		goto out;
	}
#endif

	NET_INFO("--------Sending %d sync request--------", MAX_ITERATIONS);

	ret = do_sync_reqs(&http_ctx, MAX_ITERATIONS);
	if (ret < 0) {
		goto out;
	}

	NET_INFO("--------Sending %d async request--------", MAX_ITERATIONS);

	ret = do_async_reqs(&http_ctx, MAX_ITERATIONS);
	if (ret < 0) {
		goto out;
	}

out:
	http_release(&http_ctx);

	NET_INFO("Done!");
}
