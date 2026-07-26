/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Regression test for the ABBA deadlock that could occur when two network
 * interfaces are brought up concurrently.
 *
 * Bringing an interface up runs notify_iface_up(), which transmits packets
 * (e.g. the all-nodes MLD report). The TX path runs check_ip() ->
 * net_ipv6_is_my_addr_raw() -> net_if_ipv6_addr_lookup_raw(), which walks and
 * locks *every* interface in turn. In the buggy code this ran while
 * net_if_lock(iface) of the interface being brought up was still held, so two
 * interfaces coming up concurrently could deadlock.
 *
 * To reproduce this deterministically we give the "slow" dummy interface a
 * driver start() callback that sleeps. net_if_up() invokes start() while
 * holding net_if_lock(slow_iface), so the lock is held for the whole sleep.
 * While the slow interface is parked in start(), the test brings the "fast"
 * interface up from another thread. Its bring-up TX iterates the interfaces
 * and blocks on lock(slow); once the slow interface wakes, its own bring-up TX
 * tries to take lock(fast) -> ABBA.
 *
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/dummy.h>

/* How long the slow interface holds net_if_lock() inside its start(). */
#define SLOW_START_SLEEP_MS 300

/*
 * Time allowed for each bring-up thread to finish. The fixed code only blocks
 * for the duration of the slow interface's start() sleep, so this must be
 * larger than SLOW_START_SLEEP_MS, but still finite to catch a real deadlock.
 */
#define BRINGUP_TIMEOUT K_MSEC(SLOW_START_SLEEP_MS + 3000)

static struct net_if *slow_iface;
static struct net_if *fast_iface;

/* Dedicated interface for the solicited-node multicast join regression test,
 * kept separate so it does not perturb the deadlock test above.
 */
static struct net_if *dad_iface;

static struct net_in6_addr slow_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0xaa, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct net_in6_addr fast_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0xbb, 0, 0, 0,
					     0, 0, 0, 0, 0, 0, 0, 0x1 } } };
/* Added while dad_iface is up, so addr_add() must join the solicited-node
 * group on its own (rather than relying on the interface bring-up path).
 */
static struct net_in6_addr dad_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0xcc, 0, 0, 0,
					    0, 0, 0, 0, 0, 0xab, 0xcd, 0xef } } };

static atomic_t slow_start_armed;
static K_SEM_DEFINE(slow_in_start, 0, 1);
static K_SEM_DEFINE(done_slow, 0, 1);
static K_SEM_DEFINE(done_fast, 0, 1);

struct dummy_dev_data {
	uint8_t mac[6];
};

static struct dummy_dev_data slow_data = {
	.mac = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x01 },
};
static struct dummy_dev_data fast_data = {
	.mac = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x02 },
};
static struct dummy_dev_data dad_data = {
	.mac = { 0x00, 0x00, 0x5e, 0x00, 0x53, 0x03 },
};

static void dummy_iface_init(struct net_if *iface)
{
	struct dummy_dev_data *data = net_if_get_device(iface)->data;

	net_if_set_link_addr(iface, data->mac, sizeof(data->mac),
			     NET_LINK_DUMMY);

	/* Keep the interface down until the test brings it up explicitly. */
	net_if_flag_set(iface, NET_IF_NO_AUTO_START);
}

static int dummy_dev_send(const struct device *dev, struct net_pkt *pkt)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(pkt);

	return 0;
}

static int slow_dev_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	if (atomic_get(&slow_start_armed)) {
		/*
		 * net_if_up() holds net_if_lock(slow_iface) across this call.
		 * Let the test know we are here (and thus holding the lock),
		 * then keep holding it for a while so the concurrent bring-up
		 * of the other interface runs inside this window.
		 */
		k_sem_give(&slow_in_start);
		k_msleep(SLOW_START_SLEEP_MS);
	}

	return 0;
}

static int dummy_dev_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 0;
}

static struct dummy_api slow_api = {
	.iface_api.init = dummy_iface_init,
	.send = dummy_dev_send,
	.start = slow_dev_start,
	.stop = dummy_dev_stop,
};

static struct dummy_api fast_api = {
	.iface_api.init = dummy_iface_init,
	.send = dummy_dev_send,
	.stop = dummy_dev_stop,
};

static struct dummy_api dad_api = {
	.iface_api.init = dummy_iface_init,
	.send = dummy_dev_send,
	.stop = dummy_dev_stop,
};

NET_DEVICE_INIT(slow_iface_dev, "slow_iface", NULL, NULL, &slow_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &slow_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

NET_DEVICE_INIT(fast_iface_dev, "fast_iface", NULL, NULL, &fast_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fast_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

NET_DEVICE_INIT(dad_iface_dev, "dad_iface", NULL, NULL, &dad_data, NULL,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &dad_api, DUMMY_L2,
		NET_L2_GET_CTX_TYPE(DUMMY_L2), 127);

static K_THREAD_STACK_DEFINE(slow_stack, 2048);
static K_THREAD_STACK_DEFINE(fast_stack, 2048);
static struct k_thread slow_thread;
static struct k_thread fast_thread;

static void slow_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)net_if_up(slow_iface);
	k_sem_give(&done_slow);
}

static void fast_thread_fn(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	(void)net_if_up(fast_iface);
	k_sem_give(&done_fast);
}

