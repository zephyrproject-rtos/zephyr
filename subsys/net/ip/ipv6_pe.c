/** @file
 * @brief IPv6 privacy extension (RFC 8981)
 */

/*
 * Copyright (c) 2018 Intel Corporation
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ipv6_pe, CONFIG_NET_IPV6_PE_LOG_LEVEL);

#include <errno.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/random/random.h>

#include <mbedtls/md.h>

#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/net_if.h>

#include "net_private.h"
#include "ipv6.h"

/* From RFC 5453 */
static const struct in6_addr reserved_anycast_subnet = { { {
			0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x80,
			0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
		} } };

/* RFC 8981 ch 3.8 preferred lifetime must be smaller than valid lifetime */
BUILD_ASSERT(CONFIG_NET_IPV6_PE_TEMP_PREFERRED_LIFETIME <
	     CONFIG_NET_IPV6_PE_TEMP_VALID_LIFETIME);

/* IPv6 privacy extension (RFC 8981) constants. Note that the code uses
 * seconds value internally for applicable options. These are also values
 * that can be changed at runtime if needed as recommended in RFC 8981
 * chapter 3.6.
 */
static uint32_t temp_valid_lifetime =
	CONFIG_NET_IPV6_PE_TEMP_VALID_LIFETIME * SEC_PER_MIN;
#define TEMP_VALID_LIFETIME temp_valid_lifetime

static uint32_t temp_preferred_lifetime =
	CONFIG_NET_IPV6_PE_TEMP_PREFERRED_LIFETIME * SEC_PER_MIN;
#define TEMP_PREFERRED_LIFETIME temp_preferred_lifetime

/* This is the upper bound on DESYNC_FACTOR. The value is in seconds.
 * See RFC 8981 ch 3.8 for details.
 *
 * RFC says the DESYNC_FACTOR should be 0.4 times the preferred lifetime.
 * This is too short for Zephyr as it means that the address is very long
 * time in deprecated state and not being used. Make this 7% of the preferred
 * time to deprecate the addresses later.
 */
#define MAX_DESYNC_FACTOR   ((uint32_t)((uint64_t)TEMP_PREFERRED_LIFETIME * \
				       (uint64_t)7U) / (uint64_t)100U)
#define DESYNC_FACTOR(ipv6) ((ipv6)->desync_factor)

#define TEMP_IDGEN_RETRIES      CONFIG_NET_IPV6_PE_TEMP_IDGEN_RETRIES

/* The REGEN_ADVANCE is in seconds
 * retrans_timer (in ms) is specified in RFC 4861
 * dup_addr_detect_transmits (in ms) is specified in RFC 4862
 */
static inline uint32_t REGEN_ADVANCE(uint32_t retrans_timer,
				     uint32_t dup_addr_detect_transmits)
{
	return (2U + (uint32_t)((uint64_t)TEMP_IDGEN_RETRIES *
				(uint64_t)retrans_timer *
				(uint64_t)dup_addr_detect_transmits /
				(uint64_t)1000U));
}

#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
/* Is this denylisting filter or not */
static bool ipv6_pe_denylist;
static struct in6_addr ipv6_pe_filter[CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT];

static K_MUTEX_DEFINE(lock);
#endif

/* We need to periodically update the private address. */
static struct k_work_delayable temp_lifetime;

static bool ipv6_pe_use_this_prefix(const struct in6_addr *prefix)
{
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	int filter_found = false;
	bool ret = true;

	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(ipv6_pe_filter, i) {
		if (net_ipv6_is_addr_unspecified(&ipv6_pe_filter[i])) {
			continue;
		}

		filter_found = true;

		if (net_ipv6_addr_cmp(prefix, &ipv6_pe_filter[i])) {
			if (ipv6_pe_denylist) {
				ret = false;
			}

			goto out;
		}
	}

	if (filter_found) {
		/* There was no match so if we are deny listing, then this
		 * address must be acceptable.
		 */
		if (!ipv6_pe_denylist) {
			ret = false;
			goto out;
		}
	}

out:
	k_mutex_unlock(&lock);

	return ret;
#else
	ARG_UNUSED(prefix);

	return true;
#endif
}

