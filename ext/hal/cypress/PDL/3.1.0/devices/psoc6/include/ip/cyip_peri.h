/***************************************************************************//**
* \file cyip_peri.h
*
* \brief
* PERI IP definitions
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

#ifndef _CYIP_PERI_H_
#define _CYIP_PERI_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     PERI
*******************************************************************************/

#define PERI_GR_SECTION_SIZE                    0x00000040UL
#define PERI_TR_GR_SECTION_SIZE                 0x00000200UL
#define PERI_PPU_PR_SECTION_SIZE                0x00000040UL
#define PERI_PPU_GR_SECTION_SIZE                0x00000040UL
#define PERI_GR_PPU_SL_SECTION_SIZE             0x00000040UL
#define PERI_GR_PPU_RG_SECTION_SIZE             0x00000040UL
#define PERI_SECTION_SIZE                       0x00010000UL

/**
  * \brief Peripheral group structure (PERI_GR)
  */
typedef struct {
  __IOM uint32_t CLOCK_CTL;                     /*!< 0x00000000 Clock control */
   __IM uint32_t RESERVED[7];
  __IOM uint32_t SL_CTL;                        /*!< 0x00000020 Slave control */
  __IOM uint32_t TIMEOUT_CTL;                   /*!< 0x00000024 Timeout control */
   __IM uint32_t RESERVED1[6];
} PERI_GR_V1_Type;                              /*!< Size = 64 (0x40) */

/**
  * \brief Trigger group (PERI_TR_GR)
  */
typedef struct {
  __IOM uint32_t TR_OUT_CTL[128];               /*!< 0x00000000 Trigger control register */
} PERI_TR_GR_V1_Type;                           /*!< Size = 512 (0x200) */

/**
  * \brief PPU structure with programmable address (PERI_PPU_PR)
  */
typedef struct {
  __IOM uint32_t ADDR0;                         /*!< 0x00000000 PPU region address 0 (slave structure) */
  __IOM uint32_t ATT0;                          /*!< 0x00000004 PPU region attributes 0 (slave structure) */
   __IM uint32_t RESERVED[6];
   __IM uint32_t ADDR1;                         /*!< 0x00000020 PPU region address 1 (master structure) */
  __IOM uint32_t ATT1;                          /*!< 0x00000024 PPU region attributes 1 (master structure) */
   __IM uint32_t RESERVED1[6];
} PERI_PPU_PR_V1_Type;                          /*!< Size = 64 (0x40) */

/**
  * \brief PPU structure with fixed/constant address for a peripheral group (PERI_PPU_GR)
  */
typedef struct {
   __IM uint32_t ADDR0;                         /*!< 0x00000000 PPU region address 0 (slave structure) */
  __IOM uint32_t ATT0;                          /*!< 0x00000004 PPU region attributes 0 (slave structure) */
   __IM uint32_t RESERVED[6];
   __IM uint32_t ADDR1;                         /*!< 0x00000020 PPU region address 1 (master structure) */
  __IOM uint32_t ATT1;                          /*!< 0x00000024 PPU region attributes 1 (master structure) */
   __IM uint32_t RESERVED1[6];
} PERI_PPU_GR_V1_Type;                          /*!< Size = 64 (0x40) */

/**
  * \brief PPU structure with fixed/constant address for a specific slave (PERI_GR_PPU_SL)
  */
typedef struct {
   __IM uint32_t ADDR0;                         /*!< 0x00000000 PPU region address 0 (slave structure) */
  __IOM uint32_t ATT0;                          /*!< 0x00000004 PPU region attributes 0 (slave structure) */
   __IM uint32_t RESERVED[6];
   __IM uint32_t ADDR1;                         /*!< 0x00000020 PPU region address 1 (master structure) */
  __IOM uint32_t ATT1;                          /*!< 0x00000024 PPU region attributes 1 (master structure) */
   __IM uint32_t RESERVED1[6];
} PERI_GR_PPU_SL_V1_Type;                       /*!< Size = 64 (0x40) */

/**
  * \brief PPU structure with fixed/constant address for a specific region (PERI_GR_PPU_RG)
  */
typedef struct {
   __IM uint32_t ADDR0;                         /*!< 0x00000000 PPU region address 0 (slave structure) */
  __IOM uint32_t ATT0;                          /*!< 0x00000004 PPU region attributes 0 (slave structure) */
   __IM uint32_t RESERVED[6];
   __IM uint32_t ADDR1;                         /*!< 0x00000020 PPU region address 1 (master structure) */
  __IOM uint32_t ATT1;                          /*!< 0x00000024 PPU region attributes 1 (master structure) */
   __IM uint32_t RESERVED1[6];
} PERI_GR_PPU_RG_V1_Type;                       /*!< Size = 64 (0x40) */

