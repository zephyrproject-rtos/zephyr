/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/display/cfb.h>

const uint8_t cfb_font_stub[1][1] = {
	/*   */
	{
		0x00,
	},
};

FONT_ENTRY_DEFINE(font_stub, 1, 1, CFB_FONT_MONO_VPACKED | CFB_FONT_MSB_FIRST,
		  cfb_font_stub, 32, 33);