static bool ipv6_pe_prefix_already_exists(struct net_if_ipv6 *ipv6,
					  const struct in6_addr *prefix)
{
	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    !ipv6->unicast[i].is_temporary ||
		    ipv6->unicast[i].addr_state == NET_ADDR_DEPRECATED) {
			continue;
		}

		if (net_ipv6_is_prefix(
			    (uint8_t *)&ipv6->unicast[i].address.in6_addr,
			    (uint8_t *)prefix, 64)) {
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

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (ipv6->unicast[i].is_used &&
		    ipv6->unicast[i].address.family == AF_INET6 &&
		    ipv6->unicast[i].is_temporary &&
		    net_ipv6_is_prefix(
			    (uint8_t *)&ipv6->unicast[i].address.in6_addr,
			    (uint8_t *)prefix, 64)) {
			net_if_ipv6_addr_rm(iface,
					    &ipv6->unicast[i].address.in6_addr);
			count++;
		}
	}

	return count;
}

static bool ipv6_pe_prefix_update_lifetimes(struct net_if_ipv6 *ipv6,
					    const struct in6_addr *prefix,
					    uint32_t vlifetime)
{
	int32_t addr_age, new_age;

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (!(ipv6->unicast[i].is_used &&
		      ipv6->unicast[i].address.family == AF_INET6 &&
		      ipv6->unicast[i].is_temporary &&
		      ipv6->unicast[i].addr_state == NET_ADDR_PREFERRED &&
		      net_ipv6_is_prefix(
			      (uint8_t *)&ipv6->unicast[i].address.in6_addr,
			      (uint8_t *)prefix, 64))) {
			continue;
		}

		addr_age = k_uptime_seconds() - ipv6->unicast[i].addr_create_time;
		new_age = abs(addr_age) + vlifetime;

		if ((new_age >= TEMP_VALID_LIFETIME) ||
		    (new_age >= (TEMP_PREFERRED_LIFETIME -
				 DESYNC_FACTOR(ipv6)))) {
			break;
		}

		net_if_ipv6_addr_update_lifetime(&ipv6->unicast[i], vlifetime);

		/* RFC 8981 ch 3.5, "... at most one temporary address per
		 * prefix should be in a non-deprecated state at any given
		 * time on a given interface."
		 * Because of this there is no need to continue the loop.
		 */
		return true;
	}

	return false;
}

/* RFC 8981 ch 3.3.2 */
static int gen_temporary_iid(struct net_if *iface,
			     const struct in6_addr *prefix,
			     uint8_t *network_id, size_t network_id_len,
			     uint8_t dad_counter,
			     uint8_t *temporary_iid,
			     size_t temporary_iid_len)
{
	const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	mbedtls_md_context_t ctx;
	uint8_t digest[32];
	int ret;
	static bool once;
	static uint8_t secret_key[16]; /* Min 128 bits, RFC 8981 ch 3.3.2 */
	struct {
		struct in6_addr prefix;
		uint32_t current_time;
		uint8_t network_id[16];
		uint8_t mac[6];
		uint8_t dad_counter;
	} buf = {
		.current_time = k_uptime_get_32(),
		.dad_counter = dad_counter,
	};

	memcpy(&buf.prefix, prefix, sizeof(struct in6_addr));

	if (network_id != NULL && network_id_len > 0) {
		memcpy(buf.network_id, network_id,
		       MIN(network_id_len, sizeof(buf.network_id)));
	}

	memcpy(buf.mac, net_if_get_link_addr(iface)->addr,
	       MIN(sizeof(buf.mac), net_if_get_link_addr(iface)->len));

	if (!once) {
		sys_rand_get(&secret_key, sizeof(secret_key));
		once = true;
	}

	mbedtls_md_init(&ctx);
	ret = mbedtls_md_setup(&ctx, md_info, true);
	if (ret != 0) {
		NET_DBG("Cannot %s hmac (%d)", "setup", ret);
		goto err;
	}

	ret = mbedtls_md_hmac_starts(&ctx, secret_key, sizeof(secret_key));
	if (ret != 0) {
		NET_DBG("Cannot %s hmac (%d)", "start", ret);
		goto err;
	}

	ret = mbedtls_md_hmac_update(&ctx, (uint8_t *)&buf, sizeof(buf));
	if (ret != 0) {
		NET_DBG("Cannot %s hmac (%d)", "update", ret);
		goto err;
	}

	ret = mbedtls_md_hmac_finish(&ctx, digest);
	if (ret != 0) {
		NET_DBG("Cannot %s hmac (%d)", "finish", ret);
		goto err;
	}

	memcpy(temporary_iid, digest, MIN(sizeof(digest), temporary_iid_len));

err:
	mbedtls_md_free(&ctx);

	return ret;
}

