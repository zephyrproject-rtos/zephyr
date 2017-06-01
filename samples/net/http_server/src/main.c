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

/* For Basic auth, we need base64 decoder which can be found
 * in mbedtls library.
 */
#include <mbedtls/base64.h>

#include <net/net_context.h>
#include <net/http.h>

#include <net_sample_app.h>

#include "config.h"

/* Sets the network parameters */

#define RESULT_BUF_SIZE 1024
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
NET_STACK_DEFINE(HTTPS, https_stack, CONFIG_HTTPS_STACK_SIZE,
		 CONFIG_HTTPS_STACK_SIZE);

#define RX_FIFO_DEPTH 4
K_MEM_POOL_DEFINE(ssl_rx_pool, 4, 64, RX_FIFO_DEPTH, 4);

static struct http_server_ctx https_ctx;

static u8_t https_result[RESULT_BUF_SIZE];

#else /* CONFIG_HTTPS */
#define APP_BANNER "Run HTTP server"
#endif /* CONFIG_HTTPS */

/*
 * Note that the http_server_ctx and http_server_urls are quite large so be
 * careful if those are allocated from stack.
 */
static struct http_server_ctx http_ctx;
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
				"Transfer-Encoding: chunked\r\n" \
				"\r\n"

#define HTTP_401_STATUS_US	"HTTP/1.1 401 Unauthorized status\r\n" \
				"WWW-Authenticate: Basic realm=" \
				"\""HTTP_AUTH_REALM"\"\r\n\r\n"

#define HTML_HEADER		"<html><head>" \
				"<title>Zephyr HTTP Server</title>" \
				"</head><body><h1>" \
				"<center>Zephyr HTTP server</center></h1>\r\n"

#define HTML_FOOTER		"</body></html>\r\n"

/* Prints the received HTTP header fields as an HTML list */
static void print_http_headers(struct http_server_ctx *ctx,
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

	for (u8_t i = 0; i < ctx->req.field_values_ctr; i++) {
		struct http_field_value *kv = &ctx->req.field_values[i];

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
		       http_method_str(ctx->req.parser.method));
	if (ret < 0 || ret >= size) {
		return;
	}

	offset += ret;

	ret = snprintf(str + offset, size - offset,
		       "<h2>URL: %.*s</h2>\r\n",
		       ctx->req.url_len, ctx->req.url);
	if (ret < 0 || ret >= size) {
		return;
	}

	offset += ret;

	snprintf(str + offset, size - offset,
		 "<h2>Server: %s</h2>"HTML_FOOTER, CONFIG_ARCH);
}

int http_response_header_fields(struct http_server_ctx *ctx)
{
#define HTTP_MAX_BODY_STR_SIZE		1024
	static char html_body[HTTP_MAX_BODY_STR_SIZE];

	print_http_headers(ctx, html_body, HTTP_MAX_BODY_STR_SIZE);

	return http_response(ctx, HTTP_STATUS_200_OK, html_body);
}

static int http_response_it_works(struct http_server_ctx *ctx)
{
	/* Let the peer close the connection but if it does not do it
	 * close it ourselves after 1 sec.
	 */
	return http_response_wait(ctx, HTTP_STATUS_200_OK, HTML_HEADER
				  "<body><h2><center>It Works!</center></h2>"
				  HTML_FOOTER, K_SECONDS(1));
}

static int http_response_soft_404(struct http_server_ctx *ctx)
{
	return http_response(ctx, HTTP_STATUS_200_OK, HTML_HEADER
			     "<h2><center>404 Not Found</center></h2>"
			     HTML_FOOTER);
}

static int http_response_auth(struct http_server_ctx *ctx)
{
	return http_response(ctx, HTTP_STATUS_200_OK, HTML_HEADER
			     "<h2><center>user: "HTTP_AUTH_USERNAME
			     ", password: "HTTP_AUTH_PASSWORD
			     "</center></h2>" HTML_FOOTER);
}

int http_response_401(struct http_server_ctx *ctx, s32_t timeout)
{
	return http_response_wait(ctx, HTTP_401_STATUS_US, NULL, timeout);
}

