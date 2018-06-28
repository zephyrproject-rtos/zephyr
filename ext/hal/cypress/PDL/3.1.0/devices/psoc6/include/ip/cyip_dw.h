/***************************************************************************//**
* \file cyip_dw.h
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

#ifndef _CYIP_DW_H_
#define _CYIP_DW_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                      DW
*******************************************************************************/

#define DW_CH_STRUCT_SECTION_SIZE               0x00000020UL
#define DW_SECTION_SIZE                         0x00001000UL

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
} DW_CH_STRUCT_V1_Type;                         /*!< Size = 32 (0x20) */

/**
  * \brief Datawire Controller (DW)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t STATUS;                        /*!< 0x00000004 Status */
   __IM uint32_t PENDING;                       /*!< 0x00000008 Pending channels */
   __IM uint32_t RESERVED;
   __IM uint32_t STATUS_INTR;                   /*!< 0x00000010 System interrupt control */
   __IM uint32_t STATUS_INTR_MASKED;            /*!< 0x00000014 Status of interrupts masked */
   __IM uint32_t RESERVED1[2];
   __IM uint32_t ACT_DESCR_CTL;                 /*!< 0x00000020 Active descriptor control */
   __IM uint32_t ACT_DESCR_SRC;                 /*!< 0x00000024 Active descriptor source */
   __IM uint32_t ACT_DESCR_DST;                 /*!< 0x00000028 Active descriptor destination */
   __IM uint32_t RESERVED2;
   __IM uint32_t ACT_DESCR_X_CTL;               /*!< 0x00000030 Active descriptor X loop control */
   __IM uint32_t ACT_DESCR_Y_CTL;               /*!< 0x00000034 Active descriptor Y loop control */
   __IM uint32_t ACT_DESCR_NEXT_PTR;            /*!< 0x00000038 Active descriptor next pointer */
   __IM uint32_t RESERVED3;
   __IM uint32_t ACT_SRC;                       /*!< 0x00000040 Active source */
   __IM uint32_t ACT_DST;                       /*!< 0x00000044 Active destination */
   __IM uint32_t RESERVED4[494];
        DW_CH_STRUCT_V1_Type CH_STRUCT[32];     /*!< 0x00000800 DW channel structure */
} DW_V1_Type;                                   /*!< Size = 3072 (0xC00) */


/* DW_CH_STRUCT.CH_CTL */
#define DW_CH_STRUCT_CH_CTL_P_Pos               0UL
#define DW_CH_STRUCT_CH_CTL_P_Msk               0x1UL
#define DW_CH_STRUCT_CH_CTL_NS_Pos              1UL
#define DW_CH_STRUCT_CH_CTL_NS_Msk              0x2UL
#define DW_CH_STRUCT_CH_CTL_B_Pos               2UL
#define DW_CH_STRUCT_CH_CTL_B_Msk               0x4UL
#define DW_CH_STRUCT_CH_CTL_PC_Pos              4UL
#define DW_CH_STRUCT_CH_CTL_PC_Msk              0xF0UL
#define DW_CH_STRUCT_CH_CTL_PRIO_Pos            16UL
#define DW_CH_STRUCT_CH_CTL_PRIO_Msk            0x30000UL
#define DW_CH_STRUCT_CH_CTL_PREEMPTABLE_Pos     18UL
#define DW_CH_STRUCT_CH_CTL_PREEMPTABLE_Msk     0x40000UL
#define DW_CH_STRUCT_CH_CTL_ENABLED_Pos         31UL
#define DW_CH_STRUCT_CH_CTL_ENABLED_Msk         0x80000000UL
/* DW_CH_STRUCT.CH_STATUS */
#define DW_CH_STRUCT_CH_STATUS_INTR_CAUSE_Pos   0UL
#define DW_CH_STRUCT_CH_STATUS_INTR_CAUSE_Msk   0xFUL
/* DW_CH_STRUCT.CH_IDX */
#define DW_CH_STRUCT_CH_IDX_X_IDX_Pos           0UL
#define DW_CH_STRUCT_CH_IDX_X_IDX_Msk           0xFFUL
#define DW_CH_STRUCT_CH_IDX_Y_IDX_Pos           8UL
#define DW_CH_STRUCT_CH_IDX_Y_IDX_Msk           0xFF00UL
/* DW_CH_STRUCT.CH_CURR_PTR */
#define DW_CH_STRUCT_CH_CURR_PTR_ADDR_Pos       2UL
#define DW_CH_STRUCT_CH_CURR_PTR_ADDR_Msk       0xFFFFFFFCUL
/* DW_CH_STRUCT.INTR */
#define DW_CH_STRUCT_INTR_CH_Pos                0UL
#define DW_CH_STRUCT_INTR_CH_Msk                0x1UL
/* DW_CH_STRUCT.INTR_SET */
#define DW_CH_STRUCT_INTR_SET_CH_Pos            0UL
#define DW_CH_STRUCT_INTR_SET_CH_Msk            0x1UL
/* DW_CH_STRUCT.INTR_MASK */
#define DW_CH_STRUCT_INTR_MASK_CH_Pos           0UL
#define DW_CH_STRUCT_INTR_MASK_CH_Msk           0x1UL
/* DW_CH_STRUCT.INTR_MASKED */
#define DW_CH_STRUCT_INTR_MASKED_CH_Pos         0UL
#define DW_CH_STRUCT_INTR_MASKED_CH_Msk         0x1UL


