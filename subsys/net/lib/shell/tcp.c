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

#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
static struct net_context *tcp_ctx;
static const struct shell *tcp_shell;

#define TCP_CONNECT_TIMEOUT K_SECONDS(5) /* ms */
#define TCP_TIMEOUT K_SECONDS(2) /* ms */

static void tcp_connected(struct net_context *context,
			  int status,
			  void *user_data)
{
	if (status < 0) {
		PR_SHELL(tcp_shell, "TCP connection failed (%d)\n", status);
	} else {
		PR_SHELL(tcp_shell, "TCP connected\n");
	}
}

static void get_my_ipv6_addr(struct net_if *iface,
			     struct sockaddr *myaddr)
{
#if defined(CONFIG_NET_IPV6)
	const struct in6_addr *my6addr;

	my6addr = net_if_ipv6_select_src_addr(iface,
					      &net_sin6(myaddr)->sin6_addr);

	memcpy(&net_sin6(myaddr)->sin6_addr, my6addr, sizeof(struct in6_addr));

	net_sin6(myaddr)->sin6_port = 0U; /* let the IP stack to select */
#endif
}

static void get_my_ipv4_addr(struct net_if *iface,
			     struct sockaddr *myaddr)
{
#if defined(CONFIG_NET_NATIVE_IPV4)
	/* Just take the first IPv4 address of an interface. */
	memcpy(&net_sin(myaddr)->sin_addr,
	       &iface->config.ip.ipv4->unicast[0].ipv4.address.in_addr,
	       sizeof(struct in_addr));

	net_sin(myaddr)->sin_port = 0U; /* let the IP stack to select */
#endif
}

static void print_connect_info(const struct shell *sh,
			       int family,
			       struct sockaddr *myaddr,
			       struct sockaddr *addr)
{
	switch (family) {
	case AF_INET:
		if (IS_ENABLED(CONFIG_NET_IPV4)) {
			PR("Connecting from %s:%u ",
			   net_sprint_ipv4_addr(&net_sin(myaddr)->sin_addr),
			   ntohs(net_sin(myaddr)->sin_port));
			PR("to %s:%u\n",
			   net_sprint_ipv4_addr(&net_sin(addr)->sin_addr),
			   ntohs(net_sin(addr)->sin_port));
		} else {
			PR_INFO("IPv4 not supported\n");
		}

		break;

	case AF_INET6:
		if (IS_ENABLED(CONFIG_NET_IPV6)) {
			PR("Connecting from [%s]:%u ",
			   net_sprint_ipv6_addr(&net_sin6(myaddr)->sin6_addr),
			   ntohs(net_sin6(myaddr)->sin6_port));
			PR("to [%s]:%u\n",
			   net_sprint_ipv6_addr(&net_sin6(addr)->sin6_addr),
			   ntohs(net_sin6(addr)->sin6_port));
		} else {
			PR_INFO("IPv6 not supported\n");
		}

		break;

	default:
		PR_WARNING("Unknown protocol family (%d)\n", family);
		break;
	}
}

static void tcp_connect(const struct shell *sh, char *host, uint16_t port,
			struct net_context **ctx)
{
	struct net_if *iface = net_if_get_default();
	struct sockaddr myaddr;
	struct sockaddr addr;
	struct net_nbr *nbr;
	int addrlen;
	int family;
	int ret;

