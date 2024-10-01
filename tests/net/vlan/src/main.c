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

#include <zephyr/net/socket.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet_vlan.h>
#include <zephyr/net/net_l2.h>

#include "ipv6.h"
#include "../../socket/socket_helpers.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

#define ANY_PORT 0
#define SERVER_PORT 4242
#define CLIENT_PORT 9898

#define VLAN_TAG_1 100
#define VLAN_TAG_2 200
#define VLAN_TAG_3 300
#define VLAN_TAG_4 400
#define VLAN_TAG_5 500

#define NET_ETH_MAX_COUNT 2

#define MY_IPV6_ADDR "2001:db8:200::2"
#define MY_IPV6_ADDR_SRV "2001:db8:200::1"

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* VLAN Interface 3 addresses */
static struct in6_addr my_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

/* Keep track of all ethernet interfaces */
static struct net_if *eth_interfaces[NET_ETH_MAX_COUNT];
static struct net_if *vlan_interfaces[NET_VLAN_MAX_COUNT];
static struct net_if *dummy_interfaces[2];

static bool test_failed;
static bool test_started;

static K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define BUF_AND_SIZE(buf) buf, sizeof(buf) - 1
#define STRLEN(buf) (sizeof(buf) - 1)

#define TEST_STR_SMALL "test"

/* More than 256 bytes, to use >1 net_buf. */
#define TEST_STR2 \
	"The Zephyr Project, a Linux Foundation hosted Collaboration " \
	"Project, is an open source collaborative effort uniting leaders " \
	"from across the industry to build a best-in-breed small, scalable, " \
	"real-time operating system (RTOS) optimized for resource-" \
	"constrained devices, across multiple architectures."

#define WAIT_TIME K_MSEC(500)

struct eth_context {
	struct net_if *iface;
	uint8_t mac_addr[6];

	uint16_t expecting_tag;
};

static struct eth_context eth_vlan_context;

