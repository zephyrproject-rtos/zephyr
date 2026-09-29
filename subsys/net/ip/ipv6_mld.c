/** @file
 * @brief IPv6 MLD related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(net_ipv6, CONFIG_NET_IPV6_LOG_LEVEL);

#include <errno.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_log.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_stats.h>
#include <zephyr/net/net_context.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/icmp.h>
#include <zephyr/random/random.h>
#include "net_private.h"
#include "connection.h"
#include "icmpv6.h"
#include "udp_internal.h"
#include "tcp_internal.h"
#include "ipv6.h"
#include "nbr.h"
#include "6lo.h"
#include "route_ipv6.h"
#include "route.h"
#include "net_stats.h"

/* Timeout for various buffer allocations in this file. */
#define PKT_WAIT_TIME K_MSEC(50)

#define MLDv2_MCAST_RECORD_LEN sizeof(struct net_icmpv6_mld_mcast_record)
#define IPV6_OPT_HDR_ROUTER_ALERT_LEN 8
#define MLDV2_REPORT_RESERVED_BYTES 2

#define MLDv2_LEN (MLDv2_MCAST_RECORD_LEN + sizeof(struct net_in6_addr))

/* Router Alert option value for MLD, RFC 2711 ch 2.1 */
#define IPV6_OPT_ROUTER_ALERT_MLD 0

/* Query lengths that tell the versions apart, RFC 3810 ch 8.1 */
#define MLD_V1_QUERY_LEN 24
#define MLD_V2_QUERY_MIN_LEN 28

/* Query Interval and Query Response Interval, RFC 3810 ch 9.2 and 9.3 */
#define MLD_QUERY_INTERVAL_S 125U
#define MLD_QUERY_RESPONSE_INTERVAL_S 10U

/* Unsolicited Report Interval, RFC 3810 ch 9.11 and RFC 2710 ch 7.10 */
#define MLDV2_UNSOLICITED_REPORT_INTERVAL_MS (1U * MSEC_PER_SEC)
#define MLDV1_UNSOLICITED_REPORT_INTERVAL_MS (10U * MSEC_PER_SEC)

/* Older Version Querier Present Timeout, RFC 3810 ch 9.12 */
#define MLD_OLDER_VERSION_QUERIER_PRESENT_S \
	(CONFIG_NET_IPV6_MLD_ROBUSTNESS * MLD_QUERY_INTERVAL_S + MLD_QUERY_RESPONSE_INTERVAL_S)

/* Numbered so that 0 can mark an interface without a known compatibility mode */
enum mld_version {
	MLDV1 = 1,
	MLDV2,
};

/* Protects the response deadlines and the querier present timer */
/* Protects the MLD timers and the compatibility mode of every interface. It
 * is never held while a message is sent: sending takes the interface lock,
 * which callers of net_ipv6_mld_join() may hold while taking this one.
 */
static K_MUTEX_DEFINE(mld_lock);

static void mld_timeout(struct k_work *work);
static K_WORK_DELAYABLE_DEFINE(mld_timer, mld_timeout);

static void mld_retransmit_schedule(struct net_if *iface, struct net_if_mcast_addr *group);

static k_timepoint_t mld_timepoint_never(void)
{
	return sys_timepoint_calc(K_FOREVER);
}

static bool mld_timepoint_is_never(k_timepoint_t timepoint)
{
	return K_TIMEOUT_EQ(sys_timepoint_timeout(timepoint), K_FOREVER);
}

static k_timepoint_t mld_timepoint_min(k_timepoint_t a, k_timepoint_t b)
{
	return sys_timepoint_cmp(a, b) < 0 ? a : b;
}

/* MLDv2 encodes delays of 32768 ms and above as floating point (RFC 3810 ch
 * 5.1.3), MLDv1 is linear over the whole field (RFC 2710 ch 3.4).
 */
uint32_t net_ipv6_mld_max_resp_delay(uint16_t code, bool mldv2)
{
	if (mldv2 && code >= 0x8000U) {
		uint32_t mant = code & 0x0FFFU;
		uint32_t exponent = (code >> 12) & 0x07U;

		return (mant | 0x1000U) << (exponent + 3U);
	}

	return code;
}

/* A random point in time within (0, max_ms] from now */
static k_timepoint_t mld_random_delay(uint32_t max_ms)
{
	uint32_t delay_ms = max_ms > 0U ? 1U + sys_rand32_get() % max_ms : 0U;

	return sys_timepoint_calc(K_MSEC(delay_ms));
}

/* Make the timer fire no later than at timeout. Called with mld_lock held,
 * but not from the timer handler itself, which reschedules directly.
 */
static void mld_timer_arm(k_timepoint_t timeout)
{
	k_timeout_t remaining = sys_timepoint_timeout(timeout);
	k_ticks_t left = k_work_delayable_remaining_get(&mld_timer);

	if (mld_timepoint_is_never(timeout)) {
		/* Rescheduling with K_FOREVER would cancel the timer */
		return;
	}

	if (left > 0 && left <= remaining.ticks) {
		return;
	}

	if (left == 0 && k_work_delayable_is_pending(&mld_timer)) {
		/* Already due or running: run again right away so that the new
		 * deadline is taken into account.
		 */
		remaining = K_NO_WAIT;
	}

	(void)k_work_reschedule(&mld_timer, remaining);
}

/* Cancel every pending query response and report retransmission of the
 * interface.
 */
static void mld_cancel_timers(struct net_if_ipv6 *ipv6)
{
	ipv6->mld_general_timeout = mld_timepoint_never();

	ARRAY_FOR_EACH(ipv6->mcast, i) {
		ipv6->mcast[i].mld_resp_timeout = mld_timepoint_never();
		ipv6->mcast[i].mld_retx_timeout = mld_timepoint_never();
		ipv6->mcast[i].mld_retx_left = 0U;
	}
}

