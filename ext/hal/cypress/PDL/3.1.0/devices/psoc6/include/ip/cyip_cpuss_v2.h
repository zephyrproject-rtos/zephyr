/***************************************************************************//**
* \file cyip_cpuss_v2.h
*
* \brief
* CPUSS IP definitions
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

#ifndef _CYIP_CPUSS_V2_H_
#define _CYIP_CPUSS_V2_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                    CPUSS
*******************************************************************************/

#define CPUSS_V2_SECTION_SIZE                   0x00010000UL

/**
  * \brief CPU subsystem (CPUSS) (CPUSS)
  */
typedef struct {
   __IM uint32_t IDENTITY;                      /*!< 0x00000000 Identity */
   __IM uint32_t CM4_STATUS;                    /*!< 0x00000004 CM4 status */
  __IOM uint32_t CM4_CLOCK_CTL;                 /*!< 0x00000008 CM4 clock control */
  __IOM uint32_t CM4_CTL;                       /*!< 0x0000000C CM4 control */
   __IM uint32_t RESERVED[60];
   __IM uint32_t CM4_INT0_STATUS;               /*!< 0x00000100 CM4 interrupt 0 status */
   __IM uint32_t CM4_INT1_STATUS;               /*!< 0x00000104 CM4 interrupt 1 status */
   __IM uint32_t CM4_INT2_STATUS;               /*!< 0x00000108 CM4 interrupt 2 status */
   __IM uint32_t CM4_INT3_STATUS;               /*!< 0x0000010C CM4 interrupt 3 status */
   __IM uint32_t CM4_INT4_STATUS;               /*!< 0x00000110 CM4 interrupt 4 status */
   __IM uint32_t CM4_INT5_STATUS;               /*!< 0x00000114 CM4 interrupt 5 status */
   __IM uint32_t CM4_INT6_STATUS;               /*!< 0x00000118 CM4 interrupt 6 status */
   __IM uint32_t CM4_INT7_STATUS;               /*!< 0x0000011C CM4 interrupt 7 status */
   __IM uint32_t RESERVED1[56];
  __IOM uint32_t CM4_VECTOR_TABLE_BASE;         /*!< 0x00000200 CM4 vector table base */
   __IM uint32_t RESERVED2[15];
  __IOM uint32_t CM4_NMI_CTL[4];                /*!< 0x00000240 CM4 NMI control */
   __IM uint32_t RESERVED3[44];
  __IOM uint32_t UDB_PWR_CTL;                   /*!< 0x00000300 UDB power control */
  __IOM uint32_t UDB_PWR_DELAY_CTL;             /*!< 0x00000304 UDB power control */
   __IM uint32_t RESERVED4[830];
  __IOM uint32_t CM0_CTL;                       /*!< 0x00001000 CM0+ control */
   __IM uint32_t CM0_STATUS;                    /*!< 0x00001004 CM0+ status */
  __IOM uint32_t CM0_CLOCK_CTL;                 /*!< 0x00001008 CM0+ clock control */
   __IM uint32_t RESERVED5[61];
   __IM uint32_t CM0_INT0_STATUS;               /*!< 0x00001100 CM0+ interrupt 0 status */
   __IM uint32_t CM0_INT1_STATUS;               /*!< 0x00001104 CM0+ interrupt 1 status */
   __IM uint32_t CM0_INT2_STATUS;               /*!< 0x00001108 CM0+ interrupt 2 status */
   __IM uint32_t CM0_INT3_STATUS;               /*!< 0x0000110C CM0+ interrupt 3 status */
   __IM uint32_t CM0_INT4_STATUS;               /*!< 0x00001110 CM0+ interrupt 4 status */
   __IM uint32_t CM0_INT5_STATUS;               /*!< 0x00001114 CM0+ interrupt 5 status */
   __IM uint32_t CM0_INT6_STATUS;               /*!< 0x00001118 CM0+ interrupt 6 status */
   __IM uint32_t CM0_INT7_STATUS;               /*!< 0x0000111C CM0+ interrupt 7 status */
  __IOM uint32_t CM0_VECTOR_TABLE_BASE;         /*!< 0x00001120 CM0+ vector table base */
   __IM uint32_t RESERVED6[7];
  __IOM uint32_t CM0_NMI_CTL[4];                /*!< 0x00001140 CM0+ NMI control */
   __IM uint32_t RESERVED7[44];
  __IOM uint32_t CM4_PWR_CTL;                   /*!< 0x00001200 CM4 power control */
  __IOM uint32_t CM4_PWR_DELAY_CTL;             /*!< 0x00001204 CM4 power control */
   __IM uint32_t RESERVED8[62];
  __IOM uint32_t RAM0_CTL0;                     /*!< 0x00001300 RAM 0 control */
   __IM uint32_t RAM0_STATUS;                   /*!< 0x00001304 RAM 0 status */
   __IM uint32_t RESERVED9[14];
  __IOM uint32_t RAM0_PWR_MACRO_CTL[16];        /*!< 0x00001340 RAM 0 power control */
  __IOM uint32_t RAM1_CTL0;                     /*!< 0x00001380 RAM 1 control */
   __IM uint32_t RAM1_STATUS;                   /*!< 0x00001384 RAM 1 status */
  __IOM uint32_t RAM1_PWR_CTL;                  /*!< 0x00001388 RAM 1 power control */
   __IM uint32_t RESERVED10[5];
  __IOM uint32_t RAM2_CTL0;                     /*!< 0x000013A0 RAM 2 control */
   __IM uint32_t RAM2_STATUS;                   /*!< 0x000013A4 RAM 2 status */
  __IOM uint32_t RAM2_PWR_CTL;                  /*!< 0x000013A8 RAM 2 power control */
   __IM uint32_t RESERVED11[5];
  __IOM uint32_t RAM_PWR_DELAY_CTL;             /*!< 0x000013C0 Power up delay used for all SRAM power domains */
  __IOM uint32_t ROM_CTL;                       /*!< 0x000013C4 ROM control */
  __IOM uint32_t ECC_CTL;                       /*!< 0x000013C8 ECC control */
   __IM uint32_t RESERVED12[13];
   __IM uint32_t PRODUCT_ID;                    /*!< 0x00001400 Product identifier and version (same as CoreSight RomTables) */
   __IM uint32_t RESERVED13[3];
   __IM uint32_t DP_STATUS;                     /*!< 0x00001410 Debug port status */
   __IM uint32_t RESERVED14[59];
  __IOM uint32_t BUFF_CTL;                      /*!< 0x00001500 Buffer control */
   __IM uint32_t RESERVED15[63];
  __IOM uint32_t SYSTICK_CTL;                   /*!< 0x00001600 SysTick timer control */
   __IM uint32_t RESERVED16[64];
   __IM uint32_t MBIST_STAT;                    /*!< 0x00001704 Memory BIST status */
   __IM uint32_t RESERVED17[62];
  __IOM uint32_t CAL_SUP_SET;                   /*!< 0x00001800 Calibration support set and read */
  __IOM uint32_t CAL_SUP_CLR;                   /*!< 0x00001804 Calibration support clear and reset */
   __IM uint32_t RESERVED18[510];
  __IOM uint32_t CM0_PC_CTL;                    /*!< 0x00002000 CM0+ protection context control */
   __IM uint32_t RESERVED19[15];
  __IOM uint32_t CM0_PC0_HANDLER;               /*!< 0x00002040 CM0+ protection context 0 handler */
  __IOM uint32_t CM0_PC1_HANDLER;               /*!< 0x00002044 CM0+ protection context 1 handler */
  __IOM uint32_t CM0_PC2_HANDLER;               /*!< 0x00002048 CM0+ protection context 2 handler */
  __IOM uint32_t CM0_PC3_HANDLER;               /*!< 0x0000204C CM0+ protection context 3 handler */
   __IM uint32_t RESERVED20[29];
  __IOM uint32_t PROTECTION;                    /*!< 0x000020C4 Protection status */
   __IM uint32_t RESERVED21[14];
  __IOM uint32_t TRIM_ROM_CTL;                  /*!< 0x00002100 ROM trim control */
  __IOM uint32_t TRIM_RAM_CTL;                  /*!< 0x00002104 RAM trim control */
   __IM uint32_t RESERVED22[6078];
  __IOM uint32_t CM0_SYSTEM_INT_CTL[1023];      /*!< 0x00008000 CM0+ system interrupt control */
   __IM uint32_t RESERVED23[1025];
  __IOM uint32_t CM4_SYSTEM_INT_CTL[1023];      /*!< 0x0000A000 CM4 system interrupt control */
} CPUSS_V2_Type;                                /*!< Size = 45052 (0xAFFC) */


