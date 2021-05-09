/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_ETHERNET_LOG_LEVEL

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
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_l2.h>
#include <net/udp.h>

#include "ipv6.h"
#include "udp_internal.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define TEST_PORT 9999

static char *test_data = "Test data to be sent";

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Destination address for test packets */
static struct in6_addr dst_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x2 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct in_addr in4addr_my = { { { 192, 0, 2, 1 } } };
static struct in_addr in4addr_dst = { { { 192, 168, 1, 1 } } };
static struct in_addr in4addr_my2 = { { { 192, 0, 42, 1 } } };

/* Keep track of all ethernet interfaces. For native_posix board, we need
 * to increase the count as it has one extra network interface defined in
 * eth_native_posix driver.
 */
static struct net_if *eth_interfaces[2 + IS_ENABLED(CONFIG_ETH_NATIVE_POSIX)];

static struct net_context *udp_v6_ctx_1;
static struct net_context *udp_v6_ctx_2;
static struct net_context *udp_v4_ctx_1;
static struct net_context *udp_v4_ctx_2;

static bool test_failed;
static bool test_started;
static bool start_receiving;

static K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define WAIT_TIME K_SECONDS(1)

struct eth_context {
	struct net_if *iface;
	uint8_t mac_addr[6];

	uint16_t expecting_tag;
};

static struct eth_context eth_context_offloading_disabled;
static struct eth_context eth_context_offloading_enabled;

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	DBG("Iface %p addr %s\n", iface,
	    net_sprint_ll_addr(context->mac_addr, sizeof(context->mac_addr)));

	ethernet_init(iface);
}

static uint16_t get_udp_chksum(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(udp_access, struct net_udp_hdr);
	struct net_udp_hdr *udp_hdr;
	struct net_pkt_cursor backup;

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	/* Let's move the cursor to UDP header */
	if (net_pkt_skip(pkt, sizeof(struct net_eth_hdr) +
			 net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ipv6_ext_len(pkt))) {
		return 0;
	}

	udp_hdr = (struct net_udp_hdr *)net_pkt_get_data(pkt, &udp_access);
	if (!udp_hdr) {
		return 0;
	}

	net_pkt_cursor_restore(pkt, &backup);

	return udp_hdr->chksum;
}

static int eth_tx_offloading_disabled(const struct device *dev,
				      struct net_pkt *pkt)
{
	struct eth_context *context = dev->data;

	zassert_equal_ptr(&eth_context_offloading_disabled, context,
			  "Context pointers do not match (%p vs %p)",
			  eth_context_offloading_disabled, context);

	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (start_receiving) {
		struct net_udp_hdr hdr, *udp_hdr;
		uint16_t port;
		uint8_t lladdr[6];

		DBG("Packet %p received\n", pkt);

		/* Swap IP src and destination address so that we can receive
		 * the packet and the stack will not reject it.
		 */
		if (net_pkt_family(pkt) == AF_INET6) {
			struct in6_addr addr;

			net_ipaddr_copy(&addr, &NET_IPV6_HDR(pkt)->src);
			net_ipaddr_copy(&NET_IPV6_HDR(pkt)->src,
					&NET_IPV6_HDR(pkt)->dst);
			net_ipaddr_copy(&NET_IPV6_HDR(pkt)->dst, &addr);
		} else {
			struct in_addr addr;

			net_ipaddr_copy(&addr, &NET_IPV4_HDR(pkt)->src);
			net_ipaddr_copy(&NET_IPV4_HDR(pkt)->src,
					&NET_IPV4_HDR(pkt)->dst);
			net_ipaddr_copy(&NET_IPV4_HDR(pkt)->dst, &addr);
		}

		udp_hdr = net_udp_get_hdr(pkt, &hdr);
		zassert_not_null(udp_hdr, "UDP header missing");

		port = udp_hdr->src_port;
		udp_hdr->src_port = udp_hdr->dst_port;
		udp_hdr->dst_port = port;

		memcpy(lladdr,
		       ((struct net_eth_hdr *)net_pkt_data(pkt))->src.addr,
		       sizeof(lladdr));
		memcpy(((struct net_eth_hdr *)net_pkt_data(pkt))->src.addr,
		       ((struct net_eth_hdr *)net_pkt_data(pkt))->dst.addr,
		       sizeof(lladdr));
		memcpy(((struct net_eth_hdr *)net_pkt_data(pkt))->dst.addr,
		       lladdr, sizeof(lladdr));

		if (net_recv_data(net_pkt_iface(pkt),
				  net_pkt_clone(pkt, K_NO_WAIT)) < 0) {
			test_failed = true;
			zassert_true(false, "Packet %p receive failed\n", pkt);
		}

		return 0;
	}