static void eth_vlan_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_context *context = dev->data;

	net_if_set_link_addr(iface, context->mac_addr,
			     sizeof(context->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_tx(const struct device *dev, struct net_pkt *pkt)
{
	struct eth_context *context = dev->data;
	int ret;

	zassert_equal_ptr(&eth_vlan_context, context,
			  "Context pointers do not match (%p vs %p)",
			  eth_vlan_context, context);

	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		struct net_eth_vlan_hdr *hdr =
			(struct net_eth_vlan_hdr *)NET_ETH_HDR(pkt);

		zassert_equal(context->expecting_tag,
			      net_pkt_vlan_tag(pkt),
			      "Invalid VLAN tag (%d vs %d) in TX pkt",
			      net_pkt_vlan_tag(pkt),
			      context->expecting_tag);

		zassert_equal(context->expecting_tag,
			      net_eth_vlan_get_vid(ntohs(hdr->vlan.tci)),
			      "Invalid VLAN tag in ethernet header");

		k_sleep(K_MSEC(10));
		k_sem_give(&wait_data);
	}

	ret = net_recv_data(net_pkt_iface(pkt),
			    net_pkt_clone(pkt, K_NO_WAIT));
	zassert_false(ret < 0, "Cannot receive data (%d)", ret);

	return ret;
}

static enum ethernet_hw_caps eth_capabilities(const struct device *dev)
{
	return ETHERNET_HW_VLAN;
}

static struct ethernet_api api_funcs = {
	.iface_api.init = eth_vlan_iface_init,

	.get_capabilities = eth_capabilities,
	.send = eth_tx,
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

static int eth_vlan_init(const struct device *dev)
{
	struct eth_context *context = dev->data;

	generate_mac(context->mac_addr);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_vlan_test, "eth_vlan_test",
		    eth_vlan_init, NULL,
		    &eth_vlan_context, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs, NET_ETH_MTU);

static int eth_init(const struct device *dev)
{
	struct eth_context *context = dev->data;

	generate_mac(context->mac_addr);

	return 0;
}

ETH_NET_DEVICE_INIT(eth_test, "eth_test", eth_init, NULL,
		    &eth_vlan_context, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &api_funcs, NET_ETH_MTU);

struct net_if_test {
	uint8_t idx; /* not used for anything, just a dummy value */
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static uint8_t *net_iface_get_mac(const struct device *dev)
{
	struct net_if_test *data = dev->data;

	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	data->mac_addr[0] = 0x00;
	data->mac_addr[1] = 0x00;
	data->mac_addr[2] = 0x5E;
	data->mac_addr[3] = 0x00;
	data->mac_addr[4] = 0x53;
	data->mac_addr[5] = sys_rand8_get();

	data->ll_addr.addr = data->mac_addr;
	data->ll_addr.len = 6U;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

struct net_if_test net_iface1_data;
struct net_if_test net_iface2_data;

static struct dummy_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

/* For testing purposes, create two dummy network interfaces so we can check
 * that no VLANs are created for it.
 */
NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 NULL,
			 NULL,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2),
			 127);

NET_DEVICE_INIT_INSTANCE(net_iface2_test,
			 "iface2",
			 iface2,
			 NULL,
			 NULL,
			 &net_iface2_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 DUMMY_L2,
			 NET_L2_GET_CTX_TYPE(DUMMY_L2),
			 127);

struct user_data {
	int eth_if_count;
	int vlan_if_count;
	int dummy_if_count;
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
		eth_interfaces[ud->eth_if_count++] = iface;
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		vlan_interfaces[ud->vlan_if_count++] = iface;

		zassert_true(ud->vlan_if_count <= NET_VLAN_MAX_COUNT,
			     "Too many VLAN interfaces");
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		dummy_interfaces[ud->dummy_if_count++] = iface;

		zassert_true(ud->dummy_if_count <= 2,
			     "Too many dummy interfaces");
	}

	/* By default all interfaces are down initially */
	net_if_down(iface);

	ud->total_if_count++;
}

static void test_vlan_setup(void)
{
	struct user_data ud = { 0 };
	int remaining;

	/* Make sure we have enough virtual interfaces */
	net_if_foreach(iface_cb, &ud);

	/* One extra eth interface without vlan support */
	zassert_equal(ud.vlan_if_count, NET_VLAN_MAX_COUNT,
		      "Invalid number of VLANs %d vs %d",
		      ud.vlan_if_count, NET_VLAN_MAX_COUNT);

	remaining = ud.total_if_count - NET_VLAN_MAX_COUNT -
		ud.eth_if_count - ud.dummy_if_count;
	zassert_equal(remaining, 0,
		      "Invalid number of interfaces expecting %d got %d+%d+%d",
		      ud.total_if_count, NET_VLAN_MAX_COUNT,
		      ud.eth_if_count, ud.dummy_if_count);
}

static void test_address_setup(void)
{
	struct net_if_addr *ifaddr;
	struct net_if *iface1, *iface2, *iface3;

	iface1 = eth_interfaces[1]; /* This has VLAN enabled */
	iface2 = eth_interfaces[0]; /* and this one not */
	iface3 = vlan_interfaces[0]; /* and this is the virtual VLAN interface */

	zassert_not_null(iface1, "Interface 1");
	zassert_not_null(iface2, "Interface 2");
	zassert_not_null(iface3, "Interface 3");

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

	ifaddr = net_if_ipv6_addr_add(iface2, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface3, &my_addr3,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr3));
		zassert_not_null(ifaddr, "addr3");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_if_up(iface1);
	net_if_up(iface2);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;
}

