/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/display/cfb.h>

const uint8_t cfb_font_1016_rectspace[1][20] = {
	/*   */
	{
		0xFF, 0xFF, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01,
		0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0x80, 0x01, 0xFF, 0xFF,
	},
};

FONT_ENTRY_DEFINE(font1016_rectspace, 10, 16, CFB_FONT_MONO_VPACKED | CFB_FONT_MSB_FIRST,
		  cfb_font_1016_rectspace, 32, 33);