void net_ipv6_pe_start(struct net_if *iface, const struct in6_addr *prefix,
		       uint32_t vlifetime, uint32_t preferred_lifetime)
{
	struct net_if_addr *ifaddr;
	struct net_if_ipv6 *ipv6;
	struct in6_addr addr;
	k_ticks_t remaining;
	k_timeout_t vlifetimeout;
	int i, ret, dad_count = 1;
	int32_t lifetime;
	bool valid = false;

	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		NET_WARN("Cannot do DAD IPv6 config is not valid.");
		goto out;
	}

	if (!ipv6) {
		goto out;
	}

	/* Check if user agrees to use this prefix */
	if (!ipv6_pe_use_this_prefix(prefix)) {
		NET_DBG("Prefix %s/64 is not to be used",
			net_sprint_ipv6_addr(prefix));
		goto out;
	}

	/* If the prefix is already added and it is still valid and is not
	 * deprecated, then we do not try to add it again.
	 */
	if (ipv6_pe_prefix_already_exists(ipv6, prefix)) {
		if (vlifetime == 0) {
			i = ipv6_pe_prefix_remove(iface, ipv6, prefix);

			NET_DBG("Removed %d addresses using prefix %s/64",
				i, net_sprint_ipv6_addr(prefix));
		} else {
			ipv6_pe_prefix_update_lifetimes(ipv6, prefix, vlifetime);
		}

		goto out;
	}

	preferred_lifetime = MIN(preferred_lifetime,
				 TEMP_PREFERRED_LIFETIME -
				 DESYNC_FACTOR(ipv6));
	if (preferred_lifetime == 0 ||
	    preferred_lifetime <= REGEN_ADVANCE(ipv6->retrans_timer, 1U)) {
		NET_DBG("Too short preferred lifetime (%u <= %u), temp address not "
			"created for prefix %s/64", preferred_lifetime,
			REGEN_ADVANCE(ipv6->retrans_timer, 1U),
			net_sprint_ipv6_addr(prefix));
		goto out;
	}

	NET_DBG("Starting PE process for prefix %s/64",
		net_sprint_ipv6_addr(prefix));

	net_ipaddr_copy(&addr, prefix);

	do {
		ret = gen_temporary_iid(iface, prefix,
					COND_CODE_1(CONFIG_NET_INTERFACE_NAME,
						    (iface->config.name,
						     sizeof(iface->config.name)),
						    (net_if_get_device(iface)->name,
						     strlen(net_if_get_device(iface)->name))),
					dad_count,
					&addr.s6_addr[8], 8U);
		if (ret == 0) {
			ifaddr = net_if_ipv6_addr_lookup(&addr, NULL);
			if (ifaddr == NULL && !net_ipv6_is_addr_unspecified(&addr) &&
			    memcmp(&addr, &reserved_anycast_subnet,
				   sizeof(struct in6_addr)) != 0) {
				valid = true;
				break;
			}
		}

	} while (dad_count++ < TEMP_IDGEN_RETRIES);

	if (valid == false) {
		NET_WARN("Could not create a valid iid for prefix %s/64 for interface %d",
			 net_sprint_ipv6_addr(prefix),
			 net_if_get_by_iface(iface));
		NET_WARN("Disabling IPv6 PE for interface %d",
			 net_if_get_by_iface(iface));
		net_mgmt_event_notify(NET_EVENT_IPV6_PE_DISABLED, iface);
		iface->pe_enabled = false;
		goto out;
	}

	vlifetime = MIN(TEMP_VALID_LIFETIME, vlifetime);

	ifaddr = net_if_ipv6_addr_add(iface, &addr, NET_ADDR_AUTOCONF, vlifetime);
	if (!ifaddr) {
		NET_ERR("Cannot add %s address to interface %d",
			net_sprint_ipv6_addr(&addr),
			net_if_get_by_iface(iface));
		goto out;
	}

	lifetime = TEMP_VALID_LIFETIME -
		REGEN_ADVANCE(net_if_ipv6_get_retrans_timer(iface), 1U);

	DESYNC_FACTOR(ipv6) = sys_rand32_get() % MAX_DESYNC_FACTOR;

	/* Make sure that the address timeout happens at least two seconds
	 * after the deprecation.
	 */
	DESYNC_FACTOR(ipv6) = MIN(DESYNC_FACTOR(ipv6) + 2U, lifetime);

	ifaddr->is_temporary = true;
	ifaddr->addr_preferred_lifetime = preferred_lifetime;
	ifaddr->addr_timeout = ifaddr->addr_preferred_lifetime - DESYNC_FACTOR(ipv6);
	ifaddr->addr_create_time = k_uptime_seconds();

	NET_DBG("Lifetime %d desync %d timeout %d preferred %d valid %d",
		lifetime, DESYNC_FACTOR(ipv6), ifaddr->addr_timeout,
		ifaddr->addr_preferred_lifetime, vlifetime);

	NET_DBG("Starting DAD for %s iface %d", net_sprint_ipv6_addr(&addr),
		net_if_get_by_iface(iface));

	net_if_ipv6_start_dad(iface, ifaddr);

	vlifetimeout = K_SECONDS(ifaddr->addr_timeout);

	remaining = k_work_delayable_remaining_get(&temp_lifetime);
	if (remaining == 0 || remaining > vlifetimeout.ticks) {
		NET_DBG("Next check for temp addresses in %d seconds",
			ifaddr->addr_timeout);
		k_work_schedule(&temp_lifetime, vlifetimeout);
	}

