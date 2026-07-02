/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Tests for CONFIG_NET_IPV6_UNSOLICITED_NA: when an address becomes usable on an
 * up interface, the node should multicast an unsolicited Neighbor Advertisement
 * (RFC 4861 ch 7.2.6) to the all-nodes address with the Override flag set and
 * the target set to that address. Exercised with DAD both enabled (default) and
 * disabled (no_dad variant).
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/dummy.h>

/* From RFC 4861 / subsys/net/ip/icmpv6.h, hard-coded to avoid a private
 * header dependency in the test.
 */
#define ICMPV6_TYPE_NA   136
#define NA_FLAG_OVERRIDE 0x20

static struct net_if *iface;

static struct net_in6_addr test_addr = {
	{{0x20, 0x01, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x1}}};

/* Captured from the last Neighbor Advertisement seen on the wire. */
static K_SEM_DEFINE(na_sem, 0, 1);
static atomic_t na_count;
static struct net_in6_addr captured_dst;
static struct net_in6_addr captured_tgt;
static uint8_t captured_flags;

struct dummy_dev_data {
	uint8_t mac[6];
};

static struct dummy_dev_data dev_data = {
	.mac = {0x00, 0x00, 0x5e, 0x00, 0x53, 0x2a},
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
	struct net_in6_addr dst, tgt;
	uint8_t nexthdr, icmp_type, flags;

	ARG_UNUSED(dev);

	net_pkt_cursor_init(pkt);

	/* IPv6 next header is at offset 6. */
	if (net_pkt_skip(pkt, 6) || net_pkt_read_u8(pkt, &nexthdr)) {
		return 0;
	}

	if (nexthdr != NET_IPPROTO_ICMPV6) {
		return 0;
	}

	/* Skip hop limit (1) + source address (16) to reach the destination
	 * address at offset 24, then read it.
	 */
	if (net_pkt_skip(pkt, 17) || net_pkt_read(pkt, dst.s6_addr, 16)) {
		return 0;
	}

	/* ICMPv6 type is at offset 40. */
	if (net_pkt_read_u8(pkt, &icmp_type)) {
		return 0;
	}

	if (icmp_type != ICMPV6_TYPE_NA) {
		return 0;
	}

	/* Skip code (1) + checksum (2) to reach the NA flags byte. */
	if (net_pkt_skip(pkt, 3) || net_pkt_read_u8(pkt, &flags)) {
		return 0;
	}

	/* Skip the 3 reserved bytes to reach the target address. */
	if (net_pkt_skip(pkt, 3) || net_pkt_read(pkt, tgt.s6_addr, 16)) {
		return 0;
	}

	/* Ignore NAs for other addresses (e.g. the autoconfigured link-local
	 * announced on bring-up); this test only cares about test_addr.
	 */
	if (!net_ipv6_addr_cmp(&tgt, &test_addr)) {
		return 0;
	}

	net_ipaddr_copy(&captured_dst, &dst);
	net_ipaddr_copy(&captured_tgt, &tgt);
	captured_flags = flags;

	atomic_inc(&na_count);
	k_sem_give(&na_sem);

	return 0;
}

static struct dummy_api dummy_api_funcs = {
	.iface_api.init = dummy_iface_init,
	.send = dummy_dev_send,
};