ZTEST(net_vlan, test_vlan_tci)
{
	struct net_pkt *pkt;
	uint16_t tci;
	uint16_t tag;
	uint8_t priority;
	bool dei;

	pkt = net_pkt_alloc(K_FOREVER);

	tag = NET_VLAN_TAG_UNSPEC;
	net_pkt_set_vlan_tag(pkt, tag);

	priority = 0U;
	net_pkt_set_vlan_priority(pkt, priority);

	zassert_equal(net_pkt_vlan_tag(pkt), NET_VLAN_TAG_UNSPEC,
		      "invalid VLAN tag unspec");
	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");

	net_pkt_set_vlan_tag(pkt, 0);
	zassert_equal(net_pkt_vlan_tag(pkt), 0, "invalid VLAN tag");

	/* TCI should be zero now */
	zassert_equal(net_pkt_vlan_tci(pkt), 0, "invalid VLAN TCI");

	priority = 1U;
	net_pkt_set_vlan_priority(pkt, priority);

	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");

	net_pkt_set_vlan_tag(pkt, tag);

	zassert_equal(net_pkt_vlan_tag(pkt), NET_VLAN_TAG_UNSPEC,
		      "invalid VLAN tag unspec");

	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");

	net_pkt_set_vlan_tag(pkt, 0);
	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");

	dei = true;
	net_pkt_set_vlan_dei(pkt, dei);

	zassert_equal(net_pkt_vlan_dei(pkt), dei, "invalid VLAN DEI");
	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");
	zassert_equal(net_pkt_vlan_tag(pkt), 0, "invalid VLAN tag");

	net_pkt_set_vlan_tag(pkt, tag);
	zassert_equal(net_pkt_vlan_tag(pkt), tag, "invalid VLAN tag");
	zassert_equal(net_pkt_vlan_dei(pkt), dei, "invalid VLAN DEI");
	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");

	dei = false;
	net_pkt_set_vlan_dei(pkt, dei);
	zassert_equal(net_pkt_vlan_tag(pkt), tag, "invalid VLAN tag");
	zassert_equal(net_pkt_vlan_dei(pkt), dei, "invalid VLAN DEI");
	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");

	tag = 0U;
	net_pkt_set_vlan_tag(pkt, tag);
	zassert_equal(net_pkt_vlan_tag(pkt), tag, "invalid VLAN tag");
	zassert_equal(net_pkt_vlan_dei(pkt), dei, "invalid VLAN DEI");
	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");

	priority = 0U;
	net_pkt_set_vlan_priority(pkt, priority);
	zassert_equal(net_pkt_vlan_tag(pkt), tag, "invalid VLAN tag");
	zassert_equal(net_pkt_vlan_dei(pkt), dei, "invalid VLAN DEI");
	zassert_equal(net_pkt_vlan_priority(pkt), priority,
		      "invalid VLAN priority");

	zassert_equal(net_pkt_vlan_tci(pkt), 0, "invalid VLAN TCI");

	tci = 0U;
	tag = 100U;
	priority = 3U;

	tci = net_eth_vlan_set_vid(tci, tag);
	tci = net_eth_vlan_set_pcp(tci, priority);

	zassert_equal(tag, net_eth_vlan_get_vid(tci), "Invalid VLAN tag");
	zassert_equal(priority, net_eth_vlan_get_pcp(tci),
		      "Invalid VLAN priority");

	net_pkt_unref(pkt);
}

/* Enable two VLAN tags and verity that proper interfaces are enabled.
 */