	if (IS_ENABLED(CONFIG_NET_IPV6) && !IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = net_addr_pton(AF_INET6, host,
				    &net_sin6(&addr)->sin6_addr);
		if (ret < 0) {
			PR_WARNING("Invalid IPv6 address\n");
			return;
		}

		net_sin6(&addr)->sin6_port = htons(port);
		addrlen = sizeof(struct sockaddr_in6);

		nbr = net_ipv6_nbr_lookup(NULL, &net_sin6(&addr)->sin6_addr);
		if (nbr) {
			iface = nbr->iface;
		}

		get_my_ipv6_addr(iface, &myaddr);
		family = addr.sa_family = myaddr.sa_family = AF_INET6;

	} else if (IS_ENABLED(CONFIG_NET_IPV4) &&
		   !IS_ENABLED(CONFIG_NET_IPV6)) {
		ARG_UNUSED(nbr);

		ret = net_addr_pton(AF_INET, host, &net_sin(&addr)->sin_addr);
		if (ret < 0) {
			PR_WARNING("Invalid IPv4 address\n");
			return;
		}

		get_my_ipv4_addr(iface, &myaddr);
		net_sin(&addr)->sin_port = htons(port);
		addrlen = sizeof(struct sockaddr_in);
		family = addr.sa_family = myaddr.sa_family = AF_INET;
	} else if (IS_ENABLED(CONFIG_NET_IPV6) &&
		   IS_ENABLED(CONFIG_NET_IPV4)) {
		ret = net_addr_pton(AF_INET6, host,
				    &net_sin6(&addr)->sin6_addr);
		if (ret < 0) {
			ret = net_addr_pton(AF_INET, host,
					    &net_sin(&addr)->sin_addr);
			if (ret < 0) {
				PR_WARNING("Invalid IP address\n");
				return;
			}

			net_sin(&addr)->sin_port = htons(port);
			addrlen = sizeof(struct sockaddr_in);

			get_my_ipv4_addr(iface, &myaddr);
			family = addr.sa_family = myaddr.sa_family = AF_INET;
		} else {
			net_sin6(&addr)->sin6_port = htons(port);
			addrlen = sizeof(struct sockaddr_in6);

			nbr = net_ipv6_nbr_lookup(NULL,
						  &net_sin6(&addr)->sin6_addr);
			if (nbr) {
				iface = nbr->iface;
			}

			get_my_ipv6_addr(iface, &myaddr);
			family = addr.sa_family = myaddr.sa_family = AF_INET6;
		}
	} else {
		PR_WARNING("No IPv6 nor IPv4 is enabled\n");
		return;
	}

	print_connect_info(sh, family, &myaddr, &addr);

	ret = net_context_get(family, SOCK_STREAM, IPPROTO_TCP, ctx);
	if (ret < 0) {
		PR_WARNING("Cannot get TCP context (%d)\n", ret);
		return;
	}

	ret = net_context_bind(*ctx, &myaddr, addrlen);
	if (ret < 0) {
		PR_WARNING("Cannot bind TCP (%d)\n", ret);
		return;
	}

	/* Note that we cannot put shell as a user_data when connecting
	 * because the tcp_connected() will be called much later and
	 * all local stack variables are lost at that point.
	 */
	tcp_shell = sh;

#if defined(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT)
#define CONNECT_TIMEOUT K_MSEC(CONFIG_NET_SOCKETS_CONNECT_TIMEOUT)
#else
#define CONNECT_TIMEOUT K_SECONDS(3)
#endif

	net_context_ref(*ctx);

	ret = net_context_connect(*ctx, &addr, addrlen, tcp_connected,
				  CONNECT_TIMEOUT, NULL);
	if (ret < 0) {
		PR_WARNING("Connect failed!\n");
		net_context_put(*ctx);
		tcp_ctx = NULL;
	}
}

static void tcp_sent_cb(struct net_context *context,
			int status, void *user_data)
{
	PR_SHELL(tcp_shell, "Message sent\n");
}

static void tcp_recv_cb(struct net_context *context, struct net_pkt *pkt,
			union net_ip_header *ip_hdr,
			union net_proto_header *proto_hdr,
			int status, void *user_data)
{
	int ret, len;

	if (pkt == NULL) {
		if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
			return;
		}

		ret = net_context_put(tcp_ctx);
		if (ret < 0) {
			PR_SHELL(tcp_shell,
				 "Cannot close the connection (%d)\n", ret);
			return;
		}

		PR_SHELL(tcp_shell, "Connection closed by remote peer.\n");
		tcp_ctx = NULL;

		return;
	}

	len = net_pkt_remaining_data(pkt);

	(void)net_context_update_recv_wnd(context, len);

	PR_SHELL(tcp_shell, "%zu bytes received\n", net_pkt_get_len(pkt));

	net_pkt_unref(pkt);
}
#endif

