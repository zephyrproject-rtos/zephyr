/***************************************************************************//**
* \file cyip_ctbm.h
*
* \brief
* CTBM IP definitions
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

#ifndef _CYIP_CTBM_H_
#define _CYIP_CTBM_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     CTBM
*******************************************************************************/

#define CTBM_SECTION_SIZE                       0x00010000UL

/**
  * \brief Continuous Time Block Mini (CTBM)
  */
typedef struct {
  __IOM uint32_t CTB_CTRL;                      /*!< 0x00000000 global CTB and power control */
  __IOM uint32_t OA_RES0_CTRL;                  /*!< 0x00000004 Opamp0 and resistor0 control */
  __IOM uint32_t OA_RES1_CTRL;                  /*!< 0x00000008 Opamp1 and resistor1 control */
   __IM uint32_t COMP_STAT;                     /*!< 0x0000000C Comparator status */
   __IM uint32_t RESERVED[4];
  __IOM uint32_t INTR;                          /*!< 0x00000020 Interrupt request register */
  __IOM uint32_t INTR_SET;                      /*!< 0x00000024 Interrupt request set register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x00000028 Interrupt request mask */
   __IM uint32_t INTR_MASKED;                   /*!< 0x0000002C Interrupt request masked */
   __IM uint32_t RESERVED1[20];
  __IOM uint32_t OA0_SW;                        /*!< 0x00000080 Opamp0 switch control */
  __IOM uint32_t OA0_SW_CLEAR;                  /*!< 0x00000084 Opamp0 switch control clear */
  __IOM uint32_t OA1_SW;                        /*!< 0x00000088 Opamp1 switch control */
  __IOM uint32_t OA1_SW_CLEAR;                  /*!< 0x0000008C Opamp1 switch control clear */
   __IM uint32_t RESERVED2[4];
  __IOM uint32_t CTD_SW;                        /*!< 0x000000A0 CTDAC connection switch control */
  __IOM uint32_t CTD_SW_CLEAR;                  /*!< 0x000000A4 CTDAC connection switch control clear */
   __IM uint32_t RESERVED3[6];
  __IOM uint32_t CTB_SW_DS_CTRL;                /*!< 0x000000C0 CTB bus switch control */
  __IOM uint32_t CTB_SW_SQ_CTRL;                /*!< 0x000000C4 CTB bus switch Sar Sequencer control */
   __IM uint32_t CTB_SW_STATUS;                 /*!< 0x000000C8 CTB bus switch control status */
   __IM uint32_t RESERVED4[909];
  __IOM uint32_t OA0_OFFSET_TRIM;               /*!< 0x00000F00 Opamp0 trim control */
  __IOM uint32_t OA0_SLOPE_OFFSET_TRIM;         /*!< 0x00000F04 Opamp0 trim control */
  __IOM uint32_t OA0_COMP_TRIM;                 /*!< 0x00000F08 Opamp0 trim control */
  __IOM uint32_t OA1_OFFSET_TRIM;               /*!< 0x00000F0C Opamp1 trim control */
  __IOM uint32_t OA1_SLOPE_OFFSET_TRIM;         /*!< 0x00000F10 Opamp1 trim control */
  __IOM uint32_t OA1_COMP_TRIM;                 /*!< 0x00000F14 Opamp1 trim control */
} CTBM_V1_Type;                                 /*!< Size = 3864 (0xF18) */