out:
	net_if_unlock(iface);
}

#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0

static void iface_cb(struct net_if *iface, void *user_data)
{
	bool is_new_filter_denylist = !ipv6_pe_denylist;
	struct in6_addr *prefix = user_data;
	struct net_if_ipv6 *ipv6;
	int ret;

	net_if_lock(iface);

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		goto out;
	}

	if (!ipv6) {
		goto out;
	}

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    !ipv6->unicast[i].is_temporary) {
			continue;
		}

		ret = net_ipv6_is_prefix(
				(uint8_t *)&ipv6->unicast[i].address.in6_addr,
				(uint8_t *)prefix, 64);

		/* TODO: Do this removal gracefully so that applications
		 * have time to cope with this change.
		 */
		if (is_new_filter_denylist) {
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

out:
	net_if_unlock(iface);
}

/* If we change filter value, then check if existing IPv6 prefixes will
 * conflict with the new filter.
 */
static void ipv6_pe_recheck_filters(bool is_denylist)
{
	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(ipv6_pe_filter, i) {
		if (net_ipv6_is_addr_unspecified(&ipv6_pe_filter[i])) {
			continue;
		}

		net_if_foreach(iface_cb, &ipv6_pe_filter[i]);
	}

	k_mutex_unlock(&lock);
}
#endif /* CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0 */

#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
static void send_filter_event(struct in6_addr *addr, bool is_denylist,
			      int event_type)
{
	if (IS_ENABLED(CONFIG_NET_MGMT_EVENT_INFO)) {
		struct net_event_ipv6_pe_filter info;

		net_ipaddr_copy(&info.prefix, addr);
		info.is_deny_list = is_denylist;

		net_mgmt_event_notify_with_info(event_type,
						NULL,
						(const void *)&info,
						sizeof(info));
	} else {
		net_mgmt_event_notify(event_type, NULL);
	}
}
#endif

