/* main.c - Application main entry point */

/*
 * Copyright (c) 2015 Intel Corporation
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
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <sections.h>

#include <tc_util.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>
#include <net/net_context.h>

#include "net_private.h"
#include "icmpv6.h"
#include "ipv6.h"

#if defined(CONFIG_NETWORK_IP_STACK_DEBUG_IPV6)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

static struct in6_addr my_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in6_addr peer_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					 0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct in6_addr mcast_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					  0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* ICMPv6 NS frame (74 bytes) */
static const unsigned char icmpv6_ns_invalid[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x20, 0x3A, 0xFF,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* ICMPv6 NS header starts here */
0x87, 0x00, 0x7B, 0x9C, 0x60, 0x00, 0x00, 0x00,
/* Target Address */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
/* Source link layer address */
0x01, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0xD8,
/* Target link layer address */
0x02, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0xD7,
/* Source link layer address */
0x01, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0xD6,
/* MTU option */
0x05, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0xD5,
};

/* ICMPv6 NS frame (64 bytes) */
static const unsigned char icmpv6_ns_no_sllao[] = {
/* IPv6 header starts here */
0x60, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3A, 0xFF,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
/* ICMPv6 NS header starts here */
0x87, 0x00, 0x7B, 0x9C, 0x60, 0x00, 0x00, 0x00,
/* Target Address */
0x20, 0x01, 0x0D, 0xB8, 0x00, 0x00, 0x00, 0x00,
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
};

static bool test_failed;
static struct nano_sem wait_data;

#define WAIT_TIME (sys_clock_ticks_per_sec / 4)
#define WAIT_TIME_LONG (sys_clock_ticks_per_sec)
#define SENDING 93244
#define MY_PORT 1969
#define PEER_PORT 16233

struct net_test_ipv6 {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

int net_test_dev_init(struct device *dev)
{
	return 0;
}

static uint8_t *net_test_get_mac(struct device *dev)
{
	struct net_test_ipv6 *context = dev->driver_data;

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

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr));
}

static int tester_send(struct net_if *iface, struct net_buf *buf)
{
	if (!buf->frags) {
		TC_ERROR("No data to send!\n");
		return -ENODATA;
	}

	/* TC_PRINT("Data to be sent, len %d\n", net_buf_frags_len(buf)); */

	/* Feed this data back to us */
	if (net_recv_data(iface, buf) < 0) {
		TC_ERROR("Data receive failed.");
		goto out;
	}

	return 0;

out:
	net_nbuf_unref(buf);
	test_failed = true;

	return 0;
}

struct net_test_ipv6 net_test_data;

