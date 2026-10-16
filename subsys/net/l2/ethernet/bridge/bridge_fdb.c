/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_eth_bridge_fdb, CONFIG_NET_ETHERNET_BRIDGE_LOG_LEVEL);

#include <zephyr/net/net_log.h>
#include <zephyr/net/ethernet_bridge.h>
#include <zephyr/net/ethernet_bridge_fdb.h>

#include "bridge_internal.h"

/* FDB table */
static sys_slist_t fdb_entries = SYS_SLIST_STATIC_INIT(&fdb_entries);
static K_MUTEX_DEFINE(fdb_lock);
static size_t fdb_count;

/* Memory pool for FDB entries */
K_MEM_SLAB_DEFINE_STATIC_TYPE(fdb_slab, struct eth_bridge_fdb_entry,
			      CONFIG_NET_ETHERNET_BRIDGE_FDB_MAX_ENTRIES);

/* Remove the learned entry that was learned first, the last in the list.
 * Called with fdb_lock held.
 */
static bool fdb_remove_oldest_learned(void)
{
	struct eth_bridge_fdb_entry *entry, *oldest = NULL;
	sys_snode_t *prev = NULL, *oldest_prev = NULL;

	SYS_SLIST_FOR_EACH_CONTAINER(&fdb_entries, entry, node) {
		if ((entry->flags & ETHERNET_BRIDGE_FDB_FLAG_DYNAMIC) != 0U) {
			oldest = entry;
			oldest_prev = prev;
		}

		prev = &entry->node;
	}

	if (oldest == NULL) {
		return false;
	}

	NET_DBG("FDB entry replaced: %02x:%02x:%02x:%02x:%02x:%02x -> iface %d",
		oldest->mac.addr[0], oldest->mac.addr[1], oldest->mac.addr[2],
		oldest->mac.addr[3], oldest->mac.addr[4], oldest->mac.addr[5],
		net_if_get_by_iface(oldest->iface));

	sys_slist_remove(&fdb_entries, oldest_prev, &oldest->node);
	fdb_count--;
	k_mem_slab_free(&fdb_slab, (void *)oldest);

	return true;
}

int eth_bridge_fdb_add(struct net_eth_addr *mac, struct net_if *iface)
{
	struct eth_bridge_fdb_entry *entry;
	struct net_if *bridge;
	int ret = 0;

	/* Check mac and iface is valid or not */
	if (mac == NULL || iface == NULL) {
		return -EINVAL;
	}

	if (!net_eth_iface_is_bridged(net_if_l2_data(iface))) {
		return -EINVAL;
	}

	bridge = net_eth_get_bridge(net_if_l2_data(iface));

	k_mutex_lock(&fdb_lock, K_FOREVER);

	/* Check if the mac already exists in the FDB table of this bridge */
	SYS_SLIST_FOR_EACH_CONTAINER(&fdb_entries, entry, node) {
		if (memcmp(&entry->mac, mac, sizeof(struct net_eth_addr)) == 0 &&
		    net_eth_get_bridge(net_if_l2_data(entry->iface)) == bridge) {

			if (entry->iface == iface) {
				/* A learned entry becomes static */
				entry->flags = ETHERNET_BRIDGE_FDB_FLAG_STATIC;
				NET_DBG("FDB entry exists");
				goto out;
			}

			/* For unicast address, just update it */
			if (net_eth_is_addr_valid(mac)) {
				entry->iface = iface;
				entry->flags = ETHERNET_BRIDGE_FDB_FLAG_STATIC;
				NET_DBG("FDB entry updated: %02x:%02x:%02x:%02x:%02x:%02x -> iface "
					"%d",
					mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3],
					mac->addr[4], mac->addr[5], net_if_get_by_iface(iface));
				goto out;
			}
		}
	}

	/* Learned entries share the table: a full one gives up its oldest
	 * learned entry, so that received frames cannot fill it against
	 * configuration.
	 */
	if (fdb_count >= CONFIG_NET_ETHERNET_BRIDGE_FDB_MAX_ENTRIES &&
	    !fdb_remove_oldest_learned()) {
		NET_ERR("FDB table full");
		ret = -ENOMEM;
		goto out;
	}

	/* Allocate new entry */
	if (k_mem_slab_alloc(&fdb_slab, (void **)&entry, K_NO_WAIT) != 0) {
		NET_ERR("Failed to allocate FDB entry");
		ret = -ENOMEM;
		goto out;
	}

	/* Initialize entry */
	memcpy(&entry->mac, mac, sizeof(struct net_eth_addr));
	entry->iface = iface;
	entry->flags = ETHERNET_BRIDGE_FDB_FLAG_STATIC;

	/* Add to list */
	sys_slist_prepend(&fdb_entries, &entry->node);
	fdb_count++;

	NET_DBG("FDB entry added: %02x:%02x:%02x:%02x:%02x:%02x -> iface %d", mac->addr[0],
		mac->addr[1], mac->addr[2], mac->addr[3], mac->addr[4], mac->addr[5],
		net_if_get_by_iface(iface));
out:
	k_mutex_unlock(&fdb_lock);
	return ret;
}

