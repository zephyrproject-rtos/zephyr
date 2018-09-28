/***************************************************************************//**
* \file cyip_smif.h
*
* \brief
* SMIF IP definitions
*
* \note
* Generator version: 1.3.0.1146
* Database revision: rev#1050929
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#ifndef _CYIP_SMIF_H_
#define _CYIP_SMIF_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     SMIF
*******************************************************************************/

#define SMIF_DEVICE_SECTION_SIZE                0x00000080UL
#define SMIF_SECTION_SIZE                       0x00010000UL

/**
  * \brief Device (only used in XIP mode) (SMIF_DEVICE)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t RESERVED;
  __IOM uint32_t ADDR;                          /*!< 0x00000008 Device region base address */
  __IOM uint32_t MASK;                          /*!< 0x0000000C Device region mask */
   __IM uint32_t RESERVED1[4];
  __IOM uint32_t ADDR_CTL;                      /*!< 0x00000020 Address control */
   __IM uint32_t RESERVED2[7];
  __IOM uint32_t RD_CMD_CTL;                    /*!< 0x00000040 Read command control */
  __IOM uint32_t RD_ADDR_CTL;                   /*!< 0x00000044 Read address control */
  __IOM uint32_t RD_MODE_CTL;                   /*!< 0x00000048 Read mode control */
  __IOM uint32_t RD_DUMMY_CTL;                  /*!< 0x0000004C Read dummy control */
  __IOM uint32_t RD_DATA_CTL;                   /*!< 0x00000050 Read data control */
   __IM uint32_t RESERVED3[3];
  __IOM uint32_t WR_CMD_CTL;                    /*!< 0x00000060 Write command control */
  __IOM uint32_t WR_ADDR_CTL;                   /*!< 0x00000064 Write address control */
  __IOM uint32_t WR_MODE_CTL;                   /*!< 0x00000068 Write mode control */
  __IOM uint32_t WR_DUMMY_CTL;                  /*!< 0x0000006C Write dummy control */
  __IOM uint32_t WR_DATA_CTL;                   /*!< 0x00000070 Write data control */
   __IM uint32_t RESERVED4[3];
} SMIF_DEVICE_V1_Type;                          /*!< Size = 128 (0x80) */