int net_ipv6_pe_add_filter(struct in6_addr *addr, bool is_denylist)
{
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	bool found = false;
	int free_slot = -1;
	int ret = 0;

	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(ipv6_pe_filter, i) {
		if (free_slot < 0 &&
		    net_ipv6_is_addr_unspecified(&ipv6_pe_filter[i])) {
			free_slot = i;
			continue;
		}

		if (net_ipv6_is_prefix((uint8_t *)addr,
				       (uint8_t *)&ipv6_pe_filter[i],
				       64)) {
			found = true;
			break;
		}
	}

	if (found) {
		NET_DBG("Filter %s already in the list",
			net_sprint_ipv6_addr(addr));
		ret = -EALREADY;
		goto out;
	}

	if (free_slot < 0) {
		NET_DBG("All filters in use");
		ret = -ENOMEM;
		goto out;
	}

	net_ipaddr_copy(&ipv6_pe_filter[free_slot], addr);

	if (ipv6_pe_denylist != is_denylist) {
		ipv6_pe_recheck_filters(is_denylist);
	}

	ipv6_pe_denylist = is_denylist ? true : false;

	NET_DBG("Adding %s list filter %s",
		ipv6_pe_denylist ? "deny" : "allow",
		net_sprint_ipv6_addr(&ipv6_pe_filter[free_slot]));

	send_filter_event(&ipv6_pe_filter[free_slot],
			  is_denylist,
			  NET_EVENT_IPV6_PE_FILTER_ADD);
out:
	k_mutex_unlock(&lock);

	return ret;
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(is_denylist);

	return -ENOTSUP;
#endif
}

int net_ipv6_pe_del_filter(struct in6_addr *addr)
{
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	int ret = -ENOENT;

	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(ipv6_pe_filter, i) {
		if (net_ipv6_addr_cmp(&ipv6_pe_filter[i], addr)) {
			NET_DBG("Removing %s list filter %s",
				ipv6_pe_denylist ? "deny" : "allow",
				net_sprint_ipv6_addr(&ipv6_pe_filter[i]));

			send_filter_event(&ipv6_pe_filter[i],
					  ipv6_pe_denylist,
					  NET_EVENT_IPV6_PE_FILTER_DEL);

			net_ipaddr_copy(&ipv6_pe_filter[i],
					net_ipv6_unspecified_address());

			ret = 0;
			goto out;
		}
	}

out:
	k_mutex_unlock(&lock);

	return ret;
#else
	ARG_UNUSED(addr);

	return -ENOTSUP;
#endif
}

bool net_ipv6_pe_check_dad(int count)
{
	return count <= TEMP_IDGEN_RETRIES;
}

int net_ipv6_pe_filter_foreach(net_ipv6_pe_filter_cb_t cb, void *user_data)
{
#if CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT > 0
	int i, count = 0;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < CONFIG_NET_IPV6_PE_FILTER_PREFIX_COUNT; i++) {
		if (net_ipv6_is_addr_unspecified(&ipv6_pe_filter[i])) {
			continue;
		}

		cb(&ipv6_pe_filter[i], ipv6_pe_denylist, user_data);

		count++;
	}

	k_mutex_unlock(&lock);

	return count;
#else
	ARG_UNUSED(cb);
	ARG_UNUSED(user_data);

	return 0;
#endif
}

struct deprecated_work {
	struct k_work_delayable work;
	struct net_if *iface;
	struct in6_addr addr;
};

static struct deprecated_work trigger_deprecated_event;

static void send_deprecated_event(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct deprecated_work *dw = CONTAINER_OF(dwork,
						  struct deprecated_work,
						  work);

	net_mgmt_event_notify_with_info(NET_EVENT_IPV6_ADDR_DEPRECATED,
					dw->iface, &dw->addr,
					sizeof(struct in6_addr));
}

