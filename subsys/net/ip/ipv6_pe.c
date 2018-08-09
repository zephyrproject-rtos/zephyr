/** @file
 * @brief IPv6 privacy extension related functions
 */

/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <logging/log.h>
LOG_MODULE_DECLARE(net_ipv6, CONFIG_NET_IPV6_LOG_LEVEL);

#include <zephyr.h>
#include <errno.h>
#include <stdlib.h>

#if defined(CONFIG_NET_IPV6_PE_ENABLE)
#include <mbedtls/md5.h>
#endif

#include <net/net_core.h>
#include <net/net_pkt.h>
#include <net/net_if.h>

#include "net_private.h"
#include "ipv6.h"

/* IPv6 privacy extension (RFC 4941) constants. Note that the code uses
 * seconds value internally for applicaple options. These are also values
 * that can be changed at runtime if needed as recommended in RFC 4941
 * chapter 3.5.
 */
static u32_t TEMP_VALID_LIFETIME = CONFIG_NET_IPV6_PE_TEMP_VALID_LIFETIME * 60;
static u32_t TEMP_PREFERRED_LIFETIME =
	CONFIG_NET_IPV6_PE_TEMP_PREFERRED_LIFETIME * 60;

#define REGEN_ADVANCE		CONFIG_NET_IPV6_PE_REGEN_ADVANCE
#define MAX_DESYNC_FACTOR       (CONFIG_NET_IPV6_PE_MAX_DESYNC_FACTOR * 60)

/* RFC 4941 ch 5. "DESYNC_FACTOR is a random value within the range 0 -
 * MAX_DESYNC_FACTOR.  It is computed once at system start (rather than
 * each time it is used) and must never be greater than
 * (TEMP_VALID_LIFETIME - REGEN_ADVANCE).
 *
 * The RFC is contradicting as if one changes TEMP_VALID_LIFETIME at runtime,
 * then DESYNC_FACTOR should be recalculated because it cannot be greater than
 * TEMP_VALID_LIFETIME - REGEN_ADVANCE. But the RFC also says that the
 * DESYNC_FACTOR value is only computed once at system start. For a sake of
 * clarity, lets assume that if TEMP_VALID_LIFETIME is changed at runtime, we
 * need to re-calculate DESYNC_FACTOR accordingly.
 */
static u32_t DESYNC_FACTOR;

#define TEMP_IDGEN_RETRIES      CONFIG_NET_IPV6_PE_TEMP_IDGEN_RETRIES

#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
/* Is this blacklisting filter or not */
static bool ipv6_pe_blacklist;
static struct in6_addr ipv6_pe_filter[CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT];
#endif

static bool ipv6_pe_use_this_prefix(const struct in6_addr *prefix)
{
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	int i, filters_found = false;

	for (i = 0; i < CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT; i++) {
		if (net_ipv6_is_addr_unspecified(&ipv6_pe_filter[i])) {
			continue;
		}

		filters_found = true;

		if (net_ipv6_addr_cmp(prefix, &ipv6_pe_filter[i])) {
			if (ipv6_pe_blacklist) {
				return false;
			} else {
				return true;
			}
		}
	}

	if (filters_found) {
		/* There was no match so if we are blacklisting, then this
		 * address must be acceptable.
		 */
		if (ipv6_pe_blacklist) {
			return true;
		}

		return false;
	}

	return true;
#else
	return true;
#endif
}

static bool ipv6_pe_prefix_already_exists(struct net_if_ipv6 *ipv6,
					  const struct in6_addr *prefix)
{
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    !ipv6->unicast[i].is_temporary) {
			continue;
		}

		if (net_ipv6_is_prefix(
			    (u8_t *)&ipv6->unicast[i].address.in6_addr,
			    (u8_t *)prefix, 64)) {
			return true;
		}
	}

	return false;
}

static int ipv6_pe_prefix_remove(struct net_if *iface,
				 struct net_if_ipv6 *ipv6,
				 const struct in6_addr *prefix)
{
	int count = 0;
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (ipv6->unicast[i].is_used &&
		    ipv6->unicast[i].address.family == AF_INET6 &&
		    ipv6->unicast[i].is_temporary &&
		    net_ipv6_is_prefix(
			    (u8_t *)&ipv6->unicast[i].address.in6_addr,
			    (u8_t *)prefix, 64)) {
			net_if_ipv6_addr_rm(iface,
					    &ipv6->unicast[i].address.in6_addr);
			count++;
		}
	}

	return count;
}

