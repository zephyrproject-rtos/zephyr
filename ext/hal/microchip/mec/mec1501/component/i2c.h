/**
 *
 * Copyright (c) 2019 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the Licence at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * \asf_license_stop
 *
 */

/** @file i2c.h
 *MEC1501 I2C register definitions
 */
/** @defgroup MEC1501 Peripherals I2C
 */

#ifndef _I2C_H
#define _I2C_H

#include <stdint.h>
#include <stddef.h>

#include "regaccess.h"

#define MCHP_I2C_MAX_INSTANCES		3u
#define MCHP_I2C_INST_SPACING		0x100ul
#define MCHP_I2C_INST_SPACING_P2	8u

#define MCHP_I2C0_BASE_ADDR	0x40005100ul
#define MCHP_I2C1_BASE_ADDR	0x40005200ul
#define MCHP_I2C2_BASE_ADDR	0x40005300ul

/* 0 <= n < MCHP_I2C_MAX_INSTANCES */
#define MCHP_I2C_BASE_ADDR(n) \
	((MCHP_I2C0_BASE_ADDR) + (n) << (MCHP_I2C_INST_SPACING_P2))

/*
 * Offset 0x00
 * Control and Status register
 * Write to Control
 * Read from Status
 * Size 8-bit
 */
#define MCHP_I2C_CTRL_OFS	0x00ul
#define MCHP_I2C_CTRL_MASK	0xCFu
#define MCHP_I2C_CTRL_ACK	(1u << 0)
#define MCHP_I2C_CTRL_STO	(1u << 1)
#define MCHP_I2C_CTRL_STA	(1u << 2)
#define MCHP_I2C_CTRL_ENI	(1u << 3)
/* bits [5:4] reserved */
#define MCHP_I2C_CTRL_ESO	(1u << 6)
#define MCHP_I2C_CTRL_PIN	(1u << 7)
/* Status Read-only */
#define MCHP_I2C_STS_OFS	0x00ul
#define MCHP_I2C_STS_NBB	(1u << 0)
#define MCHP_I2C_STS_LAB	(1u << 1)
#define MCHP_I2C_STS_AAS	(1u << 2)
#define MCHP_I2C_STS_LRB_AD0	(1u << 3)
#define MCHP_I2C_STS_BER	(1u << 4)
#define MCHP_I2C_STS_EXT_STOP	(1u << 5)
#define MCHP_I2C_STS_PIN	(1u << 7)

/*
 * Offset 0x04
 * Own Address b[7:0] = Slave address 1
 * b[14:8] = Slave address 2
 */
#define MCHP_I2C_OWN_ADDR_OFS	0x04ul
#define MCHP_I2C_OWN_ADDR2_OFS	0x05ul
#define MCHP_I2C_OWN_ADDR_MASK	0x7F7Ful

/*
 * Offset 0x08
 * Data register, 8-bit
 * Data to be shifted out or shifted in.
 */
#define MCHP_I2C_DATA_OFS	0x08ul

/*
 * Offset 0x18
 * Repeated Start Hold Time register, 8-bit read-write
 */
#define MCHP_I2C_RSHT_OFS	0x18ul

/*
 * Offset 0x20
 * Complettion register, 32-bit
 */
