/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define NET_LOG_LEVEL CONFIG_NET_CONFIG_LOG_LEVEL

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, NET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include <zephyr/ztest.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/virtual.h>
#include <zephyr/net/virtual_mgmt.h>
#include <zephyr/net/net_config.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#include "net_init_config.inc"

#if defined(CONFIG_NET_CONFIG_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* The MTU value here is just an arbitrary number for testing purposes */
#define VIRTUAL_TEST_MTU 1024

static int dummy_if_count;
static int eth_if_count;
static int vlan_if_count;
static int virtual_if_count;

/* We should have enough interfaces as set in test-config.yaml. */
static struct net_if *iface1; /* eth */
static struct net_if *iface2; /* eth */
static struct net_if *iface3; /* vlan */
static struct net_if *iface4; /* vlan */
static struct net_if *iface5; /* dummy */
static struct net_if *iface6; /* dummy */
static struct net_if *iface7; /* virtual */
static struct net_if *iface8; /* virtual */

static bool test_started;

struct net_if_test {
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	struct net_linkaddr ll_addr;
};

struct eth_test_context {
	struct net_if *iface;
	uint8_t mac_address[6];
};

struct virtual_test_context {
	struct net_if *iface;
	struct net_if *attached_to;
	bool status;
	bool init_done;
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
		data->mac_addr[5] = sys_rand32_get();
	}

	data->ll_addr.addr = data->mac_addr;
	data->ll_addr.len = 6U;

	return data->mac_addr;
}

static void dummy_iface_init(struct net_if *iface)
{
	uint8_t *mac = net_iface_get_mac(net_if_get_device(iface));

	net_if_set_link_addr(iface, mac, sizeof(struct net_eth_addr),
			     NET_LINK_ETHERNET);
}

static int dev_init(const struct device *dev)
{
	return 0;
}

static int sender_iface(const struct device *dev, struct net_pkt *pkt)
{
	if (!pkt->buffer) {
		DBG("No data to send!\n");
		return -ENODATA;
	}

	if (test_started) {
		DBG("Sending at iface %d %p\n",
		    net_if_get_by_iface(net_pkt_iface(pkt)),
		    net_pkt_iface(pkt));
	}

	return 0;
}

static void eth_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_test_context *ctx = dev->data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_address,
			     sizeof(ctx->mac_address),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static enum ethernet_hw_caps eth_get_capabilities(const struct device *dev)
{
	return ETHERNET_PROMISC_MODE | ETHERNET_HW_VLAN;
}

static int eth_set_config(const struct device *dev,
			  enum ethernet_config_type type,
			  const struct ethernet_config *config)
{
	struct eth_test_context *ctx = dev->data;

	ARG_UNUSED(ctx);

	switch (type) {
	default:
		return -EINVAL;
	}

	return 0;
}

static int eth_dev_init(const struct device *dev)
{
	struct eth_test_context *ctx = dev->data;

	ARG_UNUSED(ctx);

	return 0;
}

static void virtual_test_iface_init(struct net_if *iface)
{
	struct virtual_test_context *ctx = net_if_get_device(iface)->data;
	char name[sizeof("VirtualTest-+##########")];
	static int count;

	if (ctx->init_done) {
		return;
	}

	ctx->iface = iface;
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);

	snprintk(name, sizeof(name), "VirtualTest-%d", count + 1);
	count++;
	net_virtual_set_name(iface, name);
	(void)net_virtual_set_flags(iface, NET_L2_POINT_TO_POINT);

	ctx->init_done = true;
}

static enum virtual_interface_caps
virtual_test_get_capabilities(struct net_if *iface)
{
	ARG_UNUSED(iface);

	return (enum virtual_interface_caps)0;
}

static int virtual_test_interface_start(const struct device *dev)
{
	struct virtual_test_context *ctx = dev->data;

	if (ctx->status) {
		return -EALREADY;
	}

	ctx->status = true;

	LOG_DBG("Starting iface %d", net_if_get_by_iface(ctx->iface));

	/* You can implement here any special action that is needed
	 * when the network interface is coming up.
	 */

	return 0;
}

