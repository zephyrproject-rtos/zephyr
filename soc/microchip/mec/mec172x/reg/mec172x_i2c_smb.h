/*
 * Copyright (c) 2021 Microchip Technology Inc. and its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MEC172X_I2C_SMB_H
#define _MEC172X_I2C_SMB_H

#include <stdint.h>
#include <stddef.h>

/* Version 3.7 MCHP I2C/SMBus Controller specification */

#define MCHP_I2C_BAUD_CLK_HZ		16000000u

#define MCHP_I2C_SMB_INST_SPACING	0x400u
#define MCHP_I2C_SMB_INST_SPACING_P2	10u

#define MCHP_I2C_SMB0_BASE_ADDR		0x40004000u
#define MCHP_I2C_SMB1_BASE_ADDR		0x40004400u
#define MCHP_I2C_SMB2_BASE_ADDR		0x40004800u
#define MCHP_I2C_SMB3_BASE_ADDR		0x40004c00u
#define MCHP_I2C_SMB4_BASE_ADDR		0x40005000u

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
#define MCHP_I2C_SMB_CTRL_OFS		0x00u
#define MCHP_I2C_SMB_CTRL_MASK		0xcfu
#define MCHP_I2C_SMB_CTRL_ACK		BIT(0)
#define MCHP_I2C_SMB_CTRL_STO		BIT(1)
#define MCHP_I2C_SMB_CTRL_STA		BIT(2)
#define MCHP_I2C_SMB_CTRL_ENI		BIT(3)
/* bits [5:4] reserved */
#define MCHP_I2C_SMB_CTRL_ESO		BIT(6)
#define MCHP_I2C_SMB_CTRL_PIN		BIT(7)
/* Status Read-only */
#define MCHP_I2C_SMB_STS_OFS		0x00u
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
#define MCHP_I2C_SMB_OWN_ADDR_OFS	0x04u
#define MCHP_I2C_SMB_OWN_ADDR2_OFS	0x05u
#define MCHP_I2C_SMB_OWN_ADDR_MASK	0x7f7fu

/*
 * Offset 0x08
 * Data register, 8-bit
 * Data to be shifted out or shifted in.
 */
#define MCHP_I2C_SMB_DATA_OFS	0x08u

/* Offset 0x0C Leader Command register
 */
#define MCHP_I2C_SMB_MSTR_CMD_OFS		0x0cu
#define MCHP_I2C_SMB_MSTR_CMD_RD_CNT_OFS	0x0fu	/* byte 3 */
#define MCHP_I2C_SMB_MSTR_CMD_WR_CNT_OFS	0x0eu	/* byte 2 */
#define MCHP_I2C_SMB_MSTR_CMD_OP_OFS		0x0du	/* byte 1 */
#define MCHP_I2C_SMB_MSTR_CMD_M_OFS		0x0cu	/* byte 0 */
#define MCHP_I2C_SMB_MSTR_CMD_MASK		0xffff3ff3u
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
#define MCHP_I2C_SMB_SLV_CMD_OFS		0x10u
#define MCHP_I2C_SMB_SLV_CMD_MASK		0x00ffff07u
#define MCHP_I2C_SMB_SLV_CMD_SRUN		BIT(0)
#define MCHP_I2C_SMB_SLV_CMD_SPROCEED		BIT(1)
#define MCHP_I2C_SMB_SLV_CMD_SEND_PEC		BIT(2)
#define MCHP_I2C_SMB_SLV_WR_CNT_POS		8u
#define MCHP_I2C_SMB_SLV_RD_CNT_POS		16u

/* Offset 0x14 PEC CRC register, 8-bit read-write */
#define MCHP_I2C_SMB_PEC_CRC_OFS		0x14u

/* Offset 0x18 Repeated Start Hold Time register, 8-bit read-write */
#define MCHP_I2C_SMB_RSHT_OFS			0x18u

/* Offset 0x20 Completion register, 32-bit */
#define MCHP_I2C_SMB_CMPL_OFS		0x20u
#define MCHP_I2C_SMB_CMPL_MASK		0xe33b7f7Cu
#define MCHP_I2C_SMB_CMPL_RW1C_MASK	0xe1397f00u
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
#define MCHP_I2C_SMB_IDLSC_OFS		0x24u
#define MCHP_I2C_SMB_IDLSC_DLY_OFS	0x24u
#define MCHP_I2C_SMB_IDLSC_BUS_OFS	0x26u
#define MCHP_I2C_SMB_IDLSC_MASK		0x0fff0fffu
#define MCHP_I2C_SMB_IDLSC_BUS_MIN_POS	0u
#define MCHP_I2C_SMB_IDLSC_DLY_POS	16u

