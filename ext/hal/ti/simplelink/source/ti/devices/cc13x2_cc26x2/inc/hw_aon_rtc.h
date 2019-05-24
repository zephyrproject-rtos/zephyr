/******************************************************************************
*  Filename:       hw_aon_rtc_h
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

#ifndef __HW_AON_RTC_H__
#define __HW_AON_RTC_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AON_RTC component
//
//*****************************************************************************
// Control
#define AON_RTC_O_CTL                                               0x00000000

// Event Flags, RTC Status
#define AON_RTC_O_EVFLAGS                                           0x00000004

// Second Counter Value, Integer Part
#define AON_RTC_O_SEC                                               0x00000008

// Second Counter Value, Fractional Part
#define AON_RTC_O_SUBSEC                                            0x0000000C

// Subseconds Increment
#define AON_RTC_O_SUBSECINC                                         0x00000010

// Channel Configuration
#define AON_RTC_O_CHCTL                                             0x00000014

// Channel 0 Compare Value
#define AON_RTC_O_CH0CMP                                            0x00000018

// Channel 1 Compare Value
#define AON_RTC_O_CH1CMP                                            0x0000001C

// Channel 2 Compare Value
#define AON_RTC_O_CH2CMP                                            0x00000020

// Channel 2 Compare Value Auto-increment
#define AON_RTC_O_CH2CMPINC                                         0x00000024

// Channel 1 Capture Value
#define AON_RTC_O_CH1CAPT                                           0x00000028

// AON Synchronization
#define AON_RTC_O_SYNC                                              0x0000002C

// Current  Counter Value
#define AON_RTC_O_TIME                                              0x00000030

// Synchronization to SCLK_LF
#define AON_RTC_O_SYNCLF                                            0x00000034

//*****************************************************************************
//
// Register: AON_RTC_O_CTL
//
//*****************************************************************************
// Field: [18:16] COMB_EV_MASK
//
// Eventmask selecting which delayed events that form the combined event.
// ENUMs:
// CH2                      Use Channel 2 delayed event in combined event
// CH1                      Use Channel 1 delayed event in combined event
// CH0                      Use Channel 0 delayed event in combined event
// NONE                     No event is selected for combined event.
#define AON_RTC_CTL_COMB_EV_MASK_W                                           3
#define AON_RTC_CTL_COMB_EV_MASK_M                                  0x00070000
#define AON_RTC_CTL_COMB_EV_MASK_S                                          16
#define AON_RTC_CTL_COMB_EV_MASK_CH2                                0x00040000
#define AON_RTC_CTL_COMB_EV_MASK_CH1                                0x00020000
#define AON_RTC_CTL_COMB_EV_MASK_CH0                                0x00010000
#define AON_RTC_CTL_COMB_EV_MASK_NONE                               0x00000000

// Field:  [11:8] EV_DELAY
//
// Number of SCLK_LF clock cycles waited before generating delayed events.
// (Common setting for all RTC cannels)  the delayed event is delayed
// ENUMs:
// D144                     Delay by 144 clock cycles
// D128                     Delay by 128 clock cycles
// D112                     Delay by 112 clock cycles
// D96                      Delay by 96 clock cycles
// D80                      Delay by 80 clock cycles
// D64                      Delay by 64 clock cycles
// D48                      Delay by 48 clock cycles
// D32                      Delay by 32 clock cycles
// D16                      Delay by 16 clock cycles
// D8                       Delay by 8 clock cycles
// D4                       Delay by 4 clock cycles
// D2                       Delay by 2 clock cycles
// D1                       Delay by 1 clock cycles
// D0                       No delay on delayed event
#define AON_RTC_CTL_EV_DELAY_W                                               4
#define AON_RTC_CTL_EV_DELAY_M                                      0x00000F00
#define AON_RTC_CTL_EV_DELAY_S                                               8
#define AON_RTC_CTL_EV_DELAY_D144                                   0x00000D00
#define AON_RTC_CTL_EV_DELAY_D128                                   0x00000C00
#define AON_RTC_CTL_EV_DELAY_D112                                   0x00000B00
#define AON_RTC_CTL_EV_DELAY_D96                                    0x00000A00
#define AON_RTC_CTL_EV_DELAY_D80                                    0x00000900
#define AON_RTC_CTL_EV_DELAY_D64                                    0x00000800
#define AON_RTC_CTL_EV_DELAY_D48                                    0x00000700
#define AON_RTC_CTL_EV_DELAY_D32                                    0x00000600
#define AON_RTC_CTL_EV_DELAY_D16                                    0x00000500
#define AON_RTC_CTL_EV_DELAY_D8                                     0x00000400
#define AON_RTC_CTL_EV_DELAY_D4                                     0x00000300
#define AON_RTC_CTL_EV_DELAY_D2                                     0x00000200
#define AON_RTC_CTL_EV_DELAY_D1                                     0x00000100
#define AON_RTC_CTL_EV_DELAY_D0                                     0x00000000

// Field:     [7] RESET
//
// RTC Counter reset.
//
// Writing 1 to this bit will reset the RTC counter.
//
// This bit is cleared when reset takes effect
#define AON_RTC_CTL_RESET                                           0x00000080
#define AON_RTC_CTL_RESET_BITN                                               7
#define AON_RTC_CTL_RESET_M                                         0x00000080
#define AON_RTC_CTL_RESET_S                                                  7

// Field:     [2] RTC_4KHZ_EN
//
// RTC_4KHZ is a 4 KHz reference output, tapped from  SUBSEC.VALUE  bit 19
// which is used by AUX timer.
//
// 0: RTC_4KHZ signal is forced to 0
// 1: RTC_4KHZ is enabled ( provied that RTC is enabled EN)
#define AON_RTC_CTL_RTC_4KHZ_EN                                     0x00000004
#define AON_RTC_CTL_RTC_4KHZ_EN_BITN                                         2
#define AON_RTC_CTL_RTC_4KHZ_EN_M                                   0x00000004
#define AON_RTC_CTL_RTC_4KHZ_EN_S                                            2

// Field:     [1] RTC_UPD_EN
//
// RTC_UPD is a 16 KHz signal used to sync up the radio timer. The 16 Khz is
// SCLK_LF divided by 2
//
// 0: RTC_UPD signal is forced to 0
// 1: RTC_UPD signal is toggling @16 kHz
#define AON_RTC_CTL_RTC_UPD_EN                                      0x00000002
#define AON_RTC_CTL_RTC_UPD_EN_BITN                                          1
#define AON_RTC_CTL_RTC_UPD_EN_M                                    0x00000002
#define AON_RTC_CTL_RTC_UPD_EN_S                                             1

// Field:     [0] EN
//
// Enable RTC counter
//
// 0: Halted (frozen)
// 1: Running
#define AON_RTC_CTL_EN                                              0x00000001
#define AON_RTC_CTL_EN_BITN                                                  0
#define AON_RTC_CTL_EN_M                                            0x00000001
#define AON_RTC_CTL_EN_S                                                     0

//*****************************************************************************
//
// Register: AON_RTC_O_EVFLAGS
//
//*****************************************************************************
// Field:    [16] CH2
//
// Channel 2 event flag, set when CHCTL.CH2_EN = 1 and the RTC value matches or
// passes the CH2CMP value.
//
// An event will be scheduled to occur as soon as possible when writing to
// CH2CMP provided that the channel is enabled and the new value matches any
// time between next  RTC value and 1 second in the past
//
// Writing 1 clears this flag.
//
// AUX_SCE can read the flag through AUX_EVCTL:EVSTAT2.AON_RTC_CH2 and clear it
// using AUX_SYSIF:RTCEVCLR.RTC_CH2_EV_CLR.
#define AON_RTC_EVFLAGS_CH2                                         0x00010000
#define AON_RTC_EVFLAGS_CH2_BITN                                            16
#define AON_RTC_EVFLAGS_CH2_M                                       0x00010000
#define AON_RTC_EVFLAGS_CH2_S                                               16

// Field:     [8] CH1
//
// Channel 1 event flag, set when CHCTL.CH1_EN = 1 and one of the following:
// - CHCTL.CH1_CAPT_EN = 0 and the RTC value matches or passes the CH1CMP
// value.
// - CHCTL.CH1_CAPT_EN = 1 and capture occurs.
//
// An event will be scheduled to occur as soon as possible when writing to
// CH1CMP provided that the channel is enabled, in compare mode and the new
// value matches any time between next  RTC value and 1 second in the past.
//
// Writing 1 clears this flag.
#define AON_RTC_EVFLAGS_CH1                                         0x00000100
#define AON_RTC_EVFLAGS_CH1_BITN                                             8
#define AON_RTC_EVFLAGS_CH1_M                                       0x00000100
#define AON_RTC_EVFLAGS_CH1_S                                                8

// Field:     [0] CH0
//
// Channel 0 event flag, set when CHCTL.CH0_EN = 1 and the RTC value matches or
// passes the CH0CMP value.
//
// An event will be scheduled to occur as soon as possible when writing to
// CH0CMP provided that the channels is enabled and the new value matches any
// time between next  RTC value and 1 second in the past.
//
// Writing 1 clears this flag.
#define AON_RTC_EVFLAGS_CH0                                         0x00000001
#define AON_RTC_EVFLAGS_CH0_BITN                                             0
#define AON_RTC_EVFLAGS_CH0_M                                       0x00000001
#define AON_RTC_EVFLAGS_CH0_S                                                0

//*****************************************************************************
//
// Register: AON_RTC_O_SEC
//
//*****************************************************************************
// Field:  [31:0] VALUE
//
// Unsigned integer representing Real Time Clock in seconds.
//
// When reading this register the content of SUBSEC.VALUE is simultaneously
// latched. A consistent reading of the combined Real Time Clock can be
// obtained by first reading this register, then reading SUBSEC register.
#define AON_RTC_SEC_VALUE_W                                                 32
#define AON_RTC_SEC_VALUE_M                                         0xFFFFFFFF
#define AON_RTC_SEC_VALUE_S                                                  0

//*****************************************************************************
//
// Register: AON_RTC_O_SUBSEC
//
//*****************************************************************************
// Field:  [31:0] VALUE
//
// Unsigned integer representing Real Time Clock in fractions of a second
// (VALUE/2^32 seconds) at the time when SEC register was read.
//
// Examples :
// - 0x0000_0000 = 0.0 sec
// - 0x4000_0000 = 0.25 sec
// - 0x8000_0000 = 0.5 sec
// - 0xC000_0000 = 0.75 sec
#define AON_RTC_SUBSEC_VALUE_W                                              32
#define AON_RTC_SUBSEC_VALUE_M                                      0xFFFFFFFF
#define AON_RTC_SUBSEC_VALUE_S                                               0

//*****************************************************************************
//
// Register: AON_RTC_O_SUBSECINC
//
//*****************************************************************************
// Field:  [23:0] VALUEINC
//
// This value compensates for a SCLK_LF clock which has an offset from 32768
// Hz.
//
// The compensation value can be found as 2^38 / freq, where freq is SCLK_LF
// clock frequency in Hertz
//
// This value is added to SUBSEC.VALUE on every cycle, and carry of this is
// added to SEC.VALUE. To perform the addition, bits [23:6] are aligned with
// SUBSEC.VALUE bits [17:0]. The remaining bits [5:0] are accumulated in a
// hidden 6-bit register that generates a carry into the above mentioned
// addition on overflow.
// The default value corresponds to incrementing by precisely 1/32768 of a
// second.
//
// NOTE: This register is read only. Modification of the register value must be
// done using registers AUX_SYSIF:RTCSUBSECINC0 ,  AUX_SYSIF:RTCSUBSECINC1 and
// AUX_SYSIF:RTCSUBSECINCCTL
#define AON_RTC_SUBSECINC_VALUEINC_W                                        24
#define AON_RTC_SUBSECINC_VALUEINC_M                                0x00FFFFFF
#define AON_RTC_SUBSECINC_VALUEINC_S                                         0

//*****************************************************************************
//
// Register: AON_RTC_O_CHCTL
//
//*****************************************************************************
// Field:    [18] CH2_CONT_EN
//
// Set to enable continuous operation of Channel 2
#define AON_RTC_CHCTL_CH2_CONT_EN                                   0x00040000
#define AON_RTC_CHCTL_CH2_CONT_EN_BITN                                      18
#define AON_RTC_CHCTL_CH2_CONT_EN_M                                 0x00040000
#define AON_RTC_CHCTL_CH2_CONT_EN_S                                         18

// Field:    [16] CH2_EN
//
// RTC Channel 2 Enable
//
// 0: Disable RTC Channel 2
// 1: Enable RTC Channel 2
#define AON_RTC_CHCTL_CH2_EN                                        0x00010000
#define AON_RTC_CHCTL_CH2_EN_BITN                                           16
#define AON_RTC_CHCTL_CH2_EN_M                                      0x00010000
#define AON_RTC_CHCTL_CH2_EN_S                                              16

// Field:     [9] CH1_CAPT_EN
//
// Set Channel 1 mode
//
// 0: Compare mode (default)
// 1: Capture mode
#define AON_RTC_CHCTL_CH1_CAPT_EN                                   0x00000200
#define AON_RTC_CHCTL_CH1_CAPT_EN_BITN                                       9
#define AON_RTC_CHCTL_CH1_CAPT_EN_M                                 0x00000200
#define AON_RTC_CHCTL_CH1_CAPT_EN_S                                          9

// Field:     [8] CH1_EN
//
// RTC Channel 1 Enable
//
// 0: Disable RTC Channel 1
// 1: Enable RTC Channel 1
#define AON_RTC_CHCTL_CH1_EN                                        0x00000100
#define AON_RTC_CHCTL_CH1_EN_BITN                                            8
#define AON_RTC_CHCTL_CH1_EN_M                                      0x00000100
#define AON_RTC_CHCTL_CH1_EN_S                                               8

// Field:     [0] CH0_EN
//
// RTC Channel 0 Enable
//
// 0: Disable RTC Channel 0
// 1: Enable RTC Channel 0
#define AON_RTC_CHCTL_CH0_EN                                        0x00000001
#define AON_RTC_CHCTL_CH0_EN_BITN                                            0
#define AON_RTC_CHCTL_CH0_EN_M                                      0x00000001
#define AON_RTC_CHCTL_CH0_EN_S                                               0

//*****************************************************************************
//
// Register: AON_RTC_O_CH0CMP
//
//*****************************************************************************
// Field:  [31:0] VALUE
//
// RTC Channel 0 compare value.
//
// Bit 31 to 16 represents seconds and bits 15 to 0 represents subseconds of
// the compare value.
//
// The compare value is compared against SEC.VALUE (15:0) and SUBSEC.VALUE
// (31:16) values of the Real Time Clock register. A Cannel 0 event is
// generated when {SEC.VALUE(15:0),SUBSEC.VALUE (31:16)} is reaching or
// exciting the compare value.
//
// Writing to this register can trigger an immediate*) event in case the new
// compare value matches a Real Time Clock  value from 1 second in the past up
// till current Real Time Clock value.
//
// Example:
// To generate a compare  5.5 seconds RTC start,- set this value = 0x0005_8000
//
// *) It can take up to one SCLK_LF clock cycles before event occurs due to
// synchronization.
#define AON_RTC_CH0CMP_VALUE_W                                              32
#define AON_RTC_CH0CMP_VALUE_M                                      0xFFFFFFFF
#define AON_RTC_CH0CMP_VALUE_S                                               0

//*****************************************************************************
//
// Register: AON_RTC_O_CH1CMP
//
//*****************************************************************************
// Field:  [31:0] VALUE
//
// RTC Channel 1 compare value.
//
// Bit 31 to 16 represents seconds and bits 15 to 0 represents subseconds of
// the compare value.
//
// The compare value is compared against SEC.VALUE (15:0) and SUBSEC.VALUE
// (31:16) values of the Real Time Clock register. A Cannel 0 event is
// generated when {SEC.VALUE(15:0),SUBSEC.VALUE (31:16)} is reaching or
// exciting the compare value.
//
// Writing to this register can trigger an immediate*) event in case the new
// compare value matches a Real Time Clock  value from 1 second in the past up
// till current Real Time Clock value.
//
// Example:
// To generate a compare  5.5 seconds RTC start,- set this value = 0x0005_8000
//
// *) It can take up to one SCLK_LF clock cycles before event occurs due to
// synchronization.
#define AON_RTC_CH1CMP_VALUE_W                                              32
#define AON_RTC_CH1CMP_VALUE_M                                      0xFFFFFFFF
#define AON_RTC_CH1CMP_VALUE_S                                               0

//*****************************************************************************
//
// Register: AON_RTC_O_CH2CMP
//
//*****************************************************************************
// Field:  [31:0] VALUE
//
// RTC Channel 2 compare value.
//
// Bit 31 to 16 represents seconds and bits 15 to 0 represents subseconds of
// the compare value.
//
// The compare value is compared against SEC.VALUE (15:0) and SUBSEC.VALUE
// (31:16) values of the Real Time Clock register. A Cannel 0 event is
// generated when {SEC.VALUE(15:0),SUBSEC.VALUE (31:16)} is reaching or
// exciting the compare value.
//
// Writing to this register can trigger an immediate*) event in case the new
// compare value matches a Real Time Clock  value from 1 second in the past up
// till current Real Time Clock value.
//
// Example:
// To generate a compare  5.5 seconds RTC start,- set this value = 0x0005_8000
//
// *) It can take up to one SCLK_LF clock cycles before event occurs due to
// synchronization.
#define AON_RTC_CH2CMP_VALUE_W                                              32
#define AON_RTC_CH2CMP_VALUE_M                                      0xFFFFFFFF
#define AON_RTC_CH2CMP_VALUE_S                                               0

//*****************************************************************************
//
// Register: AON_RTC_O_CH2CMPINC
//
//*****************************************************************************
// Field:  [31:0] VALUE
//
// If CHCTL.CH2_CONT_EN is set, this value is added to CH2CMP.VALUE on every
// channel 2 compare event.
#define AON_RTC_CH2CMPINC_VALUE_W                                           32
#define AON_RTC_CH2CMPINC_VALUE_M                                   0xFFFFFFFF
#define AON_RTC_CH2CMPINC_VALUE_S                                            0

//*****************************************************************************
//
// Register: AON_RTC_O_CH1CAPT
//
//*****************************************************************************
// Field: [31:16] SEC
//
// Value of SEC.VALUE bits 15:0 at capture time.
#define AON_RTC_CH1CAPT_SEC_W                                               16
#define AON_RTC_CH1CAPT_SEC_M                                       0xFFFF0000
#define AON_RTC_CH1CAPT_SEC_S                                               16

// Field:  [15:0] SUBSEC
//
// Value of SUBSEC.VALUE bits 31:16 at capture time.
#define AON_RTC_CH1CAPT_SUBSEC_W                                            16
#define AON_RTC_CH1CAPT_SUBSEC_M                                    0x0000FFFF
#define AON_RTC_CH1CAPT_SUBSEC_S                                             0

//*****************************************************************************
//
// Register: AON_RTC_O_SYNC
//
//*****************************************************************************
// Field:     [0] WBUSY
//
// This register will always return 0,- however it will not return the value
// until there are no outstanding write requests between MCU and AON
//
// Note: Writing to this register prior to reading will force a wait until next
// SCLK_MF  edge. This is recommended for syncing read registers from AON when
// waking up from sleep
// Failure to do so may result in reading AON values from prior to going to
// sleep
#define AON_RTC_SYNC_WBUSY                                          0x00000001
#define AON_RTC_SYNC_WBUSY_BITN                                              0
#define AON_RTC_SYNC_WBUSY_M                                        0x00000001
#define AON_RTC_SYNC_WBUSY_S                                                 0

//*****************************************************************************
//
// Register: AON_RTC_O_TIME
//
//*****************************************************************************
// Field: [31:16] SEC_L
//
// Returns the lower halfword of SEC register.
#define AON_RTC_TIME_SEC_L_W                                                16
#define AON_RTC_TIME_SEC_L_M                                        0xFFFF0000
#define AON_RTC_TIME_SEC_L_S                                                16

// Field:  [15:0] SUBSEC_H
//
// Returns the upper halfword of SUBSEC register.
#define AON_RTC_TIME_SUBSEC_H_W                                             16
#define AON_RTC_TIME_SUBSEC_H_M                                     0x0000FFFF
#define AON_RTC_TIME_SUBSEC_H_S                                              0

//*****************************************************************************
//
// Register: AON_RTC_O_SYNCLF
//
//*****************************************************************************
// Field:     [0] PHASE
//
// This bit will always return the SCLK_LF phase. The return will delayed until
// a positive or negative edge of SCLK_LF is seen.
// 0: Falling edge of SCLK_LF
// 1: Rising edge of SCLK_LF
#define AON_RTC_SYNCLF_PHASE                                        0x00000001
#define AON_RTC_SYNCLF_PHASE_BITN                                            0
#define AON_RTC_SYNCLF_PHASE_M                                      0x00000001
#define AON_RTC_SYNCLF_PHASE_S                                               0


#endif // __AON_RTC__
