/** @file
 * @brief Network initialization
 *
 * Initialize the network IP stack. Create two fibers, one for reading data
 * from applications (Tx fiber) and one for reading data from IP stack
 * and passing that data to applications (Rx fiber).
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_CORE)
#define SYS_LOG_DOMAIN "net/core"
#define NET_DEBUG 1
#endif

#include <init.h>
#include <nanokernel.h>
#include <toolchain.h>
#include <sections.h>
#include <string.h>
#include <errno.h>

#include <net/net_if.h>
#include <net/arp.h>
#include <net/nbuf.h>
#include <net/net_core.h>
#include <net/net_stats.h>

#include "net_private.h"

#if defined(CONFIG_NET_IPV6)
#include "icmpv6.h"
#endif

#if defined(CONFIG_NET_IPV4)
#include "icmpv4.h"
#endif

/* Stack for the rx fiber.
 */
#if !defined(CONFIG_NET_RX_STACK_SIZE)
#define CONFIG_NET_RX_STACK_SIZE 1024
#endif
static char __noinit __stack rx_fiber_stack[CONFIG_NET_RX_STACK_SIZE];
static struct nano_fifo rx_queue;
static nano_thread_id_t rx_fiber_id;

#if defined(CONFIG_NET_STATISTICS)
#define PRINT_STAT(fmt, ...) printk(fmt, ##__VA_ARGS__)
struct net_stats net_stats;
#define GET_STAT(s) net_stats.s
#define PRINT_STATISTICS_INTERVAL (30 * sys_clock_ticks_per_sec)
#define net_print_statistics stats /* to make the debug print line shorter */

static inline void stats(void)
{
	static uint32_t next_print;
	uint32_t curr = sys_tick_get_32();

	if (!next_print || (next_print < curr &&
	    (!((curr - next_print) > PRINT_STATISTICS_INTERVAL)))) {
		uint32_t new_print;

#if defined(CONFIG_NET_IPV6)
		PRINT_STAT("IPv6 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
			 GET_STAT(ipv6.recv),
			 GET_STAT(ipv6.sent),
			 GET_STAT(ipv6.drop),
			 GET_STAT(ipv6.forwarded));
#endif /* CONFIG_NET_IPV6 */

#if defined(CONFIG_NET_IPV4)
		PRINT_STAT("IPv4 recv      %d\tsent\t%d\tdrop\t%d\tforwarded\t%d\n",
			 GET_STAT(ipv4.recv),
			 GET_STAT(ipv4.sent),
			 GET_STAT(ipv4.drop),
			 GET_STAT(ipv4.forwarded));
#endif /* CONFIG_NET_IPV4 */

		PRINT_STAT("IP vhlerr      %d\thblener\t%d\tlblener\t%d\n",
			 GET_STAT(ip_errors.vhlerr),
			 GET_STAT(ip_errors.hblenerr),
			 GET_STAT(ip_errors.lblenerr));
		PRINT_STAT("IP fragerr     %d\tchkerr\t%d\tprotoer\t%d\n",
			 GET_STAT(ip_errors.fragerr),
			 GET_STAT(ip_errors.chkerr),
			 GET_STAT(ip_errors.protoerr));

		PRINT_STAT("ICMP recv      %d\tsent\t%d\tdrop\t%d\n",
			 GET_STAT(icmp.recv),
			 GET_STAT(icmp.sent),
			 GET_STAT(icmp.drop));
		PRINT_STAT("ICMP typeer    %d\tchkerr\t%d\n",
			 GET_STAT(icmp.typeerr),
			 GET_STAT(icmp.chkerr));

#if defined(CONFIG_NET_UDP)
		PRINT_STAT("UDP recv       %d\tsent\t%d\tdrop\t%d\n",
			 GET_STAT(udp.recv),
			 GET_STAT(udp.sent),
			 GET_STAT(udp.drop));
		PRINT_STAT("UDP chkerr     %d\n",
			 GET_STAT(icmp.chkerr));
#endif

		PRINT_STAT("Processing err %d\n", GET_STAT(processing_error));

		new_print = curr + PRINT_STATISTICS_INTERVAL;
		if (new_print > curr) {
			next_print = new_print;
		} else {
			/* Overflow */
			next_print = PRINT_STATISTICS_INTERVAL -
				(0xffffffff - curr);
		}
	}
}
#else /* CONFIG_NET_STATISTICS */
#define net_print_statistics()
#endif /* CONFIG_NET_STATISTICS */

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