static void *iface_with_mld_setup(void)
{
	struct net_if_addr *ifaddr;

	slow_iface = net_if_lookup_by_dev(DEVICE_GET(slow_iface_dev));
	fast_iface = net_if_lookup_by_dev(DEVICE_GET(fast_iface_dev));
	dad_iface = net_if_lookup_by_dev(DEVICE_GET(dad_iface_dev));

	zassert_not_null(slow_iface, "slow_iface not found");
	zassert_not_null(fast_iface, "fast_iface not found");
	zassert_not_null(dad_iface, "dad_iface not found");

	ifaddr = net_if_ipv6_addr_add(slow_iface, &slow_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 address to slow_iface");

	ifaddr = net_if_ipv6_addr_add(fast_iface, &fast_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 address to fast_iface");

	return NULL;
}

ZTEST(net_iface_with_mld, test_concurrent_bringup_no_deadlock)
{
	int ret;

	/* Arm the slow interface so its start() holds net_if_lock() and sleeps. */
	atomic_set(&slow_start_armed, 1);

	k_thread_create(&slow_thread, slow_stack,
			K_THREAD_STACK_SIZEOF(slow_stack), slow_thread_fn,
			NULL, NULL, NULL, K_PRIO_PREEMPT(5), 0, K_NO_WAIT);

	/* Wait until the slow interface is inside start() holding its lock. */
	ret = k_sem_take(&slow_in_start, K_SECONDS(1));
	zassert_ok(ret, "slow_iface did not reach start()");

	/* Now bring the fast interface up concurrently. */
	k_thread_create(&fast_thread, fast_stack,
			K_THREAD_STACK_SIZEOF(fast_stack), fast_thread_fn,
			NULL, NULL, NULL, K_PRIO_PREEMPT(6), 0, K_NO_WAIT);

	ret = k_sem_take(&done_fast, BRINGUP_TIMEOUT);
	zassert_ok(ret, "fast_iface bring-up did not finish - deadlock with slow_iface");

	ret = k_sem_take(&done_slow, BRINGUP_TIMEOUT);
	zassert_ok(ret, "slow_iface bring-up did not finish - deadlock with fast_iface");

	zassert_true(net_if_is_up(slow_iface), "slow_iface is not up");
	zassert_true(net_if_is_up(fast_iface), "fast_iface is not up");
}

/*
 * Adding a unicast address to an up, multicast-capable interface must join the
 * corresponding solicited-node multicast group (RFC 4291 ch 2.8) so that the
 * node answers neighbor solicitations for it. This must happen regardless of
 * whether Duplicate Address Detection is enabled (CONFIG_NET_IPV6_DAD); a
 * regression once tied the join to DAD being active, leaving addresses added at
 * runtime with DAD disabled unreachable. The no_dad test variant exercises that
 * case.
 */
ZTEST(net_iface_with_mld, test_addr_add_joins_solicited_node)
{
	struct net_in6_addr solicited;
	struct net_if_addr *ifaddr;
	struct net_if_mcast_addr *maddr;
	int ret;

	/* Bring the interface up first, with no address yet, so the join can
	 * only come from the address add below (not the bring-up path).
	 */
	ret = net_if_up(dad_iface);
	zassert_ok(ret, "Cannot bring dad_iface up");

	net_ipv6_addr_create_solicited_node(&dad_addr, &solicited);

	/* The solicited-node group must not be joined yet. */
	maddr = net_if_ipv6_maddr_lookup(&solicited, &dad_iface);
	zassert_is_null(maddr, "Solicited-node group joined before address add");

	ifaddr = net_if_ipv6_addr_add(dad_iface, &dad_addr, NET_ADDR_MANUAL, 0);
	zassert_not_null(ifaddr, "Cannot add IPv6 address to dad_iface");

	/* Adding the address must have joined the solicited-node group. */
	maddr = net_if_ipv6_maddr_lookup(&solicited, &dad_iface);
	zassert_not_null(maddr,
			 "Solicited-node group not joined on address add");

	if (!IS_ENABLED(CONFIG_NET_IPV6_DAD)) {
		/* Without DAD the address must be usable immediately. */
		zassert_equal(ifaddr->addr_state, NET_ADDR_PREFERRED,
			      "Address not preferred when DAD is disabled");
	}

	ret = net_if_ipv6_addr_rm(dad_iface, &dad_addr);
	zassert_true(ret, "Cannot remove IPv6 address from dad_iface");
}

/*
 * Bringing an interface up must autoconfigure its IPv6 link-local address
 * regardless of whether DAD is enabled. The address generation used to be a
 * side effect of starting DAD, so it was skipped entirely when
 * CONFIG_NET_IPV6_DAD was disabled, leaving the interface with no link-local
 * address. The no_dad test variant exercises that case.
 */
ZTEST(net_iface_with_mld, test_bringup_adds_link_local)
{
	const struct net_in6_addr *ll;

	/* Another test in this suite may have brought the interface up
	 * already; net_if_up() returns -EALREADY in that case.
	 */
	if (!net_if_is_up(dad_iface)) {
		int ret = net_if_up(dad_iface);

		zassert_ok(ret, "Cannot bring dad_iface up");
	}

	ll = net_if_ipv6_get_ll(dad_iface, NET_ADDR_ANY_STATE);
	zassert_not_null(ll, "No link-local address configured on interface bring-up");
}

ZTEST_SUITE(net_iface_with_mld, NULL, iface_with_mld_setup, NULL, NULL, NULL);
