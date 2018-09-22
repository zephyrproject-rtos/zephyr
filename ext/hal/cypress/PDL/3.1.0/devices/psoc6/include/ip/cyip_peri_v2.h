/***************************************************************************//**
* \file cyip_peri_v2.h
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

#ifndef _CYIP_PERI_V2_H_
#define _CYIP_PERI_V2_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     PERI
*******************************************************************************/

#define PERI_GR_V2_SECTION_SIZE                 0x00000020UL
#define PERI_TR_GR_V2_SECTION_SIZE              0x00000400UL
#define PERI_TR_1TO1_GR_V2_SECTION_SIZE         0x00000400UL
#define PERI_V2_SECTION_SIZE                    0x00010000UL

/**
  * \brief Peripheral group structure (PERI_GR)
  */
typedef struct {
  __IOM uint32_t CLOCK_CTL;                     /*!< 0x00000000 Clock control */
   __IM uint32_t RESERVED[3];
  __IOM uint32_t SL_CTL;                        /*!< 0x00000010 Slave control */
   __IM uint32_t RESERVED1[3];
} PERI_GR_V2_Type;                              /*!< Size = 32 (0x20) */

/**
  * \brief Trigger group (PERI_TR_GR)
  */
typedef struct {
  __IOM uint32_t TR_CTL[256];                   /*!< 0x00000000 Trigger control register */
} PERI_TR_GR_V2_Type;                           /*!< Size = 1024 (0x400) */

/**
  * \brief Trigger 1-to-1 group (PERI_TR_1TO1_GR)
  */
typedef struct {
  __IOM uint32_t TR_CTL[256];                   /*!< 0x00000000 Trigger control register */
} PERI_TR_1TO1_GR_V2_Type;                      /*!< Size = 1024 (0x400) */

/**
  * \brief Peripheral interconnect (PERI)
  */
typedef struct {
   __IM uint32_t RESERVED[128];
  __IOM uint32_t TIMEOUT_CTL;                   /*!< 0x00000200 Timeout control */
   __IM uint32_t RESERVED1[7];
  __IOM uint32_t TR_CMD;                        /*!< 0x00000220 Trigger command */
   __IM uint32_t RESERVED2[119];
  __IOM uint32_t DIV_CMD;                       /*!< 0x00000400 Divider command */
   __IM uint32_t RESERVED3[511];
  __IOM uint32_t CLOCK_CTL[256];                /*!< 0x00000C00 Clock control */
  __IOM uint32_t DIV_8_CTL[256];                /*!< 0x00001000 Divider control (for 8.0 divider) */
  __IOM uint32_t DIV_16_CTL[256];               /*!< 0x00001400 Divider control (for 16.0 divider) */
  __IOM uint32_t DIV_16_5_CTL[256];             /*!< 0x00001800 Divider control (for 16.5 divider) */
  __IOM uint32_t DIV_24_5_CTL[255];             /*!< 0x00001C00 Divider control (for 24.5 divider) */
   __IM uint32_t RESERVED4;
  __IOM uint32_t ECC_CTL;                       /*!< 0x00002000 ECC control */
   __IM uint32_t RESERVED5[2047];
        PERI_GR_V2_Type GR[16];                 /*!< 0x00004000 Peripheral group structure */
   __IM uint32_t RESERVED6[3968];
        PERI_TR_GR_V2_Type TR_GR[16];           /*!< 0x00008000 Trigger group */
        PERI_TR_1TO1_GR_V2_Type TR_1TO1_GR[16]; /*!< 0x0000C000 Trigger 1-to-1 group */
} PERI_V2_Type;                                 /*!< Size = 65536 (0x10000) */