/* The fields shared by MLDv1 and MLDv2 queries */
struct mld_query_common {
	uint16_t max_response_code;
	uint16_t reserved;
	uint8_t mcast_address[NET_IPV6_ADDR_SIZE];
} __packed;

/* Version that follows from the querier timer, RFC 3810 ch 8.2.1: MLDv1
 * while an MLDv1 querier was heard within the Older Version Querier Present
 * Timeout, MLDv2 otherwise. Called with mld_lock held, the timepoint is not
 * read atomically.
 */
static enum mld_version mld_querier_version(const struct net_if_ipv6 *ipv6)
{
	return sys_timepoint_expired(ipv6->mld_v1_querier_timeout) ? MLDV2 : MLDV1;
}

/* Host Compatibility Mode of the interface as last applied by
 * mld_version_update(), MLDv2 before any query was heard. A single byte,
 * so it can be read without mld_lock on the send paths.
 */
static enum mld_version mld_host_version(const struct net_if_ipv6 *ipv6)
{
	return ipv6->mld_version != 0U ? (enum mld_version)ipv6->mld_version : MLDV2;
}

/* Apply the Host Compatibility Mode that follows from the querier timer. A
 * host that changes its mode cancels its pending responses and
 * retransmission timers, RFC 3810 ch 8.2.1. Called with mld_lock held.
 */
static void mld_version_update(struct net_if_ipv6 *ipv6)
{
	enum mld_version version = mld_querier_version(ipv6);
	enum mld_version old_version = mld_host_version(ipv6);

	if (old_version != version) {
		mld_cancel_timers(ipv6);
	}

	ipv6->mld_version = version;
}

/* An MLDv1 query switches the host to MLDv1 for the Older Version Querier
 * Present Timeout, RFC 3810 ch 8.2.1. The timer is armed for the end of the
 * timeout, when the host switches back.
 */
static void mld_querier_seen(struct net_if_ipv6 *ipv6, enum mld_version version)
{
	k_mutex_lock(&mld_lock, K_FOREVER);

	if (version == MLDV1) {
		ipv6->mld_v1_querier_timeout =
			sys_timepoint_calc(K_SECONDS(MLD_OLDER_VERSION_QUERIER_PRESENT_S));
		mld_timer_arm(ipv6->mld_v1_querier_timeout);
	}

	mld_version_update(ipv6);

	k_mutex_unlock(&mld_lock);
}

/* Internal structure used for appending multicast routes to MLDv2 reports */
struct mcast_route_appending_info {
	int status;
	struct net_pkt *pkt;
	struct net_if *iface;
	size_t skipped;
};

/* Groups MLD messages are sent for. No MLD message is ever sent for the
 * link-scope all-nodes address, nor for a multicast address of scope 0
 * (reserved) or 1 (interface-local), RFC 3810 ch 6.
 */
static bool mld_is_reported(const struct net_in6_addr *addr)
{
	struct net_in6_addr all_nodes;

	net_ipv6_addr_create_ll_allnodes_mcast(&all_nodes);

	return !net_ipv6_addr_cmp(addr, &all_nodes) &&
	       !net_ipv6_is_addr_mcast_scope(addr, 0x00) &&
	       !net_ipv6_is_addr_mcast_scope(addr, 0x01);
}

static int mld_create(struct net_pkt *pkt,
		      const struct net_in6_addr *addr,
		      uint8_t record_type)
{
	NET_PKT_DATA_ACCESS_DEFINE(mld_access,
				   struct net_icmpv6_mld_mcast_record);
	struct net_icmpv6_mld_mcast_record *mld;

	mld = (struct net_icmpv6_mld_mcast_record *)
				net_pkt_get_data(pkt, &mld_access);
	if (!mld) {
		return -ENOBUFS;
	}

	mld->record_type = record_type;
	mld->aux_data_len = 0U;
	mld->num_sources = 0U;

	net_ipv6_addr_copy_raw(mld->mcast_address, (uint8_t *)addr);

	if (net_pkt_set_data(pkt, &mld_access)) {
		return -ENOBUFS;
	}

	return 0;
}

/* IPv6 header with the Router Alert option in a Hop-by-Hop Options header,
 * as every MLD message carries (RFC 3810 ch 5, RFC 2710 ch 3).
 */
static int mld_create_ipv6(struct net_pkt *pkt, const struct net_in6_addr *dst)
{
	const struct net_in6_addr *src;

	/* MLD messages carry a link-local source, or the unspecified address
	 * while the interface has no valid one yet (RFC 3810 ch 5.2.13).
	 */
	src = net_if_ipv6_get_ll(net_pkt_iface(pkt), NET_ADDR_PREFERRED);
	if (src == NULL) {
		src = net_ipv6_unspecified_address();
	}

	net_pkt_set_ipv6_hop_limit(pkt, 1); /* RFC 3810 ch 5 */

	if (net_ipv6_create(pkt, src, dst)) {
		return -ENOBUFS;
	}

	/* Add hop-by-hop option and router alert option, RFC 3810 ch 5. */
	if (net_pkt_write_u8(pkt, NET_IPPROTO_ICMPV6) ||
	    net_pkt_write_u8(pkt, 0)) {
		return -ENOBUFS;
	}

	/* IPv6 router alert option is described in RFC 2711.
	 * - 0x0502 RFC 2711 ch 2.1
	 * - MLD (value 0)
	 * - 2 bytes of padding
	 */
	if (net_pkt_write_be16(pkt, 0x0502) ||
	    net_pkt_write_be16(pkt, 0) ||
	    net_pkt_write_be16(pkt, 0)) {
		return -ENOBUFS;
	}

	net_pkt_set_ipv6_ext_len(pkt, IPV6_OPT_HDR_ROUTER_ALERT_LEN);
	net_pkt_set_ipv6_next_hdr(pkt, NET_IPV6_NEXTHDR_HBHO);

	return 0;
}

