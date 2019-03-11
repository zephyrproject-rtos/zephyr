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

/** @file dma.h
 *MEC1501 DMA controller definitions
 */
/** @defgroup MEC1501 Peripherals DMA
 */

#ifndef _DMA_H
#define _DMA_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define NUM_DMA_CHANNELS            12ul

#define DMA_BLOCK_BASE_ADDR         0x40002400ul
#define DMA_CHAN_OFFSET             0x40
#define DMA_CHAN_ADDR(n)            ((DMA_BLOCK_BASE_ADDR) + (((uint32_t)(n)+1) << 6))

#define DMA_GIRQ_ID                 14
#define DMA_GIRQ_SRC_ADDR           0x4000E078ul
#define DMA_GIRQ_EN_SET_ADDR        0x4000E07Cul
#define DMA_GIRQ_RESULT_ADDR        0x4000E080ul
#define DMA_GIRQ_EN_CLR_ADDR        0x4000E084ul

#define DMACH_GIRQ_NUM              14u
#define DMACH_GIRQ_IDX              ((DMACH_GIRQ_NUM) - 8u)

/*
 * Value used for GIRQ Source, Set Enable,
 * Result, and Clear Enable registers.
 * 0 <= ch < 12
 */
#define DMACH_GIRQ_VAL(ch)           (1ul << (ch))

/*
 * DMA channel direct NVIC external interrupt inputs
 * 0 <= ch < 12
 */
#define DMACH_DIRECT_NVIC_NUM(ch)   (24ul + (ch))


//
// Device Numbers for Channel Control Reg Device Number Field
//
#define DMA_DEVNUM_I2CSMB0_SLV      0ul
#define DMA_DEVNUM_I2CSMB0_MTR      1ul
#define DMA_DEVNUM_I2CSMB1_SLV      2ul
#define DMA_DEVNUM_I2CSMB1_MTR      3ul
#define DMA_DEVNUM_I2CSMB2_SLV      4ul
#define DMA_DEVNUM_I2CSMB2_MTR      5ul
#define DMA_DEVNUM_I2CSMB3_SLV      6ul
#define DMA_DEVNUM_I2CSMB3_MTR      7ul
#define DMA_DEVNUM_I2CSMB4_SLV      8ul
#define DMA_DEVNUM_I2CSMB4_RX       9ul
#define DMA_DEVNUM_QMSPI_TX         10ul
#define DMA_DEVNUM_QMSPI_RX         11ul
#define DMA_DEVNUM_MAX              12ul


#define DMA_CHAN_REG_BLEN              0x40ul


/* DMA Block Layout
 * DMA Main registers start at block base
 * Three registers located at word boundaries (0, 4, 8)
 * Each channel starts at base + ((channel_number + 1) * DMA_CHAN_REG_BLEN)
 * DMA Main @ base
 * DMA Channel 0 @ base + DMA_CHAN_REG_BLEN
 * DMA Channel 1 @ base + (2 * DMA_CHAN_REG_BLEN)
 *
 */

/*
 * DMA Main Registers
 */
#define DMAM_CTRL_OFS                       0x00
#define DMAM_FDATA_RO_OFS                   0x04
#define DMAM_FSM_RO_OFS                     0x08

#define DMAM_CTRL_OFFSET                    0x00ul
#define DMAM_CTRL_MASK                      0x03UL
#define DMAM_CTRL_ENABLE                    (1UL << 0)
#define DMAM_CTRL_SOFT_RESET                (1UL << 1)

/*
 * DMA channel register offsets
 */
#define DMA_ACT_OFS             0x00
#define DMA_MSTART_ADDR_OFS     0x04
#define DMA_MEND_ADDR_OFS       0x08
#define DMA_DEV_ADDR_OFS        0x0C
#define DMA_CNTL_OFS            0x10
#define DMA_ISTS_OFS            0x14
#define DMA_IEN_OFS             0x18
#define DMA_FSM_RO_OFS          0x1C
/* Channels with optional ALU */
#define DMA_ALU_CNTL_OFS        0x20
#define DMA_ALU_DATA_OFS        0x24
#define DMA_ALU_STS_OFS         0x28

/*
 * Channel Activate, Offset 0x00, R/W
 */
#define DMACH_ACTIVATE_REG_OFS          0ul
#define DMACH_ACTIVATE_REG_MASK         0x01ul
#define DMACH_ACTIVATE_VAL              (1UL << 0)

