/***************************************************************************//**
* \file cyip_i2s.h
*
* \brief
* I2S IP definitions
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

#ifndef _CYIP_I2S_H_
#define _CYIP_I2S_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     I2S
*******************************************************************************/

#define I2S_SECTION_SIZE                        0x00001000UL

/**
  * \brief I2S registers (I2S)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t RESERVED[3];
  __IOM uint32_t CLOCK_CTL;                     /*!< 0x00000010 Clock control */
   __IM uint32_t RESERVED1[3];
  __IOM uint32_t CMD;                           /*!< 0x00000020 Command */
   __IM uint32_t RESERVED2[7];
  __IOM uint32_t TR_CTL;                        /*!< 0x00000040 Trigger control */
   __IM uint32_t RESERVED3[15];
  __IOM uint32_t TX_CTL;                        /*!< 0x00000080 Transmitter control */
  __IOM uint32_t TX_WATCHDOG;                   /*!< 0x00000084 Transmitter watchdog */
   __IM uint32_t RESERVED4[6];
  __IOM uint32_t RX_CTL;                        /*!< 0x000000A0 Receiver control */
  __IOM uint32_t RX_WATCHDOG;                   /*!< 0x000000A4 Receiver watchdog */
   __IM uint32_t RESERVED5[86];
  __IOM uint32_t TX_FIFO_CTL;                   /*!< 0x00000200 TX FIFO control */
   __IM uint32_t TX_FIFO_STATUS;                /*!< 0x00000204 TX FIFO status */
   __OM uint32_t TX_FIFO_WR;                    /*!< 0x00000208 TX FIFO write */
   __IM uint32_t RESERVED6[61];
  __IOM uint32_t RX_FIFO_CTL;                   /*!< 0x00000300 RX FIFO control */
   __IM uint32_t RX_FIFO_STATUS;                /*!< 0x00000304 RX FIFO status */
   __IM uint32_t RX_FIFO_RD;                    /*!< 0x00000308 RX FIFO read */
   __IM uint32_t RX_FIFO_RD_SILENT;             /*!< 0x0000030C RX FIFO silent read */
   __IM uint32_t RESERVED7[764];
  __IOM uint32_t INTR;                          /*!< 0x00000F00 Interrupt register */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000F04 Interrupt set register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000F08 Interrupt mask register */
   __IM uint32_t INTR_MASKED;                   /*!< 0x00000F0C Interrupt masked register */
} I2S_V1_Type;                                  /*!< Size = 3856 (0xF10) */


