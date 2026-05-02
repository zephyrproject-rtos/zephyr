/*
 * Copyright (c) 2023 EPAM Systems
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __RCAR_MMC_REGISTERS_H__
#define __RCAR_MMC_REGISTERS_H__

#include <zephyr/sys/util_macro.h> /* for BIT macro */

/*
 * The command type register is used to select the command type
 * and response type
 */
#define RCAR_MMC_CMD			0x000		/* command */
#define RCAR_MMC_CMD_NOSTOP		BIT(14)		/* No automatic CMD12 issue */
#define RCAR_MMC_CMD_MULTI		BIT(13)		/* multiple block transfer */
#define RCAR_MMC_CMD_RD			BIT(12)		/* 1: read, 0: write */
#define RCAR_MMC_CMD_DATA		BIT(11)		/* data transfer */
#define RCAR_MMC_CMD_APP		BIT(6)		/* ACMD preceded by CMD55 */
#define RCAR_MMC_CMD_NORMAL		(0 << 8)	/* auto-detect of resp-type */
#define RCAR_MMC_CMD_RSP_NONE		(3 << 8)	/* response: none */
#define RCAR_MMC_CMD_RSP_R1		(4 << 8)	/* response: R1, R5, R6, R7 */
#define RCAR_MMC_CMD_RSP_R1B		(5 << 8)	/* response: R1b, R5b */
#define RCAR_MMC_CMD_RSP_R2		(6 << 8)	/* response: R2 */
#define RCAR_MMC_CMD_RSP_R3		(7 << 8)	/* response: R3, R4 */

/* Command arguments register for SD card */
#define RCAR_MMC_ARG			0x010	/* command argument */

/*
 * The data stop register is used to enable or disable block counting at
 * multiple block transfer, and to control the issuing of CMD12 within
 * command sequences.
 */
#define RCAR_MMC_STOP			0x020	/* stop action control */
#define RCAR_MMC_STOP_SEC		BIT(8)	/* use sector count */
#define RCAR_MMC_STOP_STP		BIT(0)	/* issue CMD12 */

/*
 * The block count register is used to specify the number of
 * transfer blocks at multiple block transfer.
 */
#define RCAR_MMC_SECCNT			0x028	/* sector counter */

/* The SD card response registers hold the response from the SD card */
#define RCAR_MMC_RSP10			0x030	/* response[39:8] */
#define RCAR_MMC_RSP32			0x040	/* response[71:40] */
#define RCAR_MMC_RSP54			0x050	/* response[103:72] */
#define RCAR_MMC_RSP76			0x060	/* response[127:104] */

/*
 * The SD card interrupt flag register 1 indicates the response end and access
 * end in the command sequence. This register also indicates the card
 * detect/write protect state.
 */
#define RCAR_MMC_INFO1			0x070	/* IRQ status 1 */
#define RCAR_MMC_INFO1_CD		BIT(5)	/* state of card detect */
#define RCAR_MMC_INFO1_INSERT		BIT(4)	/* card inserted */
#define RCAR_MMC_INFO1_REMOVE		BIT(3)	/* card removed */
#define RCAR_MMC_INFO1_CMP		BIT(2)	/* data complete */
#define RCAR_MMC_INFO1_RSP		BIT(0)	/* response complete */

/*
 * The SD card interrupt flag register 2 indicates the access status of the
 * SD buffer and SD card.
 */
#define RCAR_MMC_INFO2			0x078	/* IRQ status 2 */
#define RCAR_MMC_INFO2_ERR_ILA		BIT(15)	/* illegal access err */
#define RCAR_MMC_INFO2_CBSY		BIT(14)	/* command busy */
#define RCAR_MMC_INFO2_SCLKDIVEN	BIT(13)	/* command setting reg ena */
#define RCAR_MMC_INFO2_CLEAR		BIT(11)	/* the write value should always be 1 */
#define RCAR_MMC_INFO2_BWE		BIT(9)	/* write buffer ready */
#define RCAR_MMC_INFO2_BRE		BIT(8)	/* read buffer ready */
#define RCAR_MMC_INFO2_DAT0		BIT(7)	/* SDDAT0 */
#define RCAR_MMC_INFO2_ERR_RTO		BIT(6)	/* response time out */
#define RCAR_MMC_INFO2_ERR_ILR		BIT(5)	/* illegal read err */
#define RCAR_MMC_INFO2_ERR_ILW		BIT(4)	/* illegal write err */
#define RCAR_MMC_INFO2_ERR_TO		BIT(3)	/* time out error */
#define RCAR_MMC_INFO2_ERR_END		BIT(2)	/* END bit error */
#define RCAR_MMC_INFO2_ERR_CRC		BIT(1)	/* CRC error */
#define RCAR_MMC_INFO2_ERR_IDX		BIT(0)	/* cmd index error */

