/** @file
 * @brief Network initialization
 *
 * Initialize the network IP stack. Create one thread for reading data
 * from IP stack and passing that data to applications (Rx thread).
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_NET_DEBUG_CORE)
#define SYS_LOG_DOMAIN "net/core"
#define NET_LOG_ENABLED 1
#endif

#include <init.h>
#include <kernel.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/net_mgmt.h>
#include <net/arp.h>
#include <net/nbuf.h>
#include <net/net_core.h>

#include "net_private.h"
#include "net_shell.h"

#include "icmpv6.h"
#include "ipv6.h"

#if defined(CONFIG_NET_IPV4)
#include "icmpv4.h"
#endif

#include "route.h"
#include "rpl.h"

#include "connection.h"
#include "udp.h"
#include "tcp.h"

#include "net_stats.h"

/* Stack for the rx thread.
 */
#if !defined(CONFIG_NET_RX_STACK_SIZE)
#define CONFIG_NET_RX_STACK_SIZE 1024
#endif

NET_STACK_DEFINE(RX, rx_stack, CONFIG_NET_RX_STACK_SIZE,
		 CONFIG_NET_RX_STACK_SIZE + CONFIG_NET_RX_STACK_RPL);

static struct k_fifo rx_queue;
static k_tid_t rx_tid;

#if defined(CONFIG_NET_IPV6)
static inline enum net_verdict process_icmpv6_pkt(struct net_buf *buf,
						  struct net_ipv6_hdr *ipv6)
{
	struct net_icmp_hdr *hdr = NET_ICMP_BUF(buf);
	uint16_t len = (ipv6->len[0] << 8) + ipv6->len[1];

	NET_DBG("ICMPv6 packet received length %d type %d code %d",
		len, hdr->type, hdr->code);

	return net_icmpv6_input(buf, len, hdr->type, hdr->code);
}

static inline struct net_buf *check_unknown_option(struct net_buf *buf,
						   uint8_t opt_type,
						   uint16_t length)
{
	/* RFC 2460 chapter 4.2 tells how to handle the unknown
	 * options by the two highest order bits of the option:
	 *
	 * 00: Skip over this option and continue processing the header.
	 * 01: Discard the packet.
	 * 10: Discard the packet and, regardless of whether or not the
	 *     packet's Destination Address was a multicast address,
	 *     send an ICMP Parameter Problem, Code 2, message to the packet's
	 *     Source Address, pointing to the unrecognized Option Type.
	 * 11: Discard the packet and, only if the packet's Destination
	 *     Address was not a multicast address, send an ICMP Parameter
	 *     Problem, Code 2, message to the packet's Source Address,
	 *     pointing to the unrecognized Option Type.
	 */
	NET_DBG("Unknown option %d MSB %d", opt_type, opt_type >> 6);

	switch (opt_type & 0xc0) {
	case 0x00:
		break;
	case 0x40:
		return NULL;
	case 0xc0:
		if (net_is_ipv6_addr_mcast(&NET_IPV6_BUF(buf)->dst)) {
			return NULL;
		}
		/* passthrough */
	case 0x80:
		net_icmpv6_send_error(buf, NET_ICMPV6_PARAM_PROBLEM,
				      NET_ICMPV6_PARAM_PROB_OPTION,
				      (uint32_t)length);
		return NULL;
	}

	return buf;
}

static inline struct net_buf *handle_ext_hdr_options(struct net_buf *buf,
						     struct net_buf *frag,
						     int total_len,
						     uint16_t len,
						     uint16_t offset,
						     uint16_t *pos,
						     enum net_verdict *verdict)
{
	uint8_t opt_type, opt_len;
	uint16_t length = 0;

	if (len > total_len) {
		NET_DBG("Corrupted packet, extension header %d too long "
			"(max %d bytes)", len, total_len);
		*verdict = NET_DROP;
		return NULL;
	}

	length += 2;

	/* Each extension option has type and length */
	frag = net_nbuf_read_u8(frag, offset, pos, &opt_type);
	frag = net_nbuf_read_u8(frag, *pos, pos, &opt_len);

