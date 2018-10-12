/***************************************************************************//**
* \file cyip_flashc.h
*
* \brief
* FLASHC IP definitions
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

#ifndef _CYIP_FLASHC_H_
#define _CYIP_FLASHC_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                    FLASHC
*******************************************************************************/

#define FLASHC_FM_CTL_SECTION_SIZE              0x00001000UL
#define FLASHC_SECTION_SIZE                     0x00010000UL

/**
  * \brief Flash Macro Registers (FLASHC_FM_CTL)
  */
typedef struct {
  __IOM uint32_t FM_CTL;                        /*!< 0x00000000 Flash macro control */
   __IM uint32_t STATUS;                        /*!< 0x00000004 Status */
  __IOM uint32_t FM_ADDR;                       /*!< 0x00000008 Flash macro  address */
   __IM uint32_t GEOMETRY;                      /*!< 0x0000000C Regular flash geometry */
   __IM uint32_t GEOMETRY_SUPERVISORY;          /*!< 0x00000010 Supervisory flash geometry */
  __IOM uint32_t TIMER_CTL;                     /*!< 0x00000014 Timer control */
  __IOM uint32_t ANA_CTL0;                      /*!< 0x00000018 Analog control 0 */
  __IOM uint32_t ANA_CTL1;                      /*!< 0x0000001C Analog control 1 */
   __IM uint32_t GEOMETRY_GEN;                  /*!< 0x00000020 N/A, DNU */
  __IOM uint32_t TEST_CTL;                      /*!< 0x00000024 Test mode control */
  __IOM uint32_t WAIT_CTL;                      /*!< 0x00000028 Wiat State control */
   __IM uint32_t MONITOR_STATUS;                /*!< 0x0000002C Monitor Status */
  __IOM uint32_t SCRATCH_CTL;                   /*!< 0x00000030 Scratch Control */
  __IOM uint32_t HV_CTL;                        /*!< 0x00000034 High voltage control */
   __OM uint32_t ACLK_CTL;                      /*!< 0x00000038 Aclk control */
  __IOM uint32_t INTR;                          /*!< 0x0000003C Interrupt */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000040 Interrupt set */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000044 Interrupt mask */
   __IM uint32_t INTR_MASKED;                   /*!< 0x00000048 Interrupt masked */
   __OM uint32_t FM_HV_DATA_ALL;                /*!< 0x0000004C Flash macro high Voltage page latches data (for all page
                                                                latches) */
  __IOM uint32_t CAL_CTL0;                      /*!< 0x00000050 Cal control BG LO trim bits */
  __IOM uint32_t CAL_CTL1;                      /*!< 0x00000054 Cal control  BG HI trim bits */
  __IOM uint32_t CAL_CTL2;                      /*!< 0x00000058 Cal control BG LO&HI ipref trim, ref sel, fm_active, turbo_ext */
  __IOM uint32_t CAL_CTL3;                      /*!< 0x0000005C Cal control osc trim bits, idac, sdac, itim, bdac. */
   __OM uint32_t BOOKMARK;                      /*!< 0x00000060 Bookmark register - keeps the current FW HV seq */
   __IM uint32_t RESERVED[7];
  __IOM uint32_t RED_CTL01;                     /*!< 0x00000080 Redundancy Control normal sectors 0,1 */
  __IOM uint32_t RED_CTL23;                     /*!< 0x00000084 Redundancy Controll normal sectors 2,3 */
  __IOM uint32_t RED_CTL45;                     /*!< 0x00000088 Redundancy Controll normal sectors 4,5 */
  __IOM uint32_t RED_CTL67;                     /*!< 0x0000008C Redundancy Controll normal sectors 6,7 */
  __IOM uint32_t RED_CTL_SM01;                  /*!< 0x00000090 Redundancy Controll special sectors 0,1 */
   __IM uint32_t RESERVED1[27];
   __IM uint32_t TM_CMPR[32];                   /*!< 0x00000100 Do Not Use */
   __IM uint32_t RESERVED2[416];
  __IOM uint32_t FM_HV_DATA[256];               /*!< 0x00000800 Flash macro high Voltage page latches data */
   __IM uint32_t FM_MEM_DATA[256];              /*!< 0x00000C00 Flash macro memory sense amplifier and column decoder data */
} FLASHC_FM_CTL_V1_Type;                        /*!< Size = 4096 (0x1000) */

/**
  * \brief Flash controller (FLASHC)
  */
