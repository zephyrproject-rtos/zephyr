/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if 1
#if defined(CONFIG_HTTPS)
#define SYS_LOG_DOMAIN "https-server"
#else
#define SYS_LOG_DOMAIN "http-server"
#endif
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1
#endif

#include <zephyr.h>
#include <stdio.h>

#include <net/net_context.h>

#include <net/http.h>

#include "config.h"

/* Sets the network parameters */

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
NET_PKT_TX_SLAB_DEFINE(http_srv_tx, 15);
NET_PKT_DATA_POOL_DEFINE(http_srv_data, 30);

static struct k_mem_slab *tx_slab(void)
{
	return &http_srv_tx;
}

static struct net_buf_pool *data_pool(void)
{
	return &http_srv_data;
}
#else
#if defined(CONFIG_NET_L2_BT)
#error "TCP connections over Bluetooth need CONFIG_NET_CONTEXT_NET_PKT_POOL "\
	"defined."
#endif /* CONFIG_NET_L2_BT */

#define tx_slab NULL
#define data_pool NULL
#endif /* CONFIG_NET_CONTEXT_NET_PKT_POOL */

#define RESULT_BUF_SIZE 1500
static u8_t http_result[RESULT_BUF_SIZE];

#if defined(CONFIG_HTTPS)
#if !defined(CONFIG_HTTPS_STACK_SIZE)
#define CONFIG_HTTPS_STACK_SIZE		8192
#endif /* CONFIG_HTTPS_STACK_SIZE */

#define APP_BANNER "Run HTTPS server"
#define INSTANCE_INFO "Zephyr HTTPS example server #1"

/* Note that each HTTPS context needs its own stack as there will be
 * a separate thread for each HTTPS context.
 */
NET_STACK_DEFINE(HTTPS, https_stack, CONFIG_NET_APP_TLS_STACK_SIZE,
		 CONFIG_NET_APP_TLS_STACK_SIZE);

#define RX_FIFO_DEPTH 4
K_MEM_POOL_DEFINE(ssl_rx_pool, 4, 64, RX_FIFO_DEPTH, 4);

#else /* CONFIG_HTTPS */
#define APP_BANNER "Run HTTP server"
#endif /* CONFIG_HTTPS */

/*
 * Note that the http_server_ctx and http_server_urls are quite large so be
 * careful if those are allocated from stack.
 */
static struct http_ctx http_ctx;
static struct http_server_urls http_urls;

void panic(const char *msg)
{
	if (msg) {
		NET_ERR("%s", msg);
	}

	for (;;) {
		k_sleep(K_FOREVER);
	}
}

#define HTTP_STATUS_200_OK	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n"

#define HTTP_STATUS_200_OK_GZ	"HTTP/1.1 200 OK\r\n" \
				"Content-Type: text/html\r\n" \
				"Transfer-Encoding: chunked\r\n" \
				"Content-Encoding: gzip\r\n"

#define HTML_HEADER		"<html><head>" \
				"<title>Zephyr HTTP Server</title>" \
				"</head><body><h1>" \
				"<center>Zephyr HTTP server</center></h1>\r\n"

#define HTML_FOOTER		"</body></html>\r\n"

static int http_response(struct http_ctx *ctx, const char *header,
			 const char *payload, size_t payload_len,
			 char *str)
{
	int ret;

	ret = http_add_header(ctx, header, str);
	if (ret < 0) {
		NET_ERR("Cannot add HTTP header (%d)", ret);
		return ret;
	}

	ret = http_add_header(ctx, HTTP_CRLF, str);
	if (ret < 0) {
		return ret;
	}

	ret = http_send_chunk(ctx, payload, payload_len, str);
	if (ret < 0) {
		NET_ERR("Cannot send data to peer (%d)", ret);
		return ret;
	}

	return http_send_flush(ctx, str);
}

