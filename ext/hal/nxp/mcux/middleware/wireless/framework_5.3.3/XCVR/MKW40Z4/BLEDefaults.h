/*!
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* \file BLEDefaults.h
*  This is a header file for the default register values of the transceiver 
*  used for BLE mode.
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

#ifndef __BLE_DEFAULTS_H__
#define __BLE_DEFAULTS_H__


/*! *********************************************************************************
*************************************************************************************
* Constants
*************************************************************************************
********************************************************************************** */
#define USE_DCOC_MAGIC_NUMBERS    0     /* Set to 1 to use RAW register settings */
#define OVERRIDE_ADC_SCALE_FACTOR 1     /* Use a manually defined ADC_SCALE_FACTOR */


/* XCVR_CTRL Defaults */

/* XCVR_CTRL  */
#define BLE_TGT_PWR_SRC_def_c 0x01
#define BLE_PROTOCOL_def_c    0x00

/* TSM Defaults (no PA ramp)*/

/*TSM_CTRL*/
#define BKPT_def_c                0xff
#define ABORT_ON_FREQ_TARG_def_c  0x00
#define ABORT_ON_CYCLE_SLIP_def_c 0x00
#define ABORT_ON_CTUNE_def_c      0x00
#define RX_ABORT_DIS_def_c        0x00
#define TX_ABORT_DIS_def_c        0x00
#define DATA_PADDING_EN_def_c     0x01  /* Turn on Data Padding */
#define PA_RAMP_SEL_def_c         0x01  /* Use 2uS Ramp */
#define FORCE_RX_EN_def_c         0x00
#define FORCE_TX_EN_def_c         0x00

/*PA_POWER*/
#define PA_POWER_def_c            0x00

/*PA_BIAS_TBL0*/
#define PA_BIAS3_def_c            0x00
#define PA_BIAS2_def_c            0x00
#define PA_BIAS1_def_c            0x00
#define PA_BIAS0_def_c            0x00

/*PA_BIAS_TBL1*/
#define PA_BIAS7_def_c            0x00
#define PA_BIAS6_def_c            0x00
#define PA_BIAS5_def_c            0x00
#define PA_BIAS4_def_c            0x00

/*DATA_PAD_CTRL*/
#define DATA_PAD_1ST_IDX_55_def_c 0x3E
#define DATA_PAD_1ST_IDX_AA_def_c 0x1E

/*TX DIG, PLL DIG  */
#define HPM_LSB_INVERT_def_c      0x3
#define HPM_DENOM_def_c           0x100
#define HPM_BANK_DELAY_def_c      0x4
#define HPM_SDM_DELAY_def_c       0x2
#define LP_SDM_DELAY_def_c        0x9
#define PLL_LFILT_CNTL_def_c      0x03
#define FSK_MODULATION_SCALE_1_def_c 0x0800

/*Analog: BBW Filter  */

/* XCVR_TZA_CTRL */
#define BLE_TZA_CAP_TUNE_def_c    3

/* XCVR_BBF_CTRL */
#define BLE_BBF_CAP_TUNE_def_c    4
#define BLE_BBF_RES_TUNE2_def_c   6

/*RX DIG: AGC, DCOC,  and filtering  */

/*RX_CHF_COEFn*/
/*Dig Channel- 580kHz (no IIR), 545kHz (with 700kHz IIR) */
#define RX_CHF_COEF0_def_c        0xFE
#define RX_CHF_COEF1_def_c        0xFC
#define RX_CHF_COEF2_def_c        0xFA
#define RX_CHF_COEF3_def_c        0xFC
#define RX_CHF_COEF4_def_c        0x03
#define RX_CHF_COEF5_def_c        0x0F
#define RX_CHF_COEF6_def_c        0x1B
#define RX_CHF_COEF7_def_c        0x23

/*RX_DIG_CTRL*/
#define RX_DIG_CTRL_def_c         0x10
#define RX_CH_FILT_BYPASS_def_c   0x00
#define RX_DEC_FILT_OSR_BLE_def_c 0x01
#define RX_INTERP_EN_def_c        0x00
#define RX_NORM_EN_BLE_def_c      0x00
#define RX_RSSI_EN_def_c          0x01
#define RX_AGC_EN_def_c           0x01
#define RX_DCOC_EN_def_c          0x01
#define RX_DCOC_CAL_EN_def_c      0x01

