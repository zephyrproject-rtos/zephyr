/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_IPV6_LOG_LEVEL);

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/linker/sections.h>
#include <zephyr/random/rand32.h>

#include <ztest.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dummy.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_IPV6_LOG_LEVEL) || defined(CONFIG_NET_IPV4_LOG_LEVEL)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct net_if *default_iface;

#define TEST_BYTE_1(value, expected)				 \
	do {							 \
		char out[3];					 \
		net_byte_to_hex(out, value, 'A', true);		 \
		zassert_false(strcmp(out, expected),		 \
			      "Test 0x%s failed.\n", expected);	 \
	} while (0)

#define TEST_BYTE_2(value, expected)				 \
	do {							 \
		char out[3];					 \
		net_byte_to_hex(out, value, 'a', true);		 \
		zassert_false(strcmp(out, expected),		 \
			      "Test 0x%s failed.\n", expected);	 \
	} while (0)

#define TEST_LL_6(a, b, c, d, e, f, expected)				\
	do {								\
		uint8_t ll[] = { a, b, c, d, e, f };			\
		zassert_false(strcmp(net_sprint_ll_addr(ll, sizeof(ll)),\
				     expected),				\
			      "Test %s failed.\n", expected);		\
	} while (0)

#define TEST_LL_8(a, b, c, d, e, f, g, h, expected)			\
	do {								\
		uint8_t ll[] = { a, b, c, d, e, f, g, h };			\
		zassert_false(strcmp(net_sprint_ll_addr(ll, sizeof(ll)), \
				     expected),				\
			      "Test %s failed.\n", expected);		\
	} while (0)

#define TEST_LL_6_TWO(a, b, c, d, e, f, expected)			\
	do {								\
		uint8_t ll1[] = { a, b, c, d, e, f };			\
		uint8_t ll2[] = { f, e, d, c, b, a };			\
		char out[2 * sizeof("xx:xx:xx:xx:xx:xx") + 1 + 1];	\
		snprintk(out, sizeof(out), "%s ",			\
			 net_sprint_ll_addr(ll1, sizeof(ll1)));		\
		snprintk(out + sizeof("xx:xx:xx:xx:xx:xx"),		\
			 sizeof(out), "%s",				\
			 net_sprint_ll_addr(ll2, sizeof(ll2)));		\
		zassert_false(strcmp(out, expected),			\
			      "Test %s failed, got %s\n", expected,	\
			      out);					\
	} while (0)

#define TEST_IPV6(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, expected)  \
	do {								     \
		struct in6_addr addr = { { { a, b, c, d, e, f, g, h,	     \
					       i, j, k, l, m, n, o, p } } }; \
		char *ptr = net_sprint_ipv6_addr(&addr);		     \
		zassert_false(strcmp(ptr, expected),			     \
			      "Test %s failed, got %s\n", expected,	     \
			      ptr);					     \
	} while (0)

#define TEST_IPV4(a, b, c, d, expected)					\
	do {								\
		struct in_addr addr = { { { a, b, c, d } } };		\
		char *ptr = net_sprint_ipv4_addr(&addr);		\
		zassert_false(strcmp(ptr, expected),			\
			      "Test %s failed, got %s\n", expected,	\
			      ptr);					\
	} while (0)

struct net_test_context {
	uint8_t mac_addr[6];
	struct net_linkaddr ll_addr;
};

int net_test_init(const struct device *dev)
{
	struct net_test_context *net_test_context = dev->data;

	net_test_context = net_test_context;

	return 0;
}

static uint8_t *net_test_get_mac(const struct device *dev)
{
	struct net_test_context *context = dev->data;

	if (context->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		context->mac_addr[0] = 0x00;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x5E;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x53;
		context->mac_addr[5] = sys_rand32_get();
	}

	return context->mac_addr;
}

static void net_test_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 6, NET_LINK_ETHERNET);
}

static int tester_send(const struct device *dev, struct net_pkt *pkt)
{
	return 0;
}

struct net_test_context net_test_context_data;

static struct dummy_api net_test_if_api = {
	.iface_api.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_addr_test1, "net_addr_test1", iface1,
			 net_test_init, NULL,
			 &net_test_context_data, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
			 127);

