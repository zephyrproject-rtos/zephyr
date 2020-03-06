/*
 * Copyright (c) 2020 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_BT_CTLR_PHY_CODED
#define PHYS_TO_TEST 3
static u16_t phy_to_test[] = { BIT(0), BIT(1), BIT(2) };

static u16_t expected_us[][PHYS_TO_TEST] = { { 328, 168, 2704 },
					  { 592, 300, 4816 },
					  { 1136, 572, 9168 },
					  { 2120, 1064, 17040 } };
#else

#define PHYS_TO_TEST 2
static u16_t phy_to_test[] = { BIT(0), BIT(1) };

static u16_t expected_us[][PHYS_TO_TEST] = { { 328, 168 },
					  { 592, 300 },
					  { 1136, 572 },
					  { 2120, 1064 } };
#endif
static u16_t octets[] = { 27, 60, 128, 251 };
