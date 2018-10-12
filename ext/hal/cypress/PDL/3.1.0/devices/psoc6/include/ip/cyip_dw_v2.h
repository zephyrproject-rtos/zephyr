/***************************************************************************//**
* \file cyip_dw_v2.h
*
* \brief
* DW IP definitions
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

#ifndef _CYIP_DW_V2_H_
#define _CYIP_DW_V2_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                      DW
*******************************************************************************/

#define DW_CH_STRUCT_V2_SECTION_SIZE            0x00000040UL
#define DW_V2_SECTION_SIZE                      0x00010000UL

/**
  * \brief DW channel structure (DW_CH_STRUCT)
  */
typedef struct {
  __IOM uint32_t CH_CTL;                        /*!< 0x00000000 Channel control */
   __IM uint32_t CH_STATUS;                     /*!< 0x00000004 Channel status */
  __IOM uint32_t CH_IDX;                        /*!< 0x00000008 Channel current indices */
  __IOM uint32_t CH_CURR_PTR;                   /*!< 0x0000000C Channel current descriptor pointer */
  __IOM uint32_t INTR;                          /*!< 0x00000010 Interrupt */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000014 Interrupt set */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000018 Interrupt mask */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000001C Interrupt masked */
  __IOM uint32_t SRAM_DATA0;                    /*!< 0x00000020 SRAM data 0 */
  __IOM uint32_t SRAM_DATA1;                    /*!< 0x00000024 SRAM data 1 */
   __IM uint32_t RESERVED[6];
} DW_CH_STRUCT_V2_Type;                         /*!< Size = 64 (0x40) */

/**
  * \brief Datawire Controller (DW)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t STATUS;                        /*!< 0x00000004 Status */
   __IM uint32_t RESERVED[6];
   __IM uint32_t ACT_DESCR_CTL;                 /*!< 0x00000020 Active descriptor control */
   __IM uint32_t ACT_DESCR_SRC;                 /*!< 0x00000024 Active descriptor source */
   __IM uint32_t ACT_DESCR_DST;                 /*!< 0x00000028 Active descriptor destination */
   __IM uint32_t RESERVED1;
   __IM uint32_t ACT_DESCR_X_CTL;               /*!< 0x00000030 Active descriptor X loop control */
   __IM uint32_t ACT_DESCR_Y_CTL;               /*!< 0x00000034 Active descriptor Y loop control */
   __IM uint32_t ACT_DESCR_NEXT_PTR;            /*!< 0x00000038 Active descriptor next pointer */
   __IM uint32_t RESERVED2;
   __IM uint32_t ACT_SRC;                       /*!< 0x00000040 Active source */
   __IM uint32_t ACT_DST;                       /*!< 0x00000044 Active destination */
   __IM uint32_t RESERVED3[14];
  __IOM uint32_t ECC_CTL;                       /*!< 0x00000080 ECC control */
   __IM uint32_t RESERVED4[31];
  __IOM uint32_t CRC_CTL;                       /*!< 0x00000100 CRC control */
   __IM uint32_t RESERVED5[3];
  __IOM uint32_t CRC_DATA_CTL;                  /*!< 0x00000110 CRC data control */
   __IM uint32_t RESERVED6[3];
  __IOM uint32_t CRC_POL_CTL;                   /*!< 0x00000120 CRC polynomial control */
   __IM uint32_t RESERVED7[3];
  __IOM uint32_t CRC_LFSR_CTL;                  /*!< 0x00000130 CRC LFSR control */
   __IM uint32_t RESERVED8[3];
  __IOM uint32_t CRC_REM_CTL;                   /*!< 0x00000140 CRC remainder control */
   __IM uint32_t RESERVED9;
   __IM uint32_t CRC_REM_RESULT;                /*!< 0x00000148 CRC remainder result */
   __IM uint32_t RESERVED10[8109];
        DW_CH_STRUCT_V2_Type CH_STRUCT[512];    /*!< 0x00008000 DW channel structure */
} DW_V2_Type;                                   /*!< Size = 65536 (0x10000) */


