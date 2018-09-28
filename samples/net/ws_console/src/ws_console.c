/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "ws-console"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

/* Printing debugs from this module looks funny in console so do not
 * do it unless you are debugging this module.
 */
#define EXTRA_DEBUG 0

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>

#include <net/websocket_console.h>

#include "common.h"
#include "config.h"

static struct http_ctx http_ctx;
static struct http_server_urls http_urls;

#define MAX_URL_LEN 128
#define SEND_TIMEOUT K_SECONDS(10)

/* Note that both tcp and udp can share the same pool but in this
 * example the UDP context and TCP context have separate pools.
 */
#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(echo_tx_tcp, 15);
NET_PKT_DATA_POOL_DEFINE(echo_data_tcp, 30);

static struct k_mem_slab *tx_tcp_slab(void)
{
	return &echo_tx_tcp;
}

static struct net_buf_pool *data_tcp_pool(void)
{
	return &echo_data_tcp;
}
#else
#define tx_tcp_slab NULL
#define data_tcp_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

/* The result buf size is set to large enough so that we can receive max size
 * buf back. Note that mbedtls needs also be configured to have equal size
 * value for its buffer size. See MBEDTLS_SSL_MAX_CONTENT_LEN option in TLS
 * config file.
 */
#define RESULT_BUF_SIZE 1500
static u8_t result[RESULT_BUF_SIZE];

#if defined(CONFIG_HTTPS)

#if !defined(CONFIG_NET_APP_TLS_STACK_SIZE)
#define CONFIG_NET_APP_TLS_STACK_SIZE		8192
#endif /* CONFIG_NET_APP_TLS_STACK_SIZE */

#define APP_BANNER "Run TLS ws-console"
#define INSTANCE_INFO "Zephyr TLS ws-console #1"

/* Note that each net_app context needs its own stack as there will be
 * a separate thread needed.
 */
