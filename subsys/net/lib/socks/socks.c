/*
 * Copyright (c) 2019 Antmicro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_REGISTER(net_socks, CONFIG_SOCKS_LOG_LEVEL);

#include <zephyr.h>
#include <net/socket.h>
#include <net/socks.h>

#include "socks_internal.h"

static int socks5_tcp_send(int fd, u8_t *data, u32_t len)
{
	u32_t offset = 0U;
	int ret;

	while (offset < len) {
		ret = send(fd, data + offset, len - offset, 0);
		if (ret < 0) {
			return ret;
		}

		offset += ret;
	}

	return 0;
}

static int socks5_tcp_recv(int fd, u8_t *data, u32_t len)
{
	u32_t offset = 0U;
	int ret;

	while (offset < len) {
		ret = recv(fd, data + offset, len - offset, 0);
		if (ret < 0) {
			return ret;
		}

		offset += ret;
	}

	return 0;
}

int socks5_client_tcp_connect(const struct sockaddr *proxy,
			      const struct sockaddr *destination)
{
	struct socks5_method_request mthd_req;
	struct socks5_method_response mthd_rep;
	struct socks5_command_request cmd_req;
	struct socks5_command_response cmd_rep;
	int size;
	int ret;
	int fd;

	fd = socket(proxy->sa_family, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0) {
		return fd;
	}

	ret = connect(fd, proxy, sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERR("Unable to connect to the proxy server");
		(void)close(fd);
		return ret;
	}

	/* Negotiate authentication method */
	mthd_req.r.ver = SOCKS5_PKT_MAGIC;

	/* We only support NOAUTH at the moment */
	mthd_req.r.nmethods = 1;
	mthd_req.methods[0] = SOCKS5_AUTH_METHOD_NOAUTH;

	/* size + 1 because just one method is supported */
	size = sizeof(struct socks5_method_request_common) + 1;

	ret = socks5_tcp_send(fd, (u8_t *)&mthd_req, size);
	if (ret < 0) {
		(void)close(fd);
		LOG_ERR("Could not send negotiation packet");
		return ret;
	}

	ret = socks5_tcp_recv(fd, (u8_t *)&mthd_rep, sizeof(mthd_rep));
	if (ret < 0) {
		LOG_ERR("Could not receive negotiation response");
		(void)close(fd);
		return ret;
	}

	if (mthd_rep.ver != SOCKS5_PKT_MAGIC) {
		LOG_ERR("Invalid negotiation response magic");
		(void)close(fd);
		return -EINVAL;
	}

	if (mthd_rep.method != SOCKS5_AUTH_METHOD_NOAUTH) {
		LOG_ERR("Invalid negotiation response");
		(void)close(fd);
		return -ENOTSUP;
	}

	/* Negotiation complete - now connect to destination */
	cmd_req.r.ver = SOCKS5_PKT_MAGIC;
	cmd_req.r.cmd = SOCKS5_CMD_CONNECT;
	cmd_req.r.rsv = SOCKS5_PKT_RSV;

	if (proxy->sa_family == AF_INET) {
		const struct sockaddr_in *d4 =
			(struct sockaddr_in *)destination;

		cmd_req.r.atyp = SOCKS5_ATYP_IPV4;

		memcpy(&cmd_req.ipv4_addr.addr,
		       (u8_t *)&d4->sin_addr,
		       sizeof(cmd_req.ipv4_addr.addr));

		cmd_req.ipv4_addr.port = d4->sin_port;

		size = sizeof(struct socks5_command_request_common)
			+ sizeof(struct socks5_ipv4_addr);
	} else if (proxy->sa_family == AF_INET6) {
		const struct sockaddr_in6 *d6 =
			(struct sockaddr_in6 *)destination;

		cmd_req.r.atyp = SOCKS5_ATYP_IPV6;

		memcpy(&cmd_req.ipv6_addr.addr,
		       (u8_t *)&d6->sin6_addr,
		       sizeof(cmd_req.ipv6_addr.addr));

		cmd_req.ipv4_addr.port = d6->sin6_port;

		size = sizeof(struct socks5_command_request_common)
			+ sizeof(struct socks5_ipv6_addr);
	}

	ret = socks5_tcp_send(fd, (u8_t *)&cmd_req, size);
	if (ret < 0) {
		LOG_ERR("Could not send CONNECT command");
		(void)close(fd);
		return -EINVAL;
	}

	ret = socks5_tcp_recv(fd, (u8_t *)&cmd_rep, size);
	if (ret < 0) {
		LOG_ERR("Could not receive CONNECT response");
		(void)close(fd);
		return -EINVAL;
	}

	if (cmd_rep.r.ver != SOCKS5_PKT_MAGIC) {
		LOG_ERR("Invalid CONNECT response");
		(void)close(fd);
		return -EINVAL;
	}

	if (cmd_rep.r.rep != SOCKS5_CMD_RESP_SUCCESS) {
		LOG_ERR("Unable to connect to destination");
		(void)close(fd);
		return -EINVAL;
	}

	/* Verifying the rest is not required */

	LOG_DBG("Connection through SOCKS5 proxy successful");

	return fd;
}
