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

const uint8_t cfb_font_0101_dot[2][1] = {
	/* 0 */
	{
		0x00,
	},
	/* 1 */
	{
		0x80,
	},
};

FONT_ENTRY_DEFINE(font_0101_dot, 1, 1, CFB_FONT_MONO_VPACKED | CFB_FONT_MSB_FIRST,
		  cfb_font_0101_dot, 48, 50);