static void test_vlan_enable(void)
{
	struct ethernet_context *eth_ctx;
	struct net_if *iface;
	int ret;

	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_1);
	zassert_equal(ret, 0, "Cannot enable %d (%d)", VLAN_TAG_1, ret);
	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_2);
	zassert_equal(ret, 0, "Cannot enable %d (%d)", VLAN_TAG_2, ret);
	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_3);
	zassert_equal(ret, 0, "Cannot enable %d (%d)", VLAN_TAG_3, ret);
	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_4);
	zassert_equal(ret, 0, "Cannot enable %d (%d)", VLAN_TAG_4, ret);

	eth_ctx = net_if_l2_data(eth_interfaces[0]);

	iface = net_eth_get_vlan_iface(eth_interfaces[0], VLAN_TAG_1);
	zassert_equal_ptr(iface, vlan_interfaces[0],
			  "Invalid interface for tag %d (%p vs %p)",
			  VLAN_TAG_1, iface, vlan_interfaces[0]);

	iface = net_eth_get_vlan_iface(eth_interfaces[0], VLAN_TAG_2);
	zassert_equal_ptr(iface, vlan_interfaces[1],
			  "Invalid interface for tag %d (%p vs %p)",
			  VLAN_TAG_2, iface, vlan_interfaces[1]);

	iface = net_eth_get_vlan_iface(eth_interfaces[0], VLAN_TAG_3);
	zassert_equal_ptr(iface, vlan_interfaces[2],
			  "Invalid interface for tag %d (%p vs %p)",
			  VLAN_TAG_3, iface, vlan_interfaces[2]);

	iface = net_eth_get_vlan_iface(eth_interfaces[0], VLAN_TAG_4);
	zassert_equal_ptr(iface, vlan_interfaces[3],
			  "Invalid interface for tag %d (%p vs %p)",
			  VLAN_TAG_4, iface, vlan_interfaces[3]);

	ret = net_eth_is_vlan_enabled(eth_ctx, eth_interfaces[0]);
	zassert_equal(ret, true, "VLAN enabled for interface 0");

	ret = net_eth_is_vlan_enabled(eth_ctx, eth_interfaces[1]);
	zassert_equal(ret, false, "VLAN enabled for interface 1");

	iface = eth_interfaces[0];
	ret = net_eth_vlan_enable(iface, NET_VLAN_TAG_UNSPEC);
	zassert_equal(ret, -EBADF, "Invalid VLAN tag value %d", ret);

	iface = eth_interfaces[1];
	ret = net_eth_vlan_enable(iface, VLAN_TAG_1);
	zassert_equal(ret, -EALREADY, "VLAN tag %d enabled for iface 1 (%d)",
		      VLAN_TAG_1, ret);

	for (int i = VLAN_TAG_1; i <= VLAN_TAG_5; i += 100) {
		iface = net_eth_get_vlan_iface(NULL, i);

		ARRAY_FOR_EACH_PTR(vlan_interfaces, vlan_iface) {
			uint16_t tag;

			ret = net_eth_is_vlan_interface(*vlan_iface);
			zassert_equal(ret, true,
				      "Not identified as VLAN interface %d",
				      net_if_get_by_iface(*vlan_iface));

			if (*vlan_iface == iface) {
				tag = net_eth_get_vlan_tag(*vlan_iface);

				zassert_equal(tag, i,
					      "Could not get the VLAN interface (%d)",
					      net_if_get_by_iface(*vlan_iface));
				break;
			}
		}
	}
}

static void test_vlan_disable(void)
{
	struct ethernet_context *eth_ctx;
	struct net_if *iface;
	int ret;

	ret = net_eth_vlan_disable(eth_interfaces[1], VLAN_TAG_1);
	zassert_not_equal(ret, 0, "Could disable %d (%d)", VLAN_TAG_1, ret);

	ret = net_eth_vlan_disable(eth_interfaces[0], VLAN_TAG_1);
	zassert_equal(ret, 0, "Cannot disable %d (%d)", VLAN_TAG_1, ret);

	ret = net_eth_vlan_disable(eth_interfaces[0], VLAN_TAG_2);
	zassert_equal(ret, 0, "Cannot disable %d (%d)", VLAN_TAG_2, ret);

	ret = net_eth_vlan_disable(eth_interfaces[0], VLAN_TAG_3);
	zassert_equal(ret, 0, "Cannot disable %d (%d)", VLAN_TAG_2, ret);

	ret = net_eth_vlan_disable(vlan_interfaces[3], VLAN_TAG_4);
	zassert_equal(ret, 0, "Cannot disable %d (%d)", VLAN_TAG_2, ret);

	eth_ctx = net_if_l2_data(eth_interfaces[0]);

	iface = net_eth_get_vlan_iface(eth_interfaces[0], VLAN_TAG_1);
	zassert_equal_ptr(iface, NULL, "Valid interface for tag %d", VLAN_TAG_1);

	iface = net_eth_get_vlan_iface(eth_interfaces[0], VLAN_TAG_2);
	zassert_equal_ptr(iface, NULL, "Valid interface for tag %d", VLAN_TAG_2);

	ret = net_eth_is_vlan_enabled(eth_ctx, eth_interfaces[0]);
	zassert_equal(ret, false, "VLAN enabled for interface 0");

	ret = net_eth_is_vlan_enabled(eth_ctx, eth_interfaces[1]);
	zassert_equal(ret, false, "VLAN enabled for interface 1");

	iface = eth_interfaces[0];
	ret = net_eth_vlan_disable(iface, NET_VLAN_TAG_UNSPEC);
	zassert_equal(ret, -EBADF, "Invalid VLAN tag value %d", ret);

	iface = eth_interfaces[1];
	ret = net_eth_vlan_disable(iface, VLAN_TAG_1);
	zassert_equal(ret, -ESRCH, "VLAN tag %d disabled for iface 1",
		      VLAN_TAG_1);
}