	if (test_started) {
		uint16_t chksum;

		chksum = get_udp_chksum(pkt);

		DBG("Chksum 0x%x offloading disabled\n", chksum);

		zassert_not_equal(chksum, 0, "Checksum calculated");

		k_sem_give(&wait_data);
	}

	return 0;
}

static int eth_tx_offloading_enabled(const struct device *dev,
				     struct net_pkt *pkt)
{
	struct eth_context *context = dev->data;

	zassert_equal_ptr(&eth_context_offloading_enabled, context,
			  "Context pointers do not match (%p vs %p)",
			  eth_context_offloading_enabled, context);

	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		uint16_t chksum;

		chksum = get_udp_chksum(pkt);

		DBG("Chksum 0x%x offloading enabled\n", chksum);

		zassert_equal(chksum, 0, "Checksum calculated");

		k_sem_give(&wait_data);
	}

	return 0;
}

static enum ethernet_hw_caps eth_offloading_enabled(const struct device *dev)
{
	return ETHERNET_HW_TX_CHKSUM_OFFLOAD |
		ETHERNET_HW_RX_CHKSUM_OFFLOAD;
}

static enum ethernet_hw_caps eth_offloading_disabled(const struct device *dev)
{
	return 0;
}

static struct ethernet_api api_funcs_offloading_disabled = {
	.iface_api.init = eth_iface_init,

	.get_capabilities = eth_offloading_disabled,
	.send = eth_tx_offloading_disabled,
};

static struct ethernet_api api_funcs_offloading_enabled = {
	.iface_api.init = eth_iface_init,

	.get_capabilities = eth_offloading_enabled,
	.send = eth_tx_offloading_enabled,
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

ETH_NET_DEVICE_INIT(eth_offloading_disabled_test,
		    "eth_offloading_disabled_test",
		    eth_init, device_pm_control_nop,
		    &eth_context_offloading_disabled, NULL,
		    CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs_offloading_disabled,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_offloading_enabled_test,
		    "eth_offloading_enabled_test",
		    eth_init, device_pm_control_nop,
		    &eth_context_offloading_enabled, NULL,
		    CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs_offloading_enabled,
		    NET_ETH_MTU);

struct user_data {
	int eth_if_count;
	int total_if_count;
};

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static const char *iface2str(struct net_if *iface)
{
#ifdef CONFIG_NET_L2_ETHERNET
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}
#endif

#ifdef CONFIG_NET_L2_DUMMY
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		return "Dummy";
	}
#endif

	return "<unknown type>";
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
	struct user_data *ud = user_data;

	DBG("Interface %p (%s) [%d]\n", iface, iface2str(iface),
	    net_if_get_by_iface(iface));

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		struct eth_context *eth_ctx =
			net_if_get_device(iface)->data;

		if (eth_ctx == &eth_context_offloading_disabled) {
			DBG("Iface %p without offloading\n", iface);
			eth_interfaces[0] = iface;
		}

		if (eth_ctx == &eth_context_offloading_enabled) {
			DBG("Iface %p with offloading\n", iface);
			eth_interfaces[1] = iface;
		}

		ud->eth_if_count++;
	}

	/* By default all interfaces are down initially */
	net_if_down(iface);

	ud->total_if_count++;
}

