/*
 * Copyright (c) 2023 IoT.bzh
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <errno.h>
#include <pinctrl_soc.h>
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-r8a779f0.h>

const struct pfc_drive_reg pfc_drive_regs[] = {
	/* DRV0CTRL0 */
	{ 0x80, {
		{ RCAR_GP_PIN(0,  7), 28, 3 },	/* TX0 */
		{ RCAR_GP_PIN(0,  6), 24, 3 },	/* RX0 */
		{ RCAR_GP_PIN(0,  5), 20, 3 },	/* HRTS0_N */
		{ RCAR_GP_PIN(0,  4), 16, 3 },	/* HCTS0_N */
		{ RCAR_GP_PIN(0,  3), 12, 3 },	/* HTX0 */
		{ RCAR_GP_PIN(0,  2),  8, 3 },	/* HRX0 */
		{ RCAR_GP_PIN(0,  1),  4, 3 },	/* HSCK0 */
		{ RCAR_GP_PIN(0,  0),  0, 3 },	/* SCIF_CLK */
	} },
	/* DRV1CTRL0 */
	{ 0x84, {
		{ RCAR_GP_PIN(0, 15), 28, 3 },	/* MSIOF0_SS1 */
		{ RCAR_GP_PIN(0, 14), 24, 3 },	/* MSIOF0_SCK */
		{ RCAR_GP_PIN(0, 13), 20, 3 },	/* MSIOF0_TXD */
		{ RCAR_GP_PIN(0, 12), 16, 3 },	/* MSIOF0_RXD */
		{ RCAR_GP_PIN(0, 11), 12, 3 },	/* MSIOF0_SYNC */
		{ RCAR_GP_PIN(0, 10),  8, 3 },	/* CTS0_N */
		{ RCAR_GP_PIN(0,  9),  4, 3 },	/* RTS0_N */
		{ RCAR_GP_PIN(0,  8),  0, 3 },	/* SCK0 */
	} },
	/* DRV2CTRL0 */
	{ 0x88, {
		{ RCAR_GP_PIN(0, 20), 16, 3 },	/* IRQ3 */
		{ RCAR_GP_PIN(0, 19), 12, 3 },	/* IRQ2 */
		{ RCAR_GP_PIN(0, 18),  8, 3 },	/* IRQ1 */
		{ RCAR_GP_PIN(0, 17),  4, 3 },	/* IRQ0 */
		{ RCAR_GP_PIN(0, 16),  0, 3 },	/* MSIOF0_SS2 */
	} },
	/* DRV3CTRL0 is empty */
	/* DRV0CTRL1 */
	{ 0x80, {
		{ RCAR_GP_PIN(1,  7), 28, 3 },	/* GP1_07 */
		{ RCAR_GP_PIN(1,  6), 24, 3 },	/* GP1_06 */
		{ RCAR_GP_PIN(1,  5), 20, 3 },	/* GP1_05 */
		{ RCAR_GP_PIN(1,  4), 16, 3 },	/* GP1_04 */
		{ RCAR_GP_PIN(1,  3), 12, 3 },	/* GP1_03 */
		{ RCAR_GP_PIN(1,  2),  8, 3 },	/* GP1_02 */
		{ RCAR_GP_PIN(1,  1),  4, 3 },	/* GP1_01 */
		{ RCAR_GP_PIN(1,  0),  0, 3 },	/* GP1_00 */
	} },
	/* DRV1CTRL1 */
	{ 0x84, {
		{ RCAR_GP_PIN(1, 15), 28, 3 },	/* MMC_SD_D2 */
		{ RCAR_GP_PIN(1, 14), 24, 3 },	/* MMC_SD_D1 */
		{ RCAR_GP_PIN(1, 13), 20, 3 },	/* MMC_SD_D0 */
		{ RCAR_GP_PIN(1, 12), 16, 3 },	/* MMC_SD_CLK */
		{ RCAR_GP_PIN(1, 11), 12, 3 },	/* GP1_11 */
		{ RCAR_GP_PIN(1, 10),  8, 3 },	/* GP1_10 */
		{ RCAR_GP_PIN(1,  9),  4, 3 },	/* GP1_09 */
		{ RCAR_GP_PIN(1,  8),  0, 3 },	/* GP1_08 */
	} },
	/* DRV2CTRL1 */
	{ 0x88, {
		{ RCAR_GP_PIN(1, 23), 28, 3 },	/* SD_CD */
		{ RCAR_GP_PIN(1, 22), 24, 3 },	/* MMC_SD_CMD */
		{ RCAR_GP_PIN(1, 21), 20, 3 },	/* MMC_D7 */
		{ RCAR_GP_PIN(1, 20), 16, 3 },	/* MMC_DS */
		{ RCAR_GP_PIN(1, 19), 12, 3 },	/* MMC_D6 */
		{ RCAR_GP_PIN(1, 18),  8, 3 },	/* MMC_D4 */
		{ RCAR_GP_PIN(1, 17),  4, 3 },	/* MMC_D5 */
		{ RCAR_GP_PIN(1, 16),  0, 3 },	/* MMC_SD_D3 */
	} },
	/* DRV3CTRL1 */
	{ 0x8c, {
		{ RCAR_GP_PIN(1, 24),  0, 3 },	/* SD_WP */
	} },
	/* DRV0CTRL2 */
	{ 0x80, {
		{ RCAR_GP_PIN(2,  7), 28, 2 },	/* QSPI1_MOSI_IO0 */
		{ RCAR_GP_PIN(2,  6), 24, 2 },	/* QSPI1_IO2 */
		{ RCAR_GP_PIN(2,  5), 20, 2 },	/* QSPI1_MISO_IO1 */
		{ RCAR_GP_PIN(2,  4), 16, 2 },	/* QSPI1_IO3 */
		{ RCAR_GP_PIN(2,  3), 12, 2 },	/* QSPI1_SSL */
		{ RCAR_GP_PIN(2,  2),  8, 2 },	/* RPC_RESET_N */
		{ RCAR_GP_PIN(2,  1),  4, 2 },	/* RPC_WP_N */
		{ RCAR_GP_PIN(2,  0),  0, 2 },	/* RPC_INT_N */
	} },
	/* DRV1CTRL2 */
	{ 0x84, {
		{ RCAR_GP_PIN(2, 15), 28, 3 },	/* PCIE0_CLKREQ_N */
		{ RCAR_GP_PIN(2, 14), 24, 2 },	/* QSPI0_IO3 */
		{ RCAR_GP_PIN(2, 13), 20, 2 },	/* QSPI0_SSL */
		{ RCAR_GP_PIN(2, 12), 16, 2 },	/* QSPI0_MISO_IO1 */
		{ RCAR_GP_PIN(2, 11), 12, 2 },	/* QSPI0_IO2 */
		{ RCAR_GP_PIN(2, 10),  8, 2 },	/* QSPI0_SPCLK */
		{ RCAR_GP_PIN(2,  9),  4, 2 },	/* QSPI0_MOSI_IO0 */
		{ RCAR_GP_PIN(2,  8),  0, 2 },	/* QSPI1_SPCLK */
	} },
	/* DRV2CTRL2 */
	{ 0x88, {
		{ RCAR_GP_PIN(2, 16),  0, 3 },	/* PCIE1_CLKREQ_N */
	} },
	/* DRV3CTRL2 is empty */
	/* DRV0CTRL3 */
	{ 0x80, {
		{ RCAR_GP_PIN(3,  7), 28, 3 },	/* TSN2_LINK_B */
		{ RCAR_GP_PIN(3,  6), 24, 3 },	/* TSN1_LINK_B */
		{ RCAR_GP_PIN(3,  5), 20, 3 },	/* TSN1_MDC_B */
		{ RCAR_GP_PIN(3,  4), 16, 3 },	/* TSN0_MDC_B */
		{ RCAR_GP_PIN(3,  3), 12, 3 },	/* TSN2_MDC_B */
		{ RCAR_GP_PIN(3,  2),  8, 3 },	/* TSN0_MDIO_B */
		{ RCAR_GP_PIN(3,  1),  4, 3 },	/* TSN2_MDIO_B */
		{ RCAR_GP_PIN(3,  0),  0, 3 },	/* TSN1_MDIO_B */
	} },
	/* DRV1CTRL3 */
	{ 0x84, {
		{ RCAR_GP_PIN(3, 15), 28, 3 },	/* TSN1_AVTP_CAPTURE_B */
		{ RCAR_GP_PIN(3, 14), 24, 3 },	/* TSN1_AVTP_MATCH_B */
		{ RCAR_GP_PIN(3, 13), 20, 3 },	/* TSN1_AVTP_PPS */
		{ RCAR_GP_PIN(3, 12), 16, 3 },	/* TSN0_MAGIC_B */
		{ RCAR_GP_PIN(3, 11), 12, 3 },	/* TSN1_PHY_INT_B */
		{ RCAR_GP_PIN(3, 10),  8, 3 },	/* TSN0_PHY_INT_B */
		{ RCAR_GP_PIN(3,  9),  4, 3 },	/* TSN2_PHY_INT_B */
		{ RCAR_GP_PIN(3,  8),  0, 3 },	/* TSN0_LINK_B */
	} },
	/* DRV2CTRL3 */
	{ 0x88, {
		{ RCAR_GP_PIN(3, 18),  8, 3 },	/* TSN0_AVTP_CAPTURE_B */
		{ RCAR_GP_PIN(3, 17),  4, 3 },	/* TSN0_AVTP_MATCH_B */
		{ RCAR_GP_PIN(3, 16),  0, 3 },	/* TSN0_AVTP_PPS */
	} },
	/* DRV3CTRL3 is empty */
	/* DRV0CTRL4 */
	{ 0x80, {
		{ RCAR_GP_PIN(4,  7), 28, 3 },	/* GP4_07 */
		{ RCAR_GP_PIN(4,  6), 24, 3 },	/* GP4_06 */
		{ RCAR_GP_PIN(4,  5), 20, 3 },	/* GP4_05 */
		{ RCAR_GP_PIN(4,  4), 16, 3 },	/* GP4_04 */
		{ RCAR_GP_PIN(4,  3), 12, 3 },	/* GP4_03 */
		{ RCAR_GP_PIN(4,  2),  8, 3 },	/* GP4_02 */
		{ RCAR_GP_PIN(4,  1),  4, 3 },	/* GP4_01 */
		{ RCAR_GP_PIN(4,  0),  0, 3 },	/* GP4_00 */
	} },
	/* DRV1CTRL4 */
	{ 0x84, {
		{ RCAR_GP_PIN(4, 15), 28, 3 },	/* GP4_15 */
		{ RCAR_GP_PIN(4, 14), 24, 3 },	/* GP4_14 */
		{ RCAR_GP_PIN(4, 13), 20, 3 },	/* GP4_13 */
		{ RCAR_GP_PIN(4, 12), 16, 3 },	/* GP4_12 */
		{ RCAR_GP_PIN(4, 11), 12, 3 },	/* GP4_11 */
		{ RCAR_GP_PIN(4, 10),  8, 3 },	/* GP4_10 */
		{ RCAR_GP_PIN(4,  9),  4, 3 },	/* GP4_09 */
		{ RCAR_GP_PIN(4,  8),  0, 3 },	/* GP4_08 */
	} },
	/* DRV2CTRL4 */
	{ 0x88, {
		{ RCAR_GP_PIN(4, 23), 28, 3 },	/* MSPI0CSS1 */
		{ RCAR_GP_PIN(4, 22), 24, 3 },	/* MPSI0SO/MSPI0DCS */
		{ RCAR_GP_PIN(4, 21), 20, 3 },	/* MPSI0SI */
		{ RCAR_GP_PIN(4, 20), 16, 3 },	/* MSPI0SC */
		{ RCAR_GP_PIN(4, 19), 12, 3 },	/* GP4_19 */
		{ RCAR_GP_PIN(4, 18),  8, 3 },	/* GP4_18 */
		{ RCAR_GP_PIN(4, 17),  4, 3 },	/* GP4_17 */
		{ RCAR_GP_PIN(4, 16),  0, 3 },	/* GP4_16 */
	} },
	/* DRV3CTRL4 */
	{ 0x8c, {
		{ RCAR_GP_PIN(4, 30),  24, 3 },	/* MSPI1CSS1 */
		{ RCAR_GP_PIN(4, 29),  20, 3 },	/* MSPI1CSS2 */
		{ RCAR_GP_PIN(4, 28),  16, 3 },	/* MSPI1SC */
		{ RCAR_GP_PIN(4, 27),  12, 3 },	/* MSPI1CSS0 */
		{ RCAR_GP_PIN(4, 26),  8, 3 },	/* MPSI1SO/MSPI1DCS */
		{ RCAR_GP_PIN(4, 25),  4, 3 },	/* MSPI1SI */
		{ RCAR_GP_PIN(4, 24),  0, 3 },	/* MSPI0CSS0 */
	} },
	/* DRV0CTRL5 */
	{ 0x80, {
		{ RCAR_GP_PIN(5,  7), 28, 3 },	/* ETNB0RXD3 */
		{ RCAR_GP_PIN(5,  6), 24, 3 },	/* ETNB0RXER */
		{ RCAR_GP_PIN(5,  5), 20, 3 },	/* ETNB0MDC */
		{ RCAR_GP_PIN(5,  4), 16, 3 },	/* ETNB0LINKSTA */
		{ RCAR_GP_PIN(5,  3), 12, 3 },	/* ETNB0WOL */
		{ RCAR_GP_PIN(5,  2),  8, 3 },	/* ETNB0MD */
		{ RCAR_GP_PIN(5,  1),  4, 3 },	/* RIIC0SDA */
		{ RCAR_GP_PIN(5,  0),  0, 3 },	/* RIIC0SCL */
	} },
	/* DRV1CTRL5 */
	{ 0x84, {
		{ RCAR_GP_PIN(5, 15), 28, 3 },	/* ETNB0TXCLK */
		{ RCAR_GP_PIN(5, 14), 24, 3 },	/* ETNB0TXD3 */
		{ RCAR_GP_PIN(5, 13), 20, 3 },	/* ETNB0TXER */
		{ RCAR_GP_PIN(5, 12), 16, 3 },	/* ETNB0RXCLK */
		{ RCAR_GP_PIN(5, 11), 12, 3 },	/* ETNB0RXD0 */
		{ RCAR_GP_PIN(5, 10),  8, 3 },	/* ETNB0RXDV */
		{ RCAR_GP_PIN(5,  9),  4, 3 },	/* ETNB0RXD2 */
		{ RCAR_GP_PIN(5,  8),  0, 3 },	/* ETNB0RXD1 */
	} },
	/* DRV2CTRL5 */
	{ 0x88, {
		{ RCAR_GP_PIN(5, 19), 12, 3 },	/* ETNB0TXD0 */
		{ RCAR_GP_PIN(5, 18),  8, 3 },	/* ETNB0TXEN */
		{ RCAR_GP_PIN(5, 17),  4, 3 },	/* ETNB0TXD2 */
		{ RCAR_GP_PIN(5, 16),  0, 3 },	/* ETNB0TXD1 */
	} },
	/* DRV3CTRL5 is empty */
	/* DRV0CTRL6 */
	{ 0x80, {
		{ RCAR_GP_PIN(6,  7), 28, 3 },	/* RLIN34RX/INTP20 */
		{ RCAR_GP_PIN(6,  6), 24, 3 },	/* RLIN34TX */
		{ RCAR_GP_PIN(6,  5), 20, 3 },	/* RLIN35RX/INTP21 */
		{ RCAR_GP_PIN(6,  4), 16, 3 },	/* RLIN35TX */
		{ RCAR_GP_PIN(6,  3), 12, 3 },	/* RLIN36RX/INTP22 */
		{ RCAR_GP_PIN(6,  2),  8, 3 },	/* RLIN36TX */
		{ RCAR_GP_PIN(6,  1),  4, 3 },	/* RLIN37RX/INTP23 */
		{ RCAR_GP_PIN(6,  0),  0, 3 },	/* RLIN37TX */
	} },
	/* DRV1CTRL6 */
	{ 0x84, {
		{ RCAR_GP_PIN(6, 15), 28, 3 },	/* RLIN30RX/INTP16 */
		{ RCAR_GP_PIN(6, 14), 24, 3 },	/* RLIN30TX */
		{ RCAR_GP_PIN(6, 13), 20, 3 },	/* RLIN31RX/INTP17 */
		{ RCAR_GP_PIN(6, 12), 16, 3 },	/* RLIN31TX */
		{ RCAR_GP_PIN(6, 11), 12, 3 },	/* RLIN32RX/INTP18 */
		{ RCAR_GP_PIN(6, 10),  8, 3 },	/* RLIN32TX */
		{ RCAR_GP_PIN(6,  9),  4, 3 },	/* RLIN33RX/INTP19 */
		{ RCAR_GP_PIN(6,  8),  0, 3 },	/* RLIN33TX */
	} },
	/* DRV2CTRL6 */
	{ 0x88, {
		{ RCAR_GP_PIN(6, 22), 24, 3 },	/* NMI1 */
		{ RCAR_GP_PIN(6, 21), 20, 3 },	/* INTP32 */
		{ RCAR_GP_PIN(6, 20), 16, 3 },	/* INTP33 */
		{ RCAR_GP_PIN(6, 19), 12, 3 },	/* INTP34 */
		{ RCAR_GP_PIN(6, 18),  8, 3 },	/* INTP35 */
		{ RCAR_GP_PIN(6, 17),  4, 3 },	/* INTP36 */
		{ RCAR_GP_PIN(6, 16),  0, 3 },	/* INTP37 */
	} },
	/* DRV3CTRL6 */
	{ 0x8c, {
		{ RCAR_GP_PIN(6, 31), 28, 3 },	/* PRESETOUT1# */
	} },
	/* DRV0CTRL7 */
	{ 0x80, {
		{ RCAR_GP_PIN(7,  7), 28, 3 },	/* CAN3RX/INTP3 */
		{ RCAR_GP_PIN(7,  6), 24, 3 },	/* CAN3TX */
		{ RCAR_GP_PIN(7,  5), 20, 3 },	/* CAN2RX/INTP2 */
		{ RCAR_GP_PIN(7,  4), 16, 3 },	/* CAN2TX */
		{ RCAR_GP_PIN(7,  3), 12, 3 },	/* CAN1RX/INTP1 */
		{ RCAR_GP_PIN(7,  2),  8, 3 },	/* CAN1TX */
		{ RCAR_GP_PIN(7,  1),  4, 3 },	/* CAN0RX/INTP0 */
		{ RCAR_GP_PIN(7,  0),  0, 3 },	/* CAN0TX */
	} },
	/* DRV1CTRL7 */
	{ 0x84, {
		{ RCAR_GP_PIN(7, 15), 28, 3 },	/* CAN7RX/INTP7 */
		{ RCAR_GP_PIN(7, 14), 24, 3 },	/* CAN7TX */
		{ RCAR_GP_PIN(7, 13), 20, 3 },	/* CAN6RX/INTP6 */
		{ RCAR_GP_PIN(7, 12), 16, 3 },	/* CAN6TX */
		{ RCAR_GP_PIN(7, 11), 12, 3 },	/* CAN5RX/INTP5 */
		{ RCAR_GP_PIN(7, 10),  8, 3 },	/* CAN5TX */
		{ RCAR_GP_PIN(7,  9),  4, 3 },	/* CAN4RX/INTP4 */
		{ RCAR_GP_PIN(7,  8),  0, 3 },	/* CAN4TX */
	} },
	/* DRV2CTRL7 */
	{ 0x88, {
		{ RCAR_GP_PIN(7, 23), 28, 3 },	/* CAN11RX/INTP11 */
		{ RCAR_GP_PIN(7, 22), 24, 3 },	/* CAN11TX */
		{ RCAR_GP_PIN(7, 21), 20, 3 },	/* CAN10RX/INTP10 */
		{ RCAR_GP_PIN(7, 20), 16, 3 },	/* CAN10TX */
		{ RCAR_GP_PIN(7, 19), 12, 3 },	/* CAN9RX/INTP9 */
		{ RCAR_GP_PIN(7, 18),  8, 3 },	/* CAN9TX */
		{ RCAR_GP_PIN(7, 17),  4, 3 },	/* CAN8RX/INTP8 */
		{ RCAR_GP_PIN(7, 16),  0, 3 },	/* CAN8TX */
	} },
	/* DRV3CTRL7 */
	{ 0x8c, {
		{ RCAR_GP_PIN(7, 31), 28, 3 },	/* CAN15RX/INTP15 */
		{ RCAR_GP_PIN(7, 30), 24, 3 },	/* CAN15TX */
		{ RCAR_GP_PIN(7, 29), 20, 3 },	/* CAN14RX/INTP14 */
		{ RCAR_GP_PIN(7, 28), 16, 3 },	/* CAN14TX */
		{ RCAR_GP_PIN(7, 27), 12, 3 },	/* CAN13RX/INTP13 */
		{ RCAR_GP_PIN(7, 26),  8, 3 },	/* CAN13TX */
		{ RCAR_GP_PIN(7, 25),  4, 3 },	/* CAN12RX/INTP12 */
		{ RCAR_GP_PIN(7, 24),  0, 3 },	/* CAN12TX */
	} },
	/* DRV0CTRLSYS0 */
	{ 0x80, {
		{ RCAR_GP_PIN(8,  0),  0, 3 },	/* PRESETOUT0# */
	} },
	/* DRV1CTRLSYS0 */
	{ 0x84, {
		{ RCAR_GP_PIN(8, 12), 16, 2 },	/* DCUTCK0 */
		{ RCAR_GP_PIN(8, 11), 12, 2 },	/* DCUTDO0 */
		{ RCAR_GP_PIN(8, 10),  8, 2 },	/* DCUTDI0 */
		{ RCAR_GP_PIN(8,  9),  4, 2 },	/* DCUTDY0# */
		{ RCAR_GP_PIN(8,  8),  0, 2 },	/* DCUTMS0 */
	} },
	{ },
};

