/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _FSL_XCVR_H_
/* clang-format off */
#define _FSL_XCVR_H_
/* clang-format on */

#include "fsl_device_registers.h"
#include "fsl_xcvr_trim.h"

#if gMWS_UseCoexistence_d
#include "MWS.h"
#endif /* gMWS_UseCoexistence_d */
/*!
 * @addtogroup xcvr
 * @{
 */

/*! @file*/

/*******************************************************************************
 * Definitions
 ******************************************************************************/
 /* Default RF OSC definition. Allows for compile time clock frequency definition */
#ifdef CLOCK_MAIN

#else
#if RF_OSC_26MHZ == 1
#define CLOCK_MAIN (EXT_CLK_26_MHZ) /* See ext_clock_config_t for this value */
#else
#define CLOCK_MAIN (EXT_CLK_32_MHZ) /* See ext_clock_config_t for this value */
#endif
#endif
 
#define AUX_PLL_DELAY               (0)
#define FSL_XCVR_DRIVER_VERSION (MAKE_VERSION(0, 0, 1))

#define USE_DEFAULT_PRE_REF             (0)
#define TRIM_BBA_DCOC_DAC_AT_INIT       (1)
#define PRESLOW_ENA                     (1)

/* TSM bitfield shift and value definitions */
#define TX_DIG_EN_ASSERT    (95)
#define ZGBE_TX_DIG_EN_ASSERT (TX_DIG_EN_ASSERT-1) /* Zigbee TX_DIG_EN must assert 1 tick sooner, see adjustment below based on data padding */
#define B0(x)   (((uint32_t)(((uint32_t)(x)) << 0)) & 0xFFU)
#define B1(x)   (((uint32_t)(((uint32_t)(x)) << 8)) & 0xFF00U)
#define B2(x)   (((uint32_t)(((uint32_t)(x)) << 16)) & 0xFF0000U)
#define B3(x)   (((uint32_t)(((uint32_t)(x)) << 24)) & 0xFF000000U)
/* EDIT THIS LINE TO CONTROL PA_RAMP! */
#define PA_RAMP_TIME    (2) /* only allowable values are 0, 2, 4, or 8 for PA RAMP times */

#if !((PA_RAMP_TIME==0) || (PA_RAMP_TIME==2) || (PA_RAMP_TIME==4) || (PA_RAMP_TIME==8))
#error "Invalid value for PA_RAMP_TIME macro"
#endif /* Error check of PA RAMP TIME */
#define ADD_FOR_26MHZ   (4)
#define END_OF_TX_WU_NORAMP     (103) /* NOTE: NORAMP and 2us ramp time behaviors are identical for TX WU and WD */
#define END_OF_TX_WD_NORAMP     (111) /* NOTE: NORAMP and 2us ramp time behaviors are identical for TX WU and WD */
/* Redefine the values of END_OF_TX_WU and END_OF_TX_WD based on whether DATA PADDING is enabled and the selection of ramp time */
/* These two constants are then used on both common configuration and mode specific configuration files to define the TSM timing values */
#if ((PA_RAMP_TIME==0) || (PA_RAMP_TIME==2))
   #define END_OF_TX_WU   (END_OF_TX_WU_NORAMP)
   #define END_OF_TX_WD   (END_OF_TX_WD_NORAMP)
   #define TX_SYNTH_DELAY_ADJ (0)
   #define PD_CYCLE_SLIP_TX_HI_ADJ (0)
   #define PD_CYCLE_SLIP_TX_LO_ADJ (1)
   #define ZGBE_TX_DIG_EN_TX_HI_ADJ (-5) /* Only applies to Zigbee mode */
   #if (PA_RAMP_TIME==0)
      #define PA_RAMP_SEL 0
      #define DATA_PADDING_EN (0)
      #define TX_DIG_EN_TX_HI_ADJ (-2)
   #else
      #define PA_RAMP_SEL 1
      #define DATA_PADDING_EN (1)
      #define TX_DIG_EN_TX_HI_ADJ (0)
   #endif
#else /* ((PA_RAMP_TIME==0) || (PA_RAMP_TIME==2)) */
  #if (PA_RAMP_TIME==4)
    #define END_OF_TX_WU    (END_OF_TX_WU_NORAMP+2)
    #define END_OF_TX_WD    (END_OF_TX_WD_NORAMP+4)
    #define TX_SYNTH_DELAY_ADJ (2)
    #define PD_CYCLE_SLIP_TX_HI_ADJ (2)
    #define PD_CYCLE_SLIP_TX_LO_ADJ (1)
    #define TX_DIG_EN_TX_HI_ADJ (0)
    #define ZGBE_TX_DIG_EN_TX_HI_ADJ (-3) /* Only applies to Zigbee mode */
    #define PA_RAMP_SEL 2
    #define DATA_PADDING_EN (1)
  #else /* (PA_RAMP_TIME==4) */
    #if (PA_RAMP_TIME==8)
      #define END_OF_TX_WU    (END_OF_TX_WU_NORAMP+6)
      #define END_OF_TX_WD    (END_OF_TX_WD_NORAMP+12)
      #define TX_SYNTH_DELAY_ADJ (6)
      #define PD_CYCLE_SLIP_TX_HI_ADJ (6)
      #define PD_CYCLE_SLIP_TX_LO_ADJ (1)
      #define TX_DIG_EN_TX_HI_ADJ (4)
      #define ZGBE_TX_DIG_EN_TX_HI_ADJ (1) /* Only applies to Zigbee mode */
      #define PA_RAMP_SEL 3
      #define DATA_PADDING_EN (1)
    #else /* (PA_RAMP_TIME==8) */
      #error "Invalid value for PA_RAMP_TIME macro"
    #endif /* (PA_RAMP_TIME==8) */
  #endif/* (PA_RAMP_TIME==4) */
