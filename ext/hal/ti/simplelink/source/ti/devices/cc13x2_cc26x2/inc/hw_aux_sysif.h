/******************************************************************************
*  Filename:       hw_aux_sysif_h
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

#ifndef __HW_AUX_SYSIF_H__
#define __HW_AUX_SYSIF_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_SYSIF component
//
//*****************************************************************************
// Operational Mode Request
#define AUX_SYSIF_O_OPMODEREQ                                       0x00000000

// Operational Mode Acknowledgement
#define AUX_SYSIF_O_OPMODEACK                                       0x00000004

// Programmable Wakeup 0 Configuration
#define AUX_SYSIF_O_PROGWU0CFG                                      0x00000008

// Programmable Wakeup 1 Configuration
#define AUX_SYSIF_O_PROGWU1CFG                                      0x0000000C

// Programmable Wakeup 2 Configuration
#define AUX_SYSIF_O_PROGWU2CFG                                      0x00000010

// Programmable Wakeup 3 Configuration
#define AUX_SYSIF_O_PROGWU3CFG                                      0x00000014

// Software Wakeup Triggers
#define AUX_SYSIF_O_SWWUTRIG                                        0x00000018

// Wakeup Flags
#define AUX_SYSIF_O_WUFLAGS                                         0x0000001C

// Wakeup Flags Clear
#define AUX_SYSIF_O_WUFLAGSCLR                                      0x00000020

// Wakeup Gate
#define AUX_SYSIF_O_WUGATE                                          0x00000024

// Vector Configuration 0
#define AUX_SYSIF_O_VECCFG0                                         0x00000028

// Vector Configuration 1
#define AUX_SYSIF_O_VECCFG1                                         0x0000002C

// Vector Configuration 2
#define AUX_SYSIF_O_VECCFG2                                         0x00000030

// Vector Configuration 3
#define AUX_SYSIF_O_VECCFG3                                         0x00000034

// Vector Configuration 4
#define AUX_SYSIF_O_VECCFG4                                         0x00000038

// Vector Configuration 5
#define AUX_SYSIF_O_VECCFG5                                         0x0000003C

// Vector Configuration 6
#define AUX_SYSIF_O_VECCFG6                                         0x00000040

// Vector Configuration 7
#define AUX_SYSIF_O_VECCFG7                                         0x00000044

// Event Synchronization Rate
#define AUX_SYSIF_O_EVSYNCRATE                                      0x00000048

// Peripheral Operational Rate
#define AUX_SYSIF_O_PEROPRATE                                       0x0000004C

// ADC Clock Control
#define AUX_SYSIF_O_ADCCLKCTL                                       0x00000050

// TDC Counter Clock Control
#define AUX_SYSIF_O_TDCCLKCTL                                       0x00000054

// TDC Reference Clock Control
#define AUX_SYSIF_O_TDCREFCLKCTL                                    0x00000058

// AUX_TIMER2 Clock Control
#define AUX_SYSIF_O_TIMER2CLKCTL                                    0x0000005C

// AUX_TIMER2 Clock Status
#define AUX_SYSIF_O_TIMER2CLKSTAT                                   0x00000060

// AUX_TIMER2 Clock Switch
#define AUX_SYSIF_O_TIMER2CLKSWITCH                                 0x00000064

// AUX_TIMER2 Debug Control
#define AUX_SYSIF_O_TIMER2DBGCTL                                    0x00000068

// Clock Shift Detection
#define AUX_SYSIF_O_CLKSHIFTDET                                     0x00000070

// VDDR Recharge Trigger
#define AUX_SYSIF_O_RECHARGETRIG                                    0x00000074

// VDDR Recharge Detection
#define AUX_SYSIF_O_RECHARGEDET                                     0x00000078

// Real Time Counter Sub Second Increment 0
#define AUX_SYSIF_O_RTCSUBSECINC0                                   0x0000007C

// Real Time Counter Sub Second Increment 1
#define AUX_SYSIF_O_RTCSUBSECINC1                                   0x00000080

// Real Time Counter Sub Second Increment Control
#define AUX_SYSIF_O_RTCSUBSECINCCTL                                 0x00000084

// Real Time Counter Second
#define AUX_SYSIF_O_RTCSEC                                          0x00000088

// Real Time Counter Sub-Second
#define AUX_SYSIF_O_RTCSUBSEC                                       0x0000008C

// AON_RTC Event Clear
#define AUX_SYSIF_O_RTCEVCLR                                        0x00000090

// AON_BATMON Battery Voltage Value
#define AUX_SYSIF_O_BATMONBAT                                       0x00000094

// AON_BATMON Temperature Value
#define AUX_SYSIF_O_BATMONTEMP                                      0x0000009C

// Timer Halt
#define AUX_SYSIF_O_TIMERHALT                                       0x000000A0

// AUX_TIMER2 Bridge
#define AUX_SYSIF_O_TIMER2BRIDGE                                    0x000000B0

// Software Power Profiler
#define AUX_SYSIF_O_SWPWRPROF                                       0x000000B4

//*****************************************************************************
//
// Register: AUX_SYSIF_O_OPMODEREQ
//
//*****************************************************************************
// Field:   [1:0] REQ
//
// AUX operational mode request.
// ENUMs:
// PDLP                     Powerdown operational mode with wakeup to lowpower
//                          mode, characterized by:
//                          - Powerdown system power
//                          supply state (uLDO) request.
//                          -
//                          AON_PMCTL:AUXSCECLK.PD_SRC sets the SCE clock
//                          frequency (SCE_RATE).
//                          - An active wakeup flag
//                          overrides the operational mode externally to
//                          lowpower (LP) as long as the flag is set.
// PDA                      Powerdown operational mode with wakeup to active
//                          mode, characterized by:
//                          - Powerdown system power
//                          supply state (uLDO) request.
//                          -
//                          AON_PMCTL:AUXSCECLK.PD_SRC sets the SCE clock
//                          frequency (SCE_RATE).
//                          - An active wakeup flag
//                          overrides the operational mode externally to
//                          active (A) as long as the flag is set.
// LP                       Lowpower operational mode, characterized by:
//                          - Powerdown system power
//                          supply state (uLDO) request.
//                          - SCE clock frequency
//                          (SCE_RATE) equals SCLK_MF.
//                          - An active wakeup flag
//                          does not change operational mode.
// A                        Active operational mode, characterized by:
//                          - Active system power
//                          supply state (GLDO or DCDC) request.
//                          - AON_PMCTL:AUXSCECLK.SRC
//                          sets the SCE clock frequency (SCE_RATE).
//                          - An active wakeup flag
//                          does not change operational mode.
#define AUX_SYSIF_OPMODEREQ_REQ_W                                            2
#define AUX_SYSIF_OPMODEREQ_REQ_M                                   0x00000003
#define AUX_SYSIF_OPMODEREQ_REQ_S                                            0
#define AUX_SYSIF_OPMODEREQ_REQ_PDLP                                0x00000003
#define AUX_SYSIF_OPMODEREQ_REQ_PDA                                 0x00000002
#define AUX_SYSIF_OPMODEREQ_REQ_LP                                  0x00000001
#define AUX_SYSIF_OPMODEREQ_REQ_A                                   0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_OPMODEACK
//
//*****************************************************************************
// Field:   [1:0] ACK
//
// AUX operational mode acknowledgement.
// ENUMs:
// PDLP                     Powerdown operational mode with wakeup to lowpower
//                          mode is acknowledged.
// PDA                      Powerdown operational mode with wakeup to active
//                          mode is acknowledged.
// LP                       Lowpower operational mode is acknowledged.
// A                        Active operational mode is acknowledged.
#define AUX_SYSIF_OPMODEACK_ACK_W                                            2
#define AUX_SYSIF_OPMODEACK_ACK_M                                   0x00000003
#define AUX_SYSIF_OPMODEACK_ACK_S                                            0
#define AUX_SYSIF_OPMODEACK_ACK_PDLP                                0x00000003
#define AUX_SYSIF_OPMODEACK_ACK_PDA                                 0x00000002
#define AUX_SYSIF_OPMODEACK_ACK_LP                                  0x00000001
#define AUX_SYSIF_OPMODEACK_ACK_A                                   0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_PROGWU0CFG
//
//*****************************************************************************
// Field:     [7] POL
//
// Polarity of WU_SRC.
//
// The procedure used to clear the wakeup flag decides level or edge
// sensitivity, see WUFLAGSCLR.PROG_WU0.
// ENUMs:
// LOW                      The wakeup flag is set when WU_SRC is low or goes
//                          low.
// HIGH                     The wakeup flag is set when WU_SRC is high or goes
//                          high.
#define AUX_SYSIF_PROGWU0CFG_POL                                    0x00000080
#define AUX_SYSIF_PROGWU0CFG_POL_BITN                                        7
#define AUX_SYSIF_PROGWU0CFG_POL_M                                  0x00000080
#define AUX_SYSIF_PROGWU0CFG_POL_S                                           7
#define AUX_SYSIF_PROGWU0CFG_POL_LOW                                0x00000080
#define AUX_SYSIF_PROGWU0CFG_POL_HIGH                               0x00000000

// Field:     [6] EN
//
// Programmable wakeup flag enable.
//
// 0: Disable wakeup flag.
// 1: Enable wakeup flag.
#define AUX_SYSIF_PROGWU0CFG_EN                                     0x00000040
#define AUX_SYSIF_PROGWU0CFG_EN_BITN                                         6
#define AUX_SYSIF_PROGWU0CFG_EN_M                                   0x00000040
#define AUX_SYSIF_PROGWU0CFG_EN_S                                            6

// Field:   [5:0] WU_SRC
//
// Wakeup source from the asynchronous AUX event bus.
//
// Only change WU_SRC when EN is 0 or WUFLAGSCLR.PROG_WU0 is 1.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
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
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_W                                        6
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_M                               0x0000003F
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_S                                        0
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_NO_EVENT                        0x0000003F
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_SMPH_AUTOTAKE_DONE          0x0000003D
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_ADC_FIFO_NOT_EMPTY          0x0000003C
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_ADC_FIFO_ALMOST_FULL        0x0000003B
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_ADC_IRQ                     0x0000003A
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_ADC_DONE                    0x00000039
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_ISRC_RESET_N                0x00000038
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_TDC_DONE                    0x00000037
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_TIMER0_EV                   0x00000036
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_TIMER1_EV                   0x00000035
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_TIMER2_PULSE                0x00000034
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_TIMER2_EV3                  0x00000033
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_TIMER2_EV2                  0x00000032
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_TIMER2_EV1                  0x00000031
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_TIMER2_EV0                  0x00000030
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_COMPB                       0x0000002F
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUX_COMPA                       0x0000002E
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_MCU_OBSMUX1                     0x0000002D
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_MCU_OBSMUX0                     0x0000002C
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_MCU_EV                          0x0000002B
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_ACLK_REF                        0x0000002A
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_VDDR_RECHARGE                   0x00000029
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_MCU_ACTIVE                      0x00000028
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_PWR_DWN                         0x00000027
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_SCLK_LF                         0x00000026
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AON_BATMON_TEMP_UPD             0x00000025
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AON_BATMON_BAT_UPD              0x00000024
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AON_RTC_4KHZ                    0x00000023
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AON_RTC_CH2_DLY                 0x00000022
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AON_RTC_CH2                     0x00000021
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_MANUAL_EV                       0x00000020
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO31                         0x0000001F
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO30                         0x0000001E
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO29                         0x0000001D
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO28                         0x0000001C
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO27                         0x0000001B
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO26                         0x0000001A
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO25                         0x00000019
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO24                         0x00000018
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO23                         0x00000017
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO22                         0x00000016
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO21                         0x00000015
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO20                         0x00000014
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO19                         0x00000013
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO18                         0x00000012
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO17                         0x00000011
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO16                         0x00000010
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO15                         0x0000000F
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO14                         0x0000000E
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO13                         0x0000000D
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO12                         0x0000000C
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO11                         0x0000000B
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO10                         0x0000000A
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO9                          0x00000009
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO8                          0x00000008
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO7                          0x00000007
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO6                          0x00000006
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO5                          0x00000005
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO4                          0x00000004
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO3                          0x00000003
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO2                          0x00000002
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO1                          0x00000001
#define AUX_SYSIF_PROGWU0CFG_WU_SRC_AUXIO0                          0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_PROGWU1CFG
//
//*****************************************************************************
// Field:     [7] POL
//
// Polarity of WU_SRC.
//
// The procedure used to clear the wakeup flag decides level or edge
// sensitivity, see WUFLAGSCLR.PROG_WU1.
// ENUMs:
// LOW                      The wakeup flag is set when WU_SRC is low or goes
//                          low.
// HIGH                     The wakeup flag is set when WU_SRC is high or goes
//                          high.
#define AUX_SYSIF_PROGWU1CFG_POL                                    0x00000080
#define AUX_SYSIF_PROGWU1CFG_POL_BITN                                        7
#define AUX_SYSIF_PROGWU1CFG_POL_M                                  0x00000080
#define AUX_SYSIF_PROGWU1CFG_POL_S                                           7
#define AUX_SYSIF_PROGWU1CFG_POL_LOW                                0x00000080
#define AUX_SYSIF_PROGWU1CFG_POL_HIGH                               0x00000000

// Field:     [6] EN
//
// Programmable wakeup flag enable.
//
// 0: Disable wakeup flag.
// 1: Enable wakeup flag.
#define AUX_SYSIF_PROGWU1CFG_EN                                     0x00000040
#define AUX_SYSIF_PROGWU1CFG_EN_BITN                                         6
#define AUX_SYSIF_PROGWU1CFG_EN_M                                   0x00000040
#define AUX_SYSIF_PROGWU1CFG_EN_S                                            6

// Field:   [5:0] WU_SRC
//
// Wakeup source from the asynchronous AUX event bus.
//
// Only change WU_SRC when EN is 0 or WUFLAGSCLR.PROG_WU1 is 1.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
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
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_W                                        6
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_M                               0x0000003F
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_S                                        0
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_NO_EVENT                        0x0000003F
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_SMPH_AUTOTAKE_DONE          0x0000003D
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_ADC_FIFO_NOT_EMPTY          0x0000003C
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_ADC_FIFO_ALMOST_FULL        0x0000003B
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_ADC_IRQ                     0x0000003A
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_ADC_DONE                    0x00000039
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_ISRC_RESET_N                0x00000038
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_TDC_DONE                    0x00000037
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_TIMER0_EV                   0x00000036
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_TIMER1_EV                   0x00000035
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_TIMER2_PULSE                0x00000034
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_TIMER2_EV3                  0x00000033
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_TIMER2_EV2                  0x00000032
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_TIMER2_EV1                  0x00000031
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_TIMER2_EV0                  0x00000030
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_COMPB                       0x0000002F
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUX_COMPA                       0x0000002E
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_MCU_OBSMUX1                     0x0000002D
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_MCU_OBSMUX0                     0x0000002C
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_MCU_EV                          0x0000002B
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_ACLK_REF                        0x0000002A
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_VDDR_RECHARGE                   0x00000029
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_MCU_ACTIVE                      0x00000028
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_PWR_DWN                         0x00000027
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_SCLK_LF                         0x00000026
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AON_BATMON_TEMP_UPD             0x00000025
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AON_BATMON_BAT_UPD              0x00000024
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AON_RTC_4KHZ                    0x00000023
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AON_RTC_CH2_DLY                 0x00000022
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AON_RTC_CH2                     0x00000021
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_MANUAL_EV                       0x00000020
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO31                         0x0000001F
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO30                         0x0000001E
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO29                         0x0000001D
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO28                         0x0000001C
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO27                         0x0000001B
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO26                         0x0000001A
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO25                         0x00000019
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO24                         0x00000018
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO23                         0x00000017
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO22                         0x00000016
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO21                         0x00000015
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO20                         0x00000014
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO19                         0x00000013
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO18                         0x00000012
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO17                         0x00000011
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO16                         0x00000010
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO15                         0x0000000F
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO14                         0x0000000E
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO13                         0x0000000D
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO12                         0x0000000C
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO11                         0x0000000B
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO10                         0x0000000A
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO9                          0x00000009
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO8                          0x00000008
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO7                          0x00000007
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO6                          0x00000006
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO5                          0x00000005
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO4                          0x00000004
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO3                          0x00000003
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO2                          0x00000002
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO1                          0x00000001
#define AUX_SYSIF_PROGWU1CFG_WU_SRC_AUXIO0                          0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_PROGWU2CFG
//
//*****************************************************************************
// Field:     [7] POL
//
// Polarity of WU_SRC.
//
// The procedure used to clear the wakeup flag decides level or edge
// sensitivity, see WUFLAGSCLR.PROG_WU2.
// ENUMs:
// LOW                      The wakeup flag is set when WU_SRC is low or goes
//                          low.
// HIGH                     The wakeup flag is set when WU_SRC is high or goes
//                          high.
#define AUX_SYSIF_PROGWU2CFG_POL                                    0x00000080
#define AUX_SYSIF_PROGWU2CFG_POL_BITN                                        7
#define AUX_SYSIF_PROGWU2CFG_POL_M                                  0x00000080
#define AUX_SYSIF_PROGWU2CFG_POL_S                                           7
#define AUX_SYSIF_PROGWU2CFG_POL_LOW                                0x00000080
#define AUX_SYSIF_PROGWU2CFG_POL_HIGH                               0x00000000

// Field:     [6] EN
//
// Programmable wakeup flag enable.
//
// 0: Disable wakeup flag.
// 1: Enable wakeup flag.
#define AUX_SYSIF_PROGWU2CFG_EN                                     0x00000040
#define AUX_SYSIF_PROGWU2CFG_EN_BITN                                         6
#define AUX_SYSIF_PROGWU2CFG_EN_M                                   0x00000040
#define AUX_SYSIF_PROGWU2CFG_EN_S                                            6

// Field:   [5:0] WU_SRC
//
// Wakeup source from the asynchronous AUX event bus.
//
// Only change WU_SRC when EN is 0 or WUFLAGSCLR.PROG_WU2 is 1.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
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
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_W                                        6
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_M                               0x0000003F
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_S                                        0
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_NO_EVENT                        0x0000003F
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_SMPH_AUTOTAKE_DONE          0x0000003D
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_ADC_FIFO_NOT_EMPTY          0x0000003C
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_ADC_FIFO_ALMOST_FULL        0x0000003B
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_ADC_IRQ                     0x0000003A
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_ADC_DONE                    0x00000039
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_ISRC_RESET_N                0x00000038
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_TDC_DONE                    0x00000037
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_TIMER0_EV                   0x00000036
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_TIMER1_EV                   0x00000035
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_TIMER2_PULSE                0x00000034
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_TIMER2_EV3                  0x00000033
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_TIMER2_EV2                  0x00000032
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_TIMER2_EV1                  0x00000031
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_TIMER2_EV0                  0x00000030
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_COMPB                       0x0000002F
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUX_COMPA                       0x0000002E
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_MCU_OBSMUX1                     0x0000002D
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_MCU_OBSMUX0                     0x0000002C
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_MCU_EV                          0x0000002B
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_ACLK_REF                        0x0000002A
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_VDDR_RECHARGE                   0x00000029
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_MCU_ACTIVE                      0x00000028
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_PWR_DWN                         0x00000027
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_SCLK_LF                         0x00000026
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AON_BATMON_TEMP_UPD             0x00000025
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AON_BATMON_BAT_UPD              0x00000024
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AON_RTC_4KHZ                    0x00000023
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AON_RTC_CH2_DLY                 0x00000022
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AON_RTC_CH2                     0x00000021
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_MANUAL_EV                       0x00000020
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO31                         0x0000001F
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO30                         0x0000001E
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO29                         0x0000001D
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO28                         0x0000001C
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO27                         0x0000001B
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO26                         0x0000001A
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO25                         0x00000019
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO24                         0x00000018
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO23                         0x00000017
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO22                         0x00000016
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO21                         0x00000015
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO20                         0x00000014
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO19                         0x00000013
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO18                         0x00000012
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO17                         0x00000011
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO16                         0x00000010
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO15                         0x0000000F
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO14                         0x0000000E
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO13                         0x0000000D
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO12                         0x0000000C
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO11                         0x0000000B
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO10                         0x0000000A
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO9                          0x00000009
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO8                          0x00000008
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO7                          0x00000007
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO6                          0x00000006
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO5                          0x00000005
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO4                          0x00000004
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO3                          0x00000003
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO2                          0x00000002
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO1                          0x00000001
#define AUX_SYSIF_PROGWU2CFG_WU_SRC_AUXIO0                          0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_PROGWU3CFG
//
//*****************************************************************************
// Field:     [7] POL
//
// Polarity of WU_SRC.
//
// The procedure used to clear the wakeup flag decides level or edge
// sensitivity, see WUFLAGSCLR.PROG_WU3.
// ENUMs:
// LOW                      The wakeup flag is set when WU_SRC is low or goes
//                          low.
// HIGH                     The wakeup flag is set when WU_SRC is high or goes
//                          high.
#define AUX_SYSIF_PROGWU3CFG_POL                                    0x00000080
#define AUX_SYSIF_PROGWU3CFG_POL_BITN                                        7
#define AUX_SYSIF_PROGWU3CFG_POL_M                                  0x00000080
#define AUX_SYSIF_PROGWU3CFG_POL_S                                           7
#define AUX_SYSIF_PROGWU3CFG_POL_LOW                                0x00000080
#define AUX_SYSIF_PROGWU3CFG_POL_HIGH                               0x00000000

// Field:     [6] EN
//
// Programmable wakeup flag enable.
//
// 0: Disable wakeup flag.
// 1: Enable wakeup flag.
#define AUX_SYSIF_PROGWU3CFG_EN                                     0x00000040
#define AUX_SYSIF_PROGWU3CFG_EN_BITN                                         6
#define AUX_SYSIF_PROGWU3CFG_EN_M                                   0x00000040
#define AUX_SYSIF_PROGWU3CFG_EN_S                                            6

// Field:   [5:0] WU_SRC
//
// Wakeup source from the asynchronous AUX event bus.
//
// Only change WU_SRC when EN is 0 or WUFLAGSCLR.PROG_WU3 is 1.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
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
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_W                                        6
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_M                               0x0000003F
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_S                                        0
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_NO_EVENT                        0x0000003F
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_SMPH_AUTOTAKE_DONE          0x0000003D
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_ADC_FIFO_NOT_EMPTY          0x0000003C
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_ADC_FIFO_ALMOST_FULL        0x0000003B
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_ADC_IRQ                     0x0000003A
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_ADC_DONE                    0x00000039
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_ISRC_RESET_N                0x00000038
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_TDC_DONE                    0x00000037
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_TIMER0_EV                   0x00000036
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_TIMER1_EV                   0x00000035
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_TIMER2_PULSE                0x00000034
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_TIMER2_EV3                  0x00000033
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_TIMER2_EV2                  0x00000032
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_TIMER2_EV1                  0x00000031
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_TIMER2_EV0                  0x00000030
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_COMPB                       0x0000002F
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUX_COMPA                       0x0000002E
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_MCU_OBSMUX1                     0x0000002D
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_MCU_OBSMUX0                     0x0000002C
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_MCU_EV                          0x0000002B
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_ACLK_REF                        0x0000002A
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_VDDR_RECHARGE                   0x00000029
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_MCU_ACTIVE                      0x00000028
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_PWR_DWN                         0x00000027
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_SCLK_LF                         0x00000026
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AON_BATMON_TEMP_UPD             0x00000025
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AON_BATMON_BAT_UPD              0x00000024
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AON_RTC_4KHZ                    0x00000023
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AON_RTC_CH2_DLY                 0x00000022
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AON_RTC_CH2                     0x00000021
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_MANUAL_EV                       0x00000020
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO31                         0x0000001F
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO30                         0x0000001E
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO29                         0x0000001D
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO28                         0x0000001C
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO27                         0x0000001B
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO26                         0x0000001A
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO25                         0x00000019
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO24                         0x00000018
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO23                         0x00000017
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO22                         0x00000016
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO21                         0x00000015
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO20                         0x00000014
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO19                         0x00000013
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO18                         0x00000012
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO17                         0x00000011
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO16                         0x00000010
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO15                         0x0000000F
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO14                         0x0000000E
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO13                         0x0000000D
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO12                         0x0000000C
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO11                         0x0000000B
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO10                         0x0000000A
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO9                          0x00000009
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO8                          0x00000008
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO7                          0x00000007
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO6                          0x00000006
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO5                          0x00000005
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO4                          0x00000004
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO3                          0x00000003
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO2                          0x00000002
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO1                          0x00000001
#define AUX_SYSIF_PROGWU3CFG_WU_SRC_AUXIO0                          0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_SWWUTRIG
//
//*****************************************************************************
// Field:     [3] SW_WU3
//
// Software wakeup 3 trigger.
//
// 0: No effect.
// 1: Set WUFLAGS.SW_WU3 and trigger AUX wakeup.
#define AUX_SYSIF_SWWUTRIG_SW_WU3                                   0x00000008
#define AUX_SYSIF_SWWUTRIG_SW_WU3_BITN                                       3
#define AUX_SYSIF_SWWUTRIG_SW_WU3_M                                 0x00000008
#define AUX_SYSIF_SWWUTRIG_SW_WU3_S                                          3

// Field:     [2] SW_WU2
//
// Software wakeup 2 trigger.
//
// 0: No effect.
// 1: Set WUFLAGS.SW_WU2 and trigger AUX wakeup.
#define AUX_SYSIF_SWWUTRIG_SW_WU2                                   0x00000004
#define AUX_SYSIF_SWWUTRIG_SW_WU2_BITN                                       2
#define AUX_SYSIF_SWWUTRIG_SW_WU2_M                                 0x00000004
#define AUX_SYSIF_SWWUTRIG_SW_WU2_S                                          2

// Field:     [1] SW_WU1
//
// Software wakeup 1 trigger.
//
// 0: No effect.
// 1: Set WUFLAGS.SW_WU1 and trigger AUX wakeup.
#define AUX_SYSIF_SWWUTRIG_SW_WU1                                   0x00000002
#define AUX_SYSIF_SWWUTRIG_SW_WU1_BITN                                       1
#define AUX_SYSIF_SWWUTRIG_SW_WU1_M                                 0x00000002
#define AUX_SYSIF_SWWUTRIG_SW_WU1_S                                          1

// Field:     [0] SW_WU0
//
// Software wakeup 0 trigger.
//
// 0: No effect.
// 1: Set WUFLAGS.SW_WU0 and trigger AUX wakeup.
#define AUX_SYSIF_SWWUTRIG_SW_WU0                                   0x00000001
#define AUX_SYSIF_SWWUTRIG_SW_WU0_BITN                                       0
#define AUX_SYSIF_SWWUTRIG_SW_WU0_M                                 0x00000001
#define AUX_SYSIF_SWWUTRIG_SW_WU0_S                                          0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_WUFLAGS
//
//*****************************************************************************
// Field:     [7] SW_WU3
//
// Software wakeup 3 flag.
//
// 0: Software wakeup 3 not triggered.
// 1: Software wakeup 3 triggered.
#define AUX_SYSIF_WUFLAGS_SW_WU3                                    0x00000080
#define AUX_SYSIF_WUFLAGS_SW_WU3_BITN                                        7
#define AUX_SYSIF_WUFLAGS_SW_WU3_M                                  0x00000080
#define AUX_SYSIF_WUFLAGS_SW_WU3_S                                           7

// Field:     [6] SW_WU2
//
// Software wakeup 2 flag.
//
// 0: Software wakeup 2 not triggered.
// 1: Software wakeup 2 triggered.
#define AUX_SYSIF_WUFLAGS_SW_WU2                                    0x00000040
#define AUX_SYSIF_WUFLAGS_SW_WU2_BITN                                        6
#define AUX_SYSIF_WUFLAGS_SW_WU2_M                                  0x00000040
#define AUX_SYSIF_WUFLAGS_SW_WU2_S                                           6

// Field:     [5] SW_WU1
//
// Software wakeup 1 flag.
//
// 0: Software wakeup 1 not triggered.
// 1: Software wakeup 1 triggered.
#define AUX_SYSIF_WUFLAGS_SW_WU1                                    0x00000020
#define AUX_SYSIF_WUFLAGS_SW_WU1_BITN                                        5
#define AUX_SYSIF_WUFLAGS_SW_WU1_M                                  0x00000020
#define AUX_SYSIF_WUFLAGS_SW_WU1_S                                           5

// Field:     [4] SW_WU0
//
// Software wakeup 0 flag.
//
// 0: Software wakeup 0 not triggered.
// 1: Software wakeup 0 triggered.
#define AUX_SYSIF_WUFLAGS_SW_WU0                                    0x00000010
#define AUX_SYSIF_WUFLAGS_SW_WU0_BITN                                        4
#define AUX_SYSIF_WUFLAGS_SW_WU0_M                                  0x00000010
#define AUX_SYSIF_WUFLAGS_SW_WU0_S                                           4

// Field:     [3] PROG_WU3
//
// Programmable wakeup 3.
//
// 0: Programmable wakeup 3 not triggered.
// 1: Programmable wakeup 3 triggered.
#define AUX_SYSIF_WUFLAGS_PROG_WU3                                  0x00000008
#define AUX_SYSIF_WUFLAGS_PROG_WU3_BITN                                      3
#define AUX_SYSIF_WUFLAGS_PROG_WU3_M                                0x00000008
#define AUX_SYSIF_WUFLAGS_PROG_WU3_S                                         3

// Field:     [2] PROG_WU2
//
// Programmable wakeup 2.
//
// 0: Programmable wakeup 2 not triggered.
// 1: Programmable wakeup 2 triggered.
#define AUX_SYSIF_WUFLAGS_PROG_WU2                                  0x00000004
#define AUX_SYSIF_WUFLAGS_PROG_WU2_BITN                                      2
#define AUX_SYSIF_WUFLAGS_PROG_WU2_M                                0x00000004
#define AUX_SYSIF_WUFLAGS_PROG_WU2_S                                         2

// Field:     [1] PROG_WU1
//
// Programmable wakeup 1.
//
// 0: Programmable wakeup 1 not triggered.
// 1: Programmable wakeup 1 triggered.
#define AUX_SYSIF_WUFLAGS_PROG_WU1                                  0x00000002
#define AUX_SYSIF_WUFLAGS_PROG_WU1_BITN                                      1
#define AUX_SYSIF_WUFLAGS_PROG_WU1_M                                0x00000002
#define AUX_SYSIF_WUFLAGS_PROG_WU1_S                                         1

// Field:     [0] PROG_WU0
//
// Programmable wakeup 0.
//
// 0: Programmable wakeup 0 not triggered.
// 1: Programmable wakeup 0 triggered.
#define AUX_SYSIF_WUFLAGS_PROG_WU0                                  0x00000001
#define AUX_SYSIF_WUFLAGS_PROG_WU0_BITN                                      0
#define AUX_SYSIF_WUFLAGS_PROG_WU0_M                                0x00000001
#define AUX_SYSIF_WUFLAGS_PROG_WU0_S                                         0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_WUFLAGSCLR
//
//*****************************************************************************
// Field:     [7] SW_WU3
//
// Clear software wakeup flag 3.
//
// 0: No effect.
// 1: Clear WUFLAGS.SW_WU3. Keep high until WUFLAGS.SW_WU3 is 0.
#define AUX_SYSIF_WUFLAGSCLR_SW_WU3                                 0x00000080
#define AUX_SYSIF_WUFLAGSCLR_SW_WU3_BITN                                     7
#define AUX_SYSIF_WUFLAGSCLR_SW_WU3_M                               0x00000080
#define AUX_SYSIF_WUFLAGSCLR_SW_WU3_S                                        7

// Field:     [6] SW_WU2
//
// Clear software wakeup flag 2.
//
// 0: No effect.
// 1: Clear WUFLAGS.SW_WU2. Keep high until WUFLAGS.SW_WU2 is 0.
#define AUX_SYSIF_WUFLAGSCLR_SW_WU2                                 0x00000040
#define AUX_SYSIF_WUFLAGSCLR_SW_WU2_BITN                                     6
#define AUX_SYSIF_WUFLAGSCLR_SW_WU2_M                               0x00000040
#define AUX_SYSIF_WUFLAGSCLR_SW_WU2_S                                        6

// Field:     [5] SW_WU1
//
// Clear software wakeup flag 1.
//
// 0: No effect.
// 1: Clear WUFLAGS.SW_WU1. Keep high until WUFLAGS.SW_WU1 is 0.
#define AUX_SYSIF_WUFLAGSCLR_SW_WU1                                 0x00000020
#define AUX_SYSIF_WUFLAGSCLR_SW_WU1_BITN                                     5
#define AUX_SYSIF_WUFLAGSCLR_SW_WU1_M                               0x00000020
#define AUX_SYSIF_WUFLAGSCLR_SW_WU1_S                                        5

// Field:     [4] SW_WU0
//
// Clear software wakeup flag 0.
//
// 0: No effect.
// 1: Clear WUFLAGS.SW_WU0. Keep high until WUFLAGS.SW_WU0 is 0.
#define AUX_SYSIF_WUFLAGSCLR_SW_WU0                                 0x00000010
#define AUX_SYSIF_WUFLAGSCLR_SW_WU0_BITN                                     4
#define AUX_SYSIF_WUFLAGSCLR_SW_WU0_M                               0x00000010
#define AUX_SYSIF_WUFLAGSCLR_SW_WU0_S                                        4

// Field:     [3] PROG_WU3
//
// Programmable wakeup flag 3.
//
// 0: No effect.
// 1: Clear WUFLAGS.PROG_WU3. Keep high until WUFLAGS.PROG_WU3 is 0.
//
// The wakeup flag becomes edge sensitive if you write PROG_WU3 to 0 when
// PROGWU3CFG.EN is 1.
// The wakeup flag becomes level sensitive if you write PROG_WU3 to 0 when
// PROGWU3CFG.EN is 0, then set PROGWU3CFG.EN.
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU3                               0x00000008
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU3_BITN                                   3
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU3_M                             0x00000008
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU3_S                                      3

// Field:     [2] PROG_WU2
//
// Programmable wakeup flag 2.
//
// 0: No effect.
// 1: Clear WUFLAGS.PROG_WU2. Keep high until WUFLAGS.PROG_WU2 is 0.
//
// The wakeup flag becomes edge sensitive if you write PROG_WU2 to 0 when
// PROGWU2CFG.EN is 1.
// The wakeup flag becomes level sensitive if you write PROG_WU2 to 0 when
// PROGWU2CFG.EN is 0, then set PROGWU2CFG.EN.
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU2                               0x00000004
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU2_BITN                                   2
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU2_M                             0x00000004
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU2_S                                      2

// Field:     [1] PROG_WU1
//
// Programmable wakeup flag 1.
//
// 0: No effect.
// 1: Clear WUFLAGS.PROG_WU1. Keep high until WUFLAGS.PROG_WU1 is 0.
//
// The wakeup flag becomes edge sensitive if you write PROG_WU1 to 0 when
// PROGWU1CFG.EN is 1.
// The wakeup flag becomes level sensitive if you write PROG_WU1 to 0 when
// PROGWU1CFG.EN is 0, then set PROGWU1CFG.EN.
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU1                               0x00000002
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU1_BITN                                   1
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU1_M                             0x00000002
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU1_S                                      1

// Field:     [0] PROG_WU0
//
// Programmable wakeup flag 0.
//
// 0: No effect.
// 1: Clear WUFLAGS.PROG_WU0. Keep high until WUFLAGS.PROG_WU0 is 0.
//
// The wakeup flag becomes edge sensitive if you write PROG_WU0 to 0 when
// PROGWU0CFG.EN is 1.
// The wakeup flag becomes level sensitive if you write PROG_WU0 to 0 when
// PROGWU0CFG.EN is 0, then set PROGWU0CFG.EN.
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU0                               0x00000001
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU0_BITN                                   0
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU0_M                             0x00000001
#define AUX_SYSIF_WUFLAGSCLR_PROG_WU0_S                                      0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_WUGATE
//
//*****************************************************************************
// Field:     [0] EN
//
// Wakeup output enable.
//
// 0: Disable AUX wakeup output.
// 1: Enable AUX wakeup output.
#define AUX_SYSIF_WUGATE_EN                                         0x00000001
#define AUX_SYSIF_WUGATE_EN_BITN                                             0
#define AUX_SYSIF_WUGATE_EN_M                                       0x00000001
#define AUX_SYSIF_WUGATE_EN_S                                                0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_VECCFG0
//
//*****************************************************************************
// Field:   [3:0] VEC_EV
//
// Select trigger event for vector 0.
//
// Non-enumerated values are treated as NONE.
// ENUMs:
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// SW_WU3                   WUFLAGS.SW_WU3
// SW_WU2                   WUFLAGS.SW_WU2
// SW_WU1                   WUFLAGS.SW_WU1
// SW_WU0                   WUFLAGS.SW_WU0
// PROG_WU3                 WUFLAGS.PROG_WU3
// PROG_WU2                 WUFLAGS.PROG_WU2
// PROG_WU1                 WUFLAGS.PROG_WU1
// PROG_WU0                 WUFLAGS.PROG_WU0
// NONE                     Vector is disabled.
#define AUX_SYSIF_VECCFG0_VEC_EV_W                                           4
#define AUX_SYSIF_VECCFG0_VEC_EV_M                                  0x0000000F
#define AUX_SYSIF_VECCFG0_VEC_EV_S                                           0
#define AUX_SYSIF_VECCFG0_VEC_EV_AON_RTC_CH2_DLY                    0x00000009
#define AUX_SYSIF_VECCFG0_VEC_EV_SW_WU3                             0x00000008
#define AUX_SYSIF_VECCFG0_VEC_EV_SW_WU2                             0x00000007
#define AUX_SYSIF_VECCFG0_VEC_EV_SW_WU1                             0x00000006
#define AUX_SYSIF_VECCFG0_VEC_EV_SW_WU0                             0x00000005
#define AUX_SYSIF_VECCFG0_VEC_EV_PROG_WU3                           0x00000004
#define AUX_SYSIF_VECCFG0_VEC_EV_PROG_WU2                           0x00000003
#define AUX_SYSIF_VECCFG0_VEC_EV_PROG_WU1                           0x00000002
#define AUX_SYSIF_VECCFG0_VEC_EV_PROG_WU0                           0x00000001
#define AUX_SYSIF_VECCFG0_VEC_EV_NONE                               0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_VECCFG1
//
//*****************************************************************************
// Field:   [3:0] VEC_EV
//
// Select trigger event for vector 1.
//
// Non-enumerated values are treated as NONE.
// ENUMs:
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// SW_WU3                   WUFLAGS.SW_WU3
// SW_WU2                   WUFLAGS.SW_WU2
// SW_WU1                   WUFLAGS.SW_WU1
// SW_WU0                   WUFLAGS.SW_WU0
// PROG_WU3                 WUFLAGS.PROG_WU3
// PROG_WU2                 WUFLAGS.PROG_WU2
// PROG_WU1                 WUFLAGS.PROG_WU1
// PROG_WU0                 WUFLAGS.PROG_WU0
// NONE                     Vector is disabled.
#define AUX_SYSIF_VECCFG1_VEC_EV_W                                           4
#define AUX_SYSIF_VECCFG1_VEC_EV_M                                  0x0000000F
#define AUX_SYSIF_VECCFG1_VEC_EV_S                                           0
#define AUX_SYSIF_VECCFG1_VEC_EV_AON_RTC_CH2_DLY                    0x00000009
#define AUX_SYSIF_VECCFG1_VEC_EV_SW_WU3                             0x00000008
#define AUX_SYSIF_VECCFG1_VEC_EV_SW_WU2                             0x00000007
#define AUX_SYSIF_VECCFG1_VEC_EV_SW_WU1                             0x00000006
#define AUX_SYSIF_VECCFG1_VEC_EV_SW_WU0                             0x00000005
#define AUX_SYSIF_VECCFG1_VEC_EV_PROG_WU3                           0x00000004
#define AUX_SYSIF_VECCFG1_VEC_EV_PROG_WU2                           0x00000003
#define AUX_SYSIF_VECCFG1_VEC_EV_PROG_WU1                           0x00000002
#define AUX_SYSIF_VECCFG1_VEC_EV_PROG_WU0                           0x00000001
#define AUX_SYSIF_VECCFG1_VEC_EV_NONE                               0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_VECCFG2
//
//*****************************************************************************
// Field:   [3:0] VEC_EV
//
// Select trigger event for vector 2.
//
// Non-enumerated values are treated as NONE.
// ENUMs:
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// SW_WU3                   WUFLAGS.SW_WU3
// SW_WU2                   WUFLAGS.SW_WU2
// SW_WU1                   WUFLAGS.SW_WU1
// SW_WU0                   WUFLAGS.SW_WU0
// PROG_WU3                 WUFLAGS.PROG_WU3
// PROG_WU2                 WUFLAGS.PROG_WU2
// PROG_WU1                 WUFLAGS.PROG_WU1
// PROG_WU0                 WUFLAGS.PROG_WU0
// NONE                     Vector is disabled.
#define AUX_SYSIF_VECCFG2_VEC_EV_W                                           4
#define AUX_SYSIF_VECCFG2_VEC_EV_M                                  0x0000000F
#define AUX_SYSIF_VECCFG2_VEC_EV_S                                           0
#define AUX_SYSIF_VECCFG2_VEC_EV_AON_RTC_CH2_DLY                    0x00000009
#define AUX_SYSIF_VECCFG2_VEC_EV_SW_WU3                             0x00000008
#define AUX_SYSIF_VECCFG2_VEC_EV_SW_WU2                             0x00000007
#define AUX_SYSIF_VECCFG2_VEC_EV_SW_WU1                             0x00000006
#define AUX_SYSIF_VECCFG2_VEC_EV_SW_WU0                             0x00000005
#define AUX_SYSIF_VECCFG2_VEC_EV_PROG_WU3                           0x00000004
#define AUX_SYSIF_VECCFG2_VEC_EV_PROG_WU2                           0x00000003
#define AUX_SYSIF_VECCFG2_VEC_EV_PROG_WU1                           0x00000002
#define AUX_SYSIF_VECCFG2_VEC_EV_PROG_WU0                           0x00000001
#define AUX_SYSIF_VECCFG2_VEC_EV_NONE                               0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_VECCFG3
//
//*****************************************************************************
// Field:   [3:0] VEC_EV
//
// Select trigger event for vector 3.
//
// Non-enumerated values are treated as NONE.
// ENUMs:
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// SW_WU3                   WUFLAGS.SW_WU3
// SW_WU2                   WUFLAGS.SW_WU2
// SW_WU1                   WUFLAGS.SW_WU1
// SW_WU0                   WUFLAGS.SW_WU0
// PROG_WU3                 WUFLAGS.PROG_WU3
// PROG_WU2                 WUFLAGS.PROG_WU2
// PROG_WU1                 WUFLAGS.PROG_WU1
// PROG_WU0                 WUFLAGS.PROG_WU0
// NONE                     Vector is disabled.
#define AUX_SYSIF_VECCFG3_VEC_EV_W                                           4
#define AUX_SYSIF_VECCFG3_VEC_EV_M                                  0x0000000F
#define AUX_SYSIF_VECCFG3_VEC_EV_S                                           0
#define AUX_SYSIF_VECCFG3_VEC_EV_AON_RTC_CH2_DLY                    0x00000009
#define AUX_SYSIF_VECCFG3_VEC_EV_SW_WU3                             0x00000008
#define AUX_SYSIF_VECCFG3_VEC_EV_SW_WU2                             0x00000007
#define AUX_SYSIF_VECCFG3_VEC_EV_SW_WU1                             0x00000006
#define AUX_SYSIF_VECCFG3_VEC_EV_SW_WU0                             0x00000005
#define AUX_SYSIF_VECCFG3_VEC_EV_PROG_WU3                           0x00000004
#define AUX_SYSIF_VECCFG3_VEC_EV_PROG_WU2                           0x00000003
#define AUX_SYSIF_VECCFG3_VEC_EV_PROG_WU1                           0x00000002
#define AUX_SYSIF_VECCFG3_VEC_EV_PROG_WU0                           0x00000001
#define AUX_SYSIF_VECCFG3_VEC_EV_NONE                               0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_VECCFG4
//
//*****************************************************************************
// Field:   [3:0] VEC_EV
//
// Select trigger event for vector 4.
//
// Non-enumerated values are treated as NONE.
// ENUMs:
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// SW_WU3                   WUFLAGS.SW_WU3
// SW_WU2                   WUFLAGS.SW_WU2
// SW_WU1                   WUFLAGS.SW_WU1
// SW_WU0                   WUFLAGS.SW_WU0
// PROG_WU3                 WUFLAGS.PROG_WU3
// PROG_WU2                 WUFLAGS.PROG_WU2
// PROG_WU1                 WUFLAGS.PROG_WU1
// PROG_WU0                 WUFLAGS.PROG_WU0
// NONE                     Vector is disabled.
#define AUX_SYSIF_VECCFG4_VEC_EV_W                                           4
#define AUX_SYSIF_VECCFG4_VEC_EV_M                                  0x0000000F
#define AUX_SYSIF_VECCFG4_VEC_EV_S                                           0
#define AUX_SYSIF_VECCFG4_VEC_EV_AON_RTC_CH2_DLY                    0x00000009
#define AUX_SYSIF_VECCFG4_VEC_EV_SW_WU3                             0x00000008
#define AUX_SYSIF_VECCFG4_VEC_EV_SW_WU2                             0x00000007
#define AUX_SYSIF_VECCFG4_VEC_EV_SW_WU1                             0x00000006
#define AUX_SYSIF_VECCFG4_VEC_EV_SW_WU0                             0x00000005
#define AUX_SYSIF_VECCFG4_VEC_EV_PROG_WU3                           0x00000004
#define AUX_SYSIF_VECCFG4_VEC_EV_PROG_WU2                           0x00000003
#define AUX_SYSIF_VECCFG4_VEC_EV_PROG_WU1                           0x00000002
#define AUX_SYSIF_VECCFG4_VEC_EV_PROG_WU0                           0x00000001
#define AUX_SYSIF_VECCFG4_VEC_EV_NONE                               0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_VECCFG5
//
//*****************************************************************************
// Field:   [3:0] VEC_EV
//
// Select trigger event for vector 5.
//
// Non-enumerated values are treated as NONE.
// ENUMs:
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// SW_WU3                   WUFLAGS.SW_WU3
// SW_WU2                   WUFLAGS.SW_WU2
// SW_WU1                   WUFLAGS.SW_WU1
// SW_WU0                   WUFLAGS.SW_WU0
// PROG_WU3                 WUFLAGS.PROG_WU3
// PROG_WU2                 WUFLAGS.PROG_WU2
// PROG_WU1                 WUFLAGS.PROG_WU1
// PROG_WU0                 WUFLAGS.PROG_WU0
// NONE                     Vector is disabled.
#define AUX_SYSIF_VECCFG5_VEC_EV_W                                           4
#define AUX_SYSIF_VECCFG5_VEC_EV_M                                  0x0000000F
#define AUX_SYSIF_VECCFG5_VEC_EV_S                                           0
#define AUX_SYSIF_VECCFG5_VEC_EV_AON_RTC_CH2_DLY                    0x00000009
#define AUX_SYSIF_VECCFG5_VEC_EV_SW_WU3                             0x00000008
#define AUX_SYSIF_VECCFG5_VEC_EV_SW_WU2                             0x00000007
#define AUX_SYSIF_VECCFG5_VEC_EV_SW_WU1                             0x00000006
#define AUX_SYSIF_VECCFG5_VEC_EV_SW_WU0                             0x00000005
#define AUX_SYSIF_VECCFG5_VEC_EV_PROG_WU3                           0x00000004
#define AUX_SYSIF_VECCFG5_VEC_EV_PROG_WU2                           0x00000003
#define AUX_SYSIF_VECCFG5_VEC_EV_PROG_WU1                           0x00000002
#define AUX_SYSIF_VECCFG5_VEC_EV_PROG_WU0                           0x00000001
#define AUX_SYSIF_VECCFG5_VEC_EV_NONE                               0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_VECCFG6
//
//*****************************************************************************
// Field:   [3:0] VEC_EV
//
// Select trigger event for vector 6.
//
// Non-enumerated values are treated as NONE.
// ENUMs:
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// SW_WU3                   WUFLAGS.SW_WU3
// SW_WU2                   WUFLAGS.SW_WU2
// SW_WU1                   WUFLAGS.SW_WU1
// SW_WU0                   WUFLAGS.SW_WU0
// PROG_WU3                 WUFLAGS.PROG_WU3
// PROG_WU2                 WUFLAGS.PROG_WU2
// PROG_WU1                 WUFLAGS.PROG_WU1
// PROG_WU0                 WUFLAGS.PROG_WU0
// NONE                     Vector is disabled.
#define AUX_SYSIF_VECCFG6_VEC_EV_W                                           4
#define AUX_SYSIF_VECCFG6_VEC_EV_M                                  0x0000000F
#define AUX_SYSIF_VECCFG6_VEC_EV_S                                           0
#define AUX_SYSIF_VECCFG6_VEC_EV_AON_RTC_CH2_DLY                    0x00000009
#define AUX_SYSIF_VECCFG6_VEC_EV_SW_WU3                             0x00000008
#define AUX_SYSIF_VECCFG6_VEC_EV_SW_WU2                             0x00000007
#define AUX_SYSIF_VECCFG6_VEC_EV_SW_WU1                             0x00000006
#define AUX_SYSIF_VECCFG6_VEC_EV_SW_WU0                             0x00000005
#define AUX_SYSIF_VECCFG6_VEC_EV_PROG_WU3                           0x00000004
#define AUX_SYSIF_VECCFG6_VEC_EV_PROG_WU2                           0x00000003
#define AUX_SYSIF_VECCFG6_VEC_EV_PROG_WU1                           0x00000002
#define AUX_SYSIF_VECCFG6_VEC_EV_PROG_WU0                           0x00000001
#define AUX_SYSIF_VECCFG6_VEC_EV_NONE                               0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_VECCFG7
//
//*****************************************************************************
// Field:   [3:0] VEC_EV
//
// Select trigger event for vector 7.
//
// Non-enumerated values are treated as NONE.
// ENUMs:
// AON_RTC_CH2_DLY          AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY
// SW_WU3                   WUFLAGS.SW_WU3
// SW_WU2                   WUFLAGS.SW_WU2
// SW_WU1                   WUFLAGS.SW_WU1
// SW_WU0                   WUFLAGS.SW_WU0
// PROG_WU3                 WUFLAGS.PROG_WU3
// PROG_WU2                 WUFLAGS.PROG_WU2
// PROG_WU1                 WUFLAGS.PROG_WU1
// PROG_WU0                 WUFLAGS.PROG_WU0
// NONE                     Vector is disabled.
#define AUX_SYSIF_VECCFG7_VEC_EV_W                                           4
#define AUX_SYSIF_VECCFG7_VEC_EV_M                                  0x0000000F
#define AUX_SYSIF_VECCFG7_VEC_EV_S                                           0
#define AUX_SYSIF_VECCFG7_VEC_EV_AON_RTC_CH2_DLY                    0x00000009
#define AUX_SYSIF_VECCFG7_VEC_EV_SW_WU3                             0x00000008
#define AUX_SYSIF_VECCFG7_VEC_EV_SW_WU2                             0x00000007
#define AUX_SYSIF_VECCFG7_VEC_EV_SW_WU1                             0x00000006
#define AUX_SYSIF_VECCFG7_VEC_EV_SW_WU0                             0x00000005
#define AUX_SYSIF_VECCFG7_VEC_EV_PROG_WU3                           0x00000004
#define AUX_SYSIF_VECCFG7_VEC_EV_PROG_WU2                           0x00000003
#define AUX_SYSIF_VECCFG7_VEC_EV_PROG_WU1                           0x00000002
#define AUX_SYSIF_VECCFG7_VEC_EV_PROG_WU0                           0x00000001
#define AUX_SYSIF_VECCFG7_VEC_EV_NONE                               0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_EVSYNCRATE
//
//*****************************************************************************
// Field:     [2] AUX_COMPA_SYNC_RATE
//
// Select synchronization rate for AUX_EVCTL:EVSTAT2.AUX_COMPA event.
// ENUMs:
// BUS_RATE                 AUX bus rate
// SCE_RATE                 SCE rate
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPA_SYNC_RATE                    0x00000004
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPA_SYNC_RATE_BITN                        2
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPA_SYNC_RATE_M                  0x00000004
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPA_SYNC_RATE_S                           2
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPA_SYNC_RATE_BUS_RATE           0x00000004
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPA_SYNC_RATE_SCE_RATE           0x00000000

// Field:     [1] AUX_COMPB_SYNC_RATE
//
// Select synchronization rate for AUX_EVCTL:EVSTAT2.AUX_COMPB event.
// ENUMs:
// BUS_RATE                 AUX bus rate
// SCE_RATE                 SCE rate
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPB_SYNC_RATE                    0x00000002
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPB_SYNC_RATE_BITN                        1
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPB_SYNC_RATE_M                  0x00000002
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPB_SYNC_RATE_S                           1
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPB_SYNC_RATE_BUS_RATE           0x00000002
#define AUX_SYSIF_EVSYNCRATE_AUX_COMPB_SYNC_RATE_SCE_RATE           0x00000000

// Field:     [0] AUX_TIMER2_SYNC_RATE
//
// Select synchronization rate for:
// - AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV0
// - AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV1
// - AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV2
// - AUX_EVCTL:EVSTAT3.AUX_TIMER2_EV3
// - AUX_EVCTL:EVSTAT3.AUX_TIMER2_PULSE
// ENUMs:
// BUS_RATE                 AUX bus rate
// SCE_RATE                 SCE rate
#define AUX_SYSIF_EVSYNCRATE_AUX_TIMER2_SYNC_RATE                   0x00000001
#define AUX_SYSIF_EVSYNCRATE_AUX_TIMER2_SYNC_RATE_BITN                       0
#define AUX_SYSIF_EVSYNCRATE_AUX_TIMER2_SYNC_RATE_M                 0x00000001
#define AUX_SYSIF_EVSYNCRATE_AUX_TIMER2_SYNC_RATE_S                          0
#define AUX_SYSIF_EVSYNCRATE_AUX_TIMER2_SYNC_RATE_BUS_RATE          0x00000001
#define AUX_SYSIF_EVSYNCRATE_AUX_TIMER2_SYNC_RATE_SCE_RATE          0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_PEROPRATE
//
//*****************************************************************************
// Field:     [3] ANAIF_DAC_OP_RATE
//
// Select operational rate for AUX_ANAIF DAC sample clock state machine.
// ENUMs:
// BUS_RATE                 AUX bus rate
// SCE_RATE                 SCE rate
#define AUX_SYSIF_PEROPRATE_ANAIF_DAC_OP_RATE                       0x00000008
#define AUX_SYSIF_PEROPRATE_ANAIF_DAC_OP_RATE_BITN                           3
#define AUX_SYSIF_PEROPRATE_ANAIF_DAC_OP_RATE_M                     0x00000008
#define AUX_SYSIF_PEROPRATE_ANAIF_DAC_OP_RATE_S                              3
#define AUX_SYSIF_PEROPRATE_ANAIF_DAC_OP_RATE_BUS_RATE              0x00000008
#define AUX_SYSIF_PEROPRATE_ANAIF_DAC_OP_RATE_SCE_RATE              0x00000000

// Field:     [2] TIMER01_OP_RATE
//
// Select operational rate for AUX_TIMER01.
// ENUMs:
// BUS_RATE                 AUX bus rate
// SCE_RATE                 SCE rate
#define AUX_SYSIF_PEROPRATE_TIMER01_OP_RATE                         0x00000004
#define AUX_SYSIF_PEROPRATE_TIMER01_OP_RATE_BITN                             2
#define AUX_SYSIF_PEROPRATE_TIMER01_OP_RATE_M                       0x00000004
#define AUX_SYSIF_PEROPRATE_TIMER01_OP_RATE_S                                2
#define AUX_SYSIF_PEROPRATE_TIMER01_OP_RATE_BUS_RATE                0x00000004
#define AUX_SYSIF_PEROPRATE_TIMER01_OP_RATE_SCE_RATE                0x00000000

// Field:     [1] SPIM_OP_RATE
//
// Select operational rate for AUX_SPIM.
// ENUMs:
// BUS_RATE                 AUX bus rate
// SCE_RATE                 SCE rate
#define AUX_SYSIF_PEROPRATE_SPIM_OP_RATE                            0x00000002
#define AUX_SYSIF_PEROPRATE_SPIM_OP_RATE_BITN                                1
#define AUX_SYSIF_PEROPRATE_SPIM_OP_RATE_M                          0x00000002
#define AUX_SYSIF_PEROPRATE_SPIM_OP_RATE_S                                   1
#define AUX_SYSIF_PEROPRATE_SPIM_OP_RATE_BUS_RATE                   0x00000002
#define AUX_SYSIF_PEROPRATE_SPIM_OP_RATE_SCE_RATE                   0x00000000

// Field:     [0] MAC_OP_RATE
//
// Select operational rate for AUX_MAC.
// ENUMs:
// BUS_RATE                 AUX bus rate
// SCE_RATE                 SCE rate
#define AUX_SYSIF_PEROPRATE_MAC_OP_RATE                             0x00000001
#define AUX_SYSIF_PEROPRATE_MAC_OP_RATE_BITN                                 0
#define AUX_SYSIF_PEROPRATE_MAC_OP_RATE_M                           0x00000001
#define AUX_SYSIF_PEROPRATE_MAC_OP_RATE_S                                    0
#define AUX_SYSIF_PEROPRATE_MAC_OP_RATE_BUS_RATE                    0x00000001
#define AUX_SYSIF_PEROPRATE_MAC_OP_RATE_SCE_RATE                    0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_ADCCLKCTL
//
//*****************************************************************************
// Field:     [1] ACK
//
// Clock acknowledgement.
//
// 0: ADC clock is disabled.
// 1: ADC clock is enabled.
#define AUX_SYSIF_ADCCLKCTL_ACK                                     0x00000002
#define AUX_SYSIF_ADCCLKCTL_ACK_BITN                                         1
#define AUX_SYSIF_ADCCLKCTL_ACK_M                                   0x00000002
#define AUX_SYSIF_ADCCLKCTL_ACK_S                                            1

// Field:     [0] REQ
//
// ADC clock request.
//
// 0: Disable ADC clock.
// 1: Enable ADC clock.
//
// Only modify REQ when equal to ACK.
#define AUX_SYSIF_ADCCLKCTL_REQ                                     0x00000001
#define AUX_SYSIF_ADCCLKCTL_REQ_BITN                                         0
#define AUX_SYSIF_ADCCLKCTL_REQ_M                                   0x00000001
#define AUX_SYSIF_ADCCLKCTL_REQ_S                                            0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_TDCCLKCTL
//
//*****************************************************************************
// Field:     [1] ACK
//
// TDC counter clock acknowledgement.
//
// 0: TDC counter clock is disabled.
// 1: TDC counter clock is enabled.
#define AUX_SYSIF_TDCCLKCTL_ACK                                     0x00000002
#define AUX_SYSIF_TDCCLKCTL_ACK_BITN                                         1
#define AUX_SYSIF_TDCCLKCTL_ACK_M                                   0x00000002
#define AUX_SYSIF_TDCCLKCTL_ACK_S                                            1

// Field:     [0] REQ
//
// TDC counter clock request.
//
// 0: Disable TDC counter clock.
// 1: Enable TDC counter clock.
//
// Only modify REQ when equal to ACK.
#define AUX_SYSIF_TDCCLKCTL_REQ                                     0x00000001
#define AUX_SYSIF_TDCCLKCTL_REQ_BITN                                         0
#define AUX_SYSIF_TDCCLKCTL_REQ_M                                   0x00000001
#define AUX_SYSIF_TDCCLKCTL_REQ_S                                            0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_TDCREFCLKCTL
//
//*****************************************************************************
// Field:     [1] ACK
//
// TDC reference clock acknowledgement.
//
// 0: TDC reference clock is disabled.
// 1: TDC reference clock is enabled.
#define AUX_SYSIF_TDCREFCLKCTL_ACK                                  0x00000002
#define AUX_SYSIF_TDCREFCLKCTL_ACK_BITN                                      1
#define AUX_SYSIF_TDCREFCLKCTL_ACK_M                                0x00000002
#define AUX_SYSIF_TDCREFCLKCTL_ACK_S                                         1

// Field:     [0] REQ
//
// TDC reference clock request.
//
// 0: Disable TDC reference clock.
// 1: Enable TDC reference clock.
//
// Only modify REQ when equal to ACK.
#define AUX_SYSIF_TDCREFCLKCTL_REQ                                  0x00000001
#define AUX_SYSIF_TDCREFCLKCTL_REQ_BITN                                      0
#define AUX_SYSIF_TDCREFCLKCTL_REQ_M                                0x00000001
#define AUX_SYSIF_TDCREFCLKCTL_REQ_S                                         0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_TIMER2CLKCTL
//
//*****************************************************************************
// Field:   [2:0] SRC
//
// Select clock source for AUX_TIMER2.
//
// Update is only accepted if SRC equals TIMER2CLKSTAT.STAT or
// TIMER2CLKSWITCH.RDY is 1.
//
// It is recommended to select NONE only when TIMER2BRIDGE.BUSY is 0.
//
// A non-enumerated value is ignored.
// ENUMs:
// SCLK_HFDIV2              SCLK_HF / 2
// SCLK_MF                  SCLK_MF
// SCLK_LF                  SCLK_LF
// NONE                     no clock
#define AUX_SYSIF_TIMER2CLKCTL_SRC_W                                         3
#define AUX_SYSIF_TIMER2CLKCTL_SRC_M                                0x00000007
#define AUX_SYSIF_TIMER2CLKCTL_SRC_S                                         0
#define AUX_SYSIF_TIMER2CLKCTL_SRC_SCLK_HFDIV2                      0x00000004
#define AUX_SYSIF_TIMER2CLKCTL_SRC_SCLK_MF                          0x00000002
#define AUX_SYSIF_TIMER2CLKCTL_SRC_SCLK_LF                          0x00000001
#define AUX_SYSIF_TIMER2CLKCTL_SRC_NONE                             0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_TIMER2CLKSTAT
//
//*****************************************************************************
// Field:   [2:0] STAT
//
// AUX_TIMER2 clock source status.
// ENUMs:
// SCLK_HFDIV2              SCLK_HF / 2
// SCLK_MF                  SCLK_MF
// SCLK_LF                  SCLK_LF
// NONE                     No clock
#define AUX_SYSIF_TIMER2CLKSTAT_STAT_W                                       3
#define AUX_SYSIF_TIMER2CLKSTAT_STAT_M                              0x00000007
#define AUX_SYSIF_TIMER2CLKSTAT_STAT_S                                       0
#define AUX_SYSIF_TIMER2CLKSTAT_STAT_SCLK_HFDIV2                    0x00000004
#define AUX_SYSIF_TIMER2CLKSTAT_STAT_SCLK_MF                        0x00000002
#define AUX_SYSIF_TIMER2CLKSTAT_STAT_SCLK_LF                        0x00000001
#define AUX_SYSIF_TIMER2CLKSTAT_STAT_NONE                           0x00000000

//*****************************************************************************
//
// Register: AUX_SYSIF_O_TIMER2CLKSWITCH
//
//*****************************************************************************
// Field:     [0] RDY
//
// Status of clock switcher.
//
// 0: TIMER2CLKCTL.SRC is different from TIMER2CLKSTAT.STAT.
// 1: TIMER2CLKCTL.SRC equals TIMER2CLKSTAT.STAT.
//
// RDY connects to AUX_EVCTL:EVSTAT3.AUX_TIMER2_CLKSWITCH_RDY.
#define AUX_SYSIF_TIMER2CLKSWITCH_RDY                               0x00000001
#define AUX_SYSIF_TIMER2CLKSWITCH_RDY_BITN                                   0
#define AUX_SYSIF_TIMER2CLKSWITCH_RDY_M                             0x00000001
#define AUX_SYSIF_TIMER2CLKSWITCH_RDY_S                                      0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_TIMER2DBGCTL
//
//*****************************************************************************
// Field:     [0] DBG_FREEZE_EN
//
// Debug freeze enable.
//
// 0: AUX_TIMER2 does not halt when the system CPU halts in debug mode.
// 1: Halt AUX_TIMER2 when the system CPU halts in debug mode.
#define AUX_SYSIF_TIMER2DBGCTL_DBG_FREEZE_EN                        0x00000001
#define AUX_SYSIF_TIMER2DBGCTL_DBG_FREEZE_EN_BITN                            0
#define AUX_SYSIF_TIMER2DBGCTL_DBG_FREEZE_EN_M                      0x00000001
#define AUX_SYSIF_TIMER2DBGCTL_DBG_FREEZE_EN_S                               0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_CLKSHIFTDET
//
//*****************************************************************************
// Field:     [0] STAT
//
// Clock shift detection.
//
// Write:
//
// 0: Restart clock shift detection.
// 1: Do not use.
//
// Read:
//
// 0: MCU domain did not enter or exit active state since you wrote 0 to STAT.
// 1: MCU domain entered or exited active state since you wrote 0 to STAT.
#define AUX_SYSIF_CLKSHIFTDET_STAT                                  0x00000001
#define AUX_SYSIF_CLKSHIFTDET_STAT_BITN                                      0
#define AUX_SYSIF_CLKSHIFTDET_STAT_M                                0x00000001
#define AUX_SYSIF_CLKSHIFTDET_STAT_S                                         0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_RECHARGETRIG
//
//*****************************************************************************
// Field:     [0] TRIG
//
// Recharge trigger.
//
// 0: No effect.
// 1: Request VDDR recharge.
//
// Request VDDR recharge only when AUX_EVCTL:EVSTAT2.PWR_DWN is 1.
//
// Follow this sequence when OPMODEREQ.REQ is LP:
// - Set TRIG.
// - Wait until AUX_EVCTL:EVSTAT2.VDDR_RECHARGE is 1.
// - Clear TRIG.
// - Wait until AUX_EVCTL:EVSTAT2.VDDR_RECHARGE is 0.
//
// Follow this sequence when OPMODEREQ.REQ is PDA or PDLP:
// - Set TRIG.
// - Clear TRIG.
#define AUX_SYSIF_RECHARGETRIG_TRIG                                 0x00000001
#define AUX_SYSIF_RECHARGETRIG_TRIG_BITN                                     0
#define AUX_SYSIF_RECHARGETRIG_TRIG_M                               0x00000001
#define AUX_SYSIF_RECHARGETRIG_TRIG_S                                        0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_RECHARGEDET
//
//*****************************************************************************
// Field:     [1] STAT
//
// VDDR recharge detector status.
//
// 0: No recharge of VDDR has occurred since EN was set.
// 1: Recharge of VDDR has occurred since EN was set.
#define AUX_SYSIF_RECHARGEDET_STAT                                  0x00000002
#define AUX_SYSIF_RECHARGEDET_STAT_BITN                                      1
#define AUX_SYSIF_RECHARGEDET_STAT_M                                0x00000002
#define AUX_SYSIF_RECHARGEDET_STAT_S                                         1

// Field:     [0] EN
//
// VDDR recharge detector enable.
//
// 0: Disable recharge detection. STAT becomes zero.
// 1: Enable recharge detection.
#define AUX_SYSIF_RECHARGEDET_EN                                    0x00000001
#define AUX_SYSIF_RECHARGEDET_EN_BITN                                        0
#define AUX_SYSIF_RECHARGEDET_EN_M                                  0x00000001
#define AUX_SYSIF_RECHARGEDET_EN_S                                           0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_RTCSUBSECINC0
//
//*****************************************************************************
// Field:  [15:0] INC15_0
//
// New value for bits 15:0 in AON_RTC:SUBSECINC.
#define AUX_SYSIF_RTCSUBSECINC0_INC15_0_W                                   16
#define AUX_SYSIF_RTCSUBSECINC0_INC15_0_M                           0x0000FFFF
#define AUX_SYSIF_RTCSUBSECINC0_INC15_0_S                                    0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_RTCSUBSECINC1
//
//*****************************************************************************
// Field:   [7:0] INC23_16
//
// New value for bits 23:16 in AON_RTC:SUBSECINC.
#define AUX_SYSIF_RTCSUBSECINC1_INC23_16_W                                   8
#define AUX_SYSIF_RTCSUBSECINC1_INC23_16_M                          0x000000FF
#define AUX_SYSIF_RTCSUBSECINC1_INC23_16_S                                   0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_RTCSUBSECINCCTL
//
//*****************************************************************************
// Field:     [1] UPD_ACK
//
// Update acknowledgement.
//
// 0: AON_RTC has not acknowledged UPD_REQ.
// 1: AON_RTC has acknowledged UPD_REQ.
#define AUX_SYSIF_RTCSUBSECINCCTL_UPD_ACK                           0x00000002
#define AUX_SYSIF_RTCSUBSECINCCTL_UPD_ACK_BITN                               1
#define AUX_SYSIF_RTCSUBSECINCCTL_UPD_ACK_M                         0x00000002
#define AUX_SYSIF_RTCSUBSECINCCTL_UPD_ACK_S                                  1

// Field:     [0] UPD_REQ
//
// Request AON_RTC to update AON_RTC:SUBSECINC.
//
// 0: Clear request to update.
// 1: Set request to update.
//
// Only change UPD_REQ when it equals UPD_ACK. Clear UPD_REQ after UPD_ACK is
// 1.
#define AUX_SYSIF_RTCSUBSECINCCTL_UPD_REQ                           0x00000001
#define AUX_SYSIF_RTCSUBSECINCCTL_UPD_REQ_BITN                               0
#define AUX_SYSIF_RTCSUBSECINCCTL_UPD_REQ_M                         0x00000001
#define AUX_SYSIF_RTCSUBSECINCCTL_UPD_REQ_S                                  0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_RTCSEC
//
//*****************************************************************************
// Field:  [15:0] SEC
//
// Bits 15:0 in AON_RTC:SEC.VALUE.
//
// Follow this procedure to get the correct value:
// - Do two dummy reads of SEC.
// - Then read SEC until two consecutive reads are equal.
#define AUX_SYSIF_RTCSEC_SEC_W                                              16
#define AUX_SYSIF_RTCSEC_SEC_M                                      0x0000FFFF
#define AUX_SYSIF_RTCSEC_SEC_S                                               0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_RTCSUBSEC
//
//*****************************************************************************
// Field:  [15:0] SUBSEC
//
// Bits 31:16 in AON_RTC:SUBSEC.VALUE.
//
// Follow this procedure to get the correct value:
// - Do two dummy reads SUBSEC.
// - Then read SUBSEC until two consecutive reads are equal.
#define AUX_SYSIF_RTCSUBSEC_SUBSEC_W                                        16
#define AUX_SYSIF_RTCSUBSEC_SUBSEC_M                                0x0000FFFF
#define AUX_SYSIF_RTCSUBSEC_SUBSEC_S                                         0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_RTCEVCLR
//
//*****************************************************************************
// Field:     [0] RTC_CH2_EV_CLR
//
// Clear events from AON_RTC channel 2.
//
// 0: No effect.
// 1: Clear events from AON_RTC channel 2.
//
// Keep RTC_CH2_EV_CLR high until AUX_EVCTL:EVSTAT2.AON_RTC_CH2 and
// AUX_EVCTL:EVSTAT2.AON_RTC_CH2_DLY are 0.
#define AUX_SYSIF_RTCEVCLR_RTC_CH2_EV_CLR                           0x00000001
#define AUX_SYSIF_RTCEVCLR_RTC_CH2_EV_CLR_BITN                               0
#define AUX_SYSIF_RTCEVCLR_RTC_CH2_EV_CLR_M                         0x00000001
#define AUX_SYSIF_RTCEVCLR_RTC_CH2_EV_CLR_S                                  0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_BATMONBAT
//
//*****************************************************************************
// Field:  [10:8] INT
//
// See AON_BATMON:BAT.INT.
//
// Follow this procedure to get the correct value:
// - Do two dummy reads of INT.
// - Then read INT until two consecutive reads are equal.
#define AUX_SYSIF_BATMONBAT_INT_W                                            3
#define AUX_SYSIF_BATMONBAT_INT_M                                   0x00000700
#define AUX_SYSIF_BATMONBAT_INT_S                                            8

// Field:   [7:0] FRAC
//
// See AON_BATMON:BAT.FRAC.
//
// Follow this procedure to get the correct value:
// - Do two dummy reads of FRAC.
// - Then read FRAC until two consecutive reads are equal.
#define AUX_SYSIF_BATMONBAT_FRAC_W                                           8
#define AUX_SYSIF_BATMONBAT_FRAC_M                                  0x000000FF
#define AUX_SYSIF_BATMONBAT_FRAC_S                                           0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_BATMONTEMP
//
//*****************************************************************************
// Field: [15:11] SIGN
//
// Sign extension of INT.
//
// Follow this procedure to get the correct value:
// - Do two dummy reads of SIGN.
// - Then read SIGN until two consecutive reads are equal.
#define AUX_SYSIF_BATMONTEMP_SIGN_W                                          5
#define AUX_SYSIF_BATMONTEMP_SIGN_M                                 0x0000F800
#define AUX_SYSIF_BATMONTEMP_SIGN_S                                         11

// Field:  [10:2] INT
//
// See AON_BATMON:TEMP.INT.
//
// Follow this procedure to get the correct value:
// - Do two dummy reads of INT.
// - Then read INT until two consecutive reads are equal.
#define AUX_SYSIF_BATMONTEMP_INT_W                                           9
#define AUX_SYSIF_BATMONTEMP_INT_M                                  0x000007FC
#define AUX_SYSIF_BATMONTEMP_INT_S                                           2

// Field:   [1:0] FRAC
//
// See AON_BATMON:TEMP.FRAC.
//
// Follow this procedure to get the correct value:
// - Do two dummy reads of FRAC.
// - Then read FRAC until two consecutive reads are equal.
#define AUX_SYSIF_BATMONTEMP_FRAC_W                                          2
#define AUX_SYSIF_BATMONTEMP_FRAC_M                                 0x00000003
#define AUX_SYSIF_BATMONTEMP_FRAC_S                                          0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_TIMERHALT
//
//*****************************************************************************
// Field:     [3] PROGDLY
//
// Halt programmable delay.
//
// 0: AUX_EVCTL:PROGDLY.VALUE decrements as normal.
// 1: Halt AUX_EVCTL:PROGDLY.VALUE decrementation.
#define AUX_SYSIF_TIMERHALT_PROGDLY                                 0x00000008
#define AUX_SYSIF_TIMERHALT_PROGDLY_BITN                                     3
#define AUX_SYSIF_TIMERHALT_PROGDLY_M                               0x00000008
#define AUX_SYSIF_TIMERHALT_PROGDLY_S                                        3

// Field:     [2] AUX_TIMER2
//
// Halt AUX_TIMER2.
//
// 0: AUX_TIMER2 operates as normal.
// 1: Halt AUX_TIMER2 operation.
#define AUX_SYSIF_TIMERHALT_AUX_TIMER2                              0x00000004
#define AUX_SYSIF_TIMERHALT_AUX_TIMER2_BITN                                  2
#define AUX_SYSIF_TIMERHALT_AUX_TIMER2_M                            0x00000004
#define AUX_SYSIF_TIMERHALT_AUX_TIMER2_S                                     2

// Field:     [1] AUX_TIMER1
//
// Halt AUX_TIMER01 Timer 1.
//
// 0: AUX_TIMER01 Timer 1 operates as normal.
// 1: Halt AUX_TIMER01 Timer 1 operation.
#define AUX_SYSIF_TIMERHALT_AUX_TIMER1                              0x00000002
#define AUX_SYSIF_TIMERHALT_AUX_TIMER1_BITN                                  1
#define AUX_SYSIF_TIMERHALT_AUX_TIMER1_M                            0x00000002
#define AUX_SYSIF_TIMERHALT_AUX_TIMER1_S                                     1

// Field:     [0] AUX_TIMER0
//
// Halt AUX_TIMER01 Timer 0.
//
// 0: AUX_TIMER01 Timer 0 operates as normal.
// 1: Halt AUX_TIMER01 Timer 0 operation.
#define AUX_SYSIF_TIMERHALT_AUX_TIMER0                              0x00000001
#define AUX_SYSIF_TIMERHALT_AUX_TIMER0_BITN                                  0
#define AUX_SYSIF_TIMERHALT_AUX_TIMER0_M                            0x00000001
#define AUX_SYSIF_TIMERHALT_AUX_TIMER0_S                                     0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_TIMER2BRIDGE
//
//*****************************************************************************
// Field:     [0] BUSY
//
// Status of bus transactions to AUX_TIMER2.
//
// 0: No unfinished bus transactions.
// 1: A bus transaction is ongoing.
#define AUX_SYSIF_TIMER2BRIDGE_BUSY                                 0x00000001
#define AUX_SYSIF_TIMER2BRIDGE_BUSY_BITN                                     0
#define AUX_SYSIF_TIMER2BRIDGE_BUSY_M                               0x00000001
#define AUX_SYSIF_TIMER2BRIDGE_BUSY_S                                        0

//*****************************************************************************
//
// Register: AUX_SYSIF_O_SWPWRPROF
//
//*****************************************************************************
// Field:   [2:0] STAT
//
// Software status bits that can be read by the power profiler.
#define AUX_SYSIF_SWPWRPROF_STAT_W                                           3
#define AUX_SYSIF_SWPWRPROF_STAT_M                                  0x00000007
#define AUX_SYSIF_SWPWRPROF_STAT_S                                           0


#endif // __AUX_SYSIF__
