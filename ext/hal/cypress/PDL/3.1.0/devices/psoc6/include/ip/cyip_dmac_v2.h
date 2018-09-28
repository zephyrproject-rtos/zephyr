/***************************************************************************//**
* \file cyip_dmac_v2.h
*
* \brief
* DMAC IP definitions
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

#ifndef _CYIP_DMAC_V2_H_
#define _CYIP_DMAC_V2_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     DMAC
*******************************************************************************/

#define DMAC_CH_V2_SECTION_SIZE                 0x00000100UL
#define DMAC_V2_SECTION_SIZE                    0x00010000UL

/**
  * \brief DMA controller channel (DMAC_CH)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Channel control */
   __IM uint32_t RESERVED[3];
   __IM uint32_t IDX;                           /*!< 0x00000010 Channel current indices */
   __IM uint32_t SRC;                           /*!< 0x00000014 Channel current source address */
   __IM uint32_t DST;                           /*!< 0x00000018 Channel current destination address */
   __IM uint32_t RESERVED1;
  __IOM uint32_t CURR;                          /*!< 0x00000020 Channel current descriptor pointer */
   __IM uint32_t RESERVED2[7];
   __IM uint32_t DESCR_STATUS;                  /*!< 0x00000040 Channel descriptor status */
   __IM uint32_t RESERVED3[7];
   __IM uint32_t DESCR_CTL;                     /*!< 0x00000060 Channel descriptor control */
   __IM uint32_t DESCR_SRC;                     /*!< 0x00000064 Channel descriptor source */
   __IM uint32_t DESCR_DST;                     /*!< 0x00000068 Channel descriptor destination */
   __IM uint32_t DESCR_X_SIZE;                  /*!< 0x0000006C Channel descriptor X size */
   __IM uint32_t DESCR_X_INCR;                  /*!< 0x00000070 Channel descriptor X increment */
   __IM uint32_t DESCR_Y_SIZE;                  /*!< 0x00000074 Channel descriptor Y size */
   __IM uint32_t DESCR_Y_INCR;                  /*!< 0x00000078 Channel descriptor Y increment */
   __IM uint32_t DESCR_NEXT;                    /*!< 0x0000007C Channel descriptor next pointer */
  __IOM uint32_t INTR;                          /*!< 0x00000080 Interrupt */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000084 Interrupt set */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000088 Interrupt mask */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000008C Interrupt masked */
   __IM uint32_t RESERVED4[28];
} DMAC_CH_V2_Type;                              /*!< Size = 256 (0x100) */

/**
  * \brief DMAC (DMAC)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control */
   __IM uint32_t RESERVED;
   __IM uint32_t ACTIVE;                        /*!< 0x00000008 Active channels */
   __IM uint32_t RESERVED1[1021];
        DMAC_CH_V2_Type CH[8];                  /*!< 0x00001000 DMA controller channel */
} DMAC_V2_Type;                                 /*!< Size = 6144 (0x1800) */