NET_DEVICE_INIT_INSTANCE(net_addr_test2, "net_addr_test2", iface2,
			 net_test_init, NULL,
			 &net_test_context_data, NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
			 127);

ZTEST(ip_addr_fn, test_ip_addresses)
{
	TEST_BYTE_1(0xde, "DE");
	TEST_BYTE_1(0x09, "09");
	TEST_BYTE_2(0xa9, "a9");
	TEST_BYTE_2(0x80, "80");

	TEST_LL_6(0x12, 0x9f, 0xe3, 0x01, 0x7f, 0x00, "12:9F:E3:01:7F:00");
	TEST_LL_8(0x12, 0x9f, 0xe3, 0x01, 0x7f, 0x00, 0xff, 0x0f, \
		  "12:9F:E3:01:7F:00:FF:0F");

	TEST_LL_6_TWO(0x12, 0x9f, 0xe3, 0x01, 0x7f, 0x00,	\
		      "12:9F:E3:01:7F:00 00:7F:01:E3:9F:12");

	TEST_IPV6(0x20, 1, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, \
		  "2001:db8::1");

	TEST_IPV6(0x20, 0x01, 0x0d, 0xb8, 0x12, 0x34, 0x56, 0x78,	\
		  0x9a, 0xbc, 0xde, 0xf0, 0x01, 0x02, 0x03, 0x04,	\
		  "2001:db8:1234:5678:9abc:def0:102:304");

	TEST_IPV6(0xfe, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	\
		  0x0c, 0xb8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,	\
		  "fe80::cb8:0:0:2");

	TEST_IPV6(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	\
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,	\
		  "::1");

	TEST_IPV6(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	\
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	\
		  "::");

	TEST_IPV4(192, 168, 0, 1, "192.168.0.1");
	TEST_IPV4(0, 0, 0, 0, "0.0.0.0");
	TEST_IPV4(127, 0, 0, 1, "127.0.0.1");
}

