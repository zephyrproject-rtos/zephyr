/***************************************************************************//**
* \file cyip_profile.h
*
* \brief
* PROFILE IP definitions
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

#ifndef _CYIP_PROFILE_H_
#define _CYIP_PROFILE_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                   PROFILE
*******************************************************************************/

#define PROFILE_CNT_STRUCT_SECTION_SIZE         0x00000010UL
#define PROFILE_SECTION_SIZE                    0x00010000UL

/**
  * \brief Profile counter structure (PROFILE_CNT_STRUCT)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Profile counter configuration */
   __IM uint32_t RESERVED;
  __IOM uint32_t CNT;                           /*!< 0x00000008 Profile counter value */
   __IM uint32_t RESERVED1;
} PROFILE_CNT_STRUCT_V1_Type;                   /*!< Size = 16 (0x10) */

/**
  * \brief Energy Profiler IP (PROFILE)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Profile control */
   __IM uint32_t STATUS;                        /*!< 0x00000004 Profile status */
   __IM uint32_t RESERVED[2];
  __IOM uint32_t CMD;                           /*!< 0x00000010 Profile command */
   __IM uint32_t RESERVED1[491];
  __IOM uint32_t INTR;                          /*!< 0x000007C0 Profile interrupt */
  __IOM uint32_t INTR_SET;                      /*!< 0x000007C4 Profile interrupt set */
  __IOM uint32_t INTR_MASK;                     /*!< 0x000007C8 Profile interrupt mask */
   __IM uint32_t INTR_MASKED;                   /*!< 0x000007CC Profile interrupt masked */
   __IM uint32_t RESERVED2[12];
        PROFILE_CNT_STRUCT_V1_Type CNT_STRUCT[16]; /*!< 0x00000800 Profile counter structure */
} PROFILE_V1_Type;                              /*!< Size = 2304 (0x900) */


/* PROFILE_CNT_STRUCT.CTL */
#define PROFILE_CNT_STRUCT_CTL_CNT_DURATION_Pos 0UL
#define PROFILE_CNT_STRUCT_CTL_CNT_DURATION_Msk 0x1UL
#define PROFILE_CNT_STRUCT_CTL_REF_CLK_SEL_Pos  4UL
#define PROFILE_CNT_STRUCT_CTL_REF_CLK_SEL_Msk  0x70UL
#define PROFILE_CNT_STRUCT_CTL_MON_SEL_Pos      16UL
#define PROFILE_CNT_STRUCT_CTL_MON_SEL_Msk      0x7F0000UL
#define PROFILE_CNT_STRUCT_CTL_ENABLED_Pos      31UL
#define PROFILE_CNT_STRUCT_CTL_ENABLED_Msk      0x80000000UL
/* PROFILE_CNT_STRUCT.CNT */
#define PROFILE_CNT_STRUCT_CNT_CNT_Pos          0UL
#define PROFILE_CNT_STRUCT_CNT_CNT_Msk          0xFFFFFFFFUL


/* PROFILE.CTL */
#define PROFILE_CTL_WIN_MODE_Pos                0UL
#define PROFILE_CTL_WIN_MODE_Msk                0x1UL
#define PROFILE_CTL_ENABLED_Pos                 31UL
#define PROFILE_CTL_ENABLED_Msk                 0x80000000UL
/* PROFILE.STATUS */
#define PROFILE_STATUS_WIN_ACTIVE_Pos           0UL
#define PROFILE_STATUS_WIN_ACTIVE_Msk           0x1UL
/* PROFILE.CMD */
#define PROFILE_CMD_START_TR_Pos                0UL
#define PROFILE_CMD_START_TR_Msk                0x1UL
#define PROFILE_CMD_STOP_TR_Pos                 1UL
#define PROFILE_CMD_STOP_TR_Msk                 0x2UL
#define PROFILE_CMD_CLR_ALL_CNT_Pos             8UL
#define PROFILE_CMD_CLR_ALL_CNT_Msk             0x100UL
/* PROFILE.INTR */
#define PROFILE_INTR_CNT_OVFLW_Pos              0UL
#define PROFILE_INTR_CNT_OVFLW_Msk              0xFFFFFFFFUL
/* PROFILE.INTR_SET */
#define PROFILE_INTR_SET_CNT_OVFLW_Pos          0UL
#define PROFILE_INTR_SET_CNT_OVFLW_Msk          0xFFFFFFFFUL
/* PROFILE.INTR_MASK */
#define PROFILE_INTR_MASK_CNT_OVFLW_Pos         0UL
#define PROFILE_INTR_MASK_CNT_OVFLW_Msk         0xFFFFFFFFUL
/* PROFILE.INTR_MASKED */
#define PROFILE_INTR_MASKED_CNT_OVFLW_Pos       0UL
#define PROFILE_INTR_MASKED_CNT_OVFLW_Msk       0xFFFFFFFFUL


#endif /* _CYIP_PROFILE_H_ */


/* [] END OF FILE */