#endif /* ((PA_RAMP_TIME==0) || (PA_RAMP_TIME==2)) */
#define TX_DIG_EN_ASSERT_MSK500    (END_OF_TX_WU-3)

#define END_OF_RX_WU    (104+AUX_PLL_DELAY)
#if RF_OSC_26MHZ == 1
#define END_OF_RX_WD    (END_OF_RX_WU+1+ADD_FOR_26MHZ)  /* Need to handle normal signals extending when 26MHZ warmdown is extended */
#else
#define END_OF_RX_WD    (END_OF_RX_WU+1)
#endif
#define END_OF_RX_WU_26MHZ  (END_OF_RX_WU+ADD_FOR_26MHZ)
#define END_OF_RX_WD_26MHZ  (END_OF_RX_WU+1+ADD_FOR_26MHZ)

/* PA Bias Table */
#define PA_RAMP_0 0x1
#define PA_RAMP_1 0x2
#define PA_RAMP_2 0x4
#define PA_RAMP_3 0x8
#define PA_RAMP_4 0xe
#define PA_RAMP_5 0x16
#define PA_RAMP_6 0x22
#define PA_RAMP_7 0x2e

/* BLE LL timing definitions */
#define TX_ON_DELAY     (0x85) /* Adjusted TX_ON_DELAY to make turnaround time 150usec */
#define RX_ON_DELAY     (29+END_OF_RX_WU)
#define RX_ON_DELAY_26MHZ     (29+END_OF_RX_WU_26MHZ)
#define TX_RX_ON_DELAY_VAL  (TX_ON_DELAY<<8 | RX_ON_DELAY)
#define TX_RX_ON_DELAY_VAL_26MHZ  (TX_ON_DELAY<<8 | RX_ON_DELAY_26MHZ)
#define TX_SYNTH_DELAY  (TX_ON_DELAY-END_OF_TX_WU-TX_SYNTH_DELAY_ADJ) /* Adjustment to TX_SYNTH_DELAY due to DATA_PADDING */
#define RX_SYNTH_DELAY  (0x18)
#define TX_RX_SYNTH_DELAY_VAL (TX_SYNTH_DELAY<<8 | RX_SYNTH_DELAY)

/* PHY reference waveform assembly */
#define RW0PS(loc,val)      (((val)&0x1F)<<((loc)*5)) /* Ref Word 0 - loc is the phase info symbol number, val is the value of the phase info */
#define RW1PS(loc,val)      (((val)&0x1F)<<(((loc)*5)-32)) /* Ref Word 1 - loc is the phase info symbol number, val is the value of the phase info */
#define RW2PS(loc,val)      (((val)&0x1F)<<(((loc)*5)-64)) /* Ref Word 2 - loc is the phase info symbol number, val is the value of the phase info */

/*! @brief Error codes for the XCVR driver. */
typedef enum _xcvrStatus
{
  gXcvrSuccess_c = 0,
  gXcvrInvalidParameters_c,
  gXcvrUnsupportedOperation_c,
  gXcvrTrimFailure_c
} xcvrStatus_t;

/*!
 * @brief Health status returned from PHY upon status check function return.
 */
typedef enum _healthStatus
{
  NO_ERRORS = 0,
  PLL_CTUNE_FAIL = 1,
  PLL_CYCLE_SLIP_FAIL = 2,
  PLL_FREQ_TARG_FAIL = 4,
  PLL_TSM_ABORT_FAIL = 8,
} healthStatus_t;

/*!
 * @brief Health status returned from PHY upon status check function return.
 */
typedef enum _ext_clock_config
{
  EXT_CLK_32_MHZ = 0,
  EXT_CLK_26_MHZ = 1,
} ext_clock_config_t;

/*!
 * @brief Radio operating mode setting types.
 */
typedef enum _radio_mode
{
  BLE_MODE = 0,
  ZIGBEE_MODE = 1,
  ANT_MODE = 2,

  /* BT=0.5, h=** */
  GFSK_BT_0p5_h_0p5  = 3, /* < BT=0.5, h=0.5 [BLE at 1MBPS data rate; CS4 at 250KBPS data rate] */
  GFSK_BT_0p5_h_0p32 = 4, /* < BT=0.5, h=0.32*/
  GFSK_BT_0p5_h_0p7  = 5, /* < BT=0.5, h=0.7 [ CS1 at 500KBPS data rate] */
  GFSK_BT_0p5_h_1p0  = 6, /* < BT=0.5, h=1.0 [ CS4 at 250KBPS data rate] */

  /* BT=** h=0.5 */
  GFSK_BT_0p3_h_0p5 = 7, /* < BT=0.3, h=0.5 [ CS2 at 1MBPS data rate] */
  GFSK_BT_0p7_h_0p5 = 8, /* < BT=0.7, h=0.5 */

  MSK = 9,
  NUM_RADIO_MODES = 10,
} radio_mode_t;

/*!
 * @brief Link layer types.
 */
