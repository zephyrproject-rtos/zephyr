/* main.c - Application main entry point */

/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_test, CONFIG_NET_PMTU_LOG_LEVEL);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/linker/sections.h>

#include <zephyr/ztest.h>

#include <zephyr/net/net_if.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/net/dummy.h>
#include <zephyr/net/loopback.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/dplpmtud.h>

#include <zephyr/random/random.h>

#include "../../socket/socket_helpers.h"

#include "route.h"
#include "icmpv6.h"
#include "icmpv4.h"
#include "ipv6.h"
#include "ipv4.h"
#include "dplpmtud_internal.h"
#include "pmtu.h"

#define NET_LOG_ENABLED 1
#include "net_private.h"

#if defined(CONFIG_BOARD_NATIVE_SIM) || defined(CONFIG_BOARD_NATIVE_SIM_NATIVE_64)
#define WAIT_PROPERLY 0
#else
#define WAIT_PROPERLY 1
#endif

#if defined(CONFIG_NET_PMTU_LOG_LEVEL_DBG)
#define DBG(fmt, ...) printk(fmt, ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif

/* This is a helper function to get the MTU value for the given destination.
 * It is implemented in tcp.c file.
 */
extern uint16_t net_tcp_get_mtu(struct net_sockaddr *dst);

/* Small sleep between tests makes sure that the PMTU destination
 * cache entries are separated from each other.
 */
#define SMALL_SLEEP K_MSEC(5)

static struct net_in_addr dest_ipv4_addr1 = { { { 198, 51, 100, 1 } } };
static struct net_in_addr dest_ipv4_addr2 = { { { 198, 51, 100, 2 } } };
static struct net_in_addr dest_ipv4_addr3 = { { { 198, 51, 100, 3 } } };
static struct net_in_addr dest_ipv4_addr4 = { { { 198, 51, 100, 4 } } };
static struct net_in_addr dest_ipv4_addr_not_found = { { { 1, 2, 3, 4 } } };

static struct net_in6_addr dest_ipv6_addr1 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x1 } } };
static struct net_in6_addr dest_ipv6_addr2 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x2 } } };
static struct net_in6_addr dest_ipv6_addr3 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x3 } } };
static struct net_in6_addr dest_ipv6_addr4 = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0, 0, 0,
					0, 0, 0, 0, 0, 0, 0, 0x4 } } };
static struct net_in6_addr dest_ipv6_addr_not_found = { { { 0x20, 0x01, 0x0d, 0xb8, 0xde,
			0xad, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x4 } } };

static struct net_if *target_iface;
static char target_iface_name[CONFIG_NET_INTERFACE_NAME_LEN + 1];

K_SEM_DEFINE(wait_data, 0, UINT_MAX);

#define PKT_WAIT_TIME K_MSEC(500)
#define WAIT_TIME 500
#define WAIT_TIME_LONG MSEC_PER_SEC
#define MY_PORT 1969
#define PEER_PORT 2024
#define PEER_IPV6_ADDR "::1"
#define MY_IPV6_ADDR "::1"
#define MY_IPV4_ADDR "127.0.0.1"
#define PEER_IPV4_ADDR "127.0.0.1"

#define THREAD_SLEEP 50 /* ms */

static K_SEM_DEFINE(wait_pmtu_changed, 0, UINT_MAX);
static bool is_pmtu_changed;

static void ipv6_pmtu_changed(struct net_mgmt_event_callback *cb,
			      uint64_t mgmt_event,
			      struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (mgmt_event != NET_EVENT_IPV6_PMTU_CHANGED) {
		return;
	}

	NET_DBG("IPv6 PMTU changed event received");

	k_sem_give(&wait_pmtu_changed);
	is_pmtu_changed = true;

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static void ipv4_pmtu_changed(struct net_mgmt_event_callback *cb,
			      uint64_t mgmt_event,
			      struct net_if *iface)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(iface);

	if (mgmt_event != NET_EVENT_IPV4_PMTU_CHANGED) {
		return;
	}

	NET_DBG("IPv4 PMTU changed event received");

	k_sem_give(&wait_pmtu_changed);
	is_pmtu_changed = true;

	/* Let the network stack to proceed */
	k_msleep(THREAD_SLEEP);
}

static struct mgmt_events {
	uint64_t event;
	net_mgmt_event_handler_t handler;
	struct net_mgmt_event_callback cb;
} mgmt_events[] = {
	{ .event = NET_EVENT_IPV6_PMTU_CHANGED, .handler = ipv6_pmtu_changed },
	{ .event = NET_EVENT_IPV4_PMTU_CHANGED, .handler = ipv4_pmtu_changed },
};

static const char *iface2str(struct net_if *iface)
{
	if (net_if_l2(iface) == &NET_L2_GET_NAME(DUMMY)) {
		return "No L2";
	}

	return "<unknown type>";
}

static void iface_cb(struct net_if *iface, void *user_data)
{
	static int if_count;

	NET_DBG("Interface %p (%s) [%d]", iface, iface2str(iface),
		net_if_get_by_iface(iface));

	switch (if_count) {
	case 0:
		target_iface = iface;
		(void)net_if_get_name(iface, target_iface_name,
				      CONFIG_NET_INTERFACE_NAME_LEN);
		break;
	}

	if_count++;
}

