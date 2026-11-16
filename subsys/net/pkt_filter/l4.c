/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(npf_l4, CONFIG_NET_PKT_FILTER_LOG_LEVEL);

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_pkt_filter.h>
#include <zephyr/net_buf.h>

/* Conditions on the header that follows the IP header and its options or
 * extension headers. They are for the local-in rules, which are applied once
 * the IP input has processed the IP headers, and use what it stored in the
 * packet instead of parsing the options and extension headers again.
 */

/* Get the protocol and the offset of the header that follows the IP headers */
static bool l4_hdr_get(struct net_pkt *pkt, uint8_t *proto, size_t *offset)
{
	if (IS_ENABLED(CONFIG_NET_IPV4) && net_pkt_family(pkt) == NET_AF_INET) {
		*proto = NET_IPV4_HDR(pkt)->proto;
		*offset = net_pkt_ip_hdr_len(pkt) + net_pkt_ipv4_opts_len(pkt);

		return true;
	}

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == NET_AF_INET6) {
		*proto = net_pkt_ipv6_next_hdr(pkt);
		*offset = net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt);

		return true;
	}

	return false;
}

/* Copy data out of the packet. A header is not read in place, as it may be
 * in any buffer of the packet and may be split between two of them.
 */
static bool l4_hdr_read(struct net_pkt *pkt, size_t offset, void *data, size_t len)
{
	return net_buf_linearize(data, len, pkt->buffer, offset, len) == len;
}

/* The ports are at the same place in the UDP and in the TCP header */
BUILD_ASSERT(offsetof(struct net_tcp_hdr, src_port) == offsetof(struct net_udp_hdr, src_port));
BUILD_ASSERT(offsetof(struct net_tcp_hdr, dst_port) == offsetof(struct net_udp_hdr, dst_port));

static bool port_match(struct npf_test *test, struct net_pkt *pkt, size_t port_offset)
{
	struct npf_test_port *test_port = CONTAINER_OF(test, struct npf_test_port, test);
	uint16_t port;
	size_t offset;
	uint8_t proto;

	if (!l4_hdr_get(pkt, &proto, &offset) || proto != test_port->proto) {
		return false;
	}

	if (!l4_hdr_read(pkt, offset + port_offset, &port, sizeof(port))) {
		return false;
	}

	NET_DBG("proto %u port %u pkt %u", proto, net_ntohs(test_port->port), net_ntohs(port));

	return port == test_port->port;
}

bool npf_src_port_match(struct npf_test *test, struct net_pkt *pkt)
{
	return port_match(test, pkt, offsetof(struct net_udp_hdr, src_port));
}

bool npf_dst_port_match(struct npf_test *test, struct net_pkt *pkt)
{
	return port_match(test, pkt, offsetof(struct net_udp_hdr, dst_port));
}
