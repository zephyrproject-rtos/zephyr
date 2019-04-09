/******************************************************************************
*  Filename:       hw_aux_anaif_h
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

#ifndef __HW_AUX_ANAIF_H__
#define __HW_AUX_ANAIF_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_ANAIF component
//
//*****************************************************************************
// ADC Control
#define AUX_ANAIF_O_ADCCTL                                          0x00000010

// ADC FIFO Status
#define AUX_ANAIF_O_ADCFIFOSTAT                                     0x00000014

// ADC FIFO
#define AUX_ANAIF_O_ADCFIFO                                         0x00000018

// ADC Trigger
#define AUX_ANAIF_O_ADCTRIG                                         0x0000001C

// Current Source Control
#define AUX_ANAIF_O_ISRCCTL                                         0x00000020

// DAC Control
#define AUX_ANAIF_O_DACCTL                                          0x00000030

// Low Power Mode Bias Control
#define AUX_ANAIF_O_LPMBIASCTL                                      0x00000034

// DAC Sample Control
#define AUX_ANAIF_O_DACSMPLCTL                                      0x00000038

// DAC Sample Configuration 0
#define AUX_ANAIF_O_DACSMPLCFG0                                     0x0000003C

// DAC Sample Configuration 1
#define AUX_ANAIF_O_DACSMPLCFG1                                     0x00000040

// DAC Value
#define AUX_ANAIF_O_DACVALUE                                        0x00000044

// DAC Status
#define AUX_ANAIF_O_DACSTAT                                         0x00000048

//*****************************************************************************
//
// Register: AUX_ANAIF_O_ADCCTL
//
//*****************************************************************************
// Field:    [14] START_POL
//
// Select active polarity for START_SRC event.
// ENUMs:
// FALL                     Set ADC trigger on falling edge of event source.
// RISE                     Set ADC trigger on rising edge of event source.
#define AUX_ANAIF_ADCCTL_START_POL                                  0x00004000
#define AUX_ANAIF_ADCCTL_START_POL_BITN                                     14
#define AUX_ANAIF_ADCCTL_START_POL_M                                0x00004000
#define AUX_ANAIF_ADCCTL_START_POL_S                                        14
#define AUX_ANAIF_ADCCTL_START_POL_FALL                             0x00004000
#define AUX_ANAIF_ADCCTL_START_POL_RISE                             0x00000000

// Field:  [13:8] START_SRC
//
// Select ADC trigger event source from the asynchronous AUX event bus.
//
// Set START_SRC to NO_EVENT if you want to trigger the ADC manually through
// ADCTRIG.START.
//
// If you write a non-enumerated value the behavior is identical to NO_EVENT.
// The written value is returned when read.
// ENUMs:
// NO_EVENT                 No event.
// AUX_SMPH_AUTOTAKE_DONE   AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
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
#define AUX_ANAIF_ADCCTL_START_SRC_W                                         6
#define AUX_ANAIF_ADCCTL_START_SRC_M                                0x00003F00
#define AUX_ANAIF_ADCCTL_START_SRC_S                                         8
#define AUX_ANAIF_ADCCTL_START_SRC_NO_EVENT                         0x00003F00
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_SMPH_AUTOTAKE_DONE           0x00003D00
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_ISRC_RESET_N                 0x00003800
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_TDC_DONE                     0x00003700
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_TIMER0_EV                    0x00003600
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_TIMER1_EV                    0x00003500
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_TIMER2_PULSE                 0x00003400
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_TIMER2_EV3                   0x00003300
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_TIMER2_EV2                   0x00003200
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_TIMER2_EV1                   0x00003100
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_TIMER2_EV0                   0x00003000
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_COMPB                        0x00002F00
#define AUX_ANAIF_ADCCTL_START_SRC_AUX_COMPA                        0x00002E00
#define AUX_ANAIF_ADCCTL_START_SRC_MCU_EV                           0x00002B00
#define AUX_ANAIF_ADCCTL_START_SRC_ACLK_REF                         0x00002A00
#define AUX_ANAIF_ADCCTL_START_SRC_VDDR_RECHARGE                    0x00002900
#define AUX_ANAIF_ADCCTL_START_SRC_MCU_ACTIVE                       0x00002800
#define AUX_ANAIF_ADCCTL_START_SRC_PWR_DWN                          0x00002700
#define AUX_ANAIF_ADCCTL_START_SRC_SCLK_LF                          0x00002600
#define AUX_ANAIF_ADCCTL_START_SRC_AON_BATMON_TEMP_UPD              0x00002500
#define AUX_ANAIF_ADCCTL_START_SRC_AON_BATMON_BAT_UPD               0x00002400
#define AUX_ANAIF_ADCCTL_START_SRC_AON_RTC_4KHZ                     0x00002300
#define AUX_ANAIF_ADCCTL_START_SRC_AON_RTC_CH2_DLY                  0x00002200
#define AUX_ANAIF_ADCCTL_START_SRC_AON_RTC_CH2                      0x00002100
#define AUX_ANAIF_ADCCTL_START_SRC_MANUAL_EV                        0x00002000
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO31                          0x00001F00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO30                          0x00001E00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO29                          0x00001D00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO28                          0x00001C00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO27                          0x00001B00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO26                          0x00001A00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO25                          0x00001900
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO24                          0x00001800
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO23                          0x00001700
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO22                          0x00001600
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO21                          0x00001500
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO20                          0x00001400
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO19                          0x00001300
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO18                          0x00001200
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO17                          0x00001100
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO16                          0x00001000
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO15                          0x00000F00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO14                          0x00000E00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO13                          0x00000D00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO12                          0x00000C00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO11                          0x00000B00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO10                          0x00000A00
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO9                           0x00000900
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO8                           0x00000800
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO7                           0x00000700
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO6                           0x00000600
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO5                           0x00000500
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO4                           0x00000400
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO3                           0x00000300
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO2                           0x00000200
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO1                           0x00000100
#define AUX_ANAIF_ADCCTL_START_SRC_AUXIO0                           0x00000000

// Field:   [1:0] CMD
//
// ADC interface command.
//
// Non-enumerated values are not supported. The written value is returned when
// read.
// ENUMs:
// FLUSH                    Flush ADC FIFO.
//
//                          You must set CMD to EN or
//                          DIS after flush.
//
//                          System CPU must wait two
//                          clock cycles before it sets CMD to EN or DIS.
// EN                       Enable ADC interface.
// DIS                      Disable ADC interface.
#define AUX_ANAIF_ADCCTL_CMD_W                                               2
#define AUX_ANAIF_ADCCTL_CMD_M                                      0x00000003
#define AUX_ANAIF_ADCCTL_CMD_S                                               0
#define AUX_ANAIF_ADCCTL_CMD_FLUSH                                  0x00000003
#define AUX_ANAIF_ADCCTL_CMD_EN                                     0x00000001
#define AUX_ANAIF_ADCCTL_CMD_DIS                                    0x00000000

//*****************************************************************************
//
// Register: AUX_ANAIF_O_ADCFIFOSTAT
//
//*****************************************************************************
// Field:     [4] OVERFLOW
//
// FIFO overflow flag.
//
// 0: FIFO has not overflowed.
// 1: FIFO has overflowed, this flag is sticky until you flush the FIFO.
//
// When the flag is set, the ADC FIFO write pointer is static. It is not
// possible to add more samples to the ADC FIFO. Flush FIFO to clear the flag.
#define AUX_ANAIF_ADCFIFOSTAT_OVERFLOW                              0x00000010
#define AUX_ANAIF_ADCFIFOSTAT_OVERFLOW_BITN                                  4
#define AUX_ANAIF_ADCFIFOSTAT_OVERFLOW_M                            0x00000010
#define AUX_ANAIF_ADCFIFOSTAT_OVERFLOW_S                                     4

// Field:     [3] UNDERFLOW
//
// FIFO underflow flag.
//
// 0: FIFO has not underflowed.
// 1: FIFO has underflowed, this flag is sticky until you flush the FIFO.
//
// When the flag is set, the ADC FIFO read pointer is static. Read returns the
// previous sample that was read. Flush FIFO to clear the flag.
#define AUX_ANAIF_ADCFIFOSTAT_UNDERFLOW                             0x00000008
#define AUX_ANAIF_ADCFIFOSTAT_UNDERFLOW_BITN                                 3
#define AUX_ANAIF_ADCFIFOSTAT_UNDERFLOW_M                           0x00000008
#define AUX_ANAIF_ADCFIFOSTAT_UNDERFLOW_S                                    3

// Field:     [2] FULL
//
// FIFO full flag.
//
// 0: FIFO is not full, there is less than 4 samples in the FIFO.
// 1: FIFO is full, there are 4 samples in the FIFO.
//
// When the flag is set, it is not possible to add more samples to the ADC
// FIFO. An attempt to add samples sets the OVERFLOW flag.
#define AUX_ANAIF_ADCFIFOSTAT_FULL                                  0x00000004
#define AUX_ANAIF_ADCFIFOSTAT_FULL_BITN                                      2
#define AUX_ANAIF_ADCFIFOSTAT_FULL_M                                0x00000004
#define AUX_ANAIF_ADCFIFOSTAT_FULL_S                                         2

// Field:     [1] ALMOST_FULL
//
// FIFO almost full flag.
//
// 0: There are less than 3 samples in the FIFO, or the FIFO is full. The FULL
// flag is also asserted in the latter case.
// 1: There are 3 samples in the FIFO, there is room for one more sample.
#define AUX_ANAIF_ADCFIFOSTAT_ALMOST_FULL                           0x00000002
#define AUX_ANAIF_ADCFIFOSTAT_ALMOST_FULL_BITN                               1
#define AUX_ANAIF_ADCFIFOSTAT_ALMOST_FULL_M                         0x00000002
#define AUX_ANAIF_ADCFIFOSTAT_ALMOST_FULL_S                                  1

// Field:     [0] EMPTY
//
// FIFO empty flag.
//
// 0: FIFO contains one or more samples.
// 1: FIFO is empty.
//
// When the flag is set, read returns the previous sample that was read and
// sets the UNDERFLOW flag.
#define AUX_ANAIF_ADCFIFOSTAT_EMPTY                                 0x00000001
#define AUX_ANAIF_ADCFIFOSTAT_EMPTY_BITN                                     0
#define AUX_ANAIF_ADCFIFOSTAT_EMPTY_M                               0x00000001
#define AUX_ANAIF_ADCFIFOSTAT_EMPTY_S                                        0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_ADCFIFO
//
//*****************************************************************************
// Field:  [11:0] DATA
//
// FIFO data.
//
// Read:
// Get oldest ADC sample from FIFO.
//
// Write:
// Write dummy sample to FIFO. This is useful for code development when you do
// not have real ADC samples.
#define AUX_ANAIF_ADCFIFO_DATA_W                                            12
#define AUX_ANAIF_ADCFIFO_DATA_M                                    0x00000FFF
#define AUX_ANAIF_ADCFIFO_DATA_S                                             0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_ADCTRIG
//
//*****************************************************************************
// Field:     [0] START
//
// Manual ADC trigger.
//
// 0: No effect.
// 1: Single ADC trigger.
//
// To manually trigger the ADC, you must set ADCCTL.START_SRC to NO_EVENT to
// avoid conflict with event-driven ADC trigger.
#define AUX_ANAIF_ADCTRIG_START                                     0x00000001
#define AUX_ANAIF_ADCTRIG_START_BITN                                         0
#define AUX_ANAIF_ADCTRIG_START_M                                   0x00000001
#define AUX_ANAIF_ADCTRIG_START_S                                            0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_ISRCCTL
//
//*****************************************************************************
// Field:     [0] RESET_N
//
// ISRC reset control.
//
// 0: ISRC drives 0 uA.
// 1: ISRC drives current ADI_4_AUX:ISRC.TRIM to COMPA_IN.
#define AUX_ANAIF_ISRCCTL_RESET_N                                   0x00000001
#define AUX_ANAIF_ISRCCTL_RESET_N_BITN                                       0
#define AUX_ANAIF_ISRCCTL_RESET_N_M                                 0x00000001
#define AUX_ANAIF_ISRCCTL_RESET_N_S                                          0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_DACCTL
//
//*****************************************************************************
// Field:     [5] DAC_EN
//
// DAC module enable.
//
// 0: Disable DAC.
// 1: Enable DAC.
//
// The Sensor Controller must not use the DAC when AUX_SYSIF:OPMODEREQ.REQ
// equals PDA.
//
// The System CPU must not use the DAC when AUX_SYSIF:OPMODEREQ.REQ equals PDA
// in Standby TI-RTOS power mode. The System CPU must set
// AUX_SYSIF:PEROPRATE.ANAIF_DAC_OP_RATE to BUS_RATE to use the DAC in Active
// and Idle TI-RTOS power modes.
#define AUX_ANAIF_DACCTL_DAC_EN                                     0x00000020
#define AUX_ANAIF_DACCTL_DAC_EN_BITN                                         5
#define AUX_ANAIF_DACCTL_DAC_EN_M                                   0x00000020
#define AUX_ANAIF_DACCTL_DAC_EN_S                                            5

// Field:     [4] DAC_BUFFER_EN
//
// DAC buffer enable.
//
// DAC buffer reduces the time required to produce the programmed voltage at
// the expense of increased current consumption.
//
// 0: Disable DAC buffer.
// 1: Enable DAC buffer.
//
// Enable buffer when DAC_VOUT_SEL equals COMPA_IN.
//
// Do not enable the buffer when AUX_SYSIF:OPMODEREQ.REQ equals PDA or PDLP.
#define AUX_ANAIF_DACCTL_DAC_BUFFER_EN                              0x00000010
#define AUX_ANAIF_DACCTL_DAC_BUFFER_EN_BITN                                  4
#define AUX_ANAIF_DACCTL_DAC_BUFFER_EN_M                            0x00000010
#define AUX_ANAIF_DACCTL_DAC_BUFFER_EN_S                                     4

// Field:     [3] DAC_PRECHARGE_EN
//
// DAC precharge enable.
//
// Only enable precharge when ADI_4_AUX:MUX2.DAC_VREF_SEL equals DCOUPL and
// VDDS is higher than 2.65 V.
//
// DAC output voltage range:
//
// 0: 0 V to 1.28 V.
// 1: 1.28 V to 2.56 V.
//
// Otherwise, see ADI_4_AUX:MUX2.DAC_VREF_SEL for DAC output voltage range.
//
// Enable precharge 1 us before you enable the DAC and the buffer.
#define AUX_ANAIF_DACCTL_DAC_PRECHARGE_EN                           0x00000008
#define AUX_ANAIF_DACCTL_DAC_PRECHARGE_EN_BITN                               3
#define AUX_ANAIF_DACCTL_DAC_PRECHARGE_EN_M                         0x00000008
#define AUX_ANAIF_DACCTL_DAC_PRECHARGE_EN_S                                  3

// Field:   [2:0] DAC_VOUT_SEL
//
// DAC output connection.
//
// An analog node must only have one driver. Other drivers for the following
// analog nodes are configured in [ANATOP_MMAP::ADI_4_AUX:*].
// ENUMs:
// COMPA_IN                 Connect to COMPA_IN analog node.
//
//                          Required setting to drive
//                          external load selected in
//                          ADI_4_AUX:MUX1.COMPA_IN.
// COMPA_REF                Connect to COMPA_REF analog node.
//
//                          It is not possible to
//                          drive external loads connected to COMPA_REF I/O
//                          mux with this setting.
// COMPB_REF                Connect to COMPB_REF analog node.
//
//                          Required setting to use
//                          Comparator B.
// NC                       Connect to nothing
//
//                          It is recommended to use
//                          NC as intermediate step when you change
//                          DAC_VOUT_SEL.
#define AUX_ANAIF_DACCTL_DAC_VOUT_SEL_W                                      3
#define AUX_ANAIF_DACCTL_DAC_VOUT_SEL_M                             0x00000007
#define AUX_ANAIF_DACCTL_DAC_VOUT_SEL_S                                      0
#define AUX_ANAIF_DACCTL_DAC_VOUT_SEL_COMPA_IN                      0x00000004
#define AUX_ANAIF_DACCTL_DAC_VOUT_SEL_COMPA_REF                     0x00000002
#define AUX_ANAIF_DACCTL_DAC_VOUT_SEL_COMPB_REF                     0x00000001
#define AUX_ANAIF_DACCTL_DAC_VOUT_SEL_NC                            0x00000000

//*****************************************************************************
//
// Register: AUX_ANAIF_O_LPMBIASCTL
//
//*****************************************************************************
// Field:     [0] EN
//
// Module enable.
//
// 0: Disable low power mode bias module.
// 1: Enable low power mode bias module.
//
// Set EN to 1 15 us before you enable the DAC or Comparator A.
#define AUX_ANAIF_LPMBIASCTL_EN                                     0x00000001
#define AUX_ANAIF_LPMBIASCTL_EN_BITN                                         0
#define AUX_ANAIF_LPMBIASCTL_EN_M                                   0x00000001
#define AUX_ANAIF_LPMBIASCTL_EN_S                                            0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_DACSMPLCTL
//
//*****************************************************************************
// Field:     [0] EN
//
// DAC sample clock enable.
//
// 0: Disable sample clock. The sample clock stops low and DACSTAT becomes 0
// when the current sample clock period completes.
// 1: Enable DAC sample clock. DACSTAT must be 0 before you enable sample
// clock.
#define AUX_ANAIF_DACSMPLCTL_EN                                     0x00000001
#define AUX_ANAIF_DACSMPLCTL_EN_BITN                                         0
#define AUX_ANAIF_DACSMPLCTL_EN_M                                   0x00000001
#define AUX_ANAIF_DACSMPLCTL_EN_S                                            0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_DACSMPLCFG0
//
//*****************************************************************************
// Field:   [5:0] CLKDIV
//
// Clock division.
//
// AUX_SYSIF:PEROPRATE.ANAIF_DAC_OP_RATE divided by (CLKDIV + 1) determines the
// sample clock base frequency.
//
// 0: Divide by 1.
// 1: Divide by 2.
// ...
// 63: Divide by 64.
#define AUX_ANAIF_DACSMPLCFG0_CLKDIV_W                                       6
#define AUX_ANAIF_DACSMPLCFG0_CLKDIV_M                              0x0000003F
#define AUX_ANAIF_DACSMPLCFG0_CLKDIV_S                                       0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_DACSMPLCFG1
//
//*****************************************************************************
// Field:    [14] H_PER
//
// High time.
//
// The sample clock period is high for this many base periods.
//
// 0: 2 periods
// 1: 4 periods
#define AUX_ANAIF_DACSMPLCFG1_H_PER                                 0x00004000
#define AUX_ANAIF_DACSMPLCFG1_H_PER_BITN                                    14
#define AUX_ANAIF_DACSMPLCFG1_H_PER_M                               0x00004000
#define AUX_ANAIF_DACSMPLCFG1_H_PER_S                                       14

// Field: [13:12] L_PER
//
// Low time.
//
// The sample clock period is low for this many base periods.
//
// 0: 1 period
// 1: 2 periods
// 2: 3 periods
// 3: 4 periods
#define AUX_ANAIF_DACSMPLCFG1_L_PER_W                                        2
#define AUX_ANAIF_DACSMPLCFG1_L_PER_M                               0x00003000
#define AUX_ANAIF_DACSMPLCFG1_L_PER_S                                       12

// Field:  [11:8] SETUP_CNT
//
// Setup count.
//
// Number of active sample clock periods during the setup phase.
//
// 0: 1 sample clock period
// 1: 2 sample clock periods
// ...
// 15 : 16 sample clock periods
#define AUX_ANAIF_DACSMPLCFG1_SETUP_CNT_W                                    4
#define AUX_ANAIF_DACSMPLCFG1_SETUP_CNT_M                           0x00000F00
#define AUX_ANAIF_DACSMPLCFG1_SETUP_CNT_S                                    8

// Field:   [7:0] HOLD_INTERVAL
//
// Hold interval.
//
// Number of inactive sample clock periods between each active sample clock
// period during hold phase. The sample clock is low when inactive.
//
// The range is 0 to 255.
#define AUX_ANAIF_DACSMPLCFG1_HOLD_INTERVAL_W                                8
#define AUX_ANAIF_DACSMPLCFG1_HOLD_INTERVAL_M                       0x000000FF
#define AUX_ANAIF_DACSMPLCFG1_HOLD_INTERVAL_S                                0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_DACVALUE
//
//*****************************************************************************
// Field:   [7:0] VALUE
//
// DAC value.
//
// Digital data word for the DAC.
//
// Only change VALUE when DACCTL.DAC_EN is 0. Then wait 1 us before you enable
// the DAC.
#define AUX_ANAIF_DACVALUE_VALUE_W                                           8
#define AUX_ANAIF_DACVALUE_VALUE_M                                  0x000000FF
#define AUX_ANAIF_DACVALUE_VALUE_S                                           0

//*****************************************************************************
//
// Register: AUX_ANAIF_O_DACSTAT
//
//*****************************************************************************
// Field:     [1] SETUP_ACTIVE
//
// DAC setup phase status.
//
// 0: Sample clock is disabled or setup phase is complete.
// 1: Setup phase in progress.
#define AUX_ANAIF_DACSTAT_SETUP_ACTIVE                              0x00000002
#define AUX_ANAIF_DACSTAT_SETUP_ACTIVE_BITN                                  1
#define AUX_ANAIF_DACSTAT_SETUP_ACTIVE_M                            0x00000002
#define AUX_ANAIF_DACSTAT_SETUP_ACTIVE_S                                     1

// Field:     [0] HOLD_ACTIVE
//
// DAC hold phase status.
//
// 0: Sample clock is disabled or DAC is not in hold phase.
// 1: Hold phase in progress.
#define AUX_ANAIF_DACSTAT_HOLD_ACTIVE                               0x00000001
#define AUX_ANAIF_DACSTAT_HOLD_ACTIVE_BITN                                   0
#define AUX_ANAIF_DACSTAT_HOLD_ACTIVE_M                             0x00000001
#define AUX_ANAIF_DACSTAT_HOLD_ACTIVE_S                                      0


#endif // __AUX_ANAIF__