static int virtual_test_interface_stop(const struct device *dev)
{
	struct virtual_test_context *ctx = dev->data;

	if (!ctx->status) {
		return -EALREADY;
	}

	ctx->status = false;

	LOG_DBG("Stopping iface %d", net_if_get_by_iface(ctx->iface));

	/* You can implement here any special action that is needed
	 * when the network interface is going down.
	 */

	return 0;
}

static int virtual_test_interface_send(struct net_if *iface,
				       struct net_pkt *pkt)
{
	struct virtual_test_context *ctx = net_if_get_device(iface)->data;

	if (ctx->attached_to == NULL) {
		return -ENOENT;
	}

	return net_send_data(pkt);
}

static enum net_verdict virtual_test_interface_recv(struct net_if *iface,
						    struct net_pkt *pkt)
{
	ARG_UNUSED(iface);
	ARG_UNUSED(pkt);

	return NET_CONTINUE;
}

static int virtual_test_interface_attach(struct net_if *virtual_iface,
					 struct net_if *iface)
{
	struct virtual_test_context *ctx = net_if_get_device(virtual_iface)->data;

	LOG_INF("This interface %d/%p attached to %d/%p",
		net_if_get_by_iface(virtual_iface), virtual_iface,
		net_if_get_by_iface(iface), iface);

	ctx->attached_to = iface;

	return 0;
}

static const struct virtual_interface_api virtual_test_iface_api = {
	.iface_api.init = virtual_test_iface_init,

	.get_capabilities = virtual_test_get_capabilities,
	.start = virtual_test_interface_start,
	.stop = virtual_test_interface_stop,
	.send = virtual_test_interface_send,
	.recv = virtual_test_interface_recv,
	.attach = virtual_test_interface_attach,
};

struct net_if_test net_eth_iface1_data;
struct net_if_test net_eth_iface2_data;
struct net_if_test net_vlan_iface3_data;
struct net_if_test net_vlan_iface4_data;
struct net_if_test net_dummy_iface6_data;
struct net_if_test net_dummy_iface7_data;
struct virtual_test_context virtual_test_iface5_data;
struct virtual_test_context virtual_test_iface8_data;

static struct ethernet_api eth_api_funcs = {
	.iface_api.init = eth_iface_init,
	.get_capabilities = eth_get_capabilities,
	.set_config = eth_set_config,
	.send = eth_send,
};

static struct dummy_api dummy_iface_api = {
	.iface_api.init = dummy_iface_init,
	.send = sender_iface,
};

ETH_NET_DEVICE_INIT(eth_1, "eth_1", eth_dev_init, NULL,
		    &net_eth_iface1_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_api_funcs, NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_2, "eth_2", eth_dev_init, NULL,
		    &net_eth_iface2_data, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_api_funcs, NET_ETH_MTU);

NET_DEVICE_INIT_INSTANCE(net_iface6_test, "dummy_6", iface6, dev_init, NULL,
			 &net_dummy_iface6_data, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &dummy_iface_api, DUMMY_L2, DUMMY_L2_CTX_TYPE, 127);

NET_DEVICE_INIT_INSTANCE(net_iface7_test, "dummy_7", iface7, NULL, NULL,
			 &net_dummy_iface7_data, NULL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			 &dummy_iface_api, DUMMY_L2, DUMMY_L2_CTX_TYPE, 127);

NET_VIRTUAL_INTERFACE_INIT(virtual_iface5_test, "virtual_5", NULL, NULL,
			   &virtual_test_iface5_data, NULL,
			   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			   &virtual_test_iface_api, VIRTUAL_TEST_MTU);

NET_VIRTUAL_INTERFACE_INIT(virtual_iface8_test, "virtual_8", NULL, NULL,
			   &virtual_test_iface8_data, NULL,
			   CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
			   &virtual_test_iface_api, VIRTUAL_TEST_MTU);


#if NET_LOG_LEVEL >= LOG_LEVEL_DBG
static const char *iface2str(struct net_if *iface)
{
	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		return "Ethernet";
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		return "Dummy";
	}

	if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		return "Virtual";
	}

	return "<unknown type>";
}
#endif