/* DMAC_CH.CTL */
#define DMAC_CH_V2_CTL_P_Pos                    0UL
#define DMAC_CH_V2_CTL_P_Msk                    0x1UL
#define DMAC_CH_V2_CTL_NS_Pos                   1UL
#define DMAC_CH_V2_CTL_NS_Msk                   0x2UL
#define DMAC_CH_V2_CTL_B_Pos                    2UL
#define DMAC_CH_V2_CTL_B_Msk                    0x4UL
#define DMAC_CH_V2_CTL_PC_Pos                   4UL
#define DMAC_CH_V2_CTL_PC_Msk                   0xF0UL
#define DMAC_CH_V2_CTL_PRIO_Pos                 8UL
#define DMAC_CH_V2_CTL_PRIO_Msk                 0x300UL
#define DMAC_CH_V2_CTL_ENABLED_Pos              31UL
#define DMAC_CH_V2_CTL_ENABLED_Msk              0x80000000UL
/* DMAC_CH.IDX */
#define DMAC_CH_V2_IDX_X_Pos                    0UL
#define DMAC_CH_V2_IDX_X_Msk                    0xFFFFUL
#define DMAC_CH_V2_IDX_Y_Pos                    16UL
#define DMAC_CH_V2_IDX_Y_Msk                    0xFFFF0000UL
/* DMAC_CH.SRC */
#define DMAC_CH_V2_SRC_ADDR_Pos                 0UL
#define DMAC_CH_V2_SRC_ADDR_Msk                 0xFFFFFFFFUL
/* DMAC_CH.DST */
#define DMAC_CH_V2_DST_ADDR_Pos                 0UL
#define DMAC_CH_V2_DST_ADDR_Msk                 0xFFFFFFFFUL
/* DMAC_CH.CURR */
#define DMAC_CH_V2_CURR_PTR_Pos                 2UL
#define DMAC_CH_V2_CURR_PTR_Msk                 0xFFFFFFFCUL
/* DMAC_CH.DESCR_STATUS */
#define DMAC_CH_V2_DESCR_STATUS_VALID_Pos       31UL
#define DMAC_CH_V2_DESCR_STATUS_VALID_Msk       0x80000000UL
/* DMAC_CH.DESCR_CTL */
#define DMAC_CH_V2_DESCR_CTL_WAIT_FOR_DEACT_Pos 0UL
#define DMAC_CH_V2_DESCR_CTL_WAIT_FOR_DEACT_Msk 0x3UL
#define DMAC_CH_V2_DESCR_CTL_INTR_TYPE_Pos      2UL
#define DMAC_CH_V2_DESCR_CTL_INTR_TYPE_Msk      0xCUL
#define DMAC_CH_V2_DESCR_CTL_TR_OUT_TYPE_Pos    4UL
#define DMAC_CH_V2_DESCR_CTL_TR_OUT_TYPE_Msk    0x30UL
#define DMAC_CH_V2_DESCR_CTL_TR_IN_TYPE_Pos     6UL
#define DMAC_CH_V2_DESCR_CTL_TR_IN_TYPE_Msk     0xC0UL
#define DMAC_CH_V2_DESCR_CTL_DATA_PREFETCH_Pos  8UL
#define DMAC_CH_V2_DESCR_CTL_DATA_PREFETCH_Msk  0x100UL
#define DMAC_CH_V2_DESCR_CTL_DATA_SIZE_Pos      16UL
#define DMAC_CH_V2_DESCR_CTL_DATA_SIZE_Msk      0x30000UL
#define DMAC_CH_V2_DESCR_CTL_CH_DISABLE_Pos     24UL
#define DMAC_CH_V2_DESCR_CTL_CH_DISABLE_Msk     0x1000000UL
#define DMAC_CH_V2_DESCR_CTL_SRC_TRANSFER_SIZE_Pos 26UL
#define DMAC_CH_V2_DESCR_CTL_SRC_TRANSFER_SIZE_Msk 0x4000000UL
#define DMAC_CH_V2_DESCR_CTL_DST_TRANSFER_SIZE_Pos 27UL
#define DMAC_CH_V2_DESCR_CTL_DST_TRANSFER_SIZE_Msk 0x8000000UL
#define DMAC_CH_V2_DESCR_CTL_DESCR_TYPE_Pos     28UL
#define DMAC_CH_V2_DESCR_CTL_DESCR_TYPE_Msk     0x70000000UL
/* DMAC_CH.DESCR_SRC */
#define DMAC_CH_V2_DESCR_SRC_ADDR_Pos           0UL
#define DMAC_CH_V2_DESCR_SRC_ADDR_Msk           0xFFFFFFFFUL
/* DMAC_CH.DESCR_DST */
#define DMAC_CH_V2_DESCR_DST_ADDR_Pos           0UL
#define DMAC_CH_V2_DESCR_DST_ADDR_Msk           0xFFFFFFFFUL
/* DMAC_CH.DESCR_X_SIZE */
#define DMAC_CH_V2_DESCR_X_SIZE_X_COUNT_Pos     0UL
#define DMAC_CH_V2_DESCR_X_SIZE_X_COUNT_Msk     0xFFFFUL
/* DMAC_CH.DESCR_X_INCR */
#define DMAC_CH_V2_DESCR_X_INCR_SRC_X_Pos       0UL
#define DMAC_CH_V2_DESCR_X_INCR_SRC_X_Msk       0xFFFFUL
#define DMAC_CH_V2_DESCR_X_INCR_DST_X_Pos       16UL
#define DMAC_CH_V2_DESCR_X_INCR_DST_X_Msk       0xFFFF0000UL
/* DMAC_CH.DESCR_Y_SIZE */
#define DMAC_CH_V2_DESCR_Y_SIZE_Y_COUNT_Pos     0UL
#define DMAC_CH_V2_DESCR_Y_SIZE_Y_COUNT_Msk     0xFFFFUL
/* DMAC_CH.DESCR_Y_INCR */
#define DMAC_CH_V2_DESCR_Y_INCR_SRC_Y_Pos       0UL
#define DMAC_CH_V2_DESCR_Y_INCR_SRC_Y_Msk       0xFFFFUL
#define DMAC_CH_V2_DESCR_Y_INCR_DST_Y_Pos       16UL
#define DMAC_CH_V2_DESCR_Y_INCR_DST_Y_Msk       0xFFFF0000UL
/* DMAC_CH.DESCR_NEXT */
#define DMAC_CH_V2_DESCR_NEXT_PTR_Pos           2UL
#define DMAC_CH_V2_DESCR_NEXT_PTR_Msk           0xFFFFFFFCUL
/* DMAC_CH.INTR */
#define DMAC_CH_V2_INTR_COMPLETION_Pos          0UL
#define DMAC_CH_V2_INTR_COMPLETION_Msk          0x1UL
#define DMAC_CH_V2_INTR_SRC_BUS_ERROR_Pos       1UL
#define DMAC_CH_V2_INTR_SRC_BUS_ERROR_Msk       0x2UL
#define DMAC_CH_V2_INTR_DST_BUS_ERROR_Pos       2UL
#define DMAC_CH_V2_INTR_DST_BUS_ERROR_Msk       0x4UL
#define DMAC_CH_V2_INTR_SRC_MISAL_Pos           3UL
#define DMAC_CH_V2_INTR_SRC_MISAL_Msk           0x8UL
#define DMAC_CH_V2_INTR_DST_MISAL_Pos           4UL
#define DMAC_CH_V2_INTR_DST_MISAL_Msk           0x10UL
#define DMAC_CH_V2_INTR_CURR_PTR_NULL_Pos       5UL
#define DMAC_CH_V2_INTR_CURR_PTR_NULL_Msk       0x20UL
#define DMAC_CH_V2_INTR_ACTIVE_CH_DISABLED_Pos  6UL
#define DMAC_CH_V2_INTR_ACTIVE_CH_DISABLED_Msk  0x40UL
#define DMAC_CH_V2_INTR_DESCR_BUS_ERROR_Pos     7UL
#define DMAC_CH_V2_INTR_DESCR_BUS_ERROR_Msk     0x80UL
/* DMAC_CH.INTR_SET */
#define DMAC_CH_V2_INTR_SET_COMPLETION_Pos      0UL
#define DMAC_CH_V2_INTR_SET_COMPLETION_Msk      0x1UL
#define DMAC_CH_V2_INTR_SET_SRC_BUS_ERROR_Pos   1UL
#define DMAC_CH_V2_INTR_SET_SRC_BUS_ERROR_Msk   0x2UL
#define DMAC_CH_V2_INTR_SET_DST_BUS_ERROR_Pos   2UL
#define DMAC_CH_V2_INTR_SET_DST_BUS_ERROR_Msk   0x4UL
#define DMAC_CH_V2_INTR_SET_SRC_MISAL_Pos       3UL
#define DMAC_CH_V2_INTR_SET_SRC_MISAL_Msk       0x8UL
#define DMAC_CH_V2_INTR_SET_DST_MISAL_Pos       4UL
#define DMAC_CH_V2_INTR_SET_DST_MISAL_Msk       0x10UL
#define DMAC_CH_V2_INTR_SET_CURR_PTR_NULL_Pos   5UL
#define DMAC_CH_V2_INTR_SET_CURR_PTR_NULL_Msk   0x20UL
#define DMAC_CH_V2_INTR_SET_ACTIVE_CH_DISABLED_Pos 6UL
#define DMAC_CH_V2_INTR_SET_ACTIVE_CH_DISABLED_Msk 0x40UL
#define DMAC_CH_V2_INTR_SET_DESCR_BUS_ERROR_Pos 7UL
#define DMAC_CH_V2_INTR_SET_DESCR_BUS_ERROR_Msk 0x80UL
/* DMAC_CH.INTR_MASK */
#define DMAC_CH_V2_INTR_MASK_COMPLETION_Pos     0UL
#define DMAC_CH_V2_INTR_MASK_COMPLETION_Msk     0x1UL
#define DMAC_CH_V2_INTR_MASK_SRC_BUS_ERROR_Pos  1UL
#define DMAC_CH_V2_INTR_MASK_SRC_BUS_ERROR_Msk  0x2UL
#define DMAC_CH_V2_INTR_MASK_DST_BUS_ERROR_Pos  2UL
#define DMAC_CH_V2_INTR_MASK_DST_BUS_ERROR_Msk  0x4UL
#define DMAC_CH_V2_INTR_MASK_SRC_MISAL_Pos      3UL
#define DMAC_CH_V2_INTR_MASK_SRC_MISAL_Msk      0x8UL
#define DMAC_CH_V2_INTR_MASK_DST_MISAL_Pos      4UL
#define DMAC_CH_V2_INTR_MASK_DST_MISAL_Msk      0x10UL
#define DMAC_CH_V2_INTR_MASK_CURR_PTR_NULL_Pos  5UL
#define DMAC_CH_V2_INTR_MASK_CURR_PTR_NULL_Msk  0x20UL
#define DMAC_CH_V2_INTR_MASK_ACTIVE_CH_DISABLED_Pos 6UL
#define DMAC_CH_V2_INTR_MASK_ACTIVE_CH_DISABLED_Msk 0x40UL
#define DMAC_CH_V2_INTR_MASK_DESCR_BUS_ERROR_Pos 7UL
#define DMAC_CH_V2_INTR_MASK_DESCR_BUS_ERROR_Msk 0x80UL
/* DMAC_CH.INTR_MASKED */
#define DMAC_CH_V2_INTR_MASKED_COMPLETION_Pos   0UL
#define DMAC_CH_V2_INTR_MASKED_COMPLETION_Msk   0x1UL
#define DMAC_CH_V2_INTR_MASKED_SRC_BUS_ERROR_Pos 1UL
#define DMAC_CH_V2_INTR_MASKED_SRC_BUS_ERROR_Msk 0x2UL
#define DMAC_CH_V2_INTR_MASKED_DST_BUS_ERROR_Pos 2UL
#define DMAC_CH_V2_INTR_MASKED_DST_BUS_ERROR_Msk 0x4UL
#define DMAC_CH_V2_INTR_MASKED_SRC_MISAL_Pos    3UL
#define DMAC_CH_V2_INTR_MASKED_SRC_MISAL_Msk    0x8UL
#define DMAC_CH_V2_INTR_MASKED_DST_MISAL_Pos    4UL
#define DMAC_CH_V2_INTR_MASKED_DST_MISAL_Msk    0x10UL
#define DMAC_CH_V2_INTR_MASKED_CURR_PTR_NULL_Pos 5UL
#define DMAC_CH_V2_INTR_MASKED_CURR_PTR_NULL_Msk 0x20UL
#define DMAC_CH_V2_INTR_MASKED_ACTIVE_CH_DISABLED_Pos 6UL
#define DMAC_CH_V2_INTR_MASKED_ACTIVE_CH_DISABLED_Msk 0x40UL
#define DMAC_CH_V2_INTR_MASKED_DESCR_BUS_ERROR_Pos 7UL
#define DMAC_CH_V2_INTR_MASKED_DESCR_BUS_ERROR_Msk 0x80UL


/* DMAC.CTL */
#define DMAC_V2_CTL_ENABLED_Pos                 31UL
#define DMAC_V2_CTL_ENABLED_Msk                 0x80000000UL
/* DMAC.ACTIVE */
#define DMAC_V2_ACTIVE_ACTIVE_Pos               0UL
#define DMAC_V2_ACTIVE_ACTIVE_Msk               0xFFUL


#endif /* _CYIP_DMAC_V2_H_ */


/* [] END OF FILE */