/**
  * \brief Serial Memory Interface (SMIF)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t STATUS;                        /*!< 0x00000004 Status */
   __IM uint32_t RESERVED[15];
   __IM uint32_t TX_CMD_FIFO_STATUS;            /*!< 0x00000044 Transmitter command FIFO status */
   __IM uint32_t RESERVED1[2];
   __OM uint32_t TX_CMD_FIFO_WR;                /*!< 0x00000050 Transmitter command FIFO write */
   __IM uint32_t RESERVED2[11];
  __IOM uint32_t TX_DATA_FIFO_CTL;              /*!< 0x00000080 Transmitter data FIFO control */
   __IM uint32_t TX_DATA_FIFO_STATUS;           /*!< 0x00000084 Transmitter data FIFO status */
   __IM uint32_t RESERVED3[2];
   __OM uint32_t TX_DATA_FIFO_WR1;              /*!< 0x00000090 Transmitter data FIFO write */
   __OM uint32_t TX_DATA_FIFO_WR2;              /*!< 0x00000094 Transmitter data FIFO write */
   __OM uint32_t TX_DATA_FIFO_WR4;              /*!< 0x00000098 Transmitter data FIFO write */
   __IM uint32_t RESERVED4[9];
  __IOM uint32_t RX_DATA_FIFO_CTL;              /*!< 0x000000C0 Receiver data FIFO control */
   __IM uint32_t RX_DATA_FIFO_STATUS;           /*!< 0x000000C4 Receiver data FIFO status */
   __IM uint32_t RESERVED5[2];
   __IM uint32_t RX_DATA_FIFO_RD1;              /*!< 0x000000D0 Receiver data FIFO read */
   __IM uint32_t RX_DATA_FIFO_RD2;              /*!< 0x000000D4 Receiver data FIFO read */
   __IM uint32_t RX_DATA_FIFO_RD4;              /*!< 0x000000D8 Receiver data FIFO read */
   __IM uint32_t RESERVED6;
   __IM uint32_t RX_DATA_FIFO_RD1_SILENT;       /*!< 0x000000E0 Receiver data FIFO silent read */
   __IM uint32_t RESERVED7[7];
  __IOM uint32_t SLOW_CA_CTL;                   /*!< 0x00000100 Slow cache control */
   __IM uint32_t RESERVED8;
  __IOM uint32_t SLOW_CA_CMD;                   /*!< 0x00000108 Slow cache command */
   __IM uint32_t RESERVED9[29];
  __IOM uint32_t FAST_CA_CTL;                   /*!< 0x00000180 Fast cache control */
   __IM uint32_t RESERVED10;
  __IOM uint32_t FAST_CA_CMD;                   /*!< 0x00000188 Fast cache command */
   __IM uint32_t RESERVED11[29];
  __IOM uint32_t CRYPTO_CMD;                    /*!< 0x00000200 Cryptography Command */
   __IM uint32_t RESERVED12[7];
  __IOM uint32_t CRYPTO_INPUT0;                 /*!< 0x00000220 Cryptography input 0 */
  __IOM uint32_t CRYPTO_INPUT1;                 /*!< 0x00000224 Cryptography input 1 */
  __IOM uint32_t CRYPTO_INPUT2;                 /*!< 0x00000228 Cryptography input 2 */
  __IOM uint32_t CRYPTO_INPUT3;                 /*!< 0x0000022C Cryptography input 3 */
   __IM uint32_t RESERVED13[4];
   __OM uint32_t CRYPTO_KEY0;                   /*!< 0x00000240 Cryptography key 0 */
   __OM uint32_t CRYPTO_KEY1;                   /*!< 0x00000244 Cryptography key 1 */
   __OM uint32_t CRYPTO_KEY2;                   /*!< 0x00000248 Cryptography key 2 */
   __OM uint32_t CRYPTO_KEY3;                   /*!< 0x0000024C Cryptography key 3 */
   __IM uint32_t RESERVED14[4];
  __IOM uint32_t CRYPTO_OUTPUT0;                /*!< 0x00000260 Cryptography output 0 */
  __IOM uint32_t CRYPTO_OUTPUT1;                /*!< 0x00000264 Cryptography output 1 */
  __IOM uint32_t CRYPTO_OUTPUT2;                /*!< 0x00000268 Cryptography output 2 */
  __IOM uint32_t CRYPTO_OUTPUT3;                /*!< 0x0000026C Cryptography output 3 */
   __IM uint32_t RESERVED15[340];
  __IOM uint32_t INTR;                          /*!< 0x000007C0 Interrupt register */
  __IOM uint32_t INTR_SET;                      /*!< 0x000007C4 Interrupt set register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x000007C8 Interrupt mask register */
   __IM uint32_t INTR_MASKED;                   /*!< 0x000007CC Interrupt masked register */
   __IM uint32_t RESERVED16[12];
        SMIF_DEVICE_V1_Type DEVICE[4];          /*!< 0x00000800 Device (only used in XIP mode) */
} SMIF_V1_Type;                                 /*!< Size = 2560 (0xA00) */