/* AGC_CTRL */
/* AGC_CTRL_0 */
#define AGC_DOWN_RSSI_THRESH_def_c 0xFF
#define AGC_UP_RSSI_THRESH_def_c   0xd0
#define AGC_DOWN_TZA_STEP_SZ_def_c 0x04
#define AGC_DOWN_BBF_STEP_SZ_def_c 0x02
#define AGC_UP_SRC_def_c           0x00
#define AGC_UP_EN_def_c            0x01
#define FREEZE_AGC_SRC_def_c       0x02
#define AGC_FREEZE_EN_def_c        0x01
#define SLOW_AGC_SRC_def_c         0x02
#define SLOW_AGC_EN_def_c          0x01

/* AGC_CTRL_1 */
#define TZA_GAIN_SETTLE_TIME_def_c 0x0c
#define PRESLOW_EN_def_c           0x01
#define USER_BBF_GAIN_EN_def_c     0x00
#define USER_LNM_GAIN_EN_def_c     0x00
#define BBF_USER_GAIN_def_c        0x00
#define LNM_USER_GAIN_def_c        0x00
#define LNM_ALT_CODE_def_c         0x00
#define BBF_ALT_CODE_def_c         0x00

/* AGC_CTRL_2 */
#define AGC_FAST_EXPIRE_def_c      0x2
#define TZA_PDET_THRESH_HI_def_c   0x02
#define TZA_PDET_THRESH_LO_def_c   0x01
#define BBF_PDET_THRESH_HI_def_c   0x06
#define BBF_PDET_THRESH_LO_def_c   0x01
#define BBF_GAIN_SETTLE_TIME_def_c 0x0c
#define TZA_PDET_RST_def_c         0x00
#define BBF_PDET_RST_def_c         0x00

/* AGC_CTRL_3 */
#define AGC_UP_STEP_SZ_def_c       0x02
#define AGC_H2S_STEP_SZ_def_c      0x18
#define AGC_RSSI_DELT_H2S_def_c    0x0e
#define AGC_PDET_LO_DLY_def_c      0x07
#define AGC_UNFREEZE_TIME_def_c    0xfff

/* RSSI_CTRL 0*/
#define RSSI_ADJ_def_c             0x00
#define RSSI_IIR_WEIGHT_def_c      0x03
#define RSSI_IIR_CW_WEIGHT_ENABLEDdef_c  0x02
#define RSSI_IIR_CW_WEIGHT_BYPASSEDdef_c 0x00
#define RSSI_DEC_EN_def_c          0x01
#define RSSI_HOLD_EN_def_c         0x01
#define RSSI_HOLD_SRC_def_c        0x00
#define RSSI_USE_VALS_def_c        0x01

/* RSSI_CTRL_1 */
#define RSSI_ED_THRESH1_H_def_c    0x04
#define RSSI_ED_THRESH0_H_def_c    0x04
#define RSSI_ED_THRESH1_def_c      0xAC
#define RSSI_ED_THRESH0_def_c      0xA4

/* AGC_GAIN_TBL_03_00 */
#define LNM_GAIN_00_def_c 0x00
#define BBF_GAIN_00_def_c 0x00
#define LNM_GAIN_01_def_c 0x00
#define BBF_GAIN_01_def_c 0x00
#define LNM_GAIN_02_def_c 0x00
#define BBF_GAIN_02_def_c 0x01
#define LNM_GAIN_03_def_c 0x01
#define BBF_GAIN_03_def_c 0x00

/* AGC_GAIN_TBL_07_04 */
#define LNM_GAIN_04_def_c 0x01
#define BBF_GAIN_04_def_c 0x01
#define LNM_GAIN_05_def_c 0x02
#define BBF_GAIN_05_def_c 0x01
#define LNM_GAIN_06_def_c 0x02
#define BBF_GAIN_06_def_c 0x02
#define LNM_GAIN_07_def_c 0x02
#define BBF_GAIN_07_def_c 0x03

/* AGC_GAIN_TBL_11_08 */
#define LNM_GAIN_08_def_c 0x02
#define BBF_GAIN_08_def_c 0x04
#define LNM_GAIN_09_def_c 0x03
#define BBF_GAIN_09_def_c 0x03
#define LNM_GAIN_10_def_c 0x03
#define BBF_GAIN_10_def_c 0x04
#define LNM_GAIN_11_def_c 0x03
#define BBF_GAIN_11_def_c 0x05

