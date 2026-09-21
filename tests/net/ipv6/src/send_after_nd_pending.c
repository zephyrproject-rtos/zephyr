/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Кирило Шипачов
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * net_if_try_send_data() must not touch a packet after
 * net_ipv6_prepare_for_send() returned NET_CONTINUE.
 *
 * NET_CONTINUE means the packet was handed to the IPv6 neighbor discovery
 * pending queue. From then on it belongs to the stack: the neighbor
 * advertisement can arrive, the packet can be transmitted and freed, and its
 * memory block can be given to another packet, all before the thread that
 * called sendto() runs again. The defect: net_if_try_send_data() reads
 * net_pkt_family(pkt) a second time after that call. If the block now holds an
 * IPv4 packet, net_ipv4_prepare_for_send() replaces the verdict with NET_OK
 * and the stale pointer is queued for transmission.
 *
 * The test forces exactly that order of events with thread priorities and
 * semaphores, no timing involved:
 *
 *   sender (preemptible, lowest)   sendto() IPv6 to an unknown neighbor
 *                                  -> NS transmitted, and the handler below
 *                                     wakes the test thread, so the sender
 *                                     stays inside net_ipv6_send_ns()
 *   test thread (cooperative)      injects the NA
 *   RX thread                      sends the pending packet P, transmits and
 *                                  frees it
 *   test thread                    sends an IPv4 packet: it gets P's block
 *   sender                         resumes after net_ipv6_prepare_for_send()
 */

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/socket.h>

#include "ipv6.h"

#include "ipv6_test.h"

/* The defect needs an IPv4 packet to land in the released block: the second,
 * stale, read of the family in net_if_try_send_data() is compiled out without
 * IPv4, and so are the address APIs below.
 */
#if defined(CONFIG_NET_IPV4)

#define STEP_TIMEOUT K_SECONDS(2)

#define ETH_HDR_LEN  14
#define IPV6_HDR_LEN 40
#define ICMPV6_NS    135
#define ICMPV6_NA    136
#define NBR_PORT     5540
#define MCAST_PORT   5353

static const uint8_t peer_mac[6] = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x02 };

/* A neighbor that is in neither the cache nor any other test of this suite,
 * so that sending to it starts neighbor discovery.
 */
static const struct net_in6_addr nbr6 = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
					      0, 0, 0x5e, 0xff, 0xfe, 0, 0x53, 0x02 } } };
static const struct net_in_addr my4 = { { { 192, 0, 2, 1 } } };
static const struct net_in_addr mask4 = { { { 255, 255, 255, 0 } } };
static const struct net_in_addr mcast4 = { { { 224, 0, 0, 251 } } };

static struct net_if *test_iface;

static K_SEM_DEFINE(ns_sent, 0, 1);
static K_SEM_DEFINE(pending_sent, 0, 1);
static K_SEM_DEFINE(v4_sent, 0, 1);
static K_SEM_DEFINE(sender_done, 0, 1);

/* What the driver saw. Written by the handler below, on whichever thread
 * transmits, and read by the test thread only after the matching semaphore.
 */
static uint8_t ns_src[NET_IPV6_ADDR_SIZE];
static struct net_pkt *pending_pkt;
static struct net_pkt *v4_pkt;
static int v4_pkt_sends;
static int sends_after_ns;

static int sock6 = -1;
static int sock4 = -1;
static ssize_t sender_ret;
static int sender_errno;

static bool tx_seen(struct net_pkt *pkt, void *user_data)
{
	uint8_t frame[ETH_HDR_LEN + IPV6_HDR_LEN + 24];
	size_t len = MIN(net_pkt_get_len(pkt), sizeof(frame));
	uint16_t ethertype;

	ARG_UNUSED(user_data);

	/* The IPv4 packet is recognised by its pointer: a second transmission
	 * of it is the defect, and then its content is not a valid frame.
	 */
	if (v4_pkt != NULL && pkt == v4_pkt) {
		v4_pkt_sends++;
		sends_after_ns++;
		return true;
	}

	memset(frame, 0, sizeof(frame));
	net_pkt_cursor_init(pkt);
	(void)net_pkt_read(pkt, frame, len);

	ethertype = (frame[12] << 8) | frame[13];

	if (ethertype == NET_ETH_PTYPE_IPV6) {
		const uint8_t *ip = &frame[ETH_HDR_LEN];
		const uint8_t *l4 = &ip[IPV6_HDR_LEN];

		if (ip[6] == NET_IPPROTO_ICMPV6 && l4[0] == ICMPV6_NS &&
		    memcmp(&l4[8], &nbr6, sizeof(nbr6)) == 0) {
			memcpy(ns_src, &ip[8], sizeof(ns_src));
			k_sem_give(&ns_sent);
		} else if (ip[6] == NET_IPPROTO_UDP &&
			   memcmp(&ip[24], &nbr6, sizeof(nbr6)) == 0) {
			pending_pkt = pkt;
			sends_after_ns++;
			k_sem_give(&pending_sent);
		}
	} else if (ethertype == NET_ETH_PTYPE_IP && v4_pkt == NULL &&
		   frame[ETH_HDR_LEN + 9] == NET_IPPROTO_UDP) {
		v4_pkt = pkt;
		v4_pkt_sends = 1;
		sends_after_ns++;

		/* One reference survives the regular transmission, the other
		 * one a second, wrong, transmission.
		 */
		net_pkt_ref(pkt);
		net_pkt_ref(pkt);

		k_sem_give(&v4_sent);
	}

	return true;
}

