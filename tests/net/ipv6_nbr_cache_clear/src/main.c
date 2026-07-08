/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Test that the IPv6 neighbor cache is cleared when the interface link goes
 * down: non-static entries must be removed (so they are re-resolved once the
 * link is back), while static entries must survive.
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_linkaddr.h>
#include <zephyr/net/dummy.h>

#include "ipv6.h"

static struct net_if *iface;

/* An address to enable IPv6 on the interface (so the link-down path runs). */
static struct net_in6_addr my_addr = {
	{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1}}};
/* A dynamically learned peer, expected to be cleared on link down. */
static struct net_in6_addr peer_addr = {
	{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x2}}};
/* A static peer, expected to survive link down. */
static struct net_in6_addr static_addr = {
	{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x3}}};

struct dummy_dev_data {
	uint8_t mac[6];
};

static struct dummy_dev_data dev_data = {
	.mac = {0x00, 0x00, 0x5e, 0x00, 0x53, 0x2b},
};

static void dummy_iface_init(struct net_if *dev_iface)
{
	struct dummy_dev_data *data = net_if_get_device(dev_iface)->data;

	net_if_set_link_addr(dev_iface, data->mac, sizeof(data->mac), NET_LINK_DUMMY);

	/* Keep the interface down until the test brings it up explicitly. */
	net_if_flag_set(dev_iface, NET_IF_NO_AUTO_START);
}

static int dummy_dev_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static struct dummy_api dummy_api_funcs = {
	.iface_api.init = dummy_iface_init,
	.send = dummy_dev_send,
};

NET_DEVICE_INIT(nbr_clear_dev, "nbr_clear", NULL, NULL, &dev_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dummy_api_funcs, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static void *nbr_clear_setup(void)
{
	iface = net_if_lookup_by_dev(DEVICE_GET(nbr_clear_dev));
	zassert_not_null(iface, "Interface not found");

	return NULL;
}

static void nbr_clear_after(void *fixture)
{
	ARG_UNUSED(fixture);

	(void)net_ipv6_nbr_rm(iface, &peer_addr);
	(void)net_ipv6_nbr_rm(iface, &static_addr);
	(void)net_if_ipv6_addr_rm(iface, &my_addr);

	if (net_if_is_admin_up(iface)) {
		(void)net_if_down(iface);
	}
}

ZTEST(net_ipv6_nbr_cache_clear, test_clear_on_link_down)
{
	struct net_linkaddr lladdr = {
		.type = NET_LINK_DUMMY,
		.len = 6,
		.addr = {0x00, 0x00, 0x5e, 0x00, 0x53, 0xaa},
	};
	struct net_if_addr *ifaddr;
	struct net_nbr *nbr;
	int ret;

	ret = net_if_up(iface);
	zassert_ok(ret, "Cannot bring interface up");

	/* Enable IPv6 on the interface so the link-down path runs. */
	ifaddr = net_if_ipv6_addr_add(iface, &my_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 address");

	/* A dynamically learned neighbor and a static one. */
	nbr = net_ipv6_nbr_add(iface, &peer_addr, &lladdr, false, NET_IPV6_NBR_STATE_STALE);
	zassert_not_null(nbr, "Cannot add dynamic neighbor");
	nbr = net_ipv6_nbr_add(iface, &static_addr, &lladdr, false, NET_IPV6_NBR_STATE_STATIC);
	zassert_not_null(nbr, "Cannot add static neighbor");

	zassert_not_null(net_ipv6_nbr_lookup(iface, &peer_addr), "Dynamic neighbor missing");
	zassert_not_null(net_ipv6_nbr_lookup(iface, &static_addr), "Static neighbor missing");

	/* Bring the link down: the dynamic entry must be cleared, the static
	 * one must survive.
	 */
	ret = net_if_down(iface);
	zassert_ok(ret, "Cannot bring interface down");

	zassert_is_null(net_ipv6_nbr_lookup(iface, &peer_addr),
			"Dynamic neighbor not cleared on link down");
	zassert_not_null(net_ipv6_nbr_lookup(iface, &static_addr),
			 "Static neighbor must survive link down");
}

ZTEST_SUITE(net_ipv6_nbr_cache_clear, NULL, nbr_clear_setup, NULL, nbr_clear_after, NULL);
