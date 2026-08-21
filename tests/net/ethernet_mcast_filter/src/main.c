/* main.c - Ethernet multicast HW filter update tests */

/*
 * Copyright (c) 2026 Fin Maaß
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_L2_ETHERNET_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/ethernet_vlan.h>
#include <zephyr/random/random.h>

#define NET_LOG_ENABLED 1
#include "net_private.h"

#define VLAN_TAG 100

/* Number of filters the fake driver can hold at the same time */
#define MAX_FILTERS 8

/* 224.0.2.63 -> 01:00:5e:00:02:3f */
static const struct net_in_addr mcast_addr4 = { { { 224, 0, 2, 63 } } };
static const struct net_eth_addr mcast_mac4 = {
	{ 0x01, 0x00, 0x5e, 0x00, 0x02, 0x3f } };

/* 239.128.2.63 maps to the very same MAC address as 224.0.2.63, as only the
 * low order 23 bits of the IPv4 address are used (RFC 1112).
 */
static const struct net_in_addr mcast_addr4_same_mac = { { { 239, 128, 2, 63 } } };

/* ff02::1:2 -> 33:33:00:01:00:02 */
static const struct net_in6_addr mcast_addr6 = { { { 0xff, 0x02, 0, 0, 0, 0, 0, 0,
						     0, 0, 0, 0, 0, 1, 0, 2 } } };
static const struct net_eth_addr mcast_mac6 = {
	{ 0x33, 0x33, 0x00, 0x01, 0x00, 0x02 } };

/* Link address used to test the L2 level API directly */
static const struct net_eth_addr other_mac = {
	{ 0x01, 0x00, 0x5e, 0x11, 0x22, 0x33 } };

struct eth_fake_filter {
	struct net_eth_addr mac;
	struct net_if *iface;
	bool used;
};

struct eth_fake_context {
	struct net_if *iface;
	uint8_t mac_addr[sizeof(struct net_eth_addr)];
	enum ethernet_hw_caps caps;

	/* What the driver should return from set_config() */
	int filter_result;

	/* Filters currently installed in the "hardware" */
	struct eth_fake_filter filters[MAX_FILTERS];

	/* Number of accepted set_config(ETHERNET_CONFIG_TYPE_FILTER) calls */
	int set_calls;
};

static struct eth_fake_context eth_filter_context = {
	.caps = ETHERNET_HW_FILTERING | ETHERNET_HW_VLAN,
};

static struct eth_fake_context eth_nofilter_context = {
	.caps = ETHERNET_HW_VLAN,
};

static struct net_if *eth_filter_iface;
static struct net_if *eth_nofilter_iface;
static struct net_if *vlan_iface;

static struct eth_fake_filter *filter_lookup(struct eth_fake_context *ctx,
					     const struct net_eth_addr *mac)
{
	ARRAY_FOR_EACH_PTR(ctx->filters, filter) {
		if (filter->used && memcmp(&filter->mac, mac, sizeof(*mac)) == 0) {
			return filter;
		}
	}

	return NULL;
}

static void filter_install(struct eth_fake_context *ctx, struct net_if *iface,
			   const struct net_eth_addr *mac)
{
	struct eth_fake_filter *filter = filter_lookup(ctx, mac);

	/* Installing the same filter twice is harmless for the hardware, so
	 * do not fail here. Whether the L2 informs us once or twice for the
	 * same MAC address is not something the tests should depend on.
	 */
	if (filter != NULL) {
		return;
	}

	ARRAY_FOR_EACH_PTR(ctx->filters, entry) {
		if (!entry->used) {
			entry->used = true;
			entry->iface = iface;
			memcpy(&entry->mac, mac, sizeof(entry->mac));
			return;
		}
	}

	zassert_unreachable("Too many filters installed");
}

static void filter_uninstall(struct eth_fake_context *ctx,
			     const struct net_eth_addr *mac)
{
	struct eth_fake_filter *filter = filter_lookup(ctx, mac);

	if (filter != NULL) {
		filter->used = false;
	}
}