static int mld_create_packet(struct net_pkt *pkt, uint16_t count)
{
	struct net_in6_addr dst;
	int ret;

	/* Sent to all MLDv2-capable routers, RFC 3810 ch 5.2.14 */
	net_ipv6_addr_create(&dst, 0xff02, 0, 0, 0, 0, 0, 0, 0x0016);

	ret = mld_create_ipv6(pkt, &dst);
	if (ret < 0) {
		return ret;
	}

	/* ICMPv6 header + reserved space + count.
	 * MLDv6 stuff will come right after
	 */
	if (net_icmpv6_create(pkt, NET_ICMPV6_MLDv2, 0) ||
	    net_pkt_write_be16(pkt, 0) ||
	    net_pkt_write_be16(pkt, count)) {
		return -ENOBUFS;
	}

	return 0;
}

static int mld_send(struct net_pkt *pkt)
{
	__maybe_unused struct net_if *iface = net_pkt_iface(pkt);
	int ret;

	net_pkt_cursor_init(pkt);
	net_ipv6_finalize(pkt, NET_IPPROTO_ICMPV6);

	ret = net_send_data(pkt);
	if (ret < 0) {
		net_stats_update_icmp_drop(iface);
		net_stats_update_ipv6_mld_drop(iface);

		net_pkt_unref(pkt);

		return ret;
	}

	net_stats_update_icmp_sent(iface);
	net_stats_update_ipv6_mld_sent(iface);

	return 0;
}

#if defined(CONFIG_NET_IPV6_MCAST_ROUTE_MLD_REPORTS)
static void count_mcast_routes(struct net_route_ipv6_entry_mcast *entry, void *user_data)
{
	/* Only routes to an address that MLD reports */
	if (mld_is_reported(&entry->group)) {
		(*((int *)user_data))++;
	}
}

static void append_mcast_routes(struct net_route_ipv6_entry_mcast *entry, void *user_data)
{
	struct mcast_route_appending_info *info = (struct mcast_route_appending_info *)user_data;
	struct net_if_mcast_addr *mcasts = info->iface->config.ip.ipv6->mcast;

	if (info->status != 0 || entry->prefix_len != 128 || !mld_is_reported(&entry->group)) {
		return;
	}

	for (int i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!mcasts[i].is_used || !mcasts[i].is_joined) {
			continue;
		}

		if (net_ipv6_addr_cmp(&entry->group, &mcasts[i].address.in6_addr)) {
			/* Address was already added to the report */
			info->skipped++;
			return;
		}
	}

	info->status = mld_create(info->pkt, &entry->group, NET_IPV6_MLDv2_MODE_IS_EXCLUDE);
}
#endif

/* An MLDv1 Report goes to the address it reports, a Done to the link-scope
 * all-routers address (RFC 2710 ch 8). Both carry the address in their
 * Multicast Address field.
 */
static int mld_v1_send(struct net_if *iface, uint8_t type, const struct net_in6_addr *addr)
{
	struct net_in6_addr all_routers;
	const struct net_in6_addr *dst = addr;
	struct net_pkt *pkt;
	int ret;

	if (type == NET_ICMPV6_MLDv1_DONE) {
		net_ipv6_addr_create_ll_allrouters_mcast(&all_routers);
		dst = &all_routers;
	}

	pkt = net_pkt_alloc_with_buffer(iface, IPV6_OPT_HDR_ROUTER_ALERT_LEN +
					NET_ICMPV6_UNUSED_LEN +
					sizeof(struct mld_query_common),
					NET_AF_INET6, NET_IPPROTO_ICMPV6,
					PKT_WAIT_TIME);
	if (pkt == NULL) {
		return -ENOMEM;
	}

	ret = mld_create_ipv6(pkt, dst);
	if (ret < 0) {
		goto drop;
	}

	/* ICMPv6 header, Maximum Response Delay and Reserved set to zero */
	if (net_icmpv6_create(pkt, type, 0) ||
	    net_pkt_write_be16(pkt, 0) ||
	    net_pkt_write_be16(pkt, 0) ||
	    net_pkt_write(pkt, addr, sizeof(struct net_in6_addr))) {
		ret = -ENOBUFS;
		goto drop;
	}

	ret = mld_send(pkt);
	if (ret < 0) {
		goto drop;
	}

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}

static int mld_v2_send_single(struct net_if *iface, const struct net_in6_addr *addr, uint8_t mode)
{
	struct net_pkt *pkt;
	int ret;

	pkt = net_pkt_alloc_with_buffer(iface, IPV6_OPT_HDR_ROUTER_ALERT_LEN +
					NET_ICMPV6_UNUSED_LEN +
					MLDv2_MCAST_RECORD_LEN +
					sizeof(struct net_in6_addr),
					NET_AF_INET6, NET_IPPROTO_ICMPV6,
					PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOMEM;
	}

	if (mld_create_packet(pkt, 1) ||
	    mld_create(pkt, addr, mode)) {
		ret = -ENOBUFS;
		goto drop;
	}

	ret = mld_send(pkt);
	if (ret) {
		goto drop;
	}

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}

/* Report one address in the version the host speaks on the interface. In
 * MLDv1 a change to INCLUDE mode, that is a leave, becomes a Done message
 * and every other record a Report.
 */
