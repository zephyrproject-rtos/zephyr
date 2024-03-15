/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_shell);

#include <stdlib.h>

#include "net_shell_private.h"

#if defined(CONFIG_NET_UDP) && defined(CONFIG_NET_NATIVE_UDP)
static struct net_context *udp_ctx;
static const struct shell *udp_shell;
K_SEM_DEFINE(udp_send_wait, 0, 1);

static void udp_rcvd(struct net_context *context, struct net_pkt *pkt,
		     union net_ip_header *ip_hdr,
		     union net_proto_header *proto_hdr, int status,
		     void *user_data)
{
	if (pkt) {
		size_t len = net_pkt_remaining_data(pkt);
		uint8_t byte;

		PR_SHELL(udp_shell, "Received UDP packet: ");
		for (size_t i = 0; i < len; ++i) {
			net_pkt_read_u8(pkt, &byte);
			PR_SHELL(udp_shell, "%02x ", byte);
		}
		PR_SHELL(udp_shell, "\n");

		net_pkt_unref(pkt);
	}
}

static void udp_sent(struct net_context *context, int status, void *user_data)
{
	ARG_UNUSED(context);
	ARG_UNUSED(status);
	ARG_UNUSED(user_data);

	PR_SHELL(udp_shell, "Message sent\n");
	k_sem_give(&udp_send_wait);
}
#endif

static int cmd_net_udp_bind(const struct shell *sh, size_t argc, char *argv[])
{
#if !defined(CONFIG_NET_UDP) || !defined(CONFIG_NET_NATIVE_UDP)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	char *addr_str = NULL;
	char *endptr = NULL;
	uint16_t port;
	int ret;

	struct net_if *iface;
	struct sockaddr addr;
	int addrlen;

	if (argc < 3) {
		PR_WARNING("Not enough arguments given for udp bind command\n");
		return -EINVAL;
	}

	addr_str = argv[1];
	port = strtol(argv[2], &endptr, 0);

	if (endptr == argv[2]) {
		PR_WARNING("Invalid port number\n");
		return -EINVAL;
	}

	if (udp_ctx && net_context_is_used(udp_ctx)) {
		PR_WARNING("Network context already in use\n");
		return -EALREADY;
	}

	memset(&addr, 0, sizeof(addr));

	ret = net_ipaddr_parse(addr_str, strlen(addr_str), &addr);
	if (ret < 0) {
		PR_WARNING("Cannot parse address \"%s\"\n", addr_str);
		return ret;
	}

	ret = net_context_get(addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot get UDP context (%d)\n", ret);
		return ret;
	}

	udp_shell = sh;

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr.sa_family == AF_INET6) {
		net_sin6(&addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);

		iface = net_if_ipv6_select_src_iface(
				&net_sin6(&addr)->sin6_addr);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr.sa_family == AF_INET) {
		net_sin(&addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);

		iface = net_if_ipv4_select_src_iface(
				&net_sin(&addr)->sin_addr);
	} else {
		PR_WARNING("IPv6 and IPv4 are disabled, cannot %s.\n", "bind");
		goto release_ctx;
	}

	if (!iface) {
		PR_WARNING("No interface to send to given host\n");
		goto release_ctx;
	}

	net_context_set_iface(udp_ctx, iface);

	ret = net_context_bind(udp_ctx, &addr, addrlen);
	if (ret < 0) {
		PR_WARNING("Binding to UDP port failed (%d)\n", ret);
		goto release_ctx;
	}

	ret = net_context_recv(udp_ctx, udp_rcvd, K_NO_WAIT, NULL);
	if (ret < 0) {
		PR_WARNING("Receiving from UDP port failed (%d)\n", ret);
		goto release_ctx;
	}

	return 0;

release_ctx:
	ret = net_context_put(udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot put UDP context (%d)\n", ret);
	}

	return 0;
#endif
}