static void eth_fake_iface_init(struct net_if *iface)
{
	const struct device *dev = net_if_get_device(iface);
	struct eth_fake_context *ctx = dev->data;

	ctx->iface = iface;

	net_if_set_link_addr(iface, ctx->mac_addr, sizeof(ctx->mac_addr),
			     NET_LINK_ETHERNET);

	ethernet_init(iface);
}

static int eth_fake_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static enum ethernet_hw_caps eth_fake_get_capabilities(const struct device *dev,
						       struct net_if *iface)
{
	struct eth_fake_context *ctx = dev->data;

	ARG_UNUSED(iface);

	return ctx->caps;
}

static int eth_fake_set_config(const struct device *dev, struct net_if *iface,
			       enum ethernet_config_type type,
			       const struct ethernet_config *config)
{
	struct eth_fake_context *ctx = dev->data;

	if (type != ETHERNET_CONFIG_TYPE_FILTER) {
		return -ENOTSUP;
	}

	if (config->filter.type != ETHERNET_FILTER_TYPE_DST_MAC_ADDRESS) {
		return -ENOTSUP;
	}

	if (ctx->filter_result < 0) {
		return ctx->filter_result;
	}

	ctx->set_calls++;

	if (config->filter.set) {
		filter_install(ctx, iface, &config->filter.mac_address);
	} else {
		filter_uninstall(ctx, &config->filter.mac_address);
	}

	return 0;
}

static const struct ethernet_api eth_fake_api_funcs = {
	.iface_api.init = eth_fake_iface_init,
	.get_capabilities = eth_fake_get_capabilities,
	.set_config = eth_fake_set_config,
	.send = eth_fake_send,
};

static int eth_fake_init(const struct device *dev)
{
	struct eth_fake_context *ctx = dev->data;

	/* 00-00-5E-00-53-xx Documentation RFC 7042 */
	ctx->mac_addr[0] = 0x00;
	ctx->mac_addr[1] = 0x00;
	ctx->mac_addr[2] = 0x5E;
	ctx->mac_addr[3] = 0x00;
	ctx->mac_addr[4] = 0x53;
	ctx->mac_addr[5] = sys_rand8_get();

	return 0;
}

