/***************************************************************************//**
* \file cyip_smartio.h
*
* \brief
* SMARTIO IP definitions
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

#ifndef _CYIP_SMARTIO_H_
#define _CYIP_SMARTIO_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                   SMARTIO
*******************************************************************************/

#define SMARTIO_PRT_SECTION_SIZE                0x00000100UL
#define SMARTIO_SECTION_SIZE                    0x00010000UL

/**
  * \brief Programmable IO port registers (SMARTIO_PRT)
  */
typedef struct {
  __IOM uint32_t CTL;                           /*!< 0x00000000 Control register */
   __IM uint32_t RESERVED[3];
  __IOM uint32_t SYNC_CTL;                      /*!< 0x00000010 Synchronization control register */
   __IM uint32_t RESERVED1[3];
  __IOM uint32_t LUT_SEL[8];                    /*!< 0x00000020 LUT component input selection */
  __IOM uint32_t LUT_CTL[8];                    /*!< 0x00000040 LUT component control register */
   __IM uint32_t RESERVED2[24];
  __IOM uint32_t DU_SEL;                        /*!< 0x000000C0 Data unit component input selection */
  __IOM uint32_t DU_CTL;                        /*!< 0x000000C4 Data unit component control register */
   __IM uint32_t RESERVED3[10];
  __IOM uint32_t DATA;                          /*!< 0x000000F0 Data register */
   __IM uint32_t RESERVED4[3];
} SMARTIO_PRT_V1_Type;                          /*!< Size = 256 (0x100) */

/**
  * \brief Programmable IO configuration (SMARTIO)
  */
typedef struct {
        SMARTIO_PRT_V1_Type PRT[128];           /*!< 0x00000000 Programmable IO port registers */
} SMARTIO_V1_Type;                              /*!< Size = 32768 (0x8000) */


/* SMARTIO_PRT.CTL */
#define SMARTIO_PRT_CTL_BYPASS_Pos              0UL
#define SMARTIO_PRT_CTL_BYPASS_Msk              0xFFUL
#define SMARTIO_PRT_CTL_CLOCK_SRC_Pos           8UL
#define SMARTIO_PRT_CTL_CLOCK_SRC_Msk           0x1F00UL
#define SMARTIO_PRT_CTL_HLD_OVR_Pos             24UL
#define SMARTIO_PRT_CTL_HLD_OVR_Msk             0x1000000UL
#define SMARTIO_PRT_CTL_PIPELINE_EN_Pos         25UL
#define SMARTIO_PRT_CTL_PIPELINE_EN_Msk         0x2000000UL
#define SMARTIO_PRT_CTL_ENABLED_Pos             31UL
#define SMARTIO_PRT_CTL_ENABLED_Msk             0x80000000UL
/* SMARTIO_PRT.SYNC_CTL */
#define SMARTIO_PRT_SYNC_CTL_IO_SYNC_EN_Pos     0UL
#define SMARTIO_PRT_SYNC_CTL_IO_SYNC_EN_Msk     0xFFUL
#define SMARTIO_PRT_SYNC_CTL_CHIP_SYNC_EN_Pos   8UL
#define SMARTIO_PRT_SYNC_CTL_CHIP_SYNC_EN_Msk   0xFF00UL
/* SMARTIO_PRT.LUT_SEL */
#define SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL_Pos     0UL
#define SMARTIO_PRT_LUT_SEL_LUT_TR0_SEL_Msk     0xFUL
#define SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL_Pos     8UL
#define SMARTIO_PRT_LUT_SEL_LUT_TR1_SEL_Msk     0xF00UL
#define SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL_Pos     16UL
#define SMARTIO_PRT_LUT_SEL_LUT_TR2_SEL_Msk     0xF0000UL
/* SMARTIO_PRT.LUT_CTL */
#define SMARTIO_PRT_LUT_CTL_LUT_Pos             0UL
#define SMARTIO_PRT_LUT_CTL_LUT_Msk             0xFFUL
#define SMARTIO_PRT_LUT_CTL_LUT_OPC_Pos         8UL
#define SMARTIO_PRT_LUT_CTL_LUT_OPC_Msk         0x300UL
/* SMARTIO_PRT.DU_SEL */
#define SMARTIO_PRT_DU_SEL_DU_TR0_SEL_Pos       0UL
#define SMARTIO_PRT_DU_SEL_DU_TR0_SEL_Msk       0xFUL
#define SMARTIO_PRT_DU_SEL_DU_TR1_SEL_Pos       8UL
#define SMARTIO_PRT_DU_SEL_DU_TR1_SEL_Msk       0xF00UL
#define SMARTIO_PRT_DU_SEL_DU_TR2_SEL_Pos       16UL
#define SMARTIO_PRT_DU_SEL_DU_TR2_SEL_Msk       0xF0000UL
#define SMARTIO_PRT_DU_SEL_DU_DATA0_SEL_Pos     24UL
#define SMARTIO_PRT_DU_SEL_DU_DATA0_SEL_Msk     0x3000000UL
#define SMARTIO_PRT_DU_SEL_DU_DATA1_SEL_Pos     28UL
#define SMARTIO_PRT_DU_SEL_DU_DATA1_SEL_Msk     0x30000000UL
/* SMARTIO_PRT.DU_CTL */
#define SMARTIO_PRT_DU_CTL_DU_SIZE_Pos          0UL
#define SMARTIO_PRT_DU_CTL_DU_SIZE_Msk          0x7UL
#define SMARTIO_PRT_DU_CTL_DU_OPC_Pos           8UL
#define SMARTIO_PRT_DU_CTL_DU_OPC_Msk           0xF00UL
/* SMARTIO_PRT.DATA */
#define SMARTIO_PRT_DATA_DATA_Pos               0UL
#define SMARTIO_PRT_DATA_DATA_Msk               0xFFUL


#endif /* _CYIP_SMARTIO_H_ */


/* [] END OF FILE */
