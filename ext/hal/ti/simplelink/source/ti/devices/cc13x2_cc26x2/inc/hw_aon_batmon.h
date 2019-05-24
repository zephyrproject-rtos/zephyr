/******************************************************************************
*  Filename:       hw_aon_batmon_h
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

#ifndef __HW_AON_BATMON_H__
#define __HW_AON_BATMON_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AON_BATMON component
//
//*****************************************************************************
// Internal
#define AON_BATMON_O_CTL                                            0x00000000

// Internal
#define AON_BATMON_O_MEASCFG                                        0x00000004

// Internal
#define AON_BATMON_O_TEMPP0                                         0x0000000C

// Internal
#define AON_BATMON_O_TEMPP1                                         0x00000010

// Internal
#define AON_BATMON_O_TEMPP2                                         0x00000014

// Internal
#define AON_BATMON_O_BATMONP0                                       0x00000018

// Internal
#define AON_BATMON_O_BATMONP1                                       0x0000001C

// Internal
#define AON_BATMON_O_IOSTRP0                                        0x00000020

// Internal
#define AON_BATMON_O_FLASHPUMPP0                                    0x00000024

// Last Measured Battery Voltage
#define AON_BATMON_O_BAT                                            0x00000028

// Battery Update
#define AON_BATMON_O_BATUPD                                         0x0000002C

// Temperature
#define AON_BATMON_O_TEMP                                           0x00000030

// Temperature Update
#define AON_BATMON_O_TEMPUPD                                        0x00000034

// Event Mask
#define AON_BATMON_O_EVENTMASK                                      0x00000048

// Event
#define AON_BATMON_O_EVENT                                          0x0000004C

// Battery Upper Limit
#define AON_BATMON_O_BATTUL                                         0x00000050

// Battery Lower Limit
#define AON_BATMON_O_BATTLL                                         0x00000054

// Temperature Upper Limit
#define AON_BATMON_O_TEMPUL                                         0x00000058

// Temperature Lower Limit
#define AON_BATMON_O_TEMPLL                                         0x0000005C

//*****************************************************************************
//
// Register: AON_BATMON_O_CTL
//
//*****************************************************************************
// Field:     [1] CALC_EN
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_CTL_CALC_EN                                      0x00000002
#define AON_BATMON_CTL_CALC_EN_BITN                                          1
#define AON_BATMON_CTL_CALC_EN_M                                    0x00000002
#define AON_BATMON_CTL_CALC_EN_S                                             1

// Field:     [0] MEAS_EN
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_CTL_MEAS_EN                                      0x00000001
#define AON_BATMON_CTL_MEAS_EN_BITN                                          0
#define AON_BATMON_CTL_MEAS_EN_M                                    0x00000001
#define AON_BATMON_CTL_MEAS_EN_S                                             0

//*****************************************************************************
//
// Register: AON_BATMON_O_MEASCFG
//
//*****************************************************************************
// Field:   [1:0] PER
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// 32CYC                    Internal. Only to be used through TI provided API.
// 16CYC                    Internal. Only to be used through TI provided API.
// 8CYC                     Internal. Only to be used through TI provided API.
// CONT                     Internal. Only to be used through TI provided API.
#define AON_BATMON_MEASCFG_PER_W                                             2
#define AON_BATMON_MEASCFG_PER_M                                    0x00000003
#define AON_BATMON_MEASCFG_PER_S                                             0
#define AON_BATMON_MEASCFG_PER_32CYC                                0x00000003
#define AON_BATMON_MEASCFG_PER_16CYC                                0x00000002
#define AON_BATMON_MEASCFG_PER_8CYC                                 0x00000001
#define AON_BATMON_MEASCFG_PER_CONT                                 0x00000000

//*****************************************************************************
//
// Register: AON_BATMON_O_TEMPP0
//
//*****************************************************************************
// Field:   [7:0] CFG
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_TEMPP0_CFG_W                                              8
#define AON_BATMON_TEMPP0_CFG_M                                     0x000000FF
#define AON_BATMON_TEMPP0_CFG_S                                              0

//*****************************************************************************
//
// Register: AON_BATMON_O_TEMPP1
//
//*****************************************************************************
// Field:   [5:0] CFG
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_TEMPP1_CFG_W                                              6
#define AON_BATMON_TEMPP1_CFG_M                                     0x0000003F
#define AON_BATMON_TEMPP1_CFG_S                                              0

//*****************************************************************************
//
// Register: AON_BATMON_O_TEMPP2
//
//*****************************************************************************
// Field:   [4:0] CFG
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_TEMPP2_CFG_W                                              5
#define AON_BATMON_TEMPP2_CFG_M                                     0x0000001F
#define AON_BATMON_TEMPP2_CFG_S                                              0

//*****************************************************************************
//
// Register: AON_BATMON_O_BATMONP0
//
//*****************************************************************************
// Field:   [6:0] CFG
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_BATMONP0_CFG_W                                            7
#define AON_BATMON_BATMONP0_CFG_M                                   0x0000007F
#define AON_BATMON_BATMONP0_CFG_S                                            0

//*****************************************************************************
//
// Register: AON_BATMON_O_BATMONP1
//
//*****************************************************************************
// Field:   [5:0] CFG
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_BATMONP1_CFG_W                                            6
#define AON_BATMON_BATMONP1_CFG_M                                   0x0000003F
#define AON_BATMON_BATMONP1_CFG_S                                            0

//*****************************************************************************
//
// Register: AON_BATMON_O_IOSTRP0
//
//*****************************************************************************
// Field:   [5:4] CFG2
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_IOSTRP0_CFG2_W                                            2
#define AON_BATMON_IOSTRP0_CFG2_M                                   0x00000030
#define AON_BATMON_IOSTRP0_CFG2_S                                            4

// Field:   [3:0] CFG1
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_IOSTRP0_CFG1_W                                            4
#define AON_BATMON_IOSTRP0_CFG1_M                                   0x0000000F
#define AON_BATMON_IOSTRP0_CFG1_S                                            0

//*****************************************************************************
//
// Register: AON_BATMON_O_FLASHPUMPP0
//
//*****************************************************************************
// Field:     [9] DIS_NOISE_FILTER
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_FLASHPUMPP0_DIS_NOISE_FILTER                     0x00000200
#define AON_BATMON_FLASHPUMPP0_DIS_NOISE_FILTER_BITN                         9
#define AON_BATMON_FLASHPUMPP0_DIS_NOISE_FILTER_M                   0x00000200
#define AON_BATMON_FLASHPUMPP0_DIS_NOISE_FILTER_S                            9

// Field:     [8] FALLB
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_FLASHPUMPP0_FALLB                                0x00000100
#define AON_BATMON_FLASHPUMPP0_FALLB_BITN                                    8
#define AON_BATMON_FLASHPUMPP0_FALLB_M                              0x00000100
#define AON_BATMON_FLASHPUMPP0_FALLB_S                                       8

// Field:   [7:6] HIGHLIM
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_FLASHPUMPP0_HIGHLIM_W                                     2
#define AON_BATMON_FLASHPUMPP0_HIGHLIM_M                            0x000000C0
#define AON_BATMON_FLASHPUMPP0_HIGHLIM_S                                     6

// Field:     [5] LOWLIM
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_FLASHPUMPP0_LOWLIM                               0x00000020
#define AON_BATMON_FLASHPUMPP0_LOWLIM_BITN                                   5
#define AON_BATMON_FLASHPUMPP0_LOWLIM_M                             0x00000020
#define AON_BATMON_FLASHPUMPP0_LOWLIM_S                                      5

// Field:     [4] OVR
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_FLASHPUMPP0_OVR                                  0x00000010
#define AON_BATMON_FLASHPUMPP0_OVR_BITN                                      4
#define AON_BATMON_FLASHPUMPP0_OVR_M                                0x00000010
#define AON_BATMON_FLASHPUMPP0_OVR_S                                         4

// Field:   [3:0] CFG
//
// Internal. Only to be used through TI provided API.
#define AON_BATMON_FLASHPUMPP0_CFG_W                                         4
#define AON_BATMON_FLASHPUMPP0_CFG_M                                0x0000000F
#define AON_BATMON_FLASHPUMPP0_CFG_S                                         0

//*****************************************************************************
//
// Register: AON_BATMON_O_BAT
//
//*****************************************************************************
// Field:  [10:8] INT
//
// Integer part:
//
// 0x0: 0V + fractional part
// ...
// 0x3: 3V + fractional part
// 0x4: 4V + fractional part
#define AON_BATMON_BAT_INT_W                                                 3
#define AON_BATMON_BAT_INT_M                                        0x00000700
#define AON_BATMON_BAT_INT_S                                                 8

// Field:   [7:0] FRAC
//
// Fractional part, standard binary fractional encoding.
//
// 0x00: .0V
// ...
// 0x20: 1/8 = .125V
// 0x40: 1/4 = .25V
// 0x80: 1/2 = .5V
// ...
// 0xA0: 1/2 + 1/8 = .625V
// ...
// 0xFF: Max
#define AON_BATMON_BAT_FRAC_W                                                8
#define AON_BATMON_BAT_FRAC_M                                       0x000000FF
#define AON_BATMON_BAT_FRAC_S                                                0

//*****************************************************************************
//
// Register: AON_BATMON_O_BATUPD
//
//*****************************************************************************
// Field:     [0] STAT
//
//
// 0: No update since last clear
// 1: New battery voltage is present.
//
// Write 1 to clear the status.
#define AON_BATMON_BATUPD_STAT                                      0x00000001
#define AON_BATMON_BATUPD_STAT_BITN                                          0
#define AON_BATMON_BATUPD_STAT_M                                    0x00000001
#define AON_BATMON_BATUPD_STAT_S                                             0

//*****************************************************************************
//
// Register: AON_BATMON_O_TEMP
//
//*****************************************************************************
// Field:  [16:8] INT
//
// Integer part (signed) of temperature value.
// Total value = INTEGER + FRACTIONAL
// 2's complement encoding
//
// 0x100: Min value
// 0x1D8: -40C
// 0x1FF: -1C
// 0x00: 0C
// 0x1B: 27C
// 0x55: 85C
// 0xFF: Max value
#define AON_BATMON_TEMP_INT_W                                                9
#define AON_BATMON_TEMP_INT_M                                       0x0001FF00
#define AON_BATMON_TEMP_INT_S                                                8

//*****************************************************************************
//
// Register: AON_BATMON_O_TEMPUPD
//
//*****************************************************************************
// Field:     [0] STAT
//
//
// 0: No update since last clear
// 1: New temperature is present.
//
// Write 1 to clear the status.
#define AON_BATMON_TEMPUPD_STAT                                     0x00000001
#define AON_BATMON_TEMPUPD_STAT_BITN                                         0
#define AON_BATMON_TEMPUPD_STAT_M                                   0x00000001
#define AON_BATMON_TEMPUPD_STAT_S                                            0

//*****************************************************************************
//
// Register: AON_BATMON_O_EVENTMASK
//
//*****************************************************************************
// Field:     [5] TEMP_UPDATE_MASK
//
// 1: EVENT.TEMP_UPDATE contributes to combined event from BATMON
// 0: EVENT.TEMP_UPDATE does not contribute to combined event from BATMON
#define AON_BATMON_EVENTMASK_TEMP_UPDATE_MASK                       0x00000020
#define AON_BATMON_EVENTMASK_TEMP_UPDATE_MASK_BITN                           5
#define AON_BATMON_EVENTMASK_TEMP_UPDATE_MASK_M                     0x00000020
#define AON_BATMON_EVENTMASK_TEMP_UPDATE_MASK_S                              5

// Field:     [4] BATT_UPDATE_MASK
//
// 1: EVENT.BATT_UPDATE contributes to combined event from BATMON
// 0: EVENT.BATT_UPDATE does not contribute to combined event from BATMON
#define AON_BATMON_EVENTMASK_BATT_UPDATE_MASK                       0x00000010
#define AON_BATMON_EVENTMASK_BATT_UPDATE_MASK_BITN                           4
#define AON_BATMON_EVENTMASK_BATT_UPDATE_MASK_M                     0x00000010
#define AON_BATMON_EVENTMASK_BATT_UPDATE_MASK_S                              4

// Field:     [3] TEMP_BELOW_LL_MASK
//
// 1: EVENT.TEMP_BELOW_LL contributes to combined event from BATMON
// 0: EVENT.TEMP_BELOW_LL does not contribute to combined event from BATMON
#define AON_BATMON_EVENTMASK_TEMP_BELOW_LL_MASK                     0x00000008
#define AON_BATMON_EVENTMASK_TEMP_BELOW_LL_MASK_BITN                         3
#define AON_BATMON_EVENTMASK_TEMP_BELOW_LL_MASK_M                   0x00000008
#define AON_BATMON_EVENTMASK_TEMP_BELOW_LL_MASK_S                            3

// Field:     [2] TEMP_OVER_UL_MASK
//
// 1: EVENT.TEMP_OVER_UL contributes to combined event from BATMON
// 0: EVENT.TEMP_OVER_UL does not contribute to combined event from BATMON
#define AON_BATMON_EVENTMASK_TEMP_OVER_UL_MASK                      0x00000004
#define AON_BATMON_EVENTMASK_TEMP_OVER_UL_MASK_BITN                          2
#define AON_BATMON_EVENTMASK_TEMP_OVER_UL_MASK_M                    0x00000004
#define AON_BATMON_EVENTMASK_TEMP_OVER_UL_MASK_S                             2

// Field:     [1] BATT_BELOW_LL_MASK
//
// 1: EVENT.BATT_BELOW_LL contributes to combined event from BATMON
// 0: EVENT.BATT_BELOW_LL does not contribute to combined event from BATMON
#define AON_BATMON_EVENTMASK_BATT_BELOW_LL_MASK                     0x00000002
#define AON_BATMON_EVENTMASK_BATT_BELOW_LL_MASK_BITN                         1
#define AON_BATMON_EVENTMASK_BATT_BELOW_LL_MASK_M                   0x00000002
#define AON_BATMON_EVENTMASK_BATT_BELOW_LL_MASK_S                            1

// Field:     [0] BATT_OVER_UL_MASK
//
// 1: EVENT.BATT_OVER_UL contributes to combined event from BATMON
// 0: EVENT.BATT_OVER_UL does not contribute to combined event from BATMON
#define AON_BATMON_EVENTMASK_BATT_OVER_UL_MASK                      0x00000001
#define AON_BATMON_EVENTMASK_BATT_OVER_UL_MASK_BITN                          0
#define AON_BATMON_EVENTMASK_BATT_OVER_UL_MASK_M                    0x00000001
#define AON_BATMON_EVENTMASK_BATT_OVER_UL_MASK_S                             0

//*****************************************************************************
//
// Register: AON_BATMON_O_EVENT
//
//*****************************************************************************
// Field:     [5] TEMP_UPDATE
//
// Alias to TEMPUPD.STAT
#define AON_BATMON_EVENT_TEMP_UPDATE                                0x00000020
#define AON_BATMON_EVENT_TEMP_UPDATE_BITN                                    5
#define AON_BATMON_EVENT_TEMP_UPDATE_M                              0x00000020
#define AON_BATMON_EVENT_TEMP_UPDATE_S                                       5

// Field:     [4] BATT_UPDATE
//
// Alias to BATUPD.STAT
#define AON_BATMON_EVENT_BATT_UPDATE                                0x00000010
#define AON_BATMON_EVENT_BATT_UPDATE_BITN                                    4
#define AON_BATMON_EVENT_BATT_UPDATE_M                              0x00000010
#define AON_BATMON_EVENT_BATT_UPDATE_S                                       4

// Field:     [3] TEMP_BELOW_LL
//
// Read:
// 1: Temperature level is below the lower limit set by TEMPLL.
// 0: Temperature level is not below the lower limit set by TEMPLL.
// Write:
// 1: Clears the flag
// 0: No change in the flag
#define AON_BATMON_EVENT_TEMP_BELOW_LL                              0x00000008
#define AON_BATMON_EVENT_TEMP_BELOW_LL_BITN                                  3
#define AON_BATMON_EVENT_TEMP_BELOW_LL_M                            0x00000008
#define AON_BATMON_EVENT_TEMP_BELOW_LL_S                                     3

// Field:     [2] TEMP_OVER_UL
//
// Read:
// 1: Temperature level is above the upper limit set by TEMPUL.
// 0: Temperature level is not above the upper limit set by TEMPUL.
// Write:
// 1: Clears the flag
// 0: No change in the flag
#define AON_BATMON_EVENT_TEMP_OVER_UL                               0x00000004
#define AON_BATMON_EVENT_TEMP_OVER_UL_BITN                                   2
#define AON_BATMON_EVENT_TEMP_OVER_UL_M                             0x00000004
#define AON_BATMON_EVENT_TEMP_OVER_UL_S                                      2

// Field:     [1] BATT_BELOW_LL
//
// Read:
// 1: Battery level is below the lower limit set by BATTLL.
// 0: Battery level is not below the lower limit set by BATTLL.
// Write:
// 1: Clears the flag
// 0: No change in the flag
#define AON_BATMON_EVENT_BATT_BELOW_LL                              0x00000002
#define AON_BATMON_EVENT_BATT_BELOW_LL_BITN                                  1
#define AON_BATMON_EVENT_BATT_BELOW_LL_M                            0x00000002
#define AON_BATMON_EVENT_BATT_BELOW_LL_S                                     1

// Field:     [0] BATT_OVER_UL
//
// Read:
// 1: Battery level is above the upper limit set by BATTUL.
// 0: Battery level is not above the upper limit set by BATTUL.
// Write:
// 1: Clears the flag
// 0: No change in the flag
#define AON_BATMON_EVENT_BATT_OVER_UL                               0x00000001
#define AON_BATMON_EVENT_BATT_OVER_UL_BITN                                   0
#define AON_BATMON_EVENT_BATT_OVER_UL_M                             0x00000001
#define AON_BATMON_EVENT_BATT_OVER_UL_S                                      0

//*****************************************************************************
//
// Register: AON_BATMON_O_BATTUL
//
//*****************************************************************************
// Field:  [10:8] INT
//
// Integer part:
//
// 0x0: 0V + fractional part
// ...
// 0x3: 3V + fractional part
// 0x4: 4V + fractional part
#define AON_BATMON_BATTUL_INT_W                                              3
#define AON_BATMON_BATTUL_INT_M                                     0x00000700
#define AON_BATMON_BATTUL_INT_S                                              8

// Field:   [7:0] FRAC
//
// Fractional part, standard binary fractional encoding.
//
// 0x00: .0V
// ...
// 0x20: 1/8 = .125V
// 0x40: 1/4 = .25V
// 0x80: 1/2 = .5V
// ...
// 0xA0: 1/2 + 1/8 = .625V
// ...
// 0xFF: Max
#define AON_BATMON_BATTUL_FRAC_W                                             8
#define AON_BATMON_BATTUL_FRAC_M                                    0x000000FF
#define AON_BATMON_BATTUL_FRAC_S                                             0

//*****************************************************************************
//
// Register: AON_BATMON_O_BATTLL
//
//*****************************************************************************
// Field:  [10:8] INT
//
// Integer part:
//
// 0x0: 0V + fractional part
// ...
// 0x3: 3V + fractional part
// 0x4: 4V + fractional part
#define AON_BATMON_BATTLL_INT_W                                              3
#define AON_BATMON_BATTLL_INT_M                                     0x00000700
#define AON_BATMON_BATTLL_INT_S                                              8

// Field:   [7:0] FRAC
//
// Fractional part, standard binary fractional encoding.
//
// 0x00: .0V
// ...
// 0x20: 1/8 = .125V
// 0x40: 1/4 = .25V
// 0x80: 1/2 = .5V
// ...
// 0xA0: 1/2 + 1/8 = .625V
// ...
// 0xFF: Max
#define AON_BATMON_BATTLL_FRAC_W                                             8
#define AON_BATMON_BATTLL_FRAC_M                                    0x000000FF
#define AON_BATMON_BATTLL_FRAC_S                                             0

//*****************************************************************************
//
// Register: AON_BATMON_O_TEMPUL
//
//*****************************************************************************
// Field:  [16:8] INT
//
// Integer part (signed) of temperature upper limit.
// Total value = INTEGER + FRACTIONAL
// 2's complement encoding
//
// 0x100: Min value
// 0x1D8: -40C
// 0x1FF: -1C
// 0x00: 0C
// 0x1B: 27C
// 0x55: 85C
// 0xFF: Max value
#define AON_BATMON_TEMPUL_INT_W                                              9
#define AON_BATMON_TEMPUL_INT_M                                     0x0001FF00
#define AON_BATMON_TEMPUL_INT_S                                              8

// Field:   [7:6] FRAC
//
// Fractional part of temperature upper limit.
// Total value = INTEGER + FRACTIONAL
// The encoding is an extension of the 2's complement encoding.
//
// 00: 0.0C
// 01: 0.25C
// 10: 0.5C
// 11: 0.75C
//
// For example:
// 000000001,00 = ( 1+0,00) =  1,00
// 000000000,11 = ( 0+0,75) =  0,75
// 000000000,10 = ( 0+0,50) =  0,50
// 000000000,01 = ( 0+0,25) =  0,25
// 000000000,00 = ( 0+0,00) =  0,00
// 111111111,11 = (-1+0,75) = -0,25
// 111111111,10 = (-1+0,50) = -0,50
// 111111111,01 = (-1+0,25) = -0,75
// 111111111,00 = (-1+0,00) = -1,00
// 111111110,11 = (-2+0,75) = -1,25
#define AON_BATMON_TEMPUL_FRAC_W                                             2
#define AON_BATMON_TEMPUL_FRAC_M                                    0x000000C0
#define AON_BATMON_TEMPUL_FRAC_S                                             6

//*****************************************************************************
//
// Register: AON_BATMON_O_TEMPLL
//
//*****************************************************************************
// Field:  [16:8] INT
//
// Integer part (signed) of temperature lower limit.
// Total value = INTEGER + FRACTIONAL
// 2's complement encoding
//
// 0x100: Min value
// 0x1D8: -40C
// 0x1FF: -1C
// 0x00: 0C
// 0x1B: 27C
// 0x55: 85C
// 0xFF: Max value
#define AON_BATMON_TEMPLL_INT_W                                              9
#define AON_BATMON_TEMPLL_INT_M                                     0x0001FF00
#define AON_BATMON_TEMPLL_INT_S                                              8

// Field:   [7:6] FRAC
//
// Fractional part of temperature lower limit.
// Total value = INTEGER + FRACTIONAL
// The encoding is an extension of the 2's complement encoding.
//
// 00: 0.0C
// 01: 0.25C
// 10: 0.5C
// 11: 0.75C
//
// For example:
// 000000001,00 = ( 1+0,00) =  1,00
// 000000000,11 = ( 0+0,75) =  0,75
// 000000000,10 = ( 0+0,50) =  0,50
// 000000000,01 = ( 0+0,25) =  0,25
// 000000000,00 = ( 0+0,00) =  0,00
// 111111111,11 = (-1+0,75) = -0,25
// 111111111,10 = (-1+0,50) = -0,50
// 111111111,01 = (-1+0,25) = -0,75
// 111111111,00 = (-1+0,00) = -1,00
// 111111110,11 = (-2+0,75) = -1,25
#define AON_BATMON_TEMPLL_FRAC_W                                             2
#define AON_BATMON_TEMPLL_FRAC_M                                    0x000000C0
#define AON_BATMON_TEMPLL_FRAC_S                                             6


#endif // __AON_BATMON__