__maybe_unused static void setup_mgmt_events(void)
{
	static bool setup_done;

	if (setup_done) {
		return;
	}

	setup_done = true;

	ARRAY_FOR_EACH(mgmt_events, i) {
		net_mgmt_init_event_callback(&mgmt_events[i].cb,
					     mgmt_events[i].handler,
					     mgmt_events[i].event);

		net_mgmt_add_event_callback(&mgmt_events[i].cb);
	}
}

static void init_dest_v4(struct net_sockaddr_in *dst, const struct net_in_addr *addr)
{
	memset(dst, 0, sizeof(*dst));
	dst->sin_family = NET_AF_INET;
	net_ipaddr_copy(&dst->sin_addr, addr);
}

static void init_dest_v6(struct net_sockaddr_in6 *dst, const struct net_in6_addr *addr)
{
	memset(dst, 0, sizeof(*dst));
	dst->sin6_family = NET_AF_INET6;
	net_ipaddr_copy(&dst->sin6_addr, addr);
}

static void seed_dplpmtud_pmtu(const struct net_sockaddr *dst, uint16_t mtu)
{
	int ret;

	ret = net_pmtu_update_mtu(dst, mtu);
	zassert_true(ret >= 0, "PMTU update failed (%d)", ret);
}

/* IP + UDP header overhead between the PLPMTU tracked by DPLPMTUD (UDP payload)
 * and the IP-layer MTU stored in the shared PMTU destination cache.
 */
static uint16_t dplpmtud_overhead(const struct net_sockaddr *dst)
{
	if (dst->sa_family == NET_AF_INET6) {
		return sizeof(struct net_ipv6_hdr) + sizeof(struct net_udp_hdr);
	}

	return sizeof(struct net_ipv4_hdr) + sizeof(struct net_udp_hdr);
}

static void run_dplpmtud_probe_lifecycle_test(const struct net_sockaddr *dst, uint16_t pmtu)
{
	struct net_dplpmtud_path path;
	int mtu;
	int probe;
	int next_probe;
	int ret;

	seed_dplpmtud_pmtu(dst, pmtu);

	ret = net_dplpmtud_init_path(&path, dst, 0U);
	zassert_ok(ret, "DPLPMTUD path init failed (%d)", ret);

	mtu = net_dplpmtud_get_path_mtu(&path);
	zassert_equal(mtu, NET_DPLPMTUD_BASE_PLPMTU,
		      "Initial DPLPMTUD MTU is wrong (%d)", mtu);

	probe = net_dplpmtud_get_path_probe_size(&path);
	zassert_true(probe > NET_DPLPMTUD_BASE_PLPMTU,
		     "Probe size did not grow above base PLPMTU (%d)", probe);
	zassert_true(probe <= pmtu, "Probe size exceeded PMTU ceiling (%d)", probe);

	ret = net_dplpmtud_on_path_probe_sent(&path, probe);
	zassert_ok(ret, "Probe send report failed (%d)", ret);

	ret = net_dplpmtud_on_path_probe_acked(&path, probe);
	zassert_ok(ret, "Probe ACK report failed (%d)", ret);

	mtu = net_dplpmtud_get_path_mtu(&path);
	zassert_equal(mtu, probe, "ACKed probe did not raise path MTU (%d)", mtu);

	mtu = net_pmtu_get_mtu(dst);
	zassert_equal(mtu, probe + dplpmtud_overhead(dst),
		      "PMTU cache must reflect ACKed probe (%d)", mtu);

	next_probe = net_dplpmtud_get_path_probe_size(&path);
	zassert_true(next_probe > probe,
		     "Search did not continue above ACKed probe size (%d)", next_probe);
	zassert_true(next_probe <= pmtu,
		     "Next probe exceeded PMTU ceiling (%d)", next_probe);
}

static void run_dplpmtud_transport_cap_test(const struct net_sockaddr *dst,
					    uint16_t pmtu, uint16_t path_max)
{
	struct net_dplpmtud_path path;
	int probe;
	int grown_probe;
	int ret;

	seed_dplpmtud_pmtu(dst, pmtu);

	ret = net_dplpmtud_init_path(&path, dst, path_max);
	zassert_ok(ret, "DPLPMTUD path init failed (%d)", ret);

	probe = net_dplpmtud_get_path_probe_size(&path);
	zassert_true(probe > NET_DPLPMTUD_BASE_PLPMTU,
		     "Transport-capped path did not schedule probing (%d)", probe);
	zassert_true(probe <= path_max, "Probe size exceeded path max (%d)", probe);

	ret = net_dplpmtud_on_path_probe_sent(&path, path_max + 1U);
	zassert_equal(ret, -EMSGSIZE, "Oversized probe was not rejected (%d)", ret);

	net_dplpmtud_set_path_max_plpmtu(&path, 0U);

	grown_probe = net_dplpmtud_get_path_probe_size(&path);
	zassert_true(grown_probe > probe,
		     "Removing path cap did not grow probe size (%d)", grown_probe);
	zassert_true(grown_probe <= pmtu,
		     "Expanded probe exceeded PMTU ceiling (%d)", grown_probe);
}

static void run_dplpmtud_low_pmtu_test(const struct net_sockaddr *dst, uint16_t pmtu)
{
	struct net_dplpmtud_path path;
	int mtu;
	int probe;
	int ret;

	seed_dplpmtud_pmtu(dst, pmtu);

	ret = net_dplpmtud_init_path(&path, dst, 0U);
	zassert_ok(ret, "DPLPMTUD path init failed (%d)", ret);

	mtu = net_dplpmtud_get_path_mtu(&path);
	zassert_equal(mtu, pmtu - dplpmtud_overhead(dst),
		      "Low PMTU ceiling was not preserved (%d)", mtu);

	probe = net_dplpmtud_get_path_probe_size(&path);
	zassert_equal(probe, 0, "Low PMTU path should not schedule probes (%d)", probe);
}

