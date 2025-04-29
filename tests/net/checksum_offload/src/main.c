/* main.c - Application main entry point */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_L2_ETHERNET_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/random.h>

#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/net/udp.h>

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
static uint8_t test_data_large[2000];
static uint8_t verify_buf[2000];

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Destination address for test packets (interface 1) */
static struct in6_addr dst_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x2 } } };

/* Destination address for test packets (interface 2) */
static struct in6_addr dst_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x2 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct in_addr in4addr_my = { { { 192, 0, 2, 1 } } };
static struct in_addr in4addr_dst = { { { 192, 0, 2, 2 } } };
static struct in_addr in4addr_my2 = { { { 192, 0, 42, 1 } } };
static struct in_addr in4addr_dst2 = { { { 192, 0, 42, 2 } } };

/* Keep track of all ethernet interfaces. For native_sim board, we need
 * to increase the count as it has one extra network interface defined in
 * eth_native_tap driver.
 */
static struct net_if *eth_interfaces[2 + IS_ENABLED(CONFIG_ETH_NATIVE_TAP)];

static bool test_failed;
static bool test_started;
static int test_proto;
static bool verify_fragment;
static bool start_receiving;
static bool change_chksum;
static int fragment_count;
static int fragment_offset;

static K_SEM_DEFINE(wait_data_off, 0, UINT_MAX);
static K_SEM_DEFINE(wait_data_nonoff, 0, UINT_MAX);

#define WAIT_TIME K_MSEC(100)

struct eth_context {
	struct net_if *iface;
	uint8_t mac_addr[6];

	uint16_t expecting_tag;
};

static struct eth_context eth_context_offloading_disabled;
static struct eth_context eth_context_offloading_enabled;

static void verify_test_data_large(uint8_t *buf, size_t offset, size_t len)
{
	zassert(offset + len <= sizeof(test_data_large), "Out of bound data");

	for (size_t i = 0; i < len; i++) {
		zassert_equal(buf[i], test_data_large[offset + i], "Invalid data");
	}
}

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

static uint16_t get_icmp_chksum(struct net_pkt *pkt)
{
	NET_PKT_DATA_ACCESS_DEFINE(icmp_access, struct net_icmp_hdr);
	struct net_icmp_hdr *icmp_hdr;
	struct net_pkt_cursor backup;

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_backup(pkt, &backup);
	net_pkt_cursor_init(pkt);

	/* Move the cursor to the ICMP header */
	if (net_pkt_skip(pkt, sizeof(struct net_eth_hdr) +
			 net_pkt_ip_hdr_len(pkt) +
			 net_pkt_ipv6_ext_len(pkt))) {
		return 0;
	}

	icmp_hdr = (struct net_icmp_hdr *)net_pkt_get_data(pkt, &icmp_access);
	if (!icmp_hdr) {
		return 0;
	}

	net_pkt_cursor_restore(pkt, &backup);

	return icmp_hdr->chksum;
}

