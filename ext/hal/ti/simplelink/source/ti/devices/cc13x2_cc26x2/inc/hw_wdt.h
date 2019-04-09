/******************************************************************************
*  Filename:       hw_wdt_h
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

#ifndef __HW_WDT_H__
#define __HW_WDT_H__

//*****************************************************************************
//
// This section defines the register offsets of
// WDT component
//
//*****************************************************************************
// Configuration
#define WDT_O_LOAD                                                  0x00000000

// Current Count Value
#define WDT_O_VALUE                                                 0x00000004

// Control
#define WDT_O_CTL                                                   0x00000008

// Interrupt Clear
#define WDT_O_ICR                                                   0x0000000C

// Raw Interrupt Status
#define WDT_O_RIS                                                   0x00000010

// Masked Interrupt Status
#define WDT_O_MIS                                                   0x00000014

// Test Mode
#define WDT_O_TEST                                                  0x00000418

// Interrupt Cause Test Mode
#define WDT_O_INT_CAUS                                              0x0000041C

// Lock
#define WDT_O_LOCK                                                  0x00000C00

//*****************************************************************************
//
// Register: WDT_O_LOAD
//
//*****************************************************************************
// Field:  [31:0] WDTLOAD
//
// This register is the 32-bit interval value used by the 32-bit counter. When
// this register is written, the value is immediately loaded and the counter is
// restarted to count down from the new value. If this register is loaded with
// 0x0000.0000, an interrupt is immediately generated.
#define WDT_LOAD_WDTLOAD_W                                                  32
#define WDT_LOAD_WDTLOAD_M                                          0xFFFFFFFF
#define WDT_LOAD_WDTLOAD_S                                                   0

//*****************************************************************************
//
// Register: WDT_O_VALUE
//
//*****************************************************************************
// Field:  [31:0] WDTVALUE
//
//  This register contains the current count value of the timer.
#define WDT_VALUE_WDTVALUE_W                                                32
#define WDT_VALUE_WDTVALUE_M                                        0xFFFFFFFF
#define WDT_VALUE_WDTVALUE_S                                                 0

//*****************************************************************************
//
// Register: WDT_O_CTL
//
//*****************************************************************************
// Field:     [2] INTTYPE
//
// WDT Interrupt Type
//
// 0:  WDT interrupt is a standard interrupt.
// 1:  WDT interrupt is a non-maskable interrupt.
// ENUMs:
// NONMASKABLE              Non-maskable interrupt
// MASKABLE                 Maskable interrupt
#define WDT_CTL_INTTYPE                                             0x00000004
#define WDT_CTL_INTTYPE_BITN                                                 2
#define WDT_CTL_INTTYPE_M                                           0x00000004
#define WDT_CTL_INTTYPE_S                                                    2
#define WDT_CTL_INTTYPE_NONMASKABLE                                 0x00000004
#define WDT_CTL_INTTYPE_MASKABLE                                    0x00000000

// Field:     [1] RESEN
//
// WDT Reset Enable. Defines the function of the WDT reset source (see
// PRCM:WARMRESET.WDT_STAT if enabled)
//
// 0:  Disabled.
// 1:  Enable the Watchdog reset output.
// ENUMs:
// EN                       Reset output Enabled
// DIS                      Reset output Disabled
#define WDT_CTL_RESEN                                               0x00000002
#define WDT_CTL_RESEN_BITN                                                   1
#define WDT_CTL_RESEN_M                                             0x00000002
#define WDT_CTL_RESEN_S                                                      1
#define WDT_CTL_RESEN_EN                                            0x00000002
#define WDT_CTL_RESEN_DIS                                           0x00000000

// Field:     [0] INTEN
//
// WDT Interrupt Enable
//
// 0: Interrupt event disabled.
// 1: Interrupt event enabled. Once set, this bit can only be cleared by a
// hardware reset.
// ENUMs:
// EN                       Interrupt Enabled
// DIS                      Interrupt Disabled
#define WDT_CTL_INTEN                                               0x00000001
#define WDT_CTL_INTEN_BITN                                                   0
#define WDT_CTL_INTEN_M                                             0x00000001
#define WDT_CTL_INTEN_S                                                      0
#define WDT_CTL_INTEN_EN                                            0x00000001
#define WDT_CTL_INTEN_DIS                                           0x00000000

//*****************************************************************************
//
// Register: WDT_O_ICR
//
//*****************************************************************************
// Field:  [31:0] WDTICR
//
// This register is the interrupt clear register. A write of any value to this
// register clears the WDT interrupt and reloads the 32-bit counter from the
// LOAD register.
#define WDT_ICR_WDTICR_W                                                    32
#define WDT_ICR_WDTICR_M                                            0xFFFFFFFF
#define WDT_ICR_WDTICR_S                                                     0

//*****************************************************************************
//
// Register: WDT_O_RIS
//
//*****************************************************************************
// Field:     [0] WDTRIS
//
// This register is the raw interrupt status register. WDT interrupt events can
// be monitored via this register if the controller interrupt is masked.
//
// Value Description
//
// 0: The WDT has not timed out
// 1: A WDT time-out event has occurred
//
#define WDT_RIS_WDTRIS                                              0x00000001
#define WDT_RIS_WDTRIS_BITN                                                  0
#define WDT_RIS_WDTRIS_M                                            0x00000001
#define WDT_RIS_WDTRIS_S                                                     0

//*****************************************************************************
//
// Register: WDT_O_MIS
//
//*****************************************************************************
// Field:     [0] WDTMIS
//
// This register is the masked interrupt status register. The value of this
// register is the logical AND of the raw interrupt bit and the WDT interrupt
// enable bit CTL.INTEN.
//
// Value Description
//
// 0: The WDT has not timed out or is masked.
// 1: An unmasked WDT time-out event has occurred.
#define WDT_MIS_WDTMIS                                              0x00000001
#define WDT_MIS_WDTMIS_BITN                                                  0
#define WDT_MIS_WDTMIS_M                                            0x00000001
#define WDT_MIS_WDTMIS_S                                                     0

//*****************************************************************************
//
// Register: WDT_O_TEST
//
//*****************************************************************************
// Field:     [8] STALL
//
// WDT Stall Enable
//
// 0:  The WDT timer continues counting if the CPU is stopped with a debugger.
// 1:  If the CPU is stopped with a debugger, the WDT stops counting. Once the
// CPU is restarted, the WDT resumes counting.
// ENUMs:
// EN                       Enable STALL
// DIS                      Disable STALL
#define WDT_TEST_STALL                                              0x00000100
#define WDT_TEST_STALL_BITN                                                  8
#define WDT_TEST_STALL_M                                            0x00000100
#define WDT_TEST_STALL_S                                                     8
#define WDT_TEST_STALL_EN                                           0x00000100
#define WDT_TEST_STALL_DIS                                          0x00000000

// Field:     [0] TEST_EN
//
// The test enable bit
//
// 0: Enable external reset
// 1: Disables the generation of an external reset. Instead bit 1 of the
// INT_CAUS register is set and an interrupt is generated
// ENUMs:
// EN                       Test mode Enabled
// DIS                      Test mode Disabled
#define WDT_TEST_TEST_EN                                            0x00000001
#define WDT_TEST_TEST_EN_BITN                                                0
#define WDT_TEST_TEST_EN_M                                          0x00000001
#define WDT_TEST_TEST_EN_S                                                   0
#define WDT_TEST_TEST_EN_EN                                         0x00000001
#define WDT_TEST_TEST_EN_DIS                                        0x00000000

//*****************************************************************************
//
// Register: WDT_O_INT_CAUS
//
//*****************************************************************************
// Field:     [1] CAUSE_RESET
//
// Indicates that the cause of an interrupt was a reset generated but blocked
// due to TEST.TEST_EN (only possible when TEST.TEST_EN is set).
#define WDT_INT_CAUS_CAUSE_RESET                                    0x00000002
#define WDT_INT_CAUS_CAUSE_RESET_BITN                                        1
#define WDT_INT_CAUS_CAUSE_RESET_M                                  0x00000002
#define WDT_INT_CAUS_CAUSE_RESET_S                                           1

// Field:     [0] CAUSE_INTR
//
// Replica of RIS.WDTRIS
#define WDT_INT_CAUS_CAUSE_INTR                                     0x00000001
#define WDT_INT_CAUS_CAUSE_INTR_BITN                                         0
#define WDT_INT_CAUS_CAUSE_INTR_M                                   0x00000001
#define WDT_INT_CAUS_CAUSE_INTR_S                                            0

//*****************************************************************************
//
// Register: WDT_O_LOCK
//
//*****************************************************************************
// Field:  [31:0] WDTLOCK
//
// WDT Lock: A write of the value 0x1ACC.E551 unlocks the watchdog registers
// for write access. A write of any other value reapplies the lock, preventing
// any register updates (NOTE: TEST.TEST_EN bit is not lockable).
//
// A read of this register returns the following values:
//
// 0x0000.0000: Unlocked
// 0x0000.0001:  Locked
#define WDT_LOCK_WDTLOCK_W                                                  32
#define WDT_LOCK_WDTLOCK_M                                          0xFFFFFFFF
#define WDT_LOCK_WDTLOCK_S                                                   0


#endif // __WDT__