static void iface_cb(struct net_if *iface, void *user_data)
{
	DBG("Interface %p (%s) [%d]\n", iface, iface2str(iface),
	    net_if_get_by_iface(iface));

	if (net_if_l2(iface) == &NET_L2_GET_NAME(ETHERNET)) {
		const struct ethernet_api *api =
			net_if_get_device(iface)->api;

		/* As native_sim board will introduce another Ethernet
		 * interface, make sure that we only use our own in this test.
		 */
		if (api->get_capabilities != eth_api_funcs.get_capabilities) {
			return;
		}

		switch (eth_if_count) {
		case 0:
			iface1 = iface;
			break;
		case 1:
			iface2 = iface;
			break;
		}

		eth_if_count++;

	} else if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		switch (dummy_if_count) {
		case 0:
			iface5 = iface;
			break;
		case 1:
			iface6 = iface;
			break;
		}

		dummy_if_count++;

	} else if (net_if_l2(iface) == &NET_L2_GET_NAME(VIRTUAL)) {
		const struct virtual_interface_api *api =
			net_if_get_device(iface)->api;
		char name[CONFIG_NET_INTERFACE_NAME_LEN];
		int ret;

		if (api->get_capabilities(iface) & VIRTUAL_INTERFACE_VLAN) {
			switch (vlan_if_count) {
			case 0:
				iface3 = iface;
				break;
			case 1:
				iface4 = iface;
				break;
			}

			vlan_if_count++;
		} else {
			switch (virtual_if_count) {
			case 0:
				iface7 = iface;
				snprintk(name, sizeof(name), "virt%d", virtual_if_count);
				ret = net_if_set_name(iface, name);
				zassert_equal(ret, 0, "Unexpected value (%d) returned", ret);
				break;
			case 1:
				iface8 = iface;
				snprintk(name, sizeof(name), "virt%d", virtual_if_count);
				ret = net_if_set_name(iface, name);
				zassert_equal(ret, 0, "Unexpected value (%d) returned", ret);
				break;
			}

			virtual_if_count++;
		}
	} else {
		zassert_true(false, "Invalid network interface type found");
	}
}

static void *iface_setup(void)
{
	int idx;

	net_if_foreach(iface_cb, NULL);

	idx = net_if_get_by_iface(iface1);
	zassert_true(idx > 0);

	idx = net_if_get_by_iface(iface2);
	zassert_true(idx > 0);

	idx = net_if_get_by_iface(iface3);
	zassert_true(idx > 0);

	idx = net_if_get_by_iface(iface4);
	zassert_true(idx > 0);

	idx = net_if_get_by_iface(iface5);
	zassert_true(idx > 0);

	idx = net_if_get_by_iface(iface6);
	zassert_true(idx > 0);

	idx = net_if_get_by_iface(iface7);
	zassert_true(idx > 0);

	idx = net_if_get_by_iface(iface8);
	zassert_true(idx > 0);

	DBG("Ethernet interfaces:\n"
	    "\t[%d] iface1 %p, [%d] iface2 %p\n",
	    net_if_get_by_iface(iface1), iface1,
	    net_if_get_by_iface(iface2), iface2);

	DBG("VLAN interfaces:\n"
	    "\t[%d] iface3 %p, [%d] iface4 %p\n",
	    net_if_get_by_iface(iface3), iface3,
	    net_if_get_by_iface(iface4), iface4);

#define EXPECTED_VLAN_IFACE_COUNT (CONFIG_NET_VLAN_COUNT)
	zassert_equal(vlan_if_count, EXPECTED_VLAN_IFACE_COUNT,
		      "Invalid number of Ethernet interface found, expected %d got %d",
		      EXPECTED_VLAN_IFACE_COUNT, vlan_if_count);

	zassert_not_null(iface1, "Ethernet interface 1");
	zassert_not_null(iface2, "Ethernet interface 2");
	zassert_not_null(iface3, "VLAN interface 3");
	zassert_not_null(iface4, "VLAN interface 4");

	DBG("Dummy interfaces:\n"
	    "\t[%d] iface5 %p, [%d] iface6 %p\n",
	    net_if_get_by_iface(iface5), iface5,
	    net_if_get_by_iface(iface6), iface6);

	zassert_equal(dummy_if_count, 2,
		      "Invalid number of Dummy interface found, expected %d got %d",
		      2, eth_if_count);

	zassert_not_null(iface5, "Dummy interface 6");
	zassert_not_null(iface6, "Dummy interface 7");

	DBG("Virtual interfaces:\n"
	    "\t[%d] iface7 %p, [%d] iface8 %p\n",
	    net_if_get_by_iface(iface7), iface7,
	    net_if_get_by_iface(iface8), iface8);

	zassert_equal(virtual_if_count, 2,
		      "Invalid number of virtual interface found, expected %d got %d",
		      2, virtual_if_count);

	test_started = true;

	return NULL;
}

