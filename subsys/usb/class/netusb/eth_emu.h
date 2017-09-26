/*
 * Ethernet Emulation driver
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define CONFIG_ETH_EMU_0_NAME "EMU_0"

struct eth_emu_context {
	struct net_if *iface;
	u8_t mac_addr[6];
};

int eth_emu_up(void);
int eth_emu_down(void);