/* DW.CTL */
#define DW_CTL_ENABLED_Pos                      31UL
#define DW_CTL_ENABLED_Msk                      0x80000000UL
/* DW.STATUS */
#define DW_STATUS_P_Pos                         0UL
#define DW_STATUS_P_Msk                         0x1UL
#define DW_STATUS_NS_Pos                        1UL
#define DW_STATUS_NS_Msk                        0x2UL
#define DW_STATUS_B_Pos                         2UL
#define DW_STATUS_B_Msk                         0x4UL
#define DW_STATUS_PC_Pos                        4UL
#define DW_STATUS_PC_Msk                        0xF0UL
#define DW_STATUS_CH_IDX_Pos                    8UL
#define DW_STATUS_CH_IDX_Msk                    0x1F00UL
#define DW_STATUS_PRIO_Pos                      16UL
#define DW_STATUS_PRIO_Msk                      0x30000UL
#define DW_STATUS_PREEMPTABLE_Pos               18UL
#define DW_STATUS_PREEMPTABLE_Msk               0x40000UL
#define DW_STATUS_STATE_Pos                     20UL
#define DW_STATUS_STATE_Msk                     0x700000UL
#define DW_STATUS_ACTIVE_Pos                    31UL
#define DW_STATUS_ACTIVE_Msk                    0x80000000UL
/* DW.PENDING */
#define DW_PENDING_CH_PENDING_Pos               0UL
#define DW_PENDING_CH_PENDING_Msk               0xFFFFFFFFUL
/* DW.STATUS_INTR */
#define DW_STATUS_INTR_CH_Pos                   0UL
#define DW_STATUS_INTR_CH_Msk                   0xFFFFFFFFUL
/* DW.STATUS_INTR_MASKED */
#define DW_STATUS_INTR_MASKED_CH_Pos            0UL
#define DW_STATUS_INTR_MASKED_CH_Msk            0xFFFFFFFFUL
/* DW.ACT_DESCR_CTL */
#define DW_ACT_DESCR_CTL_DATA_Pos               0UL
#define DW_ACT_DESCR_CTL_DATA_Msk               0xFFFFFFFFUL
/* DW.ACT_DESCR_SRC */
#define DW_ACT_DESCR_SRC_DATA_Pos               0UL
#define DW_ACT_DESCR_SRC_DATA_Msk               0xFFFFFFFFUL
/* DW.ACT_DESCR_DST */
#define DW_ACT_DESCR_DST_DATA_Pos               0UL
#define DW_ACT_DESCR_DST_DATA_Msk               0xFFFFFFFFUL
/* DW.ACT_DESCR_X_CTL */
#define DW_ACT_DESCR_X_CTL_DATA_Pos             0UL
#define DW_ACT_DESCR_X_CTL_DATA_Msk             0xFFFFFFFFUL
/* DW.ACT_DESCR_Y_CTL */
#define DW_ACT_DESCR_Y_CTL_DATA_Pos             0UL
#define DW_ACT_DESCR_Y_CTL_DATA_Msk             0xFFFFFFFFUL
/* DW.ACT_DESCR_NEXT_PTR */
#define DW_ACT_DESCR_NEXT_PTR_ADDR_Pos          2UL
#define DW_ACT_DESCR_NEXT_PTR_ADDR_Msk          0xFFFFFFFCUL
/* DW.ACT_SRC */
#define DW_ACT_SRC_SRC_ADDR_Pos                 0UL
#define DW_ACT_SRC_SRC_ADDR_Msk                 0xFFFFFFFFUL
/* DW.ACT_DST */
#define DW_ACT_DST_DST_ADDR_Pos                 0UL
#define DW_ACT_DST_DST_ADDR_Msk                 0xFFFFFFFFUL


#endif /* _CYIP_DW_H_ */


/* [] END OF FILE */