int eth_bridge_fdb_del(struct net_eth_addr *mac, struct net_if *iface)
{
	struct eth_bridge_fdb_entry *entry;
	sys_snode_t *node, *next, *prev = NULL;
	int ret = -ENOENT;

	/* A NULL iface deletes the address from every interface */
	if (mac == NULL) {
		return -EINVAL;
	}

	if (iface != NULL && net_eth_get_bridge(net_if_l2_data(iface)) == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&fdb_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&fdb_entries, node, next) {
		entry = CONTAINER_OF(node, struct eth_bridge_fdb_entry, node);
		if (memcmp(&entry->mac, mac, sizeof(struct net_eth_addr)) != 0 ||
		    (iface != NULL && entry->iface != iface)) {
			prev = node;
			continue;
		}

		NET_DBG("FDB entry deleted: %02x:%02x:%02x:%02x:%02x:%02x -> iface %d",
			mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3],
			mac->addr[4], mac->addr[5], net_if_get_by_iface(entry->iface));

		/* Remove from list, prev stays the last kept node */
		sys_slist_remove(&fdb_entries, prev, node);
		fdb_count--;

		/* Free memory */
		k_mem_slab_free(&fdb_slab, (void *)entry);

		ret = 0;

		/* eth_bridge_fdb_add() keeps at most one entry per address and interface */
		if (iface != NULL) {
			break;
		}
	}

	k_mutex_unlock(&fdb_lock);
	return ret;
}

int eth_bridge_fdb_del_iface(struct net_if *iface)
{
	struct eth_bridge_fdb_entry *entry;
	sys_snode_t *node, *next, *prev = NULL;

	/* A NULL iface deletes the entries of every interface */
	if (iface != NULL && net_eth_get_bridge(net_if_l2_data(iface)) == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&fdb_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_NODE_SAFE(&fdb_entries, node, next) {
		entry = CONTAINER_OF(node, struct eth_bridge_fdb_entry, node);
		if (iface != NULL && entry->iface != iface) {
			prev = node;
			continue;
		}

		NET_DBG("FDB entry deleted: %02x:%02x:%02x:%02x:%02x:%02x -> iface %d",
			entry->mac.addr[0], entry->mac.addr[1], entry->mac.addr[2],
			entry->mac.addr[3], entry->mac.addr[4], entry->mac.addr[5],
			net_if_get_by_iface(entry->iface));

		/* Remove from list, prev stays the last kept node */
		sys_slist_remove(&fdb_entries, prev, node);
		fdb_count--;

		/* Free memory */
		k_mem_slab_free(&fdb_slab, (void *)entry);
	}

	k_mutex_unlock(&fdb_lock);
	return 0;
}

void eth_bridge_fdb_foreach(eth_bridge_fdb_entry_cb_t cb, void *user_data)
{
	struct eth_bridge_fdb_entry *entry, *tmp;

	k_mutex_lock(&fdb_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&fdb_entries, entry, tmp, node) {
		cb(entry, user_data);
	}

	k_mutex_unlock(&fdb_lock);
}

#if defined(CONFIG_NET_ETHERNET_BRIDGE_FDB_LEARNING)
void eth_bridge_fdb_learn(struct net_if *bridge, struct net_eth_addr *mac, struct net_if *iface)
{
	struct eth_bridge_fdb_entry *entry;

	/* Only a unicast source identifies a station, and a frame with the
	 * bridge's own address as source is not from a station behind a port.
	 * Learning that would send the frames for the bridge to that port.
	 */
	if (!net_eth_is_addr_valid(mac) ||
	    memcmp(mac, net_if_get_link_addr(bridge)->addr, sizeof(*mac)) == 0) {
		return;
	}

	k_mutex_lock(&fdb_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&fdb_entries, entry, node) {
		if (memcmp(&entry->mac, mac, sizeof(struct net_eth_addr)) != 0 ||
		    net_eth_get_bridge(net_if_l2_data(entry->iface)) != bridge) {
			continue;
		}

		/* A static entry is never changed by what is received */
		if ((entry->flags & ETHERNET_BRIDGE_FDB_FLAG_DYNAMIC) != 0U &&
		    entry->iface != iface) {
			NET_DBG("FDB entry moved: %02x:%02x:%02x:%02x:%02x:%02x -> iface %d",
				mac->addr[0], mac->addr[1], mac->addr[2], mac->addr[3],
				mac->addr[4], mac->addr[5], net_if_get_by_iface(iface));
			entry->iface = iface;
		}

		goto out;
	}

	/* A full table learns nothing; such destinations are flooded */
	if (fdb_count >= CONFIG_NET_ETHERNET_BRIDGE_FDB_MAX_ENTRIES ||
	    k_mem_slab_alloc(&fdb_slab, (void **)&entry, K_NO_WAIT) != 0) {
		goto out;
	}

	memcpy(&entry->mac, mac, sizeof(struct net_eth_addr));
	entry->iface = iface;
	entry->flags = ETHERNET_BRIDGE_FDB_FLAG_DYNAMIC;

	sys_slist_prepend(&fdb_entries, &entry->node);
	fdb_count++;

	NET_DBG("FDB entry learned: %02x:%02x:%02x:%02x:%02x:%02x -> iface %d", mac->addr[0],
		mac->addr[1], mac->addr[2], mac->addr[3], mac->addr[4], mac->addr[5],
		net_if_get_by_iface(iface));
out:
	k_mutex_unlock(&fdb_lock);
}
#endif /* CONFIG_NET_ETHERNET_BRIDGE_FDB_LEARNING */

size_t eth_bridge_fdb_lookup(struct net_if *bridge, const struct net_eth_addr *mac,
			     struct net_if *ports[], size_t max_ports)
{
	struct eth_bridge_fdb_entry *entry;
	size_t count = 0;

	k_mutex_lock(&fdb_lock, K_FOREVER);

	SYS_SLIST_FOR_EACH_CONTAINER(&fdb_entries, entry, node) {
		if (memcmp(&entry->mac, mac, sizeof(struct net_eth_addr)) != 0 ||
		    net_eth_get_bridge(net_if_l2_data(entry->iface)) != bridge) {
			continue;
		}

		if (count < max_ports) {
			ports[count] = entry->iface;
		}

		count++;
	}

	k_mutex_unlock(&fdb_lock);

	return count;
}