/* PERI_GR.CLOCK_CTL */
#define PERI_GR_V2_CLOCK_CTL_INT8_DIV_Pos       8UL
#define PERI_GR_V2_CLOCK_CTL_INT8_DIV_Msk       0xFF00UL
/* PERI_GR.SL_CTL */
#define PERI_GR_V2_SL_CTL_ENABLED_0_Pos         0UL
#define PERI_GR_V2_SL_CTL_ENABLED_0_Msk         0x1UL
#define PERI_GR_V2_SL_CTL_ENABLED_1_Pos         1UL
#define PERI_GR_V2_SL_CTL_ENABLED_1_Msk         0x2UL
#define PERI_GR_V2_SL_CTL_ENABLED_2_Pos         2UL
#define PERI_GR_V2_SL_CTL_ENABLED_2_Msk         0x4UL
#define PERI_GR_V2_SL_CTL_ENABLED_3_Pos         3UL
#define PERI_GR_V2_SL_CTL_ENABLED_3_Msk         0x8UL
#define PERI_GR_V2_SL_CTL_ENABLED_4_Pos         4UL
#define PERI_GR_V2_SL_CTL_ENABLED_4_Msk         0x10UL
#define PERI_GR_V2_SL_CTL_ENABLED_5_Pos         5UL
#define PERI_GR_V2_SL_CTL_ENABLED_5_Msk         0x20UL
#define PERI_GR_V2_SL_CTL_ENABLED_6_Pos         6UL
#define PERI_GR_V2_SL_CTL_ENABLED_6_Msk         0x40UL
#define PERI_GR_V2_SL_CTL_ENABLED_7_Pos         7UL
#define PERI_GR_V2_SL_CTL_ENABLED_7_Msk         0x80UL
#define PERI_GR_V2_SL_CTL_ENABLED_8_Pos         8UL
#define PERI_GR_V2_SL_CTL_ENABLED_8_Msk         0x100UL
#define PERI_GR_V2_SL_CTL_ENABLED_9_Pos         9UL
#define PERI_GR_V2_SL_CTL_ENABLED_9_Msk         0x200UL
#define PERI_GR_V2_SL_CTL_ENABLED_10_Pos        10UL
#define PERI_GR_V2_SL_CTL_ENABLED_10_Msk        0x400UL
#define PERI_GR_V2_SL_CTL_ENABLED_11_Pos        11UL
#define PERI_GR_V2_SL_CTL_ENABLED_11_Msk        0x800UL
#define PERI_GR_V2_SL_CTL_ENABLED_12_Pos        12UL
#define PERI_GR_V2_SL_CTL_ENABLED_12_Msk        0x1000UL
#define PERI_GR_V2_SL_CTL_ENABLED_13_Pos        13UL
#define PERI_GR_V2_SL_CTL_ENABLED_13_Msk        0x2000UL
#define PERI_GR_V2_SL_CTL_ENABLED_14_Pos        14UL
#define PERI_GR_V2_SL_CTL_ENABLED_14_Msk        0x4000UL
#define PERI_GR_V2_SL_CTL_ENABLED_15_Pos        15UL
#define PERI_GR_V2_SL_CTL_ENABLED_15_Msk        0x8000UL
#define PERI_GR_V2_SL_CTL_DISABLED_0_Pos        16UL
#define PERI_GR_V2_SL_CTL_DISABLED_0_Msk        0x10000UL
#define PERI_GR_V2_SL_CTL_DISABLED_1_Pos        17UL
#define PERI_GR_V2_SL_CTL_DISABLED_1_Msk        0x20000UL
#define PERI_GR_V2_SL_CTL_DISABLED_2_Pos        18UL
#define PERI_GR_V2_SL_CTL_DISABLED_2_Msk        0x40000UL
#define PERI_GR_V2_SL_CTL_DISABLED_3_Pos        19UL
#define PERI_GR_V2_SL_CTL_DISABLED_3_Msk        0x80000UL
#define PERI_GR_V2_SL_CTL_DISABLED_4_Pos        20UL
#define PERI_GR_V2_SL_CTL_DISABLED_4_Msk        0x100000UL
#define PERI_GR_V2_SL_CTL_DISABLED_5_Pos        21UL
#define PERI_GR_V2_SL_CTL_DISABLED_5_Msk        0x200000UL
#define PERI_GR_V2_SL_CTL_DISABLED_6_Pos        22UL
#define PERI_GR_V2_SL_CTL_DISABLED_6_Msk        0x400000UL
#define PERI_GR_V2_SL_CTL_DISABLED_7_Pos        23UL
#define PERI_GR_V2_SL_CTL_DISABLED_7_Msk        0x800000UL
#define PERI_GR_V2_SL_CTL_DISABLED_8_Pos        24UL
#define PERI_GR_V2_SL_CTL_DISABLED_8_Msk        0x1000000UL
#define PERI_GR_V2_SL_CTL_DISABLED_9_Pos        25UL
#define PERI_GR_V2_SL_CTL_DISABLED_9_Msk        0x2000000UL
#define PERI_GR_V2_SL_CTL_DISABLED_10_Pos       26UL
#define PERI_GR_V2_SL_CTL_DISABLED_10_Msk       0x4000000UL
#define PERI_GR_V2_SL_CTL_DISABLED_11_Pos       27UL
#define PERI_GR_V2_SL_CTL_DISABLED_11_Msk       0x8000000UL
#define PERI_GR_V2_SL_CTL_DISABLED_12_Pos       28UL
#define PERI_GR_V2_SL_CTL_DISABLED_12_Msk       0x10000000UL
#define PERI_GR_V2_SL_CTL_DISABLED_13_Pos       29UL
#define PERI_GR_V2_SL_CTL_DISABLED_13_Msk       0x20000000UL
#define PERI_GR_V2_SL_CTL_DISABLED_14_Pos       30UL
#define PERI_GR_V2_SL_CTL_DISABLED_14_Msk       0x40000000UL
#define PERI_GR_V2_SL_CTL_DISABLED_15_Pos       31UL
#define PERI_GR_V2_SL_CTL_DISABLED_15_Msk       0x80000000UL