typedef enum _link_layer
{
  BLE_LL = 0, /* Must match bit assignment in RADIO1_IRQ_SEL */
  ZIGBEE_LL = 1, /* Must match bit assignment in RADIO1_IRQ_SEL */
  ANT_LL = 2, /* Must match bit assignment in RADIO1_IRQ_SEL */
  GENFSK_LL = 3, /* Must match bit assignment in RADIO1_IRQ_SEL */
  UNASSIGNED_LL = 4, /* Must match bit assignment in RADIO1_IRQ_SEL */
} link_layer_t;

/*!
 * @brief data rate selections.
 */
typedef enum _data_rate
{
  DR_1MBPS = 0, /* Must match bit assignment in BITRATE field */
  DR_500KBPS = 1, /* Must match bit assignment in BITRATE field */
  DR_250KBPS = 2, /* Must match bit assignment in BITRATE field */
  DR_UNASSIGNED = 3, /* Must match bit assignment in BITRATE field */
} data_rate_t;

/*!
 * @brief Control settings for Fast Antenna Diversity
 */
typedef enum  _FAD_LPPS_CTRL
{
  NONE = 0,
  FAD_ENABLED = 1,
  LPPS_ENABLED = 2
} FAD_LPPS_CTRL_T;

/*!
 * @brief XCVR XCVR Panic codes for indicating panic reason.
 */
typedef enum _XCVR_PANIC_ID
{
  WRONG_RADIO_ID_DETECTED = 1,
  CALIBRATION_INVALID = 2,
} XCVR_PANIC_ID_T;

/*!
 * @brief Initialization or mode change selection for config routine.
 */
typedef enum _XCVR_INIT_MODE_CHG
{
  XCVR_MODE_CHANGE = 0,
  XCVR_FIRST_INIT = 1,
} XCVR_INIT_MODE_CHG_T;

typedef enum _XCVR_COEX_PRIORITY
{
  XCVR_COEX_LOW_PRIO = 0,
  XCVR_COEX_HIGH_PRIO = 1
} XCVR_COEX_PRIORITY_T;

/*!
 * @brief Current configuration of the radio.
 */
typedef struct xcvr_currConfig_tag
{
    radio_mode_t radio_mode;
    data_rate_t data_rate;
} xcvr_currConfig_t;


/*!
 * @brief XCVR RX_DIG channel filter coefficient storage
 * Storage of the coefficients varies from 6 bits to 10 bits so all use int16_t for storage.
 */
typedef struct _xcvr_rx_chf_coeffs
{
    uint16_t rx_chf_coef_0; /* < 6 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_1; /* < 6 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_2; /* < 7 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_3; /* < 7 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_4; /* < 7 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_5; /* < 7 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_6; /* < 8 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_7; /* < 8 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_8; /* < 9 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_9; /* < 9 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_10; /* < 10 bit two's complement stored in a uint16_t */
    uint16_t rx_chf_coef_11; /* < 10 bit two's complement stored in a uint16_t */
} xcvr_rx_chf_coeffs_t;

/*!
 * @brief XCVR masked init type for 32 bit registers
 * Initialization uses the mask to clear selected fields of the register and then OR's in the init value. All init values must be in their proper field position.
 */
typedef struct _xcvr_masked_init_32
{
    uint32_t mask;
    uint32_t init;
} xcvr_masked_init_32_t;

/*!
 * @brief XCVR common configure structure
 */