static void test_vlan_enable_all(void)
{
	struct ethernet_context *eth_ctx;
	struct net_if *iface;
	int ret;

	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_1);
	zassert_true(ret == 0 || ret == -EALREADY, "Cannot enable %d", VLAN_TAG_1);
	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_2);
	zassert_true(ret == 0 || ret == -EALREADY, "Cannot enable %d", VLAN_TAG_2);
	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_3);
	zassert_true(ret == 0 || ret == -EALREADY, "Cannot enable %d", VLAN_TAG_3);
	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_4);
	zassert_true(ret == 0 || ret == -EALREADY, "Cannot enable %d", VLAN_TAG_4);

	eth_ctx = net_if_l2_data(eth_interfaces[0]);

	ret = net_eth_is_vlan_enabled(eth_ctx, eth_interfaces[0]);
	zassert_equal(ret, true, "VLAN disabled for interface 0");

	ret = net_eth_is_vlan_enabled(eth_ctx, eth_interfaces[0]);
	zassert_equal(ret, true, "VLAN disabled for interface 0");

	ret = net_eth_is_vlan_enabled(eth_ctx, eth_interfaces[0]);
	zassert_equal(ret, true, "VLAN disabled for interface 0");

	ret = net_eth_is_vlan_enabled(eth_ctx, eth_interfaces[0]);
	zassert_equal(ret, true, "VLAN disabled for interface 0");

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "No dummy iface found");

	zassert_equal(net_if_l2(iface), &NET_L2_GET_NAME(DUMMY),
		      "Not a dummy interface");

	ret = net_eth_vlan_enable(iface, VLAN_TAG_5);
	zassert_equal(ret, -EINVAL, "Wrong iface type (%d)", ret);
}

static void test_vlan_disable_all(void)
{
	struct ethernet_context *eth_ctx;
	struct net_if *iface;
	int ret;

	ret = net_eth_vlan_disable(eth_interfaces[0], VLAN_TAG_1);
	zassert_equal(ret, 0, "Cannot disable %d", VLAN_TAG_1);
	ret = net_eth_vlan_disable(eth_interfaces[0], VLAN_TAG_2);
	zassert_equal(ret, 0, "Cannot disable %d", VLAN_TAG_2);
	ret = net_eth_vlan_disable(eth_interfaces[0], VLAN_TAG_3);
	zassert_equal(ret, 0, "Cannot disable %d", VLAN_TAG_3);
	ret = net_eth_vlan_disable(eth_interfaces[0], VLAN_TAG_4);
	zassert_equal(ret, 0, "Cannot disable %d", VLAN_TAG_4);

	eth_ctx = net_if_l2_data(eth_interfaces[0]);

	ret = net_eth_is_vlan_enabled(eth_ctx, vlan_interfaces[0]);
	zassert_equal(ret, false, "VLAN enabled for interface 0");

	ret = net_eth_is_vlan_enabled(eth_ctx, vlan_interfaces[1]);
	zassert_equal(ret, false, "VLAN enabled for interface 1");

	ret = net_eth_is_vlan_enabled(eth_ctx, vlan_interfaces[2]);
	zassert_equal(ret, false, "VLAN enabled for interface 2");

	ret = net_eth_is_vlan_enabled(eth_ctx, vlan_interfaces[3]);
	zassert_equal(ret, false, "VLAN enabled for interface 3");

	iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));
	zassert_not_null(iface, "No dummy iface found");

	zassert_equal(net_if_l2(iface), &NET_L2_GET_NAME(DUMMY),
		      "Not a dummy interface");

	ret = net_eth_vlan_disable(iface, VLAN_TAG_5);
	zassert_equal(ret, -EINVAL, "Wrong iface type (%d)", ret);
}

static bool add_neighbor(struct net_if *iface, struct in6_addr *addr)
{
	struct net_linkaddr *ll_addr;
	struct net_nbr *nbr;

	ll_addr = net_if_get_link_addr(iface);

	nbr = net_ipv6_nbr_add(iface, addr, ll_addr, false,
			       NET_IPV6_NBR_STATE_REACHABLE);
	if (!nbr) {
		DBG("Cannot add dst %s to neighbor cache\n",
		    net_sprint_ipv6_addr(addr));
		return false;
	}

	DBG("Adding dst %s as [%s] to nbr cache\n",
	    net_sprint_ipv6_addr(addr),
	    net_sprint_ll_addr(ll_addr->addr, 6));

	return true;
}

