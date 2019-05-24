/******************************************************************************
*  Filename:       hw_aux_timer01_h
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

#ifndef __HW_AUX_TIMER01_H__
#define __HW_AUX_TIMER01_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_TIMER01 component
//
//*****************************************************************************
// Timer 0 Configuration
#define AUX_TIMER01_O_T0CFG                                         0x00000000

// Timer 0 Control
#define AUX_TIMER01_O_T0CTL                                         0x00000004

// Timer 0 Target
#define AUX_TIMER01_O_T0TARGET                                      0x00000008

// Timer 0 Counter
#define AUX_TIMER01_O_T0CNTR                                        0x0000000C

// Timer 1 Configuration
#define AUX_TIMER01_O_T1CFG                                         0x00000010

// Timer 1 Control
#define AUX_TIMER01_O_T1CTL                                         0x00000014

// Timer 1 Target
#define AUX_TIMER01_O_T1TARGET                                      0x00000018

// Timer 1 Counter
#define AUX_TIMER01_O_T1CNTR                                        0x0000001C

//*****************************************************************************
//
// Register: AUX_TIMER01_O_T0CFG
//
//*****************************************************************************
// Field:    [14] TICK_SRC_POL
//
// Tick source polarity for Timer 0.
// ENUMs:
// FALL                     Count on falling edges of TICK_SRC.
// RISE                     Count on rising edges of TICK_SRC.
#define AUX_TIMER01_T0CFG_TICK_SRC_POL                              0x00004000
#define AUX_TIMER01_T0CFG_TICK_SRC_POL_BITN                                 14
#define AUX_TIMER01_T0CFG_TICK_SRC_POL_M                            0x00004000
#define AUX_TIMER01_T0CFG_TICK_SRC_POL_S                                    14
#define AUX_TIMER01_T0CFG_TICK_SRC_POL_FALL                         0x00004000
#define AUX_TIMER01_T0CFG_TICK_SRC_POL_RISE                         0x00000000

// Field:  [13:8] TICK_SRC
//
// Select Timer 0 tick source from the synchronous event bus.
// ENUMs:
// AUX_TIMER2_CLKSW_RDY     AUX_EVCTL:EVSTAT3.AUX_TIMER2_CLKSWITCH_RDY
// AUX_DAC_HOLD_ACTIVE      AUX_EVCTL:EVSTAT3.AUX_DAC_HOLD_ACTIVE
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// NO_EVENT                 No event.
// AUX_TIMER1_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER1_EV
// AUX_TIMER2_PULSE         AUX_EVCTL:EVSTAT3.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                AUX_EVCTL:EVSTAT2.AUX_COMPB
// AUX_COMPA                AUX_EVCTL:EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              AUX_EVCTL:EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              AUX_EVCTL:EVSTAT2.MCU_OBSMUX0
// MCU_EV                   AUX_EVCTL:EVSTAT2.MCU_EV
// ACLK_REF                 AUX_EVCTL:EVSTAT2.ACLK_REF
// VDDR_RECHARGE            AUX_EVCTL:EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               AUX_EVCTL:EVSTAT2.MCU_ACTIVE
// PWR_DWN                  AUX_EVCTL:EVSTAT2.PWR_DWN
// SCLK_LF                  AUX_EVCTL:EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      AUX_EVCTL:EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       AUX_EVCTL:EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             AUX_EVCTL:EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              AUX_EVCTL:EVSTAT2.AON_RTC_CH2
// MANUAL_EV                AUX_EVCTL:EVSTAT2.MANUAL_EV
// AUXIO31                  AUX_EVCTL:EVSTAT1.AUXIO31
// AUXIO30                  AUX_EVCTL:EVSTAT1.AUXIO30
// AUXIO29                  AUX_EVCTL:EVSTAT1.AUXIO29
// AUXIO28                  AUX_EVCTL:EVSTAT1.AUXIO28
// AUXIO27                  AUX_EVCTL:EVSTAT1.AUXIO27
// AUXIO26                  AUX_EVCTL:EVSTAT1.AUXIO26
// AUXIO25                  AUX_EVCTL:EVSTAT1.AUXIO25
// AUXIO24                  AUX_EVCTL:EVSTAT1.AUXIO24
// AUXIO23                  AUX_EVCTL:EVSTAT1.AUXIO23
// AUXIO22                  AUX_EVCTL:EVSTAT1.AUXIO22
// AUXIO21                  AUX_EVCTL:EVSTAT1.AUXIO21
// AUXIO20                  AUX_EVCTL:EVSTAT1.AUXIO20
// AUXIO19                  AUX_EVCTL:EVSTAT1.AUXIO19
// AUXIO18                  AUX_EVCTL:EVSTAT1.AUXIO18
// AUXIO17                  AUX_EVCTL:EVSTAT1.AUXIO17
// AUXIO16                  AUX_EVCTL:EVSTAT1.AUXIO16
// AUXIO15                  AUX_EVCTL:EVSTAT0.AUXIO15
// AUXIO14                  AUX_EVCTL:EVSTAT0.AUXIO14
// AUXIO13                  AUX_EVCTL:EVSTAT0.AUXIO13
// AUXIO12                  AUX_EVCTL:EVSTAT0.AUXIO12
// AUXIO11                  AUX_EVCTL:EVSTAT0.AUXIO11
// AUXIO10                  AUX_EVCTL:EVSTAT0.AUXIO10
// AUXIO9                   AUX_EVCTL:EVSTAT0.AUXIO9
// AUXIO8                   AUX_EVCTL:EVSTAT0.AUXIO8
// AUXIO7                   AUX_EVCTL:EVSTAT0.AUXIO7
// AUXIO6                   AUX_EVCTL:EVSTAT0.AUXIO6
// AUXIO5                   AUX_EVCTL:EVSTAT0.AUXIO5
// AUXIO4                   AUX_EVCTL:EVSTAT0.AUXIO4
// AUXIO3                   AUX_EVCTL:EVSTAT0.AUXIO3
// AUXIO2                   AUX_EVCTL:EVSTAT0.AUXIO2
// AUXIO1                   AUX_EVCTL:EVSTAT0.AUXIO1
// AUXIO0                   AUX_EVCTL:EVSTAT0.AUXIO0
#define AUX_TIMER01_T0CFG_TICK_SRC_W                                         6
#define AUX_TIMER01_T0CFG_TICK_SRC_M                                0x00003F00
#define AUX_TIMER01_T0CFG_TICK_SRC_S                                         8
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_TIMER2_CLKSW_RDY             0x00003F00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_DAC_HOLD_ACTIVE              0x00003E00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_SMPH_AUTOTAKE_DONE           0x00003D00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_ADC_FIFO_NOT_EMPTY           0x00003C00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_ADC_FIFO_ALMOST_FULL         0x00003B00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_ADC_IRQ                      0x00003A00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_ADC_DONE                     0x00003900
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_ISRC_RESET_N                 0x00003800
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_TDC_DONE                     0x00003700
#define AUX_TIMER01_T0CFG_TICK_SRC_NO_EVENT                         0x00003600
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_TIMER1_EV                    0x00003500
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_TIMER2_PULSE                 0x00003400
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_TIMER2_EV3                   0x00003300
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_TIMER2_EV2                   0x00003200
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_TIMER2_EV1                   0x00003100
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_TIMER2_EV0                   0x00003000
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_COMPB                        0x00002F00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUX_COMPA                        0x00002E00
#define AUX_TIMER01_T0CFG_TICK_SRC_MCU_OBSMUX1                      0x00002D00
#define AUX_TIMER01_T0CFG_TICK_SRC_MCU_OBSMUX0                      0x00002C00
#define AUX_TIMER01_T0CFG_TICK_SRC_MCU_EV                           0x00002B00
#define AUX_TIMER01_T0CFG_TICK_SRC_ACLK_REF                         0x00002A00
#define AUX_TIMER01_T0CFG_TICK_SRC_VDDR_RECHARGE                    0x00002900
#define AUX_TIMER01_T0CFG_TICK_SRC_MCU_ACTIVE                       0x00002800
#define AUX_TIMER01_T0CFG_TICK_SRC_PWR_DWN                          0x00002700
#define AUX_TIMER01_T0CFG_TICK_SRC_SCLK_LF                          0x00002600
#define AUX_TIMER01_T0CFG_TICK_SRC_AON_BATMON_TEMP_UPD              0x00002500
#define AUX_TIMER01_T0CFG_TICK_SRC_AON_BATMON_BAT_UPD               0x00002400
#define AUX_TIMER01_T0CFG_TICK_SRC_AON_RTC_4KHZ                     0x00002300
#define AUX_TIMER01_T0CFG_TICK_SRC_AON_RTC_CH2_DLY                  0x00002200
#define AUX_TIMER01_T0CFG_TICK_SRC_AON_RTC_CH2                      0x00002100
#define AUX_TIMER01_T0CFG_TICK_SRC_MANUAL_EV                        0x00002000
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO31                          0x00001F00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO30                          0x00001E00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO29                          0x00001D00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO28                          0x00001C00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO27                          0x00001B00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO26                          0x00001A00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO25                          0x00001900
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO24                          0x00001800
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO23                          0x00001700
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO22                          0x00001600
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO21                          0x00001500
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO20                          0x00001400
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO19                          0x00001300
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO18                          0x00001200
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO17                          0x00001100
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO16                          0x00001000
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO15                          0x00000F00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO14                          0x00000E00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO13                          0x00000D00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO12                          0x00000C00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO11                          0x00000B00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO10                          0x00000A00
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO9                           0x00000900
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO8                           0x00000800
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO7                           0x00000700
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO6                           0x00000600
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO5                           0x00000500
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO4                           0x00000400
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO3                           0x00000300
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO2                           0x00000200
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO1                           0x00000100
#define AUX_TIMER01_T0CFG_TICK_SRC_AUXIO0                           0x00000000

// Field:   [7:4] PRE
//
// Prescaler division ratio is 2^PRE:
//
// 0x0: Divide by 1.
// 0x1: Divide by 2.
// 0x2: Divide by 4.
// ...
// 0xF: Divide by 32,768.
#define AUX_TIMER01_T0CFG_PRE_W                                              4
#define AUX_TIMER01_T0CFG_PRE_M                                     0x000000F0
#define AUX_TIMER01_T0CFG_PRE_S                                              4

// Field:     [1] MODE
//
// Timer 0 mode.
//
// Configure source for Timer 0 prescaler.
// ENUMs:
// TICK                     Use event set by TICK_SRC as source for prescaler.
// CLK                      Use clock as source for prescaler. Note that
//                          AUX_SYSIF:PEROPRATE.TIMER01_OP_RATE sets the
//                          clock frequency.
#define AUX_TIMER01_T0CFG_MODE                                      0x00000002
#define AUX_TIMER01_T0CFG_MODE_BITN                                          1
#define AUX_TIMER01_T0CFG_MODE_M                                    0x00000002
#define AUX_TIMER01_T0CFG_MODE_S                                             1
#define AUX_TIMER01_T0CFG_MODE_TICK                                 0x00000002
#define AUX_TIMER01_T0CFG_MODE_CLK                                  0x00000000

// Field:     [0] RELOAD
//
// Timer 0 reload mode.
// ENUMs:
// CONT                     Continuous mode.
//
//                          Timer 0 restarts when the
//                          counter value becomes equal to or greater than
//                          ( T0TARGET.VALUE - 1).
// MAN                      Manual mode.
//
//                          Timer 0 stops and
//                          T0CTL.EN becomes 0 when the counter value
//                          becomes equal to or greater than
//                          T0TARGET.VALUE.
#define AUX_TIMER01_T0CFG_RELOAD                                    0x00000001
#define AUX_TIMER01_T0CFG_RELOAD_BITN                                        0
#define AUX_TIMER01_T0CFG_RELOAD_M                                  0x00000001
#define AUX_TIMER01_T0CFG_RELOAD_S                                           0
#define AUX_TIMER01_T0CFG_RELOAD_CONT                               0x00000001
#define AUX_TIMER01_T0CFG_RELOAD_MAN                                0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER01_O_T0CTL
//
//*****************************************************************************
// Field:     [0] EN
//
// Timer 0 enable.
//
// 0: Disable Timer 0.
// 1: Enable Timer 0.
//
// The counter restarts from 0 when you enable Timer 0.
#define AUX_TIMER01_T0CTL_EN                                        0x00000001
#define AUX_TIMER01_T0CTL_EN_BITN                                            0
#define AUX_TIMER01_T0CTL_EN_M                                      0x00000001
#define AUX_TIMER01_T0CTL_EN_S                                               0

//*****************************************************************************
//
// Register: AUX_TIMER01_O_T0TARGET
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Timer 0 target value.
//
// Manual Reload Mode:
// - Timer 0 increments until the counter value becomes equal to or greater
// than VALUE.
// - AUX_TIMER0_EV pulses high for 1 peripheral clock period when the counter
// value is equal to or greater than VALUE.
//
// Note: When VALUE is 0, Timer 0 counts to 1. AUX_TIMER0_EV pulses high for 1
// peripheral clock period.
//
// Continuous Reload Mode:
// - Timer 0 increments until the counter value becomes equal to or greater
// than ( VALUE - 1), then restarts from 0.
// - AUX_TIMER0_EV pulses high for 1 peripheral clock period when the counter
// value is 0, except for when you enable the timer.
//
// Note: When VALUE is less than 2, Timer 0 counter value remains 0.
// AUX_TIMER0_EV goes high and remains high 1 peripheral clock period after you
// enable the timer.
//
//
// It is allowed to update the VALUE while the timer runs.
#define AUX_TIMER01_T0TARGET_VALUE_W                                        16
#define AUX_TIMER01_T0TARGET_VALUE_M                                0x0000FFFF
#define AUX_TIMER01_T0TARGET_VALUE_S                                         0

//*****************************************************************************
//
// Register: AUX_TIMER01_O_T0CNTR
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Timer 0 counter value.
#define AUX_TIMER01_T0CNTR_VALUE_W                                          16
#define AUX_TIMER01_T0CNTR_VALUE_M                                  0x0000FFFF
#define AUX_TIMER01_T0CNTR_VALUE_S                                           0

//*****************************************************************************
//
// Register: AUX_TIMER01_O_T1CFG
//
//*****************************************************************************
// Field:    [14] TICK_SRC_POL
//
// Tick source polarity for Timer 1.
// ENUMs:
// FALL                     Count on falling edges of TICK_SRC.
// RISE                     Count on rising edges of TICK_SRC.
#define AUX_TIMER01_T1CFG_TICK_SRC_POL                              0x00004000
#define AUX_TIMER01_T1CFG_TICK_SRC_POL_BITN                                 14
#define AUX_TIMER01_T1CFG_TICK_SRC_POL_M                            0x00004000
#define AUX_TIMER01_T1CFG_TICK_SRC_POL_S                                    14
#define AUX_TIMER01_T1CFG_TICK_SRC_POL_FALL                         0x00004000
#define AUX_TIMER01_T1CFG_TICK_SRC_POL_RISE                         0x00000000

// Field:  [13:8] TICK_SRC
//
// Select Timer 1 tick source from the synchronous event bus.
// ENUMs:
// AUX_TIMER2_CLKSW_RDY     AUX_EVCTL:EVSTAT3.AUX_TIMER2_CLKSWITCH_RDY
// AUX_DAC_HOLD_ACTIVE      AUX_EVCTL:EVSTAT3.AUX_DAC_HOLD_ACTIVE
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER0_EV
// NO_EVENT                 No event.
// AUX_TIMER2_PULSE         AUX_EVCTL:EVSTAT3.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                AUX_EVCTL:EVSTAT2.AUX_COMPB
// AUX_COMPA                AUX_EVCTL:EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              AUX_EVCTL:EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              AUX_EVCTL:EVSTAT2.MCU_OBSMUX0
// MCU_EV                   AUX_EVCTL:EVSTAT2.MCU_EV
// ACLK_REF                 AUX_EVCTL:EVSTAT2.ACLK_REF
// VDDR_RECHARGE            AUX_EVCTL:EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               AUX_EVCTL:EVSTAT2.MCU_ACTIVE
// PWR_DWN                  AUX_EVCTL:EVSTAT2.PWR_DWN
// SCLK_LF                  AUX_EVCTL:EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      AUX_EVCTL:EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       AUX_EVCTL:EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             AUX_EVCTL:EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              AUX_EVCTL:EVSTAT2.AON_RTC_CH2
// MANUAL_EV                AUX_EVCTL:EVSTAT2.MANUAL_EV
// AUXIO31                  AUX_EVCTL:EVSTAT1.AUXIO31
// AUXIO30                  AUX_EVCTL:EVSTAT1.AUXIO30
// AUXIO29                  AUX_EVCTL:EVSTAT1.AUXIO29
// AUXIO28                  AUX_EVCTL:EVSTAT1.AUXIO28
// AUXIO27                  AUX_EVCTL:EVSTAT1.AUXIO27
// AUXIO26                  AUX_EVCTL:EVSTAT1.AUXIO26
// AUXIO25                  AUX_EVCTL:EVSTAT1.AUXIO25
// AUXIO24                  AUX_EVCTL:EVSTAT1.AUXIO24
// AUXIO23                  AUX_EVCTL:EVSTAT1.AUXIO23
// AUXIO22                  AUX_EVCTL:EVSTAT1.AUXIO22
// AUXIO21                  AUX_EVCTL:EVSTAT1.AUXIO21
// AUXIO20                  AUX_EVCTL:EVSTAT1.AUXIO20
// AUXIO19                  AUX_EVCTL:EVSTAT1.AUXIO19
// AUXIO18                  AUX_EVCTL:EVSTAT1.AUXIO18
// AUXIO17                  AUX_EVCTL:EVSTAT1.AUXIO17
// AUXIO16                  AUX_EVCTL:EVSTAT1.AUXIO16
// AUXIO15                  AUX_EVCTL:EVSTAT0.AUXIO15
// AUXIO14                  AUX_EVCTL:EVSTAT0.AUXIO14
// AUXIO13                  AUX_EVCTL:EVSTAT0.AUXIO13
// AUXIO12                  AUX_EVCTL:EVSTAT0.AUXIO12
// AUXIO11                  AUX_EVCTL:EVSTAT0.AUXIO11
// AUXIO10                  AUX_EVCTL:EVSTAT0.AUXIO10
// AUXIO9                   AUX_EVCTL:EVSTAT0.AUXIO9
// AUXIO8                   AUX_EVCTL:EVSTAT0.AUXIO8
// AUXIO7                   AUX_EVCTL:EVSTAT0.AUXIO7
// AUXIO6                   AUX_EVCTL:EVSTAT0.AUXIO6
// AUXIO5                   AUX_EVCTL:EVSTAT0.AUXIO5
// AUXIO4                   AUX_EVCTL:EVSTAT0.AUXIO4
// AUXIO3                   AUX_EVCTL:EVSTAT0.AUXIO3
// AUXIO2                   AUX_EVCTL:EVSTAT0.AUXIO2
// AUXIO1                   AUX_EVCTL:EVSTAT0.AUXIO1
// AUXIO0                   AUX_EVCTL:EVSTAT0.AUXIO0
#define AUX_TIMER01_T1CFG_TICK_SRC_W                                         6
#define AUX_TIMER01_T1CFG_TICK_SRC_M                                0x00003F00
#define AUX_TIMER01_T1CFG_TICK_SRC_S                                         8
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_TIMER2_CLKSW_RDY             0x00003F00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_DAC_HOLD_ACTIVE              0x00003E00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_SMPH_AUTOTAKE_DONE           0x00003D00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_ADC_FIFO_NOT_EMPTY           0x00003C00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_ADC_FIFO_ALMOST_FULL         0x00003B00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_ADC_IRQ                      0x00003A00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_ADC_DONE                     0x00003900
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_ISRC_RESET_N                 0x00003800
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_TDC_DONE                     0x00003700
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_TIMER0_EV                    0x00003600
#define AUX_TIMER01_T1CFG_TICK_SRC_NO_EVENT                         0x00003500
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_TIMER2_PULSE                 0x00003400
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_TIMER2_EV3                   0x00003300
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_TIMER2_EV2                   0x00003200
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_TIMER2_EV1                   0x00003100
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_TIMER2_EV0                   0x00003000
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_COMPB                        0x00002F00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUX_COMPA                        0x00002E00
#define AUX_TIMER01_T1CFG_TICK_SRC_MCU_OBSMUX1                      0x00002D00
#define AUX_TIMER01_T1CFG_TICK_SRC_MCU_OBSMUX0                      0x00002C00
#define AUX_TIMER01_T1CFG_TICK_SRC_MCU_EV                           0x00002B00
#define AUX_TIMER01_T1CFG_TICK_SRC_ACLK_REF                         0x00002A00
#define AUX_TIMER01_T1CFG_TICK_SRC_VDDR_RECHARGE                    0x00002900
#define AUX_TIMER01_T1CFG_TICK_SRC_MCU_ACTIVE                       0x00002800
#define AUX_TIMER01_T1CFG_TICK_SRC_PWR_DWN                          0x00002700
#define AUX_TIMER01_T1CFG_TICK_SRC_SCLK_LF                          0x00002600
#define AUX_TIMER01_T1CFG_TICK_SRC_AON_BATMON_TEMP_UPD              0x00002500
#define AUX_TIMER01_T1CFG_TICK_SRC_AON_BATMON_BAT_UPD               0x00002400
#define AUX_TIMER01_T1CFG_TICK_SRC_AON_RTC_4KHZ                     0x00002300
#define AUX_TIMER01_T1CFG_TICK_SRC_AON_RTC_CH2_DLY                  0x00002200
#define AUX_TIMER01_T1CFG_TICK_SRC_AON_RTC_CH2                      0x00002100
#define AUX_TIMER01_T1CFG_TICK_SRC_MANUAL_EV                        0x00002000
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO31                          0x00001F00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO30                          0x00001E00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO29                          0x00001D00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO28                          0x00001C00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO27                          0x00001B00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO26                          0x00001A00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO25                          0x00001900
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO24                          0x00001800
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO23                          0x00001700
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO22                          0x00001600
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO21                          0x00001500
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO20                          0x00001400
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO19                          0x00001300
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO18                          0x00001200
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO17                          0x00001100
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO16                          0x00001000
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO15                          0x00000F00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO14                          0x00000E00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO13                          0x00000D00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO12                          0x00000C00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO11                          0x00000B00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO10                          0x00000A00
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO9                           0x00000900
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO8                           0x00000800
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO7                           0x00000700
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO6                           0x00000600
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO5                           0x00000500
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO4                           0x00000400
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO3                           0x00000300
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO2                           0x00000200
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO1                           0x00000100
#define AUX_TIMER01_T1CFG_TICK_SRC_AUXIO0                           0x00000000

// Field:   [7:4] PRE
//
// Prescaler division ratio is 2^PRE:
//
// 0x0: Divide by 1.
// 0x1: Divide by 2.
// 0x2: Divide by 4.
// ...
// 0xF: Divide by 32,768.
#define AUX_TIMER01_T1CFG_PRE_W                                              4
#define AUX_TIMER01_T1CFG_PRE_M                                     0x000000F0
#define AUX_TIMER01_T1CFG_PRE_S                                              4

// Field:     [1] MODE
//
// Timer 1 mode.
//
// Configure source for Timer 1 prescaler.
// ENUMs:
// TICK                     Use event set by TICK_SRC as source for prescaler.
// CLK                      Use clock as source for prescaler. Note that
//                          AUX_SYSIF:PEROPRATE.TIMER01_OP_RATE sets the
//                          clock frequency.
#define AUX_TIMER01_T1CFG_MODE                                      0x00000002
#define AUX_TIMER01_T1CFG_MODE_BITN                                          1
#define AUX_TIMER01_T1CFG_MODE_M                                    0x00000002
#define AUX_TIMER01_T1CFG_MODE_S                                             1
#define AUX_TIMER01_T1CFG_MODE_TICK                                 0x00000002
#define AUX_TIMER01_T1CFG_MODE_CLK                                  0x00000000

// Field:     [0] RELOAD
//
// Timer 1 reload mode.
// ENUMs:
// CONT                     Continuous mode.
//
//                          Timer 1 restarts when the
//                          counter value becomes equal to or greater than
//                          ( T1TARGET.VALUE - 1).
// MAN                      Manual mode.
//
//                          Timer 1 stops and
//                          T1CTL.EN becomes 0 when the counter value
//                          becomes equal to or greater than
//                          T1TARGET.VALUE.
#define AUX_TIMER01_T1CFG_RELOAD                                    0x00000001
#define AUX_TIMER01_T1CFG_RELOAD_BITN                                        0
#define AUX_TIMER01_T1CFG_RELOAD_M                                  0x00000001
#define AUX_TIMER01_T1CFG_RELOAD_S                                           0
#define AUX_TIMER01_T1CFG_RELOAD_CONT                               0x00000001
#define AUX_TIMER01_T1CFG_RELOAD_MAN                                0x00000000

//*****************************************************************************
//
// Register: AUX_TIMER01_O_T1CTL
//
//*****************************************************************************
// Field:     [0] EN
//
// Timer 1 enable.
//
// 0: Disable Timer 1.
// 1: Enable Timer 1.
//
// The counter restarts from 0 when you enable Timer 1.
#define AUX_TIMER01_T1CTL_EN                                        0x00000001
#define AUX_TIMER01_T1CTL_EN_BITN                                            0
#define AUX_TIMER01_T1CTL_EN_M                                      0x00000001
#define AUX_TIMER01_T1CTL_EN_S                                               0

//*****************************************************************************
//
// Register: AUX_TIMER01_O_T1TARGET
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Timer 1 target value.
//
// Manual Reload Mode:
// - Timer 1 increments until the counter value becomes equal to or greater
// than VALUE.
// - AUX_TIMER1_EV pulses high for 1 peripheral clock period when the counter
// value is equal to or greater than VALUE.
//
// Note: When VALUE is 0, Timer 1 counts to 1. AUX_TIMER1_EV pulses high for 1
// peripheral clock period.
//
// Continuous Reload Mode:
// - Timer 1 increments until the counter value becomes equal to or greater
// than ( VALUE - 1), then restarts from 0.
// - AUX_TIMER1_EV pulses high for 1 peripheral clock period when the counter
// value is 0, except for when you enable the timer.
//
// Note: When VALUE is less than 2, Timer 1 counter value remains 0.
// AUX_TIMER1_EV goes high and remains high 1 peripheral clock period after you
// enable the timer.
//
//
// It is allowed to update the VALUE while the timer runs.
#define AUX_TIMER01_T1TARGET_VALUE_W                                        16
#define AUX_TIMER01_T1TARGET_VALUE_M                                0x0000FFFF
#define AUX_TIMER01_T1TARGET_VALUE_S                                         0

//*****************************************************************************
//
// Register: AUX_TIMER01_O_T1CNTR
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Timer 1 counter value.
#define AUX_TIMER01_T1CNTR_VALUE_W                                          16
#define AUX_TIMER01_T1CNTR_VALUE_M                                  0x0000FFFF
#define AUX_TIMER01_T1CNTR_VALUE_S                                           0


#endif // __AUX_TIMER01__