ZTEST(ip_addr_fn, test_ipv6_addresses)
{
	struct in6_addr loopback = IN6ADDR_LOOPBACK_INIT;
	struct in6_addr any = IN6ADDR_ANY_INIT;
	struct in6_addr mcast = { { { 0xff, 0x84, 0, 0, 0, 0, 0, 0,
				      0, 0, 0, 0, 0, 0, 0, 0x2 } } };
	struct in6_addr addr6 = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0,
				      0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct in6_addr addr6_pref1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0x1 } } };
	struct in6_addr addr6_pref2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0x2 } } };
	struct in6_addr addr6_pref3 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x64, 0, 0,
					    0, 0, 0, 0, 0, 0, 0, 0, 0x2 } } };
	struct in6_addr *tmp;
	const struct in6_addr *out;
	struct net_if_addr *ifaddr1, *ifaddr2;
	struct net_if_mcast_addr *ifmaddr1;
	struct net_if *iface;
	int i;

	/**TESTPOINT: Check if the IPv6 address is a loopback address*/
	zassert_true(net_ipv6_is_addr_loopback(&loopback),
		     "IPv6 loopback address check failed.");

	/**TESTPOINT: Check if the IPv6 address is a multicast address*/
	zassert_true(net_ipv6_is_addr_mcast(&mcast),
		     "IPv6 multicast address check failed.");

	ifaddr1 = net_if_ipv6_addr_add(default_iface,
				      &addr6,
				      NET_ADDR_MANUAL,
				      0);
	/**TESTPOINT: Check if IPv6 interface address is added*/
	zassert_not_null(ifaddr1, "IPv6 interface address add failed");

	ifaddr2 = net_if_ipv6_addr_lookup(&addr6, NULL);

	/**TESTPOINT: Check if addresses match*/
	zassert_equal_ptr(ifaddr1, ifaddr2,
			  "IPv6 interface address mismatch");

	/**TESTPOINT: Check if the IPv6 address is a loopback address*/
	zassert_false(net_ipv6_is_my_addr(&loopback),
		      "My IPv6 loopback address check failed");

	/**TESTPOINT: Check IPv6 address*/
	zassert_true(net_ipv6_is_my_addr(&addr6),
		     "My IPv6 address check failed");

	/**TESTPOINTS: Check IPv6 prefix*/
	zassert_true(net_ipv6_is_prefix((uint8_t *)&addr6_pref1,
					(uint8_t *)&addr6_pref2, 64),
		     "Same IPv6 prefix test failed");

	zassert_false(net_ipv6_is_prefix((uint8_t *)&addr6_pref1,
					 (uint8_t *)&addr6_pref3, 64),
		      "Different IPv6 prefix test failed");

	zassert_false(net_ipv6_is_prefix((uint8_t *)&addr6_pref1,
					 (uint8_t *)&addr6_pref2, 128),
		      "Different full IPv6 prefix test failed");

	zassert_false(net_ipv6_is_prefix((uint8_t *)&addr6_pref1,
					 (uint8_t *)&addr6_pref3, 255),
		      "Too long prefix test failed");

	ifmaddr1 = net_if_ipv6_maddr_add(default_iface, &mcast);

	/**TESTPOINTS: Check IPv6 addresses*/
	zassert_not_null(ifmaddr1, "IPv6 multicast address add failed");

	ifmaddr1 = net_if_ipv6_maddr_add(default_iface, &addr6);

	zassert_is_null(ifmaddr1,
			"IPv6 multicast address could be added failed");

	zassert_false(memcmp(net_ipv6_unspecified_address(), &any, sizeof(any)),
		      "My IPv6 unspecified address check failed");

	ifaddr2 = net_if_ipv6_addr_add(default_iface,
				       &addr6,
				       NET_ADDR_AUTOCONF,
				       0);
	zassert_not_null(ifaddr2, "IPv6 ll address autoconf add failed");

	ifaddr2->addr_state = NET_ADDR_PREFERRED;

	tmp = net_if_ipv6_get_ll(default_iface, NET_ADDR_PREFERRED);
	zassert_false(tmp && memcmp(tmp, &addr6.s6_addr,
				    sizeof(struct in6_addr)),
		      "IPv6 ll address fetch failed");

	ifaddr2->addr_state = NET_ADDR_DEPRECATED;

	tmp = net_if_ipv6_get_ll(default_iface, NET_ADDR_PREFERRED);
	zassert_false(tmp && !memcmp(tmp, &any, sizeof(struct in6_addr)),
		      "IPv6 preferred ll address fetch failed");

	ifaddr1 = net_if_ipv6_addr_add(default_iface,
				       &addr6_pref2,
				       NET_ADDR_AUTOCONF,
				       0);
	zassert_not_null(ifaddr1, "IPv6 global address autoconf add failed");

	ifaddr1->addr_state = NET_ADDR_PREFERRED;

	/* Two tests for IPv6, first with interface given, then when
	 * iface is NULL
	 */
	for (i = 0, iface = default_iface; i < 2; i++, iface = NULL) {
		ifaddr2->addr_state = NET_ADDR_DEPRECATED;

		out = net_if_ipv6_select_src_addr(iface, &addr6_pref1);
		zassert_not_null(out,
				 "IPv6 src addr selection failed, iface %p\n",
				 iface);

		DBG("Selected IPv6 address %s, iface %p\n",
		       net_sprint_ipv6_addr(out), iface);

		zassert_false(memcmp(out->s6_addr, &addr6_pref2.s6_addr,
				     sizeof(struct in6_addr)),
			      "IPv6 wrong src address selected, iface %p\n",
			      iface);

		/* Now we should get :: address */
		out = net_if_ipv6_select_src_addr(iface, &addr6);
		zassert_not_null(out, "IPv6 src any addr selection failed, "
				 "iface %p\n", iface);

		DBG("Selected IPv6 address %s, iface %p\n",
		       net_sprint_ipv6_addr(out), iface);

		zassert_false(memcmp(out->s6_addr, &any.s6_addr,
				     sizeof(struct in6_addr)),
			      "IPv6 wrong src any address selected, iface %p\n",
			      iface);

		ifaddr2->addr_state = NET_ADDR_PREFERRED;

		/* Now we should get ll address */
		out = net_if_ipv6_select_src_addr(iface, &addr6);
		zassert_not_null(out,  "IPv6 src ll addr selection failed, "
				 "iface %p\n", iface);

		DBG("Selected IPv6 address %s, iface %p\n",
		       net_sprint_ipv6_addr(out), iface);

		zassert_false(memcmp(out->s6_addr, &addr6.s6_addr,
				     sizeof(struct in6_addr)),
			      "IPv6 wrong src ll address selected, iface %p\n",
			      iface);
	}

	zassert_true(net_if_ipv6_addr_rm(default_iface, &addr6),
		     "IPv6 removing address failed\n");
	zassert_true(net_if_ipv6_addr_rm(default_iface, &addr6_pref2),
		     "IPv6 removing address failed\n");
}