static bool ipv6_pe_prefix_update_lifetimes(struct net_if_ipv6 *ipv6,
					    const struct in6_addr *prefix,
					    u32_t vlifetime)
{
	s32_t addr_age, new_age;
	int i;

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!(ipv6->unicast[i].is_used &&
		      ipv6->unicast[i].address.family == AF_INET6 &&
		      ipv6->unicast[i].is_temporary &&
		      ipv6->unicast[i].addr_state == NET_ADDR_PREFERRED &&
		      net_ipv6_is_prefix(
			      (u8_t *)&ipv6->unicast[i].address.in6_addr,
			      (u8_t *)prefix, 64))) {
			continue;
		}

		addr_age = (u32_t)(k_uptime_get() / 1000) -
			ipv6->unicast[i].addr_create_time;
		new_age = abs(addr_age) + vlifetime;

		if ((new_age >= TEMP_VALID_LIFETIME) ||
		    (new_age >= (TEMP_PREFERRED_LIFETIME - DESYNC_FACTOR))) {
			break;
		}

		net_if_ipv6_addr_update_lifetime(&ipv6->unicast[i], vlifetime);

		/* RFC 4941 ch 3.4, "... at most one temporary address per
		 * prefix should be in a non-deprecated state at any given
		 * time on a given interface."
		 * Because of this there is no need to continue the loop.
		 */
		return true;
	}

	return false;
}

void net_ipv6_pe_start(struct net_if *iface, const struct in6_addr *prefix,
		       u32_t vlifetime, u32_t preferred_lifetime)
{
	u8_t link_addr[NET_LINK_ADDR_MAX_LENGTH];
	struct net_linkaddr temp_link_id;
	struct net_if_addr *ifaddr;
	struct net_if_ipv6 *ipv6;
	struct in6_addr addr;
	u8_t md5[128 / 8];
	int i;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		NET_WARN("Cannot do %sDAD IPv6, config is not valid.", "PE ");
		return;
	}

	if (!ipv6) {
		return;
	}

	/* Check if user agrees to use this prefix */
	if (!ipv6_pe_use_this_prefix(prefix)) {
		NET_DBG("Prefix %s/64 is not to be used",
			net_sprint_ipv6_addr(prefix));
		return;
	}

	/* If the prefix is already added and it is still valid, then we do
	 * not try to add it again.
	 */
	if (ipv6_pe_prefix_already_exists(ipv6, prefix)) {
		if (vlifetime == 0) {
			i = ipv6_pe_prefix_remove(iface, ipv6, prefix);

			NET_DBG("Removed %d addresses using prefix %s/64",
				i, net_sprint_ipv6_addr(prefix));
			return;
		}

		ipv6_pe_prefix_update_lifetimes(ipv6, prefix, vlifetime);
		return;
	}

	preferred_lifetime = MIN(preferred_lifetime,
				 TEMP_PREFERRED_LIFETIME - DESYNC_FACTOR);
	if (preferred_lifetime == 0 || preferred_lifetime <= REGEN_ADVANCE) {
		NET_DBG("Too short preferred lifetime, temp address not "
			"created for prefix %s/64",
			net_sprint_ipv6_addr(prefix));
		return;
	}

	NET_DBG("Starting PE process for prefix %s/64",
		net_sprint_ipv6_addr(prefix));

	temp_link_id.len = net_if_get_link_addr(iface)->len;
	temp_link_id.type = net_if_get_link_addr(iface)->type;
	temp_link_id.addr = link_addr;

	/* TODO: remember the random address and use it to generate a new
	 * one. RFC 4941 ch 3.2.1
	 */
	for (i = 0; i < temp_link_id.len; i++) {
		link_addr[i] = sys_rand32_get();
	}

	/* Create IPv6 address using the given prefix and iid. We first
	 * setup link local address, and then copy prefix over first 8
	 * bytes of that address.
	 */
	net_ipv6_addr_create_iid(&addr, &temp_link_id);
	memcpy(&addr, prefix, sizeof(struct in6_addr) / 2);

	mbedtls_md5_ret(addr.s6_addr + 8, sizeof(addr) - 8, md5);

	memcpy(addr.s6_addr + 8, &md5[8], sizeof(addr) - 8);

	/* Turn OFF universal bit, RFC 4941, ch 3.2.1 bullet 3. */
	addr.s6_addr[8] ^= 0x02;

	vlifetime = MIN(TEMP_VALID_LIFETIME, vlifetime);

	ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF,
				      vlifetime);
	if (!ifaddr) {
		NET_ERR("Cannot add %s address to interface %p",
			net_sprint_ipv6_addr(&addr), iface);
		return;
	}

	ifaddr->is_temporary = true;
	ifaddr->addr_timeout = vlifetime;
	ifaddr->addr_create_time = (u32_t)(k_uptime_get() / 1000);

	NET_DBG("Starting DAD for %s iface %p", net_sprint_ipv6_addr(&addr),
		iface);

	net_if_ipv6_start_dad(iface, ifaddr);
}

