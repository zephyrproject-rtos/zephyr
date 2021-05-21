/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC_I2C_SMB_H
#define _MEC_I2C_SMB_H

#include <stdint.h>
#include <stddef.h>

#include "mec_defs.h"
#include "mec_regaccess.h"

#define MCHP_I2C_BAUD_CLK_HZ		16000000UL

#define MCHP_I2C_SMB_INST_SPACING	0x400ul
#define MCHP_I2C_SMB_INST_SPACING_P2	10u

#define MCHP_I2C_SMB0_BASE_ADDR		0x40004000ul
#define MCHP_I2C_SMB1_BASE_ADDR		0x40004400ul
#define MCHP_I2C_SMB2_BASE_ADDR		0x40004800ul
#define MCHP_I2C_SMB3_BASE_ADDR		0x40004C00ul
#define MCHP_I2C_SMB4_BASE_ADDR		0x40005000ul

/* 0 <= n < MCHP_I2C_SMB_MAX_INSTANCES */
#define MCHP_I2C_SMB_BASE_ADDR(n)    \
	((MCHP_I2C_SMB0_BASE_ADDR) + \
	 ((uint32_t)(n) * (MCHP_I2C_SMB_INST_SPACING)))

/*
 * Offset 0x00
 * Control and Status register
 * Write to Control
 * Read from Status
 * Size 8-bit
 */
#define MCHP_I2C_SMB_CTRL_OFS		0x00ul
#define MCHP_I2C_SMB_CTRL_MASK		0xCFul
#define MCHP_I2C_SMB_CTRL_ACK		BIT(0)
#define MCHP_I2C_SMB_CTRL_STO		BIT(1)
#define MCHP_I2C_SMB_CTRL_STA		BIT(2)
#define MCHP_I2C_SMB_CTRL_ENI		BIT(3)
/* bits [5:4] reserved */
#define MCHP_I2C_SMB_CTRL_ESO		BIT(6)
#define MCHP_I2C_SMB_CTRL_PIN		BIT(7)
/* Status Read-only */
#define MCHP_I2C_SMB_STS_OFS		0x00ul
#define MCHP_I2C_SMB_STS_NBB		BIT(0)
#define MCHP_I2C_SMB_STS_LAB		BIT(1)
#define MCHP_I2C_SMB_STS_AAS		BIT(2)
#define MCHP_I2C_SMB_STS_LRB_AD0	BIT(3)
#define MCHP_I2C_SMB_STS_BER		BIT(4)
#define MCHP_I2C_SMB_STS_EXT_STOP	BIT(5)
#define MCHP_I2C_SMB_STS_SAD		BIT(6)
#define MCHP_I2C_SMB_STS_PIN		BIT(7)

/*
 * Offset 0x04
 * Own Address b[7:0] = Slave address 1
 * b[14:8] = Slave address 2
 */
#define MCHP_I2C_SMB_OWN_ADDR_OFS	0x04ul
#define MCHP_I2C_SMB_OWN_ADDR2_OFS	0x05ul
#define MCHP_I2C_SMB_OWN_ADDR_MASK	0x7F7Ful

/*
 * Offset 0x08
 * Data register, 8-bit
 * Data to be shifted out or shifted in.
 */
#define MCHP_I2C_SMB_DATA_OFS	0x08ul

/* Offset 0x0C Leader Command register
 */
