/*
 * SPDX-FileCopyrightText: 2024 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9806e_dsi.h"
#include "display_ili98xx_dsi.h"

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9806e_dsi, CONFIG_DISPLAY_LOG_LEVEL);

static const struct ili98xx_dsi_cmd ili9806e_init_cmds[] = {
	/* Change to Page 1 CMD */
	{.reg = 0xff, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x01}},
	/* Output SDA */
	{.reg = 0x08, .cmd_len = 1, .cmd = {0x10}},
	/* DE = 1 Active */
	{.reg = 0x21, .cmd_len = 1, .cmd = {0x01}},
	/* Resolution setting 480 X 800 */
	{.reg = 0x30, .cmd_len = 1, .cmd = {0x01}},
	/* Inversion setting */
	{.reg = 0x31, .cmd_len = 1, .cmd = {0x00}},
	/* BT 15 */
	{.reg = 0x40, .cmd_len = 1, .cmd = {0x14}},
	/* avdd +5.2v,avee-5.2v */
	{.reg = 0x41, .cmd_len = 1, .cmd = {0x33}},
	/* VGL=DDVDL+VCL-VCIP,VGH=2DDVDH-DDVDL */
	{.reg = 0x42, .cmd_len = 1, .cmd = {0x02}},
	/* Set VGH clamp level */
	{.reg = 0x43, .cmd_len = 1, .cmd = {0x09}},
	/* Set VGL clamp level */
	{.reg = 0x44, .cmd_len = 1, .cmd = {0x06}},
	/* Set VREG1 */
	{.reg = 0x50, .cmd_len = 1, .cmd = {0x70}},
	/* Set VREG2 */
	{.reg = 0x51, .cmd_len = 1, .cmd = {0x70}},
	/* Flicker MSB */
	{.reg = 0x52, .cmd_len = 1, .cmd = {0x00}},
	/* Flicker LSB */
	{.reg = 0x53, .cmd_len = 1, .cmd = {0x48}},
	/* Timing Adjust */
	{.reg = 0x60, .cmd_len = 1, .cmd = {0x07}},
	{.reg = 0x61, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x62, .cmd_len = 1, .cmd = {0x08}},
	{.reg = 0x63, .cmd_len = 1, .cmd = {0x00}},
	/* Positive Gamma Control 1 */
	{.reg = 0xa0, .cmd_len = 1, .cmd = {0x00}},
	/* Positive Gamma Control 2 */
	{.reg = 0xa1, .cmd_len = 1, .cmd = {0x03}},
	/* Positive Gamma Control 3 */
	{.reg = 0xa2, .cmd_len = 1, .cmd = {0x09}},
	/* Positive Gamma Control 4 */
	{.reg = 0xa3, .cmd_len = 1, .cmd = {0x0d}},
	/* Positive Gamma Control 5 */
	{.reg = 0xa4, .cmd_len = 1, .cmd = {0x06}},
	/* Positive Gamma Control 6 */
	{.reg = 0xa5, .cmd_len = 1, .cmd = {0x16}},
	/* Positive Gamma Control 7 */
	{.reg = 0xa6, .cmd_len = 1, .cmd = {0x09}},
	/* Positive Gamma Control 8 */
	{.reg = 0xa7, .cmd_len = 1, .cmd = {0x08}},
	/* Positive Gamma Control 9 */
	{.reg = 0xa8, .cmd_len = 1, .cmd = {0x03}},
	/* Positive Gamma Control 10 */
	{.reg = 0xa9, .cmd_len = 1, .cmd = {0x07}},
	/* Positive Gamma Control 11 */
	{.reg = 0xaa, .cmd_len = 1, .cmd = {0x06}},
	/* Positive Gamma Control 12 */
	{.reg = 0xab, .cmd_len = 1, .cmd = {0x05}},
	/* Positive Gamma Control 13 */
	{.reg = 0xac, .cmd_len = 1, .cmd = {0x0d}},
	/* Positive Gamma Control 14 */
	{.reg = 0xad, .cmd_len = 1, .cmd = {0x2c}},
	/* Positive Gamma Control 15 */
	{.reg = 0xae, .cmd_len = 1, .cmd = {0x26}},
	/* Positive Gamma Control 16 */
	{.reg = 0xaf, .cmd_len = 1, .cmd = {0x00}},
	/* Negative Gamma Correction 1 */
	{.reg = 0xc0, .cmd_len = 1, .cmd = {0x00}},
	/* Negative Gamma Correction 2 */
	{.reg = 0xc1, .cmd_len = 1, .cmd = {0x04}},
	/* Negative Gamma Correction 3 */
	{.reg = 0xc2, .cmd_len = 1, .cmd = {0x0b}},
	/* Negative Gamma Correction 4 */
	{.reg = 0xc3, .cmd_len = 1, .cmd = {0x0f}},
	/* Negative Gamma Correction 5 */
	{.reg = 0xc4, .cmd_len = 1, .cmd = {0x09}},
	/* Negative Gamma Correction 6 */
	{.reg = 0xc5, .cmd_len = 1, .cmd = {0x18}},
	/* Negative Gamma Correction 7 */
	{.reg = 0xc6, .cmd_len = 1, .cmd = {0x07}},
	/* Negative Gamma Correction 8 */
	{.reg = 0xc7, .cmd_len = 1, .cmd = {0x08}},
	/* Negative Gamma Correction 9 */
	{.reg = 0xc8, .cmd_len = 1, .cmd = {0x05}},
	/* Negative Gamma Correction 10 */
	{.reg = 0xc9, .cmd_len = 1, .cmd = {0x09}},
	/* Negative Gamma Correction 11 */
	{.reg = 0xca, .cmd_len = 1, .cmd = {0x07}},
	/* Negative Gamma Correction 12 */
	{.reg = 0xcb, .cmd_len = 1, .cmd = {0x05}},
	/* Negative Gamma Correction 13 */
	{.reg = 0xcc, .cmd_len = 1, .cmd = {0x0c}},
	/* Negative Gamma Correction 14 */
	{.reg = 0xcd, .cmd_len = 1, .cmd = {0x2d}},
	/* Negative Gamma Correction 15 */
	{.reg = 0xce, .cmd_len = 1, .cmd = {0x28}},
	/* Negative Gamma Correction 16 */
	{.reg = 0xcf, .cmd_len = 1, .cmd = {0x00}},

	/* Change to Page 6 CMD for GIP timing */
	{.reg = 0xff, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x06}},
	/* GIP Control 1 */
	{.reg = 0x00, .cmd_len = 1, .cmd = {0x21}},
	{.reg = 0x01, .cmd_len = 1, .cmd = {0x09}},
	{.reg = 0x02, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x03, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x04, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x05, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x06, .cmd_len = 1, .cmd = {0x80}},
	{.reg = 0x07, .cmd_len = 1, .cmd = {0x05}},
	{.reg = 0x08, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x09, .cmd_len = 1, .cmd = {0x80}},
	{.reg = 0x0a, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0b, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0c, .cmd_len = 1, .cmd = {0x0a}},
	{.reg = 0x0d, .cmd_len = 1, .cmd = {0x0a}},
	{.reg = 0x0e, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0f, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x10, .cmd_len = 1, .cmd = {0xe0}},
	{.reg = 0x11, .cmd_len = 1, .cmd = {0xe4}},
	{.reg = 0x12, .cmd_len = 1, .cmd = {0x04}},
	{.reg = 0x13, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x14, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x15, .cmd_len = 1, .cmd = {0xc0}},
	{.reg = 0x16, .cmd_len = 1, .cmd = {0x08}},
	{.reg = 0x17, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x18, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x19, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1a, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1b, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1c, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1d, .cmd_len = 1, .cmd = {0x00}},
	/* GIP Control 2 */
	{.reg = 0x20, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x21, .cmd_len = 1, .cmd = {0x23}},
	{.reg = 0x22, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0x23, .cmd_len = 1, .cmd = {0x67}},
	{.reg = 0x24, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x25, .cmd_len = 1, .cmd = {0x23}},
	{.reg = 0x26, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0x27, .cmd_len = 1, .cmd = {0x67}},
	/* GIP Control 3 */
	{.reg = 0x30, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x31, .cmd_len = 1, .cmd = {0x11}},
	{.reg = 0x32, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x33, .cmd_len = 1, .cmd = {0xee}},
	{.reg = 0x34, .cmd_len = 1, .cmd = {0xff}},
	{.reg = 0x35, .cmd_len = 1, .cmd = {0xcb}},
	{.reg = 0x36, .cmd_len = 1, .cmd = {0xda}},
	{.reg = 0x37, .cmd_len = 1, .cmd = {0xad}},
	{.reg = 0x38, .cmd_len = 1, .cmd = {0xbc}},
	{.reg = 0x39, .cmd_len = 1, .cmd = {0x76}},
	{.reg = 0x3a, .cmd_len = 1, .cmd = {0x67}},
	{.reg = 0x3b, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x3c, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x3d, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x3e, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x3f, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x40, .cmd_len = 1, .cmd = {0x22}},
	/* GOUT VGLO Control */
	{.reg = 0x53, .cmd_len = 1, .cmd = {0x10}},
	{.reg = 0x54, .cmd_len = 1, .cmd = {0x10}},
	/* Change to Page 7 CMD for Normal command */
	{.reg = 0xff, .cmd_len = 5, .cmd = {0xff, 0x98, 0x06, 0x04, 0x07}},
	/* VREG1/2OUT ENABLE */
	{.reg = 0x18, .cmd_len = 1, .cmd = {0x1d}},
	{.reg = 0x26, .cmd_len = 1, .cmd = {0xb2}},
	{.reg = 0x02, .cmd_len = 1, .cmd = {0x77}},
	{.reg = 0xe1, .cmd_len = 1, .cmd = {0x79}},
	{.reg = 0x17, .cmd_len = 1, .cmd = {0x22}},
	/* Change to Page 0 CMD for Normal command */
	{.reg = 0xff, .cmd_len = 5, .cmd = {0xff, 0x98, 0x06, 0x04, 0x00}},
};

int ili9806e_dsi_regs_init(const struct device *dev)
{
	return ili98xx_dsi_write_sequence(dev, ili9806e_init_cmds, ARRAY_SIZE(ili9806e_init_cmds));
}
