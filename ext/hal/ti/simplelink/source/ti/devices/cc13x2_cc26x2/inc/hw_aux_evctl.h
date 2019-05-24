/******************************************************************************
*  Filename:       hw_aux_evctl_h
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

#ifndef __HW_AUX_EVCTL_H__
#define __HW_AUX_EVCTL_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_EVCTL component
//
//*****************************************************************************
// Event Status 0
#define AUX_EVCTL_O_EVSTAT0                                         0x00000000

// Event Status 1
#define AUX_EVCTL_O_EVSTAT1                                         0x00000004

// Event Status 2
#define AUX_EVCTL_O_EVSTAT2                                         0x00000008

// Event Status 3
#define AUX_EVCTL_O_EVSTAT3                                         0x0000000C

// Sensor Controller Engine Wait Event Configuration 0
#define AUX_EVCTL_O_SCEWEVCFG0                                      0x00000010

// Sensor Controller Engine Wait Event Configuration 1
#define AUX_EVCTL_O_SCEWEVCFG1                                      0x00000014

// Direct Memory Access Control
#define AUX_EVCTL_O_DMACTL                                          0x00000018

// Software Event Set
#define AUX_EVCTL_O_SWEVSET                                         0x00000020

// Events To AON Flags
#define AUX_EVCTL_O_EVTOAONFLAGS                                    0x00000024

// Events To AON Polarity
#define AUX_EVCTL_O_EVTOAONPOL                                      0x00000028

// Events To AON Clear
#define AUX_EVCTL_O_EVTOAONFLAGSCLR                                 0x0000002C

// Events to MCU Flags
#define AUX_EVCTL_O_EVTOMCUFLAGS                                    0x00000030

// Event To MCU Polarity
#define AUX_EVCTL_O_EVTOMCUPOL                                      0x00000034

// Events To MCU Flags Clear
#define AUX_EVCTL_O_EVTOMCUFLAGSCLR                                 0x00000038

// Combined Event To MCU Mask
#define AUX_EVCTL_O_COMBEVTOMCUMASK                                 0x0000003C

// Event Observation Configuration
#define AUX_EVCTL_O_EVOBSCFG                                        0x00000040

// Programmable Delay
#define AUX_EVCTL_O_PROGDLY                                         0x00000044

// Manual
#define AUX_EVCTL_O_MANUAL                                          0x00000048

// Event Status 0 Low
#define AUX_EVCTL_O_EVSTAT0L                                        0x0000004C

// Event Status 0 High
#define AUX_EVCTL_O_EVSTAT0H                                        0x00000050

// Event Status 1 Low
#define AUX_EVCTL_O_EVSTAT1L                                        0x00000054

// Event Status 1 High
#define AUX_EVCTL_O_EVSTAT1H                                        0x00000058

// Event Status 2 Low
#define AUX_EVCTL_O_EVSTAT2L                                        0x0000005C

// Event Status 2 High
#define AUX_EVCTL_O_EVSTAT2H                                        0x00000060

// Event Status 3 Low
#define AUX_EVCTL_O_EVSTAT3L                                        0x00000064

// Event Status 3 High
#define AUX_EVCTL_O_EVSTAT3H                                        0x00000068

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT0
//
//*****************************************************************************
// Field:    [15] AUXIO15
//
// AUXIO15 pin level, read value corresponds to AUX_AIODIO1:GPIODIN bit 7.
#define AUX_EVCTL_EVSTAT0_AUXIO15                                   0x00008000
#define AUX_EVCTL_EVSTAT0_AUXIO15_BITN                                      15
#define AUX_EVCTL_EVSTAT0_AUXIO15_M                                 0x00008000
#define AUX_EVCTL_EVSTAT0_AUXIO15_S                                         15

// Field:    [14] AUXIO14
//
// AUXIO14 pin level, read value corresponds to AUX_AIODIO1:GPIODIN bit 6.
#define AUX_EVCTL_EVSTAT0_AUXIO14                                   0x00004000
#define AUX_EVCTL_EVSTAT0_AUXIO14_BITN                                      14
#define AUX_EVCTL_EVSTAT0_AUXIO14_M                                 0x00004000
#define AUX_EVCTL_EVSTAT0_AUXIO14_S                                         14

// Field:    [13] AUXIO13
//
// AUXIO13 pin level, read value corresponds to AUX_AIODIO1:GPIODIN bit 5.
#define AUX_EVCTL_EVSTAT0_AUXIO13                                   0x00002000
#define AUX_EVCTL_EVSTAT0_AUXIO13_BITN                                      13
#define AUX_EVCTL_EVSTAT0_AUXIO13_M                                 0x00002000
#define AUX_EVCTL_EVSTAT0_AUXIO13_S                                         13

// Field:    [12] AUXIO12
//
// AUXIO12 pin level, read value corresponds to AUX_AIODIO1:GPIODIN bit 4.
#define AUX_EVCTL_EVSTAT0_AUXIO12                                   0x00001000
#define AUX_EVCTL_EVSTAT0_AUXIO12_BITN                                      12
#define AUX_EVCTL_EVSTAT0_AUXIO12_M                                 0x00001000
#define AUX_EVCTL_EVSTAT0_AUXIO12_S                                         12

// Field:    [11] AUXIO11
//
// AUXIO11 pin level, read value corresponds to AUX_AIODIO1:GPIODIN bit 3.
#define AUX_EVCTL_EVSTAT0_AUXIO11                                   0x00000800
#define AUX_EVCTL_EVSTAT0_AUXIO11_BITN                                      11
#define AUX_EVCTL_EVSTAT0_AUXIO11_M                                 0x00000800
#define AUX_EVCTL_EVSTAT0_AUXIO11_S                                         11

// Field:    [10] AUXIO10
//
// AUXIO10 pin level, read value corresponds to AUX_AIODIO1:GPIODIN bit 2.
#define AUX_EVCTL_EVSTAT0_AUXIO10                                   0x00000400
#define AUX_EVCTL_EVSTAT0_AUXIO10_BITN                                      10
#define AUX_EVCTL_EVSTAT0_AUXIO10_M                                 0x00000400
#define AUX_EVCTL_EVSTAT0_AUXIO10_S                                         10

// Field:     [9] AUXIO9
//
// AUXIO9   pin level, read value corresponds to AUX_AIODIO1:GPIODIN bit 1.
#define AUX_EVCTL_EVSTAT0_AUXIO9                                    0x00000200
#define AUX_EVCTL_EVSTAT0_AUXIO9_BITN                                        9
#define AUX_EVCTL_EVSTAT0_AUXIO9_M                                  0x00000200
#define AUX_EVCTL_EVSTAT0_AUXIO9_S                                           9

// Field:     [8] AUXIO8
//
// AUXIO8   pin level, read value corresponds to AUX_AIODIO1:GPIODIN bit 0.
#define AUX_EVCTL_EVSTAT0_AUXIO8                                    0x00000100
#define AUX_EVCTL_EVSTAT0_AUXIO8_BITN                                        8
#define AUX_EVCTL_EVSTAT0_AUXIO8_M                                  0x00000100
#define AUX_EVCTL_EVSTAT0_AUXIO8_S                                           8

// Field:     [7] AUXIO7
//
// AUXIO7   pin level, read value corresponds to AUX_AIODIO0:GPIODIN bit 7.
#define AUX_EVCTL_EVSTAT0_AUXIO7                                    0x00000080
#define AUX_EVCTL_EVSTAT0_AUXIO7_BITN                                        7
#define AUX_EVCTL_EVSTAT0_AUXIO7_M                                  0x00000080
#define AUX_EVCTL_EVSTAT0_AUXIO7_S                                           7

// Field:     [6] AUXIO6
//
// AUXIO6   pin level, read value corresponds to AUX_AIODIO0:GPIODIN bit 6.
#define AUX_EVCTL_EVSTAT0_AUXIO6                                    0x00000040
#define AUX_EVCTL_EVSTAT0_AUXIO6_BITN                                        6
#define AUX_EVCTL_EVSTAT0_AUXIO6_M                                  0x00000040
#define AUX_EVCTL_EVSTAT0_AUXIO6_S                                           6

// Field:     [5] AUXIO5
//
// AUXIO5   pin level, read value corresponds to AUX_AIODIO0:GPIODIN bit 5.
#define AUX_EVCTL_EVSTAT0_AUXIO5                                    0x00000020
#define AUX_EVCTL_EVSTAT0_AUXIO5_BITN                                        5
#define AUX_EVCTL_EVSTAT0_AUXIO5_M                                  0x00000020
#define AUX_EVCTL_EVSTAT0_AUXIO5_S                                           5

// Field:     [4] AUXIO4
//
// AUXIO4   pin level, read value corresponds to AUX_AIODIO0:GPIODIN bit 4.
#define AUX_EVCTL_EVSTAT0_AUXIO4                                    0x00000010
#define AUX_EVCTL_EVSTAT0_AUXIO4_BITN                                        4
#define AUX_EVCTL_EVSTAT0_AUXIO4_M                                  0x00000010
#define AUX_EVCTL_EVSTAT0_AUXIO4_S                                           4

// Field:     [3] AUXIO3
//
// AUXIO3   pin level, read value corresponds to AUX_AIODIO0:GPIODIN bit 3.
#define AUX_EVCTL_EVSTAT0_AUXIO3                                    0x00000008
#define AUX_EVCTL_EVSTAT0_AUXIO3_BITN                                        3
#define AUX_EVCTL_EVSTAT0_AUXIO3_M                                  0x00000008
#define AUX_EVCTL_EVSTAT0_AUXIO3_S                                           3

// Field:     [2] AUXIO2
//
// AUXIO2   pin level, read value corresponds to AUX_AIODIO0:GPIODIN bit 2.
#define AUX_EVCTL_EVSTAT0_AUXIO2                                    0x00000004
#define AUX_EVCTL_EVSTAT0_AUXIO2_BITN                                        2
#define AUX_EVCTL_EVSTAT0_AUXIO2_M                                  0x00000004
#define AUX_EVCTL_EVSTAT0_AUXIO2_S                                           2

// Field:     [1] AUXIO1
//
// AUXIO1   pin level, read value corresponds to AUX_AIODIO0:GPIODIN bit 1.
#define AUX_EVCTL_EVSTAT0_AUXIO1                                    0x00000002
#define AUX_EVCTL_EVSTAT0_AUXIO1_BITN                                        1
#define AUX_EVCTL_EVSTAT0_AUXIO1_M                                  0x00000002
#define AUX_EVCTL_EVSTAT0_AUXIO1_S                                           1

// Field:     [0] AUXIO0
//
// AUXIO0   pin level, read value corresponds to AUX_AIODIO0:GPIODIN bit 0.
#define AUX_EVCTL_EVSTAT0_AUXIO0                                    0x00000001
#define AUX_EVCTL_EVSTAT0_AUXIO0_BITN                                        0
#define AUX_EVCTL_EVSTAT0_AUXIO0_M                                  0x00000001
#define AUX_EVCTL_EVSTAT0_AUXIO0_S                                           0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT1
//
//*****************************************************************************
// Field:    [15] AUXIO31
//
// AUXIO31 pin level, read value corresponds to AUX_AIODIO3:GPIODIN bit 7.
#define AUX_EVCTL_EVSTAT1_AUXIO31                                   0x00008000
#define AUX_EVCTL_EVSTAT1_AUXIO31_BITN                                      15
#define AUX_EVCTL_EVSTAT1_AUXIO31_M                                 0x00008000
#define AUX_EVCTL_EVSTAT1_AUXIO31_S                                         15

// Field:    [14] AUXIO30
//
// AUXIO30 pin level, read value corresponds to AUX_AIODIO3:GPIODIN bit 6.
#define AUX_EVCTL_EVSTAT1_AUXIO30                                   0x00004000
#define AUX_EVCTL_EVSTAT1_AUXIO30_BITN                                      14
#define AUX_EVCTL_EVSTAT1_AUXIO30_M                                 0x00004000
#define AUX_EVCTL_EVSTAT1_AUXIO30_S                                         14

// Field:    [13] AUXIO29
//
// AUXIO29 pin level, read value corresponds to AUX_AIODIO3:GPIODIN bit 5.
#define AUX_EVCTL_EVSTAT1_AUXIO29                                   0x00002000
#define AUX_EVCTL_EVSTAT1_AUXIO29_BITN                                      13
#define AUX_EVCTL_EVSTAT1_AUXIO29_M                                 0x00002000
#define AUX_EVCTL_EVSTAT1_AUXIO29_S                                         13

// Field:    [12] AUXIO28
//
// AUXIO28 pin level, read value corresponds to AUX_AIODIO3:GPIODIN bit 4.
#define AUX_EVCTL_EVSTAT1_AUXIO28                                   0x00001000
#define AUX_EVCTL_EVSTAT1_AUXIO28_BITN                                      12
#define AUX_EVCTL_EVSTAT1_AUXIO28_M                                 0x00001000
#define AUX_EVCTL_EVSTAT1_AUXIO28_S                                         12

// Field:    [11] AUXIO27
//
// AUXIO27 pin level, read value corresponds to AUX_AIODIO3:GPIODIN bit 3.
#define AUX_EVCTL_EVSTAT1_AUXIO27                                   0x00000800
#define AUX_EVCTL_EVSTAT1_AUXIO27_BITN                                      11
#define AUX_EVCTL_EVSTAT1_AUXIO27_M                                 0x00000800
#define AUX_EVCTL_EVSTAT1_AUXIO27_S                                         11

// Field:    [10] AUXIO26
//
// AUXIO26 pin level, read value corresponds to AUX_AIODIO3:GPIODIN bit 2.
#define AUX_EVCTL_EVSTAT1_AUXIO26                                   0x00000400
#define AUX_EVCTL_EVSTAT1_AUXIO26_BITN                                      10
#define AUX_EVCTL_EVSTAT1_AUXIO26_M                                 0x00000400
#define AUX_EVCTL_EVSTAT1_AUXIO26_S                                         10

// Field:     [9] AUXIO25
//
// AUXIO25 pin level, read value corresponds to AUX_AIODIO3:GPIODIN bit 1.
#define AUX_EVCTL_EVSTAT1_AUXIO25                                   0x00000200
#define AUX_EVCTL_EVSTAT1_AUXIO25_BITN                                       9
#define AUX_EVCTL_EVSTAT1_AUXIO25_M                                 0x00000200
#define AUX_EVCTL_EVSTAT1_AUXIO25_S                                          9

// Field:     [8] AUXIO24
//
// AUXIO24 pin level, read value corresponds to AUX_AIODIO3:GPIODIN bit 0.
#define AUX_EVCTL_EVSTAT1_AUXIO24                                   0x00000100
#define AUX_EVCTL_EVSTAT1_AUXIO24_BITN                                       8
#define AUX_EVCTL_EVSTAT1_AUXIO24_M                                 0x00000100
#define AUX_EVCTL_EVSTAT1_AUXIO24_S                                          8

// Field:     [7] AUXIO23
//
// AUXIO23 pin level, read value corresponds to AUX_AIODIO2:GPIODIN bit 7.
#define AUX_EVCTL_EVSTAT1_AUXIO23                                   0x00000080
#define AUX_EVCTL_EVSTAT1_AUXIO23_BITN                                       7
#define AUX_EVCTL_EVSTAT1_AUXIO23_M                                 0x00000080
#define AUX_EVCTL_EVSTAT1_AUXIO23_S                                          7

// Field:     [6] AUXIO22
//
// AUXIO22 pin level, read value corresponds to AUX_AIODIO2:GPIODIN bit 6.
#define AUX_EVCTL_EVSTAT1_AUXIO22                                   0x00000040
#define AUX_EVCTL_EVSTAT1_AUXIO22_BITN                                       6
#define AUX_EVCTL_EVSTAT1_AUXIO22_M                                 0x00000040
#define AUX_EVCTL_EVSTAT1_AUXIO22_S                                          6

// Field:     [5] AUXIO21
//
// AUXIO21 pin level, read value corresponds to AUX_AIODIO2:GPIODIN bit 5.
#define AUX_EVCTL_EVSTAT1_AUXIO21                                   0x00000020
#define AUX_EVCTL_EVSTAT1_AUXIO21_BITN                                       5
#define AUX_EVCTL_EVSTAT1_AUXIO21_M                                 0x00000020
#define AUX_EVCTL_EVSTAT1_AUXIO21_S                                          5

// Field:     [4] AUXIO20
//
// AUXIO20 pin level, read value corresponds to AUX_AIODIO2:GPIODIN bit 4.
#define AUX_EVCTL_EVSTAT1_AUXIO20                                   0x00000010
#define AUX_EVCTL_EVSTAT1_AUXIO20_BITN                                       4
#define AUX_EVCTL_EVSTAT1_AUXIO20_M                                 0x00000010
#define AUX_EVCTL_EVSTAT1_AUXIO20_S                                          4

// Field:     [3] AUXIO19
//
// AUXIO19 pin level, read value corresponds to AUX_AIODIO2:GPIODIN bit 3.
#define AUX_EVCTL_EVSTAT1_AUXIO19                                   0x00000008
#define AUX_EVCTL_EVSTAT1_AUXIO19_BITN                                       3
#define AUX_EVCTL_EVSTAT1_AUXIO19_M                                 0x00000008
#define AUX_EVCTL_EVSTAT1_AUXIO19_S                                          3

// Field:     [2] AUXIO18
//
// AUXIO18 pin level, read value corresponds to AUX_AIODIO2:GPIODIN bit 2.
#define AUX_EVCTL_EVSTAT1_AUXIO18                                   0x00000004
#define AUX_EVCTL_EVSTAT1_AUXIO18_BITN                                       2
#define AUX_EVCTL_EVSTAT1_AUXIO18_M                                 0x00000004
#define AUX_EVCTL_EVSTAT1_AUXIO18_S                                          2

// Field:     [1] AUXIO17
//
// AUXIO17 pin level, read value corresponds to AUX_AIODIO2:GPIODIN bit 1.
#define AUX_EVCTL_EVSTAT1_AUXIO17                                   0x00000002
#define AUX_EVCTL_EVSTAT1_AUXIO17_BITN                                       1
#define AUX_EVCTL_EVSTAT1_AUXIO17_M                                 0x00000002
#define AUX_EVCTL_EVSTAT1_AUXIO17_S                                          1

// Field:     [0] AUXIO16
//
// AUXIO16 pin level, read value corresponds to AUX_AIODIO2:GPIODIN bit 0.
#define AUX_EVCTL_EVSTAT1_AUXIO16                                   0x00000001
#define AUX_EVCTL_EVSTAT1_AUXIO16_BITN                                       0
#define AUX_EVCTL_EVSTAT1_AUXIO16_M                                 0x00000001
#define AUX_EVCTL_EVSTAT1_AUXIO16_S                                          0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT2
//
//*****************************************************************************
// Field:    [15] AUX_COMPB
//
// Comparator B output.
// Configuration of AUX_SYSIF:EVSYNCRATE.AUX_COMPB_SYNC_RATE sets the
// synchronization rate for this event.
#define AUX_EVCTL_EVSTAT2_AUX_COMPB                                 0x00008000
#define AUX_EVCTL_EVSTAT2_AUX_COMPB_BITN                                    15
#define AUX_EVCTL_EVSTAT2_AUX_COMPB_M                               0x00008000
#define AUX_EVCTL_EVSTAT2_AUX_COMPB_S                                       15

// Field:    [14] AUX_COMPA
//
// Comparator A output.
// Configuration of AUX_SYSIF:EVSYNCRATE.AUX_COMPA_SYNC_RATE sets the
// synchronization rate for this event.
#define AUX_EVCTL_EVSTAT2_AUX_COMPA                                 0x00004000
#define AUX_EVCTL_EVSTAT2_AUX_COMPA_BITN                                    14
#define AUX_EVCTL_EVSTAT2_AUX_COMPA_M                               0x00004000
#define AUX_EVCTL_EVSTAT2_AUX_COMPA_S                                       14

// Field:    [13] MCU_OBSMUX1
//
// Observation input 1 from IOC.
// This event is configured by IOC:OBSAUXOUTPUT.SEL1.
#define AUX_EVCTL_EVSTAT2_MCU_OBSMUX1                               0x00002000
#define AUX_EVCTL_EVSTAT2_MCU_OBSMUX1_BITN                                  13
#define AUX_EVCTL_EVSTAT2_MCU_OBSMUX1_M                             0x00002000
#define AUX_EVCTL_EVSTAT2_MCU_OBSMUX1_S                                     13

// Field:    [12] MCU_OBSMUX0
//
// Observation input 0 from IOC.
// This event is configured by IOC:OBSAUXOUTPUT.SEL0 and can be overridden by
// IOC:OBSAUXOUTPUT.SEL_MISC.
#define AUX_EVCTL_EVSTAT2_MCU_OBSMUX0                               0x00001000
#define AUX_EVCTL_EVSTAT2_MCU_OBSMUX0_BITN                                  12
#define AUX_EVCTL_EVSTAT2_MCU_OBSMUX0_M                             0x00001000
#define AUX_EVCTL_EVSTAT2_MCU_OBSMUX0_S                                     12

// Field:    [11] MCU_EV
//
// Event from EVENT configured by EVENT:AUXSEL0.
#define AUX_EVCTL_EVSTAT2_MCU_EV                                    0x00000800
#define AUX_EVCTL_EVSTAT2_MCU_EV_BITN                                       11
#define AUX_EVCTL_EVSTAT2_MCU_EV_M                                  0x00000800
#define AUX_EVCTL_EVSTAT2_MCU_EV_S                                          11

// Field:    [10] ACLK_REF
//
// TDC reference clock.
// It is configured by DDI_0_OSC:CTL0.ACLK_REF_SRC_SEL and enabled by
// AUX_SYSIF:TDCREFCLKCTL.REQ.
#define AUX_EVCTL_EVSTAT2_ACLK_REF                                  0x00000400
#define AUX_EVCTL_EVSTAT2_ACLK_REF_BITN                                     10
#define AUX_EVCTL_EVSTAT2_ACLK_REF_M                                0x00000400
#define AUX_EVCTL_EVSTAT2_ACLK_REF_S                                        10

// Field:     [9] VDDR_RECHARGE
//
// Event is high during VDDR recharge.
#define AUX_EVCTL_EVSTAT2_VDDR_RECHARGE                             0x00000200
#define AUX_EVCTL_EVSTAT2_VDDR_RECHARGE_BITN                                 9
#define AUX_EVCTL_EVSTAT2_VDDR_RECHARGE_M                           0x00000200
#define AUX_EVCTL_EVSTAT2_VDDR_RECHARGE_S                                    9

// Field:     [8] MCU_ACTIVE
//
// Event is high while system(MCU, AUX, or JTAG domains) is active or
// transitions to active (GLDO or DCDC power supply state). Event is not high
// during VDDR recharge.
#define AUX_EVCTL_EVSTAT2_MCU_ACTIVE                                0x00000100
#define AUX_EVCTL_EVSTAT2_MCU_ACTIVE_BITN                                    8
#define AUX_EVCTL_EVSTAT2_MCU_ACTIVE_M                              0x00000100
#define AUX_EVCTL_EVSTAT2_MCU_ACTIVE_S                                       8

// Field:     [7] PWR_DWN
//
// Event is high while system(MCU, AUX, or JTAG domains) is in powerdown (uLDO
// power supply).
#define AUX_EVCTL_EVSTAT2_PWR_DWN                                   0x00000080
#define AUX_EVCTL_EVSTAT2_PWR_DWN_BITN                                       7
#define AUX_EVCTL_EVSTAT2_PWR_DWN_M                                 0x00000080
#define AUX_EVCTL_EVSTAT2_PWR_DWN_S                                          7

// Field:     [6] SCLK_LF
//
// SCLK_LF clock
#define AUX_EVCTL_EVSTAT2_SCLK_LF                                   0x00000040
#define AUX_EVCTL_EVSTAT2_SCLK_LF_BITN                                       6
#define AUX_EVCTL_EVSTAT2_SCLK_LF_M                                 0x00000040
#define AUX_EVCTL_EVSTAT2_SCLK_LF_S                                          6

// Field:     [5] AON_BATMON_TEMP_UPD
//
// Event is high for two SCLK_MF clock periods when there is an update of
// AON_BATMON:TEMP.
#define AUX_EVCTL_EVSTAT2_AON_BATMON_TEMP_UPD                       0x00000020
#define AUX_EVCTL_EVSTAT2_AON_BATMON_TEMP_UPD_BITN                           5
#define AUX_EVCTL_EVSTAT2_AON_BATMON_TEMP_UPD_M                     0x00000020
#define AUX_EVCTL_EVSTAT2_AON_BATMON_TEMP_UPD_S                              5

// Field:     [4] AON_BATMON_BAT_UPD
//
// Event is high for two SCLK_MF clock periods when there is an update of
// AON_BATMON:BAT.
#define AUX_EVCTL_EVSTAT2_AON_BATMON_BAT_UPD                        0x00000010
#define AUX_EVCTL_EVSTAT2_AON_BATMON_BAT_UPD_BITN                            4
#define AUX_EVCTL_EVSTAT2_AON_BATMON_BAT_UPD_M                      0x00000010
#define AUX_EVCTL_EVSTAT2_AON_BATMON_BAT_UPD_S                               4

// Field:     [3] AON_RTC_4KHZ
//
// AON_RTC:SUBSEC.VALUE bit 19.
// AON_RTC:CTL.RTC_4KHZ_EN enables this event.
#define AUX_EVCTL_EVSTAT2_AON_RTC_4KHZ                              0x00000008
#define AUX_EVCTL_EVSTAT2_AON_RTC_4KHZ_BITN                                  3
#define AUX_EVCTL_EVSTAT2_AON_RTC_4KHZ_M                            0x00000008
#define AUX_EVCTL_EVSTAT2_AON_RTC_4KHZ_S                                     3

// Field:     [2] AON_RTC_CH2_DLY
//
// AON_RTC:EVFLAGS.CH2 delayed by AON_RTC:CTL.EV_DELAY configuration.
#define AUX_EVCTL_EVSTAT2_AON_RTC_CH2_DLY                           0x00000004
#define AUX_EVCTL_EVSTAT2_AON_RTC_CH2_DLY_BITN                               2
#define AUX_EVCTL_EVSTAT2_AON_RTC_CH2_DLY_M                         0x00000004
#define AUX_EVCTL_EVSTAT2_AON_RTC_CH2_DLY_S                                  2

// Field:     [1] AON_RTC_CH2
//
// AON_RTC:EVFLAGS.CH2.
#define AUX_EVCTL_EVSTAT2_AON_RTC_CH2                               0x00000002
#define AUX_EVCTL_EVSTAT2_AON_RTC_CH2_BITN                                   1
#define AUX_EVCTL_EVSTAT2_AON_RTC_CH2_M                             0x00000002
#define AUX_EVCTL_EVSTAT2_AON_RTC_CH2_S                                      1

// Field:     [0] MANUAL_EV
//
// Programmable event. See MANUAL for description.
#define AUX_EVCTL_EVSTAT2_MANUAL_EV                                 0x00000001
#define AUX_EVCTL_EVSTAT2_MANUAL_EV_BITN                                     0
#define AUX_EVCTL_EVSTAT2_MANUAL_EV_M                               0x00000001
#define AUX_EVCTL_EVSTAT2_MANUAL_EV_S                                        0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT3
//
//*****************************************************************************
// Field:    [15] AUX_TIMER2_CLKSWITCH_RDY
//
// AUX_SYSIF:TIMER2CLKSWITCH.RDY
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_CLKSWITCH_RDY                  0x00008000
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_CLKSWITCH_RDY_BITN                     15
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_CLKSWITCH_RDY_M                0x00008000
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_CLKSWITCH_RDY_S                        15

// Field:    [14] AUX_DAC_HOLD_ACTIVE
//
// AUX_ANAIF:DACSTAT.HOLD_ACTIVE
#define AUX_EVCTL_EVSTAT3_AUX_DAC_HOLD_ACTIVE                       0x00004000
#define AUX_EVCTL_EVSTAT3_AUX_DAC_HOLD_ACTIVE_BITN                          14
#define AUX_EVCTL_EVSTAT3_AUX_DAC_HOLD_ACTIVE_M                     0x00004000
#define AUX_EVCTL_EVSTAT3_AUX_DAC_HOLD_ACTIVE_S                             14

// Field:    [13] AUX_SMPH_AUTOTAKE_DONE
//
// See AUX_SMPH:AUTOTAKE.SMPH_ID for description.
#define AUX_EVCTL_EVSTAT3_AUX_SMPH_AUTOTAKE_DONE                    0x00002000
#define AUX_EVCTL_EVSTAT3_AUX_SMPH_AUTOTAKE_DONE_BITN                       13
#define AUX_EVCTL_EVSTAT3_AUX_SMPH_AUTOTAKE_DONE_M                  0x00002000
#define AUX_EVCTL_EVSTAT3_AUX_SMPH_AUTOTAKE_DONE_S                          13

// Field:    [12] AUX_ADC_FIFO_NOT_EMPTY
//
// AUX_ANAIF:ADCFIFOSTAT.EMPTY negated
#define AUX_EVCTL_EVSTAT3_AUX_ADC_FIFO_NOT_EMPTY                    0x00001000
#define AUX_EVCTL_EVSTAT3_AUX_ADC_FIFO_NOT_EMPTY_BITN                       12
#define AUX_EVCTL_EVSTAT3_AUX_ADC_FIFO_NOT_EMPTY_M                  0x00001000
#define AUX_EVCTL_EVSTAT3_AUX_ADC_FIFO_NOT_EMPTY_S                          12

// Field:    [11] AUX_ADC_FIFO_ALMOST_FULL
//
// AUX_ANAIF:ADCFIFOSTAT.ALMOST_FULL
#define AUX_EVCTL_EVSTAT3_AUX_ADC_FIFO_ALMOST_FULL                  0x00000800
#define AUX_EVCTL_EVSTAT3_AUX_ADC_FIFO_ALMOST_FULL_BITN                     11
#define AUX_EVCTL_EVSTAT3_AUX_ADC_FIFO_ALMOST_FULL_M                0x00000800
#define AUX_EVCTL_EVSTAT3_AUX_ADC_FIFO_ALMOST_FULL_S                        11

// Field:    [10] AUX_ADC_IRQ
//
// The logical function for this event is configurable.
//
// When DMACTL.EN = 1 :
//    Event = UDMA0 Channel 7 done event     OR
// AUX_ANAIF:ADCFIFOSTAT.OVERFLOW      OR      AUX_ANAIF:ADCFIFOSTAT.UNDERFLOW
//
// When DMACTL.EN = 0 :
//    Event = (NOT AUX_ANAIF:ADCFIFOSTAT.EMPTY)      OR
// AUX_ANAIF:ADCFIFOSTAT.OVERFLOW      OR      AUX_ANAIF:ADCFIFOSTAT.UNDERFLOW
//
// Bit 7 in UDMA0:DONEMASK must be 0.
#define AUX_EVCTL_EVSTAT3_AUX_ADC_IRQ                               0x00000400
#define AUX_EVCTL_EVSTAT3_AUX_ADC_IRQ_BITN                                  10
#define AUX_EVCTL_EVSTAT3_AUX_ADC_IRQ_M                             0x00000400
#define AUX_EVCTL_EVSTAT3_AUX_ADC_IRQ_S                                     10

// Field:     [9] AUX_ADC_DONE
//
// AUX_ANAIF ADC conversion done event.
// Event is synchronized at AUX bus rate.
#define AUX_EVCTL_EVSTAT3_AUX_ADC_DONE                              0x00000200
#define AUX_EVCTL_EVSTAT3_AUX_ADC_DONE_BITN                                  9
#define AUX_EVCTL_EVSTAT3_AUX_ADC_DONE_M                            0x00000200
#define AUX_EVCTL_EVSTAT3_AUX_ADC_DONE_S                                     9

// Field:     [8] AUX_ISRC_RESET_N
//
// AUX_ANAIF:ISRCCTL.RESET_N
#define AUX_EVCTL_EVSTAT3_AUX_ISRC_RESET_N                          0x00000100
#define AUX_EVCTL_EVSTAT3_AUX_ISRC_RESET_N_BITN                              8
#define AUX_EVCTL_EVSTAT3_AUX_ISRC_RESET_N_M                        0x00000100
#define AUX_EVCTL_EVSTAT3_AUX_ISRC_RESET_N_S                                 8

// Field:     [7] AUX_TDC_DONE
//
// AUX_TDC:STAT.DONE
#define AUX_EVCTL_EVSTAT3_AUX_TDC_DONE                              0x00000080
#define AUX_EVCTL_EVSTAT3_AUX_TDC_DONE_BITN                                  7
#define AUX_EVCTL_EVSTAT3_AUX_TDC_DONE_M                            0x00000080
#define AUX_EVCTL_EVSTAT3_AUX_TDC_DONE_S                                     7

// Field:     [6] AUX_TIMER0_EV
//
// AUX_TIMER0_EV event, see AUX_TIMER01:T0TARGET for description.
#define AUX_EVCTL_EVSTAT3_AUX_TIMER0_EV                             0x00000040
#define AUX_EVCTL_EVSTAT3_AUX_TIMER0_EV_BITN                                 6
#define AUX_EVCTL_EVSTAT3_AUX_TIMER0_EV_M                           0x00000040
#define AUX_EVCTL_EVSTAT3_AUX_TIMER0_EV_S                                    6

// Field:     [5] AUX_TIMER1_EV
//
// AUX_TIMER1_EV event, see AUX_TIMER01:T1TARGET for description.
#define AUX_EVCTL_EVSTAT3_AUX_TIMER1_EV                             0x00000020
#define AUX_EVCTL_EVSTAT3_AUX_TIMER1_EV_BITN                                 5
#define AUX_EVCTL_EVSTAT3_AUX_TIMER1_EV_M                           0x00000020
#define AUX_EVCTL_EVSTAT3_AUX_TIMER1_EV_S                                    5

// Field:     [4] AUX_TIMER2_PULSE
//
// AUX_TIMER2 pulse event.
// Configuration of AUX_SYSIF:EVSYNCRATE.AUX_TIMER2_SYNC_RATE sets the
// synchronization rate for this event.
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_PULSE                          0x00000010
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_PULSE_BITN                              4
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_PULSE_M                        0x00000010
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_PULSE_S                                 4

// Field:     [3] AUX_TIMER2_EV3
//
// AUX_TIMER2 event output 3.
// Configuration of AUX_SYSIF:EVSYNCRATE.AUX_TIMER2_SYNC_RATE sets the
// synchronization rate for this event.
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV3                            0x00000008
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV3_BITN                                3
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV3_M                          0x00000008
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV3_S                                   3

// Field:     [2] AUX_TIMER2_EV2
//
// AUX_TIMER2 event output 2.
// Configuration of AUX_SYSIF:EVSYNCRATE.AUX_TIMER2_SYNC_RATE sets the
// synchronization rate for this event.
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV2                            0x00000004
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV2_BITN                                2
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV2_M                          0x00000004
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV2_S                                   2

// Field:     [1] AUX_TIMER2_EV1
//
// AUX_TIMER2 event output 1.
// Configuration of AUX_SYSIF:EVSYNCRATE.AUX_TIMER2_SYNC_RATE sets the
// synchronization rate for this event.
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV1                            0x00000002
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV1_BITN                                1
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV1_M                          0x00000002
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV1_S                                   1

// Field:     [0] AUX_TIMER2_EV0
//
// AUX_TIMER2 event output 0.
// Configuration of AUX_SYSIF:EVSYNCRATE.AUX_TIMER2_SYNC_RATE sets the
// synchronization rate for this event.
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV0                            0x00000001
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV0_BITN                                0
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV0_M                          0x00000001
#define AUX_EVCTL_EVSTAT3_AUX_TIMER2_EV0_S                                   0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_SCEWEVCFG0
//
//*****************************************************************************
// Field:     [6] COMB_EV_EN
//
// Event combination control:
//
// 0: Disable event combination.
// 1: Enable event combination.
#define AUX_EVCTL_SCEWEVCFG0_COMB_EV_EN                             0x00000040
#define AUX_EVCTL_SCEWEVCFG0_COMB_EV_EN_BITN                                 6
#define AUX_EVCTL_SCEWEVCFG0_COMB_EV_EN_M                           0x00000040
#define AUX_EVCTL_SCEWEVCFG0_COMB_EV_EN_S                                    6

// Field:   [5:0] EV0_SEL
//
// Select the event source from the synchronous event bus to be used in event
// equation.
// ENUMs:
// AUX_TIMER2_CLKSWITCH_RDY EVSTAT3.AUX_TIMER2_CLKSWITCH_RDY
// AUX_DAC_HOLD_ACTIVE      EVSTAT3.AUX_DAC_HOLD_ACTIVE
// AUX_SMPH_AUTOTAKE_DONE   EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            EVSTAT3.AUX_TIMER0_EV
// AUX_TIMER1_EV            EVSTAT3.AUX_TIMER1_EV
// AUX_TIMER2_PULSE         EVSTAT3.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                EVSTAT2.AUX_COMPB
// AUX_COMPA                EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              EVSTAT2.MCU_OBSMUX0
// MCU_EV                   EVSTAT2.MCU_EV
// ACLK_REF                 EVSTAT2.ACLK_REF
// VDDR_RECHARGE            EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               EVSTAT2.MCU_ACTIVE
// PWR_DWN                  EVSTAT2.PWR_DWN
// SCLK_LF                  EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              EVSTAT2.AON_RTC_CH2
// AUX_PROG_DLY_IDLE        Programmable delay event as described in PROGDLY
// AUXIO31                  EVSTAT1.AUXIO31
// AUXIO30                  EVSTAT1.AUXIO30
// AUXIO29                  EVSTAT1.AUXIO29
// AUXIO28                  EVSTAT1.AUXIO28
// AUXIO27                  EVSTAT1.AUXIO27
// AUXIO26                  EVSTAT1.AUXIO26
// AUXIO25                  EVSTAT1.AUXIO25
// AUXIO24                  EVSTAT1.AUXIO24
// AUXIO23                  EVSTAT1.AUXIO23
// AUXIO22                  EVSTAT1.AUXIO22
// AUXIO21                  EVSTAT1.AUXIO21
// AUXIO20                  EVSTAT1.AUXIO20
// AUXIO19                  EVSTAT1.AUXIO19
// AUXIO18                  EVSTAT1.AUXIO18
// AUXIO17                  EVSTAT1.AUXIO17
// AUXIO16                  EVSTAT1.AUXIO16
// AUXIO15                  EVSTAT0.AUXIO15
// AUXIO14                  EVSTAT0.AUXIO14
// AUXIO13                  EVSTAT0.AUXIO13
// AUXIO12                  EVSTAT0.AUXIO12
// AUXIO11                  EVSTAT0.AUXIO11
// AUXIO10                  EVSTAT0.AUXIO10
// AUXIO9                   EVSTAT0.AUXIO9
// AUXIO8                   EVSTAT0.AUXIO8
// AUXIO7                   EVSTAT0.AUXIO7
// AUXIO6                   EVSTAT0.AUXIO6
// AUXIO5                   EVSTAT0.AUXIO5
// AUXIO4                   EVSTAT0.AUXIO4
// AUXIO3                   EVSTAT0.AUXIO3
// AUXIO2                   EVSTAT0.AUXIO2
// AUXIO1                   EVSTAT0.AUXIO1
// AUXIO0                   EVSTAT0.AUXIO0
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_W                                       6
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_M                              0x0000003F
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_S                                       0
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TIMER2_CLKSWITCH_RDY       0x0000003F
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_DAC_HOLD_ACTIVE            0x0000003E
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_SMPH_AUTOTAKE_DONE         0x0000003D
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_ADC_FIFO_NOT_EMPTY         0x0000003C
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_ADC_FIFO_ALMOST_FULL       0x0000003B
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_ADC_IRQ                    0x0000003A
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_ADC_DONE                   0x00000039
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_ISRC_RESET_N               0x00000038
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TDC_DONE                   0x00000037
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TIMER0_EV                  0x00000036
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TIMER1_EV                  0x00000035
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TIMER2_PULSE               0x00000034
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TIMER2_EV3                 0x00000033
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TIMER2_EV2                 0x00000032
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TIMER2_EV1                 0x00000031
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_TIMER2_EV0                 0x00000030
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_COMPB                      0x0000002F
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_COMPA                      0x0000002E
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_MCU_OBSMUX1                    0x0000002D
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_MCU_OBSMUX0                    0x0000002C
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_MCU_EV                         0x0000002B
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_ACLK_REF                       0x0000002A
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_VDDR_RECHARGE                  0x00000029
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_MCU_ACTIVE                     0x00000028
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_PWR_DWN                        0x00000027
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_SCLK_LF                        0x00000026
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AON_BATMON_TEMP_UPD            0x00000025
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AON_BATMON_BAT_UPD             0x00000024
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AON_RTC_4KHZ                   0x00000023
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AON_RTC_CH2_DLY                0x00000022
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AON_RTC_CH2                    0x00000021
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUX_PROG_DLY_IDLE              0x00000020
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO31                        0x0000001F
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO30                        0x0000001E
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO29                        0x0000001D
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO28                        0x0000001C
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO27                        0x0000001B
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO26                        0x0000001A
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO25                        0x00000019
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO24                        0x00000018
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO23                        0x00000017
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO22                        0x00000016
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO21                        0x00000015
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO20                        0x00000014
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO19                        0x00000013
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO18                        0x00000012
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO17                        0x00000011
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO16                        0x00000010
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO15                        0x0000000F
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO14                        0x0000000E
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO13                        0x0000000D
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO12                        0x0000000C
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO11                        0x0000000B
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO10                        0x0000000A
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO9                         0x00000009
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO8                         0x00000008
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO7                         0x00000007
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO6                         0x00000006
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO5                         0x00000005
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO4                         0x00000004
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO3                         0x00000003
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO2                         0x00000002
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO1                         0x00000001
#define AUX_EVCTL_SCEWEVCFG0_EV0_SEL_AUXIO0                         0x00000000

//*****************************************************************************
//
// Register: AUX_EVCTL_O_SCEWEVCFG1
//
//*****************************************************************************
// Field:     [7] EV0_POL
//
// Polarity of SCEWEVCFG0.EV0_SEL event.
//
// When SCEWEVCFG0.COMB_EV_EN is 0:
//
// 0: Non-inverted.
// 1: Non-inverted.
//
// When SCEWEVCFG0.COMB_EV_EN is 1.
//
// 0: Non-inverted.
// 1: Inverted.
#define AUX_EVCTL_SCEWEVCFG1_EV0_POL                                0x00000080
#define AUX_EVCTL_SCEWEVCFG1_EV0_POL_BITN                                    7
#define AUX_EVCTL_SCEWEVCFG1_EV0_POL_M                              0x00000080
#define AUX_EVCTL_SCEWEVCFG1_EV0_POL_S                                       7

// Field:     [6] EV1_POL
//
// Polarity of EV1_SEL event.
//
// When SCEWEVCFG0.COMB_EV_EN is 0:
//
// 0: Non-inverted.
// 1: Non-inverted.
//
// When SCEWEVCFG0.COMB_EV_EN is 1.
//
// 0: Non-inverted.
// 1: Inverted.
#define AUX_EVCTL_SCEWEVCFG1_EV1_POL                                0x00000040
#define AUX_EVCTL_SCEWEVCFG1_EV1_POL_BITN                                    6
#define AUX_EVCTL_SCEWEVCFG1_EV1_POL_M                              0x00000040
#define AUX_EVCTL_SCEWEVCFG1_EV1_POL_S                                       6

// Field:   [5:0] EV1_SEL
//
// Select the event source from the synchronous event bus to be used in event
// equation.
// ENUMs:
// AUX_TIMER2_CLKSWITCH_RDY EVSTAT3.AUX_TIMER2_CLKSWITCH_RDY
// AUX_DAC_HOLD_ACTIVE      EVSTAT3.AUX_DAC_HOLD_ACTIVE
// AUX_SMPH_AUTOTAKE_DONE   EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            EVSTAT3.AUX_TIMER0_EV
// AUX_TIMER1_EV            EVSTAT3.AUX_TIMER1_EV
// AUX_TIMER2_PULSE         EVSTAT3.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                EVSTAT2.AUX_COMPB
// AUX_COMPA                EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              EVSTAT2.MCU_OBSMUX0
// MCU_EV                   EVSTAT2.MCU_EV
// ACLK_REF                 EVSTAT2.ACLK_REF
// VDDR_RECHARGE            EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               EVSTAT2.MCU_ACTIVE
// PWR_DWN                  EVSTAT2.PWR_DWN
// SCLK_LF                  EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              EVSTAT2.AON_RTC_CH2
// AUX_PROG_DLY_IDLE        Programmable delay event as described in PROGDLY
// AUXIO31                  EVSTAT1.AUXIO31
// AUXIO30                  EVSTAT1.AUXIO30
// AUXIO29                  EVSTAT1.AUXIO29
// AUXIO28                  EVSTAT1.AUXIO28
// AUXIO27                  EVSTAT1.AUXIO27
// AUXIO26                  EVSTAT1.AUXIO26
// AUXIO25                  EVSTAT1.AUXIO25
// AUXIO24                  EVSTAT1.AUXIO24
// AUXIO23                  EVSTAT1.AUXIO23
// AUXIO22                  EVSTAT1.AUXIO22
// AUXIO21                  EVSTAT1.AUXIO21
// AUXIO20                  EVSTAT1.AUXIO20
// AUXIO19                  EVSTAT1.AUXIO19
// AUXIO18                  EVSTAT1.AUXIO18
// AUXIO17                  EVSTAT1.AUXIO17
// AUXIO16                  EVSTAT1.AUXIO16
// AUXIO15                  EVSTAT0.AUXIO15
// AUXIO14                  EVSTAT0.AUXIO14
// AUXIO13                  EVSTAT0.AUXIO13
// AUXIO12                  EVSTAT0.AUXIO12
// AUXIO11                  EVSTAT0.AUXIO11
// AUXIO10                  EVSTAT0.AUXIO10
// AUXIO9                   EVSTAT0.AUXIO9
// AUXIO8                   EVSTAT0.AUXIO8
// AUXIO7                   EVSTAT0.AUXIO7
// AUXIO6                   EVSTAT0.AUXIO6
// AUXIO5                   EVSTAT0.AUXIO5
// AUXIO4                   EVSTAT0.AUXIO4
// AUXIO3                   EVSTAT0.AUXIO3
// AUXIO2                   EVSTAT0.AUXIO2
// AUXIO1                   EVSTAT0.AUXIO1
// AUXIO0                   EVSTAT0.AUXIO0
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_W                                       6
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_M                              0x0000003F
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_S                                       0
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TIMER2_CLKSWITCH_RDY       0x0000003F
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_DAC_HOLD_ACTIVE            0x0000003E
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_SMPH_AUTOTAKE_DONE         0x0000003D
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_ADC_FIFO_NOT_EMPTY         0x0000003C
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_ADC_FIFO_ALMOST_FULL       0x0000003B
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_ADC_IRQ                    0x0000003A
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_ADC_DONE                   0x00000039
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_ISRC_RESET_N               0x00000038
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TDC_DONE                   0x00000037
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TIMER0_EV                  0x00000036
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TIMER1_EV                  0x00000035
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TIMER2_PULSE               0x00000034
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TIMER2_EV3                 0x00000033
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TIMER2_EV2                 0x00000032
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TIMER2_EV1                 0x00000031
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_TIMER2_EV0                 0x00000030
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_COMPB                      0x0000002F
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_COMPA                      0x0000002E
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_MCU_OBSMUX1                    0x0000002D
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_MCU_OBSMUX0                    0x0000002C
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_MCU_EV                         0x0000002B
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_ACLK_REF                       0x0000002A
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_VDDR_RECHARGE                  0x00000029
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_MCU_ACTIVE                     0x00000028
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_PWR_DWN                        0x00000027
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_SCLK_LF                        0x00000026
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AON_BATMON_TEMP_UPD            0x00000025
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AON_BATMON_BAT_UPD             0x00000024
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AON_RTC_4KHZ                   0x00000023
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AON_RTC_CH2_DLY                0x00000022
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AON_RTC_CH2                    0x00000021
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUX_PROG_DLY_IDLE              0x00000020
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO31                        0x0000001F
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO30                        0x0000001E
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO29                        0x0000001D
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO28                        0x0000001C
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO27                        0x0000001B
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO26                        0x0000001A
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO25                        0x00000019
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO24                        0x00000018
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO23                        0x00000017
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO22                        0x00000016
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO21                        0x00000015
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO20                        0x00000014
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO19                        0x00000013
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO18                        0x00000012
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO17                        0x00000011
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO16                        0x00000010
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO15                        0x0000000F
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO14                        0x0000000E
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO13                        0x0000000D
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO12                        0x0000000C
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO11                        0x0000000B
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO10                        0x0000000A
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO9                         0x00000009
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO8                         0x00000008
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO7                         0x00000007
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO6                         0x00000006
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO5                         0x00000005
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO4                         0x00000004
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO3                         0x00000003
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO2                         0x00000002
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO1                         0x00000001
#define AUX_EVCTL_SCEWEVCFG1_EV1_SEL_AUXIO0                         0x00000000

//*****************************************************************************
//
// Register: AUX_EVCTL_O_DMACTL
//
//*****************************************************************************
// Field:     [2] REQ_MODE
//
// UDMA0 Request mode
// ENUMs:
// SINGLE                   Single requests are generated on UDMA0 channel 7
//                          when the condition configured in SEL is met.
// BURST                    Burst requests are generated on UDMA0 channel 7
//                          when the condition configured in SEL is met.
#define AUX_EVCTL_DMACTL_REQ_MODE                                   0x00000004
#define AUX_EVCTL_DMACTL_REQ_MODE_BITN                                       2
#define AUX_EVCTL_DMACTL_REQ_MODE_M                                 0x00000004
#define AUX_EVCTL_DMACTL_REQ_MODE_S                                          2
#define AUX_EVCTL_DMACTL_REQ_MODE_SINGLE                            0x00000004
#define AUX_EVCTL_DMACTL_REQ_MODE_BURST                             0x00000000

// Field:     [1] EN
//
// uDMA ADC interface enable.
//
// 0: Disable UDMA0 interface to ADC.
// 1: Enable UDMA0 interface to ADC.
#define AUX_EVCTL_DMACTL_EN                                         0x00000002
#define AUX_EVCTL_DMACTL_EN_BITN                                             1
#define AUX_EVCTL_DMACTL_EN_M                                       0x00000002
#define AUX_EVCTL_DMACTL_EN_S                                                1

// Field:     [0] SEL
//
// Select FIFO watermark level required to trigger a UDMA0 transfer of ADC FIFO
// data.
// ENUMs:
// AUX_ADC_FIFO_ALMOST_FULL UDMA0 trigger event will be generated when the ADC
//                          FIFO is almost full (3/4 full).
// AUX_ADC_FIFO_NOT_EMPTY   UDMA0 trigger event will be generated when there
//                          are samples in the ADC FIFO.
#define AUX_EVCTL_DMACTL_SEL                                        0x00000001
#define AUX_EVCTL_DMACTL_SEL_BITN                                            0
#define AUX_EVCTL_DMACTL_SEL_M                                      0x00000001
#define AUX_EVCTL_DMACTL_SEL_S                                               0
#define AUX_EVCTL_DMACTL_SEL_AUX_ADC_FIFO_ALMOST_FULL               0x00000001
#define AUX_EVCTL_DMACTL_SEL_AUX_ADC_FIFO_NOT_EMPTY                 0x00000000

//*****************************************************************************
//
// Register: AUX_EVCTL_O_SWEVSET
//
//*****************************************************************************
// Field:     [2] SWEV2
//
// Software event flag 2.
//
// 0: No effect.
// 1: Set software event flag 2.
#define AUX_EVCTL_SWEVSET_SWEV2                                     0x00000004
#define AUX_EVCTL_SWEVSET_SWEV2_BITN                                         2
#define AUX_EVCTL_SWEVSET_SWEV2_M                                   0x00000004
#define AUX_EVCTL_SWEVSET_SWEV2_S                                            2

// Field:     [1] SWEV1
//
// Software event flag 1.
//
// 0: No effect.
// 1: Set software event flag 1.
#define AUX_EVCTL_SWEVSET_SWEV1                                     0x00000002
#define AUX_EVCTL_SWEVSET_SWEV1_BITN                                         1
#define AUX_EVCTL_SWEVSET_SWEV1_M                                   0x00000002
#define AUX_EVCTL_SWEVSET_SWEV1_S                                            1

// Field:     [0] SWEV0
//
// Software event flag 0.
//
// 0: No effect.
// 1: Set software event flag 0.
#define AUX_EVCTL_SWEVSET_SWEV0                                     0x00000001
#define AUX_EVCTL_SWEVSET_SWEV0_BITN                                         0
#define AUX_EVCTL_SWEVSET_SWEV0_M                                   0x00000001
#define AUX_EVCTL_SWEVSET_SWEV0_S                                            0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVTOAONFLAGS
//
//*****************************************************************************
// Field:     [8] AUX_TIMER1_EV
//
// This event flag is set when level selected by EVTOAONPOL.AUX_TIMER1_EV
// occurs on EVSTAT3.AUX_TIMER1_EV.
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TIMER1_EV                        0x00000100
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TIMER1_EV_BITN                            8
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TIMER1_EV_M                      0x00000100
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TIMER1_EV_S                               8

// Field:     [7] AUX_TIMER0_EV
//
// This event flag is set when level selected by EVTOAONPOL.AUX_TIMER0_EV
// occurs on EVSTAT3.AUX_TIMER0_EV.
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TIMER0_EV                        0x00000080
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TIMER0_EV_BITN                            7
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TIMER0_EV_M                      0x00000080
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TIMER0_EV_S                               7

// Field:     [6] AUX_TDC_DONE
//
// This event flag is set when level selected by EVTOAONPOL.AUX_TDC_DONE occurs
// on EVSTAT3.AUX_TDC_DONE.
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TDC_DONE                         0x00000040
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TDC_DONE_BITN                             6
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TDC_DONE_M                       0x00000040
#define AUX_EVCTL_EVTOAONFLAGS_AUX_TDC_DONE_S                                6

// Field:     [5] AUX_ADC_DONE
//
// This event flag is set when level selected by EVTOAONPOL.AUX_ADC_DONE occurs
// on EVSTAT3.AUX_ADC_DONE.
#define AUX_EVCTL_EVTOAONFLAGS_AUX_ADC_DONE                         0x00000020
#define AUX_EVCTL_EVTOAONFLAGS_AUX_ADC_DONE_BITN                             5
#define AUX_EVCTL_EVTOAONFLAGS_AUX_ADC_DONE_M                       0x00000020
#define AUX_EVCTL_EVTOAONFLAGS_AUX_ADC_DONE_S                                5

// Field:     [4] AUX_COMPB
//
// This event flag is set when edge selected by EVTOAONPOL.AUX_COMPB occurs on
// EVSTAT2.AUX_COMPB.
#define AUX_EVCTL_EVTOAONFLAGS_AUX_COMPB                            0x00000010
#define AUX_EVCTL_EVTOAONFLAGS_AUX_COMPB_BITN                                4
#define AUX_EVCTL_EVTOAONFLAGS_AUX_COMPB_M                          0x00000010
#define AUX_EVCTL_EVTOAONFLAGS_AUX_COMPB_S                                   4

// Field:     [3] AUX_COMPA
//
// This event flag is set when edge selected by EVTOAONPOL.AUX_COMPA occurs on
// EVSTAT2.AUX_COMPA.
#define AUX_EVCTL_EVTOAONFLAGS_AUX_COMPA                            0x00000008
#define AUX_EVCTL_EVTOAONFLAGS_AUX_COMPA_BITN                                3
#define AUX_EVCTL_EVTOAONFLAGS_AUX_COMPA_M                          0x00000008
#define AUX_EVCTL_EVTOAONFLAGS_AUX_COMPA_S                                   3

// Field:     [2] SWEV2
//
// This event flag is set when software writes a 1 to SWEVSET.SWEV2.
#define AUX_EVCTL_EVTOAONFLAGS_SWEV2                                0x00000004
#define AUX_EVCTL_EVTOAONFLAGS_SWEV2_BITN                                    2
#define AUX_EVCTL_EVTOAONFLAGS_SWEV2_M                              0x00000004
#define AUX_EVCTL_EVTOAONFLAGS_SWEV2_S                                       2

// Field:     [1] SWEV1
//
// This event flag is set when software writes a 1 to SWEVSET.SWEV1.
#define AUX_EVCTL_EVTOAONFLAGS_SWEV1                                0x00000002
#define AUX_EVCTL_EVTOAONFLAGS_SWEV1_BITN                                    1
#define AUX_EVCTL_EVTOAONFLAGS_SWEV1_M                              0x00000002
#define AUX_EVCTL_EVTOAONFLAGS_SWEV1_S                                       1

// Field:     [0] SWEV0
//
// This event flag is set when software writes a 1 to SWEVSET.SWEV0.
#define AUX_EVCTL_EVTOAONFLAGS_SWEV0                                0x00000001
#define AUX_EVCTL_EVTOAONFLAGS_SWEV0_BITN                                    0
#define AUX_EVCTL_EVTOAONFLAGS_SWEV0_M                              0x00000001
#define AUX_EVCTL_EVTOAONFLAGS_SWEV0_S                                       0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVTOAONPOL
//
//*****************************************************************************
// Field:     [8] AUX_TIMER1_EV
//
// Select the level of EVSTAT3.AUX_TIMER1_EV that sets
// EVTOAONFLAGS.AUX_TIMER1_EV.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER1_EV                          0x00000100
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER1_EV_BITN                              8
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER1_EV_M                        0x00000100
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER1_EV_S                                 8
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER1_EV_LOW                      0x00000100
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER1_EV_HIGH                     0x00000000

// Field:     [7] AUX_TIMER0_EV
//
// Select the level of EVSTAT3.AUX_TIMER0_EV that sets
// EVTOAONFLAGS.AUX_TIMER0_EV.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER0_EV                          0x00000080
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER0_EV_BITN                              7
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER0_EV_M                        0x00000080
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER0_EV_S                                 7
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER0_EV_LOW                      0x00000080
#define AUX_EVCTL_EVTOAONPOL_AUX_TIMER0_EV_HIGH                     0x00000000

// Field:     [6] AUX_TDC_DONE
//
// Select level of EVSTAT3.AUX_TDC_DONE that sets EVTOAONFLAGS.AUX_TDC_DONE.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOAONPOL_AUX_TDC_DONE                           0x00000040
#define AUX_EVCTL_EVTOAONPOL_AUX_TDC_DONE_BITN                               6
#define AUX_EVCTL_EVTOAONPOL_AUX_TDC_DONE_M                         0x00000040
#define AUX_EVCTL_EVTOAONPOL_AUX_TDC_DONE_S                                  6
#define AUX_EVCTL_EVTOAONPOL_AUX_TDC_DONE_LOW                       0x00000040
#define AUX_EVCTL_EVTOAONPOL_AUX_TDC_DONE_HIGH                      0x00000000

// Field:     [5] AUX_ADC_DONE
//
// Select the level of  EVSTAT3.AUX_ADC_DONE that sets
// EVTOAONFLAGS.AUX_ADC_DONE.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOAONPOL_AUX_ADC_DONE                           0x00000020
#define AUX_EVCTL_EVTOAONPOL_AUX_ADC_DONE_BITN                               5
#define AUX_EVCTL_EVTOAONPOL_AUX_ADC_DONE_M                         0x00000020
#define AUX_EVCTL_EVTOAONPOL_AUX_ADC_DONE_S                                  5
#define AUX_EVCTL_EVTOAONPOL_AUX_ADC_DONE_LOW                       0x00000020
#define AUX_EVCTL_EVTOAONPOL_AUX_ADC_DONE_HIGH                      0x00000000

// Field:     [4] AUX_COMPB
//
// Select the edge of  EVSTAT2.AUX_COMPB that sets EVTOAONFLAGS.AUX_COMPB.
// ENUMs:
// FALL                     Falling edge
// RISE                     Rising edge
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPB                              0x00000010
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPB_BITN                                  4
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPB_M                            0x00000010
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPB_S                                     4
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPB_FALL                         0x00000010
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPB_RISE                         0x00000000

// Field:     [3] AUX_COMPA
//
// Select the edge of  EVSTAT2.AUX_COMPA that sets EVTOAONFLAGS.AUX_COMPA.
// ENUMs:
// FALL                     Falling edge
// RISE                     Rising edge
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPA                              0x00000008
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPA_BITN                                  3
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPA_M                            0x00000008
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPA_S                                     3
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPA_FALL                         0x00000008
#define AUX_EVCTL_EVTOAONPOL_AUX_COMPA_RISE                         0x00000000

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVTOAONFLAGSCLR
//
//*****************************************************************************
// Field:     [8] AUX_TIMER1_EV
//
// Write 1 to clear EVTOAONFLAGS.AUX_TIMER1_EV.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TIMER1_EV                     0x00000100
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TIMER1_EV_BITN                         8
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TIMER1_EV_M                   0x00000100
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TIMER1_EV_S                            8

// Field:     [7] AUX_TIMER0_EV
//
// Write 1 to clear EVTOAONFLAGS.AUX_TIMER0_EV.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TIMER0_EV                     0x00000080
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TIMER0_EV_BITN                         7
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TIMER0_EV_M                   0x00000080
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TIMER0_EV_S                            7

// Field:     [6] AUX_TDC_DONE
//
// Write 1 to clear EVTOAONFLAGS.AUX_TDC_DONE.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TDC_DONE                      0x00000040
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TDC_DONE_BITN                          6
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TDC_DONE_M                    0x00000040
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_TDC_DONE_S                             6

// Field:     [5] AUX_ADC_DONE
//
// Write 1 to clear EVTOAONFLAGS.AUX_ADC_DONE.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_ADC_DONE                      0x00000020
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_ADC_DONE_BITN                          5
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_ADC_DONE_M                    0x00000020
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_ADC_DONE_S                             5

// Field:     [4] AUX_COMPB
//
// Write 1 to clear EVTOAONFLAGS.AUX_COMPB.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_COMPB                         0x00000010
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_COMPB_BITN                             4
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_COMPB_M                       0x00000010
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_COMPB_S                                4

// Field:     [3] AUX_COMPA
//
// Write 1 to clear EVTOAONFLAGS.AUX_COMPA.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_COMPA                         0x00000008
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_COMPA_BITN                             3
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_COMPA_M                       0x00000008
#define AUX_EVCTL_EVTOAONFLAGSCLR_AUX_COMPA_S                                3

// Field:     [2] SWEV2
//
// Write 1 to clear EVTOAONFLAGS.SWEV2.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV2                             0x00000004
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV2_BITN                                 2
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV2_M                           0x00000004
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV2_S                                    2

// Field:     [1] SWEV1
//
// Write 1 to clear EVTOAONFLAGS.SWEV1.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV1                             0x00000002
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV1_BITN                                 1
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV1_M                           0x00000002
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV1_S                                    1

// Field:     [0] SWEV0
//
// Write 1 to clear EVTOAONFLAGS.SWEV0.
//
// Read value is 0.
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV0                             0x00000001
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV0_BITN                                 0
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV0_M                           0x00000001
#define AUX_EVCTL_EVTOAONFLAGSCLR_SWEV0_S                                    0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVTOMCUFLAGS
//
//*****************************************************************************
// Field:    [15] AUX_TIMER2_PULSE
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_TIMER2_PULSE
// occurs on EVSTAT3.AUX_TIMER2_PULSE.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_PULSE                     0x00008000
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_PULSE_BITN                        15
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_PULSE_M                   0x00008000
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_PULSE_S                           15

// Field:    [14] AUX_TIMER2_EV3
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_TIMER2_EV3
// occurs on EVSTAT3.AUX_TIMER2_EV3.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV3                       0x00004000
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV3_BITN                          14
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV3_M                     0x00004000
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV3_S                             14

// Field:    [13] AUX_TIMER2_EV2
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_TIMER2_EV2
// occurs on EVSTAT3.AUX_TIMER2_EV2.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV2                       0x00002000
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV2_BITN                          13
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV2_M                     0x00002000
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV2_S                             13

// Field:    [12] AUX_TIMER2_EV1
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_TIMER2_EV1
// occurs on EVSTAT3.AUX_TIMER2_EV1.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV1                       0x00001000
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV1_BITN                          12
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV1_M                     0x00001000
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV1_S                             12

// Field:    [11] AUX_TIMER2_EV0
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_TIMER2_EV0
// occurs on EVSTAT3.AUX_TIMER2_EV0.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV0                       0x00000800
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV0_BITN                          11
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV0_M                     0x00000800
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER2_EV0_S                             11

// Field:    [10] AUX_ADC_IRQ
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_ADC_IRQ occurs
// on EVSTAT3.AUX_ADC_IRQ.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_IRQ                          0x00000400
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_IRQ_BITN                             10
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_IRQ_M                        0x00000400
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_IRQ_S                                10

// Field:     [9] MCU_OBSMUX0
//
// This event flag is set when level selected by EVTOMCUPOL.MCU_OBSMUX0 occurs
// on EVSTAT2.MCU_OBSMUX0.
#define AUX_EVCTL_EVTOMCUFLAGS_MCU_OBSMUX0                          0x00000200
#define AUX_EVCTL_EVTOMCUFLAGS_MCU_OBSMUX0_BITN                              9
#define AUX_EVCTL_EVTOMCUFLAGS_MCU_OBSMUX0_M                        0x00000200
#define AUX_EVCTL_EVTOMCUFLAGS_MCU_OBSMUX0_S                                 9

// Field:     [8] AUX_ADC_FIFO_ALMOST_FULL
//
// This event flag is set when level selected by
// EVTOMCUPOL.AUX_ADC_FIFO_ALMOST_FULL occurs on
// EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_FIFO_ALMOST_FULL             0x00000100
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_FIFO_ALMOST_FULL_BITN                 8
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_FIFO_ALMOST_FULL_M           0x00000100
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_FIFO_ALMOST_FULL_S                    8

// Field:     [7] AUX_ADC_DONE
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_ADC_DONE occurs
// on EVSTAT3.AUX_ADC_DONE.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_DONE                         0x00000080
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_DONE_BITN                             7
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_DONE_M                       0x00000080
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_ADC_DONE_S                                7

// Field:     [6] AUX_SMPH_AUTOTAKE_DONE
//
// This event flag is set when level selected by
// EVTOMCUPOL.AUX_SMPH_AUTOTAKE_DONE occurs on EVSTAT3.AUX_SMPH_AUTOTAKE_DONE.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_SMPH_AUTOTAKE_DONE               0x00000040
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_SMPH_AUTOTAKE_DONE_BITN                   6
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_SMPH_AUTOTAKE_DONE_M             0x00000040
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_SMPH_AUTOTAKE_DONE_S                      6

// Field:     [5] AUX_TIMER1_EV
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_TIMER1_EV
// occurs on EVSTAT3.AUX_TIMER1_EV.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER1_EV                        0x00000020
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER1_EV_BITN                            5
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER1_EV_M                      0x00000020
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER1_EV_S                               5

// Field:     [4] AUX_TIMER0_EV
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_TIMER0_EV
// occurs on EVSTAT3.AUX_TIMER0_EV.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER0_EV                        0x00000010
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER0_EV_BITN                            4
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER0_EV_M                      0x00000010
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TIMER0_EV_S                               4

// Field:     [3] AUX_TDC_DONE
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_TDC_DONE occurs
// on EVSTAT3.AUX_TDC_DONE.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TDC_DONE                         0x00000008
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TDC_DONE_BITN                             3
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TDC_DONE_M                       0x00000008
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_TDC_DONE_S                                3

// Field:     [2] AUX_COMPB
//
// This event flag is set when edge selected by EVTOMCUPOL.AUX_COMPB occurs on
// EVSTAT2.AUX_COMPB.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_COMPB                            0x00000004
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_COMPB_BITN                                2
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_COMPB_M                          0x00000004
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_COMPB_S                                   2

// Field:     [1] AUX_COMPA
//
// This event flag is set when edge selected by EVTOMCUPOL.AUX_COMPA occurs on
// EVSTAT2.AUX_COMPA.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_COMPA                            0x00000002
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_COMPA_BITN                                1
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_COMPA_M                          0x00000002
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_COMPA_S                                   1

// Field:     [0] AUX_WU_EV
//
// This event flag is set when level selected by EVTOMCUPOL.AUX_WU_EV occurs on
// reduction-OR of the AUX_SYSIF:WUFLAGS register.
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_WU_EV                            0x00000001
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_WU_EV_BITN                                0
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_WU_EV_M                          0x00000001
#define AUX_EVCTL_EVTOMCUFLAGS_AUX_WU_EV_S                                   0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVTOMCUPOL
//
//*****************************************************************************
// Field:    [15] AUX_TIMER2_PULSE
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_TIMER2_PULSE.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_PULSE                       0x00008000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_PULSE_BITN                          15
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_PULSE_M                     0x00008000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_PULSE_S                             15
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_PULSE_LOW                   0x00008000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_PULSE_HIGH                  0x00000000

// Field:    [14] AUX_TIMER2_EV3
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_TIMER2_EV3.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV3                         0x00004000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV3_BITN                            14
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV3_M                       0x00004000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV3_S                               14
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV3_LOW                     0x00004000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV3_HIGH                    0x00000000

// Field:    [13] AUX_TIMER2_EV2
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_TIMER2_EV2.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV2                         0x00002000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV2_BITN                            13
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV2_M                       0x00002000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV2_S                               13
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV2_LOW                     0x00002000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV2_HIGH                    0x00000000

// Field:    [12] AUX_TIMER2_EV1
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_TIMER2_EV1.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV1                         0x00001000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV1_BITN                            12
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV1_M                       0x00001000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV1_S                               12
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV1_LOW                     0x00001000
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV1_HIGH                    0x00000000

// Field:    [11] AUX_TIMER2_EV0
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_TIMER2_EV0.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV0                         0x00000800
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV0_BITN                            11
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV0_M                       0x00000800
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV0_S                               11
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV0_LOW                     0x00000800
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER2_EV0_HIGH                    0x00000000

// Field:    [10] AUX_ADC_IRQ
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_ADC_IRQ.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_IRQ                            0x00000400
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_IRQ_BITN                               10
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_IRQ_M                          0x00000400
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_IRQ_S                                  10
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_IRQ_LOW                        0x00000400
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_IRQ_HIGH                       0x00000000

// Field:     [9] MCU_OBSMUX0
//
// Select the event source level that sets EVTOMCUFLAGS.MCU_OBSMUX0.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_MCU_OBSMUX0                            0x00000200
#define AUX_EVCTL_EVTOMCUPOL_MCU_OBSMUX0_BITN                                9
#define AUX_EVCTL_EVTOMCUPOL_MCU_OBSMUX0_M                          0x00000200
#define AUX_EVCTL_EVTOMCUPOL_MCU_OBSMUX0_S                                   9
#define AUX_EVCTL_EVTOMCUPOL_MCU_OBSMUX0_LOW                        0x00000200
#define AUX_EVCTL_EVTOMCUPOL_MCU_OBSMUX0_HIGH                       0x00000000

// Field:     [8] AUX_ADC_FIFO_ALMOST_FULL
//
// Select the event source level that sets
// EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_FIFO_ALMOST_FULL               0x00000100
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_FIFO_ALMOST_FULL_BITN                   8
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_FIFO_ALMOST_FULL_M             0x00000100
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_FIFO_ALMOST_FULL_S                      8
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_FIFO_ALMOST_FULL_LOW           0x00000100
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_FIFO_ALMOST_FULL_HIGH          0x00000000

// Field:     [7] AUX_ADC_DONE
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_ADC_DONE.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_DONE                           0x00000080
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_DONE_BITN                               7
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_DONE_M                         0x00000080
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_DONE_S                                  7
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_DONE_LOW                       0x00000080
#define AUX_EVCTL_EVTOMCUPOL_AUX_ADC_DONE_HIGH                      0x00000000

// Field:     [6] AUX_SMPH_AUTOTAKE_DONE
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_SMPH_AUTOTAKE_DONE.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_SMPH_AUTOTAKE_DONE                 0x00000040
#define AUX_EVCTL_EVTOMCUPOL_AUX_SMPH_AUTOTAKE_DONE_BITN                     6
#define AUX_EVCTL_EVTOMCUPOL_AUX_SMPH_AUTOTAKE_DONE_M               0x00000040
#define AUX_EVCTL_EVTOMCUPOL_AUX_SMPH_AUTOTAKE_DONE_S                        6
#define AUX_EVCTL_EVTOMCUPOL_AUX_SMPH_AUTOTAKE_DONE_LOW             0x00000040
#define AUX_EVCTL_EVTOMCUPOL_AUX_SMPH_AUTOTAKE_DONE_HIGH            0x00000000

// Field:     [5] AUX_TIMER1_EV
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_TIMER1_EV.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER1_EV                          0x00000020
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER1_EV_BITN                              5
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER1_EV_M                        0x00000020
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER1_EV_S                                 5
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER1_EV_LOW                      0x00000020
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER1_EV_HIGH                     0x00000000

// Field:     [4] AUX_TIMER0_EV
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_TIMER0_EV.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER0_EV                          0x00000010
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER0_EV_BITN                              4
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER0_EV_M                        0x00000010
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER0_EV_S                                 4
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER0_EV_LOW                      0x00000010
#define AUX_EVCTL_EVTOMCUPOL_AUX_TIMER0_EV_HIGH                     0x00000000

// Field:     [3] AUX_TDC_DONE
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_TDC_DONE.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_TDC_DONE                           0x00000008
#define AUX_EVCTL_EVTOMCUPOL_AUX_TDC_DONE_BITN                               3
#define AUX_EVCTL_EVTOMCUPOL_AUX_TDC_DONE_M                         0x00000008
#define AUX_EVCTL_EVTOMCUPOL_AUX_TDC_DONE_S                                  3
#define AUX_EVCTL_EVTOMCUPOL_AUX_TDC_DONE_LOW                       0x00000008
#define AUX_EVCTL_EVTOMCUPOL_AUX_TDC_DONE_HIGH                      0x00000000

// Field:     [2] AUX_COMPB
//
// Select the event source edge that sets EVTOMCUFLAGS.AUX_COMPB.
// ENUMs:
// FALL                     Falling edge
// RISE                     Rising edge
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPB                              0x00000004
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPB_BITN                                  2
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPB_M                            0x00000004
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPB_S                                     2
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPB_FALL                         0x00000004
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPB_RISE                         0x00000000

// Field:     [1] AUX_COMPA
//
// Select the event source edge that sets EVTOMCUFLAGS.AUX_COMPA.
// ENUMs:
// FALL                     Falling edge
// RISE                     Rising edge
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPA                              0x00000002
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPA_BITN                                  1
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPA_M                            0x00000002
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPA_S                                     1
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPA_FALL                         0x00000002
#define AUX_EVCTL_EVTOMCUPOL_AUX_COMPA_RISE                         0x00000000

// Field:     [0] AUX_WU_EV
//
// Select the event source level that sets EVTOMCUFLAGS.AUX_WU_EV.
// ENUMs:
// LOW                      Low level
// HIGH                     High level
#define AUX_EVCTL_EVTOMCUPOL_AUX_WU_EV                              0x00000001
#define AUX_EVCTL_EVTOMCUPOL_AUX_WU_EV_BITN                                  0
#define AUX_EVCTL_EVTOMCUPOL_AUX_WU_EV_M                            0x00000001
#define AUX_EVCTL_EVTOMCUPOL_AUX_WU_EV_S                                     0
#define AUX_EVCTL_EVTOMCUPOL_AUX_WU_EV_LOW                          0x00000001
#define AUX_EVCTL_EVTOMCUPOL_AUX_WU_EV_HIGH                         0x00000000

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVTOMCUFLAGSCLR
//
//*****************************************************************************
// Field:    [15] AUX_TIMER2_PULSE
//
// Write 1 to clear EVTOMCUFLAGS.AUX_TIMER2_PULSE.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_PULSE                  0x00008000
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_PULSE_BITN                     15
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_PULSE_M                0x00008000
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_PULSE_S                        15

// Field:    [14] AUX_TIMER2_EV3
//
// Write 1 to clear EVTOMCUFLAGS.AUX_TIMER2_EV3.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV3                    0x00004000
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV3_BITN                       14
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV3_M                  0x00004000
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV3_S                          14

// Field:    [13] AUX_TIMER2_EV2
//
// Write 1 to clear EVTOMCUFLAGS.AUX_TIMER2_EV2.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV2                    0x00002000
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV2_BITN                       13
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV2_M                  0x00002000
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV2_S                          13

// Field:    [12] AUX_TIMER2_EV1
//
// Write 1 to clear EVTOMCUFLAGS.AUX_TIMER2_EV1.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV1                    0x00001000
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV1_BITN                       12
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV1_M                  0x00001000
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV1_S                          12

// Field:    [11] AUX_TIMER2_EV0
//
// Write 1 to clear EVTOMCUFLAGS.AUX_TIMER2_EV0.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV0                    0x00000800
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV0_BITN                       11
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV0_M                  0x00000800
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER2_EV0_S                          11

// Field:    [10] AUX_ADC_IRQ
//
// Write 1 to clear EVTOMCUFLAGS.AUX_ADC_IRQ.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_IRQ                       0x00000400
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_IRQ_BITN                          10
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_IRQ_M                     0x00000400
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_IRQ_S                             10

// Field:     [9] MCU_OBSMUX0
//
// Write 1 to clear EVTOMCUFLAGS.MCU_OBSMUX0.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_MCU_OBSMUX0                       0x00000200
#define AUX_EVCTL_EVTOMCUFLAGSCLR_MCU_OBSMUX0_BITN                           9
#define AUX_EVCTL_EVTOMCUFLAGSCLR_MCU_OBSMUX0_M                     0x00000200
#define AUX_EVCTL_EVTOMCUFLAGSCLR_MCU_OBSMUX0_S                              9

// Field:     [8] AUX_ADC_FIFO_ALMOST_FULL
//
// Write 1 to clear EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_FIFO_ALMOST_FULL          0x00000100
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_FIFO_ALMOST_FULL_BITN              8
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_FIFO_ALMOST_FULL_M        0x00000100
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_FIFO_ALMOST_FULL_S                 8

// Field:     [7] AUX_ADC_DONE
//
// Write 1 to clear EVTOMCUFLAGS.AUX_ADC_DONE.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_DONE                      0x00000080
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_DONE_BITN                          7
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_DONE_M                    0x00000080
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_ADC_DONE_S                             7

// Field:     [6] AUX_SMPH_AUTOTAKE_DONE
//
// Write 1 to clear EVTOMCUFLAGS.AUX_SMPH_AUTOTAKE_DONE.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_SMPH_AUTOTAKE_DONE            0x00000040
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_SMPH_AUTOTAKE_DONE_BITN                6
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_SMPH_AUTOTAKE_DONE_M          0x00000040
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_SMPH_AUTOTAKE_DONE_S                   6

// Field:     [5] AUX_TIMER1_EV
//
// Write 1 to clear EVTOMCUFLAGS.AUX_TIMER1_EV.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER1_EV                     0x00000020
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER1_EV_BITN                         5
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER1_EV_M                   0x00000020
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER1_EV_S                            5

// Field:     [4] AUX_TIMER0_EV
//
// Write 1 to clear EVTOMCUFLAGS.AUX_TIMER0_EV.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER0_EV                     0x00000010
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER0_EV_BITN                         4
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER0_EV_M                   0x00000010
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TIMER0_EV_S                            4

// Field:     [3] AUX_TDC_DONE
//
// Write 1 to clear EVTOMCUFLAGS.AUX_TDC_DONE.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TDC_DONE                      0x00000008
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TDC_DONE_BITN                          3
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TDC_DONE_M                    0x00000008
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_TDC_DONE_S                             3

// Field:     [2] AUX_COMPB
//
// Write 1 to clear EVTOMCUFLAGS.AUX_COMPB.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_COMPB                         0x00000004
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_COMPB_BITN                             2
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_COMPB_M                       0x00000004
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_COMPB_S                                2

// Field:     [1] AUX_COMPA
//
// Write 1 to clear EVTOMCUFLAGS.AUX_COMPA.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_COMPA                         0x00000002
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_COMPA_BITN                             1
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_COMPA_M                       0x00000002
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_COMPA_S                                1

// Field:     [0] AUX_WU_EV
//
// Write 1 to clear EVTOMCUFLAGS.AUX_WU_EV.
//
// Read value is 0.
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_WU_EV                         0x00000001
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_WU_EV_BITN                             0
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_WU_EV_M                       0x00000001
#define AUX_EVCTL_EVTOMCUFLAGSCLR_AUX_WU_EV_S                                0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_COMBEVTOMCUMASK
//
//*****************************************************************************
// Field:    [15] AUX_TIMER2_PULSE
//
// EVTOMCUFLAGS.AUX_TIMER2_PULSE contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_PULSE                  0x00008000
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_PULSE_BITN                     15
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_PULSE_M                0x00008000
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_PULSE_S                        15

// Field:    [14] AUX_TIMER2_EV3
//
// EVTOMCUFLAGS.AUX_TIMER2_EV3 contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV3                    0x00004000
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV3_BITN                       14
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV3_M                  0x00004000
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV3_S                          14

// Field:    [13] AUX_TIMER2_EV2
//
// EVTOMCUFLAGS.AUX_TIMER2_EV2 contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV2                    0x00002000
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV2_BITN                       13
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV2_M                  0x00002000
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV2_S                          13

// Field:    [12] AUX_TIMER2_EV1
//
// EVTOMCUFLAGS.AUX_TIMER2_EV1 contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV1                    0x00001000
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV1_BITN                       12
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV1_M                  0x00001000
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV1_S                          12

// Field:    [11] AUX_TIMER2_EV0
//
// EVTOMCUFLAGS.AUX_TIMER2_EV0 contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV0                    0x00000800
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV0_BITN                       11
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV0_M                  0x00000800
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER2_EV0_S                          11

// Field:    [10] AUX_ADC_IRQ
//
// EVTOMCUFLAGS.AUX_ADC_IRQ contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_IRQ                       0x00000400
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_IRQ_BITN                          10
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_IRQ_M                     0x00000400
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_IRQ_S                             10

// Field:     [9] MCU_OBSMUX0
//
// EVTOMCUFLAGS.MCU_OBSMUX0 contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_MCU_OBSMUX0                       0x00000200
#define AUX_EVCTL_COMBEVTOMCUMASK_MCU_OBSMUX0_BITN                           9
#define AUX_EVCTL_COMBEVTOMCUMASK_MCU_OBSMUX0_M                     0x00000200
#define AUX_EVCTL_COMBEVTOMCUMASK_MCU_OBSMUX0_S                              9

// Field:     [8] AUX_ADC_FIFO_ALMOST_FULL
//
// EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_FIFO_ALMOST_FULL          0x00000100
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_FIFO_ALMOST_FULL_BITN              8
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_FIFO_ALMOST_FULL_M        0x00000100
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_FIFO_ALMOST_FULL_S                 8

// Field:     [7] AUX_ADC_DONE
//
// EVTOMCUFLAGS.AUX_ADC_DONE contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_DONE                      0x00000080
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_DONE_BITN                          7
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_DONE_M                    0x00000080
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_ADC_DONE_S                             7

// Field:     [6] AUX_SMPH_AUTOTAKE_DONE
//
// EVTOMCUFLAGS.AUX_SMPH_AUTOTAKE_DONE contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_SMPH_AUTOTAKE_DONE            0x00000040
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_SMPH_AUTOTAKE_DONE_BITN                6
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_SMPH_AUTOTAKE_DONE_M          0x00000040
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_SMPH_AUTOTAKE_DONE_S                   6

// Field:     [5] AUX_TIMER1_EV
//
// EVTOMCUFLAGS.AUX_TIMER1_EV contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER1_EV                     0x00000020
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER1_EV_BITN                         5
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER1_EV_M                   0x00000020
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER1_EV_S                            5

// Field:     [4] AUX_TIMER0_EV
//
// EVTOMCUFLAGS.AUX_TIMER0_EV contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER0_EV                     0x00000010
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER0_EV_BITN                         4
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER0_EV_M                   0x00000010
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TIMER0_EV_S                            4

// Field:     [3] AUX_TDC_DONE
//
// EVTOMCUFLAGS.AUX_TDC_DONE contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TDC_DONE                      0x00000008
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TDC_DONE_BITN                          3
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TDC_DONE_M                    0x00000008
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_TDC_DONE_S                             3

// Field:     [2] AUX_COMPB
//
// EVTOMCUFLAGS.AUX_COMPB contribution to the AUX_COMB event.
//
// 0: Exclude
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_COMPB                         0x00000004
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_COMPB_BITN                             2
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_COMPB_M                       0x00000004
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_COMPB_S                                2

// Field:     [1] AUX_COMPA
//
// EVTOMCUFLAGS.AUX_COMPA contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_COMPA                         0x00000002
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_COMPA_BITN                             1
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_COMPA_M                       0x00000002
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_COMPA_S                                1

// Field:     [0] AUX_WU_EV
//
// EVTOMCUFLAGS.AUX_WU_EV contribution to the AUX_COMB event.
//
// 0: Exclude.
// 1: Include.
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_WU_EV                         0x00000001
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_WU_EV_BITN                             0
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_WU_EV_M                       0x00000001
#define AUX_EVCTL_COMBEVTOMCUMASK_AUX_WU_EV_S                                0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVOBSCFG
//
//*****************************************************************************
// Field:   [5:0] EVOBS_SEL
//
// Select which event from the asynchronous event bus that represents
// AUX_EV_OBS in AUX_AIODIOn.
// ENUMs:
// AUX_TIMER2_CLKSW_RDY     EVSTAT3.AUX_TIMER2_CLKSWITCH_RDY
// AUX_DAC_HOLD_ACTIVE      EVSTAT3.AUX_DAC_HOLD_ACTIVE
// AUX_SMPH_AUTOTAKE_DONE   EVSTAT3.AUX_SMPH_AUTOTAKE_DONE
// AUX_ADC_FIFO_NOT_EMPTY   EVSTAT3.AUX_ADC_FIFO_NOT_EMPTY
// AUX_ADC_FIFO_ALMOST_FULL EVSTAT3.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_IRQ              EVSTAT3.AUX_ADC_IRQ
// AUX_ADC_DONE             EVSTAT3.AUX_ADC_DONE
// AUX_ISRC_RESET_N         EVSTAT3.AUX_ISRC_RESET_N
// AUX_TDC_DONE             EVSTAT3.AUX_TDC_DONE
// AUX_TIMER0_EV            EVSTAT3.AUX_TIMER0_EV
// AUX_TIMER1_EV            EVSTAT3.AUX_TIMER1_EV
// AUX_TIMER2_PULSE         EVSTAT3.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           EVSTAT3.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           EVSTAT3.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           EVSTAT3.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           EVSTAT3.AUX_TIMER2_EV0
// AUX_COMPB                EVSTAT2.AUX_COMPB
// AUX_COMPA                EVSTAT2.AUX_COMPA
// MCU_OBSMUX1              EVSTAT2.MCU_OBSMUX1
// MCU_OBSMUX0              EVSTAT2.MCU_OBSMUX0
// MCU_EV                   EVSTAT2.MCU_EV
// ACLK_REF                 EVSTAT2.ACLK_REF
// VDDR_RECHARGE            EVSTAT2.VDDR_RECHARGE
// MCU_ACTIVE               EVSTAT2.MCU_ACTIVE
// PWR_DWN                  EVSTAT2.PWR_DWN
// SCLK_LF                  EVSTAT2.SCLK_LF
// AON_BATMON_TEMP_UPD      EVSTAT2.AON_BATMON_TEMP_UPD
// AON_BATMON_BAT_UPD       EVSTAT2.AON_BATMON_BAT_UPD
// AON_RTC_4KHZ             EVSTAT2.AON_RTC_4KHZ
// AON_RTC_CH2_DLY          EVSTAT2.AON_RTC_CH2_DLY
// AON_RTC_CH2              EVSTAT2.AON_RTC_CH2
// MANUAL_EV                EVSTAT2.MANUAL_EV
// AUXIO31                  EVSTAT1.AUXIO31
// AUXIO30                  EVSTAT1.AUXIO30
// AUXIO29                  EVSTAT1.AUXIO29
// AUXIO28                  EVSTAT1.AUXIO28
// AUXIO27                  EVSTAT1.AUXIO27
// AUXIO26                  EVSTAT1.AUXIO26
// AUXIO25                  EVSTAT1.AUXIO25
// AUXIO24                  EVSTAT1.AUXIO24
// AUXIO23                  EVSTAT1.AUXIO23
// AUXIO22                  EVSTAT1.AUXIO22
// AUXIO21                  EVSTAT1.AUXIO21
// AUXIO20                  EVSTAT1.AUXIO20
// AUXIO19                  EVSTAT1.AUXIO19
// AUXIO18                  EVSTAT1.AUXIO18
// AUXIO17                  EVSTAT1.AUXIO17
// AUXIO16                  EVSTAT1.AUXIO16
// AUXIO15                  EVSTAT0.AUXIO15
// AUXIO14                  EVSTAT0.AUXIO14
// AUXIO13                  EVSTAT0.AUXIO13
// AUXIO12                  EVSTAT0.AUXIO12
// AUXIO11                  EVSTAT0.AUXIO11
// AUXIO10                  EVSTAT0.AUXIO10
// AUXIO9                   EVSTAT0.AUXIO9
// AUXIO8                   EVSTAT0.AUXIO8
// AUXIO7                   EVSTAT0.AUXIO7
// AUXIO6                   EVSTAT0.AUXIO6
// AUXIO5                   EVSTAT0.AUXIO5
// AUXIO4                   EVSTAT0.AUXIO4
// AUXIO3                   EVSTAT0.AUXIO3
// AUXIO2                   EVSTAT0.AUXIO2
// AUXIO1                   EVSTAT0.AUXIO1
// AUXIO0                   EVSTAT0.AUXIO0
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_W                                       6
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_M                              0x0000003F
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_S                                       0
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TIMER2_CLKSW_RDY           0x0000003F
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_DAC_HOLD_ACTIVE            0x0000003E
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_SMPH_AUTOTAKE_DONE         0x0000003D
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_ADC_FIFO_NOT_EMPTY         0x0000003C
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_ADC_FIFO_ALMOST_FULL       0x0000003B
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_ADC_IRQ                    0x0000003A
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_ADC_DONE                   0x00000039
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_ISRC_RESET_N               0x00000038
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TDC_DONE                   0x00000037
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TIMER0_EV                  0x00000036
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TIMER1_EV                  0x00000035
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TIMER2_PULSE               0x00000034
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TIMER2_EV3                 0x00000033
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TIMER2_EV2                 0x00000032
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TIMER2_EV1                 0x00000031
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_TIMER2_EV0                 0x00000030
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_COMPB                      0x0000002F
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUX_COMPA                      0x0000002E
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_MCU_OBSMUX1                    0x0000002D
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_MCU_OBSMUX0                    0x0000002C
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_MCU_EV                         0x0000002B
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_ACLK_REF                       0x0000002A
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_VDDR_RECHARGE                  0x00000029
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_MCU_ACTIVE                     0x00000028
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_PWR_DWN                        0x00000027
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_SCLK_LF                        0x00000026
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AON_BATMON_TEMP_UPD            0x00000025
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AON_BATMON_BAT_UPD             0x00000024
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AON_RTC_4KHZ                   0x00000023
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AON_RTC_CH2_DLY                0x00000022
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AON_RTC_CH2                    0x00000021
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_MANUAL_EV                      0x00000020
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO31                        0x0000001F
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO30                        0x0000001E
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO29                        0x0000001D
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO28                        0x0000001C
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO27                        0x0000001B
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO26                        0x0000001A
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO25                        0x00000019
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO24                        0x00000018
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO23                        0x00000017
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO22                        0x00000016
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO21                        0x00000015
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO20                        0x00000014
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO19                        0x00000013
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO18                        0x00000012
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO17                        0x00000011
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO16                        0x00000010
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO15                        0x0000000F
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO14                        0x0000000E
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO13                        0x0000000D
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO12                        0x0000000C
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO11                        0x0000000B
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO10                        0x0000000A
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO9                         0x00000009
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO8                         0x00000008
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO7                         0x00000007
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO6                         0x00000006
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO5                         0x00000005
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO4                         0x00000004
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO3                         0x00000003
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO2                         0x00000002
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO1                         0x00000001
#define AUX_EVCTL_EVOBSCFG_EVOBS_SEL_AUXIO0                         0x00000000

//*****************************************************************************
//
// Register: AUX_EVCTL_O_PROGDLY
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// VALUE decrements to 0 at a rate of 1 MHz.
//
// The event AUX_PROG_DLY_IDLE is high when VALUE is 0, otherwise it is low.
//
// Only use the programmable delay counter and the AUX_PROG_DLY_IDLE event when
// AUX_SYSIF:OPMODEACK.ACK equals A or LP.
//
// Decrementation of VALUE halts when either is true:
// - AUX_SCE:CTL.DBG_FREEZE_EN is set and system CPU is halted in debug mode.
// - AUX_SYSIF:TIMERHALT.PROGDLY is set.
#define AUX_EVCTL_PROGDLY_VALUE_W                                           16
#define AUX_EVCTL_PROGDLY_VALUE_M                                   0x0000FFFF
#define AUX_EVCTL_PROGDLY_VALUE_S                                            0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_MANUAL
//
//*****************************************************************************
// Field:     [0] EV
//
// This bit field sets the value of EVSTAT2.MANUAL_EV.
#define AUX_EVCTL_MANUAL_EV                                         0x00000001
#define AUX_EVCTL_MANUAL_EV_BITN                                             0
#define AUX_EVCTL_MANUAL_EV_M                                       0x00000001
#define AUX_EVCTL_MANUAL_EV_S                                                0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT0L
//
//*****************************************************************************
// Field:   [7:0] ALIAS_EV
//
// Alias of EVSTAT0 event 7 down to 0.
#define AUX_EVCTL_EVSTAT0L_ALIAS_EV_W                                        8
#define AUX_EVCTL_EVSTAT0L_ALIAS_EV_M                               0x000000FF
#define AUX_EVCTL_EVSTAT0L_ALIAS_EV_S                                        0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT0H
//
//*****************************************************************************
// Field:   [7:0] ALIAS_EV
//
// Alias of EVSTAT0 event 15 down to 8.
#define AUX_EVCTL_EVSTAT0H_ALIAS_EV_W                                        8
#define AUX_EVCTL_EVSTAT0H_ALIAS_EV_M                               0x000000FF
#define AUX_EVCTL_EVSTAT0H_ALIAS_EV_S                                        0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT1L
//
//*****************************************************************************
// Field:   [7:0] ALIAS_EV
//
// Alias of EVSTAT1 event 7 down to 0.
#define AUX_EVCTL_EVSTAT1L_ALIAS_EV_W                                        8
#define AUX_EVCTL_EVSTAT1L_ALIAS_EV_M                               0x000000FF
#define AUX_EVCTL_EVSTAT1L_ALIAS_EV_S                                        0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT1H
//
//*****************************************************************************
// Field:   [7:0] ALIAS_EV
//
// Alias of EVSTAT1 event 15 down to 8.
#define AUX_EVCTL_EVSTAT1H_ALIAS_EV_W                                        8
#define AUX_EVCTL_EVSTAT1H_ALIAS_EV_M                               0x000000FF
#define AUX_EVCTL_EVSTAT1H_ALIAS_EV_S                                        0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT2L
//
//*****************************************************************************
// Field:   [7:0] ALIAS_EV
//
// Alias of EVSTAT2 event 7 down to 0.
#define AUX_EVCTL_EVSTAT2L_ALIAS_EV_W                                        8
#define AUX_EVCTL_EVSTAT2L_ALIAS_EV_M                               0x000000FF
#define AUX_EVCTL_EVSTAT2L_ALIAS_EV_S                                        0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT2H
//
//*****************************************************************************
// Field:   [7:0] ALIAS_EV
//
// Alias of EVSTAT2 event 15 down to 8.
#define AUX_EVCTL_EVSTAT2H_ALIAS_EV_W                                        8
#define AUX_EVCTL_EVSTAT2H_ALIAS_EV_M                               0x000000FF
#define AUX_EVCTL_EVSTAT2H_ALIAS_EV_S                                        0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT3L
//
//*****************************************************************************
// Field:   [7:0] ALIAS_EV
//
// Alias of EVSTAT3 event 7 down to 0.
#define AUX_EVCTL_EVSTAT3L_ALIAS_EV_W                                        8
#define AUX_EVCTL_EVSTAT3L_ALIAS_EV_M                               0x000000FF
#define AUX_EVCTL_EVSTAT3L_ALIAS_EV_S                                        0

//*****************************************************************************
//
// Register: AUX_EVCTL_O_EVSTAT3H
//
//*****************************************************************************
// Field:   [7:0] ALIAS_EV
//
// Alias of EVSTAT3 event 15 down to 8.
#define AUX_EVCTL_EVSTAT3H_ALIAS_EV_W                                        8
#define AUX_EVCTL_EVSTAT3H_ALIAS_EV_M                               0x000000FF
#define AUX_EVCTL_EVSTAT3H_ALIAS_EV_S                                        0


#endif // __AUX_EVCTL__
