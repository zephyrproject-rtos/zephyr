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

/** @file smb.h
 *MEC1501 SMB register definitions
 */
/** @defgroup MEC1501 Peripherals SMB
 */

#ifndef _SMB_H
#define _SMB_H

#include <stdint.h>
#include <stddef.h>


#define SMB0_BASE_ADDR      0x40004000ul
#define SMB1_BASE_ADDR      0x40004400ul
#define SMB2_BASE_ADDR      0x40004800ul
#define SMB3_BASE_ADDR      0x40004C00ul
#define SMB4_BASE_ADDR      0x40005000ul
#define SMB_INST_OFS_POF2   10u

/* 0 <= n < 5 */
#define SMB_BASE_ADDR(n)    ((SMB0_BASE_ADDR) + (n) << (SMB_INST_OFS_POF2))

/*
 * Offset 0x00
 * Control and Status register
 * Write to Control
 * Read from Status
 * Size 8-bit
 */
#define SMB_CTRL_OFS        0x00
#define SMB_CTRL_MASK       0xCFu
#define SMB_CTRL_ACK        (1u << 0)
#define SMB_CTRL_STO        (1u << 1)
#define SMB_CTRL_STA        (1u << 2)
#define SMB_CTRL_ENI        (1u << 3)
/* bits [5:4] reserved */
#define SMB_CTRL_ESO        (1u << 6)
#define SMB_CTRL_PIN        (1u << 7)
/* Status Read-only */
#define SMB_STS_OFS         0x00
#define SMB_STS_NBB         (1u << 0)
#define SMB_STS_LAB         (1u << 1)
#define SMB_STS_AAS         (1u << 2)
#define SMB_STS_LRB_AD0     (1u << 3)
#define SMB_STS_BER         (1u << 4)
#define SMB_STS_EXT_STOP    (1u << 5)
#define SMB_STS_SAD         (1u << 6)
#define SMB_STS_PIN         (1u << 7)

/*
 * Offset 0x04
 * Own Address b[7:0] = Slave address 1
 * b[14:8] = Slave address 2
 */
#define SMB_OWN_ADDR_OFS    0x04ul
#define SMB_OWN_ADDR2_OFS   0x05ul
#define SMB_OWN_ADDR_MASK   0x7F7Ful

/*
 * Offset 0x08
 * Data register, 8-bit
 * Data to be shifted out or shifted in.
 */
#define SMB_DATA_OFS        0x08ul

/*
 * Offset 0x0C
 * Master Command register
 */
#define SMB_MSTR_CMD_OFS            0x0Cul
#define SMB_MSTR_CMD_RD_CNT_OFS     0x0Ful    /* byte 3 */
#define SMB_MSTR_CMD_WR_CNT_OFS     0x0Eul    /* byte 2 */
#define SMB_MSTR_CMD_OP_OFS         0x0Dul    /* byte 1 */
#define SMB_MSTR_CMD_M_OFS          0x0Cul    /* byte 0 */
#define SMB_MSTR_CMD_MASK           0xFFFF3FF3ul
/* 32-bit definitions */
#define SMB_MSTR_CMD_MRUN           (1ul << 0)
#define SMB_MSTR_CMD_MPROCEED       (1ul << 1)
#define SMB_MSTR_CMD_START0         (1ul << 8)
#define SMB_MSTR_CMD_STARTN         (1ul << 9)
#define SMB_MSTR_CMD_STOP           (1ul << 10)
#define SMB_MSTR_CMD_PEC_TERM       (1ul << 11)
#define SMB_MSTR_CMD_READM          (1ul << 12)
#define SMB_MSTR_CMD_READ_PEC       (1ul << 13)
#define SMB_MSTR_CMD_RD_CNT_POS     24
#define SMB_MSTR_CMD_WR_CNT_POS     16
/* byte 0 definitions */
#define SMB_MSTR_CMD_B0_MRUN        (1ul << 0)
#define SMB_MSTR_CMD_B0_MPROCEED    (1ul << 1)
/* byte 1 definitions */
#define SMB_MSTR_CMD_B1_START0      (1ul << (8-8))
#define SMB_MSTR_CMD_B1_STARTN      (1ul << (9-8))
#define SMB_MSTR_CMD_B1_STOP        (1ul << (10-8))
#define SMB_MSTR_CMD_B1_PEC_TERM    (1ul << (11-8))
#define SMB_MSTR_CMD_B1_READM       (1ul << (12-8))
#define SMB_MSTR_CMD_B1_READ_PEC    (1ul << (13-8))