static int cmd_net_tcp_connect(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	int arg = 0;

	/* tcp connect <ip> port */
	char *endptr;
	char *ip;
	uint16_t port;

	/* tcp connect <ip> port */
	if (tcp_ctx && net_context_is_used(tcp_ctx)) {
		PR("Already connected\n");
		return -ENOEXEC;
	}

	if (!argv[++arg]) {
		PR_WARNING("Peer IP address missing.\n");
		return -ENOEXEC;
	}

	ip = argv[arg];

	if (!argv[++arg]) {
		PR_WARNING("Peer port missing.\n");
		return -ENOEXEC;
	}

	port = strtol(argv[arg], &endptr, 10);
	if (*endptr != '\0') {
		PR_WARNING("Invalid port %s\n", argv[arg]);
		return -ENOEXEC;
	}

	tcp_connect(sh, ip, port, &tcp_ctx);
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_NATIVE_TCP */

	return 0;
}

static int cmd_net_tcp_send(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	int arg = 0;
	int ret;
	struct net_shell_user_data user_data;

	/* tcp send <data> */
	if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
		PR_WARNING("Not connected\n");
		return -ENOEXEC;
	}

	if (!argv[++arg]) {
		PR_WARNING("No data to send.\n");
		return -ENOEXEC;
	}

	user_data.sh = sh;

	ret = net_context_send(tcp_ctx, (uint8_t *)argv[arg],
			       strlen(argv[arg]), tcp_sent_cb,
			       TCP_TIMEOUT, &user_data);
	if (ret < 0) {
		PR_WARNING("Cannot send msg (%d)\n", ret);
		return -ENOEXEC;
	}

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_NATIVE_TCP */

	return 0;
}

static int cmd_net_tcp_recv(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	int ret;
	struct net_shell_user_data user_data;

	/* tcp recv */
	if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
		PR_WARNING("Not connected\n");
		return -ENOEXEC;
	}

	user_data.sh = sh;

	ret = net_context_recv(tcp_ctx, tcp_recv_cb, K_NO_WAIT, &user_data);
	if (ret < 0) {
		PR_WARNING("Cannot recv data (%d)\n", ret);
		return -ENOEXEC;
	}

#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_NATIVE_TCP */

	return 0;
}

static int cmd_net_tcp_close(const struct shell *sh, size_t argc, char *argv[])
{
#if defined(CONFIG_NET_TCP) && defined(CONFIG_NET_NATIVE_TCP)
	int ret;

	/* tcp close */
	if (!tcp_ctx || !net_context_is_used(tcp_ctx)) {
		PR_WARNING("Not connected\n");
		return -ENOEXEC;
	}

	ret = net_context_put(tcp_ctx);
	if (ret < 0) {
		PR_WARNING("Cannot close the connection (%d)\n", ret);
		return -ENOEXEC;
	}

	PR("Connection closed.\n");
	tcp_ctx = NULL;
#else
	PR_INFO("Set %s to enable %s support.\n",
		"CONFIG_NET_TCP and CONFIG_NET_NATIVE", "TCP");
#endif /* CONFIG_NET_TCP */

	return 0;
}

static int cmd_net_tcp(const struct shell *sh, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(net_cmd_tcp,
	SHELL_CMD(connect, NULL,
		  "'net tcp connect <address> <port>' connects to TCP peer.",
		  cmd_net_tcp_connect),
	SHELL_CMD(send, NULL,
		  "'net tcp send <data>' sends data to peer using TCP.",
		  cmd_net_tcp_send),
	SHELL_CMD(recv, NULL,
		  "'net tcp recv' receives data using TCP.",
		  cmd_net_tcp_recv),
	SHELL_CMD(close, NULL,
		  "'net tcp close' closes TCP connection.", cmd_net_tcp_close),
	SHELL_SUBCMD_SET_END
);

SHELL_SUBCMD_ADD((net), tcp, &net_cmd_tcp,
		 "Connect/send/close TCP connection.",
		 cmd_net_tcp, 1, 0);
