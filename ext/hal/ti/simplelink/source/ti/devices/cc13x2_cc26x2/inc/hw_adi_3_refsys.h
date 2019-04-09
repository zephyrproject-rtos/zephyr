/******************************************************************************
*  Filename:       hw_adi_3_refsys_h
*  Revised:        2018-09-27 10:33:21 +0200 (Thu, 27 Sep 2018)
*  Revision:       52772
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

#ifndef __HW_ADI_3_REFSYS_H__
#define __HW_ADI_3_REFSYS_H__

//*****************************************************************************
//
// This section defines the register offsets of
// ADI_3_REFSYS component
//
//*****************************************************************************
// Internal
#define ADI_3_REFSYS_O_ATESTCTL1                                    0x00000001

// Internal
#define ADI_3_REFSYS_O_REFSYSCTL0                                   0x00000002

// Internal
#define ADI_3_REFSYS_O_REFSYSCTL1                                   0x00000003

// Internal
#define ADI_3_REFSYS_O_REFSYSCTL2                                   0x00000004

// Internal
#define ADI_3_REFSYS_O_REFSYSCTL3                                   0x00000005

// DCDC Control 0
#define ADI_3_REFSYS_O_DCDCCTL0                                     0x00000006

// DCDC Control 1
#define ADI_3_REFSYS_O_DCDCCTL1                                     0x00000007

// DCDC Control 2
#define ADI_3_REFSYS_O_DCDCCTL2                                     0x00000008

// Internal
#define ADI_3_REFSYS_O_DCDCCTL3                                     0x00000009

// Internal
#define ADI_3_REFSYS_O_DCDCCTL4                                     0x0000000A

// Internal
#define ADI_3_REFSYS_O_DCDCCTL5                                     0x0000000B

// RECHARGE_CONTROL_1
#define ADI_3_REFSYS_O_AUX_DEBUG                                    0x0000000C

// Recharge Comparator Control Byte 0
#define ADI_3_REFSYS_O_CTL_RECHARGE_CMP0                            0x0000000D

// Recharge Comparator Control Byte 1
#define ADI_3_REFSYS_O_CTL_RECHARGE_CMP1                            0x0000000E

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_ATESTCTL1
//
//*****************************************************************************
// Field:   [4:3] ATEST0_CTL
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// ICELL_A0                 Internal. Only to be used through TI provided API.
// IREF_A0                  Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_ATESTCTL1_ATEST0_CTL_W                                  2
#define ADI_3_REFSYS_ATESTCTL1_ATEST0_CTL_M                         0x00000018
#define ADI_3_REFSYS_ATESTCTL1_ATEST0_CTL_S                                  3
#define ADI_3_REFSYS_ATESTCTL1_ATEST0_CTL_ICELL_A0                  0x00000010
#define ADI_3_REFSYS_ATESTCTL1_ATEST0_CTL_IREF_A0                   0x00000008
#define ADI_3_REFSYS_ATESTCTL1_ATEST0_CTL_NC                        0x00000000

// Field:   [2:0] ATEST1_CTL
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// VREFM_A1                 Internal. Only to be used through TI provided API.
// VPP_DIV5_A1              Internal. Only to be used through TI provided API.
// VREAD_DIV2_A1            Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_ATESTCTL1_ATEST1_CTL_W                                  3
#define ADI_3_REFSYS_ATESTCTL1_ATEST1_CTL_M                         0x00000007
#define ADI_3_REFSYS_ATESTCTL1_ATEST1_CTL_S                                  0
#define ADI_3_REFSYS_ATESTCTL1_ATEST1_CTL_VREFM_A1                  0x00000004
#define ADI_3_REFSYS_ATESTCTL1_ATEST1_CTL_VPP_DIV5_A1               0x00000002
#define ADI_3_REFSYS_ATESTCTL1_ATEST1_CTL_VREAD_DIV2_A1             0x00000001
#define ADI_3_REFSYS_ATESTCTL1_ATEST1_CTL_NC                        0x00000000

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_REFSYSCTL0
//
//*****************************************************************************
// Field:   [7:0] TESTCTL
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// BMCOMPOUT                Internal. Only to be used through TI provided API.
// VTEMP                    Internal. Only to be used through TI provided API.
// VREF0P8V                 Internal. Only to be used through TI provided API.
// VBGUNBUFF                Internal. Only to be used through TI provided API.
// VBG                      Internal. Only to be used through TI provided API.
// IREF4U                   Internal. Only to be used through TI provided API.
// IVREF4U                  Internal. Only to be used through TI provided API.
// IPTAT2U                  Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_W                                    8
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_M                           0x000000FF
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_S                                    0
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_BMCOMPOUT                   0x00000080
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_VTEMP                       0x00000040
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_VREF0P8V                    0x00000020
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_VBGUNBUFF                   0x00000010
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_VBG                         0x00000008
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_IREF4U                      0x00000004
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_IVREF4U                     0x00000002
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_IPTAT2U                     0x00000001
#define ADI_3_REFSYS_REFSYSCTL0_TESTCTL_NC                          0x00000000

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_REFSYSCTL1
//
//*****************************************************************************
// Field:   [7:3] TRIM_VDDS_BOD
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// POS_27                   Internal. Only to be used through TI provided API.
// POS_26                   Internal. Only to be used through TI provided API.
// POS_25                   Internal. Only to be used through TI provided API.
// POS_24                   Internal. Only to be used through TI provided API.
// POS_31                   Internal. Only to be used through TI provided API.
// POS_30                   Internal. Only to be used through TI provided API.
// POS_29                   Internal. Only to be used through TI provided API.
// POS_28                   Internal. Only to be used through TI provided API.
// POS_19                   Internal. Only to be used through TI provided API.
// POS_18                   Internal. Only to be used through TI provided API.
// POS_17                   Internal. Only to be used through TI provided API.
// POS_16                   Internal. Only to be used through TI provided API.
// POS_23                   Internal. Only to be used through TI provided API.
// POS_22                   Internal. Only to be used through TI provided API.
// POS_21                   Internal. Only to be used through TI provided API.
// POS_20                   Internal. Only to be used through TI provided API.
// POS_11                   Internal. Only to be used through TI provided API.
// POS_10                   Internal. Only to be used through TI provided API.
// POS_9                    Internal. Only to be used through TI provided API.
// POS_8                    Internal. Only to be used through TI provided API.
// POS_15                   Internal. Only to be used through TI provided API.
// POS_14                   Internal. Only to be used through TI provided API.
// POS_13                   Internal. Only to be used through TI provided API.
// POS_12                   Internal. Only to be used through TI provided API.
// POS_3                    Internal. Only to be used through TI provided API.
// POS_2                    Internal. Only to be used through TI provided API.
// POS_1                    Internal. Only to be used through TI provided API.
// POS_0                    Internal. Only to be used through TI provided API.
// POS_7                    Internal. Only to be used through TI provided API.
// POS_6                    Internal. Only to be used through TI provided API.
// POS_5                    Internal. Only to be used through TI provided API.
// POS_4                    Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_W                              5
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_M                     0x000000F8
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_S                              3
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_27                0x000000F8
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_26                0x000000F0
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_25                0x000000E8
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_24                0x000000E0
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_31                0x000000D8
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_30                0x000000D0
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_29                0x000000C8
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_28                0x000000C0
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_19                0x000000B8
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_18                0x000000B0
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_17                0x000000A8
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_16                0x000000A0
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_23                0x00000098
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_22                0x00000090
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_21                0x00000088
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_20                0x00000080
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_11                0x00000078
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_10                0x00000070
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_9                 0x00000068
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_8                 0x00000060
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_15                0x00000058
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_14                0x00000050
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_13                0x00000048
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_12                0x00000040
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_3                 0x00000038
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_2                 0x00000030
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_1                 0x00000028
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_0                 0x00000020
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_7                 0x00000018
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_6                 0x00000010
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_5                 0x00000008
#define ADI_3_REFSYS_REFSYSCTL1_TRIM_VDDS_BOD_POS_4                 0x00000000

// Field:     [2] BATMON_COMP_TEST_EN
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// EN                       Internal. Only to be used through TI provided API.
// DIS                      Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL1_BATMON_COMP_TEST_EN                 0x00000004
#define ADI_3_REFSYS_REFSYSCTL1_BATMON_COMP_TEST_EN_M               0x00000004
#define ADI_3_REFSYS_REFSYSCTL1_BATMON_COMP_TEST_EN_S                        2
#define ADI_3_REFSYS_REFSYSCTL1_BATMON_COMP_TEST_EN_EN              0x00000004
#define ADI_3_REFSYS_REFSYSCTL1_BATMON_COMP_TEST_EN_DIS             0x00000000

// Field:   [1:0] TESTCTL
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// IPTAT1U                  Internal. Only to be used through TI provided API.
// BMCOMPIN                 Internal. Only to be used through TI provided API.
// NC                       Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL1_TESTCTL_W                                    2
#define ADI_3_REFSYS_REFSYSCTL1_TESTCTL_M                           0x00000003
#define ADI_3_REFSYS_REFSYSCTL1_TESTCTL_S                                    0
#define ADI_3_REFSYS_REFSYSCTL1_TESTCTL_IPTAT1U                     0x00000002
#define ADI_3_REFSYS_REFSYSCTL1_TESTCTL_BMCOMPIN                    0x00000001
#define ADI_3_REFSYS_REFSYSCTL1_TESTCTL_NC                          0x00000000

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_REFSYSCTL2
//
//*****************************************************************************
// Field:   [7:4] TRIM_VREF
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL2_TRIM_VREF_W                                  4
#define ADI_3_REFSYS_REFSYSCTL2_TRIM_VREF_M                         0x000000F0
#define ADI_3_REFSYS_REFSYSCTL2_TRIM_VREF_S                                  4

// Field:     [3] BOD_EXTERNAL_REG_MODE
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL2_BOD_EXTERNAL_REG_MODE               0x00000008
#define ADI_3_REFSYS_REFSYSCTL2_BOD_EXTERNAL_REG_MODE_M             0x00000008
#define ADI_3_REFSYS_REFSYSCTL2_BOD_EXTERNAL_REG_MODE_S                      3

// Field:   [1:0] TRIM_TSENSE
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL2_TRIM_TSENSE_W                                2
#define ADI_3_REFSYS_REFSYSCTL2_TRIM_TSENSE_M                       0x00000003
#define ADI_3_REFSYS_REFSYSCTL2_TRIM_TSENSE_S                                0

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_REFSYSCTL3
//
//*****************************************************************************
// Field:     [7] BOD_BG_TRIM_EN
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL3_BOD_BG_TRIM_EN                      0x00000080
#define ADI_3_REFSYS_REFSYSCTL3_BOD_BG_TRIM_EN_M                    0x00000080
#define ADI_3_REFSYS_REFSYSCTL3_BOD_BG_TRIM_EN_S                             7

// Field:     [6] VTEMP_EN
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// EN                       Internal. Only to be used through TI provided API.
// DIS                      Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL3_VTEMP_EN                            0x00000040
#define ADI_3_REFSYS_REFSYSCTL3_VTEMP_EN_M                          0x00000040
#define ADI_3_REFSYS_REFSYSCTL3_VTEMP_EN_S                                   6
#define ADI_3_REFSYS_REFSYSCTL3_VTEMP_EN_EN                         0x00000040
#define ADI_3_REFSYS_REFSYSCTL3_VTEMP_EN_DIS                        0x00000000

// Field:   [5:0] TRIM_VBG
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_REFSYSCTL3_TRIM_VBG_W                                   6
#define ADI_3_REFSYS_REFSYSCTL3_TRIM_VBG_M                          0x0000003F
#define ADI_3_REFSYS_REFSYSCTL3_TRIM_VBG_S                                   0

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_DCDCCTL0
//
//*****************************************************************************
// Field:   [7:5] GLDO_ISRC
//
// Set charge and re-charge current level.
// 2's complement encoding.
//
// 0x0: Default 11mA.
// 0x3: Max 15mA.
// 0x4: Max 5mA
#define ADI_3_REFSYS_DCDCCTL0_GLDO_ISRC_W                                    3
#define ADI_3_REFSYS_DCDCCTL0_GLDO_ISRC_M                           0x000000E0
#define ADI_3_REFSYS_DCDCCTL0_GLDO_ISRC_S                                    5

// Field:   [4:0] VDDR_TRIM
//
// Set the VDDR voltage.
// Proprietary encoding.
//
// Increase voltage to max: 0x00, 0x01, 0x02 ... 0x15.
// Decrease voltage to min: 0x00, 0x1F, 0x1E, 0x1D ... 0x16.
// Step size = 16mV
//
// 0x00: Default, about 1.63V.
// 0x05: Typical voltage after trim voltage 1.71V.
// 0x15: Max voltage 1.96V.
// 0x16: Min voltage 1.47V.
#define ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_W                                    5
#define ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_M                           0x0000001F
#define ADI_3_REFSYS_DCDCCTL0_VDDR_TRIM_S                                    0

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_DCDCCTL1
//
//*****************************************************************************
// Field:   [7:6] IPTAT_TRIM
//
// Trim GLDO bias current.
// Proprietary encoding.
//
// 0x0: Default
// 0x1: Increase GLDO bias by 1.3x.
// 0x2: Increase GLDO bias by 1.6x.
// 0x3: Decrease GLDO bias by 0.7x.
#define ADI_3_REFSYS_DCDCCTL1_IPTAT_TRIM_W                                   2
#define ADI_3_REFSYS_DCDCCTL1_IPTAT_TRIM_M                          0x000000C0
#define ADI_3_REFSYS_DCDCCTL1_IPTAT_TRIM_S                                   6

// Field:     [5] VDDR_OK_HYST
//
// Increase the hysteresis for when VDDR is considered ok.
//
// 0: Hysteresis = 60mV
// 1: Hysteresis = 70mV
#define ADI_3_REFSYS_DCDCCTL1_VDDR_OK_HYST                          0x00000020
#define ADI_3_REFSYS_DCDCCTL1_VDDR_OK_HYST_M                        0x00000020
#define ADI_3_REFSYS_DCDCCTL1_VDDR_OK_HYST_S                                 5

// Field:   [4:0] VDDR_TRIM_SLEEP
//
// Set the min VDDR voltage threshold during sleep mode.
// Proprietary encoding.
//
// Increase voltage to max: 0x00, 0x01, 0x02 ... 0x15.
// Decrease voltage to min: 0x00, 0x1F, 0x1E, 0x1D ... 0x16.
// Step size = 16mV
//
// 0x00: Default, about 1.63V.
// 0x19: Typical voltage after trim voltage 1.52V.
// 0x15: Max voltage 1.96V.
// 0x16: Min voltage 1.47V.
#define ADI_3_REFSYS_DCDCCTL1_VDDR_TRIM_SLEEP_W                              5
#define ADI_3_REFSYS_DCDCCTL1_VDDR_TRIM_SLEEP_M                     0x0000001F
#define ADI_3_REFSYS_DCDCCTL1_VDDR_TRIM_SLEEP_S                              0

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_DCDCCTL2
//
//*****************************************************************************
// Field:     [6] TURNON_EA_SW
//
// Turn on erroramp switch
//
// 0: Erroramp Off (Default)
// 1: Erroramp On. Turns on GLDO error amp switch.
#define ADI_3_REFSYS_DCDCCTL2_TURNON_EA_SW                          0x00000040
#define ADI_3_REFSYS_DCDCCTL2_TURNON_EA_SW_M                        0x00000040
#define ADI_3_REFSYS_DCDCCTL2_TURNON_EA_SW_S                                 6

// Field:     [5] TEST_VDDR
//
// Connect VDDR to ATEST bus
//
// 0: Not connected.
// 1: Connected
//
// Set TESTSEL = 0x0 first before setting this bit.
#define ADI_3_REFSYS_DCDCCTL2_TEST_VDDR                             0x00000020
#define ADI_3_REFSYS_DCDCCTL2_TEST_VDDR_M                           0x00000020
#define ADI_3_REFSYS_DCDCCTL2_TEST_VDDR_S                                    5

// Field:     [4] BIAS_DIS
//
// Disable dummy bias current.
//
// 0: Dummy bias current on (Default)
// 1: Dummy bias current off
#define ADI_3_REFSYS_DCDCCTL2_BIAS_DIS                              0x00000010
#define ADI_3_REFSYS_DCDCCTL2_BIAS_DIS_M                            0x00000010
#define ADI_3_REFSYS_DCDCCTL2_BIAS_DIS_S                                     4

// Field:   [3:0] TESTSEL
//
// Select signal for test bus, one hot.
// ENUMs:
// VDDROK                   VDDR_OK connected to test bus.
// IB1U                     1uA bias current connected to test bus.
// PASSGATE                 Pass transistor gate voltage connected to test
//                          bus.
// ERRAMP_OUT               Error amp output voltage connected to test bus.
// NC                       No signal connected to test bus.
#define ADI_3_REFSYS_DCDCCTL2_TESTSEL_W                                      4
#define ADI_3_REFSYS_DCDCCTL2_TESTSEL_M                             0x0000000F
#define ADI_3_REFSYS_DCDCCTL2_TESTSEL_S                                      0
#define ADI_3_REFSYS_DCDCCTL2_TESTSEL_VDDROK                        0x00000008
#define ADI_3_REFSYS_DCDCCTL2_TESTSEL_IB1U                          0x00000004
#define ADI_3_REFSYS_DCDCCTL2_TESTSEL_PASSGATE                      0x00000002
#define ADI_3_REFSYS_DCDCCTL2_TESTSEL_ERRAMP_OUT                    0x00000001
#define ADI_3_REFSYS_DCDCCTL2_TESTSEL_NC                            0x00000000

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_DCDCCTL3
//
//*****************************************************************************
// Field:   [1:0] VDDR_BOOST_COMP
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// BOOST_P1                 Internal. Only to be used through TI provided API.
// BOOST                    Internal. Only to be used through TI provided API.
// BOOST_N1                 Internal. Only to be used through TI provided API.
// DEFAULT                  Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_DCDCCTL3_VDDR_BOOST_COMP_W                              2
#define ADI_3_REFSYS_DCDCCTL3_VDDR_BOOST_COMP_M                     0x00000003
#define ADI_3_REFSYS_DCDCCTL3_VDDR_BOOST_COMP_S                              0
#define ADI_3_REFSYS_DCDCCTL3_VDDR_BOOST_COMP_BOOST_P1              0x00000003
#define ADI_3_REFSYS_DCDCCTL3_VDDR_BOOST_COMP_BOOST                 0x00000002
#define ADI_3_REFSYS_DCDCCTL3_VDDR_BOOST_COMP_BOOST_N1              0x00000001
#define ADI_3_REFSYS_DCDCCTL3_VDDR_BOOST_COMP_DEFAULT               0x00000000

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_DCDCCTL4
//
//*****************************************************************************
// Field:   [7:6] DEADTIME_TRIM
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_DCDCCTL4_DEADTIME_TRIM_W                                2
#define ADI_3_REFSYS_DCDCCTL4_DEADTIME_TRIM_M                       0x000000C0
#define ADI_3_REFSYS_DCDCCTL4_DEADTIME_TRIM_S                                6

// Field:   [5:3] LOW_EN_SEL
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_DCDCCTL4_LOW_EN_SEL_W                                   3
#define ADI_3_REFSYS_DCDCCTL4_LOW_EN_SEL_M                          0x00000038
#define ADI_3_REFSYS_DCDCCTL4_LOW_EN_SEL_S                                   3

// Field:   [2:0] HIGH_EN_SEL
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_DCDCCTL4_HIGH_EN_SEL_W                                  3
#define ADI_3_REFSYS_DCDCCTL4_HIGH_EN_SEL_M                         0x00000007
#define ADI_3_REFSYS_DCDCCTL4_HIGH_EN_SEL_S                                  0

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_DCDCCTL5
//
//*****************************************************************************
// Field:     [5] TESTN
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_DCDCCTL5_TESTN                                 0x00000020
#define ADI_3_REFSYS_DCDCCTL5_TESTN_M                               0x00000020
#define ADI_3_REFSYS_DCDCCTL5_TESTN_S                                        5

// Field:     [4] TESTP
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_DCDCCTL5_TESTP                                 0x00000010
#define ADI_3_REFSYS_DCDCCTL5_TESTP_M                               0x00000010
#define ADI_3_REFSYS_DCDCCTL5_TESTP_S                                        4

// Field:     [3] DITHER_EN
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// EN                       Internal. Only to be used through TI provided API.
// DIS                      Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_DCDCCTL5_DITHER_EN                             0x00000008
#define ADI_3_REFSYS_DCDCCTL5_DITHER_EN_M                           0x00000008
#define ADI_3_REFSYS_DCDCCTL5_DITHER_EN_S                                    3
#define ADI_3_REFSYS_DCDCCTL5_DITHER_EN_EN                          0x00000008
#define ADI_3_REFSYS_DCDCCTL5_DITHER_EN_DIS                         0x00000000

// Field:   [2:0] IPEAK
//
// Internal. Only to be used through TI provided API.
#define ADI_3_REFSYS_DCDCCTL5_IPEAK_W                                        3
#define ADI_3_REFSYS_DCDCCTL5_IPEAK_M                               0x00000007
#define ADI_3_REFSYS_DCDCCTL5_IPEAK_S                                        0

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_AUX_DEBUG
//
//*****************************************************************************
// Field:     [6] LPM_BIAS_BACKUP_EN
//
// Activate the backup circuit in case the main circuit does not work
#define ADI_3_REFSYS_AUX_DEBUG_LPM_BIAS_BACKUP_EN                   0x00000040
#define ADI_3_REFSYS_AUX_DEBUG_LPM_BIAS_BACKUP_EN_M                 0x00000040
#define ADI_3_REFSYS_AUX_DEBUG_LPM_BIAS_BACKUP_EN_S                          6

// Field:     [5] DAC_DBG_OFFSET_COMP
//
// Offset compensation signal (Debug Mode)
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_OFFSET_COMP                  0x00000020
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_OFFSET_COMP_M                0x00000020
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_OFFSET_COMP_S                         5

// Field:     [4] DAC_DBG_HOLD
//
// S-H Cap hold signal (Debug Mode)
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_HOLD                         0x00000010
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_HOLD_M                       0x00000010
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_HOLD_S                                4

// Field:     [3] DAC_DBG_PRECHARGE
//
// PRE-CHARGE signal (Debug Mode)
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_PRECHARGE                    0x00000008
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_PRECHARGE_M                  0x00000008
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_PRECHARGE_S                           3

// Field:     [2] DAC_DBG_CAP_SAMPLE
//
// Cap-array sample signal (Debug Mode)
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_CAP_SAMPLE                   0x00000004
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_CAP_SAMPLE_M                 0x00000004
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_CAP_SAMPLE_S                          2

// Field:     [1] DAC_DBG_SAMPLE
//
// S-H Cap sample signal (Debug Mode)
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_SAMPLE                       0x00000002
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_SAMPLE_M                     0x00000002
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_SAMPLE_S                              1

// Field:     [0] DAC_DBG_EN
//
// Enable Debug Mode
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_EN                           0x00000001
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_EN_M                         0x00000001
#define ADI_3_REFSYS_AUX_DEBUG_DAC_DBG_EN_S                                  0

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_CTL_RECHARGE_CMP0
//
//*****************************************************************************
// Field:     [4] COMP_CLK_DISABLE
//
// Enable/Disable the 32 kHz clock (SCLK_LF)  to the recharge comparator
// ENUMs:
// DIS                      Disable the clock
// EN                       Enable the clock
#define ADI_3_REFSYS_CTL_RECHARGE_CMP0_COMP_CLK_DISABLE             0x00000010
#define ADI_3_REFSYS_CTL_RECHARGE_CMP0_COMP_CLK_DISABLE_M           0x00000010
#define ADI_3_REFSYS_CTL_RECHARGE_CMP0_COMP_CLK_DISABLE_S                    4
#define ADI_3_REFSYS_CTL_RECHARGE_CMP0_COMP_CLK_DISABLE_DIS         0x00000010
#define ADI_3_REFSYS_CTL_RECHARGE_CMP0_COMP_CLK_DISABLE_EN          0x00000000

// Field:   [3:0] TRIM_RECHARGE_COMP_REFLEVEL
//
// Trim ref level of recharge.
//
// 0xF:  90% of VDDR level.
// 0x0:  100% of VDDR level.
//
// Step size = 0.67% of VDDR level.
#define ADI_3_REFSYS_CTL_RECHARGE_CMP0_TRIM_RECHARGE_COMP_REFLEVEL_W \
                                                                             4
#define ADI_3_REFSYS_CTL_RECHARGE_CMP0_TRIM_RECHARGE_COMP_REFLEVEL_M \
                                                                    0x0000000F
#define ADI_3_REFSYS_CTL_RECHARGE_CMP0_TRIM_RECHARGE_COMP_REFLEVEL_S \
                                                                             0

//*****************************************************************************
//
// Register: ADI_3_REFSYS_O_CTL_RECHARGE_CMP1
//
//*****************************************************************************
// Field:     [7] RECHARGE_BLOCK_VTRIG_EN
//
// Enable/Disable ATEST input to VDDR input of recharge comparator. Used for
// trimming the recharge voltage reference level
// ENUMs:
// EN                       Enable. VDDR input is connected to ATEST network
// DIS                      Disable. VDDR input is connected to VDDR itself
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_VTRIG_EN      0x00000080
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_VTRIG_EN_M     \
                                                                    0x00000080
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_VTRIG_EN_S     \
                                                                             7
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_VTRIG_EN_EN    \
                                                                    0x00000080
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_VTRIG_EN_DIS   \
                                                                    0x00000000

// Field:     [6] RECHARGE_BLOCK_ATEST_EN
//
// Enable/Disable test inputs/outputs to recharge comparator block
// ENUMs:
// EN                       Enable
// DIS                      Disable
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_ATEST_EN      0x00000040
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_ATEST_EN_M     \
                                                                    0x00000040
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_ATEST_EN_S     \
                                                                             6
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_ATEST_EN_EN    \
                                                                    0x00000040
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_RECHARGE_BLOCK_ATEST_EN_DIS   \
                                                                    0x00000000

// Field:     [5] FORCE_SAMPLE_VDDR
//
// Force Sample of VDDR on cap divider
// ENUMs:
// EN                       Enable
// DIS                      Disable
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_FORCE_SAMPLE_VDDR            0x00000020
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_FORCE_SAMPLE_VDDR_M          0x00000020
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_FORCE_SAMPLE_VDDR_S                   5
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_FORCE_SAMPLE_VDDR_EN         0x00000020
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_FORCE_SAMPLE_VDDR_DIS        0x00000000

// Field:   [4:0] TRIM_RECHARGE_COMP_OFFSET
//
// Trim offset of Recharge comparator.
//
// 0x00:  Maximum degeneration on input side (VDDR side).
// 0x1F:  Maximum degeneration on reference side from cap divider.
// 0x10: Nominal code.
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_TRIM_RECHARGE_COMP_OFFSET_W   \
                                                                             5
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_TRIM_RECHARGE_COMP_OFFSET_M   \
                                                                    0x0000001F
#define ADI_3_REFSYS_CTL_RECHARGE_CMP1_TRIM_RECHARGE_COMP_OFFSET_S   \
                                                                             0


#endif // __ADI_3_REFSYS__