#define MCHP_I2C_CMPL_OFS		0x20ul
#define MCHP_I2C_CMPL_MASK		0xE33B7F7Cul
#define MCHP_I2C_CMPL_RW1C_MASK		0xE1397F00ul
#define MCHP_I2C_CMPL_DTEN		(1ul << 2)
#define MCHP_I2C_CMPL_MCEN		(1ul << 3)
#define MCHP_I2C_CMPL_SCEN		(1ul << 4)
#define MCHP_I2C_CMPL_BIDEN		(1ul << 5)
#define MCHP_I2C_CMPL_TIMERR		(1ul << 6)
#define MCHP_I2C_CMPL_DTO_RWC		(1ul << 8)
#define MCHP_I2C_CMPL_MCTO_RWC		(1ul << 9)
#define MCHP_I2C_CMPL_SCTO_RWC		(1ul << 10)
#define MCHP_I2C_CMPL_CHDL_RWC		(1ul << 11)
#define MCHP_I2C_CMPL_CHDH_RWC		(1ul << 12)
#define MCHP_I2C_CMPL_BER_RWC		(1ul << 13)
#define MCHP_I2C_CMPL_LAB_RWC		(1ul << 14)
#define MCHP_I2C_CMPL_SNAKR_RWC		(1ul << 16)
#define MCHP_I2C_CMPL_STR_RO		(1ul << 17)
#define MCHP_I2C_CMPL_RPT_RD_RWC	(1ul << 20)
#define MCHP_I2C_CMPL_RPT_WR_RWC	(1ul << 21)
#define MCHP_I2C_CMPL_MNAKX_RWC		(1ul << 24)
#define MCHP_I2C_CMPL_MTR_RO		(1ul << 25)
#define MCHP_I2C_CMPL_IDLE_RWC		(1ul << 29)
#define MCHP_I2C_CMPL_MDONE_RWC		(1ul << 30)
#define MCHP_I2C_CMPL_SDONE_RWC		(1ul << 31)

/*
 * Offset 0x28
 * Configuration register
 */
#define MCHP_I2C_CFG_OFS		0x28ul
#define MCHP_I2C_CFG_MASK		0x0000C73Ful
#define MCHP_I2C_CFG_PORT_SEL_MASK	0x0Ful
#define MCHP_I2C_CFG_TCEN		(1ul << 4)
#define MCHP_I2C_CFG_SLOW_CLK		(1ul << 5)
#define MCHP_I2C_CFG_FEN		(1ul << 8)
#define MCHP_I2C_CFG_RESET		(1ul << 9)
#define MCHP_I2C_CFG_ENAB		(1ul << 10)
#define MCHP_I2C_CFG_GC_EN		(1ul << 14)
#define MCHP_I2C_CFG_PROM_EN		(1ul << 15)

/*
 * Offset 0x2C
 * Bus Clock register
 */
#define MCHP_I2C_BUS_CLK_OFS		0x2Cul
#define MCHP_I2C_BUS_CLK_MASK		0x0000FFFFul
#define MCHP_I2C_BUS_CLK_LO_POS		0u
#define MCHP_I2C_BUS_CLK_HI_POS		8u

/*
 * Offset 0x30
 * Block ID register, 8-bit read-only
 */
#define MCHP_I2C_BLOCK_ID_OFS		0x30ul
#define MCHP_I2C_BLOCK_ID_MASK		0xFFul

/*
 * Offset 0x34
 * Block Revision register, 8-bit read-only
 */
#define MCHP_I2C_BLOCK_REV_OFS		0x34ul
#define MCHP_I2C_BLOCK_REV_MASK		0xFFul

/*
 * Offset 0x38
 * Bit-Bang Control register, 8-bit read-write
 */
#define MCHP_I2C_BB_OFS			0x38ul
#define MCHP_I2C_BB_MASK		0x7Fu
#define MCHP_I2C_BB_EN			(1ul << 0)
#define MCHP_I2C_BB_SCL_DIR_IN		(0ul << 1)
#define MCHP_I2C_BB_SCL_DIR_OUT		(1ul << 1)
#define MCHP_I2C_BB_SDA_DIR_IN		(0ul << 2)
#define MCHP_I2C_BB_SDA_DIR_OUT		(1ul << 2)
#define MCHP_I2C_BB_CL			(1ul << 3)
#define MCHP_I2C_BB_DAT			(1ul << 4)
#define MCHP_I2C_BB_IN_POS		5u
#define MCHP_I2C_BB_IN_MASK0		0x03ul
#define MCHP_I2C_BB_IN_MASK		(0x03ul << 5)
#define MCHP_I2C_BB_CLKI_RO		(1ul << 5)
#define MCHP_I2C_BB_DATI_RO		(1ul << 6)

/*
 * Offset 0x40
 * Data Timing register
 */