#define PFC_BIAS_REG(r1, r2) \
	.puen = r1,	     \
	.pud = r2,	     \
	.pins =

const struct pfc_bias_reg pfc_bias_regs[] = {
	{ PFC_BIAS_REG(0xc0, 0xe0) {         /* PUEN0, PUD0 */
		[0]  = RCAR_GP_PIN(0,  0),	/* SCIF_CLK */
		[1]  = RCAR_GP_PIN(0,  1),	/* HSCK0 */
		[2]  = RCAR_GP_PIN(0,  2),	/* HRX0 */
		[3]  = RCAR_GP_PIN(0,  3),	/* HTX0 */
		[4]  = RCAR_GP_PIN(0,  4),	/* HCTS0_N */
		[5]  = RCAR_GP_PIN(0,  5),	/* HRTS0_N */
		[6]  = RCAR_GP_PIN(0,  6),	/* RX0 */
		[7]  = RCAR_GP_PIN(0,  7),	/* TX0 */
		[8]  = RCAR_GP_PIN(0,  8),	/* SCK0 */
		[9]  = RCAR_GP_PIN(0,  9),	/* RTS0_N */
		[10] = RCAR_GP_PIN(0, 10),	/* CTS0_N */
		[11] = RCAR_GP_PIN(0, 11),	/* MSIOF0_SYNC */
		[12] = RCAR_GP_PIN(0, 12),	/* MSIOF0_RXD */
		[13] = RCAR_GP_PIN(0, 13),	/* MSIOF0_TXD */
		[14] = RCAR_GP_PIN(0, 14),	/* MSIOF0_SCK */
		[15] = RCAR_GP_PIN(0, 15),	/* MSIOF0_SS1 */
		[16] = RCAR_GP_PIN(0, 16),	/* MSIOF0_SS2 */
		[17] = RCAR_GP_PIN(0, 17),	/* IRQ0 */
		[18] = RCAR_GP_PIN(0, 18),	/* IRQ1 */
		[19] = RCAR_GP_PIN(0, 19),	/* IRQ2 */
		[20] = RCAR_GP_PIN(0, 20),	/* IRQ3 */
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
	{ PFC_BIAS_REG(0xc0, 0xe0) {         /* PUEN1, PUD1 */
		[0]  = RCAR_GP_PIN(1,  0),	/* GP1_00 */
		[1]  = RCAR_GP_PIN(1,  1),	/* GP1_01 */
		[2]  = RCAR_GP_PIN(1,  2),	/* GP1_02 */
		[3]  = RCAR_GP_PIN(1,  3),	/* GP1_03 */
		[4]  = RCAR_GP_PIN(1,  4),	/* GP1_04 */
		[5]  = RCAR_GP_PIN(1,  5),	/* GP1_05 */
		[6]  = RCAR_GP_PIN(1,  6),	/* GP1_06 */
		[7]  = RCAR_GP_PIN(1,  7),	/* GP1_07 */
		[8]  = RCAR_GP_PIN(1,  8),	/* GP1_08 */
		[9]  = RCAR_GP_PIN(1,  9),	/* GP1_09 */
		[10] = RCAR_GP_PIN(1, 10),	/* GP1_10 */
		[11] = RCAR_GP_PIN(1, 11),	/* GP1_11 */
		[12] = RCAR_GP_PIN(1, 12),	/* MMC_SD_CLK */
		[13] = RCAR_GP_PIN(1, 13),	/* MMC_SD_D0 */
		[14] = RCAR_GP_PIN(1, 14),	/* MMC_SD_D1 */
		[15] = RCAR_GP_PIN(1, 15),	/* MMC_SD_D2 */
		[16] = RCAR_GP_PIN(1, 16),	/* MMC_SD_D3 */
		[17] = RCAR_GP_PIN(1, 17),	/* MMC_D5 */
		[18] = RCAR_GP_PIN(1, 18),	/* MMC_D4 */
		[19] = RCAR_GP_PIN(1, 19),	/* MMC_D6 */
		[20] = RCAR_GP_PIN(1, 20),	/* MMC_DS */
		[21] = RCAR_GP_PIN(1, 21),	/* MMC_D7 */
		[22] = RCAR_GP_PIN(1, 22),	/* MMC_SD_CMD */
		[23] = RCAR_GP_PIN(1, 23),	/* SD_CD */
		[24] = RCAR_GP_PIN(1, 24),	/* SD_WP */
		[25] = PIN_NONE,
		[26] = PIN_NONE,
		[27] = PIN_NONE,
		[28] = PIN_NONE,
		[29] = PIN_NONE,
		[30] = PIN_NONE,
		[31] = PIN_NONE,
	} },
	{ PFC_BIAS_REG(0xc0, 0xe0) {         /* PUEN2, PUD2 */
		[0]  = RCAR_GP_PIN(2,  0),	/* RPC_INT_N */
		[1]  = RCAR_GP_PIN(2,  1),	/* RPC_WP_N */
		[2]  = RCAR_GP_PIN(2,  2),	/* RPC_RESET_N */
		[3]  = RCAR_GP_PIN(2,  3),	/* QSPI1_SSL */
		[4]  = RCAR_GP_PIN(2,  4),	/* QSPI1_IO3 */
		[5]  = RCAR_GP_PIN(2,  5),	/* QSPI1_MISO_IO1 */
		[6]  = RCAR_GP_PIN(2,  6),	/* QSPI1_IO2 */
		[7]  = RCAR_GP_PIN(2,  7),	/* QSPI1_MOSI_IO0 */
		[8]  = RCAR_GP_PIN(2,  8),	/* QSPI1_SPCLK */
		[9]  = RCAR_GP_PIN(2,  9),	/* QSPI0_MOSI_IO0 */
		[10] = RCAR_GP_PIN(2, 10),	/* QSPI0_SPCLK */
		[11] = RCAR_GP_PIN(2, 11),	/* QSPI0_IO2 */
		[12] = RCAR_GP_PIN(2, 12),	/* QSPI0_MISO_IO1 */
		[13] = RCAR_GP_PIN(2, 13),	/* QSPI0_SSL */
		[14] = RCAR_GP_PIN(2, 14),	/* QSPI0_IO3 */
		[15] = RCAR_GP_PIN(2, 15),	/* PCIE0_CLKREQ_N */
		[16] = RCAR_GP_PIN(2, 16),	/* PCIE1_CLKREQ_N */
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
	{ PFC_BIAS_REG(0xc0, 0xe0) {         /* PUEN3, PUD3 */
		[0]  = RCAR_GP_PIN(3,  0),	/* TSN1_MDIO_B */
		[1]  = RCAR_GP_PIN(3,  1),	/* TSN2_MDIO_B */
		[2]  = RCAR_GP_PIN(3,  2),	/* TSN0_MDIO_B */
		[3]  = RCAR_GP_PIN(3,  3),	/* TSN2_MDC_B */
		[4]  = RCAR_GP_PIN(3,  4),	/* TSN0_MDC_B */
		[5]  = RCAR_GP_PIN(3,  5),	/* TSN1_MDC_B */
		[6]  = RCAR_GP_PIN(3,  6),	/* TSN1_LINK_B */
		[7]  = RCAR_GP_PIN(3,  7),	/* TSN2_LINK_B */
		[8]  = RCAR_GP_PIN(3,  8),	/* TSN0_LINK_B */
		[9]  = RCAR_GP_PIN(3,  9),	/* TSN2_PHY_INT_B */
		[10] = RCAR_GP_PIN(3, 10),	/* TSN0_PHY_INT_B */
		[11] = RCAR_GP_PIN(3, 11),	/* TSN1_PHY_INT_B */
		[12] = RCAR_GP_PIN(3, 12),	/* TSN0_MAGIC_B */
		[13] = RCAR_GP_PIN(3, 13),	/* TSN1_AVTP_PPS */
		[14] = RCAR_GP_PIN(3, 14),	/* TSN1_AVTP_MATCH_B */
		[15] = RCAR_GP_PIN(3, 15),	/* TSN1_AVTP_CAPTURE_B */
		[16] = RCAR_GP_PIN(3, 16),	/* TSN0_AVTP_PPS */
		[17] = RCAR_GP_PIN(3, 17),	/* TSN0_AVTP_MATCH_B */
		[18] = RCAR_GP_PIN(3, 18),	/* TSN0_AVTP_CAPTURE_B */
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
	{ PFC_BIAS_REG(0xc0, 0xe0) {         /* PUEN4, PUD4 */
		[0]  = RCAR_GP_PIN(4,  0),	/* GP4_00 */
		[1]  = RCAR_GP_PIN(4,  1),	/* GP4_01 */
		[2]  = RCAR_GP_PIN(4,  2),	/* GP4_02 */
		[3]  = RCAR_GP_PIN(4,  3),	/* GP4_03 */
		[4]  = RCAR_GP_PIN(4,  4),	/* GP4_04 */
		[5]  = RCAR_GP_PIN(4,  5),	/* GP4_05 */
		[6]  = RCAR_GP_PIN(4,  6),	/* GP4_06 */
		[7]  = RCAR_GP_PIN(4,  7),	/* GP4_07 */
		[8]  = RCAR_GP_PIN(4,  8),	/* GP4_08 */
		[9]  = RCAR_GP_PIN(4,  9),	/* GP4_09 */
		[10] = RCAR_GP_PIN(4, 10),	/* GP4_10 */
		[11] = RCAR_GP_PIN(4, 11),	/* GP4_11 */
		[12] = RCAR_GP_PIN(4, 12),	/* GP4_12 */
		[13] = RCAR_GP_PIN(4, 13),	/* GP4_13 */
		[14] = RCAR_GP_PIN(4, 14),	/* GP4_14 */
		[15] = RCAR_GP_PIN(4, 15),	/* GP4_15 */
		[16] = RCAR_GP_PIN(4, 16),	/* GP4_16 */
		[17] = RCAR_GP_PIN(4, 17),	/* GP4_17 */
		[18] = RCAR_GP_PIN(4, 18),	/* GP4_18 */
		[19] = RCAR_GP_PIN(4, 19),	/* GP4_19 */
		[20] = RCAR_GP_PIN(4, 20),	/* MSPI0SC */
		[21] = RCAR_GP_PIN(4, 21),	/* MSPI0SI */
		[22] = RCAR_GP_PIN(4, 22),	/* MSPI0SO/MSPI0DCS */
		[23] = RCAR_GP_PIN(4, 23),	/* MSPI0CSS1 */
		[24] = RCAR_GP_PIN(4, 24),	/* MSPI0CSS0 */
		[25] = RCAR_GP_PIN(4, 25),	/* MSPI1SI */
		[26] = RCAR_GP_PIN(4, 26),	/* MSPI1SO/MSPI1DCS */
		[27] = RCAR_GP_PIN(4, 27),	/* MSPI1CSS0 */
		[28] = RCAR_GP_PIN(4, 28),	/* MSPI1SC */
		[29] = RCAR_GP_PIN(4, 29),	/* MSPI1CSS2 */
		[30] = RCAR_GP_PIN(4, 30),	/* MSPI1CSS1 */
		[31] = PIN_NONE,
	} },
	{ PFC_BIAS_REG(0xc0, 0xe0) {         /* PUEN5, PUD5 */
		[0]  = RCAR_GP_PIN(5,  0),	/* RIIC0SCL */
		[1]  = RCAR_GP_PIN(5,  1),	/* RIIC0SDA */
		[2]  = RCAR_GP_PIN(5,  2),	/* ETNB0MD */
		[3]  = RCAR_GP_PIN(5,  3),	/* ETNB0WOL */
		[4]  = RCAR_GP_PIN(5,  4),	/* ETNB0LINKSTA */
		[5]  = RCAR_GP_PIN(5,  5),	/* ETNB0MDC */
		[6]  = RCAR_GP_PIN(5,  6),	/* ETNB0RXER */
		[7]  = RCAR_GP_PIN(5,  7),	/* ETNB0RXD3 */
		[8]  = RCAR_GP_PIN(5,  8),	/* ETNB0RXD1 */
		[9]  = RCAR_GP_PIN(5,  9),	/* ETNB0RXD2 */
		[10] = RCAR_GP_PIN(5, 10),	/* ETNB0RXDV */
		[11] = RCAR_GP_PIN(5, 11),	/* ETNB0RXD0 */
		[12] = RCAR_GP_PIN(5, 12),	/* ETNB0RXCLK */
		[13] = RCAR_GP_PIN(5, 13),	/* ETNB0TXER */
		[14] = RCAR_GP_PIN(5, 14),	/* ETNB0TXD3 */
		[15] = RCAR_GP_PIN(5, 15),	/* ETNB0TXCLK */
		[16] = RCAR_GP_PIN(5, 16),	/* ETNB0TXD1 */
		[17] = RCAR_GP_PIN(5, 17),	/* ETNB0TXD2 */
		[18] = RCAR_GP_PIN(5, 18),	/* ETNB0TXEN */
		[19] = RCAR_GP_PIN(5, 19),	/* ETNB0TXD0 */
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
	{ PFC_BIAS_REG(0xc0, 0xe0) {         /* PUEN6, PUD6 */
		[0]  = RCAR_GP_PIN(6,  0),	/* RLIN37TX */
		[1]  = RCAR_GP_PIN(6,  1),	/* RLIN37RX/INTP23 */
		[2]  = RCAR_GP_PIN(6,  2),	/* RLIN36TX */
		[3]  = RCAR_GP_PIN(6,  3),	/* RLIN36RX/INTP22 */
		[4]  = RCAR_GP_PIN(6,  4),	/* RLIN35TX */
		[5]  = RCAR_GP_PIN(6,  5),	/* RLIN35RX/INTP21 */
		[6]  = RCAR_GP_PIN(6,  6),	/* RLIN34TX */
		[7]  = RCAR_GP_PIN(6,  7),	/* RLIN34RX/INTP20 */
		[8]  = RCAR_GP_PIN(6,  8),	/* RLIN33TX */
		[9]  = RCAR_GP_PIN(6,  9),	/* RLIN33RX/INTP19 */
		[10] = RCAR_GP_PIN(6, 10),	/* RLIN32TX */
		[11] = RCAR_GP_PIN(6, 11),	/* RLIN32RX/INTP18 */
		[12] = RCAR_GP_PIN(6, 12),	/* RLIN31TX */
		[13] = RCAR_GP_PIN(6, 13),	/* RLIN31RX/INTP17 */
		[14] = RCAR_GP_PIN(6, 14),	/* RLIN30TX */
		[15] = RCAR_GP_PIN(6, 15),	/* RLIN30RX/INTP16 */
		[16] = RCAR_GP_PIN(6, 16),	/* INTP37 */
		[17] = RCAR_GP_PIN(6, 17),	/* INTP36 */
		[18] = RCAR_GP_PIN(6, 18),	/* INTP35 */
		[19] = RCAR_GP_PIN(6, 19),	/* INTP34 */
		[20] = RCAR_GP_PIN(6, 20),	/* INTP33 */
		[21] = RCAR_GP_PIN(6, 21),	/* INTP32 */
		[22] = RCAR_GP_PIN(6, 22),	/* NMI1 */
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
	{ PFC_BIAS_REG(0xc0, 0xe0) {         /* PUEN7, PUD7 */
		[0]  = RCAR_GP_PIN(7,  0),	/* CAN0TX */
		[1]  = RCAR_GP_PIN(7,  1),	/* CAN0RX/INTP0 */
		[2]  = RCAR_GP_PIN(7,  2),	/* CAN1TX */
		[3]  = RCAR_GP_PIN(7,  3),	/* CAN1RX/INTP1 */
		[4]  = RCAR_GP_PIN(7,  4),	/* CAN2TX */
		[5]  = RCAR_GP_PIN(7,  5),	/* CAN2RX/INTP2 */
		[6]  = RCAR_GP_PIN(7,  6),	/* CAN3TX */
		[7]  = RCAR_GP_PIN(7,  7),	/* CAN3RX/INTP3 */
		[8]  = RCAR_GP_PIN(7,  8),	/* CAN4TX */
		[9]  = RCAR_GP_PIN(7,  9),	/* CAN4RX/INTP4 */
		[10] = RCAR_GP_PIN(7, 10),	/* CAN5TX */
		[11] = RCAR_GP_PIN(7, 11),	/* CAN5RX/INTP5 */
		[12] = RCAR_GP_PIN(7, 12),	/* CAN6TX */
		[13] = RCAR_GP_PIN(7, 13),	/* CAN6RX/INTP6 */
		[14] = RCAR_GP_PIN(7, 14),	/* CAN7TX */
		[15] = RCAR_GP_PIN(7, 15),	/* CAN7RX/INTP7 */
		[16] = RCAR_GP_PIN(7, 16),	/* CAN8TX */
		[17] = RCAR_GP_PIN(7, 17),	/* CAN8RX/INTP8 */
		[18] = RCAR_GP_PIN(7, 18),	/* CAN9TX */
		[19] = RCAR_GP_PIN(7, 19),	/* CAN9RX/INTP9 */
		[20] = RCAR_GP_PIN(7, 20),	/* CAN10TX */
		[21] = RCAR_GP_PIN(7, 21),	/* CAN10RX/INTP10 */
		[22] = RCAR_GP_PIN(7, 22),	/* CAN11TX */
		[23] = RCAR_GP_PIN(7, 23),	/* CAN11RX/INTP11 */
		[24] = RCAR_GP_PIN(7, 24),	/* CAN12TX */
		[25] = RCAR_GP_PIN(7, 25),	/* CAN12RX/INTP12 */
		[26] = RCAR_GP_PIN(7, 26),	/* CAN13TX */
		[27] = RCAR_GP_PIN(7, 27),	/* CAN13RX/INTP13 */
		[28] = RCAR_GP_PIN(7, 28),	/* CAN14TX */
		[29] = RCAR_GP_PIN(7, 29),	/* CAN14RX/INTP14 */
		[30] = RCAR_GP_PIN(7, 30),	/* CAN15TX */
		[31] = RCAR_GP_PIN(7, 31),	/* CAN15RX/INTP15 */
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
	if (RCAR_IS_GP_PIN(pin) == false) {
		return -EINVAL;
	}

	*reg_index = pin / 32;

	return 0;
}