ZTEST(ip_addr_fn, test_ipv4_addresses)
{
	const struct in_addr *out;
	struct net_if_addr *ifaddr1;
	struct net_if_mcast_addr *ifmaddr1;
	struct in_addr addr4 = { { { 192, 168, 0, 1 } } };
	struct in_addr addr4b = { { { 192, 168, 1, 2 } } };
	struct in_addr addr4_not_found = { { { 10, 20, 30, 40 } } };
	struct in_addr lladdr4 = { { { 169, 254, 98, 203 } } };
	struct in_addr maddr4a = { { { 224, 0, 0, 1 } } };
	struct in_addr maddr4b = { { { 224, 0, 0, 2 } } };
	struct in_addr match_addr = { { { 192, 168, 0, 2 } } };
	struct in_addr fail_addr = { { { 10, 1, 0, 2 } } };
	struct in_addr netmask = { { { 255, 255, 255, 0 } } };
	struct in_addr netmask2 = { { { 255, 255, 0, 0 } } };
	struct in_addr gw = { { { 192, 168, 0, 42 } } };
	struct in_addr loopback4 = { { { 127, 0, 0, 1 } } };
	struct in_addr bcast_addr1 = { { { 255, 255, 255, 255 } } };
	struct in_addr bcast_addr2 = { { { 192, 168, 1, 255 } } };
	struct in_addr bcast_addr3 = { { { 192, 168, 255, 255 } } };
	struct in_addr bcast_addr4 = { { { 192, 0, 2, 255 } } };
	struct in_addr bcast_addr5 = { { { 192, 168, 0, 255 } } };
	struct net_if *iface, *iface1, *iface2;
	int i, ret;

	ifaddr1 = net_if_ipv4_addr_add(default_iface,
				       &addr4,
				       NET_ADDR_MANUAL,
				       0);
	zassert_not_null(ifaddr1, "IPv4 interface address add failed");

	zassert_true(net_ipv4_is_my_addr(&addr4),
		     "My IPv4 address check failed");

	ifaddr1 = net_if_ipv4_addr_add(default_iface,
				       &lladdr4,
				       NET_ADDR_MANUAL,
				       0);
	zassert_not_null(ifaddr1, "IPv4 interface address add failed");

	zassert_true(net_ipv4_is_my_addr(&lladdr4),
		     "My IPv4 address check failed");

	zassert_false(net_ipv4_is_my_addr(&loopback4),
		      "My IPv4 loopback address check failed");

	/* Two tests for IPv4, first with interface given, then when
	 * iface is NULL
	 */
	for (i = 0, iface = default_iface; i < 2; i++, iface = NULL) {
		out = net_if_ipv4_select_src_addr(iface, &addr4);
		zassert_not_null(out,  "IPv4 src addr selection failed, "
				 "iface %p\n", iface);

		DBG("Selected IPv4 address %s, iface %p\n",
		       net_sprint_ipv4_addr(out), iface);

		zassert_equal(out->s_addr, addr4.s_addr,
			      "IPv4 wrong src address selected, iface %p\n",
			      iface);

		/* Now we should get ll address */
		out = net_if_ipv4_select_src_addr(iface, &lladdr4);
		zassert_not_null(out, "IPv4 src ll addr selection failed, "
				 "iface %p\n", iface);

		DBG("Selected IPv4 address %s, iface %p\n",
		       net_sprint_ipv4_addr(out), iface);

		zassert_equal(out->s_addr, lladdr4.s_addr,
			      "IPv4 wrong src ll address selected, iface %p\n",
			      iface);

		/* Now we should get 192.168.0.1 address */
		out = net_if_ipv4_select_src_addr(iface, &addr4b);
		zassert_not_null(out, "IPv4 src any addr selection failed, "
				 "iface %p\n", iface);

		DBG("Selected IPv4 address %s, iface %p\n",
		       net_sprint_ipv4_addr(out), iface);

		zassert_equal(out->s_addr, addr4.s_addr,
			      "IPv4 wrong src address selected, iface %p\n",
			      iface);

		/* Now we should get 192.168.0.1 address */
		out = net_if_ipv4_select_src_addr(iface, &addr4_not_found);
		zassert_not_null(out, "IPv4 src any addr selection failed, "
				 "iface %p\n", iface);

		DBG("Selected IPv4 address %s, iface %p\n",
		       net_sprint_ipv4_addr(out), iface);

		zassert_equal(out->s_addr, addr4.s_addr,
			      "IPv4 wrong src address selected, iface %p\n",
			      iface);
	}

	iface = default_iface;

	net_if_ipv4_set_gw(iface, &gw);
	net_if_ipv4_set_netmask(iface, &netmask);

	zassert_false(net_ipv4_addr_mask_cmp(iface, &fail_addr),
		"IPv4 wrong match failed");

	zassert_true(net_ipv4_addr_mask_cmp(iface, &match_addr),
		     "IPv4 match failed");

	zassert_true(net_ipv4_is_addr_mcast(&maddr4a),
		     "IPv4 multicast address");

	zassert_true(net_ipv4_is_addr_mcast(&maddr4b),
		     "IPv4 multicast address");

	zassert_false(net_ipv4_is_addr_mcast(&addr4), "IPv4 address");

	zassert_false(net_ipv4_is_addr_mcast(&bcast_addr1), "IPv4 broadcast address");

	ifmaddr1 = net_if_ipv4_maddr_add(default_iface, &maddr4a);
	zassert_not_null(ifmaddr1, "IPv4 multicast address add failed");

	ifmaddr1 = net_if_ipv4_maddr_add(default_iface, &maddr4b);
	zassert_not_null(ifmaddr1, "IPv4 multicast address add failed");

	iface = NULL;

	iface1 = net_if_get_by_index(1);
	iface2 = net_if_get_by_index(2);

	ifmaddr1 = net_if_ipv4_maddr_lookup(&maddr4a, &iface);
	zassert_not_null(ifmaddr1, "IPv4 multicast address lookup failed");
	zassert_equal(iface, iface1, "Interface not found");

	ifmaddr1 = net_if_ipv4_maddr_lookup(&maddr4b, &iface);
	zassert_not_null(ifmaddr1, "IPv4 multicast address lookup failed");
	zassert_equal(iface, iface1, "Interface not found");

	ifmaddr1 = net_if_ipv4_maddr_lookup(&maddr4a, &iface2);
	zassert_is_null(ifmaddr1, "IPv4 multicast address lookup succeed");

	ret = net_if_ipv4_maddr_rm(iface2, &maddr4a);
	zassert_false(ret, "IPv4 rm succeed");

	ret = net_if_ipv4_maddr_rm(iface1, &maddr4a);
	zassert_true(ret, "IPv4 rm failed");

	ifmaddr1 = net_if_ipv4_maddr_lookup(&maddr4a, &iface1);
	zassert_is_null(ifmaddr1, "IPv4 multicast address lookup succeed");

	ret = net_if_ipv4_maddr_rm(iface1, &maddr4b);
	zassert_true(ret, "IPv4 rm failed");

	ifmaddr1 = net_if_ipv4_maddr_lookup(&maddr4b, &iface1);
	zassert_is_null(ifmaddr1, "IPv4 multicast address lookup succeed");

	ret = net_ipv4_is_addr_bcast(iface, &bcast_addr1);
	zassert_true(ret, "IPv4 address 1 is not broadcast address");

	ret = net_ipv4_is_addr_bcast(iface, &bcast_addr2);
	zassert_false(ret, "IPv4 address 2 is broadcast address");

	ret = net_ipv4_is_addr_bcast(iface, &bcast_addr4);
	zassert_false(ret, "IPv4 address 4 is broadcast address");

	ret = net_ipv4_is_addr_bcast(iface, &maddr4b);
	zassert_false(ret, "IPv4 address is broadcast address");

	ret = net_ipv4_is_addr_bcast(iface, &bcast_addr5);
	zassert_true(ret, "IPv4 address 5 is not broadcast address");

	net_if_ipv4_set_netmask(iface, &netmask2);

	ret = net_ipv4_is_addr_bcast(iface, &bcast_addr2);
	zassert_false(ret, "IPv4 address 2 is broadcast address");

	ret = net_ipv4_is_addr_bcast(iface, &bcast_addr3);
	zassert_true(ret, "IPv4 address 3 is not broadcast address");
}

