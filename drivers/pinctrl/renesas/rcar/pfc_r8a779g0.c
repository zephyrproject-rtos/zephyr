/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <errno.h>
#include <pinctrl_soc.h>
#include <zephyr/dt-bindings/pinctrl/renesas/pinctrl-r8a779g0.h>

/* Set the size and bit offset of each pin in DRVxCTRLy registers */
const struct pfc_drive_reg pfc_drive_regs[] = {
	/* DRV0CTRL0 */
	{ 0x80, {
		{RCAR_GP_PIN(0, 7), 28, 3}, /* MSIOF5_SS2 */
		{RCAR_GP_PIN(0, 6), 24, 3}, /* IRQ0 */
		{RCAR_GP_PIN(0, 5), 20, 3}, /* IRQ1 */
		{RCAR_GP_PIN(0, 4), 16, 3}, /* IRQ2 */
		{RCAR_GP_PIN(0, 3), 12, 3}, /* IRQ3 */
		{RCAR_GP_PIN(0, 2), 8, 3},  /* GP0_02 */
		{RCAR_GP_PIN(0, 1), 4, 3},  /* GP0_01 */
		{RCAR_GP_PIN(0, 0), 0, 3},  /* GP0_00 */
	}},
	/* DRV1CTRL0 */
	{ 0x84, {
		{RCAR_GP_PIN(0, 15), 28, 3}, /* MSIOF2_SYNC */
		{RCAR_GP_PIN(0, 14), 24, 3}, /* MSIOF2_SS1 */
		{RCAR_GP_PIN(0, 13), 20, 3}, /* MSIOF2_SS2 */
		{RCAR_GP_PIN(0, 12), 16, 3}, /* MSIOF5_RXD */
		{RCAR_GP_PIN(0, 11), 12, 3}, /* MSIOF5_SCK */
		{RCAR_GP_PIN(0, 10), 8, 3},  /* MSIOF5_TXD */
		{RCAR_GP_PIN(0, 9), 4, 3},   /* MSIOF5_SYNC */
		{RCAR_GP_PIN(0, 8), 0, 3},   /* MSIOF5_SS1 */
	}},
	/* DRV2CTRL0 */
	{ 0x88, {
		{RCAR_GP_PIN(0, 18), 8, 3}, /* MSIOF2_RXD */
		{RCAR_GP_PIN(0, 17), 4, 3}, /* MSIOF2_SCK */
		{RCAR_GP_PIN(0, 16), 0, 3}, /* MSIOF2_TXD */
	}},
	/* DRV3CTRL0 is empty */
	/* DRV0CTRL1 */
	{ 0x80, {
		{RCAR_GP_PIN(1, 7), 28, 3}, /* MSIOF0_SS1 */
		{RCAR_GP_PIN(1, 6), 24, 3}, /* MSIOF0_SS2 */
		{RCAR_GP_PIN(1, 5), 20, 3}, /* MSIOF1_RXD */
		{RCAR_GP_PIN(1, 4), 16, 3}, /* MSIOF1_TXD */
		{RCAR_GP_PIN(1, 3), 12, 3}, /* MSIOF1_SCK */
		{RCAR_GP_PIN(1, 2), 8, 3},  /* MSIOF1_SYNC */
		{RCAR_GP_PIN(1, 1), 4, 3},  /* MSIOF1_SS1 */
		{RCAR_GP_PIN(1, 0), 0, 3},  /* MSIOF1_SS2 */
	}},
	/* DRV1CTRL1 */
	{ 0x84, {
		{RCAR_GP_PIN(1, 15), 28, 3}, /* HSCK0 */
		{RCAR_GP_PIN(1, 14), 24, 3}, /* HRTS0 */
		{RCAR_GP_PIN(1, 13), 20, 3}, /* HCTS0 */
		{RCAR_GP_PIN(1, 12), 16, 3}, /* HTX0 */
		{RCAR_GP_PIN(1, 11), 12, 3}, /* MSIOF0_RXD */
		{RCAR_GP_PIN(1, 10), 8, 3},  /* MSIOF0_SCK */
		{RCAR_GP_PIN(1, 9), 4, 3},   /* MSIOF0_TXD */
		{RCAR_GP_PIN(1, 8), 0, 3},   /* MSIOF0_SYNC */
	}},
	/* DRV2CTRL1 */
	{ 0x88, {
		{RCAR_GP_PIN(1, 23), 28, 3}, /* GP1_23 */
		{RCAR_GP_PIN(1, 22), 24, 3}, /* AUDIO_CLKIN */
		{RCAR_GP_PIN(1, 21), 20, 3}, /* AUDIO_CLKOUT */
		{RCAR_GP_PIN(1, 20), 16, 3}, /* SSI_SD */
		{RCAR_GP_PIN(1, 19), 12, 3}, /* SSI_WS */
		{RCAR_GP_PIN(1, 18), 8, 3},  /* SSI_SCK */
		{RCAR_GP_PIN(1, 17), 4, 3},  /* SCIF_CLK */
		{RCAR_GP_PIN(1, 16), 0, 3},  /* HRX0 */
	}},
	/* DRV3CTRL1 */
	{ 0x8c, {
		{RCAR_GP_PIN(1, 28), 16, 3}, /* HTX3 */
		{RCAR_GP_PIN(1, 27), 12, 3}, /* HCTS3# */
		{RCAR_GP_PIN(1, 26), 8, 3},  /* HRTS3# */
		{RCAR_GP_PIN(1, 25), 4, 3},  /* HSCK3 */
		{RCAR_GP_PIN(1, 24), 0, 3},  /* HRX3 */
	}},
	/* DRV0CTRL2 */
	{ 0x80, {
		{RCAR_GP_PIN(2, 7), 28, 2}, /* TPU0TO1 */
		{RCAR_GP_PIN(2, 6), 24, 2}, /* FXR_TXDB */
		{RCAR_GP_PIN(2, 5), 20, 2}, /* FXR_TXENB */
		{RCAR_GP_PIN(2, 4), 16, 2}, /* RXDB_EXTFXR */
		{RCAR_GP_PIN(2, 3), 12, 2}, /* CLK_EXTFXR */
		{RCAR_GP_PIN(2, 2), 8, 2},  /* RXDA_EXTFXR */
		{RCAR_GP_PIN(2, 1), 4, 2},  /* FXR_TXENA */
		{RCAR_GP_PIN(2, 0), 0, 2},  /* FXR_TXDA */
	}},
	/* DRV1CTRL2 */
	{ 0x84, {
		{RCAR_GP_PIN(2, 15), 28, 3}, /* CANFD3_RX */
		{RCAR_GP_PIN(2, 14), 24, 2}, /* CANFD3_TX */
		{RCAR_GP_PIN(2, 13), 20, 2}, /* CANFD2_RX */
		{RCAR_GP_PIN(2, 12), 16, 2}, /* CANFD2_TX */
		{RCAR_GP_PIN(2, 11), 12, 2}, /* CANFD0_RX */
		{RCAR_GP_PIN(2, 10), 8, 2},  /* CANFD0_TX */
		{RCAR_GP_PIN(2, 9), 4, 2},   /* CAN_CLK */
		{RCAR_GP_PIN(2, 8), 0, 2},   /* TPU0TO0 */
	}},
	/* DRV2CTRL2 */
	{ 0x88, {
		{RCAR_GP_PIN(2, 19), 12, 3}, /* CANFD7_RX */
		{RCAR_GP_PIN(2, 18), 8, 3},  /* CANFD7_TX */
		{RCAR_GP_PIN(2, 17), 4, 3},  /* CANFD4_RX */
		{RCAR_GP_PIN(2, 16), 0, 3},  /* CANFD4_TX */
	}},
	/* DRV3CTRL2 is empty */
	/* DRV0CTRL3 */
	{ 0x80, {
		{RCAR_GP_PIN(3, 7), 28, 3}, /* MMC_D4 */
		{RCAR_GP_PIN(3, 6), 24, 3}, /* MMC_D5 */
		{RCAR_GP_PIN(3, 5), 20, 3}, /* MMC_SD_D3 */
		{RCAR_GP_PIN(3, 4), 16, 3}, /* MMC_DS */
		{RCAR_GP_PIN(3, 3), 12, 3}, /* MMC_SD_CLK */
		{RCAR_GP_PIN(3, 2), 8, 3},  /* MMC_SD_D2 */
		{RCAR_GP_PIN(3, 1), 4, 3},  /* MMC_SD_D0 */
		{RCAR_GP_PIN(3, 0), 0, 3},  /* MMC_SD_D1 */
	}},
	/* DRV1CTRL3 */
	{ 0x84, {
		{RCAR_GP_PIN(3, 15), 28, 2}, /* QSPI0_SSL */
		{RCAR_GP_PIN(3, 14), 24, 2}, /* IPC_CLKOUT */
		{RCAR_GP_PIN(3, 13), 20, 2}, /* IPC_CLKIN */
		{RCAR_GP_PIN(3, 12), 16, 3}, /* SD_WP */
		{RCAR_GP_PIN(3, 11), 12, 3}, /* SD_CD */
		{RCAR_GP_PIN(3, 10), 8, 3},  /* MMC_SD_CMD */
		{RCAR_GP_PIN(3, 9), 4, 3},   /* MMC_D6 */
		{RCAR_GP_PIN(3, 8), 0, 3},   /* MMC_D7 */
	}},
	/* DRV2CTRL3 */
	{ 0x88, {
		{RCAR_GP_PIN(3, 23), 28, 2}, /* QSPI1_MISO_IO1 */
		{RCAR_GP_PIN(3, 22), 24, 2}, /* QSPI1_SPCLK */
		{RCAR_GP_PIN(3, 21), 20, 2}, /* QSPI1_MOSI_IO0 */
		{RCAR_GP_PIN(3, 20), 16, 2}, /* QSPI0_SPCLK */
		{RCAR_GP_PIN(3, 19), 12, 2}, /* QSPI0_MOSI_IO0 */
		{RCAR_GP_PIN(3, 18), 8, 2},  /* QSPI0_MISO_IO1 */
		{RCAR_GP_PIN(3, 17), 4, 2},  /* QSPI0_IO2 */
		{RCAR_GP_PIN(3, 16), 0, 2},  /* QSPI0_IO3 */
	}},
	/* DRV3CTRL3 */
	{ 0x8c, {
		{RCAR_GP_PIN(3, 29), 20, 2}, /* RPC_INT */
		{RCAR_GP_PIN(3, 28), 16, 2}, /* RPC_WP */
		{RCAR_GP_PIN(3, 27), 12, 2}, /* RPC_RESET */
		{RCAR_GP_PIN(3, 26), 8, 2},  /* QSPI1_IO3 */
		{RCAR_GP_PIN(3, 25), 4, 2},  /* QSPI1_SSL */
		{RCAR_GP_PIN(3, 24), 0, 2},  /* QSPI1_IO2 */
	}},
	/* DRV0CTRL4 */
	{ 0x80, {
		{RCAR_GP_PIN(4, 7), 28, 3}, /* TSN0_RX_CTL */
		{RCAR_GP_PIN(4, 6), 24, 3}, /* TSN0_AVTP_CAPTURE */
		{RCAR_GP_PIN(4, 5), 20, 3}, /* TSN0_AVTP_MATCH */
		{RCAR_GP_PIN(4, 4), 16, 3}, /* TSN0_LINK */
		{RCAR_GP_PIN(4, 3), 12, 3}, /* TSN0_PHY_INT */
		{RCAR_GP_PIN(4, 2), 8, 3},  /* TSN0_AVTP_PPS1 */
		{RCAR_GP_PIN(4, 1), 4, 3},  /* TSN0_MDC */
		{RCAR_GP_PIN(4, 0), 0, 3},  /* TSN0_MDIO */
	}},
	/* DRV1CTRL4 */
	{ 0x84, {
		{RCAR_GP_PIN(4, 15), 28, 3}, /* TSN0_TD0 */
		{RCAR_GP_PIN(4, 14), 24, 3}, /* TSN0_TD1 */
		{RCAR_GP_PIN(4, 13), 20, 3}, /* TSN0_RD1 */
		{RCAR_GP_PIN(4, 12), 16, 3}, /* TSN0_TXC */
		{RCAR_GP_PIN(4, 11), 12, 3}, /* TSN0_RXC */
		{RCAR_GP_PIN(4, 10), 8, 3},  /* TSN0_RD0 */
		{RCAR_GP_PIN(4, 9), 4, 3},   /* TSN0_TX_CTL */
		{RCAR_GP_PIN(4, 8), 0, 3},   /* TSN0_AVTP_PPS0 */
	}},
	/* DRV2CTRL4 */
	{ 0x88, {
		{RCAR_GP_PIN(4, 23), 28, 3}, /* AVS0 */
		{RCAR_GP_PIN(4, 22), 24, 3}, /* PCIE1_CLKREQ */
		{RCAR_GP_PIN(4, 21), 20, 3}, /* PCIE0_CLKREQ */
		{RCAR_GP_PIN(4, 20), 16, 3}, /* TSN0_TXCREFCLK */
		{RCAR_GP_PIN(4, 19), 12, 3}, /* TSN0_TD2 */
		{RCAR_GP_PIN(4, 18), 8, 3},  /* TSN0_TD3 */
		{RCAR_GP_PIN(4, 17), 4, 3},  /* TSN0_RD2 */
		{RCAR_GP_PIN(4, 16), 0, 3},  /* TSN0_RD3 */
	}},
	/* DRV3CTRL4 */
	{ 0x8c, {
		{RCAR_GP_PIN(4, 24), 0, 3}, /* AVS1 */
	}},
	/* DRV0CTRL5 */
	{ 0x80, {
		{RCAR_GP_PIN(5, 7), 28, 3}, /* AVB2_TXCREFCLK */
		{RCAR_GP_PIN(5, 6), 24, 3}, /* AVB2_MDC */
		{RCAR_GP_PIN(5, 5), 20, 3}, /* AVB2_MAGIC */
		{RCAR_GP_PIN(5, 4), 16, 3}, /* AVB2_PHY_INT */
		{RCAR_GP_PIN(5, 3), 12, 3}, /* AVB2_LINK */
		{RCAR_GP_PIN(5, 2), 8, 3},  /* AVB2_AVTP_MATCH */
		{RCAR_GP_PIN(5, 1), 4, 3},  /* AVB2_AVTP_CAPTURE */
		{RCAR_GP_PIN(5, 0), 0, 3},  /* AVB2_AVTP_PPS */
	}},
	/* DRV1CTRL5 */
	{ 0x84, {
		{RCAR_GP_PIN(5, 15), 28, 3}, /* AVB2_TD0 */
		{RCAR_GP_PIN(5, 14), 24, 3}, /* AVB2_RD1 */
		{RCAR_GP_PIN(5, 13), 20, 3}, /* AVB2_RD2 */
		{RCAR_GP_PIN(5, 12), 16, 3}, /* AVB2_TD1 */
		{RCAR_GP_PIN(5, 11), 12, 3}, /* AVB2_TD2 */
		{RCAR_GP_PIN(5, 10), 8, 3},  /* AVB2_MDIO */
		{RCAR_GP_PIN(5, 9), 4, 3},   /* AVB2_RD3 */
		{RCAR_GP_PIN(5, 8), 0, 3},   /* AVB2_TD3 */
	}},
	/* DRV2CTRL5 */
	{ 0x88, {
		{RCAR_GP_PIN(5, 20), 16, 3}, /* AVB2_RX_CTL */
		{RCAR_GP_PIN(5, 19), 12, 3}, /* AVB2_TX_CTL */
		{RCAR_GP_PIN(5, 18), 8, 3},  /* AVB2_RXC */
		{RCAR_GP_PIN(5, 17), 4, 3},  /* AVB2_RD0 */
		{RCAR_GP_PIN(5, 16), 0, 3},  /* AVB2_TXC */
	}},
	/* DRV3CTRL5 is empty */
	/* DRV0CTRL6 */
	{ 0x80, {
		{RCAR_GP_PIN(6, 7), 28, 3}, /* AVB1_TX_CTL */
		{RCAR_GP_PIN(6, 6), 24, 3}, /* AVB1_TXC */
		{RCAR_GP_PIN(6, 5), 20, 3}, /* AVB1_AVTP_MATCH */
		{RCAR_GP_PIN(6, 4), 16, 3}, /* AVB1_LINK */
		{RCAR_GP_PIN(6, 3), 12, 3}, /* AVB1_PHY_INT */
		{RCAR_GP_PIN(6, 2), 8, 3},  /* AVB1_MDC */
		{RCAR_GP_PIN(6, 1), 4, 3},  /* AVB1_MAGIC */
		{RCAR_GP_PIN(6, 0), 0, 3},  /* AVB1_MDIO */
	}},
	/* DRV1CTRL6 */
	{ 0x84, {
		{RCAR_GP_PIN(6, 15), 28, 3}, /* AVB1_RD0 */
		{RCAR_GP_PIN(6, 14), 24, 3}, /* AVB1_RD1 */
		{RCAR_GP_PIN(6, 13), 20, 3}, /* AVB1_TD0 */
		{RCAR_GP_PIN(6, 12), 16, 3}, /* AVB1_TD1 */
		{RCAR_GP_PIN(6, 11), 12, 3}, /* AVB1_AVTP_CAPTURE */
		{RCAR_GP_PIN(6, 10), 8, 3},  /* AVB1_AVTP_PPS */
		{RCAR_GP_PIN(6, 9), 4, 3},   /* AVB1_RX_CTL */
		{RCAR_GP_PIN(6, 8), 0, 3},   /* AVB1_RXC */
	}},
	/* DRV2CTRL6 */
	{ 0x88, {
		{RCAR_GP_PIN(6, 20), 16, 3}, /* AVB1_TXCREFCLK */
		{RCAR_GP_PIN(6, 19), 12, 3}, /* AVB1_RD3 */
		{RCAR_GP_PIN(6, 18), 8, 3},  /* AVB1_TD3 */
		{RCAR_GP_PIN(6, 17), 4, 3},  /* AVB1_RD2 */
		{RCAR_GP_PIN(6, 16), 0, 3},  /* AVB1_TD2 */
	}},
	/* DRV0CTRL7 */
	{ 0x80, {
		{RCAR_GP_PIN(7, 7), 28, 3}, /* AVB0_TD1 */
		{RCAR_GP_PIN(7, 6), 24, 3}, /* AVB0_TD2 */
		{RCAR_GP_PIN(7, 5), 20, 3}, /* AVB0_PHY_INT */
		{RCAR_GP_PIN(7, 4), 16, 3}, /* AVB0_LINK */
		{RCAR_GP_PIN(7, 3), 12, 3}, /* AVB0_TD3 */
		{RCAR_GP_PIN(7, 2), 8, 3},  /* AVB0_AVTP_MATCH */
		{RCAR_GP_PIN(7, 1), 4, 3},  /* AVB0_AVTP_CAPTURE */
		{RCAR_GP_PIN(7, 0), 0, 3},  /* AVB0_AVTP_PPS */
	}},
	/* DRV1CTRL7 */
	{ 0x84, {
		{RCAR_GP_PIN(7, 15), 28, 3}, /* AVB0_TXC */
		{RCAR_GP_PIN(7, 14), 24, 3}, /* AVB0_MDIO */
		{RCAR_GP_PIN(7, 13), 20, 3}, /* AVB0_MDC */
		{RCAR_GP_PIN(7, 12), 16, 3}, /* AVB0_RD2 */
		{RCAR_GP_PIN(7, 11), 12, 3}, /* AVB0_TD0 */
		{RCAR_GP_PIN(7, 10), 8, 3},  /* AVB0_MAGIC */
		{RCAR_GP_PIN(7, 9), 4, 3},   /* AVB0_TXCREFCLK */
		{RCAR_GP_PIN(7, 8), 0, 3},   /* AVB0_RD3 */
	}},
	/* DRV2CTRL7 */
	{ 0x88, {
		{RCAR_GP_PIN(7, 20), 16, 3}, /* AVB0_RX_CTL */
		{RCAR_GP_PIN(7, 19), 12, 3}, /* AVB0_RXC */
		{RCAR_GP_PIN(7, 18), 8, 3},  /* AVB0_RD0 */
		{RCAR_GP_PIN(7, 17), 4, 3},  /* AVB0_RD1 */
		{RCAR_GP_PIN(7, 16), 0, 3},  /* AVB0_TX_CTL */
	}},
	/* DRV0CTRL8 */
	{ 0x80, {
		{RCAR_GP_PIN(8, 7), 28, 3}, /* SDA3 */
		{RCAR_GP_PIN(8, 6), 24, 3}, /* SCL3 */
		{RCAR_GP_PIN(8, 5), 20, 3}, /* SDA2 */
		{RCAR_GP_PIN(8, 4), 16, 3}, /* SCL2 */
		{RCAR_GP_PIN(8, 3), 12, 3}, /* SDA1 */
		{RCAR_GP_PIN(8, 2), 8, 3},  /* SCL1 */
		{RCAR_GP_PIN(8, 1), 4, 3},  /* SDA0 */
		{RCAR_GP_PIN(8, 0), 0, 3},  /* SCL0 */
	}},
	/* DRV1CTRL8 */
	{ 0x84, {
		{RCAR_GP_PIN(8, 13), 20, 3}, /* GP8_13 */
		{RCAR_GP_PIN(8, 12), 16, 3}, /* GP8_12 */
		{RCAR_GP_PIN(8, 11), 12, 3}, /* SDA5 */
		{RCAR_GP_PIN(8, 10), 8, 3},  /* SCL5 */
		{RCAR_GP_PIN(8, 9), 4, 3},   /* SDA4 */
		{RCAR_GP_PIN(8, 8), 0, 3},   /* SCL4 */
	}},
	/* DRV0CTRLSYS */
	{ 0x80, {
		{RCAR_GP_PIN(8, 0), 0, 3}, /* PRESETOUT0# */
	}},
	/* DRV1CTRLSYS */
	{ 0x84, {
		{PIN_NONE, 12, 2}, /* DCURDY#_LPDCLKOUT */
		{PIN_NONE, 8, 2},  /* DCUTDO_LPDO */
		{PIN_NONE, 0, 2},  /* DCUTMS */
	}},
	{},
};

