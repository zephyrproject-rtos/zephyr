/******************************************************************************
*  Filename:       hw_adi_4_aux_h
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

#ifndef __HW_ADI_4_AUX_H__
#define __HW_ADI_4_AUX_H__

//*****************************************************************************
//
// This section defines the register offsets of
// ADI_4_AUX component
//
//*****************************************************************************
// Internal
#define ADI_4_AUX_O_MUX0                                            0x00000000

// Internal
#define ADI_4_AUX_O_MUX1                                            0x00000001

// Internal
#define ADI_4_AUX_O_MUX2                                            0x00000002

// Internal
#define ADI_4_AUX_O_MUX3                                            0x00000003

// Current Source
#define ADI_4_AUX_O_ISRC                                            0x00000004

// Comparator
#define ADI_4_AUX_O_COMP                                            0x00000005

// Internal
#define ADI_4_AUX_O_MUX4                                            0x00000007

// ADC Control 0
#define ADI_4_AUX_O_ADC0                                            0x00000008

// ADC Control 1
#define ADI_4_AUX_O_ADC1                                            0x00000009

// ADC Reference 0
#define ADI_4_AUX_O_ADCREF0                                         0x0000000A

// ADC Reference 1
#define ADI_4_AUX_O_ADCREF1                                         0x0000000B

// Internal
#define ADI_4_AUX_O_LPMBIAS                                         0x0000000E

//*****************************************************************************
//
// Register: ADI_4_AUX_O_MUX0
//
//*****************************************************************************
// Field:     [6] ADCCOMPB_IN
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// VDDR_1P8V                Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_4_AUX_MUX0_ADCCOMPB_IN                                  0x00000040
#define ADI_4_AUX_MUX0_ADCCOMPB_IN_M                                0x00000040
#define ADI_4_AUX_MUX0_ADCCOMPB_IN_S                                         6
#define ADI_4_AUX_MUX0_ADCCOMPB_IN_VDDR_1P8V                        0x00000040
#define ADI_4_AUX_MUX0_ADCCOMPB_IN_NC                               0x00000000

// Field:   [3:0] COMPA_REF
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// ADCVREFP                 Internal. Only to be used through TI provided API.
// VDDS                     Internal. Only to be used through TI provided API.
// VSS                      Internal. Only to be used through TI provided API.
// DCOUPL                   Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_4_AUX_MUX0_COMPA_REF_W                                           4
#define ADI_4_AUX_MUX0_COMPA_REF_M                                  0x0000000F
#define ADI_4_AUX_MUX0_COMPA_REF_S                                           0
#define ADI_4_AUX_MUX0_COMPA_REF_ADCVREFP                           0x00000008
#define ADI_4_AUX_MUX0_COMPA_REF_VDDS                               0x00000004
#define ADI_4_AUX_MUX0_COMPA_REF_VSS                                0x00000002
#define ADI_4_AUX_MUX0_COMPA_REF_DCOUPL                             0x00000001
#define ADI_4_AUX_MUX0_COMPA_REF_NC                                 0x00000000

//*****************************************************************************
//
// Register: ADI_4_AUX_O_MUX1
//
//*****************************************************************************
// Field:   [7:0] COMPA_IN
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// AUXIO19                  Internal. Only to be used through TI provided API.
// AUXIO20                  Internal. Only to be used through TI provided API.
// AUXIO21                  Internal. Only to be used through TI provided API.
// AUXIO22                  Internal. Only to be used through TI provided API.
// AUXIO23                  Internal. Only to be used through TI provided API.
// AUXIO24                  Internal. Only to be used through TI provided API.
// AUXIO25                  Internal. Only to be used through TI provided API.
// AUXIO26                  Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_4_AUX_MUX1_COMPA_IN_W                                            8
#define ADI_4_AUX_MUX1_COMPA_IN_M                                   0x000000FF
#define ADI_4_AUX_MUX1_COMPA_IN_S                                            0
#define ADI_4_AUX_MUX1_COMPA_IN_AUXIO19                             0x00000080
#define ADI_4_AUX_MUX1_COMPA_IN_AUXIO20                             0x00000040
#define ADI_4_AUX_MUX1_COMPA_IN_AUXIO21                             0x00000020
#define ADI_4_AUX_MUX1_COMPA_IN_AUXIO22                             0x00000010
#define ADI_4_AUX_MUX1_COMPA_IN_AUXIO23                             0x00000008
#define ADI_4_AUX_MUX1_COMPA_IN_AUXIO24                             0x00000004
#define ADI_4_AUX_MUX1_COMPA_IN_AUXIO25                             0x00000002
#define ADI_4_AUX_MUX1_COMPA_IN_AUXIO26                             0x00000001
#define ADI_4_AUX_MUX1_COMPA_IN_NC                                  0x00000000

//*****************************************************************************
//
// Register: ADI_4_AUX_O_MUX2
//
//*****************************************************************************
// Field:   [7:3] ADCCOMPB_IN
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// VDDS                     Internal. Only to be used through TI provided API.
// VSS                      Internal. Only to be used through TI provided API.
// DCOUPL                   Internal. Only to be used through TI provided API.
// ATEST1                   Internal. Only to be used through TI provided API.
// ATEST0                   Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_W                                         5
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_M                                0x000000F8
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_S                                         3
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_VDDS                             0x00000080
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_VSS                              0x00000040
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_DCOUPL                           0x00000020
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_ATEST1                           0x00000010
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_ATEST0                           0x00000008
#define ADI_4_AUX_MUX2_ADCCOMPB_IN_NC                               0x00000000

// Field:   [2:0] DAC_VREF_SEL
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// VDDS                     Internal. Only to be used through TI provided API.
// ADCREF                   Internal. Only to be used through TI provided API.
// DCOUPL                   Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_4_AUX_MUX2_DAC_VREF_SEL_W                                        3
#define ADI_4_AUX_MUX2_DAC_VREF_SEL_M                               0x00000007
#define ADI_4_AUX_MUX2_DAC_VREF_SEL_S                                        0
#define ADI_4_AUX_MUX2_DAC_VREF_SEL_VDDS                            0x00000004
#define ADI_4_AUX_MUX2_DAC_VREF_SEL_ADCREF                          0x00000002
#define ADI_4_AUX_MUX2_DAC_VREF_SEL_DCOUPL                          0x00000001
#define ADI_4_AUX_MUX2_DAC_VREF_SEL_NC                              0x00000000

//*****************************************************************************
//
// Register: ADI_4_AUX_O_MUX3
//
//*****************************************************************************
// Field:   [7:0] ADCCOMPB_IN
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// AUXIO19                  Internal. Only to be used through TI provided API.
// AUXIO20                  Internal. Only to be used through TI provided API.
// AUXIO21                  Internal. Only to be used through TI provided API.
// AUXIO22                  Internal. Only to be used through TI provided API.
// AUXIO23                  Internal. Only to be used through TI provided API.
// AUXIO24                  Internal. Only to be used through TI provided API.
// AUXIO25                  Internal. Only to be used through TI provided API.
// AUXIO26                  Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_W                                         8
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_M                                0x000000FF
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_S                                         0
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_AUXIO19                          0x00000080
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_AUXIO20                          0x00000040
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_AUXIO21                          0x00000020
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_AUXIO22                          0x00000010
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_AUXIO23                          0x00000008
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_AUXIO24                          0x00000004
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_AUXIO25                          0x00000002
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_AUXIO26                          0x00000001
#define ADI_4_AUX_MUX3_ADCCOMPB_IN_NC                               0x00000000

//*****************************************************************************
//
// Register: ADI_4_AUX_O_ISRC
//
//*****************************************************************************
// Field:   [7:2] TRIM
//
// Adjust current from current source.
//
// Output currents may be combined to get desired total current.
// ENUMs:
// 11P75U                   11.75 uA
// 4P5U                     4.5 uA
// 2P0U                     2.0 uA
// 1P0U                     1.0 uA
// 0P5U                     0.5 uA
// 0P25U                    0.25 uA
// NC                       No current connected
#define ADI_4_AUX_ISRC_TRIM_W                                                6
#define ADI_4_AUX_ISRC_TRIM_M                                       0x000000FC
#define ADI_4_AUX_ISRC_TRIM_S                                                2
#define ADI_4_AUX_ISRC_TRIM_11P75U                                  0x00000080
#define ADI_4_AUX_ISRC_TRIM_4P5U                                    0x00000040
#define ADI_4_AUX_ISRC_TRIM_2P0U                                    0x00000020
#define ADI_4_AUX_ISRC_TRIM_1P0U                                    0x00000010
#define ADI_4_AUX_ISRC_TRIM_0P5U                                    0x00000008
#define ADI_4_AUX_ISRC_TRIM_0P25U                                   0x00000004
#define ADI_4_AUX_ISRC_TRIM_NC                                      0x00000000

// Field:     [0] EN
//
// Current source enable
#define ADI_4_AUX_ISRC_EN                                           0x00000001
#define ADI_4_AUX_ISRC_EN_M                                         0x00000001
#define ADI_4_AUX_ISRC_EN_S                                                  0

//*****************************************************************************
//
// Register: ADI_4_AUX_O_COMP
//
//*****************************************************************************
// Field:     [7] COMPA_REF_RES_EN
//
// Enables 400kohm resistance from COMPA reference node to ground. Used with
// COMPA_REF_CURR_EN to generate voltage reference for cap-sense.
#define ADI_4_AUX_COMP_COMPA_REF_RES_EN                             0x00000080
#define ADI_4_AUX_COMP_COMPA_REF_RES_EN_M                           0x00000080
#define ADI_4_AUX_COMP_COMPA_REF_RES_EN_S                                    7

// Field:     [6] COMPA_REF_CURR_EN
//
// Enables 2uA IPTAT current from ISRC to COMPA reference node. Requires
// ISRC.EN = 1. Used with COMPA_REF_RES_EN to generate voltage reference for
// cap-sense.
#define ADI_4_AUX_COMP_COMPA_REF_CURR_EN                            0x00000040
#define ADI_4_AUX_COMP_COMPA_REF_CURR_EN_M                          0x00000040
#define ADI_4_AUX_COMP_COMPA_REF_CURR_EN_S                                   6

// Field:   [5:3] LPM_BIAS_WIDTH_TRIM
//
// Internal. Only to be used through TI provided API.
#define ADI_4_AUX_COMP_LPM_BIAS_WIDTH_TRIM_W                                 3
#define ADI_4_AUX_COMP_LPM_BIAS_WIDTH_TRIM_M                        0x00000038
#define ADI_4_AUX_COMP_LPM_BIAS_WIDTH_TRIM_S                                 3

// Field:     [2] COMPB_EN
//
// COMPB enable
#define ADI_4_AUX_COMP_COMPB_EN                                     0x00000004
#define ADI_4_AUX_COMP_COMPB_EN_M                                   0x00000004
#define ADI_4_AUX_COMP_COMPB_EN_S                                            2

// Field:     [0] COMPA_EN
//
// COMPA enable
#define ADI_4_AUX_COMP_COMPA_EN                                     0x00000001
#define ADI_4_AUX_COMP_COMPA_EN_M                                   0x00000001
#define ADI_4_AUX_COMP_COMPA_EN_S                                            0

//*****************************************************************************
//
// Register: ADI_4_AUX_O_MUX4
//
//*****************************************************************************
// Field:   [7:0] COMPA_REF
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// AUXIO19                  Internal. Only to be used through TI provided API.
// AUXIO20                  Internal. Only to be used through TI provided API.
// AUXIO21                  Internal. Only to be used through TI provided API.
// AUXIO22                  Internal. Only to be used through TI provided API.
// AUXIO23                  Internal. Only to be used through TI provided API.
// AUXIO24                  Internal. Only to be used through TI provided API.
// AUXIO25                  Internal. Only to be used through TI provided API.
// AUXIO26                  Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_4_AUX_MUX4_COMPA_REF_W                                           8
#define ADI_4_AUX_MUX4_COMPA_REF_M                                  0x000000FF
#define ADI_4_AUX_MUX4_COMPA_REF_S                                           0
#define ADI_4_AUX_MUX4_COMPA_REF_AUXIO19                            0x00000080
#define ADI_4_AUX_MUX4_COMPA_REF_AUXIO20                            0x00000040
#define ADI_4_AUX_MUX4_COMPA_REF_AUXIO21                            0x00000020
#define ADI_4_AUX_MUX4_COMPA_REF_AUXIO22                            0x00000010
#define ADI_4_AUX_MUX4_COMPA_REF_AUXIO23                            0x00000008
#define ADI_4_AUX_MUX4_COMPA_REF_AUXIO24                            0x00000004
#define ADI_4_AUX_MUX4_COMPA_REF_AUXIO25                            0x00000002
#define ADI_4_AUX_MUX4_COMPA_REF_AUXIO26                            0x00000001
#define ADI_4_AUX_MUX4_COMPA_REF_NC                                 0x00000000

//*****************************************************************************
//
// Register: ADI_4_AUX_O_ADC0
//
//*****************************************************************************
// Field:     [7] SMPL_MODE
//
// ADC Sampling mode:
//
// 0: Synchronous mode
// 1: Asynchronous mode
//
// The ADC does a sample-and-hold before conversion. In synchronous mode the
// sampling starts when the ADC clock detects a rising edge on the trigger
// signal. Jitter/uncertainty will be inferred in the detection if the trigger
// signal originates from a domain that is asynchronous to the ADC clock.
// SMPL_CYCLE_EXP  determines the the duration of sampling.
// Conversion starts immediately after sampling ends.
//
// In asynchronous mode the sampling is continuous when enabled. Sampling ends
// and conversion starts immediately with the rising edge of the trigger
// signal. Sampling restarts when the conversion has finished.
// Asynchronous mode is useful when it is important to avoid jitter in the
// sampling instant of an externally driven signal
#define ADI_4_AUX_ADC0_SMPL_MODE                                    0x00000080
#define ADI_4_AUX_ADC0_SMPL_MODE_M                                  0x00000080
#define ADI_4_AUX_ADC0_SMPL_MODE_S                                           7

// Field:   [6:3] SMPL_CYCLE_EXP
//
// Controls the sampling duration before conversion when the ADC is operated in
// synchronous mode (SMPL_MODE = 0). The setting has no effect in asynchronous
// mode. The sampling duration is given as 2^(SMPL_CYCLE_EXP + 1) / 6 us.
// ENUMs:
// 10P9_MS                  65536x 6 MHz clock periods = 10.9ms
// 5P46_MS                  32768x 6 MHz clock periods = 5.46ms
// 2P73_MS                  16384x 6 MHz clock periods = 2.73ms
// 1P37_MS                  8192x 6 MHz clock periods = 1.37ms
// 682_US                   4096x 6 MHz clock periods = 682us
// 341_US                   2048x 6 MHz clock periods = 341us
// 170_US                   1024x 6 MHz clock periods = 170us
// 85P3_US                  512x 6 MHz clock periods = 85.3us
// 42P6_US                  256x 6 MHz clock periods = 42.6us
// 21P3_US                  128x 6 MHz clock periods = 21.3us
// 10P6_US                  64x 6 MHz clock periods = 10.6us
// 5P3_US                   32x 6 MHz clock periods = 5.3us
// 2P7_US                   16x 6 MHz clock periods = 2.7us
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_W                                      4
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_M                             0x00000078
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_S                                      3
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_10P9_MS                       0x00000078
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_5P46_MS                       0x00000070
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_2P73_MS                       0x00000068
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_1P37_MS                       0x00000060
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_682_US                        0x00000058
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_341_US                        0x00000050
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_170_US                        0x00000048
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_85P3_US                       0x00000040
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_42P6_US                       0x00000038
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_21P3_US                       0x00000030
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_10P6_US                       0x00000028
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_5P3_US                        0x00000020
#define ADI_4_AUX_ADC0_SMPL_CYCLE_EXP_2P7_US                        0x00000018

// Field:     [1] RESET_N
//
// Reset ADC digital subchip, active low. ADC must be reset every time it is
// reconfigured.
//
// 0: Reset
// 1: Normal operation
#define ADI_4_AUX_ADC0_RESET_N                                      0x00000002
#define ADI_4_AUX_ADC0_RESET_N_M                                    0x00000002
#define ADI_4_AUX_ADC0_RESET_N_S                                             1

// Field:     [0] EN
//
// ADC Enable
//
// 0: Disable
// 1: Enable
#define ADI_4_AUX_ADC0_EN                                           0x00000001
#define ADI_4_AUX_ADC0_EN_M                                         0x00000001
#define ADI_4_AUX_ADC0_EN_S                                                  0

//*****************************************************************************
//
// Register: ADI_4_AUX_O_ADC1
//
//*****************************************************************************
// Field:     [0] SCALE_DIS
//
// Internal. Only to be used through TI provided API.
#define ADI_4_AUX_ADC1_SCALE_DIS                                    0x00000001
#define ADI_4_AUX_ADC1_SCALE_DIS_M                                  0x00000001
#define ADI_4_AUX_ADC1_SCALE_DIS_S                                           0

//*****************************************************************************
//
// Register: ADI_4_AUX_O_ADCREF0
//
//*****************************************************************************
// Field:     [6] REF_ON_IDLE
//
// Enable ADCREF in IDLE state.
//
// 0: Disabled in IDLE state
// 1: Enabled in IDLE state
//
// Keep ADCREF enabled when ADC0.SMPL_MODE = 0.
// Recommendation: Enable ADCREF always when ADC0.SMPL_CYCLE_EXP is less than
// 0x6 (21.3us sampling time).
#define ADI_4_AUX_ADCREF0_REF_ON_IDLE                               0x00000040
#define ADI_4_AUX_ADCREF0_REF_ON_IDLE_M                             0x00000040
#define ADI_4_AUX_ADCREF0_REF_ON_IDLE_S                                      6

// Field:     [5] IOMUX
//
// Internal. Only to be used through TI provided API.
#define ADI_4_AUX_ADCREF0_IOMUX                                     0x00000020
#define ADI_4_AUX_ADCREF0_IOMUX_M                                   0x00000020
#define ADI_4_AUX_ADCREF0_IOMUX_S                                            5

// Field:     [4] EXT
//
// Internal. Only to be used through TI provided API.
#define ADI_4_AUX_ADCREF0_EXT                                       0x00000010
#define ADI_4_AUX_ADCREF0_EXT_M                                     0x00000010
#define ADI_4_AUX_ADCREF0_EXT_S                                              4

// Field:     [3] SRC
//
// ADC reference source:
//
// 0: Fixed reference = 4.3V
// 1: Relative reference = VDDS
#define ADI_4_AUX_ADCREF0_SRC                                       0x00000008
#define ADI_4_AUX_ADCREF0_SRC_M                                     0x00000008
#define ADI_4_AUX_ADCREF0_SRC_S                                              3

// Field:     [0] EN
//
// ADC reference module enable:
//
// 0: ADC reference module powered down
// 1: ADC reference module enabled
#define ADI_4_AUX_ADCREF0_EN                                        0x00000001
#define ADI_4_AUX_ADCREF0_EN_M                                      0x00000001
#define ADI_4_AUX_ADCREF0_EN_S                                               0

//*****************************************************************************
//
// Register: ADI_4_AUX_O_ADCREF1
//
//*****************************************************************************
// Field:   [5:0] VTRIM
//
// Trim output voltage of ADC fixed reference (64 steps, 2's complement).
// Applies only for ADCREF0.SRC = 0.
//
// Examples:
// 0x00 - nominal voltage 1.43V
// 0x01 - nominal + 0.4% 1.435V
// 0x3F - nominal - 0.4% 1.425V
// 0x1F - maximum voltage 1.6V
// 0x20 - minimum voltage 1.3V
#define ADI_4_AUX_ADCREF1_VTRIM_W                                            6
#define ADI_4_AUX_ADCREF1_VTRIM_M                                   0x0000003F
#define ADI_4_AUX_ADCREF1_VTRIM_S                                            0

//*****************************************************************************
//
// Register: ADI_4_AUX_O_LPMBIAS
//
//*****************************************************************************
// Field:   [5:0] LPM_TRIM_IOUT
//
// Internal. Only to be used through TI provided API.
#define ADI_4_AUX_LPMBIAS_LPM_TRIM_IOUT_W                                    6
#define ADI_4_AUX_LPMBIAS_LPM_TRIM_IOUT_M                           0x0000003F
#define ADI_4_AUX_LPMBIAS_LPM_TRIM_IOUT_S                                    0


#endif // __ADI_4_AUX__
