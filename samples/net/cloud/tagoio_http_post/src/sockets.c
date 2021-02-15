/*
 * Copyright (c) 2020 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(tagoio_http_post, CONFIG_TAGOIO_HTTP_POST_LOG_LEVEL);

#include <net/net_ip.h>
#include <net/socket.h>
#include <net/socketutils.h>
#include <net/dns_resolve.h>
#include <net/tls_credentials.h>
#include <net/http_client.h>

#include "sockets.h"

#if IS_ENABLED(CONFIG_TAGOIO_MANUAL_SERVER)
#define TAGOIO_SERVER CONFIG_TAGOIO_API_SERVER_IP
#else
#define TAGOIO_SERVER "api.tago.io"
#endif

#define TAGOIO_URL          "/data"
#define HTTP_PORT           "80"

static const char *tagoio_http_headers[] = {
	"Device-Token: " CONFIG_TAGOIO_DEVICE_TOKEN "\r\n",
	"Content-Type: application/json\r\n",
	"_ssl: false\r\n",
	NULL
};

int tagoio_connect(struct tagoio_context *ctx)
{
	struct addrinfo *addr;
	struct addrinfo hints;
	char hr_addr[INET6_ADDRSTRLEN];
	char *port;
	int dns_attempts = 3;
	int ret = -1;

	memset(&hints, 0, sizeof(hints));

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		hints.ai_family = AF_INET6;
	} else if (IS_ENABLED(CONFIG_NET_IPV4)) {
		hints.ai_family = AF_INET;
	}
	port = HTTP_PORT;

	while (dns_attempts--) {
		ret = getaddrinfo(TAGOIO_SERVER, port, &hints, &addr);
		if (ret == 0) {
			break;
		}
		k_sleep(K_SECONDS(1));
	}

	if (ret < 0) {
		LOG_ERR("Could not resolve dns, error: %d", ret);
		return ret;
	}

	LOG_DBG("%s address: %s",
		(addr->ai_family == AF_INET ? "IPv4" : "IPv6"),
		log_strdup(net_addr_ntop(addr->ai_family,
					 &net_sin(addr->ai_addr)->sin_addr,
					 hr_addr, sizeof(hr_addr))));

	ctx->sock = socket(hints.ai_family,
			   hints.ai_socktype,
			   hints.ai_protocol);
	if (ctx->sock < 0) {
		LOG_ERR("Failed to create %s HTTP socket (%d)",
			(addr->ai_family == AF_INET ? "IPv4" : "IPv6"),
			-errno);

		freeaddrinfo(addr);
		return -errno;
	}

	if (connect(ctx->sock, addr->ai_addr, addr->ai_addrlen) < 0) {
		LOG_ERR("Cannot connect to %s remote (%d)",
			(addr->ai_family == AF_INET ? "IPv4" : "IPv6"),
			-errno);

		freeaddrinfo(addr);
		return -errno;
	}

	freeaddrinfo(addr);

	return 0;
}

int tagoio_http_push(struct tagoio_context *ctx,
		     http_response_cb_t resp_cb)
{
	struct http_request req;
	int ret;

	memset(&req, 0, sizeof(req));

	req.method		= HTTP_POST;
	req.host		= TAGOIO_SERVER;
	req.port		= HTTP_PORT;
	req.url			= TAGOIO_URL;
	req.header_fields	= tagoio_http_headers;
	req.protocol		= "HTTP/1.1";
	req.response		= resp_cb;
	req.payload		= ctx->payload;
	req.payload_len		= strlen(ctx->payload);
	req.recv_buf		= ctx->resp;
	req.recv_buf_len	= sizeof(ctx->resp);

	ret = http_client_req(ctx->sock, &req,
			      CONFIG_TAGOIO_HTTP_CONN_TIMEOUT * MSEC_PER_SEC,
			      ctx);

	close(ctx->sock);
	ctx->sock = -1;

	return ret;
}
