/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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

#include "fsl_xcvr.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
const xcvr_common_config_t xcvr_common_config =
{
    /* XCVR_ANA configs */
    .ana_sy_ctrl1.mask = XCVR_ANALOG_SY_CTRL_1_SY_LPF_FILT_CTRL_MASK,
    .ana_sy_ctrl1.init = XCVR_ANALOG_SY_CTRL_1_SY_LPF_FILT_CTRL(3), /* PLL Analog Loop Filter */

#define hpm_vcm_tx 0
#define hpm_vcm_cal 1
#define hpm_fdb_res_tx 0
#define hpm_fdb_res_cal 1
#define modulation_word_manual 0
#define mod_disable 0
#define hpm_mod_manual 0
#define hpm_mod_disable 0
#define hpm_sdm_out_manual 0
#define hpm_sdm_out_disable 0
#define channel_num 0
#define boc 0
#define bmr 1
#define zoc 0
#define ctune_ldf_lev 8
#define ftf_rx_thrsh 33
#define ftw_rx 0
#define ftf_tx_thrsh 6
#define ftw_tx 0
#define freq_count_go 0
#define freq_count_time 0
#define hpm_sdm_in_manual 0
#define hpm_sdm_out_invert 0
#define hpm_sdm_in_disable 0
#define hpm_lfsr_size 4
#define hpm_dth_scl 0
#define hpm_dth_en 1
#define hpm_integer_scale 0
#define hpm_integer_invert 0
#define hpm_cal_invert 1
#define hpm_mod_in_invert 1
#define hpm_cal_not_bumped 0
#define hpm_cal_count_scale 0
#define hp_cal_disable 0
#define hpm_cal_factor_manual 0
#define hpm_cal_array_size 1
#define hpm_cal_time 0
#define hpm_sdm_denom 256
#define hpm_count_adjust 0
#define pll_ld_manual 0
#define pll_ld_disable 0
#define lpm_sdm_inv 0
#define lpm_disable 0
#define lpm_dth_scl 8
#define lpm_d_ctrl 1
#define lpm_d_ovrd 1
#define lpm_scale 8
#define lpm_sdm_use_neg 0
#define hpm_array_bias 0
#define lpm_intg 38
#define sdm_map_disable 0
#define lpm_sdm_delay 4
#define hpm_sdm_delay 0
#define hpm_integer_delay 0
#define ctune_target_manual 0
#define ctune_target_disable 0
#define ctune_adjust 0
#define ctune_manual 0
#define ctune_disable 0

/*-------------------------------------------------------------------------------------------------*/

    .pll_hpm_bump = XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_CAL(hpm_fdb_res_cal) |
                    XCVR_PLL_DIG_HPM_BUMP_HPM_FDB_RES_TX(hpm_fdb_res_tx) |
                    XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_CAL(hpm_vcm_cal) |
                    XCVR_PLL_DIG_HPM_BUMP_HPM_VCM_TX(hpm_vcm_tx),

/*-------------------------------------------------------------------------------------------------*/

    .pll_mod_ctrl = XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_DISABLE(hpm_mod_disable) |
                    XCVR_PLL_DIG_MOD_CTRL_HPM_MOD_MANUAL(hpm_mod_manual) | 
                    XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_DISABLE(hpm_sdm_out_disable) |
                    XCVR_PLL_DIG_MOD_CTRL_HPM_SDM_OUT_MANUAL(hpm_sdm_out_manual) |
                    XCVR_PLL_DIG_MOD_CTRL_MOD_DISABLE(mod_disable) |
                    XCVR_PLL_DIG_MOD_CTRL_MODULATION_WORD_MANUAL(modulation_word_manual),

/*-------------------------------------------------------------------------------------------------*/

    .pll_chan_map = XCVR_PLL_DIG_CHAN_MAP_BMR(bmr) |
                    XCVR_PLL_DIG_CHAN_MAP_BOC(boc) |
                    XCVR_PLL_DIG_CHAN_MAP_CHANNEL_NUM(channel_num)
#if !RADIO_IS_GEN_2P1
                  | XCVR_PLL_DIG_CHAN_MAP_ZOC(zoc)
#endif /* !RADIO_IS_GEN_2P1 */
    ,

/*-------------------------------------------------------------------------------------------------*/

    .pll_lock_detect = XCVR_PLL_DIG_LOCK_DETECT_CTUNE_LDF_LEV(ctune_ldf_lev) |
                       XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_GO(freq_count_go) |
                       XCVR_PLL_DIG_LOCK_DETECT_FREQ_COUNT_TIME(freq_count_time) |
                       XCVR_PLL_DIG_LOCK_DETECT_FTF_RX_THRSH(ftf_rx_thrsh) |
                       XCVR_PLL_DIG_LOCK_DETECT_FTF_TX_THRSH(ftf_tx_thrsh) |
                       XCVR_PLL_DIG_LOCK_DETECT_FTW_RX(ftw_rx) |
                       XCVR_PLL_DIG_LOCK_DETECT_FTW_TX(ftw_tx),

/*-------------------------------------------------------------------------------------------------*/

    .pll_hpm_ctrl = XCVR_PLL_DIG_HPM_CTRL_HPM_CAL_INVERT(hpm_cal_invert) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_EN(hpm_dth_en) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_DTH_SCL(hpm_dth_scl) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_INVERT(hpm_integer_invert) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_INTEGER_SCALE(hpm_integer_scale) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_LFSR_SIZE(hpm_lfsr_size) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_MOD_IN_INVERT(hpm_mod_in_invert) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_DISABLE(hpm_sdm_in_disable) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_IN_MANUAL(hpm_sdm_in_manual) |
                    XCVR_PLL_DIG_HPM_CTRL_HPM_SDM_OUT_INVERT(hpm_sdm_out_invert),
/*-------------------------------------------------------------------------------------------------*/
#if !RADIO_IS_GEN_2P1
    .pll_hpmcal_ctrl = XCVR_PLL_DIG_HPMCAL_CTRL_HP_CAL_DISABLE(hp_cal_disable) |
                       XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_ARRAY_SIZE(hpm_cal_array_size) |
                       XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_COUNT_SCALE(hpm_cal_count_scale) |
                       XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_FACTOR_MANUAL(hpm_cal_factor_manual) |
                       XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_NOT_BUMPED(hpm_cal_not_bumped) |
                       XCVR_PLL_DIG_HPMCAL_CTRL_HPM_CAL_TIME(hpm_cal_time),
#endif /* !RADIO_IS_GEN_2P1 */
/*-------------------------------------------------------------------------------------------------*/
    .pll_hpm_sdm_res = XCVR_PLL_DIG_HPM_SDM_RES_HPM_COUNT_ADJUST(hpm_count_adjust) |
                       XCVR_PLL_DIG_HPM_SDM_RES_HPM_DENOM(hpm_sdm_denom),
/*-------------------------------------------------------------------------------------------------*/
    .pll_lpm_ctrl = XCVR_PLL_DIG_LPM_CTRL_LPM_D_CTRL(lpm_d_ctrl) |
                    XCVR_PLL_DIG_LPM_CTRL_LPM_D_OVRD(lpm_d_ovrd) |
                    XCVR_PLL_DIG_LPM_CTRL_LPM_DISABLE(lpm_disable) |
                    XCVR_PLL_DIG_LPM_CTRL_LPM_DTH_SCL(lpm_dth_scl) |
                    XCVR_PLL_DIG_LPM_CTRL_LPM_SCALE(lpm_scale) |
                    XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_INV(lpm_sdm_inv) |
                    XCVR_PLL_DIG_LPM_CTRL_LPM_SDM_USE_NEG(lpm_sdm_use_neg) |
                    XCVR_PLL_DIG_LPM_CTRL_PLL_LD_DISABLE(pll_ld_disable) |
                    XCVR_PLL_DIG_LPM_CTRL_PLL_LD_MANUAL(pll_ld_manual),
/*-------------------------------------------------------------------------------------------------*/
    .pll_lpm_sdm_ctrl1 = XCVR_PLL_DIG_LPM_SDM_CTRL1_HPM_ARRAY_BIAS(hpm_array_bias) |
                         XCVR_PLL_DIG_LPM_SDM_CTRL1_LPM_INTG(lpm_intg) |
                         XCVR_PLL_DIG_LPM_SDM_CTRL1_SDM_MAP_DISABLE(sdm_map_disable),
/*-------------------------------------------------------------------------------------------------*/
    .pll_delay_match = XCVR_PLL_DIG_DELAY_MATCH_HPM_INTEGER_DELAY(hpm_integer_delay) |
                       XCVR_PLL_DIG_DELAY_MATCH_HPM_SDM_DELAY(hpm_sdm_delay) |
                       XCVR_PLL_DIG_DELAY_MATCH_LPM_SDM_DELAY(lpm_sdm_delay),
/*-------------------------------------------------------------------------------------------------*/
    .pll_ctune_ctrl = XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_ADJUST(ctune_adjust) |
                      XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_DISABLE(ctune_disable) |
                      XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_MANUAL(ctune_manual) |
                      XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_DISABLE(ctune_target_disable) |
                      XCVR_PLL_DIG_CTUNE_CTRL_CTUNE_TARGET_MANUAL(ctune_target_manual),
/*-------------------------------------------------------------------------------------------------*/

    /* XCVR_RX_DIG configs */
    /* NOTE: Clock specific settings are embedded in the mode dependent configs */
    .rx_dig_ctrl_init = XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_NEGEDGE(0) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_CH_FILT_BYPASS(0) |
#if !RADIO_IS_GEN_2P1
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_RAW_EN(0) |
#endif /* !RADIO_IS_GEN_2P1 */
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_ADC_POL(0) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_NORM_EN(1) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_RSSI_EN(1) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_AGC_EN(1) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_EN(1) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_DCOC_CAL_EN(1) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_IQ_SWAP(0) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_DMA_DTEST_EN(0) |
                        XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_HZD_CORR_DIS(1),

    .agc_ctrl_0_init = XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_EN(1) |
                       XCVR_RX_DIG_AGC_CTRL_0_SLOW_AGC_SRC(2) |
                       XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_EN(1) |
                       XCVR_RX_DIG_AGC_CTRL_0_AGC_FREEZE_PRE_OR_AA(0) |
                       XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_EN(1) |
                       XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_SRC(0) |
                       XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_BBA_STEP_SZ(2) |
                       XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_LNA_STEP_SZ(2) |
                       XCVR_RX_DIG_AGC_CTRL_0_AGC_UP_RSSI_THRESH(0xe7),

    .agc_ctrl_3_init = XCVR_RX_DIG_AGC_CTRL_3_AGC_UNFREEZE_TIME(21) |
                       XCVR_RX_DIG_AGC_CTRL_3_AGC_PDET_LO_DLY(2) |
                       XCVR_RX_DIG_AGC_CTRL_3_AGC_RSSI_DELT_H2S(20) |
                       XCVR_RX_DIG_AGC_CTRL_3_AGC_H2S_STEP_SZ(6) |
                       XCVR_RX_DIG_AGC_CTRL_3_AGC_UP_STEP_SZ(2),    

    /* DCOC configs */
    .dcoc_ctrl_0_init_26mhz = XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_DURATION(16) | /* Only the duration changes between 26MHz and 32MHz ref osc settings */
#if (RADIO_IS_GEN_2P1)
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_CHECK_EN(0) |
#endif /* (RADIO_IS_GEN_2P1) */
                              XCVR_RX_DIG_DCOC_CTRL_0_TRACK_FROM_ZERO(0) |
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_EN(1) |
                              XCVR_RX_DIG_DCOC_CTRL_0_TZA_CORR_POL(0) |
                              XCVR_RX_DIG_DCOC_CTRL_0_BBA_CORR_POL(0) | 
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC(1),
    .dcoc_ctrl_0_init_32mhz = XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_DURATION(20) | /* Only the duration changes between 26MHz and 32MHz ref osc settings */
#if (RADIO_IS_GEN_2P1)
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CAL_CHECK_EN(0) |
#endif /* (RADIO_IS_GEN_2P1) */
                              XCVR_RX_DIG_DCOC_CTRL_0_TRACK_FROM_ZERO(0) |
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_EN(1) |
                              XCVR_RX_DIG_DCOC_CTRL_0_TZA_CORR_POL(0) |
                              XCVR_RX_DIG_DCOC_CTRL_0_BBA_CORR_POL(0) |  
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORRECT_SRC(1),
    
    .dcoc_ctrl_1_init = XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_MIN_AGC_IDX(26),

    .dc_resid_ctrl_init = XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ITER_FREEZE(4) |
                          XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_ALPHA(1) |
                          XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_EXT_DC_EN(1) |
                          XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_MIN_AGC_IDX(26),

    .dcoc_cal_gain_init = XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN1(1) |
                          XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN1(1) |
                          XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN2(1) |
                          XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN2(2) |
                          XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_BBA_CAL_GAIN3(3) |
                          XCVR_RX_DIG_DCOC_CAL_GAIN_DCOC_LNA_CAL_GAIN3(1) ,

    .dcoc_cal_rcp_init = XCVR_RX_DIG_DCOC_CAL_RCP_ALPHA_CALC_RECIP(1) |
                         XCVR_RX_DIG_DCOC_CAL_RCP_DCOC_TMP_CALC_RECIP(711),

    .lna_gain_val_3_0 = XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_0(0x1DU) |
                        XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_1(0x32U) |
                        XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_2(0x09U) |
                        XCVR_RX_DIG_LNA_GAIN_VAL_3_0_LNA_GAIN_VAL_3(0x38U),

    .lna_gain_val_7_4 = XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_4(0x4FU) |
                        XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_5(0x5BU) |
                        XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_6(0x72U) |
                        XCVR_RX_DIG_LNA_GAIN_VAL_7_4_LNA_GAIN_VAL_7(0x8AU),
    .lna_gain_val_8 = XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_8(0xA0U) |
                      XCVR_RX_DIG_LNA_GAIN_VAL_8_LNA_GAIN_VAL_9(0xB6U),

    .bba_res_tune_val_7_0 = XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_0(0x0) |
                            XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_1(0x0) |
                            XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_2(0x0) |
                            XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_3(0x0) |
                            XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_4(0x0) |
                            XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_5(0x0) |
                            XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_6(0x0) |
                            XCVR_RX_DIG_BBA_RES_TUNE_VAL_7_0_BBA_RES_TUNE_VAL_7(0xF),
    .bba_res_tune_val_10_8 = XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_8(0x0) |
                             XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_9(0x1) |
                             XCVR_RX_DIG_BBA_RES_TUNE_VAL_10_8_BBA_RES_TUNE_VAL_10(0x2),

    .lna_gain_lin_val_2_0_init = XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_0(0) |
                                 XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_1(0) |
                                 XCVR_RX_DIG_LNA_GAIN_LIN_VAL_2_0_LNA_GAIN_LIN_VAL_2(1),

    .lna_gain_lin_val_5_3_init = XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_3(3) |
                                 XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_4(5) |
                                 XCVR_RX_DIG_LNA_GAIN_LIN_VAL_5_3_LNA_GAIN_LIN_VAL_5(7),

    .lna_gain_lin_val_8_6_init = XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_6(14) |
                                 XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_7(27) |
                                 XCVR_RX_DIG_LNA_GAIN_LIN_VAL_8_6_LNA_GAIN_LIN_VAL_8(50),

    .lna_gain_lin_val_9_init = XCVR_RX_DIG_LNA_GAIN_LIN_VAL_9_LNA_GAIN_LIN_VAL_9(91),

    .bba_res_tune_lin_val_3_0_init = XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_0(8) |
                                     XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_1(11) |
                                     XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_2(16) |
                                     XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_3_0_BBA_RES_TUNE_LIN_VAL_3(22),

    .bba_res_tune_lin_val_7_4_init = XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_4(31) |
                                     XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_5(44) |
                                     XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_6(62) |
                                     XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_7_4_BBA_RES_TUNE_LIN_VAL_7(42), /* Has 2 fractional bits unlike other BBA_RES_TUNE_LIN_VALs */

    .bba_res_tune_lin_val_10_8_init = XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_8(128) |
                                      XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_9(188) |
                                      XCVR_RX_DIG_BBA_RES_TUNE_LIN_VAL_10_8_BBA_RES_TUNE_LIN_VAL_10(288),

    .dcoc_bba_step_init = XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP_RECIP(939) |
                          XCVR_RX_DIG_DCOC_BBA_STEP_BBA_DCOC_STEP(279),

    .dcoc_tza_step_00_init = XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_GAIN_0(77) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_0_DCOC_TZA_STEP_RCP_0(3404),
    .dcoc_tza_step_01_init = XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_GAIN_1(108) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_1_DCOC_TZA_STEP_RCP_1(2439),
    .dcoc_tza_step_02_init = XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_GAIN_2(155) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_2_DCOC_TZA_STEP_RCP_2(1691),
    .dcoc_tza_step_03_init = XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_GAIN_3(220) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_3_DCOC_TZA_STEP_RCP_3(1192),
    .dcoc_tza_step_04_init = XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_GAIN_4(314) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_4_DCOC_TZA_STEP_RCP_4(835),
    .dcoc_tza_step_05_init = XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_GAIN_5(436) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_5_DCOC_TZA_STEP_RCP_5(601),
    .dcoc_tza_step_06_init = XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_GAIN_6(614) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_6_DCOC_TZA_STEP_RCP_6(427),
    .dcoc_tza_step_07_init = XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_GAIN_7(845) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_7_DCOC_TZA_STEP_RCP_7(310),
    .dcoc_tza_step_08_init = XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_GAIN_8(1256) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_8_DCOC_TZA_STEP_RCP_8(209),
    .dcoc_tza_step_09_init = XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_GAIN_9(1805) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_9_DCOC_TZA_STEP_RCP_9(145),
    .dcoc_tza_step_10_init = XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_GAIN_10(2653) |
                             XCVR_RX_DIG_DCOC_TZA_STEP_10_DCOC_TZA_STEP_RCP_10(99),
#if (RADIO_IS_GEN_2P1)
    .dcoc_cal_fail_th_init = XCVR_RX_DIG_DCOC_CAL_FAIL_TH_DCOC_CAL_BETA_F_TH(20) |
                             XCVR_RX_DIG_DCOC_CAL_FAIL_TH_DCOC_CAL_ALPHA_F_TH(10),
    .dcoc_cal_pass_th_init = XCVR_RX_DIG_DCOC_CAL_PASS_TH_DCOC_CAL_BETA_P_TH(16) |
                             XCVR_RX_DIG_DCOC_CAL_PASS_TH_DCOC_CAL_ALPHA_P_TH(2),
#endif /* (RADIO_IS_GEN_2P1) */
    /* AGC Configs */    
    .agc_gain_tbl_03_00_init = XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_00(0) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_00(0) | 
                               XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_01(1) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_01(1) | 
                               XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_02(2) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_02(1) | 
                               XCVR_RX_DIG_AGC_GAIN_TBL_03_00_LNA_GAIN_03(2) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_03_00_BBA_GAIN_03(2),

    .agc_gain_tbl_07_04_init = XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_04(2) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_04(3) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_05(3) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_05(0) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_06(3) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_06(1) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_07_04_LNA_GAIN_07(3) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_07_04_BBA_GAIN_07(2),

    .agc_gain_tbl_11_08_init = XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_08(3) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_08(3) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_09(4) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_09(2) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_10(4) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_10(3) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_11_08_LNA_GAIN_11(4) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_11_08_BBA_GAIN_11(4),

    .agc_gain_tbl_15_12_init = XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_12(5) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_12(4) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_13(5) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_13(5) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_14(6) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_14(4) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_15_12_LNA_GAIN_15(6) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_15_12_BBA_GAIN_15(5),

    .agc_gain_tbl_19_16_init = XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_16(6) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_16(6) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_17(6) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_17(7) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_18(7) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_18(6) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_19_16_LNA_GAIN_19(7) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_19_16_BBA_GAIN_19(7),

    .agc_gain_tbl_23_20_init = XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_20(8) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_20(6) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_21(8) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_21(7) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_22(9) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_22(6) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_23_20_LNA_GAIN_23(9) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_23_20_BBA_GAIN_23(7),

    .agc_gain_tbl_26_24_init = XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_24(9) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_24(8) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_25(9) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_25(9) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_26_24_LNA_GAIN_26(9) |
                               XCVR_RX_DIG_AGC_GAIN_TBL_26_24_BBA_GAIN_26(10),
    
    .rssi_ctrl_0_init = XCVR_RX_DIG_RSSI_CTRL_0_RSSI_USE_VALS(1) |
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_SRC(0) |
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_EN(1) |
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_CW_WEIGHT(0) |
#if !RADIO_IS_GEN_2P1
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_N_WINDOW_AVG(1) |
#else
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_N_WINDOW_NB(1) |
#endif /* !RADIO_IS_GEN_2P1 */
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_HOLD_DELAY(4) |
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_IIR_WEIGHT(3) |
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_VLD_SETTLE(3) |
                        XCVR_RX_DIG_RSSI_CTRL_0_RSSI_ADJ(0xE8) ,

    .cca_ed_lqi_ctrl_0_init = XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CORR_THRESH(0) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_CORR_CNTR_THRESH(0) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_LQI_CNTR(0x1A) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_0_SNR_ADJ(0),

    .cca_ed_lqi_ctrl_1_init = XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_DELAY(0) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_RSSI_NOISE_AVG_FACTOR(0) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_WEIGHT(0x4) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_RSSI_SENS(0x7) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_DIS(0) |
#if !RADIO_IS_GEN_2P1
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SEL_SNR_MODE(0) |
#endif /* !RADIO_IS_GEN_2P1 */
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MEAS_TRANS_TO_IDLE(0) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_CCA1_ED_EN_DIS(0) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_MEAS_COMPLETE(0) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_MAN_AA_MATCH(0) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_SNR_LQI_WEIGHT(0x5) |
                              XCVR_RX_DIG_CCA_ED_LQI_CTRL_1_LQI_BIAS(0x2),

    /* XCVR_TSM configs */
    .tsm_ctrl = XCVR_TSM_CTRL_PA_RAMP_SEL(PA_RAMP_SEL) |
                XCVR_TSM_CTRL_DATA_PADDING_EN(DATA_PADDING_EN) |
                XCVR_TSM_CTRL_TSM_IRQ0_EN(0) |
                XCVR_TSM_CTRL_TSM_IRQ1_EN(0) |
                XCVR_TSM_CTRL_RAMP_DN_DELAY(0x4) |
                XCVR_TSM_CTRL_TX_ABORT_DIS(0) |
                XCVR_TSM_CTRL_RX_ABORT_DIS(0) |
                XCVR_TSM_CTRL_ABORT_ON_CTUNE(0) |
                XCVR_TSM_CTRL_ABORT_ON_CYCLE_SLIP(0) |
                XCVR_TSM_CTRL_ABORT_ON_FREQ_TARG(0) |
                XCVR_TSM_CTRL_BKPT(0xFF) ,

    .tsm_ovrd2_init = XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD(0) | XCVR_TSM_OVRD2_FREQ_TARG_LD_EN_OVRD_EN_MASK,
    .end_of_seq_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(END_OF_RX_WU_26MHZ) | B1(END_OF_TX_WD) | B0(END_OF_TX_WU),
    .end_of_seq_init_32mhz = B3(END_OF_RX_WD) | B2(END_OF_RX_WU) | B1(END_OF_TX_WD) | B0(END_OF_TX_WU),

#if !RADIO_IS_GEN_2P1
    .lpps_ctrl_init = B3(102) | B2(40) | B1(0) | B0(0),
#endif /* !RADIO_IS_GEN_2P1 */

    .tsm_fast_ctrl2_init_26mhz = B3(102 + ADD_FOR_26MHZ) | B2(40 + ADD_FOR_26MHZ) | B1(66) | B0(8),
    .tsm_fast_ctrl2_init_32mhz = B3(102) | B2(40) | B1(66) | B0(8),

    .pa_ramp_tbl_0_init = XCVR_TSM_PA_RAMP_TBL0_PA_RAMP0(PA_RAMP_0) | XCVR_TSM_PA_RAMP_TBL0_PA_RAMP1(PA_RAMP_1) |
                          XCVR_TSM_PA_RAMP_TBL0_PA_RAMP2(PA_RAMP_2) | XCVR_TSM_PA_RAMP_TBL0_PA_RAMP3(PA_RAMP_3),
    .pa_ramp_tbl_1_init = XCVR_TSM_PA_RAMP_TBL1_PA_RAMP4(PA_RAMP_4) | XCVR_TSM_PA_RAMP_TBL1_PA_RAMP5(PA_RAMP_5) |
                          XCVR_TSM_PA_RAMP_TBL1_PA_RAMP6(PA_RAMP_6) | XCVR_TSM_PA_RAMP_TBL1_PA_RAMP7(PA_RAMP_7),

    .recycle_count_init_26mhz = B3(0) | B2(0x1C + ADD_FOR_26MHZ) | B1(0x06) | B0(0x66 + ADD_FOR_26MHZ),
    .recycle_count_init_26mhz = B3(0) | B2(0x1C) | B1(0x06) | B0(0x66),

    .tsm_timing_00_init = B3(END_OF_RX_WD) | B2(0x00) | B1(END_OF_TX_WD) | B0(0x00), /* bb_ldo_hf_en */
    .tsm_timing_01_init = B3(END_OF_RX_WD) | B2(0x00) | B1(END_OF_TX_WD) | B0(0x00), /* bb_ldo_adcdac_en */
    .tsm_timing_02_init = B3(END_OF_RX_WD) | B2(0x00) | B1(0xFF) | B0(0xFF), /* bb_ldo_bba_en */
    .tsm_timing_03_init = B3(END_OF_RX_WD) | B2(0x00) | B1(END_OF_TX_WD) | B0(0x00), /* bb_ldo_pd_en */
    .tsm_timing_04_init = B3(END_OF_RX_WD) | B2(0x00) | B1(END_OF_TX_WD) | B0(0x00), /* bb_ldo_fdbk_en */
    .tsm_timing_05_init = B3(END_OF_RX_WD) | B2(0x00) | B1(END_OF_TX_WD) | B0(0x00), /* bb_ldo_vcolo_en */
    .tsm_timing_06_init = B3(END_OF_RX_WD) | B2(0x00) | B1(END_OF_TX_WD) | B0(0x00), /* bb_ldo_vtref_en */
    .tsm_timing_07_init = B3(0x05) | B2(0x00) | B1(0x05) | B0(0x00), /* bb_ldo_fdbk_bleed_en */
    .tsm_timing_08_init = B3(0x03) | B2(0x00) | B1(0x03) | B0(0x00), /* bb_ldo_vcolo_bleed_en */
    .tsm_timing_09_init = B3(0x03) | B2(0x00) | B1(0x03) | B0(0x00), /* bb_ldo_vcolo_fastcharge_en */

    .tsm_timing_10_init = B3(END_OF_RX_WD) | B2(0x03 + AUX_PLL_DELAY) | B1(END_OF_TX_WD) | B0(0x03), /* bb_xtal_pll_ref_clk_en */
    .tsm_timing_11_init = B3(0xFF) | B2(0xFF) | B1(END_OF_TX_WD) | B0(0x03), /* bb_xtal_dac_ref_clk_en */
    .tsm_timing_12_init = B3(END_OF_RX_WD) | B2(0x03 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rxtx_auxpll_vco_ref_clk_en */
    .tsm_timing_13_init = B3(0x18) | B2(0x00) | B1(0x4C) | B0(0x00), /* sy_vco_autotune_en */
    .tsm_timing_14_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x31+ADD_FOR_26MHZ) | B1(END_OF_TX_WU + PD_CYCLE_SLIP_TX_LO_ADJ) | B0(0x63 + PD_CYCLE_SLIP_TX_HI_ADJ), /* sy_pd_cycle_slip_ld_ft_en */
    .tsm_timing_14_init_32mhz = B3(END_OF_RX_WD) | B2(0x31 + AUX_PLL_DELAY) | B1(END_OF_TX_WU + PD_CYCLE_SLIP_TX_LO_ADJ) | B0(0x63 + PD_CYCLE_SLIP_TX_HI_ADJ),
    .tsm_timing_15_init = B3(END_OF_RX_WD) | B2(0x03 + AUX_PLL_DELAY) | B1(END_OF_TX_WD) | B0(0x03), /* sy_vco_en */
    .tsm_timing_16_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x1C + ADD_FOR_26MHZ) | B1(0xFF) | B0(0xFF), /* sy_lo_rx_buf_en */
    .tsm_timing_16_init_32mhz = B3(END_OF_RX_WD) | B2(0x1C + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_17_init = B3(0xFF) | B2(0xFF) | B1(END_OF_TX_WD) | B0(0x55), /* sy_lo_tx_buf_en */
    .tsm_timing_18_init = B3(END_OF_RX_WD) | B2(0x05 + AUX_PLL_DELAY) | B1(END_OF_TX_WD) | B0(0x05), /* sy_divn_en */
    .tsm_timing_19_init = B3(0x18+AUX_PLL_DELAY) | B2(0x03 + AUX_PLL_DELAY) | B1(0x4C) | B0(0x03), /* sy_pd_filter_charge_en */

    .tsm_timing_20_init = B3(END_OF_RX_WD) | B2(0x03 + AUX_PLL_DELAY) | B1(END_OF_TX_WD) | B0(0x03), /* sy_pd_en */
    .tsm_timing_21_init = B3(END_OF_RX_WD) | B2(0x04 + AUX_PLL_DELAY) | B1(END_OF_TX_WD) | B0(0x04), /* sy_lo_divn_en */
    .tsm_timing_22_init = B3(END_OF_RX_WD) | B2(0x04 + AUX_PLL_DELAY) | B1(0xFF) |           B0(0xFF), /* sy_lo_rx_en */
    .tsm_timing_23_init = B3(0xFF) | B2(0xFF) | B1(END_OF_TX_WD) | B0(0x04), /*sy_lo_tx_en  */
    .tsm_timing_24_init = B3(0x18) | B2(0x00) | B1(0x4C) | B0(0x00), /* sy_divn_cal_en */
    .tsm_timing_25_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x1D + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_lna_mixer_en */
    .tsm_timing_25_init_32mhz = B3(END_OF_RX_WD) | B2(0x1D + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_26_init = B3(0xFF) | B2(0xFF) | B1(END_OF_TX_WD) | B0(0x58), /* tx_pa_en */
    .tsm_timing_27_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x20 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_adc_i_q_en */
    .tsm_timing_27_init_32mhz = B3(END_OF_RX_WD) | B2(0x20 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_28_init_26mhz = B3(0x21 + ADD_FOR_26MHZ) | B2(0x20 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_adc_reset_en */
    .tsm_timing_28_init_32mhz = B3(0x21 + AUX_PLL_DELAY) | B2(0x20 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_29_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x1E + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_bba_i_q_en */
    .tsm_timing_29_init_32mhz = B3(END_OF_RX_WD) | B2(0x1E + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),

    .tsm_timing_30_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x20 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_bba_pdet_en */
    .tsm_timing_30_init_32mhz = B3(END_OF_RX_WD) | B2(0x20 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_31_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x1F + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_bba_tza_dcoc_en */
    .tsm_timing_31_init_32mhz = B3(END_OF_RX_WD) | B2(0x1F + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_32_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x1D + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_tza_i_q_en */
    .tsm_timing_32_init_32mhz = B3(END_OF_RX_WD) | B2(0x1D + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_33_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x20 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_tza_pdet_en */
    .tsm_timing_33_init_32mhz = B3(END_OF_RX_WD) | B2(0x20 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_34_init = B3(END_OF_RX_WD) | B2(0x07 + AUX_PLL_DELAY) | B1(END_OF_TX_WD) | B0(0x07), /* pll_dig_en */
    .tsm_timing_35_init = B3(0xFF) | B2(0xFF) | B1(END_OF_TX_WD), /* tx_dig_en - Byte 0 comes from mode specific settings */
    .tsm_timing_36_init_26mhz = B3(END_OF_RX_WD) | B2(0x66 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_dig_en */
    .tsm_timing_36_init_32mhz = B3(END_OF_RX_WD) | B2(0x66 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_37_init_26mhz = B3(0x67 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B2(0x66 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_init */
    .tsm_timing_37_init_32mhz = B3(0x67 + AUX_PLL_DELAY) | B2(0x66 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_38_init = B3(END_OF_RX_WD) | B2(0x0E + AUX_PLL_DELAY) | B1(END_OF_TX_WD) | B0(0x42), /* sigma_delta_en */
    .tsm_timing_39_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x66 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rx_phy_en */
    .tsm_timing_39_init_32mhz = B3(END_OF_RX_WD) | B2(0x66 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),

    .tsm_timing_40_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x26 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* dcoc_en */
    .tsm_timing_40_init_32mhz = B3(END_OF_RX_WD) | B2(0x26 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_41_init_26mhz = B3(0x27 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B2(0x26 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* dcoc_init */
    .tsm_timing_41_init_32mhz = B3(0x27 + AUX_PLL_DELAY) | B2(0x26 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_51_init = B3(END_OF_RX_WD) | B2(0x03 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rxtx_auxpll_bias_en */
    .tsm_timing_52_init_26mhz = B3(0x17 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B2(0x06 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rxtx_auxpll_fcal_en */
    .tsm_timing_52_init_32mhz = B3(0x17 + AUX_PLL_DELAY) | B2(0x06 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_53_init = B3(END_OF_RX_WD) | B2(0x03 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rxtx_auxpll_lf_pd_en */
    .tsm_timing_54_init_26mhz = B3(0x17 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B2(0x03 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rxtx_auxpll_pd_lf_filter_charge_en */
    .tsm_timing_54_init_32mhz = B3(0x17 + AUX_PLL_DELAY) | B2(0x03 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_55_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x20 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rxtx_auxpll_adc_buf_en */
    .tsm_timing_55_init_32mhz = B3(END_OF_RX_WD) | B2(0x20 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_56_init_26mhz = B3(END_OF_RX_WD_26MHZ) | B2(0x20 + ADD_FOR_26MHZ + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /* rxtx_auxpll_dig_buf_en */
    .tsm_timing_56_init_32mhz = B3(END_OF_RX_WD) | B2(0x20 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF),
    .tsm_timing_57_init = B3(0x1A + AUX_PLL_DELAY) | B2(0x03 + AUX_PLL_DELAY) | B1(0xFF) | B0(0xFF), /*rxtx_rccal_en  */
    .tsm_timing_58_init = B3(0xFF) | B2(0xFF) | B1(END_OF_TX_WD) | B0(0x03), /* tx_hpm_dac_en */

/* XCVR_TX_DIG configs */
#define radio_dft_mode 0
#define lfsr_length 4
#define lfsr_en 0
#define dft_clk_sel 4
#define tx_dft_en 0
#define soc_test_sel 0
#define tx_capture_pol 0
#define freq_word_adj 0
#define lrm 0
#define data_padding_pat_1 0x55
#define data_padding_pat_0 0xAA
#define gfsk_multiply_table_manual 0
#define gfsk_mi 1
#define gfsk_mld 0
#define gfsk_fld 0
#define gfsk_mod_index_scaling 0
#define tx_image_filter_ovrd_en 0
#define tx_image_filter_0_ovrd 0
#define tx_image_filter_1_ovrd 0
#define tx_image_filter_2_ovrd 0
#define gfsk_filter_coeff_manual2 0xC0630401
#define gfsk_filter_coeff_manual1 0xBB29960D
#define fsk_modulation_scale_0 0x1800
#define fsk_modulation_scale_1 0x0800
#define dft_mod_patternval 0
#define ctune_bist_go 0
#define ctune_bist_thrshld 0
#define pa_am_mod_freq 0
#define pa_am_mod_entries 0
#define pa_am_mod_en 0
#define syn_bist_go 0
#define syn_bist_all_channels 0
#define freq_count_threshold 0
#define hpm_inl_bist_go 0
#define hpm_dnl_bist_go 0
#define dft_max_ram_size 0

    .tx_ctrl = XCVR_TX_DIG_CTRL_RADIO_DFT_MODE(radio_dft_mode) |
               XCVR_TX_DIG_CTRL_LFSR_LENGTH(lfsr_length) |
               XCVR_TX_DIG_CTRL_LFSR_EN(lfsr_en) |
               XCVR_TX_DIG_CTRL_DFT_CLK_SEL(dft_clk_sel) |
               XCVR_TX_DIG_CTRL_TX_DFT_EN(tx_dft_en) |
               XCVR_TX_DIG_CTRL_SOC_TEST_SEL(soc_test_sel) |
               XCVR_TX_DIG_CTRL_TX_CAPTURE_POL(tx_capture_pol) |
               XCVR_TX_DIG_CTRL_FREQ_WORD_ADJ(freq_word_adj),
/*-------------------------------------------------------------------------------------------------*/
    .tx_data_padding = XCVR_TX_DIG_DATA_PADDING_LRM(lrm) |
                       XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_1(data_padding_pat_1) |
                       XCVR_TX_DIG_DATA_PADDING_DATA_PADDING_PAT_0(data_padding_pat_0),
/*-------------------------------------------------------------------------------------------------*/
    .tx_dft_pattern = XCVR_TX_DIG_DFT_PATTERN_DFT_MOD_PATTERN(dft_mod_patternval),
#if !RADIO_IS_GEN_2P1
/*-------------------------------------------------------------------------------------------------*/
    .rf_dft_bist_1 = XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_GO(ctune_bist_go) |
                     XCVR_TX_DIG_RF_DFT_BIST_1_CTUNE_BIST_THRSHLD(ctune_bist_thrshld) |
                     XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_FREQ(pa_am_mod_freq) |
                     XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_ENTRIES(pa_am_mod_entries) |
                     XCVR_TX_DIG_RF_DFT_BIST_1_PA_AM_MOD_EN(pa_am_mod_en),
/*-------------------------------------------------------------------------------------------------*/
    .rf_dft_bist_2 = XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_GO(syn_bist_go) |
                     XCVR_TX_DIG_RF_DFT_BIST_2_SYN_BIST_ALL_CHANNELS(syn_bist_all_channels) |
                     XCVR_TX_DIG_RF_DFT_BIST_2_FREQ_COUNT_THRESHOLD(freq_count_threshold) |
                     XCVR_TX_DIG_RF_DFT_BIST_2_HPM_INL_BIST_GO(hpm_inl_bist_go) |
                     XCVR_TX_DIG_RF_DFT_BIST_2_HPM_DNL_BIST_GO(hpm_dnl_bist_go) |
                     XCVR_TX_DIG_RF_DFT_BIST_2_DFT_MAX_RAM_SIZE(dft_max_ram_size),
#endif /* !RADIO_IS_GEN_2P1 */
};