typedef struct _xcvr_common_config
{
    /* XCVR_ANA configs */
    xcvr_masked_init_32_t ana_sy_ctrl1;  

    /* XCVR_PLL_DIG configs */
    uint32_t pll_hpm_bump;
    uint32_t pll_mod_ctrl;
    uint32_t pll_chan_map;
    uint32_t pll_lock_detect;
    uint32_t pll_hpm_ctrl;
    uint32_t pll_hpmcal_ctrl;
    uint32_t pll_hpm_sdm_res;
    uint32_t pll_lpm_ctrl;
    uint32_t pll_lpm_sdm_ctrl1;
    uint32_t pll_delay_match;
    uint32_t pll_ctune_ctrl;

    /* XCVR_RX_DIG configs */
    uint32_t rx_dig_ctrl_init; /* NOTE: Common init, mode init, and datarate init will be OR'd together for RX_DIG_CTRL to form complete register initialization */
    uint32_t dcoc_ctrl_0_init_26mhz; /* NOTE: This will be OR'd with mode specific init for DCOC_CTRL_0 to form complete register initialization */
    uint32_t dcoc_ctrl_0_init_32mhz; /* NOTE: This will be OR'd with mode specific init for DCOC_CTRL_0 to form complete register initialization */
    uint32_t dcoc_ctrl_1_init;
    uint32_t dcoc_cal_gain_init;
    uint32_t dc_resid_ctrl_init; /* NOTE: This will be OR'd with datarate specific init for DCOC_RESID_CTRL to form complete register initialization */
    uint32_t dcoc_cal_rcp_init;
    uint32_t lna_gain_lin_val_2_0_init;
    uint32_t lna_gain_lin_val_5_3_init;
    uint32_t lna_gain_lin_val_8_6_init;
    uint32_t lna_gain_lin_val_9_init;
    uint32_t bba_res_tune_lin_val_3_0_init;
    uint32_t bba_res_tune_lin_val_7_4_init;
    uint32_t bba_res_tune_lin_val_10_8_init;
    uint32_t dcoc_bba_step_init;
    uint32_t dcoc_tza_step_00_init;
    uint32_t dcoc_tza_step_01_init;
    uint32_t dcoc_tza_step_02_init;
    uint32_t dcoc_tza_step_03_init;
    uint32_t dcoc_tza_step_04_init;
    uint32_t dcoc_tza_step_05_init;
    uint32_t dcoc_tza_step_06_init;
    uint32_t dcoc_tza_step_07_init;
    uint32_t dcoc_tza_step_08_init;
    uint32_t dcoc_tza_step_09_init;
    uint32_t dcoc_tza_step_10_init;
    uint32_t agc_ctrl_0_init;
    uint32_t agc_ctrl_3_init;
    /* Other agc config inits moved to modeXdatarate config table */
    uint32_t agc_gain_tbl_03_00_init;
    uint32_t agc_gain_tbl_07_04_init;
    uint32_t agc_gain_tbl_11_08_init;
    uint32_t agc_gain_tbl_15_12_init;
    uint32_t agc_gain_tbl_19_16_init;
    uint32_t agc_gain_tbl_23_20_init;
    uint32_t agc_gain_tbl_26_24_init;
    uint32_t rssi_ctrl_0_init;
    uint32_t cca_ed_lqi_ctrl_0_init;
    uint32_t cca_ed_lqi_ctrl_1_init;

    /* XCVR_TSM configs */
    uint32_t tsm_ctrl;
    uint32_t tsm_ovrd2_init;
    uint32_t end_of_seq_init_26mhz;
    uint32_t end_of_seq_init_32mhz;
    uint32_t lpps_ctrl_init;
    uint32_t tsm_fast_ctrl2_init_26mhz;
    uint32_t tsm_fast_ctrl2_init_32mhz;
    uint32_t recycle_count_init_26mhz;
    uint32_t recycle_count_init_32mhz;
    uint32_t pa_ramp_tbl_0_init;
    uint32_t pa_ramp_tbl_1_init;
    uint32_t tsm_timing_00_init;
    uint32_t tsm_timing_01_init;
    uint32_t tsm_timing_02_init;
    uint32_t tsm_timing_03_init;
    uint32_t tsm_timing_04_init;
    uint32_t tsm_timing_05_init;
    uint32_t tsm_timing_06_init;
    uint32_t tsm_timing_07_init;
    uint32_t tsm_timing_08_init;
    uint32_t tsm_timing_09_init;
    uint32_t tsm_timing_10_init;
    uint32_t tsm_timing_11_init;
    uint32_t tsm_timing_12_init;
    uint32_t tsm_timing_13_init;
    uint32_t tsm_timing_14_init_26mhz; /* tsm_timing_14 has mode specific LSbyte (both LS bytes) */
    uint32_t tsm_timing_14_init_32mhz; /* tsm_timing_14 has mode specific LSbyte (both LS bytes) */
    uint32_t tsm_timing_15_init;
    uint32_t tsm_timing_16_init_26mhz;
    uint32_t tsm_timing_16_init_32mhz;
    uint32_t tsm_timing_17_init;
    uint32_t tsm_timing_18_init;
    uint32_t tsm_timing_19_init;
    uint32_t tsm_timing_20_init;
    uint32_t tsm_timing_21_init;
    uint32_t tsm_timing_22_init;
    uint32_t tsm_timing_23_init;
    uint32_t tsm_timing_24_init;
    uint32_t tsm_timing_25_init_26mhz;
    uint32_t tsm_timing_25_init_32mhz;
    uint32_t tsm_timing_26_init;
    uint32_t tsm_timing_27_init_26mhz;
    uint32_t tsm_timing_27_init_32mhz;
    uint32_t tsm_timing_28_init_26mhz;
    uint32_t tsm_timing_28_init_32mhz;
    uint32_t tsm_timing_29_init_26mhz;
    uint32_t tsm_timing_29_init_32mhz;
    uint32_t tsm_timing_30_init_26mhz;
    uint32_t tsm_timing_30_init_32mhz;
    uint32_t tsm_timing_31_init_26mhz;
    uint32_t tsm_timing_31_init_32mhz;
    uint32_t tsm_timing_32_init_26mhz;
    uint32_t tsm_timing_32_init_32mhz;
    uint32_t tsm_timing_33_init_26mhz;
    uint32_t tsm_timing_33_init_32mhz;
    uint32_t tsm_timing_34_init;
    uint32_t tsm_timing_35_init; /* tsm_timing_35 has a mode specific LSbyte*/
    uint32_t tsm_timing_36_init_26mhz;
    uint32_t tsm_timing_36_init_32mhz;
    uint32_t tsm_timing_37_init_26mhz;
    uint32_t tsm_timing_37_init_32mhz;
    uint32_t tsm_timing_38_init;
    uint32_t tsm_timing_39_init_26mhz;
    uint32_t tsm_timing_39_init_32mhz;
    uint32_t tsm_timing_40_init_26mhz;
    uint32_t tsm_timing_40_init_32mhz;
    uint32_t tsm_timing_41_init_26mhz;
    uint32_t tsm_timing_41_init_32mhz;
    uint32_t tsm_timing_51_init;
    uint32_t tsm_timing_52_init_26mhz;
    uint32_t tsm_timing_52_init_32mhz;
    uint32_t tsm_timing_53_init;
    uint32_t tsm_timing_54_init_26mhz;
    uint32_t tsm_timing_54_init_32mhz;
    uint32_t tsm_timing_55_init_26mhz;
    uint32_t tsm_timing_55_init_32mhz;
    uint32_t tsm_timing_56_init_26mhz;
    uint32_t tsm_timing_56_init_32mhz;
    uint32_t tsm_timing_57_init;
    uint32_t tsm_timing_58_init;

    /* XCVR_TX_DIG configs */
    uint32_t tx_ctrl;
    uint32_t tx_data_padding;
     uint32_t tx_dft_pattern;
    uint32_t rf_dft_bist_1;
    uint32_t rf_dft_bist_2;
} xcvr_common_config_t;