/**
  * \brief Peripheral interconnect (PERI)
  */
typedef struct {
        PERI_GR_V1_Type GR[16];                 /*!< 0x00000000 Peripheral group structure */
  __IOM uint32_t DIV_CMD;                       /*!< 0x00000400 Divider command register */
   __IM uint32_t RESERVED[255];
  __IOM uint32_t DIV_8_CTL[64];                 /*!< 0x00000800 Divider control register (for 8.0 divider) */
  __IOM uint32_t DIV_16_CTL[64];                /*!< 0x00000900 Divider control register (for 16.0 divider) */
  __IOM uint32_t DIV_16_5_CTL[64];              /*!< 0x00000A00 Divider control register (for 16.5 divider) */
  __IOM uint32_t DIV_24_5_CTL[63];              /*!< 0x00000B00 Divider control register (for 24.5 divider) */
   __IM uint32_t RESERVED1;
  __IOM uint32_t CLOCK_CTL[128];                /*!< 0x00000C00 Clock control register */
   __IM uint32_t RESERVED2[128];
  __IOM uint32_t TR_CMD;                        /*!< 0x00001000 Trigger command register */
   __IM uint32_t RESERVED3[1023];
        PERI_TR_GR_V1_Type TR_GR[16];           /*!< 0x00002000 Trigger group */
        PERI_PPU_PR_V1_Type PPU_PR[32];         /*!< 0x00004000 PPU structure with programmable address */
   __IM uint32_t RESERVED4[512];
        PERI_PPU_GR_V1_Type PPU_GR[16];         /*!< 0x00005000 PPU structure with fixed/constant address for a peripheral
                                                                group */
} PERI_V1_Type;                                 /*!< Size = 21504 (0x5400) */


/* PERI_GR.CLOCK_CTL */
#define PERI_GR_CLOCK_CTL_INT8_DIV_Pos          8UL
#define PERI_GR_CLOCK_CTL_INT8_DIV_Msk          0xFF00UL
/* PERI_GR.SL_CTL */
#define PERI_GR_SL_CTL_ENABLED_0_Pos            0UL
#define PERI_GR_SL_CTL_ENABLED_0_Msk            0x1UL
#define PERI_GR_SL_CTL_ENABLED_1_Pos            1UL
#define PERI_GR_SL_CTL_ENABLED_1_Msk            0x2UL
#define PERI_GR_SL_CTL_ENABLED_2_Pos            2UL
#define PERI_GR_SL_CTL_ENABLED_2_Msk            0x4UL
#define PERI_GR_SL_CTL_ENABLED_3_Pos            3UL
#define PERI_GR_SL_CTL_ENABLED_3_Msk            0x8UL
#define PERI_GR_SL_CTL_ENABLED_4_Pos            4UL
#define PERI_GR_SL_CTL_ENABLED_4_Msk            0x10UL
#define PERI_GR_SL_CTL_ENABLED_5_Pos            5UL
#define PERI_GR_SL_CTL_ENABLED_5_Msk            0x20UL
#define PERI_GR_SL_CTL_ENABLED_6_Pos            6UL
#define PERI_GR_SL_CTL_ENABLED_6_Msk            0x40UL
#define PERI_GR_SL_CTL_ENABLED_7_Pos            7UL
#define PERI_GR_SL_CTL_ENABLED_7_Msk            0x80UL
#define PERI_GR_SL_CTL_ENABLED_8_Pos            8UL
#define PERI_GR_SL_CTL_ENABLED_8_Msk            0x100UL
#define PERI_GR_SL_CTL_ENABLED_9_Pos            9UL
#define PERI_GR_SL_CTL_ENABLED_9_Msk            0x200UL
#define PERI_GR_SL_CTL_ENABLED_10_Pos           10UL
#define PERI_GR_SL_CTL_ENABLED_10_Msk           0x400UL
#define PERI_GR_SL_CTL_ENABLED_11_Pos           11UL
#define PERI_GR_SL_CTL_ENABLED_11_Msk           0x800UL
#define PERI_GR_SL_CTL_ENABLED_12_Pos           12UL
#define PERI_GR_SL_CTL_ENABLED_12_Msk           0x1000UL
#define PERI_GR_SL_CTL_ENABLED_13_Pos           13UL
#define PERI_GR_SL_CTL_ENABLED_13_Msk           0x2000UL
#define PERI_GR_SL_CTL_ENABLED_14_Pos           14UL
#define PERI_GR_SL_CTL_ENABLED_14_Msk           0x4000UL
#define PERI_GR_SL_CTL_ENABLED_15_Pos           15UL
#define PERI_GR_SL_CTL_ENABLED_15_Msk           0x8000UL
/* PERI_GR.TIMEOUT_CTL */
#define PERI_GR_TIMEOUT_CTL_TIMEOUT_Pos         0UL
#define PERI_GR_TIMEOUT_CTL_TIMEOUT_Msk         0xFFFFUL


