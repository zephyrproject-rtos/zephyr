/***************************************************************************//**
* \file cyip_csd.h
*
* \brief
* CSD IP definitions
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

#ifndef _CYIP_CSD_H_
#define _CYIP_CSD_H_

#include "cyip_headers.h"

/*******************************************************************************
*                                     CSD
*******************************************************************************/

#define CSD_SECTION_SIZE                        0x00001000UL

/**
  * \brief Capsense Controller (CSD)
  */
typedef struct {
  __IOM uint32_t CONFIG;                        /*!< 0x00000000 Configuration and Control */
  __IOM uint32_t SPARE;                         /*!< 0x00000004 Spare MMIO */
   __IM uint32_t RESERVED[30];
   __IM uint32_t STATUS;                        /*!< 0x00000080 Status Register */
   __IM uint32_t STAT_SEQ;                      /*!< 0x00000084 Current Sequencer status */
   __IM uint32_t STAT_CNTS;                     /*!< 0x00000088 Current status counts */
   __IM uint32_t STAT_HCNT;                     /*!< 0x0000008C Current count of the HSCMP counter */
   __IM uint32_t RESERVED1[16];
   __IM uint32_t RESULT_VAL1;                   /*!< 0x000000D0 Result CSD/CSX accumulation counter value 1 */
   __IM uint32_t RESULT_VAL2;                   /*!< 0x000000D4 Result CSX accumulation counter value 2 */
   __IM uint32_t RESERVED2[2];
   __IM uint32_t ADC_RES;                       /*!< 0x000000E0 ADC measurement */
   __IM uint32_t RESERVED3[3];
  __IOM uint32_t INTR;                          /*!< 0x000000F0 CSD Interrupt Request Register */
  __IOM uint32_t INTR_SET;                      /*!< 0x000000F4 CSD Interrupt set register */
  __IOM uint32_t INTR_MASK;                     /*!< 0x000000F8 CSD Interrupt mask register */
   __IM uint32_t INTR_MASKED;                   /*!< 0x000000FC CSD Interrupt masked register */
   __IM uint32_t RESERVED4[32];
  __IOM uint32_t HSCMP;                         /*!< 0x00000180 High Speed Comparator configuration */
  __IOM uint32_t AMBUF;                         /*!< 0x00000184 Reference Generator configuration */
  __IOM uint32_t REFGEN;                        /*!< 0x00000188 Reference Generator configuration */
  __IOM uint32_t CSDCMP;                        /*!< 0x0000018C CSD Comparator configuration */
   __IM uint32_t RESERVED5[24];
  __IOM uint32_t SW_RES;                        /*!< 0x000001F0 Switch Resistance configuration */
   __IM uint32_t RESERVED6[3];
  __IOM uint32_t SENSE_PERIOD;                  /*!< 0x00000200 Sense clock period */
  __IOM uint32_t SENSE_DUTY;                    /*!< 0x00000204 Sense clock duty cycle */
   __IM uint32_t RESERVED7[30];
  __IOM uint32_t SW_HS_P_SEL;                   /*!< 0x00000280 HSCMP Pos input switch Waveform selection */
  __IOM uint32_t SW_HS_N_SEL;                   /*!< 0x00000284 HSCMP Neg input switch Waveform selection */
  __IOM uint32_t SW_SHIELD_SEL;                 /*!< 0x00000288 Shielding switches Waveform selection */
   __IM uint32_t RESERVED8;
  __IOM uint32_t SW_AMUXBUF_SEL;                /*!< 0x00000290 Amuxbuffer switches Waveform selection */
  __IOM uint32_t SW_BYP_SEL;                    /*!< 0x00000294 AMUXBUS bypass switches Waveform selection */
   __IM uint32_t RESERVED9[2];
  __IOM uint32_t SW_CMP_P_SEL;                  /*!< 0x000002A0 CSDCMP Pos Switch Waveform selection */
  __IOM uint32_t SW_CMP_N_SEL;                  /*!< 0x000002A4 CSDCMP Neg Switch Waveform selection */
  __IOM uint32_t SW_REFGEN_SEL;                 /*!< 0x000002A8 Reference Generator Switch Waveform selection */
   __IM uint32_t RESERVED10;
  __IOM uint32_t SW_FW_MOD_SEL;                 /*!< 0x000002B0 Full Wave Cmod Switch Waveform selection */
  __IOM uint32_t SW_FW_TANK_SEL;                /*!< 0x000002B4 Full Wave Csh_tank Switch Waveform selection */
   __IM uint32_t RESERVED11[2];
  __IOM uint32_t SW_DSI_SEL;                    /*!< 0x000002C0 DSI output switch control Waveform selection */
   __IM uint32_t RESERVED12[3];
  __IOM uint32_t IO_SEL;                        /*!< 0x000002D0 IO output control Waveform selection */
   __IM uint32_t RESERVED13[11];
  __IOM uint32_t SEQ_TIME;                      /*!< 0x00000300 Sequencer Timing */
   __IM uint32_t RESERVED14[3];
  __IOM uint32_t SEQ_INIT_CNT;                  /*!< 0x00000310 Sequencer Initial conversion and sample counts */
  __IOM uint32_t SEQ_NORM_CNT;                  /*!< 0x00000314 Sequencer Normal conversion and sample counts */
   __IM uint32_t RESERVED15[2];
  __IOM uint32_t ADC_CTL;                       /*!< 0x00000320 ADC Control */
   __IM uint32_t RESERVED16[7];
  __IOM uint32_t SEQ_START;                     /*!< 0x00000340 Sequencer start */
   __IM uint32_t RESERVED17[47];
  __IOM uint32_t IDACA;                         /*!< 0x00000400 IDACA Configuration */
   __IM uint32_t RESERVED18[63];
  __IOM uint32_t IDACB;                         /*!< 0x00000500 IDACB Configuration */
} CSD_V1_Type;                                  /*!< Size = 1284 (0x504) */