static inline enum net_verdict process_ipv6_pkt(struct net_buf *buf)
{
	struct net_ipv6_hdr *hdr = NET_IPV6_BUF(buf);
	int real_len = net_buf_frags_len(buf);
	int pkt_len = (hdr->len[0] << 8) + hdr->len[1] + sizeof(*hdr);

	if (real_len > pkt_len) {
		NET_DBG("IPv6 adjust pkt len to %d (was %d)",
			pkt_len, real_len);
		net_buf_frag_last(buf)->len -= real_len - pkt_len;
		real_len -= pkt_len;
	} else if (real_len < pkt_len) {
		NET_DBG("IPv6 packet size %d buf len %d", pkt_len, real_len);
		NET_STATS(++net_stats.ipv6.drop);
		goto drop;
	}

#if NET_DEBUG > 0
	do {
		char out[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx")];
		snprintf(out, sizeof(out), net_sprint_ipv6_addr(&hdr->dst));
		NET_DBG("IPv6 packet len %d received from %s to %s",
			real_len, net_sprint_ipv6_addr(&hdr->src), out);
	} while (0);
#endif /* NET_DEBUG > 0 */

	if (net_is_ipv6_addr_mcast(&hdr->src)) {
		NET_STATS(++net_stats.ipv6.drop);
		NET_DBG("Dropping src multicast packet");
		goto drop;
	}

	if (!net_is_my_ipv6_addr(&hdr->dst) &&
	    !net_is_my_ipv6_maddr(&hdr->dst) &&
	    !net_is_ipv6_addr_mcast(&hdr->dst) &&
	    !net_is_ipv6_addr_loopback(&hdr->dst)) {
		NET_DBG("IPv6 packet in buf %p not for me", buf);
		NET_STATS(++net_stats.ipv6.drop);
		goto drop;
	}

	/* Check extension headers */
	net_nbuf_next_hdr(buf) = &hdr->nexthdr;
	net_nbuf_ext_len(buf) = 0;
	net_nbuf_ext_bitmap(buf) = 0;

	while (1) {
		switch (*(net_nbuf_next_hdr(buf))) {

		case IPPROTO_ICMPV6:
			net_nbuf_ip_hdr_len(buf) = sizeof(struct net_ipv6_hdr);
			return process_icmpv6_pkt(buf, hdr);

		default:
			goto bad_header;
		}
	}

bad_header:
drop:
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

	if (real_len > pkt_len) {
		NET_DBG("IPv4 adjust pkt len to %d (was %d)",
			pkt_len, real_len);
		net_buf_frag_last(buf)->len -= real_len - pkt_len;
		real_len -= pkt_len;
	} else if (real_len < pkt_len) {
		NET_DBG("IPv4 packet size %d buf len %d", pkt_len, real_len);
		NET_STATS(++net_stats.ipv4.drop);
		return NET_DROP;
	}

#if NET_DEBUG > 0
	do {
		char out[sizeof("xxx.xxx.xxx.xxx")];
		snprintf(out, sizeof(out), net_sprint_ipv4_addr(&hdr->dst));
		NET_DBG("IPv4 packet received from %s to %s",
			net_sprint_ipv4_addr(&hdr->src), out);
	} while (0);
#endif /* NET_DEBUG > 0 */

	if (!net_is_my_ipv4_addr(&hdr->dst)) {
		NET_DBG("IPv4 packet in buf %p not for me", buf);
		NET_STATS(++net_stats.ipv4.drop);
		goto drop;
	}

	switch (hdr->proto) {
	case IPPROTO_ICMP:
		net_nbuf_ip_hdr_len(buf) = sizeof(struct net_ipv4_hdr);
		return process_icmpv4_pkt(buf, hdr);
	}

drop:
	return NET_DROP;
}
#endif /* CONFIG_NET_IPV4 */

static inline enum net_verdict process_data(struct net_buf *buf)
{
	int ret;

	/* If there is no data, then drop the packet. Also if
	 * the buffer is wrong type, then also drop the packet.
	 * The first buffer needs to have user data part that
	 * contains user data. The rest of the fragments should
	 * be data fragments without user data.
	 */
	if (!buf->frags || !buf->user_data_size) {
		NET_STATS(++net_stats.processing_error);
		return NET_DROP;
	}

	ret = net_if_recv_data(net_nbuf_iface(buf), buf);
	if (ret != NET_CONTINUE) {
		if (ret == NET_DROP) {
			NET_STATS(++net_stats.processing_error);
		}

		return ret;
	}

	/* IP version and header length. */
	switch (NET_IPV6_BUF(buf)->vtc & 0xf0) {
#if defined(CONFIG_NET_IPV6)
	case 0x60:
		NET_STATS(++net_stats.ipv6.recv);
		net_nbuf_family(buf) = PF_INET6;
		return process_ipv6_pkt(buf);
#endif
#if defined(CONFIG_NET_IPV4)
	case 0x40:
		NET_STATS(++net_stats.ipv4.recv);
		net_nbuf_family(buf) = PF_INET;
		return process_ipv4_pkt(buf);
#endif
	}

	NET_STATS(++net_stats.ip_errors.protoerr);
	NET_STATS(++net_stats.ip_errors.vhlerr);
	return NET_DROP;
}

static void net_rx_fiber(void)
{
	struct net_buf *buf;

	NET_DBG("Starting RX fiber (stack %d bytes)",
		sizeof(rx_fiber_stack));

	/* Starting TX side. The ordering is important here and the TX
	 * can only be started when RX side is ready to receive packets.
	 */
	net_if_init();

	while (1) {
		buf = nano_fifo_get(&rx_queue, TICKS_UNLIMITED);

		net_analyze_stack("RX fiber", rx_fiber_stack,
				  sizeof(rx_fiber_stack));

		NET_DBG("Received buf %p len %d", buf,
			net_buf_frags_len(buf));

		switch (process_data(buf)) {
		case NET_OK:
			NET_DBG("Consumed buf %p", buf);
			break;
		case NET_DROP:
		default:
			NET_DBG("Dropping buf %p", buf);
			net_nbuf_unref(buf);
			break;
		}

		net_print_statistics();
	}
}

static void init_rx_queue(void)
{
	nano_fifo_init(&rx_queue);

	rx_fiber_id = fiber_start(rx_fiber_stack, sizeof(rx_fiber_stack),
				  (nano_fiber_entry_t)net_rx_fiber,
				  0, 0, 8, 0);
}

/* Called when data needs to be sent to network */
int net_send_data(struct net_buf *buf)
{
	if (!buf || !buf->frags) {
		return -ENODATA;
	}

	if (!net_nbuf_iface(buf)) {
		return -EINVAL;
	}

#if defined(CONFIG_NET_STATISTICS)
	switch (net_nbuf_family(buf)) {
	case AF_INET:
		NET_STATS(++net_stats.ipv4.sent);
		break;
	case AF_INET6:
		NET_STATS(++net_stats.ipv6.sent);
		break;
	}
#endif

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

	NET_DBG("fifo %p iface %p buf %p len %d", &rx_queue, iface, buf,
		net_buf_frags_len(buf));

	net_nbuf_iface(buf) = iface;

	nano_fifo_put(&rx_queue, buf);

	fiber_wakeup(rx_fiber_id);

	return 0;
}

static inline void l3_init(void)
{
	net_icmpv6_init();

	NET_DBG("Network L3 init done");
}

static inline void l2_init(void)
{
	net_arp_init();

	NET_DBG("Network L2 init done");
}

static int net_init(struct device *unused)
{
	static bool is_initialized;

	if (is_initialized)
		return -EALREADY;

	is_initialized = true;

	NET_DBG("Priority %d", CONFIG_NET_INIT_PRIO);

	net_nbuf_init();

	net_context_init();

	l2_init();
	l3_init();

	init_rx_queue();

	return 0;
}

SYS_INIT(net_init, NANOKERNEL, CONFIG_NET_INIT_PRIO);
