/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_DWC_MAC_ETH_STM32_DWC_H_
#define ZEPHYR_DRIVERS_ETHERNET_DWC_MAC_ETH_STM32_DWC_H_

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/net/ethernet.h>
#include <zephyr/sys/crc.h>

#define ST_OUI_B0 0x00
#define ST_OUI_B1 0x80
#define ST_OUI_B2 0xE1

static inline int eth_stm32_net_eth_mac_load(const struct net_eth_mac_config *cfg,
					     uint8_t *mac_addr)
{
	int ret;

	ret = net_eth_mac_load(cfg, mac_addr);
	if (ret == -ENODATA) {
		uint8_t unique_device_ID_12_bytes[12];
		uint32_t result_mac_32_bits;

		/**
		 * Set MAC address locally administered bit (LAA) as this is not assigned by the
		 * manufacturer
		 */
		mac_addr[0] = ST_OUI_B0 | 0x02;
		mac_addr[1] = ST_OUI_B1;
		mac_addr[2] = ST_OUI_B2;

		/* Nothing defined by the user, use device id */
		hwinfo_get_device_id(unique_device_ID_12_bytes, 12);
		result_mac_32_bits = crc32_ieee((uint8_t *)unique_device_ID_12_bytes, 12);
		memcpy(&mac_addr[3], &result_mac_32_bits, 3);

		return 0;
	}

	if (ret < 0) {
		LOG_ERR("Failed to load MAC address (%d)", ret);
	}

	return ret;
}

#endif /* ZEPHYR_DRIVERS_ETHERNET_DWC_MAC_ETH_STM32_DWC_H_ */