#define MCHP_I2C_SMB_MSTR_CMD_OFS		0x0Cul
#define MCHP_I2C_SMB_MSTR_CMD_RD_CNT_OFS	0x0Ful	/* byte 3 */
#define MCHP_I2C_SMB_MSTR_CMD_WR_CNT_OFS	0x0Eul	/* byte 2 */
#define MCHP_I2C_SMB_MSTR_CMD_OP_OFS		0x0Dul	/* byte 1 */
#define MCHP_I2C_SMB_MSTR_CMD_M_OFS		0x0Cul	/* byte 0 */
#define MCHP_I2C_SMB_MSTR_CMD_MASK		0xFFFF3FF3ul
/* 32-bit definitions */
#define MCHP_I2C_SMB_MSTR_CMD_MRUN		BIT(0)
#define MCHP_I2C_SMB_MSTR_CMD_MPROCEED		BIT(1)
#define MCHP_I2C_SMB_MSTR_CMD_START0		BIT(8)
#define MCHP_I2C_SMB_MSTR_CMD_STARTN		BIT(9)
#define MCHP_I2C_SMB_MSTR_CMD_STOP		BIT(10)
#define MCHP_I2C_SMB_MSTR_CMD_PEC_TERM		BIT(11)
#define MCHP_I2C_SMB_MSTR_CMD_READM		BIT(12)
#define MCHP_I2C_SMB_MSTR_CMD_READ_PEC		BIT(13)
#define MCHP_I2C_SMB_MSTR_CMD_RD_CNT_POS	24u
#define MCHP_I2C_SMB_MSTR_CMD_WR_CNT_POS	16u
/* byte 0 definitions */
#define MCHP_I2C_SMB_MSTR_CMD_B0_MRUN		BIT(0)
#define MCHP_I2C_SMB_MSTR_CMD_B0_MPROCEED	BIT(1)
/* byte 1 definitions */
#define MCHP_I2C_SMB_MSTR_CMD_B1_START0		BIT((8 - 8))
#define MCHP_I2C_SMB_MSTR_CMD_B1_STARTN		BIT((9 - 8))
#define MCHP_I2C_SMB_MSTR_CMD_B1_STOP		BIT((10 - 8))
#define MCHP_I2C_SMB_MSTR_CMD_B1_PEC_TERM	BIT((11 - 8))
#define MCHP_I2C_SMB_MSTR_CMD_B1_READM		BIT((12 - 8))
#define MCHP_I2C_SMB_MSTR_CMD_B1_READ_PEC	BIT((13 - 8))

/* Offset 0x10 Follower Command register */
#define MCHP_I2C_SMB_SLV_CMD_OFS		0x10ul
#define MCHP_I2C_SMB_SLV_CMD_MASK		0x00FFFF07ul
#define MCHP_I2C_SMB_SLV_CMD_SRUN		BIT(0)
#define MCHP_I2C_SMB_SLV_CMD_SPROCEED		BIT(1)
#define MCHP_I2C_SMB_SLV_CMD_SEND_PEC		BIT(2)
#define MCHP_I2C_SMB_SLV_WR_CNT_POS		8u
#define MCHP_I2C_SMB_SLV_RD_CNT_POS		16u

/* Offset 0x14 PEC CRC register, 8-bit read-write */
#define MCHP_I2C_SMB_PEC_CRC_OFS		0x14ul

/* Offset 0x18 Repeated Start Hold Time register, 8-bit read-write */
#define MCHP_I2C_SMB_RSHT_OFS			0x18ul

/* Offset 0x20 Completion register, 32-bit */
#define MCHP_I2C_SMB_CMPL_OFS		0x20ul
#define MCHP_I2C_SMB_CMPL_MASK		0xE33B7F7Cul
#define MCHP_I2C_SMB_CMPL_RW1C_MASK	0xE1397F00ul
#define MCHP_I2C_SMB_CMPL_DTEN		BIT(2)
#define MCHP_I2C_SMB_CMPL_MCEN		BIT(3)
#define MCHP_I2C_SMB_CMPL_SCEN		BIT(4)
#define MCHP_I2C_SMB_CMPL_BIDEN		BIT(5)
#define MCHP_I2C_SMB_CMPL_TIMERR	BIT(6)
#define MCHP_I2C_SMB_CMPL_DTO_RWC	BIT(8)
#define MCHP_I2C_SMB_CMPL_MCTO_RWC	BIT(9)
#define MCHP_I2C_SMB_CMPL_SCTO_RWC	BIT(10)
#define MCHP_I2C_SMB_CMPL_CHDL_RWC	BIT(11)
#define MCHP_I2C_SMB_CMPL_CHDH_RWC	BIT(12)
#define MCHP_I2C_SMB_CMPL_BER_RWC	BIT(13)
#define MCHP_I2C_SMB_CMPL_LAB_RWC	BIT(14)
#define MCHP_I2C_SMB_CMPL_SNAKR_RWC	BIT(16)
#define MCHP_I2C_SMB_CMPL_STR_RO	BIT(17)
#define MCHP_I2C_SMB_CMPL_SPROT_RWC	BIT(19)
#define MCHP_I2C_SMB_CMPL_RPT_RD_RWC	BIT(20)
#define MCHP_I2C_SMB_CMPL_RPT_WR_RWC	BIT(21)
#define MCHP_I2C_SMB_CMPL_MNAKX_RWC	BIT(24)
#define MCHP_I2C_SMB_CMPL_MTR_RO	BIT(25)
#define MCHP_I2C_SMB_CMPL_IDLE_RWC	BIT(29)
#define MCHP_I2C_SMB_CMPL_MDONE_RWC	BIT(30)
#define MCHP_I2C_SMB_CMPL_SDONE_RWC	BIT(31)