/* AGC_GAIN_TBL_15_12 */
#define LNM_GAIN_12_def_c 0x04
#define BBF_GAIN_12_def_c 0x04
#define LNM_GAIN_13_def_c 0x04
#define BBF_GAIN_13_def_c 0x05
#define LNM_GAIN_14_def_c 0x04
#define BBF_GAIN_14_def_c 0x06
#define LNM_GAIN_15_def_c 0x05
#define BBF_GAIN_15_def_c 0x05

/* AGC_GAIN_TBL_19_16 */
#define LNM_GAIN_16_def_c 0x05
#define BBF_GAIN_16_def_c 0x06
#define LNM_GAIN_17_def_c 0x05
#define BBF_GAIN_17_def_c 0x07
#define LNM_GAIN_18_def_c 0x06
#define BBF_GAIN_18_def_c 0x06
#define LNM_GAIN_19_def_c 0x06
#define BBF_GAIN_19_def_c 0x07

/* AGC_GAIN_TBL_23_20 */
#define LNM_GAIN_20_def_c 0x06
#define BBF_GAIN_20_def_c 0x08
#define LNM_GAIN_21_def_c 0x07
#define BBF_GAIN_21_def_c 0x07
#define LNM_GAIN_22_def_c 0x07
#define BBF_GAIN_22_def_c 0x08
#define LNM_GAIN_23_def_c 0x07
#define BBF_GAIN_23_def_c 0x09

/* AGC_GAIN_TBL_26_24 */
#define LNM_GAIN_24_def_c 0x08
#define BBF_GAIN_24_def_c 0x08
#define LNM_GAIN_25_def_c 0x08
#define BBF_GAIN_25_def_c 0x09
#define LNM_GAIN_26_def_c 0x08
#define BBF_GAIN_26_def_c 0x0A

/* TCA_AGC gain adjust */
#define TCA_AGC_VAL_0_def_c 0x1E
#define TCA_AGC_VAL_1_def_c 0x34
#define TCA_AGC_VAL_2_def_c 0x25
#define TCA_AGC_VAL_3_def_c 0x3B
#define TCA_AGC_VAL_4_def_c 0x51
#define TCA_AGC_VAL_5_def_c 0x69
#define TCA_AGC_VAL_6_def_c 0x81
#define TCA_AGC_VAL_7_def_c 0x99
#define TCA_AGC_VAL_8_def_c 0xB1

/* BBF_RES_TUNE gain adjust */
#define BBF_RES_TUNE_VAL_0_def_c  0x00
#define BBF_RES_TUNE_VAL_1_def_c  0x00
#define BBF_RES_TUNE_VAL_2_def_c  0x00
#define BBF_RES_TUNE_VAL_3_def_c  0x01
#define BBF_RES_TUNE_VAL_4_def_c  0x01
#define BBF_RES_TUNE_VAL_5_def_c  0x00
#define BBF_RES_TUNE_VAL_6_def_c  0x00
#define BBF_RES_TUNE_VAL_7_def_c  0x00
#define BBF_RES_TUNE_VAL_8_def_c  0x01
#define BBF_RES_TUNE_VAL_9_def_c  0x02
#define BBF_RES_TUNE_VAL_10_def_c 0x02


/*! *********************************************************************************
* DCOC can be setup based on a set of raw register values (MAGIC NUMBERS) 
* or based on a set of equations which allow for easier correction of DCOC
* operation during debug or testing. Setting the USE_DCOC_MAGIC_NUMBERS flag
* selects the use of raw register values
********************************************************************************** */

/* Common DCOC settings, independent of USE_DCOC_MAGIC_NUMBERS flag */
/* DCOC_CTRL_0 */
#define DCOC_CAL_DURATION_def_c   0x12 /* Max: 1F */
#define DCOC_CORR_HOLD_TIME_def_c 0x51 /* Max: 7F - track continuously */
#define DCOC_CORR_DLY_def_c       0x0C 
#define ALPHA_RADIUS_IDX_def_c    0x03
#define ALPHAC_SCALE_IDX_def_c    0x01
#define SIGN_SCALE_IDX_def_c      0x03
#define DCOC_CORRECT_EN_def_c     0x01
#define DCOC_TRACK_EN_def_c       0x01
#define DCOC_MAN_def_c            0x00