static void run_dplpmtud_loss_clamp_test(const struct net_sockaddr *dst, uint16_t pmtu)
{
	struct net_dplpmtud_path path;
	int probe;
	int next_probe;
	int ret;
	int retries;

	seed_dplpmtud_pmtu(dst, pmtu);

	ret = net_dplpmtud_init_path(&path, dst, 0U);
	zassert_ok(ret, "DPLPMTUD path init failed (%d)", ret);

	probe = net_dplpmtud_get_path_probe_size(&path);
	zassert_true(probe > NET_DPLPMTUD_BASE_PLPMTU,
		     "Probe size did not grow above base PLPMTU (%d)", probe);

	next_probe = probe;

	for (retries = 0; retries < 8; retries++) {
		ret = net_dplpmtud_on_path_probe_sent(&path, probe);
		zassert_ok(ret, "Probe send report failed (%d)", ret);

		ret = net_dplpmtud_on_path_probe_lost(&path, probe);
		zassert_ok(ret, "Probe loss report failed (%d)", ret);

		next_probe = net_dplpmtud_get_path_probe_size(&path);
		if (next_probe != probe) {
			break;
		}
	}

	zassert_true(retries < 8, "Probe size never clamped after repeated loss");
	zassert_true(next_probe < probe, "Repeated loss did not reduce probe size (%d)",
		     next_probe);
}

static void run_dplpmtud_getter_no_side_effect_test(const struct net_sockaddr *dst,
						   const struct net_sockaddr *other_dst)
{
	struct net_dplpmtud_path path;
	int ret;

	memset(&path, 0, sizeof(path));

	ret = net_dplpmtud_get_path_mtu(&path);
	zassert_equal(ret, -ENOENT, "Getter on uninitialized path must fail (%d)", ret);

	ret = net_dplpmtud_get_path_probe_size(&path);
	zassert_equal(ret, -ENOENT, "Getter on uninitialized path must fail (%d)", ret);

	zassert_is_null(net_dplpmtud_get_entry(other_dst),
			"Unused destination must not have DPLPMTUD state yet");

	seed_dplpmtud_pmtu(dst, 1450U);

	ret = net_dplpmtud_init_path(&path, dst, 0U);
	zassert_ok(ret, "DPLPMTUD path init failed (%d)", ret);

	ret = net_dplpmtud_get_path_mtu(&path);
	zassert_true(ret > 0, "Getter must read initialized path (%d)", ret);

	ret = net_dplpmtud_get_path_probe_size(&path);
	zassert_true(ret >= 0, "Getter must not fail on initialized path (%d)", ret);

	ret = net_dplpmtud_get_path_mtu(&path);
	zassert_true(ret > 0, "Repeated getter must keep working (%d)", ret);

	zassert_is_null(net_dplpmtud_get_entry(other_dst),
			"Getter must not create state for other destinations");
}

static void run_dplpmtud_blackhole_test(const struct net_sockaddr *dst, uint16_t pmtu)
{
	struct net_dplpmtud_path path;
	const uint16_t blackhole_mtu = 1180U;
	int mtu;
	int probe;
	int ret;

	seed_dplpmtud_pmtu(dst, pmtu);

	ret = net_dplpmtud_init_path(&path, dst, 0U);
	zassert_ok(ret, "DPLPMTUD path init failed (%d)", ret);

	/* PTB-style PMTU below the base PLPMTU must trigger black-hole handling. */
	ret = net_pmtu_update_mtu(dst, blackhole_mtu);
	zassert_true(ret >= 0, "PMTU update failed (%d)", ret);

	mtu = net_dplpmtud_get_path_mtu(&path);
	zassert_equal(mtu, blackhole_mtu - dplpmtud_overhead(dst),
		      "Black-hole PMTU must cap validated PLPMTU (%d)", mtu);

	mtu = net_pmtu_get_mtu(dst);
	zassert_equal(mtu, blackhole_mtu, "PMTU cache must follow black-hole sync (%d)", mtu);

	probe = net_dplpmtud_get_path_probe_size(&path);
	zassert_equal(probe, 0, "Black-holed path must not schedule probes (%d)", probe);

	seed_dplpmtud_pmtu(dst, pmtu);

	ret = net_dplpmtud_init_path(&path, dst, 0U);
	zassert_ok(ret, "DPLPMTUD path re-init failed (%d)", ret);

	net_dplpmtud_note_path_blackhole(&path);

	mtu = net_dplpmtud_get_path_mtu(&path);
	zassert_equal(mtu, NET_DPLPMTUD_BASE_PLPMTU,
		      "Explicit black-hole must fall back to base PLPMTU (%d)", mtu);

	mtu = net_pmtu_get_mtu(dst);
	zassert_equal(mtu, NET_DPLPMTUD_BASE_PLPMTU + dplpmtud_overhead(dst),
		      "PMTU cache must follow explicit black-hole (%d)", mtu);
}

static void *test_setup(void)
{
	net_if_foreach(iface_cb, NULL);

	zassert_not_null(target_iface, "Interface is NULL");

	return NULL;
}