/* SMIF_DEVICE.CTL */
#define SMIF_DEVICE_CTL_WR_EN_Pos               0UL
#define SMIF_DEVICE_CTL_WR_EN_Msk               0x1UL
#define SMIF_DEVICE_CTL_CRYPTO_EN_Pos           8UL
#define SMIF_DEVICE_CTL_CRYPTO_EN_Msk           0x100UL
#define SMIF_DEVICE_CTL_DATA_SEL_Pos            16UL
#define SMIF_DEVICE_CTL_DATA_SEL_Msk            0x30000UL
#define SMIF_DEVICE_CTL_ENABLED_Pos             31UL
#define SMIF_DEVICE_CTL_ENABLED_Msk             0x80000000UL
/* SMIF_DEVICE.ADDR */
#define SMIF_DEVICE_ADDR_ADDR_Pos               8UL
#define SMIF_DEVICE_ADDR_ADDR_Msk               0xFFFFFF00UL
/* SMIF_DEVICE.MASK */
#define SMIF_DEVICE_MASK_MASK_Pos               8UL
#define SMIF_DEVICE_MASK_MASK_Msk               0xFFFFFF00UL
/* SMIF_DEVICE.ADDR_CTL */
#define SMIF_DEVICE_ADDR_CTL_SIZE2_Pos          0UL
#define SMIF_DEVICE_ADDR_CTL_SIZE2_Msk          0x3UL
#define SMIF_DEVICE_ADDR_CTL_DIV2_Pos           8UL
#define SMIF_DEVICE_ADDR_CTL_DIV2_Msk           0x100UL
/* SMIF_DEVICE.RD_CMD_CTL */
#define SMIF_DEVICE_RD_CMD_CTL_CODE_Pos         0UL
#define SMIF_DEVICE_RD_CMD_CTL_CODE_Msk         0xFFUL
#define SMIF_DEVICE_RD_CMD_CTL_WIDTH_Pos        16UL
#define SMIF_DEVICE_RD_CMD_CTL_WIDTH_Msk        0x30000UL
#define SMIF_DEVICE_RD_CMD_CTL_PRESENT_Pos      31UL
#define SMIF_DEVICE_RD_CMD_CTL_PRESENT_Msk      0x80000000UL
/* SMIF_DEVICE.RD_ADDR_CTL */
#define SMIF_DEVICE_RD_ADDR_CTL_WIDTH_Pos       16UL
#define SMIF_DEVICE_RD_ADDR_CTL_WIDTH_Msk       0x30000UL
/* SMIF_DEVICE.RD_MODE_CTL */
#define SMIF_DEVICE_RD_MODE_CTL_CODE_Pos        0UL
#define SMIF_DEVICE_RD_MODE_CTL_CODE_Msk        0xFFUL
#define SMIF_DEVICE_RD_MODE_CTL_WIDTH_Pos       16UL
#define SMIF_DEVICE_RD_MODE_CTL_WIDTH_Msk       0x30000UL
#define SMIF_DEVICE_RD_MODE_CTL_PRESENT_Pos     31UL
#define SMIF_DEVICE_RD_MODE_CTL_PRESENT_Msk     0x80000000UL
/* SMIF_DEVICE.RD_DUMMY_CTL */
#define SMIF_DEVICE_RD_DUMMY_CTL_SIZE5_Pos      0UL
#define SMIF_DEVICE_RD_DUMMY_CTL_SIZE5_Msk      0x1FUL
#define SMIF_DEVICE_RD_DUMMY_CTL_PRESENT_Pos    31UL
#define SMIF_DEVICE_RD_DUMMY_CTL_PRESENT_Msk    0x80000000UL
/* SMIF_DEVICE.RD_DATA_CTL */
#define SMIF_DEVICE_RD_DATA_CTL_WIDTH_Pos       16UL
#define SMIF_DEVICE_RD_DATA_CTL_WIDTH_Msk       0x30000UL
/* SMIF_DEVICE.WR_CMD_CTL */
#define SMIF_DEVICE_WR_CMD_CTL_CODE_Pos         0UL
#define SMIF_DEVICE_WR_CMD_CTL_CODE_Msk         0xFFUL
#define SMIF_DEVICE_WR_CMD_CTL_WIDTH_Pos        16UL
#define SMIF_DEVICE_WR_CMD_CTL_WIDTH_Msk        0x30000UL
#define SMIF_DEVICE_WR_CMD_CTL_PRESENT_Pos      31UL
#define SMIF_DEVICE_WR_CMD_CTL_PRESENT_Msk      0x80000000UL
/* SMIF_DEVICE.WR_ADDR_CTL */
#define SMIF_DEVICE_WR_ADDR_CTL_WIDTH_Pos       16UL
#define SMIF_DEVICE_WR_ADDR_CTL_WIDTH_Msk       0x30000UL
/* SMIF_DEVICE.WR_MODE_CTL */
#define SMIF_DEVICE_WR_MODE_CTL_CODE_Pos        0UL
#define SMIF_DEVICE_WR_MODE_CTL_CODE_Msk        0xFFUL
#define SMIF_DEVICE_WR_MODE_CTL_WIDTH_Pos       16UL
#define SMIF_DEVICE_WR_MODE_CTL_WIDTH_Msk       0x30000UL
#define SMIF_DEVICE_WR_MODE_CTL_PRESENT_Pos     31UL
#define SMIF_DEVICE_WR_MODE_CTL_PRESENT_Msk     0x80000000UL
/* SMIF_DEVICE.WR_DUMMY_CTL */
#define SMIF_DEVICE_WR_DUMMY_CTL_SIZE5_Pos      0UL
#define SMIF_DEVICE_WR_DUMMY_CTL_SIZE5_Msk      0x1FUL
#define SMIF_DEVICE_WR_DUMMY_CTL_PRESENT_Pos    31UL
#define SMIF_DEVICE_WR_DUMMY_CTL_PRESENT_Msk    0x80000000UL
/* SMIF_DEVICE.WR_DATA_CTL */
#define SMIF_DEVICE_WR_DATA_CTL_WIDTH_Pos       16UL
#define SMIF_DEVICE_WR_DATA_CTL_WIDTH_Msk       0x30000UL