typedef struct {
  __IOM uint32_t FLASH_CTL;                     /*!< 0x00000000 Control */
  __IOM uint32_t FLASH_PWR_CTL;                 /*!< 0x00000004 Flash power control */
  __IOM uint32_t FLASH_CMD;                     /*!< 0x00000008 Command */
   __IM uint32_t RESERVED[61];
  __IOM uint32_t BIST_CTL;                      /*!< 0x00000100 BIST control */
  __IOM uint32_t BIST_CMD;                      /*!< 0x00000104 BIST command */
  __IOM uint32_t BIST_ADDR_START;               /*!< 0x00000108 BIST address start register */
  __IOM uint32_t BIST_DATA[8];                  /*!< 0x0000010C BIST data register(s) */
   __IM uint32_t BIST_DATA_ACT[8];              /*!< 0x0000012C BIST data actual register(s) */
   __IM uint32_t BIST_DATA_EXP[8];              /*!< 0x0000014C BIST data expected register(s) */
   __IM uint32_t BIST_ADDR;                     /*!< 0x0000016C BIST address register */
  __IOM uint32_t BIST_STATUS;                   /*!< 0x00000170 BIST status register */
   __IM uint32_t RESERVED1[163];
  __IOM uint32_t CM0_CA_CTL0;                   /*!< 0x00000400 CM0+ cache control */
  __IOM uint32_t CM0_CA_CTL1;                   /*!< 0x00000404 CM0+ cache control */
  __IOM uint32_t CM0_CA_CTL2;                   /*!< 0x00000408 CM0+ cache control */
  __IOM uint32_t CM0_CA_CMD;                    /*!< 0x0000040C CM0+ cache command */
   __IM uint32_t RESERVED2[12];
   __IM uint32_t CM0_CA_STATUS0;                /*!< 0x00000440 CM0+ cache status 0 */
   __IM uint32_t CM0_CA_STATUS1;                /*!< 0x00000444 CM0+ cache status 1 */
   __IM uint32_t CM0_CA_STATUS2;                /*!< 0x00000448 CM0+ cache status 2 */
   __IM uint32_t RESERVED3[13];
  __IOM uint32_t CM4_CA_CTL0;                   /*!< 0x00000480 CM4 cache control */
  __IOM uint32_t CM4_CA_CTL1;                   /*!< 0x00000484 CM4 cache control */
  __IOM uint32_t CM4_CA_CTL2;                   /*!< 0x00000488 CM4 cache control */
  __IOM uint32_t CM4_CA_CMD;                    /*!< 0x0000048C CM4 cache command */
   __IM uint32_t RESERVED4[12];
   __IM uint32_t CM4_CA_STATUS0;                /*!< 0x000004C0 CM4 cache status 0 */
   __IM uint32_t CM4_CA_STATUS1;                /*!< 0x000004C4 CM4 cache status 1 */
   __IM uint32_t CM4_CA_STATUS2;                /*!< 0x000004C8 CM4 cache status 2 */
   __IM uint32_t RESERVED5[13];
  __IOM uint32_t CRYPTO_BUFF_CTL;               /*!< 0x00000500 Cryptography buffer control */
   __IM uint32_t RESERVED6;
  __IOM uint32_t CRYPTO_BUFF_CMD;               /*!< 0x00000508 Cryptography buffer command */
   __IM uint32_t RESERVED7[29];
  __IOM uint32_t DW0_BUFF_CTL;                  /*!< 0x00000580 Datawire 0 buffer control */
   __IM uint32_t RESERVED8;
  __IOM uint32_t DW0_BUFF_CMD;                  /*!< 0x00000588 Datawire 0 buffer command */
   __IM uint32_t RESERVED9[29];
  __IOM uint32_t DW1_BUFF_CTL;                  /*!< 0x00000600 Datawire 1 buffer control */
   __IM uint32_t RESERVED10;
  __IOM uint32_t DW1_BUFF_CMD;                  /*!< 0x00000608 Datawire 1 buffer command */
   __IM uint32_t RESERVED11[29];
  __IOM uint32_t DAP_BUFF_CTL;                  /*!< 0x00000680 Debug access port buffer control */
   __IM uint32_t RESERVED12;
  __IOM uint32_t DAP_BUFF_CMD;                  /*!< 0x00000688 Debug access port buffer command */
   __IM uint32_t RESERVED13[29];
  __IOM uint32_t EXT_MS0_BUFF_CTL;              /*!< 0x00000700 External master 0 buffer control */
   __IM uint32_t RESERVED14;
  __IOM uint32_t EXT_MS0_BUFF_CMD;              /*!< 0x00000708 External master 0 buffer command */
   __IM uint32_t RESERVED15[29];
  __IOM uint32_t EXT_MS1_BUFF_CTL;              /*!< 0x00000780 External master 1 buffer control */
   __IM uint32_t RESERVED16;
  __IOM uint32_t EXT_MS1_BUFF_CMD;              /*!< 0x00000788 External master 1 buffer command */
   __IM uint32_t RESERVED17[14877];
        FLASHC_FM_CTL_V1_Type FM_CTL;           /*!< 0x0000F000 Flash Macro Registers */
} FLASHC_V1_Type;                               /*!< Size = 65536 (0x10000) */


