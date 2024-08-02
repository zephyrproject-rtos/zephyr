/*
 * Copyright (c) 2021-2023 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pinctrl_soc.h>
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-r8a77961.h>

const struct pfc_drive_reg pfc_drive_regs[] = {
	/* DRVCTRL0 */
	{ 0x0300, {
		  { PIN_QSPI0_SPCLK,    28, 2 },        /* QSPI0_SPCLK */
		  { PIN_QSPI0_MOSI_IO0, 24, 2 },        /* QSPI0_MOSI_IO0 */
		  { PIN_QSPI0_MISO_IO1, 20, 2 },        /* QSPI0_MISO_IO1 */
		  { PIN_QSPI0_IO2,      16, 2 },        /* QSPI0_IO2 */
		  { PIN_QSPI0_IO3,      12, 2 },        /* QSPI0_IO3 */
		  { PIN_QSPI0_SSL,       8, 2 },        /* QSPI0_SSL */
		  { PIN_QSPI1_SPCLK,     4, 2 },        /* QSPI1_SPCLK */
		  { PIN_QSPI1_MOSI_IO0,  0, 2 },        /* QSPI1_MOSI_IO0 */
	  } },
	/* DRVCTRL1 */
	{ 0x0304, {
		  { PIN_QSPI1_MISO_IO1, 28, 2 },        /* QSPI1_MISO_IO1 */
		  { PIN_QSPI1_IO2,      24, 2 },        /* QSPI1_IO2 */
		  { PIN_QSPI1_IO3,      20, 2 },        /* QSPI1_IO3 */
		  { PIN_QSPI1_SSL,      16, 2 },        /* QSPI1_SSL */
		  { PIN_RPC_INT_N,      12, 2 },        /* RPC_INT# */
		  { PIN_RPC_WP_N,        8, 2 },        /* RPC_WP# */
		  { PIN_RPC_RESET_N,     4, 2 },        /* RPC_RESET# */
		  { PIN_AVB_RX_CTL,      0, 3 },        /* AVB_RX_CTL */
	  } },
	/* DRVCTRL2 */
	{ 0x0308, {
		  { PIN_AVB_RXC,        28, 3 },        /* AVB_RXC */
		  { PIN_AVB_RD0,        24, 3 },        /* AVB_RD0 */
		  { PIN_AVB_RD1,        20, 3 },        /* AVB_RD1 */
		  { PIN_AVB_RD2,        16, 3 },        /* AVB_RD2 */
		  { PIN_AVB_RD3,        12, 3 },        /* AVB_RD3 */
		  { PIN_AVB_TX_CTL,      8, 3 },        /* AVB_TX_CTL */
		  { PIN_AVB_TXC,         4, 3 },        /* AVB_TXC */
		  { PIN_AVB_TD0,         0, 3 },        /* AVB_TD0 */
	  } },
	/* DRVCTRL3 */
	{  0x030c, {
		  { PIN_AVB_TD1,        28, 3 },        /* AVB_TD1 */
		  { PIN_AVB_TD2,        24, 3 },        /* AVB_TD2 */
		  { PIN_AVB_TD3,        20, 3 },        /* AVB_TD3 */
		  { PIN_AVB_TXCREFCLK,  16, 3 },        /* AVB_TXCREFCLK */
		  { PIN_AVB_MDIO,       12, 3 },        /* AVB_MDIO */
		  { RCAR_GP_PIN(2,  9),  8, 3 },        /* AVB_MDC */
		  { RCAR_GP_PIN(2, 10),  4, 3 },        /* AVB_MAGIC */
		  { RCAR_GP_PIN(2, 11),  0, 3 },        /* AVB_PHY_INT */
	  } },
	/* DRVCTRL4 */
	{ 0x0310, {
		  { RCAR_GP_PIN(2, 12), 28, 3 },        /* AVB_LINK */
		  { RCAR_GP_PIN(2, 13), 24, 3 },        /* AVB_AVTP_MATCH */
		  { RCAR_GP_PIN(2, 14), 20, 3 },        /* AVB_AVTP_CAPTURE */
		  { RCAR_GP_PIN(2,  0), 16, 3 },        /* IRQ0 */
		  { RCAR_GP_PIN(2,  1), 12, 3 },        /* IRQ1 */
		  { RCAR_GP_PIN(2,  2),  8, 3 },        /* IRQ2 */
		  { RCAR_GP_PIN(2,  3),  4, 3 },        /* IRQ3 */
		  { RCAR_GP_PIN(2,  4),  0, 3 },        /* IRQ4 */
	  } },
	/* DRVCTRL5 */
	{ 0x0314, {
		  { RCAR_GP_PIN(2,  5), 28, 3 },        /* IRQ5 */
		  { RCAR_GP_PIN(2,  6), 24, 3 },        /* PWM0 */
		  { RCAR_GP_PIN(2,  7), 20, 3 },        /* PWM1 */
		  { RCAR_GP_PIN(2,  8), 16, 3 },        /* PWM2 */
		  { RCAR_GP_PIN(1,  0), 12, 3 },        /* A0 */
		  { RCAR_GP_PIN(1,  1),  8, 3 },        /* A1 */
		  { RCAR_GP_PIN(1,  2),  4, 3 },        /* A2 */
		  { RCAR_GP_PIN(1,  3),  0, 3 },        /* A3 */
	  } },
	/* DRVCTRL6 */
	{ 0x0318, {
		  { RCAR_GP_PIN(1,  4), 28, 3 },        /* A4 */
		  { RCAR_GP_PIN(1,  5), 24, 3 },        /* A5 */
		  { RCAR_GP_PIN(1,  6), 20, 3 },        /* A6 */
		  { RCAR_GP_PIN(1,  7), 16, 3 },        /* A7 */
		  { RCAR_GP_PIN(1,  8), 12, 3 },        /* A8 */
		  { RCAR_GP_PIN(1,  9),  8, 3 },        /* A9 */
		  { RCAR_GP_PIN(1, 10),  4, 3 },        /* A10 */
		  { RCAR_GP_PIN(1, 11),  0, 3 },        /* A11 */
	  } },
	/* DRVCTRL7 */
	{ 0x031c, {
		  { RCAR_GP_PIN(1, 12), 28, 3 },        /* A12 */
		  { RCAR_GP_PIN(1, 13), 24, 3 },        /* A13 */
		  { RCAR_GP_PIN(1, 14), 20, 3 },        /* A14 */
		  { RCAR_GP_PIN(1, 15), 16, 3 },        /* A15 */
		  { RCAR_GP_PIN(1, 16), 12, 3 },        /* A16 */
		  { RCAR_GP_PIN(1, 17),  8, 3 },        /* A17 */
		  { RCAR_GP_PIN(1, 18),  4, 3 },        /* A18 */
		  { RCAR_GP_PIN(1, 19),  0, 3 },        /* A19 */
	  } },
	/* DRVCTRL8 */
	{ 0x0320, {
		  { RCAR_GP_PIN(1, 28), 28, 3 },        /* CLKOUT */
		  { RCAR_GP_PIN(1, 20), 24, 3 },        /* CS0 */
		  { RCAR_GP_PIN(1, 21), 20, 3 },        /* CS1_A26 */
		  { RCAR_GP_PIN(1, 22), 16, 3 },        /* BS */
		  { RCAR_GP_PIN(1, 23), 12, 3 },        /* RD */
		  { RCAR_GP_PIN(1, 24),  8, 3 },        /* RD_WR */
		  { RCAR_GP_PIN(1, 25),  4, 3 },        /* WE0 */
		  { RCAR_GP_PIN(1, 26),  0, 3 },        /* WE1 */
	  } },
	/* DRVCTRL9 */
	{ 0x0324, {
		  { RCAR_GP_PIN(1, 27), 28, 3 },        /* EX_WAIT0 */
		  { PIN_PRESETOUT_N,    24, 3 },        /* PRESETOUT# */
		  { RCAR_GP_PIN(0,  0), 20, 3 },        /* D0 */
		  { RCAR_GP_PIN(0,  1), 16, 3 },        /* D1 */
		  { RCAR_GP_PIN(0,  2), 12, 3 },        /* D2 */
		  { RCAR_GP_PIN(0,  3),  8, 3 },        /* D3 */
		  { RCAR_GP_PIN(0,  4),  4, 3 },        /* D4 */
		  { RCAR_GP_PIN(0,  5),  0, 3 },        /* D5 */
	  } },
	/* DRVCTRL10 */
	{ 0x0328, {
		  { RCAR_GP_PIN(0,  6), 28, 3 },        /* D6 */
		  { RCAR_GP_PIN(0,  7), 24, 3 },        /* D7 */
		  { RCAR_GP_PIN(0,  8), 20, 3 },        /* D8 */
		  { RCAR_GP_PIN(0,  9), 16, 3 },        /* D9 */
		  { RCAR_GP_PIN(0, 10), 12, 3 },        /* D10 */
		  { RCAR_GP_PIN(0, 11),  8, 3 },        /* D11 */
		  { RCAR_GP_PIN(0, 12),  4, 3 },        /* D12 */
		  { RCAR_GP_PIN(0, 13),  0, 3 },        /* D13 */
	  } },
	/* DRVCTRL11 */
	{ 0x032c, {
		  { RCAR_GP_PIN(0, 14), 28, 3 },        /* D14 */
		  { RCAR_GP_PIN(0, 15), 24, 3 },        /* D15 */
		  { RCAR_GP_PIN(7,  0), 20, 3 },        /* AVS1 */
		  { RCAR_GP_PIN(7,  1), 16, 3 },        /* AVS2 */
		  { RCAR_GP_PIN(7,  2), 12, 3 },        /* GP7_02 */
		  { RCAR_GP_PIN(7,  3),  8, 3 },        /* GP7_03 */
		  { PIN_DU_DOTCLKIN0,    4, 2 },        /* DU_DOTCLKIN0 */
		  { PIN_DU_DOTCLKIN1,    0, 2 },        /* DU_DOTCLKIN1 */
	  } },
	/* DRVCTRL12 */
	{ 0x0330, {
		  { PIN_DU_DOTCLKIN2,   28, 2 },        /* DU_DOTCLKIN2 */
		  { PIN_DU_DOTCLKIN3,   24, 2 },        /* DU_DOTCLKIN3 */
		  { PIN_FSCLKST_N,      20, 2 },        /* FSCLKST# */
		  { PIN_TMS,             4, 2 },        /* TMS */
	  } },
	/* DRVCTRL13 */
	{ 0x0334, {
		  { PIN_TDO,            28, 2 },        /* TDO */
		  { PIN_ASEBRK,         24, 2 },        /* ASEBRK */
		  { RCAR_GP_PIN(3,  0), 20, 3 },        /* SD0_CLK */
		  { RCAR_GP_PIN(3,  1), 16, 3 },        /* SD0_CMD */
		  { RCAR_GP_PIN(3,  2), 12, 3 },        /* SD0_DAT0 */
		  { RCAR_GP_PIN(3,  3),  8, 3 },        /* SD0_DAT1 */
		  { RCAR_GP_PIN(3,  4),  4, 3 },        /* SD0_DAT2 */
		  { RCAR_GP_PIN(3,  5),  0, 3 },        /* SD0_DAT3 */
	  } },
	/* DRVCTRL14 */
	{ 0x0338, {
		  { RCAR_GP_PIN(3,  6), 28, 3 },        /* SD1_CLK */
		  { RCAR_GP_PIN(3,  7), 24, 3 },        /* SD1_CMD */
		  { RCAR_GP_PIN(3,  8), 20, 3 },        /* SD1_DAT0 */
		  { RCAR_GP_PIN(3,  9), 16, 3 },        /* SD1_DAT1 */
		  { RCAR_GP_PIN(3, 10), 12, 3 },        /* SD1_DAT2 */
		  { RCAR_GP_PIN(3, 11),  8, 3 },        /* SD1_DAT3 */
		  { RCAR_GP_PIN(4,  0),  4, 3 },        /* SD2_CLK */
		  { RCAR_GP_PIN(4,  1),  0, 3 },        /* SD2_CMD */
	  } },
	/* DRVCTRL15 */
	{ 0x033c, {
		  { RCAR_GP_PIN(4,  2), 28, 3 },        /* SD2_DAT0 */
		  { RCAR_GP_PIN(4,  3), 24, 3 },        /* SD2_DAT1 */
		  { RCAR_GP_PIN(4,  4), 20, 3 },        /* SD2_DAT2 */
		  { RCAR_GP_PIN(4,  5), 16, 3 },        /* SD2_DAT3 */
		  { RCAR_GP_PIN(4,  6), 12, 3 },        /* SD2_DS */
		  { RCAR_GP_PIN(4,  7),  8, 3 },        /* SD3_CLK */
		  { RCAR_GP_PIN(4,  8),  4, 3 },        /* SD3_CMD */
		  { RCAR_GP_PIN(4,  9),  0, 3 },        /* SD3_DAT0 */
	  } },
	/* DRVCTRL16 */
	{ 0x0340, {
		  { RCAR_GP_PIN(4, 10), 28, 3 },        /* SD3_DAT1 */
		  { RCAR_GP_PIN(4, 11), 24, 3 },        /* SD3_DAT2 */
		  { RCAR_GP_PIN(4, 12), 20, 3 },        /* SD3_DAT3 */
		  { RCAR_GP_PIN(4, 13), 16, 3 },        /* SD3_DAT4 */
		  { RCAR_GP_PIN(4, 14), 12, 3 },        /* SD3_DAT5 */
		  { RCAR_GP_PIN(4, 15),  8, 3 },        /* SD3_DAT6 */
		  { RCAR_GP_PIN(4, 16),  4, 3 },        /* SD3_DAT7 */
		  { RCAR_GP_PIN(4, 17),  0, 3 },        /* SD3_DS */
	  } },
	/* DRVCTRL17 */
	{ 0x0344, {
		  { RCAR_GP_PIN(3, 12), 28, 3 },        /* SD0_CD */
		  { RCAR_GP_PIN(3, 13), 24, 3 },        /* SD0_WP */
		  { RCAR_GP_PIN(3, 14), 20, 3 },        /* SD1_CD */
		  { RCAR_GP_PIN(3, 15), 16, 3 },        /* SD1_WP */
		  { RCAR_GP_PIN(5,  0), 12, 3 },        /* SCK0 */
		  { RCAR_GP_PIN(5,  1),  8, 3 },        /* RX0 */
		  { RCAR_GP_PIN(5,  2),  4, 3 },        /* TX0 */
		  { RCAR_GP_PIN(5,  3),  0, 3 },        /* CTS0 */
	  } },
	/* DRVCTRL18 */
	{ 0x0348, {
		  { RCAR_GP_PIN(5,  4), 28, 3 },        /* RTS0 */
		  { RCAR_GP_PIN(5,  5), 24, 3 },        /* RX1 */
		  { RCAR_GP_PIN(5,  6), 20, 3 },        /* TX1 */
		  { RCAR_GP_PIN(5,  7), 16, 3 },        /* CTS1 */
		  { RCAR_GP_PIN(5,  8), 12, 3 },        /* RTS1 */
		  { RCAR_GP_PIN(5,  9),  8, 3 },        /* SCK2 */
		  { RCAR_GP_PIN(5, 10),  4, 3 },        /* TX2 */
		  { RCAR_GP_PIN(5, 11),  0, 3 },        /* RX2 */
	  } },
	/* DRVCTRL19 */
	{ 0x034c, {
		  { RCAR_GP_PIN(5, 12), 28, 3 },        /* HSCK0 */
		  { RCAR_GP_PIN(5, 13), 24, 3 },        /* HRX0 */
		  { RCAR_GP_PIN(5, 14), 20, 3 },        /* HTX0 */
		  { RCAR_GP_PIN(5, 15), 16, 3 },        /* HCTS0 */
		  { RCAR_GP_PIN(5, 16), 12, 3 },        /* HRTS0 */
		  { RCAR_GP_PIN(5, 17),  8, 3 },        /* MSIOF0_SCK */
		  { RCAR_GP_PIN(5, 18),  4, 3 },        /* MSIOF0_SYNC */
		  { RCAR_GP_PIN(5, 19),  0, 3 },        /* MSIOF0_SS1 */
	  } },
	/* DRVCTRL20 */
	{ 0x0350, {
		  { RCAR_GP_PIN(5, 20), 28, 3 },        /* MSIOF0_TXD */
		  { RCAR_GP_PIN(5, 21), 24, 3 },        /* MSIOF0_SS2 */
		  { RCAR_GP_PIN(5, 22), 20, 3 },        /* MSIOF0_RXD */
		  { RCAR_GP_PIN(5, 23), 16, 3 },        /* MLB_CLK */
		  { RCAR_GP_PIN(5, 24), 12, 3 },        /* MLB_SIG */
		  { RCAR_GP_PIN(5, 25),  8, 3 },        /* MLB_DAT */
		  { PIN_MLB_REF,         4, 3 },        /* MLB_REF */
		  { RCAR_GP_PIN(6,  0),  0, 3 },        /* SSI_SCK01239 */
	  } },
	/* DRVCTRL21 */
	{ 0x0354, {
		  { RCAR_GP_PIN(6,  1), 28, 3 },        /* SSI_WS01239 */
		  { RCAR_GP_PIN(6,  2), 24, 3 },        /* SSI_SDATA0 */
		  { RCAR_GP_PIN(6,  3), 20, 3 },        /* SSI_SDATA1 */
		  { RCAR_GP_PIN(6,  4), 16, 3 },        /* SSI_SDATA2 */
		  { RCAR_GP_PIN(6,  5), 12, 3 },        /* SSI_SCK349 */
		  { RCAR_GP_PIN(6,  6),  8, 3 },        /* SSI_WS349 */
		  { RCAR_GP_PIN(6,  7),  4, 3 },        /* SSI_SDATA3 */
		  { RCAR_GP_PIN(6,  8),  0, 3 },        /* SSI_SCK4 */
	  } },
	/* DRVCTRL22 */
	{ 0x0358, {
		  { RCAR_GP_PIN(6,  9), 28, 3 },        /* SSI_WS4 */
		  { RCAR_GP_PIN(6, 10), 24, 3 },        /* SSI_SDATA4 */
		  { RCAR_GP_PIN(6, 11), 20, 3 },        /* SSI_SCK5 */
		  { RCAR_GP_PIN(6, 12), 16, 3 },        /* SSI_WS5 */
		  { RCAR_GP_PIN(6, 13), 12, 3 },        /* SSI_SDATA5 */
		  { RCAR_GP_PIN(6, 14),  8, 3 },        /* SSI_SCK6 */
		  { RCAR_GP_PIN(6, 15),  4, 3 },        /* SSI_WS6 */
		  { RCAR_GP_PIN(6, 16),  0, 3 },        /* SSI_SDATA6 */
	  } },
	/* DRVCTRL23 */
	{ 0x035c, {
		  { RCAR_GP_PIN(6, 17), 28, 3 },        /* SSI_SCK78 */
		  { RCAR_GP_PIN(6, 18), 24, 3 },        /* SSI_WS78 */
		  { RCAR_GP_PIN(6, 19), 20, 3 },        /* SSI_SDATA7 */
		  { RCAR_GP_PIN(6, 20), 16, 3 },        /* SSI_SDATA8 */
		  { RCAR_GP_PIN(6, 21), 12, 3 },        /* SSI_SDATA9 */
		  { RCAR_GP_PIN(6, 22),  8, 3 },        /* AUDIO_CLKA */
		  { RCAR_GP_PIN(6, 23),  4, 3 },        /* AUDIO_CLKB */
		  { RCAR_GP_PIN(6, 24),  0, 3 },        /* USB0_PWEN */
	  } },
	/* DRVCTRL24 */
	{ 0x0360, {
		  { RCAR_GP_PIN(6, 25), 28, 3 },        /* USB0_OVC */
		  { RCAR_GP_PIN(6, 26), 24, 3 },        /* USB1_PWEN */
		  { RCAR_GP_PIN(6, 27), 20, 3 },        /* USB1_OVC */
		  { RCAR_GP_PIN(6, 28), 16, 3 },        /* USB30_PWEN */
		  { RCAR_GP_PIN(6, 29), 12, 3 },        /* USB30_OVC */
		  { RCAR_GP_PIN(6, 30),  8, 3 },        /* GP6_30/USB2_CH3_PWEN */
		  { RCAR_GP_PIN(6, 31),  4, 3 },        /* GP6_31/USB2_CH3_OVC */
	  } },
	{ },
};