/* SMIF.CTL */
#define SMIF_CTL_XIP_MODE_Pos                   0UL
#define SMIF_CTL_XIP_MODE_Msk                   0x1UL
#define SMIF_CTL_CLOCK_IF_RX_SEL_Pos            12UL
#define SMIF_CTL_CLOCK_IF_RX_SEL_Msk            0x3000UL
#define SMIF_CTL_DESELECT_DELAY_Pos             16UL
#define SMIF_CTL_DESELECT_DELAY_Msk             0x70000UL
#define SMIF_CTL_BLOCK_Pos                      24UL
#define SMIF_CTL_BLOCK_Msk                      0x1000000UL
#define SMIF_CTL_ENABLED_Pos                    31UL
#define SMIF_CTL_ENABLED_Msk                    0x80000000UL
/* SMIF.STATUS */
#define SMIF_STATUS_BUSY_Pos                    31UL
#define SMIF_STATUS_BUSY_Msk                    0x80000000UL
/* SMIF.TX_CMD_FIFO_STATUS */
#define SMIF_TX_CMD_FIFO_STATUS_USED3_Pos       0UL
#define SMIF_TX_CMD_FIFO_STATUS_USED3_Msk       0x7UL
/* SMIF.TX_CMD_FIFO_WR */
#define SMIF_TX_CMD_FIFO_WR_DATA20_Pos          0UL
#define SMIF_TX_CMD_FIFO_WR_DATA20_Msk          0xFFFFFUL
/* SMIF.TX_DATA_FIFO_CTL */
#define SMIF_TX_DATA_FIFO_CTL_TRIGGER_LEVEL_Pos 0UL
#define SMIF_TX_DATA_FIFO_CTL_TRIGGER_LEVEL_Msk 0x7UL
/* SMIF.TX_DATA_FIFO_STATUS */
#define SMIF_TX_DATA_FIFO_STATUS_USED4_Pos      0UL
#define SMIF_TX_DATA_FIFO_STATUS_USED4_Msk      0xFUL
/* SMIF.TX_DATA_FIFO_WR1 */
#define SMIF_TX_DATA_FIFO_WR1_DATA0_Pos         0UL
#define SMIF_TX_DATA_FIFO_WR1_DATA0_Msk         0xFFUL
/* SMIF.TX_DATA_FIFO_WR2 */
#define SMIF_TX_DATA_FIFO_WR2_DATA0_Pos         0UL
#define SMIF_TX_DATA_FIFO_WR2_DATA0_Msk         0xFFUL
#define SMIF_TX_DATA_FIFO_WR2_DATA1_Pos         8UL
#define SMIF_TX_DATA_FIFO_WR2_DATA1_Msk         0xFF00UL
/* SMIF.TX_DATA_FIFO_WR4 */
#define SMIF_TX_DATA_FIFO_WR4_DATA0_Pos         0UL
#define SMIF_TX_DATA_FIFO_WR4_DATA0_Msk         0xFFUL
#define SMIF_TX_DATA_FIFO_WR4_DATA1_Pos         8UL
#define SMIF_TX_DATA_FIFO_WR4_DATA1_Msk         0xFF00UL
#define SMIF_TX_DATA_FIFO_WR4_DATA2_Pos         16UL
#define SMIF_TX_DATA_FIFO_WR4_DATA2_Msk         0xFF0000UL
#define SMIF_TX_DATA_FIFO_WR4_DATA3_Pos         24UL
#define SMIF_TX_DATA_FIFO_WR4_DATA3_Msk         0xFF000000UL
/* SMIF.RX_DATA_FIFO_CTL */
#define SMIF_RX_DATA_FIFO_CTL_TRIGGER_LEVEL_Pos 0UL
#define SMIF_RX_DATA_FIFO_CTL_TRIGGER_LEVEL_Msk 0x7UL
/* SMIF.RX_DATA_FIFO_STATUS */
#define SMIF_RX_DATA_FIFO_STATUS_USED4_Pos      0UL
#define SMIF_RX_DATA_FIFO_STATUS_USED4_Msk      0xFUL
/* SMIF.RX_DATA_FIFO_RD1 */
#define SMIF_RX_DATA_FIFO_RD1_DATA0_Pos         0UL
#define SMIF_RX_DATA_FIFO_RD1_DATA0_Msk         0xFFUL
/* SMIF.RX_DATA_FIFO_RD2 */
#define SMIF_RX_DATA_FIFO_RD2_DATA0_Pos         0UL
#define SMIF_RX_DATA_FIFO_RD2_DATA0_Msk         0xFFUL
#define SMIF_RX_DATA_FIFO_RD2_DATA1_Pos         8UL
#define SMIF_RX_DATA_FIFO_RD2_DATA1_Msk         0xFF00UL
/* SMIF.RX_DATA_FIFO_RD4 */
#define SMIF_RX_DATA_FIFO_RD4_DATA0_Pos         0UL
#define SMIF_RX_DATA_FIFO_RD4_DATA0_Msk         0xFFUL
#define SMIF_RX_DATA_FIFO_RD4_DATA1_Pos         8UL
#define SMIF_RX_DATA_FIFO_RD4_DATA1_Msk         0xFF00UL
#define SMIF_RX_DATA_FIFO_RD4_DATA2_Pos         16UL
#define SMIF_RX_DATA_FIFO_RD4_DATA2_Msk         0xFF0000UL
#define SMIF_RX_DATA_FIFO_RD4_DATA3_Pos         24UL
#define SMIF_RX_DATA_FIFO_RD4_DATA3_Msk         0xFF000000UL
/* SMIF.RX_DATA_FIFO_RD1_SILENT */
#define SMIF_RX_DATA_FIFO_RD1_SILENT_DATA0_Pos  0UL
#define SMIF_RX_DATA_FIFO_RD1_SILENT_DATA0_Msk  0xFFUL
/* SMIF.SLOW_CA_CTL */
#define SMIF_SLOW_CA_CTL_WAY_Pos                16UL
#define SMIF_SLOW_CA_CTL_WAY_Msk                0x30000UL
#define SMIF_SLOW_CA_CTL_SET_ADDR_Pos           24UL
#define SMIF_SLOW_CA_CTL_SET_ADDR_Msk           0x3000000UL
#define SMIF_SLOW_CA_CTL_PREF_EN_Pos            30UL
#define SMIF_SLOW_CA_CTL_PREF_EN_Msk            0x40000000UL
#define SMIF_SLOW_CA_CTL_ENABLED_Pos            31UL
#define SMIF_SLOW_CA_CTL_ENABLED_Msk            0x80000000UL
/* SMIF.SLOW_CA_CMD */
#define SMIF_SLOW_CA_CMD_INV_Pos                0UL
#define SMIF_SLOW_CA_CMD_INV_Msk                0x1UL
/* SMIF.FAST_CA_CTL */
#define SMIF_FAST_CA_CTL_WAY_Pos                16UL
#define SMIF_FAST_CA_CTL_WAY_Msk                0x30000UL
#define SMIF_FAST_CA_CTL_SET_ADDR_Pos           24UL
#define SMIF_FAST_CA_CTL_SET_ADDR_Msk           0x3000000UL
#define SMIF_FAST_CA_CTL_PREF_EN_Pos            30UL
#define SMIF_FAST_CA_CTL_PREF_EN_Msk            0x40000000UL
#define SMIF_FAST_CA_CTL_ENABLED_Pos            31UL
#define SMIF_FAST_CA_CTL_ENABLED_Msk            0x80000000UL
/* SMIF.FAST_CA_CMD */
#define SMIF_FAST_CA_CMD_INV_Pos                0UL
#define SMIF_FAST_CA_CMD_INV_Msk                0x1UL
/* SMIF.CRYPTO_CMD */
#define SMIF_CRYPTO_CMD_START_Pos               0UL
#define SMIF_CRYPTO_CMD_START_Msk               0x1UL
/* SMIF.CRYPTO_INPUT0 */
#define SMIF_CRYPTO_INPUT0_INPUT_Pos            0UL
#define SMIF_CRYPTO_INPUT0_INPUT_Msk            0xFFFFFFFFUL
/* SMIF.CRYPTO_INPUT1 */
#define SMIF_CRYPTO_INPUT1_INPUT_Pos            0UL
#define SMIF_CRYPTO_INPUT1_INPUT_Msk            0xFFFFFFFFUL
/* SMIF.CRYPTO_INPUT2 */
#define SMIF_CRYPTO_INPUT2_INPUT_Pos            0UL
#define SMIF_CRYPTO_INPUT2_INPUT_Msk            0xFFFFFFFFUL
/* SMIF.CRYPTO_INPUT3 */
#define SMIF_CRYPTO_INPUT3_INPUT_Pos            0UL
#define SMIF_CRYPTO_INPUT3_INPUT_Msk            0xFFFFFFFFUL
/* SMIF.CRYPTO_KEY0 */
#define SMIF_CRYPTO_KEY0_KEY_Pos                0UL
#define SMIF_CRYPTO_KEY0_KEY_Msk                0xFFFFFFFFUL
/* SMIF.CRYPTO_KEY1 */
#define SMIF_CRYPTO_KEY1_KEY_Pos                0UL
#define SMIF_CRYPTO_KEY1_KEY_Msk                0xFFFFFFFFUL
/* SMIF.CRYPTO_KEY2 */
#define SMIF_CRYPTO_KEY2_KEY_Pos                0UL
#define SMIF_CRYPTO_KEY2_KEY_Msk                0xFFFFFFFFUL
/* SMIF.CRYPTO_KEY3 */
#define SMIF_CRYPTO_KEY3_KEY_Pos                0UL
#define SMIF_CRYPTO_KEY3_KEY_Msk                0xFFFFFFFFUL
/* SMIF.CRYPTO_OUTPUT0 */
#define SMIF_CRYPTO_OUTPUT0_OUTPUT_Pos          0UL
#define SMIF_CRYPTO_OUTPUT0_OUTPUT_Msk          0xFFFFFFFFUL
/* SMIF.CRYPTO_OUTPUT1 */
#define SMIF_CRYPTO_OUTPUT1_OUTPUT_Pos          0UL
#define SMIF_CRYPTO_OUTPUT1_OUTPUT_Msk          0xFFFFFFFFUL
/* SMIF.CRYPTO_OUTPUT2 */
#define SMIF_CRYPTO_OUTPUT2_OUTPUT_Pos          0UL
#define SMIF_CRYPTO_OUTPUT2_OUTPUT_Msk          0xFFFFFFFFUL
/* SMIF.CRYPTO_OUTPUT3 */
#define SMIF_CRYPTO_OUTPUT3_OUTPUT_Pos          0UL
#define SMIF_CRYPTO_OUTPUT3_OUTPUT_Msk          0xFFFFFFFFUL
/* SMIF.INTR */
#define SMIF_INTR_TR_TX_REQ_Pos                 0UL
#define SMIF_INTR_TR_TX_REQ_Msk                 0x1UL
#define SMIF_INTR_TR_RX_REQ_Pos                 1UL
#define SMIF_INTR_TR_RX_REQ_Msk                 0x2UL
#define SMIF_INTR_XIP_ALIGNMENT_ERROR_Pos       2UL
#define SMIF_INTR_XIP_ALIGNMENT_ERROR_Msk       0x4UL
#define SMIF_INTR_TX_CMD_FIFO_OVERFLOW_Pos      3UL
#define SMIF_INTR_TX_CMD_FIFO_OVERFLOW_Msk      0x8UL
#define SMIF_INTR_TX_DATA_FIFO_OVERFLOW_Pos     4UL
#define SMIF_INTR_TX_DATA_FIFO_OVERFLOW_Msk     0x10UL
#define SMIF_INTR_RX_DATA_FIFO_UNDERFLOW_Pos    5UL
#define SMIF_INTR_RX_DATA_FIFO_UNDERFLOW_Msk    0x20UL
/* SMIF.INTR_SET */
#define SMIF_INTR_SET_TR_TX_REQ_Pos             0UL
#define SMIF_INTR_SET_TR_TX_REQ_Msk             0x1UL
#define SMIF_INTR_SET_TR_RX_REQ_Pos             1UL
#define SMIF_INTR_SET_TR_RX_REQ_Msk             0x2UL
#define SMIF_INTR_SET_XIP_ALIGNMENT_ERROR_Pos   2UL
#define SMIF_INTR_SET_XIP_ALIGNMENT_ERROR_Msk   0x4UL
#define SMIF_INTR_SET_TX_CMD_FIFO_OVERFLOW_Pos  3UL
#define SMIF_INTR_SET_TX_CMD_FIFO_OVERFLOW_Msk  0x8UL
#define SMIF_INTR_SET_TX_DATA_FIFO_OVERFLOW_Pos 4UL
#define SMIF_INTR_SET_TX_DATA_FIFO_OVERFLOW_Msk 0x10UL
#define SMIF_INTR_SET_RX_DATA_FIFO_UNDERFLOW_Pos 5UL
#define SMIF_INTR_SET_RX_DATA_FIFO_UNDERFLOW_Msk 0x20UL
/* SMIF.INTR_MASK */
#define SMIF_INTR_MASK_TR_TX_REQ_Pos            0UL
#define SMIF_INTR_MASK_TR_TX_REQ_Msk            0x1UL
#define SMIF_INTR_MASK_TR_RX_REQ_Pos            1UL
#define SMIF_INTR_MASK_TR_RX_REQ_Msk            0x2UL
#define SMIF_INTR_MASK_XIP_ALIGNMENT_ERROR_Pos  2UL
#define SMIF_INTR_MASK_XIP_ALIGNMENT_ERROR_Msk  0x4UL
#define SMIF_INTR_MASK_TX_CMD_FIFO_OVERFLOW_Pos 3UL
#define SMIF_INTR_MASK_TX_CMD_FIFO_OVERFLOW_Msk 0x8UL
#define SMIF_INTR_MASK_TX_DATA_FIFO_OVERFLOW_Pos 4UL
#define SMIF_INTR_MASK_TX_DATA_FIFO_OVERFLOW_Msk 0x10UL
#define SMIF_INTR_MASK_RX_DATA_FIFO_UNDERFLOW_Pos 5UL
#define SMIF_INTR_MASK_RX_DATA_FIFO_UNDERFLOW_Msk 0x20UL
/* SMIF.INTR_MASKED */
#define SMIF_INTR_MASKED_TR_TX_REQ_Pos          0UL
#define SMIF_INTR_MASKED_TR_TX_REQ_Msk          0x1UL
#define SMIF_INTR_MASKED_TR_RX_REQ_Pos          1UL
#define SMIF_INTR_MASKED_TR_RX_REQ_Msk          0x2UL
#define SMIF_INTR_MASKED_XIP_ALIGNMENT_ERROR_Pos 2UL
#define SMIF_INTR_MASKED_XIP_ALIGNMENT_ERROR_Msk 0x4UL
#define SMIF_INTR_MASKED_TX_CMD_FIFO_OVERFLOW_Pos 3UL
#define SMIF_INTR_MASKED_TX_CMD_FIFO_OVERFLOW_Msk 0x8UL
#define SMIF_INTR_MASKED_TX_DATA_FIFO_OVERFLOW_Pos 4UL
#define SMIF_INTR_MASKED_TX_DATA_FIFO_OVERFLOW_Msk 0x10UL
#define SMIF_INTR_MASKED_RX_DATA_FIFO_UNDERFLOW_Pos 5UL
#define SMIF_INTR_MASKED_RX_DATA_FIFO_UNDERFLOW_Msk 0x20UL


#endif /* _CYIP_SMIF_H_ */


/* [] END OF FILE */