static void test_eth_setup(void)
{
	struct user_data ud = { 0 };

	/* Make sure we have enough virtual interfaces */
	net_if_foreach(iface_cb, &ud);

	zassert_equal(ud.eth_if_count, sizeof(eth_interfaces) / sizeof(void *),
		      "Invalid number of interfaces (%d vs %d)\n",
		      ud.eth_if_count,
		      sizeof(eth_interfaces) / sizeof(void *));
}

static void test_address_setup(void)
{
	struct net_if_addr *ifaddr;
	struct net_if *iface1, *iface2;

	iface1 = eth_interfaces[0];
	iface2 = eth_interfaces[1];

	zassert_not_null(iface1, "Interface 1");
	zassert_not_null(iface2, "Interface 2");

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

	ifaddr = net_if_ipv4_addr_add(iface1, &in4addr_my,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv4 address");

	ifaddr = net_if_ipv6_addr_add(iface2, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv4_addr_add(iface2, &in4addr_my2,
				      NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv4 address");

	net_if_up(iface1);
	net_if_up(iface2);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;
}

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

static void test_tx_chksum_offload_disabled_test_v6(void)
{
	struct eth_context *ctx; /* This is interface context */
	struct net_if *iface;
	int ret, len;
	struct sockaddr_in6 dst_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(TEST_PORT),
	};
	struct sockaddr_in6 src_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
	};

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v6_ctx_1);
	zassert_equal(ret, 0, "Create IPv6 UDP context failed");

	memcpy(&src_addr6.sin6_addr, &my_addr1, sizeof(struct in6_addr));
	memcpy(&dst_addr6.sin6_addr, &dst_addr, sizeof(struct in6_addr));

	ret = net_context_bind(udp_v6_ctx_1, (struct sockaddr *)&src_addr6,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0, "Context bind failure test failed");

	iface = eth_interfaces[0];
	ctx = net_if_get_device(iface)->data;
	zassert_equal_ptr(&eth_context_offloading_disabled, ctx,
			  "eth context mismatch");

	test_started = true;

	ret = add_neighbor(iface, &dst_addr);
	zassert_true(ret, "Cannot add neighbor");

	len = strlen(test_data);

	ret = net_context_sendto(udp_v6_ctx_1, test_data, len,
				 (struct sockaddr *)&dst_addr6,
				 sizeof(struct sockaddr_in6),
				 NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	net_context_unref(udp_v6_ctx_1);
}

static void test_tx_chksum_offload_disabled_test_v4(void)
{
	struct eth_context *ctx; /* This is interface context */
	struct net_if *iface;
	int ret, len;
	struct sockaddr_in dst_addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(TEST_PORT),
	};
	struct sockaddr_in src_addr4 = {
		.sin_family = AF_INET,
		.sin_port = 0,
	};

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v4_ctx_1);
	zassert_equal(ret, 0, "Create IPv4 UDP context failed");

	memcpy(&src_addr4.sin_addr, &in4addr_my, sizeof(struct in_addr));
	memcpy(&dst_addr4.sin_addr, &in4addr_dst, sizeof(struct in_addr));

	ret = net_context_bind(udp_v4_ctx_1, (struct sockaddr *)&src_addr4,
			       sizeof(struct sockaddr_in));
	zassert_equal(ret, 0, "Context bind failure test failed");

	iface = eth_interfaces[0];
	ctx = net_if_get_device(iface)->data;
	zassert_equal_ptr(&eth_context_offloading_disabled, ctx,
			  "eth context mismatch");

	len = strlen(test_data);

	test_started = true;

	ret = add_neighbor(iface, &dst_addr);
	zassert_true(ret, "Cannot add neighbor");

	ret = net_context_sendto(udp_v4_ctx_1, test_data, len,
				 (struct sockaddr *)&dst_addr4,
				 sizeof(struct sockaddr_in),
				 NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	net_context_unref(udp_v4_ctx_1);
}

