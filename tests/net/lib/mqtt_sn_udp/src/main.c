/*
 * Copyright (c) 2026 sevenlab engineering GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/net/mqtt_sn.h>
#include <zephyr/net/socket.h>
#include <zephyr/sys/base64.h>

static struct mqtt_sn_transport_udp udp;

static int cmd_udp_create(const struct shell *sh, size_t argc, char **argv)
{
	bool success;
	int ret;
	struct sockaddr bcaddr = {0};
	const char *bcaddr_str = argv[1];

	success = net_ipaddr_parse(bcaddr_str, strlen(bcaddr_str), &bcaddr);
	if (!success) {
		shell_error(sh, "unsupported bcaddr: %s", bcaddr_str);
		return -EINVAL;
	}

	ret = mqtt_sn_transport_udp_init(&udp, &bcaddr, sizeof(bcaddr));
	if (ret) {
		shell_error(sh, "Failed to create udp transport: %d", ret);
		return ret;
	}

	shell_print(sh, "success");
	return 0;
}

static int cmd_udp_init(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ret = udp.tp.init(&udp.tp);
	if (ret) {
		shell_error(sh, "Failed to init udp transport: %d", ret);
		return ret;
	}

	shell_print(sh, "success");
	return 0;
}

static int cmd_udp_deinit(const struct shell *sh, size_t argc, char **argv)
{
	udp.tp.deinit(&udp.tp);
	shell_print(sh, "success");
	return 0;
}

static int cmd_udp_poll(const struct shell *sh, size_t argc, char **argv)
{
	int ret;

	ret = udp.tp.poll(&udp.tp);
	if (ret < 0) {
		shell_error(sh, "Failed to poll udp transport: %d", ret);
		return ret;
	}

	shell_print(sh, "%d", ret);
	return 0;
}

static int cmd_udp_recvfrom(const struct shell *sh, size_t argc, char **argv)
{
	static uint8_t buffer[4096];
	static uint8_t buffer_b64[4096];
	static char ipstr[NET_INET6_ADDRSTRLEN];
	int ret;
	struct sockaddr addr;
	size_t addrlen = sizeof(addr);
	size_t b64len;

	ret = udp.tp.recvfrom(&udp.tp, buffer, sizeof(buffer), &addr, &addrlen);
	if (ret < 0) {
		shell_error(sh, "Failed to recvfrom udp transport: %d", ret);
		return ret;
	}

	switch (addr.sa_family) {
#ifdef CONFIG_NET_IPV4
	case NET_AF_INET: {
		struct net_sockaddr_in *addr_in = (struct net_sockaddr_in *)&addr;

		zsock_inet_ntop(addr.sa_family, &addr_in->sin_addr, ipstr, sizeof(ipstr));
		shell_print(sh, "%s:%d", ipstr, net_ntohs(addr_in->sin_port));
		break;
	}
#endif

#ifdef CONFIG_NET_IPV6
	case NET_AF_INET6: {
		struct net_sockaddr_in6 *addr_in6 = (struct net_sockaddr_in6 *)&addr;

		zsock_inet_ntop(addr.sa_family, &addr_in6->sin6_addr, ipstr, sizeof(ipstr));
		shell_print(sh, "%s:%d", ipstr, net_ntohs(addr_in6->sin6_port));
		break;
	}
#endif

	default:
		shell_error(sh, "Unknown AF");
		return -EINVAL;
	}

	ret = base64_encode(buffer_b64, sizeof(buffer_b64), &b64len, buffer, ret);
	if (ret) {
		shell_error(sh, "base64 buffer is too small: %d", ret);
		return ret;
	}

	shell_print(sh, "%s", buffer_b64);
	return 0;
}

static int cmd_udp_sendto(const struct shell *sh, size_t argc, char **argv)
{
	static uint8_t buffer[4096];
	size_t bufferlen;
	struct sockaddr addr_;
	struct sockaddr *addr;
	size_t addrlen = sizeof(addr_);
	int ret;
	bool success;
	const char *b64 = argv[1];

	ret = base64_decode(buffer, sizeof(buffer), &bufferlen, b64, strlen(b64));
	if (ret) {
		shell_error(sh, "invalid base64 string: %d", ret);
		return ret;
	}

	if (argc >= 3) {
		const char *addr_str = argv[2];

		success = net_ipaddr_parse(addr_str, strlen(addr_str), &addr_);
		if (!success) {
			shell_error(sh, "unsupported addr: %s", addr_str);
			return -EINVAL;
		}

		addr = &addr_;
	}

	ret = udp.tp.sendto(&udp.tp, buffer, bufferlen, addr, addrlen);
	if (ret < 0) {
		shell_error(sh, "Failed to sendto udp transport: %d", ret);
		return ret;
	}

	shell_print(sh, "success", ret);
	return 0;
}

static int cmd_udp_sendto_bcast(const struct shell *sh, size_t argc, char **argv)
{
	static uint8_t buffer[4096];
	size_t bufferlen;
	int ret;
	const char *b64 = argv[1];
	int radius = atoi(argv[2]);

	ret = base64_decode(buffer, sizeof(buffer), &bufferlen, b64, strlen(b64));
	if (ret) {
		shell_error(sh, "invalid base64 string: %d", ret);
		return ret;
	}

	ret = udp.tp.sendto(&udp.tp, buffer, bufferlen, NULL, radius);
	if (ret < 0) {
		shell_error(sh, "Failed to sendto udp transport: %d", ret);
		return ret;
	}

	shell_print(sh, "success", ret);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mqttsn,
			       SHELL_CMD_ARG(udp_create, NULL, NULL, cmd_udp_create, 2, 0),
			       SHELL_CMD_ARG(udp_init, NULL, NULL, cmd_udp_init, 1, 0),
			       SHELL_CMD_ARG(udp_deinit, NULL, NULL, cmd_udp_deinit, 1, 0),
			       SHELL_CMD_ARG(udp_poll, NULL, NULL, cmd_udp_poll, 1, 0),
			       SHELL_CMD_ARG(udp_recvfrom, NULL, NULL, cmd_udp_recvfrom, 1, 0),
			       SHELL_CMD_ARG(udp_sendto, NULL, NULL, cmd_udp_sendto, 2, 1),
			       SHELL_CMD_ARG(udp_sendto_bcast, NULL, NULL, cmd_udp_sendto_bcast, 3,
					     0),
			       SHELL_SUBCMD_SET_END);
SHELL_CMD_ARG_REGISTER(mqttsn, &sub_mqttsn, NULL, NULL, 1, 0);