ZTEST(net_pmtu_test_suite, test_pmtu_01_ipv4_get_entry)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct net_pmtu_entry *entry;
	struct net_sockaddr_in dest_ipv4;

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	dest_ipv4.sin_family = NET_AF_INET;

	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv4);
	zassert_is_null(entry, "PMTU IPv4 entry is not NULL");

	k_sleep(SMALL_SLEEP);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_01_ipv6_get_entry)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct net_pmtu_entry *entry;
	struct net_sockaddr_in6 dest_ipv6;

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
	dest_ipv6.sin6_family = NET_AF_INET6;

	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv6);
	zassert_is_null(entry, "PMTU IPv6 entry is not NULL");

	k_sleep(SMALL_SLEEP);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_02_ipv4_update_entry)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct net_sockaddr_in dest_ipv4;
	int ret;

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	dest_ipv4.sin_family = NET_AF_INET;

	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv4, 1300);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);

	k_sleep(SMALL_SLEEP);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_02_ipv6_update_entry)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct net_sockaddr_in6 dest_ipv6;
	int ret;

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
	dest_ipv6.sin6_family = NET_AF_INET6;

	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv6, 1600);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);

	k_sleep(SMALL_SLEEP);
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_03_ipv4_create_more_entries)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct net_sockaddr_in dest_ipv4;
	struct net_pmtu_entry *entry;
	uint16_t mtu;
	int ret;

	dest_ipv4.sin_family = NET_AF_INET;

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv4, 1300);
	zassert_equal(ret, 1300, "PMTU IPv4 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv4);
	zassert_equal(entry->mtu, 1300, "PMTU IPv4 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr2);
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv4, 1400);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);
	mtu = net_pmtu_get_mtu((struct net_sockaddr *)&dest_ipv4);
	zassert_equal(mtu, 1400, "PMTU IPv4 MTU is not correct (%d)", mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr3);
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv4, 1500);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv4);
	zassert_equal(entry->mtu, 1500, "PMTU IPv4 MTU is not correct (%d)",
		      entry->mtu);

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr_not_found);
	ret = net_pmtu_get_mtu((struct net_sockaddr *)&dest_ipv4);
	zassert_equal(ret, -ENOENT, "PMTU IPv4 MTU update succeed (%d)", ret);
	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv4);
	zassert_equal(entry, NULL, "PMTU IPv4 MTU update succeed");
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_03_ipv6_create_more_entries)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct net_sockaddr_in6 dest_ipv6;
	struct net_pmtu_entry *entry;
	uint16_t mtu;
	int ret;

	dest_ipv6.sin6_family = NET_AF_INET6;

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv6, 1600);
	zassert_equal(ret, 1600, "PMTU IPv6 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv6);
	zassert_equal(entry->mtu, 1600, "PMTU IPv6 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr2);
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv6, 1700);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);
	mtu = net_pmtu_get_mtu((struct net_sockaddr *)&dest_ipv6);
	zassert_equal(mtu, 1700, "PMTU IPv6 MTU is not correct (%d)", mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr3);
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv6, 1800);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);
	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv6);
	zassert_equal(entry->mtu, 1800, "PMTU IPv6 MTU is not correct (%d)",
		      entry->mtu);

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr_not_found);
	ret = net_pmtu_get_mtu((struct net_sockaddr *)&dest_ipv6);
	zassert_equal(ret, -ENOENT, "PMTU IPv6 MTU update succeed (%d)", ret);
	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv6);
	zassert_equal(entry, NULL, "PMTU IPv6 MTU update succeed");
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_04_ipv4_overflow)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct net_sockaddr_in dest_ipv4;
	struct net_pmtu_entry *entry;
	int ret;

	dest_ipv4.sin_family = NET_AF_INET;

	/* Create more entries than we have space */
	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr4);
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv4, 1450);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);

	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv4);
	zassert_equal(entry->mtu, 1450, "PMTU IPv4 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv4);
	zassert_is_null(entry, "PMTU IPv4 MTU found when it should not be");
#else
	ztest_test_skip();
#endif
}

ZTEST(net_pmtu_test_suite, test_pmtu_04_ipv6_overflow)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct net_sockaddr_in6 dest_ipv6;
	struct net_pmtu_entry *entry;
	int ret;

	dest_ipv6.sin6_family = NET_AF_INET6;

	/* Create more entries than we have space */
	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr4);
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv6, 1650);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);

	entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv6);
	zassert_equal(entry->mtu, 1650, "PMTU IPv6 MTU is not correct (%d)",
		      entry->mtu);

	k_sleep(SMALL_SLEEP);

	/* If we have IPv4 PMTU enabled, then the oldest one is an IPv4 entry.
	 */
	if (IS_ENABLED(CONFIG_NET_IPV4_PMTU)) {
		struct net_sockaddr_in dest_ipv4;

		dest_ipv4.sin_family = NET_AF_INET;

		net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr2);
		entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv4);
	} else {
		net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
		entry = net_pmtu_get_entry((struct net_sockaddr *)&dest_ipv6);
	}

	zassert_is_null(entry, "PMTU IPv6 MTU found when it should not be");
#else
	ztest_test_skip();
#endif
}

static void test_bind(int sock, struct net_sockaddr *addr, net_socklen_t addrlen)
{
	int ret;

	ret = zsock_bind(sock, addr, addrlen);
	zassert_equal(ret, 0, "bind failed with error %d", errno);
}

static void test_listen(int sock)
{
	zassert_equal(zsock_listen(sock, 1),
		      0,
		      "listen failed with error %d", errno);
}

