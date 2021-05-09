/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_TC_LOG_LEVEL

#include <logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <linker/sections.h>
#include <random/rand32.h>

#include <ztest.h>

#include <net/ethernet.h>
#include <net/dummy.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_l2.h>
#include <net/udp.h>

#include "ipv6.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* make this large enough so that we do not overflow the sent pkt array */
#define MAX_PKT_TO_SEND 4
#define MAX_PKT_TO_RECV 4

#define MAX_PRIORITIES 8
#define MAX_TC 8

static enum net_priority send_priorities[MAX_TC][MAX_PKT_TO_SEND];
static enum net_priority recv_priorities[MAX_TC][MAX_PKT_TO_RECV];

static enum net_priority tx_tc2prio[NET_TC_TX_COUNT];
static enum net_priority rx_tc2prio[NET_TC_RX_COUNT];

#define TEST_PORT 9999

static const char *test_data = "Test data to be sent";

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 3 addresses */
static struct in6_addr my_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Destination address for test packets */
static struct in6_addr dst_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 9, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct sockaddr_in6 dst_addr6 = {
	.sin6_family = AF_INET6,
	.sin6_port = htons(TEST_PORT),
};

static struct {
	struct net_context *ctx;
} net_ctxs[NET_TC_COUNT];

static bool test_started;
static bool test_failed;
static bool start_receiving;
static bool recv_cb_called;
static struct k_sem wait_data;

#define WAIT_TIME K_SECONDS(1)

struct eth_context {
	struct net_if *iface;
	uint8_t mac_addr[6];

	uint16_t expecting_tag;
};

static struct eth_context eth_context;

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);
}

static bool check_higher_priority_pkt_sent(int tc, struct net_pkt *pkt)
{
	/* If we have sent any higher priority packets, then
	 * this test fails as those packets should have been
	 * sent before this one.
	 */
	int j, k;

	for (j = tc + 1; j < MAX_TC; j++) {
		for (k = 0; k < MAX_PKT_TO_SEND; k++) {
			if (send_priorities[j][k]) {
				return true;
			}
		}
	}

	return false;
}

static bool check_higher_priority_pkt_recv(int tc, struct net_pkt *pkt)
{
	/* If we have received any higher priority packets, then
	 * this test fails as those packets should have been
	 * received before this one.
	 */
	int j, k;

	for (j = tc + 1; j < MAX_TC; j++) {
		for (k = 0; k < MAX_PKT_TO_SEND; k++) {
			if (recv_priorities[j][k]) {
				return true;
			}
		}
	}

	return false;
}

/* The eth_tx() will handle both sent packets or and it will also
 * simulate the receiving of the packets.
 */
static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (start_receiving) {
		struct in6_addr addr;
		struct net_udp_hdr hdr, *udp_hdr;
		uint16_t port;

		DBG("Packet %p received\n", pkt);

		/* Swap IP src and destination address so that we can receive
		 * the packet and the stack will not reject it.
		 */
		net_ipaddr_copy(&addr, &NET_IPV6_HDR(pkt)->src);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
				&NET_IPV6_HDR(pkt)->dst);
		net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, &addr);

		udp_hdr = net_udp_get_hdr(pkt, &hdr);
		zassert_not_null(udp_hdr, "UDP header missing");

		port = udp_hdr->src_port;
		udp_hdr->src_port = udp_hdr->dst_port;
		udp_hdr->dst_port = port;

		if (net_recv_data(net_pkt_iface(pkt),
				  net_pkt_clone(pkt, K_NO_WAIT)) < 0) {
			test_failed = true;
			zassert_true(false, "Packet %p receive failed\n", pkt);
		}

		return 0;
	}

	if (test_started) {
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
		k_tid_t thread = k_current_get();
#endif
		int i, prio, ret;

		prio = net_pkt_priority(pkt);

		for (i = 0; i < MAX_PKT_TO_SEND; i++) {
			ret = check_higher_priority_pkt_sent(
				net_tx_priority2tc(prio), pkt);
			if (ret) {
				DBG("Current thread priority %d "
				    "pkt %p prio %d tc %d\n",
				    k_thread_priority_get(thread),
				    pkt, prio, net_tx_priority2tc(prio));

				test_failed = true;
				zassert_false(test_failed,
					      "Invalid priority sent %d TC %d,"
					      " expecting %d (pkt %p)\n",
					      prio,
					      net_tx_priority2tc(prio),
				  send_priorities[net_tx_priority2tc(prio)][i],
					      pkt);
				goto fail;
			}

			send_priorities[net_tx_priority2tc(prio)][i] = 0;
		}

		DBG("Received pkt %p from TC %c (thread prio %d)\n", pkt,
		    *(pkt->frags->data +
		      sizeof(struct net_ipv6_hdr) +
		      sizeof(struct net_udp_hdr)),
		    k_thread_priority_get(thread));

		k_sem_give(&wait_data);
	}