static int http_response_soft_404(struct http_ctx *ctx)
{
	static const char *not_found =
		HTML_HEADER
		"<h2><center>404 Not Found</center></h2>"
		HTML_FOOTER;

	return http_response(ctx, HTTP_STATUS_200_OK, not_found,
			     strlen(not_found), "Error");
}

/* Prints the received HTTP header fields as an HTML list */
static void print_http_headers(struct http_ctx *ctx,
			       char *str, int size)
{
	int offset;
	int ret;

	ret = snprintf(str, size,
		       HTML_HEADER
		       "<h2>HTTP Header Fields</h2>\r\n<ul>\r\n");
	if (ret < 0 || ret >= size) {
		return;
	}

	offset = ret;

	for (u8_t i = 0; i < ctx->http.field_values_ctr; i++) {
		struct http_field_value *kv = &ctx->http.field_values[i];

		ret = snprintf(str + offset, size - offset,
			       "<li>%.*s: %.*s</li>\r\n",
			       kv->key_len, kv->key,
			       kv->value_len, kv->value);
		if (ret < 0 || ret >= size) {
			return;
		}

		offset += ret;
	}

	ret = snprintf(str + offset, size - offset, "</ul>\r\n");
	if (ret < 0 || ret >= size) {
		return;
	}

	offset += ret;

	ret = snprintf(str + offset, size - offset,
		       "<h2>HTTP Method: %s</h2>\r\n",
		       http_method_str(ctx->http.parser.method));
	if (ret < 0 || ret >= size) {
		return;
	}

	offset += ret;

	ret = snprintf(str + offset, size - offset,
		       "<h2>URL: %.*s</h2>\r\n",
		       ctx->http.url_len, ctx->http.url);
	if (ret < 0 || ret >= size) {
		return;
	}

	offset += ret;

	snprintf(str + offset, size - offset,
		 "<h2>Server: %s</h2>"HTML_FOOTER, CONFIG_ARCH);
}

int http_serve_headers(struct http_ctx *ctx)
{
#define HTTP_MAX_BODY_STR_SIZE		1024
	static char html_body[HTTP_MAX_BODY_STR_SIZE];

	print_http_headers(ctx, html_body, HTTP_MAX_BODY_STR_SIZE);

	return http_response(ctx, HTTP_STATUS_200_OK, html_body,
			     strlen(html_body), "Headers");
}

static int http_serve_index_html(struct http_ctx *ctx)
{
	static const char index_html[] = {
#include "index.html.inc"
	};

	NET_DBG("Sending index.html (%zd bytes) to client",
		sizeof(index_html));

	return http_response(ctx, HTTP_STATUS_200_OK, index_html,
			     sizeof(index_html), "Index");
}

static void http_connected(struct http_ctx *ctx,
			   enum http_connection_type type,
			   void *user_data)
{
	char url[32];
	int len = min(sizeof(url) - 1, ctx->http.url_len);

	memcpy(url, ctx->http.url, len);
	url[len] = '\0';

	NET_DBG("%s connect attempt URL %s",
		type == HTTP_CONNECTION ? "HTTP" : "WS", url);

	if (type == HTTP_CONNECTION) {
		if (strncmp(ctx->http.url, "/",
			    ctx->http.url_len) == 0) {
			http_serve_index_html(ctx);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/index.html",
			    ctx->http.url_len) == 0) {
			http_serve_index_html(ctx);
			http_close(ctx);
			return;
		}

		if (strncmp(ctx->http.url, "/headers",
			    ctx->http.url_len) == 0) {
			http_serve_headers(ctx);
			http_close(ctx);
			return;
		}
	}

	/* Give 404 error for all the other URLs we do not want to handle
	 * right now.
	 */
	http_response_soft_404(ctx);
	http_close(ctx);
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

static void http_sent(struct http_ctx *ctx,
		      int status,
		      void *user_data_send,
		      void *user_data)
{
	NET_DBG("%s sent", (char *)user_data_send);
}