	while (frag && (length < len)) {
		switch (opt_type) {
		case NET_IPV6_EXT_HDR_OPT_PAD1:
			NET_DBG("PAD1 option");
			length++;
			break;
		case NET_IPV6_EXT_HDR_OPT_PADN:
			NET_DBG("PADN option");
			length += opt_len + 2;
			break;
		case NET_IPV6_EXT_HDR_OPT_RPL:
			/* Handle the case when a non-RPL node joins RPL
			 * network. Skip the unknown header instead of
			 * discarding the whole packet.
			 */
#if defined(CONFIG_NET_RPL)
			NET_DBG("Processing RPL option");
			if (!net_rpl_verify_header(buf, *pos, pos)) {
				NET_DBG("RPL option error, packet dropped");
				*verdict = NET_DROP;

				return NULL;
			}
#endif
			*verdict = NET_CONTINUE;
			return frag;
		default:
			if (!check_unknown_option(frag, opt_type, length)) {
				*verdict = NET_DROP;
				return NULL;
			}

			length += opt_len + 2;
			break;
		}

		frag = net_nbuf_read_u8(frag, *pos, pos, &opt_type);
		frag = net_nbuf_read_u8(frag, *pos, pos, &opt_len);
		if (!frag && *pos == 0xffff) {
			/* reading error */
			return NULL;
		}
	}

	*verdict = NET_CONTINUE;
	return frag;
}

static inline enum net_verdict process_ipv6_pkt(struct net_buf *buf)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_BUF(buf);
	int real_len = net_buf_frags_len(buf);
	int pkt_len = (hdr->len[0] << 8) + hdr->len[1] + sizeof(*hdr);
	struct net_buf *frag;
	uint8_t next, next_hdr, length;
	uint8_t first_option;
	uint16_t offset;

	if (real_len != pkt_len) {
		NET_DBG("IPv6 packet size %d buf len %d", pkt_len, real_len);
		net_stats_update_ipv6_drop();
		goto drop;
	}

#if defined(CONFIG_NET_DEBUG_CORE)
	do {
		char out[NET_IPV6_ADDR_LEN];
		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv6_addr(&hdr->dst));
		NET_DBG("IPv6 packet len %d received from %s to %s",
			real_len, net_sprint_ipv6_addr(&hdr->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_CORE */

	if (net_is_ipv6_addr_mcast(&hdr->src)) {
		NET_DBG("Dropping src multicast packet");
		net_stats_update_ipv6_drop();
		goto drop;
	}

	if (!net_is_my_ipv6_addr(&hdr->dst) &&
	    !net_is_my_ipv6_maddr(&hdr->dst) &&
	    !net_is_ipv6_addr_mcast(&hdr->dst) &&
	    !net_is_ipv6_addr_loopback(&hdr->dst)) {
		NET_DBG("IPv6 packet in buf %p not for me", buf);
		net_stats_update_ipv6_drop();
		goto drop;
	}

	/* Check extension headers */
	net_nbuf_set_next_hdr(buf, &hdr->nexthdr);
	net_nbuf_set_ext_len(buf, 0);
	net_nbuf_set_ext_bitmap(buf, 0);

	/* Fast path for main upper layer protocols. The handling of extension
	 * headers can be slow so do this checking here. There cannot
	 * be any extension headers after the upper layer protocol header.
	 */
	switch (*(net_nbuf_next_hdr(buf))) {
	case IPPROTO_ICMPV6:
		net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));
		return process_icmpv6_pkt(buf, hdr);

#if defined(CONFIG_NET_UDP)
	case IPPROTO_UDP:
		net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));
		return net_conn_input(IPPROTO_UDP, buf);
#endif
#if defined(CONFIG_NET_TCP)
	case IPPROTO_TCP:
		net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));
		return net_conn_input(IPPROTO_TCP, buf);