/* PERI_TR_GR.TR_OUT_CTL */
#define PERI_TR_GR_TR_OUT_CTL_TR_SEL_Pos        0UL
#define PERI_TR_GR_TR_OUT_CTL_TR_SEL_Msk        0xFFUL
#define PERI_TR_GR_TR_OUT_CTL_TR_INV_Pos        8UL
#define PERI_TR_GR_TR_OUT_CTL_TR_INV_Msk        0x100UL
#define PERI_TR_GR_TR_OUT_CTL_TR_EDGE_Pos       9UL
#define PERI_TR_GR_TR_OUT_CTL_TR_EDGE_Msk       0x200UL


/* PERI_PPU_PR.ADDR0 */
#define PERI_PPU_PR_ADDR0_SUBREGION_DISABLE_Pos 0UL
#define PERI_PPU_PR_ADDR0_SUBREGION_DISABLE_Msk 0xFFUL
#define PERI_PPU_PR_ADDR0_ADDR24_Pos            8UL
#define PERI_PPU_PR_ADDR0_ADDR24_Msk            0xFFFFFF00UL
/* PERI_PPU_PR.ATT0 */
#define PERI_PPU_PR_ATT0_UR_Pos                 0UL
#define PERI_PPU_PR_ATT0_UR_Msk                 0x1UL
#define PERI_PPU_PR_ATT0_UW_Pos                 1UL
#define PERI_PPU_PR_ATT0_UW_Msk                 0x2UL
#define PERI_PPU_PR_ATT0_UX_Pos                 2UL
#define PERI_PPU_PR_ATT0_UX_Msk                 0x4UL
#define PERI_PPU_PR_ATT0_PR_Pos                 3UL
#define PERI_PPU_PR_ATT0_PR_Msk                 0x8UL
#define PERI_PPU_PR_ATT0_PW_Pos                 4UL
#define PERI_PPU_PR_ATT0_PW_Msk                 0x10UL
#define PERI_PPU_PR_ATT0_PX_Pos                 5UL
#define PERI_PPU_PR_ATT0_PX_Msk                 0x20UL
#define PERI_PPU_PR_ATT0_NS_Pos                 6UL
#define PERI_PPU_PR_ATT0_NS_Msk                 0x40UL
#define PERI_PPU_PR_ATT0_PC_MASK_0_Pos          8UL
#define PERI_PPU_PR_ATT0_PC_MASK_0_Msk          0x100UL
#define PERI_PPU_PR_ATT0_PC_MASK_15_TO_1_Pos    9UL
#define PERI_PPU_PR_ATT0_PC_MASK_15_TO_1_Msk    0xFFFE00UL
#define PERI_PPU_PR_ATT0_REGION_SIZE_Pos        24UL
#define PERI_PPU_PR_ATT0_REGION_SIZE_Msk        0x1F000000UL
#define PERI_PPU_PR_ATT0_PC_MATCH_Pos           30UL
#define PERI_PPU_PR_ATT0_PC_MATCH_Msk           0x40000000UL
#define PERI_PPU_PR_ATT0_ENABLED_Pos            31UL
#define PERI_PPU_PR_ATT0_ENABLED_Msk            0x80000000UL
/* PERI_PPU_PR.ADDR1 */
#define PERI_PPU_PR_ADDR1_SUBREGION_DISABLE_Pos 0UL
#define PERI_PPU_PR_ADDR1_SUBREGION_DISABLE_Msk 0xFFUL
#define PERI_PPU_PR_ADDR1_ADDR24_Pos            8UL
#define PERI_PPU_PR_ADDR1_ADDR24_Msk            0xFFFFFF00UL
/* PERI_PPU_PR.ATT1 */
#define PERI_PPU_PR_ATT1_UR_Pos                 0UL
#define PERI_PPU_PR_ATT1_UR_Msk                 0x1UL
#define PERI_PPU_PR_ATT1_UW_Pos                 1UL
#define PERI_PPU_PR_ATT1_UW_Msk                 0x2UL
#define PERI_PPU_PR_ATT1_UX_Pos                 2UL
#define PERI_PPU_PR_ATT1_UX_Msk                 0x4UL
#define PERI_PPU_PR_ATT1_PR_Pos                 3UL
#define PERI_PPU_PR_ATT1_PR_Msk                 0x8UL
#define PERI_PPU_PR_ATT1_PW_Pos                 4UL
#define PERI_PPU_PR_ATT1_PW_Msk                 0x10UL
#define PERI_PPU_PR_ATT1_PX_Pos                 5UL
#define PERI_PPU_PR_ATT1_PX_Msk                 0x20UL
#define PERI_PPU_PR_ATT1_NS_Pos                 6UL
#define PERI_PPU_PR_ATT1_NS_Msk                 0x40UL
#define PERI_PPU_PR_ATT1_PC_MASK_0_Pos          8UL
#define PERI_PPU_PR_ATT1_PC_MASK_0_Msk          0x100UL
#define PERI_PPU_PR_ATT1_PC_MASK_15_TO_1_Pos    9UL
#define PERI_PPU_PR_ATT1_PC_MASK_15_TO_1_Msk    0xFFFE00UL
#define PERI_PPU_PR_ATT1_REGION_SIZE_Pos        24UL
#define PERI_PPU_PR_ATT1_REGION_SIZE_Msk        0x1F000000UL
#define PERI_PPU_PR_ATT1_PC_MATCH_Pos           30UL
#define PERI_PPU_PR_ATT1_PC_MATCH_Msk           0x40000000UL
#define PERI_PPU_PR_ATT1_ENABLED_Pos            31UL
#define PERI_PPU_PR_ATT1_ENABLED_Msk            0x80000000UL