#define MCHP_I2C_DATA_TM_OFS			0x40ul
#define MCHP_I2C_DATA_TM_MASK			0xFFFFFFFFul
#define MCHP_I2C_DATA_TM_DATA_HOLD_POS		0u
#define MCHP_I2C_DATA_TM_DATA_HOLD_MASK		0xFFul
#define MCHP_I2C_DATA_TM_DATA_HOLD_MASK0	0xFFul
#define MCHP_I2C_DATA_TM_RESTART_POS		8u
#define MCHP_I2C_DATA_TM_RESTART_MASK		0xFF00ul
#define MCHP_I2C_DATA_TM_RESTART_MASK0		0xFFul
#define MCHP_I2C_DATA_TM_STOP_POS		16u
#define MCHP_I2C_DATA_TM_STOP_MASK		0xFF0000ul
#define MCHP_I2C_DATA_TM_STOP_MASK0		0xFFul
#define MCHP_I2C_DATA_TM_FSTART_POS		24u
#define MCHP_I2C_DATA_TM_FSTART_MASK		0xFF000000ul
#define MCHP_I2C_DATA_TM_FSTART_MASK0		0xFFul

/*
 * Offset 0x44
 * Time-out Scaling register
 */
#define MCHP_I2C_TMTSC_OFS		0x44ul
#define MCHP_I2C_TMTSC_MASK		0xFFFFFFFFul
#define MCHP_I2C_TMTSC_CLK_HI_POS	0u
#define MCHP_I2C_TMTSC_CLK_HI_MASK	0xFFul
#define MCHP_I2C_TMTSC_CLK_HI_MASK0	0xFFul
#define MCHP_I2C_TMTSC_SLV_POS		8u
#define MCHP_I2C_TMTSC_SLV_MASK		0xFF00ul
#define MCHP_I2C_TMTSC_SLV_MASK0	0xFFul
#define MCHP_I2C_TMTSC_MSTR_POS		16u
#define MCHP_I2C_TMTSC_MSTR_MASK	0xFF0000ul
#define MCHP_I2C_TMTSC_MSTR_MASK0	0xFFul
#define MCHP_I2C_TMTSC_BUS_POS		24u
#define MCHP_I2C_TMTSC_BUS_MASK		0xFF000000ul
#define MCHP_I2C_TMTSC_BUS_MASK0	0xFFul

/*
 * Offset 0x60
 * Wake Status register
 */
#define MCHP_I2C_WAKE_STS_OFS		0x60ul
#define MCHP_I2C_WAKE_STS_START_RWC	(1ul << 0)

/*
 * Offset 0x64
 * Wake Enable register
 */
#define MCHP_I2C_WAKE_EN_OFS		0x64ul
#define MCHP_I2C_WAKE_EN		(1ul << 0)

/*
 * Offset 0x6C
 * Slave Address captured from bus
 */
#define MCHP_I2C_SLV_ADDR_OFS		0x6Cul
#define MCHP_I2C_SLV_ADDR_MASK		0xFFul

/*
 * Offset 0x70
 * Promiscuous Interrupt Status
 */
#define MCHP_I2C_PROM_INTR_STS_OFS	0x70ul
#define MCHP_I2C_PROM_INTR_STS		0x01ul

/*
 * Offset 0x74
 * Promiscuous Interrupt Enable
 */
#define MCHP_I2C_PROM_INTR_EN_OFS	0x74ul
#define MCHP_I2C_PROM_INTR_EN		0x01ul

/*
 * Offset 0x78
 * Promiscuous Control
 */
#define MCHP_I2C_PROM_CTRL_OFS		0x78ul
#define MCHP_I2C_PROM_CTRL_ACK_ADDR	0x01ul
#define MCHP_I2C_PROM_CTRL_NACK_ADDR	0x00ul

/*
 * I2C GIRQ and NVIC mapping
 */
#define MCHP_I2C_GIRQ		3u
#define MCHP_I2C_GIRQ_IDX	(13u - 8u)
#define MCHP_I2C_NVIC_GIRQ	5u
#define MCHP_I2C0_NVIC_DIRECT	168u
#define MCHP_I2C1_NVIC_DIRECT	169u
#define MCHP_I2C2_NVIC_DIRECT	170u

#define MCHP_I2C_GIRQ_SRC_ADDR		0x4000E064ul
#define MCHP_I2C_GIRQ_SET_EN_ADDR	0x4000E068ul
#define MCHP_I2C_GIRQ_RESULT_ADDR	0x4000E06Cul
#define MCHP_I2C_GIRQ_CLR_EN_ADDR	0x4000E070ul

