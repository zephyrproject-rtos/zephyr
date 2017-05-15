/*
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
/* ========================= DATA RATE ONLY settings ===============*/
/*!
 * @brief XCVR 1Mbps DATA RATE specific configure structure
 */
const xcvr_datarate_config_t xcvr_1mbps_config =
{
    .data_rate = DR_1MBPS,
    .phy_el_cfg_init = XCVR_PHY_EL_CFG_EL_WIN_SIZE(0xF) |
#if !RADIO_IS_GEN_2P1
                       XCVR_PHY_EL_CFG_EL_ZB_WIN_SIZE(0) |
#endif /* !RADIO_IS_GEN_2P1 */
                       XCVR_PHY_EL_CFG_EL_INTERVAL(0x20) ,
    .rx_dig_ctrl_init_26mhz = XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(0) |
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN(16),
    .rx_dig_ctrl_init_32mhz = XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(1) |
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN(16),

    .agc_ctrl_1_init_26mhz = XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME(10) |
                             XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN(PRESLOW_ENA),
    .agc_ctrl_1_init_32mhz = XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME(12) |
                             XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN(PRESLOW_ENA),

    .dcoc_ctrl_0_init_26mhz = XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY(10) |
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME(21),
    .dcoc_ctrl_0_init_32mhz = XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY(12) |
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME(26),
    
    .dcoc_ctrl_1_init_26mhz = XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT(0) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX(3),

    .dcoc_ctrl_1_init_32mhz = XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT(0) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX(3),

    .dcoc_cal_iir_init_26mhz = XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX(2) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX(2) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX(2),
    .dcoc_cal_iir_init_32mhz = XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX(2) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX(3) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX(2),

    .dc_resid_ctrl_26mhz = XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN(33) |
                           XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY(6),
    .dc_resid_ctrl_32mhz = XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN(40) |
                           XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY(7),
};

/*!
 * @brief XCVR 500K bps DATA RATE specific configure structure
 */
const xcvr_datarate_config_t xcvr_500kbps_config =
{
    .data_rate = DR_500KBPS,
    .phy_el_cfg_init = XCVR_PHY_EL_CFG_EL_WIN_SIZE(0x8) |
#if !RADIO_IS_GEN_2P1
                       XCVR_PHY_EL_CFG_EL_ZB_WIN_SIZE(0) |
#endif /* !RADIO_IS_GEN_2P1 */
                       XCVR_PHY_EL_CFG_EL_INTERVAL(0x10),
    .rx_dig_ctrl_init_26mhz = XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(1) |
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN(16),
    .rx_dig_ctrl_init_32mhz = XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(2) |
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN(16),

    .agc_ctrl_1_init_26mhz = XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME(15) |
                             XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN(PRESLOW_ENA),
    .agc_ctrl_1_init_32mhz = XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME(18) |
                             XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN(PRESLOW_ENA),

    .dcoc_ctrl_0_init_26mhz = XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY(13) |
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME(29),
    .dcoc_ctrl_0_init_32mhz = XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY(16) |
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME(36),

    .dcoc_ctrl_1_init_26mhz = XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX(2) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT(0) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX(2),

    .dcoc_ctrl_1_init_32mhz = XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX(2) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT(0) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX(2),

    .dcoc_cal_iir_init_26mhz = XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX(2) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX(2) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX(2),
    .dcoc_cal_iir_init_32mhz = XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX(2) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX(2) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX(2),

    .dc_resid_ctrl_26mhz = XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN(26) |
                           XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY(4),
    .dc_resid_ctrl_32mhz = XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN(32) |
                           XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY(4),
};

/*!
 * @brief XCVR 250K bps DATA RATE specific configure structure
 */
const xcvr_datarate_config_t xcvr_250kbps_config =
{
    .data_rate = DR_250KBPS,
    .phy_el_cfg_init = XCVR_PHY_EL_CFG_EL_WIN_SIZE(0x4) |
#if !RADIO_IS_GEN_2P1
                       XCVR_PHY_EL_CFG_EL_ZB_WIN_SIZE(0) |
#endif /* !RADIO_IS_GEN_2P1 */
                       XCVR_PHY_EL_CFG_EL_INTERVAL(0x8) ,
    .rx_dig_ctrl_init_26mhz = XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(2) |
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN(16),
    .rx_dig_ctrl_init_32mhz = XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_OSR(4) |
                              XCVR_RX_DIG_RX_DIG_CTRL_RX_DEC_FILT_GAIN(16),

    .agc_ctrl_1_init_26mhz = XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME(18) |
                             XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN(PRESLOW_ENA),
    .agc_ctrl_1_init_32mhz = XCVR_RX_DIG_AGC_CTRL_1_LNA_GAIN_SETTLE_TIME(22) |
                             XCVR_RX_DIG_AGC_CTRL_1_PRESLOW_EN(PRESLOW_ENA),

    .dcoc_ctrl_0_init_26mhz = XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY(16) |
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME(34),
    .dcoc_ctrl_0_init_32mhz = XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_DLY(20) |
                              XCVR_RX_DIG_DCOC_CTRL_0_DCOC_CORR_HOLD_TIME(42),

    .dcoc_ctrl_1_init_26mhz = XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT(0) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX(2),

    .dcoc_ctrl_1_init_32mhz = XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_TRK_EST_GS_CNT(0) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_SIGN_SCALE_GS_IDX(1) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHAC_SCALE_GS_IDX(3) |
                              XCVR_RX_DIG_DCOC_CTRL_1_DCOC_ALPHA_RADIUS_GS_IDX(2),

    .dcoc_cal_iir_init_26mhz = XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX(0) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX(1) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX(1),
    .dcoc_cal_iir_init_32mhz = XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR3A_IDX(0) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR2A_IDX(1) |
                               XCVR_RX_DIG_DCOC_CAL_IIR_DCOC_CAL_IIR1A_IDX(1),

    .dc_resid_ctrl_26mhz = XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN(13) |
                           XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY(4),
    .dc_resid_ctrl_32mhz = XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_NWIN(16) |
                           XCVR_RX_DIG_DC_RESID_CTRL_DC_RESID_DLY(4),
};

