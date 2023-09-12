/*
 * Copyright (c) 2022 T-Mobile USA, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "../../net/ip/icmpv4.h"

#include <zephyr/net/net_if.h>
#include <zephyr/net/ping.h>

static sys_slist_t cbs;

void net_ping_cb_register(struct net_ping_handler *cb)
{
	sys_slist_prepend(&cbs, &cb->node);
}

void net_ping_cb_unregister(struct net_ping_handler *cb)
{
	sys_slist_find_and_remove(&cbs, &cb->node);
}

void net_ping_resp_notify(uint32_t sent_ts)
{
	struct net_ping_handler *cb;

	SYS_SLIST_FOR_EACH_CONTAINER(&cbs, cb, node) {
		cb->handler(sent_ts);
	}
}

#ifdef CONFIG_NET_NATIVE
uint32_t sent;

static enum net_verdict
handle_native_ipv4_echo_reply(struct net_pkt *pkt, struct net_ipv4_hdr *ip_hdr,
			      struct net_icmp_hdr *icmp_hdr);

static enum net_verdict
handle_native_ipv6_echo_reply(struct net_pkt *pkt, struct net_ipv6_hdr *ip_hdr,
			      struct net_icmp_hdr *icmp_hdr);

static struct net_icmpv4_handler ping4_handler = {
	.type = NET_ICMPV4_ECHO_REPLY,
	.code = 0,
	.handler = handle_native_ipv4_echo_reply,
};

static struct net_icmpv6_handler ping6_handler = {
	.type = NET_ICMPV6_ECHO_REPLY,
	.code = 0,
	.handler = handle_native_ipv6_echo_reply,
};

static enum net_verdict handle_ipv4_echo_reply(struct net_pkt *pkt,
					       struct net_ipv4_hdr *ip_hdr,
					       struct net_icmp_hdr *icmp_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv4_echo_req);
	struct net_icmpv4_echo_req *icmp_echo;

	icmp_echo = (struct net_icmpv4_echo_req *)net_pkt_get_data(
		pkt, &icmp_access);
	if (icmp_echo == NULL) {
		return -NET_DROP;
	}
	net_ping_resp_notify(PING_OK, sent);
	net_pkt_unref(pkt);
	return NET_OK;
}

static enum net_verdict
handle_native_ipv6_echo_reply(struct net_pkt *pkt, struct net_ipv6_hdr *ip_hdr,
			      struct net_icmp_hdr *icmp_hdr)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(icmp_access,
					      struct net_icmpv6_echo_req);
	struct net_icmpv6_echo_req *icmp_echo;

	icmp_echo = (struct net_icmpv6_echo_req *)net_pkt_get_data(
		pkt, &icmp_access);
	if (icmp_echo == NULL) {
		return -NET_DROP;
	}

	net_pkt_skip(pkt, sizeof(*icmp_echo));

	net_ping_resp_notify(PING_OK, sent);
	net_pkt_unref(pkt);
	return NET_OK;
}
#endif

int net_ping(struct net_if *iface, struct sockaddr *dst, size_t size)
{
#ifdef CONFIG_NET_NATIVE
	int stat;

	if (!net_if_is_ip_offloaded(iface)) {
		sent = k_uptime_get_32();
		char *data = k_malloc(size);

		if (!data) {
			return -ENOMEM;
		}

		sys_rand_get(data, size);

		if (dst->sa_family == AF_INET) {
			net_icmpv4_register_handler(&ping4_handler);
			stat = net_icmpv4_send_echo_request(
				iface, (struct in_addr *)dst, sys_rand32_get(),
				1, data, size);
			return stat;
		} else if (dst->sa_family == AF_INET6) {
			net_icmpv6_register_handler(&ping6_handler);
			stat = net_icmpv6_send_echo_request(
				iface, (struct in6_addr *)dst, sys_rand32_get(),
				1, data, size);
			return stat;
		}
	}
#endif
#ifdef CONFIG_NET_OFFLOAD
	if (iface->if_dev->ping_offload) {
		return iface->if_dev->ping_offload(dst, size);
	}
#endif
	return -ENOTSUP;
}