/* DW_CH_STRUCT.CH_CTL */
#define DW_CH_STRUCT_V2_CH_CTL_P_Pos            0UL
#define DW_CH_STRUCT_V2_CH_CTL_P_Msk            0x1UL
#define DW_CH_STRUCT_V2_CH_CTL_NS_Pos           1UL
#define DW_CH_STRUCT_V2_CH_CTL_NS_Msk           0x2UL
#define DW_CH_STRUCT_V2_CH_CTL_B_Pos            2UL
#define DW_CH_STRUCT_V2_CH_CTL_B_Msk            0x4UL
#define DW_CH_STRUCT_V2_CH_CTL_PC_Pos           4UL
#define DW_CH_STRUCT_V2_CH_CTL_PC_Msk           0xF0UL
#define DW_CH_STRUCT_V2_CH_CTL_PRIO_Pos         8UL
#define DW_CH_STRUCT_V2_CH_CTL_PRIO_Msk         0x300UL
#define DW_CH_STRUCT_V2_CH_CTL_PREEMPTABLE_Pos  11UL
#define DW_CH_STRUCT_V2_CH_CTL_PREEMPTABLE_Msk  0x800UL
#define DW_CH_STRUCT_V2_CH_CTL_ENABLED_Pos      31UL
#define DW_CH_STRUCT_V2_CH_CTL_ENABLED_Msk      0x80000000UL
/* DW_CH_STRUCT.CH_STATUS */
#define DW_CH_STRUCT_V2_CH_STATUS_INTR_CAUSE_Pos 0UL
#define DW_CH_STRUCT_V2_CH_STATUS_INTR_CAUSE_Msk 0xFUL
#define DW_CH_STRUCT_V2_CH_STATUS_PENDING_Pos   31UL
#define DW_CH_STRUCT_V2_CH_STATUS_PENDING_Msk   0x80000000UL
/* DW_CH_STRUCT.CH_IDX */
#define DW_CH_STRUCT_V2_CH_IDX_X_IDX_Pos        0UL
#define DW_CH_STRUCT_V2_CH_IDX_X_IDX_Msk        0xFFUL
#define DW_CH_STRUCT_V2_CH_IDX_Y_IDX_Pos        8UL
#define DW_CH_STRUCT_V2_CH_IDX_Y_IDX_Msk        0xFF00UL
/* DW_CH_STRUCT.CH_CURR_PTR */
#define DW_CH_STRUCT_V2_CH_CURR_PTR_ADDR_Pos    2UL
#define DW_CH_STRUCT_V2_CH_CURR_PTR_ADDR_Msk    0xFFFFFFFCUL
/* DW_CH_STRUCT.INTR */
#define DW_CH_STRUCT_V2_INTR_CH_Pos             0UL
#define DW_CH_STRUCT_V2_INTR_CH_Msk             0x1UL
/* DW_CH_STRUCT.INTR_SET */
#define DW_CH_STRUCT_V2_INTR_SET_CH_Pos         0UL
#define DW_CH_STRUCT_V2_INTR_SET_CH_Msk         0x1UL
/* DW_CH_STRUCT.INTR_MASK */
#define DW_CH_STRUCT_V2_INTR_MASK_CH_Pos        0UL
#define DW_CH_STRUCT_V2_INTR_MASK_CH_Msk        0x1UL
/* DW_CH_STRUCT.INTR_MASKED */
#define DW_CH_STRUCT_V2_INTR_MASKED_CH_Pos      0UL
#define DW_CH_STRUCT_V2_INTR_MASKED_CH_Msk      0x1UL
/* DW_CH_STRUCT.SRAM_DATA0 */
#define DW_CH_STRUCT_V2_SRAM_DATA0_DATA_Pos     0UL
#define DW_CH_STRUCT_V2_SRAM_DATA0_DATA_Msk     0xFFFFFFFFUL
/* DW_CH_STRUCT.SRAM_DATA1 */
#define DW_CH_STRUCT_V2_SRAM_DATA1_DATA_Pos     0UL
#define DW_CH_STRUCT_V2_SRAM_DATA1_DATA_Msk     0xFFFFFFFFUL


