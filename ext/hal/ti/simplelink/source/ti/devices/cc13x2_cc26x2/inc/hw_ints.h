/******************************************************************************
*  Filename:       hw_ints_h
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

#ifndef __HW_INTS_H__
#define __HW_INTS_H__

//*****************************************************************************
//
// The following are defines for the interrupt assignments.
//
//*****************************************************************************
#define INT_NMI_FAULT                    2 // NMI Fault
#define INT_HARD_FAULT                   3 // Hard Fault
#define INT_MEMMANAGE_FAULT              4 // Memory Management (MemManage)
                                           // Fault
#define INT_BUS_FAULT                    5 // Bus Fault
#define INT_USAGE_FAULT                  6 // Usage Fault
#define INT_SVCALL                      11 // Supervisor Call (SVCall)
#define INT_DEBUG                       12 // Debug Monitor
#define INT_PENDSV                      14 // Pending Service Call (PendSV)
#define INT_SYSTICK                     15 // SysTick Interrupt from the
                                           // System Timer in NVIC.
#define INT_AON_GPIO_EDGE               16 // Edge detect event from IOC
#define INT_I2C_IRQ                     17 // Interrupt event from I2C
#define INT_RFC_CPE_1                   18 // Combined Interrupt for CPE
                                           // Generated events
#define INT_PKA_IRQ                     19 // PKA Interrupt event
#define INT_AON_RTC_COMB                20 // Event from AON_RTC
#define INT_UART0_COMB                  21 // UART0 combined interrupt
#define INT_AUX_SWEV0                   22 // AUX software event 0
#define INT_SSI0_COMB                   23 // SSI0 combined interrupt
#define INT_SSI1_COMB                   24 // SSI1 combined interrupt
#define INT_RFC_CPE_0                   25 // Combined Interrupt for CPE
                                           // Generated events
#define INT_RFC_HW_COMB                 26 // Combined RFC hardware interrupt
#define INT_RFC_CMD_ACK                 27 // RFC Doorbell Command
                                           // Acknowledgement Interrupt
#define INT_I2S_IRQ                     28 // Interrupt event from I2S
#define INT_AUX_SWEV1                   29 // AUX software event 1
#define INT_WDT_IRQ                     30 // Watchdog interrupt event
#define INT_GPT0A                       31 // GPT0A interrupt event
#define INT_GPT0B                       32 // GPT0B interrupt event
#define INT_GPT1A                       33 // GPT1A interrupt event
#define INT_GPT1B                       34 // GPT1B interrupt event
#define INT_GPT2A                       35 // GPT2A interrupt event
#define INT_GPT2B                       36 // GPT2B interrupt event
#define INT_GPT3A                       37 // GPT3A interrupt event
#define INT_GPT3B                       38 // GPT3B interrupt event
#define INT_CRYPTO_RESULT_AVAIL_IRQ     39 // CRYPTO result available interupt
                                           // event
#define INT_DMA_DONE_COMB               40 // Combined DMA done
#define INT_DMA_ERR                     41 // DMA bus error
#define INT_FLASH                       42 // FLASH controller error event
#define INT_SWEV0                       43 // Software event 0
#define INT_AUX_COMB                    44 // AUX combined event
#define INT_AON_PROG0                   45 // AON programmable event 0
#define INT_PROG0                       46 // Programmable Interrupt 0
#define INT_AUX_COMPA                   47 // AUX Compare A event
#define INT_AUX_ADC_IRQ                 48 // AUX ADC interrupt event
#define INT_TRNG_IRQ                    49 // TRNG Interrupt event
#define INT_OSC_COMB                    50 // Combined event from Oscillator
                                           // control
#define INT_AUX_TIMER2_EV0              51 // AUX Timer2 event 0
#define INT_UART1_COMB                  52 // UART1 combined interrupt
#define INT_BATMON_COMB                 53 // Combined event from battery
                                           // monitor

//*****************************************************************************
//
// The following are defines for number of interrupts and priority levels.
//
//*****************************************************************************
#define NUM_INTERRUPTS                  54 // Number of interrupts
#define NUM_PRIORITY_BITS                3 // Number of Priority bits
#define NUM_PRIORITY                     8 // Number of priority levels


//*****************************************************************************
//
// Aliases for backwards compatibility with Sensor Controller Studio 1.1.0
//
//*****************************************************************************

#define INT_AON_AUX_SWEV0    INT_AUX_SWEV0
#define INT_AON_AUX_SWEV1    INT_AUX_SWEV1

#endif
