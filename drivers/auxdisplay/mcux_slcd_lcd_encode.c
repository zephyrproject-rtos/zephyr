/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mcux_slcd_lcd.h"

#include <zephyr/sys/util.h>
#include <zephyr/toolchain/common.h>

static const uint8_t digits_7seg[] = {
	[0] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,
	[1] = SEG_B | SEG_C,
	[2] = SEG_A | SEG_B | SEG_D | SEG_E | SEG_G,
	[3] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_G,
	[4] = SEG_B | SEG_C | SEG_F | SEG_G,
	[5] = SEG_A | SEG_C | SEG_D | SEG_F | SEG_G,
	[6] = SEG_A | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
	[7] = SEG_A | SEG_B | SEG_C,
	[8] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G,
	[9] = SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,
};

uint8_t mcux_slcd_lcd_encode_char(uint8_t ch, bool allow_dot)
{
	uint8_t seg_mask = 0U;

	if (ch >= '0' && ch <= '9') {
		seg_mask = digits_7seg[ch - '0'];
	}
	if (ch == '-') {
		seg_mask = SEG_G;
	}
	if (ch == ' ') {
		seg_mask = 0U;
	}
	if (allow_dot) {
		seg_mask |= SEG_DP;
	}

	return seg_mask;
}
