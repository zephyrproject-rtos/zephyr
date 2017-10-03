/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#define SYS_LOG_DOMAIN "ws-echo-server"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <errno.h>
#include <stdio.h>

#include <net/net_pkt.h>
#include <net/net_core.h>
#include <net/net_context.h>

#include <net/websocket.h>

#include "common.h"
#include "config.h"

static struct ws_ctx ws;
static struct ws_server_urls ws_urls;

#define MAX_URL_LEN 128
#define SEND_TIMEOUT K_SECONDS(10)
#define HTTP_CRLF "\r\n"

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

#if defined(CONFIG_WS_TLS)

#if !defined(CONFIG_NET_APP_TLS_STACK_SIZE)
#define CONFIG_NET_APP_TLS_STACK_SIZE		8192
#endif /* CONFIG_NET_APP_TLS_STACK_SIZE */

#define APP_BANNER "Run TLS ws-server"
#define INSTANCE_INFO "Zephyr TLS ws-server #1"

/* Note that each net_app context needs its own stack as there will be
 * a separate thread needed.
 */
NET_STACK_DEFINE(WS_TLS, ws_tls_stack,
		 CONFIG_NET_APP_TLS_STACK_SIZE, CONFIG_NET_APP_TLS_STACK_SIZE);

#define RX_FIFO_DEPTH 4
K_MEM_POOL_DEFINE(ssl_pool, 4, 64, RX_FIFO_DEPTH, 4);
#endif /* CONFIG_NET_APP_TLS */

#if defined(CONFIG_NET_APP_TLS)
/* Load the certificates and private RSA key. */

#include "file_echo-apps-cert.der.h"
#include "file_echo-apps-key.der.h"

