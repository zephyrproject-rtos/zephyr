/*
 * Copyright (C) 2024 Alif Semiconductor.
 * Copyright (C) 2026 Analog Devices, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT focuslcds_mw405

#include <zephyr/logging/log.h>

#include "display_ili9806e_dsi.h"
#include "display_ili98xx_dsi.h"

LOG_MODULE_REGISTER(panel_mw405_dsi, CONFIG_DISPLAY_LOG_LEVEL);

BUILD_ASSERT(CONFIG_APPLICATION_INIT_PRIORITY > CONFIG_MIPI_DSI_INIT_PRIORITY,
	     "MW405 init priority must be greater than MIPI DSI init priority");

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

ILI98XX_DSI_API_DEFINE(ili98xx_dsi_api)

#define MW405_DSI_INIT(inst)                                                                       \
	ILI98XX_DSI_INIT(DT_DRV_INST(inst), mw405_dsi_regs_init, ili98xx_dsi_api)

DT_INST_FOREACH_STATUS_OKAY(MW405_DSI_INIT)
