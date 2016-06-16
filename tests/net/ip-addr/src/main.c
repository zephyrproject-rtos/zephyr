/* main.c - Application main entry point */

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

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <device.h>
#include <init.h>
#include <misc/printk.h>
#include <net/net_core.h>
#include <net/nbuf.h>
#include <net/net_ip.h>

#define NET_DEBUG 1
#include "net_private.h"

#define TEST_BYTE_1(value, expected)				 \
	do {							 \
		char out[3];					 \
		net_byte_to_hex(out, value, 'A', true);		 \
		if (strcmp(out, expected)) {			 \
			printk("Test 0x%s failed.\n", expected); \
			return;					 \
		}						 \
	} while (0)

#define TEST_BYTE_2(value, expected)				 \
	do {							 \
		char out[3];					 \
		net_byte_to_hex(out, value, 'a', true);		 \
		if (strcmp(out, expected)) {			 \
			printk("Test 0x%s failed.\n", expected); \
			return;					 \
		}						 \
	} while (0)

#define TEST_LL_6(a, b, c, d, e, f, expected)				\
	do {								\
		uint8_t ll[] = { a, b, c, d, e, f };			\
		if (strcmp(net_sprint_ll_addr(ll, sizeof(ll)),		\
			   expected)) {					\
			printk("Test %s failed.\n", expected);		\
			return;						\
		}							\
	} while (0)

#define TEST_LL_8(a, b, c, d, e, f, g, h, expected)			\
	do {								\
		uint8_t ll[] = { a, b, c, d, e, f, g, h };		\
		if (strcmp(net_sprint_ll_addr(ll, sizeof(ll)),		\
			   expected)) {					\
			printk("Test %s failed.\n", expected);		\
			return;						\
		}							\
	} while (0)

#define TEST_LL_6_TWO(a, b, c, d, e, f, expected)			\
	do {								\
		uint8_t ll1[] = { a, b, c, d, e, f };			\
		uint8_t ll2[] = { f, e, d, c, b, a };			\
		char out[2 * sizeof("xx:xx:xx:xx:xx:xx") + 1 + 1];	\
		sprintf(out, "%s ",					\
			net_sprint_ll_addr(ll1, sizeof(ll1)));		\
		sprintf(out + sizeof("xx:xx:xx:xx:xx:xx"), "%s",	\
			net_sprint_ll_addr(ll2, sizeof(ll2)));		\
		if (strcmp(out, expected)) {				\
			printk("Test %s failed, got %s\n", expected, out); \
			return;						\
		}							\
	} while (0)

#define TEST_IPV6(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, expected)  \
	do {								     \
		struct in6_addr addr = { { { a, b, c, d, e, f, g, h,	     \
					       i, j, k, l, m, n, o, p } } }; \
		char *ptr = net_sprint_ipv6_addr(&addr);		     \
		if (strcmp(ptr, expected)) {				     \
			printk("Test %s failed, got %s\n", expected, ptr);   \
			return;						     \
		}							     \
	} while (0)

#define TEST_IPV4(a, b, c, d, expected)					\
	do {								\
		struct in_addr addr = { { { a, b, c, d } } };		\
		char *ptr = net_sprint_ipv4_addr(&addr);		\
		if (strcmp(ptr, expected)) {				\
			printk("Test %s failed, got %s\n", expected, ptr); \
			return;						\
		}							\
	} while (0)

struct net_test_context {
	uint8_t mac_addr[6];
	struct net_linkaddr ll_addr;
};

int net_test_init(struct device *dev)
{
	struct net_test_context *net_test_context = dev->driver_data;

	net_test_context = net_test_context;

	return 0;
}

static uint8_t *net_test_get_mac(struct device *dev)
{
	struct net_test_context *context = dev->driver_data;

	if (context->mac_addr[0] == 0x00) {
		/* 10-00-00-00-00 to 10-00-00-00-FF Documentation RFC7042 */
		context->mac_addr[0] = 0x10;
		context->mac_addr[1] = 0x00;
		context->mac_addr[2] = 0x00;
		context->mac_addr[3] = 0x00;
		context->mac_addr[4] = 0x00;
		context->mac_addr[5] = sys_rand32_get();
	}

	return context->mac_addr;
}

