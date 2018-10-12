/***************************************************************************//**
* \file cyip_pass.h
*
* \brief
* PASS IP definitions
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

#ifndef _CYIP_PASS_H_
#define _CYIP_PASS_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     PASS
*******************************************************************************/

#define PASS_AREF_SECTION_SIZE                  0x00000100UL
#define PASS_SECTION_SIZE                       0x00010000UL

/**
  * \brief AREF configuration (PASS_AREF)
  */
typedef struct {
  __IOM uint32_t AREF_CTRL;                     /*!< 0x00000000 global AREF control */
   __IM uint32_t RESERVED[63];
} PASS_AREF_V1_Type;                            /*!< Size = 256 (0x100) */

/**
  * \brief PASS top-level MMIO (DSABv2, INTR) (PASS)
  */
typedef struct {
   __IM uint32_t INTR_CAUSE;                    /*!< 0x00000000 Interrupt cause register */
   __IM uint32_t RESERVED[895];
        PASS_AREF_V1_Type AREF;                 /*!< 0x00000E00 AREF configuration */
  __IOM uint32_t VREF_TRIM0;                    /*!< 0x00000F00 VREF Trim bits */
  __IOM uint32_t VREF_TRIM1;                    /*!< 0x00000F04 VREF Trim bits */
  __IOM uint32_t VREF_TRIM2;                    /*!< 0x00000F08 VREF Trim bits */
  __IOM uint32_t VREF_TRIM3;                    /*!< 0x00000F0C VREF Trim bits */
  __IOM uint32_t IZTAT_TRIM0;                   /*!< 0x00000F10 IZTAT Trim bits */
  __IOM uint32_t IZTAT_TRIM1;                   /*!< 0x00000F14 IZTAT Trim bits */
  __IOM uint32_t IPTAT_TRIM0;                   /*!< 0x00000F18 IPTAT Trim bits */
  __IOM uint32_t ICTAT_TRIM0;                   /*!< 0x00000F1C ICTAT Trim bits */
} PASS_V1_Type;                                 /*!< Size = 3872 (0xF20) */


/* PASS_AREF.AREF_CTRL */
#define PASS_AREF_AREF_CTRL_AREF_MODE_Pos       0UL
#define PASS_AREF_AREF_CTRL_AREF_MODE_Msk       0x1UL
#define PASS_AREF_AREF_CTRL_AREF_BIAS_SCALE_Pos 2UL
#define PASS_AREF_AREF_CTRL_AREF_BIAS_SCALE_Msk 0xCUL
#define PASS_AREF_AREF_CTRL_AREF_RMB_Pos        4UL
#define PASS_AREF_AREF_CTRL_AREF_RMB_Msk        0x70UL
#define PASS_AREF_AREF_CTRL_CTB_IPTAT_SCALE_Pos 7UL
#define PASS_AREF_AREF_CTRL_CTB_IPTAT_SCALE_Msk 0x80UL
#define PASS_AREF_AREF_CTRL_CTB_IPTAT_REDIRECT_Pos 8UL
#define PASS_AREF_AREF_CTRL_CTB_IPTAT_REDIRECT_Msk 0xFF00UL
#define PASS_AREF_AREF_CTRL_IZTAT_SEL_Pos       16UL
#define PASS_AREF_AREF_CTRL_IZTAT_SEL_Msk       0x10000UL
#define PASS_AREF_AREF_CTRL_CLOCK_PUMP_PERI_SEL_Pos 19UL
#define PASS_AREF_AREF_CTRL_CLOCK_PUMP_PERI_SEL_Msk 0x80000UL
#define PASS_AREF_AREF_CTRL_VREF_SEL_Pos        20UL
#define PASS_AREF_AREF_CTRL_VREF_SEL_Msk        0x300000UL
#define PASS_AREF_AREF_CTRL_DEEPSLEEP_MODE_Pos  28UL
#define PASS_AREF_AREF_CTRL_DEEPSLEEP_MODE_Msk  0x30000000UL
#define PASS_AREF_AREF_CTRL_DEEPSLEEP_ON_Pos    30UL
#define PASS_AREF_AREF_CTRL_DEEPSLEEP_ON_Msk    0x40000000UL
#define PASS_AREF_AREF_CTRL_ENABLED_Pos         31UL
#define PASS_AREF_AREF_CTRL_ENABLED_Msk         0x80000000UL