ETH_NET_DEVICE_INIT(eth_filter_test, "eth_filter_test", eth_fake_init, NULL,
		    &eth_filter_context, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

ETH_NET_DEVICE_INIT(eth_nofilter_test, "eth_nofilter_test", eth_fake_init, NULL,
		    &eth_nofilter_context, NULL, CONFIG_ETH_INIT_PRIORITY,
		    &eth_fake_api_funcs, NET_ETH_MTU);

/**
 * @brief Verify that a filter for @a mac is installed on @a iface.
 *
 * Only the resulting hardware state is checked, not how many times the driver
 * was told about it, so that the checks stay valid if the L2 starts keeping
 * track of the link addresses and informs the driver only once per address.
 */
static void check_filter_installed(struct eth_fake_context *ctx,
				   const struct net_eth_addr *mac,
				   struct net_if *iface)
{
	struct eth_fake_filter *filter = filter_lookup(ctx, mac);

	zassert_not_null(filter, "Filter for %s not installed",
			 net_sprint_ll_addr(mac->addr, sizeof(*mac)));
	zassert_equal(filter->iface, iface, "Filter installed on wrong interface");
}

static void check_filter_not_installed(struct eth_fake_context *ctx,
				       const struct net_eth_addr *mac)
{
	zassert_is_null(filter_lookup(ctx, mac), "Filter for %s still installed",
			net_sprint_ll_addr(mac->addr, sizeof(*mac)));
}

static void check_no_filters(struct eth_fake_context *ctx)
{
	ARRAY_FOR_EACH_PTR(ctx->filters, filter) {
		zassert_false(filter->used, "Filter for %s left installed",
			      net_sprint_ll_addr(filter->mac.addr,
						 sizeof(filter->mac)));
	}
}

static void reset_context(struct eth_fake_context *ctx)
{
	ctx->filter_result = 0;
	ctx->set_calls = 0;
	memset(ctx->filters, 0, sizeof(ctx->filters));
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	const struct device *dev;

	ARG_UNUSED(user_data);

	/* All the interfaces are kept down so that the IP stack does not
	 * add or remove multicast addresses behind our back.
	 */
	net_if_down(iface);

	if (net_if_l2(iface) != &NET_L2_GET_NAME(ETHERNET)) {
		return;
	}

	dev = net_if_get_device(iface);

	if (dev->data == &eth_filter_context) {
		eth_filter_iface = iface;
	} else if (dev->data == &eth_nofilter_context) {
		eth_nofilter_iface = iface;
	}
}

static void *test_setup(void)
{
	int ret;

	net_if_foreach(iface_cb, NULL);

	zassert_not_null(eth_filter_iface, "No interface with HW filtering");
	zassert_not_null(eth_nofilter_iface, "No interface without HW filtering");

	/* The suite setup can be run more than once, so only enable the VLAN
	 * if it is not there yet.
	 */
	if (net_eth_get_vlan_iface(eth_filter_iface, VLAN_TAG) == NULL) {
		ret = net_eth_vlan_enable(eth_filter_iface, VLAN_TAG);
		zassert_equal(ret, 0, "Cannot enable VLAN (%d)", ret);
	}

	vlan_iface = net_eth_get_vlan_iface(eth_filter_iface, VLAN_TAG);
	zassert_not_null(vlan_iface, "No VLAN interface");

	return NULL;
}

static void test_before(void *fixture)
{
	ARG_UNUSED(fixture);

	reset_context(&eth_filter_context);
	reset_context(&eth_nofilter_context);
}

/* Every test must leave the hardware without any filter installed, otherwise
 * the tests would depend on each other once the L2 keeps track of the
 * installed link addresses.
 */
static void test_after(void *fixture)
{
	ARG_UNUSED(fixture);

	check_no_filters(&eth_filter_context);
	check_no_filters(&eth_nofilter_context);
}

ZTEST(net_ethernet_mcast_filter, test_ipv4_maddr_add_rm)
{
	struct net_if_mcast_addr *maddr;
	bool ret;

	maddr = net_if_ipv4_maddr_add(eth_filter_iface, &mcast_addr4);
	zassert_not_null(maddr, "Cannot add IPv4 multicast address");

	/* The driver must be informed as soon as the address is added,
	 * i.e. without waiting for the IGMP join to be sent.
	 */
	check_filter_installed(&eth_filter_context, &mcast_mac4, eth_filter_iface);

	ret = net_if_ipv4_maddr_rm(eth_filter_iface, &mcast_addr4);
	zassert_true(ret, "Cannot remove IPv4 multicast address");

	check_filter_not_installed(&eth_filter_context, &mcast_mac4);
}

ZTEST(net_ethernet_mcast_filter, test_ipv6_maddr_add_rm)
{
	struct net_if_mcast_addr *maddr;
	bool ret;

	maddr = net_if_ipv6_maddr_add(eth_filter_iface, &mcast_addr6);
	zassert_not_null(maddr, "Cannot add IPv6 multicast address");

	check_filter_installed(&eth_filter_context, &mcast_mac6, eth_filter_iface);

	ret = net_if_ipv6_maddr_rm(eth_filter_iface, &mcast_addr6);
	zassert_true(ret, "Cannot remove IPv6 multicast address");

	check_filter_not_installed(&eth_filter_context, &mcast_mac6);
}

ZTEST(net_ethernet_mcast_filter, test_maddr_add_twice)
{
	struct net_if_mcast_addr *maddr;
	bool ret;

	maddr = net_if_ipv4_maddr_add(eth_filter_iface, &mcast_addr4);
	zassert_not_null(maddr, "Cannot add IPv4 multicast address");

	/* Second add only takes a reference, the driver is already set up */
	maddr = net_if_ipv4_maddr_add(eth_filter_iface, &mcast_addr4);
	zassert_not_null(maddr, "Cannot add IPv4 multicast address");

	zassert_equal(eth_filter_context.set_calls, 1,
		      "Expected 1 filter call, got %d",
		      eth_filter_context.set_calls);
	check_filter_installed(&eth_filter_context, &mcast_mac4, eth_filter_iface);

	/* Still referenced, so the filter must stay in place */
	ret = net_if_ipv4_maddr_rm(eth_filter_iface, &mcast_addr4);
	zassert_false(ret, "IPv4 multicast address removed while still in use");

	check_filter_installed(&eth_filter_context, &mcast_mac4, eth_filter_iface);

	ret = net_if_ipv4_maddr_rm(eth_filter_iface, &mcast_addr4);
	zassert_true(ret, "Cannot remove IPv4 multicast address");

	check_filter_not_installed(&eth_filter_context, &mcast_mac4);
}

ZTEST(net_ethernet_mcast_filter, test_maddr_shared_mac)
{
	struct net_if_mcast_addr *maddr;
	bool ret;

	/* Two different IPv4 multicast addresses that map to the same MAC
	 * address. The hardware filter must be there as long as at least one
	 * of them is registered, and must be gone once both are removed.
	 */
	maddr = net_if_ipv4_maddr_add(eth_filter_iface, &mcast_addr4);
	zassert_not_null(maddr, "Cannot add IPv4 multicast address");

	check_filter_installed(&eth_filter_context, &mcast_mac4, eth_filter_iface);

	maddr = net_if_ipv4_maddr_add(eth_filter_iface, &mcast_addr4_same_mac);
	zassert_not_null(maddr, "Cannot add IPv4 multicast address");

	check_filter_installed(&eth_filter_context, &mcast_mac4, eth_filter_iface);

	ret = net_if_ipv4_maddr_rm(eth_filter_iface, &mcast_addr4);
	zassert_true(ret, "Cannot remove IPv4 multicast address");

	/* The filter is still needed by mcast_addr4_same_mac here, but the L2
	 * does not keep track of the link addresses yet, so do not check the
	 * intermediate state.
	 */

	ret = net_if_ipv4_maddr_rm(eth_filter_iface, &mcast_addr4_same_mac);
	zassert_true(ret, "Cannot remove IPv4 multicast address");

	check_filter_not_installed(&eth_filter_context, &mcast_mac4);
}

ZTEST(net_ethernet_mcast_filter, test_maddr_add_without_filtering)
{
	struct net_if_mcast_addr *maddr;
	bool ret;

	/* A driver without HW filtering support must not prevent the
	 * multicast address from being added.
	 */
	maddr = net_if_ipv4_maddr_add(eth_nofilter_iface, &mcast_addr4);
	zassert_not_null(maddr, "Cannot add IPv4 multicast address");

	zassert_equal(eth_nofilter_context.set_calls, 0,
		      "Filter was set on a driver without HW filtering");

	ret = net_if_ipv4_maddr_rm(eth_nofilter_iface, &mcast_addr4);
	zassert_true(ret, "Cannot remove IPv4 multicast address");

	zassert_equal(eth_nofilter_context.set_calls, 0,
		      "Filter was set on a driver without HW filtering");
}

ZTEST(net_ethernet_mcast_filter, test_maddr_add_driver_error)
{
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = eth_filter_iface;
	bool ret;

	eth_filter_context.filter_result = -EIO;

	maddr = net_if_ipv4_maddr_add(eth_filter_iface, &mcast_addr4);
	zassert_is_null(maddr, "IPv4 multicast address added despite driver error");

	maddr = net_if_ipv4_maddr_lookup(&mcast_addr4, &iface);
	zassert_is_null(maddr, "IPv4 multicast address left in use");

	iface = eth_filter_iface;

	maddr = net_if_ipv6_maddr_add(eth_filter_iface, &mcast_addr6);
	zassert_is_null(maddr, "IPv6 multicast address added despite driver error");

	maddr = net_if_ipv6_maddr_lookup(&mcast_addr6, &iface);
	zassert_is_null(maddr, "IPv6 multicast address left in use");

	/* A failed add must not leave anything behind, so adding the same
	 * address again has to reach the driver.
	 */
	eth_filter_context.filter_result = 0;

	maddr = net_if_ipv4_maddr_add(eth_filter_iface, &mcast_addr4);
	zassert_not_null(maddr, "Cannot add IPv4 multicast address");

	check_filter_installed(&eth_filter_context, &mcast_mac4, eth_filter_iface);

	/* A failing driver must not block the removal of an address */
	eth_filter_context.filter_result = -EIO;

	ret = net_if_ipv4_maddr_rm(eth_filter_iface, &mcast_addr4);
	zassert_true(ret, "Cannot remove IPv4 multicast address");

	iface = eth_filter_iface;
	maddr = net_if_ipv4_maddr_lookup(&mcast_addr4, &iface);
	zassert_is_null(maddr, "IPv4 multicast address left in use");

	/* The driver rejected the removal, so clean up its state by hand */
	eth_filter_context.filter_result = 0;
	filter_uninstall(&eth_filter_context, &mcast_mac4);
}

ZTEST(net_ethernet_mcast_filter, test_maddr_add_vlan)
{
	struct net_if_mcast_addr *maddr;
	bool ret;

	/* The filter belongs to the main interface, not to the VLAN one */
	maddr = net_if_ipv4_maddr_add(vlan_iface, &mcast_addr4);
	zassert_not_null(maddr, "Cannot add IPv4 multicast address");

	check_filter_installed(&eth_filter_context, &mcast_mac4, eth_filter_iface);

	ret = net_if_ipv4_maddr_rm(vlan_iface, &mcast_addr4);
	zassert_true(ret, "Cannot remove IPv4 multicast address");

	check_filter_not_installed(&eth_filter_context, &mcast_mac4);
}

ZTEST(net_ethernet_mcast_filter, test_mcast_link_addr_update)
{
	struct net_linkaddr lladdr;
	int ret;

	lladdr.type = NET_LINK_ETHERNET;
	lladdr.len = NET_ETH_ADDR_LEN;
	memcpy(lladdr.addr, &other_mac, NET_ETH_ADDR_LEN);

	ret = net_if_mcast_link_addr_update(eth_filter_iface, &lladdr, true);
	zassert_equal(ret, 0, "Cannot add link address filter (%d)", ret);
	check_filter_installed(&eth_filter_context, &other_mac, eth_filter_iface);

	ret = net_if_mcast_link_addr_update(eth_filter_iface, &lladdr, false);
	zassert_equal(ret, 0, "Cannot remove link address filter (%d)", ret);
	check_filter_not_installed(&eth_filter_context, &other_mac);

	/* A VLAN interface is handled by its main interface */
	ret = net_if_mcast_link_addr_update(vlan_iface, &lladdr, true);
	zassert_equal(ret, 0, "Cannot add link address filter (%d)", ret);
	check_filter_installed(&eth_filter_context, &other_mac, eth_filter_iface);

	ret = net_if_mcast_link_addr_update(vlan_iface, &lladdr, false);
	zassert_equal(ret, 0, "Cannot remove link address filter (%d)", ret);
	check_filter_not_installed(&eth_filter_context, &other_mac);

	/* Interface without HW filtering support */
	ret = net_if_mcast_link_addr_update(eth_nofilter_iface, &lladdr, true);
	zassert_equal(ret, -ENOTSUP, "Unexpected return value %d", ret);
	zassert_equal(eth_nofilter_context.set_calls, 0,
		      "Filter was set on a driver without HW filtering");

	/* Link address that is not an Ethernet one */
	lladdr.type = NET_LINK_DUMMY;

	ret = net_if_mcast_link_addr_update(eth_filter_iface, &lladdr, true);
	zassert_equal(ret, -ENOTSUP, "Unexpected return value %d", ret);

	lladdr.type = NET_LINK_ETHERNET;
	lladdr.len = NET_ETH_ADDR_LEN - 1;

	ret = net_if_mcast_link_addr_update(eth_filter_iface, &lladdr, true);
	zassert_equal(ret, -ENOTSUP, "Unexpected return value %d", ret);

	/* None of the invalid calls should have reached the driver */
	check_filter_not_installed(&eth_filter_context, &other_mac);
}

ZTEST_SUITE(net_ethernet_mcast_filter, NULL, test_setup, test_before, test_after, NULL);