static void test_connect(int sock, struct net_sockaddr *addr, net_socklen_t addrlen)
{
	zassert_equal(zsock_connect(sock, addr, addrlen),
		      0,
		      "connect failed with error %d", errno);

	if (IS_ENABLED(CONFIG_NET_TC_THREAD_PREEMPTIVE)) {
		/* Let the connection proceed */
		k_msleep(THREAD_SLEEP);
	}
}

static void test_accept(int sock, int *new_sock, struct net_sockaddr *addr,
			net_socklen_t *addrlen)
{
	zassert_not_null(new_sock, "null newsock");

	*new_sock = zsock_accept(sock, addr, addrlen);
	zassert_true(*new_sock >= 0, "accept failed");
}

#if defined(CONFIG_NET_IPV6_PMTU)
static int get_v6_send_recv_sock(int *srv_sock,
				 struct net_sockaddr_in6 *my_saddr,
				 struct net_sockaddr_in6 *peer_saddr,
				 uint16_t my_port,
				 uint16_t peer_port)
{
	struct net_sockaddr addr;
	net_socklen_t addrlen = sizeof(addr);
	int new_sock;
	int c_sock;
	int s_sock;

	prepare_sock_tcp_v6(PEER_IPV6_ADDR, peer_port, &s_sock, peer_saddr);
	test_bind(s_sock, (struct net_sockaddr *)peer_saddr, sizeof(*peer_saddr));
	test_listen(s_sock);

	prepare_sock_tcp_v6(MY_IPV6_ADDR, my_port, &c_sock, my_saddr);
	test_bind(c_sock, (struct net_sockaddr *)my_saddr, sizeof(*my_saddr));
	test_connect(c_sock, (struct net_sockaddr *)peer_saddr, sizeof(*peer_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct net_sockaddr_in6), "wrong addrlen");

	*srv_sock = new_sock;

	return c_sock;
}

static int create_icmpv6_ptb(struct net_if *iface,
			     struct net_sockaddr_in6 *src,
			     struct net_sockaddr_in6 *dst,
			     uint32_t mtu,
			     struct net_pkt **pkt)
{
	struct net_icmpv6_ptb ptb_hdr;
	struct net_pkt *ptb_pkt;
	struct net_in6_addr *dest6;
	struct net_in6_addr *src6;
	int ret;

	ptb_pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_ipv6_hdr) +
					    sizeof(struct net_icmp_hdr) +
					    sizeof(struct net_icmpv6_ptb),
					    NET_AF_INET6, NET_IPPROTO_ICMPV6,
					    PKT_WAIT_TIME);
	if (ptb_pkt == NULL) {
		NET_DBG("No buffer");
		return -ENOMEM;
	}

	dest6 = &dst->sin6_addr;
	src6 = &src->sin6_addr;

	ret = net_ipv6_create(ptb_pkt, src6, dest6);
	if (ret < 0) {
		LOG_ERR("Cannot create IPv6 pkt (%d)", ret);
		return ret;
	}

	ret = net_icmpv6_create(ptb_pkt, NET_ICMPV6_PACKET_TOO_BIG, 0);
	if (ret < 0) {
		LOG_ERR("Cannot create ICMPv6 pkt (%d)", ret);
		return ret;
	}

	ptb_hdr.mtu = net_htonl(mtu);

	ret = net_pkt_write(ptb_pkt, &ptb_hdr, sizeof(ptb_hdr));
	if (ret < 0) {
		LOG_ERR("Cannot write payload (%d)", ret);
		return ret;
	}

	net_pkt_cursor_init(ptb_pkt);
	net_ipv6_finalize(ptb_pkt, NET_IPPROTO_ICMPV6);

	net_pkt_set_iface(ptb_pkt, iface);

	*pkt = ptb_pkt;

	return 0;
}
#endif

ZTEST(net_pmtu_test_suite, test_pmtu_05_ipv6_tcp)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct net_sockaddr_in6 dest_ipv6;
	struct net_sockaddr_in6 s_saddr = { 0 };  /* peer */
	struct net_sockaddr_in6 c_saddr = { 0 };  /* this host */
	struct net_pkt *pkt = NULL;
	int client_sock, server_sock;
	uint16_t mtu;
	int ret;

	dest_ipv6.sin6_family = NET_AF_INET6;

	client_sock = get_v6_send_recv_sock(&server_sock, &c_saddr, &s_saddr,
					    MY_PORT, PEER_PORT);
	zassert_true(client_sock >= 0, "Failed to create client socket");

	/* Set initial MTU for the destination */
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&c_saddr, 4096);
	zassert_true(ret >= 0, "PMTU IPv6 MTU update failed (%d)", ret);

	/* Send an ICMPv6 "Packet too big" message from server to client which
	 * will update the PMTU entry.
	 */
	ret = create_icmpv6_ptb(target_iface, &s_saddr, &c_saddr, 2048, &pkt);
	zassert_equal(ret, 0, "Failed to create ICMPv6 PTB message");

	ret = net_send_data(pkt);
	zassert_equal(ret, 0, "Failed to send PTB message");

	/* Check that the PMTU entry has been updated */
	mtu = net_tcp_get_mtu((struct net_sockaddr *)&s_saddr);
	zassert_equal(mtu, 2048, "PMTU IPv6 MTU is not correct (%d)", mtu);

	(void)zsock_close(client_sock);
	(void)zsock_close(server_sock);
