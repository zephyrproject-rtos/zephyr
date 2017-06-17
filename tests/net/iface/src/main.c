/* main.c - Application main entry point */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <linker/sections.h>

#include <ztest.h>

#include <net/ethernet.h>
#include <net/buf.h>
#include <net/net_ip.h>
#include <net/net_if.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_DEBUG_IF)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 2 addresses */
static struct in6_addr my_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 2, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Interface 3 addresses */
static struct in6_addr my_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 3, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct in6_addr in6addr_mcast = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct net_if *iface1;
static struct net_if *iface2;
static struct net_if *iface3;

static bool test_failed;
static bool test_started;
static struct k_sem wait_data;

#define WAIT_TIME 250

struct net_if_test {
	u8_t idx;
	u8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static int net_iface_dev_init(struct device *dev)
{
	return 0;
}

static u8_t *net_iface_get_mac(struct device *dev)
{
	struct net_if_test *data = dev->driver_data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = sys_rand32_get();
	}

	data->ll_addr.addr = data->mac_addr;
	data->ll_addr.len = 6;

	return data->mac_addr;
}

static void net_iface_init(struct net_if *iface)
{
	u8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int sender_iface(struct net_if *iface, struct net_pkt *pkt)
{
	if (!pkt->frags) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		struct net_if_test *data = iface->dev->driver_data;

		DBG("Sending at iface %d %p\n", net_if_get_by_iface(iface),
		    iface);

		if (net_pkt_iface(pkt) != iface) {
			DBG("Invalid interface %p, expecting %p\n",
				 net_pkt_iface(pkt), iface);
			test_failed = true;
		}

		if (net_if_get_by_iface(iface) != data->idx) {
			DBG("Invalid interface %d index, expecting %d\n",
				 data->idx, net_if_get_by_iface(iface));
			test_failed = true;
		}
	}

	net_pkt_unref(pkt);

	k_sem_give(&wait_data);

	return 0;
}

struct net_if_test net_iface1_data;
struct net_if_test net_iface2_data;
struct net_if_test net_iface3_data;

static struct net_if_api net_iface_api = {
	.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER DUMMY_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(DUMMY_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 net_iface_dev_init,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

NET_DEVICE_INIT_INSTANCE(net_iface2_test,
			 "iface2",
			 iface2,
			 net_iface_dev_init,
			 &net_iface2_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

NET_DEVICE_INIT_INSTANCE(net_iface3_test,
			 "iface3",
			 iface3,
			 net_iface_dev_init,
			 &net_iface3_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

static void iface_setup(void)
{
	struct net_if_mcast_addr *maddr;
	struct net_if_addr *ifaddr;
	int idx;

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	iface1 = net_if_get_by_index(0);
	iface2 = net_if_get_by_index(1);
	iface3 = net_if_get_by_index(2);

	((struct net_if_test *)iface1->dev->driver_data)->idx = 0;
	((struct net_if_test *)iface2->dev->driver_data)->idx = 1;
	((struct net_if_test *)iface3->dev->driver_data)->idx = 2;

	idx = net_if_get_by_iface(iface1);
	zassert_equal(idx, 0, "Invalid index iface1");

	idx = net_if_get_by_iface(iface2);
	zassert_equal(idx, 1, "Invalid index iface2");

	idx = net_if_get_by_iface(iface3);
	zassert_equal(idx, 2, "Invalid index iface3");

	DBG("Interfaces: [%d] iface1 %p, [%d] iface2 %p, [%d] iface3 %p\n",
	    net_if_get_by_iface(iface1), iface1,
	    net_if_get_by_iface(iface2), iface2,
	    net_if_get_by_iface(iface3), iface3);

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

	ifaddr = net_if_ipv6_addr_add(iface2, &my_addr2,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr2));
		zassert_not_null(ifaddr, "addr2");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	ifaddr = net_if_ipv6_addr_add(iface2, &my_addr3,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr3));
		zassert_not_null(ifaddr, "addr3");
	}

	ifaddr->addr_state = NET_ADDR_PREFERRED;

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface1, &in6addr_mcast);
	if (!maddr) {
		DBG("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_mcast));
		zassert_not_null(maddr, "mcast");
	}

	net_if_up(iface1);
	net_if_up(iface2);
	net_if_up(iface3);

	/* The interface might receive data which might fail the checks
	 * in the iface sending function, so we need to reset the failure
	 * flag.
	 */
	test_failed = false;

	test_started = true;
}

static bool send_iface(struct net_if *iface, int val, bool expect_fail)
{
	static u8_t data[] = { 't', 'e', 's', 't', '\0' };
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_get_reserve_tx(0, K_FOREVER);
	net_pkt_set_iface(pkt, iface);

	net_pkt_append_all(pkt, sizeof(data), data, K_FOREVER);

	ret = net_send_data(pkt);
	if (!expect_fail && ret < 0) {
		DBG("Cannot send test packet (%d)\n", ret);
		return false;
	}

	if (!expect_fail && k_sem_take(&wait_data, WAIT_TIME)) {
		DBG("Timeout while waiting interface %d data\n", val);
		return false;
	}

	return true;
}

static void send_iface1(void)
{
	bool ret;

	DBG("Sending data to iface 1 %p\n", iface1);

	ret = send_iface(iface1, 1, false);

	zassert_true(ret, "iface 1");
}

static void send_iface2(void)
{
	bool ret;

	DBG("Sending data to iface 2 %p\n", iface2);

	ret = send_iface(iface2, 2, false);

	zassert_true(ret, "iface 2");
}

static void send_iface3(void)
{
	bool ret;

	DBG("Sending data to iface 3 %p\n", iface3);

	ret = send_iface(iface3, 3, false);

	zassert_true(ret, "iface 3");
}

static void send_iface1_down(void)
{
	bool ret;

	DBG("Sending data to iface 1 %p while down\n", iface1);

	net_if_down(iface1);

	ret = send_iface(iface1, 1, true);

	zassert_true(ret, "iface 1 down");
}

static void send_iface1_up(void)
{
	bool ret;

	DBG("Sending data to iface 1 %p again\n", iface1);

	net_if_up(iface1);

	ret = send_iface(iface1, 1, false);

	zassert_true(ret, "iface 1 up again");
}

void test_main(void)
{
	ztest_test_suite(net_iface_test,
			 ztest_unit_test(iface_setup),
			 ztest_unit_test(send_iface1),
			 ztest_unit_test(send_iface2),
			 ztest_unit_test(send_iface3),
			 ztest_unit_test(send_iface1_down),
			 ztest_unit_test(send_iface1_up)
			 );

	ztest_run_test_suite(net_iface_test);
}