/* PERI_PPU_GR.ADDR0 */
#define PERI_PPU_GR_ADDR0_SUBREGION_DISABLE_Pos 0UL
#define PERI_PPU_GR_ADDR0_SUBREGION_DISABLE_Msk 0xFFUL
#define PERI_PPU_GR_ADDR0_ADDR24_Pos            8UL
#define PERI_PPU_GR_ADDR0_ADDR24_Msk            0xFFFFFF00UL
/* PERI_PPU_GR.ATT0 */
#define PERI_PPU_GR_ATT0_UR_Pos                 0UL
#define PERI_PPU_GR_ATT0_UR_Msk                 0x1UL
#define PERI_PPU_GR_ATT0_UW_Pos                 1UL
#define PERI_PPU_GR_ATT0_UW_Msk                 0x2UL
#define PERI_PPU_GR_ATT0_UX_Pos                 2UL
#define PERI_PPU_GR_ATT0_UX_Msk                 0x4UL
#define PERI_PPU_GR_ATT0_PR_Pos                 3UL
#define PERI_PPU_GR_ATT0_PR_Msk                 0x8UL
#define PERI_PPU_GR_ATT0_PW_Pos                 4UL
#define PERI_PPU_GR_ATT0_PW_Msk                 0x10UL
#define PERI_PPU_GR_ATT0_PX_Pos                 5UL
#define PERI_PPU_GR_ATT0_PX_Msk                 0x20UL
#define PERI_PPU_GR_ATT0_NS_Pos                 6UL
#define PERI_PPU_GR_ATT0_NS_Msk                 0x40UL
#define PERI_PPU_GR_ATT0_PC_MASK_0_Pos          8UL
#define PERI_PPU_GR_ATT0_PC_MASK_0_Msk          0x100UL
#define PERI_PPU_GR_ATT0_PC_MASK_15_TO_1_Pos    9UL
#define PERI_PPU_GR_ATT0_PC_MASK_15_TO_1_Msk    0xFFFE00UL
#define PERI_PPU_GR_ATT0_REGION_SIZE_Pos        24UL
#define PERI_PPU_GR_ATT0_REGION_SIZE_Msk        0x1F000000UL
#define PERI_PPU_GR_ATT0_PC_MATCH_Pos           30UL
#define PERI_PPU_GR_ATT0_PC_MATCH_Msk           0x40000000UL
#define PERI_PPU_GR_ATT0_ENABLED_Pos            31UL
#define PERI_PPU_GR_ATT0_ENABLED_Msk            0x80000000UL
/* PERI_PPU_GR.ADDR1 */
#define PERI_PPU_GR_ADDR1_SUBREGION_DISABLE_Pos 0UL
#define PERI_PPU_GR_ADDR1_SUBREGION_DISABLE_Msk 0xFFUL
#define PERI_PPU_GR_ADDR1_ADDR24_Pos            8UL
#define PERI_PPU_GR_ADDR1_ADDR24_Msk            0xFFFFFF00UL
/* PERI_PPU_GR.ATT1 */
#define PERI_PPU_GR_ATT1_UR_Pos                 0UL
#define PERI_PPU_GR_ATT1_UR_Msk                 0x1UL
#define PERI_PPU_GR_ATT1_UW_Pos                 1UL
#define PERI_PPU_GR_ATT1_UW_Msk                 0x2UL
#define PERI_PPU_GR_ATT1_UX_Pos                 2UL
#define PERI_PPU_GR_ATT1_UX_Msk                 0x4UL
#define PERI_PPU_GR_ATT1_PR_Pos                 3UL
#define PERI_PPU_GR_ATT1_PR_Msk                 0x8UL
#define PERI_PPU_GR_ATT1_PW_Pos                 4UL
#define PERI_PPU_GR_ATT1_PW_Msk                 0x10UL
#define PERI_PPU_GR_ATT1_PX_Pos                 5UL
#define PERI_PPU_GR_ATT1_PX_Msk                 0x20UL
#define PERI_PPU_GR_ATT1_NS_Pos                 6UL
#define PERI_PPU_GR_ATT1_NS_Msk                 0x40UL
#define PERI_PPU_GR_ATT1_PC_MASK_0_Pos          8UL
#define PERI_PPU_GR_ATT1_PC_MASK_0_Msk          0x100UL
#define PERI_PPU_GR_ATT1_PC_MASK_15_TO_1_Pos    9UL
#define PERI_PPU_GR_ATT1_PC_MASK_15_TO_1_Msk    0xFFFE00UL
#define PERI_PPU_GR_ATT1_REGION_SIZE_Pos        24UL
#define PERI_PPU_GR_ATT1_REGION_SIZE_Msk        0x1F000000UL
#define PERI_PPU_GR_ATT1_PC_MATCH_Pos           30UL
#define PERI_PPU_GR_ATT1_PC_MATCH_Msk           0x40000000UL
#define PERI_PPU_GR_ATT1_ENABLED_Pos            31UL
#define PERI_PPU_GR_ATT1_ENABLED_Msk            0x80000000UL


