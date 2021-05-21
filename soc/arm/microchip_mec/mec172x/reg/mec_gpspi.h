/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MEC_GPSPI_H
#define MEC_GPSPI_H

#include <stddef.h>
#include <stdint.h>

#include "mec_defs.h"
#include "mec_regaccess.h"

/* General Purpose SPI (GPSPI) */
#define MCHP_GPSPI_BASE		0x40009400ul

/* Offset between instances of the Basic Timer blocks */
#define MCHP_GPSPI_INSTANCE_OFS 0x80U

/* 0 <= n < MCHP_GPSPI_MAX_INSTANCE */
#define MCHP_GPSPI_ADDR(n) \
	(MCHP_GPSPI_BASE + ((uint32_t)(n) * MCHP_GPSPI_INSTANCE_OFS))

#define MCHP_GPSPI_0_ADDR	0x40009400ul
#define MCHP_GPSPI_1_ADDR	0x40009480ul

/* GPSPI interrupt routing */
#define MCHP_GPSPI_0_GIRQ	18u
#define MCHP_GPSPI_1_GIRQ	18u

/* Bit position in GIRQ Source, Enable-Set/Clr, and Result registers */
#define MCHP_GPSPI_0_GIRQ_TXBE_POS	2u
#define MCHP_GPSPI_0_GIRQ_RXBF_POS	3u
#define MCHP_GPSPI_1_GIRQ_TXBE_POS	4u
#define MCHP_GPSPI_1_GIRQ_RXBF_POS	5u

/* GIRQ Source, Enable-Set/Clear, Result register values */
#define MCHP_GPSPI_0_GIRQ_TXBE_VAL	0x04U
#define MCHP_GPSPI_0_GIRQ_RXBF_VAL	0x08U
#define MCHP_GPSPI_0_GIRQ_ALL_VAL	0x0Cu

#define MCHP_GPSPI_1_GIRQ_TXBE_VAL	0x10U
#define MCHP_GPSPI_1_GIRQ_RXBF_VAL	0x20U
#define MCHP_GPSPI_1_GIRQ_ALL_VAL	0x30U

/* GPSPI GIRQ aggregated NVIC input */
#define MCHP_GPSPI_0_NVIC_AGGR		10u
#define MCHP_GPSPI_1_NVIC_AGGR		10u

/* GPSPI direct NVIC inputs */
#define MCHP_GPSPI_0_NVIC_DIRECT_TXBE	92u
#define MCHP_GPSPI_0_NVIC_DIRECT_RXBF	93u
#define MCHP_GPSPI_1_NVIC_DIRECT_TXBE	94u
#define MCHP_GPSPI_1_NVIC_DIRECT_RXBF	95u

/* Block Enable register */
#define MCHP_GPSPI_BLK_EN_REG_OFS	0x00ul
#define MCHP_GPSPI_BLK_EN_REG_MASK	0x01ul
#define MCHP_GPSPI_BLK_EN_ON		0x01ul

/* Control register */
#define MCHP_GPSPI_CTRL_REG_OFS		0x04ul
#define MCHP_GPSPI_CTRL_REG_MASK	0x7Ful
#define MCHP_GPSPI_CTRL_MSBF		0x00u
#define MCHP_GPSPI_CTRL_LSBF		0x01u
#define MCHP_GPSPI_CTRL_BIOEN_OUT	0x00u
#define MCHP_GPSPI_CTRL_BIOEN_IN	0x02u
#define MCHP_GPSPI_CTRL_SPDIN_MASK	0x0Cu
#define MCHP_GPSPI_CTRL_SPDIN_FD	0x00u
#define MCHP_GPSPI_CTRL_SPDIN_HD	0x04u
#define MCHP_GPSPI_CTRL_SPDIN_DUAL	0x08u
#define MCHP_GPSPI_CTRL_SRST		0x10u
#define MCHP_GPSPI_CTRL_AUTO_READ	0x20u
#define MCHP_GPSPI_CTRL_CS_ASSERT	0x40u

