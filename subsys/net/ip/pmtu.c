/** @file
 * @brief IPv4/6 PMTU related functions
 */

/*
 * Copyright (c) 2024 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_pmtu, CONFIG_NET_PMTU_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_mgmt.h>
#include <zephyr/net/net_if.h>
#include "pmtu.h"

#if defined(CONFIG_NET_IPV4_PMTU)
#define NET_IPV4_PMTU_ENTRIES CONFIG_NET_IPV4_PMTU_DESTINATION_CACHE_ENTRIES
#else
#define NET_IPV4_PMTU_ENTRIES 0
#endif

#if defined(CONFIG_NET_IPV6_PMTU)
#define NET_IPV6_PMTU_ENTRIES CONFIG_NET_IPV6_PMTU_DESTINATION_CACHE_ENTRIES
#else
#define NET_IPV6_PMTU_ENTRIES 0
#endif

#define NET_PMTU_MAX_ENTRIES (NET_IPV4_PMTU_ENTRIES + NET_IPV6_PMTU_ENTRIES)

static struct net_pmtu_entry pmtu_entries[NET_PMTU_MAX_ENTRIES];

static K_MUTEX_DEFINE(lock);

static struct net_pmtu_entry *get_pmtu_entry(const struct sockaddr *dst)
{
	struct net_pmtu_entry *entry = NULL;
	int i;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(pmtu_entries); i++) {
		switch (dst->sa_family) {
		case AF_INET:
			if (IS_ENABLED(CONFIG_NET_IPV4_PMTU) &&
			    pmtu_entries[i].dst.family == AF_INET &&
			    net_ipv4_addr_cmp(&pmtu_entries[i].dst.in_addr,
					      &net_sin(dst)->sin_addr)) {
				entry = &pmtu_entries[i];
				goto out;
			}
			break;

		case AF_INET6:
			if (IS_ENABLED(CONFIG_NET_IPV6_PMTU) &&
			    pmtu_entries[i].dst.family == AF_INET6 &&
			    net_ipv6_addr_cmp(&pmtu_entries[i].dst.in6_addr,
					      &net_sin6(dst)->sin6_addr)) {
				entry = &pmtu_entries[i];
				goto out;
			}
			break;

		default:
			break;
		}
	}

out:
	k_mutex_unlock(&lock);

	return entry;
}

static struct net_pmtu_entry *get_free_pmtu_entry(void)
{
	struct net_pmtu_entry *entry = NULL;
	uint32_t oldest = 0U;
	int i;

	k_mutex_lock(&lock, K_FOREVER);

	for (i = 0; i < ARRAY_SIZE(pmtu_entries); i++) {
		if (!pmtu_entries[i].in_use) {
			pmtu_entries[i].in_use = true;
			pmtu_entries[i].last_update = k_uptime_get_32();

			entry = &pmtu_entries[i];
			goto out;
		}

		if (oldest == 0U || pmtu_entries[i].last_update < oldest) {
			oldest = pmtu_entries[i].last_update;
			entry = &pmtu_entries[i];
		}
	}

out:
	k_mutex_unlock(&lock);

	return entry;
}

static void update_pmtu_entry(struct net_pmtu_entry *entry, uint16_t mtu)
{
	bool changed = false;

	if (entry->mtu != mtu) {
		changed = true;
		entry->mtu = mtu;
	}

	entry->last_update = k_uptime_get_32();

	if (changed) {
		struct net_if *iface;

		if (IS_ENABLED(CONFIG_NET_IPV4_PMTU) && entry->dst.family == AF_INET) {
			struct net_event_ipv4_pmtu_info info;

			net_ipaddr_copy(&info.dst, &entry->dst.in_addr);
			info.mtu = mtu;

			iface = net_if_ipv4_select_src_iface(&info.dst);

			net_mgmt_event_notify_with_info(NET_EVENT_IPV4_PMTU_CHANGED,
							iface,
							(const void *)&info,
							sizeof(struct net_event_ipv4_pmtu_info));

		} else if (IS_ENABLED(CONFIG_NET_IPV6_PMTU) && entry->dst.family == AF_INET6) {
			struct net_event_ipv6_pmtu_info info;

			net_ipaddr_copy(&info.dst, &entry->dst.in6_addr);
			info.mtu = mtu;

			iface = net_if_ipv6_select_src_iface(&info.dst);

			net_mgmt_event_notify_with_info(NET_EVENT_IPV6_PMTU_CHANGED,
							iface,
							(const void *)&info,
							sizeof(struct net_event_ipv6_pmtu_info));
		}
	}
}

struct net_pmtu_entry *net_pmtu_get_entry(const struct sockaddr *dst)
{
	struct net_pmtu_entry *entry;

	entry = get_pmtu_entry(dst);

	return entry;
}

int net_pmtu_get_mtu(const struct sockaddr *dst)
{
	struct net_pmtu_entry *entry;

	entry = get_pmtu_entry(dst);
	if (entry == NULL) {
		return -ENOENT;
	}

	return entry->mtu;
}

static struct net_pmtu_entry *add_entry(const struct sockaddr *dst, bool *old_entry)
{
	struct net_pmtu_entry *entry;

	entry = get_pmtu_entry(dst);
	if (entry != NULL) {
		*old_entry = true;
		return entry;
	}

	entry = get_free_pmtu_entry();
	if (entry == NULL) {
		return NULL;
	}

	k_mutex_lock(&lock, K_FOREVER);

	switch (dst->sa_family) {
	case AF_INET:
		if (IS_ENABLED(CONFIG_NET_IPV4_PMTU)) {
			entry->dst.family = AF_INET;
			net_ipaddr_copy(&entry->dst.in_addr, &net_sin(dst)->sin_addr);
		} else {
			entry->in_use = false;
			goto unlock_fail;
		}
		break;

	case AF_INET6:
		if (IS_ENABLED(CONFIG_NET_IPV6_PMTU)) {
			entry->dst.family = AF_INET6;
			net_ipaddr_copy(&entry->dst.in6_addr, &net_sin6(dst)->sin6_addr);
		} else {
			entry->in_use = false;
			goto unlock_fail;
		}
		break;

	default:
		entry->in_use = false;
		goto unlock_fail;
	}

	k_mutex_unlock(&lock);
	return entry;

unlock_fail:
	*old_entry = false;

	k_mutex_unlock(&lock);
	return NULL;
}

int net_pmtu_update_mtu(const struct sockaddr *dst, uint16_t mtu)
{
	struct net_pmtu_entry *entry;
	uint16_t old_mtu = 0U;
	bool updated = false;

	entry = add_entry(dst, &updated);
	if (entry == NULL) {
		return -ENOMEM;
	}

	if (updated) {
		old_mtu = entry->mtu;
	}

	update_pmtu_entry(entry, mtu);

	return (int)old_mtu;
}

int net_pmtu_update_entry(struct net_pmtu_entry *entry, uint16_t mtu)
{
	uint16_t old_mtu;

	if (entry == NULL) {
		return -EINVAL;
	}

	if (entry->mtu == mtu) {
		return -EALREADY;
	}

	old_mtu = entry->mtu;

	update_pmtu_entry(entry, mtu);

	return (int)old_mtu;
}

int net_pmtu_foreach(net_pmtu_cb_t cb, void *user_data)
{
	int ret = 0;

	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(pmtu_entries, i) {
		ret++;
		cb(&pmtu_entries[i], user_data);
	}

	k_mutex_unlock(&lock);

	return ret;
}

void net_pmtu_init(void)
{
	k_mutex_lock(&lock, K_FOREVER);

	ARRAY_FOR_EACH(pmtu_entries, i) {
		pmtu_entries[i].in_use = false;
	}

	k_mutex_unlock(&lock);
}
