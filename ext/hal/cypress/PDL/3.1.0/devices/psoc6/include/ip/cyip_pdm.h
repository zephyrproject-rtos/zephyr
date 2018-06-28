/***************************************************************************//**
* \file cyip_pdm.h
*
* \brief
* PDM IP definitions
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

#ifndef _CYIP_PDM_H_
#define _CYIP_PDM_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     PDM
*******************************************************************************/

#define PDM_SECTION_SIZE                        0x00001000UL

/**
  * \brief PDM registers (PDM)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t RESERVED[3];
  __IOM uint32_t CLOCK_CTL;                     /*!< 0x00000010 Clock control */
  __IOM uint32_t MODE_CTL;                      /*!< 0x00000014 Mode control */
  __IOM uint32_t DATA_CTL;                      /*!< 0x00000018 Data control */
   __IM uint32_t RESERVED1;
  __IOM uint32_t CMD;                           /*!< 0x00000020 Command */
   __IM uint32_t RESERVED2[7];
  __IOM uint32_t TR_CTL;                        /*!< 0x00000040 Trigger control */
   __IM uint32_t RESERVED3[175];
  __IOM uint32_t RX_FIFO_CTL;                   /*!< 0x00000300 RX FIFO control */
   __IM uint32_t RX_FIFO_STATUS;                /*!< 0x00000304 RX FIFO status */
   __IM uint32_t RX_FIFO_RD;                    /*!< 0x00000308 RX FIFO read */
   __IM uint32_t RX_FIFO_RD_SILENT;             /*!< 0x0000030C RX FIFO silent read */
   __IM uint32_t RESERVED4[764];
  __IOM uint32_t INTR;                          /*!< 0x00000F00 Interrupt register */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000F04 Interrupt set register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000F08 Interrupt mask register */
   __IM uint32_t INTR_MASKED;                   /*!< 0x00000F0C Interrupt masked register */
} PDM_V1_Type;                                  /*!< Size = 3856 (0xF10) */