/* DCOC Tracking & GearShift Control (Misc Registers) */
#define DCOC_ALPHA_RADIUS_GS_IDX_def_c 05 /* Register: XCVR_ADC_TEST_CTRL */
#define DCOC_ALPHAC_SCALE_GS_IDX_def_c 01 /* Register: XCVR_BBF_CTRL */
#define DCOC_TRK_EST_GS_CNT_def_c      00 /* Register: XCVR_ANA_SPARE */
#define IQMC_DC_GAIN_ADJ_EN_def_c      01 /* Register: XCVR_RX_ANA_CTRL */

/* DCOC_CTRL_1 */
#define TZA_CORR_POL_def_c    1
#define BBF_CORR_POL_def_c    1
#define TRACK_FROM_ZERO_def_c 0

/* DCOC_CAL_GAIN */
#define DCOC_TZA_CAL_GAIN1_def_c 0x04
#define DCOC_BBF_CAL_GAIN1_def_c 0x04
#define DCOC_TZA_CAL_GAIN2_def_c 0x08
#define DCOC_BBF_CAL_GAIN2_def_c 0x04
#define DCOC_TZA_CAL_GAIN3_def_c 0x04
#define DCOC_BBF_CAL_GAIN3_def_c 0x08

/* DCOC_CAL_IIR */
#define DCOC_CAL_IIR3A_IDX_def_c 0x01
#define DCOC_CAL_IIR2A_IDX_def_c 0x02
#define DCOC_CAL_IIR1A_IDX_def_c 0x02

#if USE_DCOC_MAGIC_NUMBERS /* Define raw register contents */

/* DCOC_CAL_RCP */
#define ALPHA_CALC_RECIP_def_c 0x00
#define TMP_CALC_RECIP_def_c   0x00

/* TCA_AGC_LIN_VAL_2_0 */
#define TCA_AGC_LIN_VAL_0_def_c 0x00
#define TCA_AGC_LIN_VAL_1_def_c 0x00
#define TCA_AGC_LIN_VAL_2_def_c 0x00

/* TCA_AGC_LIN_VAL_5_3 */
#define TCA_AGC_LIN_VAL_3_def_c 0x00
#define TCA_AGC_LIN_VAL_4_def_c 0x00
#define TCA_AGC_LIN_VAL_5_def_c 0x00

/* TCA_AGC_LIN_VAL_8_6 */
#define TCA_AGC_LIN_VAL_6_def_c 0x00
#define TCA_AGC_LIN_VAL_7_def_c 0x00
#define TCA_AGC_LIN_VAL_8_def_c 0x00

/* BBF_RES_TUNE_LIN_VAL_3_0 */
#define BBF_RES_TUNE_LIN_VAL_0_def_c 0x00
#define BBF_RES_TUNE_LIN_VAL_1_def_c 0x00
#define BBF_RES_TUNE_LIN_VAL_2_def_c 0x00
#define BBF_RES_TUNE_LIN_VAL_3_def_c 0x00

/* BBF_RES_TUNE_LIN_VAL_7_4 */
#define BBF_RES_TUNE_LIN_VAL_4_def_c 0x00
#define BBF_RES_TUNE_LIN_VAL_5_def_c 0x00
#define BBF_RES_TUNE_LIN_VAL_6_def_c 0x00
#define BBF_RES_TUNE_LIN_VAL_7_def_c 0x00

/* BBF_RES_TUNE_LIN_VAL_10_8 */
#define BBF_RES_TUNE_LIN_VAL_0_def_c 0x00
#define BBF_RES_TUNE_LIN_VAL_0_def_c 0x00
#define BBF_RES_TUNE_LIN_VAL_0_def_c 0x00

/* DCOC_TZA_STEP */
#define DCOC_TZA_STEP_GAIN_00_def_c 0x00
#define DCOC_TZA_STEP_RCP_00_def_c  0x00
#define DCOC_TZA_STEP_GAIN_01_def_c 0x00
#define DCOC_TZA_STEP_RCP_01_def_c  0x00
#define DCOC_TZA_STEP_GAIN_02_def_c 0x00
#define DCOC_TZA_STEP_RCP_02_def_c  0x00
#define DCOC_TZA_STEP_GAIN_03_def_c 0x00
#define DCOC_TZA_STEP_RCP_03_def_c  0x00
#define DCOC_TZA_STEP_GAIN_04_def_c 0x00
#define DCOC_TZA_STEP_RCP_04_def_c  0x00
#define DCOC_TZA_STEP_GAIN_05_def_c 0x00
#define DCOC_TZA_STEP_RCP_05_def_c  0x00
#define DCOC_TZA_STEP_GAIN_06_def_c 0x00
#define DCOC_TZA_STEP_RCP_06_def_c  0x00
#define DCOC_TZA_STEP_GAIN_07_def_c 0x00
#define DCOC_TZA_STEP_RCP_07_def_c  0x00
#define DCOC_TZA_STEP_GAIN_08_def_c 0x00
#define DCOC_TZA_STEP_RCP_08_def_c  0x00
#define DCOC_TZA_STEP_GAIN_09_def_c 0x00
#define DCOC_TZA_STEP_RCP_09_def_c  0x00
#define DCOC_TZA_STEP_GAIN_10_def_c 0x00
#define DCOC_TZA_STEP_RCP_10_def_c  0x00