NET_STACK_DEFINE(WS_TLS_CONSOLE, ws_tls_console_stack,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

#define RX_FIFO_DEPTH 4
K_MEM_POOL_DEFINE(ssl_pool, 4, 64, RX_FIFO_DEPTH, 4);
#endif /* CONFIG_HTTPS */

#if defined(CONFIG_HTTPS)
/* Load the certificates and private RSA key. */

static const char echo_apps_cert_der[] = {
#include "echo-apps-cert.der.inc"
};

static const char echo_apps_key_der[] = {
#include "echo-apps-key.der.inc"
};

static int setup_cert(struct net_app_ctx *ctx,
		      mbedtls_x509_crt *cert,
		      mbedtls_pk_context *pkey)
{
	int ret;

	ret = mbedtls_x509_crt_parse(cert, echo_apps_cert_der,
				     sizeof(echo_apps_cert_der));
	if (ret != 0) {
		NET_ERR("mbedtls_x509_crt_parse returned %d", ret);
		return ret;
	}

	ret = mbedtls_pk_parse_key(pkey, echo_apps_key_der,
				   sizeof(echo_apps_key_der), NULL, 0);
	if (ret != 0) {
		NET_ERR("mbedtls_pk_parse_key returned %d", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_HTTPS */

#define HTTP_STATUS_200_OK	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n"

#define HTTP_STATUS_200_OK_GZ	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"Content-Encoding: gzip\r\n"

#define HTTP_STATUS_200_OK_GZ_CSS \
				"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/css\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"Content-Encoding: gzip\r\n"

#define HTTP_STATUS_200_OK_CSS \
				"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/css\r\n" \
				"Transfer-Encoding: chunked\r\n"

#define HTML_HEADER	"<html><head>"				  \
			"<title>Zephyr websocket console</title>" \
			"</head><body><h1>"			  \
			"<center>Zephyr websocket console</center></h1>\r\n"

#define HTML_FOOTER	"</body></html>\r\n"

static int http_response(struct http_ctx *ctx, const char *header,
			 const char *payload, size_t payload_len,
			 const struct sockaddr *dst)
{
	char content_length[6];
	int ret;

	ret = http_add_header(ctx, header, dst, NULL);
	if (ret < 0) {
		NET_ERR("Cannot add HTTP header (%d)", ret);
		return ret;
	}

	ret = snprintk(content_length, sizeof(content_length), "%zd",
		       payload_len);
	if (ret <= 0 || ret >= sizeof(content_length)) {
		ret = -ENOMEM;
		return ret;
	}

	ret = http_add_header_field(ctx, "Content-Length", content_length,
				    dst, NULL);
	if (ret < 0) {
		NET_ERR("Cannot add Content-Length HTTP header (%d)", ret);
		return ret;
	}

	ret = http_add_header(ctx, HTTP_CRLF, dst, NULL);
	if (ret < 0) {
		return ret;
	}

	ret = http_send_chunk(ctx, payload, payload_len, dst, NULL);
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		return ret;
	}

	return http_send_flush(ctx, NULL);
}

static int http_response_soft_404(struct http_ctx *ctx,
				  const struct sockaddr *dst)
{
	static const char *not_found =
		HTML_HEADER
		"<h2><center>404 Not Found</center></h2>"
		HTML_FOOTER;

	return http_response(ctx, HTTP_STATUS_200_OK, not_found,
			     strlen(not_found), dst);
}

static int http_serve_index_html(struct http_ctx *ctx,
				 const struct sockaddr *dst)
{
	static const char index_html[] = {
#include "index.html.gz.inc"
	};

	NET_DBG("Sending index.html (%zd bytes) to client",
		sizeof(index_html));

	return http_response(ctx, HTTP_STATUS_200_OK_GZ, index_html,
			     sizeof(index_html), dst);
}

static int http_serve_style_css(struct http_ctx *ctx,
				const struct sockaddr *dst)
{
	static const char style_css_gz[] = {
#include "style.css.gz.inc"
	};

	NET_DBG("Sending style.css (%zd bytes) to client",
		sizeof(style_css_gz));

	return http_response(ctx, HTTP_STATUS_200_OK_GZ_CSS,
			     style_css_gz, sizeof(style_css_gz),
			     dst);
}

static int http_serve_favicon_ico(struct http_ctx *ctx,
				  const struct sockaddr *dst)
{
	static const char favicon_ico_gz[] = {
#include "favicon.ico.gz.inc"
	};

	NET_DBG("Sending favicon.ico (%zd bytes) to client",
		sizeof(favicon_ico_gz));

	return http_response(ctx, HTTP_STATUS_200_OK_GZ,
			     favicon_ico_gz, sizeof(favicon_ico_gz),
			     dst);
}

static void ws_connected(struct http_ctx *ctx,
			 enum http_connection_type type,
			 const struct sockaddr *dst,
			 void *user_data)
{
	char url[32];
	int len = min(sizeof(url) - 1, ctx->http.url_len);

	memcpy(url, ctx->http.url, len);
	url[len] = '\0';

	NET_DBG("%s connect attempt URL %s",
		type == HTTP_CONNECTION ? "HTTP" : "WS", url);

	if (type == HTTP_CONNECTION) {
		if (strncmp(ctx->http.url, "/index.html",
			    ctx->http.url_len) == 0) {
			http_serve_index_html(ctx, dst);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/style.css",
			    ctx->http.url_len) == 0) {
			http_serve_style_css(ctx, dst);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/favicon.ico",
			    ctx->http.url_len) == 0) {
			http_serve_favicon_ico(ctx, dst);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/",
			    ctx->http.url_len) == 0) {
			http_serve_index_html(ctx, dst);
			http_close(ctx);
			return;
		}

	} else if (type == WS_CONNECTION) {
		if (strncmp(ctx->http.url, "/console",
			    ctx->http.url_len) == 0) {
			ws_console_enable(ctx);
			return;
		}
	}

	/* Give 404 error for all the other URLs we do not want to handle
	 * right now.
	 */
	http_response_soft_404(ctx, dst);
	http_close(ctx);
}

static void ws_received(struct http_ctx *ctx,
			struct net_pkt *pkt,
			int status,
			u32_t flags,
			const struct sockaddr *dst,
			void *user_data)
{
	if (!status) {
		int ret;

#if EXTRA_DEBUG
		NET_DBG("Received %d bytes data", net_pkt_appdatalen(pkt));
#endif

		ret = ws_console_recv(ctx, pkt);
		if (ret < 0) {
			goto out;
		}
	} else {
		NET_ERR("Receive error (%d)", status);

		goto out;
	}

	return;

out:
	if (pkt) {
		net_pkt_unref(pkt);
	}
}