/* CPUSS.IDENTITY */
#define CPUSS_V2_IDENTITY_P_Pos                 0UL
#define CPUSS_V2_IDENTITY_P_Msk                 0x1UL
#define CPUSS_V2_IDENTITY_NS_Pos                1UL
#define CPUSS_V2_IDENTITY_NS_Msk                0x2UL
#define CPUSS_V2_IDENTITY_PC_Pos                4UL
#define CPUSS_V2_IDENTITY_PC_Msk                0xF0UL
#define CPUSS_V2_IDENTITY_MS_Pos                8UL
#define CPUSS_V2_IDENTITY_MS_Msk                0xF00UL
/* CPUSS.CM4_STATUS */
#define CPUSS_V2_CM4_STATUS_SLEEPING_Pos        0UL
#define CPUSS_V2_CM4_STATUS_SLEEPING_Msk        0x1UL
#define CPUSS_V2_CM4_STATUS_SLEEPDEEP_Pos       1UL
#define CPUSS_V2_CM4_STATUS_SLEEPDEEP_Msk       0x2UL
#define CPUSS_V2_CM4_STATUS_PWR_DONE_Pos        4UL
#define CPUSS_V2_CM4_STATUS_PWR_DONE_Msk        0x10UL
/* CPUSS.CM4_CLOCK_CTL */
#define CPUSS_V2_CM4_CLOCK_CTL_FAST_INT_DIV_Pos 8UL
#define CPUSS_V2_CM4_CLOCK_CTL_FAST_INT_DIV_Msk 0xFF00UL
/* CPUSS.CM4_CTL */
#define CPUSS_V2_CM4_CTL_IOC_MASK_Pos           24UL
#define CPUSS_V2_CM4_CTL_IOC_MASK_Msk           0x1000000UL
#define CPUSS_V2_CM4_CTL_DZC_MASK_Pos           25UL
#define CPUSS_V2_CM4_CTL_DZC_MASK_Msk           0x2000000UL
#define CPUSS_V2_CM4_CTL_OFC_MASK_Pos           26UL
#define CPUSS_V2_CM4_CTL_OFC_MASK_Msk           0x4000000UL
#define CPUSS_V2_CM4_CTL_UFC_MASK_Pos           27UL
#define CPUSS_V2_CM4_CTL_UFC_MASK_Msk           0x8000000UL
#define CPUSS_V2_CM4_CTL_IXC_MASK_Pos           28UL
#define CPUSS_V2_CM4_CTL_IXC_MASK_Msk           0x10000000UL
#define CPUSS_V2_CM4_CTL_IDC_MASK_Pos           31UL
#define CPUSS_V2_CM4_CTL_IDC_MASK_Msk           0x80000000UL
/* CPUSS.CM4_INT0_STATUS */
#define CPUSS_V2_CM4_INT0_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_INT0_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM4_INT0_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_INT0_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_INT1_STATUS */
#define CPUSS_V2_CM4_INT1_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_INT1_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM4_INT1_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_INT1_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_INT2_STATUS */
#define CPUSS_V2_CM4_INT2_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_INT2_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM4_INT2_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_INT2_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_INT3_STATUS */
#define CPUSS_V2_CM4_INT3_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_INT3_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM4_INT3_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_INT3_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_INT4_STATUS */
#define CPUSS_V2_CM4_INT4_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_INT4_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM4_INT4_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_INT4_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_INT5_STATUS */
#define CPUSS_V2_CM4_INT5_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_INT5_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM4_INT5_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_INT5_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_INT6_STATUS */
#define CPUSS_V2_CM4_INT6_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_INT6_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM4_INT6_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_INT6_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_INT7_STATUS */
#define CPUSS_V2_CM4_INT7_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_INT7_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM4_INT7_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_INT7_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_VECTOR_TABLE_BASE */
#define CPUSS_V2_CM4_VECTOR_TABLE_BASE_ADDR22_Pos 10UL
#define CPUSS_V2_CM4_VECTOR_TABLE_BASE_ADDR22_Msk 0xFFFFFC00UL
/* CPUSS.CM4_NMI_CTL */
#define CPUSS_V2_CM4_NMI_CTL_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_NMI_CTL_SYSTEM_INT_IDX_Msk 0x3FFUL
/* CPUSS.UDB_PWR_CTL */
#define CPUSS_V2_UDB_PWR_CTL_PWR_MODE_Pos       0UL
#define CPUSS_V2_UDB_PWR_CTL_PWR_MODE_Msk       0x3UL
#define CPUSS_V2_UDB_PWR_CTL_VECTKEYSTAT_Pos    16UL
#define CPUSS_V2_UDB_PWR_CTL_VECTKEYSTAT_Msk    0xFFFF0000UL
/* CPUSS.UDB_PWR_DELAY_CTL */
#define CPUSS_V2_UDB_PWR_DELAY_CTL_UP_Pos       0UL
#define CPUSS_V2_UDB_PWR_DELAY_CTL_UP_Msk       0x3FFUL
/* CPUSS.CM0_CTL */
#define CPUSS_V2_CM0_CTL_SLV_STALL_Pos          0UL
#define CPUSS_V2_CM0_CTL_SLV_STALL_Msk          0x1UL
#define CPUSS_V2_CM0_CTL_ENABLED_Pos            1UL
#define CPUSS_V2_CM0_CTL_ENABLED_Msk            0x2UL
#define CPUSS_V2_CM0_CTL_VECTKEYSTAT_Pos        16UL
#define CPUSS_V2_CM0_CTL_VECTKEYSTAT_Msk        0xFFFF0000UL
/* CPUSS.CM0_STATUS */
#define CPUSS_V2_CM0_STATUS_SLEEPING_Pos        0UL
#define CPUSS_V2_CM0_STATUS_SLEEPING_Msk        0x1UL
#define CPUSS_V2_CM0_STATUS_SLEEPDEEP_Pos       1UL
#define CPUSS_V2_CM0_STATUS_SLEEPDEEP_Msk       0x2UL
/* CPUSS.CM0_CLOCK_CTL */
#define CPUSS_V2_CM0_CLOCK_CTL_SLOW_INT_DIV_Pos 8UL
#define CPUSS_V2_CM0_CLOCK_CTL_SLOW_INT_DIV_Msk 0xFF00UL
#define CPUSS_V2_CM0_CLOCK_CTL_PERI_INT_DIV_Pos 24UL
#define CPUSS_V2_CM0_CLOCK_CTL_PERI_INT_DIV_Msk 0xFF000000UL
/* CPUSS.CM0_INT0_STATUS */
#define CPUSS_V2_CM0_INT0_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_INT0_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM0_INT0_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_INT0_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM0_INT1_STATUS */
#define CPUSS_V2_CM0_INT1_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_INT1_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM0_INT1_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_INT1_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM0_INT2_STATUS */
#define CPUSS_V2_CM0_INT2_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_INT2_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM0_INT2_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_INT2_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM0_INT3_STATUS */
#define CPUSS_V2_CM0_INT3_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_INT3_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM0_INT3_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_INT3_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM0_INT4_STATUS */
#define CPUSS_V2_CM0_INT4_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_INT4_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM0_INT4_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_INT4_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM0_INT5_STATUS */
#define CPUSS_V2_CM0_INT5_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_INT5_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM0_INT5_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_INT5_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM0_INT6_STATUS */
#define CPUSS_V2_CM0_INT6_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_INT6_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM0_INT6_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_INT6_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM0_INT7_STATUS */
#define CPUSS_V2_CM0_INT7_STATUS_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_INT7_STATUS_SYSTEM_INT_IDX_Msk 0x3FFUL
#define CPUSS_V2_CM0_INT7_STATUS_SYSTEM_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_INT7_STATUS_SYSTEM_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM0_VECTOR_TABLE_BASE */
#define CPUSS_V2_CM0_VECTOR_TABLE_BASE_ADDR24_Pos 8UL
#define CPUSS_V2_CM0_VECTOR_TABLE_BASE_ADDR24_Msk 0xFFFFFF00UL
/* CPUSS.CM0_NMI_CTL */
#define CPUSS_V2_CM0_NMI_CTL_SYSTEM_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_NMI_CTL_SYSTEM_INT_IDX_Msk 0x3FFUL
/* CPUSS.CM4_PWR_CTL */
#define CPUSS_V2_CM4_PWR_CTL_PWR_MODE_Pos       0UL
#define CPUSS_V2_CM4_PWR_CTL_PWR_MODE_Msk       0x3UL
#define CPUSS_V2_CM4_PWR_CTL_VECTKEYSTAT_Pos    16UL
#define CPUSS_V2_CM4_PWR_CTL_VECTKEYSTAT_Msk    0xFFFF0000UL
/* CPUSS.CM4_PWR_DELAY_CTL */
#define CPUSS_V2_CM4_PWR_DELAY_CTL_UP_Pos       0UL
#define CPUSS_V2_CM4_PWR_DELAY_CTL_UP_Msk       0x3FFUL
/* CPUSS.RAM0_CTL0 */
#define CPUSS_V2_RAM0_CTL0_SLOW_WS_Pos          0UL
#define CPUSS_V2_RAM0_CTL0_SLOW_WS_Msk          0x3UL
#define CPUSS_V2_RAM0_CTL0_FAST_WS_Pos          8UL
#define CPUSS_V2_RAM0_CTL0_FAST_WS_Msk          0x300UL
#define CPUSS_V2_RAM0_CTL0_ECC_EN_Pos           16UL
#define CPUSS_V2_RAM0_CTL0_ECC_EN_Msk           0x10000UL
#define CPUSS_V2_RAM0_CTL0_ECC_AUTO_CORRECT_Pos 17UL
#define CPUSS_V2_RAM0_CTL0_ECC_AUTO_CORRECT_Msk 0x20000UL
#define CPUSS_V2_RAM0_CTL0_ECC_INJ_EN_Pos       18UL
#define CPUSS_V2_RAM0_CTL0_ECC_INJ_EN_Msk       0x40000UL
/* CPUSS.RAM0_STATUS */
#define CPUSS_V2_RAM0_STATUS_WB_EMPTY_Pos       0UL
#define CPUSS_V2_RAM0_STATUS_WB_EMPTY_Msk       0x1UL
/* CPUSS.RAM0_PWR_MACRO_CTL */
#define CPUSS_V2_RAM0_PWR_MACRO_CTL_PWR_MODE_Pos 0UL
#define CPUSS_V2_RAM0_PWR_MACRO_CTL_PWR_MODE_Msk 0x3UL
#define CPUSS_V2_RAM0_PWR_MACRO_CTL_VECTKEYSTAT_Pos 16UL
#define CPUSS_V2_RAM0_PWR_MACRO_CTL_VECTKEYSTAT_Msk 0xFFFF0000UL
/* CPUSS.RAM1_CTL0 */
#define CPUSS_V2_RAM1_CTL0_SLOW_WS_Pos          0UL
#define CPUSS_V2_RAM1_CTL0_SLOW_WS_Msk          0x3UL
#define CPUSS_V2_RAM1_CTL0_FAST_WS_Pos          8UL
#define CPUSS_V2_RAM1_CTL0_FAST_WS_Msk          0x300UL
#define CPUSS_V2_RAM1_CTL0_ECC_EN_Pos           16UL
#define CPUSS_V2_RAM1_CTL0_ECC_EN_Msk           0x10000UL
#define CPUSS_V2_RAM1_CTL0_ECC_AUTO_CORRECT_Pos 17UL
#define CPUSS_V2_RAM1_CTL0_ECC_AUTO_CORRECT_Msk 0x20000UL
#define CPUSS_V2_RAM1_CTL0_ECC_INJ_EN_Pos       18UL
#define CPUSS_V2_RAM1_CTL0_ECC_INJ_EN_Msk       0x40000UL
/* CPUSS.RAM1_STATUS */
#define CPUSS_V2_RAM1_STATUS_WB_EMPTY_Pos       0UL
#define CPUSS_V2_RAM1_STATUS_WB_EMPTY_Msk       0x1UL
/* CPUSS.RAM1_PWR_CTL */
#define CPUSS_V2_RAM1_PWR_CTL_PWR_MODE_Pos      0UL
#define CPUSS_V2_RAM1_PWR_CTL_PWR_MODE_Msk      0x3UL
#define CPUSS_V2_RAM1_PWR_CTL_VECTKEYSTAT_Pos   16UL
#define CPUSS_V2_RAM1_PWR_CTL_VECTKEYSTAT_Msk   0xFFFF0000UL
/* CPUSS.RAM2_CTL0 */
#define CPUSS_V2_RAM2_CTL0_SLOW_WS_Pos          0UL
#define CPUSS_V2_RAM2_CTL0_SLOW_WS_Msk          0x3UL
#define CPUSS_V2_RAM2_CTL0_FAST_WS_Pos          8UL
#define CPUSS_V2_RAM2_CTL0_FAST_WS_Msk          0x300UL
#define CPUSS_V2_RAM2_CTL0_ECC_EN_Pos           16UL
#define CPUSS_V2_RAM2_CTL0_ECC_EN_Msk           0x10000UL
#define CPUSS_V2_RAM2_CTL0_ECC_AUTO_CORRECT_Pos 17UL
#define CPUSS_V2_RAM2_CTL0_ECC_AUTO_CORRECT_Msk 0x20000UL
#define CPUSS_V2_RAM2_CTL0_ECC_INJ_EN_Pos       18UL
#define CPUSS_V2_RAM2_CTL0_ECC_INJ_EN_Msk       0x40000UL
/* CPUSS.RAM2_STATUS */
#define CPUSS_V2_RAM2_STATUS_WB_EMPTY_Pos       0UL
#define CPUSS_V2_RAM2_STATUS_WB_EMPTY_Msk       0x1UL
/* CPUSS.RAM2_PWR_CTL */
#define CPUSS_V2_RAM2_PWR_CTL_PWR_MODE_Pos      0UL
#define CPUSS_V2_RAM2_PWR_CTL_PWR_MODE_Msk      0x3UL
#define CPUSS_V2_RAM2_PWR_CTL_VECTKEYSTAT_Pos   16UL
#define CPUSS_V2_RAM2_PWR_CTL_VECTKEYSTAT_Msk   0xFFFF0000UL
/* CPUSS.RAM_PWR_DELAY_CTL */
#define CPUSS_V2_RAM_PWR_DELAY_CTL_UP_Pos       0UL
#define CPUSS_V2_RAM_PWR_DELAY_CTL_UP_Msk       0x3FFUL
/* CPUSS.ROM_CTL */
#define CPUSS_V2_ROM_CTL_SLOW_WS_Pos            0UL
#define CPUSS_V2_ROM_CTL_SLOW_WS_Msk            0x3UL
#define CPUSS_V2_ROM_CTL_FAST_WS_Pos            8UL
#define CPUSS_V2_ROM_CTL_FAST_WS_Msk            0x300UL
/* CPUSS.ECC_CTL */
#define CPUSS_V2_ECC_CTL_WORD_ADDR_Pos          0UL
#define CPUSS_V2_ECC_CTL_WORD_ADDR_Msk          0x1FFFFFFUL
#define CPUSS_V2_ECC_CTL_PARITY_Pos             25UL
#define CPUSS_V2_ECC_CTL_PARITY_Msk             0xFE000000UL
/* CPUSS.PRODUCT_ID */
#define CPUSS_V2_PRODUCT_ID_FAMILY_ID_Pos       0UL
#define CPUSS_V2_PRODUCT_ID_FAMILY_ID_Msk       0xFFFUL
#define CPUSS_V2_PRODUCT_ID_MAJOR_REV_Pos       16UL
#define CPUSS_V2_PRODUCT_ID_MAJOR_REV_Msk       0xF0000UL
#define CPUSS_V2_PRODUCT_ID_MINOR_REV_Pos       20UL
#define CPUSS_V2_PRODUCT_ID_MINOR_REV_Msk       0xF00000UL
/* CPUSS.DP_STATUS */
#define CPUSS_V2_DP_STATUS_SWJ_CONNECTED_Pos    0UL
#define CPUSS_V2_DP_STATUS_SWJ_CONNECTED_Msk    0x1UL
#define CPUSS_V2_DP_STATUS_SWJ_DEBUG_EN_Pos     1UL
#define CPUSS_V2_DP_STATUS_SWJ_DEBUG_EN_Msk     0x2UL
#define CPUSS_V2_DP_STATUS_SWJ_JTAG_SEL_Pos     2UL
#define CPUSS_V2_DP_STATUS_SWJ_JTAG_SEL_Msk     0x4UL
/* CPUSS.BUFF_CTL */
#define CPUSS_V2_BUFF_CTL_WRITE_BUFF_Pos        0UL
#define CPUSS_V2_BUFF_CTL_WRITE_BUFF_Msk        0x1UL
/* CPUSS.SYSTICK_CTL */
#define CPUSS_V2_SYSTICK_CTL_TENMS_Pos          0UL
#define CPUSS_V2_SYSTICK_CTL_TENMS_Msk          0xFFFFFFUL
#define CPUSS_V2_SYSTICK_CTL_CLOCK_SOURCE_Pos   24UL
#define CPUSS_V2_SYSTICK_CTL_CLOCK_SOURCE_Msk   0x3000000UL
#define CPUSS_V2_SYSTICK_CTL_SKEW_Pos           30UL
#define CPUSS_V2_SYSTICK_CTL_SKEW_Msk           0x40000000UL
#define CPUSS_V2_SYSTICK_CTL_NOREF_Pos          31UL
#define CPUSS_V2_SYSTICK_CTL_NOREF_Msk          0x80000000UL
/* CPUSS.MBIST_STAT */
#define CPUSS_V2_MBIST_STAT_SFP_READY_Pos       0UL
#define CPUSS_V2_MBIST_STAT_SFP_READY_Msk       0x1UL
#define CPUSS_V2_MBIST_STAT_SFP_FAIL_Pos        1UL
#define CPUSS_V2_MBIST_STAT_SFP_FAIL_Msk        0x2UL
/* CPUSS.CAL_SUP_SET */
#define CPUSS_V2_CAL_SUP_SET_DATA_Pos           0UL
#define CPUSS_V2_CAL_SUP_SET_DATA_Msk           0xFFFFFFFFUL
/* CPUSS.CAL_SUP_CLR */
#define CPUSS_V2_CAL_SUP_CLR_DATA_Pos           0UL
#define CPUSS_V2_CAL_SUP_CLR_DATA_Msk           0xFFFFFFFFUL
/* CPUSS.CM0_PC_CTL */
#define CPUSS_V2_CM0_PC_CTL_VALID_Pos           0UL
#define CPUSS_V2_CM0_PC_CTL_VALID_Msk           0xFUL
/* CPUSS.CM0_PC0_HANDLER */
#define CPUSS_V2_CM0_PC0_HANDLER_ADDR_Pos       0UL
#define CPUSS_V2_CM0_PC0_HANDLER_ADDR_Msk       0xFFFFFFFFUL
/* CPUSS.CM0_PC1_HANDLER */
#define CPUSS_V2_CM0_PC1_HANDLER_ADDR_Pos       0UL
#define CPUSS_V2_CM0_PC1_HANDLER_ADDR_Msk       0xFFFFFFFFUL
/* CPUSS.CM0_PC2_HANDLER */
#define CPUSS_V2_CM0_PC2_HANDLER_ADDR_Pos       0UL
#define CPUSS_V2_CM0_PC2_HANDLER_ADDR_Msk       0xFFFFFFFFUL
/* CPUSS.CM0_PC3_HANDLER */
#define CPUSS_V2_CM0_PC3_HANDLER_ADDR_Pos       0UL
#define CPUSS_V2_CM0_PC3_HANDLER_ADDR_Msk       0xFFFFFFFFUL
/* CPUSS.PROTECTION */
#define CPUSS_V2_PROTECTION_STATE_Pos           0UL
#define CPUSS_V2_PROTECTION_STATE_Msk           0x7UL
/* CPUSS.TRIM_ROM_CTL */
#define CPUSS_V2_TRIM_ROM_CTL_TRIM_Pos          0UL
#define CPUSS_V2_TRIM_ROM_CTL_TRIM_Msk          0xFFFFFFFFUL
/* CPUSS.TRIM_RAM_CTL */
#define CPUSS_V2_TRIM_RAM_CTL_TRIM_Pos          0UL
#define CPUSS_V2_TRIM_RAM_CTL_TRIM_Msk          0xFFFFFFFFUL
/* CPUSS.CM0_SYSTEM_INT_CTL */
#define CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_IDX_Pos 0UL
#define CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_IDX_Msk 0x7UL
#define CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID_Pos 31UL
#define CPUSS_V2_CM0_SYSTEM_INT_CTL_CPU_INT_VALID_Msk 0x80000000UL
/* CPUSS.CM4_SYSTEM_INT_CTL */
#define CPUSS_V2_CM4_SYSTEM_INT_CTL_CPU_INT_IDX_Pos 0UL
#define CPUSS_V2_CM4_SYSTEM_INT_CTL_CPU_INT_IDX_Msk 0x7UL
#define CPUSS_V2_CM4_SYSTEM_INT_CTL_CPU_INT_VALID_Pos 31UL
#define CPUSS_V2_CM4_SYSTEM_INT_CTL_CPU_INT_VALID_Msk 0x80000000UL


#endif /* _CYIP_CPUSS_V2_H_ */


/* [] END OF FILE */
