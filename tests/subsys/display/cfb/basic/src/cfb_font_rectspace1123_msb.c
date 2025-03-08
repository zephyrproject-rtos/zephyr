/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * This file was generated from the font DroidSansMono.ttf
 * Copyright (C) 2008 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/display/cfb.h>

const uint8_t cfb_font_1123_rectspace[1][39] = {
	/*   */
	{
		0xFF, 0xFF, 0xFE, 0x80, 0x00, 0x02, 0x80, 0x00, 0x02, 0x80, 0x00,
		0x02, 0x80, 0x00, 0x02, 0x80, 0x00, 0x02, 0x80, 0x00, 0x02, 0x80,
		0x00, 0x02, 0x80, 0x00, 0x02, 0x80, 0x00, 0x02, 0xFF, 0xFF, 0xFE,
	},
};

FONT_ENTRY_DEFINE(font1123_rect_space, 11, 23, CFB_FONT_MONO_VPACKED | CFB_FONT_MSB_FIRST,
		  cfb_font_1123_rectspace, 32, 33);
