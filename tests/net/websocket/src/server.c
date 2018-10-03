/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_WEBSOCKET)
#define SYS_LOG_DOMAIN "test-ws-server"
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

#include "../../../subsys/net/ip/net_private.h"

#define MAX_BUF_LEN 128
#define MAX_URL_LEN 128
#define SEND_TIMEOUT K_SECONDS(10)
#define ALLOC_TIMEOUT 100

struct http_ctx *ws_ctx;
static struct http_ctx http_ctx;
static struct http_server_urls http_urls;

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

#define APP_BANNER "Run TLS ws-server"
#define INSTANCE_INFO "Zephyr TLS ws-server #1"

/* Note that each net_app context needs its own stack as there will be
 * a separate thread needed.
 */
NET_STACK_DEFINE(WS_ECHO_SERVER, ws_tls_stack,
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

#define HTTP_STATUS_200_OK_CSS \
				"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/css\r\n" \
				"Transfer-Encoding: chunked\r\n"

#define HTML_HEADER "<html><head>"		    \
		"<title>Zephyr HTTP Server</title>" \
		"</head><body><h1>" \
		"<center>Zephyr HTTP websocket server</center></h1>\r\n"

#define HTML_FOOTER "</body></html>\r\n"

static int ws_works(struct http_ctx *ctx, const struct sockaddr *dst)
{
	NET_INFO("WS url called");

	return 0;
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
		return;

	} else if (type == WS_CONNECTION) {
		if (strncmp(ctx->http.url, "/ws",
			    sizeof("/ws") - 1) == 0) {
			ws_works(ctx, dst);
			return;
		}
	}
}

static void ws_received(struct http_ctx *ctx,
			struct net_pkt *pkt,
			int status,
			u32_t flags,
			const struct sockaddr *dst,
			void *user_data)
{
	if (!status) {
		struct net_buf *frag;
		enum ws_opcode opcode;
		int ret, hdr_len;

		NET_DBG("Received %d bytes data", net_pkt_appdatalen(pkt));

		if (flags & WS_FLAG_BINARY) {
			opcode = WS_OPCODE_DATA_BINARY;
		} else {
			opcode = WS_OPCODE_DATA_TEXT;
		}

		hdr_len = net_pkt_get_len(pkt) - net_pkt_appdatalen(pkt);

		frag = pkt->frags;
		while (frag) {
			net_hexdump("server recv", net_pkt_appdata(pkt),
				    net_pkt_appdatalen(pkt));

			ret = ws_send_msg_to_client(ctx, frag->data + hdr_len,
						    frag->len - hdr_len,
						    opcode,
						    frag->frags ? true : false,
						    dst, user_data);
			if (ret < 0) {
				NET_DBG("Cannot send ws data (%d bytes) "
					"back (%d)",
					frag->len - hdr_len, ret);
			} else {
				NET_DBG("Sent %d bytes to client",
					frag->len - hdr_len);
			}

			frag = frag->frags;

			/* Websocket header is found in first fragment so
			 * reset the value here.
			 */
			hdr_len = 0;
		}

		http_send_flush(ctx, user_data);

		net_pkt_unref(pkt);

	} else {
		NET_ERR("Receive error (%d)", status);

		if (pkt) {
			net_pkt_unref(pkt);
		}
	}
}

static void ws_sent(struct http_ctx *ctx,
		    int status,
		    void *user_data_send,
		    void *user_data)
{
	NET_DBG("Data sent status %d", status);
}

static void ws_closed(struct http_ctx *ctx,
		      int status,
		      void *user_data)
{
	NET_DBG("Connection %p closed", ctx);
}

#if defined(CONFIG_NET_DEBUG_WEBSOCKET) && (NET_SYS_LOG_LEVEL > 3)
static const char *get_string(int str_len, const char *str)
{
	static char buf[64];
	int len = min(str_len, sizeof(buf) - 1);

	memcpy(buf, str, len);
	buf[len] = '\0';

	return buf;
}
#endif

static enum http_verdict default_handler(struct http_ctx *ctx,
					 enum http_connection_type type,
					 const struct sockaddr *dst)
{
	NET_DBG("No handler for %s URL %s",
		type == HTTP_CONNECTION ? "HTTP" : "WS",
		get_string(ctx->http.url_len, ctx->http.url));

	return HTTP_VERDICT_DROP;
}

void test_websocket_init_server(void)
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
	http_server_add_url(&http_urls, "/ws", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/ws", HTTP_URL_WEBSOCKET);

	ret = http_server_init(&http_ctx, &http_urls, server_addr,
			       result, sizeof(result),
			       "Zephyr WS server", NULL);
	if (ret < 0) {
		NET_ERR("Cannot init web server (%d)", ret);
		return;
	}

	http_set_cb(&http_ctx, ws_connected, ws_received, ws_sent, ws_closed);

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&http_ctx.app_ctx, tx_tcp_slab, data_tcp_pool);
#endif

#if defined(CONFIG_NET_APP_TLS)
	ret = http_server_set_tls(&http_ctx,
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

	http_server_enable(&http_ctx);

	ws_ctx = &http_ctx;
}