/* Status register (read-only) */
#define MCHP_GPSPI_STS_REG_OFS		0x08ul
#define MCHP_GPSPI_STS_REG_MASK		0x07u
#define MCHP_GPSPI_STS_ALL		0x07u
#define MCHP_GPSPI_STS_TXBE		0x01u
#define MCHP_GPSPI_STS_RXBF		0x02u
/*
 * Active indicates GPSPI is in process of moving data to/from its
 * internal shift register. It will also be set for the condition
 * of RX overrun where CPU has not read the byte in RX Data before
 * another byte is ready to be moved from internal shift register
 * into RX data.
 */
#define MCHP_GPSPI_STS_ACTIVE		0x04u

/*
 * GPSPI hardware shared one internal shift register between
 * TX and RX paths. Before writing TX Data software must insure
 * RX Data is empty, all received data has been cleared. Actually
 * software should not write TX Data unitl both RXBF and Active
 * status are cleared.
 */

/*
 * TX Data register
 * Write clears TXBE status.
 * Do not write if TXBE == 0.
 */
#define MCHP_GPSPI_TXD_REG_OFS		0x0Cul
#define MCHP_GPSPI_TXD_REG_MASK		0xFFu

/*
 * RX Data register
 * Read clears RXBF status.
 */
#define MCHP_GPSPI_RXD_REG_OFS		0x10ul
#define MCHP_GPSPI_RXD_REG_MASK		0xFFu

/* SPI Clock Control register */
#define MCHP_GPSPI_CC_REG_OFS		0x14ul
#define MCHP_GPSPI_CC_REG_MASK		0x17u
/*
 * Select when GPSPI transmit data is valid on SPDOUT pin.
 * 0 = TX data transmit begines before first clock edge.
 *     Receiver device should sample on first and following odd numbered
 *     clock edges.
 * 1 = TX data transmit begins on first clock edge.
 *     Receiver device should sample on second and following even numbered
 *     clock edges.
 */
#define MCHP_GPSPI_CC_TCLKPH_ODD	0x00u
#define MCHP_GPSPI_CC_TCLKPH_EVEN	0x01u
/*
 * Select when GPSPI samples received data on SPDIN pin(s).
 * 0 = Valid data expected on first SPI clock edge. Data sampled on the
 *     the first and following odd clock edges.
 * 1 = Valid data expected after first clock edge. Data sampled on the
 *     second and following even clock edges.
 */
#define MCHP_GPSPI_CC_RCLKPH_ODD	0x00u
#define MCHP_GPSPI_CC_RCLKPH_EVEN	0x02u

/* SPI clock pin idle state */
#define MCHP_GPSPI_CC_CLK_IDLE_LO	0x00u
#define MCHP_GPSPI_CC_CLK_IDLE_HI	0x04u

/* GPSPI internal clock source */
#define MCHP_GPSPI_CC_CLK_SRC_48M	0x00u
#define MCHP_GPSPI_CC_CLK_SRC_2M	0x10u

/* SPI Clock Generator register */
#define MCHP_GPSPI_CG_REG_OFS		0x18ul
#define MCHP_GPSPI_CG_REG_MASK		0x3Fu
#define MCHP_GPSPI_CG_PRLD_MASK		0x3Fu

/*
 * SPI Clock frequency
 * Clock source of 48Mhz or 2MHz is selected in the Clock Control register.
 * Preload value is set in bits[5:0] of Clock Generator register.
 * SPI_CLK_Freq = (CLOCK_SRC_FREQ / 2 * PRELOAD)
 * Examples:
 * 24MHz SPI clock: Clock Source bit=0(48MHz) and PRELOAD=1
 * 12MHz SPI clock: Clock Source bit=0(48MHz) and PRELOAD=2
 * 1MHz SPI clock: two choices.
   Clock Source bit=0(48MHz) and PRELOAD=24
   Clock Source bit=1(2MHz) and PRELOAD=1
 */

#endif /* #ifndef MEC_GPSPI_H */