static int http_basic_auth(struct http_server_ctx *ctx)
{
	const char auth_str[] = HTTP_CRLF "Authorization: Basic ";
	char *ptr;

	ptr = strstr(ctx->req.field_values[0].key, auth_str);
	if (ptr) {
		char output[sizeof(HTTP_AUTH_USERNAME) +
			    sizeof(":") +
			    sizeof(HTTP_AUTH_PASSWORD)];
		size_t olen, ilen, alen;
		char *end, *colon;
		int ret;

		memset(output, 0, sizeof(output));

		end = strstr(ptr + 2, HTTP_CRLF);
		if (!end) {
			return http_response_401(ctx, K_NO_WAIT);
		}

		alen = sizeof(auth_str) - 1;
		ilen = end - (ptr + alen);

		ret = mbedtls_base64_decode(output, sizeof(output) - 1,
					    &olen, ptr + alen, ilen);
		if (ret) {
			return http_response_401(ctx, K_NO_WAIT);
		}

		colon = memchr(output, ':', olen);

		if (colon && colon > output && colon < (output + olen) &&
		    memcmp(output, HTTP_AUTH_USERNAME, colon - output) == 0 &&
		    memcmp(colon + 1, HTTP_AUTH_PASSWORD,
			   output + olen - colon) == 0) {
			return http_response_auth(ctx);
		}

		return http_response_401(ctx, K_NO_WAIT);
	}

	/* Wait 2 secs for the reply with proper credentials */
	return http_response_401(ctx, K_SECONDS(2));
}

#if defined(CONFIG_HTTPS)
/* Load the certificates and private RSA key. */

#include "test_certs.h"

static int setup_cert(struct http_server_ctx *ctx,
		      mbedtls_x509_crt *cert,
		      mbedtls_pk_context *pkey)
{
	int ret;

	ret = mbedtls_x509_crt_parse(cert, rsa_example_cert_der,
				     rsa_example_cert_der_len);
	if (ret != 0) {
		NET_ERR("mbedtls_x509_crt_parse returned %d", ret);
		return ret;
	}

	ret = mbedtls_pk_parse_key(pkey, rsa_example_keypair_der,
				   rsa_example_keypair_der_len, NULL, 0);
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
	u32_t flags = 0;
	int ret;

	/*
	 * If we have both IPv6 and IPv4 enabled, then set the
	 * startup flags so that we wait until both are ready
	 * before continuing.
	 */
#if defined(CONFIG_NET_IPV6)
	flags |= NET_SAMPLE_NEED_IPV6;
#endif
#if defined(CONFIG_NET_IPV4)
	flags |= NET_SAMPLE_NEED_IPV4;
#endif

	ret = net_sample_app_init(APP_BANNER, flags, APP_STARTUP_TIME);
	if (ret < 0) {
		panic("Application init failed");
	}

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

	http_server_add_default(&http_urls, http_response_soft_404);
	http_server_add_url(&http_urls, HTTP_AUTH_URL, HTTP_URL_STANDARD,
			    http_basic_auth);
	http_server_add_url(&http_urls, "/headers", HTTP_URL_STANDARD,
			    http_response_header_fields);
	http_server_add_url(&http_urls, "/index.html", HTTP_URL_STANDARD,
			    http_response_it_works);

#if defined(CONFIG_HTTPS)
	ret = https_server_init(&https_ctx, &http_urls, server_addr,
				https_result, sizeof(https_result),
				"Zephyr HTTPS Server",
				INSTANCE_INFO, strlen(INSTANCE_INFO),
				setup_cert, NULL, &ssl_rx_pool,
				https_stack, sizeof(https_stack));
	if (ret < 0) {
		NET_ERR("Cannot initialize HTTPS server (%d)", ret);
		panic(NULL);
	}

	http_server_enable(&https_ctx);
#endif

	ret = http_server_init(&http_ctx, &http_urls, server_addr, http_result,
			       sizeof(http_result), "Zephyr HTTP Server");
	if (ret < 0) {
		NET_ERR("Cannot initialize HTTP server (%d)", ret);
		panic(NULL);
	}

	/*
	 * If needed, the HTTP parser callbacks can be set according to
	 * applications own needs before enabling the server. In this example
	 * we use the default callbacks defined in HTTP server API.
	 */

	http_server_enable(&http_ctx);
}
