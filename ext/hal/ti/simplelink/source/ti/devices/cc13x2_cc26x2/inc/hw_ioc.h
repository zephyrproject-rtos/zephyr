/******************************************************************************
*  Filename:       hw_ioc_h
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

#ifndef __HW_IOC_H__
#define __HW_IOC_H__

//*****************************************************************************
//
// This section defines the register offsets of
// IOC component
//
//*****************************************************************************
// Configuration of DIO0
#define IOC_O_IOCFG0                                                0x00000000

// Configuration of DIO1
#define IOC_O_IOCFG1                                                0x00000004

// Configuration of DIO2
#define IOC_O_IOCFG2                                                0x00000008

// Configuration of DIO3
#define IOC_O_IOCFG3                                                0x0000000C

// Configuration of DIO4
#define IOC_O_IOCFG4                                                0x00000010

// Configuration of DIO5
#define IOC_O_IOCFG5                                                0x00000014

// Configuration of DIO6
#define IOC_O_IOCFG6                                                0x00000018

// Configuration of DIO7
#define IOC_O_IOCFG7                                                0x0000001C

// Configuration of DIO8
#define IOC_O_IOCFG8                                                0x00000020

// Configuration of DIO9
#define IOC_O_IOCFG9                                                0x00000024

// Configuration of DIO10
#define IOC_O_IOCFG10                                               0x00000028

// Configuration of DIO11
#define IOC_O_IOCFG11                                               0x0000002C

// Configuration of DIO12
#define IOC_O_IOCFG12                                               0x00000030

// Configuration of DIO13
#define IOC_O_IOCFG13                                               0x00000034

// Configuration of DIO14
#define IOC_O_IOCFG14                                               0x00000038

// Configuration of DIO15
#define IOC_O_IOCFG15                                               0x0000003C

// Configuration of DIO16
#define IOC_O_IOCFG16                                               0x00000040

// Configuration of DIO17
#define IOC_O_IOCFG17                                               0x00000044

// Configuration of DIO18
#define IOC_O_IOCFG18                                               0x00000048

// Configuration of DIO19
#define IOC_O_IOCFG19                                               0x0000004C

// Configuration of DIO20
#define IOC_O_IOCFG20                                               0x00000050

// Configuration of DIO21
#define IOC_O_IOCFG21                                               0x00000054

// Configuration of DIO22
#define IOC_O_IOCFG22                                               0x00000058

// Configuration of DIO23
#define IOC_O_IOCFG23                                               0x0000005C

// Configuration of DIO24
#define IOC_O_IOCFG24                                               0x00000060

// Configuration of DIO25
#define IOC_O_IOCFG25                                               0x00000064

// Configuration of DIO26
#define IOC_O_IOCFG26                                               0x00000068

// Configuration of DIO27
#define IOC_O_IOCFG27                                               0x0000006C

// Configuration of DIO28
#define IOC_O_IOCFG28                                               0x00000070

// Configuration of DIO29
#define IOC_O_IOCFG29                                               0x00000074

// Configuration of DIO30
#define IOC_O_IOCFG30                                               0x00000078

// Configuration of DIO31
#define IOC_O_IOCFG31                                               0x0000007C

//*****************************************************************************
//
// Register: IOC_O_IOCFG0
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG0_HYST_EN                                          0x40000000
#define IOC_IOCFG0_HYST_EN_BITN                                             30
#define IOC_IOCFG0_HYST_EN_M                                        0x40000000
#define IOC_IOCFG0_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG0_IE                                               0x20000000
#define IOC_IOCFG0_IE_BITN                                                  29
#define IOC_IOCFG0_IE_M                                             0x20000000
#define IOC_IOCFG0_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG0_WU_CFG_W                                                  2
#define IOC_IOCFG0_WU_CFG_M                                         0x18000000
#define IOC_IOCFG0_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input/output
// OPENSRC                  Open Source
//                          Normal input / outut
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG0_IOMODE_W                                                  3
#define IOC_IOCFG0_IOMODE_M                                         0x07000000
#define IOC_IOCFG0_IOMODE_S                                                 24
#define IOC_IOCFG0_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG0_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG0_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG0_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG0_IOMODE_INV                                       0x01000000
#define IOC_IOCFG0_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG0_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG0_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG0_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG0_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG0_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG0_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG0_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG0_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG0_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG0_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG0_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG0_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG0_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG0_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG0_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG0_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG0_EDGE_DET_W                                                2
#define IOC_IOCFG0_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG0_EDGE_DET_S                                               16
#define IOC_IOCFG0_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG0_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG0_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG0_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG0_PULL_CTL_W                                                2
#define IOC_IOCFG0_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG0_PULL_CTL_S                                               13
#define IOC_IOCFG0_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG0_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG0_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG0_SLEW_RED                                         0x00001000
#define IOC_IOCFG0_SLEW_RED_BITN                                            12
#define IOC_IOCFG0_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG0_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG0_IOCURR_W                                                  2
#define IOC_IOCFG0_IOCURR_M                                         0x00000C00
#define IOC_IOCFG0_IOCURR_S                                                 10
#define IOC_IOCFG0_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG0_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG0_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG0_IOSTR_W                                                   2
#define IOC_IOCFG0_IOSTR_M                                          0x00000300
#define IOC_IOCFG0_IOSTR_S                                                   8
#define IOC_IOCFG0_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG0_IOSTR_MED                                        0x00000200
#define IOC_IOCFG0_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG0_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG0_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG0_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG0_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG0_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG0_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG0_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG0_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG0_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO0
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG0_PORT_ID_W                                                 6
#define IOC_IOCFG0_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG0_PORT_ID_S                                                 0
#define IOC_IOCFG0_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG0_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG0_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG0_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG0_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG0_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG0_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG0_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG0_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG0_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG0_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG0_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG0_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG0_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG0_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG0_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG0_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG0_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG0_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG0_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG0_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG0_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG0_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG0_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG0_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG0_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG0_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG0_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG0_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG0_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG0_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG0_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG0_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG0_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG0_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG0_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG0_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG0_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG0_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG0_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG0_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG0_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG0_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG0_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG0_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG0_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG1
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG1_HYST_EN                                          0x40000000
#define IOC_IOCFG1_HYST_EN_BITN                                             30
#define IOC_IOCFG1_HYST_EN_M                                        0x40000000
#define IOC_IOCFG1_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG1_IE                                               0x20000000
#define IOC_IOCFG1_IE_BITN                                                  29
#define IOC_IOCFG1_IE_M                                             0x20000000
#define IOC_IOCFG1_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG1_WU_CFG_W                                                  2
#define IOC_IOCFG1_WU_CFG_M                                         0x18000000
#define IOC_IOCFG1_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG1_IOMODE_W                                                  3
#define IOC_IOCFG1_IOMODE_M                                         0x07000000
#define IOC_IOCFG1_IOMODE_S                                                 24
#define IOC_IOCFG1_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG1_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG1_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG1_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG1_IOMODE_INV                                       0x01000000
#define IOC_IOCFG1_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG1_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG1_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG1_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG1_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG1_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG1_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG1_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG1_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG1_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG1_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG1_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG1_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG1_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG1_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG1_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG1_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG1_EDGE_DET_W                                                2
#define IOC_IOCFG1_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG1_EDGE_DET_S                                               16
#define IOC_IOCFG1_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG1_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG1_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG1_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG1_PULL_CTL_W                                                2
#define IOC_IOCFG1_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG1_PULL_CTL_S                                               13
#define IOC_IOCFG1_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG1_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG1_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG1_SLEW_RED                                         0x00001000
#define IOC_IOCFG1_SLEW_RED_BITN                                            12
#define IOC_IOCFG1_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG1_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG1_IOCURR_W                                                  2
#define IOC_IOCFG1_IOCURR_M                                         0x00000C00
#define IOC_IOCFG1_IOCURR_S                                                 10
#define IOC_IOCFG1_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG1_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG1_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG1_IOSTR_W                                                   2
#define IOC_IOCFG1_IOSTR_M                                          0x00000300
#define IOC_IOCFG1_IOSTR_S                                                   8
#define IOC_IOCFG1_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG1_IOSTR_MED                                        0x00000200
#define IOC_IOCFG1_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG1_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG1_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG1_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG1_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG1_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG1_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG1_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG1_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG1_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO1
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG1_PORT_ID_W                                                 6
#define IOC_IOCFG1_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG1_PORT_ID_S                                                 0
#define IOC_IOCFG1_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG1_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG1_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG1_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG1_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG1_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG1_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG1_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG1_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG1_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG1_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG1_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG1_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG1_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG1_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG1_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG1_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG1_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG1_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG1_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG1_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG1_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG1_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG1_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG1_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG1_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG1_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG1_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG1_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG1_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG1_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG1_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG1_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG1_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG1_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG1_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG1_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG1_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG1_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG1_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG1_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG1_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG1_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG1_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG1_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG1_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG2
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG2_HYST_EN                                          0x40000000
#define IOC_IOCFG2_HYST_EN_BITN                                             30
#define IOC_IOCFG2_HYST_EN_M                                        0x40000000
#define IOC_IOCFG2_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG2_IE                                               0x20000000
#define IOC_IOCFG2_IE_BITN                                                  29
#define IOC_IOCFG2_IE_M                                             0x20000000
#define IOC_IOCFG2_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG2_WU_CFG_W                                                  2
#define IOC_IOCFG2_WU_CFG_M                                         0x18000000
#define IOC_IOCFG2_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG2_IOMODE_W                                                  3
#define IOC_IOCFG2_IOMODE_M                                         0x07000000
#define IOC_IOCFG2_IOMODE_S                                                 24
#define IOC_IOCFG2_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG2_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG2_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG2_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG2_IOMODE_INV                                       0x01000000
#define IOC_IOCFG2_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG2_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG2_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG2_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG2_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG2_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG2_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG2_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG2_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG2_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG2_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG2_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG2_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG2_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG2_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG2_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG2_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG2_EDGE_DET_W                                                2
#define IOC_IOCFG2_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG2_EDGE_DET_S                                               16
#define IOC_IOCFG2_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG2_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG2_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG2_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG2_PULL_CTL_W                                                2
#define IOC_IOCFG2_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG2_PULL_CTL_S                                               13
#define IOC_IOCFG2_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG2_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG2_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG2_SLEW_RED                                         0x00001000
#define IOC_IOCFG2_SLEW_RED_BITN                                            12
#define IOC_IOCFG2_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG2_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG2_IOCURR_W                                                  2
#define IOC_IOCFG2_IOCURR_M                                         0x00000C00
#define IOC_IOCFG2_IOCURR_S                                                 10
#define IOC_IOCFG2_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG2_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG2_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG2_IOSTR_W                                                   2
#define IOC_IOCFG2_IOSTR_M                                          0x00000300
#define IOC_IOCFG2_IOSTR_S                                                   8
#define IOC_IOCFG2_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG2_IOSTR_MED                                        0x00000200
#define IOC_IOCFG2_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG2_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG2_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG2_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG2_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG2_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG2_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG2_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG2_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG2_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO2
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG2_PORT_ID_W                                                 6
#define IOC_IOCFG2_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG2_PORT_ID_S                                                 0
#define IOC_IOCFG2_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG2_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG2_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG2_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG2_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG2_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG2_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG2_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG2_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG2_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG2_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG2_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG2_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG2_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG2_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG2_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG2_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG2_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG2_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG2_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG2_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG2_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG2_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG2_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG2_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG2_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG2_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG2_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG2_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG2_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG2_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG2_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG2_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG2_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG2_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG2_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG2_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG2_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG2_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG2_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG2_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG2_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG2_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG2_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG2_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG2_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG3
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG3_HYST_EN                                          0x40000000
#define IOC_IOCFG3_HYST_EN_BITN                                             30
#define IOC_IOCFG3_HYST_EN_M                                        0x40000000
#define IOC_IOCFG3_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG3_IE                                               0x20000000
#define IOC_IOCFG3_IE_BITN                                                  29
#define IOC_IOCFG3_IE_M                                             0x20000000
#define IOC_IOCFG3_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG3_WU_CFG_W                                                  2
#define IOC_IOCFG3_WU_CFG_M                                         0x18000000
#define IOC_IOCFG3_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG3_IOMODE_W                                                  3
#define IOC_IOCFG3_IOMODE_M                                         0x07000000
#define IOC_IOCFG3_IOMODE_S                                                 24
#define IOC_IOCFG3_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG3_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG3_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG3_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG3_IOMODE_INV                                       0x01000000
#define IOC_IOCFG3_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG3_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG3_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG3_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG3_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG3_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG3_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG3_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG3_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG3_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG3_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG3_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG3_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG3_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG3_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG3_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG3_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG3_EDGE_DET_W                                                2
#define IOC_IOCFG3_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG3_EDGE_DET_S                                               16
#define IOC_IOCFG3_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG3_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG3_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG3_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG3_PULL_CTL_W                                                2
#define IOC_IOCFG3_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG3_PULL_CTL_S                                               13
#define IOC_IOCFG3_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG3_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG3_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG3_SLEW_RED                                         0x00001000
#define IOC_IOCFG3_SLEW_RED_BITN                                            12
#define IOC_IOCFG3_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG3_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG3_IOCURR_W                                                  2
#define IOC_IOCFG3_IOCURR_M                                         0x00000C00
#define IOC_IOCFG3_IOCURR_S                                                 10
#define IOC_IOCFG3_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG3_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG3_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG3_IOSTR_W                                                   2
#define IOC_IOCFG3_IOSTR_M                                          0x00000300
#define IOC_IOCFG3_IOSTR_S                                                   8
#define IOC_IOCFG3_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG3_IOSTR_MED                                        0x00000200
#define IOC_IOCFG3_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG3_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG3_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG3_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG3_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG3_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG3_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG3_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG3_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG3_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO3
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG3_PORT_ID_W                                                 6
#define IOC_IOCFG3_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG3_PORT_ID_S                                                 0
#define IOC_IOCFG3_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG3_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG3_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG3_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG3_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG3_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG3_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG3_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG3_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG3_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG3_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG3_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG3_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG3_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG3_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG3_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG3_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG3_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG3_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG3_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG3_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG3_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG3_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG3_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG3_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG3_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG3_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG3_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG3_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG3_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG3_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG3_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG3_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG3_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG3_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG3_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG3_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG3_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG3_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG3_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG3_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG3_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG3_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG3_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG3_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG3_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG4
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG4_HYST_EN                                          0x40000000
#define IOC_IOCFG4_HYST_EN_BITN                                             30
#define IOC_IOCFG4_HYST_EN_M                                        0x40000000
#define IOC_IOCFG4_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG4_IE                                               0x20000000
#define IOC_IOCFG4_IE_BITN                                                  29
#define IOC_IOCFG4_IE_M                                             0x20000000
#define IOC_IOCFG4_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG4_WU_CFG_W                                                  2
#define IOC_IOCFG4_WU_CFG_M                                         0x18000000
#define IOC_IOCFG4_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG4_IOMODE_W                                                  3
#define IOC_IOCFG4_IOMODE_M                                         0x07000000
#define IOC_IOCFG4_IOMODE_S                                                 24
#define IOC_IOCFG4_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG4_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG4_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG4_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG4_IOMODE_INV                                       0x01000000
#define IOC_IOCFG4_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG4_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG4_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG4_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG4_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG4_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG4_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG4_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG4_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG4_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG4_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG4_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG4_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG4_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG4_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG4_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG4_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG4_EDGE_DET_W                                                2
#define IOC_IOCFG4_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG4_EDGE_DET_S                                               16
#define IOC_IOCFG4_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG4_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG4_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG4_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG4_PULL_CTL_W                                                2
#define IOC_IOCFG4_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG4_PULL_CTL_S                                               13
#define IOC_IOCFG4_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG4_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG4_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG4_SLEW_RED                                         0x00001000
#define IOC_IOCFG4_SLEW_RED_BITN                                            12
#define IOC_IOCFG4_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG4_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG4_IOCURR_W                                                  2
#define IOC_IOCFG4_IOCURR_M                                         0x00000C00
#define IOC_IOCFG4_IOCURR_S                                                 10
#define IOC_IOCFG4_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG4_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG4_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG4_IOSTR_W                                                   2
#define IOC_IOCFG4_IOSTR_M                                          0x00000300
#define IOC_IOCFG4_IOSTR_S                                                   8
#define IOC_IOCFG4_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG4_IOSTR_MED                                        0x00000200
#define IOC_IOCFG4_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG4_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG4_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG4_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG4_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG4_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG4_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG4_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG4_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG4_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO4
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG4_PORT_ID_W                                                 6
#define IOC_IOCFG4_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG4_PORT_ID_S                                                 0
#define IOC_IOCFG4_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG4_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG4_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG4_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG4_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG4_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG4_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG4_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG4_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG4_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG4_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG4_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG4_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG4_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG4_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG4_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG4_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG4_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG4_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG4_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG4_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG4_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG4_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG4_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG4_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG4_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG4_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG4_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG4_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG4_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG4_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG4_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG4_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG4_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG4_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG4_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG4_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG4_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG4_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG4_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG4_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG4_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG4_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG4_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG4_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG4_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG5
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG5_HYST_EN                                          0x40000000
#define IOC_IOCFG5_HYST_EN_BITN                                             30
#define IOC_IOCFG5_HYST_EN_M                                        0x40000000
#define IOC_IOCFG5_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG5_IE                                               0x20000000
#define IOC_IOCFG5_IE_BITN                                                  29
#define IOC_IOCFG5_IE_M                                             0x20000000
#define IOC_IOCFG5_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG5_WU_CFG_W                                                  2
#define IOC_IOCFG5_WU_CFG_M                                         0x18000000
#define IOC_IOCFG5_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG5_IOMODE_W                                                  3
#define IOC_IOCFG5_IOMODE_M                                         0x07000000
#define IOC_IOCFG5_IOMODE_S                                                 24
#define IOC_IOCFG5_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG5_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG5_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG5_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG5_IOMODE_INV                                       0x01000000
#define IOC_IOCFG5_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG5_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG5_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG5_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG5_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG5_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG5_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG5_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG5_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG5_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG5_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG5_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG5_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG5_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG5_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG5_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG5_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG5_EDGE_DET_W                                                2
#define IOC_IOCFG5_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG5_EDGE_DET_S                                               16
#define IOC_IOCFG5_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG5_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG5_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG5_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG5_PULL_CTL_W                                                2
#define IOC_IOCFG5_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG5_PULL_CTL_S                                               13
#define IOC_IOCFG5_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG5_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG5_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG5_SLEW_RED                                         0x00001000
#define IOC_IOCFG5_SLEW_RED_BITN                                            12
#define IOC_IOCFG5_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG5_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG5_IOCURR_W                                                  2
#define IOC_IOCFG5_IOCURR_M                                         0x00000C00
#define IOC_IOCFG5_IOCURR_S                                                 10
#define IOC_IOCFG5_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG5_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG5_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG5_IOSTR_W                                                   2
#define IOC_IOCFG5_IOSTR_M                                          0x00000300
#define IOC_IOCFG5_IOSTR_S                                                   8
#define IOC_IOCFG5_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG5_IOSTR_MED                                        0x00000200
#define IOC_IOCFG5_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG5_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG5_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG5_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG5_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG5_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG5_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG5_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG5_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG5_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO5
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG5_PORT_ID_W                                                 6
#define IOC_IOCFG5_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG5_PORT_ID_S                                                 0
#define IOC_IOCFG5_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG5_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG5_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG5_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG5_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG5_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG5_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG5_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG5_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG5_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG5_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG5_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG5_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG5_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG5_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG5_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG5_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG5_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG5_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG5_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG5_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG5_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG5_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG5_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG5_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG5_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG5_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG5_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG5_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG5_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG5_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG5_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG5_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG5_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG5_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG5_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG5_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG5_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG5_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG5_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG5_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG5_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG5_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG5_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG5_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG5_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG6
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG6_HYST_EN                                          0x40000000
#define IOC_IOCFG6_HYST_EN_BITN                                             30
#define IOC_IOCFG6_HYST_EN_M                                        0x40000000
#define IOC_IOCFG6_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG6_IE                                               0x20000000
#define IOC_IOCFG6_IE_BITN                                                  29
#define IOC_IOCFG6_IE_M                                             0x20000000
#define IOC_IOCFG6_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG6_WU_CFG_W                                                  2
#define IOC_IOCFG6_WU_CFG_M                                         0x18000000
#define IOC_IOCFG6_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG6_IOMODE_W                                                  3
#define IOC_IOCFG6_IOMODE_M                                         0x07000000
#define IOC_IOCFG6_IOMODE_S                                                 24
#define IOC_IOCFG6_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG6_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG6_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG6_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG6_IOMODE_INV                                       0x01000000
#define IOC_IOCFG6_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG6_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG6_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG6_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG6_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG6_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG6_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG6_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG6_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG6_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG6_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG6_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG6_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG6_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG6_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG6_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG6_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG6_EDGE_DET_W                                                2
#define IOC_IOCFG6_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG6_EDGE_DET_S                                               16
#define IOC_IOCFG6_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG6_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG6_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG6_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG6_PULL_CTL_W                                                2
#define IOC_IOCFG6_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG6_PULL_CTL_S                                               13
#define IOC_IOCFG6_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG6_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG6_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG6_SLEW_RED                                         0x00001000
#define IOC_IOCFG6_SLEW_RED_BITN                                            12
#define IOC_IOCFG6_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG6_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG6_IOCURR_W                                                  2
#define IOC_IOCFG6_IOCURR_M                                         0x00000C00
#define IOC_IOCFG6_IOCURR_S                                                 10
#define IOC_IOCFG6_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG6_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG6_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG6_IOSTR_W                                                   2
#define IOC_IOCFG6_IOSTR_M                                          0x00000300
#define IOC_IOCFG6_IOSTR_S                                                   8
#define IOC_IOCFG6_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG6_IOSTR_MED                                        0x00000200
#define IOC_IOCFG6_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG6_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG6_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG6_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG6_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG6_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG6_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG6_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG6_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG6_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO6
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG6_PORT_ID_W                                                 6
#define IOC_IOCFG6_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG6_PORT_ID_S                                                 0
#define IOC_IOCFG6_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG6_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG6_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG6_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG6_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG6_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG6_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG6_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG6_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG6_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG6_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG6_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG6_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG6_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG6_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG6_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG6_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG6_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG6_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG6_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG6_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG6_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG6_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG6_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG6_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG6_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG6_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG6_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG6_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG6_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG6_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG6_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG6_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG6_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG6_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG6_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG6_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG6_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG6_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG6_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG6_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG6_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG6_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG6_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG6_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG6_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG7
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG7_HYST_EN                                          0x40000000
#define IOC_IOCFG7_HYST_EN_BITN                                             30
#define IOC_IOCFG7_HYST_EN_M                                        0x40000000
#define IOC_IOCFG7_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG7_IE                                               0x20000000
#define IOC_IOCFG7_IE_BITN                                                  29
#define IOC_IOCFG7_IE_M                                             0x20000000
#define IOC_IOCFG7_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG7_WU_CFG_W                                                  2
#define IOC_IOCFG7_WU_CFG_M                                         0x18000000
#define IOC_IOCFG7_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG7_IOMODE_W                                                  3
#define IOC_IOCFG7_IOMODE_M                                         0x07000000
#define IOC_IOCFG7_IOMODE_S                                                 24
#define IOC_IOCFG7_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG7_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG7_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG7_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG7_IOMODE_INV                                       0x01000000
#define IOC_IOCFG7_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG7_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG7_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG7_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG7_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG7_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG7_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG7_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG7_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG7_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG7_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG7_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG7_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG7_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG7_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG7_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG7_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG7_EDGE_DET_W                                                2
#define IOC_IOCFG7_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG7_EDGE_DET_S                                               16
#define IOC_IOCFG7_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG7_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG7_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG7_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG7_PULL_CTL_W                                                2
#define IOC_IOCFG7_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG7_PULL_CTL_S                                               13
#define IOC_IOCFG7_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG7_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG7_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG7_SLEW_RED                                         0x00001000
#define IOC_IOCFG7_SLEW_RED_BITN                                            12
#define IOC_IOCFG7_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG7_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG7_IOCURR_W                                                  2
#define IOC_IOCFG7_IOCURR_M                                         0x00000C00
#define IOC_IOCFG7_IOCURR_S                                                 10
#define IOC_IOCFG7_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG7_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG7_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG7_IOSTR_W                                                   2
#define IOC_IOCFG7_IOSTR_M                                          0x00000300
#define IOC_IOCFG7_IOSTR_S                                                   8
#define IOC_IOCFG7_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG7_IOSTR_MED                                        0x00000200
#define IOC_IOCFG7_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG7_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG7_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG7_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG7_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG7_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG7_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG7_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG7_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG7_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO7
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG7_PORT_ID_W                                                 6
#define IOC_IOCFG7_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG7_PORT_ID_S                                                 0
#define IOC_IOCFG7_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG7_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG7_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG7_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG7_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG7_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG7_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG7_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG7_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG7_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG7_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG7_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG7_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG7_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG7_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG7_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG7_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG7_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG7_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG7_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG7_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG7_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG7_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG7_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG7_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG7_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG7_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG7_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG7_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG7_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG7_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG7_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG7_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG7_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG7_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG7_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG7_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG7_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG7_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG7_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG7_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG7_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG7_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG7_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG7_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG7_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG8
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG8_HYST_EN                                          0x40000000
#define IOC_IOCFG8_HYST_EN_BITN                                             30
#define IOC_IOCFG8_HYST_EN_M                                        0x40000000
#define IOC_IOCFG8_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG8_IE                                               0x20000000
#define IOC_IOCFG8_IE_BITN                                                  29
#define IOC_IOCFG8_IE_M                                             0x20000000
#define IOC_IOCFG8_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG8_WU_CFG_W                                                  2
#define IOC_IOCFG8_WU_CFG_M                                         0x18000000
#define IOC_IOCFG8_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG8_IOMODE_W                                                  3
#define IOC_IOCFG8_IOMODE_M                                         0x07000000
#define IOC_IOCFG8_IOMODE_S                                                 24
#define IOC_IOCFG8_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG8_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG8_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG8_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG8_IOMODE_INV                                       0x01000000
#define IOC_IOCFG8_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG8_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG8_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG8_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG8_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG8_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG8_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG8_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG8_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG8_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG8_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG8_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG8_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG8_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG8_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG8_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG8_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG8_EDGE_DET_W                                                2
#define IOC_IOCFG8_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG8_EDGE_DET_S                                               16
#define IOC_IOCFG8_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG8_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG8_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG8_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG8_PULL_CTL_W                                                2
#define IOC_IOCFG8_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG8_PULL_CTL_S                                               13
#define IOC_IOCFG8_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG8_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG8_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG8_SLEW_RED                                         0x00001000
#define IOC_IOCFG8_SLEW_RED_BITN                                            12
#define IOC_IOCFG8_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG8_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG8_IOCURR_W                                                  2
#define IOC_IOCFG8_IOCURR_M                                         0x00000C00
#define IOC_IOCFG8_IOCURR_S                                                 10
#define IOC_IOCFG8_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG8_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG8_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG8_IOSTR_W                                                   2
#define IOC_IOCFG8_IOSTR_M                                          0x00000300
#define IOC_IOCFG8_IOSTR_S                                                   8
#define IOC_IOCFG8_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG8_IOSTR_MED                                        0x00000200
#define IOC_IOCFG8_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG8_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG8_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG8_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG8_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG8_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG8_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG8_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG8_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG8_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO8
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG8_PORT_ID_W                                                 6
#define IOC_IOCFG8_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG8_PORT_ID_S                                                 0
#define IOC_IOCFG8_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG8_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG8_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG8_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG8_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG8_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG8_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG8_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG8_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG8_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG8_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG8_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG8_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG8_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG8_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG8_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG8_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG8_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG8_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG8_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG8_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG8_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG8_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG8_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG8_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG8_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG8_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG8_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG8_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG8_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG8_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG8_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG8_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG8_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG8_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG8_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG8_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG8_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG8_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG8_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG8_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG8_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG8_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG8_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG8_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG8_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG9
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG9_HYST_EN                                          0x40000000
#define IOC_IOCFG9_HYST_EN_BITN                                             30
#define IOC_IOCFG9_HYST_EN_M                                        0x40000000
#define IOC_IOCFG9_HYST_EN_S                                                30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG9_IE                                               0x20000000
#define IOC_IOCFG9_IE_BITN                                                  29
#define IOC_IOCFG9_IE_M                                             0x20000000
#define IOC_IOCFG9_IE_S                                                     29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG9_WU_CFG_W                                                  2
#define IOC_IOCFG9_WU_CFG_M                                         0x18000000
#define IOC_IOCFG9_WU_CFG_S                                                 27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG9_IOMODE_W                                                  3
#define IOC_IOCFG9_IOMODE_M                                         0x07000000
#define IOC_IOCFG9_IOMODE_S                                                 24
#define IOC_IOCFG9_IOMODE_OPENSRC_INV                               0x07000000
#define IOC_IOCFG9_IOMODE_OPENSRC                                   0x06000000
#define IOC_IOCFG9_IOMODE_OPENDR_INV                                0x05000000
#define IOC_IOCFG9_IOMODE_OPENDR                                    0x04000000
#define IOC_IOCFG9_IOMODE_INV                                       0x01000000
#define IOC_IOCFG9_IOMODE_NORMAL                                    0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG9_IOEV_AON_PROG2_EN                                0x00800000
#define IOC_IOCFG9_IOEV_AON_PROG2_EN_BITN                                   23
#define IOC_IOCFG9_IOEV_AON_PROG2_EN_M                              0x00800000
#define IOC_IOCFG9_IOEV_AON_PROG2_EN_S                                      23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG9_IOEV_AON_PROG1_EN                                0x00400000
#define IOC_IOCFG9_IOEV_AON_PROG1_EN_BITN                                   22
#define IOC_IOCFG9_IOEV_AON_PROG1_EN_M                              0x00400000
#define IOC_IOCFG9_IOEV_AON_PROG1_EN_S                                      22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG9_IOEV_AON_PROG0_EN                                0x00200000
#define IOC_IOCFG9_IOEV_AON_PROG0_EN_BITN                                   21
#define IOC_IOCFG9_IOEV_AON_PROG0_EN_M                              0x00200000
#define IOC_IOCFG9_IOEV_AON_PROG0_EN_S                                      21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG9_EDGE_IRQ_EN                                      0x00040000
#define IOC_IOCFG9_EDGE_IRQ_EN_BITN                                         18
#define IOC_IOCFG9_EDGE_IRQ_EN_M                                    0x00040000
#define IOC_IOCFG9_EDGE_IRQ_EN_S                                            18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG9_EDGE_DET_W                                                2
#define IOC_IOCFG9_EDGE_DET_M                                       0x00030000
#define IOC_IOCFG9_EDGE_DET_S                                               16
#define IOC_IOCFG9_EDGE_DET_BOTH                                    0x00030000
#define IOC_IOCFG9_EDGE_DET_POS                                     0x00020000
#define IOC_IOCFG9_EDGE_DET_NEG                                     0x00010000
#define IOC_IOCFG9_EDGE_DET_NONE                                    0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG9_PULL_CTL_W                                                2
#define IOC_IOCFG9_PULL_CTL_M                                       0x00006000
#define IOC_IOCFG9_PULL_CTL_S                                               13
#define IOC_IOCFG9_PULL_CTL_DIS                                     0x00006000
#define IOC_IOCFG9_PULL_CTL_UP                                      0x00004000
#define IOC_IOCFG9_PULL_CTL_DWN                                     0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG9_SLEW_RED                                         0x00001000
#define IOC_IOCFG9_SLEW_RED_BITN                                            12
#define IOC_IOCFG9_SLEW_RED_M                                       0x00001000
#define IOC_IOCFG9_SLEW_RED_S                                               12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG9_IOCURR_W                                                  2
#define IOC_IOCFG9_IOCURR_M                                         0x00000C00
#define IOC_IOCFG9_IOCURR_S                                                 10
#define IOC_IOCFG9_IOCURR_4_8MA                                     0x00000800
#define IOC_IOCFG9_IOCURR_4MA                                       0x00000400
#define IOC_IOCFG9_IOCURR_2MA                                       0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG9_IOSTR_W                                                   2
#define IOC_IOCFG9_IOSTR_M                                          0x00000300
#define IOC_IOCFG9_IOSTR_S                                                   8
#define IOC_IOCFG9_IOSTR_MAX                                        0x00000300
#define IOC_IOCFG9_IOSTR_MED                                        0x00000200
#define IOC_IOCFG9_IOSTR_MIN                                        0x00000100
#define IOC_IOCFG9_IOSTR_AUTO                                       0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG9_IOEV_RTC_EN                                      0x00000080
#define IOC_IOCFG9_IOEV_RTC_EN_BITN                                          7
#define IOC_IOCFG9_IOEV_RTC_EN_M                                    0x00000080
#define IOC_IOCFG9_IOEV_RTC_EN_S                                             7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG9_IOEV_MCU_WU_EN                                   0x00000040
#define IOC_IOCFG9_IOEV_MCU_WU_EN_BITN                                       6
#define IOC_IOCFG9_IOEV_MCU_WU_EN_M                                 0x00000040
#define IOC_IOCFG9_IOEV_MCU_WU_EN_S                                          6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO9
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG9_PORT_ID_W                                                 6
#define IOC_IOCFG9_PORT_ID_M                                        0x0000003F
#define IOC_IOCFG9_PORT_ID_S                                                 0
#define IOC_IOCFG9_PORT_ID_RFC_SMI_CL_IN                            0x00000038
#define IOC_IOCFG9_PORT_ID_RFC_SMI_CL_OUT                           0x00000037
#define IOC_IOCFG9_PORT_ID_RFC_SMI_DL_IN                            0x00000036
#define IOC_IOCFG9_PORT_ID_RFC_SMI_DL_OUT                           0x00000035
#define IOC_IOCFG9_PORT_ID_RFC_GPI1                                 0x00000034
#define IOC_IOCFG9_PORT_ID_RFC_GPI0                                 0x00000033
#define IOC_IOCFG9_PORT_ID_RFC_GPO3                                 0x00000032
#define IOC_IOCFG9_PORT_ID_RFC_GPO2                                 0x00000031
#define IOC_IOCFG9_PORT_ID_RFC_GPO1                                 0x00000030
#define IOC_IOCFG9_PORT_ID_RFC_GPO0                                 0x0000002F
#define IOC_IOCFG9_PORT_ID_RFC_TRC                                  0x0000002E
#define IOC_IOCFG9_PORT_ID_I2S_MCLK                                 0x00000029
#define IOC_IOCFG9_PORT_ID_I2S_BCLK                                 0x00000028
#define IOC_IOCFG9_PORT_ID_I2S_WCLK                                 0x00000027
#define IOC_IOCFG9_PORT_ID_I2S_AD1                                  0x00000026
#define IOC_IOCFG9_PORT_ID_I2S_AD0                                  0x00000025
#define IOC_IOCFG9_PORT_ID_SSI1_CLK                                 0x00000024
#define IOC_IOCFG9_PORT_ID_SSI1_FSS                                 0x00000023
#define IOC_IOCFG9_PORT_ID_SSI1_TX                                  0x00000022
#define IOC_IOCFG9_PORT_ID_SSI1_RX                                  0x00000021
#define IOC_IOCFG9_PORT_ID_CPU_SWV                                  0x00000020
#define IOC_IOCFG9_PORT_ID_PORT_EVENT7                              0x0000001E
#define IOC_IOCFG9_PORT_ID_PORT_EVENT6                              0x0000001D
#define IOC_IOCFG9_PORT_ID_PORT_EVENT5                              0x0000001C
#define IOC_IOCFG9_PORT_ID_PORT_EVENT4                              0x0000001B
#define IOC_IOCFG9_PORT_ID_PORT_EVENT3                              0x0000001A
#define IOC_IOCFG9_PORT_ID_PORT_EVENT2                              0x00000019
#define IOC_IOCFG9_PORT_ID_PORT_EVENT1                              0x00000018
#define IOC_IOCFG9_PORT_ID_PORT_EVENT0                              0x00000017
#define IOC_IOCFG9_PORT_ID_UART1_RTS                                0x00000016
#define IOC_IOCFG9_PORT_ID_UART1_CTS                                0x00000015
#define IOC_IOCFG9_PORT_ID_UART1_TX                                 0x00000014
#define IOC_IOCFG9_PORT_ID_UART1_RX                                 0x00000013
#define IOC_IOCFG9_PORT_ID_UART0_RTS                                0x00000012
#define IOC_IOCFG9_PORT_ID_UART0_CTS                                0x00000011
#define IOC_IOCFG9_PORT_ID_UART0_TX                                 0x00000010
#define IOC_IOCFG9_PORT_ID_UART0_RX                                 0x0000000F
#define IOC_IOCFG9_PORT_ID_I2C_MSSCL                                0x0000000E
#define IOC_IOCFG9_PORT_ID_I2C_MSSDA                                0x0000000D
#define IOC_IOCFG9_PORT_ID_SSI0_CLK                                 0x0000000C
#define IOC_IOCFG9_PORT_ID_SSI0_FSS                                 0x0000000B
#define IOC_IOCFG9_PORT_ID_SSI0_TX                                  0x0000000A
#define IOC_IOCFG9_PORT_ID_SSI0_RX                                  0x00000009
#define IOC_IOCFG9_PORT_ID_AUX_IO                                   0x00000008
#define IOC_IOCFG9_PORT_ID_AON_CLK32K                               0x00000007
#define IOC_IOCFG9_PORT_ID_GPIO                                     0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG10
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG10_HYST_EN                                         0x40000000
#define IOC_IOCFG10_HYST_EN_BITN                                            30
#define IOC_IOCFG10_HYST_EN_M                                       0x40000000
#define IOC_IOCFG10_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG10_IE                                              0x20000000
#define IOC_IOCFG10_IE_BITN                                                 29
#define IOC_IOCFG10_IE_M                                            0x20000000
#define IOC_IOCFG10_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG10_WU_CFG_W                                                 2
#define IOC_IOCFG10_WU_CFG_M                                        0x18000000
#define IOC_IOCFG10_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG10_IOMODE_W                                                 3
#define IOC_IOCFG10_IOMODE_M                                        0x07000000
#define IOC_IOCFG10_IOMODE_S                                                24
#define IOC_IOCFG10_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG10_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG10_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG10_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG10_IOMODE_INV                                      0x01000000
#define IOC_IOCFG10_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG10_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG10_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG10_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG10_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG10_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG10_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG10_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG10_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG10_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG10_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG10_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG10_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG10_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG10_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG10_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG10_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG10_EDGE_DET_W                                               2
#define IOC_IOCFG10_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG10_EDGE_DET_S                                              16
#define IOC_IOCFG10_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG10_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG10_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG10_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG10_PULL_CTL_W                                               2
#define IOC_IOCFG10_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG10_PULL_CTL_S                                              13
#define IOC_IOCFG10_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG10_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG10_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG10_SLEW_RED                                        0x00001000
#define IOC_IOCFG10_SLEW_RED_BITN                                           12
#define IOC_IOCFG10_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG10_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG10_IOCURR_W                                                 2
#define IOC_IOCFG10_IOCURR_M                                        0x00000C00
#define IOC_IOCFG10_IOCURR_S                                                10
#define IOC_IOCFG10_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG10_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG10_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG10_IOSTR_W                                                  2
#define IOC_IOCFG10_IOSTR_M                                         0x00000300
#define IOC_IOCFG10_IOSTR_S                                                  8
#define IOC_IOCFG10_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG10_IOSTR_MED                                       0x00000200
#define IOC_IOCFG10_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG10_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG10_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG10_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG10_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG10_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG10_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG10_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG10_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG10_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO10
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG10_PORT_ID_W                                                6
#define IOC_IOCFG10_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG10_PORT_ID_S                                                0
#define IOC_IOCFG10_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG10_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG10_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG10_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG10_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG10_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG10_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG10_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG10_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG10_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG10_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG10_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG10_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG10_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG10_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG10_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG10_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG10_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG10_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG10_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG10_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG10_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG10_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG10_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG10_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG10_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG10_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG10_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG10_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG10_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG10_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG10_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG10_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG10_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG10_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG10_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG10_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG10_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG10_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG10_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG10_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG10_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG10_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG10_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG10_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG10_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG11
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG11_HYST_EN                                         0x40000000
#define IOC_IOCFG11_HYST_EN_BITN                                            30
#define IOC_IOCFG11_HYST_EN_M                                       0x40000000
#define IOC_IOCFG11_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG11_IE                                              0x20000000
#define IOC_IOCFG11_IE_BITN                                                 29
#define IOC_IOCFG11_IE_M                                            0x20000000
#define IOC_IOCFG11_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG11_WU_CFG_W                                                 2
#define IOC_IOCFG11_WU_CFG_M                                        0x18000000
#define IOC_IOCFG11_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG11_IOMODE_W                                                 3
#define IOC_IOCFG11_IOMODE_M                                        0x07000000
#define IOC_IOCFG11_IOMODE_S                                                24
#define IOC_IOCFG11_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG11_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG11_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG11_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG11_IOMODE_INV                                      0x01000000
#define IOC_IOCFG11_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG11_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG11_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG11_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG11_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG11_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG11_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG11_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG11_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG11_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG11_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG11_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG11_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG11_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG11_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG11_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG11_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG11_EDGE_DET_W                                               2
#define IOC_IOCFG11_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG11_EDGE_DET_S                                              16
#define IOC_IOCFG11_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG11_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG11_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG11_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG11_PULL_CTL_W                                               2
#define IOC_IOCFG11_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG11_PULL_CTL_S                                              13
#define IOC_IOCFG11_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG11_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG11_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG11_SLEW_RED                                        0x00001000
#define IOC_IOCFG11_SLEW_RED_BITN                                           12
#define IOC_IOCFG11_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG11_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG11_IOCURR_W                                                 2
#define IOC_IOCFG11_IOCURR_M                                        0x00000C00
#define IOC_IOCFG11_IOCURR_S                                                10
#define IOC_IOCFG11_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG11_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG11_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG11_IOSTR_W                                                  2
#define IOC_IOCFG11_IOSTR_M                                         0x00000300
#define IOC_IOCFG11_IOSTR_S                                                  8
#define IOC_IOCFG11_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG11_IOSTR_MED                                       0x00000200
#define IOC_IOCFG11_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG11_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG11_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG11_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG11_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG11_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG11_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG11_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG11_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG11_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO11
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG11_PORT_ID_W                                                6
#define IOC_IOCFG11_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG11_PORT_ID_S                                                0
#define IOC_IOCFG11_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG11_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG11_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG11_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG11_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG11_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG11_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG11_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG11_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG11_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG11_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG11_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG11_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG11_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG11_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG11_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG11_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG11_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG11_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG11_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG11_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG11_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG11_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG11_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG11_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG11_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG11_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG11_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG11_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG11_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG11_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG11_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG11_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG11_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG11_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG11_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG11_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG11_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG11_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG11_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG11_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG11_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG11_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG11_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG11_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG11_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG12
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG12_HYST_EN                                         0x40000000
#define IOC_IOCFG12_HYST_EN_BITN                                            30
#define IOC_IOCFG12_HYST_EN_M                                       0x40000000
#define IOC_IOCFG12_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG12_IE                                              0x20000000
#define IOC_IOCFG12_IE_BITN                                                 29
#define IOC_IOCFG12_IE_M                                            0x20000000
#define IOC_IOCFG12_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG12_WU_CFG_W                                                 2
#define IOC_IOCFG12_WU_CFG_M                                        0x18000000
#define IOC_IOCFG12_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG12_IOMODE_W                                                 3
#define IOC_IOCFG12_IOMODE_M                                        0x07000000
#define IOC_IOCFG12_IOMODE_S                                                24
#define IOC_IOCFG12_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG12_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG12_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG12_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG12_IOMODE_INV                                      0x01000000
#define IOC_IOCFG12_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG12_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG12_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG12_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG12_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG12_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG12_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG12_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG12_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG12_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG12_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG12_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG12_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG12_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG12_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG12_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG12_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG12_EDGE_DET_W                                               2
#define IOC_IOCFG12_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG12_EDGE_DET_S                                              16
#define IOC_IOCFG12_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG12_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG12_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG12_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG12_PULL_CTL_W                                               2
#define IOC_IOCFG12_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG12_PULL_CTL_S                                              13
#define IOC_IOCFG12_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG12_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG12_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG12_SLEW_RED                                        0x00001000
#define IOC_IOCFG12_SLEW_RED_BITN                                           12
#define IOC_IOCFG12_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG12_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG12_IOCURR_W                                                 2
#define IOC_IOCFG12_IOCURR_M                                        0x00000C00
#define IOC_IOCFG12_IOCURR_S                                                10
#define IOC_IOCFG12_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG12_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG12_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG12_IOSTR_W                                                  2
#define IOC_IOCFG12_IOSTR_M                                         0x00000300
#define IOC_IOCFG12_IOSTR_S                                                  8
#define IOC_IOCFG12_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG12_IOSTR_MED                                       0x00000200
#define IOC_IOCFG12_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG12_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG12_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG12_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG12_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG12_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG12_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG12_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG12_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG12_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO12
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG12_PORT_ID_W                                                6
#define IOC_IOCFG12_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG12_PORT_ID_S                                                0
#define IOC_IOCFG12_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG12_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG12_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG12_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG12_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG12_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG12_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG12_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG12_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG12_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG12_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG12_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG12_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG12_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG12_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG12_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG12_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG12_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG12_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG12_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG12_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG12_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG12_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG12_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG12_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG12_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG12_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG12_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG12_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG12_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG12_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG12_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG12_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG12_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG12_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG12_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG12_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG12_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG12_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG12_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG12_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG12_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG12_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG12_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG12_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG12_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG13
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG13_HYST_EN                                         0x40000000
#define IOC_IOCFG13_HYST_EN_BITN                                            30
#define IOC_IOCFG13_HYST_EN_M                                       0x40000000
#define IOC_IOCFG13_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG13_IE                                              0x20000000
#define IOC_IOCFG13_IE_BITN                                                 29
#define IOC_IOCFG13_IE_M                                            0x20000000
#define IOC_IOCFG13_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG13_WU_CFG_W                                                 2
#define IOC_IOCFG13_WU_CFG_M                                        0x18000000
#define IOC_IOCFG13_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG13_IOMODE_W                                                 3
#define IOC_IOCFG13_IOMODE_M                                        0x07000000
#define IOC_IOCFG13_IOMODE_S                                                24
#define IOC_IOCFG13_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG13_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG13_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG13_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG13_IOMODE_INV                                      0x01000000
#define IOC_IOCFG13_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG13_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG13_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG13_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG13_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG13_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG13_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG13_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG13_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG13_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG13_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG13_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG13_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG13_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG13_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG13_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG13_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG13_EDGE_DET_W                                               2
#define IOC_IOCFG13_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG13_EDGE_DET_S                                              16
#define IOC_IOCFG13_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG13_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG13_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG13_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG13_PULL_CTL_W                                               2
#define IOC_IOCFG13_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG13_PULL_CTL_S                                              13
#define IOC_IOCFG13_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG13_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG13_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG13_SLEW_RED                                        0x00001000
#define IOC_IOCFG13_SLEW_RED_BITN                                           12
#define IOC_IOCFG13_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG13_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG13_IOCURR_W                                                 2
#define IOC_IOCFG13_IOCURR_M                                        0x00000C00
#define IOC_IOCFG13_IOCURR_S                                                10
#define IOC_IOCFG13_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG13_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG13_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG13_IOSTR_W                                                  2
#define IOC_IOCFG13_IOSTR_M                                         0x00000300
#define IOC_IOCFG13_IOSTR_S                                                  8
#define IOC_IOCFG13_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG13_IOSTR_MED                                       0x00000200
#define IOC_IOCFG13_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG13_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG13_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG13_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG13_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG13_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG13_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG13_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG13_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG13_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO13
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG13_PORT_ID_W                                                6
#define IOC_IOCFG13_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG13_PORT_ID_S                                                0
#define IOC_IOCFG13_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG13_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG13_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG13_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG13_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG13_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG13_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG13_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG13_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG13_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG13_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG13_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG13_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG13_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG13_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG13_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG13_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG13_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG13_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG13_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG13_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG13_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG13_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG13_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG13_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG13_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG13_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG13_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG13_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG13_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG13_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG13_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG13_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG13_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG13_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG13_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG13_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG13_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG13_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG13_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG13_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG13_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG13_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG13_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG13_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG13_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG14
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG14_HYST_EN                                         0x40000000
#define IOC_IOCFG14_HYST_EN_BITN                                            30
#define IOC_IOCFG14_HYST_EN_M                                       0x40000000
#define IOC_IOCFG14_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG14_IE                                              0x20000000
#define IOC_IOCFG14_IE_BITN                                                 29
#define IOC_IOCFG14_IE_M                                            0x20000000
#define IOC_IOCFG14_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG14_WU_CFG_W                                                 2
#define IOC_IOCFG14_WU_CFG_M                                        0x18000000
#define IOC_IOCFG14_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG14_IOMODE_W                                                 3
#define IOC_IOCFG14_IOMODE_M                                        0x07000000
#define IOC_IOCFG14_IOMODE_S                                                24
#define IOC_IOCFG14_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG14_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG14_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG14_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG14_IOMODE_INV                                      0x01000000
#define IOC_IOCFG14_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG14_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG14_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG14_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG14_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG14_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG14_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG14_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG14_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG14_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG14_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG14_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG14_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG14_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG14_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG14_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG14_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG14_EDGE_DET_W                                               2
#define IOC_IOCFG14_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG14_EDGE_DET_S                                              16
#define IOC_IOCFG14_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG14_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG14_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG14_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG14_PULL_CTL_W                                               2
#define IOC_IOCFG14_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG14_PULL_CTL_S                                              13
#define IOC_IOCFG14_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG14_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG14_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG14_SLEW_RED                                        0x00001000
#define IOC_IOCFG14_SLEW_RED_BITN                                           12
#define IOC_IOCFG14_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG14_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG14_IOCURR_W                                                 2
#define IOC_IOCFG14_IOCURR_M                                        0x00000C00
#define IOC_IOCFG14_IOCURR_S                                                10
#define IOC_IOCFG14_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG14_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG14_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG14_IOSTR_W                                                  2
#define IOC_IOCFG14_IOSTR_M                                         0x00000300
#define IOC_IOCFG14_IOSTR_S                                                  8
#define IOC_IOCFG14_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG14_IOSTR_MED                                       0x00000200
#define IOC_IOCFG14_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG14_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG14_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG14_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG14_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG14_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG14_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG14_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG14_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG14_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO14
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG14_PORT_ID_W                                                6
#define IOC_IOCFG14_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG14_PORT_ID_S                                                0
#define IOC_IOCFG14_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG14_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG14_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG14_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG14_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG14_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG14_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG14_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG14_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG14_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG14_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG14_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG14_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG14_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG14_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG14_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG14_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG14_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG14_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG14_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG14_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG14_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG14_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG14_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG14_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG14_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG14_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG14_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG14_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG14_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG14_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG14_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG14_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG14_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG14_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG14_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG14_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG14_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG14_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG14_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG14_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG14_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG14_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG14_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG14_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG14_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG15
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG15_HYST_EN                                         0x40000000
#define IOC_IOCFG15_HYST_EN_BITN                                            30
#define IOC_IOCFG15_HYST_EN_M                                       0x40000000
#define IOC_IOCFG15_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG15_IE                                              0x20000000
#define IOC_IOCFG15_IE_BITN                                                 29
#define IOC_IOCFG15_IE_M                                            0x20000000
#define IOC_IOCFG15_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG15_WU_CFG_W                                                 2
#define IOC_IOCFG15_WU_CFG_M                                        0x18000000
#define IOC_IOCFG15_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG15_IOMODE_W                                                 3
#define IOC_IOCFG15_IOMODE_M                                        0x07000000
#define IOC_IOCFG15_IOMODE_S                                                24
#define IOC_IOCFG15_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG15_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG15_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG15_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG15_IOMODE_INV                                      0x01000000
#define IOC_IOCFG15_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG15_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG15_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG15_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG15_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG15_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG15_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG15_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG15_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG15_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG15_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG15_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG15_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG15_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG15_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG15_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG15_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG15_EDGE_DET_W                                               2
#define IOC_IOCFG15_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG15_EDGE_DET_S                                              16
#define IOC_IOCFG15_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG15_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG15_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG15_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG15_PULL_CTL_W                                               2
#define IOC_IOCFG15_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG15_PULL_CTL_S                                              13
#define IOC_IOCFG15_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG15_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG15_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG15_SLEW_RED                                        0x00001000
#define IOC_IOCFG15_SLEW_RED_BITN                                           12
#define IOC_IOCFG15_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG15_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG15_IOCURR_W                                                 2
#define IOC_IOCFG15_IOCURR_M                                        0x00000C00
#define IOC_IOCFG15_IOCURR_S                                                10
#define IOC_IOCFG15_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG15_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG15_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG15_IOSTR_W                                                  2
#define IOC_IOCFG15_IOSTR_M                                         0x00000300
#define IOC_IOCFG15_IOSTR_S                                                  8
#define IOC_IOCFG15_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG15_IOSTR_MED                                       0x00000200
#define IOC_IOCFG15_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG15_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG15_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG15_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG15_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG15_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG15_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG15_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG15_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG15_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO15
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG15_PORT_ID_W                                                6
#define IOC_IOCFG15_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG15_PORT_ID_S                                                0
#define IOC_IOCFG15_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG15_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG15_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG15_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG15_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG15_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG15_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG15_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG15_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG15_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG15_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG15_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG15_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG15_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG15_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG15_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG15_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG15_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG15_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG15_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG15_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG15_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG15_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG15_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG15_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG15_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG15_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG15_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG15_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG15_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG15_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG15_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG15_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG15_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG15_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG15_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG15_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG15_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG15_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG15_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG15_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG15_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG15_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG15_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG15_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG15_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG16
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG16_HYST_EN                                         0x40000000
#define IOC_IOCFG16_HYST_EN_BITN                                            30
#define IOC_IOCFG16_HYST_EN_M                                       0x40000000
#define IOC_IOCFG16_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG16_IE                                              0x20000000
#define IOC_IOCFG16_IE_BITN                                                 29
#define IOC_IOCFG16_IE_M                                            0x20000000
#define IOC_IOCFG16_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG16_WU_CFG_W                                                 2
#define IOC_IOCFG16_WU_CFG_M                                        0x18000000
#define IOC_IOCFG16_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG16_IOMODE_W                                                 3
#define IOC_IOCFG16_IOMODE_M                                        0x07000000
#define IOC_IOCFG16_IOMODE_S                                                24
#define IOC_IOCFG16_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG16_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG16_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG16_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG16_IOMODE_INV                                      0x01000000
#define IOC_IOCFG16_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG16_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG16_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG16_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG16_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG16_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG16_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG16_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG16_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG16_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG16_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG16_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG16_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG16_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG16_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG16_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG16_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG16_EDGE_DET_W                                               2
#define IOC_IOCFG16_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG16_EDGE_DET_S                                              16
#define IOC_IOCFG16_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG16_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG16_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG16_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG16_PULL_CTL_W                                               2
#define IOC_IOCFG16_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG16_PULL_CTL_S                                              13
#define IOC_IOCFG16_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG16_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG16_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG16_SLEW_RED                                        0x00001000
#define IOC_IOCFG16_SLEW_RED_BITN                                           12
#define IOC_IOCFG16_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG16_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG16_IOCURR_W                                                 2
#define IOC_IOCFG16_IOCURR_M                                        0x00000C00
#define IOC_IOCFG16_IOCURR_S                                                10
#define IOC_IOCFG16_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG16_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG16_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG16_IOSTR_W                                                  2
#define IOC_IOCFG16_IOSTR_M                                         0x00000300
#define IOC_IOCFG16_IOSTR_S                                                  8
#define IOC_IOCFG16_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG16_IOSTR_MED                                       0x00000200
#define IOC_IOCFG16_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG16_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG16_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG16_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG16_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG16_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG16_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG16_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG16_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG16_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO16
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG16_PORT_ID_W                                                6
#define IOC_IOCFG16_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG16_PORT_ID_S                                                0
#define IOC_IOCFG16_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG16_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG16_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG16_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG16_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG16_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG16_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG16_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG16_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG16_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG16_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG16_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG16_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG16_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG16_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG16_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG16_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG16_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG16_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG16_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG16_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG16_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG16_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG16_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG16_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG16_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG16_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG16_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG16_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG16_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG16_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG16_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG16_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG16_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG16_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG16_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG16_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG16_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG16_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG16_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG16_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG16_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG16_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG16_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG16_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG16_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG17
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG17_HYST_EN                                         0x40000000
#define IOC_IOCFG17_HYST_EN_BITN                                            30
#define IOC_IOCFG17_HYST_EN_M                                       0x40000000
#define IOC_IOCFG17_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG17_IE                                              0x20000000
#define IOC_IOCFG17_IE_BITN                                                 29
#define IOC_IOCFG17_IE_M                                            0x20000000
#define IOC_IOCFG17_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG17_WU_CFG_W                                                 2
#define IOC_IOCFG17_WU_CFG_M                                        0x18000000
#define IOC_IOCFG17_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG17_IOMODE_W                                                 3
#define IOC_IOCFG17_IOMODE_M                                        0x07000000
#define IOC_IOCFG17_IOMODE_S                                                24
#define IOC_IOCFG17_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG17_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG17_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG17_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG17_IOMODE_INV                                      0x01000000
#define IOC_IOCFG17_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG17_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG17_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG17_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG17_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG17_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG17_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG17_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG17_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG17_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG17_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG17_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG17_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG17_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG17_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG17_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG17_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG17_EDGE_DET_W                                               2
#define IOC_IOCFG17_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG17_EDGE_DET_S                                              16
#define IOC_IOCFG17_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG17_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG17_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG17_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG17_PULL_CTL_W                                               2
#define IOC_IOCFG17_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG17_PULL_CTL_S                                              13
#define IOC_IOCFG17_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG17_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG17_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG17_SLEW_RED                                        0x00001000
#define IOC_IOCFG17_SLEW_RED_BITN                                           12
#define IOC_IOCFG17_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG17_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG17_IOCURR_W                                                 2
#define IOC_IOCFG17_IOCURR_M                                        0x00000C00
#define IOC_IOCFG17_IOCURR_S                                                10
#define IOC_IOCFG17_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG17_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG17_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG17_IOSTR_W                                                  2
#define IOC_IOCFG17_IOSTR_M                                         0x00000300
#define IOC_IOCFG17_IOSTR_S                                                  8
#define IOC_IOCFG17_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG17_IOSTR_MED                                       0x00000200
#define IOC_IOCFG17_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG17_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG17_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG17_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG17_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG17_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG17_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG17_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG17_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG17_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO17
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG17_PORT_ID_W                                                6
#define IOC_IOCFG17_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG17_PORT_ID_S                                                0
#define IOC_IOCFG17_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG17_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG17_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG17_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG17_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG17_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG17_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG17_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG17_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG17_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG17_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG17_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG17_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG17_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG17_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG17_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG17_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG17_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG17_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG17_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG17_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG17_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG17_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG17_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG17_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG17_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG17_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG17_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG17_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG17_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG17_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG17_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG17_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG17_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG17_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG17_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG17_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG17_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG17_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG17_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG17_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG17_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG17_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG17_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG17_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG17_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG18
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG18_HYST_EN                                         0x40000000
#define IOC_IOCFG18_HYST_EN_BITN                                            30
#define IOC_IOCFG18_HYST_EN_M                                       0x40000000
#define IOC_IOCFG18_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG18_IE                                              0x20000000
#define IOC_IOCFG18_IE_BITN                                                 29
#define IOC_IOCFG18_IE_M                                            0x20000000
#define IOC_IOCFG18_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG18_WU_CFG_W                                                 2
#define IOC_IOCFG18_WU_CFG_M                                        0x18000000
#define IOC_IOCFG18_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG18_IOMODE_W                                                 3
#define IOC_IOCFG18_IOMODE_M                                        0x07000000
#define IOC_IOCFG18_IOMODE_S                                                24
#define IOC_IOCFG18_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG18_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG18_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG18_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG18_IOMODE_INV                                      0x01000000
#define IOC_IOCFG18_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG18_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG18_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG18_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG18_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG18_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG18_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG18_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG18_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG18_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG18_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG18_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG18_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG18_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG18_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG18_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG18_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG18_EDGE_DET_W                                               2
#define IOC_IOCFG18_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG18_EDGE_DET_S                                              16
#define IOC_IOCFG18_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG18_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG18_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG18_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG18_PULL_CTL_W                                               2
#define IOC_IOCFG18_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG18_PULL_CTL_S                                              13
#define IOC_IOCFG18_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG18_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG18_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG18_SLEW_RED                                        0x00001000
#define IOC_IOCFG18_SLEW_RED_BITN                                           12
#define IOC_IOCFG18_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG18_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG18_IOCURR_W                                                 2
#define IOC_IOCFG18_IOCURR_M                                        0x00000C00
#define IOC_IOCFG18_IOCURR_S                                                10
#define IOC_IOCFG18_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG18_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG18_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG18_IOSTR_W                                                  2
#define IOC_IOCFG18_IOSTR_M                                         0x00000300
#define IOC_IOCFG18_IOSTR_S                                                  8
#define IOC_IOCFG18_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG18_IOSTR_MED                                       0x00000200
#define IOC_IOCFG18_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG18_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG18_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG18_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG18_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG18_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG18_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG18_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG18_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG18_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO18
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG18_PORT_ID_W                                                6
#define IOC_IOCFG18_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG18_PORT_ID_S                                                0
#define IOC_IOCFG18_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG18_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG18_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG18_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG18_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG18_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG18_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG18_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG18_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG18_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG18_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG18_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG18_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG18_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG18_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG18_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG18_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG18_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG18_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG18_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG18_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG18_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG18_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG18_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG18_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG18_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG18_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG18_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG18_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG18_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG18_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG18_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG18_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG18_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG18_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG18_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG18_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG18_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG18_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG18_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG18_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG18_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG18_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG18_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG18_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG18_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG19
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG19_HYST_EN                                         0x40000000
#define IOC_IOCFG19_HYST_EN_BITN                                            30
#define IOC_IOCFG19_HYST_EN_M                                       0x40000000
#define IOC_IOCFG19_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG19_IE                                              0x20000000
#define IOC_IOCFG19_IE_BITN                                                 29
#define IOC_IOCFG19_IE_M                                            0x20000000
#define IOC_IOCFG19_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG19_WU_CFG_W                                                 2
#define IOC_IOCFG19_WU_CFG_M                                        0x18000000
#define IOC_IOCFG19_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG19_IOMODE_W                                                 3
#define IOC_IOCFG19_IOMODE_M                                        0x07000000
#define IOC_IOCFG19_IOMODE_S                                                24
#define IOC_IOCFG19_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG19_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG19_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG19_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG19_IOMODE_INV                                      0x01000000
#define IOC_IOCFG19_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG19_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG19_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG19_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG19_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG19_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG19_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG19_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG19_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG19_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG19_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG19_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG19_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG19_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG19_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG19_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG19_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG19_EDGE_DET_W                                               2
#define IOC_IOCFG19_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG19_EDGE_DET_S                                              16
#define IOC_IOCFG19_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG19_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG19_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG19_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG19_PULL_CTL_W                                               2
#define IOC_IOCFG19_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG19_PULL_CTL_S                                              13
#define IOC_IOCFG19_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG19_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG19_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG19_SLEW_RED                                        0x00001000
#define IOC_IOCFG19_SLEW_RED_BITN                                           12
#define IOC_IOCFG19_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG19_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG19_IOCURR_W                                                 2
#define IOC_IOCFG19_IOCURR_M                                        0x00000C00
#define IOC_IOCFG19_IOCURR_S                                                10
#define IOC_IOCFG19_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG19_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG19_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG19_IOSTR_W                                                  2
#define IOC_IOCFG19_IOSTR_M                                         0x00000300
#define IOC_IOCFG19_IOSTR_S                                                  8
#define IOC_IOCFG19_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG19_IOSTR_MED                                       0x00000200
#define IOC_IOCFG19_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG19_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG19_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG19_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG19_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG19_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG19_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG19_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG19_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG19_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO19
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG19_PORT_ID_W                                                6
#define IOC_IOCFG19_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG19_PORT_ID_S                                                0
#define IOC_IOCFG19_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG19_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG19_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG19_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG19_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG19_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG19_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG19_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG19_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG19_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG19_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG19_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG19_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG19_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG19_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG19_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG19_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG19_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG19_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG19_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG19_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG19_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG19_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG19_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG19_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG19_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG19_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG19_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG19_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG19_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG19_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG19_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG19_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG19_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG19_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG19_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG19_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG19_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG19_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG19_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG19_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG19_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG19_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG19_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG19_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG19_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG20
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG20_HYST_EN                                         0x40000000
#define IOC_IOCFG20_HYST_EN_BITN                                            30
#define IOC_IOCFG20_HYST_EN_M                                       0x40000000
#define IOC_IOCFG20_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG20_IE                                              0x20000000
#define IOC_IOCFG20_IE_BITN                                                 29
#define IOC_IOCFG20_IE_M                                            0x20000000
#define IOC_IOCFG20_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG20_WU_CFG_W                                                 2
#define IOC_IOCFG20_WU_CFG_M                                        0x18000000
#define IOC_IOCFG20_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG20_IOMODE_W                                                 3
#define IOC_IOCFG20_IOMODE_M                                        0x07000000
#define IOC_IOCFG20_IOMODE_S                                                24
#define IOC_IOCFG20_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG20_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG20_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG20_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG20_IOMODE_INV                                      0x01000000
#define IOC_IOCFG20_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG20_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG20_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG20_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG20_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG20_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG20_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG20_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG20_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG20_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG20_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG20_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG20_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG20_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG20_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG20_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG20_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG20_EDGE_DET_W                                               2
#define IOC_IOCFG20_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG20_EDGE_DET_S                                              16
#define IOC_IOCFG20_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG20_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG20_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG20_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG20_PULL_CTL_W                                               2
#define IOC_IOCFG20_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG20_PULL_CTL_S                                              13
#define IOC_IOCFG20_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG20_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG20_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG20_SLEW_RED                                        0x00001000
#define IOC_IOCFG20_SLEW_RED_BITN                                           12
#define IOC_IOCFG20_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG20_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG20_IOCURR_W                                                 2
#define IOC_IOCFG20_IOCURR_M                                        0x00000C00
#define IOC_IOCFG20_IOCURR_S                                                10
#define IOC_IOCFG20_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG20_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG20_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG20_IOSTR_W                                                  2
#define IOC_IOCFG20_IOSTR_M                                         0x00000300
#define IOC_IOCFG20_IOSTR_S                                                  8
#define IOC_IOCFG20_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG20_IOSTR_MED                                       0x00000200
#define IOC_IOCFG20_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG20_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG20_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG20_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG20_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG20_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG20_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG20_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG20_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG20_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO20
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG20_PORT_ID_W                                                6
#define IOC_IOCFG20_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG20_PORT_ID_S                                                0
#define IOC_IOCFG20_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG20_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG20_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG20_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG20_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG20_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG20_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG20_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG20_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG20_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG20_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG20_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG20_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG20_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG20_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG20_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG20_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG20_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG20_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG20_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG20_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG20_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG20_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG20_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG20_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG20_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG20_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG20_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG20_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG20_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG20_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG20_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG20_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG20_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG20_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG20_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG20_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG20_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG20_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG20_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG20_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG20_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG20_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG20_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG20_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG20_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG21
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG21_HYST_EN                                         0x40000000
#define IOC_IOCFG21_HYST_EN_BITN                                            30
#define IOC_IOCFG21_HYST_EN_M                                       0x40000000
#define IOC_IOCFG21_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG21_IE                                              0x20000000
#define IOC_IOCFG21_IE_BITN                                                 29
#define IOC_IOCFG21_IE_M                                            0x20000000
#define IOC_IOCFG21_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG21_WU_CFG_W                                                 2
#define IOC_IOCFG21_WU_CFG_M                                        0x18000000
#define IOC_IOCFG21_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG21_IOMODE_W                                                 3
#define IOC_IOCFG21_IOMODE_M                                        0x07000000
#define IOC_IOCFG21_IOMODE_S                                                24
#define IOC_IOCFG21_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG21_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG21_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG21_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG21_IOMODE_INV                                      0x01000000
#define IOC_IOCFG21_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG21_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG21_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG21_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG21_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG21_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG21_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG21_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG21_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG21_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG21_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG21_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG21_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG21_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG21_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG21_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG21_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG21_EDGE_DET_W                                               2
#define IOC_IOCFG21_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG21_EDGE_DET_S                                              16
#define IOC_IOCFG21_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG21_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG21_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG21_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG21_PULL_CTL_W                                               2
#define IOC_IOCFG21_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG21_PULL_CTL_S                                              13
#define IOC_IOCFG21_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG21_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG21_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG21_SLEW_RED                                        0x00001000
#define IOC_IOCFG21_SLEW_RED_BITN                                           12
#define IOC_IOCFG21_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG21_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG21_IOCURR_W                                                 2
#define IOC_IOCFG21_IOCURR_M                                        0x00000C00
#define IOC_IOCFG21_IOCURR_S                                                10
#define IOC_IOCFG21_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG21_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG21_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG21_IOSTR_W                                                  2
#define IOC_IOCFG21_IOSTR_M                                         0x00000300
#define IOC_IOCFG21_IOSTR_S                                                  8
#define IOC_IOCFG21_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG21_IOSTR_MED                                       0x00000200
#define IOC_IOCFG21_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG21_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG21_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG21_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG21_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG21_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG21_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG21_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG21_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG21_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO21
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG21_PORT_ID_W                                                6
#define IOC_IOCFG21_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG21_PORT_ID_S                                                0
#define IOC_IOCFG21_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG21_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG21_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG21_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG21_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG21_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG21_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG21_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG21_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG21_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG21_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG21_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG21_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG21_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG21_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG21_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG21_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG21_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG21_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG21_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG21_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG21_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG21_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG21_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG21_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG21_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG21_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG21_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG21_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG21_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG21_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG21_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG21_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG21_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG21_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG21_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG21_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG21_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG21_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG21_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG21_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG21_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG21_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG21_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG21_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG21_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG22
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG22_HYST_EN                                         0x40000000
#define IOC_IOCFG22_HYST_EN_BITN                                            30
#define IOC_IOCFG22_HYST_EN_M                                       0x40000000
#define IOC_IOCFG22_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG22_IE                                              0x20000000
#define IOC_IOCFG22_IE_BITN                                                 29
#define IOC_IOCFG22_IE_M                                            0x20000000
#define IOC_IOCFG22_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG22_WU_CFG_W                                                 2
#define IOC_IOCFG22_WU_CFG_M                                        0x18000000
#define IOC_IOCFG22_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG22_IOMODE_W                                                 3
#define IOC_IOCFG22_IOMODE_M                                        0x07000000
#define IOC_IOCFG22_IOMODE_S                                                24
#define IOC_IOCFG22_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG22_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG22_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG22_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG22_IOMODE_INV                                      0x01000000
#define IOC_IOCFG22_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG22_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG22_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG22_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG22_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG22_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG22_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG22_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG22_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG22_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG22_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG22_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG22_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG22_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG22_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG22_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG22_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG22_EDGE_DET_W                                               2
#define IOC_IOCFG22_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG22_EDGE_DET_S                                              16
#define IOC_IOCFG22_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG22_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG22_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG22_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG22_PULL_CTL_W                                               2
#define IOC_IOCFG22_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG22_PULL_CTL_S                                              13
#define IOC_IOCFG22_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG22_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG22_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG22_SLEW_RED                                        0x00001000
#define IOC_IOCFG22_SLEW_RED_BITN                                           12
#define IOC_IOCFG22_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG22_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG22_IOCURR_W                                                 2
#define IOC_IOCFG22_IOCURR_M                                        0x00000C00
#define IOC_IOCFG22_IOCURR_S                                                10
#define IOC_IOCFG22_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG22_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG22_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG22_IOSTR_W                                                  2
#define IOC_IOCFG22_IOSTR_M                                         0x00000300
#define IOC_IOCFG22_IOSTR_S                                                  8
#define IOC_IOCFG22_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG22_IOSTR_MED                                       0x00000200
#define IOC_IOCFG22_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG22_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG22_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG22_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG22_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG22_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG22_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG22_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG22_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG22_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO22
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG22_PORT_ID_W                                                6
#define IOC_IOCFG22_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG22_PORT_ID_S                                                0
#define IOC_IOCFG22_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG22_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG22_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG22_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG22_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG22_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG22_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG22_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG22_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG22_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG22_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG22_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG22_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG22_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG22_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG22_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG22_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG22_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG22_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG22_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG22_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG22_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG22_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG22_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG22_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG22_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG22_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG22_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG22_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG22_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG22_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG22_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG22_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG22_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG22_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG22_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG22_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG22_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG22_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG22_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG22_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG22_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG22_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG22_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG22_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG22_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG23
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG23_HYST_EN                                         0x40000000
#define IOC_IOCFG23_HYST_EN_BITN                                            30
#define IOC_IOCFG23_HYST_EN_M                                       0x40000000
#define IOC_IOCFG23_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG23_IE                                              0x20000000
#define IOC_IOCFG23_IE_BITN                                                 29
#define IOC_IOCFG23_IE_M                                            0x20000000
#define IOC_IOCFG23_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG23_WU_CFG_W                                                 2
#define IOC_IOCFG23_WU_CFG_M                                        0x18000000
#define IOC_IOCFG23_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG23_IOMODE_W                                                 3
#define IOC_IOCFG23_IOMODE_M                                        0x07000000
#define IOC_IOCFG23_IOMODE_S                                                24
#define IOC_IOCFG23_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG23_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG23_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG23_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG23_IOMODE_INV                                      0x01000000
#define IOC_IOCFG23_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG23_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG23_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG23_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG23_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG23_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG23_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG23_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG23_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG23_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG23_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG23_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG23_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG23_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG23_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG23_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG23_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG23_EDGE_DET_W                                               2
#define IOC_IOCFG23_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG23_EDGE_DET_S                                              16
#define IOC_IOCFG23_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG23_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG23_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG23_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG23_PULL_CTL_W                                               2
#define IOC_IOCFG23_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG23_PULL_CTL_S                                              13
#define IOC_IOCFG23_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG23_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG23_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG23_SLEW_RED                                        0x00001000
#define IOC_IOCFG23_SLEW_RED_BITN                                           12
#define IOC_IOCFG23_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG23_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG23_IOCURR_W                                                 2
#define IOC_IOCFG23_IOCURR_M                                        0x00000C00
#define IOC_IOCFG23_IOCURR_S                                                10
#define IOC_IOCFG23_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG23_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG23_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG23_IOSTR_W                                                  2
#define IOC_IOCFG23_IOSTR_M                                         0x00000300
#define IOC_IOCFG23_IOSTR_S                                                  8
#define IOC_IOCFG23_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG23_IOSTR_MED                                       0x00000200
#define IOC_IOCFG23_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG23_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG23_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG23_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG23_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG23_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG23_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG23_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG23_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG23_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO23
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG23_PORT_ID_W                                                6
#define IOC_IOCFG23_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG23_PORT_ID_S                                                0
#define IOC_IOCFG23_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG23_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG23_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG23_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG23_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG23_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG23_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG23_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG23_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG23_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG23_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG23_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG23_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG23_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG23_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG23_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG23_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG23_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG23_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG23_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG23_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG23_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG23_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG23_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG23_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG23_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG23_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG23_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG23_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG23_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG23_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG23_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG23_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG23_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG23_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG23_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG23_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG23_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG23_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG23_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG23_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG23_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG23_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG23_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG23_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG23_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG24
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG24_HYST_EN                                         0x40000000
#define IOC_IOCFG24_HYST_EN_BITN                                            30
#define IOC_IOCFG24_HYST_EN_M                                       0x40000000
#define IOC_IOCFG24_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG24_IE                                              0x20000000
#define IOC_IOCFG24_IE_BITN                                                 29
#define IOC_IOCFG24_IE_M                                            0x20000000
#define IOC_IOCFG24_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG24_WU_CFG_W                                                 2
#define IOC_IOCFG24_WU_CFG_M                                        0x18000000
#define IOC_IOCFG24_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG24_IOMODE_W                                                 3
#define IOC_IOCFG24_IOMODE_M                                        0x07000000
#define IOC_IOCFG24_IOMODE_S                                                24
#define IOC_IOCFG24_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG24_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG24_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG24_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG24_IOMODE_INV                                      0x01000000
#define IOC_IOCFG24_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG24_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG24_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG24_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG24_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG24_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG24_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG24_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG24_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG24_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG24_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG24_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG24_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG24_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG24_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG24_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG24_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG24_EDGE_DET_W                                               2
#define IOC_IOCFG24_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG24_EDGE_DET_S                                              16
#define IOC_IOCFG24_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG24_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG24_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG24_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG24_PULL_CTL_W                                               2
#define IOC_IOCFG24_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG24_PULL_CTL_S                                              13
#define IOC_IOCFG24_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG24_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG24_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG24_SLEW_RED                                        0x00001000
#define IOC_IOCFG24_SLEW_RED_BITN                                           12
#define IOC_IOCFG24_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG24_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG24_IOCURR_W                                                 2
#define IOC_IOCFG24_IOCURR_M                                        0x00000C00
#define IOC_IOCFG24_IOCURR_S                                                10
#define IOC_IOCFG24_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG24_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG24_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG24_IOSTR_W                                                  2
#define IOC_IOCFG24_IOSTR_M                                         0x00000300
#define IOC_IOCFG24_IOSTR_S                                                  8
#define IOC_IOCFG24_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG24_IOSTR_MED                                       0x00000200
#define IOC_IOCFG24_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG24_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG24_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG24_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG24_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG24_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG24_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG24_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG24_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG24_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO24
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG24_PORT_ID_W                                                6
#define IOC_IOCFG24_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG24_PORT_ID_S                                                0
#define IOC_IOCFG24_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG24_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG24_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG24_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG24_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG24_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG24_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG24_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG24_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG24_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG24_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG24_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG24_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG24_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG24_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG24_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG24_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG24_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG24_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG24_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG24_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG24_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG24_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG24_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG24_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG24_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG24_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG24_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG24_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG24_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG24_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG24_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG24_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG24_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG24_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG24_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG24_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG24_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG24_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG24_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG24_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG24_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG24_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG24_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG24_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG24_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG25
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG25_HYST_EN                                         0x40000000
#define IOC_IOCFG25_HYST_EN_BITN                                            30
#define IOC_IOCFG25_HYST_EN_M                                       0x40000000
#define IOC_IOCFG25_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG25_IE                                              0x20000000
#define IOC_IOCFG25_IE_BITN                                                 29
#define IOC_IOCFG25_IE_M                                            0x20000000
#define IOC_IOCFG25_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG25_WU_CFG_W                                                 2
#define IOC_IOCFG25_WU_CFG_M                                        0x18000000
#define IOC_IOCFG25_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG25_IOMODE_W                                                 3
#define IOC_IOCFG25_IOMODE_M                                        0x07000000
#define IOC_IOCFG25_IOMODE_S                                                24
#define IOC_IOCFG25_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG25_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG25_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG25_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG25_IOMODE_INV                                      0x01000000
#define IOC_IOCFG25_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG25_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG25_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG25_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG25_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG25_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG25_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG25_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG25_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG25_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG25_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG25_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG25_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG25_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG25_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG25_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG25_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG25_EDGE_DET_W                                               2
#define IOC_IOCFG25_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG25_EDGE_DET_S                                              16
#define IOC_IOCFG25_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG25_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG25_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG25_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG25_PULL_CTL_W                                               2
#define IOC_IOCFG25_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG25_PULL_CTL_S                                              13
#define IOC_IOCFG25_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG25_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG25_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG25_SLEW_RED                                        0x00001000
#define IOC_IOCFG25_SLEW_RED_BITN                                           12
#define IOC_IOCFG25_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG25_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG25_IOCURR_W                                                 2
#define IOC_IOCFG25_IOCURR_M                                        0x00000C00
#define IOC_IOCFG25_IOCURR_S                                                10
#define IOC_IOCFG25_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG25_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG25_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG25_IOSTR_W                                                  2
#define IOC_IOCFG25_IOSTR_M                                         0x00000300
#define IOC_IOCFG25_IOSTR_S                                                  8
#define IOC_IOCFG25_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG25_IOSTR_MED                                       0x00000200
#define IOC_IOCFG25_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG25_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG25_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG25_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG25_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG25_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG25_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG25_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG25_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG25_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO25
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG25_PORT_ID_W                                                6
#define IOC_IOCFG25_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG25_PORT_ID_S                                                0
#define IOC_IOCFG25_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG25_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG25_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG25_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG25_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG25_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG25_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG25_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG25_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG25_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG25_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG25_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG25_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG25_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG25_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG25_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG25_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG25_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG25_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG25_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG25_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG25_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG25_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG25_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG25_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG25_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG25_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG25_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG25_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG25_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG25_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG25_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG25_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG25_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG25_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG25_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG25_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG25_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG25_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG25_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG25_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG25_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG25_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG25_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG25_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG25_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG26
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG26_HYST_EN                                         0x40000000
#define IOC_IOCFG26_HYST_EN_BITN                                            30
#define IOC_IOCFG26_HYST_EN_M                                       0x40000000
#define IOC_IOCFG26_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG26_IE                                              0x20000000
#define IOC_IOCFG26_IE_BITN                                                 29
#define IOC_IOCFG26_IE_M                                            0x20000000
#define IOC_IOCFG26_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG26_WU_CFG_W                                                 2
#define IOC_IOCFG26_WU_CFG_M                                        0x18000000
#define IOC_IOCFG26_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG26_IOMODE_W                                                 3
#define IOC_IOCFG26_IOMODE_M                                        0x07000000
#define IOC_IOCFG26_IOMODE_S                                                24
#define IOC_IOCFG26_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG26_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG26_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG26_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG26_IOMODE_INV                                      0x01000000
#define IOC_IOCFG26_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG26_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG26_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG26_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG26_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG26_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG26_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG26_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG26_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG26_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG26_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG26_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG26_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG26_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG26_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG26_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG26_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG26_EDGE_DET_W                                               2
#define IOC_IOCFG26_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG26_EDGE_DET_S                                              16
#define IOC_IOCFG26_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG26_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG26_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG26_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG26_PULL_CTL_W                                               2
#define IOC_IOCFG26_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG26_PULL_CTL_S                                              13
#define IOC_IOCFG26_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG26_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG26_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG26_SLEW_RED                                        0x00001000
#define IOC_IOCFG26_SLEW_RED_BITN                                           12
#define IOC_IOCFG26_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG26_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG26_IOCURR_W                                                 2
#define IOC_IOCFG26_IOCURR_M                                        0x00000C00
#define IOC_IOCFG26_IOCURR_S                                                10
#define IOC_IOCFG26_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG26_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG26_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG26_IOSTR_W                                                  2
#define IOC_IOCFG26_IOSTR_M                                         0x00000300
#define IOC_IOCFG26_IOSTR_S                                                  8
#define IOC_IOCFG26_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG26_IOSTR_MED                                       0x00000200
#define IOC_IOCFG26_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG26_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG26_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG26_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG26_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG26_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG26_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG26_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG26_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG26_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO26
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG26_PORT_ID_W                                                6
#define IOC_IOCFG26_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG26_PORT_ID_S                                                0
#define IOC_IOCFG26_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG26_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG26_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG26_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG26_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG26_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG26_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG26_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG26_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG26_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG26_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG26_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG26_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG26_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG26_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG26_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG26_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG26_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG26_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG26_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG26_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG26_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG26_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG26_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG26_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG26_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG26_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG26_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG26_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG26_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG26_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG26_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG26_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG26_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG26_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG26_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG26_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG26_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG26_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG26_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG26_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG26_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG26_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG26_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG26_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG26_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG27
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG27_HYST_EN                                         0x40000000
#define IOC_IOCFG27_HYST_EN_BITN                                            30
#define IOC_IOCFG27_HYST_EN_M                                       0x40000000
#define IOC_IOCFG27_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG27_IE                                              0x20000000
#define IOC_IOCFG27_IE_BITN                                                 29
#define IOC_IOCFG27_IE_M                                            0x20000000
#define IOC_IOCFG27_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG27_WU_CFG_W                                                 2
#define IOC_IOCFG27_WU_CFG_M                                        0x18000000
#define IOC_IOCFG27_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG27_IOMODE_W                                                 3
#define IOC_IOCFG27_IOMODE_M                                        0x07000000
#define IOC_IOCFG27_IOMODE_S                                                24
#define IOC_IOCFG27_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG27_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG27_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG27_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG27_IOMODE_INV                                      0x01000000
#define IOC_IOCFG27_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG27_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG27_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG27_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG27_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG27_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG27_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG27_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG27_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG27_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG27_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG27_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG27_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG27_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG27_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG27_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG27_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG27_EDGE_DET_W                                               2
#define IOC_IOCFG27_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG27_EDGE_DET_S                                              16
#define IOC_IOCFG27_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG27_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG27_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG27_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG27_PULL_CTL_W                                               2
#define IOC_IOCFG27_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG27_PULL_CTL_S                                              13
#define IOC_IOCFG27_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG27_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG27_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG27_SLEW_RED                                        0x00001000
#define IOC_IOCFG27_SLEW_RED_BITN                                           12
#define IOC_IOCFG27_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG27_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG27_IOCURR_W                                                 2
#define IOC_IOCFG27_IOCURR_M                                        0x00000C00
#define IOC_IOCFG27_IOCURR_S                                                10
#define IOC_IOCFG27_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG27_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG27_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG27_IOSTR_W                                                  2
#define IOC_IOCFG27_IOSTR_M                                         0x00000300
#define IOC_IOCFG27_IOSTR_S                                                  8
#define IOC_IOCFG27_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG27_IOSTR_MED                                       0x00000200
#define IOC_IOCFG27_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG27_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG27_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG27_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG27_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG27_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG27_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG27_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG27_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG27_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO27
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG27_PORT_ID_W                                                6
#define IOC_IOCFG27_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG27_PORT_ID_S                                                0
#define IOC_IOCFG27_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG27_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG27_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG27_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG27_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG27_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG27_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG27_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG27_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG27_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG27_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG27_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG27_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG27_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG27_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG27_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG27_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG27_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG27_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG27_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG27_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG27_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG27_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG27_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG27_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG27_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG27_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG27_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG27_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG27_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG27_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG27_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG27_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG27_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG27_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG27_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG27_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG27_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG27_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG27_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG27_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG27_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG27_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG27_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG27_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG27_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG28
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG28_HYST_EN                                         0x40000000
#define IOC_IOCFG28_HYST_EN_BITN                                            30
#define IOC_IOCFG28_HYST_EN_M                                       0x40000000
#define IOC_IOCFG28_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG28_IE                                              0x20000000
#define IOC_IOCFG28_IE_BITN                                                 29
#define IOC_IOCFG28_IE_M                                            0x20000000
#define IOC_IOCFG28_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG28_WU_CFG_W                                                 2
#define IOC_IOCFG28_WU_CFG_M                                        0x18000000
#define IOC_IOCFG28_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG28_IOMODE_W                                                 3
#define IOC_IOCFG28_IOMODE_M                                        0x07000000
#define IOC_IOCFG28_IOMODE_S                                                24
#define IOC_IOCFG28_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG28_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG28_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG28_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG28_IOMODE_INV                                      0x01000000
#define IOC_IOCFG28_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG28_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG28_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG28_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG28_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG28_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG28_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG28_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG28_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG28_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG28_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG28_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG28_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG28_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG28_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG28_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG28_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG28_EDGE_DET_W                                               2
#define IOC_IOCFG28_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG28_EDGE_DET_S                                              16
#define IOC_IOCFG28_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG28_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG28_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG28_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG28_PULL_CTL_W                                               2
#define IOC_IOCFG28_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG28_PULL_CTL_S                                              13
#define IOC_IOCFG28_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG28_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG28_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG28_SLEW_RED                                        0x00001000
#define IOC_IOCFG28_SLEW_RED_BITN                                           12
#define IOC_IOCFG28_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG28_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG28_IOCURR_W                                                 2
#define IOC_IOCFG28_IOCURR_M                                        0x00000C00
#define IOC_IOCFG28_IOCURR_S                                                10
#define IOC_IOCFG28_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG28_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG28_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG28_IOSTR_W                                                  2
#define IOC_IOCFG28_IOSTR_M                                         0x00000300
#define IOC_IOCFG28_IOSTR_S                                                  8
#define IOC_IOCFG28_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG28_IOSTR_MED                                       0x00000200
#define IOC_IOCFG28_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG28_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG28_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG28_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG28_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG28_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG28_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG28_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG28_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG28_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO28
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG28_PORT_ID_W                                                6
#define IOC_IOCFG28_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG28_PORT_ID_S                                                0
#define IOC_IOCFG28_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG28_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG28_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG28_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG28_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG28_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG28_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG28_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG28_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG28_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG28_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG28_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG28_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG28_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG28_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG28_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG28_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG28_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG28_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG28_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG28_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG28_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG28_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG28_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG28_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG28_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG28_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG28_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG28_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG28_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG28_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG28_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG28_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG28_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG28_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG28_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG28_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG28_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG28_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG28_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG28_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG28_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG28_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG28_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG28_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG28_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG29
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG29_HYST_EN                                         0x40000000
#define IOC_IOCFG29_HYST_EN_BITN                                            30
#define IOC_IOCFG29_HYST_EN_M                                       0x40000000
#define IOC_IOCFG29_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG29_IE                                              0x20000000
#define IOC_IOCFG29_IE_BITN                                                 29
#define IOC_IOCFG29_IE_M                                            0x20000000
#define IOC_IOCFG29_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG29_WU_CFG_W                                                 2
#define IOC_IOCFG29_WU_CFG_M                                        0x18000000
#define IOC_IOCFG29_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG29_IOMODE_W                                                 3
#define IOC_IOCFG29_IOMODE_M                                        0x07000000
#define IOC_IOCFG29_IOMODE_S                                                24
#define IOC_IOCFG29_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG29_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG29_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG29_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG29_IOMODE_INV                                      0x01000000
#define IOC_IOCFG29_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG29_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG29_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG29_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG29_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG29_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG29_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG29_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG29_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG29_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG29_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG29_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG29_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG29_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG29_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG29_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG29_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG29_EDGE_DET_W                                               2
#define IOC_IOCFG29_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG29_EDGE_DET_S                                              16
#define IOC_IOCFG29_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG29_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG29_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG29_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG29_PULL_CTL_W                                               2
#define IOC_IOCFG29_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG29_PULL_CTL_S                                              13
#define IOC_IOCFG29_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG29_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG29_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG29_SLEW_RED                                        0x00001000
#define IOC_IOCFG29_SLEW_RED_BITN                                           12
#define IOC_IOCFG29_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG29_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG29_IOCURR_W                                                 2
#define IOC_IOCFG29_IOCURR_M                                        0x00000C00
#define IOC_IOCFG29_IOCURR_S                                                10
#define IOC_IOCFG29_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG29_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG29_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG29_IOSTR_W                                                  2
#define IOC_IOCFG29_IOSTR_M                                         0x00000300
#define IOC_IOCFG29_IOSTR_S                                                  8
#define IOC_IOCFG29_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG29_IOSTR_MED                                       0x00000200
#define IOC_IOCFG29_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG29_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG29_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG29_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG29_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG29_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG29_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG29_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG29_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG29_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO29
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG29_PORT_ID_W                                                6
#define IOC_IOCFG29_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG29_PORT_ID_S                                                0
#define IOC_IOCFG29_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG29_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG29_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG29_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG29_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG29_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG29_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG29_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG29_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG29_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG29_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG29_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG29_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG29_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG29_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG29_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG29_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG29_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG29_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG29_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG29_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG29_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG29_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG29_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG29_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG29_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG29_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG29_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG29_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG29_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG29_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG29_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG29_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG29_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG29_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG29_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG29_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG29_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG29_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG29_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG29_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG29_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG29_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG29_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG29_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG29_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG30
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG30_HYST_EN                                         0x40000000
#define IOC_IOCFG30_HYST_EN_BITN                                            30
#define IOC_IOCFG30_HYST_EN_M                                       0x40000000
#define IOC_IOCFG30_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG30_IE                                              0x20000000
#define IOC_IOCFG30_IE_BITN                                                 29
#define IOC_IOCFG30_IE_M                                            0x20000000
#define IOC_IOCFG30_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG30_WU_CFG_W                                                 2
#define IOC_IOCFG30_WU_CFG_M                                        0x18000000
#define IOC_IOCFG30_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG30_IOMODE_W                                                 3
#define IOC_IOCFG30_IOMODE_M                                        0x07000000
#define IOC_IOCFG30_IOMODE_S                                                24
#define IOC_IOCFG30_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG30_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG30_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG30_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG30_IOMODE_INV                                      0x01000000
#define IOC_IOCFG30_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG30_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG30_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG30_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG30_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG30_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG30_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG30_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG30_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG30_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG30_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG30_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG30_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG30_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG30_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG30_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG30_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG30_EDGE_DET_W                                               2
#define IOC_IOCFG30_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG30_EDGE_DET_S                                              16
#define IOC_IOCFG30_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG30_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG30_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG30_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG30_PULL_CTL_W                                               2
#define IOC_IOCFG30_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG30_PULL_CTL_S                                              13
#define IOC_IOCFG30_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG30_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG30_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG30_SLEW_RED                                        0x00001000
#define IOC_IOCFG30_SLEW_RED_BITN                                           12
#define IOC_IOCFG30_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG30_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG30_IOCURR_W                                                 2
#define IOC_IOCFG30_IOCURR_M                                        0x00000C00
#define IOC_IOCFG30_IOCURR_S                                                10
#define IOC_IOCFG30_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG30_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG30_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG30_IOSTR_W                                                  2
#define IOC_IOCFG30_IOSTR_M                                         0x00000300
#define IOC_IOCFG30_IOSTR_S                                                  8
#define IOC_IOCFG30_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG30_IOSTR_MED                                       0x00000200
#define IOC_IOCFG30_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG30_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG30_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG30_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG30_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG30_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG30_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG30_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG30_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG30_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO30
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG30_PORT_ID_W                                                6
#define IOC_IOCFG30_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG30_PORT_ID_S                                                0
#define IOC_IOCFG30_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG30_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG30_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG30_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG30_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG30_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG30_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG30_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG30_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG30_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG30_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG30_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG30_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG30_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG30_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG30_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG30_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG30_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG30_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG30_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG30_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG30_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG30_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG30_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG30_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG30_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG30_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG30_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG30_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG30_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG30_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG30_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG30_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG30_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG30_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG30_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG30_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG30_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG30_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG30_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG30_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG30_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG30_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG30_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG30_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG30_PORT_ID_GPIO                                    0x00000000

//*****************************************************************************
//
// Register: IOC_O_IOCFG31
//
//*****************************************************************************
// Field:    [30] HYST_EN
//
// 0: Input hysteresis disable
// 1: Input hysteresis enable
#define IOC_IOCFG31_HYST_EN                                         0x40000000
#define IOC_IOCFG31_HYST_EN_BITN                                            30
#define IOC_IOCFG31_HYST_EN_M                                       0x40000000
#define IOC_IOCFG31_HYST_EN_S                                               30

// Field:    [29] IE
//
// 0: Input disabled
// 1: Input enabled
//
// Note: If IO is configured for AUX   PORT_ID = 0x08, the enable will be
// ignored.
#define IOC_IOCFG31_IE                                              0x20000000
#define IOC_IOCFG31_IE_BITN                                                 29
#define IOC_IOCFG31_IE_M                                            0x20000000
#define IOC_IOCFG31_IE_S                                                    29

// Field: [28:27] WU_CFG
//
// If DIO is configured GPIO or non-AON peripheral signals,  PORT_ID 0x00 or
// >0x08:
//
// 00: No wake-up
// 01: No wake-up
// 10: Wakes up from shutdown if this pad is going low.
// 11: Wakes up from shutdown if this pad is going high.
//
// If IO is configured for AON peripheral signals or AUX   PORT_ID 0x01-0x08,
// this register only sets wakeup enable or not.
//
// 00, 01: Wakeup disabled
// 10, 11: Wakeup enabled
//
// Polarity is controlled from AON registers.
//
// Note:When the MSB is set, the IOC will deactivate the output enable for the
// DIO.
#define IOC_IOCFG31_WU_CFG_W                                                 2
#define IOC_IOCFG31_WU_CFG_M                                        0x18000000
#define IOC_IOCFG31_WU_CFG_S                                                27

// Field: [26:24] IOMODE
//
// IO Mode
// Not applicable for IO configured for AON periph. signals and AUX   PORT_ID
// 0x01-0x08
// AUX has its own open_source/drain configuration.
//
// 0x2: Reserved. Undefined behavior.
// 0x3: Reserved. Undefined behavior.
// ENUMs:
// OPENSRC_INV              Open Source
//                          Inverted input / output
// OPENSRC                  Open Source
//                          Normal input / output
// OPENDR_INV               Open Drain
//                          Inverted input / output
// OPENDR                   Open Drain,
//                          Normal input / output
// INV                      Inverted input / ouput
// NORMAL                   Normal input / output
#define IOC_IOCFG31_IOMODE_W                                                 3
#define IOC_IOCFG31_IOMODE_M                                        0x07000000
#define IOC_IOCFG31_IOMODE_S                                                24
#define IOC_IOCFG31_IOMODE_OPENSRC_INV                              0x07000000
#define IOC_IOCFG31_IOMODE_OPENSRC                                  0x06000000
#define IOC_IOCFG31_IOMODE_OPENDR_INV                               0x05000000
#define IOC_IOCFG31_IOMODE_OPENDR                                   0x04000000
#define IOC_IOCFG31_IOMODE_INV                                      0x01000000
#define IOC_IOCFG31_IOMODE_NORMAL                                   0x00000000

// Field:    [23] IOEV_AON_PROG2_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG2 event
// 1: Input edge detection asserts AON_PROG2 event
#define IOC_IOCFG31_IOEV_AON_PROG2_EN                               0x00800000
#define IOC_IOCFG31_IOEV_AON_PROG2_EN_BITN                                  23
#define IOC_IOCFG31_IOEV_AON_PROG2_EN_M                             0x00800000
#define IOC_IOCFG31_IOEV_AON_PROG2_EN_S                                     23

// Field:    [22] IOEV_AON_PROG1_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG1 event
// 1: Input edge detection asserts AON_PROG1 event
#define IOC_IOCFG31_IOEV_AON_PROG1_EN                               0x00400000
#define IOC_IOCFG31_IOEV_AON_PROG1_EN_BITN                                  22
#define IOC_IOCFG31_IOEV_AON_PROG1_EN_M                             0x00400000
#define IOC_IOCFG31_IOEV_AON_PROG1_EN_S                                     22

// Field:    [21] IOEV_AON_PROG0_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert AON_PROG0 event
// 1: Input edge detection asserts AON_PROG0 event
#define IOC_IOCFG31_IOEV_AON_PROG0_EN                               0x00200000
#define IOC_IOCFG31_IOEV_AON_PROG0_EN_BITN                                  21
#define IOC_IOCFG31_IOEV_AON_PROG0_EN_M                             0x00200000
#define IOC_IOCFG31_IOEV_AON_PROG0_EN_S                                     21

// Field:    [18] EDGE_IRQ_EN
//
// 0: No interrupt generation
// 1: Enable interrupt generation for this IO (Only effective if EDGE_DET is
// enabled)
#define IOC_IOCFG31_EDGE_IRQ_EN                                     0x00040000
#define IOC_IOCFG31_EDGE_IRQ_EN_BITN                                        18
#define IOC_IOCFG31_EDGE_IRQ_EN_M                                   0x00040000
#define IOC_IOCFG31_EDGE_IRQ_EN_S                                           18

// Field: [17:16] EDGE_DET
//
// Enable generation of edge detection events on this IO
// ENUMs:
// BOTH                     Positive and negative edge detection
// POS                      Positive edge detection
// NEG                      Negative edge detection
// NONE                     No edge detection
#define IOC_IOCFG31_EDGE_DET_W                                               2
#define IOC_IOCFG31_EDGE_DET_M                                      0x00030000
#define IOC_IOCFG31_EDGE_DET_S                                              16
#define IOC_IOCFG31_EDGE_DET_BOTH                                   0x00030000
#define IOC_IOCFG31_EDGE_DET_POS                                    0x00020000
#define IOC_IOCFG31_EDGE_DET_NEG                                    0x00010000
#define IOC_IOCFG31_EDGE_DET_NONE                                   0x00000000

// Field: [14:13] PULL_CTL
//
// Pull control
// ENUMs:
// DIS                      No pull
// UP                       Pull up
// DWN                      Pull down
#define IOC_IOCFG31_PULL_CTL_W                                               2
#define IOC_IOCFG31_PULL_CTL_M                                      0x00006000
#define IOC_IOCFG31_PULL_CTL_S                                              13
#define IOC_IOCFG31_PULL_CTL_DIS                                    0x00006000
#define IOC_IOCFG31_PULL_CTL_UP                                     0x00004000
#define IOC_IOCFG31_PULL_CTL_DWN                                    0x00002000

// Field:    [12] SLEW_RED
//
// 0: Normal slew rate
// 1: Enables reduced slew rate in output driver.
#define IOC_IOCFG31_SLEW_RED                                        0x00001000
#define IOC_IOCFG31_SLEW_RED_BITN                                           12
#define IOC_IOCFG31_SLEW_RED_M                                      0x00001000
#define IOC_IOCFG31_SLEW_RED_S                                              12

// Field: [11:10] IOCURR
//
// Selects IO current mode of this IO.
// ENUMs:
// 4_8MA                    Extended-Current (EC) mode: Min 8 mA for double
//                          drive strength IOs (min 4 mA for normal IOs)
//                          when IOSTR is set to AUTO
// 4MA                      High-Current (HC) mode: Min 4 mA when IOSTR is set
//                          to AUTO
// 2MA                      Low-Current (LC) mode: Min 2 mA when IOSTR is set
//                          to AUTO
#define IOC_IOCFG31_IOCURR_W                                                 2
#define IOC_IOCFG31_IOCURR_M                                        0x00000C00
#define IOC_IOCFG31_IOCURR_S                                                10
#define IOC_IOCFG31_IOCURR_4_8MA                                    0x00000800
#define IOC_IOCFG31_IOCURR_4MA                                      0x00000400
#define IOC_IOCFG31_IOCURR_2MA                                      0x00000000

// Field:   [9:8] IOSTR
//
// Select source for drive strength control of this IO.
// This setting controls the drive strength of the Low-Current (LC) mode.
// Higher drive strength can be selected in IOCURR
// ENUMs:
// MAX                      Maximum drive strength, controlled by
//                          AON_IOC:IOSTRMAX (min 2 mA @1.8V with default
//                          values)
// MED                      Medium drive strength, controlled by
//                          AON_IOC:IOSTRMED (min 2 mA @2.5V with default
//                          values)
// MIN                      Minimum drive strength, controlled by
//                          AON_IOC:IOSTRMIN (min 2 mA @3.3V with default
//                          values)
// AUTO                     Automatic drive strength, controlled by AON BATMON
//                          based on battery voltage. (min 2 mA @VDDS)
#define IOC_IOCFG31_IOSTR_W                                                  2
#define IOC_IOCFG31_IOSTR_M                                         0x00000300
#define IOC_IOCFG31_IOSTR_S                                                  8
#define IOC_IOCFG31_IOSTR_MAX                                       0x00000300
#define IOC_IOCFG31_IOSTR_MED                                       0x00000200
#define IOC_IOCFG31_IOSTR_MIN                                       0x00000100
#define IOC_IOCFG31_IOSTR_AUTO                                      0x00000000

// Field:     [7] IOEV_RTC_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert RTC event
// 1: Input edge detection asserts RTC event
#define IOC_IOCFG31_IOEV_RTC_EN                                     0x00000080
#define IOC_IOCFG31_IOEV_RTC_EN_BITN                                         7
#define IOC_IOCFG31_IOEV_RTC_EN_M                                   0x00000080
#define IOC_IOCFG31_IOEV_RTC_EN_S                                            7

// Field:     [6] IOEV_MCU_WU_EN
//
// Event asserted by this IO when edge detection is enabled
//
// 0: Input edge detection does not assert MCU_WU event
// 1: Input edge detection asserts MCU_WU event
#define IOC_IOCFG31_IOEV_MCU_WU_EN                                  0x00000040
#define IOC_IOCFG31_IOEV_MCU_WU_EN_BITN                                      6
#define IOC_IOCFG31_IOEV_MCU_WU_EN_M                                0x00000040
#define IOC_IOCFG31_IOEV_MCU_WU_EN_S                                         6

// Field:   [5:0] PORT_ID
//
// Selects usage for DIO31
// ENUMs:
// RFC_SMI_CL_IN            RF Core SMI Command Link In
// RFC_SMI_CL_OUT           RF Core SMI Command Link Out
// RFC_SMI_DL_IN            RF Core SMI Data Link In
// RFC_SMI_DL_OUT           RF Core SMI Data Link Out
// RFC_GPI1                 RF Core Data In 1
// RFC_GPI0                 RF Core Data In 0
// RFC_GPO3                 RF Core Data Out 3
// RFC_GPO2                 RF Core Data Out 2
// RFC_GPO1                 RF Core Data Out 1
// RFC_GPO0                 RF Core Data Out 0
// RFC_TRC                  RF Core Trace
// I2S_MCLK                 I2S MCLK
// I2S_BCLK                 I2S BCLK
// I2S_WCLK                 I2S WCLK
// I2S_AD1                  I2S Data 1
// I2S_AD0                  I2S Data 0
// SSI1_CLK                 SSI1 CLK
// SSI1_FSS                 SSI1 FSS
// SSI1_TX                  SSI1 TX
// SSI1_RX                  SSI1 RX
// CPU_SWV                  CPU SWV
// PORT_EVENT7              PORT EVENT 7
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT6              PORT EVENT 6
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT5              PORT EVENT 5
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT4              PORT EVENT 4
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT3              PORT EVENT 3
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT2              PORT EVENT 2
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT1              PORT EVENT 1
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// PORT_EVENT0              PORT EVENT 0
//                          Can be used as a general
//                          purpose IO event by selecting it through
//                          registers in the EVENT module, for example
//                          EVENT:GPT0ACAPTSEL.EV, EVENT:UDMACH14BSEL.EV,
//                          and so on
// UART1_RTS                UART1 RTS
// UART1_CTS                UART1 CTS
// UART1_TX                 UART1 TX
// UART1_RX                 UART1 RX
// UART0_RTS                UART0 RTS
// UART0_CTS                UART0 CTS
// UART0_TX                 UART0 TX
// UART0_RX                 UART0 RX
// I2C_MSSCL                I2C Clock
// I2C_MSSDA                I2C Data
// SSI0_CLK                 SSI0 CLK
// SSI0_FSS                 SSI0 FSS
// SSI0_TX                  SSI0 TX
// SSI0_RX                  SSI0 RX
// AUX_IO                   AUX IO
// AON_CLK32K               AON 32 KHz clock (SCLK_LF)
// GPIO                     General Purpose IO
#define IOC_IOCFG31_PORT_ID_W                                                6
#define IOC_IOCFG31_PORT_ID_M                                       0x0000003F
#define IOC_IOCFG31_PORT_ID_S                                                0
#define IOC_IOCFG31_PORT_ID_RFC_SMI_CL_IN                           0x00000038
#define IOC_IOCFG31_PORT_ID_RFC_SMI_CL_OUT                          0x00000037
#define IOC_IOCFG31_PORT_ID_RFC_SMI_DL_IN                           0x00000036
#define IOC_IOCFG31_PORT_ID_RFC_SMI_DL_OUT                          0x00000035
#define IOC_IOCFG31_PORT_ID_RFC_GPI1                                0x00000034
#define IOC_IOCFG31_PORT_ID_RFC_GPI0                                0x00000033
#define IOC_IOCFG31_PORT_ID_RFC_GPO3                                0x00000032
#define IOC_IOCFG31_PORT_ID_RFC_GPO2                                0x00000031
#define IOC_IOCFG31_PORT_ID_RFC_GPO1                                0x00000030
#define IOC_IOCFG31_PORT_ID_RFC_GPO0                                0x0000002F
#define IOC_IOCFG31_PORT_ID_RFC_TRC                                 0x0000002E
#define IOC_IOCFG31_PORT_ID_I2S_MCLK                                0x00000029
#define IOC_IOCFG31_PORT_ID_I2S_BCLK                                0x00000028
#define IOC_IOCFG31_PORT_ID_I2S_WCLK                                0x00000027
#define IOC_IOCFG31_PORT_ID_I2S_AD1                                 0x00000026
#define IOC_IOCFG31_PORT_ID_I2S_AD0                                 0x00000025
#define IOC_IOCFG31_PORT_ID_SSI1_CLK                                0x00000024
#define IOC_IOCFG31_PORT_ID_SSI1_FSS                                0x00000023
#define IOC_IOCFG31_PORT_ID_SSI1_TX                                 0x00000022
#define IOC_IOCFG31_PORT_ID_SSI1_RX                                 0x00000021
#define IOC_IOCFG31_PORT_ID_CPU_SWV                                 0x00000020
#define IOC_IOCFG31_PORT_ID_PORT_EVENT7                             0x0000001E
#define IOC_IOCFG31_PORT_ID_PORT_EVENT6                             0x0000001D
#define IOC_IOCFG31_PORT_ID_PORT_EVENT5                             0x0000001C
#define IOC_IOCFG31_PORT_ID_PORT_EVENT4                             0x0000001B
#define IOC_IOCFG31_PORT_ID_PORT_EVENT3                             0x0000001A
#define IOC_IOCFG31_PORT_ID_PORT_EVENT2                             0x00000019
#define IOC_IOCFG31_PORT_ID_PORT_EVENT1                             0x00000018
#define IOC_IOCFG31_PORT_ID_PORT_EVENT0                             0x00000017
#define IOC_IOCFG31_PORT_ID_UART1_RTS                               0x00000016
#define IOC_IOCFG31_PORT_ID_UART1_CTS                               0x00000015
#define IOC_IOCFG31_PORT_ID_UART1_TX                                0x00000014
#define IOC_IOCFG31_PORT_ID_UART1_RX                                0x00000013
#define IOC_IOCFG31_PORT_ID_UART0_RTS                               0x00000012
#define IOC_IOCFG31_PORT_ID_UART0_CTS                               0x00000011
#define IOC_IOCFG31_PORT_ID_UART0_TX                                0x00000010
#define IOC_IOCFG31_PORT_ID_UART0_RX                                0x0000000F
#define IOC_IOCFG31_PORT_ID_I2C_MSSCL                               0x0000000E
#define IOC_IOCFG31_PORT_ID_I2C_MSSDA                               0x0000000D
#define IOC_IOCFG31_PORT_ID_SSI0_CLK                                0x0000000C
#define IOC_IOCFG31_PORT_ID_SSI0_FSS                                0x0000000B
#define IOC_IOCFG31_PORT_ID_SSI0_TX                                 0x0000000A
#define IOC_IOCFG31_PORT_ID_SSI0_RX                                 0x00000009
#define IOC_IOCFG31_PORT_ID_AUX_IO                                  0x00000008
#define IOC_IOCFG31_PORT_ID_AON_CLK32K                              0x00000007
#define IOC_IOCFG31_PORT_ID_GPIO                                    0x00000000


#endif // __IOC__