/* PERI_TR_GR.TR_CTL */
#define PERI_TR_GR_V2_TR_CTL_TR_SEL_Pos         0UL
#define PERI_TR_GR_V2_TR_CTL_TR_SEL_Msk         0xFFUL
#define PERI_TR_GR_V2_TR_CTL_TR_INV_Pos         8UL
#define PERI_TR_GR_V2_TR_CTL_TR_INV_Msk         0x100UL
#define PERI_TR_GR_V2_TR_CTL_TR_EDGE_Pos        9UL
#define PERI_TR_GR_V2_TR_CTL_TR_EDGE_Msk        0x200UL
#define PERI_TR_GR_V2_TR_CTL_DBG_FREEZE_EN_Pos  12UL
#define PERI_TR_GR_V2_TR_CTL_DBG_FREEZE_EN_Msk  0x1000UL


/* PERI_TR_1TO1_GR.TR_CTL */
#define PERI_TR_1TO1_GR_V2_TR_CTL_TR_SEL_Pos    0UL
#define PERI_TR_1TO1_GR_V2_TR_CTL_TR_SEL_Msk    0x1UL
#define PERI_TR_1TO1_GR_V2_TR_CTL_TR_INV_Pos    8UL
#define PERI_TR_1TO1_GR_V2_TR_CTL_TR_INV_Msk    0x100UL
#define PERI_TR_1TO1_GR_V2_TR_CTL_TR_EDGE_Pos   9UL
#define PERI_TR_1TO1_GR_V2_TR_CTL_TR_EDGE_Msk   0x200UL
#define PERI_TR_1TO1_GR_V2_TR_CTL_DBG_FREEZE_EN_Pos 12UL
#define PERI_TR_1TO1_GR_V2_TR_CTL_DBG_FREEZE_EN_Msk 0x1000UL