/* PERI_GR_PPU_SL.ADDR0 */
#define PERI_GR_PPU_SL_ADDR0_SUBREGION_DISABLE_Pos 0UL
#define PERI_GR_PPU_SL_ADDR0_SUBREGION_DISABLE_Msk 0xFFUL
#define PERI_GR_PPU_SL_ADDR0_ADDR24_Pos         8UL
#define PERI_GR_PPU_SL_ADDR0_ADDR24_Msk         0xFFFFFF00UL
/* PERI_GR_PPU_SL.ATT0 */
#define PERI_GR_PPU_SL_ATT0_UR_Pos              0UL
#define PERI_GR_PPU_SL_ATT0_UR_Msk              0x1UL
#define PERI_GR_PPU_SL_ATT0_UW_Pos              1UL
#define PERI_GR_PPU_SL_ATT0_UW_Msk              0x2UL
#define PERI_GR_PPU_SL_ATT0_UX_Pos              2UL
#define PERI_GR_PPU_SL_ATT0_UX_Msk              0x4UL
#define PERI_GR_PPU_SL_ATT0_PR_Pos              3UL
#define PERI_GR_PPU_SL_ATT0_PR_Msk              0x8UL
#define PERI_GR_PPU_SL_ATT0_PW_Pos              4UL
#define PERI_GR_PPU_SL_ATT0_PW_Msk              0x10UL
#define PERI_GR_PPU_SL_ATT0_PX_Pos              5UL
#define PERI_GR_PPU_SL_ATT0_PX_Msk              0x20UL
#define PERI_GR_PPU_SL_ATT0_NS_Pos              6UL
#define PERI_GR_PPU_SL_ATT0_NS_Msk              0x40UL
#define PERI_GR_PPU_SL_ATT0_PC_MASK_0_Pos       8UL
#define PERI_GR_PPU_SL_ATT0_PC_MASK_0_Msk       0x100UL
#define PERI_GR_PPU_SL_ATT0_PC_MASK_15_TO_1_Pos 9UL
#define PERI_GR_PPU_SL_ATT0_PC_MASK_15_TO_1_Msk 0xFFFE00UL
#define PERI_GR_PPU_SL_ATT0_REGION_SIZE_Pos     24UL
#define PERI_GR_PPU_SL_ATT0_REGION_SIZE_Msk     0x1F000000UL
#define PERI_GR_PPU_SL_ATT0_PC_MATCH_Pos        30UL
#define PERI_GR_PPU_SL_ATT0_PC_MATCH_Msk        0x40000000UL
#define PERI_GR_PPU_SL_ATT0_ENABLED_Pos         31UL
#define PERI_GR_PPU_SL_ATT0_ENABLED_Msk         0x80000000UL
/* PERI_GR_PPU_SL.ADDR1 */
#define PERI_GR_PPU_SL_ADDR1_SUBREGION_DISABLE_Pos 0UL
#define PERI_GR_PPU_SL_ADDR1_SUBREGION_DISABLE_Msk 0xFFUL
#define PERI_GR_PPU_SL_ADDR1_ADDR24_Pos         8UL
#define PERI_GR_PPU_SL_ADDR1_ADDR24_Msk         0xFFFFFF00UL
/* PERI_GR_PPU_SL.ATT1 */
#define PERI_GR_PPU_SL_ATT1_UR_Pos              0UL
#define PERI_GR_PPU_SL_ATT1_UR_Msk              0x1UL
#define PERI_GR_PPU_SL_ATT1_UW_Pos              1UL
#define PERI_GR_PPU_SL_ATT1_UW_Msk              0x2UL
#define PERI_GR_PPU_SL_ATT1_UX_Pos              2UL
#define PERI_GR_PPU_SL_ATT1_UX_Msk              0x4UL
#define PERI_GR_PPU_SL_ATT1_PR_Pos              3UL
#define PERI_GR_PPU_SL_ATT1_PR_Msk              0x8UL
#define PERI_GR_PPU_SL_ATT1_PW_Pos              4UL
#define PERI_GR_PPU_SL_ATT1_PW_Msk              0x10UL
#define PERI_GR_PPU_SL_ATT1_PX_Pos              5UL
#define PERI_GR_PPU_SL_ATT1_PX_Msk              0x20UL
#define PERI_GR_PPU_SL_ATT1_NS_Pos              6UL
#define PERI_GR_PPU_SL_ATT1_NS_Msk              0x40UL
#define PERI_GR_PPU_SL_ATT1_PC_MASK_0_Pos       8UL
#define PERI_GR_PPU_SL_ATT1_PC_MASK_0_Msk       0x100UL
#define PERI_GR_PPU_SL_ATT1_PC_MASK_15_TO_1_Pos 9UL
#define PERI_GR_PPU_SL_ATT1_PC_MASK_15_TO_1_Msk 0xFFFE00UL
#define PERI_GR_PPU_SL_ATT1_REGION_SIZE_Pos     24UL
#define PERI_GR_PPU_SL_ATT1_REGION_SIZE_Msk     0x1F000000UL
#define PERI_GR_PPU_SL_ATT1_PC_MATCH_Pos        30UL
#define PERI_GR_PPU_SL_ATT1_PC_MATCH_Msk        0x40000000UL
#define PERI_GR_PPU_SL_ATT1_ENABLED_Pos         31UL
#define PERI_GR_PPU_SL_ATT1_ENABLED_Msk         0x80000000UL