static void test_tx_chksum_offload_enabled_test_v6(void)
{
	struct eth_context *ctx; /* This is interface context */
	struct net_if *iface;
	int ret, len;
	struct sockaddr_in6 dst_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(TEST_PORT),
	};
	struct sockaddr_in6 src_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
	};

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v6_ctx_2);
	zassert_equal(ret, 0, "Create IPv6 UDP context failed");

	memcpy(&src_addr6.sin6_addr, &my_addr2, sizeof(struct in6_addr));
	memcpy(&dst_addr6.sin6_addr, &dst_addr, sizeof(struct in6_addr));

	ret = net_context_bind(udp_v6_ctx_2, (struct sockaddr *)&src_addr6,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0, "Context bind failure test failed");

	iface = eth_interfaces[1];
	ctx = net_if_get_device(iface)->data;
	zassert_equal_ptr(&eth_context_offloading_enabled, ctx,
			  "eth context mismatch");

	len = strlen(test_data);

	test_started = true;

	ret = add_neighbor(iface, &dst_addr);
	zassert_true(ret, "Cannot add neighbor");

	ret = net_context_sendto(udp_v6_ctx_2, test_data, len,
				 (struct sockaddr *)&dst_addr6,
				 sizeof(struct sockaddr_in6),
				 NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	net_context_unref(udp_v6_ctx_2);
}

static void test_tx_chksum_offload_enabled_test_v4(void)
{
	struct eth_context *ctx; /* This is interface context */
	struct net_if *iface;
	int ret, len;
	struct sockaddr_in dst_addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(TEST_PORT),
	};
	struct sockaddr_in src_addr4 = {
		.sin_family = AF_INET,
		.sin_port = 0,
	};

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v4_ctx_2);
	zassert_equal(ret, 0, "Create IPv4 UDP context failed");

	memcpy(&src_addr4.sin_addr, &in4addr_my2, sizeof(struct in_addr));
	memcpy(&dst_addr4.sin_addr, &in4addr_dst, sizeof(struct in_addr));

	ret = net_context_bind(udp_v4_ctx_2, (struct sockaddr *)&src_addr4,
			       sizeof(struct sockaddr_in));
	zassert_equal(ret, 0, "Context bind failure test failed");

	iface = eth_interfaces[1];
	ctx = net_if_get_device(iface)->data;
	zassert_equal_ptr(&eth_context_offloading_enabled, ctx,
			  "eth context mismatch");

	len = strlen(test_data);

	test_started = true;

	ret = add_neighbor(iface, &dst_addr);
	zassert_true(ret, "Cannot add neighbor");

	ret = net_context_sendto(udp_v4_ctx_2, test_data, len,
				 (struct sockaddr *)&dst_addr4,
				 sizeof(struct sockaddr_in),
				 NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	net_context_unref(udp_v4_ctx_2);
}

static void recv_cb_offload_disabled(struct net_context *context,
				     struct net_pkt *pkt,
				     union net_ip_header *ip_hdr,
				     union net_proto_header *proto_hdr,
				     int status,
				     void *user_data)
{
	zassert_not_null(proto_hdr->udp, "UDP header missing");
	zassert_not_equal(proto_hdr->udp->chksum, 0, "Checksum is not set");

	if (net_pkt_family(pkt) == AF_INET) {
		struct net_ipv4_hdr *ipv4 = NET_IPV4_HDR(pkt);

		zassert_not_equal(ipv4->chksum, 0,
				  "IPv4 checksum is not set");
	}

	k_sem_give(&wait_data);

	net_pkt_unref(pkt);
}

static void recv_cb_offload_enabled(struct net_context *context,
				    struct net_pkt *pkt,
				    union net_ip_header *ip_hdr,
				    union net_proto_header *proto_hdr,
				    int status,
				    void *user_data)
{
	zassert_not_null(proto_hdr->udp, "UDP header missing");
	zassert_equal(proto_hdr->udp->chksum, 0, "Checksum is set");

	if (net_pkt_family(pkt) == AF_INET) {
		struct net_ipv4_hdr *ipv4 = NET_IPV4_HDR(pkt);

		zassert_equal(ipv4->chksum, 0, "IPv4 checksum is set");
	}

	k_sem_give(&wait_data);

	net_pkt_unref(pkt);
}