/*
 * Target (destination) Start Memory Address, Offset 0x04
 */
#define DMACH_START_ADDR_REG_OFS        0x04ul
#define DMACH_START_ADDR_REG_MASK       0xFFFFFFFFul

/*
 * Target (destination) End Memory Address, Offset 0x08
 */
#define DMACH_END_ADDR_REG_OFS          0x08ul
#define DMACH_END_ADDR_REG_MASK         0xFFFFFFFFul

/*
 * Source (device) Address, Offset 0x0C
 */
#define DMACH_DEV_ADDR_REG_OFS          0x0Cul
#define DMACH_DEV_ADDR_REG_MASK         0xFFFFFFFFul

/*
 * Control, Offset 0x10
 */
#define DMACH_C_REG_OFS             0x10ul
#define DMACH_C_REG_MASK            0x037FFF3Ful
#define DMACH_C_RUN                 (1UL << 0)
#define DMACH_C_REQ_STS_RO          (1UL << 1)
#define DMACH_C_DONE_STS_RO         (1UL << 2)
#define DMACH_C_CHAN_STS_MASK       (0x03UL << 3)
#define DMACH_C_BUSY_STS_POS        5u
#define DMACH_C_BUSY_STS            (1UL << 5)
#define DMACH_C_DIR_POS             8u
#define DMACH_C_DEV2MEM             (0UL << 8)
#define DMACH_C_MEM2DEV             (1UL << 8)
#define DMACH_C_DEV_NUM_POS         9u
#define DMACH_C_DEV_NUM_MASK0       0x7Ful
#define DMACH_C_DEV_NUM_MASK        (0x7F << 9)
#define DMACH_C_NO_INCR_MEM         (0UL << 16)
#define DMACH_C_INCR_MEM            (1UL << 16)
#define DMACH_C_NO_INCR_DEV         (0UL << 17)
#define DMACH_C_INCR_DEV            (1UL << 17)
#define DMACH_C_LOCK_CHAN           (1UL << 18)
#define DMACH_C_DIS_HWFLC           (1UL << 19)
#define DMACH_C_XFRU_POS            20u
#define DMACH_C_XFRU_MASK0          0x07ul
#define DMACH_C_XFRU_MASK           (0x07ul << 20)
#define DMACH_C_XFRU_1B             (1UL << DMACH_C_XFRU_POS)
#define DMACH_C_XFRU_2B             (2UL << DMACH_C_XFRU_POS)
#define DMACH_C_XFRU_4B             (4UL << DMACH_C_XFRU_POS)
#define DMACH_C_XFER_GO             (1UL << 24)
#define DMACH_C_XFER_ABORT          (1UL << 25)
/* combine direction and device number fields */
#define DMACH_C_DEVDIR_POS          8
#define DMACH_C_DEVDIR_MASK0        0xFF
#define DMACH_C_DEVDIR_MASK         (0xFF << 8)

/*
 * Channel Interrupt Status, Offset 0x14
 */
#define DMACH_STS_REG_OFS           0x14ul
#define DMACH_STS_REG_MASK          0x07ul
#define DMACH_STS_BUS_ERR           (1UL << 0)
#define DMACH_STS_FLOW_CTRL_ERR     (1UL << 1)
#define DMACH_STS_DONE              (1UL << 2)
#define DMACH_STS_ALL               0x07ul

/*
 * Channel Interrupt Enable, Offset 0x18
 */
#define DMACH_IEN_REG_OFS                   0x18ul
#define DMACH_IEN_REG_MASK                  0x07ul
#define DMACH_IEN_BUS_ERR                   (1UL << 0)
#define DMACH_IEN_FLOW_CTRL_ERR             (1UL << 1)
#define DMACH_IEN_DONE                      (1UL << 2)
#define DMACH_IEN_ALL                       0x07ul

/*
 * Channel State, Offset 0x1C
 */
#define DMACH_STATE_REG_OFS                 0x1Cul
#define DMACH_STATE_REG_MASK                0x0FFFFul

/*
 * DMA Block with optional ALU includes four extra registers.
 * Channel's total register allocation is 0x40
 */

/*
 * ALU Control, Offset 0x20
 */
