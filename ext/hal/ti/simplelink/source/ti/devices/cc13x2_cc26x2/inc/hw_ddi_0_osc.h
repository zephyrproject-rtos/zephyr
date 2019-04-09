/******************************************************************************
*  Filename:       hw_ddi_0_osc_h
*  Revised:        2018-05-14 12:24:52 +0200 (Mon, 14 May 2018)
*  Revision:       51990
*
* Copyright (c) 2015 - 2017, Texas Instruments Incorporated
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1) Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*
* 2) Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* 3) Neither the name of the ORGANIZATION nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

#ifndef __HW_DDI_0_OSC_H__
#define __HW_DDI_0_OSC_H__

//*****************************************************************************
//
// This section defines the register offsets of
// DDI_0_OSC component
//
//*****************************************************************************
// Control 0
#define DDI_0_OSC_O_CTL0                                            0x00000000

// Control 1
#define DDI_0_OSC_O_CTL1                                            0x00000004

// RADC External Configuration
#define DDI_0_OSC_O_RADCEXTCFG                                      0x00000008

// Amplitude Compensation Control
#define DDI_0_OSC_O_AMPCOMPCTL                                      0x0000000C

// Amplitude Compensation Threshold 1
#define DDI_0_OSC_O_AMPCOMPTH1                                      0x00000010

// Amplitude Compensation Threshold 2
#define DDI_0_OSC_O_AMPCOMPTH2                                      0x00000014

// Analog Bypass Values 1
#define DDI_0_OSC_O_ANABYPASSVAL1                                   0x00000018

// Internal
#define DDI_0_OSC_O_ANABYPASSVAL2                                   0x0000001C

// Analog Test Control
#define DDI_0_OSC_O_ATESTCTL                                        0x00000020

// ADC Doubler Nanoamp Control
#define DDI_0_OSC_O_ADCDOUBLERNANOAMPCTL                            0x00000024

// XOSCHF Control
#define DDI_0_OSC_O_XOSCHFCTL                                       0x00000028

// Low Frequency Oscillator Control
#define DDI_0_OSC_O_LFOSCCTL                                        0x0000002C

// RCOSCHF Control
#define DDI_0_OSC_O_RCOSCHFCTL                                      0x00000030

// RCOSC_MF Control
#define DDI_0_OSC_O_RCOSCMFCTL                                      0x00000034

// Status 0
#define DDI_0_OSC_O_STAT0                                           0x0000003C

// Status 1
#define DDI_0_OSC_O_STAT1                                           0x00000040

// Status 2
#define DDI_0_OSC_O_STAT2                                           0x00000044

//*****************************************************************************
//
// Register: DDI_0_OSC_O_CTL0
//
//*****************************************************************************
// Field:    [31] XTAL_IS_24M
//
// Set based on the accurate high frequency XTAL.
// ENUMs:
// 24M                      Internal. Only to be used through TI provided API.
// 48M                      Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL0_XTAL_IS_24M                                  0x80000000
#define DDI_0_OSC_CTL0_XTAL_IS_24M_M                                0x80000000
#define DDI_0_OSC_CTL0_XTAL_IS_24M_S                                        31
#define DDI_0_OSC_CTL0_XTAL_IS_24M_24M                              0x80000000
#define DDI_0_OSC_CTL0_XTAL_IS_24M_48M                              0x00000000

// Field:    [29] BYPASS_XOSC_LF_CLK_QUAL
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL0_BYPASS_XOSC_LF_CLK_QUAL                      0x20000000
#define DDI_0_OSC_CTL0_BYPASS_XOSC_LF_CLK_QUAL_M                    0x20000000
#define DDI_0_OSC_CTL0_BYPASS_XOSC_LF_CLK_QUAL_S                            29

// Field:    [28] BYPASS_RCOSC_LF_CLK_QUAL
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL0_BYPASS_RCOSC_LF_CLK_QUAL                     0x10000000
#define DDI_0_OSC_CTL0_BYPASS_RCOSC_LF_CLK_QUAL_M                   0x10000000
#define DDI_0_OSC_CTL0_BYPASS_RCOSC_LF_CLK_QUAL_S                           28

// Field: [27:26] DOUBLER_START_DURATION
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL0_DOUBLER_START_DURATION_W                              2
#define DDI_0_OSC_CTL0_DOUBLER_START_DURATION_M                     0x0C000000
#define DDI_0_OSC_CTL0_DOUBLER_START_DURATION_S                             26

// Field:    [25] DOUBLER_RESET_DURATION
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL0_DOUBLER_RESET_DURATION                       0x02000000
#define DDI_0_OSC_CTL0_DOUBLER_RESET_DURATION_M                     0x02000000
#define DDI_0_OSC_CTL0_DOUBLER_RESET_DURATION_S                             25

// Field:    [24] CLK_DCDC_SRC_SEL
//
// Select DCDC clock source.
//
// 0: CLK_DCDC is 48 MHz clock from RCOSC or XOSC / HPOSC
// 1: CLK_DCDC is always 48 MHz clock from RCOSC
#define DDI_0_OSC_CTL0_CLK_DCDC_SRC_SEL                             0x01000000
#define DDI_0_OSC_CTL0_CLK_DCDC_SRC_SEL_M                           0x01000000
#define DDI_0_OSC_CTL0_CLK_DCDC_SRC_SEL_S                                   24

// Field:    [14] HPOSC_MODE_EN
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL0_HPOSC_MODE_EN                                0x00004000
#define DDI_0_OSC_CTL0_HPOSC_MODE_EN_M                              0x00004000
#define DDI_0_OSC_CTL0_HPOSC_MODE_EN_S                                      14

// Field:    [12] RCOSC_LF_TRIMMED
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL0_RCOSC_LF_TRIMMED                             0x00001000
#define DDI_0_OSC_CTL0_RCOSC_LF_TRIMMED_M                           0x00001000
#define DDI_0_OSC_CTL0_RCOSC_LF_TRIMMED_S                                   12

// Field:    [11] XOSC_HF_POWER_MODE
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL0_XOSC_HF_POWER_MODE                           0x00000800
#define DDI_0_OSC_CTL0_XOSC_HF_POWER_MODE_M                         0x00000800
#define DDI_0_OSC_CTL0_XOSC_HF_POWER_MODE_S                                 11

// Field:    [10] XOSC_LF_DIG_BYPASS
//
// Bypass XOSC_LF and use the digital input clock from AON for the xosc_lf
// clock.
//
// 0: Use 32kHz XOSC as xosc_lf clock source
// 1: Use digital input (from AON) as xosc_lf clock source.
//
// This bit will only have effect when SCLK_LF_SRC_SEL is selecting the xosc_lf
// as the sclk_lf source. The muxing performed by this bit is not glitch free.
// The following procedure must be followed when changing this field to avoid
// glitches on sclk_lf.
//
// 1) Set SCLK_LF_SRC_SEL to select any source other than the xosc_lf clock
// source.
// 2) Set or clear this bit to bypass or not bypass the xosc_lf.
// 3) Set SCLK_LF_SRC_SEL to use xosc_lf.
//
// It is recommended that either the rcosc_hf or xosc_hf (whichever is
// currently active) be selected as the source in step 1 above. This provides a
// faster clock change.
#define DDI_0_OSC_CTL0_XOSC_LF_DIG_BYPASS                           0x00000400
#define DDI_0_OSC_CTL0_XOSC_LF_DIG_BYPASS_M                         0x00000400
#define DDI_0_OSC_CTL0_XOSC_LF_DIG_BYPASS_S                                 10

// Field:     [9] CLK_LOSS_EN
//
// Enable clock loss detection and hence the indicators to the system
// controller.  Checks both SCLK_HF, SCLK_MF and SCLK_LF clock loss indicators.
//
// 0: Disable
// 1: Enable
//
// Clock loss detection must be disabled when changing the sclk_lf source.
// STAT0.SCLK_LF_SRC can be polled to determine when a change to a new sclk_lf
// source has completed.
#define DDI_0_OSC_CTL0_CLK_LOSS_EN                                  0x00000200
#define DDI_0_OSC_CTL0_CLK_LOSS_EN_M                                0x00000200
#define DDI_0_OSC_CTL0_CLK_LOSS_EN_S                                         9

// Field:   [8:7] ACLK_TDC_SRC_SEL
//
// Source select for aclk_tdc.
//
// 00: RCOSC_HF (48MHz)
// 01: RCOSC_HF (24MHz)
// 10: XOSC_HF (24MHz)
// 11: Not used
#define DDI_0_OSC_CTL0_ACLK_TDC_SRC_SEL_W                                    2
#define DDI_0_OSC_CTL0_ACLK_TDC_SRC_SEL_M                           0x00000180
#define DDI_0_OSC_CTL0_ACLK_TDC_SRC_SEL_S                                    7

// Field:   [6:4] ACLK_REF_SRC_SEL
//
// Source select for aclk_ref
//
// 000: RCOSC_HF derived (31.25kHz)
// 001: XOSC_HF derived (31.25kHz)
// 010: RCOSC_LF (32kHz)
// 011: XOSC_LF (32.768kHz)
// 100: RCOSC_MF (2MHz)
// 101-111: Not used
#define DDI_0_OSC_CTL0_ACLK_REF_SRC_SEL_W                                    3
#define DDI_0_OSC_CTL0_ACLK_REF_SRC_SEL_M                           0x00000070
#define DDI_0_OSC_CTL0_ACLK_REF_SRC_SEL_S                                    4

// Field:   [3:2] SCLK_LF_SRC_SEL
//
// Source select for sclk_lf
// ENUMs:
// XOSCLF                   Low frequency XOSC
// RCOSCLF                  Low frequency RCOSC
// XOSCHFDLF                Low frequency clock derived from High Frequency
//                          XOSC
// RCOSCHFDLF               Low frequency clock derived from High Frequency
//                          RCOSC
#define DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_W                                     2
#define DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_M                            0x0000000C
#define DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_S                                     2
#define DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_XOSCLF                       0x0000000C
#define DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_RCOSCLF                      0x00000008
#define DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_XOSCHFDLF                    0x00000004
#define DDI_0_OSC_CTL0_SCLK_LF_SRC_SEL_RCOSCHFDLF                   0x00000000

// Field:     [0] SCLK_HF_SRC_SEL
//
// Source select for sclk_hf.
// ENUMs:
// XOSC                     High frequency XOSC clock
// RCOSC                    High frequency RCOSC clock
#define DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL                              0x00000001
#define DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL_M                            0x00000001
#define DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL_S                                     0
#define DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL_XOSC                         0x00000001
#define DDI_0_OSC_CTL0_SCLK_HF_SRC_SEL_RCOSC                        0x00000000

//*****************************************************************************
//
// Register: DDI_0_OSC_O_CTL1
//
//*****************************************************************************
// Field: [22:18] RCOSCHFCTRIMFRACT
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL1_RCOSCHFCTRIMFRACT_W                                   5
#define DDI_0_OSC_CTL1_RCOSCHFCTRIMFRACT_M                          0x007C0000
#define DDI_0_OSC_CTL1_RCOSCHFCTRIMFRACT_S                                  18

// Field:    [17] RCOSCHFCTRIMFRACT_EN
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL1_RCOSCHFCTRIMFRACT_EN                         0x00020000
#define DDI_0_OSC_CTL1_RCOSCHFCTRIMFRACT_EN_M                       0x00020000
#define DDI_0_OSC_CTL1_RCOSCHFCTRIMFRACT_EN_S                               17

// Field:   [1:0] XOSC_HF_FAST_START
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_CTL1_XOSC_HF_FAST_START_W                                  2
#define DDI_0_OSC_CTL1_XOSC_HF_FAST_START_M                         0x00000003
#define DDI_0_OSC_CTL1_XOSC_HF_FAST_START_S                                  0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_RADCEXTCFG
//
//*****************************************************************************
// Field: [31:22] HPM_IBIAS_WAIT_CNT
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_RADCEXTCFG_HPM_IBIAS_WAIT_CNT_W                           10
#define DDI_0_OSC_RADCEXTCFG_HPM_IBIAS_WAIT_CNT_M                   0xFFC00000
#define DDI_0_OSC_RADCEXTCFG_HPM_IBIAS_WAIT_CNT_S                           22

// Field: [21:16] LPM_IBIAS_WAIT_CNT
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_RADCEXTCFG_LPM_IBIAS_WAIT_CNT_W                            6
#define DDI_0_OSC_RADCEXTCFG_LPM_IBIAS_WAIT_CNT_M                   0x003F0000
#define DDI_0_OSC_RADCEXTCFG_LPM_IBIAS_WAIT_CNT_S                           16

// Field: [15:12] IDAC_STEP
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_RADCEXTCFG_IDAC_STEP_W                                     4
#define DDI_0_OSC_RADCEXTCFG_IDAC_STEP_M                            0x0000F000
#define DDI_0_OSC_RADCEXTCFG_IDAC_STEP_S                                    12

// Field:  [11:6] RADC_DAC_TH
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_RADCEXTCFG_RADC_DAC_TH_W                                   6
#define DDI_0_OSC_RADCEXTCFG_RADC_DAC_TH_M                          0x00000FC0
#define DDI_0_OSC_RADCEXTCFG_RADC_DAC_TH_S                                   6

// Field:     [5] RADC_MODE_IS_SAR
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_RADCEXTCFG_RADC_MODE_IS_SAR                       0x00000020
#define DDI_0_OSC_RADCEXTCFG_RADC_MODE_IS_SAR_M                     0x00000020
#define DDI_0_OSC_RADCEXTCFG_RADC_MODE_IS_SAR_S                              5

//*****************************************************************************
//
// Register: DDI_0_OSC_O_AMPCOMPCTL
//
//*****************************************************************************
// Field:    [30] AMPCOMP_REQ_MODE
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_REQ_MODE                       0x40000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_REQ_MODE_M                     0x40000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_REQ_MODE_S                             30

// Field: [29:28] AMPCOMP_FSM_UPDATE_RATE
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// 250KHZ                   Internal. Only to be used through TI provided API.
// 500KHZ                   Internal. Only to be used through TI provided API.
// 1MHZ                     Internal. Only to be used through TI provided API.
// 2MHZ                     Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_FSM_UPDATE_RATE_W                       2
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_FSM_UPDATE_RATE_M              0x30000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_FSM_UPDATE_RATE_S                      28
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_FSM_UPDATE_RATE_250KHZ         0x30000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_FSM_UPDATE_RATE_500KHZ         0x20000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_FSM_UPDATE_RATE_1MHZ           0x10000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_FSM_UPDATE_RATE_2MHZ           0x00000000

// Field:    [27] AMPCOMP_SW_CTRL
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_SW_CTRL                        0x08000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_SW_CTRL_M                      0x08000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_SW_CTRL_S                              27

// Field:    [26] AMPCOMP_SW_EN
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_SW_EN                          0x04000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_SW_EN_M                        0x04000000
#define DDI_0_OSC_AMPCOMPCTL_AMPCOMP_SW_EN_S                                26

// Field: [23:20] IBIAS_OFFSET
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_IBIAS_OFFSET_W                                  4
#define DDI_0_OSC_AMPCOMPCTL_IBIAS_OFFSET_M                         0x00F00000
#define DDI_0_OSC_AMPCOMPCTL_IBIAS_OFFSET_S                                 20

// Field: [19:16] IBIAS_INIT
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_IBIAS_INIT_W                                    4
#define DDI_0_OSC_AMPCOMPCTL_IBIAS_INIT_M                           0x000F0000
#define DDI_0_OSC_AMPCOMPCTL_IBIAS_INIT_S                                   16

// Field:  [15:8] LPM_IBIAS_WAIT_CNT_FINAL
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_LPM_IBIAS_WAIT_CNT_FINAL_W                      8
#define DDI_0_OSC_AMPCOMPCTL_LPM_IBIAS_WAIT_CNT_FINAL_M             0x0000FF00
#define DDI_0_OSC_AMPCOMPCTL_LPM_IBIAS_WAIT_CNT_FINAL_S                      8

// Field:   [7:4] CAP_STEP
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_CAP_STEP_W                                      4
#define DDI_0_OSC_AMPCOMPCTL_CAP_STEP_M                             0x000000F0
#define DDI_0_OSC_AMPCOMPCTL_CAP_STEP_S                                      4

// Field:   [3:0] IBIASCAP_HPTOLP_OL_CNT
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPCTL_IBIASCAP_HPTOLP_OL_CNT_W                        4
#define DDI_0_OSC_AMPCOMPCTL_IBIASCAP_HPTOLP_OL_CNT_M               0x0000000F
#define DDI_0_OSC_AMPCOMPCTL_IBIASCAP_HPTOLP_OL_CNT_S                        0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_AMPCOMPTH1
//
//*****************************************************************************
// Field: [23:18] HPMRAMP3_LTH
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP3_LTH_W                                  6
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP3_LTH_M                         0x00FC0000
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP3_LTH_S                                 18

// Field: [15:10] HPMRAMP3_HTH
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP3_HTH_W                                  6
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP3_HTH_M                         0x0000FC00
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP3_HTH_S                                 10

// Field:   [9:6] IBIASCAP_LPTOHP_OL_CNT
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPTH1_IBIASCAP_LPTOHP_OL_CNT_W                        4
#define DDI_0_OSC_AMPCOMPTH1_IBIASCAP_LPTOHP_OL_CNT_M               0x000003C0
#define DDI_0_OSC_AMPCOMPTH1_IBIASCAP_LPTOHP_OL_CNT_S                        6

// Field:   [5:0] HPMRAMP1_TH
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP1_TH_W                                   6
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP1_TH_M                          0x0000003F
#define DDI_0_OSC_AMPCOMPTH1_HPMRAMP1_TH_S                                   0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_AMPCOMPTH2
//
//*****************************************************************************
// Field: [31:26] LPMUPDATE_LTH
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPTH2_LPMUPDATE_LTH_W                                 6
#define DDI_0_OSC_AMPCOMPTH2_LPMUPDATE_LTH_M                        0xFC000000
#define DDI_0_OSC_AMPCOMPTH2_LPMUPDATE_LTH_S                                26

// Field: [23:18] LPMUPDATE_HTH
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPTH2_LPMUPDATE_HTH_W                                 6
#define DDI_0_OSC_AMPCOMPTH2_LPMUPDATE_HTH_M                        0x00FC0000
#define DDI_0_OSC_AMPCOMPTH2_LPMUPDATE_HTH_S                                18

// Field: [15:10] ADC_COMP_AMPTH_LPM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPTH2_ADC_COMP_AMPTH_LPM_W                            6
#define DDI_0_OSC_AMPCOMPTH2_ADC_COMP_AMPTH_LPM_M                   0x0000FC00
#define DDI_0_OSC_AMPCOMPTH2_ADC_COMP_AMPTH_LPM_S                           10

// Field:   [7:2] ADC_COMP_AMPTH_HPM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_AMPCOMPTH2_ADC_COMP_AMPTH_HPM_W                            6
#define DDI_0_OSC_AMPCOMPTH2_ADC_COMP_AMPTH_HPM_M                   0x000000FC
#define DDI_0_OSC_AMPCOMPTH2_ADC_COMP_AMPTH_HPM_S                            2

//*****************************************************************************
//
// Register: DDI_0_OSC_O_ANABYPASSVAL1
//
//*****************************************************************************
// Field: [19:16] XOSC_HF_ROW_Q12
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_ANABYPASSVAL1_XOSC_HF_ROW_Q12_W                            4
#define DDI_0_OSC_ANABYPASSVAL1_XOSC_HF_ROW_Q12_M                   0x000F0000
#define DDI_0_OSC_ANABYPASSVAL1_XOSC_HF_ROW_Q12_S                           16

// Field:  [15:0] XOSC_HF_COLUMN_Q12
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_ANABYPASSVAL1_XOSC_HF_COLUMN_Q12_W                        16
#define DDI_0_OSC_ANABYPASSVAL1_XOSC_HF_COLUMN_Q12_M                0x0000FFFF
#define DDI_0_OSC_ANABYPASSVAL1_XOSC_HF_COLUMN_Q12_S                         0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_ANABYPASSVAL2
//
//*****************************************************************************
// Field:  [13:0] XOSC_HF_IBIASTHERM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_ANABYPASSVAL2_XOSC_HF_IBIASTHERM_W                        14
#define DDI_0_OSC_ANABYPASSVAL2_XOSC_HF_IBIASTHERM_M                0x00003FFF
#define DDI_0_OSC_ANABYPASSVAL2_XOSC_HF_IBIASTHERM_S                         0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_ATESTCTL
//
//*****************************************************************************
// Field:    [31] SCLK_LF_AUX_EN
//
// Enable 32 kHz clock to AUX_COMPB.
#define DDI_0_OSC_ATESTCTL_SCLK_LF_AUX_EN                           0x80000000
#define DDI_0_OSC_ATESTCTL_SCLK_LF_AUX_EN_M                         0x80000000
#define DDI_0_OSC_ATESTCTL_SCLK_LF_AUX_EN_S                                 31

// Field: [15:14] TEST_RCOSCMF
//
// Test mode control for RCOSC_MF
//
// 0x0:  test modes disabled
// 0x1:  boosted bias current into self biased inverter
// 0x2:  clock qualification disabled
// 0x3:  boosted bias current into self biased inverter + clock qualification
// disabled
#define DDI_0_OSC_ATESTCTL_TEST_RCOSCMF_W                                    2
#define DDI_0_OSC_ATESTCTL_TEST_RCOSCMF_M                           0x0000C000
#define DDI_0_OSC_ATESTCTL_TEST_RCOSCMF_S                                   14

// Field: [13:12] ATEST_RCOSCMF
//
// ATEST control for RCOSC_MF
//
// 0x0:  ATEST disabled
// 0x1:  ATEST enabled, VDD_LOCAL connected,  ATEST internal to **RCOSC_MF*
// enabled to send out 2MHz clock.
// 0x2:  ATEST disabled
// 0x3:  ATEST enabled, bias current connected, ATEST internal to **RCOSC_MF*
// enabled to send out 2MHz clock.
#define DDI_0_OSC_ATESTCTL_ATEST_RCOSCMF_W                                   2
#define DDI_0_OSC_ATESTCTL_ATEST_RCOSCMF_M                          0x00003000
#define DDI_0_OSC_ATESTCTL_ATEST_RCOSCMF_S                                  12

//*****************************************************************************
//
// Register: DDI_0_OSC_O_ADCDOUBLERNANOAMPCTL
//
//*****************************************************************************
// Field:    [24] NANOAMP_BIAS_ENABLE
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_NANOAMP_BIAS_ENABLE          0x01000000
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_NANOAMP_BIAS_ENABLE_M        0x01000000
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_NANOAMP_BIAS_ENABLE_S                24

// Field:    [23] SPARE23
//
// Software should not rely on the value of a reserved. Writing any other value
// than the reset value may result in undefined behavior
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_SPARE23                      0x00800000
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_SPARE23_M                    0x00800000
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_SPARE23_S                            23

// Field:     [5] ADC_SH_MODE_EN
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_MODE_EN               0x00000020
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_MODE_EN_M             0x00000020
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_MODE_EN_S                      5

// Field:     [4] ADC_SH_VBUF_EN
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_VBUF_EN               0x00000010
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_VBUF_EN_M             0x00000010
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_SH_VBUF_EN_S                      4

// Field:   [1:0] ADC_IREF_CTRL
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_IREF_CTRL_W                       2
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_IREF_CTRL_M              0x00000003
#define DDI_0_OSC_ADCDOUBLERNANOAMPCTL_ADC_IREF_CTRL_S                       0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_XOSCHFCTL
//
//*****************************************************************************
// Field:    [13] TCXO_MODE_XOSC_HF_EN
//
// If this register  is 1 when TCXO_MODE  is 1, then the XOSC_HF is enabled,
// turning on the XOSC_HF bias current allowing a DC bias point to be provided
// to the clipped-sine wave clock signal on external input.
#define DDI_0_OSC_XOSCHFCTL_TCXO_MODE_XOSC_HF_EN                    0x00002000
#define DDI_0_OSC_XOSCHFCTL_TCXO_MODE_XOSC_HF_EN_M                  0x00002000
#define DDI_0_OSC_XOSCHFCTL_TCXO_MODE_XOSC_HF_EN_S                          13

// Field:    [12] TCXO_MODE
//
// If this register  is 1  when BYPASS is  1, this will enable clock
// qualification on the TCXO clock on external input.  This register has no
// effect when BYPASS is 0.
#define DDI_0_OSC_XOSCHFCTL_TCXO_MODE                               0x00001000
#define DDI_0_OSC_XOSCHFCTL_TCXO_MODE_M                             0x00001000
#define DDI_0_OSC_XOSCHFCTL_TCXO_MODE_S                                     12

// Field:   [9:8] PEAK_DET_ITRIM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_XOSCHFCTL_PEAK_DET_ITRIM_W                                 2
#define DDI_0_OSC_XOSCHFCTL_PEAK_DET_ITRIM_M                        0x00000300
#define DDI_0_OSC_XOSCHFCTL_PEAK_DET_ITRIM_S                                 8

// Field:     [6] BYPASS
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_XOSCHFCTL_BYPASS                                  0x00000040
#define DDI_0_OSC_XOSCHFCTL_BYPASS_M                                0x00000040
#define DDI_0_OSC_XOSCHFCTL_BYPASS_S                                         6

// Field:   [4:2] HP_BUF_ITRIM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_XOSCHFCTL_HP_BUF_ITRIM_W                                   3
#define DDI_0_OSC_XOSCHFCTL_HP_BUF_ITRIM_M                          0x0000001C
#define DDI_0_OSC_XOSCHFCTL_HP_BUF_ITRIM_S                                   2

// Field:   [1:0] LP_BUF_ITRIM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_XOSCHFCTL_LP_BUF_ITRIM_W                                   2
#define DDI_0_OSC_XOSCHFCTL_LP_BUF_ITRIM_M                          0x00000003
#define DDI_0_OSC_XOSCHFCTL_LP_BUF_ITRIM_S                                   0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_LFOSCCTL
//
//*****************************************************************************
// Field: [23:22] XOSCLF_REGULATOR_TRIM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_LFOSCCTL_XOSCLF_REGULATOR_TRIM_W                           2
#define DDI_0_OSC_LFOSCCTL_XOSCLF_REGULATOR_TRIM_M                  0x00C00000
#define DDI_0_OSC_LFOSCCTL_XOSCLF_REGULATOR_TRIM_S                          22

// Field: [21:18] XOSCLF_CMIRRWR_RATIO
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_LFOSCCTL_XOSCLF_CMIRRWR_RATIO_W                            4
#define DDI_0_OSC_LFOSCCTL_XOSCLF_CMIRRWR_RATIO_M                   0x003C0000
#define DDI_0_OSC_LFOSCCTL_XOSCLF_CMIRRWR_RATIO_S                           18

// Field:   [9:8] RCOSCLF_RTUNE_TRIM
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// 6P0MEG                   Internal. Only to be used through TI provided API.
// 6P5MEG                   Internal. Only to be used through TI provided API.
// 7P0MEG                   Internal. Only to be used through TI provided API.
// 7P5MEG                   Internal. Only to be used through TI provided API.
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_W                              2
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_M                     0x00000300
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_S                              8
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_6P0MEG                0x00000300
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_6P5MEG                0x00000200
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_7P0MEG                0x00000100
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_RTUNE_TRIM_7P5MEG                0x00000000

// Field:   [7:0] RCOSCLF_CTUNE_TRIM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_CTUNE_TRIM_W                              8
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_CTUNE_TRIM_M                     0x000000FF
#define DDI_0_OSC_LFOSCCTL_RCOSCLF_CTUNE_TRIM_S                              0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_RCOSCHFCTL
//
//*****************************************************************************
// Field:  [15:8] RCOSCHF_CTRIM
//
// Internal. Only to be used through TI provided API.
#define DDI_0_OSC_RCOSCHFCTL_RCOSCHF_CTRIM_W                                 8
#define DDI_0_OSC_RCOSCHFCTL_RCOSCHF_CTRIM_M                        0x0000FF00
#define DDI_0_OSC_RCOSCHFCTL_RCOSCHF_CTRIM_S                                 8

//*****************************************************************************
//
// Register: DDI_0_OSC_O_RCOSCMFCTL
//
//*****************************************************************************
// Field:  [15:9] RCOSC_MF_CAP_ARRAY
//
// Adjust RCOSC_MF capacitor array.
//
// 0x0:  nominal frequency, 0.625pF
// 0x40:  highest frequency, 0.125pF
// 0x3F:  lowest frequency, 1.125pF
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_CAP_ARRAY_W                            7
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_CAP_ARRAY_M                   0x0000FE00
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_CAP_ARRAY_S                            9

// Field:     [8] RCOSC_MF_REG_SEL
//
// Choose regulator type.
//
// 0:  default
// 1:  alternate
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_REG_SEL                       0x00000100
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_REG_SEL_M                     0x00000100
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_REG_SEL_S                              8

// Field:   [7:6] RCOSC_MF_RES_COARSE
//
// Select coarse resistor for frequency adjustment.
//
// 0x0:  400kohms, default
// 0x1:  300kohms, min
// 0x2:  600kohms, max
// 0x3:  500kohms
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_RES_COARSE_W                           2
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_RES_COARSE_M                  0x000000C0
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_RES_COARSE_S                           6

// Field:   [5:4] RCOSC_MF_RES_FINE
//
// Select fine resistor for frequency adjustment.
//
// 0x0:  11kohms, minimum resistance, max freq
// 0x1:  13kohms
// 0x2:  16kohms
// 0x3:  20kohms, max resistance, min freq
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_RES_FINE_W                             2
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_RES_FINE_M                    0x00000030
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_RES_FINE_S                             4

// Field:   [3:0] RCOSC_MF_BIAS_ADJ
//
// Adjusts bias current to RCOSC_MF.
//
// 0x8 minimum current
// 0x0 default current
// 0x7 maximum current
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_BIAS_ADJ_W                             4
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_BIAS_ADJ_M                    0x0000000F
#define DDI_0_OSC_RCOSCMFCTL_RCOSC_MF_BIAS_ADJ_S                             0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_STAT0
//
//*****************************************************************************
// Field: [30:29] SCLK_LF_SRC
//
// Indicates source for the sclk_lf
// ENUMs:
// XOSCLF                   Low frequency XOSC
// RCOSCLF                  Low frequency RCOSC
// XOSCHFDLF                Low frequency clock derived from High Frequency
//                          XOSC
// RCOSCHFDLF               Low frequency clock derived from High Frequency
//                          RCOSC
#define DDI_0_OSC_STAT0_SCLK_LF_SRC_W                                        2
#define DDI_0_OSC_STAT0_SCLK_LF_SRC_M                               0x60000000
#define DDI_0_OSC_STAT0_SCLK_LF_SRC_S                                       29
#define DDI_0_OSC_STAT0_SCLK_LF_SRC_XOSCLF                          0x60000000
#define DDI_0_OSC_STAT0_SCLK_LF_SRC_RCOSCLF                         0x40000000
#define DDI_0_OSC_STAT0_SCLK_LF_SRC_XOSCHFDLF                       0x20000000
#define DDI_0_OSC_STAT0_SCLK_LF_SRC_RCOSCHFDLF                      0x00000000

// Field:    [28] SCLK_HF_SRC
//
// Indicates source for the sclk_hf
// ENUMs:
// XOSC                     High frequency XOSC
// RCOSC                    High frequency RCOSC clock
#define DDI_0_OSC_STAT0_SCLK_HF_SRC                                 0x10000000
#define DDI_0_OSC_STAT0_SCLK_HF_SRC_M                               0x10000000
#define DDI_0_OSC_STAT0_SCLK_HF_SRC_S                                       28
#define DDI_0_OSC_STAT0_SCLK_HF_SRC_XOSC                            0x10000000
#define DDI_0_OSC_STAT0_SCLK_HF_SRC_RCOSC                           0x00000000

// Field:    [22] RCOSC_HF_EN
//
// RCOSC_HF_EN
#define DDI_0_OSC_STAT0_RCOSC_HF_EN                                 0x00400000
#define DDI_0_OSC_STAT0_RCOSC_HF_EN_M                               0x00400000
#define DDI_0_OSC_STAT0_RCOSC_HF_EN_S                                       22

// Field:    [21] RCOSC_LF_EN
//
// RCOSC_LF_EN
#define DDI_0_OSC_STAT0_RCOSC_LF_EN                                 0x00200000
#define DDI_0_OSC_STAT0_RCOSC_LF_EN_M                               0x00200000
#define DDI_0_OSC_STAT0_RCOSC_LF_EN_S                                       21

// Field:    [20] XOSC_LF_EN
//
// XOSC_LF_EN
#define DDI_0_OSC_STAT0_XOSC_LF_EN                                  0x00100000
#define DDI_0_OSC_STAT0_XOSC_LF_EN_M                                0x00100000
#define DDI_0_OSC_STAT0_XOSC_LF_EN_S                                        20

// Field:    [19] CLK_DCDC_RDY
//
// CLK_DCDC_RDY
#define DDI_0_OSC_STAT0_CLK_DCDC_RDY                                0x00080000
#define DDI_0_OSC_STAT0_CLK_DCDC_RDY_M                              0x00080000
#define DDI_0_OSC_STAT0_CLK_DCDC_RDY_S                                      19

// Field:    [18] CLK_DCDC_RDY_ACK
//
// CLK_DCDC_RDY_ACK
#define DDI_0_OSC_STAT0_CLK_DCDC_RDY_ACK                            0x00040000
#define DDI_0_OSC_STAT0_CLK_DCDC_RDY_ACK_M                          0x00040000
#define DDI_0_OSC_STAT0_CLK_DCDC_RDY_ACK_S                                  18

// Field:    [17] SCLK_HF_LOSS
//
// Indicates sclk_hf is lost
#define DDI_0_OSC_STAT0_SCLK_HF_LOSS                                0x00020000
#define DDI_0_OSC_STAT0_SCLK_HF_LOSS_M                              0x00020000
#define DDI_0_OSC_STAT0_SCLK_HF_LOSS_S                                      17

// Field:    [16] SCLK_LF_LOSS
//
// Indicates sclk_lf is lost
#define DDI_0_OSC_STAT0_SCLK_LF_LOSS                                0x00010000
#define DDI_0_OSC_STAT0_SCLK_LF_LOSS_M                              0x00010000
#define DDI_0_OSC_STAT0_SCLK_LF_LOSS_S                                      16

// Field:    [15] XOSC_HF_EN
//
// Indicates that XOSC_HF is enabled.
#define DDI_0_OSC_STAT0_XOSC_HF_EN                                  0x00008000
#define DDI_0_OSC_STAT0_XOSC_HF_EN_M                                0x00008000
#define DDI_0_OSC_STAT0_XOSC_HF_EN_S                                        15

// Field:    [13] XB_48M_CLK_EN
//
// Indicates that the 48MHz clock from the  DOUBLER is enabled.
//
// It will be enabled if 24 or 48 MHz crystal is used (enabled in doubler
// bypass for the 48MHz crystal).
#define DDI_0_OSC_STAT0_XB_48M_CLK_EN                               0x00002000
#define DDI_0_OSC_STAT0_XB_48M_CLK_EN_M                             0x00002000
#define DDI_0_OSC_STAT0_XB_48M_CLK_EN_S                                     13

// Field:    [11] XOSC_HF_LP_BUF_EN
//
// XOSC_HF_LP_BUF_EN
#define DDI_0_OSC_STAT0_XOSC_HF_LP_BUF_EN                           0x00000800
#define DDI_0_OSC_STAT0_XOSC_HF_LP_BUF_EN_M                         0x00000800
#define DDI_0_OSC_STAT0_XOSC_HF_LP_BUF_EN_S                                 11

// Field:    [10] XOSC_HF_HP_BUF_EN
//
// XOSC_HF_HP_BUF_EN
#define DDI_0_OSC_STAT0_XOSC_HF_HP_BUF_EN                           0x00000400
#define DDI_0_OSC_STAT0_XOSC_HF_HP_BUF_EN_M                         0x00000400
#define DDI_0_OSC_STAT0_XOSC_HF_HP_BUF_EN_S                                 10

// Field:     [8] ADC_THMET
//
// ADC_THMET
#define DDI_0_OSC_STAT0_ADC_THMET                                   0x00000100
#define DDI_0_OSC_STAT0_ADC_THMET_M                                 0x00000100
#define DDI_0_OSC_STAT0_ADC_THMET_S                                          8

// Field:     [7] ADC_DATA_READY
//
// indicates when adc_data is ready.
#define DDI_0_OSC_STAT0_ADC_DATA_READY                              0x00000080
#define DDI_0_OSC_STAT0_ADC_DATA_READY_M                            0x00000080
#define DDI_0_OSC_STAT0_ADC_DATA_READY_S                                     7

// Field:   [6:1] ADC_DATA
//
// adc_data
#define DDI_0_OSC_STAT0_ADC_DATA_W                                           6
#define DDI_0_OSC_STAT0_ADC_DATA_M                                  0x0000007E
#define DDI_0_OSC_STAT0_ADC_DATA_S                                           1

// Field:     [0] PENDINGSCLKHFSWITCHING
//
// Indicates when SCLK_HF clock source is ready to be switched
#define DDI_0_OSC_STAT0_PENDINGSCLKHFSWITCHING                      0x00000001
#define DDI_0_OSC_STAT0_PENDINGSCLKHFSWITCHING_M                    0x00000001
#define DDI_0_OSC_STAT0_PENDINGSCLKHFSWITCHING_S                             0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_STAT1
//
//*****************************************************************************
// Field: [31:28] RAMPSTATE
//
// AMPCOMP FSM State
// ENUMs:
// FAST_START_SETTLE        FAST_START_SETTLE
// FAST_START               FAST_START
// DUMMY_TO_INIT_1          DUMMY_TO_INIT_1
// IDAC_DEC_W_MEASURE       IDAC_DECREMENT_WITH_MEASURE
// IBIAS_INC                IBIAS_INCREMENT
// LPM_UPDATE               LPM_UPDATE
// IBIAS_DEC_W_MEASURE      IBIAS_DECREMENT_WITH_MEASURE
// IBIAS_CAP_UPDATE         IBIAS_CAP_UPDATE
// IDAC_INCREMENT           IDAC_INCREMENT
// HPM_UPDATE               HPM_UPDATE
// HPM_RAMP3                HPM_RAMP3
// HPM_RAMP2                HPM_RAMP2
// HPM_RAMP1                HPM_RAMP1
// INITIALIZATION           INITIALIZATION
// RESET                    RESET
#define DDI_0_OSC_STAT1_RAMPSTATE_W                                          4
#define DDI_0_OSC_STAT1_RAMPSTATE_M                                 0xF0000000
#define DDI_0_OSC_STAT1_RAMPSTATE_S                                         28
#define DDI_0_OSC_STAT1_RAMPSTATE_FAST_START_SETTLE                 0xE0000000
#define DDI_0_OSC_STAT1_RAMPSTATE_FAST_START                        0xD0000000
#define DDI_0_OSC_STAT1_RAMPSTATE_DUMMY_TO_INIT_1                   0xC0000000
#define DDI_0_OSC_STAT1_RAMPSTATE_IDAC_DEC_W_MEASURE                0xB0000000
#define DDI_0_OSC_STAT1_RAMPSTATE_IBIAS_INC                         0xA0000000
#define DDI_0_OSC_STAT1_RAMPSTATE_LPM_UPDATE                        0x90000000
#define DDI_0_OSC_STAT1_RAMPSTATE_IBIAS_DEC_W_MEASURE               0x80000000
#define DDI_0_OSC_STAT1_RAMPSTATE_IBIAS_CAP_UPDATE                  0x70000000
#define DDI_0_OSC_STAT1_RAMPSTATE_IDAC_INCREMENT                    0x60000000
#define DDI_0_OSC_STAT1_RAMPSTATE_HPM_UPDATE                        0x50000000
#define DDI_0_OSC_STAT1_RAMPSTATE_HPM_RAMP3                         0x40000000
#define DDI_0_OSC_STAT1_RAMPSTATE_HPM_RAMP2                         0x30000000
#define DDI_0_OSC_STAT1_RAMPSTATE_HPM_RAMP1                         0x20000000
#define DDI_0_OSC_STAT1_RAMPSTATE_INITIALIZATION                    0x10000000
#define DDI_0_OSC_STAT1_RAMPSTATE_RESET                             0x00000000

// Field: [27:22] HPM_UPDATE_AMP
//
// XOSC_HF amplitude during HPM_UPDATE state.
// When amplitude compensation of XOSC_HF is enabled in high performance mode,
// this value is the amplitude of the crystal oscillations measured by the
// on-chip oscillator ADC, divided by 15 mV.  For example, a value of 0x20
// would indicate that the amplitude of the crystal is approximately 480 mV.
// To enable amplitude compensation, AON_WUC OSCCFG must be set to a non-zero
// value.
#define DDI_0_OSC_STAT1_HPM_UPDATE_AMP_W                                     6
#define DDI_0_OSC_STAT1_HPM_UPDATE_AMP_M                            0x0FC00000
#define DDI_0_OSC_STAT1_HPM_UPDATE_AMP_S                                    22

// Field: [21:16] LPM_UPDATE_AMP
//
// XOSC_HF amplitude during LPM_UPDATE state
// When amplitude compensation of XOSC_HF is enabled in low power  mode, this
// value is the amplitude of the crystal oscillations measured by the on-chip
// oscillator ADC, divided by 15 mV.  For example, a value of 0x20 would
// indicate that the amplitude of the crystal is approximately 480 mV.  To
// enable amplitude compensation, AON_WUC OSCCFG must be set to a non-zero
// value.
#define DDI_0_OSC_STAT1_LPM_UPDATE_AMP_W                                     6
#define DDI_0_OSC_STAT1_LPM_UPDATE_AMP_M                            0x003F0000
#define DDI_0_OSC_STAT1_LPM_UPDATE_AMP_S                                    16

// Field:    [15] FORCE_RCOSC_HF
//
// force_rcosc_hf
#define DDI_0_OSC_STAT1_FORCE_RCOSC_HF                              0x00008000
#define DDI_0_OSC_STAT1_FORCE_RCOSC_HF_M                            0x00008000
#define DDI_0_OSC_STAT1_FORCE_RCOSC_HF_S                                    15

// Field:    [14] SCLK_HF_EN
//
// SCLK_HF_EN
#define DDI_0_OSC_STAT1_SCLK_HF_EN                                  0x00004000
#define DDI_0_OSC_STAT1_SCLK_HF_EN_M                                0x00004000
#define DDI_0_OSC_STAT1_SCLK_HF_EN_S                                        14

// Field:    [13] SCLK_MF_EN
//
// SCLK_MF_EN
#define DDI_0_OSC_STAT1_SCLK_MF_EN                                  0x00002000
#define DDI_0_OSC_STAT1_SCLK_MF_EN_M                                0x00002000
#define DDI_0_OSC_STAT1_SCLK_MF_EN_S                                        13

// Field:    [12] ACLK_ADC_EN
//
// ACLK_ADC_EN
#define DDI_0_OSC_STAT1_ACLK_ADC_EN                                 0x00001000
#define DDI_0_OSC_STAT1_ACLK_ADC_EN_M                               0x00001000
#define DDI_0_OSC_STAT1_ACLK_ADC_EN_S                                       12

// Field:    [11] ACLK_TDC_EN
//
// ACLK_TDC_EN
#define DDI_0_OSC_STAT1_ACLK_TDC_EN                                 0x00000800
#define DDI_0_OSC_STAT1_ACLK_TDC_EN_M                               0x00000800
#define DDI_0_OSC_STAT1_ACLK_TDC_EN_S                                       11

// Field:    [10] ACLK_REF_EN
//
// ACLK_REF_EN
#define DDI_0_OSC_STAT1_ACLK_REF_EN                                 0x00000400
#define DDI_0_OSC_STAT1_ACLK_REF_EN_M                               0x00000400
#define DDI_0_OSC_STAT1_ACLK_REF_EN_S                                       10

// Field:     [9] CLK_CHP_EN
//
// CLK_CHP_EN
#define DDI_0_OSC_STAT1_CLK_CHP_EN                                  0x00000200
#define DDI_0_OSC_STAT1_CLK_CHP_EN_M                                0x00000200
#define DDI_0_OSC_STAT1_CLK_CHP_EN_S                                         9

// Field:     [8] CLK_DCDC_EN
//
// CLK_DCDC_EN
#define DDI_0_OSC_STAT1_CLK_DCDC_EN                                 0x00000100
#define DDI_0_OSC_STAT1_CLK_DCDC_EN_M                               0x00000100
#define DDI_0_OSC_STAT1_CLK_DCDC_EN_S                                        8

// Field:     [7] SCLK_HF_GOOD
//
// SCLK_HF_GOOD
#define DDI_0_OSC_STAT1_SCLK_HF_GOOD                                0x00000080
#define DDI_0_OSC_STAT1_SCLK_HF_GOOD_M                              0x00000080
#define DDI_0_OSC_STAT1_SCLK_HF_GOOD_S                                       7

// Field:     [6] SCLK_MF_GOOD
//
// SCLK_MF_GOOD
#define DDI_0_OSC_STAT1_SCLK_MF_GOOD                                0x00000040
#define DDI_0_OSC_STAT1_SCLK_MF_GOOD_M                              0x00000040
#define DDI_0_OSC_STAT1_SCLK_MF_GOOD_S                                       6

// Field:     [5] SCLK_LF_GOOD
//
// SCLK_LF_GOOD
#define DDI_0_OSC_STAT1_SCLK_LF_GOOD                                0x00000020
#define DDI_0_OSC_STAT1_SCLK_LF_GOOD_M                              0x00000020
#define DDI_0_OSC_STAT1_SCLK_LF_GOOD_S                                       5

// Field:     [4] ACLK_ADC_GOOD
//
// ACLK_ADC_GOOD
#define DDI_0_OSC_STAT1_ACLK_ADC_GOOD                               0x00000010
#define DDI_0_OSC_STAT1_ACLK_ADC_GOOD_M                             0x00000010
#define DDI_0_OSC_STAT1_ACLK_ADC_GOOD_S                                      4

// Field:     [3] ACLK_TDC_GOOD
//
// ACLK_TDC_GOOD
#define DDI_0_OSC_STAT1_ACLK_TDC_GOOD                               0x00000008
#define DDI_0_OSC_STAT1_ACLK_TDC_GOOD_M                             0x00000008
#define DDI_0_OSC_STAT1_ACLK_TDC_GOOD_S                                      3

// Field:     [2] ACLK_REF_GOOD
//
// ACLK_REF_GOOD.
#define DDI_0_OSC_STAT1_ACLK_REF_GOOD                               0x00000004
#define DDI_0_OSC_STAT1_ACLK_REF_GOOD_M                             0x00000004
#define DDI_0_OSC_STAT1_ACLK_REF_GOOD_S                                      2

// Field:     [1] CLK_CHP_GOOD
//
// CLK_CHP_GOOD
#define DDI_0_OSC_STAT1_CLK_CHP_GOOD                                0x00000002
#define DDI_0_OSC_STAT1_CLK_CHP_GOOD_M                              0x00000002
#define DDI_0_OSC_STAT1_CLK_CHP_GOOD_S                                       1

// Field:     [0] CLK_DCDC_GOOD
//
// CLK_DCDC_GOOD
#define DDI_0_OSC_STAT1_CLK_DCDC_GOOD                               0x00000001
#define DDI_0_OSC_STAT1_CLK_DCDC_GOOD_M                             0x00000001
#define DDI_0_OSC_STAT1_CLK_DCDC_GOOD_S                                      0

//*****************************************************************************
//
// Register: DDI_0_OSC_O_STAT2
//
//*****************************************************************************
// Field: [31:26] ADC_DCBIAS
//
// DC Bias read by RADC during SAR mode
// The value is an unsigned integer. It is used for debug only.
#define DDI_0_OSC_STAT2_ADC_DCBIAS_W                                         6
#define DDI_0_OSC_STAT2_ADC_DCBIAS_M                                0xFC000000
#define DDI_0_OSC_STAT2_ADC_DCBIAS_S                                        26

// Field:    [25] HPM_RAMP1_THMET
//
// Indication of threshold is met for hpm_ramp1
#define DDI_0_OSC_STAT2_HPM_RAMP1_THMET                             0x02000000
#define DDI_0_OSC_STAT2_HPM_RAMP1_THMET_M                           0x02000000
#define DDI_0_OSC_STAT2_HPM_RAMP1_THMET_S                                   25

// Field:    [24] HPM_RAMP2_THMET
//
// Indication of threshold is met for hpm_ramp2
#define DDI_0_OSC_STAT2_HPM_RAMP2_THMET                             0x01000000
#define DDI_0_OSC_STAT2_HPM_RAMP2_THMET_M                           0x01000000
#define DDI_0_OSC_STAT2_HPM_RAMP2_THMET_S                                   24

// Field:    [23] HPM_RAMP3_THMET
//
// Indication of threshold is met for hpm_ramp3
#define DDI_0_OSC_STAT2_HPM_RAMP3_THMET                             0x00800000
#define DDI_0_OSC_STAT2_HPM_RAMP3_THMET_M                           0x00800000
#define DDI_0_OSC_STAT2_HPM_RAMP3_THMET_S                                   23

// Field: [15:12] RAMPSTATE
//
// xosc_hf amplitude compensation FSM
//
// This is identical to STAT1.RAMPSTATE. See that description for encoding.
#define DDI_0_OSC_STAT2_RAMPSTATE_W                                          4
#define DDI_0_OSC_STAT2_RAMPSTATE_M                                 0x0000F000
#define DDI_0_OSC_STAT2_RAMPSTATE_S                                         12

// Field:     [3] AMPCOMP_REQ
//
// ampcomp_req
#define DDI_0_OSC_STAT2_AMPCOMP_REQ                                 0x00000008
#define DDI_0_OSC_STAT2_AMPCOMP_REQ_M                               0x00000008
#define DDI_0_OSC_STAT2_AMPCOMP_REQ_S                                        3

// Field:     [2] XOSC_HF_AMPGOOD
//
// amplitude of xosc_hf is within the required threshold (set by DDI). Not used
// for anything just for debug/status
#define DDI_0_OSC_STAT2_XOSC_HF_AMPGOOD                             0x00000004
#define DDI_0_OSC_STAT2_XOSC_HF_AMPGOOD_M                           0x00000004
#define DDI_0_OSC_STAT2_XOSC_HF_AMPGOOD_S                                    2

// Field:     [1] XOSC_HF_FREQGOOD
//
// frequency of xosc_hf is good to use for the digital clocks
#define DDI_0_OSC_STAT2_XOSC_HF_FREQGOOD                            0x00000002
#define DDI_0_OSC_STAT2_XOSC_HF_FREQGOOD_M                          0x00000002
#define DDI_0_OSC_STAT2_XOSC_HF_FREQGOOD_S                                   1

// Field:     [0] XOSC_HF_RF_FREQGOOD
//
// frequency of xosc_hf is within +/- 20 ppm and xosc_hf is good for radio
// operations. Used for SW to start synthesizer.
#define DDI_0_OSC_STAT2_XOSC_HF_RF_FREQGOOD                         0x00000001
#define DDI_0_OSC_STAT2_XOSC_HF_RF_FREQGOOD_M                       0x00000001
#define DDI_0_OSC_STAT2_XOSC_HF_RF_FREQGOOD_S                                0


#endif // __DDI_0_OSC__