#endif
	}

	/* Go through the extensions */
	frag = buf->frags;
	next = hdr->nexthdr;
	first_option = next;
	offset = sizeof(struct net_ipv6_hdr);

	while (frag) {
		enum net_verdict verdict;

		frag = net_nbuf_read_u8(frag, offset, &offset, &next_hdr);
		frag = net_nbuf_read_u8(frag, offset, &offset, &length);
		if (!frag) {
			goto drop;
		}

		length = length * 8 + 8;
		verdict = NET_OK;

#if defined(CONFIG_NET_DEBUG_CORE)
		/* Print the length only for IPv6 extension */
		if (next != IPPROTO_ICMPV6 && next != IPPROTO_UDP &&
		    next != IPPROTO_TCP) {
			NET_DBG("IPv6 next header %d length %d bytes",
				next, length);
		} else {
			NET_DBG("IPv6 next header %d", next);
		}
#endif /* CONFIG_NET_DEBUG_CORE */

		switch (next) {
		case NET_IPV6_NEXTHDR_NONE:
			/* There is nothing after this header (see RFC 2460,
			 * ch 4.7), so we can drop the packet now.
			 * This is not an error case so do not update drop
			 * statistics.
			 */
			goto drop;

		case NET_IPV6_NEXTHDR_HBHO:
			/* Hop by hop option */
			if (net_nbuf_ext_bitmap(buf) &
			    NET_IPV6_EXT_HDR_BITMAP_HBHO) {
				goto bad_hdr;
			}
			/* HBH option needs to be the first one */
			if (first_option != NET_IPV6_NEXTHDR_HBHO) {
				goto bad_hdr;
			}

			net_nbuf_add_ext_bitmap(buf,
						NET_IPV6_EXT_HDR_BITMAP_HBHO);

			frag = handle_ext_hdr_options(buf, frag, real_len,
						      length, offset, &offset,
						      &verdict);
			if (verdict == NET_DROP) {
				goto drop;
			} else if (verdict == NET_CONTINUE) {
				/* ignore the option */
				break;
			}

			if (!frag && offset) {
				/* Header issue, the ICMPv6 parameter problem
				 * error is already sent so just drop the msg
				 * here.
				 */
				goto drop;
			}

			break;

		/* The next header after the extensions can be also
		 * one of the main protocols.
		 */
		case IPPROTO_ICMPV6:
			net_nbuf_set_ext_len(buf,
					     offset -
					     sizeof(struct net_ipv6_hdr));
			net_nbuf_set_ip_hdr_len(buf,
						sizeof(struct net_ipv6_hdr));
			return process_icmpv6_pkt(buf, hdr);

#if defined(CONFIG_NET_UDP)
		case IPPROTO_UDP:
			net_nbuf_set_ext_len(buf,
					     offset -
					     sizeof(struct net_ipv6_hdr));
			net_nbuf_set_ip_hdr_len(buf,
						sizeof(struct net_ipv6_hdr));
			return net_conn_input(IPPROTO_UDP, buf);
#endif
#if defined(CONFIG_NET_TCP)
		case IPPROTO_TCP:
			net_nbuf_set_ext_len(buf,
					     offset -
					     sizeof(struct net_ipv6_hdr));
			net_nbuf_set_ip_hdr_len(buf,
						sizeof(struct net_ipv6_hdr));
			return net_conn_input(IPPROTO_TCP, buf);
#endif
		default:
			goto bad_hdr;
		}

		next = next_hdr;
	}

drop:
	return NET_DROP;

bad_hdr:
	/* Send error message about parameter problem (RFC 2460)
	 */
	net_icmpv6_send_error(buf, NET_ICMPV6_PARAM_PROBLEM,
			      NET_ICMPV6_PARAM_PROB_NEXTHEADER,
			      offset - 1);

	NET_DBG("Unknown next header type");
	net_stats_update_ip_errors_protoerr();

	return NET_DROP;
}
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
static inline enum net_verdict process_icmpv4_pkt(struct net_buf *buf,
						  struct net_ipv4_hdr *ipv4)
{
	struct net_icmp_hdr *hdr = NET_ICMP_BUF(buf);
	uint16_t len = (ipv4->len[0] << 8) + ipv4->len[1];

	NET_DBG("ICMPv4 packet received length %d type %d code %d",
		len, hdr->type, hdr->code);

	return net_icmpv4_input(buf, len, hdr->type, hdr->code);
}
#endif /* CONFIG_NET_IPV4 */

#if defined(CONFIG_NET_IPV4)
static inline enum net_verdict process_ipv4_pkt(struct net_buf *buf)
{
	struct net_ipv4_hdr *hdr = NET_IPV4_BUF(buf);
	int real_len = net_buf_frags_len(buf);
	int pkt_len = (hdr->len[0] << 8) + hdr->len[1];
	enum net_verdict verdict = NET_DROP;

	if (real_len != pkt_len) {
		NET_DBG("IPv4 packet size %d buf len %d", pkt_len, real_len);
		goto drop;
	}

#if defined(CONFIG_NET_DEBUG_CORE)
	do {
		char out[sizeof("xxx.xxx.xxx.xxx")];
		snprintk(out, sizeof(out), "%s",
			 net_sprint_ipv4_addr(&hdr->dst));
		NET_DBG("IPv4 packet received from %s to %s",
			net_sprint_ipv4_addr(&hdr->src), out);
	} while (0);
#endif /* CONFIG_NET_DEBUG_CORE */

	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv4_hdr));

	if (!net_is_my_ipv4_addr(&hdr->dst)) {
#if defined(CONFIG_NET_DHCPV4)
		if (hdr->proto == IPPROTO_UDP &&
		    net_ipv4_addr_cmp(&hdr->dst,
				      net_ipv4_broadcast_address())) {

			verdict = net_conn_input(IPPROTO_UDP, buf);
			if (verdict != NET_DROP) {
				return verdict;
			}
		}
#endif
		NET_DBG("IPv4 packet in buf %p not for me", buf);
		goto drop;
	}

	switch (hdr->proto) {
	case IPPROTO_ICMP:
		verdict = process_icmpv4_pkt(buf, hdr);
		break;
	case IPPROTO_UDP:
		verdict = net_conn_input(IPPROTO_UDP, buf);
		break;
	case IPPROTO_TCP:
		verdict = net_conn_input(IPPROTO_TCP, buf);
		break;
	}

	if (verdict != NET_DROP) {
		return verdict;
	}

