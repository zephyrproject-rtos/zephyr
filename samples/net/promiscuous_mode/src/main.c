/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_promisc_sample, LOG_LEVEL_INF);

#include <zephyr/zephyr.h>
#include <errno.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/promiscuous.h>
#include <zephyr/net/udp.h>

static void net_pkt_hexdump(struct net_pkt *pkt, const char *str)
{
	struct net_buf *buf = pkt->buffer;

	while (buf) {
		LOG_HEXDUMP_DBG(buf->data, buf->len, str);
		buf = buf->frags;
	}
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	int ret;

	ret = net_promisc_mode_on(iface);
	if (ret < 0) {
		LOG_INF("Cannot set promiscuous mode for interface %p (%d)",
			iface, ret);
		return;
	}

	LOG_INF("Promiscuous mode enabled for interface %p", iface);
}

static int get_ports(struct net_pkt *pkt, uint16_t *src, uint16_t *dst)
{
	struct net_udp_hdr hdr, *udp_hdr;

	udp_hdr = net_udp_get_hdr(pkt, &hdr);
	if (!udp_hdr) {
		return -EINVAL;
	}

	*src = ntohs(udp_hdr->src_port);
	*dst = ntohs(udp_hdr->dst_port);

	return 0;
}

static void print_info(struct net_pkt *pkt)
{
	char src_addr_buf[NET_IPV6_ADDR_LEN], *src_addr;
	char dst_addr_buf[NET_IPV6_ADDR_LEN], *dst_addr;
	uint16_t dst_port = 0U, src_port = 0U;
	sa_family_t family = AF_UNSPEC;
	void *dst, *src;
	uint8_t next_hdr;
	const char *proto;
	size_t len;
	int ret;

	/* Enable hexdump by setting the log level to LOG_LEVEL_DBG */
	net_pkt_hexdump(pkt, "Network packet");

	switch (NET_IPV6_HDR(pkt)->vtc & 0xf0) {
	case 0x60:
		family = AF_INET6;
		net_pkt_set_family(pkt, AF_INET6);
		dst = &NET_IPV6_HDR(pkt)->dst;
		src = &NET_IPV6_HDR(pkt)->src;
		next_hdr = NET_IPV6_HDR(pkt)->nexthdr;
		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv6_hdr));
		break;
	case 0x40:
		family = AF_INET;
		net_pkt_set_family(pkt, AF_INET);
		dst = &NET_IPV4_HDR(pkt)->dst;
		src = &NET_IPV4_HDR(pkt)->src;
		next_hdr = NET_IPV4_HDR(pkt)->proto;
		net_pkt_set_ip_hdr_len(pkt, sizeof(struct net_ipv4_hdr));
		break;
	}

	if (family == AF_UNSPEC) {
		LOG_INF("Recv %p len %zd (unknown address family)",
			pkt, net_pkt_get_len(pkt));
		return;
	}

	ret = 0;

	switch (next_hdr) {
	case IPPROTO_TCP:
		proto = "TCP";
		ret = get_ports(pkt, &src_port, &dst_port);
		break;
	case IPPROTO_UDP:
		proto = "UDP";
		ret = get_ports(pkt, &src_port, &dst_port);
		break;
	case IPPROTO_ICMPV6:
	case IPPROTO_ICMP:
		proto = "ICMP";
		break;
	default:
		proto = "<unknown>";
		break;
	}

	if (ret < 0) {
		LOG_ERR("Cannot get port numbers for pkt %p", pkt);
		return;
	}

	src_addr = net_addr_ntop(family, src,
				 src_addr_buf, sizeof(src_addr_buf));
	dst_addr = net_addr_ntop(family, dst,
				 dst_addr_buf, sizeof(dst_addr_buf));

	len = net_pkt_get_len(pkt);

	if (family == AF_INET) {
		if (next_hdr == IPPROTO_TCP || next_hdr == IPPROTO_UDP) {
			LOG_INF("%s %s (%zd) %s:%u -> %s:%u",
				"IPv4", proto, len,
				log_strdup(src_addr), src_port,
				log_strdup(dst_addr), dst_port);
		} else {
			LOG_INF("%s %s (%zd) %s -> %s", "IPv4", proto,
				len, log_strdup(src_addr),
				log_strdup(dst_addr));
		}
	} else {
		if (next_hdr == IPPROTO_TCP || next_hdr == IPPROTO_UDP) {
			LOG_INF("%s %s (%zd) [%s]:%u -> [%s]:%u",
				"IPv6", proto, len,
				log_strdup(src_addr), src_port,
				log_strdup(dst_addr), dst_port);
		} else {
			LOG_INF("%s %s (%zd) %s -> %s", "IPv6", proto,
				len, log_strdup(src_addr),
				log_strdup(dst_addr));
		}
	}
}

static int set_promisc_mode(const struct shell *shell,
			    size_t argc, char *argv[], bool enable)
{
	struct net_if *iface;
	char *endptr;
	int idx, ret;

	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "Invalid arguments.\n");
		return -ENOEXEC;
	}

	idx = strtol(argv[1], &endptr, 10);

	iface = net_if_get_by_index(idx);
	if (!iface) {
		shell_fprintf(shell, SHELL_ERROR,
			      "Cannot find network interface for index %d\n",
			      idx);
		return -ENOEXEC;
	}

	shell_fprintf(shell, SHELL_INFO, "Promiscuous mode %s...\n",
		      enable ? "ON" : "OFF");

	if (enable) {
		ret = net_promisc_mode_on(iface);
	} else {
		ret = net_promisc_mode_off(iface);
	}

	if (ret < 0) {
		if (ret == -EALREADY) {
			shell_fprintf(shell, SHELL_INFO,
				      "Promiscuous mode already %s\n",
				      enable ? "enabled" : "disabled");
		} else {
			shell_fprintf(shell, SHELL_ERROR,
				      "Cannot %s promiscuous mode for "
				      "interface %p (%d)\n",
				      enable ? "set" : "unset", iface, ret);
		}

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_promisc_on(const struct shell *shell,
			  size_t argc, char *argv[])
{
	return set_promisc_mode(shell, argc, argv, true);
}

static int cmd_promisc_off(const struct shell *shell,
			   size_t argc, char *argv[])
{
	return set_promisc_mode(shell, argc, argv, false);
}

SHELL_STATIC_SUBCMD_SET_CREATE(promisc_commands,
	SHELL_CMD(on, NULL,
		  "Turn promiscuous mode on\n"
		  "promisc on  <interface index>  "
		      "Turn on promiscuous mode for the interface\n",
		  cmd_promisc_on),
	SHELL_CMD(off, NULL, "Turn promiscuous mode off\n"
		  "promisc off <interface index>  "
		      "Turn off promiscuous mode for the interface\n",
		  cmd_promisc_off),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(promisc, &promisc_commands,
		   "Promiscuous mode commands", NULL);

void main(void)
{
	struct net_pkt *pkt;

	net_if_foreach(iface_cb, NULL);

	while (1) {
		pkt = net_promisc_mode_wait_data(K_FOREVER);
		if (pkt) {
			print_info(pkt);
		}

		net_pkt_unref(pkt);
	}
}
