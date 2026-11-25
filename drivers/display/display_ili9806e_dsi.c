/*
 * SPDX-FileCopyrightText: 2024 - 2026 Renesas Electronics Corporation
 * SPDX-FileCopyrightText: Copyright 2026 Analog Devices, Inc.
 * SPDX-FileCopyrightText: Copyright (C) 2024 Alif Semiconductor.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9806e_dsi.h"
#include "display_ili98xx_dsi.h"

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(display_ili9806e_dsi, CONFIG_DISPLAY_LOG_LEVEL);

static const struct ili98xx_dsi_cmd rtkmipilcdb00000be_init_cmds[] = {
	/* Change to Page 1 CMD */
	{.reg = ILI98XX_DSI_ENEXTC, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x01}},
	/* Output SDA */
	{.reg = ILI9806E_IFMODE_1, .cmd_len = 1, .cmd = {0x10}},
	/* DE = 1 Active */
	{.reg = ILI9806E_DISCTRL2, .cmd_len = 1, .cmd = {0x01}},
	/* Resolution setting 480 X 800 */
	{.reg = ILI9806E_RESCTRL, .cmd_len = 1, .cmd = {0x01}},
	/* Inversion setting */
	{.reg = ILI98XX_DSI_INVTR, .cmd_len = 1, .cmd = {0x00}},
	/* BT 15 */
	{.reg = ILI9806E_PWCTRL_1, .cmd_len = 1, .cmd = {0x14}},
	/* avdd +5.2v,avee-5.2v */
	{.reg = ILI9806E_PWCTRL_2, .cmd_len = 1, .cmd = {0x33}},
	/* VGL=DDVDL+VCL-VCIP,VGH=2DDVDH-DDVDL */
	{.reg = ILI9806E_PWCTRL_3, .cmd_len = 1, .cmd = {0x02}},
	/* Set VGH clamp level */
	{.reg = ILI9806E_PWCTRL_4, .cmd_len = 1, .cmd = {0x09}},
	/* Set VGL clamp level */
	{.reg = ILI9806E_PWCTRL_5, .cmd_len = 1, .cmd = {0x06}},
	/* Set VREG1 */
	{.reg = ILI98XX_DSI_VREG1, .cmd_len = 1, .cmd = {0x70}},
	/* Set VREG2 */
	{.reg = ILI98XX_DSI_VREG2, .cmd_len = 1, .cmd = {0x70}},
	/* Flicker MSB */
	{.reg = ILI98XX_DSI_VCM1_MSB, .cmd_len = 1, .cmd = {0x00}},
	/* Flicker LSB */
	{.reg = ILI98XX_DSI_VCM1_LSB, .cmd_len = 1, .cmd = {0x48}},
	/* Timing Adjust */
	{.reg = ILI98XX_DSI_SOURCE_TIMING_ADJUST_1, .cmd_len = 1, .cmd = {0x07}},
	{.reg = ILI98XX_DSI_SOURCE_TIMING_ADJUST_2, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI98XX_DSI_SOURCE_TIMING_ADJUST_3, .cmd_len = 1, .cmd = {0x08}},
	{.reg = ILI98XX_DSI_SOURCE_TIMING_ADJUST_4, .cmd_len = 1, .cmd = {0x00}},
	/* Positive Gamma Control 1 */
	{.reg = ILI98XX_DSI_PGAMCTRL1, .cmd_len = 1, .cmd = {0x00}},
	/* Positive Gamma Control 2 */
	{.reg = ILI98XX_DSI_PGAMCTRL2, .cmd_len = 1, .cmd = {0x03}},
	/* Positive Gamma Control 3 */
	{.reg = ILI98XX_DSI_PGAMCTRL3, .cmd_len = 1, .cmd = {0x09}},
	/* Positive Gamma Control 4 */
	{.reg = ILI98XX_DSI_PGAMCTRL4, .cmd_len = 1, .cmd = {0x0d}},
	/* Positive Gamma Control 5 */
	{.reg = ILI98XX_DSI_PGAMCTRL5, .cmd_len = 1, .cmd = {0x06}},
	/* Positive Gamma Control 6 */
	{.reg = ILI98XX_DSI_PGAMCTRL6, .cmd_len = 1, .cmd = {0x16}},
	/* Positive Gamma Control 7 */
	{.reg = ILI98XX_DSI_PGAMCTRL7, .cmd_len = 1, .cmd = {0x09}},
	/* Positive Gamma Control 8 */
	{.reg = ILI98XX_DSI_PGAMCTRL8, .cmd_len = 1, .cmd = {0x08}},
	/* Positive Gamma Control 9 */
	{.reg = ILI98XX_DSI_PGAMCTRL9, .cmd_len = 1, .cmd = {0x03}},
	/* Positive Gamma Control 10 */
	{.reg = ILI98XX_DSI_PGAMCTRL10, .cmd_len = 1, .cmd = {0x07}},
	/* Positive Gamma Control 11 */
	{.reg = ILI98XX_DSI_PGAMCTRL11, .cmd_len = 1, .cmd = {0x06}},
	/* Positive Gamma Control 12 */
	{.reg = ILI98XX_DSI_PGAMCTRL12, .cmd_len = 1, .cmd = {0x05}},
	/* Positive Gamma Control 13 */
	{.reg = ILI98XX_DSI_PGAMCTRL13, .cmd_len = 1, .cmd = {0x0d}},
	/* Positive Gamma Control 14 */
	{.reg = ILI98XX_DSI_PGAMCTRL14, .cmd_len = 1, .cmd = {0x2c}},
	/* Positive Gamma Control 15 */
	{.reg = ILI98XX_DSI_PGAMCTRL15, .cmd_len = 1, .cmd = {0x26}},
	/* Positive Gamma Control 16 */
	{.reg = ILI98XX_DSI_PGAMCTRL16, .cmd_len = 1, .cmd = {0x00}},
	/* Negative Gamma Correction 1 */
	{.reg = ILI98XX_DSI_NGAMCTRL1, .cmd_len = 1, .cmd = {0x00}},
	/* Negative Gamma Correction 2 */
	{.reg = ILI98XX_DSI_NGAMCTRL2, .cmd_len = 1, .cmd = {0x04}},
	/* Negative Gamma Correction 3 */
	{.reg = ILI98XX_DSI_NGAMCTRL3, .cmd_len = 1, .cmd = {0x0b}},
	/* Negative Gamma Correction 4 */
	{.reg = ILI98XX_DSI_NGAMCTRL4, .cmd_len = 1, .cmd = {0x0f}},
	/* Negative Gamma Correction 5 */
	{.reg = ILI98XX_DSI_NGAMCTRL5, .cmd_len = 1, .cmd = {0x09}},
	/* Negative Gamma Correction 6 */
	{.reg = ILI98XX_DSI_NGAMCTRL6, .cmd_len = 1, .cmd = {0x18}},
	/* Negative Gamma Correction 7 */
	{.reg = ILI98XX_DSI_NGAMCTRL7, .cmd_len = 1, .cmd = {0x07}},
	/* Negative Gamma Correction 8 */
	{.reg = ILI98XX_DSI_NGAMCTRL8, .cmd_len = 1, .cmd = {0x08}},
	/* Negative Gamma Correction 9 */
	{.reg = ILI98XX_DSI_NGAMCTRL9, .cmd_len = 1, .cmd = {0x05}},
	/* Negative Gamma Correction 10 */
	{.reg = ILI98XX_DSI_NGAMCTRL10, .cmd_len = 1, .cmd = {0x09}},
	/* Negative Gamma Correction 11 */
	{.reg = ILI98XX_DSI_NGAMCTRL11, .cmd_len = 1, .cmd = {0x07}},
	/* Negative Gamma Correction 12 */
	{.reg = ILI98XX_DSI_NGAMCTRL12, .cmd_len = 1, .cmd = {0x05}},
	/* Negative Gamma Correction 13 */
	{.reg = ILI98XX_DSI_NGAMCTRL13, .cmd_len = 1, .cmd = {0x0c}},
	/* Negative Gamma Correction 14 */
	{.reg = ILI98XX_DSI_NGAMCTRL14, .cmd_len = 1, .cmd = {0x2d}},
	/* Negative Gamma Correction 15 */
	{.reg = ILI98XX_DSI_NGAMCTRL15, .cmd_len = 1, .cmd = {0x28}},
	/* Negative Gamma Correction 16 */
	{.reg = ILI98XX_DSI_NGAMCTRL16, .cmd_len = 1, .cmd = {0x00}},

	/* Change to Page 6 CMD for GIP timing */
	{.reg = ILI98XX_DSI_ENEXTC, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x06}},
	/* GIP Control 1 */
	{.reg = ILI9806E_GIPCTRL1_00, .cmd_len = 1, .cmd = {0x21}},
	{.reg = ILI9806E_GIPCTRL1_01, .cmd_len = 1, .cmd = {0x09}},
	{.reg = ILI9806E_GIPCTRL1_02, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_03, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_04, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_05, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_06, .cmd_len = 1, .cmd = {0x80}},
	{.reg = ILI9806E_GIPCTRL1_07, .cmd_len = 1, .cmd = {0x05}},
	{.reg = ILI9806E_GIPCTRL1_08, .cmd_len = 1, .cmd = {0x02}},
	{.reg = ILI9806E_GIPCTRL1_09, .cmd_len = 1, .cmd = {0x80}},
	{.reg = ILI9806E_GIPCTRL1_0A, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_0B, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_0C, .cmd_len = 1, .cmd = {0x0a}},
	{.reg = ILI9806E_GIPCTRL1_0D, .cmd_len = 1, .cmd = {0x0a}},
	{.reg = ILI9806E_GIPCTRL1_0E, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_0F, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_10, .cmd_len = 1, .cmd = {0xe0}},
	{.reg = ILI9806E_GIPCTRL1_11, .cmd_len = 1, .cmd = {0xe4}},
	{.reg = ILI9806E_GIPCTRL1_12, .cmd_len = 1, .cmd = {0x04}},
	{.reg = ILI9806E_GIPCTRL1_13, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_14, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_15, .cmd_len = 1, .cmd = {0xc0}},
	{.reg = ILI9806E_GIPCTRL1_16, .cmd_len = 1, .cmd = {0x08}},
	{.reg = ILI9806E_GIPCTRL1_17, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_18, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_19, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_1A, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_1B, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_1C, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_1D, .cmd_len = 1, .cmd = {0x00}},
	/* GIP Control 2 */
	{.reg = ILI9806E_GIPCTRL2_20, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL2_21, .cmd_len = 1, .cmd = {0x23}},
	{.reg = ILI9806E_GIPCTRL2_22, .cmd_len = 1, .cmd = {0x45}},
	{.reg = ILI9806E_GIPCTRL2_23, .cmd_len = 1, .cmd = {0x67}},
	{.reg = ILI9806E_GIPCTRL2_24, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL2_25, .cmd_len = 1, .cmd = {0x23}},
	{.reg = ILI9806E_GIPCTRL2_26, .cmd_len = 1, .cmd = {0x45}},
	{.reg = ILI9806E_GIPCTRL2_27, .cmd_len = 1, .cmd = {0x67}},
	/* GIP Control 3 */
	{.reg = ILI9806E_GIPCTRL3_30, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL3_31, .cmd_len = 1, .cmd = {0x11}},
	{.reg = ILI9806E_GIPCTRL3_32, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL3_33, .cmd_len = 1, .cmd = {0xee}},
	{.reg = ILI9806E_GIPCTRL3_34, .cmd_len = 1, .cmd = {0xff}},
	{.reg = ILI9806E_GIPCTRL3_35, .cmd_len = 1, .cmd = {0xcb}},
	{.reg = ILI9806E_GIPCTRL3_36, .cmd_len = 1, .cmd = {0xda}},
	{.reg = ILI9806E_GIPCTRL3_37, .cmd_len = 1, .cmd = {0xad}},
	{.reg = ILI9806E_GIPCTRL3_38, .cmd_len = 1, .cmd = {0xbc}},
	{.reg = ILI9806E_GIPCTRL3_39, .cmd_len = 1, .cmd = {0x76}},
	{.reg = ILI9806E_GIPCTRL3_3A, .cmd_len = 1, .cmd = {0x67}},
	{.reg = ILI9806E_GIPCTRL3_3B, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3C, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3D, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3E, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3F, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_40, .cmd_len = 1, .cmd = {0x22}},
	/* GOUT VGLO Control */
	{.reg = ILI9806E_GVLOCTRL, .cmd_len = 1, .cmd = {0x10}},
	{.reg = ILI9806E_GVHOCTRL, .cmd_len = 1, .cmd = {0x10}},
	/* Change to Page 7 CMD for Normal command */
	{.reg = ILI98XX_DSI_ENEXTC, .cmd_len = 5, .cmd = {0xff, 0x98, 0x06, 0x04, 0x07}},
	/* VREG1/2OUT ENABLE */
	{.reg = ILI9806E_VREG12EN, .cmd_len = 1, .cmd = {0x1d}},
	{.reg = ILI9806E_P7_REG_26, .cmd_len = 1, .cmd = {0xb2}},
	{.reg = ILI9806E_P7_REG_02, .cmd_len = 1, .cmd = {0x77}},
	{.reg = ILI9806E_P7_REG_E1, .cmd_len = 1, .cmd = {0x79}},
	{.reg = ILI9806E_VGLREGEN, .cmd_len = 1, .cmd = {0x22}},
	/* Change to Page 0 CMD for Normal command */
	{.reg = ILI98XX_DSI_ENEXTC, .cmd_len = 5, .cmd = {0xff, 0x98, 0x06, 0x04, 0x00}},
};