/* Offset 0x24 Idle Scaling register */
#define MCHP_I2C_SMB_IDLSC_OFS		0x24ul
#define MCHP_I2C_SMB_IDLSC_DLY_OFS	0x24ul
#define MCHP_I2C_SMB_IDLSC_BUS_OFS	0x26ul
#define MCHP_I2C_SMB_IDLSC_MASK		0x0FFF0FFFul
#define MCHP_I2C_SMB_IDLSC_BUS_MIN_POS	0u
#define MCHP_I2C_SMB_IDLSC_DLY_POS	16u

/* Offset 0x28 Configuration register */
#define MCHP_I2C_SMB_CFG_OFS		0x28ul
#define MCHP_I2C_SMB_CFG_MASK		0xF00F5FBFul
#define MCHP_I2C_SMB_CFG_PORT_SEL_POS	0
#define MCHP_I2C_SMB_CFG_PORT_SEL_MASK	0x0Ful
#define MCHP_I2C_SMB_CFG_TCEN		BIT(4)
#define MCHP_I2C_SMB_CFG_SLOW_CLK	BIT(5)
#define MCHP_I2C_SMB_CFG_PCEN		BIT(7)
#define MCHP_I2C_SMB_CFG_FEN		BIT(8)
#define MCHP_I2C_SMB_CFG_RESET		BIT(9)
#define MCHP_I2C_SMB_CFG_ENAB		BIT(10)
#define MCHP_I2C_SMB_CFG_DSA		BIT(11)
#define MCHP_I2C_SMB_CFG_FAIR		BIT(12)
#define MCHP_I2C_SMB_CFG_GC_DIS		BIT(14)
#define MCHP_I2C_SMB_CFG_FLUSH_SXBUF_WO BIT(16)
#define MCHP_I2C_SMB_CFG_FLUSH_SRBUF_WO BIT(17)
#define MCHP_I2C_SMB_CFG_FLUSH_MXBUF_WO BIT(18)
#define MCHP_I2C_SMB_CFG_FLUSH_MRBUF_WO BIT(19)
#define MCHP_I2C_SMB_CFG_EN_AAS		BIT(28)
#define MCHP_I2C_SMB_CFG_ENIDI		BIT(29)
#define MCHP_I2C_SMB_CFG_ENMI		BIT(30)
#define MCHP_I2C_SMB_CFG_ENSI		BIT(31)

/* Offset 0x2C Bus Clock register */
#define MCHP_I2C_SMB_BUS_CLK_OFS	0x2Cul
#define MCHP_I2C_SMB_BUS_CLK_MASK	0x0000FFFFul
#define MCHP_I2C_SMB_BUS_CLK_LO_POS	0u
#define MCHP_I2C_SMB_BUS_CLK_HI_POS	8u

/* Offset 0x30 Block ID register, 8-bit read-only */
#define MCHP_I2C_SMB_BLOCK_ID_OFS	0x30ul
#define MCHP_I2C_SMB_BLOCK_ID_MASK	0xFFul

/* Offset 0x34 Block Revision register, 8-bit read-only */
#define MCHP_I2C_SMB_BLOCK_REV_OFS	0x34ul
#define MCHP_I2C_SMB_BLOCK_REV_MASK	0xFFul

/* Offset 0x38 Bit-Bang Control register, 8-bit read-write */
#define MCHP_I2C_SMB_BB_OFS		0x38ul
#define MCHP_I2C_SMB_BB_MASK		0x7Ful
#define MCHP_I2C_SMB_BB_EN		BIT(0)
#define MCHP_I2C_SMB_BB_SCL_DIR_IN	0U
#define MCHP_I2C_SMB_BB_SCL_DIR_OUT	BIT(1)
#define MCHP_I2C_SMB_BB_SDA_DIR_IN	0U
#define MCHP_I2C_SMB_BB_SDA_DIR_OUT	BIT(2)
#define MCHP_I2C_SMB_BB_CL		BIT(3)
#define MCHP_I2C_SMB_BB_DAT		BIT(4)
#define MCHP_I2C_SMB_BB_IN_POS		5u
#define MCHP_I2C_SMB_BB_IN_MASK0	0x03u
#define MCHP_I2C_SMB_BB_IN_MASK		SHLU32(0x03u, 5)
#define MCHP_I2C_SMB_BB_CLKI_RO		BIT(5)
#define MCHP_I2C_SMB_BB_DATI_RO		BIT(6)