static void test_receiving(struct net_pkt *pkt)
{
	uint16_t port;
	uint8_t lladdr[6];

	DBG("Packet %p received\n", pkt);

	memcpy(lladdr, ((struct net_eth_hdr *)net_pkt_data(pkt))->src.addr,
	       sizeof(lladdr));
	memcpy(((struct net_eth_hdr *)net_pkt_data(pkt))->src.addr,
	       ((struct net_eth_hdr *)net_pkt_data(pkt))->dst.addr,
	       sizeof(lladdr));
	memcpy(((struct net_eth_hdr *)net_pkt_data(pkt))->dst.addr,
	       lladdr, sizeof(lladdr));

	net_pkt_skip(pkt, sizeof(struct net_eth_hdr));

	/* Swap IP src and destination address so that we can receive
	 * the packet and the stack will not reject it.
	 */
	if (net_pkt_family(pkt) == AF_INET6) {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv6_access,
						      struct net_ipv6_hdr);
		struct net_ipv6_hdr *ipv6_hdr;
		struct in6_addr addr;

		ipv6_hdr = (struct net_ipv6_hdr *)
					net_pkt_get_data(pkt, &ipv6_access);
		zassert_not_null(ipv6_hdr, "Can't access IPv6 header");

		net_ipv6_addr_copy_raw((uint8_t *)&addr, ipv6_hdr->src);
		net_ipv6_addr_copy_raw(ipv6_hdr->src, ipv6_hdr->dst);
		net_ipv6_addr_copy_raw(ipv6_hdr->dst, (uint8_t *)&addr);
	} else {
		NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(ipv4_access,
						      struct net_ipv4_hdr);
		struct net_ipv4_hdr *ipv4_hdr;
		struct in_addr addr;

		ipv4_hdr = (struct net_ipv4_hdr *)
					net_pkt_get_data(pkt, &ipv4_access);
		zassert_not_null(ipv4_hdr, "Can't access IPv4 header");

		net_ipv4_addr_copy_raw((uint8_t *)&addr, ipv4_hdr->src);
		net_ipv4_addr_copy_raw(ipv4_hdr->src, ipv4_hdr->dst);
		net_ipv4_addr_copy_raw(ipv4_hdr->dst, (uint8_t *)&addr);
	}

	if (!verify_fragment || fragment_count == 1) {
		net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) + net_pkt_ip_opts_len(pkt));
		if (test_proto == IPPROTO_UDP) {
			NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(
					udp_access, struct net_udp_hdr);
			struct net_udp_hdr *udp_hdr;

			udp_hdr = (struct net_udp_hdr *)
					net_pkt_get_data(pkt, &udp_access);
			zassert_not_null(udp_hdr, "Can't access UDP header");

			port = udp_hdr->src_port;
			udp_hdr->src_port = udp_hdr->dst_port;
			udp_hdr->dst_port = port;

			if (change_chksum) {
				udp_hdr->chksum++;
			}
		} else if (test_proto == IPPROTO_ICMP ||
			   test_proto == IPPROTO_ICMPV6) {
			NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(
					icmp_access, struct net_icmp_hdr);
			struct net_icmp_hdr *icmp_hdr;

			icmp_hdr = (struct net_icmp_hdr *)
					net_pkt_get_data(pkt, &icmp_access);
			zassert_not_null(icmp_hdr, "Can't access ICMP header");

			if (change_chksum) {
				icmp_hdr->chksum++;
			}
		}
	}

	net_pkt_cursor_init(pkt);

	if (net_recv_data(net_pkt_iface(pkt),
			  net_pkt_rx_clone(pkt, K_NO_WAIT)) < 0) {
		test_failed = true;
		zassert_true(false, "Packet %p receive failed\n", pkt);
	}
}