#define MCHP_I2C0_GIRQ_POS	5u
#define MCHP_I2C1_GIRQ_POS	6u
#define MCHP_I2C2_GIRQ_POS	7u

#define MCHP_I2C0_GIRQ_VAL	(1ul << 5)
#define MCHP_I2C1_GIRQ_VAL	(1ul << 6)
#define MCHP_I2C2_GIRQ_VAL	(1ul << 7)

/*
 * Register access by controller base address
 */

/* I2C Control register, write-only */
#define MCHP_I2C_CTRL_WO(ba) REG8(ba)
/* I2C Status register, read-only */
#define MCHP_I2C_STS_RO(ba)  REG8(ba)

#define MCHP_I2C_CTRL(ba) REG8_OFS(ba, MCHP_I2C_CTRL_OFS)

/* Own Address register (slave addresses) */
#define MCHP_I2C_OWN_ADDR(ba) REG16_OFS(ba, MCHP_I2C_OWN_ADDR_OFS)
/* access bits[7:0] OWN_ADDRESS_1 */
#define MCHP_I2C_OWN_ADDR1(ba) REG8_OFS(ba, MCHP_I2C_OWN_ADDR_OFS)
/* access bits[15:8] OWN_ADDRESS_2 */
#define MCHP_I2C_OWN_ADDR2(ba) REG8_OFS(ba, (MCHP_I2C_OWN_ADDR_OFS + 1))

/* I2C Data register */
#define MCHP_I2C_DATA(ba) REG8_OFS(ba, MCHP_I2C_DATA_OFS)

/* Repeated Start Hold Time register */
#define MCHP_I2C_RSHT(ba) REG8_OFS(ba, MCHP_I2C_RSHT_OFS)

/* Completion register */
#define MCHP_I2C_CMPL(ba) REG32_OFS(ba, MCHP_I2C_CMPL_OFS)
/* access only bits[7:0] R/W timeout enables */
#define MCHP_I2C_CMPL_B0(ba) REG8_OFS(ba, MCHP_I2C_CMPL_OFS)

/* Idle Scaling register */
#define MCHP_I2C_IDLSC(ba) REG32_OFS(ba, MCHP_I2C_IDLSC_OFS)

/* Configuration register */
#define MCHP_I2C_CFG(ba) REG32_OFS(ba, MCHP_I2C_CFG_OFS)
/* access each byte */
#define MCHP_I2C_CFG_B0(ba) REG8_OFS(ba, (MCHP_I2C_CFG_OFS + 0x00ul))
#define MCHP_I2C_CFG_B1(ba) REG8_OFS(ba, (MCHP_I2C_CFG_OFS + 0x01ul))
#define MCHP_I2C_CFG_B2(ba) REG8_OFS(ba, (MCHP_I2C_CFG_OFS + 0x02ul))
#define MCHP_I2C_CFG_B3(ba) REG8_OFS(ba, (MCHP_I2C_CFG_OFS + 0x03ul))

/* Bus Clock register */
#define MCHP_I2C_BUS_CLK(ba) REG32_OFS(ba, MCHP_I2C_BUS_CLK_OFS)
#define MCHP_I2C_BUS_CLK_LO_PERIOD(ba) \
	REG8_OFS(ba, (MCHP_I2C_BUS_CLK_OFS + 0x00ul))
#define MCHP_I2C_BUS_CLK_HI_PERIOD(ba) \
	REG8_OFS(ba, (MCHP_I2C_BUS_CLK_OFS + 0x01ul))

/* Bit-Bang Control register */
#define MCHP_I2C_BB_CTRL(ba) REG8_OFS(ba, MCHP_I2C_BB_OFS)

/* MCHP Reserved 0x3C register */
#define MCHP_I2C_RSVD_3C(ba) REG8_OFS(ba, MCHP_I2C_RSVD_3C)

/* Data Timing register */
#define MCHP_I2C_DATA_TM(ba) REG32_OFS(ba, MCHP_I2C_DATA_TM_OFS)