/* PASS.INTR_CAUSE */
#define PASS_INTR_CAUSE_CTB0_INT_Pos            0UL
#define PASS_INTR_CAUSE_CTB0_INT_Msk            0x1UL
#define PASS_INTR_CAUSE_CTB1_INT_Pos            1UL
#define PASS_INTR_CAUSE_CTB1_INT_Msk            0x2UL
#define PASS_INTR_CAUSE_CTB2_INT_Pos            2UL
#define PASS_INTR_CAUSE_CTB2_INT_Msk            0x4UL
#define PASS_INTR_CAUSE_CTB3_INT_Pos            3UL
#define PASS_INTR_CAUSE_CTB3_INT_Msk            0x8UL
#define PASS_INTR_CAUSE_CTDAC0_INT_Pos          4UL
#define PASS_INTR_CAUSE_CTDAC0_INT_Msk          0x10UL
#define PASS_INTR_CAUSE_CTDAC1_INT_Pos          5UL
#define PASS_INTR_CAUSE_CTDAC1_INT_Msk          0x20UL
#define PASS_INTR_CAUSE_CTDAC2_INT_Pos          6UL
#define PASS_INTR_CAUSE_CTDAC2_INT_Msk          0x40UL
#define PASS_INTR_CAUSE_CTDAC3_INT_Pos          7UL
#define PASS_INTR_CAUSE_CTDAC3_INT_Msk          0x80UL
/* PASS.VREF_TRIM0 */
#define PASS_VREF_TRIM0_VREF_ABS_TRIM_Pos       0UL
#define PASS_VREF_TRIM0_VREF_ABS_TRIM_Msk       0xFFUL
/* PASS.VREF_TRIM1 */
#define PASS_VREF_TRIM1_VREF_TEMPCO_TRIM_Pos    0UL
#define PASS_VREF_TRIM1_VREF_TEMPCO_TRIM_Msk    0xFFUL
/* PASS.VREF_TRIM2 */
#define PASS_VREF_TRIM2_VREF_CURV_TRIM_Pos      0UL
#define PASS_VREF_TRIM2_VREF_CURV_TRIM_Msk      0xFFUL
/* PASS.VREF_TRIM3 */
#define PASS_VREF_TRIM3_VREF_ATTEN_TRIM_Pos     0UL
#define PASS_VREF_TRIM3_VREF_ATTEN_TRIM_Msk     0xFUL
/* PASS.IZTAT_TRIM0 */
#define PASS_IZTAT_TRIM0_IZTAT_ABS_TRIM_Pos     0UL
#define PASS_IZTAT_TRIM0_IZTAT_ABS_TRIM_Msk     0xFFUL
/* PASS.IZTAT_TRIM1 */
#define PASS_IZTAT_TRIM1_IZTAT_TC_TRIM_Pos      0UL
#define PASS_IZTAT_TRIM1_IZTAT_TC_TRIM_Msk      0xFFUL
/* PASS.IPTAT_TRIM0 */
#define PASS_IPTAT_TRIM0_IPTAT_CORE_TRIM_Pos    0UL
#define PASS_IPTAT_TRIM0_IPTAT_CORE_TRIM_Msk    0xFUL
#define PASS_IPTAT_TRIM0_IPTAT_CTBM_TRIM_Pos    4UL
#define PASS_IPTAT_TRIM0_IPTAT_CTBM_TRIM_Msk    0xF0UL
/* PASS.ICTAT_TRIM0 */
#define PASS_ICTAT_TRIM0_ICTAT_TRIM_Pos         0UL
#define PASS_ICTAT_TRIM0_ICTAT_TRIM_Msk         0xFUL


#endif /* _CYIP_PASS_H_ */


/* [] END OF FILE */