static void test_fragment(struct net_pkt *pkt, bool offloaded)
{
	uint16_t chksum = 0;
	size_t data_len;
	size_t hdr_offset = sizeof(struct net_eth_hdr) +
			    net_pkt_ip_hdr_len(pkt) +
			    net_pkt_ip_opts_len(pkt);

	fragment_count++;

	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);

	if (start_receiving) {
		test_receiving(pkt);
		return;
	}

	if (fragment_count == 1) {
		if (test_proto == IPPROTO_UDP) {
			chksum = get_udp_chksum(pkt);
			hdr_offset += sizeof(struct net_udp_hdr);
		} else if (test_proto == IPPROTO_ICMP ||
			   test_proto == IPPROTO_ICMPV6) {
			chksum = get_icmp_chksum(pkt);
			hdr_offset += sizeof(struct net_icmp_hdr) +
				      sizeof(struct net_icmpv6_echo_req);
		}

		/* Fragmented packet should have checksum set regardless of
		 * checksum offloading
		 */
		zassert_not_equal(chksum, 0, "Checksum missing");
	}

	zassert_true(net_pkt_is_chksum_done(pkt),
		     "Checksum should me marked as ready on net_pkt");

	/* Verify that payload has not been altered. */
	data_len = net_pkt_get_len(pkt) - hdr_offset;
	net_pkt_skip(pkt, hdr_offset);
	net_pkt_read(pkt, verify_buf, data_len);

	verify_test_data_large(verify_buf, fragment_offset, data_len);
	fragment_offset += data_len;

	if (fragment_count > 1) {
		if (offloaded) {
			k_sem_give(&wait_data_off);
		} else {
			k_sem_give(&wait_data_nonoff);
		}
	}
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

	if (verify_fragment) {
		test_fragment(pkt, false);
		return 0;
	}

	if (start_receiving) {
		test_receiving(pkt);
		return 0;
	}

	if (test_started) {
		uint16_t chksum = 0;

		if (test_proto == IPPROTO_UDP) {
			chksum = get_udp_chksum(pkt);
		} else if (test_proto == IPPROTO_ICMP ||
			   test_proto == IPPROTO_ICMPV6) {
			chksum = get_icmp_chksum(pkt);
		}

		DBG("Chksum 0x%x offloading disabled\n", chksum);

		zassert_not_equal(chksum, 0, "Checksum not calculated");

		k_sem_give(&wait_data_nonoff);
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

	if (verify_fragment) {
		test_fragment(pkt, true);
		return 0;
	}

	if (start_receiving) {
		test_receiving(pkt);
	}

	if (test_started) {
		uint16_t chksum = 0;

		if (test_proto == IPPROTO_UDP) {
			chksum = get_udp_chksum(pkt);
		} else if (test_proto == IPPROTO_ICMP ||
			   test_proto == IPPROTO_ICMPV6) {
			chksum = get_icmp_chksum(pkt);
		}

		DBG("Chksum 0x%x offloading enabled\n", chksum);

		zassert_equal(chksum, 0, "Checksum calculated");

		k_sem_give(&wait_data_off);
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
	mac_addr[5] = sys_rand8_get();
}

static int eth_init(const struct device *dev)
{
	struct eth_context *context = dev->data;

	generate_mac(context->mac_addr);

	return 0;
}

ETH_NET_DEVICE_INIT(eth1_offloading_disabled_test,
		    "eth1_offloading_disabled_test",
		    eth_init, NULL,
		    &eth_context_offloading_disabled, NULL,
		    CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs_offloading_disabled,
		    NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth0_offloading_enabled_test,
		    "eth0_offloading_enabled_test",
		    eth_init, NULL,
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
	struct in_addr netmask = { { { 255, 255, 255, 0 } } };
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

	/* For testing purposes we need to set the addresses preferred */
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

	net_if_ipv4_set_netmask_by_addr(iface1, &in4addr_my, &netmask);

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

	net_if_ipv4_set_netmask_by_addr(iface2, &in4addr_my2, &netmask);

	net_if_up(iface1);
	net_if_up(iface2);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;
}

static void add_neighbor(struct net_if *iface, struct in6_addr *addr)
{
	struct net_linkaddr lladdr;
	struct net_nbr *nbr;

	lladdr.addr[0] = 0x01;
	lladdr.addr[1] = 0x02;
	lladdr.addr[2] = 0x33;
	lladdr.addr[3] = 0x44;
	lladdr.addr[4] = 0x05;
	lladdr.addr[5] = 0x06;

	lladdr.len = 6U;
	lladdr.type = NET_LINK_ETHERNET;

	nbr = net_ipv6_nbr_add(iface, addr, &lladdr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	if (!nbr) {
		DBG("Cannot add dst %s to neighbor cache\n",
		    net_sprint_ipv6_addr(addr));
	}
}

static struct net_context *test_udp_context_prepare(sa_family_t family,
						    bool offloaded,
						    struct sockaddr *dst_addr)
{
	struct net_context *net_ctx;
	struct eth_context *ctx; /* This is interface context */
	struct sockaddr src_addr;
	socklen_t addrlen;
	struct net_if *iface;
	int ret;

	if (family == AF_INET6) {
		struct sockaddr_in6 *dst_addr6 =
					(struct sockaddr_in6 *)dst_addr;
		struct sockaddr_in6 *src_addr6 =
					(struct sockaddr_in6 *)&src_addr;

		dst_addr6->sin6_family = AF_INET6;
		dst_addr6->sin6_port = htons(TEST_PORT);
		src_addr6->sin6_family = AF_INET6;
		src_addr6->sin6_port = 0;

		if (offloaded) {
			memcpy(&src_addr6->sin6_addr, &my_addr2,
			       sizeof(struct in6_addr));
			memcpy(&dst_addr6->sin6_addr, &dst_addr2,
			       sizeof(struct in6_addr));
		} else {
			memcpy(&src_addr6->sin6_addr, &my_addr1,
			       sizeof(struct in6_addr));
			memcpy(&dst_addr6->sin6_addr, &dst_addr1,
			       sizeof(struct in6_addr));
		}

		addrlen = sizeof(struct sockaddr_in6);
	} else {
		struct sockaddr_in *dst_addr4 =
					(struct sockaddr_in *)dst_addr;
		struct sockaddr_in *src_addr4 =
					(struct sockaddr_in *)&src_addr;

		dst_addr4->sin_family = AF_INET;
		dst_addr4->sin_port = htons(TEST_PORT);
		src_addr4->sin_family = AF_INET;
		src_addr4->sin_port = 0;

		if (offloaded) {
			memcpy(&src_addr4->sin_addr, &in4addr_my2,
			       sizeof(struct in_addr));
			memcpy(&dst_addr4->sin_addr, &in4addr_dst2,
			       sizeof(struct in_addr));
		} else {
			memcpy(&src_addr4->sin_addr, &in4addr_my,
			       sizeof(struct in_addr));
			memcpy(&dst_addr4->sin_addr, &in4addr_dst,
			       sizeof(struct in_addr));
		}

		addrlen = sizeof(struct sockaddr_in6);
	}

	ret = net_context_get(family, SOCK_DGRAM, IPPROTO_UDP, &net_ctx);
	zassert_equal(ret, 0, "Create %s UDP context failed",
		      family == AF_INET6 ? "IPv6" : "IPv4");

	ret = net_context_bind(net_ctx, &src_addr, addrlen);
	zassert_equal(ret, 0, "Context bind failure test failed");

	/* Verify iface data */
	if (offloaded) {
		iface = eth_interfaces[1];
		ctx = net_if_get_device(iface)->data;
		zassert_equal_ptr(&eth_context_offloading_enabled, ctx,
				  "eth context mismatch");
	} else {
		iface = eth_interfaces[0];
		ctx = net_if_get_device(iface)->data;
		zassert_equal_ptr(&eth_context_offloading_disabled, ctx,
				  "eth context mismatch");
	}

	return net_ctx;
}

static void test_tx_chksum(sa_family_t family, bool offloaded)
{
	struct k_sem *wait_data = offloaded ? &wait_data_off : &wait_data_nonoff;
	socklen_t addrlen = (family == AF_INET6) ? sizeof(struct sockaddr_in6) :
						   sizeof(struct sockaddr_in);
	struct net_context *net_ctx;
	struct sockaddr dst_addr;
	int ret, len;

	net_ctx = test_udp_context_prepare(family, offloaded, &dst_addr);
	zassert_not_null(net_ctx, "Failed to obtain net_ctx");

	test_started = true;
	test_proto = IPPROTO_UDP;

	len = strlen(test_data);
	ret = net_context_sendto(net_ctx, test_data, len, &dst_addr,
				 addrlen, NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	net_context_unref(net_ctx);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_disabled_test_v6)
{
	test_tx_chksum(AF_INET6, false);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_disabled_test_v4)
{
	test_tx_chksum(AF_INET, false);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_enabled_test_v6)
{
	test_tx_chksum(AF_INET6, true);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_enabled_test_v4)
{
	test_tx_chksum(AF_INET, true);
}

static void test_tx_chksum_udp_frag(sa_family_t family, bool offloaded)
{
	struct k_sem *wait_data = offloaded ? &wait_data_off : &wait_data_nonoff;
	socklen_t addrlen = (family == AF_INET6) ? sizeof(struct sockaddr_in6) :
						   sizeof(struct sockaddr_in);
	struct net_context *net_ctx;
	struct sockaddr dst_addr;
	int ret, len;

	net_ctx = test_udp_context_prepare(family, offloaded, &dst_addr);
	zassert_not_null(net_ctx, "Failed to obtain net_ctx");

	test_started = true;
	test_proto = IPPROTO_UDP;
	verify_fragment = true;

	len = sizeof(test_data_large);
	ret = net_context_sendto(net_ctx, test_data_large, len, &dst_addr,
				 addrlen, NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	net_context_unref(net_ctx);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_disabled_test_v6_udp_frag)
{
	test_tx_chksum_udp_frag(AF_INET6, false);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_disabled_test_v4_udp_frag)
{
	test_tx_chksum_udp_frag(AF_INET, false);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_enabled_test_v6_udp_frag)
{
	test_tx_chksum_udp_frag(AF_INET6, true);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_enabled_test_v4_udp_frag)
{
	test_tx_chksum_udp_frag(AF_INET, true);
}

static int dummy_icmp_handler(struct net_icmp_ctx *ctx,
			      struct net_pkt *pkt,
			      struct net_icmp_ip_hdr *hdr,
			      struct net_icmp_hdr *icmp_hdr,
			      void *user_data)
{
	ARG_UNUSED(ctx);
	ARG_UNUSED(pkt);
	ARG_UNUSED(hdr);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	return 0;
}

static void test_icmp_init(sa_family_t family, bool offloaded,
			   struct sockaddr *dst_addr, struct net_if **iface)
{
	if (family == AF_INET6) {
		struct sockaddr_in6 *dst_addr6 =
					(struct sockaddr_in6 *)dst_addr;

		dst_addr6->sin6_family = AF_INET6;

		if (offloaded) {
			memcpy(&dst_addr6->sin6_addr, &dst_addr2,
			       sizeof(struct in6_addr));
		} else {
			memcpy(&dst_addr6->sin6_addr, &dst_addr1,
			       sizeof(struct in6_addr));
		}
	} else {
		struct sockaddr_in *dst_addr4 =
					(struct sockaddr_in *)dst_addr;

		dst_addr4->sin_family = AF_INET;

		if (offloaded) {
			memcpy(&dst_addr4->sin_addr, &in4addr_dst2,
			       sizeof(struct in_addr));
		} else {
			memcpy(&dst_addr4->sin_addr, &in4addr_dst,
			       sizeof(struct in_addr));
		}
	}

	if (offloaded) {
		*iface = eth_interfaces[1];
	} else {
		*iface = eth_interfaces[0];
	}
}

static void test_tx_chksum_icmp_frag(sa_family_t family, bool offloaded)
{
	struct k_sem *wait_data = offloaded ? &wait_data_off : &wait_data_nonoff;
	struct net_icmp_ping_params params = { 0 };
	struct net_icmp_ctx ctx;
	struct sockaddr dst_addr;
	struct net_if *iface;
	int ret;

	test_icmp_init(family, offloaded, &dst_addr, &iface);

	ret = net_icmp_init_ctx(&ctx, 0, 0, dummy_icmp_handler);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	test_started = true;
	test_proto = (family == AF_INET6) ? IPPROTO_ICMPV6 : IPPROTO_ICMP;
	verify_fragment = true;

	params.data = test_data_large;
	params.data_size = sizeof(test_data_large);
	ret = net_icmp_send_echo_request(&ctx, iface, &dst_addr, &params, NULL);
	zassert_equal(ret, 0, "Cannot send ICMP Echo-Request (%d)", ret);

	if (k_sem_take(wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	ret = net_icmp_cleanup_ctx(&ctx);
	zassert_equal(ret, 0, "Cannot cleanup ICMP (%d)", ret);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_disabled_test_v6_icmp_frag)
{
	test_tx_chksum_icmp_frag(AF_INET6, false);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_disabled_test_v4_icmp_frag)
{
	test_tx_chksum_icmp_frag(AF_INET, false);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_enabled_test_v6_icmp_frag)
{
	test_tx_chksum_icmp_frag(AF_INET6, true);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_enabled_test_v4_icmp_frag)
{
	test_tx_chksum_icmp_frag(AF_INET, true);
}

static void test_fragment_rx_udp(struct net_pkt *pkt,
				 union net_proto_header *proto_hdr)
{
	size_t hdr_offset = net_pkt_ip_hdr_len(pkt) +
			    net_pkt_ip_opts_len(pkt) +
			    sizeof(struct net_udp_hdr);
	size_t data_len = net_pkt_get_len(pkt) - hdr_offset;

	/* In case of fragmented packets, checksum shall be present/verified
	 * regardless.
	 */
	zassert_not_equal(proto_hdr->udp->chksum, 0, "Checksum is not set");
	zassert_equal(net_calc_verify_chksum_udp(pkt), 0, "Incorrect checksum");

	/* Verify that packet content has not been altered */
	net_pkt_read(pkt, verify_buf, data_len);
	verify_test_data_large(verify_buf, 0, data_len);
}

static void recv_cb_offload_disabled(struct net_context *context,
				     struct net_pkt *pkt,
				     union net_ip_header *ip_hdr,
				     union net_proto_header *proto_hdr,
				     int status,
				     void *user_data)
{
	zassert_not_null(proto_hdr->udp, "UDP header missing");

	if (verify_fragment) {
		test_fragment_rx_udp(pkt, proto_hdr);
	} else {
		zassert_not_equal(proto_hdr->udp->chksum, 0, "Checksum is not set");
		zassert_equal(net_calc_verify_chksum_udp(pkt), 0, "Incorrect checksum");
	}

	if (net_pkt_family(pkt) == AF_INET) {
		struct net_ipv4_hdr *ipv4 = NET_IPV4_HDR(pkt);

		zassert_not_equal(ipv4->chksum, 0,
				  "IPv4 checksum is not set");
	}

	k_sem_give(&wait_data_nonoff);

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

	if (verify_fragment) {
		test_fragment_rx_udp(pkt, proto_hdr);
	} else {
		zassert_equal(proto_hdr->udp->chksum, 0, "Checksum is set");

		if (net_pkt_family(pkt) == AF_INET) {
			struct net_ipv4_hdr *ipv4 = NET_IPV4_HDR(pkt);

			zassert_equal(ipv4->chksum, 0, "IPv4 checksum is set");
		}
	}

	k_sem_give(&wait_data_off);

	net_pkt_unref(pkt);
}

static void test_rx_chksum(sa_family_t family, bool offloaded)
{
	struct k_sem *wait_data = offloaded ? &wait_data_off : &wait_data_nonoff;
	net_context_recv_cb_t cb = offloaded ? recv_cb_offload_enabled :
					       recv_cb_offload_disabled;
	socklen_t addrlen = (family == AF_INET6) ? sizeof(struct sockaddr_in6) :
						   sizeof(struct sockaddr_in);
	struct net_context *net_ctx;
	struct sockaddr dst_addr;
	int ret, len;

	net_ctx = test_udp_context_prepare(family, offloaded, &dst_addr);
	zassert_not_null(net_ctx, "Failed to obtain net_ctx");

	test_started = true;
	test_proto = IPPROTO_UDP;
	start_receiving = true;

	ret = net_context_recv(net_ctx, cb, K_NO_WAIT, NULL);
	zassert_equal(ret, 0, "Recv UDP failed (%d)\n", ret);

	len = strlen(test_data);
	ret = net_context_sendto(net_ctx, test_data, len, &dst_addr,
				 addrlen, NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	/* Let the receiver to receive the packets */
	k_sleep(K_MSEC(10));

	net_context_unref(net_ctx);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_disabled_test_v6)
{
	test_rx_chksum(AF_INET6, false);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_disabled_test_v4)
{
	test_rx_chksum(AF_INET, false);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_enabled_test_v6)
{
	test_rx_chksum(AF_INET6, true);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_enabled_test_v4)
{
	test_rx_chksum(AF_INET, true);
}

static void test_rx_chksum_udp_frag(sa_family_t family, bool offloaded)
{
	struct k_sem *wait_data = offloaded ? &wait_data_off : &wait_data_nonoff;
	net_context_recv_cb_t cb = offloaded ? recv_cb_offload_enabled :
					       recv_cb_offload_disabled;
	socklen_t addrlen = (family == AF_INET6) ? sizeof(struct sockaddr_in6) :
						   sizeof(struct sockaddr_in);
	struct net_context *net_ctx;
	struct sockaddr dst_addr;
	int ret, len;

	net_ctx = test_udp_context_prepare(family, offloaded, &dst_addr);
	zassert_not_null(net_ctx, "Failed to obtain net_ctx");

	test_started = true;
	test_proto = IPPROTO_UDP;
	start_receiving = true;
	verify_fragment = true;

	ret = net_context_recv(net_ctx, cb, K_NO_WAIT, NULL);
	zassert_equal(ret, 0, "Recv UDP failed (%d)\n", ret);

	len = sizeof(test_data_large);
	ret = net_context_sendto(net_ctx, test_data_large, len, &dst_addr,
				 addrlen, NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	/* Let the receiver to receive the packets */
	k_sleep(K_MSEC(10));

	net_context_unref(net_ctx);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_disabled_test_v6_udp_frag)
{
	test_rx_chksum_udp_frag(AF_INET6, false);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_disabled_test_v4_udp_frag)
{
	test_rx_chksum_udp_frag(AF_INET, false);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_enabled_test_v6_udp_frag)
{
	test_rx_chksum_udp_frag(AF_INET6, true);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_enabled_test_v4_udp_frag)
{
	test_rx_chksum_udp_frag(AF_INET, true);
}

static void test_rx_chksum_udp_frag_bad(sa_family_t family, bool offloaded)
{
	struct k_sem *wait_data = offloaded ? &wait_data_off : &wait_data_nonoff;
	net_context_recv_cb_t cb = offloaded ? recv_cb_offload_enabled :
					       recv_cb_offload_disabled;
	socklen_t addrlen = (family == AF_INET6) ? sizeof(struct sockaddr_in6) :
						   sizeof(struct sockaddr_in);
	struct net_context *net_ctx;
	struct sockaddr dst_addr;
	int ret, len;

	net_ctx = test_udp_context_prepare(family, offloaded, &dst_addr);
	zassert_not_null(net_ctx, "Failed to obtain net_ctx");

	test_started = true;
	test_proto = IPPROTO_UDP;
	start_receiving = true;
	verify_fragment = true;
	change_chksum = true;

	ret = net_context_recv(net_ctx, cb, K_NO_WAIT, NULL);
	zassert_equal(ret, 0, "Recv UDP failed (%d)\n", ret);

	len = sizeof(test_data_large);
	ret = net_context_sendto(net_ctx, test_data_large, len, &dst_addr,
				 addrlen, NULL, K_FOREVER, NULL);
	zassert_equal(ret, len, "Send UDP pkt failed (%d)\n", ret);

	if (k_sem_take(wait_data, WAIT_TIME) == 0) {
		DBG("Packet with bad chksum should be dropped\n");
		zassert_false(true, "Packet received");
	}

	/* Let the receiver to receive the packets */
	k_sleep(K_MSEC(10));

	net_context_unref(net_ctx);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_disabled_test_v6_udp_frag_bad)
{
	test_rx_chksum_udp_frag_bad(AF_INET6, false);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_disabled_test_v4_udp_frag_bad)
{
	test_rx_chksum_udp_frag_bad(AF_INET, false);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_enabled_test_v6_udp_frag_bad)
{
	test_rx_chksum_udp_frag_bad(AF_INET6, true);
}

ZTEST(net_chksum_offload, test_tx_chksum_offload_enabled_test_v4_udp_frag_bad)
{
	test_rx_chksum_udp_frag_bad(AF_INET, true);
}

static int icmp_handler(struct net_icmp_ctx *ctx,
			struct net_pkt *pkt,
			struct net_icmp_ip_hdr *hdr,
			struct net_icmp_hdr *icmp_hdr,
			void *user_data)
{
	struct k_sem *wait_data = user_data;

	size_t hdr_offset = net_pkt_ip_hdr_len(pkt) +
			    net_pkt_ip_opts_len(pkt) +
			    sizeof(struct net_icmp_hdr) +
			    sizeof(struct net_icmpv6_echo_req);
	size_t data_len = net_pkt_get_len(pkt) - hdr_offset;

	/* In case of fragmented packets, checksum shall be present/verified
	 * regardless.
	 */
	zassert_not_equal(icmp_hdr->chksum, 0, "Checksum is not set");

	if (test_proto == IPPROTO_ICMPV6) {
		zassert_equal(net_calc_chksum_icmpv6(pkt), 0, "Incorrect checksum");
	} else {
		zassert_equal(net_calc_chksum_icmpv4(pkt), 0, "Incorrect checksum");
	}

	/* Verify that packet content has not been altered */
	net_pkt_set_overwrite(pkt, true);
	net_pkt_cursor_init(pkt);
	net_pkt_skip(pkt, hdr_offset);
	net_pkt_read(pkt, verify_buf, data_len);
	verify_test_data_large(verify_buf, 0, data_len);

	k_sem_give(wait_data);

	return 0;
}

static void test_rx_chksum_icmp_frag(sa_family_t family, bool offloaded)
{
	struct k_sem *wait_data = offloaded ? &wait_data_off : &wait_data_nonoff;
	struct net_icmp_ping_params params = { 0 };
	struct net_icmp_ctx ctx;
	struct sockaddr dst_addr;
	struct net_if *iface;
	int ret;

	test_icmp_init(family, offloaded, &dst_addr, &iface);

	ret = net_icmp_init_ctx(&ctx,
				family == AF_INET6 ? NET_ICMPV6_ECHO_REPLY :
						     NET_ICMPV4_ECHO_REPLY,
				0, icmp_handler);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	test_started = true;
	test_proto = (family == AF_INET6) ? IPPROTO_ICMPV6 : IPPROTO_ICMP;
	start_receiving = true;
	verify_fragment = true;

	params.data = test_data_large;
	params.data_size = sizeof(test_data_large);
	ret = net_icmp_send_echo_request(&ctx, iface, &dst_addr, &params,
					 wait_data);
	zassert_equal(ret, 0, "Cannot send ICMP Echo-Request (%d)", ret);

	if (k_sem_take(wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	ret = net_icmp_cleanup_ctx(&ctx);
	zassert_equal(ret, 0, "Cannot cleanup ICMP (%d)", ret);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_disabled_test_v6_icmp_frag)
{
	test_rx_chksum_icmp_frag(AF_INET6, false);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_disabled_test_v4_icmp_frag)
{
	test_rx_chksum_icmp_frag(AF_INET, false);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_enabled_test_v6_icmp_frag)
{
	test_rx_chksum_icmp_frag(AF_INET6, true);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_enabled_test_v4_icmp_frag)
{
	test_rx_chksum_icmp_frag(AF_INET, true);
}

static void test_rx_chksum_icmp_frag_bad(sa_family_t family, bool offloaded)
{
	struct k_sem *wait_data = offloaded ? &wait_data_off : &wait_data_nonoff;
	struct net_icmp_ping_params params = { 0 };
	struct net_icmp_ctx ctx;
	struct sockaddr dst_addr;
	struct net_if *iface;
	int ret;

	test_icmp_init(family, offloaded, &dst_addr, &iface);

	ret = net_icmp_init_ctx(&ctx,
				family == AF_INET6 ? NET_ICMPV6_ECHO_REPLY :
						     NET_ICMPV4_ECHO_REPLY,
				0, icmp_handler);
	zassert_equal(ret, 0, "Cannot init ICMP (%d)", ret);

	test_started = true;
	test_proto = (family == AF_INET6) ? IPPROTO_ICMPV6 : IPPROTO_ICMP;
	start_receiving = true;
	verify_fragment = true;
	change_chksum = true;

	params.data = test_data_large;
	params.data_size = sizeof(test_data_large);
	ret = net_icmp_send_echo_request(&ctx, iface, &dst_addr, &params,
					 wait_data);
	zassert_equal(ret, 0, "Cannot send ICMP Echo-Request (%d)", ret);

	if (k_sem_take(wait_data, WAIT_TIME) == 0) {
		DBG("Packet with bad chksum should be dropped\n");
		zassert_false(true, "Packet received");
	}

	ret = net_icmp_cleanup_ctx(&ctx);
	zassert_equal(ret, 0, "Cannot cleanup ICMP (%d)", ret);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_disabled_test_v6_icmp_frag_bad)
{
	test_rx_chksum_icmp_frag_bad(AF_INET6, false);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_disabled_test_v4_icmp_frag_bad)
{
	test_rx_chksum_icmp_frag_bad(AF_INET, false);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_enabled_test_v6_icmp_frag_bad)
{
	test_rx_chksum_icmp_frag_bad(AF_INET6, true);
}

ZTEST(net_chksum_offload, test_rx_chksum_offload_enabled_test_v4_icmp_frag_bad)
{
	test_rx_chksum_icmp_frag_bad(AF_INET, true);
}

static void *net_chksum_offload_tests_setup(void)
{
	test_eth_setup();
	test_address_setup();

	add_neighbor(eth_interfaces[0], &dst_addr1);
	add_neighbor(eth_interfaces[1], &dst_addr2);

	for (size_t i = 0; i < sizeof(test_data_large); i++) {
		test_data_large[i] = (uint8_t)i;
	}

	return NULL;
}

static void net_chksum_offload_tests_before(void *fixture)
{
	ARG_UNUSED(fixture);

	k_sem_reset(&wait_data_off);
	k_sem_reset(&wait_data_nonoff);

	test_failed = false;
	test_started = false;
	start_receiving = false;
	verify_fragment = false;
	change_chksum = false;
	fragment_count = 0;
	fragment_offset = 0;
	test_proto = 0;

	memset(verify_buf, 0, sizeof(verify_buf));
}

ZTEST_SUITE(net_chksum_offload, NULL, net_chksum_offload_tests_setup,
	    net_chksum_offload_tests_before, NULL, NULL);