static void test_rx_chksum_offload_disabled_test_v6(void)
{
	struct eth_context *ctx; /* This is interface context */
	struct net_if *iface;
	int ret, len;
	struct sockaddr_in6 dst_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(TEST_PORT),
	};
	struct sockaddr_in6 src_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
	};

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v6_ctx_1);
	zassert_equal(ret, 0, "Create IPv6 UDP context failed");

	memcpy(&src_addr6.sin6_addr, &my_addr1, sizeof(struct in6_addr));
	memcpy(&dst_addr6.sin6_addr, &dst_addr, sizeof(struct in6_addr));

	ret = net_context_bind(udp_v6_ctx_1, (struct sockaddr *)&src_addr6,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0, "Context bind failure test failed");

	iface = eth_interfaces[0];
	ctx = net_if_get_device(iface)->data;
	zassert_equal_ptr(&eth_context_offloading_disabled, ctx,
			  "eth context mismatch");

	len = strlen(test_data);

	test_started = true;
	start_receiving = true;

	ret = net_context_recv(udp_v6_ctx_1, recv_cb_offload_disabled,
			       K_NO_WAIT, NULL);
	zassert_equal(ret, 0, "Recv UDP failed (%d)\n", ret);

	start_receiving = false;

	ret = net_context_sendto(udp_v6_ctx_1, test_data, len,
				 (struct sockaddr *)&dst_addr6,
				 sizeof(struct sockaddr_in6),
				 NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	/* Let the receiver to receive the packets */
	k_sleep(K_MSEC(10));
}

static void test_rx_chksum_offload_disabled_test_v4(void)
{
	struct eth_context *ctx; /* This is interface context */
	struct net_if *iface;
	int ret, len;
	struct sockaddr_in dst_addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(TEST_PORT),
	};
	struct sockaddr_in src_addr4 = {
		.sin_family = AF_INET,
		.sin_port = 0,
	};

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v4_ctx_1);
	zassert_equal(ret, 0, "Create IPv4 UDP context failed");

	memcpy(&src_addr4.sin_addr, &in4addr_my, sizeof(struct in_addr));
	memcpy(&dst_addr4.sin_addr, &in4addr_dst, sizeof(struct in_addr));

	ret = net_context_bind(udp_v4_ctx_1, (struct sockaddr *)&src_addr4,
			       sizeof(struct sockaddr_in));
	zassert_equal(ret, 0, "Context bind failure test failed");

	iface = eth_interfaces[0];
	ctx = net_if_get_device(iface)->data;
	zassert_equal_ptr(&eth_context_offloading_disabled, ctx,
			  "eth context mismatch");

	len = strlen(test_data);

	test_started = true;
	start_receiving = true;

	ret = net_context_recv(udp_v4_ctx_1, recv_cb_offload_disabled,
			       K_NO_WAIT, NULL);
	zassert_equal(ret, 0, "Recv UDP failed (%d)\n", ret);

	start_receiving = false;

	ret = net_context_sendto(udp_v4_ctx_1, test_data, len,
				 (struct sockaddr *)&dst_addr4,
				 sizeof(struct sockaddr_in),
				 NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	/* Let the receiver to receive the packets */
	k_sleep(K_MSEC(10));
}