/* Offset 0x28 Configuration register */
#define MCHP_I2C_SMB_CFG_OFS		0x28u
#define MCHP_I2C_SMB_CFG_MASK		0xf00f5Fbfu
#define MCHP_I2C_SMB_CFG_PORT_SEL_POS	0
#define MCHP_I2C_SMB_CFG_PORT_SEL_MASK	0x0fu
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
#define MCHP_I2C_SMB_BUS_CLK_OFS	0x2cu
#define MCHP_I2C_SMB_BUS_CLK_MASK	0x0000ffffu
#define MCHP_I2C_SMB_BUS_CLK_LO_POS	0u
#define MCHP_I2C_SMB_BUS_CLK_HI_POS	8u

/* Offset 0x30 Block ID register, 8-bit read-only */
#define MCHP_I2C_SMB_BLOCK_ID_OFS	0x30u
#define MCHP_I2C_SMB_BLOCK_ID_MASK	0xffu

/* Offset 0x34 Block Revision register, 8-bit read-only */
#define MCHP_I2C_SMB_BLOCK_REV_OFS	0x34u
#define MCHP_I2C_SMB_BLOCK_REV_MASK	0xffu

/* Offset 0x38 Bit-Bang Control register, 8-bit read-write */
#define MCHP_I2C_SMB_BB_OFS		0x38u
#define MCHP_I2C_SMB_BB_MASK		0x7fu
#define MCHP_I2C_SMB_BB_EN		BIT(0)
#define MCHP_I2C_SMB_BB_SCL_DIR_IN	0
#define MCHP_I2C_SMB_BB_SCL_DIR_OUT	BIT(1)
#define MCHP_I2C_SMB_BB_SDA_DIR_IN	0
#define MCHP_I2C_SMB_BB_SDA_DIR_OUT	BIT(2)
#define MCHP_I2C_SMB_BB_CL		BIT(3)
#define MCHP_I2C_SMB_BB_DAT		BIT(4)
#define MCHP_I2C_SMB_BB_IN_POS		5u
#define MCHP_I2C_SMB_BB_IN_MASK0	0x03u
#define MCHP_I2C_SMB_BB_IN_MASK		SHLU32(0x03, 5)
#define MCHP_I2C_SMB_BB_CLKI_RO		BIT(5)
#define MCHP_I2C_SMB_BB_DATI_RO		BIT(6)

/* Offset 0x40 Data Timing register */
#define MCHP_I2C_SMB_DATA_TM_OFS		0x40u
#define MCHP_I2C_SMB_DATA_TM_MASK		GENMASK(31, 0)
#define MCHP_I2C_SMB_DATA_TM_DATA_HOLD_POS	0u
#define MCHP_I2C_SMB_DATA_TM_DATA_HOLD_MASK	0xffu
#define MCHP_I2C_SMB_DATA_TM_DATA_HOLD_MASK0	0xffu
#define MCHP_I2C_SMB_DATA_TM_RESTART_POS	8u
#define MCHP_I2C_SMB_DATA_TM_RESTART_MASK	0xff00u
#define MCHP_I2C_SMB_DATA_TM_RESTART_MASK0	0xffu
#define MCHP_I2C_SMB_DATA_TM_STOP_POS		16u
#define MCHP_I2C_SMB_DATA_TM_STOP_MASK		0xff0000u
#define MCHP_I2C_SMB_DATA_TM_STOP_MASK0		0xffu
#define MCHP_I2C_SMB_DATA_TM_FSTART_POS		24u
#define MCHP_I2C_SMB_DATA_TM_FSTART_MASK	0xff000000u
#define MCHP_I2C_SMB_DATA_TM_FSTART_MASK0	0xffu