static struct test_tx_handler handler = {
	.fn = tx_seen,
};

static uint16_t icmpv6_checksum(const uint8_t *src, const uint8_t *dst,
				const uint8_t *icmp, size_t len)
{
	uint32_t sum = len + NET_IPPROTO_ICMPV6;

	for (size_t i = 0; i < NET_IPV6_ADDR_SIZE; i += 2) {
		sum += (src[i] << 8) | src[i + 1];
		sum += (dst[i] << 8) | dst[i + 1];
	}

	for (size_t i = 0; i < len; i += 2) {
		sum += (icmp[i] << 8) | icmp[i + 1];
	}

	while (sum >> 16) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return ~sum & 0xffff;
}

/* A solicited neighbor advertisement from the neighbor, as it arrives from
 * the wire.
 */
static void inject_na(void)
{
	uint8_t frame[ETH_HDR_LEN + IPV6_HDR_LEN + 32] = { 0 };
	uint8_t *ip = &frame[ETH_HDR_LEN];
	uint8_t *icmp = &ip[IPV6_HDR_LEN];
	struct net_pkt *pkt;
	uint16_t sum;

	memcpy(&frame[0], net_if_get_link_addr(test_iface)->addr, 6);
	memcpy(&frame[6], peer_mac, 6);
	frame[12] = NET_ETH_PTYPE_IPV6 >> 8;
	frame[13] = NET_ETH_PTYPE_IPV6 & 0xff;

	ip[0] = 0x60;
	ip[5] = 32;                      /* payload length */
	ip[6] = NET_IPPROTO_ICMPV6;
	ip[7] = 255;                     /* hop limit, checked by the receiver */
	memcpy(&ip[8], &nbr6, 16);
	memcpy(&ip[24], ns_src, 16);

	icmp[0] = ICMPV6_NA;
	icmp[4] = 0x60;                  /* solicited, override */
	memcpy(&icmp[8], &nbr6, 16);     /* target */
	icmp[24] = 2;                    /* target link-layer address option */
	icmp[25] = 1;
	memcpy(&icmp[26], peer_mac, 6);

	sum = icmpv6_checksum(&ip[8], &ip[24], icmp, 32);
	icmp[2] = sum >> 8;
	icmp[3] = sum & 0xff;

	pkt = net_pkt_rx_alloc_with_buffer(test_iface, sizeof(frame), NET_AF_UNSPEC, 0,
					   K_NO_WAIT);
	zassert_not_null(pkt, "no RX packet for the NA");
	zassert_ok(net_pkt_write(pkt, frame, sizeof(frame)));
	zassert_ok(net_recv_data(test_iface, pkt), "NA not accepted");
}

static void sender_fn(void *p1, void *p2, void *p3)
{
	static const uint8_t payload[98] = { 0 };
	struct net_sockaddr_in6 dst = {
		.sin6_family = NET_AF_INET6,
		.sin6_port = net_htons(NBR_PORT),
		.sin6_scope_id = net_if_get_by_iface(test_iface),
	};

	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	net_ipaddr_copy(&dst.sin6_addr, &nbr6);

	sender_ret = zsock_sendto(sock6, payload, sizeof(payload), 0,
				  (struct net_sockaddr *)&dst, sizeof(dst));
	sender_errno = errno;
	k_sem_give(&sender_done);
}

K_THREAD_STACK_DEFINE(sender_stack, 4096);
static struct k_thread sender_thread;