#else
	ztest_test_skip();
#endif /* CONFIG_NET_IPV6_PMTU */
}

#if defined(CONFIG_NET_IPV4_PMTU)
static int get_v4_send_recv_sock(int *srv_sock,
				 struct net_sockaddr_in *my_saddr,
				 struct net_sockaddr_in *peer_saddr,
				 uint16_t my_port,
				 uint16_t peer_port)
{
	struct net_sockaddr addr;
	net_socklen_t addrlen = sizeof(addr);
	int new_sock;
	int c_sock;
	int s_sock;

	prepare_sock_tcp_v4(PEER_IPV4_ADDR, peer_port, &s_sock, peer_saddr);
	test_bind(s_sock, (struct net_sockaddr *)peer_saddr, sizeof(*peer_saddr));
	test_listen(s_sock);

	prepare_sock_tcp_v4(MY_IPV4_ADDR, my_port, &c_sock, my_saddr);
	test_bind(c_sock, (struct net_sockaddr *)my_saddr, sizeof(*my_saddr));
	test_connect(c_sock, (struct net_sockaddr *)peer_saddr, sizeof(*peer_saddr));

	test_accept(s_sock, &new_sock, &addr, &addrlen);
	zassert_equal(addrlen, sizeof(struct net_sockaddr_in), "wrong addrlen");

	*srv_sock = new_sock;

	return c_sock;
}

static int create_icmpv4_dest_unreach(struct net_if *iface,
				      struct net_sockaddr_in *src,
				      struct net_sockaddr_in *dst,
				      uint32_t mtu,
				      struct net_pkt **pkt)
{
	struct net_icmpv4_dest_unreach du_hdr;
	struct net_pkt *du_pkt;
	struct net_in_addr *dest4;
	struct net_in_addr *src4;
	int ret;

	du_pkt = net_pkt_alloc_with_buffer(iface, sizeof(struct net_ipv4_hdr) +
					   sizeof(struct net_icmp_hdr) +
					   sizeof(struct net_icmpv4_dest_unreach),
					   NET_AF_INET, NET_IPPROTO_ICMP,
					   PKT_WAIT_TIME);
	if (du_pkt == NULL) {
		NET_DBG("No buffer");
		return -ENOMEM;
	}

	dest4 = &dst->sin_addr;
	src4 = &src->sin_addr;

	ret = net_ipv4_create(du_pkt, src4, dest4);
	if (ret < 0) {
		LOG_ERR("Cannot create IPv4 pkt (%d)", ret);
		return ret;
	}

	ret = net_icmpv4_create(du_pkt, NET_ICMPV4_DST_UNREACH, 0);
	if (ret < 0) {
		LOG_ERR("Cannot create ICMPv4 pkt (%d)", ret);
		return ret;
	}

	du_hdr.mtu = net_htons(mtu);

	ret = net_pkt_write(du_pkt, &du_hdr, sizeof(du_hdr));
	if (ret < 0) {
		LOG_ERR("Cannot write payload (%d)", ret);
		return ret;
	}

	net_pkt_cursor_init(du_pkt);
	net_ipv4_finalize(du_pkt, NET_IPPROTO_ICMP);

	net_pkt_set_iface(du_pkt, iface);

	*pkt = du_pkt;

	return 0;
}
#endif

ZTEST(net_pmtu_test_suite, test_pmtu_05_ipv4_tcp)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct net_sockaddr_in dest_ipv4;
	struct net_sockaddr_in s_saddr = { 0 };  /* peer */
	struct net_sockaddr_in c_saddr = { 0 };  /* this host */
	struct net_pkt *pkt = NULL;
	int client_sock, server_sock;
	uint16_t mtu;
	int ret;

	dest_ipv4.sin_family = NET_AF_INET;

	client_sock = get_v4_send_recv_sock(&server_sock, &c_saddr, &s_saddr,
					    MY_PORT, PEER_PORT);
	zassert_true(client_sock >= 0, "Failed to create client socket");

	/* Set initial MTU for the destination */
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&c_saddr, 4096);
	zassert_true(ret >= 0, "PMTU IPv6 MTU update failed (%d)", ret);

	/* Send an ICMPv4 "Destination Unreachable" message from server to client which
	 * will update the PMTU entry.
	 */
	ret = create_icmpv4_dest_unreach(target_iface, &s_saddr, &c_saddr, 2048, &pkt);
	zassert_equal(ret, 0, "Failed to create ICMPv4 Destination Unreachable message");

	ret = net_send_data(pkt);
	zassert_equal(ret, 0, "Failed to send Destination Unreachable message");

	/* Check that the PMTU entry has been updated */
	mtu = net_tcp_get_mtu((struct net_sockaddr *)&s_saddr);
	zassert_equal(mtu, 2048, "PMTU IPv4 MTU is not correct (%d)", mtu);

	(void)zsock_close(client_sock);
	(void)zsock_close(server_sock);
#else
	ztest_test_skip();
#endif /* CONFIG_NET_IPV4_PMTU */
}