#define DMACH_ALU_CTRL_OFS                  0x20ul
#define DMACH_ALU_CTRL_MASK                 0x03ul
#define DMACH_ALU_ENABLE_POS                0
#define DMACH_ALU_MASK                      (1ul << (DMACH_ALU_ENABLE_POS))
#define DMACH_ALU_DISABLE                   (0ul << (DMACH_ALU_ENABLE_POS))
#define DMACH_ALU_ENABLE                    (1ul << (DMACH_ALU_ENABLE_POS))
#define DMACH_ALU_POST_XFER_EN_POS          1
#define DMACH_ALU_POST_XFER_EN_MASK         (1ul << (DMACH_ALU_POST_XFER_EN_POS))
#define DMACH_ALU_POST_XFER_DIS             (0ul << (DMACH_ALU_POST_XFER_EN_POS))
#define DMACH_ALU_POST_XFER_EN              (1ul << (DMACH_ALU_POST_XFER_EN_POS))

/*
 * ALU Data, Offset 0x24
 */
#define DMACH_ALU_DATA_OFS                  0x24ul
#define DMACH_ALU_DATA_MASK                 0xFFFFFFFFul

/*
 * ALU Status, Offset 0x28 Read-Only
 */
#define DMACH_ALU_STS_OFS               0x28ul
#define DMACH_ALU_STS_MASK              0x0Ful
#define DMACH_ALU_STS_DONE_POS          0u
#define DMACH_ALU_STS_RUN_POS           1u
#define DMACH_ALU_STS_XFR_DONE_POS      2u
#define DMACH_ALU_STS_DATA_RDY_POS      3u
#define DMACH_ALU_STS_DONE              (1ul << (DMACH_ALU_STS_DONE_POS))
#define DMACH_ALU_STS_RUN               (1ul << (DMACH_ALU_STS_RUN_POS))
#define DMACH_ALU_STS_XFR_DONE          (1ul << (DMACH_ALU_STS_XFR_DONE_POS))
#define DMACH_ALU_STS_DATA_RDY          (1ul << ((DMACH_ALU_STS_DATA_RDY_POS))

/*
 * ALU Test, Offset 0x2C Reserved
 */
#define DMACH_ALU_TEST_OFFSET               0x2Cul
#define DMACH_ALU_TEST_MASK                 0xFFFFFFFFul

/*
 * Channel 0 has ALU for CRC32
 * Channel 1 has ALU for memory fill
 * Channels 2-11 do not implement an ALU
 */
#define DMA_NUM_CHAN            12
#define DMA_CHAN_SPACING        0x40
#define DMA_CHAN_SPACING_POF2   6

#define DMA_CHAN_WCRC         0
#define DMA_CHAN_WMF          1
#define MAX_DMA_CHAN          12
#define NUM_DMA_CHAN_NO_ALU   ((MAX_DMA_CHAN) - 2)
/*
 * NOTE: structure size is 0x40 (64) bytes as each channel
 * is spaced every 0x40 bytes from DMA block base address.
 */
typedef struct
{
    __IOM uint8_t  ACTV;            /*!< (@ 0x00000000) DMA channel activate  */
          uint8_t  RSVD1[3];
    __IOM uint32_t MSTART;          /*!< (@ 0x00000004) DMA channel memory start address  */
    __IOM uint32_t MEND;            /*!< (@ 0x00000008) DMA channel memory end address  */
    __IOM uint32_t DSTART;          /*!< (@ 0x0000000C) DMA channel device start address  */
    __IOM uint32_t CTRL;            /*!< (@ 0x00000010) DMA channel control  */
    __IOM uint8_t  ISTS;            /*!< (@ 0x00000014) DMA channel interrupt status  */
          uint8_t  RSVD2[3];
    __IOM uint8_t  IEN;             /*!< (@ 0x00000018) DMA channel interrupt enable  */
          uint8_t  RSVD3[3];
    __IM  uint32_t FSM;             /*!< (@ 0x0000001C) DMA channel FSM (RO)  */
    __IOM uint8_t  CRC_EN;          /*!< (@ 0x00000020) DMA channels [0-1] CRC Enable */
          uint8_t  RSVD4[3];
    __IOM uint32_t CRC_DATA;        /*!< (@ 0x00000024) DMA channels [0-1] CRC Data */
    __IOM uint8_t  CRC_STS;         /*!< (@ 0x00000028) DMA channels [0-1] CRC post status (RO) */
          uint8_t  RSVD5[3];
    __IM  uint32_t CRC_FSM;         /*!< (@ 0x0000002C) DMA channels [0-1] CRC FSM (RO) */
          uint8_t  RSVD6[16];       /* pad to 0x40(64) byte size */
} DMA_CHAN_WCRC_Type; /* DMA Channel with CRC ALU */