static void comm_sendto_recvfrom(int client_sock,
				 struct sockaddr *client_addr,
				 socklen_t client_addrlen,
				 int server_sock,
				 struct sockaddr *server_addr,
				 socklen_t server_addrlen)
{
	ssize_t sent = 0;

	ARG_UNUSED(server_sock);
	ARG_UNUSED(client_addr);
	ARG_UNUSED(client_addrlen);

	zassert_not_null(client_addr, "null client addr");
	zassert_not_null(server_addr, "null server addr");

	/*
	 * Test client -> server sending
	 */

	sent = zsock_sendto(client_sock, TEST_STR_SMALL, strlen(TEST_STR_SMALL),
			    0, server_addr, server_addrlen);
	zassert_equal(sent, strlen(TEST_STR_SMALL), "sendto failed (%d vs %d)",
		      sent, strlen(TEST_STR_SMALL));

	if (k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface data\n");
		zassert_false(true, "Timeout");
	}

	/* The data verification cannot be done easily because we are in the
	 * same link and our send/recv MAC addresses are the same and the
	 * packet is dropped by core stack.
	 * We cannot use loopback as then VLAN would not be used.
	 * One option would be to make a fully functional network but that is
	 * out of scope for this test application (at least for now).
	 * So do not try to verify data, just check the received tag value.
	 */
}

ZTEST(net_vlan, test_vlan_ipv6_sendto_recvfrom)
{
	struct net_if *iface;
	int ret;
	int client_sock;
	int server_sock;
	struct sockaddr_in6 client_addr;
	struct sockaddr_in6 server_addr;
	struct eth_context *ctx;

	/* Setup the interfaces */
	test_vlan_enable();
	test_vlan_disable_all();

	ret = net_eth_vlan_enable(eth_interfaces[0], VLAN_TAG_1);
	zassert_equal(ret, 0, "Could not enable %d (%d)", VLAN_TAG_1, ret);

	iface = net_eth_get_vlan_iface(eth_interfaces[0], VLAN_TAG_1);
	ret = net_eth_is_vlan_enabled(NULL, eth_interfaces[0]);
	zassert_equal(ret, true, "VLAN not enabled for interface 1");

	ctx = net_if_get_device(eth_interfaces[0])->data;
	ctx->expecting_tag = VLAN_TAG_1;

	net_if_up(eth_interfaces[0]);
	net_if_up(vlan_interfaces[0]);

	prepare_sock_udp_v6(MY_IPV6_ADDR, ANY_PORT, &client_sock, &client_addr);
	prepare_sock_udp_v6(MY_IPV6_ADDR_SRV, SERVER_PORT, &server_sock, &server_addr);

	ret = add_neighbor(iface, &server_addr.sin6_addr);
	zassert_true(ret, "Cannot add neighbor");

	ret = zsock_bind(server_sock,
			 (struct sockaddr *)&server_addr,
			 sizeof(server_addr));
	zassert_equal(ret, 0, "bind failed");

	test_started = true;

	comm_sendto_recvfrom(client_sock,
			     (struct sockaddr *)&client_addr,
			     sizeof(client_addr),
			     server_sock,
			     (struct sockaddr *)&server_addr,
			     sizeof(server_addr));

	ret = zsock_close(client_sock);
	zassert_equal(ret, 0, "close failed");
	ret = zsock_close(server_sock);
	zassert_equal(ret, 0, "close failed");
}

static void *setup(void)
{
	test_vlan_setup();
	test_address_setup();
	return NULL;
}

ZTEST(net_vlan, test_vlan_enable_disable)
{
	test_vlan_enable();
	test_vlan_disable();
}

ZTEST(net_vlan, test_vlan_enable_disable_all)
{
	test_vlan_enable_all();
	test_vlan_disable_all();
}

ZTEST_SUITE(net_vlan, NULL, setup, NULL, NULL, NULL);
