/* tcp.c - TCP specific code for echo client */

/*
 * Copyright (c) 2017 Intel Corporation.
 * Copyright (c) 2018 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_echo_client_sample, LOG_LEVEL_DBG);

#include <zephyr/kernel.h>
#include <errno.h>
#include <stdio.h>

#include <zephyr/net/socket.h>
#include <zephyr/net/tls_credentials.h>
#include <zephyr/random/random.h>

#include "common.h"
#include "ca_certificate.h"

#define RECV_BUF_SIZE 128

/* These proxy server addresses are only used when CONFIG_SOCKS
 * is enabled. To connect to a proxy server that is not running
 * under the same IP as the peer or uses a different port number,
 * modify the values.
 */
#define SOCKS5_PROXY_V6_ADDR CONFIG_NET_CONFIG_PEER_IPV6_ADDR
#define SOCKS5_PROXY_V4_ADDR CONFIG_NET_CONFIG_PEER_IPV4_ADDR
#define SOCKS5_PROXY_PORT 1080

static ssize_t sendall(int sock, const void *buf, size_t len)
{
	while (len) {
		ssize_t out_len = send(sock, buf, len, 0);

		if (out_len < 0) {
			return out_len;
		}
		buf = (const char *)buf + out_len;
		len -= out_len;
	}

	return 0;
}

static int send_tcp_data(struct data *data)
{
	int ret;

	do {
		data->tcp.expecting = sys_rand32_get() % ipsum_len;
	} while (data->tcp.expecting == 0U);

	data->tcp.received = 0U;

	ret =  sendall(data->tcp.sock, lorem_ipsum, data->tcp.expecting);

	if (ret < 0) {
		LOG_ERR("%s TCP: Failed to send data, errno %d", data->proto,
			errno);
	} else {
		LOG_DBG("%s TCP: Sent %d bytes", data->proto,
			data->tcp.expecting);
	}

	return ret;
}

static int compare_tcp_data(struct data *data, const char *buf, uint32_t received)
{
	if (data->tcp.received + received > data->tcp.expecting) {
		LOG_ERR("Too much data received: TCP %s", data->proto);
		return -EIO;
	}

	if (memcmp(buf, lorem_ipsum + data->tcp.received, received) != 0) {
		LOG_ERR("Invalid data received: TCP %s", data->proto);
		return -EIO;
	}

	return 0;
}

static int start_tcp_proto(struct data *data, struct sockaddr *addr,
			   socklen_t addrlen)
{
	int optval;
	int ret;

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	data->tcp.sock = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TLS_1_2);
#else
	data->tcp.sock = socket(addr->sa_family, SOCK_STREAM, IPPROTO_TCP);
#endif
	if (data->tcp.sock < 0) {
		LOG_ERR("Failed to create TCP socket (%s): %d", data->proto,
			errno);
		return -errno;
	}

	if (IS_ENABLED(CONFIG_SOCKS)) {
		struct sockaddr proxy_addr;
		socklen_t proxy_addrlen;

		if (addr->sa_family == AF_INET) {
			struct sockaddr_in *proxy4 =
				(struct sockaddr_in *)&proxy_addr;

			proxy4->sin_family = AF_INET;
			proxy4->sin_port = htons(SOCKS5_PROXY_PORT);
			inet_pton(AF_INET, SOCKS5_PROXY_V4_ADDR,
				  &proxy4->sin_addr);
			proxy_addrlen = sizeof(struct sockaddr_in);
		} else if (addr->sa_family == AF_INET6) {
			struct sockaddr_in6 *proxy6 =
				(struct sockaddr_in6 *)&proxy_addr;

			proxy6->sin6_family = AF_INET6;
			proxy6->sin6_port = htons(SOCKS5_PROXY_PORT);
			inet_pton(AF_INET6, SOCKS5_PROXY_V6_ADDR,
				  &proxy6->sin6_addr);
			proxy_addrlen = sizeof(struct sockaddr_in6);
		} else {
			return -EINVAL;
		}

		ret = setsockopt(data->tcp.sock, SOL_SOCKET, SO_SOCKS5,
				 &proxy_addr, proxy_addrlen);
		if (ret < 0) {
			return ret;
		}
	}