/* Offset 0x40 Data Timing register */
#define MCHP_I2C_SMB_DATA_TM_OFS		0x40ul
#define MCHP_I2C_SMB_DATA_TM_MASK		0xFFFFFFFFul
#define MCHP_I2C_SMB_DATA_TM_DATA_HOLD_POS	0u
#define MCHP_I2C_SMB_DATA_TM_DATA_HOLD_MASK	0xFFul
#define MCHP_I2C_SMB_DATA_TM_DATA_HOLD_MASK0	0xFFul
#define MCHP_I2C_SMB_DATA_TM_RESTART_POS	8u
#define MCHP_I2C_SMB_DATA_TM_RESTART_MASK	0xFF00ul
#define MCHP_I2C_SMB_DATA_TM_RESTART_MASK0	0xFFul
#define MCHP_I2C_SMB_DATA_TM_STOP_POS		16u
#define MCHP_I2C_SMB_DATA_TM_STOP_MASK		0xFF0000ul
#define MCHP_I2C_SMB_DATA_TM_STOP_MASK0		0xFFul
#define MCHP_I2C_SMB_DATA_TM_FSTART_POS		24u
#define MCHP_I2C_SMB_DATA_TM_FSTART_MASK	0xFF000000ul
#define MCHP_I2C_SMB_DATA_TM_FSTART_MASK0	0xFFul

/* Offset 0x44 Time-out Scaling register */
#define MCHP_I2C_SMB_TMTSC_OFS		0x44ul
#define MCHP_I2C_SMB_TMTSC_MASK		0xFFFFFFFFul
#define MCHP_I2C_SMB_TMTSC_CLK_HI_POS	0u
#define MCHP_I2C_SMB_TMTSC_CLK_HI_MASK	0xFFul
#define MCHP_I2C_SMB_TMTSC_CLK_HI_MASK0 0xFFul
#define MCHP_I2C_SMB_TMTSC_SLV_POS	8u
#define MCHP_I2C_SMB_TMTSC_SLV_MASK	0xFF00ul
#define MCHP_I2C_SMB_TMTSC_SLV_MASK0	0xFFul
#define MCHP_I2C_SMB_TMTSC_MSTR_POS	16u
#define MCHP_I2C_SMB_TMTSC_MSTR_MASK	0xFF0000ul
#define MCHP_I2C_SMB_TMTSC_MSTR_MASK0	0xFFul
#define MCHP_I2C_SMB_TMTSC_BUS_POS	24u
#define MCHP_I2C_SMB_TMTSC_BUS_MASK	0xFF000000ul
#define MCHP_I2C_SMB_TMTSC_BUS_MASK0	0xFFul

/* Offset 0x48 Follower Transmit Buffer register 8-bit read-write */
#define MCHP_I2C_SMB_SLV_TX_BUF_OFS	0x48ul

/* Offset 0x4C Follower Receive Buffer register 8-bit read-write */
#define MCHP_I2C_SMB_SLV_RX_BUF_OFS	0x4Cul

/* Offset 0x50 Leader Transmit Buffer register 8-bit read-write */
#define MCHP_I2C_SMB_MTR_TX_BUF_OFS	0x50ul

/* Offset 0x54 Leader Receive Buffer register 8-bit read-write */
#define MCHP_I2C_SMB_MTR_RX_BUF_OFS	0x54ul

/* Offset 0x58 I2C FSM read-only */
#define MCHP_I2C_SMB_I2C_FSM_OFS	0x58ul

/* Offset 0x5C SMB Netork layer FSM read-only */
#define MCHP_I2C_SMB_FSM_OFS		0x5Cul

/* Offset 0x60 Wake Status register */
#define MCHP_I2C_SMB_WAKE_STS_OFS	0x60ul
#define MCHP_I2C_SMB_WAKE_STS_START_RWC BIT(0)

/* Offset 0x64 Wake Enable register */
#define MCHP_I2C_SMB_WAKE_EN_OFS	0x64ul
#define MCHP_I2C_SMB_WAKE_EN		BIT(0)

/* Offset 0x68 */
#define MCHP_I2C_SMB_WAKE_SYNC_OFS		0x68ul
#define MCHP_I2C_SMB_WAKE_FAST_RESYNC_EN	BIT(0)