static void iface_teardown(void *dummy)
{
	ARG_UNUSED(dummy);

	net_if_down(iface1);
	net_if_down(iface2);
	net_if_down(iface3);
	net_if_down(iface4);
	net_if_down(iface5);
	net_if_down(iface6);
	net_if_down(iface7);
	net_if_down(iface8);
}

static int setup_net_config_test(void)
{
	(void)iface_setup();
	return 0;
}

/* We must setup the network interfaces just before the config library
 * is initializing itself. If we use ztest setup function, then the config
 * library has already ran, and things happens too late and will fail.
 */
#define TEST_NET_CONFIG_INIT_PRIO 85
SYS_INIT(setup_net_config_test, APPLICATION, TEST_NET_CONFIG_INIT_PRIO);

/* Fail the compilation if the network config library is initialized
 * before this code.
 */
BUILD_ASSERT(TEST_NET_CONFIG_INIT_PRIO < CONFIG_NET_CONFIG_INIT_PRIO);

static int get_ifindex(const struct net_init_config_network_interfaces *cfg)
{
	int ifindex = -1;

	/* Both name and device cannot be given at the same time
	 * as then we would not know what device to get. If both
	 * are missing, then the bind-to field will tell which
	 * interface to use.
	 */
	zassert_false(cfg->name == NULL &&
		      cfg->device_name == NULL &&
		      cfg->bind_to == 0,
		      "Cannot find the interface.");

	if (cfg->set_name != NULL) {
		ifindex = net_if_get_by_name(cfg->set_name);
	}

	if (cfg->name != NULL) {
		if (ifindex < 0) {
			ifindex = net_if_get_by_name(cfg->name);
		}
	}

	if (cfg->device_name != NULL) {
		const struct device *dev;
		struct net_if *iface;

		if (ifindex < 0) {
			dev = device_get_binding(cfg->device_name);
			zassert_not_null(dev, "Device %s not found.", cfg->device_name);

			iface = net_if_lookup_by_dev(dev);
			zassert_not_null(iface, "Cannot find interface.");

			ifindex = net_if_get_by_iface(iface);
		}
	}

	if ((cfg->bind_to - 1) > 0) {
		if (ifindex < 0) {
			ifindex = cfg->bind_to - 1;
		}
	}

	zassert_true(ifindex > 0, "Invalid network interface %d\n"
		     "name '%s', new_name '%s', dev '%s', bind-to %d",
		     ifindex, cfg->name ? cfg->name : "?",
		     cfg->set_name ? cfg->set_name : "?",
		     cfg->device_name ? cfg->device_name : "?",
		     cfg->bind_to - 1);

	return ifindex;
}

ZTEST(net_config, test_interface_names)
{
	const struct net_init_config *config;
	int ifindex;

	config = net_config_get_init_config();
	zassert_not_null(config, "Network configuration not found.");

	zassert_true(NET_CONFIG_NETWORK_INTERFACE_COUNT > 0);

	for (int i = 0; i < NET_CONFIG_NETWORK_INTERFACE_COUNT; i++) {
		ifindex = get_ifindex(&config->network_interfaces[i]);
	}
}

