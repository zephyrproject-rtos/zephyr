/*
 * Copyright (c) 2018-2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Range Delay
 * Refer to BT Spec v5.1 Vol.6, Part B, Section 4.2.3 Range Delay
 * "4 / 1000" is an approximation of the propagation time in us of the
 * signal to travel 1 meter.
 */
#define RANGE_DISTANCE 1000 /* meters */
#define RANGE_DELAY_US (2 * RANGE_DISTANCE * 4 / 1000)

static inline u32_t addr_us_get(u8_t phy)
{
	switch (phy) {
	default:
	case BIT(0):
		return 40;
	case BIT(1):
		return 24;
	case BIT(2):
		return 376;
	}
}
