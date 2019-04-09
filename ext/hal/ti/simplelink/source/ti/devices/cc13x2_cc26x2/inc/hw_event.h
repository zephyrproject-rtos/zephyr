/******************************************************************************
*  Filename:       hw_event_h
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

#ifndef __HW_EVENT_H__
#define __HW_EVENT_H__

//*****************************************************************************
//
// This section defines the register offsets of
// EVENT component
//
//*****************************************************************************
// Output Selection for CPU Interrupt 0
#define EVENT_O_CPUIRQSEL0                                          0x00000000

// Output Selection for CPU Interrupt 1
#define EVENT_O_CPUIRQSEL1                                          0x00000004

// Output Selection for CPU Interrupt 2
#define EVENT_O_CPUIRQSEL2                                          0x00000008

// Output Selection for CPU Interrupt 3
#define EVENT_O_CPUIRQSEL3                                          0x0000000C

// Output Selection for CPU Interrupt 4
#define EVENT_O_CPUIRQSEL4                                          0x00000010

// Output Selection for CPU Interrupt 5
#define EVENT_O_CPUIRQSEL5                                          0x00000014

// Output Selection for CPU Interrupt 6
#define EVENT_O_CPUIRQSEL6                                          0x00000018

// Output Selection for CPU Interrupt 7
#define EVENT_O_CPUIRQSEL7                                          0x0000001C

// Output Selection for CPU Interrupt 8
#define EVENT_O_CPUIRQSEL8                                          0x00000020

// Output Selection for CPU Interrupt 9
#define EVENT_O_CPUIRQSEL9                                          0x00000024

// Output Selection for CPU Interrupt 10
#define EVENT_O_CPUIRQSEL10                                         0x00000028

// Output Selection for CPU Interrupt 11
#define EVENT_O_CPUIRQSEL11                                         0x0000002C

// Output Selection for CPU Interrupt 12
#define EVENT_O_CPUIRQSEL12                                         0x00000030

// Output Selection for CPU Interrupt 13
#define EVENT_O_CPUIRQSEL13                                         0x00000034

// Output Selection for CPU Interrupt 14
#define EVENT_O_CPUIRQSEL14                                         0x00000038

// Output Selection for CPU Interrupt 15
#define EVENT_O_CPUIRQSEL15                                         0x0000003C

// Output Selection for CPU Interrupt 16
#define EVENT_O_CPUIRQSEL16                                         0x00000040

// Output Selection for CPU Interrupt 17
#define EVENT_O_CPUIRQSEL17                                         0x00000044

// Output Selection for CPU Interrupt 18
#define EVENT_O_CPUIRQSEL18                                         0x00000048

// Output Selection for CPU Interrupt 19
#define EVENT_O_CPUIRQSEL19                                         0x0000004C

// Output Selection for CPU Interrupt 20
#define EVENT_O_CPUIRQSEL20                                         0x00000050

// Output Selection for CPU Interrupt 21
#define EVENT_O_CPUIRQSEL21                                         0x00000054

// Output Selection for CPU Interrupt 22
#define EVENT_O_CPUIRQSEL22                                         0x00000058

// Output Selection for CPU Interrupt 23
#define EVENT_O_CPUIRQSEL23                                         0x0000005C

// Output Selection for CPU Interrupt 24
#define EVENT_O_CPUIRQSEL24                                         0x00000060

// Output Selection for CPU Interrupt 25
#define EVENT_O_CPUIRQSEL25                                         0x00000064

// Output Selection for CPU Interrupt 26
#define EVENT_O_CPUIRQSEL26                                         0x00000068

// Output Selection for CPU Interrupt 27
#define EVENT_O_CPUIRQSEL27                                         0x0000006C

// Output Selection for CPU Interrupt 28
#define EVENT_O_CPUIRQSEL28                                         0x00000070

// Output Selection for CPU Interrupt 29
#define EVENT_O_CPUIRQSEL29                                         0x00000074

// Output Selection for CPU Interrupt 30
#define EVENT_O_CPUIRQSEL30                                         0x00000078

// Output Selection for CPU Interrupt 31
#define EVENT_O_CPUIRQSEL31                                         0x0000007C

// Output Selection for CPU Interrupt 32
#define EVENT_O_CPUIRQSEL32                                         0x00000080

// Output Selection for CPU Interrupt 33
#define EVENT_O_CPUIRQSEL33                                         0x00000084

// Output Selection for CPU Interrupt 34
#define EVENT_O_CPUIRQSEL34                                         0x00000088

// Output Selection for CPU Interrupt 35
#define EVENT_O_CPUIRQSEL35                                         0x0000008C

// Output Selection for CPU Interrupt 36
#define EVENT_O_CPUIRQSEL36                                         0x00000090

// Output Selection for CPU Interrupt 37
#define EVENT_O_CPUIRQSEL37                                         0x00000094

// Output Selection for RFC Event 0
#define EVENT_O_RFCSEL0                                             0x00000100

// Output Selection for RFC Event 1
#define EVENT_O_RFCSEL1                                             0x00000104

// Output Selection for RFC Event 2
#define EVENT_O_RFCSEL2                                             0x00000108

// Output Selection for RFC Event 3
#define EVENT_O_RFCSEL3                                             0x0000010C

// Output Selection for RFC Event 4
#define EVENT_O_RFCSEL4                                             0x00000110

// Output Selection for RFC Event 5
#define EVENT_O_RFCSEL5                                             0x00000114

// Output Selection for RFC Event 6
#define EVENT_O_RFCSEL6                                             0x00000118

// Output Selection for RFC Event 7
#define EVENT_O_RFCSEL7                                             0x0000011C

// Output Selection for RFC Event 8
#define EVENT_O_RFCSEL8                                             0x00000120

// Output Selection for RFC Event 9
#define EVENT_O_RFCSEL9                                             0x00000124

// Output Selection for GPT0 0
#define EVENT_O_GPT0ACAPTSEL                                        0x00000200

// Output Selection for GPT0 1
#define EVENT_O_GPT0BCAPTSEL                                        0x00000204

// Output Selection for GPT1 0
#define EVENT_O_GPT1ACAPTSEL                                        0x00000300

// Output Selection for GPT1 1
#define EVENT_O_GPT1BCAPTSEL                                        0x00000304

// Output Selection for GPT2 0
#define EVENT_O_GPT2ACAPTSEL                                        0x00000400

// Output Selection for GPT2 1
#define EVENT_O_GPT2BCAPTSEL                                        0x00000404

// Output Selection for DMA Channel 1 SREQ
#define EVENT_O_UDMACH1SSEL                                         0x00000508

// Output Selection for DMA Channel 1 REQ
#define EVENT_O_UDMACH1BSEL                                         0x0000050C

// Output Selection for DMA Channel 2 SREQ
#define EVENT_O_UDMACH2SSEL                                         0x00000510

// Output Selection for DMA Channel 2 REQ
#define EVENT_O_UDMACH2BSEL                                         0x00000514

// Output Selection for DMA Channel 3 SREQ
#define EVENT_O_UDMACH3SSEL                                         0x00000518

// Output Selection for DMA Channel 3 REQ
#define EVENT_O_UDMACH3BSEL                                         0x0000051C

// Output Selection for DMA Channel 4 SREQ
#define EVENT_O_UDMACH4SSEL                                         0x00000520

// Output Selection for DMA Channel 4 REQ
#define EVENT_O_UDMACH4BSEL                                         0x00000524

// Output Selection for DMA Channel 5 SREQ
#define EVENT_O_UDMACH5SSEL                                         0x00000528

// Output Selection for DMA Channel 5 REQ
#define EVENT_O_UDMACH5BSEL                                         0x0000052C

// Output Selection for DMA Channel 6 SREQ
#define EVENT_O_UDMACH6SSEL                                         0x00000530

// Output Selection for DMA Channel 6 REQ
#define EVENT_O_UDMACH6BSEL                                         0x00000534

// Output Selection for DMA Channel 7 SREQ
#define EVENT_O_UDMACH7SSEL                                         0x00000538

// Output Selection for DMA Channel 7 REQ
#define EVENT_O_UDMACH7BSEL                                         0x0000053C

// Output Selection for DMA Channel 8 SREQ
#define EVENT_O_UDMACH8SSEL                                         0x00000540

// Output Selection for DMA Channel 8 REQ
#define EVENT_O_UDMACH8BSEL                                         0x00000544

// Output Selection for DMA Channel 9 SREQ
#define EVENT_O_UDMACH9SSEL                                         0x00000548

// Output Selection for DMA Channel 9 REQ
#define EVENT_O_UDMACH9BSEL                                         0x0000054C

// Output Selection for DMA Channel 10 SREQ
#define EVENT_O_UDMACH10SSEL                                        0x00000550

// Output Selection for DMA Channel 10 REQ
#define EVENT_O_UDMACH10BSEL                                        0x00000554

// Output Selection for DMA Channel 11 SREQ
#define EVENT_O_UDMACH11SSEL                                        0x00000558

// Output Selection for DMA Channel 11 REQ
#define EVENT_O_UDMACH11BSEL                                        0x0000055C

// Output Selection for DMA Channel 12 SREQ
#define EVENT_O_UDMACH12SSEL                                        0x00000560

// Output Selection for DMA Channel 12 REQ
#define EVENT_O_UDMACH12BSEL                                        0x00000564

// Output Selection for DMA Channel 13 REQ
#define EVENT_O_UDMACH13BSEL                                        0x0000056C

// Output Selection for DMA Channel 14 REQ
#define EVENT_O_UDMACH14BSEL                                        0x00000574

// Output Selection for DMA Channel 15 REQ
#define EVENT_O_UDMACH15BSEL                                        0x0000057C

// Output Selection for DMA Channel 16 SREQ
#define EVENT_O_UDMACH16SSEL                                        0x00000580

// Output Selection for DMA Channel 16 REQ
#define EVENT_O_UDMACH16BSEL                                        0x00000584

// Output Selection for DMA Channel 17 SREQ
#define EVENT_O_UDMACH17SSEL                                        0x00000588

// Output Selection for DMA Channel 17 REQ
#define EVENT_O_UDMACH17BSEL                                        0x0000058C

// Output Selection for DMA Channel 21 SREQ
#define EVENT_O_UDMACH21SSEL                                        0x000005A8

// Output Selection for DMA Channel 21 REQ
#define EVENT_O_UDMACH21BSEL                                        0x000005AC

// Output Selection for DMA Channel 22 SREQ
#define EVENT_O_UDMACH22SSEL                                        0x000005B0

// Output Selection for DMA Channel 22 REQ
#define EVENT_O_UDMACH22BSEL                                        0x000005B4

// Output Selection for DMA Channel 23 SREQ
#define EVENT_O_UDMACH23SSEL                                        0x000005B8

// Output Selection for DMA Channel 23 REQ
#define EVENT_O_UDMACH23BSEL                                        0x000005BC

// Output Selection for DMA Channel 24 SREQ
#define EVENT_O_UDMACH24SSEL                                        0x000005C0

// Output Selection for DMA Channel 24 REQ
#define EVENT_O_UDMACH24BSEL                                        0x000005C4

// Output Selection for GPT3 0
#define EVENT_O_GPT3ACAPTSEL                                        0x00000600

// Output Selection for GPT3 1
#define EVENT_O_GPT3BCAPTSEL                                        0x00000604

// Output Selection for AUX Subscriber 0
#define EVENT_O_AUXSEL0                                             0x00000700

// Output Selection for NMI Subscriber 0
#define EVENT_O_CM3NMISEL0                                          0x00000800

// Output Selection for I2S Subscriber 0
#define EVENT_O_I2SSTMPSEL0                                         0x00000900

// Output Selection for FRZ Subscriber
#define EVENT_O_FRZSEL0                                             0x00000A00

// Set or Clear Software Events
#define EVENT_O_SWEV                                                0x00000F00

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL0
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
#define EVENT_CPUIRQSEL0_EV_W                                                7
#define EVENT_CPUIRQSEL0_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL0_EV_S                                                0
#define EVENT_CPUIRQSEL0_EV_AON_GPIO_EDGE                           0x00000004

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL1
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// I2C_IRQ                  Interrupt event from I2C
#define EVENT_CPUIRQSEL1_EV_W                                                7
#define EVENT_CPUIRQSEL1_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL1_EV_S                                                0
#define EVENT_CPUIRQSEL1_EV_I2C_IRQ                                 0x00000009

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL2
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
#define EVENT_CPUIRQSEL2_EV_W                                                7
#define EVENT_CPUIRQSEL2_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL2_EV_S                                                0
#define EVENT_CPUIRQSEL2_EV_RFC_CPE_1                               0x0000001E

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL3
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// PKA_IRQ                  PKA Interrupt event
#define EVENT_CPUIRQSEL3_EV_W                                                7
#define EVENT_CPUIRQSEL3_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL3_EV_S                                                0
#define EVENT_CPUIRQSEL3_EV_PKA_IRQ                                 0x0000001F

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL4
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
#define EVENT_CPUIRQSEL4_EV_W                                                7
#define EVENT_CPUIRQSEL4_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL4_EV_S                                                0
#define EVENT_CPUIRQSEL4_EV_AON_RTC_COMB                            0x00000007

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL5
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
#define EVENT_CPUIRQSEL5_EV_W                                                7
#define EVENT_CPUIRQSEL5_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL5_EV_S                                                0
#define EVENT_CPUIRQSEL5_EV_UART0_COMB                              0x00000024

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL6
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_SWEV0                AUX software event 0, triggered by
//                          AUX_EVCTL:SWEVSET.SWEV0, also available as
//                          AUX_EVENT0 AON wake up event.
//                          MCU domain wakeup control
//                          AON_EVENT:MCUWUSEL
#define EVENT_CPUIRQSEL6_EV_W                                                7
#define EVENT_CPUIRQSEL6_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL6_EV_S                                                0
#define EVENT_CPUIRQSEL6_EV_AUX_SWEV0                               0x0000001C

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL7
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
#define EVENT_CPUIRQSEL7_EV_W                                                7
#define EVENT_CPUIRQSEL7_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL7_EV_S                                                0
#define EVENT_CPUIRQSEL7_EV_SSI0_COMB                               0x00000022

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL8
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
#define EVENT_CPUIRQSEL8_EV_W                                                7
#define EVENT_CPUIRQSEL8_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL8_EV_S                                                0
#define EVENT_CPUIRQSEL8_EV_SSI1_COMB                               0x00000023

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL9
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
#define EVENT_CPUIRQSEL9_EV_W                                                7
#define EVENT_CPUIRQSEL9_EV_M                                       0x0000007F
#define EVENT_CPUIRQSEL9_EV_S                                                0
#define EVENT_CPUIRQSEL9_EV_RFC_CPE_0                               0x0000001B

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL10
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
#define EVENT_CPUIRQSEL10_EV_W                                               7
#define EVENT_CPUIRQSEL10_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL10_EV_S                                               0
#define EVENT_CPUIRQSEL10_EV_RFC_HW_COMB                            0x0000001A

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL11
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
#define EVENT_CPUIRQSEL11_EV_W                                               7
#define EVENT_CPUIRQSEL11_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL11_EV_S                                               0
#define EVENT_CPUIRQSEL11_EV_RFC_CMD_ACK                            0x00000019

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL12
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// I2S_IRQ                  Interrupt event from I2S
#define EVENT_CPUIRQSEL12_EV_W                                               7
#define EVENT_CPUIRQSEL12_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL12_EV_S                                               0
#define EVENT_CPUIRQSEL12_EV_I2S_IRQ                                0x00000008

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL13
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_SWEV1                AUX software event 1, triggered by
//                          AUX_EVCTL:SWEVSET.SWEV1, also available as
//                          AUX_EVENT2 AON wake up event.
//                          MCU domain wakeup control
//                          AON_EVENT:MCUWUSEL
#define EVENT_CPUIRQSEL13_EV_W                                               7
#define EVENT_CPUIRQSEL13_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL13_EV_S                                               0
#define EVENT_CPUIRQSEL13_EV_AUX_SWEV1                              0x0000001D

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL14
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// WDT_IRQ                  Watchdog interrupt event, controlled by
//                          WDT:CTL.INTEN
#define EVENT_CPUIRQSEL14_EV_W                                               7
#define EVENT_CPUIRQSEL14_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL14_EV_S                                               0
#define EVENT_CPUIRQSEL14_EV_WDT_IRQ                                0x00000018

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL15
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT0A                    GPT0A interrupt event, controlled by GPT0:TAMR
#define EVENT_CPUIRQSEL15_EV_W                                               7
#define EVENT_CPUIRQSEL15_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL15_EV_S                                               0
#define EVENT_CPUIRQSEL15_EV_GPT0A                                  0x00000010

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL16
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT0B                    GPT0B interrupt event, controlled by GPT0:TBMR
#define EVENT_CPUIRQSEL16_EV_W                                               7
#define EVENT_CPUIRQSEL16_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL16_EV_S                                               0
#define EVENT_CPUIRQSEL16_EV_GPT0B                                  0x00000011

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL17
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT1A                    GPT1A interrupt event, controlled by GPT1:TAMR
#define EVENT_CPUIRQSEL17_EV_W                                               7
#define EVENT_CPUIRQSEL17_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL17_EV_S                                               0
#define EVENT_CPUIRQSEL17_EV_GPT1A                                  0x00000012

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL18
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT1B                    GPT1B interrupt event, controlled by GPT1:TBMR
#define EVENT_CPUIRQSEL18_EV_W                                               7
#define EVENT_CPUIRQSEL18_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL18_EV_S                                               0
#define EVENT_CPUIRQSEL18_EV_GPT1B                                  0x00000013

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL19
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT2A                    GPT2A interrupt event, controlled by GPT2:TAMR
#define EVENT_CPUIRQSEL19_EV_W                                               7
#define EVENT_CPUIRQSEL19_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL19_EV_S                                               0
#define EVENT_CPUIRQSEL19_EV_GPT2A                                  0x0000000C

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL20
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT2B                    GPT2B interrupt event, controlled by GPT2:TBMR
#define EVENT_CPUIRQSEL20_EV_W                                               7
#define EVENT_CPUIRQSEL20_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL20_EV_S                                               0
#define EVENT_CPUIRQSEL20_EV_GPT2B                                  0x0000000D

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL21
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT3A                    GPT3A interrupt event, controlled by GPT3:TAMR
#define EVENT_CPUIRQSEL21_EV_W                                               7
#define EVENT_CPUIRQSEL21_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL21_EV_S                                               0
#define EVENT_CPUIRQSEL21_EV_GPT3A                                  0x0000000E

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL22
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT3B                    GPT3B interrupt event, controlled by GPT3:TBMR
#define EVENT_CPUIRQSEL22_EV_W                                               7
#define EVENT_CPUIRQSEL22_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL22_EV_S                                               0
#define EVENT_CPUIRQSEL22_EV_GPT3B                                  0x0000000F

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL23
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// CRYPTO_RESULT_AVAIL_IRQ  CRYPTO result available interupt event, the
//                          corresponding flag is found here
//                          CRYPTO:IRQSTAT.RESULT_AVAIL. Controlled by
//                          CRYPTO:IRQSTAT.RESULT_AVAIL
#define EVENT_CPUIRQSEL23_EV_W                                               7
#define EVENT_CPUIRQSEL23_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL23_EV_S                                               0
#define EVENT_CPUIRQSEL23_EV_CRYPTO_RESULT_AVAIL_IRQ                0x0000005D

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL24
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// DMA_DONE_COMB            Combined DMA done, corresponding flags are here
//                          UDMA0:REQDONE
#define EVENT_CPUIRQSEL24_EV_W                                               7
#define EVENT_CPUIRQSEL24_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL24_EV_S                                               0
#define EVENT_CPUIRQSEL24_EV_DMA_DONE_COMB                          0x00000027

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL25
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// DMA_ERR                  DMA bus error, corresponds to UDMA0:ERROR.STATUS
#define EVENT_CPUIRQSEL25_EV_W                                               7
#define EVENT_CPUIRQSEL25_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL25_EV_S                                               0
#define EVENT_CPUIRQSEL25_EV_DMA_ERR                                0x00000026

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL26
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
#define EVENT_CPUIRQSEL26_EV_W                                               7
#define EVENT_CPUIRQSEL26_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL26_EV_S                                               0
#define EVENT_CPUIRQSEL26_EV_FLASH                                  0x00000015

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL27
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV0                    Software event 0, triggered by SWEV.SWEV0
#define EVENT_CPUIRQSEL27_EV_W                                               7
#define EVENT_CPUIRQSEL27_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL27_EV_S                                               0
#define EVENT_CPUIRQSEL27_EV_SWEV0                                  0x00000064

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL28
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
#define EVENT_CPUIRQSEL28_EV_W                                               7
#define EVENT_CPUIRQSEL28_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL28_EV_S                                               0
#define EVENT_CPUIRQSEL28_EV_AUX_COMB                               0x0000000B

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL29
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AON_PROG0                AON programmable event 0. Event selected by
//                          AON_EVENT  MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG0_EV
#define EVENT_CPUIRQSEL29_EV_W                                               7
#define EVENT_CPUIRQSEL29_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL29_EV_S                                               0
#define EVENT_CPUIRQSEL29_EV_AON_PROG0                              0x00000001

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL30
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// CRYPTO_DMA_DONE_IRQ      CRYPTO DMA input done event, the correspondingg
//                          flag is CRYPTO:IRQSTAT.DMA_IN_DONE. Controlled
//                          by CRYPTO:IRQEN.DMA_IN_DONE
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// DMA_CH18_DONE            DMA done for software tiggered UDMA channel 18,
//                          see UDMA0:SOFTREQ
// DMA_CH0_DONE             DMA done for software tiggered UDMA channel 0, see
//                          UDMA0:SOFTREQ
// AON_AUX_SWEV0            AUX Software event 0, AUX_EVCTL:SWEVSET.SWEV0
// I2S_IRQ                  Interrupt event from I2S
// AON_PROG2                AON programmable event 2. Event selected by
//                          AON_EVENT MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG2_EV
// AON_PROG1                AON programmable event 1. Event selected by
//                          AON_EVENT MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG1_EV
// NONE                     Always inactive
#define EVENT_CPUIRQSEL30_EV_W                                               7
#define EVENT_CPUIRQSEL30_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL30_EV_S                                               0
#define EVENT_CPUIRQSEL30_EV_ALWAYS_ACTIVE                          0x00000079
#define EVENT_CPUIRQSEL30_EV_AON_RTC_UPD                            0x00000077
#define EVENT_CPUIRQSEL30_EV_AUX_OBSMUX0                            0x00000072
#define EVENT_CPUIRQSEL30_EV_AUX_ADC_FIFO_ALMOST_FULL               0x00000071
#define EVENT_CPUIRQSEL30_EV_AUX_ADC_DONE                           0x00000070
#define EVENT_CPUIRQSEL30_EV_AUX_SMPH_AUTOTAKE_DONE                 0x0000006F
#define EVENT_CPUIRQSEL30_EV_AUX_TIMER1_EV                          0x0000006E
#define EVENT_CPUIRQSEL30_EV_AUX_TIMER0_EV                          0x0000006D
#define EVENT_CPUIRQSEL30_EV_AUX_TDC_DONE                           0x0000006C
#define EVENT_CPUIRQSEL30_EV_AUX_COMPB                              0x0000006B
#define EVENT_CPUIRQSEL30_EV_AUX_AON_WU_EV                          0x00000069
#define EVENT_CPUIRQSEL30_EV_CRYPTO_DMA_DONE_IRQ                    0x0000005E
#define EVENT_CPUIRQSEL30_EV_AUX_TIMER2_PULSE                       0x0000003C
#define EVENT_CPUIRQSEL30_EV_AUX_TIMER2_EV3                         0x0000003B
#define EVENT_CPUIRQSEL30_EV_AUX_TIMER2_EV2                         0x0000003A
#define EVENT_CPUIRQSEL30_EV_AUX_TIMER2_EV1                         0x00000039
#define EVENT_CPUIRQSEL30_EV_AUX_TIMER2_EV0                         0x00000038
#define EVENT_CPUIRQSEL30_EV_DMA_CH18_DONE                          0x00000016
#define EVENT_CPUIRQSEL30_EV_DMA_CH0_DONE                           0x00000014
#define EVENT_CPUIRQSEL30_EV_AON_AUX_SWEV0                          0x0000000A
#define EVENT_CPUIRQSEL30_EV_I2S_IRQ                                0x00000008
#define EVENT_CPUIRQSEL30_EV_AON_PROG2                              0x00000003
#define EVENT_CPUIRQSEL30_EV_AON_PROG1                              0x00000002
#define EVENT_CPUIRQSEL30_EV_NONE                                   0x00000000

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL31
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
#define EVENT_CPUIRQSEL31_EV_W                                               7
#define EVENT_CPUIRQSEL31_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL31_EV_S                                               0
#define EVENT_CPUIRQSEL31_EV_AUX_COMPA                              0x0000006A

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL32
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
#define EVENT_CPUIRQSEL32_EV_W                                               7
#define EVENT_CPUIRQSEL32_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL32_EV_S                                               0
#define EVENT_CPUIRQSEL32_EV_AUX_ADC_IRQ                            0x00000073

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL33
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// TRNG_IRQ                 TRNG Interrupt event, controlled by TRNG:IRQEN.EN
#define EVENT_CPUIRQSEL33_EV_W                                               7
#define EVENT_CPUIRQSEL33_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL33_EV_S                                               0
#define EVENT_CPUIRQSEL33_EV_TRNG_IRQ                               0x00000068

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL34
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// OSC_COMB                 Combined event from Oscillator control
#define EVENT_CPUIRQSEL34_EV_W                                               7
#define EVENT_CPUIRQSEL34_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL34_EV_S                                               0
#define EVENT_CPUIRQSEL34_EV_OSC_COMB                               0x00000006

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL35
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
#define EVENT_CPUIRQSEL35_EV_W                                               7
#define EVENT_CPUIRQSEL35_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL35_EV_S                                               0
#define EVENT_CPUIRQSEL35_EV_AUX_TIMER2_EV0                         0x00000038

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL36
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
#define EVENT_CPUIRQSEL36_EV_W                                               7
#define EVENT_CPUIRQSEL36_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL36_EV_S                                               0
#define EVENT_CPUIRQSEL36_EV_UART1_COMB                             0x00000025

//*****************************************************************************
//
// Register: EVENT_O_CPUIRQSEL37
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// BATMON_COMB              Combined event from battery monitor
#define EVENT_CPUIRQSEL37_EV_W                                               7
#define EVENT_CPUIRQSEL37_EV_M                                      0x0000007F
#define EVENT_CPUIRQSEL37_EV_S                                               0
#define EVENT_CPUIRQSEL37_EV_BATMON_COMB                            0x00000005

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL0
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
#define EVENT_RFCSEL0_EV_W                                                   7
#define EVENT_RFCSEL0_EV_M                                          0x0000007F
#define EVENT_RFCSEL0_EV_S                                                   0
#define EVENT_RFCSEL0_EV_GPT0A_CMP                                  0x0000003D

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL1
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
#define EVENT_RFCSEL1_EV_W                                                   7
#define EVENT_RFCSEL1_EV_M                                          0x0000007F
#define EVENT_RFCSEL1_EV_S                                                   0
#define EVENT_RFCSEL1_EV_GPT0B_CMP                                  0x0000003E

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL2
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
#define EVENT_RFCSEL2_EV_W                                                   7
#define EVENT_RFCSEL2_EV_M                                          0x0000007F
#define EVENT_RFCSEL2_EV_S                                                   0
#define EVENT_RFCSEL2_EV_GPT1A_CMP                                  0x0000003F

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL3
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
#define EVENT_RFCSEL3_EV_W                                                   7
#define EVENT_RFCSEL3_EV_M                                          0x0000007F
#define EVENT_RFCSEL3_EV_S                                                   0
#define EVENT_RFCSEL3_EV_GPT1B_CMP                                  0x00000040

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL4
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
#define EVENT_RFCSEL4_EV_W                                                   7
#define EVENT_RFCSEL4_EV_M                                          0x0000007F
#define EVENT_RFCSEL4_EV_S                                                   0
#define EVENT_RFCSEL4_EV_GPT2A_CMP                                  0x00000041

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL5
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
#define EVENT_RFCSEL5_EV_W                                                   7
#define EVENT_RFCSEL5_EV_M                                          0x0000007F
#define EVENT_RFCSEL5_EV_S                                                   0
#define EVENT_RFCSEL5_EV_GPT2B_CMP                                  0x00000042

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL6
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
#define EVENT_RFCSEL6_EV_W                                                   7
#define EVENT_RFCSEL6_EV_M                                          0x0000007F
#define EVENT_RFCSEL6_EV_S                                                   0
#define EVENT_RFCSEL6_EV_GPT3A_CMP                                  0x00000043

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL7
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
#define EVENT_RFCSEL7_EV_W                                                   7
#define EVENT_RFCSEL7_EV_M                                          0x0000007F
#define EVENT_RFCSEL7_EV_S                                                   0
#define EVENT_RFCSEL7_EV_GPT3B_CMP                                  0x00000044

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL8
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
#define EVENT_RFCSEL8_EV_W                                                   7
#define EVENT_RFCSEL8_EV_M                                          0x0000007F
#define EVENT_RFCSEL8_EV_S                                                   0
#define EVENT_RFCSEL8_EV_AON_RTC_UPD                                0x00000077

//*****************************************************************************
//
// Register: EVENT_O_RFCSEL9
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// SWEV1                    Software event 1, triggered by SWEV.SWEV1
// SWEV0                    Software event 0, triggered by SWEV.SWEV0
// CRYPTO_RESULT_AVAIL_IRQ  CRYPTO result available interupt event, the
//                          corresponding flag is found here
//                          CRYPTO:IRQSTAT.RESULT_AVAIL. Controlled by
//                          CRYPTO:IRQSTAT.RESULT_AVAIL
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// DMA_DONE_COMB            Combined DMA done, corresponding flags are here
//                          UDMA0:REQDONE
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// WDT_IRQ                  Watchdog interrupt event, controlled by
//                          WDT:CTL.INTEN
// AON_AUX_SWEV0            AUX Software event 0, AUX_EVCTL:SWEVSET.SWEV0
// I2S_IRQ                  Interrupt event from I2S
// AON_PROG1                AON programmable event 1. Event selected by
//                          AON_EVENT MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG1_EV
// AON_PROG0                AON programmable event 0. Event selected by
//                          AON_EVENT  MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG0_EV
// NONE                     Always inactive
#define EVENT_RFCSEL9_EV_W                                                   7
#define EVENT_RFCSEL9_EV_M                                          0x0000007F
#define EVENT_RFCSEL9_EV_S                                                   0
#define EVENT_RFCSEL9_EV_ALWAYS_ACTIVE                              0x00000079
#define EVENT_RFCSEL9_EV_AUX_ADC_IRQ                                0x00000073
#define EVENT_RFCSEL9_EV_AUX_OBSMUX0                                0x00000072
#define EVENT_RFCSEL9_EV_AUX_ADC_FIFO_ALMOST_FULL                   0x00000071
#define EVENT_RFCSEL9_EV_AUX_ADC_DONE                               0x00000070
#define EVENT_RFCSEL9_EV_AUX_SMPH_AUTOTAKE_DONE                     0x0000006F
#define EVENT_RFCSEL9_EV_AUX_TIMER1_EV                              0x0000006E
#define EVENT_RFCSEL9_EV_AUX_TIMER0_EV                              0x0000006D
#define EVENT_RFCSEL9_EV_AUX_TDC_DONE                               0x0000006C
#define EVENT_RFCSEL9_EV_AUX_COMPB                                  0x0000006B
#define EVENT_RFCSEL9_EV_AUX_COMPA                                  0x0000006A
#define EVENT_RFCSEL9_EV_AUX_AON_WU_EV                              0x00000069
#define EVENT_RFCSEL9_EV_SWEV1                                      0x00000065
#define EVENT_RFCSEL9_EV_SWEV0                                      0x00000064
#define EVENT_RFCSEL9_EV_CRYPTO_RESULT_AVAIL_IRQ                    0x0000005D
#define EVENT_RFCSEL9_EV_AUX_TIMER2_PULSE                           0x0000003C
#define EVENT_RFCSEL9_EV_AUX_TIMER2_EV3                             0x0000003B
#define EVENT_RFCSEL9_EV_AUX_TIMER2_EV2                             0x0000003A
#define EVENT_RFCSEL9_EV_AUX_TIMER2_EV1                             0x00000039
#define EVENT_RFCSEL9_EV_AUX_TIMER2_EV0                             0x00000038
#define EVENT_RFCSEL9_EV_DMA_DONE_COMB                              0x00000027
#define EVENT_RFCSEL9_EV_UART1_COMB                                 0x00000025
#define EVENT_RFCSEL9_EV_UART0_COMB                                 0x00000024
#define EVENT_RFCSEL9_EV_SSI1_COMB                                  0x00000023
#define EVENT_RFCSEL9_EV_SSI0_COMB                                  0x00000022
#define EVENT_RFCSEL9_EV_WDT_IRQ                                    0x00000018
#define EVENT_RFCSEL9_EV_AON_AUX_SWEV0                              0x0000000A
#define EVENT_RFCSEL9_EV_I2S_IRQ                                    0x00000008
#define EVENT_RFCSEL9_EV_AON_PROG1                                  0x00000002
#define EVENT_RFCSEL9_EV_AON_PROG0                                  0x00000001
#define EVENT_RFCSEL9_EV_NONE                                       0x00000000

//*****************************************************************************
//
// Register: EVENT_O_GPT0ACAPTSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// PORT_EVENT1              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT1 wil be routed here.
// PORT_EVENT0              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT0 wil be routed here.
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// I2C_IRQ                  Interrupt event from I2C
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// NONE                     Always inactive
#define EVENT_GPT0ACAPTSEL_EV_W                                              7
#define EVENT_GPT0ACAPTSEL_EV_M                                     0x0000007F
#define EVENT_GPT0ACAPTSEL_EV_S                                              0
#define EVENT_GPT0ACAPTSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_GPT0ACAPTSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_GPT0ACAPTSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_GPT0ACAPTSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_GPT0ACAPTSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_GPT0ACAPTSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_GPT0ACAPTSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_GPT0ACAPTSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_GPT0ACAPTSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_GPT0ACAPTSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_GPT0ACAPTSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_GPT0ACAPTSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_GPT0ACAPTSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_GPT0ACAPTSEL_EV_PORT_EVENT1                           0x00000056
#define EVENT_GPT0ACAPTSEL_EV_PORT_EVENT0                           0x00000055
#define EVENT_GPT0ACAPTSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_GPT0ACAPTSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_GPT0ACAPTSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_GPT0ACAPTSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_GPT0ACAPTSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_GPT0ACAPTSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_GPT0ACAPTSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_GPT0ACAPTSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_GPT0ACAPTSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_GPT0ACAPTSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_GPT0ACAPTSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_GPT0ACAPTSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_GPT0ACAPTSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_GPT0ACAPTSEL_EV_UART1_COMB                            0x00000025
#define EVENT_GPT0ACAPTSEL_EV_UART0_COMB                            0x00000024
#define EVENT_GPT0ACAPTSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_GPT0ACAPTSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_GPT0ACAPTSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_GPT0ACAPTSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_GPT0ACAPTSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_GPT0ACAPTSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_GPT0ACAPTSEL_EV_FLASH                                 0x00000015
#define EVENT_GPT0ACAPTSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_GPT0ACAPTSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_GPT0ACAPTSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_GPT0ACAPTSEL_EV_OSC_COMB                              0x00000006
#define EVENT_GPT0ACAPTSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_GPT0ACAPTSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_GPT0ACAPTSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_GPT0BCAPTSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// PORT_EVENT1              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT1 wil be routed here.
// PORT_EVENT0              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT0 wil be routed here.
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// I2C_IRQ                  Interrupt event from I2C
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// NONE                     Always inactive
#define EVENT_GPT0BCAPTSEL_EV_W                                              7
#define EVENT_GPT0BCAPTSEL_EV_M                                     0x0000007F
#define EVENT_GPT0BCAPTSEL_EV_S                                              0
#define EVENT_GPT0BCAPTSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_GPT0BCAPTSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_GPT0BCAPTSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_GPT0BCAPTSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_GPT0BCAPTSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_GPT0BCAPTSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_GPT0BCAPTSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_GPT0BCAPTSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_GPT0BCAPTSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_GPT0BCAPTSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_GPT0BCAPTSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_GPT0BCAPTSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_GPT0BCAPTSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_GPT0BCAPTSEL_EV_PORT_EVENT1                           0x00000056
#define EVENT_GPT0BCAPTSEL_EV_PORT_EVENT0                           0x00000055
#define EVENT_GPT0BCAPTSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_GPT0BCAPTSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_GPT0BCAPTSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_GPT0BCAPTSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_GPT0BCAPTSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_GPT0BCAPTSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_GPT0BCAPTSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_GPT0BCAPTSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_GPT0BCAPTSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_GPT0BCAPTSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_GPT0BCAPTSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_GPT0BCAPTSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_GPT0BCAPTSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_GPT0BCAPTSEL_EV_UART1_COMB                            0x00000025
#define EVENT_GPT0BCAPTSEL_EV_UART0_COMB                            0x00000024
#define EVENT_GPT0BCAPTSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_GPT0BCAPTSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_GPT0BCAPTSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_GPT0BCAPTSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_GPT0BCAPTSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_GPT0BCAPTSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_GPT0BCAPTSEL_EV_FLASH                                 0x00000015
#define EVENT_GPT0BCAPTSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_GPT0BCAPTSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_GPT0BCAPTSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_GPT0BCAPTSEL_EV_OSC_COMB                              0x00000006
#define EVENT_GPT0BCAPTSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_GPT0BCAPTSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_GPT0BCAPTSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_GPT1ACAPTSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// PORT_EVENT3              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT3 wil be routed here.
// PORT_EVENT2              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT2 wil be routed here.
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// I2C_IRQ                  Interrupt event from I2C
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// NONE                     Always inactive
#define EVENT_GPT1ACAPTSEL_EV_W                                              7
#define EVENT_GPT1ACAPTSEL_EV_M                                     0x0000007F
#define EVENT_GPT1ACAPTSEL_EV_S                                              0
#define EVENT_GPT1ACAPTSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_GPT1ACAPTSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_GPT1ACAPTSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_GPT1ACAPTSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_GPT1ACAPTSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_GPT1ACAPTSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_GPT1ACAPTSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_GPT1ACAPTSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_GPT1ACAPTSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_GPT1ACAPTSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_GPT1ACAPTSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_GPT1ACAPTSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_GPT1ACAPTSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_GPT1ACAPTSEL_EV_PORT_EVENT3                           0x00000058
#define EVENT_GPT1ACAPTSEL_EV_PORT_EVENT2                           0x00000057
#define EVENT_GPT1ACAPTSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_GPT1ACAPTSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_GPT1ACAPTSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_GPT1ACAPTSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_GPT1ACAPTSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_GPT1ACAPTSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_GPT1ACAPTSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_GPT1ACAPTSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_GPT1ACAPTSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_GPT1ACAPTSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_GPT1ACAPTSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_GPT1ACAPTSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_GPT1ACAPTSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_GPT1ACAPTSEL_EV_UART1_COMB                            0x00000025
#define EVENT_GPT1ACAPTSEL_EV_UART0_COMB                            0x00000024
#define EVENT_GPT1ACAPTSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_GPT1ACAPTSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_GPT1ACAPTSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_GPT1ACAPTSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_GPT1ACAPTSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_GPT1ACAPTSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_GPT1ACAPTSEL_EV_FLASH                                 0x00000015
#define EVENT_GPT1ACAPTSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_GPT1ACAPTSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_GPT1ACAPTSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_GPT1ACAPTSEL_EV_OSC_COMB                              0x00000006
#define EVENT_GPT1ACAPTSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_GPT1ACAPTSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_GPT1ACAPTSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_GPT1BCAPTSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// PORT_EVENT3              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT3 wil be routed here.
// PORT_EVENT2              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT2 wil be routed here.
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// I2C_IRQ                  Interrupt event from I2C
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// NONE                     Always inactive
#define EVENT_GPT1BCAPTSEL_EV_W                                              7
#define EVENT_GPT1BCAPTSEL_EV_M                                     0x0000007F
#define EVENT_GPT1BCAPTSEL_EV_S                                              0
#define EVENT_GPT1BCAPTSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_GPT1BCAPTSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_GPT1BCAPTSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_GPT1BCAPTSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_GPT1BCAPTSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_GPT1BCAPTSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_GPT1BCAPTSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_GPT1BCAPTSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_GPT1BCAPTSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_GPT1BCAPTSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_GPT1BCAPTSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_GPT1BCAPTSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_GPT1BCAPTSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_GPT1BCAPTSEL_EV_PORT_EVENT3                           0x00000058
#define EVENT_GPT1BCAPTSEL_EV_PORT_EVENT2                           0x00000057
#define EVENT_GPT1BCAPTSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_GPT1BCAPTSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_GPT1BCAPTSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_GPT1BCAPTSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_GPT1BCAPTSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_GPT1BCAPTSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_GPT1BCAPTSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_GPT1BCAPTSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_GPT1BCAPTSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_GPT1BCAPTSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_GPT1BCAPTSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_GPT1BCAPTSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_GPT1BCAPTSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_GPT1BCAPTSEL_EV_UART1_COMB                            0x00000025
#define EVENT_GPT1BCAPTSEL_EV_UART0_COMB                            0x00000024
#define EVENT_GPT1BCAPTSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_GPT1BCAPTSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_GPT1BCAPTSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_GPT1BCAPTSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_GPT1BCAPTSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_GPT1BCAPTSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_GPT1BCAPTSEL_EV_FLASH                                 0x00000015
#define EVENT_GPT1BCAPTSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_GPT1BCAPTSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_GPT1BCAPTSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_GPT1BCAPTSEL_EV_OSC_COMB                              0x00000006
#define EVENT_GPT1BCAPTSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_GPT1BCAPTSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_GPT1BCAPTSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_GPT2ACAPTSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// PORT_EVENT5              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT4 wil be routed here.
// PORT_EVENT4              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT4 wil be routed here.
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// I2C_IRQ                  Interrupt event from I2C
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// NONE                     Always inactive
#define EVENT_GPT2ACAPTSEL_EV_W                                              7
#define EVENT_GPT2ACAPTSEL_EV_M                                     0x0000007F
#define EVENT_GPT2ACAPTSEL_EV_S                                              0
#define EVENT_GPT2ACAPTSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_GPT2ACAPTSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_GPT2ACAPTSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_GPT2ACAPTSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_GPT2ACAPTSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_GPT2ACAPTSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_GPT2ACAPTSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_GPT2ACAPTSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_GPT2ACAPTSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_GPT2ACAPTSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_GPT2ACAPTSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_GPT2ACAPTSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_GPT2ACAPTSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_GPT2ACAPTSEL_EV_PORT_EVENT5                           0x0000005A
#define EVENT_GPT2ACAPTSEL_EV_PORT_EVENT4                           0x00000059
#define EVENT_GPT2ACAPTSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_GPT2ACAPTSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_GPT2ACAPTSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_GPT2ACAPTSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_GPT2ACAPTSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_GPT2ACAPTSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_GPT2ACAPTSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_GPT2ACAPTSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_GPT2ACAPTSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_GPT2ACAPTSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_GPT2ACAPTSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_GPT2ACAPTSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_GPT2ACAPTSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_GPT2ACAPTSEL_EV_UART1_COMB                            0x00000025
#define EVENT_GPT2ACAPTSEL_EV_UART0_COMB                            0x00000024
#define EVENT_GPT2ACAPTSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_GPT2ACAPTSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_GPT2ACAPTSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_GPT2ACAPTSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_GPT2ACAPTSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_GPT2ACAPTSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_GPT2ACAPTSEL_EV_FLASH                                 0x00000015
#define EVENT_GPT2ACAPTSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_GPT2ACAPTSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_GPT2ACAPTSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_GPT2ACAPTSEL_EV_OSC_COMB                              0x00000006
#define EVENT_GPT2ACAPTSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_GPT2ACAPTSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_GPT2ACAPTSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_GPT2BCAPTSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// PORT_EVENT5              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT4 wil be routed here.
// PORT_EVENT4              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT4 wil be routed here.
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// I2C_IRQ                  Interrupt event from I2C
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// NONE                     Always inactive
#define EVENT_GPT2BCAPTSEL_EV_W                                              7
#define EVENT_GPT2BCAPTSEL_EV_M                                     0x0000007F
#define EVENT_GPT2BCAPTSEL_EV_S                                              0
#define EVENT_GPT2BCAPTSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_GPT2BCAPTSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_GPT2BCAPTSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_GPT2BCAPTSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_GPT2BCAPTSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_GPT2BCAPTSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_GPT2BCAPTSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_GPT2BCAPTSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_GPT2BCAPTSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_GPT2BCAPTSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_GPT2BCAPTSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_GPT2BCAPTSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_GPT2BCAPTSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_GPT2BCAPTSEL_EV_PORT_EVENT5                           0x0000005A
#define EVENT_GPT2BCAPTSEL_EV_PORT_EVENT4                           0x00000059
#define EVENT_GPT2BCAPTSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_GPT2BCAPTSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_GPT2BCAPTSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_GPT2BCAPTSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_GPT2BCAPTSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_GPT2BCAPTSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_GPT2BCAPTSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_GPT2BCAPTSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_GPT2BCAPTSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_GPT2BCAPTSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_GPT2BCAPTSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_GPT2BCAPTSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_GPT2BCAPTSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_GPT2BCAPTSEL_EV_UART1_COMB                            0x00000025
#define EVENT_GPT2BCAPTSEL_EV_UART0_COMB                            0x00000024
#define EVENT_GPT2BCAPTSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_GPT2BCAPTSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_GPT2BCAPTSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_GPT2BCAPTSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_GPT2BCAPTSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_GPT2BCAPTSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_GPT2BCAPTSEL_EV_FLASH                                 0x00000015
#define EVENT_GPT2BCAPTSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_GPT2BCAPTSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_GPT2BCAPTSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_GPT2BCAPTSEL_EV_OSC_COMB                              0x00000006
#define EVENT_GPT2BCAPTSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_GPT2BCAPTSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_GPT2BCAPTSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH1SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART0_RX_DMASREQ         UART0 RX DMA single request, controlled by
//                          UART0:DMACTL.RXDMAE
#define EVENT_UDMACH1SSEL_EV_W                                               7
#define EVENT_UDMACH1SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH1SSEL_EV_S                                               0
#define EVENT_UDMACH1SSEL_EV_UART0_RX_DMASREQ                       0x00000031

//*****************************************************************************
//
// Register: EVENT_O_UDMACH1BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART0_RX_DMABREQ         UART0 RX DMA burst request, controlled by
//                          UART0:DMACTL.RXDMAE
#define EVENT_UDMACH1BSEL_EV_W                                               7
#define EVENT_UDMACH1BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH1BSEL_EV_S                                               0
#define EVENT_UDMACH1BSEL_EV_UART0_RX_DMABREQ                       0x00000030

//*****************************************************************************
//
// Register: EVENT_O_UDMACH2SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART0_TX_DMASREQ         UART0 TX DMA single request, controlled by
//                          UART0:DMACTL.TXDMAE
#define EVENT_UDMACH2SSEL_EV_W                                               7
#define EVENT_UDMACH2SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH2SSEL_EV_S                                               0
#define EVENT_UDMACH2SSEL_EV_UART0_TX_DMASREQ                       0x00000033

//*****************************************************************************
//
// Register: EVENT_O_UDMACH2BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART0_TX_DMABREQ         UART0 TX DMA burst request, controlled by
//                          UART0:DMACTL.TXDMAE
#define EVENT_UDMACH2BSEL_EV_W                                               7
#define EVENT_UDMACH2BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH2BSEL_EV_S                                               0
#define EVENT_UDMACH2BSEL_EV_UART0_TX_DMABREQ                       0x00000032

//*****************************************************************************
//
// Register: EVENT_O_UDMACH3SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI0_RX_DMASREQ          SSI0 RX DMA single request, controlled by
//                          SSI0:DMACR.RXDMAE
#define EVENT_UDMACH3SSEL_EV_W                                               7
#define EVENT_UDMACH3SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH3SSEL_EV_S                                               0
#define EVENT_UDMACH3SSEL_EV_SSI0_RX_DMASREQ                        0x00000029

//*****************************************************************************
//
// Register: EVENT_O_UDMACH3BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI0_RX_DMABREQ          SSI0 RX DMA burst request , controlled by
//                          SSI0:DMACR.RXDMAE
#define EVENT_UDMACH3BSEL_EV_W                                               7
#define EVENT_UDMACH3BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH3BSEL_EV_S                                               0
#define EVENT_UDMACH3BSEL_EV_SSI0_RX_DMABREQ                        0x00000028

//*****************************************************************************
//
// Register: EVENT_O_UDMACH4SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI0_TX_DMASREQ          SSI0 TX DMA single request, controlled by
//                          SSI0:DMACR.TXDMAE
#define EVENT_UDMACH4SSEL_EV_W                                               7
#define EVENT_UDMACH4SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH4SSEL_EV_S                                               0
#define EVENT_UDMACH4SSEL_EV_SSI0_TX_DMASREQ                        0x0000002B

//*****************************************************************************
//
// Register: EVENT_O_UDMACH4BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI0_TX_DMABREQ          SSI0 TX DMA burst request , controlled by
//                          SSI0:DMACR.TXDMAE
#define EVENT_UDMACH4BSEL_EV_W                                               7
#define EVENT_UDMACH4BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH4BSEL_EV_S                                               0
#define EVENT_UDMACH4BSEL_EV_SSI0_TX_DMABREQ                        0x0000002A

//*****************************************************************************
//
// Register: EVENT_O_UDMACH5SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART1_RX_DMASREQ         UART1 RX DMA single request, controlled by
//                          UART1:DMACTL.RXDMAE
#define EVENT_UDMACH5SSEL_EV_W                                               7
#define EVENT_UDMACH5SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH5SSEL_EV_S                                               0
#define EVENT_UDMACH5SSEL_EV_UART1_RX_DMASREQ                       0x00000035

//*****************************************************************************
//
// Register: EVENT_O_UDMACH5BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART1_RX_DMABREQ         UART1 RX DMA burst request, controlled by
//                          UART1:DMACTL.RXDMAE
#define EVENT_UDMACH5BSEL_EV_W                                               7
#define EVENT_UDMACH5BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH5BSEL_EV_S                                               0
#define EVENT_UDMACH5BSEL_EV_UART1_RX_DMABREQ                       0x00000034

//*****************************************************************************
//
// Register: EVENT_O_UDMACH6SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART1_TX_DMASREQ         UART1 TX DMA single request, controlled by
//                          UART1:DMACTL.TXDMAE
#define EVENT_UDMACH6SSEL_EV_W                                               7
#define EVENT_UDMACH6SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH6SSEL_EV_S                                               0
#define EVENT_UDMACH6SSEL_EV_UART1_TX_DMASREQ                       0x00000037

//*****************************************************************************
//
// Register: EVENT_O_UDMACH6BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// UART1_TX_DMABREQ         UART1 TX DMA burst request, controlled by
//                          UART1:DMACTL.TXDMAE
#define EVENT_UDMACH6BSEL_EV_W                                               7
#define EVENT_UDMACH6BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH6BSEL_EV_S                                               0
#define EVENT_UDMACH6BSEL_EV_UART1_TX_DMABREQ                       0x00000036

//*****************************************************************************
//
// Register: EVENT_O_UDMACH7SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_DMASREQ              DMA single request event from AUX, configured by
//                          AUX_EVCTL:DMACTL
#define EVENT_UDMACH7SSEL_EV_W                                               7
#define EVENT_UDMACH7SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH7SSEL_EV_S                                               0
#define EVENT_UDMACH7SSEL_EV_AUX_DMASREQ                            0x00000075

//*****************************************************************************
//
// Register: EVENT_O_UDMACH7BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_DMABREQ              DMA burst request event from AUX, configured by
//                          AUX_EVCTL:DMACTL
#define EVENT_UDMACH7BSEL_EV_W                                               7
#define EVENT_UDMACH7BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH7BSEL_EV_S                                               0
#define EVENT_UDMACH7BSEL_EV_AUX_DMABREQ                            0x00000076

//*****************************************************************************
//
// Register: EVENT_O_UDMACH8SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_SW_DMABREQ           DMA sofware trigger from AUX, triggered by
//                          AUX_EVCTL:DMASWREQ.START
#define EVENT_UDMACH8SSEL_EV_W                                               7
#define EVENT_UDMACH8SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH8SSEL_EV_S                                               0
#define EVENT_UDMACH8SSEL_EV_AUX_SW_DMABREQ                         0x00000074

//*****************************************************************************
//
// Register: EVENT_O_UDMACH8BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AUX_SW_DMABREQ           DMA sofware trigger from AUX, triggered by
//                          AUX_EVCTL:DMASWREQ.START
#define EVENT_UDMACH8BSEL_EV_W                                               7
#define EVENT_UDMACH8BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH8BSEL_EV_S                                               0
#define EVENT_UDMACH8BSEL_EV_AUX_SW_DMABREQ                         0x00000074

//*****************************************************************************
//
// Register: EVENT_O_UDMACH9SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// TIE_LOW                  Not used tied to 0
// NONE                     Always inactive
#define EVENT_UDMACH9SSEL_EV_W                                               7
#define EVENT_UDMACH9SSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH9SSEL_EV_S                                               0
#define EVENT_UDMACH9SSEL_EV_ALWAYS_ACTIVE                          0x00000079
#define EVENT_UDMACH9SSEL_EV_GPT3B_DMABREQ                          0x00000054
#define EVENT_UDMACH9SSEL_EV_GPT3A_DMABREQ                          0x00000053
#define EVENT_UDMACH9SSEL_EV_GPT2B_DMABREQ                          0x00000052
#define EVENT_UDMACH9SSEL_EV_GPT2A_DMABREQ                          0x00000051
#define EVENT_UDMACH9SSEL_EV_GPT1B_DMABREQ                          0x00000050
#define EVENT_UDMACH9SSEL_EV_GPT1A_DMABREQ                          0x0000004F
#define EVENT_UDMACH9SSEL_EV_GPT0B_DMABREQ                          0x0000004E
#define EVENT_UDMACH9SSEL_EV_GPT0A_DMABREQ                          0x0000004D
#define EVENT_UDMACH9SSEL_EV_TIE_LOW                                0x00000045
#define EVENT_UDMACH9SSEL_EV_NONE                                   0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH9BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// NONE                     Always inactive
#define EVENT_UDMACH9BSEL_EV_W                                               7
#define EVENT_UDMACH9BSEL_EV_M                                      0x0000007F
#define EVENT_UDMACH9BSEL_EV_S                                               0
#define EVENT_UDMACH9BSEL_EV_ALWAYS_ACTIVE                          0x00000079
#define EVENT_UDMACH9BSEL_EV_GPT3B_DMABREQ                          0x00000054
#define EVENT_UDMACH9BSEL_EV_GPT3A_DMABREQ                          0x00000053
#define EVENT_UDMACH9BSEL_EV_GPT2B_DMABREQ                          0x00000052
#define EVENT_UDMACH9BSEL_EV_GPT2A_DMABREQ                          0x00000051
#define EVENT_UDMACH9BSEL_EV_GPT1B_DMABREQ                          0x00000050
#define EVENT_UDMACH9BSEL_EV_GPT1A_DMABREQ                          0x0000004F
#define EVENT_UDMACH9BSEL_EV_GPT0B_DMABREQ                          0x0000004E
#define EVENT_UDMACH9BSEL_EV_GPT0A_DMABREQ                          0x0000004D
#define EVENT_UDMACH9BSEL_EV_NONE                                   0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH10SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// TIE_LOW                  Not used tied to 0
// NONE                     Always inactive
#define EVENT_UDMACH10SSEL_EV_W                                              7
#define EVENT_UDMACH10SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH10SSEL_EV_S                                              0
#define EVENT_UDMACH10SSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_UDMACH10SSEL_EV_GPT3B_DMABREQ                         0x00000054
#define EVENT_UDMACH10SSEL_EV_GPT3A_DMABREQ                         0x00000053
#define EVENT_UDMACH10SSEL_EV_GPT2B_DMABREQ                         0x00000052
#define EVENT_UDMACH10SSEL_EV_GPT2A_DMABREQ                         0x00000051
#define EVENT_UDMACH10SSEL_EV_GPT1B_DMABREQ                         0x00000050
#define EVENT_UDMACH10SSEL_EV_GPT1A_DMABREQ                         0x0000004F
#define EVENT_UDMACH10SSEL_EV_GPT0B_DMABREQ                         0x0000004E
#define EVENT_UDMACH10SSEL_EV_GPT0A_DMABREQ                         0x0000004D
#define EVENT_UDMACH10SSEL_EV_TIE_LOW                               0x00000046
#define EVENT_UDMACH10SSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH10BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// NONE                     Always inactive
#define EVENT_UDMACH10BSEL_EV_W                                              7
#define EVENT_UDMACH10BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH10BSEL_EV_S                                              0
#define EVENT_UDMACH10BSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_UDMACH10BSEL_EV_GPT3B_DMABREQ                         0x00000054
#define EVENT_UDMACH10BSEL_EV_GPT3A_DMABREQ                         0x00000053
#define EVENT_UDMACH10BSEL_EV_GPT2B_DMABREQ                         0x00000052
#define EVENT_UDMACH10BSEL_EV_GPT2A_DMABREQ                         0x00000051
#define EVENT_UDMACH10BSEL_EV_GPT1B_DMABREQ                         0x00000050
#define EVENT_UDMACH10BSEL_EV_GPT1A_DMABREQ                         0x0000004F
#define EVENT_UDMACH10BSEL_EV_GPT0B_DMABREQ                         0x0000004E
#define EVENT_UDMACH10BSEL_EV_GPT0A_DMABREQ                         0x0000004D
#define EVENT_UDMACH10BSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH11SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// TIE_LOW                  Not used tied to 0
// NONE                     Always inactive
#define EVENT_UDMACH11SSEL_EV_W                                              7
#define EVENT_UDMACH11SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH11SSEL_EV_S                                              0
#define EVENT_UDMACH11SSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_UDMACH11SSEL_EV_GPT3B_DMABREQ                         0x00000054
#define EVENT_UDMACH11SSEL_EV_GPT3A_DMABREQ                         0x00000053
#define EVENT_UDMACH11SSEL_EV_GPT2B_DMABREQ                         0x00000052
#define EVENT_UDMACH11SSEL_EV_GPT2A_DMABREQ                         0x00000051
#define EVENT_UDMACH11SSEL_EV_GPT1B_DMABREQ                         0x00000050
#define EVENT_UDMACH11SSEL_EV_GPT1A_DMABREQ                         0x0000004F
#define EVENT_UDMACH11SSEL_EV_GPT0B_DMABREQ                         0x0000004E
#define EVENT_UDMACH11SSEL_EV_GPT0A_DMABREQ                         0x0000004D
#define EVENT_UDMACH11SSEL_EV_TIE_LOW                               0x00000047
#define EVENT_UDMACH11SSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH11BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// NONE                     Always inactive
#define EVENT_UDMACH11BSEL_EV_W                                              7
#define EVENT_UDMACH11BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH11BSEL_EV_S                                              0
#define EVENT_UDMACH11BSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_UDMACH11BSEL_EV_GPT3B_DMABREQ                         0x00000054
#define EVENT_UDMACH11BSEL_EV_GPT3A_DMABREQ                         0x00000053
#define EVENT_UDMACH11BSEL_EV_GPT2B_DMABREQ                         0x00000052
#define EVENT_UDMACH11BSEL_EV_GPT2A_DMABREQ                         0x00000051
#define EVENT_UDMACH11BSEL_EV_GPT1B_DMABREQ                         0x00000050
#define EVENT_UDMACH11BSEL_EV_GPT1A_DMABREQ                         0x0000004F
#define EVENT_UDMACH11BSEL_EV_GPT0B_DMABREQ                         0x0000004E
#define EVENT_UDMACH11BSEL_EV_GPT0A_DMABREQ                         0x0000004D
#define EVENT_UDMACH11BSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH12SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// TIE_LOW                  Not used tied to 0
// NONE                     Always inactive
#define EVENT_UDMACH12SSEL_EV_W                                              7
#define EVENT_UDMACH12SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH12SSEL_EV_S                                              0
#define EVENT_UDMACH12SSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_UDMACH12SSEL_EV_GPT3B_DMABREQ                         0x00000054
#define EVENT_UDMACH12SSEL_EV_GPT3A_DMABREQ                         0x00000053
#define EVENT_UDMACH12SSEL_EV_GPT2B_DMABREQ                         0x00000052
#define EVENT_UDMACH12SSEL_EV_GPT2A_DMABREQ                         0x00000051
#define EVENT_UDMACH12SSEL_EV_GPT1B_DMABREQ                         0x00000050
#define EVENT_UDMACH12SSEL_EV_GPT1A_DMABREQ                         0x0000004F
#define EVENT_UDMACH12SSEL_EV_GPT0B_DMABREQ                         0x0000004E
#define EVENT_UDMACH12SSEL_EV_GPT0A_DMABREQ                         0x0000004D
#define EVENT_UDMACH12SSEL_EV_TIE_LOW                               0x00000048
#define EVENT_UDMACH12SSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH12BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// NONE                     Always inactive
#define EVENT_UDMACH12BSEL_EV_W                                              7
#define EVENT_UDMACH12BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH12BSEL_EV_S                                              0
#define EVENT_UDMACH12BSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_UDMACH12BSEL_EV_GPT3B_DMABREQ                         0x00000054
#define EVENT_UDMACH12BSEL_EV_GPT3A_DMABREQ                         0x00000053
#define EVENT_UDMACH12BSEL_EV_GPT2B_DMABREQ                         0x00000052
#define EVENT_UDMACH12BSEL_EV_GPT2A_DMABREQ                         0x00000051
#define EVENT_UDMACH12BSEL_EV_GPT1B_DMABREQ                         0x00000050
#define EVENT_UDMACH12BSEL_EV_GPT1A_DMABREQ                         0x0000004F
#define EVENT_UDMACH12BSEL_EV_GPT0B_DMABREQ                         0x0000004E
#define EVENT_UDMACH12BSEL_EV_GPT0A_DMABREQ                         0x0000004D
#define EVENT_UDMACH12BSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH13BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AON_PROG2                AON programmable event 2. Event selected by
//                          AON_EVENT MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG2_EV
#define EVENT_UDMACH13BSEL_EV_W                                              7
#define EVENT_UDMACH13BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH13BSEL_EV_S                                              0
#define EVENT_UDMACH13BSEL_EV_AON_PROG2                             0x00000003

//*****************************************************************************
//
// Register: EVENT_O_UDMACH14BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// CPU_HALTED               CPU halted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_DMABREQ              DMA burst request event from AUX, configured by
//                          AUX_EVCTL:DMACTL
// AUX_DMASREQ              DMA single request event from AUX, configured by
//                          AUX_EVCTL:DMACTL
// AUX_SW_DMABREQ           DMA sofware trigger from AUX, triggered by
//                          AUX_EVCTL:DMASWREQ.START
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// TRNG_IRQ                 TRNG Interrupt event, controlled by TRNG:IRQEN.EN
// SWEV3                    Software event 3, triggered by SWEV.SWEV3
// SWEV2                    Software event 2, triggered by SWEV.SWEV2
// SWEV1                    Software event 1, triggered by SWEV.SWEV1
// SWEV0                    Software event 0, triggered by SWEV.SWEV0
// WDT_NMI                  Watchdog non maskable interrupt event, controlled
//                          by WDT:CTL.INTTYPE
// CRYPTO_DMA_DONE_IRQ      CRYPTO DMA input done event, the correspondingg
//                          flag is CRYPTO:IRQSTAT.DMA_IN_DONE. Controlled
//                          by CRYPTO:IRQEN.DMA_IN_DONE
// CRYPTO_RESULT_AVAIL_IRQ  CRYPTO result available interupt event, the
//                          corresponding flag is found here
//                          CRYPTO:IRQSTAT.RESULT_AVAIL. Controlled by
//                          CRYPTO:IRQSTAT.RESULT_AVAIL
// PORT_EVENT7              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT7 wil be routed here.
// PORT_EVENT6              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT6 wil be routed here.
// PORT_EVENT5              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT4 wil be routed here.
// PORT_EVENT4              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT4 wil be routed here.
// PORT_EVENT3              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT3 wil be routed here.
// PORT_EVENT2              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT2 wil be routed here.
// PORT_EVENT1              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT1 wil be routed here.
// PORT_EVENT0              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT0 wil be routed here.
// GPT3B_DMABREQ            GPT3B DMA trigger event. Configured by GPT3:DMAEV
// GPT3A_DMABREQ            GPT3A DMA trigger event. Configured by GPT3:DMAEV
// GPT2B_DMABREQ            GPT2B DMA trigger event. Configured by GPT2:DMAEV
// GPT2A_DMABREQ            GPT2A DMA trigger event. Configured by GPT2:DMAEV
// GPT1B_DMABREQ            GPT1B DMA trigger event. Configured by GPT1:DMAEV
// GPT1A_DMABREQ            GPT1A DMA trigger event. Configured by GPT1:DMAEV
// GPT0B_DMABREQ            GPT0B DMA trigger event. Configured by GPT0:DMAEV
// GPT0A_DMABREQ            GPT0A DMA trigger event. Configured by GPT0:DMAEV
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_TX_DMASREQ         UART1 TX DMA single request, controlled by
//                          UART1:DMACTL.TXDMAE
// UART1_TX_DMABREQ         UART1 TX DMA burst request, controlled by
//                          UART1:DMACTL.TXDMAE
// UART1_RX_DMASREQ         UART1 RX DMA single request, controlled by
//                          UART1:DMACTL.RXDMAE
// UART1_RX_DMABREQ         UART1 RX DMA burst request, controlled by
//                          UART1:DMACTL.RXDMAE
// UART0_TX_DMASREQ         UART0 TX DMA single request, controlled by
//                          UART0:DMACTL.TXDMAE
// UART0_TX_DMABREQ         UART0 TX DMA burst request, controlled by
//                          UART0:DMACTL.TXDMAE
// UART0_RX_DMASREQ         UART0 RX DMA single request, controlled by
//                          UART0:DMACTL.RXDMAE
// UART0_RX_DMABREQ         UART0 RX DMA burst request, controlled by
//                          UART0:DMACTL.RXDMAE
// SSI1_TX_DMASREQ          SSI1 TX DMA single request, controlled by
//                          SSI0:DMACR.TXDMAE
// SSI1_TX_DMABREQ          SSI1 TX DMA burst request , controlled by
//                          SSI0:DMACR.TXDMAE
// SSI1_RX_DMASREQ          SSI1 RX DMA single request, controlled by
//                          SSI0:DMACR.RXDMAE
// SSI1_RX_DMABREQ          SSI1 RX DMA burst request , controlled by
//                          SSI0:DMACR.RXDMAE
// SSI0_TX_DMASREQ          SSI0 TX DMA single request, controlled by
//                          SSI0:DMACR.TXDMAE
// SSI0_TX_DMABREQ          SSI0 TX DMA burst request , controlled by
//                          SSI0:DMACR.TXDMAE
// SSI0_RX_DMASREQ          SSI0 RX DMA single request, controlled by
//                          SSI0:DMACR.RXDMAE
// SSI0_RX_DMABREQ          SSI0 RX DMA burst request , controlled by
//                          SSI0:DMACR.RXDMAE
// DMA_DONE_COMB            Combined DMA done, corresponding flags are here
//                          UDMA0:REQDONE
// DMA_ERR                  DMA bus error, corresponds to UDMA0:ERROR.STATUS
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// PKA_IRQ                  PKA Interrupt event
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// AUX_SWEV1                AUX software event 1, triggered by
//                          AUX_EVCTL:SWEVSET.SWEV1, also available as
//                          AUX_EVENT2 AON wake up event.
//                          MCU domain wakeup control
//                          AON_EVENT:MCUWUSEL
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// WDT_IRQ                  Watchdog interrupt event, controlled by
//                          WDT:CTL.INTEN
// DMA_CH18_DONE            DMA done for software tiggered UDMA channel 18,
//                          see UDMA0:SOFTREQ
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// DMA_CH0_DONE             DMA done for software tiggered UDMA channel 0, see
//                          UDMA0:SOFTREQ
// GPT1B                    GPT1B interrupt event, controlled by GPT1:TBMR
// GPT1A                    GPT1A interrupt event, controlled by GPT1:TAMR
// GPT0B                    GPT0B interrupt event, controlled by GPT0:TBMR
// GPT0A                    GPT0A interrupt event, controlled by GPT0:TAMR
// GPT3B                    GPT3B interrupt event, controlled by GPT3:TBMR
// GPT3A                    GPT3A interrupt event, controlled by GPT3:TAMR
// GPT2B                    GPT2B interrupt event, controlled by GPT2:TBMR
// GPT2A                    GPT2A interrupt event, controlled by GPT2:TAMR
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// AON_AUX_SWEV0            AUX Software event 0, AUX_EVCTL:SWEVSET.SWEV0
// I2C_IRQ                  Interrupt event from I2C
// I2S_IRQ                  Interrupt event from I2S
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// AON_PROG2                AON programmable event 2. Event selected by
//                          AON_EVENT MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG2_EV
// AON_PROG1                AON programmable event 1. Event selected by
//                          AON_EVENT MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG1_EV
// AON_PROG0                AON programmable event 0. Event selected by
//                          AON_EVENT  MCU event selector,
//                          AON_EVENT:EVTOMCUSEL.AON_PROG0_EV
// NONE                     Always inactive
#define EVENT_UDMACH14BSEL_EV_W                                              7
#define EVENT_UDMACH14BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH14BSEL_EV_S                                              0
#define EVENT_UDMACH14BSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_UDMACH14BSEL_EV_CPU_HALTED                            0x00000078
#define EVENT_UDMACH14BSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_UDMACH14BSEL_EV_AUX_DMABREQ                           0x00000076
#define EVENT_UDMACH14BSEL_EV_AUX_DMASREQ                           0x00000075
#define EVENT_UDMACH14BSEL_EV_AUX_SW_DMABREQ                        0x00000074
#define EVENT_UDMACH14BSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_UDMACH14BSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_UDMACH14BSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_UDMACH14BSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_UDMACH14BSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_UDMACH14BSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_UDMACH14BSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_UDMACH14BSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_UDMACH14BSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_UDMACH14BSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_UDMACH14BSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_UDMACH14BSEL_EV_TRNG_IRQ                              0x00000068
#define EVENT_UDMACH14BSEL_EV_SWEV3                                 0x00000067
#define EVENT_UDMACH14BSEL_EV_SWEV2                                 0x00000066
#define EVENT_UDMACH14BSEL_EV_SWEV1                                 0x00000065
#define EVENT_UDMACH14BSEL_EV_SWEV0                                 0x00000064
#define EVENT_UDMACH14BSEL_EV_WDT_NMI                               0x00000063
#define EVENT_UDMACH14BSEL_EV_CRYPTO_DMA_DONE_IRQ                   0x0000005E
#define EVENT_UDMACH14BSEL_EV_CRYPTO_RESULT_AVAIL_IRQ               0x0000005D
#define EVENT_UDMACH14BSEL_EV_PORT_EVENT7                           0x0000005C
#define EVENT_UDMACH14BSEL_EV_PORT_EVENT6                           0x0000005B
#define EVENT_UDMACH14BSEL_EV_PORT_EVENT5                           0x0000005A
#define EVENT_UDMACH14BSEL_EV_PORT_EVENT4                           0x00000059
#define EVENT_UDMACH14BSEL_EV_PORT_EVENT3                           0x00000058
#define EVENT_UDMACH14BSEL_EV_PORT_EVENT2                           0x00000057
#define EVENT_UDMACH14BSEL_EV_PORT_EVENT1                           0x00000056
#define EVENT_UDMACH14BSEL_EV_PORT_EVENT0                           0x00000055
#define EVENT_UDMACH14BSEL_EV_GPT3B_DMABREQ                         0x00000054
#define EVENT_UDMACH14BSEL_EV_GPT3A_DMABREQ                         0x00000053
#define EVENT_UDMACH14BSEL_EV_GPT2B_DMABREQ                         0x00000052
#define EVENT_UDMACH14BSEL_EV_GPT2A_DMABREQ                         0x00000051
#define EVENT_UDMACH14BSEL_EV_GPT1B_DMABREQ                         0x00000050
#define EVENT_UDMACH14BSEL_EV_GPT1A_DMABREQ                         0x0000004F
#define EVENT_UDMACH14BSEL_EV_GPT0B_DMABREQ                         0x0000004E
#define EVENT_UDMACH14BSEL_EV_GPT0A_DMABREQ                         0x0000004D
#define EVENT_UDMACH14BSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_UDMACH14BSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_UDMACH14BSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_UDMACH14BSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_UDMACH14BSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_UDMACH14BSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_UDMACH14BSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_UDMACH14BSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_UDMACH14BSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_UDMACH14BSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_UDMACH14BSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_UDMACH14BSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_UDMACH14BSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_UDMACH14BSEL_EV_UART1_TX_DMASREQ                      0x00000037
#define EVENT_UDMACH14BSEL_EV_UART1_TX_DMABREQ                      0x00000036
#define EVENT_UDMACH14BSEL_EV_UART1_RX_DMASREQ                      0x00000035
#define EVENT_UDMACH14BSEL_EV_UART1_RX_DMABREQ                      0x00000034
#define EVENT_UDMACH14BSEL_EV_UART0_TX_DMASREQ                      0x00000033
#define EVENT_UDMACH14BSEL_EV_UART0_TX_DMABREQ                      0x00000032
#define EVENT_UDMACH14BSEL_EV_UART0_RX_DMASREQ                      0x00000031
#define EVENT_UDMACH14BSEL_EV_UART0_RX_DMABREQ                      0x00000030
#define EVENT_UDMACH14BSEL_EV_SSI1_TX_DMASREQ                       0x0000002F
#define EVENT_UDMACH14BSEL_EV_SSI1_TX_DMABREQ                       0x0000002E
#define EVENT_UDMACH14BSEL_EV_SSI1_RX_DMASREQ                       0x0000002D
#define EVENT_UDMACH14BSEL_EV_SSI1_RX_DMABREQ                       0x0000002C
#define EVENT_UDMACH14BSEL_EV_SSI0_TX_DMASREQ                       0x0000002B
#define EVENT_UDMACH14BSEL_EV_SSI0_TX_DMABREQ                       0x0000002A
#define EVENT_UDMACH14BSEL_EV_SSI0_RX_DMASREQ                       0x00000029
#define EVENT_UDMACH14BSEL_EV_SSI0_RX_DMABREQ                       0x00000028
#define EVENT_UDMACH14BSEL_EV_DMA_DONE_COMB                         0x00000027
#define EVENT_UDMACH14BSEL_EV_DMA_ERR                               0x00000026
#define EVENT_UDMACH14BSEL_EV_UART1_COMB                            0x00000025
#define EVENT_UDMACH14BSEL_EV_UART0_COMB                            0x00000024
#define EVENT_UDMACH14BSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_UDMACH14BSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_UDMACH14BSEL_EV_PKA_IRQ                               0x0000001F
#define EVENT_UDMACH14BSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_UDMACH14BSEL_EV_AUX_SWEV1                             0x0000001D
#define EVENT_UDMACH14BSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_UDMACH14BSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_UDMACH14BSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_UDMACH14BSEL_EV_WDT_IRQ                               0x00000018
#define EVENT_UDMACH14BSEL_EV_DMA_CH18_DONE                         0x00000016
#define EVENT_UDMACH14BSEL_EV_FLASH                                 0x00000015
#define EVENT_UDMACH14BSEL_EV_DMA_CH0_DONE                          0x00000014
#define EVENT_UDMACH14BSEL_EV_GPT1B                                 0x00000013
#define EVENT_UDMACH14BSEL_EV_GPT1A                                 0x00000012
#define EVENT_UDMACH14BSEL_EV_GPT0B                                 0x00000011
#define EVENT_UDMACH14BSEL_EV_GPT0A                                 0x00000010
#define EVENT_UDMACH14BSEL_EV_GPT3B                                 0x0000000F
#define EVENT_UDMACH14BSEL_EV_GPT3A                                 0x0000000E
#define EVENT_UDMACH14BSEL_EV_GPT2B                                 0x0000000D
#define EVENT_UDMACH14BSEL_EV_GPT2A                                 0x0000000C
#define EVENT_UDMACH14BSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_UDMACH14BSEL_EV_AON_AUX_SWEV0                         0x0000000A
#define EVENT_UDMACH14BSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_UDMACH14BSEL_EV_I2S_IRQ                               0x00000008
#define EVENT_UDMACH14BSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_UDMACH14BSEL_EV_OSC_COMB                              0x00000006
#define EVENT_UDMACH14BSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_UDMACH14BSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_UDMACH14BSEL_EV_AON_PROG2                             0x00000003
#define EVENT_UDMACH14BSEL_EV_AON_PROG1                             0x00000002
#define EVENT_UDMACH14BSEL_EV_AON_PROG0                             0x00000001
#define EVENT_UDMACH14BSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_UDMACH15BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
#define EVENT_UDMACH15BSEL_EV_W                                              7
#define EVENT_UDMACH15BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH15BSEL_EV_S                                              0
#define EVENT_UDMACH15BSEL_EV_AON_RTC_COMB                          0x00000007

//*****************************************************************************
//
// Register: EVENT_O_UDMACH16SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI1_RX_DMASREQ          SSI1 RX DMA single request, controlled by
//                          SSI0:DMACR.RXDMAE
#define EVENT_UDMACH16SSEL_EV_W                                              7
#define EVENT_UDMACH16SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH16SSEL_EV_S                                              0
#define EVENT_UDMACH16SSEL_EV_SSI1_RX_DMASREQ                       0x0000002D

//*****************************************************************************
//
// Register: EVENT_O_UDMACH16BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI1_RX_DMABREQ          SSI1 RX DMA burst request , controlled by
//                          SSI0:DMACR.RXDMAE
#define EVENT_UDMACH16BSEL_EV_W                                              7
#define EVENT_UDMACH16BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH16BSEL_EV_S                                              0
#define EVENT_UDMACH16BSEL_EV_SSI1_RX_DMABREQ                       0x0000002C

//*****************************************************************************
//
// Register: EVENT_O_UDMACH17SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI1_TX_DMASREQ          SSI1 TX DMA single request, controlled by
//                          SSI0:DMACR.TXDMAE
#define EVENT_UDMACH17SSEL_EV_W                                              7
#define EVENT_UDMACH17SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH17SSEL_EV_S                                              0
#define EVENT_UDMACH17SSEL_EV_SSI1_TX_DMASREQ                       0x0000002F

//*****************************************************************************
//
// Register: EVENT_O_UDMACH17BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SSI1_TX_DMABREQ          SSI1 TX DMA burst request , controlled by
//                          SSI0:DMACR.TXDMAE
#define EVENT_UDMACH17BSEL_EV_W                                              7
#define EVENT_UDMACH17BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH17BSEL_EV_S                                              0
#define EVENT_UDMACH17BSEL_EV_SSI1_TX_DMABREQ                       0x0000002E

//*****************************************************************************
//
// Register: EVENT_O_UDMACH21SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV0                    Software event 0, triggered by SWEV.SWEV0
#define EVENT_UDMACH21SSEL_EV_W                                              7
#define EVENT_UDMACH21SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH21SSEL_EV_S                                              0
#define EVENT_UDMACH21SSEL_EV_SWEV0                                 0x00000064

//*****************************************************************************
//
// Register: EVENT_O_UDMACH21BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV0                    Software event 0, triggered by SWEV.SWEV0
#define EVENT_UDMACH21BSEL_EV_W                                              7
#define EVENT_UDMACH21BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH21BSEL_EV_S                                              0
#define EVENT_UDMACH21BSEL_EV_SWEV0                                 0x00000064

//*****************************************************************************
//
// Register: EVENT_O_UDMACH22SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV1                    Software event 1, triggered by SWEV.SWEV1
#define EVENT_UDMACH22SSEL_EV_W                                              7
#define EVENT_UDMACH22SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH22SSEL_EV_S                                              0
#define EVENT_UDMACH22SSEL_EV_SWEV1                                 0x00000065

//*****************************************************************************
//
// Register: EVENT_O_UDMACH22BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV1                    Software event 1, triggered by SWEV.SWEV1
#define EVENT_UDMACH22BSEL_EV_W                                              7
#define EVENT_UDMACH22BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH22BSEL_EV_S                                              0
#define EVENT_UDMACH22BSEL_EV_SWEV1                                 0x00000065

//*****************************************************************************
//
// Register: EVENT_O_UDMACH23SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV2                    Software event 2, triggered by SWEV.SWEV2
#define EVENT_UDMACH23SSEL_EV_W                                              7
#define EVENT_UDMACH23SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH23SSEL_EV_S                                              0
#define EVENT_UDMACH23SSEL_EV_SWEV2                                 0x00000066

//*****************************************************************************
//
// Register: EVENT_O_UDMACH23BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV2                    Software event 2, triggered by SWEV.SWEV2
#define EVENT_UDMACH23BSEL_EV_W                                              7
#define EVENT_UDMACH23BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH23BSEL_EV_S                                              0
#define EVENT_UDMACH23BSEL_EV_SWEV2                                 0x00000066

//*****************************************************************************
//
// Register: EVENT_O_UDMACH24SSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV3                    Software event 3, triggered by SWEV.SWEV3
#define EVENT_UDMACH24SSEL_EV_W                                              7
#define EVENT_UDMACH24SSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH24SSEL_EV_S                                              0
#define EVENT_UDMACH24SSEL_EV_SWEV3                                 0x00000067

//*****************************************************************************
//
// Register: EVENT_O_UDMACH24BSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// SWEV3                    Software event 3, triggered by SWEV.SWEV3
#define EVENT_UDMACH24BSEL_EV_W                                              7
#define EVENT_UDMACH24BSEL_EV_M                                     0x0000007F
#define EVENT_UDMACH24BSEL_EV_S                                              0
#define EVENT_UDMACH24BSEL_EV_SWEV3                                 0x00000067

//*****************************************************************************
//
// Register: EVENT_O_GPT3ACAPTSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// PORT_EVENT7              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT7 wil be routed here.
// PORT_EVENT6              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT6 wil be routed here.
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// I2C_IRQ                  Interrupt event from I2C
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// NONE                     Always inactive
#define EVENT_GPT3ACAPTSEL_EV_W                                              7
#define EVENT_GPT3ACAPTSEL_EV_M                                     0x0000007F
#define EVENT_GPT3ACAPTSEL_EV_S                                              0
#define EVENT_GPT3ACAPTSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_GPT3ACAPTSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_GPT3ACAPTSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_GPT3ACAPTSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_GPT3ACAPTSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_GPT3ACAPTSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_GPT3ACAPTSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_GPT3ACAPTSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_GPT3ACAPTSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_GPT3ACAPTSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_GPT3ACAPTSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_GPT3ACAPTSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_GPT3ACAPTSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_GPT3ACAPTSEL_EV_PORT_EVENT7                           0x0000005C
#define EVENT_GPT3ACAPTSEL_EV_PORT_EVENT6                           0x0000005B
#define EVENT_GPT3ACAPTSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_GPT3ACAPTSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_GPT3ACAPTSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_GPT3ACAPTSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_GPT3ACAPTSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_GPT3ACAPTSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_GPT3ACAPTSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_GPT3ACAPTSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_GPT3ACAPTSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_GPT3ACAPTSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_GPT3ACAPTSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_GPT3ACAPTSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_GPT3ACAPTSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_GPT3ACAPTSEL_EV_UART1_COMB                            0x00000025
#define EVENT_GPT3ACAPTSEL_EV_UART0_COMB                            0x00000024
#define EVENT_GPT3ACAPTSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_GPT3ACAPTSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_GPT3ACAPTSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_GPT3ACAPTSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_GPT3ACAPTSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_GPT3ACAPTSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_GPT3ACAPTSEL_EV_FLASH                                 0x00000015
#define EVENT_GPT3ACAPTSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_GPT3ACAPTSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_GPT3ACAPTSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_GPT3ACAPTSEL_EV_OSC_COMB                              0x00000006
#define EVENT_GPT3ACAPTSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_GPT3ACAPTSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_GPT3ACAPTSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_GPT3BCAPTSEL
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// AON_RTC_UPD              RTC periodic event controlled by
//                          AON_RTC:CTL.RTC_UPD_EN
// AUX_ADC_IRQ              AUX ADC interrupt event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_IRQ. Status
//                          flags are found here AUX_EVCTL:EVTOMCUFLAGS
// AUX_OBSMUX0              Loopback of OBSMUX0 through AUX, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.MCU_OBSMUX0
// AUX_ADC_FIFO_ALMOST_FULL AUX ADC FIFO watermark event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_FIFO_ALMOST_FULL
// AUX_ADC_DONE             AUX ADC done, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_ADC_DONE
// AUX_SMPH_AUTOTAKE_DONE   Autotake event from AUX semaphore, configured by
//                          AUX_SMPH:AUTOTAKE
// AUX_TIMER1_EV            AUX timer 1 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER1_EV
// AUX_TIMER0_EV            AUX timer 0 event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER0_EV
// AUX_TDC_DONE             AUX TDC measurement done event, corresponds to the
//                          flag AUX_EVCTL:EVTOMCUFLAGS.AUX_TDC_DONE and
//                          the AUX_TDC status AUX_TDC:STAT.DONE
// AUX_COMPB                AUX Compare B event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPB
// AUX_COMPA                AUX Compare A event, corresponds to
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_COMPA
// AUX_AON_WU_EV            AON wakeup event, the corresponding flag is here
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_WU_EV
// PORT_EVENT7              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT7 wil be routed here.
// PORT_EVENT6              Port capture event from IOC, configured by
//                          IOC:IOCFGn.PORT_ID. Events on ports configured
//                          with ENUM PORT_EVENT6 wil be routed here.
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// AUX_TIMER2_PULSE         AUX Timer2 pulse, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_PULSE
// AUX_TIMER2_EV3           AUX Timer2 event 3, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV3
// AUX_TIMER2_EV2           AUX Timer2 event 2, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV2
// AUX_TIMER2_EV1           AUX Timer2 event 1, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV1
// AUX_TIMER2_EV0           AUX Timer2 event 0, corresponding to flag
//                          AUX_EVCTL:EVTOMCUFLAGS.AUX_TIMER2_EV0
// UART1_COMB               UART1 combined interrupt, interrupt flags are
//                          found here UART1:MIS
// UART0_COMB               UART0 combined interrupt, interrupt flags are
//                          found here UART0:MIS
// SSI1_COMB                SSI1 combined interrupt, interrupt flags are found
//                          here SSI1:MIS
// SSI0_COMB                SSI0 combined interrupt, interrupt flags are found
//                          here SSI0:MIS
// RFC_CPE_1                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE1 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_1 event
// RFC_CPE_0                Combined Interrupt for CPE Generated events.
//                          Corresponding flags are here
//                          RFC_DBELL:RFCPEIFG. Only interrupts selected
//                          with CPE0 in RFC_DBELL:RFCPEIFG can trigger a
//                          RFC_CPE_0 event
// RFC_HW_COMB              Combined RFC hardware interrupt, corresponding
//                          flag is here RFC_DBELL:RFHWIFG
// RFC_CMD_ACK              RFC Doorbell Command Acknowledgement Interrupt,
//                          equvialent to RFC_DBELL:RFACKIFG.ACKFLAG
// FLASH                    FLASH controller error event,  the status flags
//                          are FLASH:FEDACSTAT.FSM_DONE and
//                          FLASH:FEDACSTAT.RVF_INT
// AUX_COMB                 AUX combined event, the corresponding flag
//                          register is here AUX_EVCTL:EVTOMCUFLAGS
// I2C_IRQ                  Interrupt event from I2C
// AON_RTC_COMB             Event from AON_RTC, controlled by the
//                          AON_RTC:CTL.COMB_EV_MASK setting
// OSC_COMB                 Combined event from Oscillator control
// BATMON_COMB              Combined event from battery monitor
// AON_GPIO_EDGE            Edge detect event from IOC. Configureded by the
//                          IOC:IOCFGn.EDGE_IRQ_EN and  IOC:IOCFGn.EDGE_DET
//                          settings
// NONE                     Always inactive
#define EVENT_GPT3BCAPTSEL_EV_W                                              7
#define EVENT_GPT3BCAPTSEL_EV_M                                     0x0000007F
#define EVENT_GPT3BCAPTSEL_EV_S                                              0
#define EVENT_GPT3BCAPTSEL_EV_ALWAYS_ACTIVE                         0x00000079
#define EVENT_GPT3BCAPTSEL_EV_AON_RTC_UPD                           0x00000077
#define EVENT_GPT3BCAPTSEL_EV_AUX_ADC_IRQ                           0x00000073
#define EVENT_GPT3BCAPTSEL_EV_AUX_OBSMUX0                           0x00000072
#define EVENT_GPT3BCAPTSEL_EV_AUX_ADC_FIFO_ALMOST_FULL              0x00000071
#define EVENT_GPT3BCAPTSEL_EV_AUX_ADC_DONE                          0x00000070
#define EVENT_GPT3BCAPTSEL_EV_AUX_SMPH_AUTOTAKE_DONE                0x0000006F
#define EVENT_GPT3BCAPTSEL_EV_AUX_TIMER1_EV                         0x0000006E
#define EVENT_GPT3BCAPTSEL_EV_AUX_TIMER0_EV                         0x0000006D
#define EVENT_GPT3BCAPTSEL_EV_AUX_TDC_DONE                          0x0000006C
#define EVENT_GPT3BCAPTSEL_EV_AUX_COMPB                             0x0000006B
#define EVENT_GPT3BCAPTSEL_EV_AUX_COMPA                             0x0000006A
#define EVENT_GPT3BCAPTSEL_EV_AUX_AON_WU_EV                         0x00000069
#define EVENT_GPT3BCAPTSEL_EV_PORT_EVENT7                           0x0000005C
#define EVENT_GPT3BCAPTSEL_EV_PORT_EVENT6                           0x0000005B
#define EVENT_GPT3BCAPTSEL_EV_GPT3B_CMP                             0x00000044
#define EVENT_GPT3BCAPTSEL_EV_GPT3A_CMP                             0x00000043
#define EVENT_GPT3BCAPTSEL_EV_GPT2B_CMP                             0x00000042
#define EVENT_GPT3BCAPTSEL_EV_GPT2A_CMP                             0x00000041
#define EVENT_GPT3BCAPTSEL_EV_GPT1B_CMP                             0x00000040
#define EVENT_GPT3BCAPTSEL_EV_GPT1A_CMP                             0x0000003F
#define EVENT_GPT3BCAPTSEL_EV_GPT0B_CMP                             0x0000003E
#define EVENT_GPT3BCAPTSEL_EV_GPT0A_CMP                             0x0000003D
#define EVENT_GPT3BCAPTSEL_EV_AUX_TIMER2_PULSE                      0x0000003C
#define EVENT_GPT3BCAPTSEL_EV_AUX_TIMER2_EV3                        0x0000003B
#define EVENT_GPT3BCAPTSEL_EV_AUX_TIMER2_EV2                        0x0000003A
#define EVENT_GPT3BCAPTSEL_EV_AUX_TIMER2_EV1                        0x00000039
#define EVENT_GPT3BCAPTSEL_EV_AUX_TIMER2_EV0                        0x00000038
#define EVENT_GPT3BCAPTSEL_EV_UART1_COMB                            0x00000025
#define EVENT_GPT3BCAPTSEL_EV_UART0_COMB                            0x00000024
#define EVENT_GPT3BCAPTSEL_EV_SSI1_COMB                             0x00000023
#define EVENT_GPT3BCAPTSEL_EV_SSI0_COMB                             0x00000022
#define EVENT_GPT3BCAPTSEL_EV_RFC_CPE_1                             0x0000001E
#define EVENT_GPT3BCAPTSEL_EV_RFC_CPE_0                             0x0000001B
#define EVENT_GPT3BCAPTSEL_EV_RFC_HW_COMB                           0x0000001A
#define EVENT_GPT3BCAPTSEL_EV_RFC_CMD_ACK                           0x00000019
#define EVENT_GPT3BCAPTSEL_EV_FLASH                                 0x00000015
#define EVENT_GPT3BCAPTSEL_EV_AUX_COMB                              0x0000000B
#define EVENT_GPT3BCAPTSEL_EV_I2C_IRQ                               0x00000009
#define EVENT_GPT3BCAPTSEL_EV_AON_RTC_COMB                          0x00000007
#define EVENT_GPT3BCAPTSEL_EV_OSC_COMB                              0x00000006
#define EVENT_GPT3BCAPTSEL_EV_BATMON_COMB                           0x00000005
#define EVENT_GPT3BCAPTSEL_EV_AON_GPIO_EDGE                         0x00000004
#define EVENT_GPT3BCAPTSEL_EV_NONE                                  0x00000000

//*****************************************************************************
//
// Register: EVENT_O_AUXSEL0
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// GPT3B_CMP                GPT3B compare event. Configured by GPT3:TBMR.TCACT
// GPT3A_CMP                GPT3A compare event. Configured by GPT3:TAMR.TCACT
// GPT2B_CMP                GPT2B compare event. Configured by GPT2:TBMR.TCACT
// GPT2A_CMP                GPT2A compare event. Configured by GPT2:TAMR.TCACT
// GPT1B_CMP                GPT1B compare event. Configured by GPT1:TBMR.TCACT
// GPT1A_CMP                GPT1A compare event. Configured by GPT1:TAMR.TCACT
// GPT0B_CMP                GPT0B compare event. Configured by GPT0:TBMR.TCACT
// GPT0A_CMP                GPT0A compare event. Configured by GPT0:TAMR.TCACT
// GPT1B                    GPT1B interrupt event, controlled by GPT1:TBMR
// GPT1A                    GPT1A interrupt event, controlled by GPT1:TAMR
// GPT0B                    GPT0B interrupt event, controlled by GPT0:TBMR
// GPT0A                    GPT0A interrupt event, controlled by GPT0:TAMR
// GPT3B                    GPT3B interrupt event, controlled by GPT3:TBMR
// GPT3A                    GPT3A interrupt event, controlled by GPT3:TAMR
// GPT2B                    GPT2B interrupt event, controlled by GPT2:TBMR
// GPT2A                    GPT2A interrupt event, controlled by GPT2:TAMR
// NONE                     Always inactive
#define EVENT_AUXSEL0_EV_W                                                   7
#define EVENT_AUXSEL0_EV_M                                          0x0000007F
#define EVENT_AUXSEL0_EV_S                                                   0
#define EVENT_AUXSEL0_EV_ALWAYS_ACTIVE                              0x00000079
#define EVENT_AUXSEL0_EV_GPT3B_CMP                                  0x00000044
#define EVENT_AUXSEL0_EV_GPT3A_CMP                                  0x00000043
#define EVENT_AUXSEL0_EV_GPT2B_CMP                                  0x00000042
#define EVENT_AUXSEL0_EV_GPT2A_CMP                                  0x00000041
#define EVENT_AUXSEL0_EV_GPT1B_CMP                                  0x00000040
#define EVENT_AUXSEL0_EV_GPT1A_CMP                                  0x0000003F
#define EVENT_AUXSEL0_EV_GPT0B_CMP                                  0x0000003E
#define EVENT_AUXSEL0_EV_GPT0A_CMP                                  0x0000003D
#define EVENT_AUXSEL0_EV_GPT1B                                      0x00000013
#define EVENT_AUXSEL0_EV_GPT1A                                      0x00000012
#define EVENT_AUXSEL0_EV_GPT0B                                      0x00000011
#define EVENT_AUXSEL0_EV_GPT0A                                      0x00000010
#define EVENT_AUXSEL0_EV_GPT3B                                      0x0000000F
#define EVENT_AUXSEL0_EV_GPT3A                                      0x0000000E
#define EVENT_AUXSEL0_EV_GPT2B                                      0x0000000D
#define EVENT_AUXSEL0_EV_GPT2A                                      0x0000000C
#define EVENT_AUXSEL0_EV_NONE                                       0x00000000

//*****************************************************************************
//
// Register: EVENT_O_CM3NMISEL0
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read only selection value
// ENUMs:
// WDT_NMI                  Watchdog non maskable interrupt event, controlled
//                          by WDT:CTL.INTTYPE
#define EVENT_CM3NMISEL0_EV_W                                                7
#define EVENT_CM3NMISEL0_EV_M                                       0x0000007F
#define EVENT_CM3NMISEL0_EV_S                                                0
#define EVENT_CM3NMISEL0_EV_WDT_NMI                                 0x00000063

//*****************************************************************************
//
// Register: EVENT_O_I2SSTMPSEL0
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// NONE                     Always inactive
#define EVENT_I2SSTMPSEL0_EV_W                                               7
#define EVENT_I2SSTMPSEL0_EV_M                                      0x0000007F
#define EVENT_I2SSTMPSEL0_EV_S                                               0
#define EVENT_I2SSTMPSEL0_EV_ALWAYS_ACTIVE                          0x00000079
#define EVENT_I2SSTMPSEL0_EV_NONE                                   0x00000000

//*****************************************************************************
//
// Register: EVENT_O_FRZSEL0
//
//*****************************************************************************
// Field:   [6:0] EV
//
// Read/write selection value
//
// Writing any other value than values defined by a ENUM may result in
// undefined behavior.
// ENUMs:
// ALWAYS_ACTIVE            Always asserted
// CPU_HALTED               CPU halted
// NONE                     Always inactive
#define EVENT_FRZSEL0_EV_W                                                   7
#define EVENT_FRZSEL0_EV_M                                          0x0000007F
#define EVENT_FRZSEL0_EV_S                                                   0
#define EVENT_FRZSEL0_EV_ALWAYS_ACTIVE                              0x00000079
#define EVENT_FRZSEL0_EV_CPU_HALTED                                 0x00000078
#define EVENT_FRZSEL0_EV_NONE                                       0x00000000

//*****************************************************************************
//
// Register: EVENT_O_SWEV
//
//*****************************************************************************
// Field:    [24] SWEV3
//
// Writing "1" to this bit when the value is "0" triggers the Software 3 event.
#define EVENT_SWEV_SWEV3                                            0x01000000
#define EVENT_SWEV_SWEV3_BITN                                               24
#define EVENT_SWEV_SWEV3_M                                          0x01000000
#define EVENT_SWEV_SWEV3_S                                                  24

// Field:    [16] SWEV2
//
// Writing "1" to this bit when the value is "0" triggers the Software 2 event.
#define EVENT_SWEV_SWEV2                                            0x00010000
#define EVENT_SWEV_SWEV2_BITN                                               16
#define EVENT_SWEV_SWEV2_M                                          0x00010000
#define EVENT_SWEV_SWEV2_S                                                  16

// Field:     [8] SWEV1
//
// Writing "1" to this bit when the value is "0" triggers the Software 1 event.
#define EVENT_SWEV_SWEV1                                            0x00000100
#define EVENT_SWEV_SWEV1_BITN                                                8
#define EVENT_SWEV_SWEV1_M                                          0x00000100
#define EVENT_SWEV_SWEV1_S                                                   8

// Field:     [0] SWEV0
//
// Writing "1" to this bit when the value is "0" triggers the Software 0 event.
#define EVENT_SWEV_SWEV0                                            0x00000001
#define EVENT_SWEV_SWEV0_BITN                                                0
#define EVENT_SWEV_SWEV0_M                                          0x00000001
#define EVENT_SWEV_SWEV0_S                                                   0


#endif // __EVENT__
