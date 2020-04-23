/*
 * Copyright (c) 2020 Linaro Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_ETHERNET_ETH_H_
#define ZEPHYR_DRIVERS_ETHERNET_ETH_H_

#include <zephyr/types.h>

static inline void gen_random_mac(u8_t *mac_addr, u8_t b0, u8_t b1, u8_t b2)
{
	u32_t entropy;

	entropy = sys_rand32_get();

	mac_addr[0] = b0;
	mac_addr[1] = b1;
	mac_addr[2] = b2;

	/* Set MAC address locally administered, unicast (LAA) */
	mac_addr[0] |= 0x02;

	mac_addr[3] = (entropy >> 16) & 0xff;
	mac_addr[4] = (entropy >>  8) & 0xff;
	mac_addr[5] = (entropy >>  0) & 0xff;
}

#endif /* ZEPHYR_DRIVERS_ETHERNET_ETH_H_ */
