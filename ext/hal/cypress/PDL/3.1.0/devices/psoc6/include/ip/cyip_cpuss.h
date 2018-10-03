/***************************************************************************//**
* \file cyip_cpuss.h
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

#ifndef _CYIP_CPUSS_H_
#define _CYIP_CPUSS_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                    CPUSS
*******************************************************************************/

#define CPUSS_SECTION_SIZE                      0x00010000UL

/**
  * \brief CPU subsystem (CPUSS) (CPUSS)
  */
typedef struct {
  __IOM uint32_t CM0_CTL;                       /*!< 0x00000000 CM0+ control */
   __IM uint32_t RESERVED;
   __IM uint32_t CM0_STATUS;                    /*!< 0x00000008 CM0+ status */
   __IM uint32_t RESERVED1;
  __IOM uint32_t CM0_CLOCK_CTL;                 /*!< 0x00000010 CM0+ clock control */
   __IM uint32_t RESERVED2[3];
  __IOM uint32_t CM0_INT_CTL0;                  /*!< 0x00000020 CM0+ interrupt control 0 */
  __IOM uint32_t CM0_INT_CTL1;                  /*!< 0x00000024 CM0+ interrupt control 1 */
  __IOM uint32_t CM0_INT_CTL2;                  /*!< 0x00000028 CM0+ interrupt control 2 */
  __IOM uint32_t CM0_INT_CTL3;                  /*!< 0x0000002C CM0+ interrupt control 3 */
  __IOM uint32_t CM0_INT_CTL4;                  /*!< 0x00000030 CM0+ interrupt control 4 */
  __IOM uint32_t CM0_INT_CTL5;                  /*!< 0x00000034 CM0+ interrupt control 5 */
  __IOM uint32_t CM0_INT_CTL6;                  /*!< 0x00000038 CM0+ interrupt control 6 */
  __IOM uint32_t CM0_INT_CTL7;                  /*!< 0x0000003C CM0+ interrupt control 7 */
   __IM uint32_t RESERVED3[16];
  __IOM uint32_t CM4_PWR_CTL;                   /*!< 0x00000080 CM4 power control */
  __IOM uint32_t CM4_PWR_DELAY_CTL;             /*!< 0x00000084 CM4 power control */
   __IM uint32_t CM4_STATUS;                    /*!< 0x00000088 CM4 status */
   __IM uint32_t RESERVED4;
  __IOM uint32_t CM4_CLOCK_CTL;                 /*!< 0x00000090 CM4 clock control */
   __IM uint32_t RESERVED5[3];
  __IOM uint32_t CM4_NMI_CTL;                   /*!< 0x000000A0 CM4 NMI control */
   __IM uint32_t RESERVED6[23];
  __IOM uint32_t RAM0_CTL0;                     /*!< 0x00000100 RAM 0 control 0 */
   __IM uint32_t RESERVED7[15];
  __IOM uint32_t RAM0_PWR_MACRO_CTL[16];        /*!< 0x00000140 RAM 0 power control */
  __IOM uint32_t RAM1_CTL0;                     /*!< 0x00000180 RAM 1 control 0 */
   __IM uint32_t RESERVED8[3];
  __IOM uint32_t RAM1_PWR_CTL;                  /*!< 0x00000190 RAM1 power control */
   __IM uint32_t RESERVED9[3];
  __IOM uint32_t RAM2_CTL0;                     /*!< 0x000001A0 RAM 2 control 0 */
   __IM uint32_t RESERVED10[3];
  __IOM uint32_t RAM2_PWR_CTL;                  /*!< 0x000001B0 RAM2 power control */
   __IM uint32_t RESERVED11[3];
  __IOM uint32_t RAM_PWR_DELAY_CTL;             /*!< 0x000001C0 Power up delay used for all SRAM power domains */
   __IM uint32_t RESERVED12[3];
  __IOM uint32_t ROM_CTL;                       /*!< 0x000001D0 ROM control */
   __IM uint32_t RESERVED13[7];
  __IOM uint32_t UDB_PWR_CTL;                   /*!< 0x000001F0 UDB power control */
  __IOM uint32_t UDB_PWR_DELAY_CTL;             /*!< 0x000001F4 UDB power control */
   __IM uint32_t RESERVED14[4];
   __IM uint32_t DP_STATUS;                     /*!< 0x00000208 Debug port status */
   __IM uint32_t RESERVED15[5];
  __IOM uint32_t BUFF_CTL;                      /*!< 0x00000220 Buffer control */
   __IM uint32_t RESERVED16[3];
  __IOM uint32_t DDFT_CTL;                      /*!< 0x00000230 DDFT control */
   __IM uint32_t RESERVED17[3];
  __IOM uint32_t SYSTICK_CTL;                   /*!< 0x00000240 SysTick timer control */
   __IM uint32_t RESERVED18[27];
  __IOM uint32_t CM0_VECTOR_TABLE_BASE;         /*!< 0x000002B0 CM0+ vector table base */
   __IM uint32_t RESERVED19[3];
  __IOM uint32_t CM4_VECTOR_TABLE_BASE;         /*!< 0x000002C0 CM4 vector table base */
   __IM uint32_t RESERVED20[23];
  __IOM uint32_t CM0_PC0_HANDLER;               /*!< 0x00000320 CM0+ protection context 0 handler */
   __IM uint32_t RESERVED21[55];
   __IM uint32_t IDENTITY;                      /*!< 0x00000400 Identity */
   __IM uint32_t RESERVED22[63];
  __IOM uint32_t PROTECTION;                    /*!< 0x00000500 Protection status */
   __IM uint32_t RESERVED23[7];
  __IOM uint32_t CM0_NMI_CTL;                   /*!< 0x00000520 CM0+ NMI control */
   __IM uint32_t RESERVED24[31];
   __IM uint32_t MBIST_STAT;                    /*!< 0x000005A0 Memory BIST status */
   __IM uint32_t RESERVED25[14999];
  __IOM uint32_t TRIM_ROM_CTL;                  /*!< 0x0000F000 ROM trim control */
  __IOM uint32_t TRIM_RAM_CTL;                  /*!< 0x0000F004 RAM trim control */
} CPUSS_V1_Type;                                /*!< Size = 61448 (0xF008) */