ZTEST(net_ipv6, test_send_after_nd_pending)
{
	static const uint8_t payload[32] = { 0 };
	struct net_sockaddr_in dst4 = {
		.sin_family = NET_AF_INET,
		.sin_port = net_htons(MCAST_PORT),
	};
	struct net_if_addr *ifaddr;
	struct net_ifreq ifreq = { 0 };
	char ifname[CONFIG_NET_INTERFACE_NAME_LEN + 1];

	net_ipaddr_copy(&dst4.sin_addr, &mcast4);

	/* Only this scenario of the suite builds IPv4 in, so the address the
	 * reusing packet is sent from is added here and removed again below.
	 */
	test_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(ETHERNET));
	zassert_not_null(test_iface);

	ifaddr = net_if_ipv4_addr_add(test_iface, (struct net_in_addr *)&my4,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "cannot add the IPv4 address");
	net_if_ipv4_set_netmask_by_addr(test_iface, &my4, &mask4);

	/* Other tests of this suite populate the neighbor cache, so make sure
	 * this neighbor really has to be discovered.
	 */
	net_ipv6_nbr_rm(test_iface, (struct net_in6_addr *)&nbr6);

	/* Those tests also take the interface down and up, which restarts
	 * duplicate address detection. A tentative address cannot be used as a
	 * source, and the packet would be dropped before neighbor discovery.
	 */
	for (int i = 0; i < 100; i++) {
		if (net_if_ipv6_get_ll(test_iface, NET_ADDR_PREFERRED) != NULL) {
			break;
		}

		k_sleep(K_MSEC(20));
	}

	zassert_not_null(net_if_ipv6_get_ll(test_iface, NET_ADDR_PREFERRED),
			 "no link-local address to send from");

	sock6 = zsock_socket(NET_AF_INET6, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	sock4 = zsock_socket(NET_AF_INET, NET_SOCK_DGRAM, NET_IPPROTO_UDP);
	zassert_true(sock6 >= 0 && sock4 >= 0, "cannot create the sockets");

	/* This suite has a second, dummy, interface that source selection can
	 * pick instead, and then nothing reaches the handler above.
	 */
	zassert_true(net_if_get_name(test_iface, ifname, sizeof(ifname)) > 0,
		     "cannot get the interface name");
	strncpy(ifreq.ifr_name, ifname, sizeof(ifreq.ifr_name) - 1);
	zassert_ok(zsock_setsockopt(sock6, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE,
				    &ifreq, sizeof(ifreq)),
		   "cannot bind the IPv6 socket to the interface");
	zassert_ok(zsock_setsockopt(sock4, ZSOCK_SOL_SOCKET, ZSOCK_SO_BINDTODEVICE,
				    &ifreq, sizeof(ifreq)),
		   "cannot bind the IPv4 socket to the interface");

	/* From here on the driver reports to this test instead of answering
	 * ICMPv6, so that nothing else allocates from the packet slab.
	 */
	tx_handler = &handler;

	/* 1. The sender, preemptible and below every other thread, sends to a
	 *    neighbor that is not in the cache. The handler above sees the NS
	 *    and wakes this thread, which is cooperative and runs next, so the
	 *    sender stays inside net_ipv6_send_ns() holding the nbr lock.
	 */
	k_thread_create(&sender_thread, sender_stack, K_THREAD_STACK_SIZEOF(sender_stack),
			sender_fn, NULL, NULL, NULL, K_PRIO_PREEMPT(10), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&ns_sent, STEP_TIMEOUT),
		   "no NS for the neighbor (sender done %d, ret %d, errno %d)",
		   k_sem_count_get(&sender_done), (int)sender_ret, sender_errno);
	zassert_equal(k_sem_count_get(&sender_done), 0, "sender finished before the NA");

	/* 2. The NA. The RX thread blocks on the neighbor lock the sender holds,
	 *    the sender runs up to the unlock, then the RX thread sends the
	 *    pending packet, transmits it and frees it.
	 */
	inject_na();
	zassert_ok(k_sem_take(&pending_sent, STEP_TIMEOUT), "pending packet not sent");
	zassert_equal(k_sem_count_get(&sender_done), 0, "sender finished too early");

	/* 3. An IPv4 packet. The packet slab is LIFO: it gets the block the
	 *    pending packet has just released.
	 */
	zassert_equal(zsock_sendto(sock4, payload, sizeof(payload), 0,
				   (struct net_sockaddr *)&dst4, sizeof(dst4)),
		      sizeof(payload));
	zassert_ok(k_sem_take(&v4_sent, STEP_TIMEOUT), "IPv4 packet not sent");
	zassert_equal_ptr(v4_pkt, pending_pkt,
			  "precondition: the IPv4 packet did not reuse the block");
	zassert_equal(k_sem_count_get(&sender_done), 0, "sender finished too early");

	/* 4. Only now the sender gets to return from net_ipv6_prepare_for_send(). */
	zassert_ok(k_sem_take(&sender_done, STEP_TIMEOUT), "sender did not finish");
	k_thread_join(&sender_thread, K_FOREVER);
	/* On a stack with the defect the sender has just queued the stale
	 * pointer: give the interface time to transmit it.
	 */
	k_sleep(K_MSEC(100));

	zassert_equal(sender_ret, 98, "sendto() failed: %d", (int)sender_ret);
	zassert_equal(v4_pkt_sends, 1,
		      "the IPv6 sender queued a packet it no longer owned: "
		      "the IPv4 packet in its memory block was transmitted %d times",
		      v4_pkt_sends);
	zassert_equal(sends_after_ns, 2, "unexpected transmissions: %d", sends_after_ns);

	/* The suite has more tests after this one: give back the references
	 * held above, the neighbor discovery created and the IPv4 address.
	 */
	for (int i = 2 - (v4_pkt_sends - 1); i > 0; i--) {
		net_pkt_unref(v4_pkt);
	}

	tx_handler = NULL;
	(void)zsock_close(sock6);
	(void)zsock_close(sock4);
	net_ipv6_nbr_rm(test_iface, (struct net_in6_addr *)&nbr6);
	zassert_true(net_if_ipv4_addr_rm(test_iface, &my4),
		     "cannot remove the IPv4 address");
}

#else /* CONFIG_NET_IPV4 */

ZTEST(net_ipv6, test_send_after_nd_pending)
{
	ztest_test_skip();
}

#endif /* CONFIG_NET_IPV4 */