static struct net_if_api net_test_if_api = {
	.init = net_test_iface_init,
	.send = tester_send,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT(net_test_ipv6, "net_test_ipv6",
		net_test_dev_init, &net_test_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		&net_test_if_api, _ETH_L2_LAYER, _ETH_L2_CTX_TYPE,
		127);

static bool test_init(void)
{
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = net_if_get_default();

	if (!iface) {
		TC_ERROR("Interface is NULL\n");
		return false;
	}

	ifaddr = net_if_ipv6_addr_add(iface, &my_addr,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		TC_ERROR("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr));
		return false;
	}

	net_ipv6_addr_create(&mcast_addr, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface, &mcast_addr);
	if (!maddr) {
		TC_ERROR("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&mcast_addr));
		return false;
	}

	/* The semaphore is there to wait the data to be received. */
	nano_sem_init(&wait_data);

	return true;
}

static bool net_test_send_ns_mcast(void)
{
	int ret;
	struct in6_addr tgt;

	net_ipv6_addr_create_solicited_node(&my_addr, &tgt);

	ret = net_ipv6_send_ns(net_if_get_default(),
			       NULL,
			       &peer_addr,
			       &my_addr,
			       &tgt,
			       false);
	if (ret < 0) {
		TC_ERROR("Cannot send NS (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_test_send_ns(void)
{
	int ret;

	ret = net_ipv6_send_ns(net_if_get_default(),
			       NULL,
			       &peer_addr,
			       &my_addr,
			       &my_addr,
			       false);
	if (ret < 0) {
		TC_ERROR("Cannot send NS (%d)\n", ret);
		return false;
	}

	return true;
}

static bool net_test_nbr_lookup_fail(void)
{
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(net_if_get_default(),
				  &peer_addr);
	if (nbr) {
		TC_ERROR("Neighbor %s found in cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
		return false;
	}

	return true;
}

static bool net_test_nbr_lookup_ok(void)
{
	struct net_nbr *nbr;

	nbr = net_ipv6_nbr_lookup(net_if_get_default(),
				  &peer_addr);
	if (!nbr) {
		TC_ERROR("Neighbor %s not found in cache\n",
			 net_sprint_ipv6_addr(&peer_addr));
		return false;
	}

	return true;
}

static bool net_test_send_ns_extra_options(void)
{
	struct net_buf *buf, *frag;
	struct net_if *iface;
	uint16_t reserve;

	buf = net_nbuf_get_reserve_tx(0);

	NET_ASSERT_INFO(buf, "Out of TX buffers");

	iface = net_if_get_default();

	reserve = net_if_get_ll_reserve(iface, NULL);

	frag = net_nbuf_get_reserve_data(reserve);

	net_buf_frag_add(buf, frag);

	net_nbuf_set_ll_reserve(buf, reserve);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	memcpy(net_buf_add(frag, sizeof(icmpv6_ns_invalid)),
	       icmpv6_ns_invalid, sizeof(icmpv6_ns_invalid));

	if (net_recv_data(iface, buf) < 0) {
		TC_ERROR("Data receive for invalid NS failed.");
		return false;
	}

	return true;
}

static bool net_test_send_ns_no_options(void)
{
	struct net_buf *buf, *frag;
	struct net_if *iface;
	uint16_t reserve;

	buf = net_nbuf_get_reserve_tx(0);

	NET_ASSERT_INFO(buf, "Out of TX buffers");

	iface = net_if_get_default();

	reserve = net_if_get_ll_reserve(iface, NULL);

	frag = net_nbuf_get_reserve_data(reserve);

	net_buf_frag_add(buf, frag);

	net_nbuf_set_ll_reserve(buf, reserve);
	net_nbuf_set_iface(buf, iface);
	net_nbuf_set_family(buf, AF_INET6);
	net_nbuf_set_ip_hdr_len(buf, sizeof(struct net_ipv6_hdr));

	net_nbuf_ll_clear(buf);

	memcpy(net_buf_add(frag, sizeof(icmpv6_ns_no_sllao)),
	       icmpv6_ns_no_sllao, sizeof(icmpv6_ns_no_sllao));

	if (net_recv_data(iface, buf) < 0) {
		TC_ERROR("Data receive for invalid NS failed.");
		return false;
	}

	return true;
}

static bool net_test_prefix_timeout(void)
{
	struct net_if_ipv6_prefix *prefix;
	struct in6_addr addr = { { { 0x20, 1, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 1 } } };
	int len = 64, lifetime = 1;

	prefix = net_if_ipv6_prefix_add(net_if_get_default(),
					&addr, len, lifetime);

	net_if_ipv6_prefix_set_lf(prefix, false);
	net_if_ipv6_prefix_set_timer(prefix, lifetime);

	nano_sem_take(&wait_data, SECONDS(lifetime * 3/2));

	prefix = net_if_ipv6_prefix_lookup(net_if_get_default(),
					   &addr, len);
	if (prefix) {
		TC_ERROR("Prefix %s/%d should have expired",
			 net_sprint_ipv6_addr(&addr), len);
		return false;
	}

	return true;
}

static bool net_test_prefix_timeout_overflow(void)
{
	struct net_if_ipv6_prefix *prefix;
	struct in6_addr addr = { { { 0x20, 1, 0x0d, 0xb8, 0, 0, 0, 0,
				     0, 0, 0, 0, 0, 0, 0, 1 } } };
	int len = 64, lifetime = 0xfffffffe;

	prefix = net_if_ipv6_prefix_add(net_if_get_default(),
					&addr, len, lifetime);

	net_if_ipv6_prefix_set_lf(prefix, false);
	net_if_ipv6_prefix_set_timer(prefix, lifetime);

	if (nano_sem_take(&wait_data, SECONDS(lifetime * 3/2))) {
		TC_ERROR("Prefix %s/%d lock should still be there",
			 net_sprint_ipv6_addr(&addr), len);
		return false;
	}

	if (!net_if_ipv6_prefix_rm(net_if_get_default(), &addr, len)) {
		TC_ERROR("Prefix %s/%d should have been removed",
			 net_sprint_ipv6_addr(&addr), len);
		return false;
	}

	return true;
}

static const struct {
	const char *name;
	bool (*func)(void);
} tests[] = {
	{ "test init", test_init },
	{ "IPv6 send NS mcast", net_test_send_ns_mcast },
	{ "IPv6 neighbor lookup fail", net_test_nbr_lookup_fail },
	{ "IPv6 send NS", net_test_send_ns },
	{ "IPv6 neighbor lookup ok", net_test_nbr_lookup_ok },
	{ "IPv6 send NS extra options", net_test_send_ns_extra_options },
	{ "IPv6 send NS no options", net_test_send_ns_no_options },
	{ "IPv6 prefix timeout", net_test_prefix_timeout },
	{ "IPv6 prefix timeout overflow", net_test_prefix_timeout_overflow },
};

void main(void)
{
	int count, pass;

	for (count = 0, pass = 0; count < ARRAY_SIZE(tests); count++) {
		TC_START(tests[count].name);
		test_failed = false;
		if (!tests[count].func() || test_failed) {
			TC_END(FAIL, "failed\n");
		} else {
			TC_END(PASS, "passed\n");
			pass++;
		}

		fiber_yield();
	}

	TC_END_REPORT(((pass != ARRAY_SIZE(tests)) ? TC_FAIL : TC_PASS));
}