#if defined(CONFIG_NET_SOCKETS_SOCKOPT_TLS)
	sec_tag_t sec_tag_list[] = {
		CA_CERTIFICATE_TAG,
#if defined(CONFIG_MBEDTLS_KEY_EXCHANGE_PSK_ENABLED)
		PSK_TAG,
#endif
	};

	ret = setsockopt(data->tcp.sock, SOL_TLS, TLS_SEC_TAG_LIST,
			 sec_tag_list, sizeof(sec_tag_list));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_SEC_TAG_LIST option (%s): %d",
			data->proto, errno);
		ret = -errno;
	}

	ret = setsockopt(data->tcp.sock, SOL_TLS, TLS_HOSTNAME,
			 TLS_PEER_HOSTNAME, sizeof(TLS_PEER_HOSTNAME));
	if (ret < 0) {
		LOG_ERR("Failed to set TLS_HOSTNAME option (%s): %d",
			data->proto, errno);
		ret = -errno;
	}
#endif

	/* Prefer IPv6 temporary addresses */
	if (addr->sa_family == AF_INET6) {
		optval = IPV6_PREFER_SRC_TMP;
		(void)setsockopt(data->tcp.sock, IPPROTO_IPV6,
				 IPV6_ADDR_PREFERENCES,
				 &optval, sizeof(optval));
	}

	ret = connect(data->tcp.sock, addr, addrlen);
	if (ret < 0) {
		LOG_ERR("Cannot connect to TCP remote (%s): %d", data->proto,
			errno);
		ret = -errno;
	}

	return ret;
}

static int process_tcp_proto(struct data *data)
{
	int ret, received;
	char buf[RECV_BUF_SIZE];

	do {
		received = recv(data->tcp.sock, buf, sizeof(buf), MSG_DONTWAIT);

		/* No data or error. */
		if (received == 0) {
			ret = -EIO;
			continue;
		} else if (received < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				ret = 0;
			} else {
				ret = -errno;
			}
			continue;
		}

		ret = compare_tcp_data(data, buf, received);
		if (ret != 0) {
			break;
		}

		/* Successful comparison. */
		data->tcp.received += received;
		if (data->tcp.received < data->tcp.expecting) {
			continue;
		}

		/* Response complete */
		LOG_DBG("%s TCP: Received and compared %d bytes, all ok",
			data->proto, data->tcp.received);


		if (++data->tcp.counter % 1000 == 0U) {
			LOG_INF("%s TCP: Exchanged %u packets", data->proto,
				data->tcp.counter);
		}

		ret = send_tcp_data(data);
		break;
	} while (received > 0);

	return ret;
}

int start_tcp(void)
{
	int ret = 0;
	struct sockaddr_in addr4;
	struct sockaddr_in6 addr6;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		addr6.sin6_family = AF_INET6;
		addr6.sin6_port = htons(PEER_PORT);
		inet_pton(AF_INET6, CONFIG_NET_CONFIG_PEER_IPV6_ADDR,
			  &addr6.sin6_addr);

		ret = start_tcp_proto(&conf.ipv6,
				      (struct sockaddr *)&addr6,
				      sizeof(addr6));
		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		addr4.sin_family = AF_INET;
		addr4.sin_port = htons(PEER_PORT);
		inet_pton(AF_INET, CONFIG_NET_CONFIG_PEER_IPV4_ADDR,
			  &addr4.sin_addr);

		ret = start_tcp_proto(&conf.ipv4, (struct sockaddr *)&addr4,
				      sizeof(addr4));
		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = send_tcp_data(&conf.ipv6);
		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = send_tcp_data(&conf.ipv4);
	}

	return ret;
}

int process_tcp(void)
{
	int ret = 0;

	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		ret = process_tcp_proto(&conf.ipv6);
		if (ret < 0) {
			return ret;
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = process_tcp_proto(&conf.ipv4);
		if (ret < 0) {
			return ret;
		}
	}

	return ret;
}

void stop_tcp(void)
{
	if (IS_ENABLED(CONFIG_NET_IPV6)) {
		if (conf.ipv6.tcp.sock >= 0) {
			(void)close(conf.ipv6.tcp.sock);
		}
	}

	if (IS_ENABLED(CONFIG_NET_IPV4)) {
		if (conf.ipv4.tcp.sock >= 0) {
			(void)close(conf.ipv4.tcp.sock);
		}
	}
}