/* FLASHC_FM_CTL.FM_CTL */
#define FLASHC_FM_CTL_FM_CTL_FM_MODE_Pos        0UL
#define FLASHC_FM_CTL_FM_CTL_FM_MODE_Msk        0xFUL
#define FLASHC_FM_CTL_FM_CTL_FM_SEQ_Pos         8UL
#define FLASHC_FM_CTL_FM_CTL_FM_SEQ_Msk         0x300UL
#define FLASHC_FM_CTL_FM_CTL_DAA_MUX_SEL_Pos    16UL
#define FLASHC_FM_CTL_FM_CTL_DAA_MUX_SEL_Msk    0x7F0000UL
#define FLASHC_FM_CTL_FM_CTL_IF_SEL_Pos         24UL
#define FLASHC_FM_CTL_FM_CTL_IF_SEL_Msk         0x1000000UL
#define FLASHC_FM_CTL_FM_CTL_WR_EN_Pos          25UL
#define FLASHC_FM_CTL_FM_CTL_WR_EN_Msk          0x2000000UL
/* FLASHC_FM_CTL.STATUS */
#define FLASHC_FM_CTL_STATUS_HV_TIMER_RUNNING_Pos 0UL
#define FLASHC_FM_CTL_STATUS_HV_TIMER_RUNNING_Msk 0x1UL
#define FLASHC_FM_CTL_STATUS_HV_REGS_ISOLATED_Pos 1UL
#define FLASHC_FM_CTL_STATUS_HV_REGS_ISOLATED_Msk 0x2UL
#define FLASHC_FM_CTL_STATUS_ILLEGAL_HVOP_Pos   2UL
#define FLASHC_FM_CTL_STATUS_ILLEGAL_HVOP_Msk   0x4UL
#define FLASHC_FM_CTL_STATUS_TURBO_N_Pos        3UL
#define FLASHC_FM_CTL_STATUS_TURBO_N_Msk        0x8UL
#define FLASHC_FM_CTL_STATUS_WR_EN_MON_Pos      4UL
#define FLASHC_FM_CTL_STATUS_WR_EN_MON_Msk      0x10UL
#define FLASHC_FM_CTL_STATUS_IF_SEL_MON_Pos     5UL
#define FLASHC_FM_CTL_STATUS_IF_SEL_MON_Msk     0x20UL
/* FLASHC_FM_CTL.FM_ADDR */
#define FLASHC_FM_CTL_FM_ADDR_RA_Pos            0UL
#define FLASHC_FM_CTL_FM_ADDR_RA_Msk            0xFFFFUL
#define FLASHC_FM_CTL_FM_ADDR_BA_Pos            16UL
#define FLASHC_FM_CTL_FM_ADDR_BA_Msk            0xFF0000UL
#define FLASHC_FM_CTL_FM_ADDR_AXA_Pos           24UL
#define FLASHC_FM_CTL_FM_ADDR_AXA_Msk           0x1000000UL
/* FLASHC_FM_CTL.GEOMETRY */
#define FLASHC_FM_CTL_GEOMETRY_WORD_SIZE_LOG2_Pos 0UL
#define FLASHC_FM_CTL_GEOMETRY_WORD_SIZE_LOG2_Msk 0xFUL
#define FLASHC_FM_CTL_GEOMETRY_PAGE_SIZE_LOG2_Pos 4UL
#define FLASHC_FM_CTL_GEOMETRY_PAGE_SIZE_LOG2_Msk 0xF0UL
#define FLASHC_FM_CTL_GEOMETRY_ROW_COUNT_Pos    8UL
#define FLASHC_FM_CTL_GEOMETRY_ROW_COUNT_Msk    0xFFFF00UL
#define FLASHC_FM_CTL_GEOMETRY_BANK_COUNT_Pos   24UL
#define FLASHC_FM_CTL_GEOMETRY_BANK_COUNT_Msk   0xFF000000UL
/* FLASHC_FM_CTL.GEOMETRY_SUPERVISORY */
#define FLASHC_FM_CTL_GEOMETRY_SUPERVISORY_WORD_SIZE_LOG2_Pos 0UL
#define FLASHC_FM_CTL_GEOMETRY_SUPERVISORY_WORD_SIZE_LOG2_Msk 0xFUL
#define FLASHC_FM_CTL_GEOMETRY_SUPERVISORY_PAGE_SIZE_LOG2_Pos 4UL
#define FLASHC_FM_CTL_GEOMETRY_SUPERVISORY_PAGE_SIZE_LOG2_Msk 0xF0UL
#define FLASHC_FM_CTL_GEOMETRY_SUPERVISORY_ROW_COUNT_Pos 8UL
#define FLASHC_FM_CTL_GEOMETRY_SUPERVISORY_ROW_COUNT_Msk 0xFFFF00UL
#define FLASHC_FM_CTL_GEOMETRY_SUPERVISORY_BANK_COUNT_Pos 24UL
#define FLASHC_FM_CTL_GEOMETRY_SUPERVISORY_BANK_COUNT_Msk 0xFF000000UL
/* FLASHC_FM_CTL.TIMER_CTL */
#define FLASHC_FM_CTL_TIMER_CTL_PERIOD_Pos      0UL
#define FLASHC_FM_CTL_TIMER_CTL_PERIOD_Msk      0xFFFFUL
#define FLASHC_FM_CTL_TIMER_CTL_SCALE_Pos       16UL
#define FLASHC_FM_CTL_TIMER_CTL_SCALE_Msk       0x10000UL
#define FLASHC_FM_CTL_TIMER_CTL_PUMP_CLOCK_SEL_Pos 24UL
#define FLASHC_FM_CTL_TIMER_CTL_PUMP_CLOCK_SEL_Msk 0x1000000UL
#define FLASHC_FM_CTL_TIMER_CTL_PRE_PROG_Pos    25UL
#define FLASHC_FM_CTL_TIMER_CTL_PRE_PROG_Msk    0x2000000UL
#define FLASHC_FM_CTL_TIMER_CTL_PRE_PROG_CSL_Pos 26UL
#define FLASHC_FM_CTL_TIMER_CTL_PRE_PROG_CSL_Msk 0x4000000UL
#define FLASHC_FM_CTL_TIMER_CTL_PUMP_EN_Pos     29UL
#define FLASHC_FM_CTL_TIMER_CTL_PUMP_EN_Msk     0x20000000UL
#define FLASHC_FM_CTL_TIMER_CTL_ACLK_EN_Pos     30UL
#define FLASHC_FM_CTL_TIMER_CTL_ACLK_EN_Msk     0x40000000UL
#define FLASHC_FM_CTL_TIMER_CTL_TIMER_EN_Pos    31UL
#define FLASHC_FM_CTL_TIMER_CTL_TIMER_EN_Msk    0x80000000UL
/* FLASHC_FM_CTL.ANA_CTL0 */
#define FLASHC_FM_CTL_ANA_CTL0_CSLDAC_Pos       8UL
#define FLASHC_FM_CTL_ANA_CTL0_CSLDAC_Msk       0x700UL
#define FLASHC_FM_CTL_ANA_CTL0_VCC_SEL_Pos      24UL
#define FLASHC_FM_CTL_ANA_CTL0_VCC_SEL_Msk      0x1000000UL
#define FLASHC_FM_CTL_ANA_CTL0_FLIP_AMUXBUS_AB_Pos 27UL
#define FLASHC_FM_CTL_ANA_CTL0_FLIP_AMUXBUS_AB_Msk 0x8000000UL
/* FLASHC_FM_CTL.ANA_CTL1 */
#define FLASHC_FM_CTL_ANA_CTL1_MDAC_Pos         0UL
#define FLASHC_FM_CTL_ANA_CTL1_MDAC_Msk         0xFFUL
#define FLASHC_FM_CTL_ANA_CTL1_PDAC_Pos         16UL
#define FLASHC_FM_CTL_ANA_CTL1_PDAC_Msk         0xF0000UL
#define FLASHC_FM_CTL_ANA_CTL1_NDAC_Pos         24UL
#define FLASHC_FM_CTL_ANA_CTL1_NDAC_Msk         0xF000000UL
#define FLASHC_FM_CTL_ANA_CTL1_VPROT_OVERRIDE_Pos 28UL
#define FLASHC_FM_CTL_ANA_CTL1_VPROT_OVERRIDE_Msk 0x10000000UL
#define FLASHC_FM_CTL_ANA_CTL1_R_GRANT_CTL_Pos  29UL
#define FLASHC_FM_CTL_ANA_CTL1_R_GRANT_CTL_Msk  0x20000000UL
#define FLASHC_FM_CTL_ANA_CTL1_RST_SFT_HVPL_Pos 30UL
#define FLASHC_FM_CTL_ANA_CTL1_RST_SFT_HVPL_Msk 0x40000000UL
/* FLASHC_FM_CTL.GEOMETRY_GEN */
#define FLASHC_FM_CTL_GEOMETRY_GEN_DNU_0X20_1_Pos 1UL
#define FLASHC_FM_CTL_GEOMETRY_GEN_DNU_0X20_1_Msk 0x2UL
#define FLASHC_FM_CTL_GEOMETRY_GEN_DNU_0X20_2_Pos 2UL
#define FLASHC_FM_CTL_GEOMETRY_GEN_DNU_0X20_2_Msk 0x4UL
#define FLASHC_FM_CTL_GEOMETRY_GEN_DNU_0X20_3_Pos 3UL
#define FLASHC_FM_CTL_GEOMETRY_GEN_DNU_0X20_3_Msk 0x8UL
/* FLASHC_FM_CTL.TEST_CTL */
#define FLASHC_FM_CTL_TEST_CTL_TEST_MODE_Pos    0UL
#define FLASHC_FM_CTL_TEST_CTL_TEST_MODE_Msk    0x1FUL
#define FLASHC_FM_CTL_TEST_CTL_PN_CTL_Pos       8UL
#define FLASHC_FM_CTL_TEST_CTL_PN_CTL_Msk       0x100UL
#define FLASHC_FM_CTL_TEST_CTL_TM_PE_Pos        9UL
#define FLASHC_FM_CTL_TEST_CTL_TM_PE_Msk        0x200UL
#define FLASHC_FM_CTL_TEST_CTL_TM_DISPOS_Pos    10UL
#define FLASHC_FM_CTL_TEST_CTL_TM_DISPOS_Msk    0x400UL
#define FLASHC_FM_CTL_TEST_CTL_TM_DISNEG_Pos    11UL
#define FLASHC_FM_CTL_TEST_CTL_TM_DISNEG_Msk    0x800UL
#define FLASHC_FM_CTL_TEST_CTL_EN_CLK_MON_Pos   16UL
#define FLASHC_FM_CTL_TEST_CTL_EN_CLK_MON_Msk   0x10000UL
#define FLASHC_FM_CTL_TEST_CTL_CSL_DEBUG_Pos    17UL
#define FLASHC_FM_CTL_TEST_CTL_CSL_DEBUG_Msk    0x20000UL
#define FLASHC_FM_CTL_TEST_CTL_ENABLE_OSC_Pos   18UL
#define FLASHC_FM_CTL_TEST_CTL_ENABLE_OSC_Msk   0x40000UL
#define FLASHC_FM_CTL_TEST_CTL_UNSCRAMBLE_WA_Pos 31UL
#define FLASHC_FM_CTL_TEST_CTL_UNSCRAMBLE_WA_Msk 0x80000000UL
/* FLASHC_FM_CTL.WAIT_CTL */
#define FLASHC_FM_CTL_WAIT_CTL_WAIT_FM_MEM_RD_Pos 0UL
#define FLASHC_FM_CTL_WAIT_CTL_WAIT_FM_MEM_RD_Msk 0xFUL
#define FLASHC_FM_CTL_WAIT_CTL_WAIT_FM_HV_RD_Pos 8UL
#define FLASHC_FM_CTL_WAIT_CTL_WAIT_FM_HV_RD_Msk 0xF00UL
#define FLASHC_FM_CTL_WAIT_CTL_WAIT_FM_HV_WR_Pos 16UL
#define FLASHC_FM_CTL_WAIT_CTL_WAIT_FM_HV_WR_Msk 0x70000UL
/* FLASHC_FM_CTL.MONITOR_STATUS */
#define FLASHC_FM_CTL_MONITOR_STATUS_POS_PUMP_VLO_Pos 1UL
#define FLASHC_FM_CTL_MONITOR_STATUS_POS_PUMP_VLO_Msk 0x2UL
#define FLASHC_FM_CTL_MONITOR_STATUS_NEG_PUMP_VHI_Pos 2UL
#define FLASHC_FM_CTL_MONITOR_STATUS_NEG_PUMP_VHI_Msk 0x4UL
/* FLASHC_FM_CTL.SCRATCH_CTL */
#define FLASHC_FM_CTL_SCRATCH_CTL_DUMMY32_Pos   0UL
#define FLASHC_FM_CTL_SCRATCH_CTL_DUMMY32_Msk   0xFFFFFFFFUL
/* FLASHC_FM_CTL.HV_CTL */
#define FLASHC_FM_CTL_HV_CTL_TIMER_CLOCK_FREQ_Pos 0UL
#define FLASHC_FM_CTL_HV_CTL_TIMER_CLOCK_FREQ_Msk 0xFFUL
/* FLASHC_FM_CTL.ACLK_CTL */
#define FLASHC_FM_CTL_ACLK_CTL_ACLK_GEN_Pos     0UL
#define FLASHC_FM_CTL_ACLK_CTL_ACLK_GEN_Msk     0x1UL
/* FLASHC_FM_CTL.INTR */
#define FLASHC_FM_CTL_INTR_TIMER_EXPIRED_Pos    0UL
#define FLASHC_FM_CTL_INTR_TIMER_EXPIRED_Msk    0x1UL
/* FLASHC_FM_CTL.INTR_SET */
#define FLASHC_FM_CTL_INTR_SET_TIMER_EXPIRED_Pos 0UL
#define FLASHC_FM_CTL_INTR_SET_TIMER_EXPIRED_Msk 0x1UL
/* FLASHC_FM_CTL.INTR_MASK */
#define FLASHC_FM_CTL_INTR_MASK_TIMER_EXPIRED_Pos 0UL
#define FLASHC_FM_CTL_INTR_MASK_TIMER_EXPIRED_Msk 0x1UL
/* FLASHC_FM_CTL.INTR_MASKED */
#define FLASHC_FM_CTL_INTR_MASKED_TIMER_EXPIRED_Pos 0UL
#define FLASHC_FM_CTL_INTR_MASKED_TIMER_EXPIRED_Msk 0x1UL
/* FLASHC_FM_CTL.FM_HV_DATA_ALL */
#define FLASHC_FM_CTL_FM_HV_DATA_ALL_DATA32_Pos 0UL
#define FLASHC_FM_CTL_FM_HV_DATA_ALL_DATA32_Msk 0xFFFFFFFFUL
/* FLASHC_FM_CTL.CAL_CTL0 */
#define FLASHC_FM_CTL_CAL_CTL0_VCT_TRIM_LO_HV_Pos 0UL
#define FLASHC_FM_CTL_CAL_CTL0_VCT_TRIM_LO_HV_Msk 0x1FUL
#define FLASHC_FM_CTL_CAL_CTL0_CDAC_LO_HV_Pos   5UL
#define FLASHC_FM_CTL_CAL_CTL0_CDAC_LO_HV_Msk   0xE0UL
#define FLASHC_FM_CTL_CAL_CTL0_VBG_TRIM_LO_HV_Pos 8UL
#define FLASHC_FM_CTL_CAL_CTL0_VBG_TRIM_LO_HV_Msk 0x1F00UL
#define FLASHC_FM_CTL_CAL_CTL0_VBG_TC_TRIM_LO_HV_Pos 13UL
#define FLASHC_FM_CTL_CAL_CTL0_VBG_TC_TRIM_LO_HV_Msk 0xE000UL
#define FLASHC_FM_CTL_CAL_CTL0_IPREF_TRIM_LO_HV_Pos 16UL
#define FLASHC_FM_CTL_CAL_CTL0_IPREF_TRIM_LO_HV_Msk 0xF0000UL
/* FLASHC_FM_CTL.CAL_CTL1 */
#define FLASHC_FM_CTL_CAL_CTL1_VCT_TRIM_HI_HV_Pos 0UL
#define FLASHC_FM_CTL_CAL_CTL1_VCT_TRIM_HI_HV_Msk 0x1FUL
#define FLASHC_FM_CTL_CAL_CTL1_CDAC_HI_HV_Pos   5UL
#define FLASHC_FM_CTL_CAL_CTL1_CDAC_HI_HV_Msk   0xE0UL
#define FLASHC_FM_CTL_CAL_CTL1_VBG_TRIM_HI_HV_Pos 8UL
#define FLASHC_FM_CTL_CAL_CTL1_VBG_TRIM_HI_HV_Msk 0x1F00UL
#define FLASHC_FM_CTL_CAL_CTL1_VBG_TC_TRIM_HI_HV_Pos 13UL
#define FLASHC_FM_CTL_CAL_CTL1_VBG_TC_TRIM_HI_HV_Msk 0xE000UL
#define FLASHC_FM_CTL_CAL_CTL1_IPREF_TRIM_HI_HV_Pos 16UL
#define FLASHC_FM_CTL_CAL_CTL1_IPREF_TRIM_HI_HV_Msk 0xF0000UL
/* FLASHC_FM_CTL.CAL_CTL2 */
#define FLASHC_FM_CTL_CAL_CTL2_ICREF_TRIM_LO_HV_Pos 0UL
#define FLASHC_FM_CTL_CAL_CTL2_ICREF_TRIM_LO_HV_Msk 0x1FUL
#define FLASHC_FM_CTL_CAL_CTL2_ICREF_TC_TRIM_LO_HV_Pos 5UL
#define FLASHC_FM_CTL_CAL_CTL2_ICREF_TC_TRIM_LO_HV_Msk 0xE0UL
#define FLASHC_FM_CTL_CAL_CTL2_ICREF_TRIM_HI_HV_Pos 8UL
#define FLASHC_FM_CTL_CAL_CTL2_ICREF_TRIM_HI_HV_Msk 0x1F00UL
#define FLASHC_FM_CTL_CAL_CTL2_ICREF_TC_TRIM_HI_HV_Pos 13UL
#define FLASHC_FM_CTL_CAL_CTL2_ICREF_TC_TRIM_HI_HV_Msk 0xE000UL
#define FLASHC_FM_CTL_CAL_CTL2_VREF_SEL_HV_Pos  16UL
#define FLASHC_FM_CTL_CAL_CTL2_VREF_SEL_HV_Msk  0x10000UL
#define FLASHC_FM_CTL_CAL_CTL2_IREF_SEL_HV_Pos  17UL
#define FLASHC_FM_CTL_CAL_CTL2_IREF_SEL_HV_Msk  0x20000UL
#define FLASHC_FM_CTL_CAL_CTL2_FM_ACTIVE_HV_Pos 18UL
#define FLASHC_FM_CTL_CAL_CTL2_FM_ACTIVE_HV_Msk 0x40000UL
#define FLASHC_FM_CTL_CAL_CTL2_TURBO_EXT_HV_Pos 19UL
#define FLASHC_FM_CTL_CAL_CTL2_TURBO_EXT_HV_Msk 0x80000UL
/* FLASHC_FM_CTL.CAL_CTL3 */
#define FLASHC_FM_CTL_CAL_CTL3_OSC_TRIM_HV_Pos  0UL
#define FLASHC_FM_CTL_CAL_CTL3_OSC_TRIM_HV_Msk  0xFUL
#define FLASHC_FM_CTL_CAL_CTL3_OSC_RANGE_TRIM_HV_Pos 4UL
#define FLASHC_FM_CTL_CAL_CTL3_OSC_RANGE_TRIM_HV_Msk 0x10UL
#define FLASHC_FM_CTL_CAL_CTL3_IDAC_HV_Pos      5UL
#define FLASHC_FM_CTL_CAL_CTL3_IDAC_HV_Msk      0x1E0UL
#define FLASHC_FM_CTL_CAL_CTL3_SDAC_HV_Pos      9UL
#define FLASHC_FM_CTL_CAL_CTL3_SDAC_HV_Msk      0x600UL
#define FLASHC_FM_CTL_CAL_CTL3_ITIM_HV_Pos      11UL
#define FLASHC_FM_CTL_CAL_CTL3_ITIM_HV_Msk      0x7800UL
#define FLASHC_FM_CTL_CAL_CTL3_VDDHI_HV_Pos     15UL
#define FLASHC_FM_CTL_CAL_CTL3_VDDHI_HV_Msk     0x8000UL
#define FLASHC_FM_CTL_CAL_CTL3_TURBO_PULSEW_HV_Pos 16UL
#define FLASHC_FM_CTL_CAL_CTL3_TURBO_PULSEW_HV_Msk 0x30000UL
#define FLASHC_FM_CTL_CAL_CTL3_BGLO_EN_HV_Pos   18UL
#define FLASHC_FM_CTL_CAL_CTL3_BGLO_EN_HV_Msk   0x40000UL
#define FLASHC_FM_CTL_CAL_CTL3_BGHI_EN_HV_Pos   19UL
#define FLASHC_FM_CTL_CAL_CTL3_BGHI_EN_HV_Msk   0x80000UL
/* FLASHC_FM_CTL.BOOKMARK */
#define FLASHC_FM_CTL_BOOKMARK_BOOKMARK_Pos     0UL
#define FLASHC_FM_CTL_BOOKMARK_BOOKMARK_Msk     0xFFFFFFFFUL
/* FLASHC_FM_CTL.RED_CTL01 */
#define FLASHC_FM_CTL_RED_CTL01_RED_ADDR_0_Pos  0UL
#define FLASHC_FM_CTL_RED_CTL01_RED_ADDR_0_Msk  0xFFUL
#define FLASHC_FM_CTL_RED_CTL01_RED_EN_0_Pos    8UL
#define FLASHC_FM_CTL_RED_CTL01_RED_EN_0_Msk    0x100UL
#define FLASHC_FM_CTL_RED_CTL01_RED_ADDR_1_Pos  16UL
#define FLASHC_FM_CTL_RED_CTL01_RED_ADDR_1_Msk  0xFF0000UL
#define FLASHC_FM_CTL_RED_CTL01_RED_EN_1_Pos    24UL
#define FLASHC_FM_CTL_RED_CTL01_RED_EN_1_Msk    0x1000000UL
/* FLASHC_FM_CTL.RED_CTL23 */
#define FLASHC_FM_CTL_RED_CTL23_RED_ADDR_2_Pos  0UL
#define FLASHC_FM_CTL_RED_CTL23_RED_ADDR_2_Msk  0xFFUL
#define FLASHC_FM_CTL_RED_CTL23_RED_EN_2_Pos    8UL
#define FLASHC_FM_CTL_RED_CTL23_RED_EN_2_Msk    0x100UL
#define FLASHC_FM_CTL_RED_CTL23_RED_ADDR_3_Pos  16UL
#define FLASHC_FM_CTL_RED_CTL23_RED_ADDR_3_Msk  0xFF0000UL
#define FLASHC_FM_CTL_RED_CTL23_RED_EN_3_Pos    24UL
#define FLASHC_FM_CTL_RED_CTL23_RED_EN_3_Msk    0x1000000UL
/* FLASHC_FM_CTL.RED_CTL45 */
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_1_Pos    0UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_1_Msk    0x1UL
#define FLASHC_FM_CTL_RED_CTL45_REG_ACT_HV_Pos  1UL
#define FLASHC_FM_CTL_RED_CTL45_REG_ACT_HV_Msk  0x2UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_3_Pos    2UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_3_Msk    0x4UL
#define FLASHC_FM_CTL_RED_CTL45_FDIV_TRIM_HV_0_Pos 3UL
#define FLASHC_FM_CTL_RED_CTL45_FDIV_TRIM_HV_0_Msk 0x8UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_5_Pos    4UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_5_Msk    0x10UL
#define FLASHC_FM_CTL_RED_CTL45_FDIV_TRIM_HV_1_Pos 5UL
#define FLASHC_FM_CTL_RED_CTL45_FDIV_TRIM_HV_1_Msk 0x20UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_6_Pos    6UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_6_Msk    0x40UL
#define FLASHC_FM_CTL_RED_CTL45_VLIM_TRIM_HV_0_Pos 7UL
#define FLASHC_FM_CTL_RED_CTL45_VLIM_TRIM_HV_0_Msk 0x80UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_8_Pos    8UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_8_Msk    0x100UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_23_16_Pos 16UL
#define FLASHC_FM_CTL_RED_CTL45_DNU_45_23_16_Msk 0xFF0000UL
/* FLASHC_FM_CTL.RED_CTL67 */
#define FLASHC_FM_CTL_RED_CTL67_VLIM_TRIM_HV_1_Pos 0UL
#define FLASHC_FM_CTL_RED_CTL67_VLIM_TRIM_HV_1_Msk 0x1UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_1_Pos    1UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_1_Msk    0x2UL
#define FLASHC_FM_CTL_RED_CTL67_VPROT_ACT_HV_Pos 2UL
#define FLASHC_FM_CTL_RED_CTL67_VPROT_ACT_HV_Msk 0x4UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_3_Pos    3UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_3_Msk    0x8UL
#define FLASHC_FM_CTL_RED_CTL67_IPREF_TC_HV_Pos 4UL
#define FLASHC_FM_CTL_RED_CTL67_IPREF_TC_HV_Msk 0x10UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_5_Pos    5UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_5_Msk    0x20UL
#define FLASHC_FM_CTL_RED_CTL67_IPREF_TRIMA_HI_HV_Pos 6UL
#define FLASHC_FM_CTL_RED_CTL67_IPREF_TRIMA_HI_HV_Msk 0x40UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_7_Pos    7UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_7_Msk    0x80UL
#define FLASHC_FM_CTL_RED_CTL67_IPREF_TRIMA_LO_HV_Pos 8UL
#define FLASHC_FM_CTL_RED_CTL67_IPREF_TRIMA_LO_HV_Msk 0x100UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_23_16_Pos 16UL
#define FLASHC_FM_CTL_RED_CTL67_DNU_67_23_16_Msk 0xFF0000UL
/* FLASHC_FM_CTL.RED_CTL_SM01 */
#define FLASHC_FM_CTL_RED_CTL_SM01_RED_ADDR_SM0_Pos 0UL
#define FLASHC_FM_CTL_RED_CTL_SM01_RED_ADDR_SM0_Msk 0xFFUL
#define FLASHC_FM_CTL_RED_CTL_SM01_RED_EN_SM0_Pos 8UL
#define FLASHC_FM_CTL_RED_CTL_SM01_RED_EN_SM0_Msk 0x100UL
#define FLASHC_FM_CTL_RED_CTL_SM01_RED_ADDR_SM1_Pos 16UL
#define FLASHC_FM_CTL_RED_CTL_SM01_RED_ADDR_SM1_Msk 0xFF0000UL
#define FLASHC_FM_CTL_RED_CTL_SM01_RED_EN_SM1_Pos 24UL
#define FLASHC_FM_CTL_RED_CTL_SM01_RED_EN_SM1_Msk 0x1000000UL
#define FLASHC_FM_CTL_RED_CTL_SM01_TRKD_Pos     30UL
#define FLASHC_FM_CTL_RED_CTL_SM01_TRKD_Msk     0x40000000UL
#define FLASHC_FM_CTL_RED_CTL_SM01_R_GRANT_EN_Pos 31UL
#define FLASHC_FM_CTL_RED_CTL_SM01_R_GRANT_EN_Msk 0x80000000UL
/* FLASHC_FM_CTL.TM_CMPR */
#define FLASHC_FM_CTL_TM_CMPR_DATA_COMP_RESULT_Pos 0UL
#define FLASHC_FM_CTL_TM_CMPR_DATA_COMP_RESULT_Msk 0x1UL
/* FLASHC_FM_CTL.FM_HV_DATA */
#define FLASHC_FM_CTL_FM_HV_DATA_DATA32_Pos     0UL
#define FLASHC_FM_CTL_FM_HV_DATA_DATA32_Msk     0xFFFFFFFFUL
/* FLASHC_FM_CTL.FM_MEM_DATA */
#define FLASHC_FM_CTL_FM_MEM_DATA_DATA32_Pos    0UL
#define FLASHC_FM_CTL_FM_MEM_DATA_DATA32_Msk    0xFFFFFFFFUL


