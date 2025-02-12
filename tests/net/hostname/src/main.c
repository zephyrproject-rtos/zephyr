/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_HOSTNAME_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/random/random.h>
#include <zephyr/sys/printk.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>

#include <zephyr/net/ethernet.h>
#include <zephyr/net/buf.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_NET_HOSTNAME_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* Interface 1 addresses */
static struct in6_addr my_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 1, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct in_addr my_ipv4_addr1 = { { { 192, 0, 2, 1 } } };

/* Extra address is assigned to ll_addr */
static struct in6_addr ll_addr = { { { 0xfe, 0x80, 0x43, 0xb8, 0, 0, 0, 0,
				       0, 0, 0, 0xf2, 0xaa, 0x29, 0x02,
				       0x04 } } };

static struct in6_addr in6addr_mcast = { { { 0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };

static struct net_if *iface1;

static bool test_started;
static struct k_sem wait_data;

#ifdef CONFIG_NET_MGMT_EVENT
static struct k_sem wait_hostname;
static struct net_mgmt_event_callback hostname_cb;
#endif

#define WAIT_TIME 250

#define EVENT_HANDLER_INIT_PRIO 55

BUILD_ASSERT(EVENT_HANDLER_INIT_PRIO < CONFIG_NET_INIT_PRIO);

struct net_if_test {
	uint8_t idx;
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

static uint8_t *net_iface_get_mac(const struct device *dev)
{
	struct net_if_test *data = dev->data;

	if (data->mac_addr[2] == 0x00) {
		/* 00-00-5E-00-53-xx Documentation RFC 7042 */
		data->mac_addr[0] = 0x00;
		data->mac_addr[1] = 0x00;
		data->mac_addr[2] = 0x5E;
		data->mac_addr[3] = 0x00;
		data->mac_addr[4] = 0x53;
		data->mac_addr[5] = sys_rand8_get();
	}

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

#ifdef CONFIG_NET_MGMT_EVENT
static void hostname_changed(struct net_mgmt_event_callback *cb,
			     uint32_t mgmt_event, struct net_if *iface)
{
	if (mgmt_event == NET_EVENT_HOSTNAME_CHANGED) {
#ifdef CONFIG_NET_MGMT_EVENT_INFO
		const struct net_event_l4_hostname *info = cb->info;

		if (strncmp(net_hostname_get(), info->hostname, sizeof(info->hostname))) {
			/** Invalid value - do not give the semaphore **/
			return;
		}
#endif
		k_sem_give(&wait_hostname);
	}
}
#endif

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	k_sem_give(&wait_data);

	return 0;
}

struct net_if_test net_iface1_data;

static struct ethernet_api net_iface_api = {
	.iface_api.init = net_iface_init,
	.send = sender_iface,
};

#define _ETH_L2_LAYER ETHERNET_L2
#define _ETH_L2_CTX_TYPE NET_L2_GET_CTX_TYPE(ETHERNET_L2)

NET_DEVICE_INIT_INSTANCE(net_iface1_test,
			 "iface1",
			 iface1,
			 NULL,
			 NULL,
			 &net_iface1_data,
			 NULL,
			 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &net_iface_api,
			 _ETH_L2_LAYER,
			 _ETH_L2_CTX_TYPE,
			 127);

struct eth_fake_context {
	struct net_if *iface;
	uint8_t mac_address[6];
	bool promisc_mode;
};

static struct eth_fake_context eth_fake_data;

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	ctx->mac_address[0] = 0x00;
	ctx->mac_address[1] = 0x00;
	ctx->mac_address[2] = 0x5E;
	ctx->mac_address[3] = 0x00;
	ctx->mac_address[4] = 0x53;
	ctx->mac_address[5] = sys_rand8_get();

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev,
			 struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.send = eth_fake_send,
};

static int eth_fake_init(const struct device *dev)
{
	struct eth_fake_context *ctx = dev->data;

	ctx->promisc_mode = false;

	return 0;
}

ETH_NET_DEVICE_INIT(eth_fake, "eth_fake", eth_fake_init, NULL,
		    &eth_fake_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static const char *iface2str(struct net_if *iface)
{
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}

	return "<unknown type>";
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
	static int if_count;

	DBG("Interface %p (%s) [%d]\n", iface, iface2str(iface),
	    net_if_get_by_iface(iface));

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		switch (if_count) {
		case 0:
			iface1 = iface;
			break;
		}

		if_count++;
	}
}

static void *test_iface_setup(void)
{
	struct net_if_mcast_addr *maddr;
	struct net_if_addr *ifaddr;
	int idx;

	/* The semaphore is there to wait the data to be received. */
	k_sem_init(&wait_data, 0, UINT_MAX);

	net_if_foreach(iface_cb, NULL);

	idx = net_if_get_by_iface(iface1);
	((struct net_if_test *)
	 net_if_get_device(iface1)->data)->idx = idx;

	DBG("Interfaces: [%d] iface1 %p\n",
	    net_if_get_by_iface(iface1), iface1);

	zassert_not_null(iface1, "Interface 1");

	ifaddr = net_if_ipv6_addr_add(iface1, &my_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv6 address %s\n",
		       net_sprint_ipv6_addr(&my_addr1));
		zassert_not_null(ifaddr, "addr1");
	}