#define RCAR_MMC_INFO2_ERRORS \
	(RCAR_MMC_INFO2_ERR_RTO | RCAR_MMC_INFO2_ERR_ILR | \
	 RCAR_MMC_INFO2_ERR_ILW | RCAR_MMC_INFO2_ERR_TO  | \
	 RCAR_MMC_INFO2_ERR_END | RCAR_MMC_INFO2_ERR_CRC | \
	 RCAR_MMC_INFO2_ERR_IDX | RCAR_MMC_INFO2_ERR_ILA)

/*
 * The interrupt mask 1 register is used to enable or disable
 * the RCAR_MMC_INFO1 interrupt.
 */
#define RCAR_MMC_INFO1_MASK		0x080

/*
 * The interrupt mask 2 register is used to enable or disable
 * the RCAR_MMC_INFO2 interrupt.
 */
#define RCAR_MMC_INFO2_MASK		0x088

/*
 * The SD clock control register is used to control
 * the SD clock output and to set the frequency.
 */
#define RCAR_MMC_CLKCTL			0x090
#define RCAR_MMC_CLKCTL_DIV_MASK	0x104ff
#define RCAR_MMC_CLKCTL_DIV512		BIT(7)	/* SDCLK = CLK / 512 */
#define RCAR_MMC_CLKCTL_DIV256		BIT(6)	/* SDCLK = CLK / 256 */
#define RCAR_MMC_CLKCTL_DIV128		BIT(5)	/* SDCLK = CLK / 128 */
#define RCAR_MMC_CLKCTL_DIV64		BIT(4)	/* SDCLK = CLK / 64 */
#define RCAR_MMC_CLKCTL_DIV32		BIT(3)	/* SDCLK = CLK / 32 */
#define RCAR_MMC_CLKCTL_DIV16		BIT(2)	/* SDCLK = CLK / 16 */
#define RCAR_MMC_CLKCTL_DIV8		BIT(1)	/* SDCLK = CLK / 8 */
#define RCAR_MMC_CLKCTL_DIV4		BIT(0)	/* SDCLK = CLK / 4 */
#define RCAR_MMC_CLKCTL_DIV2		0	/* SDCLK = CLK / 2 */
#define RCAR_MMC_CLKCTL_RCAR_DIV1	0xff	/* SDCLK = CLK (RCar ver.) */
#define RCAR_MMC_CLKCTL_OFFEN		BIT(9)	/* stop SDCLK when unused */
#define RCAR_MMC_CLKCTL_SCLKEN		BIT(8)	/* SDCLK output enable */

/*
 * The transfer data length register is used to specify
 * the transfer data size.
 */
#define RCAR_MMC_SIZE			0x098

/*
 * The SD card access control option register is used to set
 * the bus width and timeout counter.
 */
#define RCAR_MMC_OPTION			0x0A0
#define RCAR_MMC_OPTION_WIDTH_MASK	(5 << 13)
#define RCAR_MMC_OPTION_WIDTH_1		(4 << 13)
#define RCAR_MMC_OPTION_WIDTH_4		(0 << 13)
#define RCAR_MMC_OPTION_WIDTH_8		(1 << 13)

/*
 * The SD error status register 1 indicates the CRC status, CRC error,
 * End error, and CMD error.
 */
#define RCAR_MMC_ERR_STS1		0x0B0

/* The SD error status register 2 indicates the timeout state. */
#define RCAR_MMC_ERR_STS2		0x0B8

/* SD Buffer Read/Write Register */
#define RCAR_MMC_BUF0			0x0C0

/* The DMA mode enable register enables the DMA transfer. */
#define RCAR_MMC_EXTMODE		0x360
#define RCAR_MMC_EXTMODE_DMA_EN		BIT(1)	/* transfer 1: DMA, 0: pio */

/* The software reset register sets a software reset. */
#define RCAR_MMC_SOFT_RST		0x380
#define RCAR_MMC_SOFT_RST_RSTX		BIT(0)	/* reset deassert */