int net_ipv6_mld_send_single(struct net_if *iface, const struct net_in6_addr *addr, uint8_t mode)
{
	if (!mld_is_reported(addr)) {
		/* No MLD messages for this address, RFC 3810 ch 6 */
		return 0;
	}

	if (mld_host_version(iface->config.ip.ipv6) == MLDV1) {
		uint8_t type = mode == NET_IPV6_MLDv2_CHANGE_TO_INCLUDE_MODE
				       ? NET_ICMPV6_MLDv1_DONE
				       : NET_ICMPV6_MLDv1_REPORT;

		return mld_v1_send(iface, type, addr);
	}

	return mld_v2_send_single(iface, addr, mode);
}

int net_ipv6_mld_rejoin(struct net_if *iface, struct net_if_mcast_addr *addr)
{
	int ret;

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
		return 0;
	}

	if (net_if_is_offloaded(iface) || !mld_is_reported(&addr->address.in6_addr)) {
		goto out;
	}

	ret = net_ipv6_mld_send_single(iface, &addr->address.in6_addr,
				       NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE);
	if (ret < 0) {
		return ret;
	}

	mld_retransmit_schedule(iface, addr);

out:
	net_if_ipv6_maddr_join(iface, addr);

	net_if_mcast_monitor(iface, &addr->address, true);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_MCAST_JOIN, iface,
					&addr->address.in6_addr,
					sizeof(struct net_in6_addr));

	return 0;
}

int net_ipv6_mld_join(struct net_if *iface, const struct net_in6_addr *addr)
{
	struct net_if_mcast_addr *maddr;
	int ret = 0;

	maddr = net_if_ipv6_maddr_add(iface, addr);
	if (maddr == NULL) {
		return -ENOMEM;
	}

	if (net_if_ipv6_maddr_is_joined(maddr)) {
		return 0;
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
		return 0;
	}

	if (net_if_is_offloaded(iface) || !mld_is_reported(addr)) {
		goto out;
	}

	ret = net_ipv6_mld_send_single(iface, addr, NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE);
	if (ret < 0) {
		/* -ENETDOWN Indicate that network interface is down - this may
		 * happen and should not be considered fatal, address group will
		 * be joined when the interface goes up. Any other error should
		 * be considered fatal though and address should be cleaned up.
		 */
		if (ret != -ENETDOWN) {
			net_if_ipv6_maddr_rm(iface, addr);
		}

		return ret;
	}

	mld_retransmit_schedule(iface, maddr);

out:
	net_if_ipv6_maddr_join(iface, maddr);

	net_if_mcast_monitor(iface, &maddr->address, true);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_MCAST_JOIN, iface,
					&maddr->address.in6_addr,
					sizeof(struct net_in6_addr));

	return ret;
}

int net_ipv6_mld_leave(struct net_if *iface, const struct net_in6_addr *addr)
{
	struct net_if_mcast_addr *maddr;
	struct net_addr removed_addr;
	int ret = 0;

	maddr = net_if_ipv6_maddr_lookup(addr, &iface);
	if (maddr == NULL) {
		return -ENOENT;
	}

	removed_addr = maddr->address;
	if (!net_if_ipv6_maddr_rm(iface, addr)) {
		/* Address still in use */
		return 0;
	}

	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
		return 0;
	}

	if (net_if_is_offloaded(iface) || !mld_is_reported(addr)) {
		goto out;
	}

	ret = net_ipv6_mld_send_single(iface, addr, NET_IPV6_MLDv2_CHANGE_TO_INCLUDE_MODE);
	if (ret < 0) {
		return ret;
	}

out:
	net_if_mcast_monitor(iface, &removed_addr, false);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_MCAST_LEAVE, iface,
					&removed_addr.in6_addr,
					sizeof(struct net_in6_addr));

	return ret;
}

void net_ipv6_mld_send_leave(struct net_if *iface, const struct net_if_mcast_addr *addr)
{
	if (net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
		return;
	}

	if (net_if_is_offloaded(iface) || !mld_is_reported(&addr->address.in6_addr)) {
		goto out;
	}

	net_ipv6_mld_send_single(iface, &addr->address.in6_addr,
				 NET_IPV6_MLDv2_CHANGE_TO_INCLUDE_MODE);

out:
	net_if_mcast_monitor(iface, &addr->address, false);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_MCAST_LEAVE, iface,
					&addr->address.in6_addr,
					sizeof(struct net_in6_addr));
}

#if defined(CONFIG_NET_IPV6_MCAST_ROUTE_MLD_REPORTS)
static void send_v1_mcast_route_report(struct net_route_ipv6_entry_mcast *entry, void *user_data)
{
	struct mcast_route_appending_info *info = user_data;
	struct net_if_mcast_addr *maddr;
	struct net_if *iface = info->iface;

	if (info->status != 0 || entry->prefix_len != 128 || !mld_is_reported(&entry->group)) {
		return;
	}

	maddr = net_if_ipv6_maddr_lookup(&entry->group, &iface);
	if (maddr != NULL && net_if_ipv6_maddr_is_joined(maddr)) {
		/* Reported with the joined groups already */
		return;
	}

	info->status = mld_v1_send(info->iface, NET_ICMPV6_MLDv1_REPORT, &entry->group);
}
#endif

/* Respond to a General Query in MLDv1: one Report per listened to group
 * (RFC 2710 ch 4), plus one per multicast route when so configured.
 */