/*!
 * @brief XCVR mode specific configure structure (varies by radio mode)
 */
typedef struct _xcvr_mode_config
{
    radio_mode_t radio_mode;
    uint32_t scgc5_clock_ena_bits;

    /* XCVR_MISC configs */
    xcvr_masked_init_32_t xcvr_ctrl;  

    /* XCVR_PHY configs */
    uint32_t phy_pre_ref0_init;
    uint32_t phy_pre_ref1_init;
    uint32_t phy_pre_ref2_init;
    uint32_t phy_cfg1_init;
    uint32_t phy_el_cfg_init; /* Should leave EL_WIN_SIZE and EL_INTERVAL to the data_rate specific configuration */

    /* XCVR_RX_DIG configs */
    uint32_t rx_dig_ctrl_init_26mhz; /* NOTE: Common init, mode init, and datarate init will be OR'd together for RX_DIG_CTRL to form complete register initialization */
    uint32_t rx_dig_ctrl_init_32mhz; /* NOTE: Common init, mode init, and datarate init will be OR'd together for RX_DIG_CTRL to form complete register initialization */
    uint32_t agc_ctrl_0_init; /* NOTE:  Common init and mode init will be OR'd together for AGC_CTRL_0 to form complete register initialization */ 

    /* XCVR_TSM configs */
    uint32_t tsm_timing_35_init; /* Only the LSbyte is mode specific */

    /* XCVR_TX_DIG configs */
    uint32_t tx_gfsk_ctrl;
    uint32_t tx_gfsk_coeff1_26mhz;
    uint32_t tx_gfsk_coeff2_26mhz;
    uint32_t tx_gfsk_coeff1_32mhz;
    uint32_t tx_gfsk_coeff2_32mhz;
} xcvr_mode_config_t;


/*!
 * @brief XCVR modeXdatarate specific configure structure (varies by radio mode AND data rate)
 * This structure is used to store all of the XCVR settings which are dependent upon both radio mode and data rate. It is used as an overlay
 * on top of the xcvr_mode_config_t structure to supply definitions which are either not in that table or which must be overridden for data rate.
 */
typedef struct _xcvr_mode_datarate_config
{
    radio_mode_t radio_mode;
    data_rate_t data_rate;

    /* XCVR_ANA configs */
    xcvr_masked_init_32_t ana_sy_ctrl2;  
    xcvr_masked_init_32_t ana_rx_bba;
    xcvr_masked_init_32_t ana_rx_tza;

    /* XCVR_PHY configs */
    uint32_t phy_cfg2_init;

    uint32_t agc_ctrl_2_init_26mhz;
    uint32_t agc_ctrl_2_init_32mhz;
    xcvr_rx_chf_coeffs_t rx_chf_coeffs_26mhz; /* 26MHz ext clk */
    xcvr_rx_chf_coeffs_t rx_chf_coeffs_32mhz; /* 32MHz ext clk */
    uint32_t rx_rccal_ctrl_0;
    uint32_t rx_rccal_ctrl_1;

    /* XCVR_TX_DIG configs */
    uint32_t tx_fsk_scale_26mhz; /* Only used by MSK mode, but dependent on datarate */
    uint32_t tx_fsk_scale_32mhz; /* Only used by MSK mode, but dependent on datarate */
} xcvr_mode_datarate_config_t;

/*!
 * @brief XCVR datarate specific configure structure (varies by data rate)
 * This structure is used to store all of the XCVR settings which are dependent upon data rate. It is used as an overlay
 * on top of the xcvr_mode_config_t structure to supply definitions which are either not in that table or which must be overridden for data rate.
 */
typedef struct _xcvr_datarate_config
{
    data_rate_t data_rate;

    /* XCVR_PHY configs */
    uint32_t phy_el_cfg_init; /* Note: EL_ENABLE  is set in xcvr_mode_config_t settings */

    /* XCVR_RX_DIG configs */
    uint32_t rx_dig_ctrl_init_26mhz; /* NOTE: Common init, mode init, and datarate init will be OR'd together for RX_DIG_CTRL to form complete register initialization */
    uint32_t rx_dig_ctrl_init_32mhz; /* NOTE: Common init, mode init, and datarate init will be OR'd together for RX_DIG_CTRL to form complete register initialization */
    uint32_t agc_ctrl_1_init_26mhz;
    uint32_t agc_ctrl_1_init_32mhz;
    uint32_t dcoc_ctrl_0_init_26mhz; /* NOTE: This will be OR'd with common init for DCOC_CTRL_0 to form complete register initialization */
    uint32_t dcoc_ctrl_0_init_32mhz; /* NOTE: This will be OR'd with common init for DCOC_CTRL_0 to form complete register initialization */
    uint32_t dcoc_ctrl_1_init_26mhz; /* NOTE: This will be OR'd with common init for DCOC_CTRL_1 to form complete register initialization */
    uint32_t dcoc_ctrl_1_init_32mhz; /* NOTE: This will be OR'd with common init for DCOC_CTRL_1 to form complete register initialization */
    uint32_t dcoc_cal_iir_init_26mhz;
    uint32_t dcoc_cal_iir_init_32mhz;
    uint32_t dc_resid_ctrl_26mhz;/* NOTE: This will be OR'd with common init for DCOC_RESID_CTRL to form complete register initialization */
    uint32_t dc_resid_ctrl_32mhz;/* NOTE: This will be OR'd with common init for DCOC_RESID_CTRL to form complete register initialization */
} xcvr_datarate_config_t;