int rtkmipilcdb00000be_dsi_regs_init(const struct device *dev)
{
	return ili98xx_dsi_write_sequence(dev, rtkmipilcdb00000be_init_cmds,
					  ARRAY_SIZE(rtkmipilcdb00000be_init_cmds));
}

static const struct ili98xx_dsi_cmd mw405_init_cmds[] = {
	/* Change to Page 1 */
	{.reg = ILI98XX_DSI_ENEXTC, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x01}},
	{.reg = ILI9806E_IFMODE_1,
	 .cmd_len = 1,
	 .cmd = {0x10}}, /* SDO always output without tri-state */
	{.reg = ILI9806E_DISCTRL1, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_DISCTRL2, .cmd_len = 1, .cmd = {0x01}}, /* DE Active High */
	{.reg = ILI9806E_RESCTRL, .cmd_len = 1, .cmd = {0x02}},  /* Resolution 480 X 800 */
	{.reg = ILI98XX_DSI_INVTR, .cmd_len = 1, .cmd = {0x00}}, /* Column Inversion */
	{.reg = ILI9806E_PWCTRL_1, .cmd_len = 1, .cmd = {0x14}}, /* DDVDH/DDVDL voltage control */
	{.reg = ILI9806E_PWCTRL_2, .cmd_len = 1, .cmd = {0x22}}, /* avdd +5.0V, avee -5.0V*/
	/* VGL=DDVDH+VCIP -DDVDL, VGH=2DDVDL-VCIP */
	{.reg = ILI9806E_PWCTRL_3, .cmd_len = 1, .cmd = {0x02}},
	{.reg = ILI9806E_PWCTRL_4, .cmd_len = 1, .cmd = {0x84}}, /* Set VGH Clamp level = 12.5 V */
	{.reg = ILI9806E_PWCTRL_5, .cmd_len = 1, .cmd = {0x8A}}, /* Set VGL Clamp level = -12.5 V */
	{.reg = ILI98XX_DSI_VREG1,
	 .cmd_len = 1,
	 .cmd = {0x78}}, /* Positive Gamma VREG1OUT = 4.5 V */
	{.reg = ILI98XX_DSI_VREG2,
	 .cmd_len = 1,
	 .cmd = {0x78}}, /* Negative Gamma VREG2OUT = -4.5 V */
	{.reg = ILI98XX_DSI_VCM1_MSB, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI98XX_DSI_VCM1_LSB,
	 .cmd_len = 1,
	 .cmd = {0x2B}}, /* VCOM = -0.7250 V (regs 0x52~0x53)*/
	{.reg = ILI98XX_DSI_VCM2_MSB, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI98XX_DSI_VCM2_LSB, .cmd_len = 1, .cmd = {0x2B}}, /* VCOM voltage factor */
	{.reg = ILI98XX_DSI_SOURCE_TIMING_ADJUST_1, .cmd_len = 1, .cmd = {0x07}},
	{.reg = ILI98XX_DSI_SOURCE_TIMING_ADJUST_2, .cmd_len = 1, .cmd = {0x06}},
	{.reg = ILI98XX_DSI_SOURCE_TIMING_ADJUST_3, .cmd_len = 1, .cmd = {0x06}},
	{.reg = ILI98XX_DSI_SOURCE_TIMING_ADJUST_4, .cmd_len = 1, .cmd = {0x04}},
	/* Positive Gamma Control */
	{.reg = ILI98XX_DSI_PGAMCTRL1, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI98XX_DSI_PGAMCTRL2, .cmd_len = 1, .cmd = {0x0B}},
	{.reg = ILI98XX_DSI_PGAMCTRL3, .cmd_len = 1, .cmd = {0x19}},
	{.reg = ILI98XX_DSI_PGAMCTRL4, .cmd_len = 1, .cmd = {0x10}},
	{.reg = ILI98XX_DSI_PGAMCTRL5, .cmd_len = 1, .cmd = {0x06}},
	{.reg = ILI98XX_DSI_PGAMCTRL6, .cmd_len = 1, .cmd = {0x0F}},
	{.reg = ILI98XX_DSI_PGAMCTRL7, .cmd_len = 1, .cmd = {0x09}},
	{.reg = ILI98XX_DSI_PGAMCTRL8, .cmd_len = 1, .cmd = {0x06}},
	{.reg = ILI98XX_DSI_PGAMCTRL9, .cmd_len = 1, .cmd = {0x0C}},
	{.reg = ILI98XX_DSI_PGAMCTRL10, .cmd_len = 1, .cmd = {0x0E}},
	{.reg = ILI98XX_DSI_PGAMCTRL11, .cmd_len = 1, .cmd = {0x16}},
	{.reg = ILI98XX_DSI_PGAMCTRL12, .cmd_len = 1, .cmd = {0x0D}},
	{.reg = ILI98XX_DSI_PGAMCTRL13, .cmd_len = 1, .cmd = {0x15}},
	{.reg = ILI98XX_DSI_PGAMCTRL14, .cmd_len = 1, .cmd = {0x0F}},
	{.reg = ILI98XX_DSI_PGAMCTRL15, .cmd_len = 1, .cmd = {0x11}},
	{.reg = ILI98XX_DSI_PGAMCTRL16, .cmd_len = 1, .cmd = {0x00}},
	/* Negative Gamma Control */
	{.reg = ILI98XX_DSI_NGAMCTRL1, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI98XX_DSI_NGAMCTRL2, .cmd_len = 1, .cmd = {0x24}},
	{.reg = ILI98XX_DSI_NGAMCTRL3, .cmd_len = 1, .cmd = {0x29}},
	{.reg = ILI98XX_DSI_NGAMCTRL4, .cmd_len = 1, .cmd = {0x0C}},
	{.reg = ILI98XX_DSI_NGAMCTRL5, .cmd_len = 1, .cmd = {0x07}},
	{.reg = ILI98XX_DSI_NGAMCTRL6, .cmd_len = 1, .cmd = {0x03}},
	{.reg = ILI98XX_DSI_NGAMCTRL7, .cmd_len = 1, .cmd = {0x03}},
	{.reg = ILI98XX_DSI_NGAMCTRL8, .cmd_len = 1, .cmd = {0x03}},
	{.reg = ILI98XX_DSI_NGAMCTRL9, .cmd_len = 1, .cmd = {0x03}},
	{.reg = ILI98XX_DSI_NGAMCTRL10, .cmd_len = 1, .cmd = {0x09}},
	{.reg = ILI98XX_DSI_NGAMCTRL11, .cmd_len = 1, .cmd = {0x0D}},
	{.reg = ILI98XX_DSI_NGAMCTRL12, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI98XX_DSI_NGAMCTRL13, .cmd_len = 1, .cmd = {0x06}},
	{.reg = ILI98XX_DSI_NGAMCTRL14, .cmd_len = 1, .cmd = {0x1B}},
	{.reg = ILI98XX_DSI_NGAMCTRL15, .cmd_len = 1, .cmd = {0x08}},
	{.reg = ILI98XX_DSI_NGAMCTRL16, .cmd_len = 1, .cmd = {0x00}},

	/* Change to Page 6 */
	{.reg = ILI98XX_DSI_ENEXTC, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x06}},
	/* GIP (Gate driver in Panel) timing */
	{.reg = ILI9806E_GIPCTRL1_00, .cmd_len = 1, .cmd = {0x20}},
	{.reg = ILI9806E_GIPCTRL1_01, .cmd_len = 1, .cmd = {0x04}},
	{.reg = ILI9806E_GIPCTRL1_02, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_03, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_04, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_05, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_06, .cmd_len = 1, .cmd = {0x88}},
	{.reg = ILI9806E_GIPCTRL1_07, .cmd_len = 1, .cmd = {0x04}},
	{.reg = ILI9806E_GIPCTRL1_08, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_09, .cmd_len = 1, .cmd = {0x90}},
	{.reg = ILI9806E_GIPCTRL1_0A, .cmd_len = 1, .cmd = {0x03}},
	{.reg = ILI9806E_GIPCTRL1_0B, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_0C, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_0D, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_0E, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_0F, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_10, .cmd_len = 1, .cmd = {0x55}},
	{.reg = ILI9806E_GIPCTRL1_11, .cmd_len = 1, .cmd = {0x53}},
	{.reg = ILI9806E_GIPCTRL1_12, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL1_13, .cmd_len = 1, .cmd = {0x0D}},
	{.reg = ILI9806E_GIPCTRL1_14, .cmd_len = 1, .cmd = {0x0D}},
	{.reg = ILI9806E_GIPCTRL1_15, .cmd_len = 1, .cmd = {0x43}},
	{.reg = ILI9806E_GIPCTRL1_16, .cmd_len = 1, .cmd = {0x0B}},
	{.reg = ILI9806E_GIPCTRL1_17, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_18, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_19, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_1A, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_1B, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_1C, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL1_1D, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL2_20, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL2_21, .cmd_len = 1, .cmd = {0x23}},
	{.reg = ILI9806E_GIPCTRL2_22, .cmd_len = 1, .cmd = {0x45}},
	{.reg = ILI9806E_GIPCTRL2_23, .cmd_len = 1, .cmd = {0x67}},
	{.reg = ILI9806E_GIPCTRL2_24, .cmd_len = 1, .cmd = {0x01}},
	{.reg = ILI9806E_GIPCTRL2_25, .cmd_len = 1, .cmd = {0x23}},
	{.reg = ILI9806E_GIPCTRL2_26, .cmd_len = 1, .cmd = {0x45}},
	{.reg = ILI9806E_GIPCTRL2_27, .cmd_len = 1, .cmd = {0x67}},
	{.reg = ILI9806E_GIPCTRL3_30, .cmd_len = 1, .cmd = {0x02}},
	{.reg = ILI9806E_GIPCTRL3_31, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_32, .cmd_len = 1, .cmd = {0x11}},
	{.reg = ILI9806E_GIPCTRL3_33, .cmd_len = 1, .cmd = {0xAA}},
	{.reg = ILI9806E_GIPCTRL3_34, .cmd_len = 1, .cmd = {0xBB}},
	{.reg = ILI9806E_GIPCTRL3_35, .cmd_len = 1, .cmd = {0x66}},
	{.reg = ILI9806E_GIPCTRL3_36, .cmd_len = 1, .cmd = {0x00}},
	{.reg = ILI9806E_GIPCTRL3_37, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_38, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_39, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3A, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3B, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3C, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3D, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3E, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_3F, .cmd_len = 1, .cmd = {0x22}},
	{.reg = ILI9806E_GIPCTRL3_40, .cmd_len = 1, .cmd = {0x22}},

	/* Change to Page 5 */
	{.reg = ILI98XX_DSI_ENEXTC, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x05}},
	/* Backlight Control */
	{.reg = ILI9806E_BLCTRL9, .cmd_len = 1, .cmd = {0xFC}},
	/* Light Adaptive backlight control ON, Enable SRE function, Backlight LED ON */
	{.reg = ILI9806E_BLCTRL8, .cmd_len = 1, .cmd = {0xBC}},

	/* Change to Page 0 */
	{.reg = ILI98XX_DSI_ENEXTC, .cmd_len = 5, .cmd = {0xFF, 0x98, 0x06, 0x04, 0x00}},
	/* Fix Panel RGB-BGR order to RGB and Horizontal/Vertical flip to zero. */
	{.reg = ILI9XXX_MADCTL, .cmd_len = 1, .cmd = {0}},
};

int mw405_dsi_regs_init(const struct device *dev)
{
	return ili98xx_dsi_write_sequence(dev, mw405_init_cmds, ARRAY_SIZE(mw405_init_cmds));
}