/* I2S.CTL */
#define I2S_CTL_TX_ENABLED_Pos                  30UL
#define I2S_CTL_TX_ENABLED_Msk                  0x40000000UL
#define I2S_CTL_RX_ENABLED_Pos                  31UL
#define I2S_CTL_RX_ENABLED_Msk                  0x80000000UL
/* I2S.CLOCK_CTL */
#define I2S_CLOCK_CTL_CLOCK_DIV_Pos             0UL
#define I2S_CLOCK_CTL_CLOCK_DIV_Msk             0x3FUL
#define I2S_CLOCK_CTL_CLOCK_SEL_Pos             8UL
#define I2S_CLOCK_CTL_CLOCK_SEL_Msk             0x100UL
/* I2S.CMD */
#define I2S_CMD_TX_START_Pos                    0UL
#define I2S_CMD_TX_START_Msk                    0x1UL
#define I2S_CMD_TX_PAUSE_Pos                    8UL
#define I2S_CMD_TX_PAUSE_Msk                    0x100UL
#define I2S_CMD_RX_START_Pos                    16UL
#define I2S_CMD_RX_START_Msk                    0x10000UL
/* I2S.TR_CTL */
#define I2S_TR_CTL_TX_REQ_EN_Pos                0UL
#define I2S_TR_CTL_TX_REQ_EN_Msk                0x1UL
#define I2S_TR_CTL_RX_REQ_EN_Pos                16UL
#define I2S_TR_CTL_RX_REQ_EN_Msk                0x10000UL
/* I2S.TX_CTL */
#define I2S_TX_CTL_B_CLOCK_INV_Pos              3UL
#define I2S_TX_CTL_B_CLOCK_INV_Msk              0x8UL
#define I2S_TX_CTL_CH_NR_Pos                    4UL
#define I2S_TX_CTL_CH_NR_Msk                    0x70UL
#define I2S_TX_CTL_MS_Pos                       7UL
#define I2S_TX_CTL_MS_Msk                       0x80UL
#define I2S_TX_CTL_I2S_MODE_Pos                 8UL
#define I2S_TX_CTL_I2S_MODE_Msk                 0x300UL
#define I2S_TX_CTL_WS_PULSE_Pos                 10UL
#define I2S_TX_CTL_WS_PULSE_Msk                 0x400UL
#define I2S_TX_CTL_OVHDATA_Pos                  12UL
#define I2S_TX_CTL_OVHDATA_Msk                  0x1000UL
#define I2S_TX_CTL_WD_EN_Pos                    13UL
#define I2S_TX_CTL_WD_EN_Msk                    0x2000UL
#define I2S_TX_CTL_CH_LEN_Pos                   16UL
#define I2S_TX_CTL_CH_LEN_Msk                   0x70000UL
#define I2S_TX_CTL_WORD_LEN_Pos                 20UL
#define I2S_TX_CTL_WORD_LEN_Msk                 0x700000UL
#define I2S_TX_CTL_SCKO_POL_Pos                 24UL
#define I2S_TX_CTL_SCKO_POL_Msk                 0x1000000UL
#define I2S_TX_CTL_SCKI_POL_Pos                 25UL
#define I2S_TX_CTL_SCKI_POL_Msk                 0x2000000UL
/* I2S.TX_WATCHDOG */
#define I2S_TX_WATCHDOG_WD_COUNTER_Pos          0UL
#define I2S_TX_WATCHDOG_WD_COUNTER_Msk          0xFFFFFFFFUL
/* I2S.RX_CTL */
#define I2S_RX_CTL_B_CLOCK_INV_Pos              3UL
#define I2S_RX_CTL_B_CLOCK_INV_Msk              0x8UL
#define I2S_RX_CTL_CH_NR_Pos                    4UL
#define I2S_RX_CTL_CH_NR_Msk                    0x70UL
#define I2S_RX_CTL_MS_Pos                       7UL
#define I2S_RX_CTL_MS_Msk                       0x80UL
#define I2S_RX_CTL_I2S_MODE_Pos                 8UL
#define I2S_RX_CTL_I2S_MODE_Msk                 0x300UL
#define I2S_RX_CTL_WS_PULSE_Pos                 10UL
#define I2S_RX_CTL_WS_PULSE_Msk                 0x400UL
#define I2S_RX_CTL_WD_EN_Pos                    13UL
#define I2S_RX_CTL_WD_EN_Msk                    0x2000UL
#define I2S_RX_CTL_CH_LEN_Pos                   16UL
#define I2S_RX_CTL_CH_LEN_Msk                   0x70000UL
#define I2S_RX_CTL_WORD_LEN_Pos                 20UL
#define I2S_RX_CTL_WORD_LEN_Msk                 0x700000UL
#define I2S_RX_CTL_BIT_EXTENSION_Pos            23UL
#define I2S_RX_CTL_BIT_EXTENSION_Msk            0x800000UL
#define I2S_RX_CTL_SCKO_POL_Pos                 24UL
#define I2S_RX_CTL_SCKO_POL_Msk                 0x1000000UL
#define I2S_RX_CTL_SCKI_POL_Pos                 25UL
#define I2S_RX_CTL_SCKI_POL_Msk                 0x2000000UL
/* I2S.RX_WATCHDOG */
#define I2S_RX_WATCHDOG_WD_COUNTER_Pos          0UL
#define I2S_RX_WATCHDOG_WD_COUNTER_Msk          0xFFFFFFFFUL
/* I2S.TX_FIFO_CTL */
#define I2S_TX_FIFO_CTL_TRIGGER_LEVEL_Pos       0UL
#define I2S_TX_FIFO_CTL_TRIGGER_LEVEL_Msk       0xFFUL
#define I2S_TX_FIFO_CTL_CLEAR_Pos               16UL
#define I2S_TX_FIFO_CTL_CLEAR_Msk               0x10000UL
#define I2S_TX_FIFO_CTL_FREEZE_Pos              17UL
#define I2S_TX_FIFO_CTL_FREEZE_Msk              0x20000UL
/* I2S.TX_FIFO_STATUS */
#define I2S_TX_FIFO_STATUS_USED_Pos             0UL
#define I2S_TX_FIFO_STATUS_USED_Msk             0x1FFUL
#define I2S_TX_FIFO_STATUS_RD_PTR_Pos           16UL
#define I2S_TX_FIFO_STATUS_RD_PTR_Msk           0xFF0000UL
#define I2S_TX_FIFO_STATUS_WR_PTR_Pos           24UL
#define I2S_TX_FIFO_STATUS_WR_PTR_Msk           0xFF000000UL
/* I2S.TX_FIFO_WR */
#define I2S_TX_FIFO_WR_DATA_Pos                 0UL
#define I2S_TX_FIFO_WR_DATA_Msk                 0xFFFFFFFFUL
/* I2S.RX_FIFO_CTL */
#define I2S_RX_FIFO_CTL_TRIGGER_LEVEL_Pos       0UL
#define I2S_RX_FIFO_CTL_TRIGGER_LEVEL_Msk       0xFFUL
#define I2S_RX_FIFO_CTL_CLEAR_Pos               16UL
#define I2S_RX_FIFO_CTL_CLEAR_Msk               0x10000UL
#define I2S_RX_FIFO_CTL_FREEZE_Pos              17UL
#define I2S_RX_FIFO_CTL_FREEZE_Msk              0x20000UL
/* I2S.RX_FIFO_STATUS */
#define I2S_RX_FIFO_STATUS_USED_Pos             0UL
#define I2S_RX_FIFO_STATUS_USED_Msk             0x1FFUL
#define I2S_RX_FIFO_STATUS_RD_PTR_Pos           16UL
#define I2S_RX_FIFO_STATUS_RD_PTR_Msk           0xFF0000UL
#define I2S_RX_FIFO_STATUS_WR_PTR_Pos           24UL
#define I2S_RX_FIFO_STATUS_WR_PTR_Msk           0xFF000000UL
/* I2S.RX_FIFO_RD */
#define I2S_RX_FIFO_RD_DATA_Pos                 0UL
#define I2S_RX_FIFO_RD_DATA_Msk                 0xFFFFFFFFUL
/* I2S.RX_FIFO_RD_SILENT */
#define I2S_RX_FIFO_RD_SILENT_DATA_Pos          0UL
#define I2S_RX_FIFO_RD_SILENT_DATA_Msk          0xFFFFFFFFUL
/* I2S.INTR */
#define I2S_INTR_TX_TRIGGER_Pos                 0UL
#define I2S_INTR_TX_TRIGGER_Msk                 0x1UL
#define I2S_INTR_TX_NOT_FULL_Pos                1UL
#define I2S_INTR_TX_NOT_FULL_Msk                0x2UL
#define I2S_INTR_TX_EMPTY_Pos                   4UL
#define I2S_INTR_TX_EMPTY_Msk                   0x10UL
#define I2S_INTR_TX_OVERFLOW_Pos                5UL
#define I2S_INTR_TX_OVERFLOW_Msk                0x20UL
#define I2S_INTR_TX_UNDERFLOW_Pos               6UL
#define I2S_INTR_TX_UNDERFLOW_Msk               0x40UL
#define I2S_INTR_TX_WD_Pos                      8UL
#define I2S_INTR_TX_WD_Msk                      0x100UL
#define I2S_INTR_RX_TRIGGER_Pos                 16UL
#define I2S_INTR_RX_TRIGGER_Msk                 0x10000UL
#define I2S_INTR_RX_NOT_EMPTY_Pos               18UL
#define I2S_INTR_RX_NOT_EMPTY_Msk               0x40000UL
#define I2S_INTR_RX_FULL_Pos                    19UL
#define I2S_INTR_RX_FULL_Msk                    0x80000UL
#define I2S_INTR_RX_OVERFLOW_Pos                21UL
#define I2S_INTR_RX_OVERFLOW_Msk                0x200000UL
#define I2S_INTR_RX_UNDERFLOW_Pos               22UL
#define I2S_INTR_RX_UNDERFLOW_Msk               0x400000UL
#define I2S_INTR_RX_WD_Pos                      24UL
#define I2S_INTR_RX_WD_Msk                      0x1000000UL
/* I2S.INTR_SET */
#define I2S_INTR_SET_TX_TRIGGER_Pos             0UL
#define I2S_INTR_SET_TX_TRIGGER_Msk             0x1UL
#define I2S_INTR_SET_TX_NOT_FULL_Pos            1UL
#define I2S_INTR_SET_TX_NOT_FULL_Msk            0x2UL
#define I2S_INTR_SET_TX_EMPTY_Pos               4UL
#define I2S_INTR_SET_TX_EMPTY_Msk               0x10UL
#define I2S_INTR_SET_TX_OVERFLOW_Pos            5UL
#define I2S_INTR_SET_TX_OVERFLOW_Msk            0x20UL
#define I2S_INTR_SET_TX_UNDERFLOW_Pos           6UL
#define I2S_INTR_SET_TX_UNDERFLOW_Msk           0x40UL
#define I2S_INTR_SET_TX_WD_Pos                  8UL
#define I2S_INTR_SET_TX_WD_Msk                  0x100UL
#define I2S_INTR_SET_RX_TRIGGER_Pos             16UL
#define I2S_INTR_SET_RX_TRIGGER_Msk             0x10000UL
#define I2S_INTR_SET_RX_NOT_EMPTY_Pos           18UL
#define I2S_INTR_SET_RX_NOT_EMPTY_Msk           0x40000UL
#define I2S_INTR_SET_RX_FULL_Pos                19UL
#define I2S_INTR_SET_RX_FULL_Msk                0x80000UL
#define I2S_INTR_SET_RX_OVERFLOW_Pos            21UL
#define I2S_INTR_SET_RX_OVERFLOW_Msk            0x200000UL
#define I2S_INTR_SET_RX_UNDERFLOW_Pos           22UL
#define I2S_INTR_SET_RX_UNDERFLOW_Msk           0x400000UL
#define I2S_INTR_SET_RX_WD_Pos                  24UL
#define I2S_INTR_SET_RX_WD_Msk                  0x1000000UL
/* I2S.INTR_MASK */
#define I2S_INTR_MASK_TX_TRIGGER_Pos            0UL
#define I2S_INTR_MASK_TX_TRIGGER_Msk            0x1UL
#define I2S_INTR_MASK_TX_NOT_FULL_Pos           1UL
#define I2S_INTR_MASK_TX_NOT_FULL_Msk           0x2UL
#define I2S_INTR_MASK_TX_EMPTY_Pos              4UL
#define I2S_INTR_MASK_TX_EMPTY_Msk              0x10UL
#define I2S_INTR_MASK_TX_OVERFLOW_Pos           5UL
#define I2S_INTR_MASK_TX_OVERFLOW_Msk           0x20UL
#define I2S_INTR_MASK_TX_UNDERFLOW_Pos          6UL
#define I2S_INTR_MASK_TX_UNDERFLOW_Msk          0x40UL
#define I2S_INTR_MASK_TX_WD_Pos                 8UL
#define I2S_INTR_MASK_TX_WD_Msk                 0x100UL
#define I2S_INTR_MASK_RX_TRIGGER_Pos            16UL
#define I2S_INTR_MASK_RX_TRIGGER_Msk            0x10000UL
#define I2S_INTR_MASK_RX_NOT_EMPTY_Pos          18UL
#define I2S_INTR_MASK_RX_NOT_EMPTY_Msk          0x40000UL
#define I2S_INTR_MASK_RX_FULL_Pos               19UL
#define I2S_INTR_MASK_RX_FULL_Msk               0x80000UL
#define I2S_INTR_MASK_RX_OVERFLOW_Pos           21UL
#define I2S_INTR_MASK_RX_OVERFLOW_Msk           0x200000UL
#define I2S_INTR_MASK_RX_UNDERFLOW_Pos          22UL
#define I2S_INTR_MASK_RX_UNDERFLOW_Msk          0x400000UL
#define I2S_INTR_MASK_RX_WD_Pos                 24UL
#define I2S_INTR_MASK_RX_WD_Msk                 0x1000000UL
/* I2S.INTR_MASKED */
#define I2S_INTR_MASKED_TX_TRIGGER_Pos          0UL
#define I2S_INTR_MASKED_TX_TRIGGER_Msk          0x1UL
#define I2S_INTR_MASKED_TX_NOT_FULL_Pos         1UL
#define I2S_INTR_MASKED_TX_NOT_FULL_Msk         0x2UL
#define I2S_INTR_MASKED_TX_EMPTY_Pos            4UL
#define I2S_INTR_MASKED_TX_EMPTY_Msk            0x10UL
#define I2S_INTR_MASKED_TX_OVERFLOW_Pos         5UL
#define I2S_INTR_MASKED_TX_OVERFLOW_Msk         0x20UL
#define I2S_INTR_MASKED_TX_UNDERFLOW_Pos        6UL
#define I2S_INTR_MASKED_TX_UNDERFLOW_Msk        0x40UL
#define I2S_INTR_MASKED_TX_WD_Pos               8UL
#define I2S_INTR_MASKED_TX_WD_Msk               0x100UL
#define I2S_INTR_MASKED_RX_TRIGGER_Pos          16UL
#define I2S_INTR_MASKED_RX_TRIGGER_Msk          0x10000UL
#define I2S_INTR_MASKED_RX_NOT_EMPTY_Pos        18UL
#define I2S_INTR_MASKED_RX_NOT_EMPTY_Msk        0x40000UL
#define I2S_INTR_MASKED_RX_FULL_Pos             19UL
#define I2S_INTR_MASKED_RX_FULL_Msk             0x80000UL
#define I2S_INTR_MASKED_RX_OVERFLOW_Pos         21UL
#define I2S_INTR_MASKED_RX_OVERFLOW_Msk         0x200000UL
#define I2S_INTR_MASKED_RX_UNDERFLOW_Pos        22UL
#define I2S_INTR_MASKED_RX_UNDERFLOW_Msk        0x400000UL
#define I2S_INTR_MASKED_RX_WD_Pos               24UL
#define I2S_INTR_MASKED_RX_WD_Msk               0x1000000UL


#endif /* _CYIP_I2S_H_ */


/* [] END OF FILE */