#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0

static void iface_cb(struct net_if *iface, void *user_data)
{
	bool is_new_filter_blacklist = !ipv6_pe_blacklist;
	struct in6_addr *prefix = user_data;
	struct net_if_ipv6 *ipv6;
	int i, ret;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return;
	}

	if (!ipv6) {
		return;
	}

	for (i = 0; i < NET_IF_MAX_IPV6_ADDR; i++) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    !ipv6->unicast[i].is_temporary) {
			continue;
		}

		ret = net_ipv6_is_prefix(
				(u8_t *)&ipv6->unicast[i].address.in6_addr,
				(u8_t *)prefix, 64);

		/* TODO: Do this removal gracefully so that applications
		 * have time to cope with this change.
		 */
		if (is_new_filter_blacklist) {
			if (ret) {
				net_if_ipv6_addr_rm(iface,
					    &ipv6->unicast[i].address.in6_addr);
			}
		} else {
			if (!ret) {
				net_if_ipv6_addr_rm(iface,
					    &ipv6->unicast[i].address.in6_addr);
			}
		}
	}
}

/* If we change filter value, then check if existing IPv6 prefixes will
 * conflict with the new filter.
 */
static void ipv6_pe_recheck_filters(bool is_blacklist)
{
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT; i++) {
		if (net_ipv6_is_addr_unspecified(&ipv6_pe_filter[i])) {
			continue;
		}

		net_if_foreach(iface_cb, &ipv6_pe_filter[i]);
	}
}
#endif /* CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0 */

int net_ipv6_pe_add_filter(struct in6_addr *addr, bool is_blacklist)
{
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT; i++) {
		if (net_ipv6_is_addr_unspecified(&ipv6_pe_filter[i])) {
			net_ipaddr_copy(&ipv6_pe_filter[i], addr);

			if (ipv6_pe_blacklist != is_blacklist) {
				ipv6_pe_recheck_filters(is_blacklist);
			}

			ipv6_pe_blacklist = is_blacklist ? true : false;

			NET_DBG("Adding %s filter %s",
				ipv6_pe_blacklist ? "blacklist" : "whitelist",
				net_sprint_ipv6_addr(&ipv6_pe_filter[i]));

			return 0;
		}
	}

	return -ENOENT;
#else
	return -ENOTSUP;
#endif
}

int net_ipv6_pe_del_filter(struct in6_addr *addr)
{
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	int i;

	for (i = 0; i < CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT; i++) {
		if (net_ipv6_addr_cmp(&ipv6_pe_filter[i], addr)) {
			NET_DBG("Removing %s filter %s",
				ipv6_pe_blacklist ? "blacklist" : "whitelist",
				net_sprint_ipv6_addr(&ipv6_pe_filter[i]));

			net_ipaddr_copy(&ipv6_pe_filter[i],
					net_ipv6_unspecified_address());

			return 0;
		}
	}

	return -ENOENT;
#else
	return -ENOTSUP;
#endif
}

bool net_ipv6_pe_check_dad(int count)
{
	if (count > TEMP_IDGEN_RETRIES) {
		return false;
	}

	return true;
}

int net_ipv6_pe_filter_foreach(net_ipv6_pe_filter_cb_t cb, void *user_data)
{
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	int i, count = 0;

	for (i = 0; i < CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT; i++) {
		if (net_ipv6_is_addr_unspecified(&ipv6_pe_filter[i])) {
			continue;
		}

		cb(&ipv6_pe_filter[i], ipv6_pe_blacklist, user_data);

		count++;
	}

	return count;
#else
	return 0;
#endif
}

int net_ipv6_pe_init(struct net_if *iface)
{
	static bool init_done;
	u32_t lifetime;

	if (TEMP_VALID_LIFETIME - REGEN_ADVANCE <= 0) {
		iface->pe_enabled = false;
		return -EINVAL;
	}

	iface->pe_enabled = true;
	iface->pe_prefer_public =
		IS_ENABLED(CONFIG_NET_IPV6_PE_PREFER_PUBLIC_ADDRESSES) ?
		true : false;

	if (!init_done) {
		lifetime = TEMP_VALID_LIFETIME - REGEN_ADVANCE;

		/* RFC 4941 ch 5 */
		DESYNC_FACTOR = sys_rand32_get() % MIN(MAX_DESYNC_FACTOR,
						       lifetime);
		init_done = true;
	}

	return 0;
}