static int send_mld_v1_reports(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	int ret = 0;

	ARRAY_FOR_EACH(ipv6->mcast, i) {
		if (!ipv6->mcast[i].is_used || !ipv6->mcast[i].is_joined ||
		    !mld_is_reported(&ipv6->mcast[i].address.in6_addr)) {
			continue;
		}

		ret = mld_v1_send(iface, NET_ICMPV6_MLDv1_REPORT,
				  &ipv6->mcast[i].address.in6_addr);
		if (ret < 0) {
			return ret;
		}
	}

#if defined(CONFIG_NET_IPV6_MCAST_ROUTE_MLD_REPORTS)
	struct mcast_route_appending_info info = {
		.iface = iface,
	};

	net_route_ipv6_mcast_foreach(send_v1_mcast_route_report, NULL, &info);

	ret = info.status;
#endif

	return ret;
}

/* Respond to a General Query with the current state of every listened to
 * group, packed into one report (RFC 3810 ch 6.3), including the multicast
 * routes when so configured.
 */
static int send_mld_report(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	struct net_pkt *pkt;
	int i, count = 0;
	int ret;

	NET_ASSERT(ipv6);

	if (mld_host_version(ipv6) == MLDV1) {
		return send_mld_v1_reports(iface);
	}

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!ipv6->mcast[i].is_used || !ipv6->mcast[i].is_joined ||
		    !mld_is_reported(&ipv6->mcast[i].address.in6_addr)) {
			continue;
		}

		count++;
	}

#if defined(CONFIG_NET_IPV6_MCAST_ROUTE_MLD_REPORTS)
	/* Increase number of slots by a number of multicast routes that
	 * can be later added to the report. Checking for duplicates is done
	 * while appending an entry.
	 */
	net_route_ipv6_mcast_foreach(count_mcast_routes, NULL, (void *)&count);
#endif

	if (count == 0) {
		/* Nothing to report, RFC 3810 ch 6.2 */
		return 0;
	}

	pkt = net_pkt_alloc_with_buffer(iface, IPV6_OPT_HDR_ROUTER_ALERT_LEN +
					NET_ICMPV6_UNUSED_LEN +
					count * MLDv2_MCAST_RECORD_LEN,
					NET_AF_INET6, NET_IPPROTO_ICMPV6,
					PKT_WAIT_TIME);
	if (!pkt) {
		return -ENOBUFS;
	}

	ret = mld_create_packet(pkt, count);
	if (ret < 0) {
		goto drop;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_MADDR; i++) {
		if (!ipv6->mcast[i].is_used || !ipv6->mcast[i].is_joined ||
		    !mld_is_reported(&ipv6->mcast[i].address.in6_addr)) {
			continue;
		}

		ret = mld_create(pkt, &ipv6->mcast[i].address.in6_addr,
				 NET_IPV6_MLDv2_MODE_IS_EXCLUDE);
		if (ret < 0) {
			goto drop;
		}
	}

#if defined(CONFIG_NET_IPV6_MCAST_ROUTE_MLD_REPORTS)
	/* Append information about multicast routes as packets will be
	 * forwarded to these interfaces on reception.
	 */
	struct mcast_route_appending_info info;

	info.status = 0;
	info.pkt = pkt;
	info.iface = iface;
	info.skipped = 0;

	net_route_ipv6_mcast_foreach(append_mcast_routes, NULL, &info);

	ret = info.status;
	if (ret < 0) {
		goto drop;
	}

	/* We may have skipped duplicated addresses that we reserved space for,
	 * modify number of records.
	 */
	if (info.skipped) {
		net_pkt_cursor_init(pkt);
		net_pkt_set_overwrite(pkt, true);

		if (net_pkt_skip(pkt, net_pkt_ip_hdr_len(pkt) + net_pkt_ipv6_ext_len(pkt) +
					      sizeof(struct net_icmp_hdr) +
					      MLDV2_REPORT_RESERVED_BYTES) < 0) {
			goto drop;
		}

		count -= info.skipped;

		ret = net_pkt_write_be16(pkt, count);
		if (ret < 0) {
			goto drop;
		}

		net_pkt_remove_tail(pkt, info.skipped * sizeof(struct net_icmpv6_mld_mcast_record));
	}
#endif

	ret = mld_send(pkt);
	if (ret < 0) {
		goto drop;
	}

	return 0;

drop:
	net_pkt_unref(pkt);

	return ret;
}

static int mld_router_alert_cb(struct net_pkt *pkt, uint8_t hdr_type, uint8_t opt_type,
			       uint8_t opt_len, void *user_data)
{
	bool *found = user_data;
	uint16_t value;

	if (hdr_type != NET_IPV6_NEXTHDR_HBHO || opt_type != NET_IPV6_EXT_HDR_OPT_RTR_ALERT ||
	    opt_len != sizeof(value)) {
		return 0;
	}

	if (net_pkt_read_be16(pkt, &value) < 0) {
		return -EINVAL;
	}

	*found = value == IPV6_OPT_ROUTER_ALERT_MLD;

	return *found ? 1 : 0;
}

/* Queries carry the Router Alert option in a Hop-by-Hop Options header
 * (RFC 3810 ch 5), a query without it is dropped (ch 6.2).
 */
static bool mld_has_router_alert(struct net_pkt *pkt, const struct net_ipv6_hdr *ip_hdr)
{
	bool found = false;

	if (ip_hdr->nexthdr != NET_IPV6_NEXTHDR_HBHO) {
		return false;
	}

	if (net_ipv6_parse_ext_hdr_options(pkt, mld_router_alert_cb, &found) < 0) {
		return false;
	}

	return found;
}

/* Respond to a query with the current state of the given group, or of every
 * listened to group for a General Query (group is NULL).
 */