/* CPUSS.CM0_CTL */
#define CPUSS_CM0_CTL_SLV_STALL_Pos             0UL
#define CPUSS_CM0_CTL_SLV_STALL_Msk             0x1UL
#define CPUSS_CM0_CTL_ENABLED_Pos               1UL
#define CPUSS_CM0_CTL_ENABLED_Msk               0x2UL
#define CPUSS_CM0_CTL_VECTKEYSTAT_Pos           16UL
#define CPUSS_CM0_CTL_VECTKEYSTAT_Msk           0xFFFF0000UL
/* CPUSS.CM0_STATUS */
#define CPUSS_CM0_STATUS_SLEEPING_Pos           0UL
#define CPUSS_CM0_STATUS_SLEEPING_Msk           0x1UL
#define CPUSS_CM0_STATUS_SLEEPDEEP_Pos          1UL
#define CPUSS_CM0_STATUS_SLEEPDEEP_Msk          0x2UL
/* CPUSS.CM0_CLOCK_CTL */
#define CPUSS_CM0_CLOCK_CTL_SLOW_INT_DIV_Pos    8UL
#define CPUSS_CM0_CLOCK_CTL_SLOW_INT_DIV_Msk    0xFF00UL
#define CPUSS_CM0_CLOCK_CTL_PERI_INT_DIV_Pos    24UL
#define CPUSS_CM0_CLOCK_CTL_PERI_INT_DIV_Msk    0xFF000000UL
/* CPUSS.CM0_INT_CTL0 */
#define CPUSS_CM0_INT_CTL0_MUX0_SEL_Pos         0UL
#define CPUSS_CM0_INT_CTL0_MUX0_SEL_Msk         0xFFUL
#define CPUSS_CM0_INT_CTL0_MUX1_SEL_Pos         8UL
#define CPUSS_CM0_INT_CTL0_MUX1_SEL_Msk         0xFF00UL
#define CPUSS_CM0_INT_CTL0_MUX2_SEL_Pos         16UL
#define CPUSS_CM0_INT_CTL0_MUX2_SEL_Msk         0xFF0000UL
#define CPUSS_CM0_INT_CTL0_MUX3_SEL_Pos         24UL
#define CPUSS_CM0_INT_CTL0_MUX3_SEL_Msk         0xFF000000UL
/* CPUSS.CM0_INT_CTL1 */
#define CPUSS_CM0_INT_CTL1_MUX0_SEL_Pos         0UL
#define CPUSS_CM0_INT_CTL1_MUX0_SEL_Msk         0xFFUL
#define CPUSS_CM0_INT_CTL1_MUX1_SEL_Pos         8UL
#define CPUSS_CM0_INT_CTL1_MUX1_SEL_Msk         0xFF00UL
#define CPUSS_CM0_INT_CTL1_MUX2_SEL_Pos         16UL
#define CPUSS_CM0_INT_CTL1_MUX2_SEL_Msk         0xFF0000UL
#define CPUSS_CM0_INT_CTL1_MUX3_SEL_Pos         24UL
#define CPUSS_CM0_INT_CTL1_MUX3_SEL_Msk         0xFF000000UL
/* CPUSS.CM0_INT_CTL2 */
#define CPUSS_CM0_INT_CTL2_MUX0_SEL_Pos         0UL
#define CPUSS_CM0_INT_CTL2_MUX0_SEL_Msk         0xFFUL
#define CPUSS_CM0_INT_CTL2_MUX1_SEL_Pos         8UL
#define CPUSS_CM0_INT_CTL2_MUX1_SEL_Msk         0xFF00UL
#define CPUSS_CM0_INT_CTL2_MUX2_SEL_Pos         16UL
#define CPUSS_CM0_INT_CTL2_MUX2_SEL_Msk         0xFF0000UL
#define CPUSS_CM0_INT_CTL2_MUX3_SEL_Pos         24UL
#define CPUSS_CM0_INT_CTL2_MUX3_SEL_Msk         0xFF000000UL
/* CPUSS.CM0_INT_CTL3 */
#define CPUSS_CM0_INT_CTL3_MUX0_SEL_Pos         0UL
#define CPUSS_CM0_INT_CTL3_MUX0_SEL_Msk         0xFFUL
#define CPUSS_CM0_INT_CTL3_MUX1_SEL_Pos         8UL
#define CPUSS_CM0_INT_CTL3_MUX1_SEL_Msk         0xFF00UL
#define CPUSS_CM0_INT_CTL3_MUX2_SEL_Pos         16UL
#define CPUSS_CM0_INT_CTL3_MUX2_SEL_Msk         0xFF0000UL
#define CPUSS_CM0_INT_CTL3_MUX3_SEL_Pos         24UL
#define CPUSS_CM0_INT_CTL3_MUX3_SEL_Msk         0xFF000000UL
/* CPUSS.CM0_INT_CTL4 */
#define CPUSS_CM0_INT_CTL4_MUX0_SEL_Pos         0UL
#define CPUSS_CM0_INT_CTL4_MUX0_SEL_Msk         0xFFUL
#define CPUSS_CM0_INT_CTL4_MUX1_SEL_Pos         8UL
#define CPUSS_CM0_INT_CTL4_MUX1_SEL_Msk         0xFF00UL
#define CPUSS_CM0_INT_CTL4_MUX2_SEL_Pos         16UL
#define CPUSS_CM0_INT_CTL4_MUX2_SEL_Msk         0xFF0000UL
#define CPUSS_CM0_INT_CTL4_MUX3_SEL_Pos         24UL
#define CPUSS_CM0_INT_CTL4_MUX3_SEL_Msk         0xFF000000UL
/* CPUSS.CM0_INT_CTL5 */
#define CPUSS_CM0_INT_CTL5_MUX0_SEL_Pos         0UL
#define CPUSS_CM0_INT_CTL5_MUX0_SEL_Msk         0xFFUL
#define CPUSS_CM0_INT_CTL5_MUX1_SEL_Pos         8UL
#define CPUSS_CM0_INT_CTL5_MUX1_SEL_Msk         0xFF00UL
#define CPUSS_CM0_INT_CTL5_MUX2_SEL_Pos         16UL
#define CPUSS_CM0_INT_CTL5_MUX2_SEL_Msk         0xFF0000UL
#define CPUSS_CM0_INT_CTL5_MUX3_SEL_Pos         24UL
#define CPUSS_CM0_INT_CTL5_MUX3_SEL_Msk         0xFF000000UL
/* CPUSS.CM0_INT_CTL6 */
#define CPUSS_CM0_INT_CTL6_MUX0_SEL_Pos         0UL
#define CPUSS_CM0_INT_CTL6_MUX0_SEL_Msk         0xFFUL
#define CPUSS_CM0_INT_CTL6_MUX1_SEL_Pos         8UL
#define CPUSS_CM0_INT_CTL6_MUX1_SEL_Msk         0xFF00UL
#define CPUSS_CM0_INT_CTL6_MUX2_SEL_Pos         16UL
#define CPUSS_CM0_INT_CTL6_MUX2_SEL_Msk         0xFF0000UL
#define CPUSS_CM0_INT_CTL6_MUX3_SEL_Pos         24UL
#define CPUSS_CM0_INT_CTL6_MUX3_SEL_Msk         0xFF000000UL
/* CPUSS.CM0_INT_CTL7 */
#define CPUSS_CM0_INT_CTL7_MUX0_SEL_Pos         0UL
#define CPUSS_CM0_INT_CTL7_MUX0_SEL_Msk         0xFFUL
#define CPUSS_CM0_INT_CTL7_MUX1_SEL_Pos         8UL
#define CPUSS_CM0_INT_CTL7_MUX1_SEL_Msk         0xFF00UL
#define CPUSS_CM0_INT_CTL7_MUX2_SEL_Pos         16UL
#define CPUSS_CM0_INT_CTL7_MUX2_SEL_Msk         0xFF0000UL
#define CPUSS_CM0_INT_CTL7_MUX3_SEL_Pos         24UL
#define CPUSS_CM0_INT_CTL7_MUX3_SEL_Msk         0xFF000000UL
/* CPUSS.CM4_PWR_CTL */
#define CPUSS_CM4_PWR_CTL_PWR_MODE_Pos          0UL
#define CPUSS_CM4_PWR_CTL_PWR_MODE_Msk          0x3UL
#define CPUSS_CM4_PWR_CTL_VECTKEYSTAT_Pos       16UL
#define CPUSS_CM4_PWR_CTL_VECTKEYSTAT_Msk       0xFFFF0000UL
/* CPUSS.CM4_PWR_DELAY_CTL */
#define CPUSS_CM4_PWR_DELAY_CTL_UP_Pos          0UL
#define CPUSS_CM4_PWR_DELAY_CTL_UP_Msk          0x3FFUL
/* CPUSS.CM4_STATUS */
#define CPUSS_CM4_STATUS_SLEEPING_Pos           0UL
#define CPUSS_CM4_STATUS_SLEEPING_Msk           0x1UL
#define CPUSS_CM4_STATUS_SLEEPDEEP_Pos          1UL
#define CPUSS_CM4_STATUS_SLEEPDEEP_Msk          0x2UL
#define CPUSS_CM4_STATUS_PWR_DONE_Pos           4UL
#define CPUSS_CM4_STATUS_PWR_DONE_Msk           0x10UL
/* CPUSS.CM4_CLOCK_CTL */
#define CPUSS_CM4_CLOCK_CTL_FAST_INT_DIV_Pos    8UL
#define CPUSS_CM4_CLOCK_CTL_FAST_INT_DIV_Msk    0xFF00UL
/* CPUSS.CM4_NMI_CTL */
#define CPUSS_CM4_NMI_CTL_MUX0_SEL_Pos          0UL
#define CPUSS_CM4_NMI_CTL_MUX0_SEL_Msk          0xFFUL
/* CPUSS.RAM0_CTL0 */
#define CPUSS_RAM0_CTL0_SLOW_WS_Pos             0UL
#define CPUSS_RAM0_CTL0_SLOW_WS_Msk             0x3UL
#define CPUSS_RAM0_CTL0_FAST_WS_Pos             8UL
#define CPUSS_RAM0_CTL0_FAST_WS_Msk             0x300UL
/* CPUSS.RAM0_PWR_MACRO_CTL */
#define CPUSS_RAM0_PWR_MACRO_CTL_PWR_MODE_Pos   0UL
#define CPUSS_RAM0_PWR_MACRO_CTL_PWR_MODE_Msk   0x3UL
#define CPUSS_RAM0_PWR_MACRO_CTL_VECTKEYSTAT_Pos 16UL
#define CPUSS_RAM0_PWR_MACRO_CTL_VECTKEYSTAT_Msk 0xFFFF0000UL
/* CPUSS.RAM1_CTL0 */
#define CPUSS_RAM1_CTL0_SLOW_WS_Pos             0UL
#define CPUSS_RAM1_CTL0_SLOW_WS_Msk             0x3UL
#define CPUSS_RAM1_CTL0_FAST_WS_Pos             8UL
#define CPUSS_RAM1_CTL0_FAST_WS_Msk             0x300UL
/* CPUSS.RAM1_PWR_CTL */
#define CPUSS_RAM1_PWR_CTL_PWR_MODE_Pos         0UL
#define CPUSS_RAM1_PWR_CTL_PWR_MODE_Msk         0x3UL
#define CPUSS_RAM1_PWR_CTL_VECTKEYSTAT_Pos      16UL
#define CPUSS_RAM1_PWR_CTL_VECTKEYSTAT_Msk      0xFFFF0000UL
/* CPUSS.RAM2_CTL0 */
#define CPUSS_RAM2_CTL0_SLOW_WS_Pos             0UL
#define CPUSS_RAM2_CTL0_SLOW_WS_Msk             0x3UL
#define CPUSS_RAM2_CTL0_FAST_WS_Pos             8UL
#define CPUSS_RAM2_CTL0_FAST_WS_Msk             0x300UL
/* CPUSS.RAM2_PWR_CTL */
#define CPUSS_RAM2_PWR_CTL_PWR_MODE_Pos         0UL
#define CPUSS_RAM2_PWR_CTL_PWR_MODE_Msk         0x3UL
#define CPUSS_RAM2_PWR_CTL_VECTKEYSTAT_Pos      16UL
#define CPUSS_RAM2_PWR_CTL_VECTKEYSTAT_Msk      0xFFFF0000UL
/* CPUSS.RAM_PWR_DELAY_CTL */
#define CPUSS_RAM_PWR_DELAY_CTL_UP_Pos          0UL
#define CPUSS_RAM_PWR_DELAY_CTL_UP_Msk          0x3FFUL
/* CPUSS.ROM_CTL */
#define CPUSS_ROM_CTL_SLOW_WS_Pos               0UL
#define CPUSS_ROM_CTL_SLOW_WS_Msk               0x3UL
#define CPUSS_ROM_CTL_FAST_WS_Pos               8UL
#define CPUSS_ROM_CTL_FAST_WS_Msk               0x300UL
/* CPUSS.UDB_PWR_CTL */
#define CPUSS_UDB_PWR_CTL_PWR_MODE_Pos          0UL
#define CPUSS_UDB_PWR_CTL_PWR_MODE_Msk          0x3UL
#define CPUSS_UDB_PWR_CTL_VECTKEYSTAT_Pos       16UL
#define CPUSS_UDB_PWR_CTL_VECTKEYSTAT_Msk       0xFFFF0000UL
/* CPUSS.UDB_PWR_DELAY_CTL */
#define CPUSS_UDB_PWR_DELAY_CTL_UP_Pos          0UL
#define CPUSS_UDB_PWR_DELAY_CTL_UP_Msk          0x3FFUL
/* CPUSS.DP_STATUS */
#define CPUSS_DP_STATUS_SWJ_CONNECTED_Pos       0UL
#define CPUSS_DP_STATUS_SWJ_CONNECTED_Msk       0x1UL
#define CPUSS_DP_STATUS_SWJ_DEBUG_EN_Pos        1UL
#define CPUSS_DP_STATUS_SWJ_DEBUG_EN_Msk        0x2UL
#define CPUSS_DP_STATUS_SWJ_JTAG_SEL_Pos        2UL
#define CPUSS_DP_STATUS_SWJ_JTAG_SEL_Msk        0x4UL
/* CPUSS.BUFF_CTL */
#define CPUSS_BUFF_CTL_WRITE_BUFF_Pos           0UL
#define CPUSS_BUFF_CTL_WRITE_BUFF_Msk           0x1UL
/* CPUSS.DDFT_CTL */
#define CPUSS_DDFT_CTL_DDFT_OUT0_SEL_Pos        0UL
#define CPUSS_DDFT_CTL_DDFT_OUT0_SEL_Msk        0x1FUL
#define CPUSS_DDFT_CTL_DDFT_OUT1_SEL_Pos        8UL
#define CPUSS_DDFT_CTL_DDFT_OUT1_SEL_Msk        0x1F00UL
/* CPUSS.SYSTICK_CTL */
#define CPUSS_SYSTICK_CTL_TENMS_Pos             0UL
#define CPUSS_SYSTICK_CTL_TENMS_Msk             0xFFFFFFUL
#define CPUSS_SYSTICK_CTL_CLOCK_SOURCE_Pos      24UL
#define CPUSS_SYSTICK_CTL_CLOCK_SOURCE_Msk      0x3000000UL
#define CPUSS_SYSTICK_CTL_SKEW_Pos              30UL
#define CPUSS_SYSTICK_CTL_SKEW_Msk              0x40000000UL
#define CPUSS_SYSTICK_CTL_NOREF_Pos             31UL
#define CPUSS_SYSTICK_CTL_NOREF_Msk             0x80000000UL
/* CPUSS.CM0_VECTOR_TABLE_BASE */
#define CPUSS_CM0_VECTOR_TABLE_BASE_ADDR24_Pos  8UL
#define CPUSS_CM0_VECTOR_TABLE_BASE_ADDR24_Msk  0xFFFFFF00UL
/* CPUSS.CM4_VECTOR_TABLE_BASE */
#define CPUSS_CM4_VECTOR_TABLE_BASE_ADDR22_Pos  10UL
#define CPUSS_CM4_VECTOR_TABLE_BASE_ADDR22_Msk  0xFFFFFC00UL
/* CPUSS.CM0_PC0_HANDLER */
#define CPUSS_CM0_PC0_HANDLER_ADDR_Pos          0UL
#define CPUSS_CM0_PC0_HANDLER_ADDR_Msk          0xFFFFFFFFUL
/* CPUSS.IDENTITY */
#define CPUSS_IDENTITY_P_Pos                    0UL
#define CPUSS_IDENTITY_P_Msk                    0x1UL
#define CPUSS_IDENTITY_NS_Pos                   1UL
#define CPUSS_IDENTITY_NS_Msk                   0x2UL
#define CPUSS_IDENTITY_PC_Pos                   4UL
#define CPUSS_IDENTITY_PC_Msk                   0xF0UL
#define CPUSS_IDENTITY_MS_Pos                   8UL
#define CPUSS_IDENTITY_MS_Msk                   0xF00UL
/* CPUSS.PROTECTION */
#define CPUSS_PROTECTION_STATE_Pos              0UL
#define CPUSS_PROTECTION_STATE_Msk              0x7UL
/* CPUSS.CM0_NMI_CTL */
#define CPUSS_CM0_NMI_CTL_MUX0_SEL_Pos          0UL
#define CPUSS_CM0_NMI_CTL_MUX0_SEL_Msk          0xFFUL
/* CPUSS.MBIST_STAT */
#define CPUSS_MBIST_STAT_SFP_READY_Pos          0UL
#define CPUSS_MBIST_STAT_SFP_READY_Msk          0x1UL
#define CPUSS_MBIST_STAT_SFP_FAIL_Pos           1UL
#define CPUSS_MBIST_STAT_SFP_FAIL_Msk           0x2UL
/* CPUSS.TRIM_ROM_CTL */
#define CPUSS_TRIM_ROM_CTL_RM_Pos               0UL
#define CPUSS_TRIM_ROM_CTL_RM_Msk               0xFUL
#define CPUSS_TRIM_ROM_CTL_RME_Pos              4UL
#define CPUSS_TRIM_ROM_CTL_RME_Msk              0x10UL
/* CPUSS.TRIM_RAM_CTL */
#define CPUSS_TRIM_RAM_CTL_RM_Pos               0UL
#define CPUSS_TRIM_RAM_CTL_RM_Msk               0xFUL
#define CPUSS_TRIM_RAM_CTL_RME_Pos              4UL
#define CPUSS_TRIM_RAM_CTL_RME_Msk              0x10UL
#define CPUSS_TRIM_RAM_CTL_WPULSE_Pos           5UL
#define CPUSS_TRIM_RAM_CTL_WPULSE_Msk           0xE0UL
#define CPUSS_TRIM_RAM_CTL_RA_Pos               8UL
#define CPUSS_TRIM_RAM_CTL_RA_Msk               0x300UL
#define CPUSS_TRIM_RAM_CTL_WA_Pos               12UL
#define CPUSS_TRIM_RAM_CTL_WA_Msk               0x7000UL


#endif /* _CYIP_CPUSS_H_ */


/* [] END OF FILE */