	ifaddr = net_if_ipv4_addr_add(iface1, &my_ipv4_addr1,
				      NET_ADDR_MANUAL, 0);
	if (!ifaddr) {
		DBG("Cannot add IPv4 address %s\n",
		       net_sprint_ipv4_addr(&my_ipv4_addr1));
		zassert_not_null(ifaddr, "ipv4 addr1");
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

	net_ipv6_addr_create(&in6addr_mcast, 0xff02, 0, 0, 0, 0, 0, 0, 0x0001);

	maddr = net_if_ipv6_maddr_add(iface1, &in6addr_mcast);
	if (!maddr) {
		DBG("Cannot add multicast IPv6 address %s\n",
		       net_sprint_ipv6_addr(&in6addr_mcast));
		zassert_not_null(maddr, "mcast");
	}

	net_if_up(iface1);

	test_started = true;

	return NULL;
}

static int bytes_from_hostname_unique(uint8_t *buf, int buf_len, const char *src)
{
	unsigned int i;

	(void)memset(buf, 0, buf_len);

	if ((2 * buf_len) < strlen(src)) {
		return -ENOMEM;
	}

	for (i = 0U; i < strlen(src); i++) {
		buf[i/2] <<= 4;

		if (src[i] >= '0' && src[i] <= '9') {
			buf[i/2] += (src[i] - '0');
			continue;
		}

		if (src[i] >= 'A' && src[i] <= 'F') {
			buf[i/2] += (10 + (src[i] - 'A'));
			continue;
		}

		if (src[i] >= 'a' && src[i] <= 'f') {
			buf[i/2] += (10 + (src[i] - 'a'));
			continue;
		}

		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_NET_MGMT_EVENT
static int init_event_handler(void)
{
	k_sem_init(&wait_hostname, 0, K_SEM_MAX_LIMIT);

	net_mgmt_init_event_callback(&hostname_cb, hostname_changed,
				NET_EVENT_HOSTNAME_CHANGED);
	net_mgmt_add_event_callback(&hostname_cb);

	return 0;
}
#endif

ZTEST(net_hostname, test_hostname_get)
{
	const char *hostname;
	const char *config_hostname = CONFIG_NET_HOSTNAME;

	hostname = net_hostname_get();

	zassert_mem_equal(hostname, config_hostname,
			  sizeof(CONFIG_NET_HOSTNAME) - 1, "");

	if (IS_ENABLED(CONFIG_NET_HOSTNAME_UNIQUE)) {
		char mac[6];
		int ret;

		ret = bytes_from_hostname_unique(mac, sizeof(mac),
				 hostname + sizeof(CONFIG_NET_HOSTNAME) - 1);
		zassert_equal(ret, 0, "");
		zassert_mem_equal(mac, net_if_get_link_addr(iface1)->addr,
				  net_if_get_link_addr(iface1)->len, "");
	}
}

ZTEST(net_hostname, test_hostname_set)
{
	if (IS_ENABLED(CONFIG_NET_HOSTNAME_UNIQUE)) {
		int ret;

		ret = net_hostname_set_postfix("foobar", sizeof("foobar") - 1);
		zassert_equal(ret, -EALREADY,
			      "Could set hostname postfix (%d)", ret);
	}

	if (IS_ENABLED(CONFIG_NET_HOSTNAME_DYNAMIC)) {
		int ret;

		ret = net_hostname_set("foobar", sizeof("foobar") - 1);
		zassert_equal(ret, 0, "Could not set hostname (%d)", ret);
		zassert_mem_equal("foobar", net_hostname_get(), sizeof("foobar")-1);
	}
}

#ifdef CONFIG_NET_MGMT_EVENT
ZTEST(net_hostname, test_hostname_event)
{
	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT)) {
		int ret;

		ret = k_sem_take(&wait_hostname, K_NO_WAIT);
		zassert_equal(ret, 0, "");

		if (IS_ENABLED(CONFIG_NET_HOSTNAME_UNIQUE)) {
			ret = k_sem_take(&wait_hostname, K_NO_WAIT);
			zassert_equal(ret, 0, "");
		}
	}
}

/** Make sure that hostname related events are caught from the beginning  **/
SYS_INIT(init_event_handler, POST_KERNEL, EVENT_HANDLER_INIT_PRIO);
#endif

ZTEST_SUITE(net_hostname, NULL, test_iface_setup, NULL, NULL, NULL);