/* PERI_GR_PPU_RG.ADDR0 */
#define PERI_GR_PPU_RG_ADDR0_SUBREGION_DISABLE_Pos 0UL
#define PERI_GR_PPU_RG_ADDR0_SUBREGION_DISABLE_Msk 0xFFUL
#define PERI_GR_PPU_RG_ADDR0_ADDR24_Pos         8UL
#define PERI_GR_PPU_RG_ADDR0_ADDR24_Msk         0xFFFFFF00UL
/* PERI_GR_PPU_RG.ATT0 */
#define PERI_GR_PPU_RG_ATT0_UR_Pos              0UL
#define PERI_GR_PPU_RG_ATT0_UR_Msk              0x1UL
#define PERI_GR_PPU_RG_ATT0_UW_Pos              1UL
#define PERI_GR_PPU_RG_ATT0_UW_Msk              0x2UL
#define PERI_GR_PPU_RG_ATT0_UX_Pos              2UL
#define PERI_GR_PPU_RG_ATT0_UX_Msk              0x4UL
#define PERI_GR_PPU_RG_ATT0_PR_Pos              3UL
#define PERI_GR_PPU_RG_ATT0_PR_Msk              0x8UL
#define PERI_GR_PPU_RG_ATT0_PW_Pos              4UL
#define PERI_GR_PPU_RG_ATT0_PW_Msk              0x10UL
#define PERI_GR_PPU_RG_ATT0_PX_Pos              5UL
#define PERI_GR_PPU_RG_ATT0_PX_Msk              0x20UL
#define PERI_GR_PPU_RG_ATT0_NS_Pos              6UL
#define PERI_GR_PPU_RG_ATT0_NS_Msk              0x40UL
#define PERI_GR_PPU_RG_ATT0_PC_MASK_0_Pos       8UL
#define PERI_GR_PPU_RG_ATT0_PC_MASK_0_Msk       0x100UL
#define PERI_GR_PPU_RG_ATT0_PC_MASK_15_TO_1_Pos 9UL
#define PERI_GR_PPU_RG_ATT0_PC_MASK_15_TO_1_Msk 0xFFFE00UL
#define PERI_GR_PPU_RG_ATT0_REGION_SIZE_Pos     24UL
#define PERI_GR_PPU_RG_ATT0_REGION_SIZE_Msk     0x1F000000UL
#define PERI_GR_PPU_RG_ATT0_PC_MATCH_Pos        30UL
#define PERI_GR_PPU_RG_ATT0_PC_MATCH_Msk        0x40000000UL
#define PERI_GR_PPU_RG_ATT0_ENABLED_Pos         31UL
#define PERI_GR_PPU_RG_ATT0_ENABLED_Msk         0x80000000UL
/* PERI_GR_PPU_RG.ADDR1 */
#define PERI_GR_PPU_RG_ADDR1_SUBREGION_DISABLE_Pos 0UL
#define PERI_GR_PPU_RG_ADDR1_SUBREGION_DISABLE_Msk 0xFFUL
#define PERI_GR_PPU_RG_ADDR1_ADDR24_Pos         8UL
#define PERI_GR_PPU_RG_ADDR1_ADDR24_Msk         0xFFFFFF00UL
/* PERI_GR_PPU_RG.ATT1 */
#define PERI_GR_PPU_RG_ATT1_UR_Pos              0UL
#define PERI_GR_PPU_RG_ATT1_UR_Msk              0x1UL
#define PERI_GR_PPU_RG_ATT1_UW_Pos              1UL
#define PERI_GR_PPU_RG_ATT1_UW_Msk              0x2UL
#define PERI_GR_PPU_RG_ATT1_UX_Pos              2UL
#define PERI_GR_PPU_RG_ATT1_UX_Msk              0x4UL
#define PERI_GR_PPU_RG_ATT1_PR_Pos              3UL
#define PERI_GR_PPU_RG_ATT1_PR_Msk              0x8UL
#define PERI_GR_PPU_RG_ATT1_PW_Pos              4UL
#define PERI_GR_PPU_RG_ATT1_PW_Msk              0x10UL
#define PERI_GR_PPU_RG_ATT1_PX_Pos              5UL
#define PERI_GR_PPU_RG_ATT1_PX_Msk              0x20UL
#define PERI_GR_PPU_RG_ATT1_NS_Pos              6UL
#define PERI_GR_PPU_RG_ATT1_NS_Msk              0x40UL
#define PERI_GR_PPU_RG_ATT1_PC_MASK_0_Pos       8UL
#define PERI_GR_PPU_RG_ATT1_PC_MASK_0_Msk       0x100UL
#define PERI_GR_PPU_RG_ATT1_PC_MASK_15_TO_1_Pos 9UL
#define PERI_GR_PPU_RG_ATT1_PC_MASK_15_TO_1_Msk 0xFFFE00UL
#define PERI_GR_PPU_RG_ATT1_REGION_SIZE_Pos     24UL
#define PERI_GR_PPU_RG_ATT1_REGION_SIZE_Msk     0x1F000000UL
#define PERI_GR_PPU_RG_ATT1_PC_MATCH_Pos        30UL
#define PERI_GR_PPU_RG_ATT1_PC_MATCH_Msk        0x40000000UL
#define PERI_GR_PPU_RG_ATT1_ENABLED_Pos         31UL
#define PERI_GR_PPU_RG_ATT1_ENABLED_Msk         0x80000000UL