static int setup_cert(struct net_app_ctx *ctx,
		      mbedtls_x509_crt *cert,
		      mbedtls_pk_context *pkey)
{
	int ret;

	ret = mbedtls_x509_crt_parse(cert, file_echo_apps_cert_der,
				     sizeof(file_echo_apps_cert_der));
	if (ret != 0) {
		NET_ERR("mbedtls_x509_crt_parse returned %d", ret);
		return ret;
	}

	ret = mbedtls_pk_parse_key(pkey, file_echo_apps_key_der,
				   sizeof(file_echo_apps_key_der), NULL, 0);
	if (ret != 0) {
		NET_ERR("mbedtls_pk_parse_key returned %d", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_NET_APP_TLS */

#define HTTP_STATUS_200_OK	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"\r\n"

#define HTML_HEADER		"<html><head>" \
				"<title>Zephyr HTTP Server</title>" \
				"</head><body><h1>" \
				"<center>Zephyr HTTP server</center></h1>\r\n"

#define HTML_FOOTER		"</body></html>\r\n"

static int http_response(struct ws_ctx *ctx, const char *header,
			 const char *payload)
{
	struct net_pkt *pkt = NULL;
	int ret;

	ret = ws_add_http_header(ctx, &pkt, header);
	if (ret < 0) {
		NET_ERR("Cannot add HTTP header (%d)", ret);
		return ret;
	}

	ws_add_http_data(ctx, &pkt, HTTP_CRLF, strlen(HTTP_CRLF));
	ws_add_http_data(ctx, &pkt, payload, strlen(payload));

	return ws_send_data_http(ctx, pkt, NULL);
}

static int http_response_it_works(struct ws_ctx *ctx)
{
	const char *it_works = "<body><h2><center>It Works!</center></h2>";

	return http_response(ctx, HTTP_STATUS_200_OK, it_works);
}

static int http_response_soft_404(struct ws_ctx *ctx)
{
	return http_response(ctx, HTTP_STATUS_200_OK, HTML_HEADER
			     "<h2><center>404 Not Found</center></h2>"
			     HTML_FOOTER);
}

static int http_ws_works(struct ws_ctx *ctx)
{
	NET_INFO("WS url called");

	return 0;
}

static void ws_connected(struct ws_ctx *ctx,
			 enum ws_connection_type type,
			 void *user_data)
{
	char url[32];
	int len = min(sizeof(url), ctx->http.url_len);

	memcpy(url, ctx->http.url, len);
	url[len] = '\0';

	NET_DBG("%s connect attempt URL %s",
		type == WS_HTTP_CONNECT ? "HTTP" : "WS", url);

	if (type == WS_HTTP_CONNECT) {
		if (strncmp(ctx->http.url, "/index.html",
			    ctx->http.url_len) == 0) {
			http_response_it_works(ctx);
			return;
		}
	} else {
		if (strncmp(ctx->http.url, "/index.ws",
			    ctx->http.url_len) == 0) {
			http_ws_works(ctx);
			return;
		}
	}
}

static void ws_received(struct ws_ctx *ctx,
			struct net_pkt *pkt,
			int status,
			u32_t flags,
			void *user_data)
{
	if (!status) {
		enum ws_opcode opcode;
		int ret;

		NET_DBG("Received %d bytes data", net_pkt_appdatalen(pkt));

		if (flags & WS_FLAG_BINARY) {
			opcode = WS_OPCODE_DATA_BINARY;
		} else {
			opcode = WS_OPCODE_DATA_TEXT;
		}

		ret = ws_send_data_to_client(ctx, pkt, opcode, true,
					     user_data);
		if (ret < 0) {
			NET_DBG("Cannot send ws data (%d bytes) back (%d)",
				net_pkt_appdatalen(pkt), ret);

			if (pkt) {
				net_pkt_unref(pkt);
			}
		}
	} else {
		NET_ERR("Receive error (%d)", status);

		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

static void ws_sent(struct ws_ctx *ctx,
		    int status,
		    void *user_data_send,
		    void *user_data)
{
	NET_DBG("Data sent status %d", status);
}

static void ws_closed(struct ws_ctx *ctx,
		      int status,
		      void *user_data)
{
	NET_DBG("Connection %p closed", ctx);
}

static const char *get_string(int str_len, const char *str)
{
	static char buf[64];
	int len = min(str_len, sizeof(buf) - 1);

	memcpy(buf, str, len);
	buf[len] = '\0';

	return buf;
}

static enum ws_verdict default_handler(struct ws_ctx *ctx,
				       enum ws_connection_type type)
{
	NET_DBG("No handler for %s URL %s",
		type == WS_HTTP_CONNECT ? "HTTP" : "WS",
		get_string(ctx->http.url_len, ctx->http.url));

	if (type == WS_HTTP_CONNECT) {
		http_response_soft_404(ctx);
	}

	return WS_VERDICT_DROP;
}

void start_server(void)
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
	memset(&addr, 0, sizeof(addr));

	net_sin(&addr)->sin_port = htons(ZEPHYR_PORT);

	/* In this example, listen both IPv4 and IPv6 */
	addr.sa_family = AF_UNSPEC;

	server_addr = &addr;

#elif ADDR_OPTION == 3
	/* Set the bind address according to your configuration */
	memset(&addr, 0, sizeof(addr));

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

	ws_server_add_default(&ws_urls, default_handler);
	ws_server_add_url(&ws_urls, "/index.html", HTTP_URL_STANDARD);
	ws_server_add_url(&ws_urls, "/index.ws", HTTP_URL_WS);

	ret = ws_server_init(&ws, &ws_urls, server_addr,
			     result, sizeof(result),
			     "Zephyr WS server", NULL);
	if (ret < 0) {
		NET_ERR("Cannot init web server (%d)", ret);
		return;
	}

	ws_set_cb(&ws, ws_connected, ws_received, ws_sent, ws_closed);

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&ws.app_ctx, tx_tcp_slab, data_tcp_pool);
#endif

#if defined(CONFIG_NET_APP_TLS)
	ret = ws_server_set_tls(&ws,
				APP_BANNER,
				INSTANCE_INFO,
				strlen(INSTANCE_INFO),
				setup_cert,
				NULL,
				&ssl_pool,
				ws_tls_stack,
				K_THREAD_STACK_SIZEOF(ws_tls_stack));
	if (ret < 0) {
		NET_ERR("Cannot enable TLS support (%d)", ret);
	}
#endif

	ws_server_enable(&ws);
}

void stop_server(void)
{
	ws_server_disable(&ws);
	ws_release(&ws);
}