static void ws_sent(struct http_ctx *ctx,
		    int status,
		    void *user_data_send,
		    void *user_data)
{
#if EXTRA_DEBUG
	NET_DBG("Data sent status %d", status);
#endif
}

static void ws_closed(struct http_ctx *ctx,
		      int status,
		      void *user_data)
{
	NET_DBG("Connection %p closed", ctx);

	ws_console_disable(ctx);
}

static const char *get_string(int str_len, const char *str)
{
	static char buf[64];
	int len = min(str_len, sizeof(buf) - 1);

	memcpy(buf, str, len);
	buf[len] = '\0';

	return buf;
}

static enum http_verdict default_handler(struct http_ctx *ctx,
					 enum http_connection_type type,
					 const struct sockaddr *dst)
{
	NET_DBG("No handler for %s URL %s",
		type == HTTP_CONNECTION ? "HTTP" : "WS",
		get_string(ctx->http.url_len, ctx->http.url));

	if (type == HTTP_CONNECTION) {
		http_response_soft_404(ctx, dst);
	}

	return HTTP_VERDICT_DROP;
}

void start_ws_console(void)
{
	struct sockaddr addr, *server_addr;
	int ret;

	/*
	 * There are several options here for binding to local address.
	 * 1) The server address can be left empty in which case the
	 *    library will bind to both IPv4 and IPv6 addresses and to
	 *    default port 80 or 443 if TLS is enabled.
	 * 2) The server address can be partially filled, meaning that
	 *    the address can be left to 0 and port can be set to desired
	 *    value. If the protocol family in sockaddr is set to AF_UNSPEC,
	 *    then both IPv4 and IPv6 socket is bound.
	 * 3) The address can be set to some real value.
	 */
#define ADDR_OPTION 1

#if ADDR_OPTION == 1
	server_addr = NULL;

	ARG_UNUSED(addr);

#elif ADDR_OPTION == 2
	/* Accept any local listening address */
	(void)memset(&addr, 0, sizeof(addr));

	net_sin(&addr)->sin_port = htons(ZEPHYR_PORT);

	/* In this example, listen both IPv4 and IPv6 */
	addr.sa_family = AF_UNSPEC;

	server_addr = &addr;

#elif ADDR_OPTION == 3
	/* Set the bind address according to your configuration */
	(void)memset(&addr, 0, sizeof(addr));

	/* In this example, listen only IPv6 */
	addr.sa_family = AF_INET6;
	net_sin6(&addr)->sin6_port = htons(ZEPHYR_PORT);

	ret = net_ipaddr_parse(ZEPHYR_ADDR, &addr);
	if (ret < 0) {
		NET_ERR("Cannot set local address (%d)", ret);
		panic(NULL);
	}

	server_addr = &addr;

#else
	server_addr = NULL;

	ARG_UNUSED(addr);
#endif

	http_server_add_default(&http_urls, default_handler);
	http_server_add_url(&http_urls, "/", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/index.html", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/style.css", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/favicon.ico", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/console", HTTP_URL_WEBSOCKET);

	ret = http_server_init(&http_ctx, &http_urls, server_addr,
			       result, sizeof(result),
			       "Zephyr WS console", NULL);
	if (ret < 0) {
		NET_ERR("Cannot init web server (%d)", ret);
		return;
	}

	http_set_cb(&http_ctx, ws_connected, ws_received, ws_sent, ws_closed);

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&http_ctx.app_ctx, tx_tcp_slab,
				 data_tcp_pool);
#endif

#if defined(CONFIG_HTTPS)
	ret = http_server_set_tls(&http_ctx,
				  APP_BANNER,
				  INSTANCE_INFO,
				  strlen(INSTANCE_INFO),
				  setup_cert,
				  NULL,
				  &ssl_pool,
				  ws_tls_console_stack,
				  K_THREAD_STACK_SIZEOF(ws_tls_console_stack));
	if (ret < 0) {
		NET_ERR("Cannot enable TLS support (%d)", ret);
	}
#endif

	http_server_enable(&http_ctx);
}

void stop_ws_console(void)
{
	http_server_disable(&http_ctx);
	http_release(&http_ctx);
}