/*
 * Offset 0x10
 * Slave Command register
 */
#define SMB_SLV_CMD_OFS             0x10ul
#define SMB_SLV_CMD_MASK            0x00FFFF07ul
#define SMB_SLV_CMD_SRUN            (1ul << 0)
#define SMB_SLV_CMD_SPROCEED        (1ul << 1)
#define SMB_SLV_CMD_SEND_PEC        (1ul << 2)
#define SMB_SLV_WR_CNT_POS          8u
#define SMB_SLV_RD_CNT_POS          16u

/*
 * Offset 0x14
 * PEC CRC register, 8-bit read-write
 */
#define SMB_PEC_CRC_OFS             0x14ul

/*
 * Offset 0x18
 * Repeated Start Hold Time register, 8-bit read-write
 */
#define SMB_RSHT_OFS                0x18ul

/*
 * Offset 0x20
 * Complettion register, 32-bit
 */
#define SMB_CMPL_OFS                0x20ul
#define SMB_CMPL_MASK               0xE33B7F7Cul
#define SMB_CMPL_RW1C_MASK          0xE1397F00ul
#define SMB_CMPL_DTEN               (1ul << 2)
#define SMB_CMPL_MCEN               (1ul << 3)
#define SMB_CMPL_SCEN               (1ul << 4)
#define SMB_CMPL_BIDEN              (1ul << 5)
#define SMB_CMPL_TIMERR             (1ul << 6)
#define SMB_CMPL_DTO_RWC            (1ul << 8)
#define SMB_CMPL_MCTO_RWC           (1ul << 9)
#define SMB_CMPL_SCTO_RWC           (1ul << 10)
#define SMB_CMPL_CHDL_RWC           (1ul << 11)
#define SMB_CMPL_CHDH_RWC           (1ul << 12)
#define SMB_CMPL_BER_RWC            (1ul << 13)
#define SMB_CMPL_LEB_RWC            (1ul << 14)
#define SMB_CMPL_SNAKR_RWC          (1ul << 16)
#define SMB_CMPL_STR_RO             (1ul << 17)
#define SMB_CMPL_SPROT_RWC          (1ul << 19)
#define SMB_CMPL_RPT_RD_RWC         (1ul << 20)
#define SMB_CMPL_RPT_WR_RWC         (1ul << 21)
#define SMB_CMPL_MNAKX_RWC          (1ul << 24)
#define SMB_CMPL_MTR_RO             (1ul << 25)
#define SMB_CMPL_IDLE_RWC           (1ul << 29)
#define SMB_CMPL_MDONE_RWC          (1ul << 30)
#define SMB_CMPL_SDONE_RWC          (1ul << 31)

/*
 * Offset 0x24
 * Idle Scaling register
 */
#define SMB_IDLSC_OFS               0x24ul
#define SMB_IDLSC_DLY_OFS           0x24ul
#define SMB_IDLSC_BUS_OFS           0x26ul
#define SMB_IDLSC_MASK              0x0FFF0FFFul
#define SMB_IDLSC_BUS_MIN_POS       0
#define SMB_IDLSC_DLY_POS           16

/*
 * Offset 0x28
 * Configure register
 */