fail:
	return 0;
}

static struct dummy_api api_funcs = {
	.iface_api.init	= eth_iface_init,
	.send	= eth_tx,
};

static void generate_mac(uint8_t *mac_addr)
{
	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	mac_addr[0] = 0x00;
	mac_addr[1] = 0x00;
	mac_addr[2] = 0x5E;
	mac_addr[3] = 0x00;
	mac_addr[4] = 0x53;
	mac_addr[5] = sys_rand32_get();
}

static int eth_init(const struct device *dev)
{
	struct eth_context *context = dev->data;

	generate_mac(context->mac_addr);

	return 0;
}

/* Create one ethernet interface that does not have VLAN support. This
 * is quite unlikely that this would be done in real life but for testing
 * purposes create it here.
 */
NET_DEVICE_INIT(eth_test, "eth_test", eth_init, device_pm_control_nop,
		&eth_context, NULL, CONFIG_ETH_INIT_PRIORITY, &api_funcs,
		DUMMY_L2, NET_L2_GET_CTX_TYPE(DUMMY_L2),
		NET_ETH_MTU);

static void address_setup(void)
{
	struct net_if_addr *ifaddr;
	struct net_if *iface1;

	iface1 = net_if_get_default();

	zassert_not_null(iface1, "Interface 1");

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	/* For testing purposes we need to set the adddresses preferred */
	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &ll_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&ll_addr));
		zassert_not_null(ifaddr, "ll_addr");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr3,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr3));
		zassert_not_null(ifaddr, "addr3");
	}

	net_if_up(iface1);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;
}

static void priority_setup(void)
{
	int i;

	for (i = 0; i < MAX_PRIORITIES; i++) {
		tx_tc2prio[net_tx_priority2tc(i)] = i;
		rx_tc2prio[net_rx_priority2tc(i)] = i;
	}
}

#if defined(CONFIG_NET_IPV6_NBR_CACHE)
static bool add_neighbor(struct net_if *iface, struct in6_addr *addr)
{
	struct net_linkaddr_storage llstorage;
	struct net_linkaddr lladdr;
	struct net_nbr *nbr;

	llstorage.addr[0] = 0x01;
	llstorage.addr[1] = 0x02;
	llstorage.addr[2] = 0x33;
	llstorage.addr[3] = 0x44;
	llstorage.addr[4] = 0x05;
	llstorage.addr[5] = 0x06;

	lladdr.len = 6U;
	lladdr.addr = llstorage.addr;
	lladdr.type = NET_LINK_ETHERNET;

	nbr = net_ipv6_nbr_add(iface, addr, &lladdr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	if (!nbr) {
		DBG("Cannot add dst %s to neighbor cache\n",
		    net_sprint_ipv6_addr(addr));
		return false;
	}

	return true;
}
#else
#define add_neighbor(iface, addr) true
#endif /* CONFIG_NET_IPV6_NBR_CACHE */

static void setup_net_context(struct net_context **ctx)
{
	struct sockaddr_in6 src_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
	};
	int ret;
	struct net_if *iface1;

	iface1 = net_if_get_default();

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP, ctx);
	zassert_equal(ret, 0, "Create IPv6 UDP context %p failed (%d)\n",
		      *ctx, ret);

	memcpy(&src_addr6.sin6_addr, &my_addr1, sizeof(struct in6_addr));
	memcpy(&dst_addr6.sin6_addr, &dst_addr, sizeof(struct in6_addr));

	ret = add_neighbor(iface1, &dst_addr);
	zassert_true(ret, "Cannot add neighbor");

	ret = net_context_bind(*ctx, (struct sockaddr *)&src_addr6,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0,
		      "Context bind failure test failed (%d)\n", ret);
}