static void mld_query_respond(struct net_if *iface, struct net_if_mcast_addr *group)
{
	int ret;

	if (group != NULL) {
		ret = net_ipv6_mld_send_single(iface, &group->address.in6_addr,
					       NET_IPV6_MLDv2_MODE_IS_EXCLUDE);
	} else {
		ret = send_mld_report(iface);
	}

	if (ret < 0) {
		NET_DBG("Cannot send MLD report (%d)", ret);
	}
}

/* Rules 3 and 4 of RFC 3810 ch 6.2: a response for the group is sent at the
 * earliest of a pending response and the new delay. Called with mld_lock
 * held.
 */
static void mld_group_schedule(struct net_if_mcast_addr *group, k_timepoint_t delay)
{
	group->mld_resp_timeout = mld_timepoint_min(group->mld_resp_timeout, delay);

	mld_timer_arm(group->mld_resp_timeout);
}

/* Schedule the response to a query after a random delay bounded by its
 * Maximum Response Delay, RFC 3810 ch 6.2. A General Query has group NULL.
 */
static void mld_query_schedule(struct net_if *iface, struct net_if_mcast_addr *group,
			       uint32_t max_resp_ms)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	k_timepoint_t delay;

	k_mutex_lock(&mld_lock, K_FOREVER);

	delay = mld_random_delay(max_resp_ms);

	/* Rule 1: a pending response to a General Query due sooner covers this
	 * query as well.
	 */
	if (sys_timepoint_cmp(ipv6->mld_general_timeout, delay) <= 0) {
		goto out;
	}

	if (group != NULL) {
		mld_group_schedule(group, delay);
	} else if (mld_host_version(ipv6) == MLDV2) {
		/* Rule 2: the Interface Timer answers with a single report */
		ipv6->mld_general_timeout = delay;
		mld_timer_arm(delay);
	} else {
		/* MLDv1 answers with one report per group, each after its own
		 * random delay (RFC 2710 ch 4).
		 */
		ARRAY_FOR_EACH(ipv6->mcast, i) {
			if (ipv6->mcast[i].is_used && ipv6->mcast[i].is_joined &&
			    mld_is_reported(&ipv6->mcast[i].address.in6_addr)) {
				mld_group_schedule(&ipv6->mcast[i], mld_random_delay(max_resp_ms));
			}
		}
	}

out:
	k_mutex_unlock(&mld_lock);
}

static uint32_t mld_unsolicited_report_interval(const struct net_if_ipv6 *ipv6)
{
	if (mld_host_version(ipv6) == MLDV1) {
		return MLDV1_UNSOLICITED_REPORT_INTERVAL_MS;
	}

	return MLDV2_UNSOLICITED_REPORT_INTERVAL_MS;
}

/* The unsolicited report of a join may get lost, so it is repeated
 * [Robustness Variable] - 1 times at random intervals within the
 * Unsolicited Report Interval (RFC 3810 ch 6.1, RFC 2710 ch 4). Called after
 * the first report of the join has been sent.
 */
static void mld_retransmit_schedule(struct net_if *iface, struct net_if_mcast_addr *group)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	k_mutex_lock(&mld_lock, K_FOREVER);

	group->mld_retx_left = CONFIG_NET_IPV6_MLD_ROBUSTNESS - 1;

	if (group->mld_retx_left > 0U) {
		group->mld_retx_timeout = mld_random_delay(mld_unsolicited_report_interval(ipv6));
		mld_timer_arm(group->mld_retx_timeout);
	} else {
		group->mld_retx_timeout = mld_timepoint_never();
	}

	k_mutex_unlock(&mld_lock);
}

/* Retransmit the unsolicited report of a group when due and note the next
 * retransmission in next. Called with mld_lock held, which is released while
 * the report is sent.
 */
static void mld_group_retransmit(struct net_if *iface, struct net_if_ipv6 *ipv6,
				 struct net_if_mcast_addr *group, k_timepoint_t *next)
{
	int ret;

	if (group->mld_retx_left == 0U) {
		return;
	}

	if (!sys_timepoint_expired(group->mld_retx_timeout)) {
		*next = mld_timepoint_min(*next, group->mld_retx_timeout);
		return;
	}

	if (!group->is_joined || !mld_is_reported(&group->address.in6_addr)) {
		/* Left or the interface went down in the meantime */
		group->mld_retx_left = 0U;
		group->mld_retx_timeout = mld_timepoint_never();
		return;
	}

	k_mutex_unlock(&mld_lock);
	ret = net_ipv6_mld_send_single(iface, &group->address.in6_addr,
				       NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE);
	k_mutex_lock(&mld_lock, K_FOREVER);

	if (ret < 0) {
		NET_DBG("Cannot retransmit MLD report (%d)", ret);
	}

	/* The group may have been left while the report was sent */
	if (group->mld_retx_left > 0U) {
		group->mld_retx_left--;
	}

	if (group->mld_retx_left > 0U) {
		group->mld_retx_timeout = mld_random_delay(mld_unsolicited_report_interval(ipv6));
		*next = mld_timepoint_min(*next, group->mld_retx_timeout);
	} else {
		group->mld_retx_timeout = mld_timepoint_never();
	}
}

/* Send the responses and retransmissions of the interface that are due and
 * note the earliest remaining one in next. Called with mld_lock held, which
 * is released while a message is sent.
 */