/* I2C GIRQ and NVIC mapping */
#define MCHP_I2C_SMB_GIRQ		13u
#define MCHP_I2C_SMB_GIRQ_IDX		(13u - 8u)
#define MCHP_I2C_SMB_NVIC_GIRQ		5u
#define MCHP_I2C_SMB0_NVIC_DIRECT	20u
#define MCHP_I2C_SMB1_NVIC_DIRECT	21u
#define MCHP_I2C_SMB2_NVIC_DIRECT	22u
#define MCHP_I2C_SMB3_NVIC_DIRECT	23u
#define MCHP_I2C_SMB4_NVIC_DIRECT	158u

#define MCHP_I2C_SMB_GIRQ_SRC_ADDR	0x4000E064ul
#define MCHP_I2C_SMB_GIRQ_SET_EN_ADDR	0x4000E068ul
#define MCHP_I2C_SMB_GIRQ_RESULT_ADDR	0x4000E06Cul
#define MCHP_I2C_SMB_GIRQ_CLR_EN_ADDR	0x4000E070ul

#define MCHP_I2C_SMB0_GIRQ_POS	0u
#define MCHP_I2C_SMB1_GIRQ_POS	1u
#define MCHP_I2C_SMB2_GIRQ_POS	2u
#define MCHP_I2C_SMB3_GIRQ_POS	3u
#define MCHP_I2C_SMB4_GIRQ_POS	4u

#define MCHP_I2C_SMB0_GIRQ_VAL	BIT(0)
#define MCHP_I2C_SMB1_GIRQ_VAL	BIT(1)
#define MCHP_I2C_SMB2_GIRQ_VAL	BIT(2)
#define MCHP_I2C_SMB3_GIRQ_VAL	BIT(3)
#define MCHP_I2C_SMB4_GIRQ_VAL	BIT(4)

/* Register access by controller base address */

/* I2C Control register, write-only */
#define MCHP_I2C_SMB_CTRL_WO(ba) REG8(ba)
/* I2C Status register, read-only */
#define MCHP_I2C_SMB_STS_RO(ba)	 REG8(ba)

#define MCHP_I2C_SMB_CTRL(ba) REG8_OFS(ba, MCHP_I2C_SMB_CTRL_OFS)

/* Own Address register (slave addresses) */
#define MCHP_I2C_SMB_OWN_ADDR(ba) \
	REG16_OFS(ba, MCHP_I2C_SMB_OWN_ADDR_OFS)
/* access bits[7:0] OWN_ADDRESS_1 */
#define MCHP_I2C_SMB_OWN_ADDR1(ba) \
	REG8_OFS(ba, MCHP_I2C_SMB_OWN_ADDR_OFS)
/* access bits[15:8] OWN_ADDRESS_2 */
#define MCHP_I2C_SMB_OWN_ADDR2(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_OWN_ADDR_OFS + 1))

/* I2C Data register */
#define MCHP_I2C_SMB_DATA(ba) \
	REG8_OFS(ba, MCHP_I2C_SMB_DATA_OFS)

/* Network layer Master Command register */
#define MCHP_I2C_SMB_MCMD(ba)  REG32_OFS(ba, MCHP_I2C_SMB_MSTR_CMD_OFS)
#define MCHP_I2C_SMB_MCMD_MRP(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_MSTR_CMD_OFS + 0ul))
#define MCHP_I2C_SMB_MCMD_CTRL(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_MSTR_CMD_OFS + 1ul))
#define MCHP_I2C_SMB_MCMD_WCNT(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_MSTR_CMD_OFS + 2ul))
#define MCHP_I2C_SMB_MCMD_RCNT(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_MSTR_CMD_OFS + 3ul))

/* Network layer Slave Command register */
#define MCHP_I2C_SMB_SCMD(ba) REG32_OFS(ba, MCHP_I2C_SMB_SLV_CMD_OFS)
#define MCHP_I2C_SMB_SCMD_SRP(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_SLV_CMD_OFS + 0ul))
#define MCHP_I2C_SMB_SCMD_WCNT(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_SLV_CMD_OFS + 1ul))
#define MCHP_I2C_SMB_SCMD_RCNT(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_SLV_CMD_OFS + 2ul))

/* PEC register */
#define MCHP_I2C_SMB_PEC(ba) REG8_OFS(ba, MCHP_I2C_SMB_PEC_CRC_OFS)