/* DW.CTL */
#define DW_V2_CTL_ECC_EN_Pos                    0UL
#define DW_V2_CTL_ECC_EN_Msk                    0x1UL
#define DW_V2_CTL_ECC_INJ_EN_Pos                1UL
#define DW_V2_CTL_ECC_INJ_EN_Msk                0x2UL
#define DW_V2_CTL_ENABLED_Pos                   31UL
#define DW_V2_CTL_ENABLED_Msk                   0x80000000UL
/* DW.STATUS */
#define DW_V2_STATUS_P_Pos                      0UL
#define DW_V2_STATUS_P_Msk                      0x1UL
#define DW_V2_STATUS_NS_Pos                     1UL
#define DW_V2_STATUS_NS_Msk                     0x2UL
#define DW_V2_STATUS_B_Pos                      2UL
#define DW_V2_STATUS_B_Msk                      0x4UL
#define DW_V2_STATUS_PC_Pos                     4UL
#define DW_V2_STATUS_PC_Msk                     0xF0UL
#define DW_V2_STATUS_PRIO_Pos                   8UL
#define DW_V2_STATUS_PRIO_Msk                   0x300UL
#define DW_V2_STATUS_PREEMPTABLE_Pos            11UL
#define DW_V2_STATUS_PREEMPTABLE_Msk            0x800UL
#define DW_V2_STATUS_CH_IDX_Pos                 16UL
#define DW_V2_STATUS_CH_IDX_Msk                 0x1FF0000UL
#define DW_V2_STATUS_STATE_Pos                  28UL
#define DW_V2_STATUS_STATE_Msk                  0x70000000UL
#define DW_V2_STATUS_ACTIVE_Pos                 31UL
#define DW_V2_STATUS_ACTIVE_Msk                 0x80000000UL
/* DW.ACT_DESCR_CTL */
#define DW_V2_ACT_DESCR_CTL_DATA_Pos            0UL
#define DW_V2_ACT_DESCR_CTL_DATA_Msk            0xFFFFFFFFUL
/* DW.ACT_DESCR_SRC */
#define DW_V2_ACT_DESCR_SRC_DATA_Pos            0UL
#define DW_V2_ACT_DESCR_SRC_DATA_Msk            0xFFFFFFFFUL
/* DW.ACT_DESCR_DST */
#define DW_V2_ACT_DESCR_DST_DATA_Pos            0UL
#define DW_V2_ACT_DESCR_DST_DATA_Msk            0xFFFFFFFFUL
/* DW.ACT_DESCR_X_CTL */
#define DW_V2_ACT_DESCR_X_CTL_DATA_Pos          0UL
#define DW_V2_ACT_DESCR_X_CTL_DATA_Msk          0xFFFFFFFFUL
/* DW.ACT_DESCR_Y_CTL */
#define DW_V2_ACT_DESCR_Y_CTL_DATA_Pos          0UL
#define DW_V2_ACT_DESCR_Y_CTL_DATA_Msk          0xFFFFFFFFUL
/* DW.ACT_DESCR_NEXT_PTR */
#define DW_V2_ACT_DESCR_NEXT_PTR_ADDR_Pos       2UL
#define DW_V2_ACT_DESCR_NEXT_PTR_ADDR_Msk       0xFFFFFFFCUL
/* DW.ACT_SRC */
#define DW_V2_ACT_SRC_SRC_ADDR_Pos              0UL
#define DW_V2_ACT_SRC_SRC_ADDR_Msk              0xFFFFFFFFUL
/* DW.ACT_DST */
#define DW_V2_ACT_DST_DST_ADDR_Pos              0UL
#define DW_V2_ACT_DST_DST_ADDR_Msk              0xFFFFFFFFUL
/* DW.ECC_CTL */
#define DW_V2_ECC_CTL_WORD_ADDR_Pos             0UL
#define DW_V2_ECC_CTL_WORD_ADDR_Msk             0x3FFUL
#define DW_V2_ECC_CTL_PARITY_Pos                25UL
#define DW_V2_ECC_CTL_PARITY_Msk                0xFE000000UL
/* DW.CRC_CTL */
#define DW_V2_CRC_CTL_DATA_REVERSE_Pos          0UL
#define DW_V2_CRC_CTL_DATA_REVERSE_Msk          0x1UL
#define DW_V2_CRC_CTL_REM_REVERSE_Pos           8UL
#define DW_V2_CRC_CTL_REM_REVERSE_Msk           0x100UL
/* DW.CRC_DATA_CTL */
#define DW_V2_CRC_DATA_CTL_DATA_XOR_Pos         0UL
#define DW_V2_CRC_DATA_CTL_DATA_XOR_Msk         0xFFUL
/* DW.CRC_POL_CTL */
#define DW_V2_CRC_POL_CTL_POLYNOMIAL_Pos        0UL
#define DW_V2_CRC_POL_CTL_POLYNOMIAL_Msk        0xFFFFFFFFUL
/* DW.CRC_LFSR_CTL */
#define DW_V2_CRC_LFSR_CTL_LFSR32_Pos           0UL
#define DW_V2_CRC_LFSR_CTL_LFSR32_Msk           0xFFFFFFFFUL
/* DW.CRC_REM_CTL */
#define DW_V2_CRC_REM_CTL_REM_XOR_Pos           0UL
#define DW_V2_CRC_REM_CTL_REM_XOR_Msk           0xFFFFFFFFUL
/* DW.CRC_REM_RESULT */
#define DW_V2_CRC_REM_RESULT_REM_Pos            0UL
#define DW_V2_CRC_REM_RESULT_REM_Msk            0xFFFFFFFFUL


#endif /* _CYIP_DW_V2_H_ */


/* [] END OF FILE */