ZTEST(net_pmtu_test_suite, test_pmtu_06_ipv4_event)
{
#if defined(CONFIG_NET_IPV4_PMTU) && WAIT_PROPERLY
	struct net_sockaddr_in dest_ipv4;
	int ret;

	setup_mgmt_events();

	is_pmtu_changed = false;

	net_ipaddr_copy(&dest_ipv4.sin_addr, &dest_ipv4_addr1);
	dest_ipv4.sin_family = NET_AF_INET;

	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv4, 1200);
	zassert_equal(ret, 0, "PMTU IPv4 MTU update failed (%d)", ret);

	if (k_sem_take(&wait_pmtu_changed, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting pmtu changed event");
	}

	zassert_true(is_pmtu_changed, "Did not catch pmtu changed event");

	is_pmtu_changed = false;
#else
	ztest_test_skip();
#endif /* CONFIG_NET_IPV4_PMTU */
}

ZTEST(net_pmtu_test_suite, test_pmtu_06_ipv6_event)
{
#if defined(CONFIG_NET_IPV6_PMTU) && WAIT_PROPERLY
	struct net_sockaddr_in6 dest_ipv6;
	int ret;

	setup_mgmt_events();

	is_pmtu_changed = false;

	net_ipaddr_copy(&dest_ipv6.sin6_addr, &dest_ipv6_addr1);
	dest_ipv6.sin6_family = NET_AF_INET6;

	ret = net_pmtu_update_mtu((struct net_sockaddr *)&dest_ipv6, 1500);
	zassert_equal(ret, 0, "PMTU IPv6 MTU update failed (%d)", ret);

	if (k_sem_take(&wait_pmtu_changed, K_MSEC(WAIT_TIME))) {
		zassert_true(0, "Timeout while waiting pmtu changed event");
	}

	zassert_true(is_pmtu_changed, "Did not catch pmtu changed event");

	is_pmtu_changed = false;
#else
	ztest_test_skip();
#endif /* CONFIG_NET_IPV6_PMTU */
}

ZTEST(net_pmtu_test_suite, test_pmtu_07_socket_api_ipv4)
{
#if defined(CONFIG_NET_IPV4_PMTU)
	struct net_sockaddr_in s_saddr = { 0 };  /* peer */
	struct net_sockaddr_in c_saddr = { 0 };  /* this host */
	int ret, client_sock, server_sock;
	size_t optlen;
	int optval;
	int err;

	client_sock = get_v4_send_recv_sock(&server_sock, &c_saddr, &s_saddr,
					    MY_PORT + 1, PEER_PORT + 1);
	zassert_true(client_sock >= 0, "Failed to create client socket");

	/* Set initial MTU for the destination */
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&c_saddr, 4096);
	zassert_true(ret >= 0, "PMTU IPv4 MTU update failed (%d)", ret);

	optval = 0; optlen = sizeof(int);
	ret = zsock_getsockopt(client_sock, NET_IPPROTO_IP, ZSOCK_IP_MTU, &optval, &optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(int), "setsockopt optlen (%d)", optlen);
	zexpect_equal(optval, 4096, "setsockopt mtu (%d)", optval);

	optval = 0; optlen = sizeof(int);
	ret = zsock_setsockopt(client_sock, NET_IPPROTO_IP, ZSOCK_IP_MTU, &optval, optlen);
	err = -errno;
	zexpect_equal(ret, -1, "setsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(int), "setsockopt optlen (%d)", optlen);
	zexpect_equal(optval, 0, "setsockopt mtu (%d)", optval);

	(void)zsock_close(client_sock);
	(void)zsock_close(server_sock);
#else
	ztest_test_skip();
#endif /* CONFIG_NET_IPV4_PMTU */
}