/* Repeated Start Hold Time register */
#define MCHP_I2C_SMB_RSHT(ba) REG8_OFS(ba, MCHP_I2C_SMB_RSHT_OFS)

/* Completion register */
#define MCHP_I2C_SMB_CMPL(ba) REG32_OFS(ba, MCHP_I2C_SMB_CMPL_OFS)
/* access only bits[7:0] R/W timeout enables */
#define MCHP_I2C_SMB_CMPL_B0(ba) REG8_OFS(ba, MCHP_I2C_SMB_CMPL_OFS)

/* Idle Scaling register */
#define MCHP_I2C_SMB_IDLSC(ba) REG32_OFS(ba, MCHP_I2C_SMB_IDLSC_OFS)

/* Configuration register */
#define MCHP_I2C_SMB_CFG(ba) REG32_OFS(ba, MCHP_I2C_SMB_CFG_OFS)
/* access each byte */
#define MCHP_I2C_SMB_CFG_B0(ba)	\
	REG8_OFS(ba, (MCHP_I2C_SMB_CFG_OFS + 0x00ul))
#define MCHP_I2C_SMB_CFG_B1(ba)	\
	REG8_OFS(ba, (MCHP_I2C_SMB_CFG_OFS + 0x01ul))
#define MCHP_I2C_SMB_CFG_B2(ba)	\
	REG8_OFS(ba, (MCHP_I2C_SMB_CFG_OFS + 0x02ul))
#define MCHP_I2C_SMB_CFG_B3(ba)	\
	REG8_OFS(ba, (MCHP_I2C_SMB_CFG_OFS + 0x03ul))

/* Bus Clock register */
#define MCHP_I2C_SMB_BUS_CLK(ba) REG32_OFS(ba, MCHP_I2C_SMB_BUS_CLK_OFS)
#define MCHP_I2C_SMB_BUS_CLK_LO_PERIOD(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_BUS_CLK_OFS + 0x00ul))
#define MCHP_I2C_SMB_BUS_CLK_HI_PERIOD(ba) \
	REG8_OFS(ba, (MCHP_I2C_SMB_BUS_CLK_OFS + 0x01ul))

#define MCHP_I2C_SMB_BLK_ID(ba) REG8_OFS(ba, MCHP_I2C_SMB_BLOCK_ID_OFS)
#define MCHP_I2C_SMB_BLK_REV(ba) REG8_OFS(ba, MCHP_I2C_SMB_BLOCK_REV_OFS)

/* Bit-Bang Control register */
#define MCHP_I2C_SMB_BB_CTRL(ba) REG8_OFS(ba, MCHP_I2C_SMB_BB_OFS)

/* MCHP Reserved 0x3C register */
#define MCHP_I2C_SMB_RSVD_3C(ba) REG8_OFS(ba, MCHP_I2C_SMB_RSVD_3C)

/* Data Timing register */
#define MCHP_I2C_SMB_DATA_TM(ba) REG32_OFS(ba, MCHP_I2C_SMB_DATA_TM_OFS)

/* Timeout Scaling register */
#define MCHP_I2C_SMB_TMTSC(ba) REG32_OFS(ba, MCHP_I2C_SMB_TMTSC_OFS)

/* Network layer Slave Transmit Buffer register */
#define MCHP_I2C_SMB_SLV_TXB(ba) REG8_OFS(ba, MCHP_I2C_SMB_SLV_TX_BUF_OFS)

/* Network layer Slave Receive Buffer register */
#define MCHP_I2C_SMB_SLV_RXB(ba) REG8_OFS(ba, MCHP_I2C_SMB_SLV_RX_BUF_OFS)

/* Network layer Master Transmit Buffer register */
#define MCHP_I2C_SMB_MTR_TXB(ba) REG8_OFS(ba, MCHP_I2C_SMB_MTR_TX_BUF_OFS)

/* Network layer Master Receive Buffer register */
#define MCHP_I2C_SMB_MTR_RXB(ba) REG8_OFS(ba, MCHP_I2C_SMB_MTR_RX_BUF_OFS)

/* Wake Status register */
#define MCHP_I2C_SMB_WAKE_STS(ba) REG8_OFS(ba, MCHP_I2C_SMB_WAKE_STS_OFS)

/* Wake Enable register */
#define MCHP_I2C_SMB_WAKE_ENABLE(ba) REG8_OFS(ba, MCHP_SMB_WAKE_EN_OFS)

#endif	/* #ifndef _MEC_I2C_SMB_H */