#define PFC_BIAS_REG(r1, r2) \
	.puen = r1,	     \
	.pud = r2,	     \
	.pins =

const struct pfc_bias_reg pfc_bias_regs[] = {
	{ PFC_BIAS_REG(0x0400, 0x0440) {        /* PUEN0, PUD0 */
		  [0]  = PIN_QSPI0_SPCLK,       /* QSPI0_SPCLK */
		  [1]  = PIN_QSPI0_MOSI_IO0,    /* QSPI0_MOSI_IO0 */
		  [2]  = PIN_QSPI0_MISO_IO1,    /* QSPI0_MISO_IO1 */
		  [3]  = PIN_QSPI0_IO2,	        /* QSPI0_IO2 */
		  [4]  = PIN_QSPI0_IO3,	        /* QSPI0_IO3 */
		  [5]  = PIN_QSPI0_SSL,	        /* QSPI0_SSL */
		  [6]  = PIN_QSPI1_SPCLK,       /* QSPI1_SPCLK */
		  [7]  = PIN_QSPI1_MOSI_IO0,    /* QSPI1_MOSI_IO0 */
		  [8]  = PIN_QSPI1_MISO_IO1,    /* QSPI1_MISO_IO1 */
		  [9]  = PIN_QSPI1_IO2,	        /* QSPI1_IO2 */
		  [10] = PIN_QSPI1_IO3,         /* QSPI1_IO3 */
		  [11] = PIN_QSPI1_SSL,         /* QSPI1_SSL */
		  [12] = PIN_RPC_INT_N,         /* RPC_INT# */
		  [13] = PIN_RPC_WP_N,          /* RPC_WP# */
		  [14] = PIN_RPC_RESET_N,       /* RPC_RESET# */
		  [15] = PIN_AVB_RX_CTL,        /* AVB_RX_CTL */
		  [16] = PIN_AVB_RXC,           /* AVB_RXC */
		  [17] = PIN_AVB_RD0,           /* AVB_RD0 */
		  [18] = PIN_AVB_RD1,           /* AVB_RD1 */
		  [19] = PIN_AVB_RD2,           /* AVB_RD2 */
		  [20] = PIN_AVB_RD3,           /* AVB_RD3 */
		  [21] = PIN_AVB_TX_CTL,        /* AVB_TX_CTL */
		  [22] = PIN_AVB_TXC,           /* AVB_TXC */
		  [23] = PIN_AVB_TD0,           /* AVB_TD0 */
		  [24] = PIN_AVB_TD1,           /* AVB_TD1 */
		  [25] = PIN_AVB_TD2,           /* AVB_TD2 */
		  [26] = PIN_AVB_TD3,           /* AVB_TD3 */
		  [27] = PIN_AVB_TXCREFCLK,     /* AVB_TXCREFCLK */
		  [28] = PIN_AVB_MDIO,          /* AVB_MDIO */
		  [29] = RCAR_GP_PIN(2,  9),    /* AVB_MDC */
		  [30] = RCAR_GP_PIN(2, 10),    /* AVB_MAGIC */
		  [31] = RCAR_GP_PIN(2, 11),    /* AVB_PHY_INT */
	  } },
	{ PFC_BIAS_REG(0x0404, 0x0444) {        /* PUEN1, PUD1 */
		  [0]  = RCAR_GP_PIN(2, 12),    /* AVB_LINK */
		  [1]  = RCAR_GP_PIN(2, 13),    /* AVB_AVTP_MATCH_A */
		  [2]  = RCAR_GP_PIN(2, 14),    /* AVB_AVTP_CAPTURE_A */
		  [3]  = RCAR_GP_PIN(2,	0),     /* IRQ0 */
		  [4]  = RCAR_GP_PIN(2,	1),     /* IRQ1 */
		  [5]  = RCAR_GP_PIN(2,	2),     /* IRQ2 */
		  [6]  = RCAR_GP_PIN(2,	3),     /* IRQ3 */
		  [7]  = RCAR_GP_PIN(2,	4),     /* IRQ4 */
		  [8]  = RCAR_GP_PIN(2,	5),     /* IRQ5 */
		  [9]  = RCAR_GP_PIN(2,	6),     /* PWM0 */
		  [10] = RCAR_GP_PIN(2,  7),    /* PWM1_A */
		  [11] = RCAR_GP_PIN(2,  8),    /* PWM2_A */
		  [12] = RCAR_GP_PIN(1,  0),    /* A0 */
		  [13] = RCAR_GP_PIN(1,  1),    /* A1 */
		  [14] = RCAR_GP_PIN(1,  2),    /* A2 */
		  [15] = RCAR_GP_PIN(1,  3),    /* A3 */
		  [16] = RCAR_GP_PIN(1,  4),    /* A4 */
		  [17] = RCAR_GP_PIN(1,  5),    /* A5 */
		  [18] = RCAR_GP_PIN(1,  6),    /* A6 */
		  [19] = RCAR_GP_PIN(1,  7),    /* A7 */
		  [20] = RCAR_GP_PIN(1,  8),    /* A8 */
		  [21] = RCAR_GP_PIN(1,  9),    /* A9 */
		  [22] = RCAR_GP_PIN(1, 10),    /* A10 */
		  [23] = RCAR_GP_PIN(1, 11),    /* A11 */
		  [24] = RCAR_GP_PIN(1, 12),    /* A12 */
		  [25] = RCAR_GP_PIN(1, 13),    /* A13 */
		  [26] = RCAR_GP_PIN(1, 14),    /* A14 */
		  [27] = RCAR_GP_PIN(1, 15),    /* A15 */
		  [28] = RCAR_GP_PIN(1, 16),    /* A16 */
		  [29] = RCAR_GP_PIN(1, 17),    /* A17 */
		  [30] = RCAR_GP_PIN(1, 18),    /* A18 */
		  [31] = RCAR_GP_PIN(1, 19),    /* A19 */
	  } },
	{ PFC_BIAS_REG(0x0408, 0x0448) {        /* PUEN2, PUD2 */
		  [0]  = RCAR_GP_PIN(1, 28),    /* CLKOUT */
		  [1]  = RCAR_GP_PIN(1, 20),    /* CS0_N */
		  [2]  = RCAR_GP_PIN(1, 21),    /* CS1_N */
		  [3]  = RCAR_GP_PIN(1, 22),    /* BS_N */
		  [4]  = RCAR_GP_PIN(1, 23),    /* RD_N */
		  [5]  = RCAR_GP_PIN(1, 24),    /* RD_WR_N */
		  [6]  = RCAR_GP_PIN(1, 25),    /* WE0_N */
		  [7]  = RCAR_GP_PIN(1, 26),    /* WE1_N */
		  [8]  = RCAR_GP_PIN(1, 27),    /* EX_WAIT0_A */
		  [9]  = PIN_PRESETOUT_N,       /* PRESETOUT# */
		  [10] = RCAR_GP_PIN(0,  0),    /* D0 */
		  [11] = RCAR_GP_PIN(0,  1),    /* D1 */
		  [12] = RCAR_GP_PIN(0,  2),    /* D2 */
		  [13] = RCAR_GP_PIN(0,  3),    /* D3 */
		  [14] = RCAR_GP_PIN(0,  4),    /* D4 */
		  [15] = RCAR_GP_PIN(0,  5),    /* D5 */
		  [16] = RCAR_GP_PIN(0,  6),    /* D6 */
		  [17] = RCAR_GP_PIN(0,  7),    /* D7 */
		  [18] = RCAR_GP_PIN(0,  8),    /* D8 */
		  [19] = RCAR_GP_PIN(0,  9),    /* D9 */
		  [20] = RCAR_GP_PIN(0, 10),    /* D10 */
		  [21] = RCAR_GP_PIN(0, 11),    /* D11 */
		  [22] = RCAR_GP_PIN(0, 12),    /* D12 */
		  [23] = RCAR_GP_PIN(0, 13),    /* D13 */
		  [24] = RCAR_GP_PIN(0, 14),    /* D14 */
		  [25] = RCAR_GP_PIN(0, 15),    /* D15 */
		  [26] = RCAR_GP_PIN(7,  0),    /* AVS1 */
		  [27] = RCAR_GP_PIN(7,  1),    /* AVS2 */
		  [28] = RCAR_GP_PIN(7,  2),    /* GP7_02 */
		  [29] = RCAR_GP_PIN(7,  3),    /* GP7_03 */
		  [30] = PIN_DU_DOTCLKIN0,      /* DU_DOTCLKIN0 */
		  [31] = PIN_DU_DOTCLKIN1,      /* DU_DOTCLKIN1 */
	  } },
	{ PFC_BIAS_REG(0x040c, 0x044c) {        /* PUEN3, PUD3 */
		  [0]  = PIN_DU_DOTCLKIN2,      /* DU_DOTCLKIN2 */
		  [1]  = PIN_DU_DOTCLKIN3,      /* DU_DOTCLKIN3 */
		  [2]  = PIN_FSCLKST_N,	        /* FSCLKST# */
		  [3]  = PIN_EXTALR,	        /* EXTALR*/
		  [4]  = PIN_TRST_N,	        /* TRST# */
		  [5]  = PIN_TCK,	        /* TCK */
		  [6]  = PIN_TMS,	        /* TMS */
		  [7]  = PIN_TDI,	        /* TDI */
		  [8]  = PIN_NONE,
		  [9]  = PIN_ASEBRK,	        /* ASEBRK */
		  [10] = RCAR_GP_PIN(3,  0),    /* SD0_CLK */
		  [11] = RCAR_GP_PIN(3,  1),    /* SD0_CMD */
		  [12] = RCAR_GP_PIN(3,  2),    /* SD0_DAT0 */
		  [13] = RCAR_GP_PIN(3,  3),    /* SD0_DAT1 */
		  [14] = RCAR_GP_PIN(3,  4),    /* SD0_DAT2 */
		  [15] = RCAR_GP_PIN(3,  5),    /* SD0_DAT3 */
		  [16] = RCAR_GP_PIN(3,  6),    /* SD1_CLK */
		  [17] = RCAR_GP_PIN(3,  7),    /* SD1_CMD */
		  [18] = RCAR_GP_PIN(3,  8),    /* SD1_DAT0 */
		  [19] = RCAR_GP_PIN(3,  9),    /* SD1_DAT1 */
		  [20] = RCAR_GP_PIN(3, 10),    /* SD1_DAT2 */
		  [21] = RCAR_GP_PIN(3, 11),    /* SD1_DAT3 */
		  [22] = RCAR_GP_PIN(4,  0),    /* SD2_CLK */
		  [23] = RCAR_GP_PIN(4,  1),    /* SD2_CMD */
		  [24] = RCAR_GP_PIN(4,  2),    /* SD2_DAT0 */
		  [25] = RCAR_GP_PIN(4,  3),    /* SD2_DAT1 */
		  [26] = RCAR_GP_PIN(4,  4),    /* SD2_DAT2 */
		  [27] = RCAR_GP_PIN(4,  5),    /* SD2_DAT3 */
		  [28] = RCAR_GP_PIN(4,  6),    /* SD2_DS */
		  [29] = RCAR_GP_PIN(4,  7),    /* SD3_CLK */
		  [30] = RCAR_GP_PIN(4,  8),    /* SD3_CMD */
		  [31] = RCAR_GP_PIN(4,  9),    /* SD3_DAT0 */
	  } },
	{ PFC_BIAS_REG(0x0410, 0x0450) {        /* PUEN4, PUD4 */
		  [0]  = RCAR_GP_PIN(4, 10),    /* SD3_DAT1 */
		  [1]  = RCAR_GP_PIN(4, 11),    /* SD3_DAT2 */
		  [2]  = RCAR_GP_PIN(4, 12),    /* SD3_DAT3 */
		  [3]  = RCAR_GP_PIN(4, 13),    /* SD3_DAT4 */
		  [4]  = RCAR_GP_PIN(4, 14),    /* SD3_DAT5 */
		  [5]  = RCAR_GP_PIN(4, 15),    /* SD3_DAT6 */
		  [6]  = RCAR_GP_PIN(4, 16),    /* SD3_DAT7 */
		  [7]  = RCAR_GP_PIN(4, 17),    /* SD3_DS */
		  [8]  = RCAR_GP_PIN(3, 12),    /* SD0_CD */
		  [9]  = RCAR_GP_PIN(3, 13),    /* SD0_WP */
		  [10] = RCAR_GP_PIN(3, 14),    /* SD1_CD */
		  [11] = RCAR_GP_PIN(3, 15),    /* SD1_WP */
		  [12] = RCAR_GP_PIN(5,  0),    /* SCK0 */
		  [13] = RCAR_GP_PIN(5,  1),    /* RX0 */
		  [14] = RCAR_GP_PIN(5,  2),    /* TX0 */
		  [15] = RCAR_GP_PIN(5,  3),    /* CTS0_N */
		  [16] = RCAR_GP_PIN(5,  4),    /* RTS0_N */
		  [17] = RCAR_GP_PIN(5,  5),    /* RX1_A */
		  [18] = RCAR_GP_PIN(5,  6),    /* TX1_A */
		  [19] = RCAR_GP_PIN(5,  7),    /* CTS1_N */
		  [20] = RCAR_GP_PIN(5,  8),    /* RTS1_N */
		  [21] = RCAR_GP_PIN(5,  9),    /* SCK2 */
		  [22] = RCAR_GP_PIN(5, 10),    /* TX2_A */
		  [23] = RCAR_GP_PIN(5, 11),    /* RX2_A */
		  [24] = RCAR_GP_PIN(5, 12),    /* HSCK0 */
		  [25] = RCAR_GP_PIN(5, 13),    /* HRX0 */
		  [26] = RCAR_GP_PIN(5, 14),    /* HTX0 */
		  [27] = RCAR_GP_PIN(5, 15),    /* HCTS0_N */
		  [28] = RCAR_GP_PIN(5, 16),    /* HRTS0_N */
		  [29] = RCAR_GP_PIN(5, 17),    /* MSIOF0_SCK */
		  [30] = RCAR_GP_PIN(5, 18),    /* MSIOF0_SYNC */
		  [31] = RCAR_GP_PIN(5, 19),    /* MSIOF0_SS1 */
	  } },
	{ PFC_BIAS_REG(0x0414, 0x0454) {        /* PUEN5, PUD5 */
		  [0]  = RCAR_GP_PIN(5, 20),    /* MSIOF0_TXD */
		  [1]  = RCAR_GP_PIN(5, 21),    /* MSIOF0_SS2 */
		  [2]  = RCAR_GP_PIN(5, 22),    /* MSIOF0_RXD */
		  [3]  = RCAR_GP_PIN(5, 23),    /* MLB_CLK */
		  [4]  = RCAR_GP_PIN(5, 24),    /* MLB_SIG */
		  [5]  = RCAR_GP_PIN(5, 25),    /* MLB_DAT */
		  [6]  = PIN_MLB_REF,	        /* MLB_REF */
		  [7]  = RCAR_GP_PIN(6,	0),     /* SSI_SCK01239 */
		  [8]  = RCAR_GP_PIN(6,	1),     /* SSI_WS01239 */
		  [9]  = RCAR_GP_PIN(6,	2),     /* SSI_SDATA0 */
		  [10] = RCAR_GP_PIN(6,  3),    /* SSI_SDATA1_A */
		  [11] = RCAR_GP_PIN(6,  4),    /* SSI_SDATA2_A */
		  [12] = RCAR_GP_PIN(6,  5),    /* SSI_SCK349 */
		  [13] = RCAR_GP_PIN(6,  6),    /* SSI_WS349 */
		  [14] = RCAR_GP_PIN(6,  7),    /* SSI_SDATA3 */
		  [15] = RCAR_GP_PIN(6,  8),    /* SSI_SCK4 */
		  [16] = RCAR_GP_PIN(6,  9),    /* SSI_WS4 */
		  [17] = RCAR_GP_PIN(6, 10),    /* SSI_SDATA4 */
		  [18] = RCAR_GP_PIN(6, 11),    /* SSI_SCK5 */
		  [19] = RCAR_GP_PIN(6, 12),    /* SSI_WS5 */
		  [20] = RCAR_GP_PIN(6, 13),    /* SSI_SDATA5 */
		  [21] = RCAR_GP_PIN(6, 14),    /* SSI_SCK6 */
		  [22] = RCAR_GP_PIN(6, 15),    /* SSI_WS6 */
		  [23] = RCAR_GP_PIN(6, 16),    /* SSI_SDATA6 */
		  [24] = RCAR_GP_PIN(6, 17),    /* SSI_SCK78 */
		  [25] = RCAR_GP_PIN(6, 18),    /* SSI_WS78 */
		  [26] = RCAR_GP_PIN(6, 19),    /* SSI_SDATA7 */
		  [27] = RCAR_GP_PIN(6, 20),    /* SSI_SDATA8 */
		  [28] = RCAR_GP_PIN(6, 21),    /* SSI_SDATA9_A */
		  [29] = RCAR_GP_PIN(6, 22),    /* AUDIO_CLKA_A */
		  [30] = RCAR_GP_PIN(6, 23),    /* AUDIO_CLKB_B */
		  [31] = RCAR_GP_PIN(6, 24),    /* USB0_PWEN */
	  } },
	{ PFC_BIAS_REG(0x0418, 0x0458) {        /* PUEN6, PUD6 */
		  [0]  = RCAR_GP_PIN(6, 25),    /* USB0_OVC */
		  [1]  = RCAR_GP_PIN(6, 26),    /* USB1_PWEN */
		  [2]  = RCAR_GP_PIN(6, 27),    /* USB1_OVC */
		  [3]  = RCAR_GP_PIN(6, 28),    /* USB30_PWEN */
		  [4]  = RCAR_GP_PIN(6, 29),    /* USB30_OVC */
		  [5]  = RCAR_GP_PIN(6, 30),    /* USB2_CH3_PWEN */
		  [6]  = RCAR_GP_PIN(6, 31),    /* USB2_CH3_OVC */
		  [7]  = PIN_NONE,
		  [8]  = PIN_NONE,
		  [9]  = PIN_NONE,
		  [10] = PIN_NONE,
		  [11] = PIN_NONE,
		  [12] = PIN_NONE,
		  [13] = PIN_NONE,
		  [14] = PIN_NONE,
		  [15] = PIN_NONE,
		  [16] = PIN_NONE,
		  [17] = PIN_NONE,
		  [18] = PIN_NONE,
		  [19] = PIN_NONE,
		  [20] = PIN_NONE,
		  [21] = PIN_NONE,
		  [22] = PIN_NONE,
		  [23] = PIN_NONE,
		  [24] = PIN_NONE,
		  [25] = PIN_NONE,
		  [26] = PIN_NONE,
		  [27] = PIN_NONE,
		  [28] = PIN_NONE,
		  [29] = PIN_NONE,
		  [30] = PIN_NONE,
		  [31] = PIN_NONE,
	  } },
	{ /* sentinel */ },
};

const struct pfc_bias_reg *pfc_rcar_get_bias_regs(void)
{
	return pfc_bias_regs;
}
const struct pfc_drive_reg *pfc_rcar_get_drive_regs(void)
{
	return pfc_drive_regs;
}

int pfc_rcar_get_reg_index(uint8_t pin, uint8_t *reg_index)
{
	/* There is only one register on Gen 3 */
	*reg_index = 0;
	return 0;
}