drop:
	net_stats_update_ipv4_drop();
	return NET_DROP;
}
#endif /* CONFIG_NET_IPV4 */

static inline enum net_verdict process_data(struct net_buf *buf,
					    bool is_loopback)
{
	int ret;

	/* If there is no data, then drop the packet. Also if
	 * the buffer is wrong type, then also drop the packet.
	 * The first buffer needs to have user data part that
	 * contains user data. The rest of the fragments should
	 * be data fragments without user data.
	 */
	if (!buf->frags || !buf->pool->user_data_size) {
		NET_DBG("Corrupted buffer (frags %p, data size %u)",
			buf->frags, buf->pool->user_data_size);
		net_stats_update_processing_error();

		return NET_DROP;
	}

	if (!is_loopback) {
		ret = net_if_recv_data(net_nbuf_iface(buf), buf);
		if (ret != NET_CONTINUE) {
			if (ret == NET_DROP) {
				NET_DBG("Buffer %p discarded by L2", buf);
				net_stats_update_processing_error();
			}

			return ret;
		}
	}

	/* IP version and header length. */
	switch (NET_IPV6_BUF(buf)->vtc & 0xf0) {
#if defined(CONFIG_NET_IPV6)
	case 0x60:
		net_stats_update_ipv6_recv();
		net_nbuf_set_family(buf, PF_INET6);
		return process_ipv6_pkt(buf);
#endif
#if defined(CONFIG_NET_IPV4)
	case 0x40:
		net_stats_update_ipv4_recv();
		net_nbuf_set_family(buf, PF_INET);
		return process_ipv4_pkt(buf);
#endif
	}

	NET_DBG("Unknown IP family packet (0x%x)",
		NET_IPV6_BUF(buf)->vtc & 0xf0);
	net_stats_update_ip_errors_protoerr();
	net_stats_update_ip_errors_vhlerr();

	return NET_DROP;
}

static void processing_data(struct net_buf *buf, bool is_loopback)
{
	switch (process_data(buf, is_loopback)) {
	case NET_OK:
		NET_DBG("Consumed buf %p", buf);
		break;
	case NET_DROP:
	default:
		NET_DBG("Dropping buf %p", buf);
		net_nbuf_unref(buf);
		break;
	}
}

static void net_rx_thread(void)
{
	struct net_buf *buf;

	NET_DBG("Starting RX thread (stack %zu bytes)", sizeof(rx_stack));

	/* Starting TX side. The ordering is important here and the TX
	 * can only be started when RX side is ready to receive packets.
	 */
	net_if_init();

	while (1) {
		buf = net_buf_get(&rx_queue, K_FOREVER);

		net_analyze_stack("RX thread", rx_stack, sizeof(rx_stack));

		NET_DBG("Received buf %p len %zu", buf,
			net_buf_frags_len(buf));

		processing_data(buf, false);

		net_print_statistics();
		net_nbuf_print();

		k_yield();
	}
}

static void init_rx_queue(void)
{
	k_fifo_init(&rx_queue);

	rx_tid = k_thread_spawn(rx_stack, sizeof(rx_stack),
				(k_thread_entry_t)net_rx_thread,
				NULL, NULL, NULL, K_PRIO_COOP(8), 0, 0);
}

#if defined(CONFIG_NET_IP_ADDR_CHECK)
/* Check if the IPv{4|6} addresses are proper. As this can be expensive,
 * make this optional.
 */