NET_DEVICE_INIT(unsolicited_na_dev, "unsolicited_na", NULL, NULL, &dev_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dummy_api_funcs, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static void *unsolicited_na_setup(void)
{
	iface = net_if_lookup_by_dev(DEVICE_GET(unsolicited_na_dev));
	zassert_not_null(iface, "Interface not found");

	return NULL;
}

/* Leave the interface down after each test so the next one starts from a known
 * state and can bring it up unconditionally.
 */
static void unsolicited_na_after(void *fixture)
{
	ARG_UNUSED(fixture);

	if (net_if_is_up(iface)) {
		(void)net_if_down(iface);
	}

	(void)net_if_ipv6_addr_rm(iface, &test_addr);

	/* Reset the capture state so each test starts clean. */
	k_sem_reset(&na_sem);
	atomic_clear(&na_count);
	captured_dst = (struct net_in6_addr){0};
	captured_tgt = (struct net_in6_addr){0};
	captured_flags = 0;
}

/*
 * Bringing the interface up and adding a unicast address must result in an
 * unsolicited NA to the all-nodes address, target = that address, Override set.
 * With DAD enabled the NA is emitted once the address passes DAD; with DAD
 * disabled it is emitted as soon as the address is added (preferred).
 */
ZTEST(net_ipv6_unsolicited_na, test_unsolicited_na_on_address)
{
	struct net_in6_addr all_nodes;
	struct net_if_addr *ifaddr;
	int ret;

	net_ipv6_addr_create_ll_allnodes_mcast(&all_nodes);

	k_sem_reset(&na_sem);

	ret = net_if_up(iface);
	zassert_ok(ret, "Cannot bring interface up");

	ifaddr = net_if_ipv6_addr_add(iface, &test_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 address");

	/* Allow time for DAD to complete when it is enabled. */
	ret = k_sem_take(&na_sem, K_SECONDS(2));
	zassert_ok(ret, "No unsolicited NA was transmitted");

	zexpect_true(net_ipv6_addr_cmp(&captured_dst, &all_nodes),
		     "Unsolicited NA not sent to the all-nodes address");
	zexpect_true(net_ipv6_addr_cmp(&captured_tgt, &test_addr),
		     "Unsolicited NA target is not our address");
	zexpect_equal(captured_flags & NA_FLAG_OVERRIDE, NA_FLAG_OVERRIDE,
		      "Unsolicited NA is missing the Override flag");
}

/*
 * A link cycle (e.g. a Wi-Fi disconnect and reconnect) must re-announce a
 * still-configured address exactly once. With DAD enabled the address is
 * re-validated as the interface comes back up and announced from DAD
 * completion. (Without DAD there is no such re-validation, so this reconnect
 * re-announce only applies when DAD is enabled.)
 */
ZTEST(net_ipv6_unsolicited_na, test_unsolicited_na_on_link_up)
{
	struct net_if_addr *ifaddr;
	int ret;

	if (!IS_ENABLED(CONFIG_NET_IPV6_DAD)) {
		ztest_test_skip();
	}

	ret = net_if_up(iface);
	zassert_ok(ret, "Cannot bring interface up");

	/* A manual address persists across a link down (only autoconf
	 * addresses are removed), like a static or DHCP address would.
	 */
	ifaddr = net_if_ipv6_addr_add(iface, &test_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 address");

	/* Consume the NA from the initial address configuration. */
	ret = k_sem_take(&na_sem, K_SECONDS(2));
	zassert_ok(ret, "No unsolicited NA after adding the address");

	/* Simulate the disconnect and reconnect. */
	ret = net_if_down(iface);
	zassert_ok(ret, "Cannot bring interface down");

	k_sem_reset(&na_sem);
	atomic_clear(&na_count);

	ret = net_if_up(iface);
	zassert_ok(ret, "Cannot bring interface up");

	ret = k_sem_take(&na_sem, K_SECONDS(2));
	zassert_ok(ret, "No unsolicited NA re-announced on link up");

	zexpect_true(net_ipv6_addr_cmp(&captured_tgt, &test_addr),
		     "Re-announced NA target is not our address");

	/* The address must be announced exactly once on link up, not once per
	 * triggering event (interface up and DAD completion).
	 */
	k_msleep(200);
	zexpect_equal(atomic_get(&na_count), 1,
		      "Expected a single NA on link up, got %d",
		      (int)atomic_get(&na_count));
}

ZTEST_SUITE(net_ipv6_unsolicited_na, NULL, unsolicited_na_setup, NULL, unsolicited_na_after, NULL);