#define SMB_CFG_OFS                 0x28ul
#define SMB_CFG_MASK                0xF00F5FBFul
#define SMB_CFG_PORT_SEL_MASK       0x0Ful
#define SMB_CFG_TCEN                (1ul << 4)
#define SMB_CFG_SLOW_CLK            (1ul << 5)
#define SMB_CFG_PCEN                (1ul << 7)
#define SMB_CFG_FEN                 (1ul << 8)
#define SMB_CFG_RESET               (1ul << 9)
#define SMB_CFG_ENAB                (1ul << 10)
#define SMB_CFG_DSA                 (1ul << 11)
#define SMB_CFG_FAIR                (1ul << 12)
#define SMB_CFG_GC_EN               (1ul << 14)
#define SMB_CFG_FLUSH_SXBUF_WO      (1ul << 16)
#define SMB_CFG_FLUSH_SRBUF_WO      (1ul << 17)
#define SMB_CFG_FLUSH_MXBUF_WO      (1ul << 18)
#define SMB_CFG_FLUSH_MRBUF_WO      (1ul << 19)
#define SMB_CFG_EN_AAS              (1ul << 28)
#define SMB_CFG_ENIDI               (1ul << 29)
#define SMB_CFG_ENMI                (1ul << 30)
#define SMB_CFG_ENSI                (1ul << 31)

/*
 * Offset 0x2C
 * Bus Clock register
 */
#define SMB_BUS_CLK_OFS             0x2Cul
#define SMB_BUS_CLK_MASK            0x0000FFFFul
#define SMB_BUS_CLK_LO_POS          0
#define SMB_BUS_CLK_HI_POS          8

/*
 * Offset 0x30
 * Block ID register, 8-bit read-only
 */
#define SMB_BLOCK_ID_OFS            0x30ul
#define SMB_BLOCK_ID_MASK           0xFFul

/*
 * Offset 0x34
 * Block Revision register, 8-bit read-only
 */
#define SMB_BLOCK_REV_OFS           0x34ul
#define SMB_BLOCK_REV_MASK          0xFFul


/*
 * Offset 0x38
 * Bit-Bang Control register, 8-bit read-write
 */
#define SMB_BB_OFS                  0x38ul
#define SMB_BB_MASK                 0x7Fu
#define SMB_BB_EN                   (1u << 0)
#define SMB_BB_SCL_DIR_IN           (0u << 1)
#define SMB_BB_SCL_DIR_OUT          (1u << 1)
#define SMB_BB_SDA_DIR_IN           (0u << 2)
#define SMB_BB_SDA_DIR_OUT          (1u << 2)
#define SMB_BB_CL                   (1u << 3)
#define SMB_BB_DAT                  (1u << 4)
#define SMB_BB_IN_POS               5u
#define SMB_BB_IN_MASK0             0x03
#define SMB_BB_IN_MASK              (0x03 << 5)
#define SMB_BB_CLKI_RO              (1u << 5)
#define SMB_BB_DATI_RO              (1u << 6)

/*
 * Offset 0x40
 * Data Timing register
 */
#define SMB_DATA_TM_OFS                 0x40ul
#define SMB_DATA_TM_MASK                0xFFFFFFFFul
#define SMB_DATA_TM_DATA_HOLD_POS       0
#define SMB_DATA_TM_DATA_HOLD_MASK      0xFFul
#define SMB_DATA_TM_DATA_HOLD_MASK0     0xFFul
#define SMB_DATA_TM_RESTART_POS         8
#define SMB_DATA_TM_RESTART_MASK        0xFF00ul
#define SMB_DATA_TM_RESTART_MASK0       0xFFul
#define SMB_DATA_TM_STOP_POS            16
#define SMB_DATA_TM_STOP_MASK           0xFF0000ul
#define SMB_DATA_TM_STOP_MASK0          0xFFul
#define SMB_DATA_TM_FSTART_POS          24
#define SMB_DATA_TM_FSTART_MASK         0xFF000000ul
#define SMB_DATA_TM_FSTART_MASK0        0xFFul

/*
 * Offset 0x44
 * Time-out Scaling register
 */
