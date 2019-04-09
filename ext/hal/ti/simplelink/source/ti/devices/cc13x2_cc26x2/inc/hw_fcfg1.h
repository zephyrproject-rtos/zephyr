/******************************************************************************
*  Filename:       hw_fcfg1_h
*  Revised:        2018-11-06 14:08:24 +0100 (Tue, 06 Nov 2018)
*  Revision:       53237
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

#ifndef __HW_FCFG1_H__
#define __HW_FCFG1_H__

//*****************************************************************************
//
// This section defines the register offsets of
// FCFG1 component
//
//*****************************************************************************
// Misc configurations
#define FCFG1_O_MISC_CONF_1                                         0x000000A0

// Internal
#define FCFG1_O_MISC_CONF_2                                         0x000000A4

// Internal
#define FCFG1_O_HPOSC_MEAS_5                                        0x000000B0

// Internal
#define FCFG1_O_HPOSC_MEAS_4                                        0x000000B4

// Internal
#define FCFG1_O_HPOSC_MEAS_3                                        0x000000B8

// Internal
#define FCFG1_O_HPOSC_MEAS_2                                        0x000000BC

// Internal
#define FCFG1_O_HPOSC_MEAS_1                                        0x000000C0

// Internal
#define FCFG1_O_CONFIG_CC26_FE                                      0x000000C4

// Internal
#define FCFG1_O_CONFIG_CC13_FE                                      0x000000C8

// Internal
#define FCFG1_O_CONFIG_RF_COMMON                                    0x000000CC

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV2_CC26_2G4                          0x000000D0

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV2_CC13_2G4                          0x000000D4

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV2_CC26_1G                           0x000000D8

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV2_CC13_1G                           0x000000DC

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV4_CC26                              0x000000E0

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV4_CC13                              0x000000E4

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV5                                   0x000000E8

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV6_CC26                              0x000000EC

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV6_CC13                              0x000000F0

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV10                                  0x000000F4

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV12_CC26                             0x000000F8

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV12_CC13                             0x000000FC

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV15                                  0x00000100

// Internal
#define FCFG1_O_CONFIG_SYNTH_DIV30                                  0x00000104

// Flash information
#define FCFG1_O_FLASH_NUMBER                                        0x00000164

// Flash information
#define FCFG1_O_FLASH_COORDINATE                                    0x0000016C

// Internal
#define FCFG1_O_FLASH_E_P                                           0x00000170

// Internal
#define FCFG1_O_FLASH_C_E_P_R                                       0x00000174

// Internal
#define FCFG1_O_FLASH_P_R_PV                                        0x00000178

// Internal
#define FCFG1_O_FLASH_EH_SEQ                                        0x0000017C

// Internal
#define FCFG1_O_FLASH_VHV_E                                         0x00000180

// Internal
#define FCFG1_O_FLASH_PP                                            0x00000184

// Internal
#define FCFG1_O_FLASH_PROG_EP                                       0x00000188

// Internal
#define FCFG1_O_FLASH_ERA_PW                                        0x0000018C

// Internal
#define FCFG1_O_FLASH_VHV                                           0x00000190

// Internal
#define FCFG1_O_FLASH_VHV_PV                                        0x00000194

// Internal
#define FCFG1_O_FLASH_V                                             0x00000198

// User Identification.
#define FCFG1_O_USER_ID                                             0x00000294

// Internal
#define FCFG1_O_FLASH_OTP_DATA3                                     0x000002B0

// Internal
#define FCFG1_O_ANA2_TRIM                                           0x000002B4

// Internal
#define FCFG1_O_LDO_TRIM                                            0x000002B8

// MAC BLE Address 0
#define FCFG1_O_MAC_BLE_0                                           0x000002E8

// MAC BLE Address 1
#define FCFG1_O_MAC_BLE_1                                           0x000002EC

// MAC IEEE 802.15.4 Address 0
#define FCFG1_O_MAC_15_4_0                                          0x000002F0

// MAC IEEE 802.15.4 Address 1
#define FCFG1_O_MAC_15_4_1                                          0x000002F4

// Internal
#define FCFG1_O_FLASH_OTP_DATA4                                     0x00000308

// Miscellaneous Trim  Parameters
#define FCFG1_O_MISC_TRIM                                           0x0000030C

// Internal
#define FCFG1_O_RCOSC_HF_TEMPCOMP                                   0x00000310

// IcePick Device Identification
#define FCFG1_O_ICEPICK_DEVICE_ID                                   0x00000318

// Factory Configuration (FCFG1) Revision
#define FCFG1_O_FCFG1_REVISION                                      0x0000031C

// Misc OTP Data
#define FCFG1_O_MISC_OTP_DATA                                       0x00000320

// IO Configuration
#define FCFG1_O_IOCONF                                              0x00000344

// Internal
#define FCFG1_O_CONFIG_IF_ADC                                       0x0000034C

// Internal
#define FCFG1_O_CONFIG_OSC_TOP                                      0x00000350

// AUX_ADC Gain in Absolute Reference Mode
#define FCFG1_O_SOC_ADC_ABS_GAIN                                    0x0000035C

// AUX_ADC Gain in Relative Reference Mode
#define FCFG1_O_SOC_ADC_REL_GAIN                                    0x00000360

// AUX_ADC Temperature Offsets in Absolute Reference Mode
#define FCFG1_O_SOC_ADC_OFFSET_INT                                  0x00000368

// Internal
#define FCFG1_O_SOC_ADC_REF_TRIM_AND_OFFSET_EXT                     0x0000036C

// Internal
#define FCFG1_O_AMPCOMP_TH1                                         0x00000370

// Internal
#define FCFG1_O_AMPCOMP_TH2                                         0x00000374

// Internal
#define FCFG1_O_AMPCOMP_CTRL1                                       0x00000378

// Internal
#define FCFG1_O_ANABYPASS_VALUE2                                    0x0000037C

// Internal
#define FCFG1_O_VOLT_TRIM                                           0x00000388

// OSC Configuration
#define FCFG1_O_OSC_CONF                                            0x0000038C

// Internal
#define FCFG1_O_FREQ_OFFSET                                         0x00000390

// Internal
#define FCFG1_O_MISC_OTP_DATA_1                                     0x00000398

// Shadow of EFUSE:DIE_ID_0 register
#define FCFG1_O_SHDW_DIE_ID_0                                       0x000003D0

// Shadow of EFUSE:DIE_ID_1 register
#define FCFG1_O_SHDW_DIE_ID_1                                       0x000003D4

// Shadow of EFUSE:DIE_ID_2 register
#define FCFG1_O_SHDW_DIE_ID_2                                       0x000003D8

// Shadow of EFUSE:DIE_ID_3 register
#define FCFG1_O_SHDW_DIE_ID_3                                       0x000003DC

// Internal
#define FCFG1_O_SHDW_OSC_BIAS_LDO_TRIM                              0x000003F8

// Internal
#define FCFG1_O_SHDW_ANA_TRIM                                       0x000003FC

// Internal
#define FCFG1_O_DAC_BIAS_CNF                                        0x0000040C

// Internal
#define FCFG1_O_TFW_PROBE                                           0x00000418

// Internal
#define FCFG1_O_TFW_FT                                              0x0000041C

// Internal
#define FCFG1_O_DAC_CAL0                                            0x00000420

// Internal
#define FCFG1_O_DAC_CAL1                                            0x00000424

// Internal
#define FCFG1_O_DAC_CAL2                                            0x00000428

// Internal
#define FCFG1_O_DAC_CAL3                                            0x0000042C

//*****************************************************************************
//
// Register: FCFG1_O_MISC_CONF_1
//
//*****************************************************************************
// Field:   [7:0] DEVICE_MINOR_REV
//
// HW minor revision number (a value of 0xFF shall be treated equally to 0x00).
// Any test of this field by SW should be implemented as a 'greater or equal'
// comparison as signed integer.
// Value may change without warning.
#define FCFG1_MISC_CONF_1_DEVICE_MINOR_REV_W                                 8
#define FCFG1_MISC_CONF_1_DEVICE_MINOR_REV_M                        0x000000FF
#define FCFG1_MISC_CONF_1_DEVICE_MINOR_REV_S                                 0

//*****************************************************************************
//
// Register: FCFG1_O_MISC_CONF_2
//
//*****************************************************************************
// Field:   [7:0] HPOSC_COMP_P3
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_CONF_2_HPOSC_COMP_P3_W                                    8
#define FCFG1_MISC_CONF_2_HPOSC_COMP_P3_M                           0x000000FF
#define FCFG1_MISC_CONF_2_HPOSC_COMP_P3_S                                    0

//*****************************************************************************
//
// Register: FCFG1_O_HPOSC_MEAS_5
//
//*****************************************************************************
// Field: [31:16] HPOSC_D5
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_5_HPOSC_D5_W                                       16
#define FCFG1_HPOSC_MEAS_5_HPOSC_D5_M                               0xFFFF0000
#define FCFG1_HPOSC_MEAS_5_HPOSC_D5_S                                       16

// Field:  [15:8] HPOSC_T5
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_5_HPOSC_T5_W                                        8
#define FCFG1_HPOSC_MEAS_5_HPOSC_T5_M                               0x0000FF00
#define FCFG1_HPOSC_MEAS_5_HPOSC_T5_S                                        8

// Field:   [7:0] HPOSC_DT5
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_5_HPOSC_DT5_W                                       8
#define FCFG1_HPOSC_MEAS_5_HPOSC_DT5_M                              0x000000FF
#define FCFG1_HPOSC_MEAS_5_HPOSC_DT5_S                                       0

//*****************************************************************************
//
// Register: FCFG1_O_HPOSC_MEAS_4
//
//*****************************************************************************
// Field: [31:16] HPOSC_D4
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_4_HPOSC_D4_W                                       16
#define FCFG1_HPOSC_MEAS_4_HPOSC_D4_M                               0xFFFF0000
#define FCFG1_HPOSC_MEAS_4_HPOSC_D4_S                                       16

// Field:  [15:8] HPOSC_T4
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_4_HPOSC_T4_W                                        8
#define FCFG1_HPOSC_MEAS_4_HPOSC_T4_M                               0x0000FF00
#define FCFG1_HPOSC_MEAS_4_HPOSC_T4_S                                        8

// Field:   [7:0] HPOSC_DT4
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_4_HPOSC_DT4_W                                       8
#define FCFG1_HPOSC_MEAS_4_HPOSC_DT4_M                              0x000000FF
#define FCFG1_HPOSC_MEAS_4_HPOSC_DT4_S                                       0

//*****************************************************************************
//
// Register: FCFG1_O_HPOSC_MEAS_3
//
//*****************************************************************************
// Field: [31:16] HPOSC_D3
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_3_HPOSC_D3_W                                       16
#define FCFG1_HPOSC_MEAS_3_HPOSC_D3_M                               0xFFFF0000
#define FCFG1_HPOSC_MEAS_3_HPOSC_D3_S                                       16

// Field:  [15:8] HPOSC_T3
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_3_HPOSC_T3_W                                        8
#define FCFG1_HPOSC_MEAS_3_HPOSC_T3_M                               0x0000FF00
#define FCFG1_HPOSC_MEAS_3_HPOSC_T3_S                                        8

// Field:   [7:0] HPOSC_DT3
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_3_HPOSC_DT3_W                                       8
#define FCFG1_HPOSC_MEAS_3_HPOSC_DT3_M                              0x000000FF
#define FCFG1_HPOSC_MEAS_3_HPOSC_DT3_S                                       0

//*****************************************************************************
//
// Register: FCFG1_O_HPOSC_MEAS_2
//
//*****************************************************************************
// Field: [31:16] HPOSC_D2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_2_HPOSC_D2_W                                       16
#define FCFG1_HPOSC_MEAS_2_HPOSC_D2_M                               0xFFFF0000
#define FCFG1_HPOSC_MEAS_2_HPOSC_D2_S                                       16

// Field:  [15:8] HPOSC_T2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_2_HPOSC_T2_W                                        8
#define FCFG1_HPOSC_MEAS_2_HPOSC_T2_M                               0x0000FF00
#define FCFG1_HPOSC_MEAS_2_HPOSC_T2_S                                        8

// Field:   [7:0] HPOSC_DT2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_2_HPOSC_DT2_W                                       8
#define FCFG1_HPOSC_MEAS_2_HPOSC_DT2_M                              0x000000FF
#define FCFG1_HPOSC_MEAS_2_HPOSC_DT2_S                                       0

//*****************************************************************************
//
// Register: FCFG1_O_HPOSC_MEAS_1
//
//*****************************************************************************
// Field: [31:16] HPOSC_D1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_1_HPOSC_D1_W                                       16
#define FCFG1_HPOSC_MEAS_1_HPOSC_D1_M                               0xFFFF0000
#define FCFG1_HPOSC_MEAS_1_HPOSC_D1_S                                       16

// Field:  [15:8] HPOSC_T1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_1_HPOSC_T1_W                                        8
#define FCFG1_HPOSC_MEAS_1_HPOSC_T1_M                               0x0000FF00
#define FCFG1_HPOSC_MEAS_1_HPOSC_T1_S                                        8

// Field:   [7:0] HPOSC_DT1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_HPOSC_MEAS_1_HPOSC_DT1_W                                       8
#define FCFG1_HPOSC_MEAS_1_HPOSC_DT1_M                              0x000000FF
#define FCFG1_HPOSC_MEAS_1_HPOSC_DT1_S                                       0

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_CC26_FE
//
//*****************************************************************************
// Field: [31:28] IFAMP_IB
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC26_FE_IFAMP_IB_W                                      4
#define FCFG1_CONFIG_CC26_FE_IFAMP_IB_M                             0xF0000000
#define FCFG1_CONFIG_CC26_FE_IFAMP_IB_S                                     28

// Field: [27:24] LNA_IB
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC26_FE_LNA_IB_W                                        4
#define FCFG1_CONFIG_CC26_FE_LNA_IB_M                               0x0F000000
#define FCFG1_CONFIG_CC26_FE_LNA_IB_S                                       24

// Field: [23:19] IFAMP_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC26_FE_IFAMP_TRIM_W                                    5
#define FCFG1_CONFIG_CC26_FE_IFAMP_TRIM_M                           0x00F80000
#define FCFG1_CONFIG_CC26_FE_IFAMP_TRIM_S                                   19

// Field: [18:14] CTL_PA0_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC26_FE_CTL_PA0_TRIM_W                                  5
#define FCFG1_CONFIG_CC26_FE_CTL_PA0_TRIM_M                         0x0007C000
#define FCFG1_CONFIG_CC26_FE_CTL_PA0_TRIM_S                                 14

// Field:    [13] PATRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC26_FE_PATRIMCOMPLETE_N                       0x00002000
#define FCFG1_CONFIG_CC26_FE_PATRIMCOMPLETE_N_BITN                          13
#define FCFG1_CONFIG_CC26_FE_PATRIMCOMPLETE_N_M                     0x00002000
#define FCFG1_CONFIG_CC26_FE_PATRIMCOMPLETE_N_S                             13

// Field:    [12] RSSITRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC26_FE_RSSITRIMCOMPLETE_N                     0x00001000
#define FCFG1_CONFIG_CC26_FE_RSSITRIMCOMPLETE_N_BITN                        12
#define FCFG1_CONFIG_CC26_FE_RSSITRIMCOMPLETE_N_M                   0x00001000
#define FCFG1_CONFIG_CC26_FE_RSSITRIMCOMPLETE_N_S                           12

// Field:   [7:0] RSSI_OFFSET
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC26_FE_RSSI_OFFSET_W                                   8
#define FCFG1_CONFIG_CC26_FE_RSSI_OFFSET_M                          0x000000FF
#define FCFG1_CONFIG_CC26_FE_RSSI_OFFSET_S                                   0

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_CC13_FE
//
//*****************************************************************************
// Field: [31:28] IFAMP_IB
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC13_FE_IFAMP_IB_W                                      4
#define FCFG1_CONFIG_CC13_FE_IFAMP_IB_M                             0xF0000000
#define FCFG1_CONFIG_CC13_FE_IFAMP_IB_S                                     28

// Field: [27:24] LNA_IB
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC13_FE_LNA_IB_W                                        4
#define FCFG1_CONFIG_CC13_FE_LNA_IB_M                               0x0F000000
#define FCFG1_CONFIG_CC13_FE_LNA_IB_S                                       24

// Field: [23:19] IFAMP_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC13_FE_IFAMP_TRIM_W                                    5
#define FCFG1_CONFIG_CC13_FE_IFAMP_TRIM_M                           0x00F80000
#define FCFG1_CONFIG_CC13_FE_IFAMP_TRIM_S                                   19

// Field: [18:14] CTL_PA0_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC13_FE_CTL_PA0_TRIM_W                                  5
#define FCFG1_CONFIG_CC13_FE_CTL_PA0_TRIM_M                         0x0007C000
#define FCFG1_CONFIG_CC13_FE_CTL_PA0_TRIM_S                                 14

// Field:    [13] PATRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC13_FE_PATRIMCOMPLETE_N                       0x00002000
#define FCFG1_CONFIG_CC13_FE_PATRIMCOMPLETE_N_BITN                          13
#define FCFG1_CONFIG_CC13_FE_PATRIMCOMPLETE_N_M                     0x00002000
#define FCFG1_CONFIG_CC13_FE_PATRIMCOMPLETE_N_S                             13

// Field:    [12] RSSITRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC13_FE_RSSITRIMCOMPLETE_N                     0x00001000
#define FCFG1_CONFIG_CC13_FE_RSSITRIMCOMPLETE_N_BITN                        12
#define FCFG1_CONFIG_CC13_FE_RSSITRIMCOMPLETE_N_M                   0x00001000
#define FCFG1_CONFIG_CC13_FE_RSSITRIMCOMPLETE_N_S                           12

// Field:   [7:0] RSSI_OFFSET
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_CC13_FE_RSSI_OFFSET_W                                   8
#define FCFG1_CONFIG_CC13_FE_RSSI_OFFSET_M                          0x000000FF
#define FCFG1_CONFIG_CC13_FE_RSSI_OFFSET_S                                   0

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_RF_COMMON
//
//*****************************************************************************
// Field:    [31] DISABLE_CORNER_CAP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_RF_COMMON_DISABLE_CORNER_CAP                   0x80000000
#define FCFG1_CONFIG_RF_COMMON_DISABLE_CORNER_CAP_BITN                      31
#define FCFG1_CONFIG_RF_COMMON_DISABLE_CORNER_CAP_M                 0x80000000
#define FCFG1_CONFIG_RF_COMMON_DISABLE_CORNER_CAP_S                         31

// Field: [30:25] SLDO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_RF_COMMON_SLDO_TRIM_OUTPUT_W                            6
#define FCFG1_CONFIG_RF_COMMON_SLDO_TRIM_OUTPUT_M                   0x7E000000
#define FCFG1_CONFIG_RF_COMMON_SLDO_TRIM_OUTPUT_S                           25

// Field:    [21] PA20DBMTRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_RF_COMMON_PA20DBMTRIMCOMPLETE_N                0x00200000
#define FCFG1_CONFIG_RF_COMMON_PA20DBMTRIMCOMPLETE_N_BITN                   21
#define FCFG1_CONFIG_RF_COMMON_PA20DBMTRIMCOMPLETE_N_M              0x00200000
#define FCFG1_CONFIG_RF_COMMON_PA20DBMTRIMCOMPLETE_N_S                      21

// Field: [20:16] CTL_PA_20DBM_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_RF_COMMON_CTL_PA_20DBM_TRIM_W                           5
#define FCFG1_CONFIG_RF_COMMON_CTL_PA_20DBM_TRIM_M                  0x001F0000
#define FCFG1_CONFIG_RF_COMMON_CTL_PA_20DBM_TRIM_S                          16

// Field:  [15:9] RFLDO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_RF_COMMON_RFLDO_TRIM_OUTPUT_W                           7
#define FCFG1_CONFIG_RF_COMMON_RFLDO_TRIM_OUTPUT_M                  0x0000FE00
#define FCFG1_CONFIG_RF_COMMON_RFLDO_TRIM_OUTPUT_S                           9

// Field:   [8:6] QUANTCTLTHRES
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_RF_COMMON_QUANTCTLTHRES_W                               3
#define FCFG1_CONFIG_RF_COMMON_QUANTCTLTHRES_M                      0x000001C0
#define FCFG1_CONFIG_RF_COMMON_QUANTCTLTHRES_S                               6

// Field:   [5:0] DACTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_RF_COMMON_DACTRIM_W                                     6
#define FCFG1_CONFIG_RF_COMMON_DACTRIM_M                            0x0000003F
#define FCFG1_CONFIG_RF_COMMON_DACTRIM_S                                     0

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV2_CC26_2G4
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_MIN_ALLOWED_RTRIM_W                 4
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_MIN_ALLOWED_RTRIM_M        0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_MIN_ALLOWED_RTRIM_S                28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_RFC_MDM_DEMIQMC0_W                 16
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_RFC_MDM_DEMIQMC0_M         0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_RFC_MDM_DEMIQMC0_S                 12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_LDOVCO_TRIM_OUTPUT_W                6
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_LDOVCO_TRIM_OUTPUT_M       0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_LDOVCO_TRIM_OUTPUT_S                6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_2G4_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV2_CC13_2G4
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_MIN_ALLOWED_RTRIM_W                 4
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_MIN_ALLOWED_RTRIM_M        0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_MIN_ALLOWED_RTRIM_S                28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_RFC_MDM_DEMIQMC0_W                 16
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_RFC_MDM_DEMIQMC0_M         0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_RFC_MDM_DEMIQMC0_S                 12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_LDOVCO_TRIM_OUTPUT_W                6
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_LDOVCO_TRIM_OUTPUT_M       0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_LDOVCO_TRIM_OUTPUT_S                6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_2G4_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV2_CC26_1G
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_MIN_ALLOWED_RTRIM_W                  4
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_MIN_ALLOWED_RTRIM_M         0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_MIN_ALLOWED_RTRIM_S                 28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_RFC_MDM_DEMIQMC0_W                  16
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_RFC_MDM_DEMIQMC0_M          0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_RFC_MDM_DEMIQMC0_S                  12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_LDOVCO_TRIM_OUTPUT_W                 6
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_LDOVCO_TRIM_OUTPUT_M        0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_LDOVCO_TRIM_OUTPUT_S                 6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV2_CC26_1G_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV2_CC13_1G
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_MIN_ALLOWED_RTRIM_W                  4
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_MIN_ALLOWED_RTRIM_M         0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_MIN_ALLOWED_RTRIM_S                 28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_RFC_MDM_DEMIQMC0_W                  16
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_RFC_MDM_DEMIQMC0_M          0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_RFC_MDM_DEMIQMC0_S                  12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_LDOVCO_TRIM_OUTPUT_W                 6
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_LDOVCO_TRIM_OUTPUT_M        0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_LDOVCO_TRIM_OUTPUT_S                 6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV2_CC13_1G_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV4_CC26
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_MIN_ALLOWED_RTRIM_W                     4
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_MIN_ALLOWED_RTRIM_M            0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_MIN_ALLOWED_RTRIM_S                    28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_RFC_MDM_DEMIQMC0_W                     16
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_RFC_MDM_DEMIQMC0_M             0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_RFC_MDM_DEMIQMC0_S                     12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_LDOVCO_TRIM_OUTPUT_W                    6
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_LDOVCO_TRIM_OUTPUT_M           0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_LDOVCO_TRIM_OUTPUT_S                    6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV4_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV4_CC13
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_MIN_ALLOWED_RTRIM_W                     4
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_MIN_ALLOWED_RTRIM_M            0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_MIN_ALLOWED_RTRIM_S                    28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_RFC_MDM_DEMIQMC0_W                     16
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_RFC_MDM_DEMIQMC0_M             0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_RFC_MDM_DEMIQMC0_S                     12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_LDOVCO_TRIM_OUTPUT_W                    6
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_LDOVCO_TRIM_OUTPUT_M           0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_LDOVCO_TRIM_OUTPUT_S                    6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV4_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV5
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV5_MIN_ALLOWED_RTRIM_W                          4
#define FCFG1_CONFIG_SYNTH_DIV5_MIN_ALLOWED_RTRIM_M                 0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV5_MIN_ALLOWED_RTRIM_S                         28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV5_RFC_MDM_DEMIQMC0_W                          16
#define FCFG1_CONFIG_SYNTH_DIV5_RFC_MDM_DEMIQMC0_M                  0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV5_RFC_MDM_DEMIQMC0_S                          12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV5_LDOVCO_TRIM_OUTPUT_W                         6
#define FCFG1_CONFIG_SYNTH_DIV5_LDOVCO_TRIM_OUTPUT_M                0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV5_LDOVCO_TRIM_OUTPUT_S                         6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV5_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N     0x00000020
#define FCFG1_CONFIG_SYNTH_DIV5_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV5_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M    \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV5_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S    \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV6_CC26
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_MIN_ALLOWED_RTRIM_W                     4
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_MIN_ALLOWED_RTRIM_M            0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_MIN_ALLOWED_RTRIM_S                    28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_RFC_MDM_DEMIQMC0_W                     16
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_RFC_MDM_DEMIQMC0_M             0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_RFC_MDM_DEMIQMC0_S                     12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_LDOVCO_TRIM_OUTPUT_W                    6
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_LDOVCO_TRIM_OUTPUT_M           0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_LDOVCO_TRIM_OUTPUT_S                    6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV6_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV6_CC13
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_MIN_ALLOWED_RTRIM_W                     4
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_MIN_ALLOWED_RTRIM_M            0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_MIN_ALLOWED_RTRIM_S                    28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_RFC_MDM_DEMIQMC0_W                     16
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_RFC_MDM_DEMIQMC0_M             0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_RFC_MDM_DEMIQMC0_S                     12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_LDOVCO_TRIM_OUTPUT_W                    6
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_LDOVCO_TRIM_OUTPUT_M           0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_LDOVCO_TRIM_OUTPUT_S                    6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV6_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV10
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV10_MIN_ALLOWED_RTRIM_W                         4
#define FCFG1_CONFIG_SYNTH_DIV10_MIN_ALLOWED_RTRIM_M                0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV10_MIN_ALLOWED_RTRIM_S                        28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV10_RFC_MDM_DEMIQMC0_W                         16
#define FCFG1_CONFIG_SYNTH_DIV10_RFC_MDM_DEMIQMC0_M                 0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV10_RFC_MDM_DEMIQMC0_S                         12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV10_LDOVCO_TRIM_OUTPUT_W                        6
#define FCFG1_CONFIG_SYNTH_DIV10_LDOVCO_TRIM_OUTPUT_M               0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV10_LDOVCO_TRIM_OUTPUT_S                        6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV10_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N     \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV10_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV10_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M   \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV10_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S   \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV12_CC26
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_MIN_ALLOWED_RTRIM_W                    4
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_MIN_ALLOWED_RTRIM_M           0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_MIN_ALLOWED_RTRIM_S                   28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_RFC_MDM_DEMIQMC0_W                    16
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_RFC_MDM_DEMIQMC0_M            0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_RFC_MDM_DEMIQMC0_S                    12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_LDOVCO_TRIM_OUTPUT_W                   6
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_LDOVCO_TRIM_OUTPUT_M          0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_LDOVCO_TRIM_OUTPUT_S                   6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV12_CC26_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV12_CC13
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_MIN_ALLOWED_RTRIM_W                    4
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_MIN_ALLOWED_RTRIM_M           0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_MIN_ALLOWED_RTRIM_S                   28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_RFC_MDM_DEMIQMC0_W                    16
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_RFC_MDM_DEMIQMC0_M            0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_RFC_MDM_DEMIQMC0_S                    12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_LDOVCO_TRIM_OUTPUT_W                   6
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_LDOVCO_TRIM_OUTPUT_M          0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_LDOVCO_TRIM_OUTPUT_S                   6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV12_CC13_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV15
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV15_MIN_ALLOWED_RTRIM_W                         4
#define FCFG1_CONFIG_SYNTH_DIV15_MIN_ALLOWED_RTRIM_M                0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV15_MIN_ALLOWED_RTRIM_S                        28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV15_RFC_MDM_DEMIQMC0_W                         16
#define FCFG1_CONFIG_SYNTH_DIV15_RFC_MDM_DEMIQMC0_M                 0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV15_RFC_MDM_DEMIQMC0_S                         12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV15_LDOVCO_TRIM_OUTPUT_W                        6
#define FCFG1_CONFIG_SYNTH_DIV15_LDOVCO_TRIM_OUTPUT_M               0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV15_LDOVCO_TRIM_OUTPUT_S                        6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV15_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N     \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV15_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV15_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M   \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV15_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S   \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_SYNTH_DIV30
//
//*****************************************************************************
// Field: [31:28] MIN_ALLOWED_RTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV30_MIN_ALLOWED_RTRIM_W                         4
#define FCFG1_CONFIG_SYNTH_DIV30_MIN_ALLOWED_RTRIM_M                0xF0000000
#define FCFG1_CONFIG_SYNTH_DIV30_MIN_ALLOWED_RTRIM_S                        28

// Field: [27:12] RFC_MDM_DEMIQMC0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV30_RFC_MDM_DEMIQMC0_W                         16
#define FCFG1_CONFIG_SYNTH_DIV30_RFC_MDM_DEMIQMC0_M                 0x0FFFF000
#define FCFG1_CONFIG_SYNTH_DIV30_RFC_MDM_DEMIQMC0_S                         12

// Field:  [11:6] LDOVCO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV30_LDOVCO_TRIM_OUTPUT_W                        6
#define FCFG1_CONFIG_SYNTH_DIV30_LDOVCO_TRIM_OUTPUT_M               0x00000FC0
#define FCFG1_CONFIG_SYNTH_DIV30_LDOVCO_TRIM_OUTPUT_S                        6

// Field:     [5] RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_SYNTH_DIV30_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N     \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV30_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_BITN \
                                                                             5
#define FCFG1_CONFIG_SYNTH_DIV30_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_M   \
                                                                    0x00000020
#define FCFG1_CONFIG_SYNTH_DIV30_RFC_MDM_DEMIQMC0_TRIMCOMPLETE_N_S   \
                                                                             5

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_NUMBER
//
//*****************************************************************************
// Field:  [31:0] LOT_NUMBER
//
// Number of the manufacturing lot that produced this unit.
#define FCFG1_FLASH_NUMBER_LOT_NUMBER_W                                     32
#define FCFG1_FLASH_NUMBER_LOT_NUMBER_M                             0xFFFFFFFF
#define FCFG1_FLASH_NUMBER_LOT_NUMBER_S                                      0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_COORDINATE
//
//*****************************************************************************
// Field: [31:16] XCOORDINATE
//
// X coordinate of this unit on the wafer.
#define FCFG1_FLASH_COORDINATE_XCOORDINATE_W                                16
#define FCFG1_FLASH_COORDINATE_XCOORDINATE_M                        0xFFFF0000
#define FCFG1_FLASH_COORDINATE_XCOORDINATE_S                                16

// Field:  [15:0] YCOORDINATE
//
// Y coordinate of this unit on the wafer.
#define FCFG1_FLASH_COORDINATE_YCOORDINATE_W                                16
#define FCFG1_FLASH_COORDINATE_YCOORDINATE_M                        0x0000FFFF
#define FCFG1_FLASH_COORDINATE_YCOORDINATE_S                                 0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_E_P
//
//*****************************************************************************
// Field: [31:24] PSU
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_E_P_PSU_W                                                8
#define FCFG1_FLASH_E_P_PSU_M                                       0xFF000000
#define FCFG1_FLASH_E_P_PSU_S                                               24

// Field: [23:16] ESU
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_E_P_ESU_W                                                8
#define FCFG1_FLASH_E_P_ESU_M                                       0x00FF0000
#define FCFG1_FLASH_E_P_ESU_S                                               16

// Field:  [15:8] PVSU
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_E_P_PVSU_W                                               8
#define FCFG1_FLASH_E_P_PVSU_M                                      0x0000FF00
#define FCFG1_FLASH_E_P_PVSU_S                                               8

// Field:   [7:0] EVSU
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_E_P_EVSU_W                                               8
#define FCFG1_FLASH_E_P_EVSU_M                                      0x000000FF
#define FCFG1_FLASH_E_P_EVSU_S                                               0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_C_E_P_R
//
//*****************************************************************************
// Field: [31:24] RVSU
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_C_E_P_R_RVSU_W                                           8
#define FCFG1_FLASH_C_E_P_R_RVSU_M                                  0xFF000000
#define FCFG1_FLASH_C_E_P_R_RVSU_S                                          24

// Field: [23:16] PV_ACCESS
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_C_E_P_R_PV_ACCESS_W                                      8
#define FCFG1_FLASH_C_E_P_R_PV_ACCESS_M                             0x00FF0000
#define FCFG1_FLASH_C_E_P_R_PV_ACCESS_S                                     16

// Field: [15:12] A_EXEZ_SETUP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_C_E_P_R_A_EXEZ_SETUP_W                                   4
#define FCFG1_FLASH_C_E_P_R_A_EXEZ_SETUP_M                          0x0000F000
#define FCFG1_FLASH_C_E_P_R_A_EXEZ_SETUP_S                                  12

// Field:  [11:0] CVSU
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_C_E_P_R_CVSU_W                                          12
#define FCFG1_FLASH_C_E_P_R_CVSU_M                                  0x00000FFF
#define FCFG1_FLASH_C_E_P_R_CVSU_S                                           0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_P_R_PV
//
//*****************************************************************************
// Field: [31:24] PH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_P_R_PV_PH_W                                              8
#define FCFG1_FLASH_P_R_PV_PH_M                                     0xFF000000
#define FCFG1_FLASH_P_R_PV_PH_S                                             24

// Field: [23:16] RH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_P_R_PV_RH_W                                              8
#define FCFG1_FLASH_P_R_PV_RH_M                                     0x00FF0000
#define FCFG1_FLASH_P_R_PV_RH_S                                             16

// Field:  [15:8] PVH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_P_R_PV_PVH_W                                             8
#define FCFG1_FLASH_P_R_PV_PVH_M                                    0x0000FF00
#define FCFG1_FLASH_P_R_PV_PVH_S                                             8

// Field:   [7:0] PVH2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_P_R_PV_PVH2_W                                            8
#define FCFG1_FLASH_P_R_PV_PVH2_M                                   0x000000FF
#define FCFG1_FLASH_P_R_PV_PVH2_S                                            0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_EH_SEQ
//
//*****************************************************************************
// Field: [31:24] EH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_EH_SEQ_EH_W                                              8
#define FCFG1_FLASH_EH_SEQ_EH_M                                     0xFF000000
#define FCFG1_FLASH_EH_SEQ_EH_S                                             24

// Field: [23:16] SEQ
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_EH_SEQ_SEQ_W                                             8
#define FCFG1_FLASH_EH_SEQ_SEQ_M                                    0x00FF0000
#define FCFG1_FLASH_EH_SEQ_SEQ_S                                            16

// Field: [15:12] VSTAT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_EH_SEQ_VSTAT_W                                           4
#define FCFG1_FLASH_EH_SEQ_VSTAT_M                                  0x0000F000
#define FCFG1_FLASH_EH_SEQ_VSTAT_S                                          12

// Field:  [11:0] SM_FREQUENCY
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_EH_SEQ_SM_FREQUENCY_W                                   12
#define FCFG1_FLASH_EH_SEQ_SM_FREQUENCY_M                           0x00000FFF
#define FCFG1_FLASH_EH_SEQ_SM_FREQUENCY_S                                    0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_VHV_E
//
//*****************************************************************************
// Field: [31:16] VHV_E_START
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_E_VHV_E_START_W                                     16
#define FCFG1_FLASH_VHV_E_VHV_E_START_M                             0xFFFF0000
#define FCFG1_FLASH_VHV_E_VHV_E_START_S                                     16

// Field:  [15:0] VHV_E_STEP_HIGHT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_E_VHV_E_STEP_HIGHT_W                                16
#define FCFG1_FLASH_VHV_E_VHV_E_STEP_HIGHT_M                        0x0000FFFF
#define FCFG1_FLASH_VHV_E_VHV_E_STEP_HIGHT_S                                 0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_PP
//
//*****************************************************************************
// Field: [31:24] PUMP_SU
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_PP_PUMP_SU_W                                             8
#define FCFG1_FLASH_PP_PUMP_SU_M                                    0xFF000000
#define FCFG1_FLASH_PP_PUMP_SU_S                                            24

// Field: [23:16] TRIM3P4
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_PP_TRIM3P4_W                                             8
#define FCFG1_FLASH_PP_TRIM3P4_M                                    0x00FF0000
#define FCFG1_FLASH_PP_TRIM3P4_S                                            16

// Field:  [15:0] MAX_PP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_PP_MAX_PP_W                                             16
#define FCFG1_FLASH_PP_MAX_PP_M                                     0x0000FFFF
#define FCFG1_FLASH_PP_MAX_PP_S                                              0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_PROG_EP
//
//*****************************************************************************
// Field: [31:16] MAX_EP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_PROG_EP_MAX_EP_W                                        16
#define FCFG1_FLASH_PROG_EP_MAX_EP_M                                0xFFFF0000
#define FCFG1_FLASH_PROG_EP_MAX_EP_S                                        16

// Field:  [15:0] PROGRAM_PW
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_PROG_EP_PROGRAM_PW_W                                    16
#define FCFG1_FLASH_PROG_EP_PROGRAM_PW_M                            0x0000FFFF
#define FCFG1_FLASH_PROG_EP_PROGRAM_PW_S                                     0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_ERA_PW
//
//*****************************************************************************
// Field:  [31:0] ERASE_PW
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_ERA_PW_ERASE_PW_W                                       32
#define FCFG1_FLASH_ERA_PW_ERASE_PW_M                               0xFFFFFFFF
#define FCFG1_FLASH_ERA_PW_ERASE_PW_S                                        0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_VHV
//
//*****************************************************************************
// Field: [27:24] TRIM13_P
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_TRIM13_P_W                                           4
#define FCFG1_FLASH_VHV_TRIM13_P_M                                  0x0F000000
#define FCFG1_FLASH_VHV_TRIM13_P_S                                          24

// Field: [19:16] VHV_P
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_VHV_P_W                                              4
#define FCFG1_FLASH_VHV_VHV_P_M                                     0x000F0000
#define FCFG1_FLASH_VHV_VHV_P_S                                             16

// Field:  [11:8] TRIM13_E
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_TRIM13_E_W                                           4
#define FCFG1_FLASH_VHV_TRIM13_E_M                                  0x00000F00
#define FCFG1_FLASH_VHV_TRIM13_E_S                                           8

// Field:   [3:0] VHV_E
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_VHV_E_W                                              4
#define FCFG1_FLASH_VHV_VHV_E_M                                     0x0000000F
#define FCFG1_FLASH_VHV_VHV_E_S                                              0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_VHV_PV
//
//*****************************************************************************
// Field: [27:24] TRIM13_PV
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_PV_TRIM13_PV_W                                       4
#define FCFG1_FLASH_VHV_PV_TRIM13_PV_M                              0x0F000000
#define FCFG1_FLASH_VHV_PV_TRIM13_PV_S                                      24

// Field: [19:16] VHV_PV
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_PV_VHV_PV_W                                          4
#define FCFG1_FLASH_VHV_PV_VHV_PV_M                                 0x000F0000
#define FCFG1_FLASH_VHV_PV_VHV_PV_S                                         16

// Field:  [15:8] VCG2P5
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_PV_VCG2P5_W                                          8
#define FCFG1_FLASH_VHV_PV_VCG2P5_M                                 0x0000FF00
#define FCFG1_FLASH_VHV_PV_VCG2P5_S                                          8

// Field:   [7:0] VINH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_VHV_PV_VINH_W                                            8
#define FCFG1_FLASH_VHV_PV_VINH_M                                   0x000000FF
#define FCFG1_FLASH_VHV_PV_VINH_S                                            0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_V
//
//*****************************************************************************
// Field: [31:24] VSL_P
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_V_VSL_P_W                                                8
#define FCFG1_FLASH_V_VSL_P_M                                       0xFF000000
#define FCFG1_FLASH_V_VSL_P_S                                               24

// Field: [23:16] VWL_P
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_V_VWL_P_W                                                8
#define FCFG1_FLASH_V_VWL_P_M                                       0x00FF0000
#define FCFG1_FLASH_V_VWL_P_S                                               16

// Field:  [15:8] V_READ
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_V_V_READ_W                                               8
#define FCFG1_FLASH_V_V_READ_M                                      0x0000FF00
#define FCFG1_FLASH_V_V_READ_S                                               8

// Field:   [7:0] TRIM0P8
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_V_TRIM0P8_W                                              8
#define FCFG1_FLASH_V_TRIM0P8_M                                     0x000000FF
#define FCFG1_FLASH_V_TRIM0P8_S                                              0

//*****************************************************************************
//
// Register: FCFG1_O_USER_ID
//
//*****************************************************************************
// Field: [31:28] PG_REV
//
// Field used to distinguish revisions of the device
#define FCFG1_USER_ID_PG_REV_W                                               4
#define FCFG1_USER_ID_PG_REV_M                                      0xF0000000
#define FCFG1_USER_ID_PG_REV_S                                              28

// Field: [27:26] VER
//
// Version number.
//
// 0x0: Bits [25:12] of this register has the stated meaning.
//
// Any other setting indicate a different encoding of these bits.
#define FCFG1_USER_ID_VER_W                                                  2
#define FCFG1_USER_ID_VER_M                                         0x0C000000
#define FCFG1_USER_ID_VER_S                                                 26

// Field:    [25] PA
//
// 0: Does not support 20dBm PA
// 1: Supports 20dBM PA
#define FCFG1_USER_ID_PA                                            0x02000000
#define FCFG1_USER_ID_PA_BITN                                               25
#define FCFG1_USER_ID_PA_M                                          0x02000000
#define FCFG1_USER_ID_PA_S                                                  25

// Field:    [23] CC13
//
// 0: CC26xx device type
// 1: CC13xx device type
#define FCFG1_USER_ID_CC13                                          0x00800000
#define FCFG1_USER_ID_CC13_BITN                                             23
#define FCFG1_USER_ID_CC13_M                                        0x00800000
#define FCFG1_USER_ID_CC13_S                                                23

// Field: [22:19] SEQUENCE
//
// Sequence.
//
// Used to differentiate between marketing/orderable product where other fields
// of this register are the same (temp range, flash size, voltage range etc)
#define FCFG1_USER_ID_SEQUENCE_W                                             4
#define FCFG1_USER_ID_SEQUENCE_M                                    0x00780000
#define FCFG1_USER_ID_SEQUENCE_S                                            19

// Field: [18:16] PKG
//
// Package type.
//
// 0x0: 4x4mm QFN (RHB) package
// 0x1: 5x5mm QFN (RSM) package
// 0x2: 7x7mm QFN (RGZ) package
// 0x3: Wafer sale package (naked die)
// 0x4: WCSP (YFV)
// 0x5: 7x7mm QFN package with Wettable Flanks
//
// Other values are reserved for future use.
// Packages available for a specific device are shown in the device datasheet.
#define FCFG1_USER_ID_PKG_W                                                  3
#define FCFG1_USER_ID_PKG_M                                         0x00070000
#define FCFG1_USER_ID_PKG_S                                                 16

// Field: [15:12] PROTOCOL
//
// Protocols supported.
//
// 0x1: BLE
// 0x2: RF4CE
// 0x4: Zigbee/6lowpan
// 0x8: Proprietary
//
// More than one protocol can be supported on same device - values above are
// then combined.
#define FCFG1_USER_ID_PROTOCOL_W                                             4
#define FCFG1_USER_ID_PROTOCOL_M                                    0x0000F000
#define FCFG1_USER_ID_PROTOCOL_S                                            12

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_OTP_DATA3
//
//*****************************************************************************
// Field: [31:23] EC_STEP_SIZE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA3_EC_STEP_SIZE_W                                 9
#define FCFG1_FLASH_OTP_DATA3_EC_STEP_SIZE_M                        0xFF800000
#define FCFG1_FLASH_OTP_DATA3_EC_STEP_SIZE_S                                23

// Field:    [22] DO_PRECOND
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA3_DO_PRECOND                            0x00400000
#define FCFG1_FLASH_OTP_DATA3_DO_PRECOND_BITN                               22
#define FCFG1_FLASH_OTP_DATA3_DO_PRECOND_M                          0x00400000
#define FCFG1_FLASH_OTP_DATA3_DO_PRECOND_S                                  22

// Field: [21:18] MAX_EC_LEVEL
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA3_MAX_EC_LEVEL_W                                 4
#define FCFG1_FLASH_OTP_DATA3_MAX_EC_LEVEL_M                        0x003C0000
#define FCFG1_FLASH_OTP_DATA3_MAX_EC_LEVEL_S                                18

// Field: [17:16] TRIM_1P7
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA3_TRIM_1P7_W                                     2
#define FCFG1_FLASH_OTP_DATA3_TRIM_1P7_M                            0x00030000
#define FCFG1_FLASH_OTP_DATA3_TRIM_1P7_S                                    16

// Field:  [15:8] FLASH_SIZE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA3_FLASH_SIZE_W                                   8
#define FCFG1_FLASH_OTP_DATA3_FLASH_SIZE_M                          0x0000FF00
#define FCFG1_FLASH_OTP_DATA3_FLASH_SIZE_S                                   8

// Field:   [7:0] WAIT_SYSCODE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA3_WAIT_SYSCODE_W                                 8
#define FCFG1_FLASH_OTP_DATA3_WAIT_SYSCODE_M                        0x000000FF
#define FCFG1_FLASH_OTP_DATA3_WAIT_SYSCODE_S                                 0

//*****************************************************************************
//
// Register: FCFG1_O_ANA2_TRIM
//
//*****************************************************************************
// Field:    [31] RCOSCHFCTRIMFRACT_EN
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_RCOSCHFCTRIMFRACT_EN                        0x80000000
#define FCFG1_ANA2_TRIM_RCOSCHFCTRIMFRACT_EN_BITN                           31
#define FCFG1_ANA2_TRIM_RCOSCHFCTRIMFRACT_EN_M                      0x80000000
#define FCFG1_ANA2_TRIM_RCOSCHFCTRIMFRACT_EN_S                              31

// Field: [30:26] RCOSCHFCTRIMFRACT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_RCOSCHFCTRIMFRACT_W                                  5
#define FCFG1_ANA2_TRIM_RCOSCHFCTRIMFRACT_M                         0x7C000000
#define FCFG1_ANA2_TRIM_RCOSCHFCTRIMFRACT_S                                 26

// Field: [24:23] SET_RCOSC_HF_FINE_RESISTOR
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_SET_RCOSC_HF_FINE_RESISTOR_W                         2
#define FCFG1_ANA2_TRIM_SET_RCOSC_HF_FINE_RESISTOR_M                0x01800000
#define FCFG1_ANA2_TRIM_SET_RCOSC_HF_FINE_RESISTOR_S                        23

// Field:    [22] ATESTLF_UDIGLDO_IBIAS_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_ATESTLF_UDIGLDO_IBIAS_TRIM                  0x00400000
#define FCFG1_ANA2_TRIM_ATESTLF_UDIGLDO_IBIAS_TRIM_BITN                     22
#define FCFG1_ANA2_TRIM_ATESTLF_UDIGLDO_IBIAS_TRIM_M                0x00400000
#define FCFG1_ANA2_TRIM_ATESTLF_UDIGLDO_IBIAS_TRIM_S                        22

// Field: [21:15] NANOAMP_RES_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_NANOAMP_RES_TRIM_W                                   7
#define FCFG1_ANA2_TRIM_NANOAMP_RES_TRIM_M                          0x003F8000
#define FCFG1_ANA2_TRIM_NANOAMP_RES_TRIM_S                                  15

// Field:    [11] DITHER_EN
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_DITHER_EN                                   0x00000800
#define FCFG1_ANA2_TRIM_DITHER_EN_BITN                                      11
#define FCFG1_ANA2_TRIM_DITHER_EN_M                                 0x00000800
#define FCFG1_ANA2_TRIM_DITHER_EN_S                                         11

// Field:  [10:8] DCDC_IPEAK
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_DCDC_IPEAK_W                                         3
#define FCFG1_ANA2_TRIM_DCDC_IPEAK_M                                0x00000700
#define FCFG1_ANA2_TRIM_DCDC_IPEAK_S                                         8

// Field:   [7:6] DEAD_TIME_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_DEAD_TIME_TRIM_W                                     2
#define FCFG1_ANA2_TRIM_DEAD_TIME_TRIM_M                            0x000000C0
#define FCFG1_ANA2_TRIM_DEAD_TIME_TRIM_S                                     6

// Field:   [5:3] DCDC_LOW_EN_SEL
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_DCDC_LOW_EN_SEL_W                                    3
#define FCFG1_ANA2_TRIM_DCDC_LOW_EN_SEL_M                           0x00000038
#define FCFG1_ANA2_TRIM_DCDC_LOW_EN_SEL_S                                    3

// Field:   [2:0] DCDC_HIGH_EN_SEL
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANA2_TRIM_DCDC_HIGH_EN_SEL_W                                   3
#define FCFG1_ANA2_TRIM_DCDC_HIGH_EN_SEL_M                          0x00000007
#define FCFG1_ANA2_TRIM_DCDC_HIGH_EN_SEL_S                                   0

//*****************************************************************************
//
// Register: FCFG1_O_LDO_TRIM
//
//*****************************************************************************
// Field: [28:24] VDDR_TRIM_SLEEP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_LDO_TRIM_VDDR_TRIM_SLEEP_W                                     5
#define FCFG1_LDO_TRIM_VDDR_TRIM_SLEEP_M                            0x1F000000
#define FCFG1_LDO_TRIM_VDDR_TRIM_SLEEP_S                                    24

// Field: [18:16] GLDO_CURSRC
//
// Internal. Only to be used through TI provided API.
#define FCFG1_LDO_TRIM_GLDO_CURSRC_W                                         3
#define FCFG1_LDO_TRIM_GLDO_CURSRC_M                                0x00070000
#define FCFG1_LDO_TRIM_GLDO_CURSRC_S                                        16

// Field: [12:11] ITRIM_DIGLDO_LOAD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_LDO_TRIM_ITRIM_DIGLDO_LOAD_W                                   2
#define FCFG1_LDO_TRIM_ITRIM_DIGLDO_LOAD_M                          0x00001800
#define FCFG1_LDO_TRIM_ITRIM_DIGLDO_LOAD_S                                  11

// Field:  [10:8] ITRIM_UDIGLDO
//
// Internal. Only to be used through TI provided API.
#define FCFG1_LDO_TRIM_ITRIM_UDIGLDO_W                                       3
#define FCFG1_LDO_TRIM_ITRIM_UDIGLDO_M                              0x00000700
#define FCFG1_LDO_TRIM_ITRIM_UDIGLDO_S                                       8

// Field:   [2:0] VTRIM_DELTA
//
// Internal. Only to be used through TI provided API.
#define FCFG1_LDO_TRIM_VTRIM_DELTA_W                                         3
#define FCFG1_LDO_TRIM_VTRIM_DELTA_M                                0x00000007
#define FCFG1_LDO_TRIM_VTRIM_DELTA_S                                         0

//*****************************************************************************
//
// Register: FCFG1_O_MAC_BLE_0
//
//*****************************************************************************
// Field:  [31:0] ADDR_0_31
//
// The first 32-bits of the 64-bit MAC BLE address
#define FCFG1_MAC_BLE_0_ADDR_0_31_W                                         32
#define FCFG1_MAC_BLE_0_ADDR_0_31_M                                 0xFFFFFFFF
#define FCFG1_MAC_BLE_0_ADDR_0_31_S                                          0

//*****************************************************************************
//
// Register: FCFG1_O_MAC_BLE_1
//
//*****************************************************************************
// Field:  [31:0] ADDR_32_63
//
// The last 32-bits of the 64-bit MAC BLE address
#define FCFG1_MAC_BLE_1_ADDR_32_63_W                                        32
#define FCFG1_MAC_BLE_1_ADDR_32_63_M                                0xFFFFFFFF
#define FCFG1_MAC_BLE_1_ADDR_32_63_S                                         0

//*****************************************************************************
//
// Register: FCFG1_O_MAC_15_4_0
//
//*****************************************************************************
// Field:  [31:0] ADDR_0_31
//
// The first 32-bits of the 64-bit MAC 15.4 address
#define FCFG1_MAC_15_4_0_ADDR_0_31_W                                        32
#define FCFG1_MAC_15_4_0_ADDR_0_31_M                                0xFFFFFFFF
#define FCFG1_MAC_15_4_0_ADDR_0_31_S                                         0

//*****************************************************************************
//
// Register: FCFG1_O_MAC_15_4_1
//
//*****************************************************************************
// Field:  [31:0] ADDR_32_63
//
// The last 32-bits of the 64-bit MAC 15.4 address
#define FCFG1_MAC_15_4_1_ADDR_32_63_W                                       32
#define FCFG1_MAC_15_4_1_ADDR_32_63_M                               0xFFFFFFFF
#define FCFG1_MAC_15_4_1_ADDR_32_63_S                                        0

//*****************************************************************************
//
// Register: FCFG1_O_FLASH_OTP_DATA4
//
//*****************************************************************************
// Field:    [31] STANDBY_MODE_SEL_INT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_WRT              0x80000000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_WRT_BITN                 31
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_WRT_M            0x80000000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_WRT_S                    31

// Field: [30:29] STANDBY_PW_SEL_INT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_INT_WRT_W                       2
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_INT_WRT_M              0x60000000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_INT_WRT_S                      29

// Field:    [28] DIS_STANDBY_INT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_WRT                   0x10000000
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_WRT_BITN                      28
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_WRT_M                 0x10000000
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_WRT_S                         28

// Field:    [27] DIS_IDLE_INT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_WRT                      0x08000000
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_WRT_BITN                         27
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_WRT_M                    0x08000000
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_WRT_S                            27

// Field: [26:24] VIN_AT_X_INT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_INT_WRT_W                             3
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_INT_WRT_M                    0x07000000
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_INT_WRT_S                            24

// Field:    [23] STANDBY_MODE_SEL_EXT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_WRT              0x00800000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_WRT_BITN                 23
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_WRT_M            0x00800000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_WRT_S                    23

// Field: [22:21] STANDBY_PW_SEL_EXT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_EXT_WRT_W                       2
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_EXT_WRT_M              0x00600000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_EXT_WRT_S                      21

// Field:    [20] DIS_STANDBY_EXT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_WRT                   0x00100000
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_WRT_BITN                      20
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_WRT_M                 0x00100000
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_WRT_S                         20

// Field:    [19] DIS_IDLE_EXT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_WRT                      0x00080000
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_WRT_BITN                         19
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_WRT_M                    0x00080000
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_WRT_S                            19

// Field: [18:16] VIN_AT_X_EXT_WRT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_EXT_WRT_W                             3
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_EXT_WRT_M                    0x00070000
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_EXT_WRT_S                            16

// Field:    [15] STANDBY_MODE_SEL_INT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_RD               0x00008000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_RD_BITN                  15
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_RD_M             0x00008000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_INT_RD_S                     15

// Field: [14:13] STANDBY_PW_SEL_INT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_INT_RD_W                        2
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_INT_RD_M               0x00006000
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_INT_RD_S                       13

// Field:    [12] DIS_STANDBY_INT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_RD                    0x00001000
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_RD_BITN                       12
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_RD_M                  0x00001000
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_INT_RD_S                          12

// Field:    [11] DIS_IDLE_INT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_RD                       0x00000800
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_RD_BITN                          11
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_RD_M                     0x00000800
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_INT_RD_S                             11

// Field:  [10:8] VIN_AT_X_INT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_INT_RD_W                              3
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_INT_RD_M                     0x00000700
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_INT_RD_S                              8

// Field:     [7] STANDBY_MODE_SEL_EXT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_RD               0x00000080
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_RD_BITN                   7
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_RD_M             0x00000080
#define FCFG1_FLASH_OTP_DATA4_STANDBY_MODE_SEL_EXT_RD_S                      7

// Field:   [6:5] STANDBY_PW_SEL_EXT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_EXT_RD_W                        2
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_EXT_RD_M               0x00000060
#define FCFG1_FLASH_OTP_DATA4_STANDBY_PW_SEL_EXT_RD_S                        5

// Field:     [4] DIS_STANDBY_EXT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_RD                    0x00000010
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_RD_BITN                        4
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_RD_M                  0x00000010
#define FCFG1_FLASH_OTP_DATA4_DIS_STANDBY_EXT_RD_S                           4

// Field:     [3] DIS_IDLE_EXT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_RD                       0x00000008
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_RD_BITN                           3
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_RD_M                     0x00000008
#define FCFG1_FLASH_OTP_DATA4_DIS_IDLE_EXT_RD_S                              3

// Field:   [2:0] VIN_AT_X_EXT_RD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_EXT_RD_W                              3
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_EXT_RD_M                     0x00000007
#define FCFG1_FLASH_OTP_DATA4_VIN_AT_X_EXT_RD_S                              0

//*****************************************************************************
//
// Register: FCFG1_O_MISC_TRIM
//
//*****************************************************************************
// Field: [16:12] TRIM_RECHARGE_COMP_OFFSET
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_TRIM_TRIM_RECHARGE_COMP_OFFSET_W                          5
#define FCFG1_MISC_TRIM_TRIM_RECHARGE_COMP_OFFSET_M                 0x0001F000
#define FCFG1_MISC_TRIM_TRIM_RECHARGE_COMP_OFFSET_S                         12

// Field:  [11:8] TRIM_RECHARGE_COMP_REFLEVEL
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_TRIM_TRIM_RECHARGE_COMP_REFLEVEL_W                        4
#define FCFG1_MISC_TRIM_TRIM_RECHARGE_COMP_REFLEVEL_M               0x00000F00
#define FCFG1_MISC_TRIM_TRIM_RECHARGE_COMP_REFLEVEL_S                        8

// Field:   [7:0] TEMPVSLOPE
//
// Signed byte value representing the TEMP slope with battery voltage, in
// degrees C / V, with four fractional bits.
#define FCFG1_MISC_TRIM_TEMPVSLOPE_W                                         8
#define FCFG1_MISC_TRIM_TEMPVSLOPE_M                                0x000000FF
#define FCFG1_MISC_TRIM_TEMPVSLOPE_S                                         0

//*****************************************************************************
//
// Register: FCFG1_O_RCOSC_HF_TEMPCOMP
//
//*****************************************************************************
// Field: [31:24] FINE_RESISTOR
//
// Internal. Only to be used through TI provided API.
#define FCFG1_RCOSC_HF_TEMPCOMP_FINE_RESISTOR_W                              8
#define FCFG1_RCOSC_HF_TEMPCOMP_FINE_RESISTOR_M                     0xFF000000
#define FCFG1_RCOSC_HF_TEMPCOMP_FINE_RESISTOR_S                             24

// Field: [23:16] CTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIM_W                                      8
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIM_M                             0x00FF0000
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIM_S                                     16

// Field:  [15:8] CTRIMFRACT_QUAD
//
// Internal. Only to be used through TI provided API.
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIMFRACT_QUAD_W                            8
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIMFRACT_QUAD_M                   0x0000FF00
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIMFRACT_QUAD_S                            8

// Field:   [7:0] CTRIMFRACT_SLOPE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIMFRACT_SLOPE_W                           8
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIMFRACT_SLOPE_M                  0x000000FF
#define FCFG1_RCOSC_HF_TEMPCOMP_CTRIMFRACT_SLOPE_S                           0

//*****************************************************************************
//
// Register: FCFG1_O_ICEPICK_DEVICE_ID
//
//*****************************************************************************
// Field: [31:28] PG_REV
//
// Field used to distinguish revisions of the device.
#define FCFG1_ICEPICK_DEVICE_ID_PG_REV_W                                     4
#define FCFG1_ICEPICK_DEVICE_ID_PG_REV_M                            0xF0000000
#define FCFG1_ICEPICK_DEVICE_ID_PG_REV_S                                    28

// Field: [27:12] WAFER_ID
//
// Field used to identify silicon die.
#define FCFG1_ICEPICK_DEVICE_ID_WAFER_ID_W                                  16
#define FCFG1_ICEPICK_DEVICE_ID_WAFER_ID_M                          0x0FFFF000
#define FCFG1_ICEPICK_DEVICE_ID_WAFER_ID_S                                  12

// Field:  [11:0] MANUFACTURER_ID
//
// Manufacturer code.
//
// 0x02F: Texas Instruments
#define FCFG1_ICEPICK_DEVICE_ID_MANUFACTURER_ID_W                           12
#define FCFG1_ICEPICK_DEVICE_ID_MANUFACTURER_ID_M                   0x00000FFF
#define FCFG1_ICEPICK_DEVICE_ID_MANUFACTURER_ID_S                            0

//*****************************************************************************
//
// Register: FCFG1_O_FCFG1_REVISION
//
//*****************************************************************************
// Field:  [31:0] REV
//
// The revision number of the FCFG1 layout. This value will be read by
// application SW in order to determine which FCFG1 parameters that have valid
// values. This revision number must be incremented by 1 before any devices are
// to be produced if the FCFG1 layout has changed since the previous production
// of devices.
// Value migth change without warning.
#define FCFG1_FCFG1_REVISION_REV_W                                          32
#define FCFG1_FCFG1_REVISION_REV_M                                  0xFFFFFFFF
#define FCFG1_FCFG1_REVISION_REV_S                                           0

//*****************************************************************************
//
// Register: FCFG1_O_MISC_OTP_DATA
//
//*****************************************************************************
// Field: [31:28] RCOSC_HF_ITUNE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_RCOSC_HF_ITUNE_W                                 4
#define FCFG1_MISC_OTP_DATA_RCOSC_HF_ITUNE_M                        0xF0000000
#define FCFG1_MISC_OTP_DATA_RCOSC_HF_ITUNE_S                                28

// Field: [27:20] RCOSC_HF_CRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_RCOSC_HF_CRIM_W                                  8
#define FCFG1_MISC_OTP_DATA_RCOSC_HF_CRIM_M                         0x0FF00000
#define FCFG1_MISC_OTP_DATA_RCOSC_HF_CRIM_S                                 20

// Field: [19:15] PER_M
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_PER_M_W                                          5
#define FCFG1_MISC_OTP_DATA_PER_M_M                                 0x000F8000
#define FCFG1_MISC_OTP_DATA_PER_M_S                                         15

// Field: [14:12] PER_E
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_PER_E_W                                          3
#define FCFG1_MISC_OTP_DATA_PER_E_M                                 0x00007000
#define FCFG1_MISC_OTP_DATA_PER_E_S                                         12

//*****************************************************************************
//
// Register: FCFG1_O_IOCONF
//
//*****************************************************************************
// Field:   [6:0] GPIO_CNT
//
// Number of available DIOs.
#define FCFG1_IOCONF_GPIO_CNT_W                                              7
#define FCFG1_IOCONF_GPIO_CNT_M                                     0x0000007F
#define FCFG1_IOCONF_GPIO_CNT_S                                              0

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_IF_ADC
//
//*****************************************************************************
// Field: [31:28] FF2ADJ
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_IF_ADC_FF2ADJ_W                                         4
#define FCFG1_CONFIG_IF_ADC_FF2ADJ_M                                0xF0000000
#define FCFG1_CONFIG_IF_ADC_FF2ADJ_S                                        28

// Field: [27:24] FF3ADJ
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_IF_ADC_FF3ADJ_W                                         4
#define FCFG1_CONFIG_IF_ADC_FF3ADJ_M                                0x0F000000
#define FCFG1_CONFIG_IF_ADC_FF3ADJ_S                                        24

// Field: [23:20] INT3ADJ
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_IF_ADC_INT3ADJ_W                                        4
#define FCFG1_CONFIG_IF_ADC_INT3ADJ_M                               0x00F00000
#define FCFG1_CONFIG_IF_ADC_INT3ADJ_S                                       20

// Field: [19:16] FF1ADJ
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_IF_ADC_FF1ADJ_W                                         4
#define FCFG1_CONFIG_IF_ADC_FF1ADJ_M                                0x000F0000
#define FCFG1_CONFIG_IF_ADC_FF1ADJ_S                                        16

// Field: [15:14] AAFCAP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_IF_ADC_AAFCAP_W                                         2
#define FCFG1_CONFIG_IF_ADC_AAFCAP_M                                0x0000C000
#define FCFG1_CONFIG_IF_ADC_AAFCAP_S                                        14

// Field: [13:10] INT2ADJ
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_IF_ADC_INT2ADJ_W                                        4
#define FCFG1_CONFIG_IF_ADC_INT2ADJ_M                               0x00003C00
#define FCFG1_CONFIG_IF_ADC_INT2ADJ_S                                       10

// Field:   [9:5] IFDIGLDO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_IF_ADC_IFDIGLDO_TRIM_OUTPUT_W                           5
#define FCFG1_CONFIG_IF_ADC_IFDIGLDO_TRIM_OUTPUT_M                  0x000003E0
#define FCFG1_CONFIG_IF_ADC_IFDIGLDO_TRIM_OUTPUT_S                           5

// Field:   [4:0] IFANALDO_TRIM_OUTPUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_IF_ADC_IFANALDO_TRIM_OUTPUT_W                           5
#define FCFG1_CONFIG_IF_ADC_IFANALDO_TRIM_OUTPUT_M                  0x0000001F
#define FCFG1_CONFIG_IF_ADC_IFANALDO_TRIM_OUTPUT_S                           0

//*****************************************************************************
//
// Register: FCFG1_O_CONFIG_OSC_TOP
//
//*****************************************************************************
// Field: [29:26] XOSC_HF_ROW_Q12
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_OSC_TOP_XOSC_HF_ROW_Q12_W                               4
#define FCFG1_CONFIG_OSC_TOP_XOSC_HF_ROW_Q12_M                      0x3C000000
#define FCFG1_CONFIG_OSC_TOP_XOSC_HF_ROW_Q12_S                              26

// Field: [25:10] XOSC_HF_COLUMN_Q12
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_OSC_TOP_XOSC_HF_COLUMN_Q12_W                           16
#define FCFG1_CONFIG_OSC_TOP_XOSC_HF_COLUMN_Q12_M                   0x03FFFC00
#define FCFG1_CONFIG_OSC_TOP_XOSC_HF_COLUMN_Q12_S                           10

// Field:   [9:2] RCOSCLF_CTUNE_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_OSC_TOP_RCOSCLF_CTUNE_TRIM_W                            8
#define FCFG1_CONFIG_OSC_TOP_RCOSCLF_CTUNE_TRIM_M                   0x000003FC
#define FCFG1_CONFIG_OSC_TOP_RCOSCLF_CTUNE_TRIM_S                            2

// Field:   [1:0] RCOSCLF_RTUNE_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_CONFIG_OSC_TOP_RCOSCLF_RTUNE_TRIM_W                            2
#define FCFG1_CONFIG_OSC_TOP_RCOSCLF_RTUNE_TRIM_M                   0x00000003
#define FCFG1_CONFIG_OSC_TOP_RCOSCLF_RTUNE_TRIM_S                            0

//*****************************************************************************
//
// Register: FCFG1_O_SOC_ADC_ABS_GAIN
//
//*****************************************************************************
// Field:  [15:0] SOC_ADC_ABS_GAIN_TEMP1
//
// SOC_ADC gain in absolute reference mode at temperature 1 (30C). Calculated
// in production test..
#define FCFG1_SOC_ADC_ABS_GAIN_SOC_ADC_ABS_GAIN_TEMP1_W                     16
#define FCFG1_SOC_ADC_ABS_GAIN_SOC_ADC_ABS_GAIN_TEMP1_M             0x0000FFFF
#define FCFG1_SOC_ADC_ABS_GAIN_SOC_ADC_ABS_GAIN_TEMP1_S                      0

//*****************************************************************************
//
// Register: FCFG1_O_SOC_ADC_REL_GAIN
//
//*****************************************************************************
// Field:  [15:0] SOC_ADC_REL_GAIN_TEMP1
//
// SOC_ADC gain in relative reference mode at temperature 1 (30C). Calculated
// in production test..
#define FCFG1_SOC_ADC_REL_GAIN_SOC_ADC_REL_GAIN_TEMP1_W                     16
#define FCFG1_SOC_ADC_REL_GAIN_SOC_ADC_REL_GAIN_TEMP1_M             0x0000FFFF
#define FCFG1_SOC_ADC_REL_GAIN_SOC_ADC_REL_GAIN_TEMP1_S                      0

//*****************************************************************************
//
// Register: FCFG1_O_SOC_ADC_OFFSET_INT
//
//*****************************************************************************
// Field: [23:16] SOC_ADC_REL_OFFSET_TEMP1
//
// SOC_ADC offset in relative reference mode at temperature 1 (30C). Signed
// 8-bit number. Calculated in production test..
#define FCFG1_SOC_ADC_OFFSET_INT_SOC_ADC_REL_OFFSET_TEMP1_W                  8
#define FCFG1_SOC_ADC_OFFSET_INT_SOC_ADC_REL_OFFSET_TEMP1_M         0x00FF0000
#define FCFG1_SOC_ADC_OFFSET_INT_SOC_ADC_REL_OFFSET_TEMP1_S                 16

// Field:   [7:0] SOC_ADC_ABS_OFFSET_TEMP1
//
// SOC_ADC offset in absolute reference mode at temperature 1 (30C). Signed
// 8-bit number. Calculated in production test..
#define FCFG1_SOC_ADC_OFFSET_INT_SOC_ADC_ABS_OFFSET_TEMP1_W                  8
#define FCFG1_SOC_ADC_OFFSET_INT_SOC_ADC_ABS_OFFSET_TEMP1_M         0x000000FF
#define FCFG1_SOC_ADC_OFFSET_INT_SOC_ADC_ABS_OFFSET_TEMP1_S                  0

//*****************************************************************************
//
// Register: FCFG1_O_SOC_ADC_REF_TRIM_AND_OFFSET_EXT
//
//*****************************************************************************
// Field:   [5:0] SOC_ADC_REF_VOLTAGE_TRIM_TEMP1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SOC_ADC_REF_TRIM_AND_OFFSET_EXT_SOC_ADC_REF_VOLTAGE_TRIM_TEMP1_W \
                                                                             6
#define FCFG1_SOC_ADC_REF_TRIM_AND_OFFSET_EXT_SOC_ADC_REF_VOLTAGE_TRIM_TEMP1_M \
                                                                    0x0000003F
#define FCFG1_SOC_ADC_REF_TRIM_AND_OFFSET_EXT_SOC_ADC_REF_VOLTAGE_TRIM_TEMP1_S \
                                                                             0

//*****************************************************************************
//
// Register: FCFG1_O_AMPCOMP_TH1
//
//*****************************************************************************
// Field: [23:18] HPMRAMP3_LTH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_TH1_HPMRAMP3_LTH_W                                     6
#define FCFG1_AMPCOMP_TH1_HPMRAMP3_LTH_M                            0x00FC0000
#define FCFG1_AMPCOMP_TH1_HPMRAMP3_LTH_S                                    18

// Field: [15:10] HPMRAMP3_HTH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_TH1_HPMRAMP3_HTH_W                                     6
#define FCFG1_AMPCOMP_TH1_HPMRAMP3_HTH_M                            0x0000FC00
#define FCFG1_AMPCOMP_TH1_HPMRAMP3_HTH_S                                    10

// Field:   [9:6] IBIASCAP_LPTOHP_OL_CNT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_TH1_IBIASCAP_LPTOHP_OL_CNT_W                           4
#define FCFG1_AMPCOMP_TH1_IBIASCAP_LPTOHP_OL_CNT_M                  0x000003C0
#define FCFG1_AMPCOMP_TH1_IBIASCAP_LPTOHP_OL_CNT_S                           6

// Field:   [5:0] HPMRAMP1_TH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_TH1_HPMRAMP1_TH_W                                      6
#define FCFG1_AMPCOMP_TH1_HPMRAMP1_TH_M                             0x0000003F
#define FCFG1_AMPCOMP_TH1_HPMRAMP1_TH_S                                      0

//*****************************************************************************
//
// Register: FCFG1_O_AMPCOMP_TH2
//
//*****************************************************************************
// Field: [31:26] LPMUPDATE_LTH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_TH2_LPMUPDATE_LTH_W                                    6
#define FCFG1_AMPCOMP_TH2_LPMUPDATE_LTH_M                           0xFC000000
#define FCFG1_AMPCOMP_TH2_LPMUPDATE_LTH_S                                   26

// Field: [23:18] LPMUPDATE_HTM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_TH2_LPMUPDATE_HTM_W                                    6
#define FCFG1_AMPCOMP_TH2_LPMUPDATE_HTM_M                           0x00FC0000
#define FCFG1_AMPCOMP_TH2_LPMUPDATE_HTM_S                                   18

// Field: [15:10] ADC_COMP_AMPTH_LPM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_LPM_W                               6
#define FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_LPM_M                      0x0000FC00
#define FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_LPM_S                              10

// Field:   [7:2] ADC_COMP_AMPTH_HPM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_HPM_W                               6
#define FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_HPM_M                      0x000000FC
#define FCFG1_AMPCOMP_TH2_ADC_COMP_AMPTH_HPM_S                               2

//*****************************************************************************
//
// Register: FCFG1_O_AMPCOMP_CTRL1
//
//*****************************************************************************
// Field:    [30] AMPCOMP_REQ_MODE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_CTRL1_AMPCOMP_REQ_MODE                        0x40000000
#define FCFG1_AMPCOMP_CTRL1_AMPCOMP_REQ_MODE_BITN                           30
#define FCFG1_AMPCOMP_CTRL1_AMPCOMP_REQ_MODE_M                      0x40000000
#define FCFG1_AMPCOMP_CTRL1_AMPCOMP_REQ_MODE_S                              30

// Field: [23:20] IBIAS_OFFSET
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_CTRL1_IBIAS_OFFSET_W                                   4
#define FCFG1_AMPCOMP_CTRL1_IBIAS_OFFSET_M                          0x00F00000
#define FCFG1_AMPCOMP_CTRL1_IBIAS_OFFSET_S                                  20

// Field: [19:16] IBIAS_INIT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_CTRL1_IBIAS_INIT_W                                     4
#define FCFG1_AMPCOMP_CTRL1_IBIAS_INIT_M                            0x000F0000
#define FCFG1_AMPCOMP_CTRL1_IBIAS_INIT_S                                    16

// Field:  [15:8] LPM_IBIAS_WAIT_CNT_FINAL
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_CTRL1_LPM_IBIAS_WAIT_CNT_FINAL_W                       8
#define FCFG1_AMPCOMP_CTRL1_LPM_IBIAS_WAIT_CNT_FINAL_M              0x0000FF00
#define FCFG1_AMPCOMP_CTRL1_LPM_IBIAS_WAIT_CNT_FINAL_S                       8

// Field:   [7:4] CAP_STEP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_CTRL1_CAP_STEP_W                                       4
#define FCFG1_AMPCOMP_CTRL1_CAP_STEP_M                              0x000000F0
#define FCFG1_AMPCOMP_CTRL1_CAP_STEP_S                                       4

// Field:   [3:0] IBIASCAP_HPTOLP_OL_CNT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_AMPCOMP_CTRL1_IBIASCAP_HPTOLP_OL_CNT_W                         4
#define FCFG1_AMPCOMP_CTRL1_IBIASCAP_HPTOLP_OL_CNT_M                0x0000000F
#define FCFG1_AMPCOMP_CTRL1_IBIASCAP_HPTOLP_OL_CNT_S                         0

//*****************************************************************************
//
// Register: FCFG1_O_ANABYPASS_VALUE2
//
//*****************************************************************************
// Field:  [13:0] XOSC_HF_IBIASTHERM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_ANABYPASS_VALUE2_XOSC_HF_IBIASTHERM_W                         14
#define FCFG1_ANABYPASS_VALUE2_XOSC_HF_IBIASTHERM_M                 0x00003FFF
#define FCFG1_ANABYPASS_VALUE2_XOSC_HF_IBIASTHERM_S                          0

//*****************************************************************************
//
// Register: FCFG1_O_VOLT_TRIM
//
//*****************************************************************************
// Field: [28:24] VDDR_TRIM_HH
//
// Internal. Only to be used through TI provided API.
#define FCFG1_VOLT_TRIM_VDDR_TRIM_HH_W                                       5
#define FCFG1_VOLT_TRIM_VDDR_TRIM_HH_M                              0x1F000000
#define FCFG1_VOLT_TRIM_VDDR_TRIM_HH_S                                      24

// Field: [20:16] VDDR_TRIM_H
//
// Internal. Only to be used through TI provided API.
#define FCFG1_VOLT_TRIM_VDDR_TRIM_H_W                                        5
#define FCFG1_VOLT_TRIM_VDDR_TRIM_H_M                               0x001F0000
#define FCFG1_VOLT_TRIM_VDDR_TRIM_H_S                                       16

// Field:  [12:8] VDDR_TRIM_SLEEP_H
//
// Internal. Only to be used through TI provided API.
#define FCFG1_VOLT_TRIM_VDDR_TRIM_SLEEP_H_W                                  5
#define FCFG1_VOLT_TRIM_VDDR_TRIM_SLEEP_H_M                         0x00001F00
#define FCFG1_VOLT_TRIM_VDDR_TRIM_SLEEP_H_S                                  8

// Field:   [4:0] TRIMBOD_H
//
// Internal. Only to be used through TI provided API.
#define FCFG1_VOLT_TRIM_TRIMBOD_H_W                                          5
#define FCFG1_VOLT_TRIM_TRIMBOD_H_M                                 0x0000001F
#define FCFG1_VOLT_TRIM_TRIMBOD_H_S                                          0

//*****************************************************************************
//
// Register: FCFG1_O_OSC_CONF
//
//*****************************************************************************
// Field:    [29] ADC_SH_VBUF_EN
//
// Trim value for DDI_0_OSC:ADCDOUBLERNANOAMPCTL.ADC_SH_VBUF_EN.
#define FCFG1_OSC_CONF_ADC_SH_VBUF_EN                               0x20000000
#define FCFG1_OSC_CONF_ADC_SH_VBUF_EN_BITN                                  29
#define FCFG1_OSC_CONF_ADC_SH_VBUF_EN_M                             0x20000000
#define FCFG1_OSC_CONF_ADC_SH_VBUF_EN_S                                     29

// Field:    [28] ADC_SH_MODE_EN
//
// Trim value for DDI_0_OSC:ADCDOUBLERNANOAMPCTL.ADC_SH_MODE_EN.
#define FCFG1_OSC_CONF_ADC_SH_MODE_EN                               0x10000000
#define FCFG1_OSC_CONF_ADC_SH_MODE_EN_BITN                                  28
#define FCFG1_OSC_CONF_ADC_SH_MODE_EN_M                             0x10000000
#define FCFG1_OSC_CONF_ADC_SH_MODE_EN_S                                     28

// Field:    [27] ATESTLF_RCOSCLF_IBIAS_TRIM
//
// Trim value for DDI_0_OSC:ATESTCTL.ATESTLF_RCOSCLF_IBIAS_TRIM.
#define FCFG1_OSC_CONF_ATESTLF_RCOSCLF_IBIAS_TRIM                   0x08000000
#define FCFG1_OSC_CONF_ATESTLF_RCOSCLF_IBIAS_TRIM_BITN                      27
#define FCFG1_OSC_CONF_ATESTLF_RCOSCLF_IBIAS_TRIM_M                 0x08000000
#define FCFG1_OSC_CONF_ATESTLF_RCOSCLF_IBIAS_TRIM_S                         27

// Field: [26:25] XOSCLF_REGULATOR_TRIM
//
// Trim value for DDI_0_OSC:LFOSCCTL.XOSCLF_REGULATOR_TRIM.
#define FCFG1_OSC_CONF_XOSCLF_REGULATOR_TRIM_W                               2
#define FCFG1_OSC_CONF_XOSCLF_REGULATOR_TRIM_M                      0x06000000
#define FCFG1_OSC_CONF_XOSCLF_REGULATOR_TRIM_S                              25

// Field: [24:21] XOSCLF_CMIRRWR_RATIO
//
// Trim value for DDI_0_OSC:LFOSCCTL.XOSCLF_CMIRRWR_RATIO.
#define FCFG1_OSC_CONF_XOSCLF_CMIRRWR_RATIO_W                                4
#define FCFG1_OSC_CONF_XOSCLF_CMIRRWR_RATIO_M                       0x01E00000
#define FCFG1_OSC_CONF_XOSCLF_CMIRRWR_RATIO_S                               21

// Field: [20:19] XOSC_HF_FAST_START
//
// Trim value for DDI_0_OSC:CTL1.XOSC_HF_FAST_START.
#define FCFG1_OSC_CONF_XOSC_HF_FAST_START_W                                  2
#define FCFG1_OSC_CONF_XOSC_HF_FAST_START_M                         0x00180000
#define FCFG1_OSC_CONF_XOSC_HF_FAST_START_S                                 19

// Field:    [18] XOSC_OPTION
//
// 0: XOSC_HF unavailable (may not be bonded out)
// 1: XOSC_HF available (default)
#define FCFG1_OSC_CONF_XOSC_OPTION                                  0x00040000
#define FCFG1_OSC_CONF_XOSC_OPTION_BITN                                     18
#define FCFG1_OSC_CONF_XOSC_OPTION_M                                0x00040000
#define FCFG1_OSC_CONF_XOSC_OPTION_S                                        18

// Field:    [17] HPOSC_OPTION
//
// Internal. Only to be used through TI provided API.
#define FCFG1_OSC_CONF_HPOSC_OPTION                                 0x00020000
#define FCFG1_OSC_CONF_HPOSC_OPTION_BITN                                    17
#define FCFG1_OSC_CONF_HPOSC_OPTION_M                               0x00020000
#define FCFG1_OSC_CONF_HPOSC_OPTION_S                                       17

// Field:    [16] HPOSC_BIAS_HOLD_MODE_EN
//
// Internal. Only to be used through TI provided API.
#define FCFG1_OSC_CONF_HPOSC_BIAS_HOLD_MODE_EN                      0x00010000
#define FCFG1_OSC_CONF_HPOSC_BIAS_HOLD_MODE_EN_BITN                         16
#define FCFG1_OSC_CONF_HPOSC_BIAS_HOLD_MODE_EN_M                    0x00010000
#define FCFG1_OSC_CONF_HPOSC_BIAS_HOLD_MODE_EN_S                            16

// Field: [15:12] HPOSC_CURRMIRR_RATIO
//
// Internal. Only to be used through TI provided API.
#define FCFG1_OSC_CONF_HPOSC_CURRMIRR_RATIO_W                                4
#define FCFG1_OSC_CONF_HPOSC_CURRMIRR_RATIO_M                       0x0000F000
#define FCFG1_OSC_CONF_HPOSC_CURRMIRR_RATIO_S                               12

// Field:  [11:8] HPOSC_BIAS_RES_SET
//
// Internal. Only to be used through TI provided API.
#define FCFG1_OSC_CONF_HPOSC_BIAS_RES_SET_W                                  4
#define FCFG1_OSC_CONF_HPOSC_BIAS_RES_SET_M                         0x00000F00
#define FCFG1_OSC_CONF_HPOSC_BIAS_RES_SET_S                                  8

// Field:     [7] HPOSC_FILTER_EN
//
// Internal. Only to be used through TI provided API.
#define FCFG1_OSC_CONF_HPOSC_FILTER_EN                              0x00000080
#define FCFG1_OSC_CONF_HPOSC_FILTER_EN_BITN                                  7
#define FCFG1_OSC_CONF_HPOSC_FILTER_EN_M                            0x00000080
#define FCFG1_OSC_CONF_HPOSC_FILTER_EN_S                                     7

// Field:   [6:5] HPOSC_BIAS_RECHARGE_DELAY
//
// Internal. Only to be used through TI provided API.
#define FCFG1_OSC_CONF_HPOSC_BIAS_RECHARGE_DELAY_W                           2
#define FCFG1_OSC_CONF_HPOSC_BIAS_RECHARGE_DELAY_M                  0x00000060
#define FCFG1_OSC_CONF_HPOSC_BIAS_RECHARGE_DELAY_S                           5

// Field:   [2:1] HPOSC_SERIES_CAP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_OSC_CONF_HPOSC_SERIES_CAP_W                                    2
#define FCFG1_OSC_CONF_HPOSC_SERIES_CAP_M                           0x00000006
#define FCFG1_OSC_CONF_HPOSC_SERIES_CAP_S                                    1

// Field:     [0] HPOSC_DIV3_BYPASS
//
// Internal. Only to be used through TI provided API.
#define FCFG1_OSC_CONF_HPOSC_DIV3_BYPASS                            0x00000001
#define FCFG1_OSC_CONF_HPOSC_DIV3_BYPASS_BITN                                0
#define FCFG1_OSC_CONF_HPOSC_DIV3_BYPASS_M                          0x00000001
#define FCFG1_OSC_CONF_HPOSC_DIV3_BYPASS_S                                   0

//*****************************************************************************
//
// Register: FCFG1_O_FREQ_OFFSET
//
//*****************************************************************************
// Field: [31:16] HPOSC_COMP_P0
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P0_W                                   16
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P0_M                           0xFFFF0000
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P0_S                                   16

// Field:  [15:8] HPOSC_COMP_P1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P1_W                                    8
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P1_M                           0x0000FF00
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P1_S                                    8

// Field:   [7:0] HPOSC_COMP_P2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P2_W                                    8
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P2_M                           0x000000FF
#define FCFG1_FREQ_OFFSET_HPOSC_COMP_P2_S                                    0

//*****************************************************************************
//
// Register: FCFG1_O_MISC_OTP_DATA_1
//
//*****************************************************************************
// Field: [28:27] PEAK_DET_ITRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_1_PEAK_DET_ITRIM_W                               2
#define FCFG1_MISC_OTP_DATA_1_PEAK_DET_ITRIM_M                      0x18000000
#define FCFG1_MISC_OTP_DATA_1_PEAK_DET_ITRIM_S                              27

// Field: [26:24] HP_BUF_ITRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_1_HP_BUF_ITRIM_W                                 3
#define FCFG1_MISC_OTP_DATA_1_HP_BUF_ITRIM_M                        0x07000000
#define FCFG1_MISC_OTP_DATA_1_HP_BUF_ITRIM_S                                24

// Field: [23:22] LP_BUF_ITRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_1_LP_BUF_ITRIM_W                                 2
#define FCFG1_MISC_OTP_DATA_1_LP_BUF_ITRIM_M                        0x00C00000
#define FCFG1_MISC_OTP_DATA_1_LP_BUF_ITRIM_S                                22

// Field: [21:20] DBLR_LOOP_FILTER_RESET_VOLTAGE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_1_DBLR_LOOP_FILTER_RESET_VOLTAGE_W               2
#define FCFG1_MISC_OTP_DATA_1_DBLR_LOOP_FILTER_RESET_VOLTAGE_M      0x00300000
#define FCFG1_MISC_OTP_DATA_1_DBLR_LOOP_FILTER_RESET_VOLTAGE_S              20

// Field: [19:10] HPM_IBIAS_WAIT_CNT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_1_HPM_IBIAS_WAIT_CNT_W                          10
#define FCFG1_MISC_OTP_DATA_1_HPM_IBIAS_WAIT_CNT_M                  0x000FFC00
#define FCFG1_MISC_OTP_DATA_1_HPM_IBIAS_WAIT_CNT_S                          10

// Field:   [9:4] LPM_IBIAS_WAIT_CNT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_1_LPM_IBIAS_WAIT_CNT_W                           6
#define FCFG1_MISC_OTP_DATA_1_LPM_IBIAS_WAIT_CNT_M                  0x000003F0
#define FCFG1_MISC_OTP_DATA_1_LPM_IBIAS_WAIT_CNT_S                           4

// Field:   [3:0] IDAC_STEP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_MISC_OTP_DATA_1_IDAC_STEP_W                                    4
#define FCFG1_MISC_OTP_DATA_1_IDAC_STEP_M                           0x0000000F
#define FCFG1_MISC_OTP_DATA_1_IDAC_STEP_S                                    0

//*****************************************************************************
//
// Register: FCFG1_O_SHDW_DIE_ID_0
//
//*****************************************************************************
// Field:  [31:0] ID_31_0
//
// Shadow of DIE_ID_0 register in eFuse row number 5
#define FCFG1_SHDW_DIE_ID_0_ID_31_0_W                                       32
#define FCFG1_SHDW_DIE_ID_0_ID_31_0_M                               0xFFFFFFFF
#define FCFG1_SHDW_DIE_ID_0_ID_31_0_S                                        0

//*****************************************************************************
//
// Register: FCFG1_O_SHDW_DIE_ID_1
//
//*****************************************************************************
// Field:  [31:0] ID_63_32
//
// Shadow of DIE_ID_1 register in eFuse row number 6
#define FCFG1_SHDW_DIE_ID_1_ID_63_32_W                                      32
#define FCFG1_SHDW_DIE_ID_1_ID_63_32_M                              0xFFFFFFFF
#define FCFG1_SHDW_DIE_ID_1_ID_63_32_S                                       0

//*****************************************************************************
//
// Register: FCFG1_O_SHDW_DIE_ID_2
//
//*****************************************************************************
// Field:  [31:0] ID_95_64
//
// Shadow of DIE_ID_2 register in eFuse row number 7
#define FCFG1_SHDW_DIE_ID_2_ID_95_64_W                                      32
#define FCFG1_SHDW_DIE_ID_2_ID_95_64_M                              0xFFFFFFFF
#define FCFG1_SHDW_DIE_ID_2_ID_95_64_S                                       0

//*****************************************************************************
//
// Register: FCFG1_O_SHDW_DIE_ID_3
//
//*****************************************************************************
// Field:  [31:0] ID_127_96
//
// Shadow of DIE_ID_3 register in eFuse row number 8
#define FCFG1_SHDW_DIE_ID_3_ID_127_96_W                                     32
#define FCFG1_SHDW_DIE_ID_3_ID_127_96_M                             0xFFFFFFFF
#define FCFG1_SHDW_DIE_ID_3_ID_127_96_S                                      0

//*****************************************************************************
//
// Register: FCFG1_O_SHDW_OSC_BIAS_LDO_TRIM
//
//*****************************************************************************
// Field: [26:23] TRIMMAG
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_TRIMMAG_W                               4
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_TRIMMAG_M                      0x07800000
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_TRIMMAG_S                              23

// Field: [22:18] TRIMIREF
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_TRIMIREF_W                              5
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_TRIMIREF_M                     0x007C0000
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_TRIMIREF_S                             18

// Field: [17:16] ITRIM_DIG_LDO
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_ITRIM_DIG_LDO_W                         2
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_ITRIM_DIG_LDO_M                0x00030000
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_ITRIM_DIG_LDO_S                        16

// Field: [15:12] VTRIM_DIG
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_VTRIM_DIG_W                             4
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_VTRIM_DIG_M                    0x0000F000
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_VTRIM_DIG_S                            12

// Field:  [11:8] VTRIM_COARSE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_VTRIM_COARSE_W                          4
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_VTRIM_COARSE_M                 0x00000F00
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_VTRIM_COARSE_S                          8

// Field:   [7:0] RCOSCHF_CTRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_RCOSCHF_CTRIM_W                         8
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_RCOSCHF_CTRIM_M                0x000000FF
#define FCFG1_SHDW_OSC_BIAS_LDO_TRIM_RCOSCHF_CTRIM_S                         0

//*****************************************************************************
//
// Register: FCFG1_O_SHDW_ANA_TRIM
//
//*****************************************************************************
// Field:    [30] ALT_VDDR_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_ALT_VDDR_TRIM                           0x40000000
#define FCFG1_SHDW_ANA_TRIM_ALT_VDDR_TRIM_BITN                              30
#define FCFG1_SHDW_ANA_TRIM_ALT_VDDR_TRIM_M                         0x40000000
#define FCFG1_SHDW_ANA_TRIM_ALT_VDDR_TRIM_S                                 30

// Field:    [29] DET_LOGIC_DIS
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_DET_LOGIC_DIS                           0x20000000
#define FCFG1_SHDW_ANA_TRIM_DET_LOGIC_DIS_BITN                              29
#define FCFG1_SHDW_ANA_TRIM_DET_LOGIC_DIS_M                         0x20000000
#define FCFG1_SHDW_ANA_TRIM_DET_LOGIC_DIS_S                                 29

// Field: [28:27] BOD_BANDGAP_TRIM_CNF_EXT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_BOD_BANDGAP_TRIM_CNF_EXT_W                       2
#define FCFG1_SHDW_ANA_TRIM_BOD_BANDGAP_TRIM_CNF_EXT_M              0x18000000
#define FCFG1_SHDW_ANA_TRIM_BOD_BANDGAP_TRIM_CNF_EXT_S                      27

// Field: [26:25] BOD_BANDGAP_TRIM_CNF
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_BOD_BANDGAP_TRIM_CNF_W                           2
#define FCFG1_SHDW_ANA_TRIM_BOD_BANDGAP_TRIM_CNF_M                  0x06000000
#define FCFG1_SHDW_ANA_TRIM_BOD_BANDGAP_TRIM_CNF_S                          25

// Field:    [24] VDDR_ENABLE_PG1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_VDDR_ENABLE_PG1                         0x01000000
#define FCFG1_SHDW_ANA_TRIM_VDDR_ENABLE_PG1_BITN                            24
#define FCFG1_SHDW_ANA_TRIM_VDDR_ENABLE_PG1_M                       0x01000000
#define FCFG1_SHDW_ANA_TRIM_VDDR_ENABLE_PG1_S                               24

// Field:    [23] VDDR_OK_HYS
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_VDDR_OK_HYS                             0x00800000
#define FCFG1_SHDW_ANA_TRIM_VDDR_OK_HYS_BITN                                23
#define FCFG1_SHDW_ANA_TRIM_VDDR_OK_HYS_M                           0x00800000
#define FCFG1_SHDW_ANA_TRIM_VDDR_OK_HYS_S                                   23

// Field: [22:21] IPTAT_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_IPTAT_TRIM_W                                     2
#define FCFG1_SHDW_ANA_TRIM_IPTAT_TRIM_M                            0x00600000
#define FCFG1_SHDW_ANA_TRIM_IPTAT_TRIM_S                                    21

// Field: [20:16] VDDR_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_VDDR_TRIM_W                                      5
#define FCFG1_SHDW_ANA_TRIM_VDDR_TRIM_M                             0x001F0000
#define FCFG1_SHDW_ANA_TRIM_VDDR_TRIM_S                                     16

// Field: [15:11] TRIMBOD_INTMODE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_TRIMBOD_INTMODE_W                                5
#define FCFG1_SHDW_ANA_TRIM_TRIMBOD_INTMODE_M                       0x0000F800
#define FCFG1_SHDW_ANA_TRIM_TRIMBOD_INTMODE_S                               11

// Field:  [10:6] TRIMBOD_EXTMODE
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_TRIMBOD_EXTMODE_W                                5
#define FCFG1_SHDW_ANA_TRIM_TRIMBOD_EXTMODE_M                       0x000007C0
#define FCFG1_SHDW_ANA_TRIM_TRIMBOD_EXTMODE_S                                6

// Field:   [5:0] TRIMTEMP
//
// Internal. Only to be used through TI provided API.
#define FCFG1_SHDW_ANA_TRIM_TRIMTEMP_W                                       6
#define FCFG1_SHDW_ANA_TRIM_TRIMTEMP_M                              0x0000003F
#define FCFG1_SHDW_ANA_TRIM_TRIMTEMP_S                                       0

//*****************************************************************************
//
// Register: FCFG1_O_DAC_BIAS_CNF
//
//*****************************************************************************
// Field: [17:12] LPM_TRIM_IOUT
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_BIAS_CNF_LPM_TRIM_IOUT_W                                   6
#define FCFG1_DAC_BIAS_CNF_LPM_TRIM_IOUT_M                          0x0003F000
#define FCFG1_DAC_BIAS_CNF_LPM_TRIM_IOUT_S                                  12

// Field:  [11:9] LPM_BIAS_WIDTH_TRIM
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_BIAS_CNF_LPM_BIAS_WIDTH_TRIM_W                             3
#define FCFG1_DAC_BIAS_CNF_LPM_BIAS_WIDTH_TRIM_M                    0x00000E00
#define FCFG1_DAC_BIAS_CNF_LPM_BIAS_WIDTH_TRIM_S                             9

// Field:     [8] LPM_BIAS_BACKUP_EN
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_BIAS_CNF_LPM_BIAS_BACKUP_EN                       0x00000100
#define FCFG1_DAC_BIAS_CNF_LPM_BIAS_BACKUP_EN_BITN                           8
#define FCFG1_DAC_BIAS_CNF_LPM_BIAS_BACKUP_EN_M                     0x00000100
#define FCFG1_DAC_BIAS_CNF_LPM_BIAS_BACKUP_EN_S                              8

//*****************************************************************************
//
// Register: FCFG1_O_TFW_PROBE
//
//*****************************************************************************
// Field:  [31:0] REV
//
// Internal. Only to be used through TI provided API.
#define FCFG1_TFW_PROBE_REV_W                                               32
#define FCFG1_TFW_PROBE_REV_M                                       0xFFFFFFFF
#define FCFG1_TFW_PROBE_REV_S                                                0

//*****************************************************************************
//
// Register: FCFG1_O_TFW_FT
//
//*****************************************************************************
// Field:  [31:0] REV
//
// Internal. Only to be used through TI provided API.
#define FCFG1_TFW_FT_REV_W                                                  32
#define FCFG1_TFW_FT_REV_M                                          0xFFFFFFFF
#define FCFG1_TFW_FT_REV_S                                                   0

//*****************************************************************************
//
// Register: FCFG1_O_DAC_CAL0
//
//*****************************************************************************
// Field: [31:16] SOC_DAC_VOUT_CAL_DECOUPLE_C2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_CAL0_SOC_DAC_VOUT_CAL_DECOUPLE_C2_W                       16
#define FCFG1_DAC_CAL0_SOC_DAC_VOUT_CAL_DECOUPLE_C2_M               0xFFFF0000
#define FCFG1_DAC_CAL0_SOC_DAC_VOUT_CAL_DECOUPLE_C2_S                       16

// Field:  [15:0] SOC_DAC_VOUT_CAL_DECOUPLE_C1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_CAL0_SOC_DAC_VOUT_CAL_DECOUPLE_C1_W                       16
#define FCFG1_DAC_CAL0_SOC_DAC_VOUT_CAL_DECOUPLE_C1_M               0x0000FFFF
#define FCFG1_DAC_CAL0_SOC_DAC_VOUT_CAL_DECOUPLE_C1_S                        0

//*****************************************************************************
//
// Register: FCFG1_O_DAC_CAL1
//
//*****************************************************************************
// Field: [31:16] SOC_DAC_VOUT_CAL_PRECH_C2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_CAL1_SOC_DAC_VOUT_CAL_PRECH_C2_W                          16
#define FCFG1_DAC_CAL1_SOC_DAC_VOUT_CAL_PRECH_C2_M                  0xFFFF0000
#define FCFG1_DAC_CAL1_SOC_DAC_VOUT_CAL_PRECH_C2_S                          16

// Field:  [15:0] SOC_DAC_VOUT_CAL_PRECH_C1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_CAL1_SOC_DAC_VOUT_CAL_PRECH_C1_W                          16
#define FCFG1_DAC_CAL1_SOC_DAC_VOUT_CAL_PRECH_C1_M                  0x0000FFFF
#define FCFG1_DAC_CAL1_SOC_DAC_VOUT_CAL_PRECH_C1_S                           0

//*****************************************************************************
//
// Register: FCFG1_O_DAC_CAL2
//
//*****************************************************************************
// Field: [31:16] SOC_DAC_VOUT_CAL_ADCREF_C2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_CAL2_SOC_DAC_VOUT_CAL_ADCREF_C2_W                         16
#define FCFG1_DAC_CAL2_SOC_DAC_VOUT_CAL_ADCREF_C2_M                 0xFFFF0000
#define FCFG1_DAC_CAL2_SOC_DAC_VOUT_CAL_ADCREF_C2_S                         16

// Field:  [15:0] SOC_DAC_VOUT_CAL_ADCREF_C1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_CAL2_SOC_DAC_VOUT_CAL_ADCREF_C1_W                         16
#define FCFG1_DAC_CAL2_SOC_DAC_VOUT_CAL_ADCREF_C1_M                 0x0000FFFF
#define FCFG1_DAC_CAL2_SOC_DAC_VOUT_CAL_ADCREF_C1_S                          0

//*****************************************************************************
//
// Register: FCFG1_O_DAC_CAL3
//
//*****************************************************************************
// Field: [31:16] SOC_DAC_VOUT_CAL_VDDS_C2
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_CAL3_SOC_DAC_VOUT_CAL_VDDS_C2_W                           16
#define FCFG1_DAC_CAL3_SOC_DAC_VOUT_CAL_VDDS_C2_M                   0xFFFF0000
#define FCFG1_DAC_CAL3_SOC_DAC_VOUT_CAL_VDDS_C2_S                           16

// Field:  [15:0] SOC_DAC_VOUT_CAL_VDDS_C1
//
// Internal. Only to be used through TI provided API.
#define FCFG1_DAC_CAL3_SOC_DAC_VOUT_CAL_VDDS_C1_W                           16
#define FCFG1_DAC_CAL3_SOC_DAC_VOUT_CAL_VDDS_C1_M                   0x0000FFFF
#define FCFG1_DAC_CAL3_SOC_DAC_VOUT_CAL_VDDS_C1_S                            0


#endif // __FCFG1__
