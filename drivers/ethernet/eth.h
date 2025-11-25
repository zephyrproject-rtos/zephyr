/*
 * Copyright (c) 2020 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_H_

#include <zephyr/types.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/random/random.h>

/* helper macro to return mac address octet from local_mac_address prop */
#define NODE_MAC_ADDR_OCTET(node, n) DT_PROP_BY_IDX(node, local_mac_address, n)

/* Determine if a mac address is all 0's */
#define NODE_MAC_ADDR_NULL(node) \
	((NODE_MAC_ADDR_OCTET(node, 0) == 0) && \
	 (NODE_MAC_ADDR_OCTET(node, 1) == 0) && \
	 (NODE_MAC_ADDR_OCTET(node, 2) == 0) && \
	 (NODE_MAC_ADDR_OCTET(node, 3) == 0) && \
	 (NODE_MAC_ADDR_OCTET(node, 4) == 0) && \
	 (NODE_MAC_ADDR_OCTET(node, 5) == 0))

/* Given a device tree node for an ethernet controller will
 * returns false if there is no local-mac-address property or
 * the property is all zero's.  Otherwise will return True
 */
#define NODE_HAS_VALID_MAC_ADDR(node) \
	UTIL_AND(DT_NODE_HAS_PROP(node, local_mac_address),\
			(!NODE_MAC_ADDR_NULL(node)))

static inline void gen_random_mac(uint8_t *mac_addr, uint8_t b0, uint8_t b1, uint8_t b2)
{
	mac_addr[0] = b0;
	mac_addr[1] = b1;
	mac_addr[2] = b2;

	/* Set MAC address locally administered, unicast (LAA) */
	mac_addr[0] |= 0x02;

	sys_rand_get(&mac_addr[3], 3U);
}

static inline int net_eth_mac_load(const struct net_eth_mac_config *cfg, uint8_t *mac_addr)
{
	if (cfg == NULL || cfg->type == NET_ETH_MAC_DEFAULT) {
		return -ENODATA;
	}

	/* Copy the static part */
	memcpy(mac_addr, cfg->addr, cfg->addr_len);

	if (cfg->type == NET_ETH_MAC_RANDOM) {
		sys_rand_get(&mac_addr[cfg->addr_len], NET_ETH_ADDR_LEN - cfg->addr_len);

		/* Clear group bit, multicast (I/G) */
		mac_addr[0] &= ~0x01;
		/* Set MAC address locally administered, unicast (LAA) */
		mac_addr[0] |= 0x02;

		return 0;
	}

#if defined(CONFIG_NVMEM)
	if (cfg->type == NET_ETH_MAC_NVMEM) {
		return nvmem_cell_read(&cfg->cell, &mac_addr[cfg->addr_len], 0,
				       NET_ETH_ADDR_LEN - cfg->addr_len);
	}
#endif

	if (cfg->type == NET_ETH_MAC_STATIC) {
		return 0;
	}

	return -ENODATA;
}

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_H_ */