#if defined(CONFIG_NET_IPV4)
static void check_ipv4(const struct net_init_config_network_interfaces *cfg)
{
	struct net_if *iface;
	struct in_addr addr;
	int ifindex;
	int ret;

	ARRAY_FOR_EACH(cfg->ipv4.ipv4_addresses, i) {
		struct net_if_addr *ifaddr;
		struct net_if *iface_addr;
		struct sockaddr_in saddr;
		uint8_t netmask_len = 0;

		ifindex = get_ifindex(cfg);
		zassert_true(ifindex > 0, "No interface found for cfg %p", cfg);

		iface = net_if_get_by_index(ifindex);
		zassert_not_null(iface);

		if (cfg->ipv4.ipv4_addresses[i].value == NULL) {
			continue;
		}

		zassert_true(net_ipaddr_mask_parse(cfg->ipv4.ipv4_addresses[i].value,
						   strlen(cfg->ipv4.ipv4_addresses[i].value),
						   (struct sockaddr *)&saddr, &netmask_len),
			     "Cannot parse the address \"%s\"",
			     cfg->ipv4.ipv4_addresses[i].value);

		memcpy(&addr, &saddr.sin_addr, sizeof(addr));

		if (net_ipv4_is_addr_unspecified(&addr)) {
			continue;
		}

		ifaddr = net_if_ipv4_addr_lookup(&addr, &iface_addr);
		zassert_not_null(ifaddr, "Address %s not found.",
				 net_sprint_ipv4_addr(&addr));
		zassert_equal(iface, iface_addr, "Invalid network interface. "
			      "Got %p (%d) expected %p (%d).",
			      iface_addr, net_if_get_by_iface(iface_addr),
			      iface, net_if_get_by_iface(iface));

		if (netmask_len > 0) {
			struct in_addr gen_mask;

			addr = net_if_ipv4_get_netmask_by_addr(iface, &addr);
			gen_mask.s_addr = BIT_MASK(netmask_len);

			zassert_equal(gen_mask.s_addr, addr.s_addr,
				      "Netmask invalid, expecting %s got %s",
				      net_sprint_ipv4_addr(&gen_mask),
				      net_sprint_ipv4_addr(&addr));
		}
	}

	ARRAY_FOR_EACH(cfg->ipv4.ipv4_multicast_addresses, i) {
		struct net_if_mcast_addr *ifmaddr;
		struct net_if *iface_addr = NULL;

		if (cfg->ipv4.ipv4_multicast_addresses[i].value == NULL) {
			continue;
		}

		ret = net_addr_pton(AF_INET, cfg->ipv4.ipv4_multicast_addresses[i].value,
				    &addr);
		zassert_equal(ret, 0, "Cannot convert multicast address \"%s\"",
			      cfg->ipv4.ipv4_multicast_addresses[i].value);

		if (net_ipv4_is_addr_unspecified(&addr)) {
			continue;
		}

		ifmaddr = net_if_ipv4_maddr_lookup(&addr, &iface_addr);
		zassert_not_null(ifmaddr, "Multicast address %s not found.",
				 net_sprint_ipv4_addr(&addr));
		zassert_equal(iface, iface_addr, "Invalid network interface. "
			      "Got %p (%d) expected %p (%d).",
			      iface_addr, net_if_get_by_iface(iface_addr),
			      iface, net_if_get_by_iface(iface));
	}

	if (cfg->ipv4.gateway != NULL) {
		ret = net_addr_pton(AF_INET, cfg->ipv4.gateway, &addr);
		zassert_equal(ret, 0, "Cannot convert gateway address \"%s\"",
			      cfg->ipv4.gateway);

		if (!net_ipv4_is_addr_unspecified(&addr)) {
			zassert_mem_equal(&iface->config.ip.ipv4->gw,
					  &addr,
					  sizeof(struct in_addr),
					  "Mismatch gateway address. "
					  "Expecting %s got %s.",
					  net_sprint_ipv4_addr(&addr),
					  net_sprint_ipv4_addr(&iface->config.ip.ipv4->gw));
		}
	}

	/* We cannot verify default values of TTL and multicast TTL */
	if (cfg->ipv4.time_to_live > 0) {
		zassert_equal(net_if_ipv4_get_ttl(iface), cfg->ipv4.time_to_live,
			      "TTL mismatch, expecting %d got %d",
			      cfg->ipv4.time_to_live, net_if_ipv4_get_ttl(iface));
	}

	if (cfg->ipv4.multicast_time_to_live > 0) {
		zassert_equal(net_if_ipv4_get_mcast_ttl(iface),
			      cfg->ipv4.multicast_time_to_live,
			      "Multicast TTL mismatch, expecting %d got %d",
			      cfg->ipv4.multicast_time_to_live,
			      net_if_ipv4_get_mcast_ttl(iface));
	}

	if (cfg->ipv4.dhcpv4_enabled) {
#if defined(CONFIG_NET_DHCPV4)
		zassert_true((iface->config.dhcpv4.state == NET_DHCPV4_INIT) ||
			     (iface->config.dhcpv4.state == NET_DHCPV4_SELECTING),
			      "DHCPv4 not in correct state, expecting '%s' or '%s' got '%s'",
			      net_dhcpv4_state_name(NET_DHCPV4_INIT),
			      net_dhcpv4_state_name(NET_DHCPV4_SELECTING),
			      net_dhcpv4_state_name(iface->config.dhcpv4.state));
#endif
	}

#if defined(CONFIG_NET_IPV4_AUTO)
	if (cfg->ipv4.ipv4_autoconf_enabled) {
		zassert_equal(iface->config.ipv4auto.state,
			      NET_IPV4_AUTOCONF_ASSIGNED,
			      "IPv4 autoconf not in correct state, expecting '%d' got '%d'",
			      NET_IPV4_AUTOCONF_ASSIGNED,
			      iface->config.ipv4auto.state);
	}
#endif
}
#else
static inline void check_ipv4(const struct net_init_config *cfg, int index)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(index);
}
#endif