static void test_traffic_class_general_setup(void)
{
	address_setup();
	priority_setup();
}

static void traffic_class_setup(enum net_priority *tc2prio, int count)
{
	uint8_t priority;
	int i, ret;

	for (i = 0; i < count; i++) {
		setup_net_context(&net_ctxs[i].ctx);

		priority = tc2prio[i];

		ret = net_context_set_option(net_ctxs[i].ctx,
					     NET_OPT_PRIORITY,
					     &priority, sizeof(priority));
		zassert_equal(ret, 0,
			      "Cannot set priority %d to ctx %p (%d)\n",
			      priority, net_ctxs[i].ctx, ret);
	}
}

static void test_traffic_class_setup_tx(void)
{
	traffic_class_setup(tx_tc2prio, NET_TC_TX_COUNT);
}

static void test_traffic_class_setup_rx(void)
{
	traffic_class_setup(rx_tc2prio, NET_TC_RX_COUNT);
}

static void traffic_class_cleanup(int count)
{
	int i;

	for (i = 0; i < count; i++) {
		if (net_ctxs[i].ctx) {
			net_context_unref(net_ctxs[i].ctx);
			net_ctxs[i].ctx = NULL;
		}
	}
}

static void test_traffic_class_cleanup_tx(void)
{
	traffic_class_cleanup(NET_TC_TX_COUNT);
}

static void test_traffic_class_cleanup_rx(void)
{
	traffic_class_cleanup(NET_TC_RX_COUNT);
}

static void traffic_class_send_packets_with_prio(enum net_priority prio,
						 int pkt_count)
{
	/* Start to send data to each queue and verify that the data
	 * is received in correct order.
	 */
	uint8_t data[128];
	int len, ret;
	int tc = net_tx_priority2tc(prio);

	/* Convert num to ascii */
	data[0] = tc + 0x30;
	len = strlen(test_data);
	memcpy(data+1, test_data, strlen(test_data));

	len += 1;

	test_started = true;

	DBG("Sending on TC %d priority %d\n", tc, prio);

	send_priorities[net_tx_priority2tc(prio)][pkt_count - 1] = prio + 1;

	ret = net_context_sendto(net_ctxs[tc].ctx, data, len,
				 (struct sockaddr *)&dst_addr6,
				 sizeof(struct sockaddr_in6),
				 NULL, K_NO_WAIT, NULL);
	zassert_true(ret > 0, "Send UDP pkt failed");
}

static void traffic_class_send_priority(enum net_priority prio,
					int num_packets,
					bool wait_for_packets)
{
	int i;

	if (wait_for_packets) {
		k_sem_init(&wait_data, MAX_PKT_TO_SEND, UINT_MAX);
	}

	for (i = 0; i < num_packets; i++) {
		traffic_class_send_packets_with_prio(prio, i + 1);
	}

	if (wait_for_packets) {
		if (k_sem_take(&wait_data, WAIT_TIME)) {
			DBG("Timeout while waiting ok status\n");
			zassert_false(true, "Timeout");
		}

		/* This sleep is needed here so that the sending side
		 * can run properly.
		 */
		k_sleep(K_MSEC(1));
	}
}

static void test_traffic_class_send_data_prio_bk(void)
{
	/* Send number of packets with each priority and make sure
	 * they are sent properly.
	 */
	traffic_class_send_priority(NET_PRIORITY_BK, MAX_PKT_TO_SEND, true);
}