/* CSD.CONFIG */
#define CSD_CONFIG_IREF_SEL_Pos                 0UL
#define CSD_CONFIG_IREF_SEL_Msk                 0x1UL
#define CSD_CONFIG_FILTER_DELAY_Pos             4UL
#define CSD_CONFIG_FILTER_DELAY_Msk             0x1F0UL
#define CSD_CONFIG_SHIELD_DELAY_Pos             10UL
#define CSD_CONFIG_SHIELD_DELAY_Msk             0xC00UL
#define CSD_CONFIG_SENSE_EN_Pos                 12UL
#define CSD_CONFIG_SENSE_EN_Msk                 0x1000UL
#define CSD_CONFIG_FULL_WAVE_Pos                17UL
#define CSD_CONFIG_FULL_WAVE_Msk                0x20000UL
#define CSD_CONFIG_MUTUAL_CAP_Pos               18UL
#define CSD_CONFIG_MUTUAL_CAP_Msk               0x40000UL
#define CSD_CONFIG_CSX_DUAL_CNT_Pos             19UL
#define CSD_CONFIG_CSX_DUAL_CNT_Msk             0x80000UL
#define CSD_CONFIG_DSI_COUNT_SEL_Pos            24UL
#define CSD_CONFIG_DSI_COUNT_SEL_Msk            0x1000000UL
#define CSD_CONFIG_DSI_SAMPLE_EN_Pos            25UL
#define CSD_CONFIG_DSI_SAMPLE_EN_Msk            0x2000000UL
#define CSD_CONFIG_SAMPLE_SYNC_Pos              26UL
#define CSD_CONFIG_SAMPLE_SYNC_Msk              0x4000000UL
#define CSD_CONFIG_DSI_SENSE_EN_Pos             27UL
#define CSD_CONFIG_DSI_SENSE_EN_Msk             0x8000000UL
#define CSD_CONFIG_LP_MODE_Pos                  30UL
#define CSD_CONFIG_LP_MODE_Msk                  0x40000000UL
#define CSD_CONFIG_ENABLE_Pos                   31UL
#define CSD_CONFIG_ENABLE_Msk                   0x80000000UL
/* CSD.SPARE */
#define CSD_SPARE_SPARE_Pos                     0UL
#define CSD_SPARE_SPARE_Msk                     0xFUL
/* CSD.STATUS */
#define CSD_STATUS_CSD_SENSE_Pos                1UL
#define CSD_STATUS_CSD_SENSE_Msk                0x2UL
#define CSD_STATUS_HSCMP_OUT_Pos                2UL
#define CSD_STATUS_HSCMP_OUT_Msk                0x4UL
#define CSD_STATUS_CSDCMP_OUT_Pos               3UL
#define CSD_STATUS_CSDCMP_OUT_Msk               0x8UL
/* CSD.STAT_SEQ */
#define CSD_STAT_SEQ_SEQ_STATE_Pos              0UL
#define CSD_STAT_SEQ_SEQ_STATE_Msk              0x7UL
#define CSD_STAT_SEQ_ADC_STATE_Pos              16UL
#define CSD_STAT_SEQ_ADC_STATE_Msk              0x70000UL
/* CSD.STAT_CNTS */
#define CSD_STAT_CNTS_NUM_CONV_Pos              0UL
#define CSD_STAT_CNTS_NUM_CONV_Msk              0xFFFFUL
/* CSD.STAT_HCNT */
#define CSD_STAT_HCNT_CNT_Pos                   0UL
#define CSD_STAT_HCNT_CNT_Msk                   0xFFFFUL
/* CSD.RESULT_VAL1 */
#define CSD_RESULT_VAL1_VALUE_Pos               0UL
#define CSD_RESULT_VAL1_VALUE_Msk               0xFFFFUL
#define CSD_RESULT_VAL1_BAD_CONVS_Pos           16UL
#define CSD_RESULT_VAL1_BAD_CONVS_Msk           0xFF0000UL
/* CSD.RESULT_VAL2 */
#define CSD_RESULT_VAL2_VALUE_Pos               0UL
#define CSD_RESULT_VAL2_VALUE_Msk               0xFFFFUL
/* CSD.ADC_RES */
#define CSD_ADC_RES_VIN_CNT_Pos                 0UL
#define CSD_ADC_RES_VIN_CNT_Msk                 0xFFFFUL
#define CSD_ADC_RES_HSCMP_POL_Pos               16UL
#define CSD_ADC_RES_HSCMP_POL_Msk               0x10000UL
#define CSD_ADC_RES_ADC_OVERFLOW_Pos            30UL
#define CSD_ADC_RES_ADC_OVERFLOW_Msk            0x40000000UL
#define CSD_ADC_RES_ADC_ABORT_Pos               31UL
#define CSD_ADC_RES_ADC_ABORT_Msk               0x80000000UL
/* CSD.INTR */
#define CSD_INTR_SAMPLE_Pos                     1UL
#define CSD_INTR_SAMPLE_Msk                     0x2UL
#define CSD_INTR_INIT_Pos                       2UL
#define CSD_INTR_INIT_Msk                       0x4UL
#define CSD_INTR_ADC_RES_Pos                    8UL
#define CSD_INTR_ADC_RES_Msk                    0x100UL
/* CSD.INTR_SET */
#define CSD_INTR_SET_SAMPLE_Pos                 1UL
#define CSD_INTR_SET_SAMPLE_Msk                 0x2UL
#define CSD_INTR_SET_INIT_Pos                   2UL
#define CSD_INTR_SET_INIT_Msk                   0x4UL
#define CSD_INTR_SET_ADC_RES_Pos                8UL
#define CSD_INTR_SET_ADC_RES_Msk                0x100UL
/* CSD.INTR_MASK */
#define CSD_INTR_MASK_SAMPLE_Pos                1UL
#define CSD_INTR_MASK_SAMPLE_Msk                0x2UL
#define CSD_INTR_MASK_INIT_Pos                  2UL
#define CSD_INTR_MASK_INIT_Msk                  0x4UL
#define CSD_INTR_MASK_ADC_RES_Pos               8UL
#define CSD_INTR_MASK_ADC_RES_Msk               0x100UL
/* CSD.INTR_MASKED */
#define CSD_INTR_MASKED_SAMPLE_Pos              1UL
#define CSD_INTR_MASKED_SAMPLE_Msk              0x2UL
#define CSD_INTR_MASKED_INIT_Pos                2UL
#define CSD_INTR_MASKED_INIT_Msk                0x4UL
#define CSD_INTR_MASKED_ADC_RES_Pos             8UL
#define CSD_INTR_MASKED_ADC_RES_Msk             0x100UL
/* CSD.HSCMP */
#define CSD_HSCMP_HSCMP_EN_Pos                  0UL
#define CSD_HSCMP_HSCMP_EN_Msk                  0x1UL
#define CSD_HSCMP_HSCMP_INVERT_Pos              4UL
#define CSD_HSCMP_HSCMP_INVERT_Msk              0x10UL
#define CSD_HSCMP_AZ_EN_Pos                     31UL
#define CSD_HSCMP_AZ_EN_Msk                     0x80000000UL
/* CSD.AMBUF */
#define CSD_AMBUF_PWR_MODE_Pos                  0UL
#define CSD_AMBUF_PWR_MODE_Msk                  0x3UL
/* CSD.REFGEN */
#define CSD_REFGEN_REFGEN_EN_Pos                0UL
#define CSD_REFGEN_REFGEN_EN_Msk                0x1UL
#define CSD_REFGEN_BYPASS_Pos                   4UL
#define CSD_REFGEN_BYPASS_Msk                   0x10UL
#define CSD_REFGEN_VDDA_EN_Pos                  5UL
#define CSD_REFGEN_VDDA_EN_Msk                  0x20UL
#define CSD_REFGEN_RES_EN_Pos                   6UL
#define CSD_REFGEN_RES_EN_Msk                   0x40UL
#define CSD_REFGEN_GAIN_Pos                     8UL
#define CSD_REFGEN_GAIN_Msk                     0x1F00UL
#define CSD_REFGEN_VREFLO_SEL_Pos               16UL
#define CSD_REFGEN_VREFLO_SEL_Msk               0x1F0000UL
#define CSD_REFGEN_VREFLO_INT_Pos               23UL
#define CSD_REFGEN_VREFLO_INT_Msk               0x800000UL
/* CSD.CSDCMP */
#define CSD_CSDCMP_CSDCMP_EN_Pos                0UL
#define CSD_CSDCMP_CSDCMP_EN_Msk                0x1UL
#define CSD_CSDCMP_POLARITY_SEL_Pos             4UL
#define CSD_CSDCMP_POLARITY_SEL_Msk             0x30UL
#define CSD_CSDCMP_CMP_PHASE_Pos                8UL
#define CSD_CSDCMP_CMP_PHASE_Msk                0x300UL
#define CSD_CSDCMP_CMP_MODE_Pos                 28UL
#define CSD_CSDCMP_CMP_MODE_Msk                 0x10000000UL
#define CSD_CSDCMP_FEEDBACK_MODE_Pos            29UL
#define CSD_CSDCMP_FEEDBACK_MODE_Msk            0x20000000UL
#define CSD_CSDCMP_AZ_EN_Pos                    31UL
#define CSD_CSDCMP_AZ_EN_Msk                    0x80000000UL
/* CSD.SW_RES */
#define CSD_SW_RES_RES_HCAV_Pos                 0UL
#define CSD_SW_RES_RES_HCAV_Msk                 0x3UL
#define CSD_SW_RES_RES_HCAG_Pos                 2UL
#define CSD_SW_RES_RES_HCAG_Msk                 0xCUL
#define CSD_SW_RES_RES_HCBV_Pos                 4UL
#define CSD_SW_RES_RES_HCBV_Msk                 0x30UL
#define CSD_SW_RES_RES_HCBG_Pos                 6UL
#define CSD_SW_RES_RES_HCBG_Msk                 0xC0UL
#define CSD_SW_RES_RES_F1PM_Pos                 16UL
#define CSD_SW_RES_RES_F1PM_Msk                 0x30000UL
#define CSD_SW_RES_RES_F2PT_Pos                 18UL
#define CSD_SW_RES_RES_F2PT_Msk                 0xC0000UL
/* CSD.SENSE_PERIOD */
#define CSD_SENSE_PERIOD_SENSE_DIV_Pos          0UL
#define CSD_SENSE_PERIOD_SENSE_DIV_Msk          0xFFFUL
#define CSD_SENSE_PERIOD_LFSR_SIZE_Pos          16UL
#define CSD_SENSE_PERIOD_LFSR_SIZE_Msk          0x70000UL
#define CSD_SENSE_PERIOD_LFSR_SCALE_Pos         20UL
#define CSD_SENSE_PERIOD_LFSR_SCALE_Msk         0xF00000UL
#define CSD_SENSE_PERIOD_LFSR_CLEAR_Pos         24UL
#define CSD_SENSE_PERIOD_LFSR_CLEAR_Msk         0x1000000UL
#define CSD_SENSE_PERIOD_SEL_LFSR_MSB_Pos       25UL
#define CSD_SENSE_PERIOD_SEL_LFSR_MSB_Msk       0x2000000UL
#define CSD_SENSE_PERIOD_LFSR_BITS_Pos          26UL
#define CSD_SENSE_PERIOD_LFSR_BITS_Msk          0xC000000UL
/* CSD.SENSE_DUTY */
#define CSD_SENSE_DUTY_SENSE_WIDTH_Pos          0UL
#define CSD_SENSE_DUTY_SENSE_WIDTH_Msk          0xFFFUL
#define CSD_SENSE_DUTY_SENSE_POL_Pos            16UL
#define CSD_SENSE_DUTY_SENSE_POL_Msk            0x10000UL
#define CSD_SENSE_DUTY_OVERLAP_PHI1_Pos         18UL
#define CSD_SENSE_DUTY_OVERLAP_PHI1_Msk         0x40000UL
#define CSD_SENSE_DUTY_OVERLAP_PHI2_Pos         19UL
#define CSD_SENSE_DUTY_OVERLAP_PHI2_Msk         0x80000UL
/* CSD.SW_HS_P_SEL */
#define CSD_SW_HS_P_SEL_SW_HMPM_Pos             0UL
#define CSD_SW_HS_P_SEL_SW_HMPM_Msk             0x1UL
#define CSD_SW_HS_P_SEL_SW_HMPT_Pos             4UL
#define CSD_SW_HS_P_SEL_SW_HMPT_Msk             0x10UL
#define CSD_SW_HS_P_SEL_SW_HMPS_Pos             8UL
#define CSD_SW_HS_P_SEL_SW_HMPS_Msk             0x100UL
#define CSD_SW_HS_P_SEL_SW_HMMA_Pos             12UL
#define CSD_SW_HS_P_SEL_SW_HMMA_Msk             0x1000UL
#define CSD_SW_HS_P_SEL_SW_HMMB_Pos             16UL
#define CSD_SW_HS_P_SEL_SW_HMMB_Msk             0x10000UL
#define CSD_SW_HS_P_SEL_SW_HMCA_Pos             20UL
#define CSD_SW_HS_P_SEL_SW_HMCA_Msk             0x100000UL
#define CSD_SW_HS_P_SEL_SW_HMCB_Pos             24UL
#define CSD_SW_HS_P_SEL_SW_HMCB_Msk             0x1000000UL
#define CSD_SW_HS_P_SEL_SW_HMRH_Pos             28UL
#define CSD_SW_HS_P_SEL_SW_HMRH_Msk             0x10000000UL
/* CSD.SW_HS_N_SEL */
#define CSD_SW_HS_N_SEL_SW_HCCC_Pos             16UL
#define CSD_SW_HS_N_SEL_SW_HCCC_Msk             0x10000UL
#define CSD_SW_HS_N_SEL_SW_HCCD_Pos             20UL
#define CSD_SW_HS_N_SEL_SW_HCCD_Msk             0x100000UL
#define CSD_SW_HS_N_SEL_SW_HCRH_Pos             24UL
#define CSD_SW_HS_N_SEL_SW_HCRH_Msk             0x7000000UL
#define CSD_SW_HS_N_SEL_SW_HCRL_Pos             28UL
#define CSD_SW_HS_N_SEL_SW_HCRL_Msk             0x70000000UL
/* CSD.SW_SHIELD_SEL */
#define CSD_SW_SHIELD_SEL_SW_HCAV_Pos           0UL
#define CSD_SW_SHIELD_SEL_SW_HCAV_Msk           0x7UL
#define CSD_SW_SHIELD_SEL_SW_HCAG_Pos           4UL
#define CSD_SW_SHIELD_SEL_SW_HCAG_Msk           0x70UL
#define CSD_SW_SHIELD_SEL_SW_HCBV_Pos           8UL
#define CSD_SW_SHIELD_SEL_SW_HCBV_Msk           0x700UL
#define CSD_SW_SHIELD_SEL_SW_HCBG_Pos           12UL
#define CSD_SW_SHIELD_SEL_SW_HCBG_Msk           0x7000UL
#define CSD_SW_SHIELD_SEL_SW_HCCV_Pos           16UL
#define CSD_SW_SHIELD_SEL_SW_HCCV_Msk           0x10000UL
#define CSD_SW_SHIELD_SEL_SW_HCCG_Pos           20UL
#define CSD_SW_SHIELD_SEL_SW_HCCG_Msk           0x100000UL
/* CSD.SW_AMUXBUF_SEL */
#define CSD_SW_AMUXBUF_SEL_SW_IRBY_Pos          4UL
#define CSD_SW_AMUXBUF_SEL_SW_IRBY_Msk          0x10UL
#define CSD_SW_AMUXBUF_SEL_SW_IRLB_Pos          8UL
#define CSD_SW_AMUXBUF_SEL_SW_IRLB_Msk          0x100UL
#define CSD_SW_AMUXBUF_SEL_SW_ICA_Pos           12UL
#define CSD_SW_AMUXBUF_SEL_SW_ICA_Msk           0x1000UL
#define CSD_SW_AMUXBUF_SEL_SW_ICB_Pos           16UL
#define CSD_SW_AMUXBUF_SEL_SW_ICB_Msk           0x70000UL
#define CSD_SW_AMUXBUF_SEL_SW_IRLI_Pos          20UL
#define CSD_SW_AMUXBUF_SEL_SW_IRLI_Msk          0x100000UL
#define CSD_SW_AMUXBUF_SEL_SW_IRH_Pos           24UL
#define CSD_SW_AMUXBUF_SEL_SW_IRH_Msk           0x1000000UL
#define CSD_SW_AMUXBUF_SEL_SW_IRL_Pos           28UL
#define CSD_SW_AMUXBUF_SEL_SW_IRL_Msk           0x10000000UL
/* CSD.SW_BYP_SEL */
#define CSD_SW_BYP_SEL_SW_BYA_Pos               12UL
#define CSD_SW_BYP_SEL_SW_BYA_Msk               0x1000UL
#define CSD_SW_BYP_SEL_SW_BYB_Pos               16UL
#define CSD_SW_BYP_SEL_SW_BYB_Msk               0x10000UL
#define CSD_SW_BYP_SEL_SW_CBCC_Pos              20UL
#define CSD_SW_BYP_SEL_SW_CBCC_Msk              0x100000UL
/* CSD.SW_CMP_P_SEL */
#define CSD_SW_CMP_P_SEL_SW_SFPM_Pos            0UL
#define CSD_SW_CMP_P_SEL_SW_SFPM_Msk            0x7UL
#define CSD_SW_CMP_P_SEL_SW_SFPT_Pos            4UL
#define CSD_SW_CMP_P_SEL_SW_SFPT_Msk            0x70UL
#define CSD_SW_CMP_P_SEL_SW_SFPS_Pos            8UL
#define CSD_SW_CMP_P_SEL_SW_SFPS_Msk            0x700UL
#define CSD_SW_CMP_P_SEL_SW_SFMA_Pos            12UL
#define CSD_SW_CMP_P_SEL_SW_SFMA_Msk            0x1000UL
#define CSD_SW_CMP_P_SEL_SW_SFMB_Pos            16UL
#define CSD_SW_CMP_P_SEL_SW_SFMB_Msk            0x10000UL
#define CSD_SW_CMP_P_SEL_SW_SFCA_Pos            20UL
#define CSD_SW_CMP_P_SEL_SW_SFCA_Msk            0x100000UL
#define CSD_SW_CMP_P_SEL_SW_SFCB_Pos            24UL
#define CSD_SW_CMP_P_SEL_SW_SFCB_Msk            0x1000000UL
/* CSD.SW_CMP_N_SEL */
#define CSD_SW_CMP_N_SEL_SW_SCRH_Pos            24UL
#define CSD_SW_CMP_N_SEL_SW_SCRH_Msk            0x7000000UL
#define CSD_SW_CMP_N_SEL_SW_SCRL_Pos            28UL
#define CSD_SW_CMP_N_SEL_SW_SCRL_Msk            0x70000000UL
/* CSD.SW_REFGEN_SEL */
#define CSD_SW_REFGEN_SEL_SW_IAIB_Pos           0UL
#define CSD_SW_REFGEN_SEL_SW_IAIB_Msk           0x1UL
#define CSD_SW_REFGEN_SEL_SW_IBCB_Pos           4UL
#define CSD_SW_REFGEN_SEL_SW_IBCB_Msk           0x10UL
#define CSD_SW_REFGEN_SEL_SW_SGMB_Pos           16UL
#define CSD_SW_REFGEN_SEL_SW_SGMB_Msk           0x10000UL
#define CSD_SW_REFGEN_SEL_SW_SGRP_Pos           20UL
#define CSD_SW_REFGEN_SEL_SW_SGRP_Msk           0x100000UL
#define CSD_SW_REFGEN_SEL_SW_SGRE_Pos           24UL
#define CSD_SW_REFGEN_SEL_SW_SGRE_Msk           0x1000000UL
#define CSD_SW_REFGEN_SEL_SW_SGR_Pos            28UL
#define CSD_SW_REFGEN_SEL_SW_SGR_Msk            0x10000000UL
/* CSD.SW_FW_MOD_SEL */
#define CSD_SW_FW_MOD_SEL_SW_F1PM_Pos           0UL
#define CSD_SW_FW_MOD_SEL_SW_F1PM_Msk           0x1UL
#define CSD_SW_FW_MOD_SEL_SW_F1MA_Pos           8UL
#define CSD_SW_FW_MOD_SEL_SW_F1MA_Msk           0x700UL
#define CSD_SW_FW_MOD_SEL_SW_F1CA_Pos           16UL
#define CSD_SW_FW_MOD_SEL_SW_F1CA_Msk           0x70000UL
#define CSD_SW_FW_MOD_SEL_SW_C1CC_Pos           20UL
#define CSD_SW_FW_MOD_SEL_SW_C1CC_Msk           0x100000UL
#define CSD_SW_FW_MOD_SEL_SW_C1CD_Pos           24UL
#define CSD_SW_FW_MOD_SEL_SW_C1CD_Msk           0x1000000UL
#define CSD_SW_FW_MOD_SEL_SW_C1F1_Pos           28UL
#define CSD_SW_FW_MOD_SEL_SW_C1F1_Msk           0x10000000UL
/* CSD.SW_FW_TANK_SEL */
#define CSD_SW_FW_TANK_SEL_SW_F2PT_Pos          4UL
#define CSD_SW_FW_TANK_SEL_SW_F2PT_Msk          0x10UL
#define CSD_SW_FW_TANK_SEL_SW_F2MA_Pos          8UL
#define CSD_SW_FW_TANK_SEL_SW_F2MA_Msk          0x700UL
#define CSD_SW_FW_TANK_SEL_SW_F2CA_Pos          12UL
#define CSD_SW_FW_TANK_SEL_SW_F2CA_Msk          0x7000UL
#define CSD_SW_FW_TANK_SEL_SW_F2CB_Pos          16UL
#define CSD_SW_FW_TANK_SEL_SW_F2CB_Msk          0x70000UL
#define CSD_SW_FW_TANK_SEL_SW_C2CC_Pos          20UL
#define CSD_SW_FW_TANK_SEL_SW_C2CC_Msk          0x100000UL
#define CSD_SW_FW_TANK_SEL_SW_C2CD_Pos          24UL
#define CSD_SW_FW_TANK_SEL_SW_C2CD_Msk          0x1000000UL
#define CSD_SW_FW_TANK_SEL_SW_C2F2_Pos          28UL
#define CSD_SW_FW_TANK_SEL_SW_C2F2_Msk          0x10000000UL
/* CSD.SW_DSI_SEL */
#define CSD_SW_DSI_SEL_DSI_CSH_TANK_Pos         0UL
#define CSD_SW_DSI_SEL_DSI_CSH_TANK_Msk         0xFUL
#define CSD_SW_DSI_SEL_DSI_CMOD_Pos             4UL
#define CSD_SW_DSI_SEL_DSI_CMOD_Msk             0xF0UL
/* CSD.IO_SEL */
#define CSD_IO_SEL_CSD_TX_OUT_Pos               0UL
#define CSD_IO_SEL_CSD_TX_OUT_Msk               0xFUL
#define CSD_IO_SEL_CSD_TX_OUT_EN_Pos            4UL
#define CSD_IO_SEL_CSD_TX_OUT_EN_Msk            0xF0UL
#define CSD_IO_SEL_CSD_TX_AMUXB_EN_Pos          12UL
#define CSD_IO_SEL_CSD_TX_AMUXB_EN_Msk          0xF000UL
#define CSD_IO_SEL_CSD_TX_N_OUT_Pos             16UL
#define CSD_IO_SEL_CSD_TX_N_OUT_Msk             0xF0000UL
#define CSD_IO_SEL_CSD_TX_N_OUT_EN_Pos          20UL
#define CSD_IO_SEL_CSD_TX_N_OUT_EN_Msk          0xF00000UL
#define CSD_IO_SEL_CSD_TX_N_AMUXA_EN_Pos        24UL
#define CSD_IO_SEL_CSD_TX_N_AMUXA_EN_Msk        0xF000000UL
/* CSD.SEQ_TIME */
#define CSD_SEQ_TIME_AZ_TIME_Pos                0UL
#define CSD_SEQ_TIME_AZ_TIME_Msk                0xFFUL
/* CSD.SEQ_INIT_CNT */
#define CSD_SEQ_INIT_CNT_CONV_CNT_Pos           0UL
#define CSD_SEQ_INIT_CNT_CONV_CNT_Msk           0xFFFFUL
/* CSD.SEQ_NORM_CNT */
#define CSD_SEQ_NORM_CNT_CONV_CNT_Pos           0UL
#define CSD_SEQ_NORM_CNT_CONV_CNT_Msk           0xFFFFUL
/* CSD.ADC_CTL */
#define CSD_ADC_CTL_ADC_TIME_Pos                0UL
#define CSD_ADC_CTL_ADC_TIME_Msk                0xFFUL
#define CSD_ADC_CTL_ADC_MODE_Pos                16UL
#define CSD_ADC_CTL_ADC_MODE_Msk                0x30000UL
/* CSD.SEQ_START */
#define CSD_SEQ_START_START_Pos                 0UL
#define CSD_SEQ_START_START_Msk                 0x1UL
#define CSD_SEQ_START_SEQ_MODE_Pos              1UL
#define CSD_SEQ_START_SEQ_MODE_Msk              0x2UL
#define CSD_SEQ_START_ABORT_Pos                 3UL
#define CSD_SEQ_START_ABORT_Msk                 0x8UL
#define CSD_SEQ_START_DSI_START_EN_Pos          4UL
#define CSD_SEQ_START_DSI_START_EN_Msk          0x10UL
#define CSD_SEQ_START_AZ0_SKIP_Pos              8UL
#define CSD_SEQ_START_AZ0_SKIP_Msk              0x100UL
#define CSD_SEQ_START_AZ1_SKIP_Pos              9UL
#define CSD_SEQ_START_AZ1_SKIP_Msk              0x200UL
/* CSD.IDACA */
#define CSD_IDACA_VAL_Pos                       0UL
#define CSD_IDACA_VAL_Msk                       0x7FUL
#define CSD_IDACA_POL_DYN_Pos                   7UL
#define CSD_IDACA_POL_DYN_Msk                   0x80UL
#define CSD_IDACA_POLARITY_Pos                  8UL
#define CSD_IDACA_POLARITY_Msk                  0x300UL
#define CSD_IDACA_BAL_MODE_Pos                  10UL
#define CSD_IDACA_BAL_MODE_Msk                  0xC00UL
#define CSD_IDACA_LEG1_MODE_Pos                 16UL
#define CSD_IDACA_LEG1_MODE_Msk                 0x30000UL
#define CSD_IDACA_LEG2_MODE_Pos                 18UL
#define CSD_IDACA_LEG2_MODE_Msk                 0xC0000UL
#define CSD_IDACA_DSI_CTRL_EN_Pos               21UL
#define CSD_IDACA_DSI_CTRL_EN_Msk               0x200000UL
#define CSD_IDACA_RANGE_Pos                     22UL
#define CSD_IDACA_RANGE_Msk                     0xC00000UL
#define CSD_IDACA_LEG1_EN_Pos                   24UL
#define CSD_IDACA_LEG1_EN_Msk                   0x1000000UL
#define CSD_IDACA_LEG2_EN_Pos                   25UL
#define CSD_IDACA_LEG2_EN_Msk                   0x2000000UL
/* CSD.IDACB */
#define CSD_IDACB_VAL_Pos                       0UL
#define CSD_IDACB_VAL_Msk                       0x7FUL
#define CSD_IDACB_POL_DYN_Pos                   7UL
#define CSD_IDACB_POL_DYN_Msk                   0x80UL
#define CSD_IDACB_POLARITY_Pos                  8UL
#define CSD_IDACB_POLARITY_Msk                  0x300UL
#define CSD_IDACB_BAL_MODE_Pos                  10UL
#define CSD_IDACB_BAL_MODE_Msk                  0xC00UL
#define CSD_IDACB_LEG1_MODE_Pos                 16UL
#define CSD_IDACB_LEG1_MODE_Msk                 0x30000UL
#define CSD_IDACB_LEG2_MODE_Pos                 18UL
#define CSD_IDACB_LEG2_MODE_Msk                 0xC0000UL
#define CSD_IDACB_DSI_CTRL_EN_Pos               21UL
#define CSD_IDACB_DSI_CTRL_EN_Msk               0x200000UL
#define CSD_IDACB_RANGE_Pos                     22UL
#define CSD_IDACB_RANGE_Msk                     0xC00000UL
#define CSD_IDACB_LEG1_EN_Pos                   24UL
#define CSD_IDACB_LEG1_EN_Msk                   0x1000000UL
#define CSD_IDACB_LEG2_EN_Pos                   25UL
#define CSD_IDACB_LEG2_EN_Msk                   0x2000000UL
#define CSD_IDACB_LEG3_EN_Pos                   26UL
#define CSD_IDACB_LEG3_EN_Msk                   0x4000000UL


#endif /* _CYIP_CSD_H_ */


/* [] END OF FILE */