#define PFC_BIAS_REG(r1, r2) .puen = r1, .pud = r2, .pins =

/* Set the bit position of a pin in PUENn, PUDn registers */
const struct pfc_bias_reg pfc_bias_regs[] = {
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN0, PUD0 */
		[0] = RCAR_GP_PIN(0, 0),   /* GP0_00 */
		[1] = RCAR_GP_PIN(0, 1),   /* GP0_01 */
		[2] = RCAR_GP_PIN(0, 2),   /* GP0_02 */
		[3] = RCAR_GP_PIN(0, 3),   /* IRQ3 */
		[4] = RCAR_GP_PIN(0, 4),   /* IRQ2 */
		[5] = RCAR_GP_PIN(0, 5),   /* IRQ1 */
		[6] = RCAR_GP_PIN(0, 6),   /* IRQ0 */
		[7] = RCAR_GP_PIN(0, 7),   /* MSIOF5_SS2 */
		[8] = RCAR_GP_PIN(0, 8),   /* MSIOF5_SS1 */
		[9] = RCAR_GP_PIN(0, 9),   /* MSIOF5_SYNC */
		[10] = RCAR_GP_PIN(0, 10), /* MSIOF5_TXD */
		[11] = RCAR_GP_PIN(0, 11), /* MSIOF5_SCK */
		[12] = RCAR_GP_PIN(0, 12), /* MSIOF5_RXD */
		[13] = RCAR_GP_PIN(0, 13), /* MSIOF2_SS2 */
		[14] = RCAR_GP_PIN(0, 14), /* MSIOF2_SS1 */
		[15] = RCAR_GP_PIN(0, 15), /* MSIOF2_SYNC */
		[16] = RCAR_GP_PIN(0, 16), /* MSIOF2_TXD */
		[17] = RCAR_GP_PIN(0, 17), /* MSIOF2_SCK */
		[18] = RCAR_GP_PIN(0, 18), /* MSIOF2_RXD */
		[19] = PIN_NONE,           [20] = PIN_NONE, [21] = PIN_NONE, [22] = PIN_NONE,
		[23] = PIN_NONE,           [24] = PIN_NONE, [25] = PIN_NONE, [26] = PIN_NONE,
		[27] = PIN_NONE,           [28] = PIN_NONE, [29] = PIN_NONE, [30] = PIN_NONE,
		[31] = PIN_NONE,
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN1, PUD1 */
		[0] = RCAR_GP_PIN(1, 0),   /* MSIOF1_SS2 */
		[1] = RCAR_GP_PIN(1, 1),   /* MSIOF1_SS1 */
		[2] = RCAR_GP_PIN(1, 2),   /* MSIOF1_SYNC */
		[3] = RCAR_GP_PIN(1, 3),   /* MSIOF1_SCK */
		[4] = RCAR_GP_PIN(1, 4),   /* MSIOF1_TXD */
		[5] = RCAR_GP_PIN(1, 5),   /* MSIOF1_RXD */
		[6] = RCAR_GP_PIN(1, 6),   /* MSIOF0_SS2 */
		[7] = RCAR_GP_PIN(1, 7),   /* MSIOF0_SS1 */
		[8] = RCAR_GP_PIN(1, 8),   /* MSIOF0_SYNC */
		[9] = RCAR_GP_PIN(1, 9),   /* MSIOF0_TXD */
		[10] = RCAR_GP_PIN(1, 10), /* MSIOF0_SCK */
		[11] = RCAR_GP_PIN(1, 11), /* MSIOF0_RXD */
		[12] = RCAR_GP_PIN(1, 12), /* HTX0 */
		[13] = RCAR_GP_PIN(1, 13), /* HCTS0# */
		[14] = RCAR_GP_PIN(1, 14), /* HRTS0# */
		[15] = RCAR_GP_PIN(1, 15), /* HSCK0 */
		[16] = RCAR_GP_PIN(1, 16), /* HRX0 */
		[17] = RCAR_GP_PIN(1, 17), /* SCIF_CLK */
		[18] = RCAR_GP_PIN(1, 18), /* SSI_SCK */
		[19] = RCAR_GP_PIN(1, 19), /* SSI_WS */
		[20] = RCAR_GP_PIN(1, 20), /* SSI_SD */
		[21] = RCAR_GP_PIN(1, 21), /* AUDIO_CLKOUT */
		[22] = RCAR_GP_PIN(1, 22), /* AUDIO_CLKIN */
		[23] = RCAR_GP_PIN(1, 23), /* GP1_23 */
		[24] = RCAR_GP_PIN(1, 24), /* HRX3 */
		[25] = RCAR_GP_PIN(1, 25), /* HSCK3 */
		[26] = RCAR_GP_PIN(1, 26), /* HRTS3# */
		[27] = RCAR_GP_PIN(1, 27), /* HCTS3# */
		[28] = RCAR_GP_PIN(1, 28), /* HTX3 */
		[29] = PIN_NONE,           [30] = PIN_NONE, [31] = PIN_NONE,
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN2, PUD2 */
		[0] = RCAR_GP_PIN(2, 0),   /* FXR_TXDA */
		[1] = RCAR_GP_PIN(2, 1),   /* FXR_TXENA# */
		[2] = RCAR_GP_PIN(2, 2),   /* RXDA_EXTFXR */
		[3] = RCAR_GP_PIN(2, 3),   /* CLK_EXTFXR */
		[4] = RCAR_GP_PIN(2, 4),   /* RXDB_EXTFXR */
		[5] = RCAR_GP_PIN(2, 5),   /* FXR_TXENB# */
		[6] = RCAR_GP_PIN(2, 6),   /* FXR_TXDB */
		[7] = RCAR_GP_PIN(2, 7),   /* TPU0TO1 */
		[8] = RCAR_GP_PIN(2, 8),   /* TPU0TO0 */
		[9] = RCAR_GP_PIN(2, 9),   /* CAN_CLK */
		[10] = RCAR_GP_PIN(2, 10), /* CANFD0_TX */
		[11] = RCAR_GP_PIN(2, 11), /* CANFD0_RX */
		[12] = RCAR_GP_PIN(2, 12), /* CANFD2_TX */
		[13] = RCAR_GP_PIN(2, 13), /* CANFD2_RX */
		[14] = RCAR_GP_PIN(2, 14), /* CANFD3_TX */
		[15] = RCAR_GP_PIN(2, 15), /* CANFD3_RX */
		[16] = RCAR_GP_PIN(2, 16), /* CANFD4_TX */
		[17] = RCAR_GP_PIN(2, 17), /* CANFD4_RX */
		[18] = RCAR_GP_PIN(2, 18), /* CANFD7_TX */
		[19] = RCAR_GP_PIN(2, 19), /* CANFD7_RX */
		[20] = RCAR_GP_PIN(2, 20), /* - */
		[21] = RCAR_GP_PIN(2, 21), /* - */
		[22] = RCAR_GP_PIN(2, 22), /* - */
		[23] = RCAR_GP_PIN(2, 23), /* - */
		[24] = RCAR_GP_PIN(2, 24), /* - */
		[25] = RCAR_GP_PIN(2, 25), /* - */
		[26] = RCAR_GP_PIN(2, 26), /* - */
		[27] = RCAR_GP_PIN(2, 27), /* - */
		[28] = RCAR_GP_PIN(2, 28), /* - */
		[29] = RCAR_GP_PIN(2, 29), /* - */
		[30] = RCAR_GP_PIN(2, 30), /* - */
		[31] = RCAR_GP_PIN(2, 31), /* - */
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN3, PUD3 */
		[0] = RCAR_GP_PIN(3, 0),   /* MMC_SD_D1 */
		[1] = RCAR_GP_PIN(3, 1),   /* MMC_SD_D0 */
		[2] = RCAR_GP_PIN(3, 2),   /* MMC_SD_D2 */
		[3] = RCAR_GP_PIN(3, 3),   /* MMC_SD_CLK */
		[4] = RCAR_GP_PIN(3, 4),   /* MMC_DS */
		[5] = RCAR_GP_PIN(3, 5),   /* MMC_SD_D3 */
		[6] = RCAR_GP_PIN(3, 6),   /* MMC_D5 */
		[7] = RCAR_GP_PIN(3, 7),   /* MMC_D4 */
		[8] = RCAR_GP_PIN(3, 8),   /* MMC_D7 */
		[9] = RCAR_GP_PIN(3, 9),   /* MMC_D6 */
		[10] = RCAR_GP_PIN(3, 10), /* MMC_SD_CMD */
		[11] = RCAR_GP_PIN(3, 11), /* SD_CD */
		[12] = RCAR_GP_PIN(3, 12), /* SD_WP */
		[13] = RCAR_GP_PIN(3, 13), /* IPC_CLKIN */
		[14] = RCAR_GP_PIN(3, 14), /* IPC_CLKOUT */
		[15] = RCAR_GP_PIN(3, 15), /* QSPI0_SSL */
		[16] = RCAR_GP_PIN(3, 16), /* QSPI0_IO3 */
		[17] = RCAR_GP_PIN(3, 17), /* QSPI0_IO2 */
		[18] = RCAR_GP_PIN(3, 18), /* QSPI0_MISO_IO1 */
		[19] = RCAR_GP_PIN(3, 19), /* QSPI0_MOSI_IO0 */
		[20] = RCAR_GP_PIN(3, 20), /* QSPI0_SPCLK */
		[21] = RCAR_GP_PIN(3, 21), /* QSPI1_MOSI_IO0 */
		[22] = RCAR_GP_PIN(3, 22), /* QSPI1_SPCLK */
		[23] = RCAR_GP_PIN(3, 23), /* QSPI1_MISO_IO1 */
		[24] = RCAR_GP_PIN(3, 24), /* QSPI1_IO2 */
		[25] = RCAR_GP_PIN(3, 25), /* QSPI1_SSL */
		[26] = RCAR_GP_PIN(3, 26), /* QSPI1_IO3 */
		[27] = RCAR_GP_PIN(3, 27), /* RPC_RESET# */
		[28] = RCAR_GP_PIN(3, 28), /* RPC_WP# */
		[29] = RCAR_GP_PIN(3, 29), /* RPC_INT# */
		[30] = PIN_NONE,           /* - */
		[31] = PIN_NONE,           /* - */
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN4, PUD4 */
		[0] = RCAR_GP_PIN(4, 0),   /* TSN0_MDIO*/
		[1] = RCAR_GP_PIN(4, 1),   /* TSN0_MDC*/
		[2] = RCAR_GP_PIN(4, 2),   /* TSN0_AVTP_PPS1*/
		[3] = RCAR_GP_PIN(4, 3),   /* TSN0_PHY_INT*/
		[4] = RCAR_GP_PIN(4, 4),   /* TSN0_LINK*/
		[5] = RCAR_GP_PIN(4, 5),   /* TSN0_AVTP_MATCH*/
		[6] = RCAR_GP_PIN(4, 6),   /* TSN0_AVTP_CAPTURE*/
		[7] = RCAR_GP_PIN(4, 7),   /* TSN0_RX_CTL*/
		[8] = RCAR_GP_PIN(4, 8),   /* TSN0_AVTP_PPS0*/
		[9] = RCAR_GP_PIN(4, 9),   /* TSN0_TX_CTL*/
		[10] = RCAR_GP_PIN(4, 10), /* TSN0_RD0*/
		[11] = RCAR_GP_PIN(4, 11), /* TSN0_RXC*/
		[12] = RCAR_GP_PIN(4, 12), /* TSN0_TXC*/
		[13] = RCAR_GP_PIN(4, 13), /* TSN0_RD1*/
		[14] = RCAR_GP_PIN(4, 14), /* TSN0_TD1*/
		[15] = RCAR_GP_PIN(4, 15), /* TSN0_TD0*/
		[16] = RCAR_GP_PIN(4, 16), /* TSN0_RD3*/
		[17] = RCAR_GP_PIN(4, 17), /* TSN0_RD2*/
		[18] = RCAR_GP_PIN(4, 18), /* TSN0_TD3*/
		[19] = RCAR_GP_PIN(4, 19), /* TSN0_TD2*/
		[20] = RCAR_GP_PIN(4, 20), /* TSN0_TXCREFCLK*/
		[21] = RCAR_GP_PIN(4, 21), /* PCIE0_CLKREQ#*/
		[22] = RCAR_GP_PIN(4, 22), /* PCIE1_CLKREQ#*/
		[23] = RCAR_GP_PIN(4, 23), /* AVS0*/
		[24] = RCAR_GP_PIN(4, 24), /* AVS1*/
		[25] = PIN_NONE,           /* -*/
		[26] = PIN_NONE,           /* -*/
		[27] = PIN_NONE,           /* -*/
		[28] = PIN_NONE,           /* -*/
		[29] = PIN_NONE,           /* -*/
		[30] = PIN_NONE,           /* -*/
		[31] = PIN_NONE            /* -*/
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN5, PUD5 */
		[0] = RCAR_GP_PIN(5, 0),   /* AVB2_AVTP_PPS*/
		[1] = RCAR_GP_PIN(5, 1),   /* AVB2_AVTP_CAPTURE*/
		[2] = RCAR_GP_PIN(5, 2),   /* AVB2_AVTP_MATCH*/
		[3] = RCAR_GP_PIN(5, 3),   /* AVB2_LINK*/
		[4] = RCAR_GP_PIN(5, 4),   /* AVB2_PHY_INT*/
		[5] = RCAR_GP_PIN(5, 5),   /* AVB2_MAGIC*/
		[6] = RCAR_GP_PIN(5, 6),   /* AVB2_MDC*/
		[7] = RCAR_GP_PIN(5, 7),   /* AVB2_TXCREFCLK*/
		[8] = RCAR_GP_PIN(5, 8),   /* AVB2_TD3*/
		[9] = RCAR_GP_PIN(5, 9),   /* AVB2_RD3*/
		[10] = RCAR_GP_PIN(5, 10), /* AVB2_MDIO*/
		[11] = RCAR_GP_PIN(5, 11), /* AVB2_TD2*/
		[12] = RCAR_GP_PIN(5, 12), /* AVB2_TD1*/
		[13] = RCAR_GP_PIN(5, 13), /* AVB2_RD2*/
		[14] = RCAR_GP_PIN(5, 14), /* AVB2_RD1*/
		[15] = RCAR_GP_PIN(5, 15), /* AVB2_TD0*/
		[16] = RCAR_GP_PIN(5, 16), /* AVB2_TXC*/
		[17] = RCAR_GP_PIN(5, 17), /* AVB2_RD0*/
		[18] = RCAR_GP_PIN(5, 18), /* AVB2_RXC*/
		[19] = RCAR_GP_PIN(5, 19), /* AVB2_TX_CTL*/
		[20] = RCAR_GP_PIN(5, 20), /* AVB2_RX_CTL*/
		[21] = PIN_NONE,           /* -*/
		[22] = PIN_NONE,           /* -*/
		[23] = PIN_NONE,           /* -*/
		[24] = PIN_NONE,           /* -*/
		[25] = PIN_NONE,           /* -*/
		[26] = PIN_NONE,           /* -*/
		[27] = PIN_NONE,           /* -*/
		[28] = PIN_NONE,           /* -*/
		[29] = PIN_NONE,           /* -*/
		[30] = PIN_NONE,           /* -*/
		[31] = PIN_NONE            /* -*/
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN6, PUD6 */
		[0] = RCAR_GP_PIN(6, 0),   /* AVB1_MDIO */
		[1] = RCAR_GP_PIN(6, 1),   /* AVB1_MAGIC */
		[2] = RCAR_GP_PIN(6, 2),   /* AVB1_MDC */
		[3] = RCAR_GP_PIN(6, 3),   /* AVB1_PHY_INT */
		[4] = RCAR_GP_PIN(6, 4),   /* AVB1_LINK */
		[5] = RCAR_GP_PIN(6, 5),   /* AVB1_AVTP_MATCH */
		[6] = RCAR_GP_PIN(6, 6),   /* AVB1_TXC */
		[7] = RCAR_GP_PIN(6, 7),   /* AVB1_TX_CTL */
		[8] = RCAR_GP_PIN(6, 8),   /* AVB1_RXC */
		[9] = RCAR_GP_PIN(6, 9),   /* AVB1_RX_CTL */
		[10] = RCAR_GP_PIN(6, 10), /* AVB1_AVTP_PPS */
		[11] = RCAR_GP_PIN(6, 11), /* AVB1_AVTP_CAPTURE */
		[12] = RCAR_GP_PIN(6, 12), /* AVB1_TD1 */
		[13] = RCAR_GP_PIN(6, 13), /* AVB1_TD0 */
		[14] = RCAR_GP_PIN(6, 14), /* AVB1_RD1 */
		[15] = RCAR_GP_PIN(6, 15), /* AVB1_RD0 */
		[16] = RCAR_GP_PIN(6, 16), /* AVB1_TD2 */
		[17] = RCAR_GP_PIN(6, 17), /* AVB1_RD2 */
		[18] = RCAR_GP_PIN(6, 18), /* AVB1_TD3 */
		[19] = RCAR_GP_PIN(6, 19), /* AVB1_RD3 */
		[20] = RCAR_GP_PIN(6, 20), /* AVB1_TXCREFCLK */
		[21] = PIN_NONE,           /* - */
		[22] = PIN_NONE,           /* - */
		[23] = PIN_NONE,           /* - */
		[24] = PIN_NONE,           /* - */
		[25] = PIN_NONE,           /* - */
		[26] = PIN_NONE,           /* - */
		[27] = PIN_NONE,           /* - */
		[28] = PIN_NONE,           /* - */
		[29] = PIN_NONE,           /* - */
		[30] = PIN_NONE,           /* - */
		[31] = PIN_NONE,           /* - */
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN7, PUD7 */
		[0] = RCAR_GP_PIN(7, 0),   /* AVB0_AVTP_PPS */
		[1] = RCAR_GP_PIN(7, 1),   /* AVB0_AVTP_CAPTURE */
		[2] = RCAR_GP_PIN(7, 2),   /* AVB0_AVTP_MATCH */
		[3] = RCAR_GP_PIN(7, 3),   /* AVB0_TD3 */
		[4] = RCAR_GP_PIN(7, 4),   /* AVB0_LINK */
		[5] = RCAR_GP_PIN(7, 5),   /* AVB0_PHY_INT */
		[6] = RCAR_GP_PIN(7, 6),   /* AVB0_TD2 */
		[7] = RCAR_GP_PIN(7, 7),   /* AVB0_TD1 */
		[8] = RCAR_GP_PIN(7, 8),   /* AVB0_RD3 */
		[9] = RCAR_GP_PIN(7, 9),   /* AVB0_TXCREFCLK */
		[10] = RCAR_GP_PIN(7, 10), /* AVB0_MAGIC */
		[11] = RCAR_GP_PIN(7, 11), /* AVB0_TD0 */
		[12] = RCAR_GP_PIN(7, 12), /* AVB0_RD2 */
		[13] = RCAR_GP_PIN(7, 13), /* AVB0_MDC */
		[14] = RCAR_GP_PIN(7, 14), /* AVB0_MDIO */
		[15] = RCAR_GP_PIN(7, 15), /* AVB0_TXC */
		[16] = RCAR_GP_PIN(7, 16), /* AVB0_TX_CTL */
		[17] = RCAR_GP_PIN(7, 17), /* AVB0_RD1 */
		[18] = RCAR_GP_PIN(7, 18), /* AVB0_RD0 */
		[19] = RCAR_GP_PIN(7, 19), /* AVB0_RXC */
		[20] = RCAR_GP_PIN(7, 20), /* AVB0_RX_CTL */
		[21] = PIN_NONE,           /* - */
		[22] = PIN_NONE,           /* - */
		[23] = PIN_NONE,           /* - */
		[24] = PIN_NONE,           /* - */
		[25] = PIN_NONE,           /* - */
		[26] = PIN_NONE,           /* - */
		[27] = PIN_NONE,           /* - */
		[28] = PIN_NONE,           /* - */
		[29] = PIN_NONE,           /* - */
		[30] = PIN_NONE,           /* - */
		[31] = PIN_NONE,           /* - */
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUEN8, PUD8 */
		[0] = RCAR_GP_PIN(8, 0),   /* SCL0 */
		[1] = RCAR_GP_PIN(8, 1),   /* SDA0 */
		[2] = RCAR_GP_PIN(8, 2),   /* SCL1 */
		[3] = RCAR_GP_PIN(8, 3),   /* SDA1 */
		[4] = RCAR_GP_PIN(8, 4),   /* SCL2 */
		[5] = RCAR_GP_PIN(8, 5),   /* SDA2 */
		[6] = RCAR_GP_PIN(8, 6),   /* SCL3 */
		[7] = RCAR_GP_PIN(8, 7),   /* SDA3 */
		[8] = RCAR_GP_PIN(8, 8),   /* SCL4 */
		[9] = RCAR_GP_PIN(8, 9),   /* SDA4 */
		[10] = RCAR_GP_PIN(8, 10), /* SCL5 */
		[11] = RCAR_GP_PIN(8, 11), /* SDA5 */
		[12] = RCAR_GP_PIN(8, 12), /* GP8_12 */
		[13] = RCAR_GP_PIN(8, 13), /* GP8_13 */
		[14] = PIN_NONE,           /* - */
		[15] = PIN_NONE,           /* - */
		[16] = PIN_NONE,           /* - */
		[17] = PIN_NONE,           /* - */
		[18] = PIN_NONE,           /* - */
		[19] = PIN_NONE,           /* - */
		[20] = PIN_NONE,           /* - */
		[21] = PIN_NONE,           /* - */
		[22] = PIN_NONE,           /* - */
		[23] = PIN_NONE,           /* - */
		[24] = PIN_NONE,           /* - */
		[25] = PIN_NONE,           /* - */
		[26] = PIN_NONE,           /* - */
		[27] = PIN_NONE,           /* - */
		[28] = PIN_NONE,           /* - */
		[29] = PIN_NONE,           /* - */
		[30] = PIN_NONE,           /* - */
		[31] = PIN_NONE,           /* - */
	}},
	{PFC_BIAS_REG(0xc0, 0xe0) {
		/* PUENsys, PUDsys */
		[0] = PIN_NONE,  /* PRESETOUT_N */
		[1] = PIN_NONE,  /* - */
		[2] = PIN_NONE,  /* - */
		[3] = PIN_NONE,  /* - */
		[4] = PIN_NONE,  /* EXTALR */
		[5] = PIN_NONE,  /* - */
		[6] = PIN_NONE,  /* DCUTRST_N_LPDRST_N */
		[7] = PIN_NONE,  /* DCUTCK_LPDCLK */
		[8] = PIN_NONE,  /* DCUTMS */
		[9] = PIN_NONE,  /* DCUTDI_LPDI */
		[10] = PIN_NONE, /* - */
		[11] = PIN_NONE, /* - */
		[12] = PIN_NONE, /* - */
		[13] = PIN_NONE, /* - */
		[14] = PIN_NONE, /* - */
		[15] = PIN_NONE, /* - */
		[16] = PIN_NONE, /* - */
		[17] = PIN_NONE, /* - */
		[18] = PIN_NONE, /* - */
		[19] = PIN_NONE, /* - */
		[20] = PIN_NONE, /* - */
		[21] = PIN_NONE, /* - */
		[22] = PIN_NONE, /* - */
		[23] = PIN_NONE, /* - */
		[24] = PIN_NONE, /* - */
		[25] = PIN_NONE, /* - */
		[26] = PIN_NONE, /* - */
		[27] = PIN_NONE, /* - */
		[28] = PIN_NONE, /* - */
		[29] = PIN_NONE, /* - */
		[30] = PIN_NONE, /* - */
		[31] = PIN_NONE, /* - */
	}},
	{/* sentinel */},
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