static void test_traffic_class_send_data_prio_be(void)
{
	traffic_class_send_priority(NET_PRIORITY_BE, MAX_PKT_TO_SEND, true);
}

static void test_traffic_class_send_data_prio_ee(void)
{
	traffic_class_send_priority(NET_PRIORITY_EE, MAX_PKT_TO_SEND, true);
}

static void test_traffic_class_send_data_prio_ca(void)
{
	traffic_class_send_priority(NET_PRIORITY_CA, MAX_PKT_TO_SEND, true);
}

static void test_traffic_class_send_data_prio_vi(void)
{
	traffic_class_send_priority(NET_PRIORITY_VI, MAX_PKT_TO_SEND, true);
}

static void test_traffic_class_send_data_prio_vo(void)
{
	traffic_class_send_priority(NET_PRIORITY_VO, MAX_PKT_TO_SEND, true);
}

static void test_traffic_class_send_data_prio_ic(void)
{
	traffic_class_send_priority(NET_PRIORITY_IC, MAX_PKT_TO_SEND, true);
}

static void test_traffic_class_send_data_prio_nc(void)
{
	traffic_class_send_priority(NET_PRIORITY_NC, MAX_PKT_TO_SEND, true);
}

static void test_traffic_class_send_data_mix(void)
{
	/* Start to send data to each queue and verify that the data
	 * is received in correct order.
	 */
	int total_packets = 0;

	(void)memset(send_priorities, 0, sizeof(send_priorities));

	traffic_class_send_priority(NET_PRIORITY_BK, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	traffic_class_send_priority(NET_PRIORITY_BE, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	/* The semaphore is released as many times as we have sent packets */
	k_sem_init(&wait_data, total_packets, UINT_MAX);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting ok status\n");
		zassert_false(true, "Timeout");
	}

	zassert_false(test_failed, "Traffic class verification failed.");
}

static void test_traffic_class_send_data_mix_all_1(void)
{
	int total_packets = 0;

	(void)memset(send_priorities, 0, sizeof(send_priorities));

	traffic_class_send_priority(NET_PRIORITY_BK, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	traffic_class_send_priority(NET_PRIORITY_BE, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	traffic_class_send_priority(NET_PRIORITY_EE, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	traffic_class_send_priority(NET_PRIORITY_CA, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	traffic_class_send_priority(NET_PRIORITY_VI, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	traffic_class_send_priority(NET_PRIORITY_VO, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	traffic_class_send_priority(NET_PRIORITY_IC, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	traffic_class_send_priority(NET_PRIORITY_NC, MAX_PKT_TO_SEND, false);
	total_packets += MAX_PKT_TO_SEND;

	/* The semaphore is released as many times as we have sent packets */
	k_sem_init(&wait_data, total_packets, UINT_MAX);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting ok status\n");
		zassert_false(true, "Timeout");
	}

	zassert_false(test_failed, "Traffic class verification failed.");
}

static void test_traffic_class_send_data_mix_all_2(void)
{
	/* Start to send data to each queue and verify that the data
	 * is received in correct order.
	 */
	int total_packets = 0;
	int i;

	(void)memset(send_priorities, 0, sizeof(send_priorities));

	/* In this test send one packet for each queue instead of sending
	 * n packets to same queue at a time.
	 */
	for (i = 0; i < MAX_PKT_TO_SEND; i++) {
		traffic_class_send_priority(NET_PRIORITY_BK, 1, false);
		total_packets += 1;

		traffic_class_send_priority(NET_PRIORITY_BE, 1, false);
		total_packets += 1;

		traffic_class_send_priority(NET_PRIORITY_EE, 1, false);
		total_packets += 1;

		traffic_class_send_priority(NET_PRIORITY_CA, 1, false);
		total_packets += 1;

		traffic_class_send_priority(NET_PRIORITY_VI, 1, false);
		total_packets += 1;

		traffic_class_send_priority(NET_PRIORITY_VO, 1, false);
		total_packets += 1;

		traffic_class_send_priority(NET_PRIORITY_IC, 1, false);
		total_packets += 1;

		traffic_class_send_priority(NET_PRIORITY_NC, 1, false);
		total_packets += 1;
	}

	/* The semaphore is released as many times as we have sent packets */
	k_sem_init(&wait_data, total_packets, UINT_MAX);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting ok status\n");
		zassert_false(true, "Timeout");
	}

	zassert_false(test_failed, "Traffic class verification failed.");
}

static void recv_cb(struct net_context *context,
		    struct net_pkt *pkt,
		    union net_ip_header *ip_hdr,
		    union net_proto_header *proto_hdr,
		    int status,
		    void *user_data)
{
#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
	k_tid_t thread = k_current_get();
#endif
	int i, prio, ret;

	DBG("Data received in priority %d\n", k_thread_priority_get(thread));

	prio = net_pkt_priority(pkt);

	for (i = 0; i < MAX_PKT_TO_RECV; i++) {
		ret = check_higher_priority_pkt_recv(net_rx_priority2tc(prio),
						     pkt);
		if (ret) {
			DBG("Current thread priority %d "
			    "pkt %p prio %d tc %d\n",
			    k_thread_priority_get(thread),
			    pkt, prio, net_rx_priority2tc(prio));

			test_failed = true;
			zassert_false(test_failed,
				      "Invalid priority received %d TC %d,"
				      " expecting %d (pkt %p)\n",
				      prio,
				      net_rx_priority2tc(prio),
				  recv_priorities[net_rx_priority2tc(prio)][i],
				      pkt);
			goto fail;
		}

		recv_priorities[net_rx_priority2tc(prio)][i] = 0;
	}

fail:
	recv_cb_called = true;
	k_sem_give(&wait_data);

	net_pkt_unref(pkt);
}

static void test_traffic_class_setup_recv(void)
{
	int ret, i;

	recv_cb_called = false;

	for (i = 0; i < NET_TC_RX_COUNT; i++) {
		ret = net_context_recv(net_ctxs[i].ctx, recv_cb,
				       K_NO_WAIT, NULL);
		zassert_equal(ret, 0,
			      "[%d] Context recv UDP setup failed (%d)\n",
			      i, ret);
	}
}

static void traffic_class_recv_packets_with_prio(enum net_priority prio,
						 int pkt_count)
{
	/* Start to receive data to each queue and verify that the data
	 * is received in correct order.
	 */
	uint8_t data[128];
	int len, ret;
	int tc = net_rx_priority2tc(prio);
	const struct in6_addr *src_addr;
	struct net_if_addr *ifaddr;
	struct net_if *iface = NULL;

	/* Convert num to ascii */
	data[0] = tc + 0x30;
	len = strlen(test_data);
	memcpy(data+1, test_data, strlen(test_data));

	len += 1;

	test_started = true;
	start_receiving = true;

	DBG("Receiving on TC %d priority %d\n", tc, prio);

	recv_priorities[net_rx_priority2tc(prio)][pkt_count - 1] = prio + 1;

	src_addr = net_if_ipv6_select_src_addr(NULL, &dst_addr);
	zassert_not_null(src_addr, "Cannot select source address");

	ifaddr = net_if_ipv6_addr_lookup(src_addr, &iface);
	zassert_not_null(ifaddr, "Cannot find source address");
	zassert_not_null(iface, "Interface not found");

	/* We cannot use net_recv_data() here as the packet does not have
	 * UDP header.
	 */
	ret = net_context_sendto(net_ctxs[tc].ctx, data, len,
				 (struct sockaddr *)&dst_addr6,
				 sizeof(struct sockaddr_in6),
				 NULL, K_NO_WAIT, NULL);
	zassert_true(ret > 0, "Send UDP pkt failed");

	/* Let the receiver to receive the packets */
	k_sleep(K_MSEC(1));
}

static void traffic_class_recv_priority(enum net_priority prio,
					int num_packets,
					bool wait_for_packets)
{
	int i;

	if (wait_for_packets) {
		k_sem_init(&wait_data, MAX_PKT_TO_RECV, UINT_MAX);
	}

	for (i = 0; i < num_packets; i++) {
		traffic_class_recv_packets_with_prio(prio, i + 1);
	}

	if (wait_for_packets) {
		if (k_sem_take(&wait_data, WAIT_TIME)) {
			DBG("Timeout while waiting ok status\n");
			zassert_false(true, "Timeout");
		}

		/* This sleep is needed here so that the receiving side
		 * can run properly.
		 */
		k_sleep(K_MSEC(1));
	}
}

static void test_traffic_class_recv_data_prio_bk(void)
{
	/* Receive number of packets with each priority and make sure
	 * they are received properly.
	 */
	traffic_class_recv_priority(NET_PRIORITY_BK, MAX_PKT_TO_RECV, true);

	zassert_false(test_failed, "Traffic class verification failed.");
}

static void test_traffic_class_recv_data_prio_be(void)
{
	traffic_class_recv_priority(NET_PRIORITY_BE, MAX_PKT_TO_RECV, true);
}

static void test_traffic_class_recv_data_prio_ee(void)
{
	traffic_class_recv_priority(NET_PRIORITY_EE, MAX_PKT_TO_RECV, true);
}

static void test_traffic_class_recv_data_prio_ca(void)
{
	traffic_class_recv_priority(NET_PRIORITY_CA, MAX_PKT_TO_RECV, true);
}

static void test_traffic_class_recv_data_prio_vi(void)
{
	traffic_class_recv_priority(NET_PRIORITY_VI, MAX_PKT_TO_RECV, true);
}

static void test_traffic_class_recv_data_prio_vo(void)
{
	traffic_class_recv_priority(NET_PRIORITY_VO, MAX_PKT_TO_RECV, true);
}

static void test_traffic_class_recv_data_prio_ic(void)
{
	traffic_class_recv_priority(NET_PRIORITY_IC, MAX_PKT_TO_RECV, true);
}

static void test_traffic_class_recv_data_prio_nc(void)
{
	traffic_class_recv_priority(NET_PRIORITY_NC, MAX_PKT_TO_RECV, true);
}

static void test_traffic_class_recv_data_mix(void)
{
	/* Start to receive data to each queue and verify that the data
	 * is received in correct order.
	 */
	int total_packets = 0;

	(void)memset(recv_priorities, 0, sizeof(recv_priorities));

	traffic_class_recv_priority(NET_PRIORITY_BK, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	traffic_class_recv_priority(NET_PRIORITY_BE, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	/* The semaphore is released as many times as we have sent packets */
	k_sem_init(&wait_data, total_packets, UINT_MAX);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting ok status\n");
		zassert_false(true, "Timeout");
	}

	zassert_false(test_failed, "Traffic class verification failed.");
}

static void test_traffic_class_recv_data_mix_all_1(void)
{
	int total_packets = 0;

	(void)memset(recv_priorities, 0, sizeof(recv_priorities));

	traffic_class_recv_priority(NET_PRIORITY_BK, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	traffic_class_recv_priority(NET_PRIORITY_BE, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	traffic_class_recv_priority(NET_PRIORITY_EE, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	traffic_class_recv_priority(NET_PRIORITY_CA, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	traffic_class_recv_priority(NET_PRIORITY_VI, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	traffic_class_recv_priority(NET_PRIORITY_VO, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	traffic_class_recv_priority(NET_PRIORITY_IC, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	traffic_class_recv_priority(NET_PRIORITY_NC, MAX_PKT_TO_RECV, false);
	total_packets += MAX_PKT_TO_RECV;

	/* The semaphore is released as many times as we have sent packets */
	k_sem_init(&wait_data, total_packets, UINT_MAX);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting ok status\n");
		zassert_false(true, "Timeout");
	}

	zassert_false(test_failed, "Traffic class verification failed.");
}

static void test_traffic_class_recv_data_mix_all_2(void)
{
	/* Start to receive data to each queue and verify that the data
	 * is received in correct order.
	 */
	int total_packets = 0;
	int i;

	(void)memset(recv_priorities, 0, sizeof(recv_priorities));

	/* In this test receive one packet for each queue instead of receiving
	 * n packets to same queue at a time.
	 */
	for (i = 0; i < MAX_PKT_TO_RECV; i++) {
		traffic_class_recv_priority(NET_PRIORITY_BK, 1, false);
		total_packets += 1;

		traffic_class_recv_priority(NET_PRIORITY_BE, 1, false);
		total_packets += 1;

		traffic_class_recv_priority(NET_PRIORITY_EE, 1, false);
		total_packets += 1;

		traffic_class_recv_priority(NET_PRIORITY_CA, 1, false);
		total_packets += 1;

		traffic_class_recv_priority(NET_PRIORITY_VI, 1, false);
		total_packets += 1;

		traffic_class_recv_priority(NET_PRIORITY_VO, 1, false);
		total_packets += 1;

		traffic_class_recv_priority(NET_PRIORITY_IC, 1, false);
		total_packets += 1;

		traffic_class_recv_priority(NET_PRIORITY_NC, 1, false);
		total_packets += 1;
	}

	/* The semaphore is released as many times as we have sent packets */
	k_sem_init(&wait_data, total_packets, UINT_MAX);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting ok status\n");
		zassert_false(true, "Timeout");
	}

	zassert_false(test_failed, "Traffic class verification failed.");
}

void test_main(void)
{
	ztest_test_suite(net_traffic_class_test,
			 ztest_unit_test(test_traffic_class_general_setup),
			 ztest_unit_test(test_traffic_class_setup_tx),
			 /* Send only same priority packets and verify that
			  * all are sent with proper traffic class.
			  */
			 ztest_unit_test(test_traffic_class_send_data_prio_bk),
			 ztest_unit_test(test_traffic_class_send_data_prio_be),
			 ztest_unit_test(test_traffic_class_send_data_prio_ee),
			 ztest_unit_test(test_traffic_class_send_data_prio_ca),
			 ztest_unit_test(test_traffic_class_send_data_prio_vi),
			 ztest_unit_test(test_traffic_class_send_data_prio_vo),
			 ztest_unit_test(test_traffic_class_send_data_prio_ic),
			 ztest_unit_test(test_traffic_class_send_data_prio_nc),
			 /* Then mix traffic classes and verify that higher
			  * class packets are sent first.
			  */
			 ztest_unit_test(test_traffic_class_send_data_mix),
			 ztest_unit_test(test_traffic_class_send_data_mix_all_1),
			 ztest_unit_test(test_traffic_class_send_data_mix_all_2),
			 ztest_unit_test(test_traffic_class_cleanup_tx),

			 /* Same tests for received packets */
			 ztest_unit_test(test_traffic_class_setup_rx),
			 ztest_unit_test(test_traffic_class_setup_recv),
			 ztest_unit_test(test_traffic_class_recv_data_prio_bk),
			 ztest_unit_test(test_traffic_class_recv_data_prio_be),
			 ztest_unit_test(test_traffic_class_recv_data_prio_ee),
			 ztest_unit_test(test_traffic_class_recv_data_prio_ca),
			 ztest_unit_test(test_traffic_class_recv_data_prio_vi),
			 ztest_unit_test(test_traffic_class_recv_data_prio_vo),
			 ztest_unit_test(test_traffic_class_recv_data_prio_ic),
			 ztest_unit_test(test_traffic_class_recv_data_prio_nc),
			 ztest_unit_test(test_traffic_class_recv_data_mix),
			 ztest_unit_test(test_traffic_class_recv_data_mix_all_1),
			 ztest_unit_test(test_traffic_class_recv_data_mix_all_2),
			 ztest_unit_test(test_traffic_class_cleanup_rx)
			 );

	ztest_run_test_suite(net_traffic_class_test);
}