/* The version register indicates the version of the SD host interface. */
#define RCAR_MMC_VERSION		0x388
#define RCAR_MMC_VERSION_IP		0xff	/* IP version */

/*
 * The host interface mode setting register selects the width for access to
 * the data bus.
 */
#define RCAR_MMC_HOST_MODE		0x390

/* The SD interface mode setting register specifies HS400 mode. */
#define RCAR_MMC_IF_MODE		0x398
#define RCAR_MMC_IF_MODE_DDR		BIT(0)	/* DDR mode */

/* Set of DMAC registers */
#define RCAR_MMC_DMA_MODE		0x820
#define RCAR_MMC_DMA_MODE_DIR_RD	BIT(16)	/* 1: from device, 0: to dev */
#define RCAR_MMC_DMA_MODE_WIDTH		(BIT(4) | BIT(5))
#define RCAR_MMC_DMA_MODE_ADDR_INC	BIT(0)	/* 1: address inc, 0: fixed */
#define RCAR_MMC_DMA_CTL		0x828
#define RCAR_MMC_DMA_CTL_START		BIT(0)	/* start DMA (auto cleared) */
#define RCAR_MMC_DMA_RST		0x830
#define RCAR_MMC_DMA_RST_DTRAN0		BIT(8)
#define RCAR_MMC_DMA_RST_DTRAN1		BIT(9)
#define RCAR_MMC_DMA_INFO1		0x840
#define RCAR_MMC_DMA_INFO1_END_RD2	BIT(20)	/* DMA from device is complete (uniphier) */
#define RCAR_MMC_DMA_INFO1_END_RD	BIT(17)	/* DMA from device is complete (renesas) */
#define RCAR_MMC_DMA_INFO1_END_WR	BIT(16)	/* DMA to device is complete */
#define RCAR_MMC_DMA_INFO1_MASK		0x848
#define RCAR_MMC_DMA_INFO2		0x850
#define RCAR_MMC_DMA_INFO2_ERR_RD	BIT(17)
#define RCAR_MMC_DMA_INFO2_ERR_WR	BIT(16)
#define RCAR_MMC_DMA_INFO2_MASK		0x858
#define RCAR_MMC_DMA_ADDR_L		0x880
#define RCAR_MMC_DMA_ADDR_H		0x888

/* set of SCC registers */

/* Initial setting register */
#define RENESAS_SDHI_SCC_DTCNTL			0x1000
#define RENESAS_SDHI_SCC_DTCNTL_TAPEN		BIT(0)
/* Sampling clock position setting register */
#define RENESAS_SDHI_SCC_TAPSET			0x1008
#define RENESAS_SDHI_SCC_DT2FF			0x1010
/* Sampling Clock Selection Register */
#define RENESAS_SDHI_SCC_CKSEL			0x1018
#define RENESAS_SDHI_SCC_CKSEL_DTSEL		BIT(0)
/* Sampling Clock Position Correction Register */
#define RENESAS_SDHI_SCC_RVSCNTL		0x1020
#define RENESAS_SDHI_SCC_RVSCNTL_RVSEN		BIT(0)
/* Sampling Clock Position Correction Request Register */
#define RENESAS_SDHI_SCC_RVSREQ			0x1028
#define RENESAS_SDHI_SCC_RVSREQ_REQTAPDOWN	BIT(0)
#define RENESAS_SDHI_SCC_RVSREQ_REQTAPUP	BIT(1)
#define RENESAS_SDHI_SCC_RVSREQ_REQTAP_MASK \
	(RENESAS_SDHI_SCC_RVSREQ_REQTAPDOWN | RENESAS_SDHI_SCC_RVSREQ_REQTAPUP)
#define RENESAS_SDHI_SCC_RVSREQ_ERR		BIT(2)
/* Sampling data comparison register */
#define RENESAS_SDHI_SCC_SMPCMP			0x1030
/* Hardware Adjustment Register 2, used for configuration HS400 mode */
#define RENESAS_SDHI_SCC_TMPPORT2		0x1038
#define RENESAS_SDHI_SCC_TMPPORT2_HS400EN	BIT(31)
#define RENESAS_SDHI_SCC_TMPPORT2_HS400OSEL	BIT(4)

#endif /* __RCAR_MMC_REGISTERS_H__ */