static inline int check_ip_addr(struct net_buf *buf)
{
#if defined(CONFIG_NET_IPV6)
	if (net_nbuf_family(buf) == AF_INET6) {
		if (net_ipv6_addr_cmp(&NET_IPV6_BUF(buf)->dst,
				      net_ipv6_unspecified_address())) {
			NET_DBG("IPv6 dst address missing");
			return -EADDRNOTAVAIL;
		}

		if (net_is_ipv6_addr_loopback(&NET_IPV6_BUF(buf)->dst)) {
			struct in6_addr addr;

			/* Swap the addresses so that in receiving side
			 * the packet is accepted.
			 */
			net_ipaddr_copy(&addr, &NET_IPV6_BUF(buf)->src);
			net_ipaddr_copy(&NET_IPV6_BUF(buf)->src,
					&NET_IPV6_BUF(buf)->dst);
			net_ipaddr_copy(&NET_IPV6_BUF(buf)->dst, &addr);

			return 1;
		}

		/* The source check must be done after the destination check
		 * as having src ::1 is perfectly ok if dst is ::1 too.
		 */
		if (net_is_ipv6_addr_loopback(&NET_IPV6_BUF(buf)->src)) {
			NET_DBG("IPv6 loopback src address");
			return -EADDRNOTAVAIL;
		}
	} else
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
	if (net_nbuf_family(buf) == AF_INET) {
		if (net_ipv4_addr_cmp(&NET_IPV4_BUF(buf)->dst,
				      net_ipv4_unspecified_address())) {
			return -EADDRNOTAVAIL;
		}

		if (net_is_ipv4_addr_loopback(&NET_IPV4_BUF(buf)->dst)) {
			struct in_addr addr;

			/* Swap the addresses so that in receiving side
			 * the packet is accepted.
			 */
			net_ipaddr_copy(&addr, &NET_IPV4_BUF(buf)->src);
			net_ipaddr_copy(&NET_IPV4_BUF(buf)->src,
					&NET_IPV4_BUF(buf)->dst);
			net_ipaddr_copy(&NET_IPV4_BUF(buf)->dst, &addr);

			return 1;
		}

		/* The source check must be done after the destination check
		 * as having src 127.0.0.0/8 is perfectly ok if dst is in
		 * localhost subnet too.
		 */
		if (net_is_ipv4_addr_loopback(&NET_IPV4_BUF(buf)->src)) {
			NET_DBG("IPv4 loopback src address");
			return -EADDRNOTAVAIL;
		}
	} else
#endif /* CONFIG_NET_IPV4 */

	{
		;
	}

	return 0;
}
#else
#define check_ip_addr(buf) 0
#endif

/* Called when data needs to be sent to network */
int net_send_data(struct net_buf *buf)
{
	int status;

	if (!buf || !buf->frags) {
		return -ENODATA;
	}

	if (!net_nbuf_iface(buf)) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_STATISTICS)
	switch (net_nbuf_family(buf)) {
	case AF_INET:
		net_stats_update_ipv4_sent();
		break;
	case AF_INET6:
		net_stats_update_ipv6_sent();
		break;
	}
#endif

	status = check_ip_addr(buf);
	if (status < 0) {
		return status;
	} else if (status > 0) {
		/* Packet is destined back to us so send it directly
		 * to RX processing.
		 */
		processing_data(buf, true);
		return 0;
	}

	if (net_if_send_data(net_nbuf_iface(buf), buf) == NET_DROP) {
		return -EIO;
	}

	return 0;
}

/* Called by driver when an IP packet has been received */
int net_recv_data(struct net_if *iface, struct net_buf *buf)
{
	if (!buf->frags) {
		return -ENODATA;
	}

	NET_DBG("fifo %p iface %p buf %p len %zu", &rx_queue, iface, buf,
		net_buf_frags_len(buf));

	net_nbuf_set_iface(buf, iface);

	net_buf_put(&rx_queue, buf);

	return 0;
}

static inline void l3_init(void)
{
	net_icmpv6_init();
	net_ipv6_init();

#if defined(CONFIG_NET_UDP) || defined(CONFIG_NET_TCP)
	net_conn_init();
#endif
	net_udp_init();
	net_tcp_init();

	net_route_init();

	NET_DBG("Network L3 init done");
}

static inline void l2_init(void)
{
	net_arp_init();

	NET_DBG("Network L2 init done");
}

static int net_init(struct device *unused)
{
	NET_DBG("Priority %d", CONFIG_NET_INIT_PRIO);

	net_shell_init();

	net_nbuf_init();

	net_context_init();

	l2_init();
	l3_init();

	net_mgmt_event_init();

	init_rx_queue();

	return 0;
}

SYS_INIT(net_init, POST_KERNEL, CONFIG_NET_INIT_PRIO);