#if defined(CONFIG_NET_IPV6)
static void check_ipv6(const struct net_init_config_network_interfaces *cfg)
{
	struct net_if *iface;
	int ifindex;

	ARRAY_FOR_EACH(cfg->ipv6.ipv6_addresses, i) {
		struct net_if_addr *ifaddr;
		struct net_if *iface_addr = NULL;
		struct sockaddr_in6 saddr;
		uint8_t prefix_len = 0;

		ifindex = get_ifindex(cfg);
		zassert_true(ifindex > 0);

		iface = net_if_get_by_index(ifindex);
		zassert_not_null(iface);

		if (cfg->ipv6.ipv6_addresses[i].value == NULL) {
			continue;
		}

		zassert_true(net_ipaddr_mask_parse(cfg->ipv6.ipv6_addresses[i].value,
						   strlen(cfg->ipv6.ipv6_addresses[i].value),
						   (struct sockaddr *)&saddr, &prefix_len),
			     "Cannot parse the address \"%s\"",
			     cfg->ipv6.ipv6_addresses[i].value);

		if (net_ipv6_is_addr_unspecified(&saddr.sin6_addr)) {
			continue;
		}

		ifaddr = net_if_ipv6_addr_lookup(&saddr.sin6_addr, &iface_addr);
		zassert_not_null(ifaddr, "Address %s not found.",
				 net_sprint_ipv6_addr(&saddr.sin6_addr));
		zassert_equal(iface, iface_addr, "Invalid network interface. "
			      "Got %p (%d) expected %p (%d).",
			      iface_addr, net_if_get_by_iface(iface_addr),
			      iface, net_if_get_by_iface(iface));

	}

	ARRAY_FOR_EACH(cfg->ipv6.ipv6_multicast_addresses, i) {
		struct net_if_mcast_addr *ifmaddr;
		struct net_if *iface_addr;
		struct sockaddr_in6 saddr;

		if (cfg->ipv6.ipv6_multicast_addresses[i].value == NULL) {
			continue;
		}

		zassert_true(net_ipaddr_mask_parse(
				     cfg->ipv6.ipv6_multicast_addresses[i].value,
				     strlen(cfg->ipv6.ipv6_multicast_addresses[i].value),
				     (struct sockaddr *)&saddr, NULL),
			     "Cannot parse the address \"%s\"",
			     cfg->ipv6.ipv6_multicast_addresses[i].value);

		if (net_ipv6_is_addr_unspecified(&saddr.sin6_addr)) {
			continue;
		}

		ifmaddr = net_if_ipv6_maddr_lookup(&saddr.sin6_addr, &iface_addr);
		zassert_not_null(ifmaddr, "Multicast address %s not found.",
				 net_sprint_ipv6_addr(&cfg->ipv6.ipv6_multicast_addresses[i]));
		zassert_equal(iface, iface_addr, "Invalid network interface. "
			      "Got %p (%d) expected %p (%d).",
			      iface_addr, net_if_get_by_iface(iface_addr),
			      iface, net_if_get_by_iface(iface));
	}

	ARRAY_FOR_EACH(cfg->ipv6.prefixes, i) {
		struct net_if_ipv6_prefix *prefix;
		struct sockaddr_in6 saddr;

		if (cfg->ipv6.prefixes[i].address == NULL) {
			continue;
		}

		zassert_true(net_ipaddr_mask_parse(
				     cfg->ipv6.prefixes[i].address,
				     strlen(cfg->ipv6.prefixes[i].address),
				     (struct sockaddr *)&saddr, NULL),
			     "Cannot parse the address \"%s\"",
			     cfg->ipv6.prefixes[i].address);

		if (net_ipv6_is_addr_unspecified(&saddr.sin6_addr)) {
			continue;
		}

		prefix = net_if_ipv6_prefix_lookup(iface, &saddr.sin6_addr,
						   cfg->ipv6.prefixes[i].len);
		zassert_not_null(prefix, "Prefix %s/%d not found.",
				 net_sprint_ipv6_addr(&saddr.sin6_addr),
				 cfg->ipv6.prefixes[i].len);
		zassert_equal(prefix->len, cfg->ipv6.prefixes[i].len,
			      "Prefix len differs, expected %u got %u",
			      cfg->ipv6.prefixes[i].len,
			      prefix->len);
		if (cfg->ipv6.prefixes[i].lifetime == 0xffffffff) {
			zassert_true(prefix->is_infinite,
				     "Prefix lifetime not infinite");
		}
	}

	/* We cannot verify default values of hop limit and multicast hop limit */
	if (cfg->ipv6.hop_limit > 0) {
		zassert_equal(net_if_ipv6_get_hop_limit(iface), cfg->ipv6.hop_limit,
			      "hop limit mismatch, expecting %d got %d",
			      cfg->ipv6.hop_limit, net_if_ipv6_get_hop_limit(iface));
	}

	if (cfg->ipv6.multicast_hop_limit > 0) {
		zassert_equal(net_if_ipv6_get_mcast_hop_limit(iface),
			      cfg->ipv6.multicast_hop_limit,
			      "Multicast hop limit mismatch, expecting %d got %d",
			      cfg->ipv6.multicast_hop_limit,
			      net_if_ipv6_get_mcast_hop_limit(iface));
	}

	if (cfg->ipv6.dhcpv6.status) {
#if defined(CONFIG_NET_DHCPV6)
		zassert_true((iface->config.dhcpv6.state == NET_DHCPV6_INIT) ||
			     (iface->config.dhcpv6.state == NET_DHCPV6_SOLICITING),
			      "DHCPv6 not in correct state, expecting '%s' or '%s' got '%s'",
			      net_dhcpv6_state_name(NET_DHCPV6_INIT),
			      net_dhcpv6_state_name(NET_DHCPV6_SOLICITING),
			      net_dhcpv6_state_name(iface->config.dhcpv6.state));
#endif
	}
}
#else
static inline void check_ipv6(const struct net_init_config_iface *cfg)
{
}
#endif

