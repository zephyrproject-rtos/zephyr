/******************************************************************************
*  Filename:       hw_aux_tdc_h
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

#ifndef __HW_AUX_TDC_H__
#define __HW_AUX_TDC_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_TDC component
//
//*****************************************************************************
// Control
#define AUX_TDC_O_CTL                                               0x00000000

// Status
#define AUX_TDC_O_STAT                                              0x00000004

// Result
#define AUX_TDC_O_RESULT                                            0x00000008

// Saturation Configuration
#define AUX_TDC_O_SATCFG                                            0x0000000C

// Trigger Source
#define AUX_TDC_O_TRIGSRC                                           0x00000010

// Trigger Counter
#define AUX_TDC_O_TRIGCNT                                           0x00000014

// Trigger Counter Load
#define AUX_TDC_O_TRIGCNTLOAD                                       0x00000018

// Trigger Counter Configuration
#define AUX_TDC_O_TRIGCNTCFG                                        0x0000001C

// Prescaler Control
#define AUX_TDC_O_PRECTL                                            0x00000020

// Prescaler Counter
#define AUX_TDC_O_PRECNTR                                           0x00000024

//*****************************************************************************
//
// Register: AUX_TDC_O_CTL
//
//*****************************************************************************
// Field:   [1:0] CMD
//
// TDC commands.
// ENUMs:
// ABORT                    Force TDC state machine back to IDLE state.
//
//                          Never write this command
//                          while AUX_TDC:STAT.STATE equals CLR_CNT or
//                          WAIT_CLR_CNT_DONE.
// RUN                      Asynchronous counter start.
//
//                          The counter starts to
//                          count when the start event is high. To achieve
//                          precise edge-to-edge measurements you must
//                          ensure that the start event is low for at least
//                          420 ns after you write this command.
// RUN_SYNC_START           Synchronous counter start.
//
//                          The counter looks for the
//                          opposite edge of the selected start event
//                          before it starts to count when the selected
//                          edge occurs. This guarantees an edge-triggered
//                          start and is recommended for frequency
//                          measurements.
// CLR_RESULT               Clear STAT.SAT, STAT.DONE, and RESULT.VALUE.
//
//                          This is not needed as
//                          prerequisite for a measurement. Reliable clear
//                          is only guaranteed from IDLE state.
#define AUX_TDC_CTL_CMD_W                                                    2
#define AUX_TDC_CTL_CMD_M                                           0x00000003
#define AUX_TDC_CTL_CMD_S                                                    0
#define AUX_TDC_CTL_CMD_ABORT                                       0x00000003
#define AUX_TDC_CTL_CMD_RUN                                         0x00000002
#define AUX_TDC_CTL_CMD_RUN_SYNC_START                              0x00000001
#define AUX_TDC_CTL_CMD_CLR_RESULT                                  0x00000000

//*****************************************************************************
//
// Register: AUX_TDC_O_STAT
//
//*****************************************************************************
// Field:     [7] SAT
//
// TDC measurement saturation flag.
//
// 0: Conversion has not saturated.
// 1: Conversion stopped due to saturation.
//
// This field is cleared when a new measurement is started or when CLR_RESULT
// is written to CTL.CMD.
#define AUX_TDC_STAT_SAT                                            0x00000080
#define AUX_TDC_STAT_SAT_BITN                                                7
#define AUX_TDC_STAT_SAT_M                                          0x00000080
#define AUX_TDC_STAT_SAT_S                                                   7

// Field:     [6] DONE
//
// TDC measurement complete flag.
//
// 0: TDC measurement has not yet completed.
// 1: TDC measurement has completed.
//
// This field clears when a new TDC measurement starts or when you write
// CLR_RESULT to CTL.CMD.
#define AUX_TDC_STAT_DONE                                           0x00000040
#define AUX_TDC_STAT_DONE_BITN                                               6
#define AUX_TDC_STAT_DONE_M                                         0x00000040
#define AUX_TDC_STAT_DONE_S                                                  6

// Field:   [5:0] STATE
//
// TDC state machine status.
// ENUMs:
// FORCE_STOP               Current state is TDC_FORCESTOP.
//                          You wrote ABORT to
//                          CTL.CMD to abort the TDC measurement.
// START_FALL               Current state is TDC_WAIT_STARTFALL.
//                          The fast-counter circuit
//                          waits for a falling edge on the start event.
// WAIT_CLR_CNT_DONE        Current state is TDC_STATE_WAIT_CLRCNT_DONE.
//                          The state machine waits
//                          for fast-counter circuit to finish reset.
// POR                      Current state is TDC_STATE_POR.
//                          This is the reset state.
// GET_RESULT               Current state is TDC_STATE_GETRESULTS.
//                          The state machine copies
//                          the counter value from the fast-counter
//                          circuit.
// WAIT_STOP_CNTDWN         Current state is TDC_STATE_WAIT_STOPCNTDOWN.
//                          The fast-counter circuit
//                          looks for the stop condition. It will ignore a
//                          number of stop events configured in
//                          TRIGCNTLOAD.CNT.
// WAIT_STOP                Current state is TDC_STATE_WAIT_STOP.
//                          The state machine waits
//                          for the fast-counter circuit to stop.
// CLR_CNT                  Current state is TDC_STATE_CLRCNT. The
//                          fast-counter circuit is reset.
// IDLE                     Current state is TDC_STATE_IDLE.
//                          This is the default state
//                          after reset and abortion. State will change
//                          when you write CTL.CMD to either RUN_SYNC_START
//                          or RUN.
// WAIT_START_STOP_CNT_EN   Current state is TDC_STATE_WAIT_STARTSTOPCNTEN.
//                          The fast-counter circuit
//                          looks for the start condition. The state
//                          machine waits for the fast-counter to
//                          increment.
// WAIT_START               Current state is TDC_STATE_WAIT_START.
//                          The fast-counter circuit
//                          looks for the start condition. The state
//                          machine waits for the fast-counter to
//                          increment.
#define AUX_TDC_STAT_STATE_W                                                 6
#define AUX_TDC_STAT_STATE_M                                        0x0000003F
#define AUX_TDC_STAT_STATE_S                                                 0
#define AUX_TDC_STAT_STATE_FORCE_STOP                               0x0000002E
#define AUX_TDC_STAT_STATE_START_FALL                               0x0000001E
#define AUX_TDC_STAT_STATE_WAIT_CLR_CNT_DONE                        0x00000016
#define AUX_TDC_STAT_STATE_POR                                      0x0000000F
#define AUX_TDC_STAT_STATE_GET_RESULT                               0x0000000E
#define AUX_TDC_STAT_STATE_WAIT_STOP_CNTDWN                         0x0000000C
#define AUX_TDC_STAT_STATE_WAIT_STOP                                0x00000008
#define AUX_TDC_STAT_STATE_CLR_CNT                                  0x00000007
#define AUX_TDC_STAT_STATE_IDLE                                     0x00000006
#define AUX_TDC_STAT_STATE_WAIT_START_STOP_CNT_EN                   0x00000004
#define AUX_TDC_STAT_STATE_WAIT_START                               0x00000000

//*****************************************************************************
//
// Register: AUX_TDC_O_RESULT
//
//*****************************************************************************
// Field:  [24:0] VALUE
//
// TDC conversion result.
//
// The result of the TDC conversion is given in number of clock edges of the
// clock source selected in DDI_0_OSC:CTL0.ACLK_TDC_SRC_SEL. Both rising and
// falling edges are counted.
//
// If TDC counter saturates, VALUE is slightly higher than SATCFG.LIMIT, as it
// takes a non-zero time to stop the measurement. Hence, the maximum value of
// this field becomes slightly higher than 2^24 if you configure SATCFG.LIMIT
// to R24.
#define AUX_TDC_RESULT_VALUE_W                                              25
#define AUX_TDC_RESULT_VALUE_M                                      0x01FFFFFF
#define AUX_TDC_RESULT_VALUE_S                                               0

//*****************************************************************************
//
// Register: AUX_TDC_O_SATCFG
//
//*****************************************************************************
// Field:   [3:0] LIMIT
//
// Saturation limit.
//
// The flag STAT.SAT is set when the TDC counter saturates.
//
// Values not enumerated are not supported
// ENUMs:
// R24                      Result bit 24: TDC conversion saturates and stops
//                          when RESULT.VALUE[24] is set.
// R23                      Result bit 23: TDC conversion saturates and stops
//                          when RESULT.VALUE[23] is set.
// R22                      Result bit 22: TDC conversion saturates and stops
//                          when RESULT.VALUE[22] is set.
// R21                      Result bit 21: TDC conversion saturates and stops
//                          when RESULT.VALUE[21] is set.
// R20                      Result bit 20: TDC conversion saturates and stops
//                          when RESULT.VALUE[20] is set.
// R19                      Result bit 19: TDC conversion saturates and stops
//                          when RESULT.VALUE[19] is set.
// R18                      Result bit 18: TDC conversion saturates and stops
//                          when RESULT.VALUE[18] is set.
// R17                      Result bit 17: TDC conversion saturates and stops
//                          when RESULT.VALUE[17] is set.
// R16                      Result bit 16: TDC conversion saturates and stops
//                          when RESULT.VALUE[16] is set.
// R15                      Result bit 15: TDC conversion saturates and stops
//                          when RESULT.VALUE[15] is set.
// R14                      Result bit 14: TDC conversion saturates and stops
//                          when RESULT.VALUE[14] is set.
// R13                      Result bit 13: TDC conversion saturates and stops
//                          when RESULT.VALUE[13] is set.
// R12                      Result bit 12: TDC conversion saturates and stops
//                          when RESULT.VALUE[12] is set.
#define AUX_TDC_SATCFG_LIMIT_W                                               4
#define AUX_TDC_SATCFG_LIMIT_M                                      0x0000000F
#define AUX_TDC_SATCFG_LIMIT_S                                               0
#define AUX_TDC_SATCFG_LIMIT_R24                                    0x0000000F
#define AUX_TDC_SATCFG_LIMIT_R23                                    0x0000000E
#define AUX_TDC_SATCFG_LIMIT_R22                                    0x0000000D
#define AUX_TDC_SATCFG_LIMIT_R21                                    0x0000000C
#define AUX_TDC_SATCFG_LIMIT_R20                                    0x0000000B
#define AUX_TDC_SATCFG_LIMIT_R19                                    0x0000000A
#define AUX_TDC_SATCFG_LIMIT_R18                                    0x00000009
#define AUX_TDC_SATCFG_LIMIT_R17                                    0x00000008
#define AUX_TDC_SATCFG_LIMIT_R16                                    0x00000007
#define AUX_TDC_SATCFG_LIMIT_R15                                    0x00000006
#define AUX_TDC_SATCFG_LIMIT_R14                                    0x00000005
#define AUX_TDC_SATCFG_LIMIT_R13                                    0x00000004
#define AUX_TDC_SATCFG_LIMIT_R12                                    0x00000003

//*****************************************************************************
//
// Register: AUX_TDC_O_TRIGSRC
//
//*****************************************************************************
// Field:    [14] STOP_POL
//
// Polarity of stop source.
//
// Change only while STAT.STATE is IDLE.
// ENUMs:
// LOW                      TDC conversion stops when low level is detected.
// HIGH                     TDC conversion stops when high level is detected.
#define AUX_TDC_TRIGSRC_STOP_POL                                    0x00004000
#define AUX_TDC_TRIGSRC_STOP_POL_BITN                                       14
#define AUX_TDC_TRIGSRC_STOP_POL_M                                  0x00004000
#define AUX_TDC_TRIGSRC_STOP_POL_S                                          14
#define AUX_TDC_TRIGSRC_STOP_POL_LOW                                0x00004000
#define AUX_TDC_TRIGSRC_STOP_POL_HIGH                               0x00000000

// Field:  [13:8] STOP_SRC
//
// Select stop source from the asynchronous AUX event bus.
//
// Change only while STAT.STATE is IDLE.
// ENUMs:
// NO_EVENT                 No event.
// AUX_TDC_PRE              Select TDC Prescaler event which is generated by
//                          configuration of PRECTL.
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER0_EV
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
#define AUX_TDC_TRIGSRC_STOP_SRC_W                                           6
#define AUX_TDC_TRIGSRC_STOP_SRC_M                                  0x00003F00
#define AUX_TDC_TRIGSRC_STOP_SRC_S                                           8
#define AUX_TDC_TRIGSRC_STOP_SRC_NO_EVENT                           0x00003F00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TDC_PRE                        0x00003E00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_SMPH_AUTOTAKE_DONE             0x00003D00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_ADC_FIFO_NOT_EMPTY             0x00003C00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_ADC_FIFO_ALMOST_FULL           0x00003B00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_ADC_IRQ                        0x00003A00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_ADC_DONE                       0x00003900
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_ISRC_RESET_N                   0x00003800
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TDC_DONE                       0x00003700
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER0_EV                      0x00003600
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER1_EV                      0x00003500
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_PULSE                   0x00003400
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_EV3                     0x00003300
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_EV2                     0x00003200
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_EV1                     0x00003100
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_TIMER2_EV0                     0x00003000
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_COMPB                          0x00002F00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUX_COMPA                          0x00002E00
#define AUX_TDC_TRIGSRC_STOP_SRC_MCU_OBSMUX1                        0x00002D00
#define AUX_TDC_TRIGSRC_STOP_SRC_MCU_OBSMUX0                        0x00002C00
#define AUX_TDC_TRIGSRC_STOP_SRC_MCU_EV                             0x00002B00
#define AUX_TDC_TRIGSRC_STOP_SRC_ACLK_REF                           0x00002A00
#define AUX_TDC_TRIGSRC_STOP_SRC_VDDR_RECHARGE                      0x00002900
#define AUX_TDC_TRIGSRC_STOP_SRC_MCU_ACTIVE                         0x00002800
#define AUX_TDC_TRIGSRC_STOP_SRC_PWR_DWN                            0x00002700
#define AUX_TDC_TRIGSRC_STOP_SRC_SCLK_LF                            0x00002600
#define AUX_TDC_TRIGSRC_STOP_SRC_AON_BATMON_TEMP_UPD                0x00002500
#define AUX_TDC_TRIGSRC_STOP_SRC_AON_BATMON_BAT_UPD                 0x00002400
#define AUX_TDC_TRIGSRC_STOP_SRC_AON_RTC_4KHZ                       0x00002300
#define AUX_TDC_TRIGSRC_STOP_SRC_AON_RTC_CH2_DLY                    0x00002200
#define AUX_TDC_TRIGSRC_STOP_SRC_AON_RTC_CH2                        0x00002100
#define AUX_TDC_TRIGSRC_STOP_SRC_MANUAL_EV                          0x00002000
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO31                            0x00001F00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO30                            0x00001E00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO29                            0x00001D00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO28                            0x00001C00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO27                            0x00001B00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO26                            0x00001A00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO25                            0x00001900
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO24                            0x00001800
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO23                            0x00001700
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO22                            0x00001600
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO21                            0x00001500
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO20                            0x00001400
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO19                            0x00001300
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO18                            0x00001200
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO17                            0x00001100
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO16                            0x00001000
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO15                            0x00000F00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO14                            0x00000E00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO13                            0x00000D00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO12                            0x00000C00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO11                            0x00000B00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO10                            0x00000A00
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO9                             0x00000900
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO8                             0x00000800
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO7                             0x00000700
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO6                             0x00000600
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO5                             0x00000500
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO4                             0x00000400
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO3                             0x00000300
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO2                             0x00000200
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO1                             0x00000100
#define AUX_TDC_TRIGSRC_STOP_SRC_AUXIO0                             0x00000000

// Field:     [6] START_POL
//
// Polarity of start source.
//
// Change only while STAT.STATE is IDLE.
// ENUMs:
// LOW                      TDC conversion starts when low level is detected.
// HIGH                     TDC conversion starts when high level is detected.
#define AUX_TDC_TRIGSRC_START_POL                                   0x00000040
#define AUX_TDC_TRIGSRC_START_POL_BITN                                       6
#define AUX_TDC_TRIGSRC_START_POL_M                                 0x00000040
#define AUX_TDC_TRIGSRC_START_POL_S                                          6
#define AUX_TDC_TRIGSRC_START_POL_LOW                               0x00000040
#define AUX_TDC_TRIGSRC_START_POL_HIGH                              0x00000000

// Field:   [5:0] START_SRC
//
// Select start source from the asynchronous AUX event bus.
//
// Change only while STAT.STATE is IDLE.
// ENUMs:
// NO_EVENT                 No event.
// AUX_TDC_PRE              Select TDC Prescaler event which is generated by
//                          configuration of PRECTL.
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER0_EV
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
#define AUX_TDC_TRIGSRC_START_SRC_W                                          6
#define AUX_TDC_TRIGSRC_START_SRC_M                                 0x0000003F
#define AUX_TDC_TRIGSRC_START_SRC_S                                          0
#define AUX_TDC_TRIGSRC_START_SRC_NO_EVENT                          0x0000003F
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TDC_PRE                       0x0000003E
#define AUX_TDC_TRIGSRC_START_SRC_AUX_SMPH_AUTOTAKE_DONE            0x0000003D
#define AUX_TDC_TRIGSRC_START_SRC_AUX_ADC_FIFO_NOT_EMPTY            0x0000003C
#define AUX_TDC_TRIGSRC_START_SRC_AUX_ADC_FIFO_ALMOST_FULL          0x0000003B
#define AUX_TDC_TRIGSRC_START_SRC_AUX_ADC_IRQ                       0x0000003A
#define AUX_TDC_TRIGSRC_START_SRC_AUX_ADC_DONE                      0x00000039
#define AUX_TDC_TRIGSRC_START_SRC_AUX_ISRC_RESET_N                  0x00000038
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TDC_DONE                      0x00000037
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER0_EV                     0x00000036
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER1_EV                     0x00000035
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_PULSE                  0x00000034
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_EV3                    0x00000033
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_EV2                    0x00000032
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_EV1                    0x00000031
#define AUX_TDC_TRIGSRC_START_SRC_AUX_TIMER2_EV0                    0x00000030
#define AUX_TDC_TRIGSRC_START_SRC_AUX_COMPB                         0x0000002F
#define AUX_TDC_TRIGSRC_START_SRC_AUX_COMPA                         0x0000002E
#define AUX_TDC_TRIGSRC_START_SRC_MCU_OBSMUX1                       0x0000002D
#define AUX_TDC_TRIGSRC_START_SRC_MCU_OBSMUX0                       0x0000002C
#define AUX_TDC_TRIGSRC_START_SRC_MCU_EV                            0x0000002B
#define AUX_TDC_TRIGSRC_START_SRC_ACLK_REF                          0x0000002A
#define AUX_TDC_TRIGSRC_START_SRC_VDDR_RECHARGE                     0x00000029
#define AUX_TDC_TRIGSRC_START_SRC_MCU_ACTIVE                        0x00000028
#define AUX_TDC_TRIGSRC_START_SRC_PWR_DWN                           0x00000027
#define AUX_TDC_TRIGSRC_START_SRC_SCLK_LF                           0x00000026
#define AUX_TDC_TRIGSRC_START_SRC_AON_BATMON_TEMP_UPD               0x00000025
#define AUX_TDC_TRIGSRC_START_SRC_AON_BATMON_BAT_UPD                0x00000024
#define AUX_TDC_TRIGSRC_START_SRC_AON_RTC_4KHZ                      0x00000023
#define AUX_TDC_TRIGSRC_START_SRC_AON_RTC_CH2_DLY                   0x00000022
#define AUX_TDC_TRIGSRC_START_SRC_AON_RTC_CH2                       0x00000021
#define AUX_TDC_TRIGSRC_START_SRC_MANUAL_EV                         0x00000020
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO31                           0x0000001F
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO30                           0x0000001E
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO29                           0x0000001D
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO28                           0x0000001C
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO27                           0x0000001B
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO26                           0x0000001A
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO25                           0x00000019
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO24                           0x00000018
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO23                           0x00000017
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO22                           0x00000016
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO21                           0x00000015
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO20                           0x00000014
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO19                           0x00000013
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO18                           0x00000012
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO17                           0x00000011
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO16                           0x00000010
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO15                           0x0000000F
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO14                           0x0000000E
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO13                           0x0000000D
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO12                           0x0000000C
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO11                           0x0000000B
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO10                           0x0000000A
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO9                            0x00000009
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO8                            0x00000008
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO7                            0x00000007
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO6                            0x00000006
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO5                            0x00000005
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO4                            0x00000004
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO3                            0x00000003
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO2                            0x00000002
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO1                            0x00000001
#define AUX_TDC_TRIGSRC_START_SRC_AUXIO0                            0x00000000

//*****************************************************************************
//
// Register: AUX_TDC_O_TRIGCNT
//
//*****************************************************************************
// Field:  [15:0] CNT
//
// Number of stop events to ignore when AUX_TDC:TRIGCNTCFG.EN is 1.
//
// Read CNT to get the remaining number of stop events to ignore during a TDC
// measurement.
//
// Write CNT to update the remaining number of stop events to ignore during a
// TDC measurement. The TDC measurement ignores updates of CNT if there are no
// more stop events left to ignore.
//
// When AUX_TDC:TRIGCNTCFG.EN is 1, TRIGCNTLOAD.CNT is loaded into CNT at the
// start of the measurement.
#define AUX_TDC_TRIGCNT_CNT_W                                               16
#define AUX_TDC_TRIGCNT_CNT_M                                       0x0000FFFF
#define AUX_TDC_TRIGCNT_CNT_S                                                0

//*****************************************************************************
//
// Register: AUX_TDC_O_TRIGCNTLOAD
//
//*****************************************************************************
// Field:  [15:0] CNT
//
// Number of stop events to ignore when AUX_TDC:TRIGCNTCFG.EN is 1.
//
// To measure frequency of an event source:
// - Set start event equal to stop event.
// - Set CNT to number of periods to measure. Both 0 and 1 values measures a
// single event source period.
//
// To measure pulse width of an event source:
// - Set start event source equal to stop event source.
// - Select different polarity for start and stop event.
// - Set CNT to 0.
//
// To measure time from the start event to the Nth stop event when N > 1:
// - Select different start and stop event source.
// - Set CNT to (N-1).
//
// See the Technical Reference Manual for event timing requirements.
//
// When AUX_TDC:TRIGCNTCFG.EN is 1, CNT is loaded into TRIGCNT.CNT at the start
// of the measurement.
#define AUX_TDC_TRIGCNTLOAD_CNT_W                                           16
#define AUX_TDC_TRIGCNTLOAD_CNT_M                                   0x0000FFFF
#define AUX_TDC_TRIGCNTLOAD_CNT_S                                            0

//*****************************************************************************
//
// Register: AUX_TDC_O_TRIGCNTCFG
//
//*****************************************************************************
// Field:     [0] EN
//
// Enable stop-counter.
//
// 0: Disable stop-counter.
// 1: Enable stop-counter.
//
// Change only while STAT.STATE is IDLE.
#define AUX_TDC_TRIGCNTCFG_EN                                       0x00000001
#define AUX_TDC_TRIGCNTCFG_EN_BITN                                           0
#define AUX_TDC_TRIGCNTCFG_EN_M                                     0x00000001
#define AUX_TDC_TRIGCNTCFG_EN_S                                              0

//*****************************************************************************
//
// Register: AUX_TDC_O_PRECTL
//
//*****************************************************************************
// Field:     [7] RESET_N
//
// Prescaler reset.
//
// 0: Reset prescaler.
// 1: Release reset of prescaler.
//
// AUX_TDC_PRE event becomes 0 when you reset the prescaler.
#define AUX_TDC_PRECTL_RESET_N                                      0x00000080
#define AUX_TDC_PRECTL_RESET_N_BITN                                          7
#define AUX_TDC_PRECTL_RESET_N_M                                    0x00000080
#define AUX_TDC_PRECTL_RESET_N_S                                             7

// Field:     [6] RATIO
//
// Prescaler ratio.
//
// This controls how often the AUX_TDC_PRE event is generated by the prescaler.
// ENUMs:
// DIV64                    Prescaler divides input by 64.
//
//                          AUX_TDC_PRE event has a
//                          rising edge for every 64 rising edges of the
//                          input. AUX_TDC_PRE event toggles on every 32nd
//                          rising edge of the input.
// DIV16                    Prescaler divides input by 16.
//
//                          AUX_TDC_PRE event has a
//                          rising edge for every 16 rising edges of the
//                          input. AUX_TDC_PRE event toggles on every 8th
//                          rising edge of the input.
#define AUX_TDC_PRECTL_RATIO                                        0x00000040
#define AUX_TDC_PRECTL_RATIO_BITN                                            6
#define AUX_TDC_PRECTL_RATIO_M                                      0x00000040
#define AUX_TDC_PRECTL_RATIO_S                                               6
#define AUX_TDC_PRECTL_RATIO_DIV64                                  0x00000040
#define AUX_TDC_PRECTL_RATIO_DIV16                                  0x00000000

// Field:   [5:0] SRC
//
// Prescaler event source.
//
// Select an event from the asynchronous AUX event bus to connect to the
// prescaler input.
//
// Configure only while RESET_N is 0.
// ENUMs:
// NO_EVENT                 No event.
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL AUX_EVCTL:EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              AUX_EVCTL:EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             AUX_EVCTL:EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         AUX_EVCTL:EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             AUX_EVCTL:EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            AUX_EVCTL:EVSTAT3.AUX_TIMER0_EV
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
#define AUX_TDC_PRECTL_SRC_W                                                 6
#define AUX_TDC_PRECTL_SRC_M                                        0x0000003F
#define AUX_TDC_PRECTL_SRC_S                                                 0
#define AUX_TDC_PRECTL_SRC_NO_EVENT                                 0x0000003F
#define AUX_TDC_PRECTL_SRC_AUX_SMPH_AUTOTAKE_DONE                   0x0000003D
#define AUX_TDC_PRECTL_SRC_AUX_ADC_FIFO_NOT_EMPTY                   0x0000003C
#define AUX_TDC_PRECTL_SRC_AUX_ADC_FIFO_ALMOST_FULL                 0x0000003B
#define AUX_TDC_PRECTL_SRC_AUX_ADC_IRQ                              0x0000003A
#define AUX_TDC_PRECTL_SRC_AUX_ADC_DONE                             0x00000039
#define AUX_TDC_PRECTL_SRC_AUX_ISRC_RESET_N                         0x00000038
#define AUX_TDC_PRECTL_SRC_AUX_TDC_DONE                             0x00000037
#define AUX_TDC_PRECTL_SRC_AUX_TIMER0_EV                            0x00000036
#define AUX_TDC_PRECTL_SRC_AUX_TIMER1_EV                            0x00000035
#define AUX_TDC_PRECTL_SRC_AUX_TIMER2_PULSE                         0x00000034
#define AUX_TDC_PRECTL_SRC_AUX_TIMER2_EV3                           0x00000033
#define AUX_TDC_PRECTL_SRC_AUX_TIMER2_EV2                           0x00000032
#define AUX_TDC_PRECTL_SRC_AUX_TIMER2_EV1                           0x00000031
#define AUX_TDC_PRECTL_SRC_AUX_TIMER2_EV0                           0x00000030
#define AUX_TDC_PRECTL_SRC_AUX_COMPB                                0x0000002F
#define AUX_TDC_PRECTL_SRC_AUX_COMPA                                0x0000002E
#define AUX_TDC_PRECTL_SRC_MCU_OBSMUX1                              0x0000002D
#define AUX_TDC_PRECTL_SRC_MCU_OBSMUX0                              0x0000002C
#define AUX_TDC_PRECTL_SRC_MCU_EV                                   0x0000002B
#define AUX_TDC_PRECTL_SRC_ACLK_REF                                 0x0000002A
#define AUX_TDC_PRECTL_SRC_VDDR_RECHARGE                            0x00000029
#define AUX_TDC_PRECTL_SRC_MCU_ACTIVE                               0x00000028
#define AUX_TDC_PRECTL_SRC_PWR_DWN                                  0x00000027
#define AUX_TDC_PRECTL_SRC_SCLK_LF                                  0x00000026
#define AUX_TDC_PRECTL_SRC_AON_BATMON_TEMP_UPD                      0x00000025
#define AUX_TDC_PRECTL_SRC_AON_BATMON_BAT_UPD                       0x00000024
#define AUX_TDC_PRECTL_SRC_AON_RTC_4KHZ                             0x00000023
#define AUX_TDC_PRECTL_SRC_AON_RTC_CH2_DLY                          0x00000022
#define AUX_TDC_PRECTL_SRC_AON_RTC_CH2                              0x00000021
#define AUX_TDC_PRECTL_SRC_MANUAL_EV                                0x00000020
#define AUX_TDC_PRECTL_SRC_AUXIO31                                  0x0000001F
#define AUX_TDC_PRECTL_SRC_AUXIO30                                  0x0000001E
#define AUX_TDC_PRECTL_SRC_AUXIO29                                  0x0000001D
#define AUX_TDC_PRECTL_SRC_AUXIO28                                  0x0000001C
#define AUX_TDC_PRECTL_SRC_AUXIO27                                  0x0000001B
#define AUX_TDC_PRECTL_SRC_AUXIO26                                  0x0000001A
#define AUX_TDC_PRECTL_SRC_AUXIO25                                  0x00000019
#define AUX_TDC_PRECTL_SRC_AUXIO24                                  0x00000018
#define AUX_TDC_PRECTL_SRC_AUXIO23                                  0x00000017
#define AUX_TDC_PRECTL_SRC_AUXIO22                                  0x00000016
#define AUX_TDC_PRECTL_SRC_AUXIO21                                  0x00000015
#define AUX_TDC_PRECTL_SRC_AUXIO20                                  0x00000014
#define AUX_TDC_PRECTL_SRC_AUXIO19                                  0x00000013
#define AUX_TDC_PRECTL_SRC_AUXIO18                                  0x00000012
#define AUX_TDC_PRECTL_SRC_AUXIO17                                  0x00000011
#define AUX_TDC_PRECTL_SRC_AUXIO16                                  0x00000010
#define AUX_TDC_PRECTL_SRC_AUXIO15                                  0x0000000F
#define AUX_TDC_PRECTL_SRC_AUXIO14                                  0x0000000E
#define AUX_TDC_PRECTL_SRC_AUXIO13                                  0x0000000D
#define AUX_TDC_PRECTL_SRC_AUXIO12                                  0x0000000C
#define AUX_TDC_PRECTL_SRC_AUXIO11                                  0x0000000B
#define AUX_TDC_PRECTL_SRC_AUXIO10                                  0x0000000A
#define AUX_TDC_PRECTL_SRC_AUXIO9                                   0x00000009
#define AUX_TDC_PRECTL_SRC_AUXIO8                                   0x00000008
#define AUX_TDC_PRECTL_SRC_AUXIO7                                   0x00000007
#define AUX_TDC_PRECTL_SRC_AUXIO6                                   0x00000006
#define AUX_TDC_PRECTL_SRC_AUXIO5                                   0x00000005
#define AUX_TDC_PRECTL_SRC_AUXIO4                                   0x00000004
#define AUX_TDC_PRECTL_SRC_AUXIO3                                   0x00000003
#define AUX_TDC_PRECTL_SRC_AUXIO2                                   0x00000002
#define AUX_TDC_PRECTL_SRC_AUXIO1                                   0x00000001
#define AUX_TDC_PRECTL_SRC_AUXIO0                                   0x00000000

//*****************************************************************************
//
// Register: AUX_TDC_O_PRECNTR
//
//*****************************************************************************
// Field:  [15:0] CNT
//
// Prescaler counter value.
//
// Write a value to CNT to capture the value of the 16-bit prescaler counter
// into CNT. Read CNT to get the captured value.
//
// The read value gets 1 LSB uncertainty if the event source level rises when
// you release the reset.
// The read value gets 1 LSB uncertainty if the event source level rises when
// you capture the prescaler counter.
//
// Please note the following:
// - The prescaler counter is reset to 2 by PRECTL.RESET_N.
// - The captured value is 2 when the number of rising edges on prescaler input
// is less than 3. Otherwise, captured value equals number of event pulses - 1.
#define AUX_TDC_PRECNTR_CNT_W                                               16
#define AUX_TDC_PRECNTR_CNT_M                                       0x0000FFFF
#define AUX_TDC_PRECNTR_CNT_S                                                0


#endif // __AUX_TDC__