/* FLASHC.FLASH_CTL */
#define FLASHC_FLASH_CTL_MAIN_WS_Pos            0UL
#define FLASHC_FLASH_CTL_MAIN_WS_Msk            0xFUL
#define FLASHC_FLASH_CTL_REMAP_Pos              8UL
#define FLASHC_FLASH_CTL_REMAP_Msk              0x100UL
/* FLASHC.FLASH_PWR_CTL */
#define FLASHC_FLASH_PWR_CTL_ENABLE_Pos         0UL
#define FLASHC_FLASH_PWR_CTL_ENABLE_Msk         0x1UL
#define FLASHC_FLASH_PWR_CTL_ENABLE_HV_Pos      1UL
#define FLASHC_FLASH_PWR_CTL_ENABLE_HV_Msk      0x2UL
/* FLASHC.FLASH_CMD */
#define FLASHC_FLASH_CMD_INV_Pos                0UL
#define FLASHC_FLASH_CMD_INV_Msk                0x1UL
/* FLASHC.BIST_CTL */
#define FLASHC_BIST_CTL_OPCODE_Pos              0UL
#define FLASHC_BIST_CTL_OPCODE_Msk              0x3UL
#define FLASHC_BIST_CTL_UP_Pos                  2UL
#define FLASHC_BIST_CTL_UP_Msk                  0x4UL
#define FLASHC_BIST_CTL_ROW_FIRST_Pos           3UL
#define FLASHC_BIST_CTL_ROW_FIRST_Msk           0x8UL
#define FLASHC_BIST_CTL_ADDR_START_ENABLED_Pos  4UL
#define FLASHC_BIST_CTL_ADDR_START_ENABLED_Msk  0x10UL
#define FLASHC_BIST_CTL_ADDR_COMPLIMENT_ENABLED_Pos 5UL
#define FLASHC_BIST_CTL_ADDR_COMPLIMENT_ENABLED_Msk 0x20UL
#define FLASHC_BIST_CTL_INCR_DECR_BOTH_Pos      6UL
#define FLASHC_BIST_CTL_INCR_DECR_BOTH_Msk      0x40UL
#define FLASHC_BIST_CTL_STOP_ON_ERROR_Pos       7UL
#define FLASHC_BIST_CTL_STOP_ON_ERROR_Msk       0x80UL
/* FLASHC.BIST_CMD */
#define FLASHC_BIST_CMD_START_Pos               0UL
#define FLASHC_BIST_CMD_START_Msk               0x1UL
/* FLASHC.BIST_ADDR_START */
#define FLASHC_BIST_ADDR_START_COL_ADDR_START_Pos 0UL
#define FLASHC_BIST_ADDR_START_COL_ADDR_START_Msk 0xFFFFUL
#define FLASHC_BIST_ADDR_START_ROW_ADDR_START_Pos 16UL
#define FLASHC_BIST_ADDR_START_ROW_ADDR_START_Msk 0xFFFF0000UL
/* FLASHC.BIST_DATA */
#define FLASHC_BIST_DATA_DATA_Pos               0UL
#define FLASHC_BIST_DATA_DATA_Msk               0xFFFFFFFFUL
/* FLASHC.BIST_DATA_ACT */
#define FLASHC_BIST_DATA_ACT_DATA_Pos           0UL
#define FLASHC_BIST_DATA_ACT_DATA_Msk           0xFFFFFFFFUL
/* FLASHC.BIST_DATA_EXP */
#define FLASHC_BIST_DATA_EXP_DATA_Pos           0UL
#define FLASHC_BIST_DATA_EXP_DATA_Msk           0xFFFFFFFFUL
/* FLASHC.BIST_ADDR */
#define FLASHC_BIST_ADDR_COL_ADDR_Pos           0UL
#define FLASHC_BIST_ADDR_COL_ADDR_Msk           0xFFFFUL
#define FLASHC_BIST_ADDR_ROW_ADDR_Pos           16UL
#define FLASHC_BIST_ADDR_ROW_ADDR_Msk           0xFFFF0000UL
/* FLASHC.BIST_STATUS */
#define FLASHC_BIST_STATUS_FAIL_Pos             0UL
#define FLASHC_BIST_STATUS_FAIL_Msk             0x1UL
/* FLASHC.CM0_CA_CTL0 */
#define FLASHC_CM0_CA_CTL0_WAY_Pos              16UL
#define FLASHC_CM0_CA_CTL0_WAY_Msk              0x30000UL
#define FLASHC_CM0_CA_CTL0_SET_ADDR_Pos         24UL
#define FLASHC_CM0_CA_CTL0_SET_ADDR_Msk         0x7000000UL
#define FLASHC_CM0_CA_CTL0_PREF_EN_Pos          30UL
#define FLASHC_CM0_CA_CTL0_PREF_EN_Msk          0x40000000UL
#define FLASHC_CM0_CA_CTL0_ENABLED_Pos          31UL
#define FLASHC_CM0_CA_CTL0_ENABLED_Msk          0x80000000UL
/* FLASHC.CM0_CA_CTL1 */
#define FLASHC_CM0_CA_CTL1_PWR_MODE_Pos         0UL
#define FLASHC_CM0_CA_CTL1_PWR_MODE_Msk         0x3UL
#define FLASHC_CM0_CA_CTL1_VECTKEYSTAT_Pos      16UL
#define FLASHC_CM0_CA_CTL1_VECTKEYSTAT_Msk      0xFFFF0000UL
/* FLASHC.CM0_CA_CTL2 */
#define FLASHC_CM0_CA_CTL2_PWRUP_DELAY_Pos      0UL
#define FLASHC_CM0_CA_CTL2_PWRUP_DELAY_Msk      0x3FFUL
/* FLASHC.CM0_CA_CMD */
#define FLASHC_CM0_CA_CMD_INV_Pos               0UL
#define FLASHC_CM0_CA_CMD_INV_Msk               0x1UL
/* FLASHC.CM0_CA_STATUS0 */
#define FLASHC_CM0_CA_STATUS0_VALID16_Pos       0UL
#define FLASHC_CM0_CA_STATUS0_VALID16_Msk       0xFFFFUL
/* FLASHC.CM0_CA_STATUS1 */
#define FLASHC_CM0_CA_STATUS1_TAG_Pos           0UL
#define FLASHC_CM0_CA_STATUS1_TAG_Msk           0xFFFFFFFFUL
/* FLASHC.CM0_CA_STATUS2 */
#define FLASHC_CM0_CA_STATUS2_LRU_Pos           0UL
#define FLASHC_CM0_CA_STATUS2_LRU_Msk           0x3FUL
/* FLASHC.CM4_CA_CTL0 */
#define FLASHC_CM4_CA_CTL0_WAY_Pos              16UL
#define FLASHC_CM4_CA_CTL0_WAY_Msk              0x30000UL
#define FLASHC_CM4_CA_CTL0_SET_ADDR_Pos         24UL
#define FLASHC_CM4_CA_CTL0_SET_ADDR_Msk         0x7000000UL
#define FLASHC_CM4_CA_CTL0_PREF_EN_Pos          30UL
#define FLASHC_CM4_CA_CTL0_PREF_EN_Msk          0x40000000UL
#define FLASHC_CM4_CA_CTL0_ENABLED_Pos          31UL
#define FLASHC_CM4_CA_CTL0_ENABLED_Msk          0x80000000UL
/* FLASHC.CM4_CA_CTL1 */
#define FLASHC_CM4_CA_CTL1_PWR_MODE_Pos         0UL
#define FLASHC_CM4_CA_CTL1_PWR_MODE_Msk         0x3UL
#define FLASHC_CM4_CA_CTL1_VECTKEYSTAT_Pos      16UL
#define FLASHC_CM4_CA_CTL1_VECTKEYSTAT_Msk      0xFFFF0000UL
/* FLASHC.CM4_CA_CTL2 */
#define FLASHC_CM4_CA_CTL2_PWRUP_DELAY_Pos      0UL
#define FLASHC_CM4_CA_CTL2_PWRUP_DELAY_Msk      0x3FFUL
/* FLASHC.CM4_CA_CMD */
#define FLASHC_CM4_CA_CMD_INV_Pos               0UL
#define FLASHC_CM4_CA_CMD_INV_Msk               0x1UL
/* FLASHC.CM4_CA_STATUS0 */
#define FLASHC_CM4_CA_STATUS0_VALID16_Pos       0UL
#define FLASHC_CM4_CA_STATUS0_VALID16_Msk       0xFFFFUL
/* FLASHC.CM4_CA_STATUS1 */
#define FLASHC_CM4_CA_STATUS1_TAG_Pos           0UL
#define FLASHC_CM4_CA_STATUS1_TAG_Msk           0xFFFFFFFFUL
/* FLASHC.CM4_CA_STATUS2 */
#define FLASHC_CM4_CA_STATUS2_LRU_Pos           0UL
#define FLASHC_CM4_CA_STATUS2_LRU_Msk           0x3FUL
/* FLASHC.CRYPTO_BUFF_CTL */
#define FLASHC_CRYPTO_BUFF_CTL_PREF_EN_Pos      30UL
#define FLASHC_CRYPTO_BUFF_CTL_PREF_EN_Msk      0x40000000UL
#define FLASHC_CRYPTO_BUFF_CTL_ENABLED_Pos      31UL
#define FLASHC_CRYPTO_BUFF_CTL_ENABLED_Msk      0x80000000UL
/* FLASHC.CRYPTO_BUFF_CMD */
#define FLASHC_CRYPTO_BUFF_CMD_INV_Pos          0UL
#define FLASHC_CRYPTO_BUFF_CMD_INV_Msk          0x1UL
/* FLASHC.DW0_BUFF_CTL */
#define FLASHC_DW0_BUFF_CTL_PREF_EN_Pos         30UL
#define FLASHC_DW0_BUFF_CTL_PREF_EN_Msk         0x40000000UL
#define FLASHC_DW0_BUFF_CTL_ENABLED_Pos         31UL
#define FLASHC_DW0_BUFF_CTL_ENABLED_Msk         0x80000000UL
/* FLASHC.DW0_BUFF_CMD */
#define FLASHC_DW0_BUFF_CMD_INV_Pos             0UL
#define FLASHC_DW0_BUFF_CMD_INV_Msk             0x1UL
/* FLASHC.DW1_BUFF_CTL */
#define FLASHC_DW1_BUFF_CTL_PREF_EN_Pos         30UL
#define FLASHC_DW1_BUFF_CTL_PREF_EN_Msk         0x40000000UL
#define FLASHC_DW1_BUFF_CTL_ENABLED_Pos         31UL
#define FLASHC_DW1_BUFF_CTL_ENABLED_Msk         0x80000000UL
/* FLASHC.DW1_BUFF_CMD */
#define FLASHC_DW1_BUFF_CMD_INV_Pos             0UL
#define FLASHC_DW1_BUFF_CMD_INV_Msk             0x1UL
/* FLASHC.DAP_BUFF_CTL */
#define FLASHC_DAP_BUFF_CTL_PREF_EN_Pos         30UL
#define FLASHC_DAP_BUFF_CTL_PREF_EN_Msk         0x40000000UL
#define FLASHC_DAP_BUFF_CTL_ENABLED_Pos         31UL
#define FLASHC_DAP_BUFF_CTL_ENABLED_Msk         0x80000000UL
/* FLASHC.DAP_BUFF_CMD */
#define FLASHC_DAP_BUFF_CMD_INV_Pos             0UL
#define FLASHC_DAP_BUFF_CMD_INV_Msk             0x1UL
/* FLASHC.EXT_MS0_BUFF_CTL */
#define FLASHC_EXT_MS0_BUFF_CTL_PREF_EN_Pos     30UL
#define FLASHC_EXT_MS0_BUFF_CTL_PREF_EN_Msk     0x40000000UL
#define FLASHC_EXT_MS0_BUFF_CTL_ENABLED_Pos     31UL
#define FLASHC_EXT_MS0_BUFF_CTL_ENABLED_Msk     0x80000000UL
/* FLASHC.EXT_MS0_BUFF_CMD */
#define FLASHC_EXT_MS0_BUFF_CMD_INV_Pos         0UL
#define FLASHC_EXT_MS0_BUFF_CMD_INV_Msk         0x1UL
/* FLASHC.EXT_MS1_BUFF_CTL */
#define FLASHC_EXT_MS1_BUFF_CTL_PREF_EN_Pos     30UL
#define FLASHC_EXT_MS1_BUFF_CTL_PREF_EN_Msk     0x40000000UL
#define FLASHC_EXT_MS1_BUFF_CTL_ENABLED_Pos     31UL
#define FLASHC_EXT_MS1_BUFF_CTL_ENABLED_Msk     0x80000000UL
/* FLASHC.EXT_MS1_BUFF_CMD */
#define FLASHC_EXT_MS1_BUFF_CMD_INV_Pos         0UL
#define FLASHC_EXT_MS1_BUFF_CMD_INV_Msk         0x1UL


#endif /* _CYIP_FLASHC_H_ */


/* [] END OF FILE */