ZTEST(ip_addr_fn, test_ipv6_mesh_addresses)
{
	struct net_if_addr *ifaddr;
	const struct in6_addr *out;
	struct in6_addr lla = { { { 0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0x54, 0xdb,
				    0x88, 0x1c, 0x38, 0x45, 0x57, 0xf4 } } };
	struct in6_addr ml_eid = { { { 0xfd, 0xe5, 0x8d, 0xba, 0x82, 0xe1, 0,
				       0x01, 0x40, 0x16, 0x99, 0x3c, 0x83, 0x99,
				       0x35, 0xab } } };
	struct in6_addr ll_mcast = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					 0, 0, 0, 0, 0x1 } } };
	struct in6_addr ml_mcast = { { { 0xff, 0x03, 0, 0, 0, 0, 0, 0, 0, 0, 0,
					 0, 0, 0, 0, 0x1 } } };
	struct net_if *iface = default_iface;

	ifaddr = net_if_ipv6_addr_add(iface, &lla, NET_ADDR_AUTOCONF, 0);
	zassert_not_null(ifaddr, "IPv6 ll address autoconf add failed");

	ifaddr = net_if_ipv6_addr_add(iface, &ml_eid, NET_ADDR_AUTOCONF, 0);
	zassert_not_null(ifaddr, "IPv6 ll address autoconf add failed");

	ifaddr->is_mesh_local = true;

	zassert_true(net_ipv6_is_addr_mcast_mesh(&ml_mcast),
		     "IPv6 multicast mesh address check failed");

	out = net_if_ipv6_select_src_addr(iface, &ll_mcast);
	zassert_not_null(out, "IPv6 src addr selection failed\n");

	DBG("IPv6: destination: %s - selected %s\n",
	    net_sprint_ipv6_addr(&ll_mcast), net_sprint_ipv6_addr(out));

	zassert_false(memcmp(out->s6_addr, &lla.s6_addr,
			     sizeof(struct in6_addr)),
		      "IPv6 wrong src address selected\n");

	out = net_if_ipv6_select_src_addr(iface, &ml_mcast);
	zassert_not_null(out, "IPv6 src addr selection failed\n");

	DBG("IPv6: destination: %s - selected %s\n",
	    net_sprint_ipv6_addr(&ml_mcast), net_sprint_ipv6_addr(out));

	zassert_false(memcmp(out->s6_addr, &ml_eid.s6_addr,
			     sizeof(struct in6_addr)),
		      "IPv6 wrong src address selected\n");

	zassert_true(net_if_ipv6_addr_rm(iface, &lla),
		     "IPv6 removing address failed\n");
	zassert_true(net_if_ipv6_addr_rm(iface, &ml_eid),
		     "IPv6 removing address failed\n");
}

void *test_setup(void)
{
	default_iface = net_if_get_first_by_type(&NET_L2_GET_NAME(DUMMY));

	return NULL;
}

ZTEST_SUITE(ip_addr_fn, NULL, test_setup, NULL, NULL, NULL);