static void test_rx_chksum_offload_enabled_test_v6(void)
{
	struct eth_context *ctx; /* This is interface context */
	struct net_if *iface;
	int ret, len;
	struct sockaddr_in6 dst_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = htons(TEST_PORT),
	};
	struct sockaddr_in6 src_addr6 = {
		.sin6_family = AF_INET6,
		.sin6_port = 0,
	};

	ret = net_context_get(AF_INET6, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v6_ctx_2);
	zassert_equal(ret, 0, "Create IPv6 UDP context failed");

	memcpy(&src_addr6.sin6_addr, &my_addr2, sizeof(struct in6_addr));
	memcpy(&dst_addr6.sin6_addr, &dst_addr, sizeof(struct in6_addr));

	ret = net_context_bind(udp_v6_ctx_2, (struct sockaddr *)&src_addr6,
			       sizeof(struct sockaddr_in6));
	zassert_equal(ret, 0, "Context bind failure test failed");

	iface = net_if_ipv6_select_src_iface(&dst_addr6.sin6_addr);
	ctx = net_if_get_device(iface)->data;
	zassert_equal_ptr(&eth_context_offloading_enabled, ctx,
			  "eth context mismatch");

	len = strlen(test_data);

	test_started = true;
	start_receiving = true;

	ret = net_context_recv(udp_v6_ctx_2, recv_cb_offload_enabled,
			       K_NO_WAIT, NULL);
	zassert_equal(ret, 0, "Recv UDP failed (%d)\n", ret);

	ret = net_context_sendto(udp_v6_ctx_2, test_data, len,
				 (struct sockaddr *)&dst_addr6,
				 sizeof(struct sockaddr_in6),
				 NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	/* Let the receiver to receive the packets */
	k_sleep(K_MSEC(10));
}

static void test_rx_chksum_offload_enabled_test_v4(void)
{
	struct eth_context *ctx; /* This is interface context */
	struct net_if *iface;
	int ret, len;
	struct sockaddr_in dst_addr4 = {
		.sin_family = AF_INET,
		.sin_port = htons(TEST_PORT),
	};
	struct sockaddr_in src_addr4 = {
		.sin_family = AF_INET,
		.sin_port = 0,
	};

	ret = net_context_get(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
			      &udp_v4_ctx_2);
	zassert_equal(ret, 0, "Create IPv4 UDP context failed");

	memcpy(&src_addr4.sin_addr, &in4addr_my2, sizeof(struct in_addr));
	memcpy(&dst_addr4.sin_addr, &in4addr_dst, sizeof(struct in_addr));

	ret = net_context_bind(udp_v4_ctx_2, (struct sockaddr *)&src_addr4,
			       sizeof(struct sockaddr_in));
	zassert_equal(ret, 0, "Context bind failure test failed");

	iface = eth_interfaces[1];
	ctx = net_if_get_device(iface)->data;
	zassert_equal_ptr(&eth_context_offloading_enabled, ctx,
			  "eth context mismatch");

	len = strlen(test_data);

	test_started = true;
	start_receiving = true;

	ret = net_context_recv(udp_v4_ctx_2, recv_cb_offload_enabled,
			       K_NO_WAIT, NULL);
	zassert_equal(ret, 0, "Recv UDP failed (%d)\n", ret);

	ret = net_context_sendto(udp_v4_ctx_2, test_data, len,
				 (struct sockaddr *)&dst_addr4,
				 sizeof(struct sockaddr_in),
				 NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	/* Let the receiver to receive the packets */
	k_sleep(K_MSEC(10));
}

void test_main(void)
{
	ztest_test_suite(net_chksum_offload_test,
			 ztest_unit_test(test_eth_setup),
			 ztest_unit_test(test_address_setup),
			 ztest_unit_test(test_tx_chksum_offload_disabled_test_v6),
			 ztest_unit_test(test_tx_chksum_offload_disabled_test_v4),
			 ztest_unit_test(test_tx_chksum_offload_enabled_test_v6),
			 ztest_unit_test(test_tx_chksum_offload_enabled_test_v4),
			 ztest_unit_test(test_rx_chksum_offload_disabled_test_v6),
			 ztest_unit_test(test_rx_chksum_offload_disabled_test_v4),
			 ztest_unit_test(test_rx_chksum_offload_enabled_test_v6),
			 ztest_unit_test(test_rx_chksum_offload_enabled_test_v4)
			 );

	ztest_run_test_suite(net_chksum_offload_test);
}
