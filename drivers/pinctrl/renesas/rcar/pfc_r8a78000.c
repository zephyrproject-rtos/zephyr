/*
 * Copyright (c) 2026 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pinctrl_soc.h>
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-r8a78000.h>

#define PFC_DRIVE_REG(r1, r2, r3)	\
	.drvctrl0 = r1,			\
	.drvctrl1 = r2,			\
	.drvctrl2 = r3,			\
	.pins =

const struct pfc_drive_reg pfc_drive_regs[] = {
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP0_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(0,  0),	/* GP0_00 */
		[1]  = RCAR_GP_PIN(0,  1),	/* GP0_01 */
		[2]  = RCAR_GP_PIN(0,  2),	/* GP0_02 */
		[3]  = RCAR_GP_PIN(0,  3),	/* STPWT_EXTFXR_A */
		[4]  = RCAR_GP_PIN(0,  4),	/* FXR_CLKOUT1_A */
		[5]  = RCAR_GP_PIN(0,  5),	/* FXR_CLKOUT2_A */
		[6]  = RCAR_GP_PIN(0,  6),	/* CLK_EXTFXR_A */
		[7]  = RCAR_GP_PIN(0,  7),	/* FXR_TXDA_A */
		[8]  = RCAR_GP_PIN(0,  8),	/* FXR_TXENA_N_A */
		[9]  = RCAR_GP_PIN(0,  9),	/* RXDA_EXTFXR_A */
		[10] = RCAR_GP_PIN(0, 10),	/* FXR_TXDB_A */
		[11] = RCAR_GP_PIN(0, 11),	/* FXR_TXENB_N_A */
		[12] = RCAR_GP_PIN(0, 12),	/* RXDB_EXTFXR_A */
		[13] = RCAR_GP_PIN(0, 13),	/* MSIOF0_SCK */
		[14] = RCAR_GP_PIN(0, 14),	/* MSIOF0_TXD */
		[15] = RCAR_GP_PIN(0, 15),	/* MSIOF0_RXD */
		[16] = RCAR_GP_PIN(0, 16),	/* MSIOF0_SYNC */
		[17] = RCAR_GP_PIN(0, 17),	/* MSIOF0_SS1 */
		[18] = RCAR_GP_PIN(0, 18),	/* MSIOF0_SS2 */
		[19] = RCAR_GP_PIN(0, 19),	/* MSIOF1_SCK_A */
		[20] = RCAR_GP_PIN(0, 20),	/* MSIOF1_TXD_A */
		[21] = RCAR_GP_PIN(0, 21),	/* MSIOF1_RXD_A */
		[22] = RCAR_GP_PIN(0, 22),	/* MSIOF1_SYNC_A */
		[23] = RCAR_GP_PIN(0, 23),	/* MSIOF1_SS1_A */
		[24] = RCAR_GP_PIN(0, 24),	/* MSIOF1_SS2_A */
		[25] = RCAR_GP_PIN(0, 25),	/* DP0_HOTPLUG */
		[26] = RCAR_GP_PIN(0, 26),	/* DP1_HOTPLUG */
		[27] = RCAR_GP_PIN(0, 27),	/* DP2_HOTPLUG */
		[28] = PIN_NONE,
		[29] = PIN_NONE,
		[30] = PIN_NONE,
		[31] = PIN_NONE,
	} },
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP1_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(1,  0),	/* CAN0RX_INTP0 */
		[1]  = RCAR_GP_PIN(1,  1),	/* CAN0TX */
		[2]  = RCAR_GP_PIN(1,  2),	/* CAN1RX_INTP1 */
		[3]  = RCAR_GP_PIN(1,  3),	/* CAN1TX */
		[4]  = RCAR_GP_PIN(1,  4),	/* CAN2RX_INTP2 */
		[5]  = RCAR_GP_PIN(1,  5),	/* CAN2TX */
		[6]  = RCAR_GP_PIN(1,  6),	/* CAN3RX_INTP3 */
		[7]  = RCAR_GP_PIN(1,  7),	/* CAN3TX */
		[8]  = RCAR_GP_PIN(1,  8),	/* CAN4RX_INTP4 */
		[9]  = RCAR_GP_PIN(1,  9),	/* CAN4TX */
		[10] = RCAR_GP_PIN(1, 10),	/* CAN5RX_INTP5 */
		[11] = RCAR_GP_PIN(1, 11),	/* CAN5TX */
		[12] = RCAR_GP_PIN(1, 12),	/* CAN6RX_INTP6 */
		[13] = RCAR_GP_PIN(1, 13),	/* CAN6TX */
		[14] = RCAR_GP_PIN(1, 14),	/* RLIN30RX_INTP16 */
		[15] = RCAR_GP_PIN(1, 15),	/* RLIN30TX */
		[16] = RCAR_GP_PIN(1, 16),	/* RLIN31RX_INTP17 */
		[17] = RCAR_GP_PIN(1, 17),	/* RLIN31TX */
		[18] = RCAR_GP_PIN(1, 18),	/* RLIN32RX_INTP18 */
		[19] = RCAR_GP_PIN(1, 19),	/* RLIN32TX */
		[20] = RCAR_GP_PIN(1, 20),	/* RLIN33RX_INTP19 */
		[21] = RCAR_GP_PIN(1, 21),	/* RLIN33TX */
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
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP2_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(2,  0),	/* RLIN34RX_INTP20_B */
		[1]  = RCAR_GP_PIN(2,  1),	/* RLIN34TX_B */
		[2]  = RCAR_GP_PIN(2,  2),	/* RLIN35RX_INTP21_B */
		[3]  = RCAR_GP_PIN(2,  3),	/* RLIN35TX_B */
		[4]  = RCAR_GP_PIN(2,  4),	/* RLIN36RX_INTP22_B */
		[5]  = RCAR_GP_PIN(2,  5),	/* RLIN36TX_B */
		[6]  = RCAR_GP_PIN(2,  6),	/* RLIN37RX_INTP23_B */
		[7]  = RCAR_GP_PIN(2,  7),	/* RLIN37TX_B */
		[8]  = RCAR_GP_PIN(2,  8),	/* CAN12RX_INTP12_B */
		[9]  = RCAR_GP_PIN(2,  9),	/* CAN12TX_B */
		[10] = RCAR_GP_PIN(2, 10),	/* CAN13RX_INTP13_B */
		[11] = RCAR_GP_PIN(2, 11),	/* CAN13TX_B */
		[12] = RCAR_GP_PIN(2, 12),	/* CAN14RX_INTP14_B */
		[13] = RCAR_GP_PIN(2, 13),	/* CAN14TX_B */
		[14] = RCAR_GP_PIN(2, 14),	/* CAN15RX_INTP15_B */
		[15] = RCAR_GP_PIN(2, 15),	/* CAN15TX_B */
		[16] = RCAR_GP_PIN(2, 16),	/* CAN_CLK */
		[17] = RCAR_GP_PIN(2, 17),	/* INTP32_B */
		[18] = RCAR_GP_PIN(2, 18),	/* INTP33_B */
		[19] = RCAR_GP_PIN(2, 19),	/* SCL0 */
		[20] = RCAR_GP_PIN(2, 20),	/* SDA0 */
		[21] = RCAR_GP_PIN(2, 21),	/* AVS0 */
		[22] = RCAR_GP_PIN(2, 22),	/* AVS1 */
		[23] = RCAR_GP_PIN(2, 23),	/* EXTCLK0O_B */
		[24] = RCAR_GP_PIN(2, 24),	/* TAUD1O0 */
		[25] = RCAR_GP_PIN(2, 25),	/* TAUD1O1 */
		[26] = RCAR_GP_PIN(2, 26),	/* TAUD1O2 */
		[27] = RCAR_GP_PIN(2, 27),	/* TAUD1O3 */
		[28] = RCAR_GP_PIN(2, 28),	/* INTP34_B */
		[29] = PIN_NONE,
		[30] = PIN_NONE,
		[31] = PIN_NONE,
	} },
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP3_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(3,  0),	/* QSPI0_SPCLK */
		[1]  = RCAR_GP_PIN(3,  1),	/* QSPI0_MOSI_IO0 */
		[2]  = RCAR_GP_PIN(3,  2),	/* QSPI0_MISO_IO1 */
		[3]  = RCAR_GP_PIN(3,  3),	/* QSPI0_IO2 */
		[4]  = RCAR_GP_PIN(3,  4),	/* QSPI0_IO3 */
		[5]  = RCAR_GP_PIN(3,  5),	/* QSPI0_SSL */
		[6]  = RCAR_GP_PIN(3,  6),	/* RPC_RESET_N */
		[7]  = RCAR_GP_PIN(3,  7),	/* RPC_WP_N */
		[8]  = RCAR_GP_PIN(3,  8),	/* RPC_INT_N */
		[9]  = RCAR_GP_PIN(3,  9),	/* QSPI1_SPCLK */
		[10] = RCAR_GP_PIN(3, 10),	/* QSPI1_MOSI_IO0 */
		[11] = RCAR_GP_PIN(3, 11),	/* QSPI1_MISO_IO1 */
		[12] = RCAR_GP_PIN(3, 12),	/* QSPI1_IO2 */
		[13] = RCAR_GP_PIN(3, 13),	/* QSPI1_IO3 */
		[14] = RCAR_GP_PIN(3, 14),	/* QSPI1_SSL */
		[15] = RCAR_GP_PIN(3, 15),	/* ERROROUT_N */
		[16] = RCAR_GP_PIN(3, 16),	/* ERRORIN_N */
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
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP4_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(4,  0),	/* MMC0_SD_CLK */
		[1]  = RCAR_GP_PIN(4,  1),	/* MMC0_SD_CMD */
		[2]  = RCAR_GP_PIN(4,  2),	/* MMC0_SD_D0 */
		[3]  = RCAR_GP_PIN(4,  3),	/* MMC0_SD_D1 */
		[4]  = RCAR_GP_PIN(4,  4),	/* MMC0_SD_D2 */
		[5]  = RCAR_GP_PIN(4,  5),	/* MMC0_SD_D3 */
		[6]  = RCAR_GP_PIN(4,  6),	/* MMC0_D4 */
		[7]  = RCAR_GP_PIN(4,  7),	/* MMC0_D5 */
		[8]  = RCAR_GP_PIN(4,  8),	/* MMC0_D6 */
		[9]  = RCAR_GP_PIN(4,  9),	/* MMC0_D7 */
		[10] = RCAR_GP_PIN(4, 10),	/* MMC0_DS */
		[11] = RCAR_GP_PIN(4, 11),	/* SD0_WP */
		[12] = RCAR_GP_PIN(4, 12),	/* SD0_CD */
		[13] = RCAR_GP_PIN(4, 13),	/* ERRORIN_N */
		[14] = RCAR_GP_PIN(4, 14),	/* PCIE60_CLKREQ_N */
		[15] = RCAR_GP_PIN(4, 15),	/* PCIE61_CLKREQ_N */
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
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP5_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(5,  0),	/* HTX0 */
		[1]  = RCAR_GP_PIN(5,  1),	/* HRX0 */
		[2]  = RCAR_GP_PIN(5,  2),	/* HRTS0_N */
		[3]  = RCAR_GP_PIN(5,  3),	/* HCTS0_N */
		[4]  = RCAR_GP_PIN(5,  4),	/* HSCK0 */
		[5]  = RCAR_GP_PIN(5,  5),	/* SCIF_CLK */
		[6]  = RCAR_GP_PIN(5,  6),	/* HTX1 */
		[7]  = RCAR_GP_PIN(5,  7),	/* HRX1 */
		[8]  = RCAR_GP_PIN(5,  8),	/* HRTS1_N */
		[9]  = RCAR_GP_PIN(5,  9),	/* HCTS1_N */
		[10] = RCAR_GP_PIN(5, 10),	/* HSCK1 */
		[11] = RCAR_GP_PIN(5, 11),	/* IRQ0_A */
		[12] = RCAR_GP_PIN(5, 12),	/* IRQ1_A */
		[13] = RCAR_GP_PIN(5, 13),	/* IRQ2_A */
		[14] = RCAR_GP_PIN(5, 14),	/* IRQ3_A */
		[15] = RCAR_GP_PIN(5, 15),	/* TCLK1 */
		[16] = RCAR_GP_PIN(5, 16),	/* TCLK2 */
		[17] = RCAR_GP_PIN(5, 17),	/* TCLK3 */
		[18] = RCAR_GP_PIN(5, 18),	/* TCLK4 */
		[19] = RCAR_GP_PIN(5, 19),	/* TPU0TO0 */
		[20] = RCAR_GP_PIN(5, 20),	/* TPU0TO1 */
		[21] = RCAR_GP_PIN(5, 21),	/* TPU0TO2 */
		[22] = RCAR_GP_PIN(5, 22),	/* TPU0TO3 */
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
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP6_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(6,  0),	/* RIF6_D0 */
		[1]  = RCAR_GP_PIN(6,  1),	/* RIF6_D1 */
		[2]  = RCAR_GP_PIN(6,  2),	/* RIF6_SYNC */
		[3]  = RCAR_GP_PIN(6,  3),	/* RIF6_CLK */
		[4]  = RCAR_GP_PIN(6,  4),	/* MSIOF7_SCK_A */
		[5]  = RCAR_GP_PIN(6,  5),	/* MSIOF7_TXD_A */
		[6]  = RCAR_GP_PIN(6,  6),	/* MSIOF7_RXD_A */
		[7]  = RCAR_GP_PIN(6,  7),	/* MSIOF7_SYNC_A */
		[8]  = RCAR_GP_PIN(6,  8),	/* MSIOF7_SS1_A */
		[9]  = RCAR_GP_PIN(6,  9),	/* MSIOF7_SS2_A */
		[10] = RCAR_GP_PIN(6, 10),	/* MSIOF4_SCK_B */
		[11] = RCAR_GP_PIN(6, 11),	/* MSIOF4_TXD_B */
		[12] = RCAR_GP_PIN(6, 12),	/* MSIOF4_RXD_B */
		[13] = RCAR_GP_PIN(6, 13),	/* MSIOF4_SYNC_B */
		[14] = RCAR_GP_PIN(6, 14),	/* MSIOF4_SS1_B */
		[15] = RCAR_GP_PIN(6, 15),	/* MSIOF4_SS2_B */
		[16] = RCAR_GP_PIN(6, 16),	/* SSI0_SCK */
		[17] = RCAR_GP_PIN(6, 17),	/* SSI0_WS */
		[18] = RCAR_GP_PIN(6, 18),	/* SSI0_SD */
		[19] = RCAR_GP_PIN(6, 19),	/* AUDIO0_CLKOUT0 */
		[20] = RCAR_GP_PIN(6, 20),	/* AUDIO0_CLKOUT1 */
		[21] = RCAR_GP_PIN(6, 21),	/* SSI1_SCK */
		[22] = RCAR_GP_PIN(6, 22),	/* SSI1_WS */
		[23] = RCAR_GP_PIN(6, 23),	/* SSI1_SD */
		[24] = RCAR_GP_PIN(6, 24),	/* AUDIO0_CLKOUT2 */
		[25] = RCAR_GP_PIN(6, 25),	/* AUDIO0_CLKOUT3 */
		[26] = RCAR_GP_PIN(6, 26),	/* SSI2_SCK */
		[27] = RCAR_GP_PIN(6, 27),	/* SSI2_WS */
		[28] = RCAR_GP_PIN(6, 28),	/* SSI2_SD */
		[29] = RCAR_GP_PIN(6, 29),	/* AUDIO1_CLKOUT0 */
		[30] = RCAR_GP_PIN(6, 30),	/* AUDIO1_CLKOUT1 */
		[31] = PIN_NONE,
	} },
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP7_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(7,  0),	/* SSI3_SCK */
		[1]  = RCAR_GP_PIN(7,  1),	/* SSI3_WS */
		[2]  = RCAR_GP_PIN(7,  2),	/* SSI3_SD */
		[3]  = RCAR_GP_PIN(7,  3),	/* AUDIO1_CLKOUT2 */
		[4]  = RCAR_GP_PIN(7,  4),	/* AUDIO1_CLKOUT3 */
		[5]  = RCAR_GP_PIN(7,  5),	/* SSI4_SCK */
		[6]  = RCAR_GP_PIN(7,  6),	/* SSI4_WS */
		[7]  = RCAR_GP_PIN(7,  7),	/* SSI4_SD */
		[8]  = RCAR_GP_PIN(7,  8),	/* AUDIO_CLKA_A */
		[9]  = RCAR_GP_PIN(7,  9),	/* SSI5_SCK */
		[10] = RCAR_GP_PIN(7, 10),	/* SSI5_WS */
		[11] = RCAR_GP_PIN(7, 11),	/* SSI5_SD */
		[12] = RCAR_GP_PIN(7, 12),	/* AUDIO_CLKB_A */
		[13] = RCAR_GP_PIN(7, 13),	/* SSI6_SCK */
		[14] = RCAR_GP_PIN(7, 14),	/* SSI6_WS */
		[15] = RCAR_GP_PIN(7, 15),	/* SSI6_SD */
		[16] = RCAR_GP_PIN(7, 16),	/* AUDIO_CLKC_A */
		[17] = RCAR_GP_PIN(7, 17),	/* MSIOF5_SCK */
		[18] = RCAR_GP_PIN(7, 18),	/* GP07_18 */
		[19] = RCAR_GP_PIN(7, 19),	/* GP07_19 */
		[20] = RCAR_GP_PIN(7, 20),	/* MSIOF5_TXD */
		[21] = RCAR_GP_PIN(7, 21),	/* MSIOF5_RXD */
		[22] = RCAR_GP_PIN(7, 22),	/* MSIOF5_SYNC */
		[23] = RCAR_GP_PIN(7, 23),	/* MSIOF5_SS1 */
		[24] = RCAR_GP_PIN(7, 24),	/* MSIOF5_SS2 */
		[25] = RCAR_GP_PIN(7, 25),	/* MSIOF6_SCK_B */
		[26] = RCAR_GP_PIN(7, 26),	/* MSIOF6_TXD_B */
		[27] = RCAR_GP_PIN(7, 27),	/* MSIOF6_RXD_B */
		[28] = RCAR_GP_PIN(7, 28),	/* MSIOF6_SYNC_B */
		[29] = RCAR_GP_PIN(7, 29),	/* MSIOF6_SS1_B */
		[30] = RCAR_GP_PIN(7, 30),	/* MSIOF6_SS2_B */
		[31] = PIN_NONE,
	} },
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP8_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(8,  0),	/* SCL1 */
		[1]  = RCAR_GP_PIN(8,  1),	/* SDA1 */
		[2]  = RCAR_GP_PIN(8,  2),	/* SCL2 */
		[3]  = RCAR_GP_PIN(8,  3),	/* SDA2 */
		[4]  = RCAR_GP_PIN(8,  4),	/* SCL3 */
		[5]  = RCAR_GP_PIN(8,  5),	/* SDA3 */
		[6]  = RCAR_GP_PIN(8,  6),	/* SCL4 */
		[7]  = RCAR_GP_PIN(8,  7),	/* SDA4 */
		[8]  = RCAR_GP_PIN(8,  8),	/* SCL5 */
		[9]  = RCAR_GP_PIN(8,  9),	/* SDA5 */
		[10] = RCAR_GP_PIN(8, 10),	/* SCL6 */
		[11] = RCAR_GP_PIN(8, 11),	/* SDA6 */
		[12] = RCAR_GP_PIN(8, 12),	/* SCL7 */
		[13] = RCAR_GP_PIN(8, 13),	/* SDA7 */
		[14] = RCAR_GP_PIN(8, 14),	/* SCL8 */
		[15] = RCAR_GP_PIN(8, 15),	/* SDA8 */
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
		[26] = RCAR_GP_PIN(8, 26),	/* S3CL0 */
		[27] = RCAR_GP_PIN(8, 27),	/* S3DA0 */
		[28] = RCAR_GP_PIN(8, 28),	/* S3CL1 */
		[29] = RCAR_GP_PIN(8, 29),	/* S3DA1 */
		[30] = RCAR_GP_PIN(8, 30),	/* S3CL2 */
		[31] = RCAR_GP_PIN(8, 31),	/* S3DA2 */
	} },
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP9_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(9,  0),	/* ETHES0_PPS */
		[1]  = RCAR_GP_PIN(9,  1),	/* ETHES0_CAPTURE */
		[2]  = RCAR_GP_PIN(9,  2),	/* ETHES0_MATCH */
		[3]  = RCAR_GP_PIN(9,  3),	/* ETHES4_PPS */
		[4]  = RCAR_GP_PIN(9,  4),	/* ETHES4_CAPTURE */
		[5]  = RCAR_GP_PIN(9,  5),	/* ETHES4_MATCH */
		[6]  = RCAR_GP_PIN(9,  6),	/* ETH25G0_MDIO */
		[7]  = RCAR_GP_PIN(9,  7),	/* ETH25G0_MDC */
		[8]  = RCAR_GP_PIN(9,  8),	/* ETH25G0_LINK */
		[9]  = RCAR_GP_PIN(9,  9),	/* ETH25G0_PHYINT */
		[10] = RCAR_GP_PIN(9, 10),	/* ETH10G0_MDIO */
		[11] = RCAR_GP_PIN(9, 11),	/* ETH10G0_MDC */
		[12] = RCAR_GP_PIN(9, 12),	/* ETH10G0_LINK */
		[13] = RCAR_GP_PIN(9, 13),	/* ETH10G0_PHYINT */
		[14] = RCAR_GP_PIN(9, 14),	/* RSW3_PPS */
		[15] = RCAR_GP_PIN(9, 15),	/* RSW3_CAPTURE */
		[16] = RCAR_GP_PIN(9, 16),	/* RSW3_MATCH */
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
	{ PFC_DRIVE_REG(0x80, 0x84, 0x88) { /* GP10_DRVCTRL0/1/2 */
		[0]  = RCAR_GP_PIN(10,  0),	/* USB0_PWEN */
		[1]  = RCAR_GP_PIN(10,  1),	/* USB0_OVC */
		[2]  = RCAR_GP_PIN(10,  2),	/* USB0_VBUS_VALID */
		[3]  = RCAR_GP_PIN(10,  3),	/* USB1_PWEN */
		[4]  = RCAR_GP_PIN(10,  4),	/* USB1_OVC */
		[5]  = RCAR_GP_PIN(10,  5),	/* USB1_VBUS_VALID */
		[6]  = RCAR_GP_PIN(10,  6),	/* USB2_PWEN */
		[7]  = RCAR_GP_PIN(10,  7),	/* USB2_OVC */
		[8]  = RCAR_GP_PIN(10,  8),	/* USB2_VBUS_VALID */
		[9]  = RCAR_GP_PIN(10,  9),	/* USB3_PWEN */
		[10] = RCAR_GP_PIN(10, 10),	/* USB3_OVC */
		[11] = RCAR_GP_PIN(10, 11),	/* USB3_VBUS_VALID */
		[12] = RCAR_GP_PIN(10, 12),	/* PCIE40_CLKREQ_N */
		[13] = RCAR_GP_PIN(10, 13),	/* PCIE41_CLKREQ_N */
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

#define PFC_BIAS_REG(r1, r2)	\
	.puen = r1,		\
	.pud = r2,		\
	.pins =

const struct pfc_bias_reg pfc_bias_regs[] = {
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP0_PUEN, GP0_PUD */
		[0]  = RCAR_GP_PIN(0,  0),	/* GP0_00 */
		[1]  = RCAR_GP_PIN(0,  1),	/* GP0_01 */
		[2]  = RCAR_GP_PIN(0,  2),	/* GP0_02 */
		[3]  = RCAR_GP_PIN(0,  3),	/* STPWT_EXTFXR_A */
		[4]  = RCAR_GP_PIN(0,  4),	/* FXR_CLKOUT1_A */
		[5]  = RCAR_GP_PIN(0,  5),	/* FXR_CLKOUT2_A */
		[6]  = RCAR_GP_PIN(0,  6),	/* CLK_EXTFXR_A */
		[7]  = RCAR_GP_PIN(0,  7),	/* FXR_TXDA_A */
		[8]  = RCAR_GP_PIN(0,  8),	/* FXR_TXENA_N_A */
		[9]  = RCAR_GP_PIN(0,  9),	/* RXDA_EXTFXR_A */
		[10] = RCAR_GP_PIN(0, 10),	/* FXR_TXDB_A */
		[11] = RCAR_GP_PIN(0, 11),	/* FXR_TXENB_N_A */
		[12] = RCAR_GP_PIN(0, 12),	/* RXDB_EXTFXR_A */
		[13] = RCAR_GP_PIN(0, 13),	/* MSIOF0_SCK */
		[14] = RCAR_GP_PIN(0, 14),	/* MSIOF0_TXD */
		[15] = RCAR_GP_PIN(0, 15),	/* MSIOF0_RXD */
		[16] = RCAR_GP_PIN(0, 16),	/* MSIOF0_SYNC */
		[17] = RCAR_GP_PIN(0, 17),	/* MSIOF0_SS1 */
		[18] = RCAR_GP_PIN(0, 18),	/* MSIOF0_SS2 */
		[19] = RCAR_GP_PIN(0, 19),	/* MSIOF1_SCK_A */
		[20] = RCAR_GP_PIN(0, 20),	/* MSIOF1_TXD_A */
		[21] = RCAR_GP_PIN(0, 21),	/* MSIOF1_RXD_A */
		[22] = RCAR_GP_PIN(0, 22),	/* MSIOF1_SYNC_A */
		[23] = RCAR_GP_PIN(0, 23),	/* MSIOF1_SS1_A */
		[24] = RCAR_GP_PIN(0, 24),	/* MSIOF1_SS2_A */
		[25] = RCAR_GP_PIN(0, 25),	/* DP0_HOTPLUG */
		[26] = RCAR_GP_PIN(0, 26),	/* DP1_HOTPLUG */
		[27] = RCAR_GP_PIN(0, 27),	/* DP2_HOTPLUG */
		[28] = PIN_NONE,
		[29] = PIN_NONE,
		[30] = PIN_NONE,
		[31] = PIN_NONE,
	} },
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP1_PUEN, GP1_PUD */
		[0]  = RCAR_GP_PIN(1,  0),	/* CAN0RX_INTP0 */
		[1]  = RCAR_GP_PIN(1,  1),	/* CAN0TX */
		[2]  = RCAR_GP_PIN(1,  2),	/* CAN1RX_INTP1 */
		[3]  = RCAR_GP_PIN(1,  3),	/* CAN1TX */
		[4]  = RCAR_GP_PIN(1,  4),	/* CAN2RX_INTP2 */
		[5]  = RCAR_GP_PIN(1,  5),	/* CAN2TX */
		[6]  = RCAR_GP_PIN(1,  6),	/* CAN3RX_INTP3 */
		[7]  = RCAR_GP_PIN(1,  7),	/* CAN3TX */
		[8]  = RCAR_GP_PIN(1,  8),	/* CAN4RX_INTP4 */
		[9]  = RCAR_GP_PIN(1,  9),	/* CAN4TX */
		[10] = RCAR_GP_PIN(1, 10),	/* CAN5RX_INTP5 */
		[11] = RCAR_GP_PIN(1, 11),	/* CAN5TX */
		[12] = RCAR_GP_PIN(1, 12),	/* CAN6RX_INTP6 */
		[13] = RCAR_GP_PIN(1, 13),	/* CAN6TX */
		[14] = RCAR_GP_PIN(1, 14),	/* RLIN30RX_INTP16 */
		[15] = RCAR_GP_PIN(1, 15),	/* RLIN30TX */
		[16] = RCAR_GP_PIN(1, 16),	/* RLIN31RX_INTP17 */
		[17] = RCAR_GP_PIN(1, 17),	/* RLIN31TX */
		[18] = RCAR_GP_PIN(1, 18),	/* RLIN32RX_INTP18 */
		[19] = RCAR_GP_PIN(1, 19),	/* RLIN32TX */
		[20] = RCAR_GP_PIN(1, 20),	/* RLIN33RX_INTP19 */
		[21] = RCAR_GP_PIN(1, 21),	/* RLIN33TX */
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
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP2_PUEN, GP2_PUD */
		[0]  = RCAR_GP_PIN(2,  0),	/* RLIN34RX_INTP20_B */
		[1]  = RCAR_GP_PIN(2,  1),	/* RLIN34TX_B */
		[2]  = RCAR_GP_PIN(2,  2),	/* RLIN35RX_INTP21_B */
		[3]  = RCAR_GP_PIN(2,  3),	/* RLIN35TX_B */
		[4]  = RCAR_GP_PIN(2,  4),	/* RLIN36RX_INTP22_B */
		[5]  = RCAR_GP_PIN(2,  5),	/* RLIN36TX_B */
		[6]  = RCAR_GP_PIN(2,  6),	/* RLIN37RX_INTP23_B */
		[7]  = RCAR_GP_PIN(2,  7),	/* RLIN37TX_B */
		[8]  = RCAR_GP_PIN(2,  8),	/* CAN12RX_INTP12_B */
		[9]  = RCAR_GP_PIN(2,  9),	/* CAN12TX_B */
		[10] = RCAR_GP_PIN(2, 10),	/* CAN13RX_INTP13_B */
		[11] = RCAR_GP_PIN(2, 11),	/* CAN13TX_B */
		[12] = RCAR_GP_PIN(2, 12),	/* CAN14RX_INTP14_B */
		[13] = RCAR_GP_PIN(2, 13),	/* CAN14TX_B */
		[14] = RCAR_GP_PIN(2, 14),	/* CAN15RX_INTP15_B */
		[15] = RCAR_GP_PIN(2, 15),	/* CAN15TX_B */
		[16] = RCAR_GP_PIN(2, 16),	/* CAN_CLK */
		[17] = RCAR_GP_PIN(2, 17),	/* INTP32_B */
		[18] = RCAR_GP_PIN(2, 18),	/* INTP33_B */
		[19] = RCAR_GP_PIN(2, 19),	/* SCL0 */
		[20] = RCAR_GP_PIN(2, 20),	/* SDA0 */
		[21] = RCAR_GP_PIN(2, 21),	/* AVS0 */
		[22] = RCAR_GP_PIN(2, 22),	/* AVS1 */
		[23] = RCAR_GP_PIN(2, 23),	/* EXTCLK0O_B */
		[24] = RCAR_GP_PIN(2, 24),	/* TAUD1O0 */
		[25] = RCAR_GP_PIN(2, 25),	/* TAUD1O1 */
		[26] = RCAR_GP_PIN(2, 26),	/* TAUD1O2 */
		[27] = RCAR_GP_PIN(2, 27),	/* TAUD1O3 */
		[28] = RCAR_GP_PIN(2, 28),	/* INTP34_B */
		[29] = PIN_NONE,
		[30] = PIN_NONE,
		[31] = PIN_NONE,
	} },
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP3_PUEN, GP3_PUD */
		[0]  = RCAR_GP_PIN(3,  0),	/* QSPI0_SPCLK */
		[1]  = RCAR_GP_PIN(3,  1),	/* QSPI0_MOSI_IO0 */
		[2]  = RCAR_GP_PIN(3,  2),	/* QSPI0_MISO_IO1 */
		[3]  = RCAR_GP_PIN(3,  3),	/* QSPI0_IO2 */
		[4]  = RCAR_GP_PIN(3,  4),	/* QSPI0_IO3 */
		[5]  = RCAR_GP_PIN(3,  5),	/* QSPI0_SSL */
		[6]  = RCAR_GP_PIN(3,  6),	/* RPC_RESET_N */
		[7]  = RCAR_GP_PIN(3,  7),	/* RPC_WP_N */
		[8]  = RCAR_GP_PIN(3,  8),	/* RPC_INT_N */
		[9]  = RCAR_GP_PIN(3,  9),	/* QSPI1_SPCLK */
		[10] = RCAR_GP_PIN(3, 10),	/* QSPI1_MOSI_IO0 */
		[11] = RCAR_GP_PIN(3, 11),	/* QSPI1_MISO_IO1 */
		[12] = RCAR_GP_PIN(3, 12),	/* QSPI1_IO2 */
		[13] = RCAR_GP_PIN(3, 13),	/* QSPI1_IO3 */
		[14] = RCAR_GP_PIN(3, 14),	/* QSPI1_SSL */
		[15] = RCAR_GP_PIN(3, 15),	/* ERROROUT_N */
		[16] = RCAR_GP_PIN(3, 16),	/* ERRORIN_N */
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
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP4_PUEN, GP4_PUD */
		[0]  = RCAR_GP_PIN(4,  0),	/* MMC0_SD_CLK */
		[1]  = RCAR_GP_PIN(4,  1),	/* MMC0_SD_CMD */
		[2]  = RCAR_GP_PIN(4,  2),	/* MMC0_SD_D0 */
		[3]  = RCAR_GP_PIN(4,  3),	/* MMC0_SD_D1 */
		[4]  = RCAR_GP_PIN(4,  4),	/* MMC0_SD_D2 */
		[5]  = RCAR_GP_PIN(4,  5),	/* MMC0_SD_D3 */
		[6]  = RCAR_GP_PIN(4,  6),	/* MMC0_D4 */
		[7]  = RCAR_GP_PIN(4,  7),	/* MMC0_D5 */
		[8]  = RCAR_GP_PIN(4,  8),	/* MMC0_D6 */
		[9]  = RCAR_GP_PIN(4,  9),	/* MMC0_D7 */
		[10] = RCAR_GP_PIN(4, 10),	/* MMC0_DS */
		[11] = RCAR_GP_PIN(4, 11),	/* SD0_WP */
		[12] = RCAR_GP_PIN(4, 12),	/* SD0_CD */
		[13] = RCAR_GP_PIN(4, 13),	/* ERRORIN_N */
		[14] = RCAR_GP_PIN(4, 14),	/* PCIE60_CLKREQ_N */
		[15] = RCAR_GP_PIN(4, 15),	/* PCIE61_CLKREQ_N */
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
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP5_PUEN, GP5_PUD */
		[0]  = RCAR_GP_PIN(5,  0),	/* HTX0 */
		[1]  = RCAR_GP_PIN(5,  1),	/* HRX0 */
		[2]  = RCAR_GP_PIN(5,  2),	/* HRTS0_N */
		[3]  = RCAR_GP_PIN(5,  3),	/* HCTS0_N */
		[4]  = RCAR_GP_PIN(5,  4),	/* HSCK0 */
		[5]  = RCAR_GP_PIN(5,  5),	/* SCIF_CLK */
		[6]  = RCAR_GP_PIN(5,  6),	/* HTX1 */
		[7]  = RCAR_GP_PIN(5,  7),	/* HRX1 */
		[8]  = RCAR_GP_PIN(5,  8),	/* HRTS1_N */
		[9]  = RCAR_GP_PIN(5,  9),	/* HCTS1_N */
		[10] = RCAR_GP_PIN(5, 10),	/* HSCK1 */
		[11] = RCAR_GP_PIN(5, 11),	/* IRQ0_A */
		[12] = RCAR_GP_PIN(5, 12),	/* IRQ1_A */
		[13] = RCAR_GP_PIN(5, 13),	/* IRQ2_A */
		[14] = RCAR_GP_PIN(5, 14),	/* IRQ3_A */
		[15] = RCAR_GP_PIN(5, 15),	/* TCLK1 */
		[16] = RCAR_GP_PIN(5, 16),	/* TCLK2 */
		[17] = RCAR_GP_PIN(5, 17),	/* TCLK3 */
		[18] = RCAR_GP_PIN(5, 18),	/* TCLK4 */
		[19] = RCAR_GP_PIN(5, 19),	/* TPU0TO0 */
		[20] = RCAR_GP_PIN(5, 20),	/* TPU0TO1 */
		[21] = RCAR_GP_PIN(5, 21),	/* TPU0TO2 */
		[22] = RCAR_GP_PIN(5, 22),	/* TPU0TO3 */
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
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP6_PUEN, GP6_PUD */
		[0]  = RCAR_GP_PIN(6,  0),	/* RIF6_D0 */
		[1]  = RCAR_GP_PIN(6,  1),	/* RIF6_D1 */
		[2]  = RCAR_GP_PIN(6,  2),	/* RIF6_SYNC */
		[3]  = RCAR_GP_PIN(6,  3),	/* RIF6_CLK */
		[4]  = RCAR_GP_PIN(6,  4),	/* MSIOF7_SCK_A */
		[5]  = RCAR_GP_PIN(6,  5),	/* MSIOF7_TXD_A */
		[6]  = RCAR_GP_PIN(6,  6),	/* MSIOF7_RXD_A */
		[7]  = RCAR_GP_PIN(6,  7),	/* MSIOF7_SYNC_A */
		[8]  = RCAR_GP_PIN(6,  8),	/* MSIOF7_SS1_A */
		[9]  = RCAR_GP_PIN(6,  9),	/* MSIOF7_SS2_A */
		[10] = RCAR_GP_PIN(6, 10),	/* MSIOF4_SCK_B */
		[11] = RCAR_GP_PIN(6, 11),	/* MSIOF4_TXD_B */
		[12] = RCAR_GP_PIN(6, 12),	/* MSIOF4_RXD_B */
		[13] = RCAR_GP_PIN(6, 13),	/* MSIOF4_SYNC_B */
		[14] = RCAR_GP_PIN(6, 14),	/* MSIOF4_SS1_B */
		[15] = RCAR_GP_PIN(6, 15),	/* MSIOF4_SS2_B */
		[16] = RCAR_GP_PIN(6, 16),	/* SSI0_SCK */
		[17] = RCAR_GP_PIN(6, 17),	/* SSI0_WS */
		[18] = RCAR_GP_PIN(6, 18),	/* SSI0_SD */
		[19] = RCAR_GP_PIN(6, 19),	/* AUDIO0_CLKOUT0 */
		[20] = RCAR_GP_PIN(6, 20),	/* AUDIO0_CLKOUT1 */
		[21] = RCAR_GP_PIN(6, 21),	/* SSI1_SCK */
		[22] = RCAR_GP_PIN(6, 22),	/* SSI1_WS */
		[23] = RCAR_GP_PIN(6, 23),	/* SSI1_SD */
		[24] = RCAR_GP_PIN(6, 24),	/* AUDIO0_CLKOUT2 */
		[25] = RCAR_GP_PIN(6, 25),	/* AUDIO0_CLKOUT3 */
		[26] = RCAR_GP_PIN(6, 26),	/* SSI2_SCK */
		[27] = RCAR_GP_PIN(6, 27),	/* SSI2_WS */
		[28] = RCAR_GP_PIN(6, 28),	/* SSI2_SD */
		[29] = RCAR_GP_PIN(6, 29),	/* AUDIO1_CLKOUT0 */
		[30] = RCAR_GP_PIN(6, 30),	/* AUDIO1_CLKOUT1 */
		[31] = PIN_NONE,
	} },
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP7_PUEN, GP7_PUD */
		[0]  = RCAR_GP_PIN(7,  0),	/* SSI3_SCK */
		[1]  = RCAR_GP_PIN(7,  1),	/* SSI3_WS */
		[2]  = RCAR_GP_PIN(7,  2),	/* SSI3_SD */
		[3]  = RCAR_GP_PIN(7,  3),	/* AUDIO1_CLKOUT2 */
		[4]  = RCAR_GP_PIN(7,  4),	/* AUDIO1_CLKOUT3 */
		[5]  = RCAR_GP_PIN(7,  5),	/* SSI4_SCK */
		[6]  = RCAR_GP_PIN(7,  6),	/* SSI4_WS */
		[7]  = RCAR_GP_PIN(7,  7),	/* SSI4_SD */
		[8]  = RCAR_GP_PIN(7,  8),	/* AUDIO_CLKA_A */
		[9]  = RCAR_GP_PIN(7,  9),	/* SSI5_SCK */
		[10] = RCAR_GP_PIN(7, 10),	/* SSI5_WS */
		[11] = RCAR_GP_PIN(7, 11),	/* SSI5_SD */
		[12] = RCAR_GP_PIN(7, 12),	/* AUDIO_CLKB_A */
		[13] = RCAR_GP_PIN(7, 13),	/* SSI6_SCK */
		[14] = RCAR_GP_PIN(7, 14),	/* SSI6_WS */
		[15] = RCAR_GP_PIN(7, 15),	/* SSI6_SD */
		[16] = RCAR_GP_PIN(7, 16),	/* AUDIO_CLKC_A */
		[17] = RCAR_GP_PIN(7, 17),	/* MSIOF5_SCK */
		[18] = RCAR_GP_PIN(7, 18),	/* GP07_18 */
		[19] = RCAR_GP_PIN(7, 19),	/* GP07_19 */
		[20] = RCAR_GP_PIN(7, 20),	/* MSIOF5_TXD */
		[21] = RCAR_GP_PIN(7, 21),	/* MSIOF5_RXD */
		[22] = RCAR_GP_PIN(7, 22),	/* MSIOF5_SYNC */
		[23] = RCAR_GP_PIN(7, 23),	/* MSIOF5_SS1 */
		[24] = RCAR_GP_PIN(7, 24),	/* MSIOF5_SS2 */
		[25] = RCAR_GP_PIN(7, 25),	/* MSIOF6_SCK_B */
		[26] = RCAR_GP_PIN(7, 26),	/* MSIOF6_TXD_B */
		[27] = RCAR_GP_PIN(7, 27),	/* MSIOF6_RXD_B */
		[28] = RCAR_GP_PIN(7, 28),	/* MSIOF6_SYNC_B */
		[29] = RCAR_GP_PIN(7, 29),	/* MSIOF6_SS1_B */
		[30] = RCAR_GP_PIN(7, 30),	/* MSIOF6_SS2_B */
		[31] = PIN_NONE,
	} },
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP8_PUEN, GP8_PUD */
		[0]  = RCAR_GP_PIN(8,  0),	/* SCL1 */
		[1]  = RCAR_GP_PIN(8,  1),	/* SDA1 */
		[2]  = RCAR_GP_PIN(8,  2),	/* SCL2 */
		[3]  = RCAR_GP_PIN(8,  3),	/* SDA2 */
		[4]  = RCAR_GP_PIN(8,  4),	/* SCL3 */
		[5]  = RCAR_GP_PIN(8,  5),	/* SDA3 */
		[6]  = RCAR_GP_PIN(8,  6),	/* SCL4 */
		[7]  = RCAR_GP_PIN(8,  7),	/* SDA4 */
		[8]  = RCAR_GP_PIN(8,  8),	/* SCL5 */
		[9]  = RCAR_GP_PIN(8,  9),	/* SDA5 */
		[10] = RCAR_GP_PIN(8, 10),	/* SCL6 */
		[11] = RCAR_GP_PIN(8, 11),	/* SDA6 */
		[12] = RCAR_GP_PIN(8, 12),	/* SCL7 */
		[13] = RCAR_GP_PIN(8, 13),	/* SDA7 */
		[14] = RCAR_GP_PIN(8, 14),	/* SCL8 */
		[15] = RCAR_GP_PIN(8, 15),	/* SDA8 */
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
		[26] = RCAR_GP_PIN(8, 26),	/* S3CL0 */
		[27] = RCAR_GP_PIN(8, 27),	/* S3DA0 */
		[28] = RCAR_GP_PIN(8, 28),	/* S3CL1 */
		[29] = RCAR_GP_PIN(8, 29),	/* S3DA1 */
		[30] = RCAR_GP_PIN(8, 30),	/* S3CL2 */
		[31] = RCAR_GP_PIN(8, 31),	/* S3DA2 */
	} },
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP9_PUEN, GP9_PUD */
		[0]  = RCAR_GP_PIN(9,  0),	/* ETHES0_PPS */
		[1]  = RCAR_GP_PIN(9,  1),	/* ETHES0_CAPTURE */
		[2]  = RCAR_GP_PIN(9,  2),	/* ETHES0_MATCH */
		[3]  = RCAR_GP_PIN(9,  3),	/* ETHES4_PPS */
		[4]  = RCAR_GP_PIN(9,  4),	/* ETHES4_CAPTURE */
		[5]  = RCAR_GP_PIN(9,  5),	/* ETHES4_MATCH */
		[6]  = RCAR_GP_PIN(9,  6),	/* ETH25G0_MDIO */
		[7]  = RCAR_GP_PIN(9,  7),	/* ETH25G0_MDC */
		[8]  = RCAR_GP_PIN(9,  8),	/* ETH25G0_LINK */
		[9]  = RCAR_GP_PIN(9,  9),	/* ETH25G0_PHYINT */
		[10] = RCAR_GP_PIN(9, 10),	/* ETH10G0_MDIO */
		[11] = RCAR_GP_PIN(9, 11),	/* ETH10G0_MDC */
		[12] = RCAR_GP_PIN(9, 12),	/* ETH10G0_LINK */
		[13] = RCAR_GP_PIN(9, 13),	/* ETH10G0_PHYINT */
		[14] = RCAR_GP_PIN(9, 14),	/* RSW3_PPS */
		[15] = RCAR_GP_PIN(9, 15),	/* RSW3_CAPTURE */
		[16] = RCAR_GP_PIN(9, 16),	/* RSW3_MATCH */
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
	{ PFC_BIAS_REG(0xc0, 0xc4) { /* GP10_PUEN, GP10_PUD */
		[0]  = RCAR_GP_PIN(10,  0),	/* USB0_PWEN */
		[1]  = RCAR_GP_PIN(10,  1),	/* USB0_OVC */
		[2]  = RCAR_GP_PIN(10,  2),	/* USB0_VBUS_VALID */
		[3]  = RCAR_GP_PIN(10,  3),	/* USB1_PWEN */
		[4]  = RCAR_GP_PIN(10,  4),	/* USB1_OVC */
		[5]  = RCAR_GP_PIN(10,  5),	/* USB1_VBUS_VALID */
		[6]  = RCAR_GP_PIN(10,  6),	/* USB2_PWEN */
		[7]  = RCAR_GP_PIN(10,  7),	/* USB2_OVC */
		[8]  = RCAR_GP_PIN(10,  8),	/* USB2_VBUS_VALID */
		[9]  = RCAR_GP_PIN(10,  9),	/* USB3_PWEN */
		[10] = RCAR_GP_PIN(10, 10),	/* USB3_OVC */
		[11] = RCAR_GP_PIN(10, 11),	/* USB3_VBUS_VALID */
		[12] = RCAR_GP_PIN(10, 12),	/* PCIE40_CLKREQ_N */
		[13] = RCAR_GP_PIN(10, 13),	/* PCIE41_CLKREQ_N */
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

int pfc_rcar_get_reg_index(uint16_t pin, uint8_t *reg_index)
{
	*reg_index = pin / 32;

	return 0;
}
