/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9881c_dsi.h"
#include "display_ili98xx_dsi.h"

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9881c_dsi, CONFIG_DISPLAY_LOG_LEVEL);

static const struct ili98xx_dsi_cmd ili9881c_init_cmds[] = {
	/* Change to Page 3 CMD */
	{.reg = 0xFF, .cmd_len = 3, .cmd = {0x98, 0x81, 0x03}},
	/* GIP_1 */
	{.reg = 0x01, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x02, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x03, .cmd_len = 1, .cmd = {0x72}},
	{.reg = 0x04, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x05, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x06, .cmd_len = 1, .cmd = {0x09}},
	{.reg = 0x07, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x08, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x09, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x0A, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0B, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0C, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x0D, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0E, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x0F, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x10, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x11, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x12, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x13, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x14, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x15, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x16, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x17, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x18, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x19, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1A, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1B, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1C, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1D, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x1E, .cmd_len = 1, .cmd = {0x40}},
	{.reg = 0x1F, .cmd_len = 1, .cmd = {0x80}},
	{.reg = 0x20, .cmd_len = 1, .cmd = {0x05}},
	{.reg = 0x21, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x22, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x23, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x24, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x25, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x26, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x27, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x28, .cmd_len = 1, .cmd = {0x33}},
	{.reg = 0x29, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x2A, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2B, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2C, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2D, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2E, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x2F, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x30, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x31, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x32, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x32, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x33, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x34, .cmd_len = 1, .cmd = {0x04}},
	{.reg = 0x35, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x36, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x37, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x38, .cmd_len = 1, .cmd = {0x3C}},
	{.reg = 0x39, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x3A, .cmd_len = 1, .cmd = {0x40}},
	{.reg = 0x3B, .cmd_len = 1, .cmd = {0x40}},
	{.reg = 0x3C, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x3D, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x3E, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x3F, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x40, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x41, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x42, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x43, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x44, .cmd_len = 1, .cmd = {0x00}},
	/* GIP_2 */
	{.reg = 0x50, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x51, .cmd_len = 1, .cmd = {0x23}},
	{.reg = 0x52, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0x53, .cmd_len = 1, .cmd = {0x67}},
	{.reg = 0x54, .cmd_len = 1, .cmd = {0x89}},
	{.reg = 0x55, .cmd_len = 1, .cmd = {0xAB}},
	{.reg = 0x56, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x57, .cmd_len = 1, .cmd = {0x23}},
	{.reg = 0x58, .cmd_len = 1, .cmd = {0x45}},
	{.reg = 0x59, .cmd_len = 1, .cmd = {0x67}},
	{.reg = 0x5A, .cmd_len = 1, .cmd = {0x89}},
	{.reg = 0x5B, .cmd_len = 1, .cmd = {0xAB}},
	{.reg = 0x5C, .cmd_len = 1, .cmd = {0xCD}},
	{.reg = 0x5D, .cmd_len = 1, .cmd = {0xEF}},
	/* GIP_3 */
	{.reg = 0x5E, .cmd_len = 1, .cmd = {0x11}},
	{.reg = 0x5F, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x60, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x61, .cmd_len = 1, .cmd = {0x15}},
	{.reg = 0x62, .cmd_len = 1, .cmd = {0x14}},
	{.reg = 0x63, .cmd_len = 1, .cmd = {0x0E}},
	{.reg = 0x64, .cmd_len = 1, .cmd = {0x0F}},
	{.reg = 0x65, .cmd_len = 1, .cmd = {0x0C}},
	{.reg = 0x66, .cmd_len = 1, .cmd = {0x0D}},
	{.reg = 0x67, .cmd_len = 1, .cmd = {0x06}},
	{.reg = 0x68, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x69, .cmd_len = 1, .cmd = {0x07}},
	{.reg = 0x6A, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6B, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6C, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6D, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6E, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x6F, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x70, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x71, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x72, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x73, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x74, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x75, .cmd_len = 1, .cmd = {0x01}},
	{.reg = 0x76, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x77, .cmd_len = 1, .cmd = {0x14}},
	{.reg = 0x78, .cmd_len = 1, .cmd = {0x15}},
	{.reg = 0x79, .cmd_len = 1, .cmd = {0x0E}},
	{.reg = 0x7A, .cmd_len = 1, .cmd = {0x0F}},
	{.reg = 0x7B, .cmd_len = 1, .cmd = {0x0C}},
	{.reg = 0x7C, .cmd_len = 1, .cmd = {0x0D}},
	{.reg = 0x7D, .cmd_len = 1, .cmd = {0x06}},
	{.reg = 0x7E, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x7F, .cmd_len = 1, .cmd = {0x07}},
	{.reg = 0x80, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x81, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x83, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x84, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x85, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x86, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x87, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x88, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x89, .cmd_len = 1, .cmd = {0x02}},
	{.reg = 0x8A, .cmd_len = 1, .cmd = {0x02}},
	/* Change to Page 4 CMD */
	{.reg = 0xFF, .cmd_len = 3, .cmd = {0x98, 0x81, 0x04}},
	/* 6Ch: VCORE setting */
	{.reg = 0x6C, .cmd_len = 1, .cmd = {0x15}}, /* 1.5 V*/
	/* 6Eh Power Control 2 */
	{.reg = 0x6E, .cmd_len = 1, .cmd = {0x2A}}, /* VGH 15V */
	/* 6Fh: Power Control 3 */
	{.reg = 0x6F, .cmd_len = 1, .cmd = {0x33}},
	{.reg = 0x3A, .cmd_len = 1, .cmd = {0x94}},
	/* 8Dh: Power Control 4 */
	{.reg = 0x8D, .cmd_len = 1, .cmd = {0x15}}, /* VGL 10.23V */
	{.reg = 0x87, .cmd_len = 1, .cmd = {0xBA}},
	{.reg = 0x26, .cmd_len = 1, .cmd = {0x76}},
	{.reg = 0xB2, .cmd_len = 1, .cmd = {0xD1}},
	{.reg = 0xB5, .cmd_len = 1, .cmd = {0x06}},
	/* Change to Page 1 CMD */
	{.reg = 0xFF, .cmd_len = 3, .cmd = {0x98, 0x81, 0x01}},
	/* 22h: Panel Operation Mode */
	{.reg = 0x22, .cmd_len = 1, .cmd = {0x0A}}, /* BGR_PANEL | SS_PANEL */
	/* 31h: Display Inversion Control */
	{.reg = 0x31, .cmd_len = 1, .cmd = {0x00}}, /* Column inversion */
	/* 53h/55h: VCOM Control 1 */
	{.reg = 0x53, .cmd_len = 1, .cmd = {0xA5}}, /* VCOM forward scan 1.992V */
	{.reg = 0x55, .cmd_len = 1, .cmd = {0xA2}}, /* VCOM backward scan 1.956V */
	/* 50h/51h: Power Control 1 */
	{.reg = 0x50, .cmd_len = 1, .cmd = {0xB7}}, /* VREG1OUT 4.896V */
	{.reg = 0x51, .cmd_len = 1, .cmd = {0xB7}}, /* VREG2IUT -4.896V */
	/* 60h..63h: Source Timing SDT/CRT/EQT/PCT, TOP_CLK = 62.5ns */
	{.reg = 0x60, .cmd_len = 1, .cmd = {0x22}},
	{.reg = 0x61, .cmd_len = 1, .cmd = {0x00}},
	{.reg = 0x62, .cmd_len = 1, .cmd = {0x19}},
	{.reg = 0x63, .cmd_len = 1, .cmd = {0x10}},
	/* A0h..B3h: Positive Gamma Correction */
	{.reg = 0xA0, .cmd_len = 1, .cmd = {0x08}}, /* VP0 */
	{.reg = 0xA1, .cmd_len = 1, .cmd = {0x17}}, /* VP4 */
	{.reg = 0xA2, .cmd_len = 1, .cmd = {0x1E}}, /* VP8 */
	{.reg = 0xA3, .cmd_len = 1, .cmd = {0x0E}}, /* VP12 */
	{.reg = 0xA4, .cmd_len = 1, .cmd = {0x13}}, /* VP16 */
	{.reg = 0xA5, .cmd_len = 1, .cmd = {0x24}}, /* VP24 */
	{.reg = 0xA6, .cmd_len = 1, .cmd = {0x1B}}, /* VP36 */
	{.reg = 0xA7, .cmd_len = 1, .cmd = {0x1B}}, /* VP52 */
	{.reg = 0xA8, .cmd_len = 1, .cmd = {0x53}}, /* VP80 */
	{.reg = 0xA9, .cmd_len = 1, .cmd = {0x1B}}, /* VP111 */
	{.reg = 0xAA, .cmd_len = 1, .cmd = {0x28}}, /* VP144 */
	{.reg = 0xAB, .cmd_len = 1, .cmd = {0x45}}, /* VP175 */
	{.reg = 0xAC, .cmd_len = 1, .cmd = {0x1A}}, /* VP203 */
	{.reg = 0xAD, .cmd_len = 1, .cmd = {0x1A}}, /* VP219 */
	{.reg = 0xAE, .cmd_len = 1, .cmd = {0x50}}, /* VP231 */
	{.reg = 0xAF, .cmd_len = 1, .cmd = {0x21}}, /* VP239 */
	{.reg = 0xB0, .cmd_len = 1, .cmd = {0x2C}}, /* VP243 */
	{.reg = 0xB1, .cmd_len = 1, .cmd = {0x3B}}, /* VP247 */
	{.reg = 0xB2, .cmd_len = 1, .cmd = {0x63}}, /* VP251 */
	{.reg = 0xB3, .cmd_len = 1, .cmd = {0x39}}, /* VP255 */
	/* C0h..D3h: Negative Gamma Correction */
	{.reg = 0xC0, .cmd_len = 1, .cmd = {0x08}}, /* VN0 */
	{.reg = 0xC1, .cmd_len = 1, .cmd = {0x0C}}, /* VN4 */
	{.reg = 0xC2, .cmd_len = 1, .cmd = {0x17}}, /* VN8 */
	{.reg = 0xC3, .cmd_len = 1, .cmd = {0x0F}}, /* VN12 */
	{.reg = 0xC4, .cmd_len = 1, .cmd = {0x0B}}, /* VN16 */
	{.reg = 0xC5, .cmd_len = 1, .cmd = {0x1C}}, /* VN24 */
	{.reg = 0xC6, .cmd_len = 1, .cmd = {0x10}}, /* VN36 */
	{.reg = 0xC7, .cmd_len = 1, .cmd = {0x16}}, /* VN52 */
	{.reg = 0xC8, .cmd_len = 1, .cmd = {0x5B}}, /* VN80 */
	{.reg = 0xC9, .cmd_len = 1, .cmd = {0x1A}}, /* VN111 */
	{.reg = 0xCA, .cmd_len = 1, .cmd = {0x26}}, /* VN144 */
	{.reg = 0xCB, .cmd_len = 1, .cmd = {0x55}}, /* VN175 */
	{.reg = 0xCC, .cmd_len = 1, .cmd = {0x1D}}, /* VN203 */
	{.reg = 0xCD, .cmd_len = 1, .cmd = {0x1E}}, /* VN219 */
	{.reg = 0xCE, .cmd_len = 1, .cmd = {0x52}}, /* VN231 */
	{.reg = 0xCF, .cmd_len = 1, .cmd = {0x26}}, /* VN239 */
	{.reg = 0xD0, .cmd_len = 1, .cmd = {0x29}}, /* VN243 */
	{.reg = 0xD1, .cmd_len = 1, .cmd = {0x45}}, /* VN247 */
	{.reg = 0xD2, .cmd_len = 1, .cmd = {0x63}}, /* VN251 */
	{.reg = 0xD3, .cmd_len = 1, .cmd = {0x39}}, /* VN255 */
	/* Change to Page 0 CMD */
	{.reg = 0xFF, .cmd_len = 3, .cmd = {0x98, 0x81, 0x00}},
};

int ili9881c_dsi_regs_init(const struct device *dev)
{
	return ili98xx_dsi_write_sequence(dev, ili9881c_init_cmds, ARRAY_SIZE(ili9881c_init_cmds));
}