/* Offset 0x44 Time-out Scaling register */
#define MCHP_I2C_SMB_TMTSC_OFS		0x44u
#define MCHP_I2C_SMB_TMTSC_MASK		GENMASK(31, 0)
#define MCHP_I2C_SMB_TMTSC_CLK_HI_POS	0u
#define MCHP_I2C_SMB_TMTSC_CLK_HI_MASK	0xffu
#define MCHP_I2C_SMB_TMTSC_CLK_HI_MASK0 0xffu
#define MCHP_I2C_SMB_TMTSC_SLV_POS	8u
#define MCHP_I2C_SMB_TMTSC_SLV_MASK	0xff00u
#define MCHP_I2C_SMB_TMTSC_SLV_MASK0	0xffu
#define MCHP_I2C_SMB_TMTSC_MSTR_POS	16u
#define MCHP_I2C_SMB_TMTSC_MSTR_MASK	0xff0000u
#define MCHP_I2C_SMB_TMTSC_MSTR_MASK0	0xffu
#define MCHP_I2C_SMB_TMTSC_BUS_POS	24u
#define MCHP_I2C_SMB_TMTSC_BUS_MASK	0xff000000u
#define MCHP_I2C_SMB_TMTSC_BUS_MASK0	0xffu

/* Offset 0x48 Follower Transmit Buffer register 8-bit read-write */
#define MCHP_I2C_SMB_SLV_TX_BUF_OFS	0x48u

/* Offset 0x4C Follower Receive Buffer register 8-bit read-write */
#define MCHP_I2C_SMB_SLV_RX_BUF_OFS	0x4cu

/* Offset 0x50 Leader Transmit Buffer register 8-bit read-write */
#define MCHP_I2C_SMB_MTR_TX_BUF_OFS	0x50u

/* Offset 0x54 Leader Receive Buffer register 8-bit read-write */
#define MCHP_I2C_SMB_MTR_RX_BUF_OFS	0x54u

/* Offset 0x58 I2C FSM read-only */
#define MCHP_I2C_SMB_I2C_FSM_OFS	0x58u

/* Offset 0x5C SMB Network layer FSM read-only */
#define MCHP_I2C_SMB_FSM_OFS		0x5cu

/* Offset 0x60 Wake Status register */
#define MCHP_I2C_SMB_WAKE_STS_OFS	0x60u
#define MCHP_I2C_SMB_WAKE_STS_START_RWC BIT(0)

/* Offset 0x64 Wake Enable register */
#define MCHP_I2C_SMB_WAKE_EN_OFS	0x64u
#define MCHP_I2C_SMB_WAKE_EN		BIT(0)

/* Offset 0x68 */
#define MCHP_I2C_SMB_WAKE_SYNC_OFS		0x68u
#define MCHP_I2C_SMB_WAKE_FAST_RESYNC_EN	BIT(0)

/** @brief I2C-SMBus with network layer registers.  */
struct i2c_smb_regs {
	volatile uint8_t CTRLSTS;
	uint8_t RSVD1[3];
	volatile uint32_t OWN_ADDR;
	volatile uint8_t I2CDATA;
	uint8_t RSVD2[3];
	volatile uint32_t MCMD;
	volatile uint32_t SCMD;
	volatile uint8_t PEC;
	uint8_t RSVD3[3];
	volatile uint32_t RSHTM;
	volatile uint32_t EXTLEN;
	volatile uint32_t COMPL;
	volatile uint32_t IDLSC;
	volatile uint32_t CFG;
	volatile uint32_t BUSCLK;
	volatile uint32_t BLKID;
	volatile uint32_t BLKREV;
	volatile uint8_t BBCTRL;
	uint8_t RSVD7[3];
	volatile uint32_t CLKSYNC;
	volatile uint32_t DATATM;
	volatile uint32_t TMOUTSC;
	volatile uint8_t SLV_TXB;
	uint8_t RSVD8[3];
	volatile uint8_t SLV_RXB;
	uint8_t RSVD9[3];
	volatile uint8_t MTR_TXB;
	uint8_t RSVD10[3];
	volatile uint8_t MTR_RXB;
	uint8_t RSVD11[3];
	volatile uint32_t FSM;
	volatile uint32_t FSM_SMB;
	volatile uint8_t WAKE_STS;
	uint8_t RSVD12[3];
	volatile uint8_t WAKE_EN;
	uint32_t RSVD13[2];
	volatile uint32_t PROM_ISTS;
	volatile uint32_t PROM_IEN;
	volatile uint32_t PROM_CTRL;
	volatile uint32_t SHADOW_DATA;
}; /* Size = 128(0x80) */

#endif	/* #ifndef _MEC172X_I2C_SMB_H */