static void mld_iface_timeout(struct net_if *iface, struct net_if_ipv6 *ipv6, k_timepoint_t *next)
{
	/* An expired querier present timer changes the compatibility mode */
	mld_version_update(ipv6);
	if (!sys_timepoint_expired(ipv6->mld_v1_querier_timeout)) {
		*next = mld_timepoint_min(*next, ipv6->mld_v1_querier_timeout);
	}

	if (sys_timepoint_expired(ipv6->mld_general_timeout)) {
		ipv6->mld_general_timeout = mld_timepoint_never();
		k_mutex_unlock(&mld_lock);
		mld_query_respond(iface, NULL);
		k_mutex_lock(&mld_lock, K_FOREVER);
	} else {
		*next = mld_timepoint_min(*next, ipv6->mld_general_timeout);
	}

	ARRAY_FOR_EACH(ipv6->mcast, i) {
		struct net_if_mcast_addr *mcast = &ipv6->mcast[i];

		if (!mcast->is_used) {
			continue;
		}

		if (sys_timepoint_expired(mcast->mld_resp_timeout)) {
			mcast->mld_resp_timeout = mld_timepoint_never();

			if (mcast->is_joined && mld_is_reported(&mcast->address.in6_addr)) {
				k_mutex_unlock(&mld_lock);
				mld_query_respond(iface, mcast);
				k_mutex_lock(&mld_lock, K_FOREVER);
			}
		} else {
			*next = mld_timepoint_min(*next, mcast->mld_resp_timeout);
		}

		mld_group_retransmit(iface, ipv6, mcast, next);
	}
}

static void mld_timeout(struct k_work *work)
{
	k_timepoint_t next = mld_timepoint_never();

	ARG_UNUSED(work);

	k_mutex_lock(&mld_lock, K_FOREVER);

	STRUCT_SECTION_FOREACH(net_if, iface) {
		struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

		if (ipv6 == NULL || net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD)) {
			continue;
		}

		mld_iface_timeout(iface, ipv6, &next);
	}

	if (!mld_timepoint_is_never(next)) {
		(void)k_work_reschedule(&mld_timer, sys_timepoint_timeout(next));
	}

	k_mutex_unlock(&mld_lock);
}

#define dbg_addr(action, pkt_str, src, dst)				\
	do {								\
		NET_DBG("%s %s from %s to %s", action, pkt_str,         \
			net_sprint_ipv6_addr(src),		\
			net_sprint_ipv6_addr(dst));		\
	} while (0)

#define dbg_addr_recv(pkt_str, src, dst)	\
	dbg_addr("Received", pkt_str, src, dst)

static enum net_verdict handle_mld_query(struct net_icmp_ctx *ctx,
					 struct net_pkt *pkt,
					 struct net_icmp_ip_hdr *hdr,
					 struct net_icmp_hdr *icmp_hdr,
					 void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(mld_access, struct mld_query_common);
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	uint16_t length = net_pkt_get_len(pkt);
	struct mld_query_common *mld_query;
	struct net_if_mcast_addr *maddr = NULL;
	struct net_pkt_cursor backup;
	enum mld_version version;
	uint16_t num_sources = 0U;
	size_t mld_len;
	uint16_t pkt_len;
	int ret = -EIO;

	net_pkt_cursor_backup(pkt, &backup);

	/* The version of a query follows from its length, RFC 3810 ch 8.1 */
	mld_len = net_pkt_remaining_data(pkt) + sizeof(struct net_icmp_hdr);
	if (mld_len == MLD_V1_QUERY_LEN) {
		version = MLDV1;
	} else if (mld_len >= MLD_V2_QUERY_MIN_LEN) {
		version = MLDV2;
	} else {
		NET_DBG("DROP: unsupported query length %zu", mld_len);
		goto drop;
	}

	mld_query = (struct mld_query_common *)net_pkt_get_data(pkt, &mld_access);
	if (mld_query == NULL) {
		NET_DBG("DROP: NULL MLD query");
		goto drop;
	}

	ret = net_pkt_acknowledge_data(pkt, &mld_access);
	if (ret < 0) {
		NET_DBG("DROP: cannot acknowledge data");
		goto drop;
	}

	if (version == MLDV2) {
		/* Skip Resv, S, QRV and QQIC, then the Number of Sources */
		if (net_pkt_skip(pkt, sizeof(uint16_t)) < 0 ||
		    net_pkt_read_be16(pkt, &num_sources) < 0) {
			NET_DBG("DROP: cannot read the number of sources");
			ret = -EIO;
			goto drop;
		}
	}

	dbg_addr_recv("Multicast Listener Query", &ip_hdr->src, &ip_hdr->dst);

	net_stats_update_ipv6_mld_recv(net_pkt_iface(pkt));

	pkt_len = sizeof(struct net_ipv6_hdr) + net_pkt_ipv6_ext_len(pkt) +
		  (version == MLDV1 ? MLD_V1_QUERY_LEN : MLD_V2_QUERY_MIN_LEN) +
		  sizeof(struct net_in6_addr) * num_sources;

	if (length < pkt_len || pkt_len > NET_IPV6_MTU ||
	    ip_hdr->hop_limit != 1U || icmp_hdr->code != 0U) {
		ret = -EIO;
		goto drop;
	}

	/* A query comes from a link-local address with the Router Alert
	 * option, RFC 3810 ch 5.1.14 and 6.2.
	 */
	if (!net_ipv6_is_ll_addr_raw(ip_hdr->src)) {
		NET_DBG("DROP: query source is not link-local");
		goto drop;
	}

	if (!mld_has_router_alert(pkt, ip_hdr)) {
		NET_DBG("DROP: query without Router Alert option");
		goto drop;
	}

	if (net_pkt_iface(pkt)->config.ip.ipv6 == NULL) {
		NET_DBG("DROP: no IPv6 configuration");
		goto drop;
	}

	mld_querier_seen(net_pkt_iface(pkt)->config.ip.ipv6, version);

	if (!net_ipv6_addr_cmp_raw(mld_query->mcast_address,
				   (uint8_t *)net_ipv6_unspecified_address())) {
		/* A Multicast Address Specific Query is answered with the state
		 * of that address only, and only by its listeners (RFC 3810 ch
		 * 6.3). Multicast Address and Source Specific Queries are
		 * answered the same way.
		 */
		struct net_if *iface = net_pkt_iface(pkt);
		struct net_in6_addr group;

		net_ipv6_addr_copy_raw(group.s6_addr, mld_query->mcast_address);

		maddr = net_if_ipv6_maddr_lookup(&group, &iface);
		if (maddr == NULL || !net_if_ipv6_maddr_is_joined(maddr) ||
		    !mld_is_reported(&group)) {
			NET_DBG("Ignoring query for group %s", net_sprint_ipv6_addr(&group));
			goto out;
		}
	}

	mld_query_schedule(net_pkt_iface(pkt), maddr,
			   net_ipv6_mld_max_resp_delay(net_ntohs(mld_query->max_response_code),
						       version == MLDV2));

out:
	net_pkt_cursor_restore(pkt, &backup);
	return NET_CONTINUE;

drop:
	net_stats_update_ipv6_mld_drop(net_pkt_iface(pkt));

	net_pkt_cursor_restore(pkt, &backup);
	return ret < 0 ? NET_DROP : NET_CONTINUE;
}