/* PERI.TIMEOUT_CTL */
#define PERI_V2_TIMEOUT_CTL_TIMEOUT_Pos         0UL
#define PERI_V2_TIMEOUT_CTL_TIMEOUT_Msk         0xFFFFUL
/* PERI.TR_CMD */
#define PERI_V2_TR_CMD_TR_SEL_Pos               0UL
#define PERI_V2_TR_CMD_TR_SEL_Msk               0xFFUL
#define PERI_V2_TR_CMD_GROUP_SEL_Pos            8UL
#define PERI_V2_TR_CMD_GROUP_SEL_Msk            0x1F00UL
#define PERI_V2_TR_CMD_TR_EDGE_Pos              29UL
#define PERI_V2_TR_CMD_TR_EDGE_Msk              0x20000000UL
#define PERI_V2_TR_CMD_OUT_SEL_Pos              30UL
#define PERI_V2_TR_CMD_OUT_SEL_Msk              0x40000000UL
#define PERI_V2_TR_CMD_ACTIVATE_Pos             31UL
#define PERI_V2_TR_CMD_ACTIVATE_Msk             0x80000000UL
/* PERI.DIV_CMD */
#define PERI_V2_DIV_CMD_DIV_SEL_Pos             0UL
#define PERI_V2_DIV_CMD_DIV_SEL_Msk             0xFFUL
#define PERI_V2_DIV_CMD_TYPE_SEL_Pos            8UL
#define PERI_V2_DIV_CMD_TYPE_SEL_Msk            0x300UL
#define PERI_V2_DIV_CMD_PA_DIV_SEL_Pos          16UL
#define PERI_V2_DIV_CMD_PA_DIV_SEL_Msk          0xFF0000UL
#define PERI_V2_DIV_CMD_PA_TYPE_SEL_Pos         24UL
#define PERI_V2_DIV_CMD_PA_TYPE_SEL_Msk         0x3000000UL
#define PERI_V2_DIV_CMD_DISABLE_Pos             30UL
#define PERI_V2_DIV_CMD_DISABLE_Msk             0x40000000UL
#define PERI_V2_DIV_CMD_ENABLE_Pos              31UL
#define PERI_V2_DIV_CMD_ENABLE_Msk              0x80000000UL
/* PERI.CLOCK_CTL */
#define PERI_V2_CLOCK_CTL_DIV_SEL_Pos           0UL
#define PERI_V2_CLOCK_CTL_DIV_SEL_Msk           0xFFUL
#define PERI_V2_CLOCK_CTL_TYPE_SEL_Pos          8UL
#define PERI_V2_CLOCK_CTL_TYPE_SEL_Msk          0x300UL
/* PERI.DIV_8_CTL */
#define PERI_V2_DIV_8_CTL_EN_Pos                0UL
#define PERI_V2_DIV_8_CTL_EN_Msk                0x1UL
#define PERI_V2_DIV_8_CTL_INT8_DIV_Pos          8UL
#define PERI_V2_DIV_8_CTL_INT8_DIV_Msk          0xFF00UL
/* PERI.DIV_16_CTL */
#define PERI_V2_DIV_16_CTL_EN_Pos               0UL
#define PERI_V2_DIV_16_CTL_EN_Msk               0x1UL
#define PERI_V2_DIV_16_CTL_INT16_DIV_Pos        8UL
#define PERI_V2_DIV_16_CTL_INT16_DIV_Msk        0xFFFF00UL
/* PERI.DIV_16_5_CTL */
#define PERI_V2_DIV_16_5_CTL_EN_Pos             0UL
#define PERI_V2_DIV_16_5_CTL_EN_Msk             0x1UL
#define PERI_V2_DIV_16_5_CTL_FRAC5_DIV_Pos      3UL
#define PERI_V2_DIV_16_5_CTL_FRAC5_DIV_Msk      0xF8UL
#define PERI_V2_DIV_16_5_CTL_INT16_DIV_Pos      8UL
#define PERI_V2_DIV_16_5_CTL_INT16_DIV_Msk      0xFFFF00UL
/* PERI.DIV_24_5_CTL */
#define PERI_V2_DIV_24_5_CTL_EN_Pos             0UL
#define PERI_V2_DIV_24_5_CTL_EN_Msk             0x1UL
#define PERI_V2_DIV_24_5_CTL_FRAC5_DIV_Pos      3UL
#define PERI_V2_DIV_24_5_CTL_FRAC5_DIV_Msk      0xF8UL
#define PERI_V2_DIV_24_5_CTL_INT24_DIV_Pos      8UL
#define PERI_V2_DIV_24_5_CTL_INT24_DIV_Msk      0xFFFFFF00UL
/* PERI.ECC_CTL */
#define PERI_V2_ECC_CTL_WORD_ADDR_Pos           0UL
#define PERI_V2_ECC_CTL_WORD_ADDR_Msk           0x7FFUL
#define PERI_V2_ECC_CTL_ECC_EN_Pos              16UL
#define PERI_V2_ECC_CTL_ECC_EN_Msk              0x10000UL
#define PERI_V2_ECC_CTL_ECC_INJ_EN_Pos          18UL
#define PERI_V2_ECC_CTL_ECC_INJ_EN_Msk          0x40000UL
#define PERI_V2_ECC_CTL_PARITY_Pos              24UL
#define PERI_V2_ECC_CTL_PARITY_Msk              0xFF000000UL


#endif /* _CYIP_PERI_V2_H_ */


/* [] END OF FILE */
