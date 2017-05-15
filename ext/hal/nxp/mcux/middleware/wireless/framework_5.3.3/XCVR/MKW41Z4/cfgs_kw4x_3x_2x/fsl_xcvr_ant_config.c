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
const xcvr_mode_config_t ant_mode_config =
{
    .radio_mode = ANT_MODE,
    .scgc5_clock_ena_bits = SIM_SCGC5_PHYDIG_MASK 
#if !RADIO_IS_GEN_2P1
    | SIM_SCGC5_ANT_MASK
#endif /* !RADIO_IS_GEN_2P1 */
    ,

    /* XCVR_MISC configs */
    .xcvr_ctrl.mask = XCVR_CTRL_XCVR_CTRL_PROTOCOL_MASK |
                      XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC_MASK |
                      XCVR_CTRL_XCVR_CTRL_DEMOD_SEL_MASK,
    .xcvr_ctrl.init = XCVR_CTRL_XCVR_CTRL_PROTOCOL(3) |
                      XCVR_CTRL_XCVR_CTRL_TGT_PWR_SRC(7) |
                      XCVR_CTRL_XCVR_CTRL_DEMOD_SEL(1),

    /* XCVR_PHY configs */
    .phy_pre_ref0_init = RW0PS(0, 0x1B) |
                         RW0PS(1, 0x1CU) |
                         RW0PS(2, 0x1CU) |
                         RW0PS(3, 0x1CU) |
                         RW0PS(4, 0x1DU) |
                         RW0PS(5, 0x1DU) |
                         RW0PS(6, 0x1EU & 0x3U), /* Phase info #6 overlaps two initialization words - only need two lowest bits*/
    .phy_pre_ref1_init = (0x1E) >> 2 | /* Phase info #6 overlaps two initialization words  - manually compute the shift */
                         RW1PS(7, 0x1EU) |
                         RW1PS(8, 0x1EU) |
                         RW1PS(9, 0x1EU) |
                         RW1PS(10, 0x1EU) |
                         RW1PS(11, 0x1DU) |
                         RW1PS(12, 0x1DU & 0xFU), /* Phase info #12 overlaps two initialization words */
    .phy_pre_ref2_init = (0x1D) >> 4 | /* Phase info #12 overlaps two initialization words - manually compute the shift */
                         RW2PS(13, 0x1CU) |
                         RW2PS(14, 0x1CU) |
                         RW2PS(15, 0x1CU),

    .phy_cfg1_init = XCVR_PHY_CFG1_AA_PLAYBACK(1) |
                     XCVR_PHY_CFG1_AA_OUTPUT_SEL(1) | 
                     XCVR_PHY_CFG1_FSK_BIT_INVERT(0) |
                     XCVR_PHY_CFG1_BSM_EN_BLE(0) |
                     XCVR_PHY_CFG1_DEMOD_CLK_MODE(0) |
                     XCVR_PHY_CFG1_CTS_THRESH(0xF8) |
                     XCVR_PHY_CFG1_FSK_FTS_TIMEOUT(2),

    .phy_el_cfg_init = XCVR_PHY_EL_CFG_EL_ENABLE(1) 
#if !RADIO_IS_GEN_2P1
                     | XCVR_PHY_EL_CFG_EL_ZB_ENABLE(0)
#endif /* !RADIO_IS_GEN_2P1 */
    ,

    /* XCVR_RX_DIG configs */
    .rx_dig_ctrl_init_26mhz = XCVR_RX_DIG_RX_DIG_CTRL_RX_FSK_ZB_SEL(0) | /* Depends on protocol */
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_DC_RESID_EN(1) | /* Depends on protocol */
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_SRC_RATE(0),

    .rx_dig_ctrl_init_32mhz = XCVR_RX_DIG_RX_DIG_CTRL_RX_FSK_ZB_SEL(0) | /* Depends on protocol */
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_DC_RESID_EN(1), /* Depends on protocol */

    .agc_ctrl_0_init = XCVR_RX_DIG_AGC_CTRL_0_AGC_DOWN_RSSI_THRESH(0xFF),
    /* XCVR_TSM configs */
#if (DATA_PADDING_EN)
    .tsm_timing_35_init = B0(TX_DIG_EN_ASSERT+TX_DIG_EN_TX_HI_ADJ),
#else
    .tsm_timing_35_init = B0(TX_DIG_EN_ASSERT),
#endif /* (DATA_PADDING_EN) */

    /* XCVR_TX_DIG configs */
    .tx_gfsk_ctrl = XCVR_TX_DIG_GFSK_CTRL_GFSK_MULTIPLY_TABLE_MANUAL(0x4000) |
                    XCVR_TX_DIG_GFSK_CTRL_GFSK_MI(0) |
                    XCVR_TX_DIG_GFSK_CTRL_GFSK_MLD(0) |
                    XCVR_TX_DIG_GFSK_CTRL_GFSK_FLD(0) |
                    XCVR_TX_DIG_GFSK_CTRL_GFSK_MOD_INDEX_SCALING(0) |
                    XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_OVRD_EN(0) |
                    XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_0_OVRD(0) |
                    XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_1_OVRD(0) |
                    XCVR_TX_DIG_GFSK_CTRL_TX_IMAGE_FILTER_2_OVRD(0),
    .tx_gfsk_coeff1_26mhz = 0,
    .tx_gfsk_coeff2_26mhz = 0,
    .tx_gfsk_coeff1_32mhz = 0,
    .tx_gfsk_coeff2_32mhz = 0,
};