/* CTBM.CTB_CTRL */
#define CTBM_CTB_CTRL_DEEPSLEEP_ON_Pos          30UL
#define CTBM_CTB_CTRL_DEEPSLEEP_ON_Msk          0x40000000UL
#define CTBM_CTB_CTRL_ENABLED_Pos               31UL
#define CTBM_CTB_CTRL_ENABLED_Msk               0x80000000UL
/* CTBM.OA_RES0_CTRL */
#define CTBM_OA_RES0_CTRL_OA0_PWR_MODE_Pos      0UL
#define CTBM_OA_RES0_CTRL_OA0_PWR_MODE_Msk      0x7UL
#define CTBM_OA_RES0_CTRL_OA0_DRIVE_STR_SEL_Pos 3UL
#define CTBM_OA_RES0_CTRL_OA0_DRIVE_STR_SEL_Msk 0x8UL
#define CTBM_OA_RES0_CTRL_OA0_COMP_EN_Pos       4UL
#define CTBM_OA_RES0_CTRL_OA0_COMP_EN_Msk       0x10UL
#define CTBM_OA_RES0_CTRL_OA0_HYST_EN_Pos       5UL
#define CTBM_OA_RES0_CTRL_OA0_HYST_EN_Msk       0x20UL
#define CTBM_OA_RES0_CTRL_OA0_BYPASS_DSI_SYNC_Pos 6UL
#define CTBM_OA_RES0_CTRL_OA0_BYPASS_DSI_SYNC_Msk 0x40UL
#define CTBM_OA_RES0_CTRL_OA0_DSI_LEVEL_Pos     7UL
#define CTBM_OA_RES0_CTRL_OA0_DSI_LEVEL_Msk     0x80UL
#define CTBM_OA_RES0_CTRL_OA0_COMPINT_Pos       8UL
#define CTBM_OA_RES0_CTRL_OA0_COMPINT_Msk       0x300UL
#define CTBM_OA_RES0_CTRL_OA0_PUMP_EN_Pos       11UL
#define CTBM_OA_RES0_CTRL_OA0_PUMP_EN_Msk       0x800UL
#define CTBM_OA_RES0_CTRL_OA0_BOOST_EN_Pos      12UL
#define CTBM_OA_RES0_CTRL_OA0_BOOST_EN_Msk      0x1000UL
/* CTBM.OA_RES1_CTRL */
#define CTBM_OA_RES1_CTRL_OA1_PWR_MODE_Pos      0UL
#define CTBM_OA_RES1_CTRL_OA1_PWR_MODE_Msk      0x7UL
#define CTBM_OA_RES1_CTRL_OA1_DRIVE_STR_SEL_Pos 3UL
#define CTBM_OA_RES1_CTRL_OA1_DRIVE_STR_SEL_Msk 0x8UL
#define CTBM_OA_RES1_CTRL_OA1_COMP_EN_Pos       4UL
#define CTBM_OA_RES1_CTRL_OA1_COMP_EN_Msk       0x10UL
#define CTBM_OA_RES1_CTRL_OA1_HYST_EN_Pos       5UL
#define CTBM_OA_RES1_CTRL_OA1_HYST_EN_Msk       0x20UL
#define CTBM_OA_RES1_CTRL_OA1_BYPASS_DSI_SYNC_Pos 6UL
#define CTBM_OA_RES1_CTRL_OA1_BYPASS_DSI_SYNC_Msk 0x40UL
#define CTBM_OA_RES1_CTRL_OA1_DSI_LEVEL_Pos     7UL
#define CTBM_OA_RES1_CTRL_OA1_DSI_LEVEL_Msk     0x80UL
#define CTBM_OA_RES1_CTRL_OA1_COMPINT_Pos       8UL
#define CTBM_OA_RES1_CTRL_OA1_COMPINT_Msk       0x300UL
#define CTBM_OA_RES1_CTRL_OA1_PUMP_EN_Pos       11UL
#define CTBM_OA_RES1_CTRL_OA1_PUMP_EN_Msk       0x800UL
#define CTBM_OA_RES1_CTRL_OA1_BOOST_EN_Pos      12UL
#define CTBM_OA_RES1_CTRL_OA1_BOOST_EN_Msk      0x1000UL
/* CTBM.COMP_STAT */
#define CTBM_COMP_STAT_OA0_COMP_Pos             0UL
#define CTBM_COMP_STAT_OA0_COMP_Msk             0x1UL
#define CTBM_COMP_STAT_OA1_COMP_Pos             16UL
#define CTBM_COMP_STAT_OA1_COMP_Msk             0x10000UL
/* CTBM.INTR */
#define CTBM_INTR_COMP0_Pos                     0UL
#define CTBM_INTR_COMP0_Msk                     0x1UL
#define CTBM_INTR_COMP1_Pos                     1UL
#define CTBM_INTR_COMP1_Msk                     0x2UL
/* CTBM.INTR_SET */
#define CTBM_INTR_SET_COMP0_SET_Pos             0UL
#define CTBM_INTR_SET_COMP0_SET_Msk             0x1UL
#define CTBM_INTR_SET_COMP1_SET_Pos             1UL
#define CTBM_INTR_SET_COMP1_SET_Msk             0x2UL
/* CTBM.INTR_MASK */
#define CTBM_INTR_MASK_COMP0_MASK_Pos           0UL
#define CTBM_INTR_MASK_COMP0_MASK_Msk           0x1UL
#define CTBM_INTR_MASK_COMP1_MASK_Pos           1UL
#define CTBM_INTR_MASK_COMP1_MASK_Msk           0x2UL
/* CTBM.INTR_MASKED */
#define CTBM_INTR_MASKED_COMP0_MASKED_Pos       0UL
#define CTBM_INTR_MASKED_COMP0_MASKED_Msk       0x1UL
#define CTBM_INTR_MASKED_COMP1_MASKED_Pos       1UL
#define CTBM_INTR_MASKED_COMP1_MASKED_Msk       0x2UL
/* CTBM.OA0_SW */
#define CTBM_OA0_SW_OA0P_A00_Pos                0UL
#define CTBM_OA0_SW_OA0P_A00_Msk                0x1UL
#define CTBM_OA0_SW_OA0P_A20_Pos                2UL
#define CTBM_OA0_SW_OA0P_A20_Msk                0x4UL
#define CTBM_OA0_SW_OA0P_A30_Pos                3UL
#define CTBM_OA0_SW_OA0P_A30_Msk                0x8UL
#define CTBM_OA0_SW_OA0M_A11_Pos                8UL
#define CTBM_OA0_SW_OA0M_A11_Msk                0x100UL
#define CTBM_OA0_SW_OA0M_A81_Pos                14UL
#define CTBM_OA0_SW_OA0M_A81_Msk                0x4000UL
#define CTBM_OA0_SW_OA0O_D51_Pos                18UL
#define CTBM_OA0_SW_OA0O_D51_Msk                0x40000UL
#define CTBM_OA0_SW_OA0O_D81_Pos                21UL
#define CTBM_OA0_SW_OA0O_D81_Msk                0x200000UL
/* CTBM.OA0_SW_CLEAR */
#define CTBM_OA0_SW_CLEAR_OA0P_A00_Pos          0UL
#define CTBM_OA0_SW_CLEAR_OA0P_A00_Msk          0x1UL
#define CTBM_OA0_SW_CLEAR_OA0P_A20_Pos          2UL
#define CTBM_OA0_SW_CLEAR_OA0P_A20_Msk          0x4UL
#define CTBM_OA0_SW_CLEAR_OA0P_A30_Pos          3UL
#define CTBM_OA0_SW_CLEAR_OA0P_A30_Msk          0x8UL
#define CTBM_OA0_SW_CLEAR_OA0M_A11_Pos          8UL
#define CTBM_OA0_SW_CLEAR_OA0M_A11_Msk          0x100UL
#define CTBM_OA0_SW_CLEAR_OA0M_A81_Pos          14UL
#define CTBM_OA0_SW_CLEAR_OA0M_A81_Msk          0x4000UL
#define CTBM_OA0_SW_CLEAR_OA0O_D51_Pos          18UL
#define CTBM_OA0_SW_CLEAR_OA0O_D51_Msk          0x40000UL
#define CTBM_OA0_SW_CLEAR_OA0O_D81_Pos          21UL
#define CTBM_OA0_SW_CLEAR_OA0O_D81_Msk          0x200000UL
/* CTBM.OA1_SW */
#define CTBM_OA1_SW_OA1P_A03_Pos                0UL
#define CTBM_OA1_SW_OA1P_A03_Msk                0x1UL
#define CTBM_OA1_SW_OA1P_A13_Pos                1UL
#define CTBM_OA1_SW_OA1P_A13_Msk                0x2UL
#define CTBM_OA1_SW_OA1P_A43_Pos                4UL
#define CTBM_OA1_SW_OA1P_A43_Msk                0x10UL
#define CTBM_OA1_SW_OA1P_A73_Pos                7UL
#define CTBM_OA1_SW_OA1P_A73_Msk                0x80UL
#define CTBM_OA1_SW_OA1M_A22_Pos                8UL
#define CTBM_OA1_SW_OA1M_A22_Msk                0x100UL
#define CTBM_OA1_SW_OA1M_A82_Pos                14UL
#define CTBM_OA1_SW_OA1M_A82_Msk                0x4000UL
#define CTBM_OA1_SW_OA1O_D52_Pos                18UL
#define CTBM_OA1_SW_OA1O_D52_Msk                0x40000UL
#define CTBM_OA1_SW_OA1O_D62_Pos                19UL
#define CTBM_OA1_SW_OA1O_D62_Msk                0x80000UL
#define CTBM_OA1_SW_OA1O_D82_Pos                21UL
#define CTBM_OA1_SW_OA1O_D82_Msk                0x200000UL
/* CTBM.OA1_SW_CLEAR */
#define CTBM_OA1_SW_CLEAR_OA1P_A03_Pos          0UL
#define CTBM_OA1_SW_CLEAR_OA1P_A03_Msk          0x1UL
#define CTBM_OA1_SW_CLEAR_OA1P_A13_Pos          1UL
#define CTBM_OA1_SW_CLEAR_OA1P_A13_Msk          0x2UL
#define CTBM_OA1_SW_CLEAR_OA1P_A43_Pos          4UL
#define CTBM_OA1_SW_CLEAR_OA1P_A43_Msk          0x10UL
#define CTBM_OA1_SW_CLEAR_OA1P_A73_Pos          7UL
#define CTBM_OA1_SW_CLEAR_OA1P_A73_Msk          0x80UL
#define CTBM_OA1_SW_CLEAR_OA1M_A22_Pos          8UL
#define CTBM_OA1_SW_CLEAR_OA1M_A22_Msk          0x100UL
#define CTBM_OA1_SW_CLEAR_OA1M_A82_Pos          14UL
#define CTBM_OA1_SW_CLEAR_OA1M_A82_Msk          0x4000UL
#define CTBM_OA1_SW_CLEAR_OA1O_D52_Pos          18UL
#define CTBM_OA1_SW_CLEAR_OA1O_D52_Msk          0x40000UL
#define CTBM_OA1_SW_CLEAR_OA1O_D62_Pos          19UL
#define CTBM_OA1_SW_CLEAR_OA1O_D62_Msk          0x80000UL
#define CTBM_OA1_SW_CLEAR_OA1O_D82_Pos          21UL
#define CTBM_OA1_SW_CLEAR_OA1O_D82_Msk          0x200000UL
/* CTBM.CTD_SW */
#define CTBM_CTD_SW_CTDD_CRD_Pos                1UL
#define CTBM_CTD_SW_CTDD_CRD_Msk                0x2UL
#define CTBM_CTD_SW_CTDS_CRS_Pos                4UL
#define CTBM_CTD_SW_CTDS_CRS_Msk                0x10UL
#define CTBM_CTD_SW_CTDS_COR_Pos                5UL
#define CTBM_CTD_SW_CTDS_COR_Msk                0x20UL
#define CTBM_CTD_SW_CTDO_C6H_Pos                8UL
#define CTBM_CTD_SW_CTDO_C6H_Msk                0x100UL
#define CTBM_CTD_SW_CTDO_COS_Pos                9UL
#define CTBM_CTD_SW_CTDO_COS_Msk                0x200UL
#define CTBM_CTD_SW_CTDH_COB_Pos                10UL
#define CTBM_CTD_SW_CTDH_COB_Msk                0x400UL
#define CTBM_CTD_SW_CTDH_CHD_Pos                12UL
#define CTBM_CTD_SW_CTDH_CHD_Msk                0x1000UL
#define CTBM_CTD_SW_CTDH_CA0_Pos                13UL
#define CTBM_CTD_SW_CTDH_CA0_Msk                0x2000UL
#define CTBM_CTD_SW_CTDH_CIS_Pos                14UL
#define CTBM_CTD_SW_CTDH_CIS_Msk                0x4000UL
#define CTBM_CTD_SW_CTDH_ILR_Pos                15UL
#define CTBM_CTD_SW_CTDH_ILR_Msk                0x8000UL
/* CTBM.CTD_SW_CLEAR */
#define CTBM_CTD_SW_CLEAR_CTDD_CRD_Pos          1UL
#define CTBM_CTD_SW_CLEAR_CTDD_CRD_Msk          0x2UL
#define CTBM_CTD_SW_CLEAR_CTDS_CRS_Pos          4UL
#define CTBM_CTD_SW_CLEAR_CTDS_CRS_Msk          0x10UL
#define CTBM_CTD_SW_CLEAR_CTDS_COR_Pos          5UL
#define CTBM_CTD_SW_CLEAR_CTDS_COR_Msk          0x20UL
#define CTBM_CTD_SW_CLEAR_CTDO_C6H_Pos          8UL
#define CTBM_CTD_SW_CLEAR_CTDO_C6H_Msk          0x100UL
#define CTBM_CTD_SW_CLEAR_CTDO_COS_Pos          9UL
#define CTBM_CTD_SW_CLEAR_CTDO_COS_Msk          0x200UL
#define CTBM_CTD_SW_CLEAR_CTDH_COB_Pos          10UL
#define CTBM_CTD_SW_CLEAR_CTDH_COB_Msk          0x400UL
#define CTBM_CTD_SW_CLEAR_CTDH_CHD_Pos          12UL
#define CTBM_CTD_SW_CLEAR_CTDH_CHD_Msk          0x1000UL
#define CTBM_CTD_SW_CLEAR_CTDH_CA0_Pos          13UL
#define CTBM_CTD_SW_CLEAR_CTDH_CA0_Msk          0x2000UL
#define CTBM_CTD_SW_CLEAR_CTDH_CIS_Pos          14UL
#define CTBM_CTD_SW_CLEAR_CTDH_CIS_Msk          0x4000UL
#define CTBM_CTD_SW_CLEAR_CTDH_ILR_Pos          15UL
#define CTBM_CTD_SW_CLEAR_CTDH_ILR_Msk          0x8000UL
/* CTBM.CTB_SW_DS_CTRL */
#define CTBM_CTB_SW_DS_CTRL_P2_DS_CTRL23_Pos    10UL
#define CTBM_CTB_SW_DS_CTRL_P2_DS_CTRL23_Msk    0x400UL
#define CTBM_CTB_SW_DS_CTRL_P3_DS_CTRL23_Pos    11UL
#define CTBM_CTB_SW_DS_CTRL_P3_DS_CTRL23_Msk    0x800UL
#define CTBM_CTB_SW_DS_CTRL_CTD_COS_DS_CTRL_Pos 31UL
#define CTBM_CTB_SW_DS_CTRL_CTD_COS_DS_CTRL_Msk 0x80000000UL
/* CTBM.CTB_SW_SQ_CTRL */
#define CTBM_CTB_SW_SQ_CTRL_P2_SQ_CTRL23_Pos    10UL
#define CTBM_CTB_SW_SQ_CTRL_P2_SQ_CTRL23_Msk    0x400UL
#define CTBM_CTB_SW_SQ_CTRL_P3_SQ_CTRL23_Pos    11UL
#define CTBM_CTB_SW_SQ_CTRL_P3_SQ_CTRL23_Msk    0x800UL
/* CTBM.CTB_SW_STATUS */
#define CTBM_CTB_SW_STATUS_OA0O_D51_STAT_Pos    28UL
#define CTBM_CTB_SW_STATUS_OA0O_D51_STAT_Msk    0x10000000UL
#define CTBM_CTB_SW_STATUS_OA1O_D52_STAT_Pos    29UL
#define CTBM_CTB_SW_STATUS_OA1O_D52_STAT_Msk    0x20000000UL
#define CTBM_CTB_SW_STATUS_OA1O_D62_STAT_Pos    30UL
#define CTBM_CTB_SW_STATUS_OA1O_D62_STAT_Msk    0x40000000UL
#define CTBM_CTB_SW_STATUS_CTD_COS_STAT_Pos     31UL
#define CTBM_CTB_SW_STATUS_CTD_COS_STAT_Msk     0x80000000UL
/* CTBM.OA0_OFFSET_TRIM */
#define CTBM_OA0_OFFSET_TRIM_OA0_OFFSET_TRIM_Pos 0UL
#define CTBM_OA0_OFFSET_TRIM_OA0_OFFSET_TRIM_Msk 0x3FUL
/* CTBM.OA0_SLOPE_OFFSET_TRIM */
#define CTBM_OA0_SLOPE_OFFSET_TRIM_OA0_SLOPE_OFFSET_TRIM_Pos 0UL
#define CTBM_OA0_SLOPE_OFFSET_TRIM_OA0_SLOPE_OFFSET_TRIM_Msk 0x3FUL
/* CTBM.OA0_COMP_TRIM */
#define CTBM_OA0_COMP_TRIM_OA0_COMP_TRIM_Pos    0UL
#define CTBM_OA0_COMP_TRIM_OA0_COMP_TRIM_Msk    0x3UL
/* CTBM.OA1_OFFSET_TRIM */
#define CTBM_OA1_OFFSET_TRIM_OA1_OFFSET_TRIM_Pos 0UL
#define CTBM_OA1_OFFSET_TRIM_OA1_OFFSET_TRIM_Msk 0x3FUL
/* CTBM.OA1_SLOPE_OFFSET_TRIM */
#define CTBM_OA1_SLOPE_OFFSET_TRIM_OA1_SLOPE_OFFSET_TRIM_Pos 0UL
#define CTBM_OA1_SLOPE_OFFSET_TRIM_OA1_SLOPE_OFFSET_TRIM_Msk 0x3FUL
/* CTBM.OA1_COMP_TRIM */
#define CTBM_OA1_COMP_TRIM_OA1_COMP_TRIM_Pos    0UL
#define CTBM_OA1_COMP_TRIM_OA1_COMP_TRIM_Msk    0x3UL


#endif /* _CYIP_CTBM_H_ */


/* [] END OF FILE */