ZTEST(net_config, test_interface_ipv4_addresses)
{
	const struct net_init_config *config;
	int ifindex = 0;

	config = net_config_get_init_config();
	zassert_not_null(config, "Network configuration not found.");

	zassert_true(NET_CONFIG_NETWORK_INTERFACE_COUNT > 0);

	for (int i = 0; i < NET_CONFIG_NETWORK_INTERFACE_COUNT; i++) {
		ifindex = get_ifindex(&config->network_interfaces[i]);

#define IF_ENABLED_IPV4(cfg, i)						\
		COND_CODE_1(CONFIG_NET_IPV4,				\
			    ((cfg)->network_interfaces[i].ipv4.status), \
			    (false))

		if (IF_ENABLED_IPV4(config, i)) {
			check_ipv4(&config->network_interfaces[i]);
		}
	}
}

ZTEST(net_config, test_interface_ipv6_addresses)
{
	const struct net_init_config *config;
	int ifindex = 0;

	config = net_config_get_init_config();
	zassert_not_null(config, "Network configuration not found.");

	zassert_true(NET_CONFIG_NETWORK_INTERFACE_COUNT > 0);

	for (int i = 0; i < NET_CONFIG_NETWORK_INTERFACE_COUNT; i++) {
		ifindex = get_ifindex(&config->network_interfaces[i]);

#define IF_ENABLED_IPV6(cfg, i)						\
		COND_CODE_1(CONFIG_NET_IPV6,				\
			    ((cfg)->network_interfaces[i].ipv6.status),	\
			    (false))

		if (IF_ENABLED_IPV6(config, i)) {
			check_ipv6(&config->network_interfaces[i]);
		}
	}
}