/*!
 * @brief LPUART callback function type
 *
 * The panic callback function is defined by system if system need to be informed of XCVR fatal errors.
 * refer to #XCVR_RegisterPanicCb
 */
typedef void (*panic_fptr)(uint32_t panic_id, uint32_t location, uint32_t extra1, uint32_t extra2);

/* Make available const structures from config files */
extern const xcvr_common_config_t xcvr_common_config;
extern const xcvr_mode_config_t zgbe_mode_config;
extern const xcvr_mode_config_t ble_mode_config;
extern const xcvr_mode_config_t ant_mode_config;
extern const xcvr_mode_config_t gfsk_bt_0p5_h_0p5_mode_config;
extern const xcvr_mode_config_t gfsk_bt_0p5_h_0p7_mode_config;
extern const xcvr_mode_config_t gfsk_bt_0p5_h_0p32_mode_config;
extern const xcvr_mode_config_t gfsk_bt_0p5_h_1p0_mode_config;
extern const xcvr_mode_config_t gfsk_bt_0p3_h_0p5_mode_config;
extern const xcvr_mode_config_t gfsk_bt_0p7_h_0p5_mode_config;
extern const xcvr_mode_config_t msk_mode_config;

extern const xcvr_datarate_config_t xcvr_1mbps_config;
extern const xcvr_datarate_config_t xcvr_500kbps_config;
extern const xcvr_datarate_config_t xcvr_250kbps_config;
extern const xcvr_datarate_config_t xcvr_802_15_4_500kbps_config; /* Custom datarate settings for 802.15.4 since it is 2MChips/sec */

extern const xcvr_mode_datarate_config_t xcvr_BLE_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_ZIGBEE_500kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_ANT_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p5_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p5_500kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p5_250kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p32_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p32_500kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p32_250kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p7_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p7_500kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_0p7_250kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_1p0_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_1p0_500kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p5_h_1p0_250kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p3_h_0p5_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p3_h_0p5_500kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p3_h_0p5_250kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p7_h_0p5_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p7_h_0p5_500kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_GFSK_BT_0p7_h_0p5_250kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_MSK_1mbps_config;
extern const xcvr_mode_datarate_config_t xcvr_MSK_500kbps_config;
extern const xcvr_mode_datarate_config_t xcvr_MSK_250kbps_config;