/* Timeout Scaling register */
#define MCHP_I2C_TMTSC(ba) REG32_OFS(ba, MCHP_I2C_TMTSC_OFS)

/* Wake Status register */
#define MCHP_I2C_WAKE_STS(ba) REG8_OFS(ba, MCHP_I2C_WAKE_STS_OFS)

/* Wake Enable register */
#define MCHP_I2C_WAKE_ENABLE(ba) REG8_OFS(ba, MCHP_I2C_WAKE_EN_OFS)

/* Captured Slave Address */
#define MCHP_I2C_SLV_ADDR(ba) REG8_OFS(ba, MCHP_I2C_SLV_ADDR_OFS)

/* Promiscuous Interrupt Status */
#define MCHP_I2C_PROM_ISTS(ba) REG8_OFS(ba, MCHP_I2C_PROM_INTR_STS_OFS)

/* Promiscuous Interrupt Enable */
#define MCHP_I2C_PROM_IEN(ba) REG8_OFS(ba, MCHP_I2C_PROM_INTR_EN_OFS)

/* Promiscuous Interrupt Control */
#define MCHP_I2C_PROM_CTLR(ba) REG8_OFS(ba, MCHP_I2C_PROM_CTRL_OFS)

/* =========================================================================*/
/* ================	       SMB			   ================ */
/* =========================================================================*/

/**
  * @brief SMBus Network Layer Block (SMB)
  */
typedef struct i2c_regs
{		/*!< (@ 0x40004000) SMB Structure	 */
	__IOM uint8_t CTRLSTS;	/*!< (@ 0x00000000) I2C Status(RO), Control(WO) */
	uint8_t RSVD1[3];
	__IOM uint32_t OWN_ADDR;	/*!< (@ 0x00000004) I2C Own address */
	__IOM uint8_t I2CDATA;	/*!< (@ 0x00000008) I2C I2C Data */
	uint8_t RSVD2[15];
	__IOM uint8_t RSHTM;	/*!< (@ 0x00000018) I2C Repeated-Start hold time */
	uint8_t RSVD3[7];
	__IOM uint32_t COMPL;	/*!< (@ 0x00000020) I2C Completion */
	uint8_t RSVD4[4];
	__IOM uint32_t CFG;	/*!< (@ 0x00000028) I2C Configuration */
	__IOM uint32_t BUSCLK;	/*!< (@ 0x0000002C) I2C Bus Clock */
	__IOM uint8_t BLKID;	/*!< (@ 0x00000030) I2C Block ID */
	uint8_t RSVD5[3];
	__IOM uint8_t BLKREV;	/*!< (@ 0x00000034) I2C Block revision */
	uint8_t RSVD6[3];
	__IOM uint8_t BBCTRL;	/*!< (@ 0x00000038) I2C Bit-Bang control */
	uint8_t RSVD7[7];
	__IOM uint32_t DATATM;	/*!< (@ 0x00000040) I2C Data timing */
	__IOM uint32_t TMOUTSC;	/*!< (@ 0x00000044) I2C Time-out scaling */
	uint8_t RSVD8[0x60u - 0x48u];
	__IOM uint8_t WAKE_STS;	/*!< (@ 0x00000060) I2C Wake status */
	uint8_t RSVD9[3];
	__IOM uint8_t WAKE_EN;	/*!< (@ 0x00000064) I2C Wake enable */
	uint8_t RSVD10[4];
	__IOM uint8_t SLV_ADDR;	/*!< (@ 0x0000006C) I2C Slave Address */
	uint8_t RSVD11[3];
	__IOM uint8_t PROM_INTR_STS;	/*!< (@ 0x00000070) I2C Promiscuous Interrupt Status */
	uint8_t RSVD12[3];
	__IOM uint8_t PROM_INTR_EN;	/*!< (@ 0x00000074) I2C Promiscuous Interrupt Enable */
	uint8_t RSVD13[3];
	__IOM uint8_t PROM_CTRL;	/*!< (@ 0x00000078) I2C Promiscuous Interrupt Enable */
	uint8_t RSVD14[3];
} I2C_Type;

#endif				// #ifndef _I2C_H
/* end i2c.h */
/**   @}
 */
