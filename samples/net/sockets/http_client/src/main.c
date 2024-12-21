/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_http_client_sample, LOG_LEVEL_DBG);

#include <zephyr/net/net_ip.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/net/http/client.h>

#include "ca_certificate.h"

#define HTTP_PORT 8000
#define HTTPS_PORT 4443

#if defined(CONFIG_NET_CONFIG_PEER_IPV6_ADDR)
#define SERVER_ADDR6  CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#else
#define SERVER_ADDR6 ""
#endif

#if defined(CONFIG_NET_CONFIG_PEER_IPV4_ADDR)
#define SERVER_ADDR4  CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#else
#define SERVER_ADDR4 ""
#endif

#define MAX_RECV_BUF_LEN 512

static uint8_t recv_buf_ipv4[MAX_RECV_BUF_LEN];
static uint8_t recv_buf_ipv6[MAX_RECV_BUF_LEN];

static int setup_socket(sa_family_t family, const char *server, int port,
			int *sock, struct sockaddr *addr, socklen_t addr_len)
{
	const char *family_str = family == AF_INET ? "IPv4" : "IPv6";
	int ret = 0;

	memset(addr, 0, addr_len);

	if (family == AF_INET) {
		net_sin(addr)->sin_family = AF_INET;
		net_sin(addr)->sin_port = htons(port);
		inet_pton(family, server, &net_sin(addr)->sin_addr);
	} else {
		net_sin6(addr)->sin6_family = AF_INET6;
		net_sin6(addr)->sin6_port = htons(port);
		inet_pton(family, server, &net_sin6(addr)->sin6_addr);
	}

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		sec_tag_t sec_tag_list[] = {
			CA_CERTIFICATE_TAG,
		};

		*sock = socket(family, SOCK_STREAM, IPPROTO_TLS_1_2);
		if (*sock >= 0) {
			ret = setsockopt(*sock, SOL_TLS, TLS_SEC_TAG_LIST,
					 sec_tag_list, sizeof(sec_tag_list));
			if (ret < 0) {
				LOG_ERR("Failed to set %s secure option (%d)",
					family_str, -errno);
				ret = -errno;
			}

			ret = setsockopt(*sock, SOL_TLS, TLS_HOSTNAME,
					 TLS_PEER_HOSTNAME,
					 sizeof(TLS_PEER_HOSTNAME));
			if (ret < 0) {
				LOG_ERR("Failed to set %s TLS_HOSTNAME "
					"option (%d)", family_str, -errno);
				ret = -errno;
			}
		}
	} else {
		*sock = socket(family, SOCK_STREAM, IPPROTO_TCP);
	}

	if (*sock < 0) {
		LOG_ERR("Failed to create %s HTTP socket (%d)", family_str,
			-errno);
	}

	return ret;
}

static int payload_cb(int sock, struct http_request *req, void *user_data)
{
	const char *content[] = {
		"foobar",
		"chunked",
		"last"
	};
	char tmp[64];
	int i, pos = 0;

	for (i = 0; i < ARRAY_SIZE(content); i++) {
		pos += snprintk(tmp + pos, sizeof(tmp) - pos,
				"%x\r\n%s\r\n",
				(unsigned int)strlen(content[i]),
				content[i]);
	}

	pos += snprintk(tmp + pos, sizeof(tmp) - pos, "0\r\n\r\n");

	(void)send(sock, tmp, pos, 0);

	return pos;
}

static void response_cb(struct http_response *rsp,
			enum http_final_call final_data,
			void *user_data)
{
	if (final_data == HTTP_DATA_MORE) {
		LOG_INF("Partial data received (%zd bytes)", rsp->data_len);
	} else if (final_data == HTTP_DATA_FINAL) {
		LOG_INF("All the data received (%zd bytes)", rsp->data_len);
	}

	LOG_INF("Response to %s", (const char *)user_data);
	LOG_INF("Response status %s", rsp->http_status);
}

static int connect_socket(sa_family_t family, const char *server, int port,
			  int *sock, struct sockaddr *addr, socklen_t addr_len)
{
	int ret;

	ret = setup_socket(family, server, port, sock, addr, addr_len);
	if (ret < 0 || *sock < 0) {
		return -1;
	}

	ret = connect(*sock, addr, addr_len);
	if (ret < 0) {
		LOG_ERR("Cannot connect to %s remote (%d)",
			family == AF_INET ? "IPv4" : "IPv6",
			-errno);
		close(*sock);
		*sock = -1;
		ret = -errno;
	}

	return ret;
}