static void net_test_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_test_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, 8);
}

static int tester_send(struct net_if *iface, struct net_buf *buf)
{
	net_nbuf_unref(buf);
	return 0;
}

struct net_test_context net_test_context_data;

static struct net_if_api net_test_if_api = {
	.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_addr_test, "net_addr_test",
		net_test_init, &net_test_context_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE, 127);

#ifdef CONFIG_MICROKERNEL
void mainloop(void)
#else
void main(void)
#endif
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
	struct in_addr addr4 = { { { 192, 168, 0, 1 } } };
	struct in_addr match_addr = { { { 192, 168, 0, 2 } } };
	struct in_addr fail_addr = { { { 10, 1, 0, 2 } } };
	struct in_addr netmask = { { { 255, 255, 255, 0 } } };
	struct in_addr gw = { { { 192, 168, 0, 42 } } };
	struct in_addr loopback4 = { { { 127, 0, 0, 1 } } };
	struct net_if *iface;
	int i;

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

	printk("IP address print tests passed\n");

	if (!net_is_ipv6_addr_loopback(&loopback)) {
		printk("IPv6 loopback address check failed.\n");
		return;
	}

	if (!net_is_ipv6_addr_mcast(&mcast)) {
		printk("IPv6 multicast address check failed.\n");
		return;
	}

	ifaddr1 = net_if_ipv6_addr_add(net_if_get_default(),
				      &addr6,
				      NET_ADDR_MANUAL,
				      0);
	if (!ifaddr1) {
		printk("IPv6 interface address add failed\n");
		return;
	}

	ifaddr2 = net_if_ipv6_addr_lookup(&addr6, NULL);
	if (ifaddr1 != ifaddr2) {
		printk("IPv6 interface address mismatch\n");
		return;
	}

	if (net_is_my_ipv6_addr(&loopback)) {
		printk("My IPv6 loopback address check failed\n");
		return;
	}

	if (!net_is_my_ipv6_addr(&addr6)) {
		printk("My IPv6 address check failed\n");
		return;
	}

	if (!net_is_ipv6_prefix((uint8_t *)&addr6_pref1,
				(uint8_t *)&addr6_pref2,
				64)) {
		printk("Same IPv6 prefix test failed\n");
		return;
	}

	if (net_is_ipv6_prefix((uint8_t *)&addr6_pref1,
			       (uint8_t *)&addr6_pref3,
			       64)) {
		printk("Different IPv6 prefix test failed\n");
		return;
	}

	if (net_is_ipv6_prefix((uint8_t *)&addr6_pref1,
			       (uint8_t *)&addr6_pref2,
			       128)) {
		printk("Different full IPv6 prefix test failed\n");
		return;
	}

	if (net_is_ipv6_prefix((uint8_t *)&addr6_pref1,
			       (uint8_t *)&addr6_pref3,
			       255)) {
		printk("Too long prefix test failed\n");
		return;
	}

	ifmaddr1 = net_if_ipv6_maddr_add(net_if_get_default(), &mcast);
	if (!ifmaddr1) {
		printk("IPv6 multicast address add failed\n");
		return;
	}

	ifmaddr1 = net_if_ipv6_maddr_add(net_if_get_default(), &addr6);
	if (ifmaddr1) {
		printk("IPv6 multicast address could be added failed\n");
		return;
	}

	ifaddr1 = net_if_ipv4_addr_add(net_if_get_default(),
				      &addr4,
				      NET_ADDR_MANUAL,
				      0);
	if (!ifaddr1) {
		printk("IPv4 interface address add failed\n");
		return;
	}

	if (!net_is_my_ipv4_addr(&addr4)) {
		printk("My IPv4 address check failed\n");
		return;
	}

	if (net_is_my_ipv4_addr(&loopback4)) {
		printk("My IPv4 loopback address check failed\n");
		return;
	}

	if (memcmp(net_if_ipv6_unspecified_addr(), &any, sizeof(any))) {
		printk("My IPv6 unspecified address check failed\n");
		return;
	}

	ifaddr2 = net_if_ipv6_addr_add(net_if_get_default(),
				       &addr6,
				       NET_ADDR_AUTOCONF,
				       0);
	if (!ifaddr2) {
		printk("IPv6 ll address autoconf add failed\n");
		return;
	}
	ifaddr2->addr_state = NET_ADDR_PREFERRED;

	tmp = net_if_ipv6_get_ll(net_if_get_default(), NET_ADDR_PREFERRED);
	if (memcmp(tmp, &addr6.s6_addr, sizeof(struct in6_addr))) {
		printk("IPv6 ll address fetch failed\n");
		return;
	}

	ifaddr2->addr_state = NET_ADDR_DEPRECATED;

	tmp = net_if_ipv6_get_ll(net_if_get_default(), NET_ADDR_PREFERRED);
	if (tmp || !memcmp(tmp, &any, sizeof(struct in6_addr))) {
		printk("IPv6 preferred ll address fetch failed\n");
		return;
	}

	ifaddr1 = net_if_ipv6_addr_add(net_if_get_default(),
				       &addr6_pref2,
				       NET_ADDR_AUTOCONF,
				       0);
	if (!ifaddr1) {
		printk("IPv6 global address autoconf add failed\n");
		return;
	}
	ifaddr1->addr_state = NET_ADDR_PREFERRED;

	/* Two tests, first with interface given, then when iface is NULL */
	for (i = 0, iface = net_if_get_default(); i < 2; i++, iface = NULL) {
		ifaddr2->addr_state = NET_ADDR_DEPRECATED;

		out = net_if_ipv6_select_src_addr(iface, &addr6_pref1);
		if (!out) {
			printk("IPv6 src addr selection failed, iface %p\n",
				iface);
			return;
		}
		printk("Selected IPv6 address %s, iface %p\n",
		       net_sprint_ipv6_addr(out), iface);

		if (memcmp(out->s6_addr, &addr6_pref2.s6_addr,
			   sizeof(struct in6_addr))) {
			printk("IPv6 wrong src address selected, iface %p\n",
			       iface);
			return;
		}

		/* Now we should get :: address */
		out = net_if_ipv6_select_src_addr(iface, &addr6);
		if (!out) {
			printk("IPv6 src any addr selection failed, "
			       "iface %p\n", iface);
			return;
		}
		printk("Selected IPv6 address %s, iface %p\n",
		       net_sprint_ipv6_addr(out), iface);

		if (memcmp(out->s6_addr, &any.s6_addr,
			   sizeof(struct in6_addr))) {
			printk("IPv6 wrong src any address selected, "
			       "iface %p\n", iface);
			return;
		}

		ifaddr2->addr_state = NET_ADDR_PREFERRED;

		/* Now we should get ll address */
		out = net_if_ipv6_select_src_addr(iface, &addr6);
		if (!out) {
			printk("IPv6 src ll addr selection failed, iface %p\n",
				iface);
			return;
		}
		printk("Selected IPv6 address %s, iface %p\n",
		       net_sprint_ipv6_addr(out), iface);

		if (memcmp(out->s6_addr, &addr6.s6_addr,
			   sizeof(struct in6_addr))) {
			printk("IPv6 wrong src ll address selected, "
			       "iface %p\n", iface);
			return;
		}
	}

	iface = net_if_get_default();

	net_if_ipv4_set_gw(iface, &gw);
	net_if_ipv4_set_netmask(iface, &netmask);

	if (net_ipv4_addr_mask_cmp(iface, &fail_addr)) {
		printk("IPv4 wrong match failed\n");
		return;
	}

	if (!net_ipv4_addr_mask_cmp(iface, &match_addr)) {
		printk("IPv4 match failed\n");
		return;
	}

	printk("IP address checks passed\n");
}