ZTEST(net_config, test_interface_vlan)
{
	const struct net_init_config *config;
	int ifindex = 0;
	int vlan_count = 0;

	config = net_config_get_init_config();
	zassert_not_null(config, "Network configuration not found.");

	ARRAY_FOR_EACH(config->network_interfaces, i) {
		const struct net_init_config_network_interfaces *cfg;
		const struct net_init_config_vlan *vlan;
		struct net_if *iface;
		uint16_t tag;

		cfg = &config->network_interfaces[i];
		ifindex = get_ifindex(cfg);

		vlan = &cfg->vlan;
		if (!vlan->status) {
			continue;
		}

		iface = net_eth_get_vlan_iface(NULL, vlan->tag);
		zassert_equal(net_if_get_by_index(ifindex), iface,
			      "Could not get the VLAN interface (%d)",
			      ifindex);

		tag = net_eth_get_vlan_tag(net_if_get_by_index(ifindex));
		zassert_equal(tag, vlan->tag,
			      "Tag 0x%04x (%d) not set to iface %d (got 0x%04x (%d))",
			      vlan->tag, vlan->tag, ifindex, tag, tag);

		vlan_count++;
	}

	zassert_equal(vlan_count, CONFIG_NET_VLAN_COUNT,
		      "Invalid VLAN count, expecting %d got %d",
		      CONFIG_NET_VLAN_COUNT, vlan_count);
}

/* From subsys/net/lib/config/init.c */
extern enum net_if_flag get_iface_flag(const char *flag_str, bool *clear);

ZTEST(net_config, test_interface_flags)
{
	const struct net_init_config *config;
	int ifindex = 0;

	config = net_config_get_init_config();
	zassert_not_null(config, "Network configuration not found.");

	ARRAY_FOR_EACH(config->network_interfaces, i) {
		const struct net_init_config_network_interfaces *cfg;

		cfg = &config->network_interfaces[i];
		ifindex = get_ifindex(cfg);

		ARRAY_FOR_EACH(cfg->flags, j) {
			enum net_if_flag flag;
			bool status;
			bool clear;

			if (cfg->flags[j].value == NULL ||
			    cfg->flags[j].value[0] == '\0') {
				continue;
			}

			flag = get_iface_flag(cfg->flags[j].value, &clear);
			zassert_not_equal(flag, NET_IF_NUM_FLAGS,
					  "Unknown flag %s",
					  cfg->flags[j].value);

			status = net_if_flag_is_set(net_if_get_by_index(ifindex),
						    flag);
			if (clear) {
				zassert_equal(status, false, "Flag %s (%d) was set",
					      cfg->flags[j].value, flag);
			} else {
				zassert_equal(status, true, "Flag %s (%d) was not set",
					      cfg->flags[j].value, flag);
			}
		}
	}
}

ZTEST_SUITE(net_config, NULL, NULL, NULL, NULL, iface_teardown);
