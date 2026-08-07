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

static inline uint32_t addr_us_get(uint8_t phy)
{
	switch (phy) {
	default:
	case PHY_1M:
		return 40;
	case PHY_2M:
		return 24;
	case PHY_CODED:
		return 376;
	}
}
