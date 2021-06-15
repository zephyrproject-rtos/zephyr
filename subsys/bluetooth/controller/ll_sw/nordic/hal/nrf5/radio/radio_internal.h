/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SW_SWITCH_PREV_RX 0
#define SW_SWITCH_NEXT_RX 0
#define SW_SWITCH_PREV_TX 1
#define SW_SWITCH_NEXT_TX 1
#define SW_SWITCH_PREV_PHY_1M 0
#define SW_SWITCH_PREV_FLAGS_DONTCARE 0
#define SW_SWITCH_NEXT_FLAGS_DONTCARE 0

void sw_switch(uint8_t dir_curr, uint8_t dir_next, uint8_t phy_curr, uint8_t flags_curr,
	       uint8_t phy_next, uint8_t flags_next);
