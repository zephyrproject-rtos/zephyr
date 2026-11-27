/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(dwmac_core, CONFIG_ETHERNET_LOG_LEVEL);

#include <zephyr/kernel.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include "eth_dwmac_priv.h"

#ifdef CONFIG_ETH_DWC_ETHER_1000_CORE
#define DWMAC_MCAST_ADDR_HIGH(n)	DWMAC_MACAHR(n)
#define DWMAC_MCAST_ADDR_LOW(n)		DWMAC_MACALR(n)
#define DWMAC_MCAST_ADDR_HIGH_AE	DWMAC_MACAHR_AE
#define DWMAC_PASS_ALL_MCAST_REG	DWMAC_MACFFR
#define DWMAC_PASS_ALL_MCAST_BIT	DWMAC_MACFFR_PAM

BUILD_ASSERT(DWMAC_PERFECT_FILTER_ENTRIES <= 16,
	     "this driver only supports the first 16 MAC address entries");
#endif
#ifdef CONFIG_ETH_DWC_ETHER_QOS_CORE
#define DWMAC_MCAST_ADDR_HIGH(n)	MAC_ADDRESS_HIGH(n)
#define DWMAC_MCAST_ADDR_LOW(n)		MAC_ADDRESS_LOW(n)
#define DWMAC_MCAST_ADDR_HIGH_AE	MAC_ADDRESS_HIGH_AE
#define DWMAC_PASS_ALL_MCAST_REG	MAC_PKT_FILTER
#define DWMAC_PASS_ALL_MCAST_BIT	MAC_PKT_FILTER_PM

BUILD_ASSERT(DWMAC_PERFECT_FILTER_ENTRIES <= 128,
	     "this core supports at most 128 MAC address entries");
#endif

static void dwmac_mcast_perfect_cb(struct net_if *iface,
				   const struct net_eth_mcast_addr *mcast_addr, void *user_data)
{
	const struct device *dev = net_if_get_device(iface);
	unsigned int *count = user_data;

	(*count)++;
	if (*count > DWMAC_MULTICAST_PERFECT_SLOTS) {
		return;
	}

	/* MAC address entry 0 holds the station address, so start at entry 1 */
	DWMAC_REG_WRITE(DWMAC_MCAST_ADDR_HIGH(*count),
			sys_get_le16(&mcast_addr->addr.addr[4]) | DWMAC_MCAST_ADDR_HIGH_AE);
	DWMAC_REG_WRITE(DWMAC_MCAST_ADDR_LOW(*count), sys_get_le32(mcast_addr->addr.addr));
}

void dwmac_setup_multicast_filter(const struct device *dev, const struct ethernet_filter *filter)
{
	struct dwmac_priv *p = dev->data;
	unsigned int count = 0;

	ARG_UNUSED(filter);

	net_eth_mcast_addr_foreach(p->iface, dwmac_mcast_perfect_cb, &count);

	if (NET_ETH_MCAST_FILTER_COUNT > DWMAC_MULTICAST_PERFECT_SLOTS) {
		uint32_t reg_val = DWMAC_REG_READ(DWMAC_PASS_ALL_MCAST_REG);

		if (count > DWMAC_MULTICAST_PERFECT_SLOTS) {
			/* the addresses do not fit into the entries, accept all multicast */
			DWMAC_REG_WRITE(DWMAC_PASS_ALL_MCAST_REG,
					reg_val | DWMAC_PASS_ALL_MCAST_BIT);
		} else {
			DWMAC_REG_WRITE(DWMAC_PASS_ALL_MCAST_REG,
					reg_val & ~DWMAC_PASS_ALL_MCAST_BIT);
		}
	}

	/* disable the now unused entries */
	for (unsigned int i = MIN(count, DWMAC_MULTICAST_PERFECT_SLOTS) + 1;
	     i <= DWMAC_MULTICAST_PERFECT_SLOTS; i++) {
		DWMAC_REG_WRITE(DWMAC_MCAST_ADDR_HIGH(i), 0);
	}
}