/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name XCVR functional Operation
 * @{
 */

/*!
 * @brief Initializes an XCVR instance.
 *
 * This function initializes the XCVR module according to the radio_mode and data_rate settings. This the only function call required to 
 * start up the XCVR in most situations. 
 *
 * @param radio_mode The radio mode for which the XCVR should be configured.
 * @param data_rate The data rate for which the XCVR should be configured. Only matters when GFSK/MSK radio_mode is selected.
 * @note This function encompasses the ::XCVRGetDefafultConfig() and ::XCVR_Configure() functions.
 */
xcvrStatus_t XCVR_Init(radio_mode_t radio_mode, data_rate_t data_rate);


/*!
 * @brief Deinitializes an XCVR instance.
 *
 * This function gate the XCVR module clock and set all register value to reset value.
 *
 */
void XCVR_Deinit(void);


/*!
 * @brief Initializes XCVR configure structure.
 *
 * This function updates pointers to the XCVR configure structures with default values. 
 * The configurations are divided into a common structure, a set of radio mode specific 
 * structures (one per radio_mode), a set of mode&datarate specific structures (for each mode at
 * each datarate), and a set of data rate specific structures.
 * The pointers provided by this routine point to const structures which can be
 * copied to variable structures if changes to settings are required.
 *
 * @param radio_mode [in] The radio mode for which the configuration structures are requested.
 * @param data_rate [in] The data rate for which the configuration structures are requested.
 * @param com_config [in,out] Pointer to a pointer to the common configuration settings structure.
 * @param mode_config  [in,out] Pointer to a pointer to the mode specific configuration settings structure.
 * @param mode_datarate_config  [in,out] Pointer to a pointer to the modeXdata rate specific configuration settings structure.
 * @param datarate_config  [in,out] Pointer to a pointer to the data rate specific configuration settings structure.
 * @return 0 success, others failure
 * @see XCVR_Configure
 */
xcvrStatus_t XCVR_GetDefaultConfig(radio_mode_t radio_mode, data_rate_t data_rate, 
            const xcvr_common_config_t ** com_config, 
            const xcvr_mode_config_t ** mode_config, 
            const xcvr_mode_datarate_config_t ** mode_datarate_config, 
            const xcvr_datarate_config_t ** datarate_config);

/*!
 * @brief Initializes an XCVR instance.
 *
 * This function initializes the XCVR module with user-defined settings.
 *
 * @param com_config Pointer to the common configuration settings structure.
 * @param mode_config Pointer to the mode specific configuration settings structure.
 * @param mode_datarate_config Pointer to a pointer to the modeXdata rate specific configuration settings structure.
 * @param datarate_config Pointer to a pointer to the data rate specific configuration settings structure.
 * @param tempDegC temperature of the die in degrees C.
 * @param ext_clk indicates the external clock setting, 32MHz or 26MHz.
 * @param first_init indicates whether the call is to initialize (== 1) or the call is to perform a mode change (== 0)
 * @return 0 succeed, others failed
 */
xcvrStatus_t XCVR_Configure( const xcvr_common_config_t *com_config, 
                                    const xcvr_mode_config_t *mode_config, 
                                    const xcvr_mode_datarate_config_t *mode_datarate_config, 
                                    const xcvr_datarate_config_t *datarate_config, 
                                    int16_t tempDegC, 
                                    XCVR_INIT_MODE_CHG_T first_init);


/*!
 * @brief Set XCVR register to reset value.
 *
 * This function set XCVR register to the reset value.
 *
 */
void XCVR_Reset(void);


/*!
 * @brief Change the operating mode of the radio.
 *
 * This function changes the XCVR to a new radio operating mode.
 *
 * @param new_radio_mode The radio mode for which the XCVR should be configured.
 * @param new_data_rate The data rate for which the XCVR should be configured. Only matters when GFSK/MSK radio_mode is selected.
 * @return status of the mode change.
 */
 xcvrStatus_t XCVR_ChangeMode (radio_mode_t new_radio_mode, data_rate_t new_data_rate);


/*!
 * @brief Enable Narrowband RSSI measurement.
 *
 * This function enables the narrowband RSSI measurement
 *
 * @param IIRnbEnable true causes the NB RSSI to be enabled, false disabled.
 */
void XCVR_EnaNBRSSIMeas( uint8_t IIRnbEnable );


/*!
 * @brief Set an arbitrary frequency for RX and TX for the radio.
 *
 * This function sets the radio frequency used for RX and RX..
 *
 * @param freq target frequency setting in Hz.
 * @param refOsc reference oscillator setting in Hz.
 * @return status of the frequency change.
 * @details
 */
 xcvrStatus_t XCVR_OverrideFrequency ( uint32_t freq, uint32_t refOsc );


/*!
 * @brief Register a callback from upper layers.
 *
 * This function registers a callback from the upper layers for the radio to call in case of fatal errors.
 *
 * @param fptr The function pointer to a panic callback.
 */
void XCVR_RegisterPanicCb ( panic_fptr fptr ); /* allow upper layers to provide PANIC callback */


/*!
 * @brief Read the health status of the XCVR to detect errors.
 *
 * This function enables the upper layers to request the current radio health.
 *
 * @return The health status of the radio..
 */
healthStatus_t XCVR_HealthCheck ( void ); /* allow upper layers to poll the radio health */


/*!
 * @brief Control FAD and LPPS features.
 *
 * This function controls the Fast Antenna Diversity (FAD) and Low Power Preamble Search.
 *
 * @param fptr control the FAD and LPPS settings.
 *
 */
 void XCVR_FadLppsControl(FAD_LPPS_CTRL_T control);


/*!
 * @brief Change the mapping of the radio IRQs.
 *
 * This function changes the mapping of the radio LL IRQ signals to the 2.4G Radio INT0 and 2.4G Radio INT1 lines.
 *
 * @param irq0_mapping  the LL which should be mapped to the INT0 line.
 * @param irq1_mapping  the LL which should be mapped to the INT1 line.
 * @return status of the mapping request.
 * @ note The radio_mode_t parameters map to ::link_layer_t selections for the LL which is connected to the INT line.
 * @warning
 *  The same LL must NOT be mapped to both INT lines.
 */
 xcvrStatus_t XCVR_SetIRQMapping(radio_mode_t irq0_mapping, radio_mode_t irq1_mapping);

/*!
 * @brief Get the mapping of the one of the radio IRQs.
 *
 * This function reads the setting for the mapping of one of the radio LL IRQ signals to the 2.4G Radio INT0 and 2.4G Radio INT1 lines.
 *
 * @param int_num the number, 0 or 1, of the INT line to fetched.
 * @return the mapping setting of the specified line.
 * @note Any value passed into this routine other than 0 will be treated as a 1.
 */
 link_layer_t XCVR_GetIRQMapping(uint8_t int_num);

/*!
 * @brief Get the current configuration of the XCVR.
 *
 * This function fetches the current configuration (radio mode and radio data rate) of the XCVR to allow LL to properly config data rates, etc
 *
 * @param curr_config pointer to a structure to be updated with the current mode and data rate.
 * @return the status of the request, success or invalid parameter (null pointer).
 * @note This API will return meaningless results if called before the radio is initialized...
 */
xcvrStatus_t XCVR_GetCurrentConfig(xcvr_currConfig_t * curr_config);

/*******************************************************************************
 * Customer level trim functions
 ******************************************************************************/
/*!
 * @brief Controls setting the XTAL trim value..
 *
 * This function enables the upper layers set a crystal trim compensation facor
 *
 * @param xtalTrim the trim value to apply to the XTAL trimming register. Only the 7 LSB are valid, setting the 8th bit returns an error.
 * @return The health status of the radio..
 */
xcvrStatus_t XCVR_SetXtalTrim(uint8_t xtalTrim);


/*!
 * @brief Controls getting the XTAL trim value..
 *
 * This function enables the upper layers to read the current XTAL compensation factors.
 * The returned value is in the range 0..127 (7 bits).
 *
 * @return The XTAL trim compensation factors..
 */ 
uint8_t  XCVR_GetXtalTrim(void);


/*!
 * @brief Controls setting the RSSI adjustment..
 *
 * This function enables the upper layers to set an RSSI adjustment value.
 *
 * @param adj the adjustment value to apply to the RSSI adjustment register. The value must be a signed 8-bit value, in 1/4 dBm step.
 * @return The health status of the radio..
 */
xcvrStatus_t XCVR_SetRssiAdjustment(int8_t adj);


/*!
 * @brief Controls getting the RSSI adjustment..
 *
 * This function enables the upper layers to read the current XCVR RSSI adjustment value.
 * The returned value is a signed 8-bit value, in 1/4 dBm step.
 *
 * @return The RSSI adjustment value..
 */ 
int8_t  XCVR_GetRssiAdjustment(void);


/*!
 * @brief Controls setting the PLL to a particular channel.
 *
 * This function enables setting the radio channel for TX and RX.
 *
 * @param channel the channel number to set
 * @param useMappedChannel when true, channel is assumed to be from the protocol specific channel map. when false, channel is assumed to be from the 128 general channel list..
 * @return The status of the channel over-ride.
 */ 
xcvrStatus_t XCVR_OverrideChannel ( uint8_t channel, uint8_t useMappedChannel );


/*!
 * @brief Reads the current frequency for RX and TX for the radio.
 *
 * This function reads the radio frequency used for RX and RX..
 *
 * @return Current radio frequency setting.
 */
uint32_t XCVR_GetFreq ( void );


/*!
 * @brief Force receiver warmup.
 *
 * This function forces the initiation of a receiver warmup sequence.
 *
 */
void XCVR_ForceRxWu ( void );


/*!
 * @brief Force receiver warmdown.
 *
 * This function forces the initiation of a receiver warmdown sequence.
 *
 */
 void XCVR_ForceRxWd ( void );


/*!
 * @brief Force transmitter warmup.
 *
 * This function forces the initiation of a transmit warmup sequence.
 *
 */
 void XCVR_ForceTxWu ( void );


/*!
 * @brief Force transmitter warmdown.
 *
 * This function forces the initiation of a transmit warmdown sequence.
 *
 */
void XCVR_ForceTxWd ( void );


/*!
 * @brief Starts transmit with a TX pattern register data sequence.
 *
 * This function starts transmitting using the DFT pattern register mode.
 *
 * @param channel_num -  the protocol specific channel to transmit on. Valid values are defined in the CHANNEL_NUM register documentation.
 * @param radio_mode The radio mode for which the XCVR should be configured.
 * @param data_rate The data rate for which the XCVR should be configured. Only matters when GFSK/MSK radio_mode is selected.
 * @param tx_pattern -  the data pattern to transmit on.
 * @return The status of the pattern reg transmit.
 * @note The XCVR_DftTxOff() function must be called to turn off TX and revert all settings. This routine calls XCVR_ChangeMode() with the desired radio mode
 *   and data rate. 
 */ 
xcvrStatus_t XCVR_DftTxPatternReg(uint16_t channel_num, radio_mode_t radio_mode, data_rate_t data_rate, uint32_t tx_pattern);

/*!
 * @brief Starts transmit with a TX LFSR register data sequence.
 *
 * This function starts transmitting using the DFT LFSR register mode.
 *
 * @param channel_num -  the protocol specific channel to transmit on. Valid values are defined in the CHANNEL_NUM register documentation.
 * @param radio_mode The radio mode for which the XCVR should be configured.
 * @param data_rate The data rate for which the XCVR should be configured. Only matters when GFSK/MSK radio_mode is selected.
 * @param lfsr_length -  the length of the LFSR sequence to use.
 * @return The status of the LFSR reg transmit.
 * @note The XCVR_DftTxOff() function must be called to turn off TX and revert all settings. This routine calls XCVR_ChangeMode() with the desired radio mode
 *   and data rate. 
 */ 
xcvrStatus_t XCVR_DftTxLfsrReg(uint16_t channel_num, radio_mode_t radio_mode, data_rate_t data_rate, uint8_t lfsr_length);

/*!
 * @brief Controls clearing all TX DFT settings.
 *
 * This function reverts all TX DFT settings from the test modes to normal operating mode.
 *
 */ 
void XCVR_DftTxOff(void);

/*!
 * @brief Controls setting the PA power level.
 *
 * This function enables setting the PA power level to a specific setting, overriding any link layer settings.
 *
 * @param pa_power -  the power level to set. Valid values are 0, 1, and even values from 2 to 0x3E, inclusive.
 * @return The status of the PA power over-ride.
 */ 
xcvrStatus_t XCVR_ForcePAPower(uint8_t pa_power);

/*!
 * @brief Starts CW TX.
 *
 * This function starts transmitting CW (no modulation).
 *
 * @param rf_channel_freq -  the RF channel to transmit on. Valid values are integer values from 2360 to 2487MHz, inclusive.
 * @param protocol -  the protocol setting to use, valid settings are 6 (GFSK) and 7 (FSK).
 * @return The status of the CW transmit.
 */ 
xcvrStatus_t XCVR_DftTxCW(uint16_t rf_channel_freq, uint8_t  protocol);

xcvrStatus_t XCVR_CoexistenceInit(void);
xcvrStatus_t XCVR_CoexistenceSetPriority(XCVR_COEX_PRIORITY_T rxPriority, XCVR_COEX_PRIORITY_T txPriority);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_XCVR_H_ */