/* Reports sent before the interface had a valid link-local address carried the
 * unspecified source, which routers discard. Announce every group again once
 * it does, RFC 3810 ch 5.2.13.
 */
void net_ipv6_mld_report_all(struct net_if *iface)
{
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;

	if (ipv6 == NULL || net_if_flag_is_set(iface, NET_IF_IPV6_NO_MLD) ||
	    net_if_is_offloaded(iface)) {
		return;
	}

	ARRAY_FOR_EACH(ipv6->mcast, i) {
		struct net_if_mcast_addr *maddr = &ipv6->mcast[i];
		int ret;

		if (!maddr->is_used || !maddr->is_joined ||
		    !mld_is_reported(&maddr->address.in6_addr)) {
			continue;
		}

		ret = net_ipv6_mld_send_single(iface, &maddr->address.in6_addr,
					       NET_IPV6_MLDv2_CHANGE_TO_EXCLUDE_MODE);
		if (ret < 0) {
			NET_DBG("Cannot report %s (%d)",
				net_sprint_ipv6_addr(&maddr->address.in6_addr), ret);
			continue;
		}

		mld_retransmit_schedule(iface, maddr);
	}
}

/* Another listener reported the address, so this node does not have to: in
 * MLDv1 mode the pending response and retransmission for the address are
 * cancelled (RFC 2710 ch 4). In MLDv2 mode reports are not suppressed
 * (RFC 3810 ch 8.2.2). Only a report that came from the link, with a
 * link-local source and a hop limit of 1, is taken into account.
 */
static enum net_verdict handle_mld_v1_report(struct net_icmp_ctx *ctx, struct net_pkt *pkt,
					     struct net_icmp_ip_hdr *hdr,
					     struct net_icmp_hdr *icmp_hdr, void *user_data)
{
	NET_PKT_DATA_ACCESS_CONTIGUOUS_DEFINE(mld_access, struct mld_query_common);
	struct net_if *iface = net_pkt_iface(pkt);
	struct net_if_ipv6 *ipv6 = iface->config.ip.ipv6;
	struct net_ipv6_hdr *ip_hdr = hdr->ipv6;
	struct mld_query_common *report;
	struct net_if_mcast_addr *maddr;
	struct net_pkt_cursor backup;
	struct net_in6_addr group;

	ARG_UNUSED(ctx);
	ARG_UNUSED(icmp_hdr);
	ARG_UNUSED(user_data);

	net_pkt_cursor_backup(pkt, &backup);

	if (ipv6 == NULL || ip_hdr->hop_limit != 1U || !net_ipv6_is_ll_addr_raw(ip_hdr->src)) {
		goto out;
	}

	/* A Version 1 Report has the layout of a Version 1 Query */
	report = (struct mld_query_common *)net_pkt_get_data(pkt, &mld_access);
	if (report == NULL) {
		goto out;
	}

	net_ipv6_addr_copy_raw(group.s6_addr, report->mcast_address);

	maddr = net_if_ipv6_maddr_lookup(&group, &iface);
	if (maddr == NULL) {
		goto out;
	}

	k_mutex_lock(&mld_lock, K_FOREVER);

	if (mld_host_version(ipv6) == MLDV1) {
		maddr->mld_resp_timeout = mld_timepoint_never();
		maddr->mld_retx_timeout = mld_timepoint_never();
		maddr->mld_retx_left = 0U;
	}

	k_mutex_unlock(&mld_lock);

out:
	net_pkt_cursor_restore(pkt, &backup);

	return NET_CONTINUE;
}

void net_ipv6_mld_init(void)
{
	static struct net_icmp_ctx query_ctx;
	static struct net_icmp_ctx report_ctx;
	int ret;

	ret = net_icmp_init_ctx(&query_ctx, NET_AF_INET6, NET_ICMPV6_MLD_QUERY, 0,
				handle_mld_query);
	if (ret < 0) {
		NET_ERR("Cannot register %s handler (%d)", STRINGIFY(NET_ICMPV6_MLD_QUERY),
			ret);
	}

	ret = net_icmp_init_ctx(&report_ctx, NET_AF_INET6, NET_ICMPV6_MLDv1_REPORT, 0,
				handle_mld_v1_report);
	if (ret < 0) {
		NET_ERR("Cannot register %s handler (%d)", STRINGIFY(NET_ICMPV6_MLDv1_REPORT),
			ret);
	}
}