ZTEST(net_pmtu_test_suite, test_pmtu_08_socket_api_ipv6)
{
#if defined(CONFIG_NET_IPV6_PMTU)
	struct net_sockaddr_in6 s_saddr = { 0 };  /* peer */
	struct net_sockaddr_in6 c_saddr = { 0 };  /* this host */
	int ret, client_sock, server_sock;
	size_t optlen;
	int optval;
	int err;

	client_sock = get_v6_send_recv_sock(&server_sock, &c_saddr, &s_saddr,
					    MY_PORT + 2, PEER_PORT + 2);
	zassert_true(client_sock >= 0, "Failed to create client socket");

	/* Set initial MTU for the destination */
	ret = net_pmtu_update_mtu((struct net_sockaddr *)&c_saddr, 2048);
	zassert_true(ret >= 0, "PMTU IPv6 MTU update failed (%d)", ret);

	optval = 0; optlen = sizeof(int);
	ret = zsock_getsockopt(client_sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_MTU, &optval, &optlen);
	err = -errno;
	zexpect_equal(ret, 0, "getsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(int), "getsockopt optlen (%d)", optlen);
	zexpect_equal(optval, 2048, "getsockopt mtu (%d)", optval);

	optval = 1500; optlen = sizeof(int);
	ret = zsock_setsockopt(client_sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_MTU, &optval, optlen);
	err = -errno;
	zexpect_equal(ret, 0, "setsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(int), "setsockopt optlen (%d)", optlen);
	zexpect_equal(optval, 1500, "setsockopt mtu (%d)", optval);

	optval = 0; optlen = sizeof(int);
	ret = zsock_getsockopt(client_sock, NET_IPPROTO_IPV6, ZSOCK_IPV6_MTU, &optval, &optlen);
	err = -errno;
	zexpect_equal(ret, 0, "getsockopt failed (%d)", err);
	zexpect_equal(optlen, sizeof(int), "getsockopt optlen (%d)", optlen);
	zexpect_equal(optval, 1500, "getsockopt mtu (%d)", optval);

	(void)zsock_close(client_sock);
	(void)zsock_close(server_sock);
#else
	ztest_test_skip();
#endif /* CONFIG_NET_IPV6_PMTU */
}

ZTEST(net_pmtu_test_suite, test_pmtu_09_ipv4_dplpmtud_probe_lifecycle)
{
	struct net_sockaddr_in dest_ipv4;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_PMTU_DPLPMTUD);

	init_dest_v4(&dest_ipv4, &dest_ipv4_addr_not_found);
	run_dplpmtud_probe_lifecycle_test((struct net_sockaddr *)&dest_ipv4, 1450U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_09_ipv6_dplpmtud_probe_lifecycle)
{
	struct net_sockaddr_in6 dest_ipv6;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_PMTU_DPLPMTUD);

	init_dest_v6(&dest_ipv6, &dest_ipv6_addr_not_found);
	run_dplpmtud_probe_lifecycle_test((struct net_sockaddr *)&dest_ipv6, 1650U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_10_ipv4_dplpmtud_transport_cap)
{
	struct net_sockaddr_in dest_ipv4;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_PMTU_DPLPMTUD);

	init_dest_v4(&dest_ipv4, &dest_ipv4_addr2);
	run_dplpmtud_transport_cap_test((struct net_sockaddr *)&dest_ipv4, 1450U, 1300U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_10_ipv6_dplpmtud_transport_cap)
{
	struct net_sockaddr_in6 dest_ipv6;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_PMTU_DPLPMTUD);

	init_dest_v6(&dest_ipv6, &dest_ipv6_addr2);
	run_dplpmtud_transport_cap_test((struct net_sockaddr *)&dest_ipv6, 1650U, 1300U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_11_ipv4_dplpmtud_low_pmtu)
{
	struct net_sockaddr_in dest_ipv4;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_PMTU_DPLPMTUD);

	init_dest_v4(&dest_ipv4, &dest_ipv4_addr3);
	run_dplpmtud_low_pmtu_test((struct net_sockaddr *)&dest_ipv4, 1180U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_11_ipv6_dplpmtud_low_pmtu)
{
	struct net_sockaddr_in6 dest_ipv6;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_PMTU_DPLPMTUD);

	init_dest_v6(&dest_ipv6, &dest_ipv6_addr3);
	run_dplpmtud_low_pmtu_test((struct net_sockaddr *)&dest_ipv6, 1180U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_12_ipv4_dplpmtud_loss_clamp)
{
	struct net_sockaddr_in dest_ipv4;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_PMTU_DPLPMTUD);

	init_dest_v4(&dest_ipv4, &dest_ipv4_addr4);
	run_dplpmtud_loss_clamp_test((struct net_sockaddr *)&dest_ipv4, 1450U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_12_ipv6_dplpmtud_loss_clamp)
{
	struct net_sockaddr_in6 dest_ipv6;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_PMTU_DPLPMTUD);

	init_dest_v6(&dest_ipv6, &dest_ipv6_addr4);
	run_dplpmtud_loss_clamp_test((struct net_sockaddr *)&dest_ipv6, 1650U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_14_ipv4_dplpmtud_getter_no_side_effect)
{
	static const struct net_in_addr other_ipv4_addr = { { { 198, 51, 100, 99 } } };
	struct net_sockaddr_in dest_ipv4;
	struct net_sockaddr_in other_ipv4;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_PMTU_DPLPMTUD);

	init_dest_v4(&dest_ipv4, &dest_ipv4_addr2);
	init_dest_v4(&other_ipv4, &other_ipv4_addr);
	run_dplpmtud_getter_no_side_effect_test((struct net_sockaddr *)&dest_ipv4,
						(struct net_sockaddr *)&other_ipv4);
}

ZTEST(net_pmtu_test_suite, test_pmtu_14_ipv6_dplpmtud_getter_no_side_effect)
{
	static const struct net_in6_addr other_ipv6_addr = { { { 0x20, 0x01, 0x0d, 0xb8, 0x01, 0,
								     0, 0, 0, 0, 0, 0, 0, 0, 0,
								     0x99 } } };
	struct net_sockaddr_in6 dest_ipv6;
	struct net_sockaddr_in6 other_ipv6;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_PMTU_DPLPMTUD);

	init_dest_v6(&dest_ipv6, &dest_ipv6_addr2);
	init_dest_v6(&other_ipv6, &other_ipv6_addr);
	run_dplpmtud_getter_no_side_effect_test((struct net_sockaddr *)&dest_ipv6,
						(struct net_sockaddr *)&other_ipv6);
}

ZTEST(net_pmtu_test_suite, test_pmtu_13_ipv4_dplpmtud_blackhole)
{
	struct net_sockaddr_in dest_ipv4;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV4_PMTU_DPLPMTUD);

	init_dest_v4(&dest_ipv4, &dest_ipv4_addr1);
	run_dplpmtud_blackhole_test((struct net_sockaddr *)&dest_ipv4, 1450U);
}

ZTEST(net_pmtu_test_suite, test_pmtu_13_ipv6_dplpmtud_blackhole)
{
	struct net_sockaddr_in6 dest_ipv6;

	Z_TEST_SKIP_IFNDEF(CONFIG_NET_IPV6_PMTU_DPLPMTUD);

	init_dest_v6(&dest_ipv6, &dest_ipv6_addr1);
	run_dplpmtud_blackhole_test((struct net_sockaddr *)&dest_ipv6, 1650U);
}

ZTEST_SUITE(net_pmtu_test_suite, NULL, test_setup, NULL, NULL, NULL);