static void renewal_cb(struct net_if *iface, void *user_data)
{
	struct net_if_ipv6 *ipv6;
	struct in6_addr prefix;

	if (net_if_config_ipv6_get(iface, &ipv6) < 0) {
		return;
	}

	if (!ipv6) {
		return;
	}

	ARRAY_FOR_EACH(ipv6->unicast, i) {
		int32_t diff;

		if (!ipv6->unicast[i].is_used ||
		    ipv6->unicast[i].address.family != AF_INET6 ||
		    !ipv6->unicast[i].is_temporary ||
		    ipv6->unicast[i].addr_state == NET_ADDR_DEPRECATED) {
			continue;
		}

		/* If the address is too old, then generate a new one
		 * and remove the old address.
		 */
		diff = (int32_t)(ipv6->unicast[i].addr_create_time - k_uptime_seconds());
		diff = abs(diff);

		if (diff < (ipv6->unicast[i].addr_preferred_lifetime -
			    REGEN_ADVANCE(ipv6->retrans_timer, 1U) -
			    DESYNC_FACTOR(ipv6))) {
			continue;
		}

		net_ipaddr_copy(&prefix, &ipv6->unicast[i].address.in6_addr);
		memset(prefix.s6_addr + 8, 0, sizeof(prefix) - 8);

		NET_DBG("IPv6 address %s is deprecated",
			net_sprint_ipv6_addr(&ipv6->unicast[i].address.in6_addr));

		ipv6->unicast[i].addr_state = NET_ADDR_DEPRECATED;

		/* Create a new temporary address and then notify users
		 * that the old address is deprecated so that they can
		 * re-connect to use the new address. We cannot send deprecated
		 * event immediately because the new IPv6 will need to do DAD
		 * and that takes a bit time.
		 */
		net_ipv6_pe_start(iface, &prefix,
				  TEMP_VALID_LIFETIME,
				  TEMP_PREFERRED_LIFETIME);

		/* It is very unlikely but still a small possibility that there
		 * could be another work already pending.
		 * Fixing this would require allocating space for the work
		 * somewhere. Currently this does not look like worth fixing.
		 * Print a warning if there is a work already pending.
		 */
		if (k_work_delayable_is_pending(&trigger_deprecated_event.work)) {
			NET_WARN("Work already pending for deprecated event sending.");
		}

		trigger_deprecated_event.iface = iface;
		memcpy(&trigger_deprecated_event.addr,
		       &ipv6->unicast[i].address.in6_addr,
		       sizeof(struct in6_addr));

		/* 500ms should be enough for DAD to pass */
		k_work_schedule(&trigger_deprecated_event.work, K_MSEC(500));
	}
}

static void ipv6_pe_renew(struct k_work *work)
{
	ARG_UNUSED(work);

	net_if_foreach(renewal_cb, NULL);
}

int net_ipv6_pe_init(struct net_if *iface)
{
	int32_t lifetime;
	int ret = 0;

	net_mgmt_event_notify(NET_EVENT_IPV6_PE_ENABLED, iface);

	lifetime = TEMP_VALID_LIFETIME -
		REGEN_ADVANCE(net_if_ipv6_get_retrans_timer(iface), 1U);

	if (lifetime <= 0) {
		iface->pe_enabled = false;
		net_mgmt_event_notify(NET_EVENT_IPV6_PE_DISABLED, iface);
		ret = -EINVAL;
		goto out;
	}

	iface->pe_enabled = true;
	iface->pe_prefer_public =
		IS_ENABLED(CONFIG_NET_IPV6_PE_PREFER_PUBLIC_ADDRESSES) ?
		true : false;

	k_work_init_delayable(&temp_lifetime, ipv6_pe_renew);
	k_work_init_delayable(&trigger_deprecated_event.work,
			      send_deprecated_event);

out:
	NET_DBG("pe %s prefer %s lifetime %d sec",
		iface->pe_enabled ? "enabled" : "disabled",
		iface->pe_prefer_public ? "public" : "temporary",
		lifetime);

	return ret;
}
