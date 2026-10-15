/*
 * Copyright (c) 2026 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "test_region.h"

static const struct test_region_datarate eu868_datarates[] = {
	[LORAWAN_DR_0] = {SF_12, BW_125_KHZ, 2893U}, [LORAWAN_DR_1] = {SF_11, BW_125_KHZ, 1661U},
	[LORAWAN_DR_2] = {SF_10, BW_125_KHZ, 798U},  [LORAWAN_DR_3] = {SF_9, BW_125_KHZ, 777U},
	[LORAWAN_DR_4] = {SF_8, BW_125_KHZ, 756U},   [LORAWAN_DR_5] = {SF_7, BW_125_KHZ, 469U},
	[LORAWAN_DR_6] = {SF_7, BW_250_KHZ, 284U},
};

static const uint32_t cflist_frequencies[] = {
	867100000U, 867300000U, 867500000U, 867900000U, 867900000U,
};

static bool is_default_channel(uint32_t frequency)
{
	return frequency == 868100000U || frequency == 868300000U || frequency == 868500000U;
}

const struct test_region *test_region_get(void)
{
	static const struct test_region region = {
		.id = LORAWAN_REGION_EU868,
		.datarates = eu868_datarates,
		.datarate_count = ARRAY_SIZE(eu868_datarates),
		.rx1_dr_offset = 1U,
		.rx2_datarate = LORAWAN_DR_0,
		.test_datarate = LORAWAN_DR_5,
		.rx2_frequency = 869525000U,
		.is_default_channel = is_default_channel,
		.has_1_0_x_vector = true,
		.cflist_frequencies = cflist_frequencies,
		.cflist_frequency_count = ARRAY_SIZE(cflist_frequencies),
		.cflist_first_channel_idx = 3U,
	};

	return &region;
}
