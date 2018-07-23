/*
 * Copyright (c) 2018 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_MODULE_NAME net_promisc
#define SYS_LOG_DOMAIN "promisc"
#define NET_SYS_LOG_LEVEL SYS_LOG_LEVEL_DEBUG
#define NET_LOG_ENABLED 1

#include <zephyr.h>
#include <errno.h>

#include <net/net_core.h>
#include <net/promiscuous.h>
#include <net/tcp.h>

static void iface_cb(struct net_if *iface, void *user_data)
{
	int ret;

	ret = net_promisc_mode_on(iface);
	if (ret < 0) {
		NET_INFO("Cannot set promiscuous mode for interface %p (%d)",
			 iface, ret);
		return;
	}

	NET_INFO("Promiscuous mode enabled for interface %p", iface);
}

static int get_ports(struct net_pkt *pkt, u16_t *src, u16_t *dst)
{
	struct net_tcp_hdr hdr, *tcp_hdr;

	tcp_hdr = net_tcp_get_hdr(pkt, &hdr);
	if (!tcp_hdr) {
		return -EINVAL;
	}

	*src = ntohs(tcp_hdr->src_port);
	*dst = ntohs(tcp_hdr->dst_port);

	return 0;
}

static void print_info(struct net_pkt *pkt)
{
	char src_addr_buf[NET_IPV6_ADDR_LEN], *src_addr;
	char dst_addr_buf[NET_IPV6_ADDR_LEN], *dst_addr;
	sa_family_t family = AF_UNSPEC;
	void *dst, *src;
	u16_t dst_port, src_port;
	u8_t next_hdr;
	const char *proto;
	size_t len;
	int ret;

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
		NET_INFO("Recv %p len %d (unknown address family)",
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
		NET_ERR("Cannot get port numbers for pkt %p", pkt);
		return;
	}

	src_addr = net_addr_ntop(family, src,
				 src_addr_buf, sizeof(src_addr_buf));
	dst_addr = net_addr_ntop(family, dst,
				 dst_addr_buf, sizeof(dst_addr_buf));

	len = net_pkt_get_len(pkt);

	if (family == AF_INET) {
		if (next_hdr == IPPROTO_TCP || next_hdr == IPPROTO_UDP) {
			NET_INFO("%s %s (%zd) %s:%u -> %s:%u",
				 "IPv4", proto, len, src_addr, src_port,
				 dst_addr, dst_port);
		} else {
			NET_INFO("%s %s (%zd) %s -> %s", "IPv4", proto,
				 len, src_addr, dst_addr);
		}
	} else {
		if (next_hdr == IPPROTO_TCP || next_hdr == IPPROTO_UDP) {
			NET_INFO("%s %s (%zd) [%s]:%u -> [%s]:%u",
				 "IPv6", proto, len, src_addr, src_port,
				 dst_addr, dst_port);
		} else {
			NET_INFO("%s %s (%zd) %s -> %s", "IPv6", proto,
				 len, src_addr, dst_addr);
		}
	}
}

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
