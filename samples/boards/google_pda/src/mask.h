/*
 * Copyright (c) 2022 The Chromium OS Authors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MASK_H_
#define _MASK_H_

/**
 * @defgroup Bitmask for snooper state
 * Bit positions for the bitmask for setting the role,
 * active CC line, and pull resistors of the Twinkie
 * @{
 */
#define snooper_mask_t 		uint8_t

#define PULL_RESISTOR_BITS 	(BIT(0) | BIT(1))
#define SINK_BIT 		BIT(2)
#define CC1_CHANNEL_BIT 	BIT(3)
#define CC2_CHANNEL_BIT 	BIT(4)
/** @} */

#endif