#define SMB_TMTSC_OFS                   0x44ul
#define SMB_TMTSC_MASK                  0xFFFFFFFFul
#define SMB_TMTSC_CLK_HI_POS            0
#define SMB_TMTSC_CLK_HI_MASK           0xFFul
#define SMB_TMTSC_CLK_HI_MASK0          0xFFul
#define SMB_TMTSC_SLV_POS               8
#define SMB_TMTSC_SLV_MASK              0xFF00ul
#define SMB_TMTSC_SLV_MASK0             0xFFul
#define SMB_TMTSC_MSTR_POS              16
#define SMB_TMTSC_MSTR_MASK             0xFF0000ul
#define SMB_TMTSC_MSTR_MASK0            0xFFul
#define SMB_TMTSC_BUS_POS               24
#define SMB_TMTSC_BUS_MASK              0xFF000000ul
#define SMB_TMTSC_BUS_MASK0             0xFFul

/*
 * Offset 0x48
 * Slave Transmit Buffer register
 * 8-bit read-write
 */
#define SMB_SLV_TX_BUF_OFS              0x48ul

/*
 * Offset 0x4C
 * Slave Receive Buffer register
 * 8-bit read-write
 */
#define SMB_SLV_RX_BUF_OFS              0x4Cul

/*
 * Offset 0x50
 * Master Transmit Buffer register
 * 8-bit read-write
 */
#define SMB_MSTR_TX_BUF_OFS             0x50ul

/*
 * Offset 0x54
 * Master Receive Buffer register
 * 8-bit read-write
 */
#define SMB_MSTR_RX_BUF_OFS             0x54ul

/*
 * Offset 0x58
 * I2C FSM read-only
 */
#define SMB_I2C_FSM_OFS                 0x58ul

/*
 * Offset 0x5C
 * SMB Netork layer FSM read-only
 */
#define SMB_FSM_OFS                     0x5Cul

/*
 * Offset 0x60
 * Wake Status register
 */
#define SMB_WAKE_STS_OFS                0x60ul
#define SMB_WAKE_STS_START_RWC          1ul << 0

/*
 * Offset 0x64
 * Wake Enable register
 */
#define SMB_WAKE_EN_OFS                 0x64ul
#define SMB_WAKE_EN                     (1ul << 0)

/*
 * Offset 0x68
 *
 */
#define SMB_WAKE_SYNC_OFS               0x68ul
#define SMB_WAKE_FAST_RESYNC_EN         (1ul << 0)

/*
 * I2C GIRQ and NVIC mapping
 */
#define SMB_GIRQ            13u
#define SMB_GIRQ_IDX        (13u - 8u)
#define SMB_NVIC_BLOCK      5
#define SMB0_NVIC_DIRECT    20
#define SMB1_NVIC_DIRECT    21
#define SMB2_NVIC_DIRECT    22
#define SMB3_NVIC_DIRECT    23
#define SMB4_NVIC_DIRECT    158

#define SMB_GIRQ_SRC_ADDR       0x4000E064ul
#define SMB_GIRQ_SET_EN_ADDR    0x4000E068ul
#define SMB_GIRQ_RESULT_ADDR    0x4000E06Cul
#define SMB_GIRQ_CLR_EN_ADDR    0x4000E070ul

/* register access */

static inline uintptr_t smb_base_addr(uint8_t id)
{
    return (uintptr_t)(SMB0_BASE_ADDR) + ((uintptr_t)id << (SMB_INST_OFS_POF2));
}

/* =========================================================================*/
/* ================            SMB                         ================ */
/* =========================================================================*/
#define SMB_MAX_INSTANCES     5
#define SMB_INST_SPACING      0x400
#define SMB_INST_SPACING_P2   10

typedef union
{
    __OM  uint8_t  CTRL;
    __IM  uint8_t  STS;
} SMB_CTRL_STS_Type;

typedef union
{
    __IOM uint32_t u32;
    __IOM uint16_t u16[2];
    __IOM uint8_t  u8[4];
} SMB_MTRCMD_Type;

typedef union
{
    __IOM uint32_t u32;
    __IOM uint16_t u16[2];
    __IOM uint8_t  u8[4];
} SMB_SLVCMD_Type;