static void http_closed(struct http_ctx *ctx,
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

static enum http_verdict default_handler(struct http_ctx *ctx,
					 enum http_connection_type type)
{
	NET_DBG("No handler for %s URL %s",
		type == HTTP_CONNECTION ? "HTTP" : "WS",
		get_string(ctx->http.url_len, ctx->http.url));

	if (type == HTTP_CONNECTION) {
		http_response_soft_404(ctx);
	}

	return HTTP_VERDICT_DROP;
}


#if defined(CONFIG_HTTPS)
/* Load the certificates and private RSA key. */

static const char echo_apps_cert_der[] = {
#include "echo-apps-cert.der.inc"
};

static const char echo_apps_key_der[] = {
#include "echo-apps-key.der.inc"
};

static int setup_cert(struct net_app_ctx *app_ctx,
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
				   sizeof(echo_apps_key_der),
				   NULL, 0);
	if (ret != 0) {
		NET_ERR("mbedtls_pk_parse_key returned %d", ret);
		return ret;
	}

	return 0;
}
#endif /* CONFIG_HTTPS */

void main(void)
{
	struct sockaddr addr, *server_addr;
	int ret;

	/*
	 * There are several options here for binding to local address.
	 * 1) The server address can be left empty in which case the
	 *    library will bind to both IPv4 and IPv6 addresses and to
	 *    port 80 which is the default.
	 * 2) The server address can be partially filled, meaning that
	 *    the address can be left to 0 and port can be set if a value
	 *    other than 80 is desired. If the protocol family in sockaddr
	 *    is set to AF_UNSPEC, then both IPv4 and IPv6 socket is bound.
	 * 3) The address can be set to some real value. There is a helper
	 *    function that can be used to fill the socket address struct.
	 */
#define ADDR_OPTION 1

#if ADDR_OPTION == 1
	server_addr = NULL;

	ARG_UNUSED(addr);

#elif ADDR_OPTION == 2
	/* Accept any local listening address */
	memset(&addr, 0, sizeof(addr));

	net_sin(&addr)->sin_port = htons(ZEPHYR_PORT);

	/* In this example, listen only IPv4 */
	addr.family = AF_INET;

	server_addr = &addr;

#elif ADDR_OPTION == 3
	/* Set the bind address according to your configuration */
	memset(&addr, 0, sizeof(addr));

	/* In this example, listen only IPv6 */
	addr.family = AF_INET6;

	ret = http_server_set_local_addr(&addr, ZEPHYR_ADDR, ZEPHYR_PORT);
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
	http_server_add_url(&http_urls, "/headers", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/index.html", HTTP_URL_STANDARD);
	http_server_add_url(&http_urls, "/", HTTP_URL_STANDARD);

	ret = http_server_init(&http_ctx, &http_urls, server_addr, http_result,
			       sizeof(http_result), "Zephyr HTTP Server",
			       NULL);
	if (ret < 0) {
		NET_ERR("Cannot initialize HTTP server (%d)", ret);
		panic(NULL);
	}

#if defined(CONFIG_NET_CONTEXT_NET_PKT_POOL)
	net_app_set_net_pkt_pool(&http_ctx.app_ctx, tx_slab, data_pool);
#endif

	http_set_cb(&http_ctx, http_connected, http_received,
		    http_sent, http_closed);

#if defined(CONFIG_HTTPS)
	ret = http_server_set_tls(&http_ctx,
				  APP_BANNER,
				  INSTANCE_INFO,
				  strlen(INSTANCE_INFO),
				  setup_cert,
				  NULL,
				  &ssl_rx_pool,
				  https_stack,
				  K_THREAD_STACK_SIZEOF(https_stack));
	if (ret < 0) {
		NET_ERR("Cannot enable TLS support (%d)", ret);
	}
#endif

	/*
	 * If needed, the HTTP parser callbacks can be set according to
	 * applications own needs before enabling the server. In this example
	 * we use the default callbacks defined in HTTP server API.
	 */

	http_server_enable(&http_ctx);
}