/* PDM.CTL */
#define PDM_CTL_PGA_R_Pos                       0UL
#define PDM_CTL_PGA_R_Msk                       0xFUL
#define PDM_CTL_PGA_L_Pos                       8UL
#define PDM_CTL_PGA_L_Msk                       0xF00UL
#define PDM_CTL_SOFT_MUTE_Pos                   16UL
#define PDM_CTL_SOFT_MUTE_Msk                   0x10000UL
#define PDM_CTL_STEP_SEL_Pos                    17UL
#define PDM_CTL_STEP_SEL_Msk                    0x20000UL
#define PDM_CTL_ENABLED_Pos                     31UL
#define PDM_CTL_ENABLED_Msk                     0x80000000UL
/* PDM.CLOCK_CTL */
#define PDM_CLOCK_CTL_CLK_CLOCK_DIV_Pos         0UL
#define PDM_CLOCK_CTL_CLK_CLOCK_DIV_Msk         0x3UL
#define PDM_CLOCK_CTL_MCLKQ_CLOCK_DIV_Pos       4UL
#define PDM_CLOCK_CTL_MCLKQ_CLOCK_DIV_Msk       0x30UL
#define PDM_CLOCK_CTL_CKO_CLOCK_DIV_Pos         8UL
#define PDM_CLOCK_CTL_CKO_CLOCK_DIV_Msk         0xF00UL
#define PDM_CLOCK_CTL_SINC_RATE_Pos             16UL
#define PDM_CLOCK_CTL_SINC_RATE_Msk             0x7F0000UL
/* PDM.MODE_CTL */
#define PDM_MODE_CTL_PCM_CH_SET_Pos             0UL
#define PDM_MODE_CTL_PCM_CH_SET_Msk             0x3UL
#define PDM_MODE_CTL_SWAP_LR_Pos                2UL
#define PDM_MODE_CTL_SWAP_LR_Msk                0x4UL
#define PDM_MODE_CTL_S_CYCLES_Pos               8UL
#define PDM_MODE_CTL_S_CYCLES_Msk               0x700UL
#define PDM_MODE_CTL_CKO_DELAY_Pos              16UL
#define PDM_MODE_CTL_CKO_DELAY_Msk              0x70000UL
#define PDM_MODE_CTL_HPF_GAIN_Pos               24UL
#define PDM_MODE_CTL_HPF_GAIN_Msk               0xF000000UL
#define PDM_MODE_CTL_HPF_EN_N_Pos               28UL
#define PDM_MODE_CTL_HPF_EN_N_Msk               0x10000000UL
/* PDM.DATA_CTL */
#define PDM_DATA_CTL_WORD_LEN_Pos               0UL
#define PDM_DATA_CTL_WORD_LEN_Msk               0x3UL
#define PDM_DATA_CTL_BIT_EXTENSION_Pos          8UL
#define PDM_DATA_CTL_BIT_EXTENSION_Msk          0x100UL
/* PDM.CMD */
#define PDM_CMD_STREAM_EN_Pos                   0UL
#define PDM_CMD_STREAM_EN_Msk                   0x1UL
/* PDM.TR_CTL */
#define PDM_TR_CTL_RX_REQ_EN_Pos                16UL
#define PDM_TR_CTL_RX_REQ_EN_Msk                0x10000UL
/* PDM.RX_FIFO_CTL */
#define PDM_RX_FIFO_CTL_TRIGGER_LEVEL_Pos       0UL
#define PDM_RX_FIFO_CTL_TRIGGER_LEVEL_Msk       0xFFUL
#define PDM_RX_FIFO_CTL_CLEAR_Pos               16UL
#define PDM_RX_FIFO_CTL_CLEAR_Msk               0x10000UL
#define PDM_RX_FIFO_CTL_FREEZE_Pos              17UL
#define PDM_RX_FIFO_CTL_FREEZE_Msk              0x20000UL
/* PDM.RX_FIFO_STATUS */
#define PDM_RX_FIFO_STATUS_USED_Pos             0UL
#define PDM_RX_FIFO_STATUS_USED_Msk             0xFFUL
#define PDM_RX_FIFO_STATUS_RD_PTR_Pos           16UL
#define PDM_RX_FIFO_STATUS_RD_PTR_Msk           0xFF0000UL
#define PDM_RX_FIFO_STATUS_WR_PTR_Pos           24UL
#define PDM_RX_FIFO_STATUS_WR_PTR_Msk           0xFF000000UL
/* PDM.RX_FIFO_RD */
#define PDM_RX_FIFO_RD_DATA_Pos                 0UL
#define PDM_RX_FIFO_RD_DATA_Msk                 0xFFFFFFFFUL
/* PDM.RX_FIFO_RD_SILENT */
#define PDM_RX_FIFO_RD_SILENT_DATA_Pos          0UL
#define PDM_RX_FIFO_RD_SILENT_DATA_Msk          0xFFFFFFFFUL
/* PDM.INTR */
#define PDM_INTR_RX_TRIGGER_Pos                 16UL
#define PDM_INTR_RX_TRIGGER_Msk                 0x10000UL
#define PDM_INTR_RX_NOT_EMPTY_Pos               18UL
#define PDM_INTR_RX_NOT_EMPTY_Msk               0x40000UL
#define PDM_INTR_RX_OVERFLOW_Pos                21UL
#define PDM_INTR_RX_OVERFLOW_Msk                0x200000UL
#define PDM_INTR_RX_UNDERFLOW_Pos               22UL
#define PDM_INTR_RX_UNDERFLOW_Msk               0x400000UL
/* PDM.INTR_SET */
#define PDM_INTR_SET_RX_TRIGGER_Pos             16UL
#define PDM_INTR_SET_RX_TRIGGER_Msk             0x10000UL
#define PDM_INTR_SET_RX_NOT_EMPTY_Pos           18UL
#define PDM_INTR_SET_RX_NOT_EMPTY_Msk           0x40000UL
#define PDM_INTR_SET_RX_OVERFLOW_Pos            21UL
#define PDM_INTR_SET_RX_OVERFLOW_Msk            0x200000UL
#define PDM_INTR_SET_RX_UNDERFLOW_Pos           22UL
#define PDM_INTR_SET_RX_UNDERFLOW_Msk           0x400000UL
/* PDM.INTR_MASK */
#define PDM_INTR_MASK_RX_TRIGGER_Pos            16UL
#define PDM_INTR_MASK_RX_TRIGGER_Msk            0x10000UL
#define PDM_INTR_MASK_RX_NOT_EMPTY_Pos          18UL
#define PDM_INTR_MASK_RX_NOT_EMPTY_Msk          0x40000UL
#define PDM_INTR_MASK_RX_OVERFLOW_Pos           21UL
#define PDM_INTR_MASK_RX_OVERFLOW_Msk           0x200000UL
#define PDM_INTR_MASK_RX_UNDERFLOW_Pos          22UL
#define PDM_INTR_MASK_RX_UNDERFLOW_Msk          0x400000UL
/* PDM.INTR_MASKED */
#define PDM_INTR_MASKED_RX_TRIGGER_Pos          16UL
#define PDM_INTR_MASKED_RX_TRIGGER_Msk          0x10000UL
#define PDM_INTR_MASKED_RX_NOT_EMPTY_Pos        18UL
#define PDM_INTR_MASKED_RX_NOT_EMPTY_Msk        0x40000UL
#define PDM_INTR_MASKED_RX_OVERFLOW_Pos         21UL
#define PDM_INTR_MASKED_RX_OVERFLOW_Msk         0x200000UL
#define PDM_INTR_MASKED_RX_UNDERFLOW_Pos        22UL
#define PDM_INTR_MASKED_RX_UNDERFLOW_Msk        0x400000UL


#endif /* _CYIP_PDM_H_ */


/* [] END OF FILE */