typedef union
{
    __IOM uint32_t u32;
    __IOM uint16_t u16[2];
    __IOM uint8_t  u8[4];
} SMB_COMPL_Type;

typedef union
{
    __IOM uint32_t u32;
    __IOM uint16_t u16[2];
    __IOM uint8_t  u8[4];
} SMB_CFG_Type;

/**
  * @brief SMBus Network Layer Block (SMB)
  */
typedef struct
{                         /*!< (@ 0x40004000) SMB Structure        */
    SMB_CTRL_STS_Type   CTRLSTS;        /*!< (@ 0x00000000) SMB Status(RO), Control(WO) */
          uint8_t       RSVD1[3];
    __IOM uint32_t      OWN_ADDR;       /*!< (@ 0x00000004) SMB Own address 1 */
    __IOM uint8_t       I2CDATA;        /*!< (@ 0x00000008) SMB I2C Data */
          uint8_t       RSVD2[3];
    SMB_MTRCMD_Type     MCMD;           /*!< (@ 0x0000000C) SMB SMB master command */
    SMB_SLVCMD_Type     SCMD;           /*!< (@ 0x00000010) SMB SMB slave command */
    __IOM uint8_t       PEC;            /*!< (@ 0x00000014) SMB PEC value */
          uint8_t       RSVD3[3];
    __IOM uint8_t       RSHTM;          /*!< (@ 0x00000018) SMB Repeated-Start hold time */
          uint8_t       RSVD4[7];
    SMB_COMPL_Type      COMPL;          /*!< (@ 0x00000020) SMB Completion */
    __IOM uint32_t      IDLSC;          /*!< (@ 0x00000024) SMB Idle scaling */
    SMB_CFG_Type        CFG;            /*!< (@ 0x00000028) SMB Configuration */
    __IOM uint32_t      BUSCLK;         /*!< (@ 0x0000002C) SMB Bus Clock */
    __IOM uint8_t       BLKID;          /*!< (@ 0x00000030) SMB Block ID */
          uint8_t       RSVD5[3];
    __IOM uint8_t       BLKREV;         /*!< (@ 0x00000034) SMB Block revision */
          uint8_t       RSVD6[3];
    __IOM uint8_t       BBCTRL;         /*!< (@ 0x00000038) SMB Bit-Bang control */
          uint8_t       RSVD7[3];
    __IOM uint32_t      CLKSYNC;        /*!< (@ 0x0000003C) SMB Clock Sync */
    __IOM uint32_t      DATATM;         /*!< (@ 0x00000040) SMB Data timing */
    __IOM uint32_t      TMOUTSC;        /*!< (@ 0x00000044) SMB Time-out scaling */
    __IOM uint8_t       SLV_TXB;        /*!< (@ 0x00000048) SMB SMB slave TX buffer */
          uint8_t       RSVD8[3];
    __IOM uint8_t       SLV_RXB;        /*!< (@ 0x0000004C) SMB SMB slave RX buffer */
          uint8_t       RSVD9[3];
    __IOM uint8_t       MTR_TXB;        /*!< (@ 0x00000050) SMB SMB Master TX buffer */
          uint8_t       RSVD10[3];
    __IOM uint8_t       MTR_RXB;        /*!< (@ 0x00000054) SMB SMB Master RX buffer */
          uint8_t       RSVD11[3];
    __IOM uint32_t      FSM;            /*!< (@ 0x00000058) SMB FSM (RO) */
    __IOM uint32_t      FSM_SMB;        /*!< (@ 0x0000005C) SMB FSM SMB (RO) */
    __IOM uint8_t       WAKE_STS;       /*!< (@ 0x00000060) SMB Wake status */
          uint8_t       RSVD12[3];
    __IOM uint8_t       WAKE_EN;        /*!< (@ 0x00000064) SMB Wake enable */
          uint8_t       RSVD13[3];
    __IOM uint32_t      FAST_RSYNC;     /*!< (@ 0x00000068) SMB Fast Resync */
} SMB_Type;

#endif // #ifndef _SMB_H
/* end smb.h */
/**   @}
 */
