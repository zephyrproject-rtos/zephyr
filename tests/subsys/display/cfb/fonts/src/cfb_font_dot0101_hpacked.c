/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/display/cfb.h>

#if IS_ENABLED(CONFIG_TEST_MSB_FIRST_FONT)
#define FONT_CAPS (CFB_FONT_MONO_HPACKED | CFB_FONT_MSB_FIRST)

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
#else
#define FONT_CAPS CFB_FONT_MONO_HPACKED

const uint8_t cfb_font_0101_dot[2][1] = {
	/* 0 */
	{
		0x00,
	},
	/* 1 */
	{
		0x01,
	},
};
#endif

FONT_ENTRY_DEFINE(font_0101_dot, 1, 1, FONT_CAPS, cfb_font_0101_dot, 48, 49);