static int cmd_net_udp_close(const struct shell *sh, size_t argc, char *argv[])
{
#if !defined(CONFIG_NET_UDP) || !defined(CONFIG_NET_NATIVE_UDP)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	int ret;

	if (!udp_ctx || !net_context_is_used(udp_ctx)) {
		PR_WARNING("Network context is not used. Cannot close.\n");
		return -EINVAL;
	}

	ret = net_context_put(udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot close UDP port (%d)\n", ret);
	}

	return 0;
#endif
}

static int cmd_net_udp_send(const struct shell *sh, size_t argc, char *argv[])
{
#if !defined(CONFIG_NET_UDP) || !defined(CONFIG_NET_NATIVE_UDP)
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return -EOPNOTSUPP;
#else
	char *host = NULL;
	char *endptr = NULL;
	uint16_t port;
	uint8_t *payload = NULL;
	int ret;

	struct net_if *iface;
	struct sockaddr addr;
	int addrlen;

	if (argc < 4) {
		PR_WARNING("Not enough arguments given for udp send command\n");
		return -EINVAL;
	}

	host = argv[1];
	port = strtol(argv[2], &endptr, 0);
	payload = argv[3];

	if (endptr == argv[2]) {
		PR_WARNING("Invalid port number\n");
		return -EINVAL;
	}

	if (udp_ctx && net_context_is_used(udp_ctx)) {
		PR_WARNING("Network context already in use\n");
		return -EALREADY;
	}

	memset(&addr, 0, sizeof(addr));
	ret = net_ipaddr_parse(host, strlen(host), &addr);
	if (ret < 0) {
		PR_WARNING("Cannot parse address \"%s\"\n", host);
		return ret;
	}

	ret = net_context_get(addr.sa_family, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot get UDP context (%d)\n", ret);
		return ret;
	}

	udp_shell = sh;

	if (IS_ENABLED(CONFIG_NET_IPV6) && addr.sa_family == AF_INET6) {
		net_sin6(&addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);

		iface = net_if_ipv6_select_src_iface(
				&net_sin6(&addr)->sin6_addr);
	} else if (IS_ENABLED(CONFIG_NET_IPV4) && addr.sa_family == AF_INET) {
		net_sin(&addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);

		iface = net_if_ipv4_select_src_iface(
				&net_sin(&addr)->sin_addr);
	} else {
		PR_WARNING("IPv6 and IPv4 are disabled, cannot %s.\n", "send");
		goto release_ctx;
	}

	if (!iface) {
		PR_WARNING("No interface to send to given host\n");
		goto release_ctx;
	}

	net_context_set_iface(udp_ctx, iface);

	ret = net_context_recv(udp_ctx, udp_rcvd, K_NO_WAIT, NULL);
	if (ret < 0) {
		PR_WARNING("Setting rcv callback failed (%d)\n", ret);
		goto release_ctx;
	}

	ret = net_context_sendto(udp_ctx, payload, strlen(payload), &addr,
				 addrlen, udp_sent, K_FOREVER, NULL);
	if (ret < 0) {
		PR_WARNING("Sending packet failed (%d)\n", ret);
		goto release_ctx;
	}

	ret = k_sem_take(&udp_send_wait, K_SECONDS(2));
	if (ret == -EAGAIN) {
		PR_WARNING("UDP packet sending failed\n");
	}

release_ctx:
	ret = net_context_put(udp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot put UDP context (%d)\n", ret);
	}

	return 0;
#endif
}

static int cmd_net_udp(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(sh);
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_udp,
	SHELL_CMD(bind, NULL,
		  "'net udp bind <addr> <port>' binds to UDP local port.",
		  cmd_net_udp_bind),
	SHELL_CMD(close, NULL,
		  "'net udp close' closes previously bound port.",
		  cmd_net_udp_close),
	SHELL_CMD(send, NULL,
		  "'net udp send <host> <port> <payload>' "
		  "sends UDP packet to a network host.",
		  cmd_net_udp_send),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), udp, &net_cmd_udp,
		 "Send/recv UDP packet",
		 cmd_net_udp, 1, 0);