typedef struct
{
    __IOM uint8_t  ACTV;            /*!< (@ 0x00000000) DMA channel activate  */
          uint8_t  RSVD1[3];
    __IOM uint32_t MSTART;          /*!< (@ 0x00000004) DMA channel memory start address  */
    __IOM uint32_t MEND;            /*!< (@ 0x00000008) DMA channel memory end address  */
    __IOM uint32_t DSTART;          /*!< (@ 0x0000000C) DMA channel device start address  */
    __IOM uint32_t CTRL;            /*!< (@ 0x00000010) DMA channel control  */
    __IOM uint8_t  ISTS;            /*!< (@ 0x00000014) DMA channel interrupt status  */
          uint8_t  RSVD2[3];
    __IOM uint8_t  IEN;             /*!< (@ 0x00000018) DMA channel interrupt enable  */
          uint8_t  RSVD3[3];
    __IM  uint32_t FSM;             /*!< (@ 0x0000001C) DMA channel FSM (RO)  */
    __IOM uint8_t  FILL_EN;         /*!< (@ 0x00000020) DMA channel 1 Memory Fill Enable */
          uint8_t  RSVD4[3];
    __IOM uint32_t FILL_DATA;       /*!< (@ 0x00000024) DMA channels 1 Memory Fill Data */
    __IOM uint8_t  FILL_STS;        /*!< (@ 0x00000028) DMA channels 1 Memory Fill post status (RO) */
          uint8_t  RSVD5[3];
    __IM  uint32_t FILL_FSM;        /*!< (@ 0x0000002C) DMA channels 1 Memory Fill FSM (RO) */
          uint8_t  RSVD6[16];       /* pad to 0x40(64) byte size */
} DMA_CHAN_WMF_Type; /* DMA Channel with Memory Fill ALU */

typedef struct
{
    __IOM uint8_t  ACTV;            /*!< (@ 0x00000000) DMA channel activate  */
          uint8_t  RSVD1[3];
    __IOM uint32_t MSTART;          /*!< (@ 0x00000004) DMA channel memory start address  */
    __IOM uint32_t MEND;            /*!< (@ 0x00000008) DMA channel memory end address  */
    __IOM uint32_t DSTART;          /*!< (@ 0x0000000C) DMA channel device start address  */
    __IOM uint32_t CTRL;            /*!< (@ 0x00000010) DMA channel control  */
    __IOM uint8_t  ISTS;            /*!< (@ 0x00000014) DMA channel interrupt status  */
          uint8_t  RSVD2[3];
    __IOM uint8_t  IEN;             /*!< (@ 0x00000018) DMA channel interrupt enable  */
          uint8_t  RSVD3[3];
    __IM  uint32_t FSM;             /*!< (@ 0x0000001C) DMA channel FSM (RO)  */
          uint8_t  RSVD4[0x20];     /* pad to 0x40(64) byte size */
} DMA_CHAN_Type; /* DMA Channel with no ALU */

typedef union {
    DMA_CHAN_WCRC_Type  CHWCRC;     /* Channel with CRC-32 ALU */
    DMA_CHAN_WMF_Type   CHWMF;      /* Channel with Memory Fill ALU */
    DMA_CHAN_Type       CHNALU;     /* Channel without ALU */
} DMA_CHAN;

/**
  * @brief DMA Block(DMA)
  */
typedef struct
{                           /*!< (@ 0x40002400) DMA Structure        */
  __IOM uint8_t  ACTRST;    /*!< (@ 0x00000000) DMA block activate/reset */
        uint8_t  RSVDA[3];
  __IOM uint32_t DATA_PKT;  /*!< (@ 0x00000004) DMA data packet (RO) */
  __IOM uint32_t ARB_FSM;   /*!< (@ 0x00000008) DMA Arbiter FSM (RO) */
        uint32_t RSVDB[13];
    DMA_CHAN     CHAN[DMA_NUM_CHAN]; /*!< (@ 0x00000040 - 0x0000027F) DMA Channels 0 - 11 */
} DMA_Type;

#endif // #ifndef _DMA_H
/* end dma.h */
/**   @}
 */