/* MODE & DATA RATE combined configuration */
const xcvr_mode_datarate_config_t xcvr_ANT_1mbps_config =
{
    .radio_mode = ANT_MODE,
    .data_rate = DR_1MBPS,

    .ana_sy_ctrl2.mask = XCVR_ANALOG_SY_CTRL_2_SY_VCO_KVM_MASK,
    .ana_sy_ctrl2.init = XCVR_ANALOG_SY_CTRL_2_SY_VCO_KVM(0), /* VCO KVM */
    .ana_rx_bba.mask = XCVR_ANALOG_RX_BBA_RX_BBA_BW_SEL_MASK | XCVR_ANALOG_RX_BBA_RX_BBA2_BW_SEL_MASK,
    .ana_rx_bba.init = XCVR_ANALOG_RX_BBA_RX_BBA_BW_SEL(5) | XCVR_ANALOG_RX_BBA_RX_BBA2_BW_SEL(5), /* BBA_BW_SEL and BBA2_BW_SEL */ 
    .ana_rx_tza.mask = XCVR_ANALOG_RX_TZA_RX_TZA_BW_SEL_MASK,
    .ana_rx_tza.init = XCVR_ANALOG_RX_TZA_RX_TZA_BW_SEL(5), /* TZA_BW_SEL */ 

    .phy_cfg2_init = XCVR_PHY_CFG2_PHY_FIFO_PRECHG(8) |
                     XCVR_PHY_CFG2_X2_DEMOD_GAIN(0x6) ,

    /* AGC configs */
    .agc_ctrl_2_init_26mhz = XCVR_RX_DIG_AGC_CTRL_2_BBA_GAIN_SETTLE_TIME(12) |
                             XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_LO(5) |
                             XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_HI(6) |
                             XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_LO(3) |
                             XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_HI(7) |
                             XCVR_RX_DIG_AGC_CTRL_2_AGC_FAST_EXPIRE(5),
    .agc_ctrl_2_init_32mhz = XCVR_RX_DIG_AGC_CTRL_2_BBA_GAIN_SETTLE_TIME(14) |
                             XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_LO(5) |
                             XCVR_RX_DIG_AGC_CTRL_2_BBA_PDET_SEL_HI(6) |
                             XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_LO(3) |
                             XCVR_RX_DIG_AGC_CTRL_2_TZA_PDET_SEL_HI(7) |
                             XCVR_RX_DIG_AGC_CTRL_2_AGC_FAST_EXPIRE(5),

    /* All constant values are represented as 16 bits, register writes will remove unused bits */
    .rx_chf_coeffs_26mhz.rx_chf_coef_0 = 0xFFFB,
    .rx_chf_coeffs_26mhz.rx_chf_coef_1 = 0xFFF5,
    .rx_chf_coeffs_26mhz.rx_chf_coef_2 = 0xFFF0,
    .rx_chf_coeffs_26mhz.rx_chf_coef_3 = 0xFFEC,
    .rx_chf_coeffs_26mhz.rx_chf_coef_4 = 0xFFEC,
    .rx_chf_coeffs_26mhz.rx_chf_coef_5 = 0xFFF3,
    .rx_chf_coeffs_26mhz.rx_chf_coef_6 = 0x0001,
    .rx_chf_coeffs_26mhz.rx_chf_coef_7 = 0x0016,
    .rx_chf_coeffs_26mhz.rx_chf_coef_8 = 0x002F,
    .rx_chf_coeffs_26mhz.rx_chf_coef_9 = 0x0049,
    .rx_chf_coeffs_26mhz.rx_chf_coef_10 = 0x005D,
    .rx_chf_coeffs_26mhz.rx_chf_coef_11 = 0x0069,

     /* ANT 32MHz Channel Filter */
    .rx_chf_coeffs_32mhz.rx_chf_coef_0 = 0xFFF9,
    .rx_chf_coeffs_32mhz.rx_chf_coef_1 = 0xFFF4,
    .rx_chf_coeffs_32mhz.rx_chf_coef_2 = 0xFFED,
    .rx_chf_coeffs_32mhz.rx_chf_coef_3 = 0xFFE7,
    .rx_chf_coeffs_32mhz.rx_chf_coef_4 = 0xFFE7,
    .rx_chf_coeffs_32mhz.rx_chf_coef_5 = 0xFFEE,
    .rx_chf_coeffs_32mhz.rx_chf_coef_6 = 0xFFFD,
    .rx_chf_coeffs_32mhz.rx_chf_coef_7 = 0x0015,
    .rx_chf_coeffs_32mhz.rx_chf_coef_8 = 0x0031,
    .rx_chf_coeffs_32mhz.rx_chf_coef_9 = 0x004E,
    .rx_chf_coeffs_32mhz.rx_chf_coef_10 = 0x0066,
    .rx_chf_coeffs_32mhz.rx_chf_coef_11 = 0x0073,

    .rx_rccal_ctrl_0 = XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_OFFSET(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_MANUAL(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL0_BBA_RCCAL_DIS(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_SMP_DLY(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL0_RCCAL_COMP_INV(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_OFFSET(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_MANUAL(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL0_TZA_RCCAL_DIS(0) ,
    .rx_rccal_ctrl_1 = XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_OFFSET(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_MANUAL(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL1_ADC_RCCAL_DIS(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_OFFSET(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_MANUAL(0) |
                       XCVR_RX_DIG_RX_RCCAL_CTRL1_BBA2_RCCAL_DIS(0) ,

    .tx_fsk_scale_26mhz = XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_0(0x1627) | XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_1(0x09d9),
    .tx_fsk_scale_32mhz = XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_0(0x1800) | XCVR_TX_DIG_FSK_SCALE_FSK_MODULATION_SCALE_1(0x0800),
};