/* PERI.DIV_CMD */
#define PERI_DIV_CMD_DIV_SEL_Pos                0UL
#define PERI_DIV_CMD_DIV_SEL_Msk                0x3FUL
#define PERI_DIV_CMD_TYPE_SEL_Pos               6UL
#define PERI_DIV_CMD_TYPE_SEL_Msk               0xC0UL
#define PERI_DIV_CMD_PA_DIV_SEL_Pos             8UL
#define PERI_DIV_CMD_PA_DIV_SEL_Msk             0x3F00UL
#define PERI_DIV_CMD_PA_TYPE_SEL_Pos            14UL
#define PERI_DIV_CMD_PA_TYPE_SEL_Msk            0xC000UL
#define PERI_DIV_CMD_DISABLE_Pos                30UL
#define PERI_DIV_CMD_DISABLE_Msk                0x40000000UL
#define PERI_DIV_CMD_ENABLE_Pos                 31UL
#define PERI_DIV_CMD_ENABLE_Msk                 0x80000000UL
/* PERI.DIV_8_CTL */
#define PERI_DIV_8_CTL_EN_Pos                   0UL
#define PERI_DIV_8_CTL_EN_Msk                   0x1UL
#define PERI_DIV_8_CTL_INT8_DIV_Pos             8UL
#define PERI_DIV_8_CTL_INT8_DIV_Msk             0xFF00UL
/* PERI.DIV_16_CTL */
#define PERI_DIV_16_CTL_EN_Pos                  0UL
#define PERI_DIV_16_CTL_EN_Msk                  0x1UL
#define PERI_DIV_16_CTL_INT16_DIV_Pos           8UL
#define PERI_DIV_16_CTL_INT16_DIV_Msk           0xFFFF00UL
/* PERI.DIV_16_5_CTL */
#define PERI_DIV_16_5_CTL_EN_Pos                0UL
#define PERI_DIV_16_5_CTL_EN_Msk                0x1UL
#define PERI_DIV_16_5_CTL_FRAC5_DIV_Pos         3UL
#define PERI_DIV_16_5_CTL_FRAC5_DIV_Msk         0xF8UL
#define PERI_DIV_16_5_CTL_INT16_DIV_Pos         8UL
#define PERI_DIV_16_5_CTL_INT16_DIV_Msk         0xFFFF00UL
/* PERI.DIV_24_5_CTL */
#define PERI_DIV_24_5_CTL_EN_Pos                0UL
#define PERI_DIV_24_5_CTL_EN_Msk                0x1UL
#define PERI_DIV_24_5_CTL_FRAC5_DIV_Pos         3UL
#define PERI_DIV_24_5_CTL_FRAC5_DIV_Msk         0xF8UL
#define PERI_DIV_24_5_CTL_INT24_DIV_Pos         8UL
#define PERI_DIV_24_5_CTL_INT24_DIV_Msk         0xFFFFFF00UL
/* PERI.CLOCK_CTL */
#define PERI_CLOCK_CTL_DIV_SEL_Pos              0UL
#define PERI_CLOCK_CTL_DIV_SEL_Msk              0x3FUL
#define PERI_CLOCK_CTL_TYPE_SEL_Pos             6UL
#define PERI_CLOCK_CTL_TYPE_SEL_Msk             0xC0UL
/* PERI.TR_CMD */
#define PERI_TR_CMD_TR_SEL_Pos                  0UL
#define PERI_TR_CMD_TR_SEL_Msk                  0xFFUL
#define PERI_TR_CMD_GROUP_SEL_Pos               8UL
#define PERI_TR_CMD_GROUP_SEL_Msk               0xF00UL
#define PERI_TR_CMD_COUNT_Pos                   16UL
#define PERI_TR_CMD_COUNT_Msk                   0xFF0000UL
#define PERI_TR_CMD_OUT_SEL_Pos                 30UL
#define PERI_TR_CMD_OUT_SEL_Msk                 0x40000000UL
#define PERI_TR_CMD_ACTIVATE_Pos                31UL
#define PERI_TR_CMD_ACTIVATE_Msk                0x80000000UL


#endif /* _CYIP_PERI_H_ */


/* [] END OF FILE */