/* DCOC_CTRL_1 DCOC_CTRL_2 */
#define BBF_STEP_def_c       0x00
#define BBF_STEP_RECIP_def_c 0x00

/* DCOC_CAL_RCP */
#define RCP_GLHmGLLxGBL_def_c 0x00
#define RCP_GBHmGBL_def_c     0x00

#else /* USE_DCOC_MAGIC_NUMBERS == 0 */

/* Uses unsigned inputs for val and biaspt */
#define DCOC_ADD_BIAS(val,biaspt)  ((val > biaspt) ? ((val-biaspt)) : ((biaspt+val))) /* Use when taking DCOC_OFFSET_n values to use for manual corrections */
#define DCOC_REMOVE_BIAS(val,biaspt) ((val >= biaspt) ? ((val-biaspt)) : ((biaspt+val))) /* Use when writing desired values to  DCOC_OFFSET_n register. */

#define DB_TO_LINEAR(x) (pow(10,((x)/(double)20.0)))
#define ABS(x) ((x) > 0 ? (x) : -(x))
#define MAX(x,y) ((x) > (y) ? (x) : (y))
/* This scale factor will be copied to a variable which is overwritten by a trimmed variable if one is available from IFR. */
#if OVERRIDE_ADC_SCALE_FACTOR
#define ADC_SCALE_FACTOR 2.25 /* Range: 1.8 to 2.2. Different ADC_GAIN needed for DCOC_Q CAL (I/Q gain mismatch) */
#else
#define ADC_SCALE_FACTOR (DB_TO_LINEAR(-1.7)*(0x800)/((double)(1000)))
#endif
#define TZA_DCOC_STEP_RAW (((1.0)*1200)/((double)(0x100))) 
#define BBF_DCOC_STEP_RAW (((1.0)*1200)/((double)(0x40)))

#define TCA_GAIN_DB_0_def_c ((double)(-0.525)) /* (-3)) */
#define TCA_GAIN_DB_1_def_c ((double)(4.883))  /* (3)) */
#define TCA_GAIN_DB_2_def_c ((double)(9.355))  /* (9)) */
#define TCA_GAIN_DB_3_def_c ((double)(14.731)) /* (15)) */
#define TCA_GAIN_DB_4_def_c ((double)(20.188)) /* (21)) */
#define TCA_GAIN_DB_5_def_c ((double)(26.320)) /* (27)) */
#define TCA_GAIN_DB_6_def_c ((double)(32.309)) /* (33)) */
#define TCA_GAIN_DB_7_def_c ((double)(38.249)) /* (39)) */
#define TCA_GAIN_DB_8_def_c ((double)(44.120)) /* (45)) */

#define BBF_GAIN_DB_0_def_c ((double)(0))      /* (0)) */
#define BBF_GAIN_DB_1_def_c ((double)(3.071))  /* (3)) */
#define BBF_GAIN_DB_2_def_c ((double)(6.144))  /* (6)) */
#define BBF_GAIN_DB_3_def_c ((double)(9.266))  /* (9)) */
#define BBF_GAIN_DB_4_def_c ((double)(12.299)) /* (12)) */
#define BBF_GAIN_DB_5_def_c ((double)(15.242)) /* (15)) */
#define BBF_GAIN_DB_6_def_c ((double)(18.114)) /* (18)) */
#define BBF_GAIN_DB_7_def_c ((double)(21.198)) /* (21)) */
#define BBF_GAIN_DB_8_def_c ((double)(24.584)) /* (24)) */
#define BBF_GAIN_DB_9_def_c ((double)(27.885)) /* (27)) */
#define BBF_GAIN_DB_10_def_c ((double)(30.95)) /* (30)) */

#endif /* USE_DCOC_MAGIC_NUMBERS */

#endif /* __BLE_DEFAULTS_H__*/