static int run_queries(void)
{
	struct sockaddr_in6 addr6;
	struct sockaddr_in addr4;
	int sock4 = -1, sock6 = -1;
	int32_t timeout = 3 * MSEC_PER_SEC;
	int ret = 0;
	int port = HTTP_PORT;

	if (IS_ENABLED(CONFIG_NET_SOCKETS_SOCKOPT_TLS)) {
		ret = tls_credential_add(CA_CERTIFICATE_TAG,
					 TLS_CREDENTIAL_CA_CERTIFICATE,
					 ca_certificate,
					 sizeof(ca_certificate));
		if (ret < 0) {
			LOG_ERR("Failed to register public certificate: %d",
				ret);
			return ret;
		}

		port = HTTPS_PORT;
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		(void)connect_socket(AF_INET, SERVER_ADDR4, port,
				     &sock4, (struct sockaddr *)&addr4,
				     sizeof(addr4));
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		(void)connect_socket(AF_INET6, SERVER_ADDR6, port,
				     &sock6, (struct sockaddr *)&addr6,
				     sizeof(addr6));
	}

	if (sock4 < 0 && sock6 < 0) {
		LOG_ERR("Cannot create HTTP connection.");
		return -ECONNABORTED;
	}

	if (sock4 >= 0 && IS_ENABLED(CONFIG_NET_IPV4)) {
		struct http_request req;

		memset(&req, 0, sizeof(req));

		req.method = HTTP_GET;
		req.url = "/";
		req.host = SERVER_ADDR4;
		req.protocol = "HTTP/1.1";
		req.response = response_cb;
		req.recv_buf = recv_buf_ipv4;
		req.recv_buf_len = sizeof(recv_buf_ipv4);

		ret = http_client_req(sock4, &req, timeout, "IPv4 GET");

		close(sock4);
	}

	if (sock6 >= 0 && IS_ENABLED(CONFIG_NET_IPV6)) {
		struct http_request req;

		memset(&req, 0, sizeof(req));

		req.method = HTTP_GET;
		req.url = "/";
		req.host = SERVER_ADDR6;
		req.protocol = "HTTP/1.1";
		req.response = response_cb;
		req.recv_buf = recv_buf_ipv6;
		req.recv_buf_len = sizeof(recv_buf_ipv6);

		ret = http_client_req(sock6, &req, timeout, "IPv6 GET");

		close(sock6);
	}

	sock4 = -1;
	sock6 = -1;

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		(void)connect_socket(AF_INET, SERVER_ADDR4, port,
				     &sock4, (struct sockaddr *)&addr4,
				     sizeof(addr4));
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		(void)connect_socket(AF_INET6, SERVER_ADDR6, port,
				     &sock6, (struct sockaddr *)&addr6,
				     sizeof(addr6));
	}

	if (sock4 < 0 && sock6 < 0) {
		LOG_ERR("Cannot create HTTP connection.");
		return -ECONNABORTED;
	}

	if (sock4 >= 0 && IS_ENABLED(CONFIG_NET_IPV4)) {
		struct http_request req;

		memset(&req, 0, sizeof(req));

		req.method = HTTP_POST;
		req.url = "/foobar";
		req.host = SERVER_ADDR4;
		req.protocol = "HTTP/1.1";
		req.payload = "foobar";
		req.payload_len = strlen(req.payload);
		req.response = response_cb;
		req.recv_buf = recv_buf_ipv4;
		req.recv_buf_len = sizeof(recv_buf_ipv4);

		ret = http_client_req(sock4, &req, timeout, "IPv4 POST");

		close(sock4);
	}

	if (sock6 >= 0 && IS_ENABLED(CONFIG_NET_IPV6)) {
		struct http_request req;

		memset(&req, 0, sizeof(req));

		req.method = HTTP_POST;
		req.url = "/";
		req.host = SERVER_ADDR6;
		req.protocol = "HTTP/1.1";
		req.payload = "foobar";
		req.payload_len = strlen(req.payload);
		req.response = response_cb;
		req.recv_buf = recv_buf_ipv6;
		req.recv_buf_len = sizeof(recv_buf_ipv6);

		ret = http_client_req(sock6, &req, timeout, "IPv6 POST");

		close(sock6);
	}

	/* Do a chunked POST request */

	sock4 = -1;
	sock6 = -1;

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		(void)connect_socket(AF_INET, SERVER_ADDR4, port,
				     &sock4, (struct sockaddr *)&addr4,
				     sizeof(addr4));
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		(void)connect_socket(AF_INET6, SERVER_ADDR6, port,
				     &sock6, (struct sockaddr *)&addr6,
				     sizeof(addr6));
	}

	if (sock4 < 0 && sock6 < 0) {
		LOG_ERR("Cannot create HTTP connection.");
		return -ECONNABORTED;
	}

	if (sock4 >= 0 && IS_ENABLED(CONFIG_NET_IPV4)) {
		struct http_request req;
		const char *headers[] = {
			"Transfer-Encoding: chunked\r\n",
			NULL
		};

		memset(&req, 0, sizeof(req));

		req.method = HTTP_POST;
		req.url = "/chunked-test";
		req.host = SERVER_ADDR4;
		req.protocol = "HTTP/1.1";
		req.payload_cb = payload_cb;
		req.header_fields = headers;
		req.response = response_cb;
		req.recv_buf = recv_buf_ipv4;
		req.recv_buf_len = sizeof(recv_buf_ipv4);

		ret = http_client_req(sock4, &req, timeout, "IPv4 POST");

		close(sock4);
	}

	if (sock6 >= 0 && IS_ENABLED(CONFIG_NET_IPV6)) {
		struct http_request req;
		const char *headers[] = {
			"Transfer-Encoding: chunked\r\n",
			NULL
		};

		memset(&req, 0, sizeof(req));

		req.method = HTTP_POST;
		req.url = "/chunked-test";
		req.host = SERVER_ADDR6;
		req.protocol = "HTTP/1.1";
		req.payload_cb = payload_cb;
		req.header_fields = headers;
		req.response = response_cb;
		req.recv_buf = recv_buf_ipv6;
		req.recv_buf_len = sizeof(recv_buf_ipv6);

		ret = http_client_req(sock6, &req, timeout, "IPv6 POST");

		close(sock6);
	}

	return ret;
}

int main(void)
{
	int iterations = CONFIG_NET_SAMPLE_SEND_ITERATIONS;
	int i = 0;
	int ret = 0;

	while (iterations == 0 || i < iterations) {
		ret = run_queries();
		if (ret < 0) {
			ret = 1;
			break;
		}

		if (iterations > 0) {
			i++;
			if (i >= iterations) {
				ret = 0;
				break;
			}
		} else {
			ret = 0;
			break;
		}
	}

	if (iterations == 0) {
		k_sleep(K_FOREVER);
	}

	exit(ret);
	return ret;
}
