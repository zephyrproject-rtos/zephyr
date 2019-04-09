/******************************************************************************
*  Filename:       hw_cpu_scs_h
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

#ifndef __HW_CPU_SCS_H__
#define __HW_CPU_SCS_H__

//*****************************************************************************
//
// This section defines the register offsets of
// CPU_SCS component
//
//*****************************************************************************
// Interrupt Control Type
#define CPU_SCS_O_ICTR                                              0x00000004

// Auxiliary Control
#define CPU_SCS_O_ACTLR                                             0x00000008

// SysTick Control and Status
#define CPU_SCS_O_STCSR                                             0x00000010

// SysTick Reload Value
#define CPU_SCS_O_STRVR                                             0x00000014

// SysTick Current Value
#define CPU_SCS_O_STCVR                                             0x00000018

// SysTick Calibration Value
#define CPU_SCS_O_STCR                                              0x0000001C

// Irq 0 to 31 Set Enable
#define CPU_SCS_O_NVIC_ISER0                                        0x00000100

// Irq 32 to 63 Set Enable
#define CPU_SCS_O_NVIC_ISER1                                        0x00000104

// Irq 0 to 31 Clear Enable
#define CPU_SCS_O_NVIC_ICER0                                        0x00000180

// Irq 32 to 63 Clear Enable
#define CPU_SCS_O_NVIC_ICER1                                        0x00000184

// Irq 0 to 31 Set Pending
#define CPU_SCS_O_NVIC_ISPR0                                        0x00000200

// Irq 32 to 63 Set Pending
#define CPU_SCS_O_NVIC_ISPR1                                        0x00000204

// Irq 0 to 31 Clear Pending
#define CPU_SCS_O_NVIC_ICPR0                                        0x00000280

// Irq 32 to 63 Clear Pending
#define CPU_SCS_O_NVIC_ICPR1                                        0x00000284

// Irq 0 to 31 Active Bit
#define CPU_SCS_O_NVIC_IABR0                                        0x00000300

// Irq 32 to 63 Active Bit
#define CPU_SCS_O_NVIC_IABR1                                        0x00000304

// Irq 0 to 3 Priority
#define CPU_SCS_O_NVIC_IPR0                                         0x00000400

// Irq 4 to 7 Priority
#define CPU_SCS_O_NVIC_IPR1                                         0x00000404

// Irq 8 to 11 Priority
#define CPU_SCS_O_NVIC_IPR2                                         0x00000408

// Irq 12 to 15 Priority
#define CPU_SCS_O_NVIC_IPR3                                         0x0000040C

// Irq 16 to 19 Priority
#define CPU_SCS_O_NVIC_IPR4                                         0x00000410

// Irq 20 to 23 Priority
#define CPU_SCS_O_NVIC_IPR5                                         0x00000414

// Irq 24 to 27 Priority
#define CPU_SCS_O_NVIC_IPR6                                         0x00000418

// Irq 28 to 31 Priority
#define CPU_SCS_O_NVIC_IPR7                                         0x0000041C

// Irq 32 to 35 Priority
#define CPU_SCS_O_NVIC_IPR8                                         0x00000420

// Irq 32 to 35 Priority
#define CPU_SCS_O_NVIC_IPR9                                         0x00000424

// CPUID Base
#define CPU_SCS_O_CPUID                                             0x00000D00

// Interrupt Control State
#define CPU_SCS_O_ICSR                                              0x00000D04

// Vector Table Offset
#define CPU_SCS_O_VTOR                                              0x00000D08

// Application Interrupt/Reset Control
#define CPU_SCS_O_AIRCR                                             0x00000D0C

// System Control
#define CPU_SCS_O_SCR                                               0x00000D10

// Configuration Control
#define CPU_SCS_O_CCR                                               0x00000D14

// System Handlers 4-7 Priority
#define CPU_SCS_O_SHPR1                                             0x00000D18

// System Handlers 8-11 Priority
#define CPU_SCS_O_SHPR2                                             0x00000D1C

// System Handlers 12-15 Priority
#define CPU_SCS_O_SHPR3                                             0x00000D20

// System Handler Control and State
#define CPU_SCS_O_SHCSR                                             0x00000D24

// Configurable Fault Status
#define CPU_SCS_O_CFSR                                              0x00000D28

// Hard Fault Status
#define CPU_SCS_O_HFSR                                              0x00000D2C

// Debug Fault Status
#define CPU_SCS_O_DFSR                                              0x00000D30

// Mem Manage Fault Address
#define CPU_SCS_O_MMFAR                                             0x00000D34

// Bus Fault Address
#define CPU_SCS_O_BFAR                                              0x00000D38

// Auxiliary Fault Status
#define CPU_SCS_O_AFSR                                              0x00000D3C

// Processor Feature 0
#define CPU_SCS_O_ID_PFR0                                           0x00000D40

// Processor Feature 1
#define CPU_SCS_O_ID_PFR1                                           0x00000D44

// Debug Feature 0
#define CPU_SCS_O_ID_DFR0                                           0x00000D48

// Auxiliary Feature 0
#define CPU_SCS_O_ID_AFR0                                           0x00000D4C

// Memory Model Feature 0
#define CPU_SCS_O_ID_MMFR0                                          0x00000D50

// Memory Model Feature 1
#define CPU_SCS_O_ID_MMFR1                                          0x00000D54

// Memory Model Feature 2
#define CPU_SCS_O_ID_MMFR2                                          0x00000D58

// Memory Model Feature 3
#define CPU_SCS_O_ID_MMFR3                                          0x00000D5C

// ISA Feature 0
#define CPU_SCS_O_ID_ISAR0                                          0x00000D60

// ISA Feature 1
#define CPU_SCS_O_ID_ISAR1                                          0x00000D64

// ISA Feature 2
#define CPU_SCS_O_ID_ISAR2                                          0x00000D68

// ISA Feature 3
#define CPU_SCS_O_ID_ISAR3                                          0x00000D6C

// ISA Feature 4
#define CPU_SCS_O_ID_ISAR4                                          0x00000D70

// Coprocessor Access Control
#define CPU_SCS_O_CPACR                                             0x00000D88

// MPU Type
#define CPU_SCS_O_MPU_TYPE                                          0x00000D90

// MPU Control
#define CPU_SCS_O_MPU_CTRL                                          0x00000D94

// MPU Region Number
#define CPU_SCS_O_MPU_RNR                                           0x00000D98

// MPU Region Base Address
#define CPU_SCS_O_MPU_RBAR                                          0x00000D9C

// MPU Region Attribute and Size
#define CPU_SCS_O_MPU_RASR                                          0x00000DA0

// MPU Alias 1 Region Base Address
#define CPU_SCS_O_MPU_RBAR_A1                                       0x00000DA4

// MPU Alias 1 Region Attribute and Size
#define CPU_SCS_O_MPU_RASR_A1                                       0x00000DA8

// MPU Alias 2 Region Base Address
#define CPU_SCS_O_MPU_RBAR_A2                                       0x00000DAC

// MPU Alias 2 Region Attribute and Size
#define CPU_SCS_O_MPU_RASR_A2                                       0x00000DB0

// MPU Alias 3 Region Base Address
#define CPU_SCS_O_MPU_RBAR_A3                                       0x00000DB4

// MPU Alias 3 Region Attribute and Size
#define CPU_SCS_O_MPU_RASR_A3                                       0x00000DB8

// Debug Halting Control and Status
#define CPU_SCS_O_DHCSR                                             0x00000DF0

// Deubg Core Register Selector
#define CPU_SCS_O_DCRSR                                             0x00000DF4

// Debug Core Register Data
#define CPU_SCS_O_DCRDR                                             0x00000DF8

// Debug Exception and Monitor Control
#define CPU_SCS_O_DEMCR                                             0x00000DFC

// Software Trigger Interrupt
#define CPU_SCS_O_STIR                                              0x00000F00

// Floating Point Context Control
#define CPU_SCS_O_FPCCR                                             0x00000F34

// Floating-Point Context Address
#define CPU_SCS_O_FPCAR                                             0x00000F38

// Floating Point Default Status Control
#define CPU_SCS_O_FPDSCR                                            0x00000F3C

// Media and FP Feature 0
#define CPU_SCS_O_MVFR0                                             0x00000F40

// Media and FP Feature 1
#define CPU_SCS_O_MVFR1                                             0x00000F44

//*****************************************************************************
//
// Register: CPU_SCS_O_ICTR
//
//*****************************************************************************
// Field:   [2:0] INTLINESNUM
//
// Total number of interrupt lines in groups of 32.
//
// 0: 0...32
// 1: 33...64
// 2: 65...96
// 3: 97...128
// 4: 129...160
// 5: 161...192
// 6: 193...224
// 7: 225...256
#define CPU_SCS_ICTR_INTLINESNUM_W                                           3
#define CPU_SCS_ICTR_INTLINESNUM_M                                  0x00000007
#define CPU_SCS_ICTR_INTLINESNUM_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_ACTLR
//
//*****************************************************************************
// Field:     [9] DISOOFP
//
// Disables floating point instructions completing out of order with respect to
// integer instructions.
#define CPU_SCS_ACTLR_DISOOFP                                       0x00000200
#define CPU_SCS_ACTLR_DISOOFP_BITN                                           9
#define CPU_SCS_ACTLR_DISOOFP_M                                     0x00000200
#define CPU_SCS_ACTLR_DISOOFP_S                                              9

// Field:     [8] DISFPCA
//
// Disable automatic update of CONTROL.FPCA
#define CPU_SCS_ACTLR_DISFPCA                                       0x00000100
#define CPU_SCS_ACTLR_DISFPCA_BITN                                           8
#define CPU_SCS_ACTLR_DISFPCA_M                                     0x00000100
#define CPU_SCS_ACTLR_DISFPCA_S                                              8

// Field:     [2] DISFOLD
//
// Disables folding of IT instruction.
#define CPU_SCS_ACTLR_DISFOLD                                       0x00000004
#define CPU_SCS_ACTLR_DISFOLD_BITN                                           2
#define CPU_SCS_ACTLR_DISFOLD_M                                     0x00000004
#define CPU_SCS_ACTLR_DISFOLD_S                                              2

// Field:     [1] DISDEFWBUF
//
// Disables write buffer use during default memory map accesses. This causes
// all bus faults to be precise bus faults but decreases the performance of the
// processor because the stores to memory have to complete before the next
// instruction can be executed.
#define CPU_SCS_ACTLR_DISDEFWBUF                                    0x00000002
#define CPU_SCS_ACTLR_DISDEFWBUF_BITN                                        1
#define CPU_SCS_ACTLR_DISDEFWBUF_M                                  0x00000002
#define CPU_SCS_ACTLR_DISDEFWBUF_S                                           1

// Field:     [0] DISMCYCINT
//
// Disables interruption of multi-cycle instructions. This increases the
// interrupt latency of the processor becuase LDM/STM completes before
// interrupt stacking occurs.
#define CPU_SCS_ACTLR_DISMCYCINT                                    0x00000001
#define CPU_SCS_ACTLR_DISMCYCINT_BITN                                        0
#define CPU_SCS_ACTLR_DISMCYCINT_M                                  0x00000001
#define CPU_SCS_ACTLR_DISMCYCINT_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_STCSR
//
//*****************************************************************************
// Field:    [16] COUNTFLAG
//
// Returns 1 if timer counted to 0 since last time this was read. Clears on
// read by application of any part of the SysTick Control and Status Register.
// If read by the debugger using the DAP, this bit is cleared on read-only if
// the MasterType bit in the **AHB-AP** Control Register is set to 0.
// Otherwise, COUNTFLAG is not changed by the debugger read.
#define CPU_SCS_STCSR_COUNTFLAG                                     0x00010000
#define CPU_SCS_STCSR_COUNTFLAG_BITN                                        16
#define CPU_SCS_STCSR_COUNTFLAG_M                                   0x00010000
#define CPU_SCS_STCSR_COUNTFLAG_S                                           16

// Field:     [2] CLKSOURCE
//
// Clock source:
//
// 0: External reference clock.
// 1: Core clock
//
// External clock is not available in this device. Writes to this field will be
// ignored.
#define CPU_SCS_STCSR_CLKSOURCE                                     0x00000004
#define CPU_SCS_STCSR_CLKSOURCE_BITN                                         2
#define CPU_SCS_STCSR_CLKSOURCE_M                                   0x00000004
#define CPU_SCS_STCSR_CLKSOURCE_S                                            2

// Field:     [1] TICKINT
//
// 0: Counting down to zero does not pend the SysTick handler. Software can use
// COUNTFLAG to determine if the SysTick handler has ever counted to zero.
// 1: Counting down to zero pends the SysTick handler.
#define CPU_SCS_STCSR_TICKINT                                       0x00000002
#define CPU_SCS_STCSR_TICKINT_BITN                                           1
#define CPU_SCS_STCSR_TICKINT_M                                     0x00000002
#define CPU_SCS_STCSR_TICKINT_S                                              1

// Field:     [0] ENABLE
//
// Enable SysTick counter
//
// 0: Counter disabled
// 1: Counter operates in a multi-shot way. That is, counter loads with the
// Reload value STRVR.RELOAD and then begins counting down. On reaching 0, it
// sets COUNTFLAG to 1 and optionally pends the SysTick handler, based on
// TICKINT. It then loads STRVR.RELOAD again, and begins counting.
#define CPU_SCS_STCSR_ENABLE                                        0x00000001
#define CPU_SCS_STCSR_ENABLE_BITN                                            0
#define CPU_SCS_STCSR_ENABLE_M                                      0x00000001
#define CPU_SCS_STCSR_ENABLE_S                                               0

//*****************************************************************************
//
// Register: CPU_SCS_O_STRVR
//
//*****************************************************************************
// Field:  [23:0] RELOAD
//
// Value to load into the SysTick Current Value Register STCVR.CURRENT when the
// counter reaches 0.
#define CPU_SCS_STRVR_RELOAD_W                                              24
#define CPU_SCS_STRVR_RELOAD_M                                      0x00FFFFFF
#define CPU_SCS_STRVR_RELOAD_S                                               0

//*****************************************************************************
//
// Register: CPU_SCS_O_STCVR
//
//*****************************************************************************
// Field:  [23:0] CURRENT
//
// Current value at the time the register is accessed. No read-modify-write
// protection is provided, so change with care. Writing to it with any value
// clears the register to 0. Clearing this register also clears
// STCSR.COUNTFLAG.
#define CPU_SCS_STCVR_CURRENT_W                                             24
#define CPU_SCS_STCVR_CURRENT_M                                     0x00FFFFFF
#define CPU_SCS_STCVR_CURRENT_S                                              0

//*****************************************************************************
//
// Register: CPU_SCS_O_STCR
//
//*****************************************************************************
// Field:    [31] NOREF
//
// Reads as one. Indicates that no separate reference clock is provided.
#define CPU_SCS_STCR_NOREF                                          0x80000000
#define CPU_SCS_STCR_NOREF_BITN                                             31
#define CPU_SCS_STCR_NOREF_M                                        0x80000000
#define CPU_SCS_STCR_NOREF_S                                                31

// Field:    [30] SKEW
//
// Reads as one. The calibration value is not exactly 10ms because of clock
// frequency. This could affect its suitability as a software real time clock.
#define CPU_SCS_STCR_SKEW                                           0x40000000
#define CPU_SCS_STCR_SKEW_BITN                                              30
#define CPU_SCS_STCR_SKEW_M                                         0x40000000
#define CPU_SCS_STCR_SKEW_S                                                 30

// Field:  [23:0] TENMS
//
// An optional Reload value to be used for 10ms (100Hz) timing, subject to
// system clock skew errors. The value read is valid only when core clock is at
// 48MHz.
#define CPU_SCS_STCR_TENMS_W                                                24
#define CPU_SCS_STCR_TENMS_M                                        0x00FFFFFF
#define CPU_SCS_STCR_TENMS_S                                                 0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_ISER0
//
//*****************************************************************************
// Field:    [31] SETENA31
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 31 (See EVENT:CPUIRQSEL31.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA31                                 0x80000000
#define CPU_SCS_NVIC_ISER0_SETENA31_BITN                                    31
#define CPU_SCS_NVIC_ISER0_SETENA31_M                               0x80000000
#define CPU_SCS_NVIC_ISER0_SETENA31_S                                       31

// Field:    [30] SETENA30
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 30 (See EVENT:CPUIRQSEL30.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA30                                 0x40000000
#define CPU_SCS_NVIC_ISER0_SETENA30_BITN                                    30
#define CPU_SCS_NVIC_ISER0_SETENA30_M                               0x40000000
#define CPU_SCS_NVIC_ISER0_SETENA30_S                                       30

// Field:    [29] SETENA29
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 29 (See EVENT:CPUIRQSEL29.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA29                                 0x20000000
#define CPU_SCS_NVIC_ISER0_SETENA29_BITN                                    29
#define CPU_SCS_NVIC_ISER0_SETENA29_M                               0x20000000
#define CPU_SCS_NVIC_ISER0_SETENA29_S                                       29

// Field:    [28] SETENA28
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 28 (See EVENT:CPUIRQSEL28.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA28                                 0x10000000
#define CPU_SCS_NVIC_ISER0_SETENA28_BITN                                    28
#define CPU_SCS_NVIC_ISER0_SETENA28_M                               0x10000000
#define CPU_SCS_NVIC_ISER0_SETENA28_S                                       28

// Field:    [27] SETENA27
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 27 (See EVENT:CPUIRQSEL27.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA27                                 0x08000000
#define CPU_SCS_NVIC_ISER0_SETENA27_BITN                                    27
#define CPU_SCS_NVIC_ISER0_SETENA27_M                               0x08000000
#define CPU_SCS_NVIC_ISER0_SETENA27_S                                       27

// Field:    [26] SETENA26
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 26 (See EVENT:CPUIRQSEL26.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA26                                 0x04000000
#define CPU_SCS_NVIC_ISER0_SETENA26_BITN                                    26
#define CPU_SCS_NVIC_ISER0_SETENA26_M                               0x04000000
#define CPU_SCS_NVIC_ISER0_SETENA26_S                                       26

// Field:    [25] SETENA25
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 25 (See EVENT:CPUIRQSEL25.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA25                                 0x02000000
#define CPU_SCS_NVIC_ISER0_SETENA25_BITN                                    25
#define CPU_SCS_NVIC_ISER0_SETENA25_M                               0x02000000
#define CPU_SCS_NVIC_ISER0_SETENA25_S                                       25

// Field:    [24] SETENA24
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 24 (See EVENT:CPUIRQSEL24.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA24                                 0x01000000
#define CPU_SCS_NVIC_ISER0_SETENA24_BITN                                    24
#define CPU_SCS_NVIC_ISER0_SETENA24_M                               0x01000000
#define CPU_SCS_NVIC_ISER0_SETENA24_S                                       24

// Field:    [23] SETENA23
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 23 (See EVENT:CPUIRQSEL23.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA23                                 0x00800000
#define CPU_SCS_NVIC_ISER0_SETENA23_BITN                                    23
#define CPU_SCS_NVIC_ISER0_SETENA23_M                               0x00800000
#define CPU_SCS_NVIC_ISER0_SETENA23_S                                       23

// Field:    [22] SETENA22
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 22 (See EVENT:CPUIRQSEL22.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA22                                 0x00400000
#define CPU_SCS_NVIC_ISER0_SETENA22_BITN                                    22
#define CPU_SCS_NVIC_ISER0_SETENA22_M                               0x00400000
#define CPU_SCS_NVIC_ISER0_SETENA22_S                                       22

// Field:    [21] SETENA21
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 21 (See EVENT:CPUIRQSEL21.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA21                                 0x00200000
#define CPU_SCS_NVIC_ISER0_SETENA21_BITN                                    21
#define CPU_SCS_NVIC_ISER0_SETENA21_M                               0x00200000
#define CPU_SCS_NVIC_ISER0_SETENA21_S                                       21

// Field:    [20] SETENA20
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 20 (See EVENT:CPUIRQSEL20.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA20                                 0x00100000
#define CPU_SCS_NVIC_ISER0_SETENA20_BITN                                    20
#define CPU_SCS_NVIC_ISER0_SETENA20_M                               0x00100000
#define CPU_SCS_NVIC_ISER0_SETENA20_S                                       20

// Field:    [19] SETENA19
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 19 (See EVENT:CPUIRQSEL19.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA19                                 0x00080000
#define CPU_SCS_NVIC_ISER0_SETENA19_BITN                                    19
#define CPU_SCS_NVIC_ISER0_SETENA19_M                               0x00080000
#define CPU_SCS_NVIC_ISER0_SETENA19_S                                       19

// Field:    [18] SETENA18
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 18 (See EVENT:CPUIRQSEL18.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA18                                 0x00040000
#define CPU_SCS_NVIC_ISER0_SETENA18_BITN                                    18
#define CPU_SCS_NVIC_ISER0_SETENA18_M                               0x00040000
#define CPU_SCS_NVIC_ISER0_SETENA18_S                                       18

// Field:    [17] SETENA17
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 17 (See EVENT:CPUIRQSEL17.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA17                                 0x00020000
#define CPU_SCS_NVIC_ISER0_SETENA17_BITN                                    17
#define CPU_SCS_NVIC_ISER0_SETENA17_M                               0x00020000
#define CPU_SCS_NVIC_ISER0_SETENA17_S                                       17

// Field:    [16] SETENA16
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 16 (See EVENT:CPUIRQSEL16.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA16                                 0x00010000
#define CPU_SCS_NVIC_ISER0_SETENA16_BITN                                    16
#define CPU_SCS_NVIC_ISER0_SETENA16_M                               0x00010000
#define CPU_SCS_NVIC_ISER0_SETENA16_S                                       16

// Field:    [15] SETENA15
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 15 (See EVENT:CPUIRQSEL15.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA15                                 0x00008000
#define CPU_SCS_NVIC_ISER0_SETENA15_BITN                                    15
#define CPU_SCS_NVIC_ISER0_SETENA15_M                               0x00008000
#define CPU_SCS_NVIC_ISER0_SETENA15_S                                       15

// Field:    [14] SETENA14
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 14 (See EVENT:CPUIRQSEL14.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA14                                 0x00004000
#define CPU_SCS_NVIC_ISER0_SETENA14_BITN                                    14
#define CPU_SCS_NVIC_ISER0_SETENA14_M                               0x00004000
#define CPU_SCS_NVIC_ISER0_SETENA14_S                                       14

// Field:    [13] SETENA13
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 13 (See EVENT:CPUIRQSEL13.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA13                                 0x00002000
#define CPU_SCS_NVIC_ISER0_SETENA13_BITN                                    13
#define CPU_SCS_NVIC_ISER0_SETENA13_M                               0x00002000
#define CPU_SCS_NVIC_ISER0_SETENA13_S                                       13

// Field:    [12] SETENA12
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 12 (See EVENT:CPUIRQSEL12.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA12                                 0x00001000
#define CPU_SCS_NVIC_ISER0_SETENA12_BITN                                    12
#define CPU_SCS_NVIC_ISER0_SETENA12_M                               0x00001000
#define CPU_SCS_NVIC_ISER0_SETENA12_S                                       12

// Field:    [11] SETENA11
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 11 (See EVENT:CPUIRQSEL11.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA11                                 0x00000800
#define CPU_SCS_NVIC_ISER0_SETENA11_BITN                                    11
#define CPU_SCS_NVIC_ISER0_SETENA11_M                               0x00000800
#define CPU_SCS_NVIC_ISER0_SETENA11_S                                       11

// Field:    [10] SETENA10
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 10 (See EVENT:CPUIRQSEL10.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA10                                 0x00000400
#define CPU_SCS_NVIC_ISER0_SETENA10_BITN                                    10
#define CPU_SCS_NVIC_ISER0_SETENA10_M                               0x00000400
#define CPU_SCS_NVIC_ISER0_SETENA10_S                                       10

// Field:     [9] SETENA9
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 9 (See EVENT:CPUIRQSEL9.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA9                                  0x00000200
#define CPU_SCS_NVIC_ISER0_SETENA9_BITN                                      9
#define CPU_SCS_NVIC_ISER0_SETENA9_M                                0x00000200
#define CPU_SCS_NVIC_ISER0_SETENA9_S                                         9

// Field:     [8] SETENA8
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 8 (See EVENT:CPUIRQSEL8.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA8                                  0x00000100
#define CPU_SCS_NVIC_ISER0_SETENA8_BITN                                      8
#define CPU_SCS_NVIC_ISER0_SETENA8_M                                0x00000100
#define CPU_SCS_NVIC_ISER0_SETENA8_S                                         8

// Field:     [7] SETENA7
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 7 (See EVENT:CPUIRQSEL7.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA7                                  0x00000080
#define CPU_SCS_NVIC_ISER0_SETENA7_BITN                                      7
#define CPU_SCS_NVIC_ISER0_SETENA7_M                                0x00000080
#define CPU_SCS_NVIC_ISER0_SETENA7_S                                         7

// Field:     [6] SETENA6
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 6 (See EVENT:CPUIRQSEL6.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA6                                  0x00000040
#define CPU_SCS_NVIC_ISER0_SETENA6_BITN                                      6
#define CPU_SCS_NVIC_ISER0_SETENA6_M                                0x00000040
#define CPU_SCS_NVIC_ISER0_SETENA6_S                                         6

// Field:     [5] SETENA5
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 5 (See EVENT:CPUIRQSEL5.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA5                                  0x00000020
#define CPU_SCS_NVIC_ISER0_SETENA5_BITN                                      5
#define CPU_SCS_NVIC_ISER0_SETENA5_M                                0x00000020
#define CPU_SCS_NVIC_ISER0_SETENA5_S                                         5

// Field:     [4] SETENA4
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 4 (See EVENT:CPUIRQSEL4.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA4                                  0x00000010
#define CPU_SCS_NVIC_ISER0_SETENA4_BITN                                      4
#define CPU_SCS_NVIC_ISER0_SETENA4_M                                0x00000010
#define CPU_SCS_NVIC_ISER0_SETENA4_S                                         4

// Field:     [3] SETENA3
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 3 (See EVENT:CPUIRQSEL3.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA3                                  0x00000008
#define CPU_SCS_NVIC_ISER0_SETENA3_BITN                                      3
#define CPU_SCS_NVIC_ISER0_SETENA3_M                                0x00000008
#define CPU_SCS_NVIC_ISER0_SETENA3_S                                         3

// Field:     [2] SETENA2
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 2 (See EVENT:CPUIRQSEL2.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA2                                  0x00000004
#define CPU_SCS_NVIC_ISER0_SETENA2_BITN                                      2
#define CPU_SCS_NVIC_ISER0_SETENA2_M                                0x00000004
#define CPU_SCS_NVIC_ISER0_SETENA2_S                                         2

// Field:     [1] SETENA1
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 1 (See EVENT:CPUIRQSEL1.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA1                                  0x00000002
#define CPU_SCS_NVIC_ISER0_SETENA1_BITN                                      1
#define CPU_SCS_NVIC_ISER0_SETENA1_M                                0x00000002
#define CPU_SCS_NVIC_ISER0_SETENA1_S                                         1

// Field:     [0] SETENA0
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 0 (See EVENT:CPUIRQSEL0.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER0_SETENA0                                  0x00000001
#define CPU_SCS_NVIC_ISER0_SETENA0_BITN                                      0
#define CPU_SCS_NVIC_ISER0_SETENA0_M                                0x00000001
#define CPU_SCS_NVIC_ISER0_SETENA0_S                                         0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_ISER1
//
//*****************************************************************************
// Field:     [5] SETENA37
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 37 (See EVENT:CPUIRQSEL37.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER1_SETENA37                                 0x00000020
#define CPU_SCS_NVIC_ISER1_SETENA37_BITN                                     5
#define CPU_SCS_NVIC_ISER1_SETENA37_M                               0x00000020
#define CPU_SCS_NVIC_ISER1_SETENA37_S                                        5

// Field:     [4] SETENA36
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 36 (See EVENT:CPUIRQSEL36.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER1_SETENA36                                 0x00000010
#define CPU_SCS_NVIC_ISER1_SETENA36_BITN                                     4
#define CPU_SCS_NVIC_ISER1_SETENA36_M                               0x00000010
#define CPU_SCS_NVIC_ISER1_SETENA36_S                                        4

// Field:     [3] SETENA35
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 35 (See EVENT:CPUIRQSEL35.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER1_SETENA35                                 0x00000008
#define CPU_SCS_NVIC_ISER1_SETENA35_BITN                                     3
#define CPU_SCS_NVIC_ISER1_SETENA35_M                               0x00000008
#define CPU_SCS_NVIC_ISER1_SETENA35_S                                        3

// Field:     [2] SETENA34
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 34 (See EVENT:CPUIRQSEL34.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER1_SETENA34                                 0x00000004
#define CPU_SCS_NVIC_ISER1_SETENA34_BITN                                     2
#define CPU_SCS_NVIC_ISER1_SETENA34_M                               0x00000004
#define CPU_SCS_NVIC_ISER1_SETENA34_S                                        2

// Field:     [1] SETENA33
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 33 (See EVENT:CPUIRQSEL33.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER1_SETENA33                                 0x00000002
#define CPU_SCS_NVIC_ISER1_SETENA33_BITN                                     1
#define CPU_SCS_NVIC_ISER1_SETENA33_M                               0x00000002
#define CPU_SCS_NVIC_ISER1_SETENA33_S                                        1

// Field:     [0] SETENA32
//
// Writing 0 to this bit has no effect, writing 1 to this bit enables the
// interrupt number 32 (See EVENT:CPUIRQSEL32.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ISER1_SETENA32                                 0x00000001
#define CPU_SCS_NVIC_ISER1_SETENA32_BITN                                     0
#define CPU_SCS_NVIC_ISER1_SETENA32_M                               0x00000001
#define CPU_SCS_NVIC_ISER1_SETENA32_S                                        0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_ICER0
//
//*****************************************************************************
// Field:    [31] CLRENA31
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  31 (See EVENT:CPUIRQSEL31.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA31                                 0x80000000
#define CPU_SCS_NVIC_ICER0_CLRENA31_BITN                                    31
#define CPU_SCS_NVIC_ICER0_CLRENA31_M                               0x80000000
#define CPU_SCS_NVIC_ICER0_CLRENA31_S                                       31

// Field:    [30] CLRENA30
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  30 (See EVENT:CPUIRQSEL30.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA30                                 0x40000000
#define CPU_SCS_NVIC_ICER0_CLRENA30_BITN                                    30
#define CPU_SCS_NVIC_ICER0_CLRENA30_M                               0x40000000
#define CPU_SCS_NVIC_ICER0_CLRENA30_S                                       30

// Field:    [29] CLRENA29
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  29 (See EVENT:CPUIRQSEL29.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA29                                 0x20000000
#define CPU_SCS_NVIC_ICER0_CLRENA29_BITN                                    29
#define CPU_SCS_NVIC_ICER0_CLRENA29_M                               0x20000000
#define CPU_SCS_NVIC_ICER0_CLRENA29_S                                       29

// Field:    [28] CLRENA28
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  28 (See EVENT:CPUIRQSEL28.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA28                                 0x10000000
#define CPU_SCS_NVIC_ICER0_CLRENA28_BITN                                    28
#define CPU_SCS_NVIC_ICER0_CLRENA28_M                               0x10000000
#define CPU_SCS_NVIC_ICER0_CLRENA28_S                                       28

// Field:    [27] CLRENA27
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  27 (See EVENT:CPUIRQSEL27.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA27                                 0x08000000
#define CPU_SCS_NVIC_ICER0_CLRENA27_BITN                                    27
#define CPU_SCS_NVIC_ICER0_CLRENA27_M                               0x08000000
#define CPU_SCS_NVIC_ICER0_CLRENA27_S                                       27

// Field:    [26] CLRENA26
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  26 (See EVENT:CPUIRQSEL26.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA26                                 0x04000000
#define CPU_SCS_NVIC_ICER0_CLRENA26_BITN                                    26
#define CPU_SCS_NVIC_ICER0_CLRENA26_M                               0x04000000
#define CPU_SCS_NVIC_ICER0_CLRENA26_S                                       26

// Field:    [25] CLRENA25
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  25 (See EVENT:CPUIRQSEL25.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA25                                 0x02000000
#define CPU_SCS_NVIC_ICER0_CLRENA25_BITN                                    25
#define CPU_SCS_NVIC_ICER0_CLRENA25_M                               0x02000000
#define CPU_SCS_NVIC_ICER0_CLRENA25_S                                       25

// Field:    [24] CLRENA24
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  24 (See EVENT:CPUIRQSEL24.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA24                                 0x01000000
#define CPU_SCS_NVIC_ICER0_CLRENA24_BITN                                    24
#define CPU_SCS_NVIC_ICER0_CLRENA24_M                               0x01000000
#define CPU_SCS_NVIC_ICER0_CLRENA24_S                                       24

// Field:    [23] CLRENA23
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  23 (See EVENT:CPUIRQSEL23.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA23                                 0x00800000
#define CPU_SCS_NVIC_ICER0_CLRENA23_BITN                                    23
#define CPU_SCS_NVIC_ICER0_CLRENA23_M                               0x00800000
#define CPU_SCS_NVIC_ICER0_CLRENA23_S                                       23

// Field:    [22] CLRENA22
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  22 (See EVENT:CPUIRQSEL22.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA22                                 0x00400000
#define CPU_SCS_NVIC_ICER0_CLRENA22_BITN                                    22
#define CPU_SCS_NVIC_ICER0_CLRENA22_M                               0x00400000
#define CPU_SCS_NVIC_ICER0_CLRENA22_S                                       22

// Field:    [21] CLRENA21
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  21 (See EVENT:CPUIRQSEL21.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA21                                 0x00200000
#define CPU_SCS_NVIC_ICER0_CLRENA21_BITN                                    21
#define CPU_SCS_NVIC_ICER0_CLRENA21_M                               0x00200000
#define CPU_SCS_NVIC_ICER0_CLRENA21_S                                       21

// Field:    [20] CLRENA20
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  20 (See EVENT:CPUIRQSEL20.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA20                                 0x00100000
#define CPU_SCS_NVIC_ICER0_CLRENA20_BITN                                    20
#define CPU_SCS_NVIC_ICER0_CLRENA20_M                               0x00100000
#define CPU_SCS_NVIC_ICER0_CLRENA20_S                                       20

// Field:    [19] CLRENA19
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  19 (See EVENT:CPUIRQSEL19.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA19                                 0x00080000
#define CPU_SCS_NVIC_ICER0_CLRENA19_BITN                                    19
#define CPU_SCS_NVIC_ICER0_CLRENA19_M                               0x00080000
#define CPU_SCS_NVIC_ICER0_CLRENA19_S                                       19

// Field:    [18] CLRENA18
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  18 (See EVENT:CPUIRQSEL18.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA18                                 0x00040000
#define CPU_SCS_NVIC_ICER0_CLRENA18_BITN                                    18
#define CPU_SCS_NVIC_ICER0_CLRENA18_M                               0x00040000
#define CPU_SCS_NVIC_ICER0_CLRENA18_S                                       18

// Field:    [17] CLRENA17
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  17 (See EVENT:CPUIRQSEL17.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA17                                 0x00020000
#define CPU_SCS_NVIC_ICER0_CLRENA17_BITN                                    17
#define CPU_SCS_NVIC_ICER0_CLRENA17_M                               0x00020000
#define CPU_SCS_NVIC_ICER0_CLRENA17_S                                       17

// Field:    [16] CLRENA16
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  16 (See EVENT:CPUIRQSEL16.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA16                                 0x00010000
#define CPU_SCS_NVIC_ICER0_CLRENA16_BITN                                    16
#define CPU_SCS_NVIC_ICER0_CLRENA16_M                               0x00010000
#define CPU_SCS_NVIC_ICER0_CLRENA16_S                                       16

// Field:    [15] CLRENA15
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  15 (See EVENT:CPUIRQSEL15.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA15                                 0x00008000
#define CPU_SCS_NVIC_ICER0_CLRENA15_BITN                                    15
#define CPU_SCS_NVIC_ICER0_CLRENA15_M                               0x00008000
#define CPU_SCS_NVIC_ICER0_CLRENA15_S                                       15

// Field:    [14] CLRENA14
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  14 (See EVENT:CPUIRQSEL14.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA14                                 0x00004000
#define CPU_SCS_NVIC_ICER0_CLRENA14_BITN                                    14
#define CPU_SCS_NVIC_ICER0_CLRENA14_M                               0x00004000
#define CPU_SCS_NVIC_ICER0_CLRENA14_S                                       14

// Field:    [13] CLRENA13
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  13 (See EVENT:CPUIRQSEL13.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA13                                 0x00002000
#define CPU_SCS_NVIC_ICER0_CLRENA13_BITN                                    13
#define CPU_SCS_NVIC_ICER0_CLRENA13_M                               0x00002000
#define CPU_SCS_NVIC_ICER0_CLRENA13_S                                       13

// Field:    [12] CLRENA12
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  12 (See EVENT:CPUIRQSEL12.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA12                                 0x00001000
#define CPU_SCS_NVIC_ICER0_CLRENA12_BITN                                    12
#define CPU_SCS_NVIC_ICER0_CLRENA12_M                               0x00001000
#define CPU_SCS_NVIC_ICER0_CLRENA12_S                                       12

// Field:    [11] CLRENA11
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  11 (See EVENT:CPUIRQSEL11.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA11                                 0x00000800
#define CPU_SCS_NVIC_ICER0_CLRENA11_BITN                                    11
#define CPU_SCS_NVIC_ICER0_CLRENA11_M                               0x00000800
#define CPU_SCS_NVIC_ICER0_CLRENA11_S                                       11

// Field:    [10] CLRENA10
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  10 (See EVENT:CPUIRQSEL10.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA10                                 0x00000400
#define CPU_SCS_NVIC_ICER0_CLRENA10_BITN                                    10
#define CPU_SCS_NVIC_ICER0_CLRENA10_M                               0x00000400
#define CPU_SCS_NVIC_ICER0_CLRENA10_S                                       10

// Field:     [9] CLRENA9
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  9 (See EVENT:CPUIRQSEL9.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA9                                  0x00000200
#define CPU_SCS_NVIC_ICER0_CLRENA9_BITN                                      9
#define CPU_SCS_NVIC_ICER0_CLRENA9_M                                0x00000200
#define CPU_SCS_NVIC_ICER0_CLRENA9_S                                         9

// Field:     [8] CLRENA8
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  8 (See EVENT:CPUIRQSEL8.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA8                                  0x00000100
#define CPU_SCS_NVIC_ICER0_CLRENA8_BITN                                      8
#define CPU_SCS_NVIC_ICER0_CLRENA8_M                                0x00000100
#define CPU_SCS_NVIC_ICER0_CLRENA8_S                                         8

// Field:     [7] CLRENA7
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  7 (See EVENT:CPUIRQSEL7.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA7                                  0x00000080
#define CPU_SCS_NVIC_ICER0_CLRENA7_BITN                                      7
#define CPU_SCS_NVIC_ICER0_CLRENA7_M                                0x00000080
#define CPU_SCS_NVIC_ICER0_CLRENA7_S                                         7

// Field:     [6] CLRENA6
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  6 (See EVENT:CPUIRQSEL6.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA6                                  0x00000040
#define CPU_SCS_NVIC_ICER0_CLRENA6_BITN                                      6
#define CPU_SCS_NVIC_ICER0_CLRENA6_M                                0x00000040
#define CPU_SCS_NVIC_ICER0_CLRENA6_S                                         6

// Field:     [5] CLRENA5
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  5 (See EVENT:CPUIRQSEL5.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA5                                  0x00000020
#define CPU_SCS_NVIC_ICER0_CLRENA5_BITN                                      5
#define CPU_SCS_NVIC_ICER0_CLRENA5_M                                0x00000020
#define CPU_SCS_NVIC_ICER0_CLRENA5_S                                         5

// Field:     [4] CLRENA4
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  4 (See EVENT:CPUIRQSEL4.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA4                                  0x00000010
#define CPU_SCS_NVIC_ICER0_CLRENA4_BITN                                      4
#define CPU_SCS_NVIC_ICER0_CLRENA4_M                                0x00000010
#define CPU_SCS_NVIC_ICER0_CLRENA4_S                                         4

// Field:     [3] CLRENA3
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  3 (See EVENT:CPUIRQSEL3.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA3                                  0x00000008
#define CPU_SCS_NVIC_ICER0_CLRENA3_BITN                                      3
#define CPU_SCS_NVIC_ICER0_CLRENA3_M                                0x00000008
#define CPU_SCS_NVIC_ICER0_CLRENA3_S                                         3

// Field:     [2] CLRENA2
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  2 (See EVENT:CPUIRQSEL2.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA2                                  0x00000004
#define CPU_SCS_NVIC_ICER0_CLRENA2_BITN                                      2
#define CPU_SCS_NVIC_ICER0_CLRENA2_M                                0x00000004
#define CPU_SCS_NVIC_ICER0_CLRENA2_S                                         2

// Field:     [1] CLRENA1
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  1 (See EVENT:CPUIRQSEL1.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA1                                  0x00000002
#define CPU_SCS_NVIC_ICER0_CLRENA1_BITN                                      1
#define CPU_SCS_NVIC_ICER0_CLRENA1_M                                0x00000002
#define CPU_SCS_NVIC_ICER0_CLRENA1_S                                         1

// Field:     [0] CLRENA0
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  0 (See EVENT:CPUIRQSEL0.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER0_CLRENA0                                  0x00000001
#define CPU_SCS_NVIC_ICER0_CLRENA0_BITN                                      0
#define CPU_SCS_NVIC_ICER0_CLRENA0_M                                0x00000001
#define CPU_SCS_NVIC_ICER0_CLRENA0_S                                         0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_ICER1
//
//*****************************************************************************
// Field:     [5] CLRENA37
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  37 (See EVENT:CPUIRQSEL37.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER1_CLRENA37                                 0x00000020
#define CPU_SCS_NVIC_ICER1_CLRENA37_BITN                                     5
#define CPU_SCS_NVIC_ICER1_CLRENA37_M                               0x00000020
#define CPU_SCS_NVIC_ICER1_CLRENA37_S                                        5

// Field:     [4] CLRENA36
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  36 (See EVENT:CPUIRQSEL36.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER1_CLRENA36                                 0x00000010
#define CPU_SCS_NVIC_ICER1_CLRENA36_BITN                                     4
#define CPU_SCS_NVIC_ICER1_CLRENA36_M                               0x00000010
#define CPU_SCS_NVIC_ICER1_CLRENA36_S                                        4

// Field:     [3] CLRENA35
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  35 (See EVENT:CPUIRQSEL35.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER1_CLRENA35                                 0x00000008
#define CPU_SCS_NVIC_ICER1_CLRENA35_BITN                                     3
#define CPU_SCS_NVIC_ICER1_CLRENA35_M                               0x00000008
#define CPU_SCS_NVIC_ICER1_CLRENA35_S                                        3

// Field:     [2] CLRENA34
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  34 (See EVENT:CPUIRQSEL34.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER1_CLRENA34                                 0x00000004
#define CPU_SCS_NVIC_ICER1_CLRENA34_BITN                                     2
#define CPU_SCS_NVIC_ICER1_CLRENA34_M                               0x00000004
#define CPU_SCS_NVIC_ICER1_CLRENA34_S                                        2

// Field:     [1] CLRENA33
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  33 (See EVENT:CPUIRQSEL33.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER1_CLRENA33                                 0x00000002
#define CPU_SCS_NVIC_ICER1_CLRENA33_BITN                                     1
#define CPU_SCS_NVIC_ICER1_CLRENA33_M                               0x00000002
#define CPU_SCS_NVIC_ICER1_CLRENA33_S                                        1

// Field:     [0] CLRENA32
//
// Writing 0 to this bit has no effect, writing 1 to this bit disables the
// interrupt number  32 (See EVENT:CPUIRQSEL32.EV for details). Reading the bit
// returns its current enable state.
#define CPU_SCS_NVIC_ICER1_CLRENA32                                 0x00000001
#define CPU_SCS_NVIC_ICER1_CLRENA32_BITN                                     0
#define CPU_SCS_NVIC_ICER1_CLRENA32_M                               0x00000001
#define CPU_SCS_NVIC_ICER1_CLRENA32_S                                        0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_ISPR0
//
//*****************************************************************************
// Field:    [31] SETPEND31
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 31 (See EVENT:CPUIRQSEL31.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND31                                0x80000000
#define CPU_SCS_NVIC_ISPR0_SETPEND31_BITN                                   31
#define CPU_SCS_NVIC_ISPR0_SETPEND31_M                              0x80000000
#define CPU_SCS_NVIC_ISPR0_SETPEND31_S                                      31

// Field:    [30] SETPEND30
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 30 (See EVENT:CPUIRQSEL30.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND30                                0x40000000
#define CPU_SCS_NVIC_ISPR0_SETPEND30_BITN                                   30
#define CPU_SCS_NVIC_ISPR0_SETPEND30_M                              0x40000000
#define CPU_SCS_NVIC_ISPR0_SETPEND30_S                                      30

// Field:    [29] SETPEND29
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 29 (See EVENT:CPUIRQSEL29.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND29                                0x20000000
#define CPU_SCS_NVIC_ISPR0_SETPEND29_BITN                                   29
#define CPU_SCS_NVIC_ISPR0_SETPEND29_M                              0x20000000
#define CPU_SCS_NVIC_ISPR0_SETPEND29_S                                      29

// Field:    [28] SETPEND28
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 28 (See EVENT:CPUIRQSEL28.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND28                                0x10000000
#define CPU_SCS_NVIC_ISPR0_SETPEND28_BITN                                   28
#define CPU_SCS_NVIC_ISPR0_SETPEND28_M                              0x10000000
#define CPU_SCS_NVIC_ISPR0_SETPEND28_S                                      28

// Field:    [27] SETPEND27
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 27 (See EVENT:CPUIRQSEL27.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND27                                0x08000000
#define CPU_SCS_NVIC_ISPR0_SETPEND27_BITN                                   27
#define CPU_SCS_NVIC_ISPR0_SETPEND27_M                              0x08000000
#define CPU_SCS_NVIC_ISPR0_SETPEND27_S                                      27

// Field:    [26] SETPEND26
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 26 (See EVENT:CPUIRQSEL26.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND26                                0x04000000
#define CPU_SCS_NVIC_ISPR0_SETPEND26_BITN                                   26
#define CPU_SCS_NVIC_ISPR0_SETPEND26_M                              0x04000000
#define CPU_SCS_NVIC_ISPR0_SETPEND26_S                                      26

// Field:    [25] SETPEND25
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 25 (See EVENT:CPUIRQSEL25.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND25                                0x02000000
#define CPU_SCS_NVIC_ISPR0_SETPEND25_BITN                                   25
#define CPU_SCS_NVIC_ISPR0_SETPEND25_M                              0x02000000
#define CPU_SCS_NVIC_ISPR0_SETPEND25_S                                      25

// Field:    [24] SETPEND24
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 24 (See EVENT:CPUIRQSEL24.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND24                                0x01000000
#define CPU_SCS_NVIC_ISPR0_SETPEND24_BITN                                   24
#define CPU_SCS_NVIC_ISPR0_SETPEND24_M                              0x01000000
#define CPU_SCS_NVIC_ISPR0_SETPEND24_S                                      24

// Field:    [23] SETPEND23
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 23 (See EVENT:CPUIRQSEL23.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND23                                0x00800000
#define CPU_SCS_NVIC_ISPR0_SETPEND23_BITN                                   23
#define CPU_SCS_NVIC_ISPR0_SETPEND23_M                              0x00800000
#define CPU_SCS_NVIC_ISPR0_SETPEND23_S                                      23

// Field:    [22] SETPEND22
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 22 (See EVENT:CPUIRQSEL22.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND22                                0x00400000
#define CPU_SCS_NVIC_ISPR0_SETPEND22_BITN                                   22
#define CPU_SCS_NVIC_ISPR0_SETPEND22_M                              0x00400000
#define CPU_SCS_NVIC_ISPR0_SETPEND22_S                                      22

// Field:    [21] SETPEND21
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 21 (See EVENT:CPUIRQSEL21.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND21                                0x00200000
#define CPU_SCS_NVIC_ISPR0_SETPEND21_BITN                                   21
#define CPU_SCS_NVIC_ISPR0_SETPEND21_M                              0x00200000
#define CPU_SCS_NVIC_ISPR0_SETPEND21_S                                      21

// Field:    [20] SETPEND20
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 20 (See EVENT:CPUIRQSEL20.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND20                                0x00100000
#define CPU_SCS_NVIC_ISPR0_SETPEND20_BITN                                   20
#define CPU_SCS_NVIC_ISPR0_SETPEND20_M                              0x00100000
#define CPU_SCS_NVIC_ISPR0_SETPEND20_S                                      20

// Field:    [19] SETPEND19
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 19 (See EVENT:CPUIRQSEL19.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND19                                0x00080000
#define CPU_SCS_NVIC_ISPR0_SETPEND19_BITN                                   19
#define CPU_SCS_NVIC_ISPR0_SETPEND19_M                              0x00080000
#define CPU_SCS_NVIC_ISPR0_SETPEND19_S                                      19

// Field:    [18] SETPEND18
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 18 (See EVENT:CPUIRQSEL18.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND18                                0x00040000
#define CPU_SCS_NVIC_ISPR0_SETPEND18_BITN                                   18
#define CPU_SCS_NVIC_ISPR0_SETPEND18_M                              0x00040000
#define CPU_SCS_NVIC_ISPR0_SETPEND18_S                                      18

// Field:    [17] SETPEND17
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 17 (See EVENT:CPUIRQSEL17.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND17                                0x00020000
#define CPU_SCS_NVIC_ISPR0_SETPEND17_BITN                                   17
#define CPU_SCS_NVIC_ISPR0_SETPEND17_M                              0x00020000
#define CPU_SCS_NVIC_ISPR0_SETPEND17_S                                      17

// Field:    [16] SETPEND16
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 16 (See EVENT:CPUIRQSEL16.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND16                                0x00010000
#define CPU_SCS_NVIC_ISPR0_SETPEND16_BITN                                   16
#define CPU_SCS_NVIC_ISPR0_SETPEND16_M                              0x00010000
#define CPU_SCS_NVIC_ISPR0_SETPEND16_S                                      16

// Field:    [15] SETPEND15
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 15 (See EVENT:CPUIRQSEL15.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND15                                0x00008000
#define CPU_SCS_NVIC_ISPR0_SETPEND15_BITN                                   15
#define CPU_SCS_NVIC_ISPR0_SETPEND15_M                              0x00008000
#define CPU_SCS_NVIC_ISPR0_SETPEND15_S                                      15

// Field:    [14] SETPEND14
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 14 (See EVENT:CPUIRQSEL14.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND14                                0x00004000
#define CPU_SCS_NVIC_ISPR0_SETPEND14_BITN                                   14
#define CPU_SCS_NVIC_ISPR0_SETPEND14_M                              0x00004000
#define CPU_SCS_NVIC_ISPR0_SETPEND14_S                                      14

// Field:    [13] SETPEND13
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 13 (See EVENT:CPUIRQSEL13.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND13                                0x00002000
#define CPU_SCS_NVIC_ISPR0_SETPEND13_BITN                                   13
#define CPU_SCS_NVIC_ISPR0_SETPEND13_M                              0x00002000
#define CPU_SCS_NVIC_ISPR0_SETPEND13_S                                      13

// Field:    [12] SETPEND12
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 12 (See EVENT:CPUIRQSEL12.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND12                                0x00001000
#define CPU_SCS_NVIC_ISPR0_SETPEND12_BITN                                   12
#define CPU_SCS_NVIC_ISPR0_SETPEND12_M                              0x00001000
#define CPU_SCS_NVIC_ISPR0_SETPEND12_S                                      12

// Field:    [11] SETPEND11
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 11 (See EVENT:CPUIRQSEL11.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND11                                0x00000800
#define CPU_SCS_NVIC_ISPR0_SETPEND11_BITN                                   11
#define CPU_SCS_NVIC_ISPR0_SETPEND11_M                              0x00000800
#define CPU_SCS_NVIC_ISPR0_SETPEND11_S                                      11

// Field:    [10] SETPEND10
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 10 (See EVENT:CPUIRQSEL10.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND10                                0x00000400
#define CPU_SCS_NVIC_ISPR0_SETPEND10_BITN                                   10
#define CPU_SCS_NVIC_ISPR0_SETPEND10_M                              0x00000400
#define CPU_SCS_NVIC_ISPR0_SETPEND10_S                                      10

// Field:     [9] SETPEND9
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 9 (See EVENT:CPUIRQSEL9.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND9                                 0x00000200
#define CPU_SCS_NVIC_ISPR0_SETPEND9_BITN                                     9
#define CPU_SCS_NVIC_ISPR0_SETPEND9_M                               0x00000200
#define CPU_SCS_NVIC_ISPR0_SETPEND9_S                                        9

// Field:     [8] SETPEND8
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 8 (See EVENT:CPUIRQSEL8.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND8                                 0x00000100
#define CPU_SCS_NVIC_ISPR0_SETPEND8_BITN                                     8
#define CPU_SCS_NVIC_ISPR0_SETPEND8_M                               0x00000100
#define CPU_SCS_NVIC_ISPR0_SETPEND8_S                                        8

// Field:     [7] SETPEND7
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 7 (See EVENT:CPUIRQSEL7.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND7                                 0x00000080
#define CPU_SCS_NVIC_ISPR0_SETPEND7_BITN                                     7
#define CPU_SCS_NVIC_ISPR0_SETPEND7_M                               0x00000080
#define CPU_SCS_NVIC_ISPR0_SETPEND7_S                                        7

// Field:     [6] SETPEND6
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 6 (See EVENT:CPUIRQSEL6.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND6                                 0x00000040
#define CPU_SCS_NVIC_ISPR0_SETPEND6_BITN                                     6
#define CPU_SCS_NVIC_ISPR0_SETPEND6_M                               0x00000040
#define CPU_SCS_NVIC_ISPR0_SETPEND6_S                                        6

// Field:     [5] SETPEND5
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 5 (See EVENT:CPUIRQSEL5.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND5                                 0x00000020
#define CPU_SCS_NVIC_ISPR0_SETPEND5_BITN                                     5
#define CPU_SCS_NVIC_ISPR0_SETPEND5_M                               0x00000020
#define CPU_SCS_NVIC_ISPR0_SETPEND5_S                                        5

// Field:     [4] SETPEND4
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 4 (See EVENT:CPUIRQSEL4.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND4                                 0x00000010
#define CPU_SCS_NVIC_ISPR0_SETPEND4_BITN                                     4
#define CPU_SCS_NVIC_ISPR0_SETPEND4_M                               0x00000010
#define CPU_SCS_NVIC_ISPR0_SETPEND4_S                                        4

// Field:     [3] SETPEND3
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 3 (See EVENT:CPUIRQSEL3.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND3                                 0x00000008
#define CPU_SCS_NVIC_ISPR0_SETPEND3_BITN                                     3
#define CPU_SCS_NVIC_ISPR0_SETPEND3_M                               0x00000008
#define CPU_SCS_NVIC_ISPR0_SETPEND3_S                                        3

// Field:     [2] SETPEND2
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 2 (See EVENT:CPUIRQSEL2.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND2                                 0x00000004
#define CPU_SCS_NVIC_ISPR0_SETPEND2_BITN                                     2
#define CPU_SCS_NVIC_ISPR0_SETPEND2_M                               0x00000004
#define CPU_SCS_NVIC_ISPR0_SETPEND2_S                                        2

// Field:     [1] SETPEND1
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 1 (See EVENT:CPUIRQSEL1.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND1                                 0x00000002
#define CPU_SCS_NVIC_ISPR0_SETPEND1_BITN                                     1
#define CPU_SCS_NVIC_ISPR0_SETPEND1_M                               0x00000002
#define CPU_SCS_NVIC_ISPR0_SETPEND1_S                                        1

// Field:     [0] SETPEND0
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 0 (See EVENT:CPUIRQSEL0.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR0_SETPEND0                                 0x00000001
#define CPU_SCS_NVIC_ISPR0_SETPEND0_BITN                                     0
#define CPU_SCS_NVIC_ISPR0_SETPEND0_M                               0x00000001
#define CPU_SCS_NVIC_ISPR0_SETPEND0_S                                        0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_ISPR1
//
//*****************************************************************************
// Field:     [5] SETPEND37
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 37 (See EVENT:CPUIRQSEL37.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR1_SETPEND37                                0x00000020
#define CPU_SCS_NVIC_ISPR1_SETPEND37_BITN                                    5
#define CPU_SCS_NVIC_ISPR1_SETPEND37_M                              0x00000020
#define CPU_SCS_NVIC_ISPR1_SETPEND37_S                                       5

// Field:     [4] SETPEND36
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 36 (See EVENT:CPUIRQSEL36.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR1_SETPEND36                                0x00000010
#define CPU_SCS_NVIC_ISPR1_SETPEND36_BITN                                    4
#define CPU_SCS_NVIC_ISPR1_SETPEND36_M                              0x00000010
#define CPU_SCS_NVIC_ISPR1_SETPEND36_S                                       4

// Field:     [3] SETPEND35
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 35 (See EVENT:CPUIRQSEL35.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR1_SETPEND35                                0x00000008
#define CPU_SCS_NVIC_ISPR1_SETPEND35_BITN                                    3
#define CPU_SCS_NVIC_ISPR1_SETPEND35_M                              0x00000008
#define CPU_SCS_NVIC_ISPR1_SETPEND35_S                                       3

// Field:     [2] SETPEND34
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 34 (See EVENT:CPUIRQSEL34.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR1_SETPEND34                                0x00000004
#define CPU_SCS_NVIC_ISPR1_SETPEND34_BITN                                    2
#define CPU_SCS_NVIC_ISPR1_SETPEND34_M                              0x00000004
#define CPU_SCS_NVIC_ISPR1_SETPEND34_S                                       2

// Field:     [1] SETPEND33
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 33 (See EVENT:CPUIRQSEL33.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR1_SETPEND33                                0x00000002
#define CPU_SCS_NVIC_ISPR1_SETPEND33_BITN                                    1
#define CPU_SCS_NVIC_ISPR1_SETPEND33_M                              0x00000002
#define CPU_SCS_NVIC_ISPR1_SETPEND33_S                                       1

// Field:     [0] SETPEND32
//
// Writing 0 to this bit has no effect, writing 1 to this bit pends the
// interrupt number 32 (See EVENT:CPUIRQSEL32.EV for details). Reading the bit
// returns its current state.
#define CPU_SCS_NVIC_ISPR1_SETPEND32                                0x00000001
#define CPU_SCS_NVIC_ISPR1_SETPEND32_BITN                                    0
#define CPU_SCS_NVIC_ISPR1_SETPEND32_M                              0x00000001
#define CPU_SCS_NVIC_ISPR1_SETPEND32_S                                       0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_ICPR0
//
//*****************************************************************************
// Field:    [31] CLRPEND31
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 31 (See EVENT:CPUIRQSEL31.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND31                                0x80000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND31_BITN                                   31
#define CPU_SCS_NVIC_ICPR0_CLRPEND31_M                              0x80000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND31_S                                      31

// Field:    [30] CLRPEND30
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 30 (See EVENT:CPUIRQSEL30.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND30                                0x40000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND30_BITN                                   30
#define CPU_SCS_NVIC_ICPR0_CLRPEND30_M                              0x40000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND30_S                                      30

// Field:    [29] CLRPEND29
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 29 (See EVENT:CPUIRQSEL29.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND29                                0x20000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND29_BITN                                   29
#define CPU_SCS_NVIC_ICPR0_CLRPEND29_M                              0x20000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND29_S                                      29

// Field:    [28] CLRPEND28
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 28 (See EVENT:CPUIRQSEL28.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND28                                0x10000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND28_BITN                                   28
#define CPU_SCS_NVIC_ICPR0_CLRPEND28_M                              0x10000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND28_S                                      28

// Field:    [27] CLRPEND27
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 27 (See EVENT:CPUIRQSEL27.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND27                                0x08000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND27_BITN                                   27
#define CPU_SCS_NVIC_ICPR0_CLRPEND27_M                              0x08000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND27_S                                      27

// Field:    [26] CLRPEND26
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 26 (See EVENT:CPUIRQSEL26.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND26                                0x04000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND26_BITN                                   26
#define CPU_SCS_NVIC_ICPR0_CLRPEND26_M                              0x04000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND26_S                                      26

// Field:    [25] CLRPEND25
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 25 (See EVENT:CPUIRQSEL25.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND25                                0x02000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND25_BITN                                   25
#define CPU_SCS_NVIC_ICPR0_CLRPEND25_M                              0x02000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND25_S                                      25

// Field:    [24] CLRPEND24
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 24 (See EVENT:CPUIRQSEL24.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND24                                0x01000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND24_BITN                                   24
#define CPU_SCS_NVIC_ICPR0_CLRPEND24_M                              0x01000000
#define CPU_SCS_NVIC_ICPR0_CLRPEND24_S                                      24

// Field:    [23] CLRPEND23
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 23 (See EVENT:CPUIRQSEL23.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND23                                0x00800000
#define CPU_SCS_NVIC_ICPR0_CLRPEND23_BITN                                   23
#define CPU_SCS_NVIC_ICPR0_CLRPEND23_M                              0x00800000
#define CPU_SCS_NVIC_ICPR0_CLRPEND23_S                                      23

// Field:    [22] CLRPEND22
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 22 (See EVENT:CPUIRQSEL22.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND22                                0x00400000
#define CPU_SCS_NVIC_ICPR0_CLRPEND22_BITN                                   22
#define CPU_SCS_NVIC_ICPR0_CLRPEND22_M                              0x00400000
#define CPU_SCS_NVIC_ICPR0_CLRPEND22_S                                      22

// Field:    [21] CLRPEND21
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 21 (See EVENT:CPUIRQSEL21.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND21                                0x00200000
#define CPU_SCS_NVIC_ICPR0_CLRPEND21_BITN                                   21
#define CPU_SCS_NVIC_ICPR0_CLRPEND21_M                              0x00200000
#define CPU_SCS_NVIC_ICPR0_CLRPEND21_S                                      21

// Field:    [20] CLRPEND20
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 20 (See EVENT:CPUIRQSEL20.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND20                                0x00100000
#define CPU_SCS_NVIC_ICPR0_CLRPEND20_BITN                                   20
#define CPU_SCS_NVIC_ICPR0_CLRPEND20_M                              0x00100000
#define CPU_SCS_NVIC_ICPR0_CLRPEND20_S                                      20

// Field:    [19] CLRPEND19
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 19 (See EVENT:CPUIRQSEL19.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND19                                0x00080000
#define CPU_SCS_NVIC_ICPR0_CLRPEND19_BITN                                   19
#define CPU_SCS_NVIC_ICPR0_CLRPEND19_M                              0x00080000
#define CPU_SCS_NVIC_ICPR0_CLRPEND19_S                                      19

// Field:    [18] CLRPEND18
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 18 (See EVENT:CPUIRQSEL18.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND18                                0x00040000
#define CPU_SCS_NVIC_ICPR0_CLRPEND18_BITN                                   18
#define CPU_SCS_NVIC_ICPR0_CLRPEND18_M                              0x00040000
#define CPU_SCS_NVIC_ICPR0_CLRPEND18_S                                      18

// Field:    [17] CLRPEND17
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 17 (See EVENT:CPUIRQSEL17.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND17                                0x00020000
#define CPU_SCS_NVIC_ICPR0_CLRPEND17_BITN                                   17
#define CPU_SCS_NVIC_ICPR0_CLRPEND17_M                              0x00020000
#define CPU_SCS_NVIC_ICPR0_CLRPEND17_S                                      17

// Field:    [16] CLRPEND16
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 16 (See EVENT:CPUIRQSEL16.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND16                                0x00010000
#define CPU_SCS_NVIC_ICPR0_CLRPEND16_BITN                                   16
#define CPU_SCS_NVIC_ICPR0_CLRPEND16_M                              0x00010000
#define CPU_SCS_NVIC_ICPR0_CLRPEND16_S                                      16

// Field:    [15] CLRPEND15
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 15 (See EVENT:CPUIRQSEL15.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND15                                0x00008000
#define CPU_SCS_NVIC_ICPR0_CLRPEND15_BITN                                   15
#define CPU_SCS_NVIC_ICPR0_CLRPEND15_M                              0x00008000
#define CPU_SCS_NVIC_ICPR0_CLRPEND15_S                                      15

// Field:    [14] CLRPEND14
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 14 (See EVENT:CPUIRQSEL14.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND14                                0x00004000
#define CPU_SCS_NVIC_ICPR0_CLRPEND14_BITN                                   14
#define CPU_SCS_NVIC_ICPR0_CLRPEND14_M                              0x00004000
#define CPU_SCS_NVIC_ICPR0_CLRPEND14_S                                      14

// Field:    [13] CLRPEND13
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 13 (See EVENT:CPUIRQSEL13.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND13                                0x00002000
#define CPU_SCS_NVIC_ICPR0_CLRPEND13_BITN                                   13
#define CPU_SCS_NVIC_ICPR0_CLRPEND13_M                              0x00002000
#define CPU_SCS_NVIC_ICPR0_CLRPEND13_S                                      13

// Field:    [12] CLRPEND12
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 12 (See EVENT:CPUIRQSEL12.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND12                                0x00001000
#define CPU_SCS_NVIC_ICPR0_CLRPEND12_BITN                                   12
#define CPU_SCS_NVIC_ICPR0_CLRPEND12_M                              0x00001000
#define CPU_SCS_NVIC_ICPR0_CLRPEND12_S                                      12

// Field:    [11] CLRPEND11
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 11 (See EVENT:CPUIRQSEL11.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND11                                0x00000800
#define CPU_SCS_NVIC_ICPR0_CLRPEND11_BITN                                   11
#define CPU_SCS_NVIC_ICPR0_CLRPEND11_M                              0x00000800
#define CPU_SCS_NVIC_ICPR0_CLRPEND11_S                                      11

// Field:    [10] CLRPEND10
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 10 (See EVENT:CPUIRQSEL10.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND10                                0x00000400
#define CPU_SCS_NVIC_ICPR0_CLRPEND10_BITN                                   10
#define CPU_SCS_NVIC_ICPR0_CLRPEND10_M                              0x00000400
#define CPU_SCS_NVIC_ICPR0_CLRPEND10_S                                      10

// Field:     [9] CLRPEND9
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 9 (See EVENT:CPUIRQSEL9.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND9                                 0x00000200
#define CPU_SCS_NVIC_ICPR0_CLRPEND9_BITN                                     9
#define CPU_SCS_NVIC_ICPR0_CLRPEND9_M                               0x00000200
#define CPU_SCS_NVIC_ICPR0_CLRPEND9_S                                        9

// Field:     [8] CLRPEND8
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 8 (See EVENT:CPUIRQSEL8.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND8                                 0x00000100
#define CPU_SCS_NVIC_ICPR0_CLRPEND8_BITN                                     8
#define CPU_SCS_NVIC_ICPR0_CLRPEND8_M                               0x00000100
#define CPU_SCS_NVIC_ICPR0_CLRPEND8_S                                        8

// Field:     [7] CLRPEND7
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 7 (See EVENT:CPUIRQSEL7.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND7                                 0x00000080
#define CPU_SCS_NVIC_ICPR0_CLRPEND7_BITN                                     7
#define CPU_SCS_NVIC_ICPR0_CLRPEND7_M                               0x00000080
#define CPU_SCS_NVIC_ICPR0_CLRPEND7_S                                        7

// Field:     [6] CLRPEND6
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 6 (See EVENT:CPUIRQSEL6.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND6                                 0x00000040
#define CPU_SCS_NVIC_ICPR0_CLRPEND6_BITN                                     6
#define CPU_SCS_NVIC_ICPR0_CLRPEND6_M                               0x00000040
#define CPU_SCS_NVIC_ICPR0_CLRPEND6_S                                        6

// Field:     [5] CLRPEND5
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 5 (See EVENT:CPUIRQSEL5.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND5                                 0x00000020
#define CPU_SCS_NVIC_ICPR0_CLRPEND5_BITN                                     5
#define CPU_SCS_NVIC_ICPR0_CLRPEND5_M                               0x00000020
#define CPU_SCS_NVIC_ICPR0_CLRPEND5_S                                        5

// Field:     [4] CLRPEND4
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 4 (See EVENT:CPUIRQSEL4.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND4                                 0x00000010
#define CPU_SCS_NVIC_ICPR0_CLRPEND4_BITN                                     4
#define CPU_SCS_NVIC_ICPR0_CLRPEND4_M                               0x00000010
#define CPU_SCS_NVIC_ICPR0_CLRPEND4_S                                        4

// Field:     [3] CLRPEND3
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 3 (See EVENT:CPUIRQSEL3.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND3                                 0x00000008
#define CPU_SCS_NVIC_ICPR0_CLRPEND3_BITN                                     3
#define CPU_SCS_NVIC_ICPR0_CLRPEND3_M                               0x00000008
#define CPU_SCS_NVIC_ICPR0_CLRPEND3_S                                        3

// Field:     [2] CLRPEND2
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 2 (See EVENT:CPUIRQSEL2.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND2                                 0x00000004
#define CPU_SCS_NVIC_ICPR0_CLRPEND2_BITN                                     2
#define CPU_SCS_NVIC_ICPR0_CLRPEND2_M                               0x00000004
#define CPU_SCS_NVIC_ICPR0_CLRPEND2_S                                        2

// Field:     [1] CLRPEND1
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 1 (See EVENT:CPUIRQSEL1.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND1                                 0x00000002
#define CPU_SCS_NVIC_ICPR0_CLRPEND1_BITN                                     1
#define CPU_SCS_NVIC_ICPR0_CLRPEND1_M                               0x00000002
#define CPU_SCS_NVIC_ICPR0_CLRPEND1_S                                        1

// Field:     [0] CLRPEND0
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 0 (See EVENT:CPUIRQSEL0.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR0_CLRPEND0                                 0x00000001
#define CPU_SCS_NVIC_ICPR0_CLRPEND0_BITN                                     0
#define CPU_SCS_NVIC_ICPR0_CLRPEND0_M                               0x00000001
#define CPU_SCS_NVIC_ICPR0_CLRPEND0_S                                        0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_ICPR1
//
//*****************************************************************************
// Field:     [5] CLRPEND37
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 37 (See EVENT:CPUIRQSEL37.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR1_CLRPEND37                                0x00000020
#define CPU_SCS_NVIC_ICPR1_CLRPEND37_BITN                                    5
#define CPU_SCS_NVIC_ICPR1_CLRPEND37_M                              0x00000020
#define CPU_SCS_NVIC_ICPR1_CLRPEND37_S                                       5

// Field:     [4] CLRPEND36
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 36 (See EVENT:CPUIRQSEL36.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR1_CLRPEND36                                0x00000010
#define CPU_SCS_NVIC_ICPR1_CLRPEND36_BITN                                    4
#define CPU_SCS_NVIC_ICPR1_CLRPEND36_M                              0x00000010
#define CPU_SCS_NVIC_ICPR1_CLRPEND36_S                                       4

// Field:     [3] CLRPEND35
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 35 (See EVENT:CPUIRQSEL35.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR1_CLRPEND35                                0x00000008
#define CPU_SCS_NVIC_ICPR1_CLRPEND35_BITN                                    3
#define CPU_SCS_NVIC_ICPR1_CLRPEND35_M                              0x00000008
#define CPU_SCS_NVIC_ICPR1_CLRPEND35_S                                       3

// Field:     [2] CLRPEND34
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 34 (See EVENT:CPUIRQSEL34.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR1_CLRPEND34                                0x00000004
#define CPU_SCS_NVIC_ICPR1_CLRPEND34_BITN                                    2
#define CPU_SCS_NVIC_ICPR1_CLRPEND34_M                              0x00000004
#define CPU_SCS_NVIC_ICPR1_CLRPEND34_S                                       2

// Field:     [1] CLRPEND33
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 33 (See EVENT:CPUIRQSEL33.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR1_CLRPEND33                                0x00000002
#define CPU_SCS_NVIC_ICPR1_CLRPEND33_BITN                                    1
#define CPU_SCS_NVIC_ICPR1_CLRPEND33_M                              0x00000002
#define CPU_SCS_NVIC_ICPR1_CLRPEND33_S                                       1

// Field:     [0] CLRPEND32
//
// Writing 0 to this bit has no effect, writing 1 to this bit clears the
// corresponding pending interrupt 32 (See EVENT:CPUIRQSEL32.EV for details).
// Reading the bit returns its current state.
#define CPU_SCS_NVIC_ICPR1_CLRPEND32                                0x00000001
#define CPU_SCS_NVIC_ICPR1_CLRPEND32_BITN                                    0
#define CPU_SCS_NVIC_ICPR1_CLRPEND32_M                              0x00000001
#define CPU_SCS_NVIC_ICPR1_CLRPEND32_S                                       0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IABR0
//
//*****************************************************************************
// Field:    [31] ACTIVE31
//
// Reading 0 from this bit implies that interrupt line 31 is not active.
// Reading 1 from this bit implies that the interrupt line 31 is active (See
// EVENT:CPUIRQSEL31.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE31                                 0x80000000
#define CPU_SCS_NVIC_IABR0_ACTIVE31_BITN                                    31
#define CPU_SCS_NVIC_IABR0_ACTIVE31_M                               0x80000000
#define CPU_SCS_NVIC_IABR0_ACTIVE31_S                                       31

// Field:    [30] ACTIVE30
//
// Reading 0 from this bit implies that interrupt line 30 is not active.
// Reading 1 from this bit implies that the interrupt line 30 is active (See
// EVENT:CPUIRQSEL30.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE30                                 0x40000000
#define CPU_SCS_NVIC_IABR0_ACTIVE30_BITN                                    30
#define CPU_SCS_NVIC_IABR0_ACTIVE30_M                               0x40000000
#define CPU_SCS_NVIC_IABR0_ACTIVE30_S                                       30

// Field:    [29] ACTIVE29
//
// Reading 0 from this bit implies that interrupt line 29 is not active.
// Reading 1 from this bit implies that the interrupt line 29 is active (See
// EVENT:CPUIRQSEL29.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE29                                 0x20000000
#define CPU_SCS_NVIC_IABR0_ACTIVE29_BITN                                    29
#define CPU_SCS_NVIC_IABR0_ACTIVE29_M                               0x20000000
#define CPU_SCS_NVIC_IABR0_ACTIVE29_S                                       29

// Field:    [28] ACTIVE28
//
// Reading 0 from this bit implies that interrupt line 28 is not active.
// Reading 1 from this bit implies that the interrupt line 28 is active (See
// EVENT:CPUIRQSEL28.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE28                                 0x10000000
#define CPU_SCS_NVIC_IABR0_ACTIVE28_BITN                                    28
#define CPU_SCS_NVIC_IABR0_ACTIVE28_M                               0x10000000
#define CPU_SCS_NVIC_IABR0_ACTIVE28_S                                       28

// Field:    [27] ACTIVE27
//
// Reading 0 from this bit implies that interrupt line 27 is not active.
// Reading 1 from this bit implies that the interrupt line 27 is active (See
// EVENT:CPUIRQSEL27.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE27                                 0x08000000
#define CPU_SCS_NVIC_IABR0_ACTIVE27_BITN                                    27
#define CPU_SCS_NVIC_IABR0_ACTIVE27_M                               0x08000000
#define CPU_SCS_NVIC_IABR0_ACTIVE27_S                                       27

// Field:    [26] ACTIVE26
//
// Reading 0 from this bit implies that interrupt line 26 is not active.
// Reading 1 from this bit implies that the interrupt line 26 is active (See
// EVENT:CPUIRQSEL26.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE26                                 0x04000000
#define CPU_SCS_NVIC_IABR0_ACTIVE26_BITN                                    26
#define CPU_SCS_NVIC_IABR0_ACTIVE26_M                               0x04000000
#define CPU_SCS_NVIC_IABR0_ACTIVE26_S                                       26

// Field:    [25] ACTIVE25
//
// Reading 0 from this bit implies that interrupt line 25 is not active.
// Reading 1 from this bit implies that the interrupt line 25 is active (See
// EVENT:CPUIRQSEL25.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE25                                 0x02000000
#define CPU_SCS_NVIC_IABR0_ACTIVE25_BITN                                    25
#define CPU_SCS_NVIC_IABR0_ACTIVE25_M                               0x02000000
#define CPU_SCS_NVIC_IABR0_ACTIVE25_S                                       25

// Field:    [24] ACTIVE24
//
// Reading 0 from this bit implies that interrupt line 24 is not active.
// Reading 1 from this bit implies that the interrupt line 24 is active (See
// EVENT:CPUIRQSEL24.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE24                                 0x01000000
#define CPU_SCS_NVIC_IABR0_ACTIVE24_BITN                                    24
#define CPU_SCS_NVIC_IABR0_ACTIVE24_M                               0x01000000
#define CPU_SCS_NVIC_IABR0_ACTIVE24_S                                       24

// Field:    [23] ACTIVE23
//
// Reading 0 from this bit implies that interrupt line 23 is not active.
// Reading 1 from this bit implies that the interrupt line 23 is active (See
// EVENT:CPUIRQSEL23.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE23                                 0x00800000
#define CPU_SCS_NVIC_IABR0_ACTIVE23_BITN                                    23
#define CPU_SCS_NVIC_IABR0_ACTIVE23_M                               0x00800000
#define CPU_SCS_NVIC_IABR0_ACTIVE23_S                                       23

// Field:    [22] ACTIVE22
//
// Reading 0 from this bit implies that interrupt line 22 is not active.
// Reading 1 from this bit implies that the interrupt line 22 is active (See
// EVENT:CPUIRQSEL22.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE22                                 0x00400000
#define CPU_SCS_NVIC_IABR0_ACTIVE22_BITN                                    22
#define CPU_SCS_NVIC_IABR0_ACTIVE22_M                               0x00400000
#define CPU_SCS_NVIC_IABR0_ACTIVE22_S                                       22

// Field:    [21] ACTIVE21
//
// Reading 0 from this bit implies that interrupt line 21 is not active.
// Reading 1 from this bit implies that the interrupt line 21 is active (See
// EVENT:CPUIRQSEL21.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE21                                 0x00200000
#define CPU_SCS_NVIC_IABR0_ACTIVE21_BITN                                    21
#define CPU_SCS_NVIC_IABR0_ACTIVE21_M                               0x00200000
#define CPU_SCS_NVIC_IABR0_ACTIVE21_S                                       21

// Field:    [20] ACTIVE20
//
// Reading 0 from this bit implies that interrupt line 20 is not active.
// Reading 1 from this bit implies that the interrupt line 20 is active (See
// EVENT:CPUIRQSEL20.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE20                                 0x00100000
#define CPU_SCS_NVIC_IABR0_ACTIVE20_BITN                                    20
#define CPU_SCS_NVIC_IABR0_ACTIVE20_M                               0x00100000
#define CPU_SCS_NVIC_IABR0_ACTIVE20_S                                       20

// Field:    [19] ACTIVE19
//
// Reading 0 from this bit implies that interrupt line 19 is not active.
// Reading 1 from this bit implies that the interrupt line 19 is active (See
// EVENT:CPUIRQSEL19.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE19                                 0x00080000
#define CPU_SCS_NVIC_IABR0_ACTIVE19_BITN                                    19
#define CPU_SCS_NVIC_IABR0_ACTIVE19_M                               0x00080000
#define CPU_SCS_NVIC_IABR0_ACTIVE19_S                                       19

// Field:    [18] ACTIVE18
//
// Reading 0 from this bit implies that interrupt line 18 is not active.
// Reading 1 from this bit implies that the interrupt line 18 is active (See
// EVENT:CPUIRQSEL18.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE18                                 0x00040000
#define CPU_SCS_NVIC_IABR0_ACTIVE18_BITN                                    18
#define CPU_SCS_NVIC_IABR0_ACTIVE18_M                               0x00040000
#define CPU_SCS_NVIC_IABR0_ACTIVE18_S                                       18

// Field:    [17] ACTIVE17
//
// Reading 0 from this bit implies that interrupt line 17 is not active.
// Reading 1 from this bit implies that the interrupt line 17 is active (See
// EVENT:CPUIRQSEL17.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE17                                 0x00020000
#define CPU_SCS_NVIC_IABR0_ACTIVE17_BITN                                    17
#define CPU_SCS_NVIC_IABR0_ACTIVE17_M                               0x00020000
#define CPU_SCS_NVIC_IABR0_ACTIVE17_S                                       17

// Field:    [16] ACTIVE16
//
// Reading 0 from this bit implies that interrupt line 16 is not active.
// Reading 1 from this bit implies that the interrupt line 16 is active (See
// EVENT:CPUIRQSEL16.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE16                                 0x00010000
#define CPU_SCS_NVIC_IABR0_ACTIVE16_BITN                                    16
#define CPU_SCS_NVIC_IABR0_ACTIVE16_M                               0x00010000
#define CPU_SCS_NVIC_IABR0_ACTIVE16_S                                       16

// Field:    [15] ACTIVE15
//
// Reading 0 from this bit implies that interrupt line 15 is not active.
// Reading 1 from this bit implies that the interrupt line 15 is active (See
// EVENT:CPUIRQSEL15.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE15                                 0x00008000
#define CPU_SCS_NVIC_IABR0_ACTIVE15_BITN                                    15
#define CPU_SCS_NVIC_IABR0_ACTIVE15_M                               0x00008000
#define CPU_SCS_NVIC_IABR0_ACTIVE15_S                                       15

// Field:    [14] ACTIVE14
//
// Reading 0 from this bit implies that interrupt line 14 is not active.
// Reading 1 from this bit implies that the interrupt line 14 is active (See
// EVENT:CPUIRQSEL14.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE14                                 0x00004000
#define CPU_SCS_NVIC_IABR0_ACTIVE14_BITN                                    14
#define CPU_SCS_NVIC_IABR0_ACTIVE14_M                               0x00004000
#define CPU_SCS_NVIC_IABR0_ACTIVE14_S                                       14

// Field:    [13] ACTIVE13
//
// Reading 0 from this bit implies that interrupt line 13 is not active.
// Reading 1 from this bit implies that the interrupt line 13 is active (See
// EVENT:CPUIRQSEL13.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE13                                 0x00002000
#define CPU_SCS_NVIC_IABR0_ACTIVE13_BITN                                    13
#define CPU_SCS_NVIC_IABR0_ACTIVE13_M                               0x00002000
#define CPU_SCS_NVIC_IABR0_ACTIVE13_S                                       13

// Field:    [12] ACTIVE12
//
// Reading 0 from this bit implies that interrupt line 12 is not active.
// Reading 1 from this bit implies that the interrupt line 12 is active (See
// EVENT:CPUIRQSEL12.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE12                                 0x00001000
#define CPU_SCS_NVIC_IABR0_ACTIVE12_BITN                                    12
#define CPU_SCS_NVIC_IABR0_ACTIVE12_M                               0x00001000
#define CPU_SCS_NVIC_IABR0_ACTIVE12_S                                       12

// Field:    [11] ACTIVE11
//
// Reading 0 from this bit implies that interrupt line 11 is not active.
// Reading 1 from this bit implies that the interrupt line 11 is active (See
// EVENT:CPUIRQSEL11.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE11                                 0x00000800
#define CPU_SCS_NVIC_IABR0_ACTIVE11_BITN                                    11
#define CPU_SCS_NVIC_IABR0_ACTIVE11_M                               0x00000800
#define CPU_SCS_NVIC_IABR0_ACTIVE11_S                                       11

// Field:    [10] ACTIVE10
//
// Reading 0 from this bit implies that interrupt line 10 is not active.
// Reading 1 from this bit implies that the interrupt line 10 is active (See
// EVENT:CPUIRQSEL10.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE10                                 0x00000400
#define CPU_SCS_NVIC_IABR0_ACTIVE10_BITN                                    10
#define CPU_SCS_NVIC_IABR0_ACTIVE10_M                               0x00000400
#define CPU_SCS_NVIC_IABR0_ACTIVE10_S                                       10

// Field:     [9] ACTIVE9
//
// Reading 0 from this bit implies that interrupt line 9 is not active. Reading
// 1 from this bit implies that the interrupt line 9 is active (See
// EVENT:CPUIRQSEL9.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE9                                  0x00000200
#define CPU_SCS_NVIC_IABR0_ACTIVE9_BITN                                      9
#define CPU_SCS_NVIC_IABR0_ACTIVE9_M                                0x00000200
#define CPU_SCS_NVIC_IABR0_ACTIVE9_S                                         9

// Field:     [8] ACTIVE8
//
// Reading 0 from this bit implies that interrupt line 8 is not active. Reading
// 1 from this bit implies that the interrupt line 8 is active (See
// EVENT:CPUIRQSEL8.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE8                                  0x00000100
#define CPU_SCS_NVIC_IABR0_ACTIVE8_BITN                                      8
#define CPU_SCS_NVIC_IABR0_ACTIVE8_M                                0x00000100
#define CPU_SCS_NVIC_IABR0_ACTIVE8_S                                         8

// Field:     [7] ACTIVE7
//
// Reading 0 from this bit implies that interrupt line 7 is not active. Reading
// 1 from this bit implies that the interrupt line 7 is active (See
// EVENT:CPUIRQSEL7.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE7                                  0x00000080
#define CPU_SCS_NVIC_IABR0_ACTIVE7_BITN                                      7
#define CPU_SCS_NVIC_IABR0_ACTIVE7_M                                0x00000080
#define CPU_SCS_NVIC_IABR0_ACTIVE7_S                                         7

// Field:     [6] ACTIVE6
//
// Reading 0 from this bit implies that interrupt line 6 is not active. Reading
// 1 from this bit implies that the interrupt line 6 is active (See
// EVENT:CPUIRQSEL6.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE6                                  0x00000040
#define CPU_SCS_NVIC_IABR0_ACTIVE6_BITN                                      6
#define CPU_SCS_NVIC_IABR0_ACTIVE6_M                                0x00000040
#define CPU_SCS_NVIC_IABR0_ACTIVE6_S                                         6

// Field:     [5] ACTIVE5
//
// Reading 0 from this bit implies that interrupt line 5 is not active. Reading
// 1 from this bit implies that the interrupt line 5 is active (See
// EVENT:CPUIRQSEL5.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE5                                  0x00000020
#define CPU_SCS_NVIC_IABR0_ACTIVE5_BITN                                      5
#define CPU_SCS_NVIC_IABR0_ACTIVE5_M                                0x00000020
#define CPU_SCS_NVIC_IABR0_ACTIVE5_S                                         5

// Field:     [4] ACTIVE4
//
// Reading 0 from this bit implies that interrupt line 4 is not active. Reading
// 1 from this bit implies that the interrupt line 4 is active (See
// EVENT:CPUIRQSEL4.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE4                                  0x00000010
#define CPU_SCS_NVIC_IABR0_ACTIVE4_BITN                                      4
#define CPU_SCS_NVIC_IABR0_ACTIVE4_M                                0x00000010
#define CPU_SCS_NVIC_IABR0_ACTIVE4_S                                         4

// Field:     [3] ACTIVE3
//
// Reading 0 from this bit implies that interrupt line 3 is not active. Reading
// 1 from this bit implies that the interrupt line 3 is active (See
// EVENT:CPUIRQSEL3.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE3                                  0x00000008
#define CPU_SCS_NVIC_IABR0_ACTIVE3_BITN                                      3
#define CPU_SCS_NVIC_IABR0_ACTIVE3_M                                0x00000008
#define CPU_SCS_NVIC_IABR0_ACTIVE3_S                                         3

// Field:     [2] ACTIVE2
//
// Reading 0 from this bit implies that interrupt line 2 is not active. Reading
// 1 from this bit implies that the interrupt line 2 is active (See
// EVENT:CPUIRQSEL2.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE2                                  0x00000004
#define CPU_SCS_NVIC_IABR0_ACTIVE2_BITN                                      2
#define CPU_SCS_NVIC_IABR0_ACTIVE2_M                                0x00000004
#define CPU_SCS_NVIC_IABR0_ACTIVE2_S                                         2

// Field:     [1] ACTIVE1
//
// Reading 0 from this bit implies that interrupt line 1 is not active. Reading
// 1 from this bit implies that the interrupt line 1 is active (See
// EVENT:CPUIRQSEL1.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE1                                  0x00000002
#define CPU_SCS_NVIC_IABR0_ACTIVE1_BITN                                      1
#define CPU_SCS_NVIC_IABR0_ACTIVE1_M                                0x00000002
#define CPU_SCS_NVIC_IABR0_ACTIVE1_S                                         1

// Field:     [0] ACTIVE0
//
// Reading 0 from this bit implies that interrupt line 0 is not active. Reading
// 1 from this bit implies that the interrupt line 0 is active (See
// EVENT:CPUIRQSEL0.EV for details).
#define CPU_SCS_NVIC_IABR0_ACTIVE0                                  0x00000001
#define CPU_SCS_NVIC_IABR0_ACTIVE0_BITN                                      0
#define CPU_SCS_NVIC_IABR0_ACTIVE0_M                                0x00000001
#define CPU_SCS_NVIC_IABR0_ACTIVE0_S                                         0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IABR1
//
//*****************************************************************************
// Field:     [5] ACTIVE37
//
// Reading 0 from this bit implies that interrupt line 37 is not active.
// Reading 1 from this bit implies that the interrupt line 37 is active (See
// EVENT:CPUIRQSEL37.EV for details).
#define CPU_SCS_NVIC_IABR1_ACTIVE37                                 0x00000020
#define CPU_SCS_NVIC_IABR1_ACTIVE37_BITN                                     5
#define CPU_SCS_NVIC_IABR1_ACTIVE37_M                               0x00000020
#define CPU_SCS_NVIC_IABR1_ACTIVE37_S                                        5

// Field:     [4] ACTIVE36
//
// Reading 0 from this bit implies that interrupt line 36 is not active.
// Reading 1 from this bit implies that the interrupt line 36 is active (See
// EVENT:CPUIRQSEL36.EV for details).
#define CPU_SCS_NVIC_IABR1_ACTIVE36                                 0x00000010
#define CPU_SCS_NVIC_IABR1_ACTIVE36_BITN                                     4
#define CPU_SCS_NVIC_IABR1_ACTIVE36_M                               0x00000010
#define CPU_SCS_NVIC_IABR1_ACTIVE36_S                                        4

// Field:     [3] ACTIVE35
//
// Reading 0 from this bit implies that interrupt line 35 is not active.
// Reading 1 from this bit implies that the interrupt line 35 is active (See
// EVENT:CPUIRQSEL35.EV for details).
#define CPU_SCS_NVIC_IABR1_ACTIVE35                                 0x00000008
#define CPU_SCS_NVIC_IABR1_ACTIVE35_BITN                                     3
#define CPU_SCS_NVIC_IABR1_ACTIVE35_M                               0x00000008
#define CPU_SCS_NVIC_IABR1_ACTIVE35_S                                        3

// Field:     [2] ACTIVE34
//
// Reading 0 from this bit implies that interrupt line 34 is not active.
// Reading 1 from this bit implies that the interrupt line 34 is active (See
// EVENT:CPUIRQSEL34.EV for details).
#define CPU_SCS_NVIC_IABR1_ACTIVE34                                 0x00000004
#define CPU_SCS_NVIC_IABR1_ACTIVE34_BITN                                     2
#define CPU_SCS_NVIC_IABR1_ACTIVE34_M                               0x00000004
#define CPU_SCS_NVIC_IABR1_ACTIVE34_S                                        2

// Field:     [1] ACTIVE33
//
// Reading 0 from this bit implies that interrupt line 33 is not active.
// Reading 1 from this bit implies that the interrupt line 33 is active (See
// EVENT:CPUIRQSEL33.EV for details).
#define CPU_SCS_NVIC_IABR1_ACTIVE33                                 0x00000002
#define CPU_SCS_NVIC_IABR1_ACTIVE33_BITN                                     1
#define CPU_SCS_NVIC_IABR1_ACTIVE33_M                               0x00000002
#define CPU_SCS_NVIC_IABR1_ACTIVE33_S                                        1

// Field:     [0] ACTIVE32
//
// Reading 0 from this bit implies that interrupt line 32 is not active.
// Reading 1 from this bit implies that the interrupt line 32 is active (See
// EVENT:CPUIRQSEL32.EV for details).
#define CPU_SCS_NVIC_IABR1_ACTIVE32                                 0x00000001
#define CPU_SCS_NVIC_IABR1_ACTIVE32_BITN                                     0
#define CPU_SCS_NVIC_IABR1_ACTIVE32_M                               0x00000001
#define CPU_SCS_NVIC_IABR1_ACTIVE32_S                                        0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR0
//
//*****************************************************************************
// Field: [31:24] PRI_3
//
// Priority of interrupt 3 (See EVENT:CPUIRQSEL3.EV for details).
#define CPU_SCS_NVIC_IPR0_PRI_3_W                                            8
#define CPU_SCS_NVIC_IPR0_PRI_3_M                                   0xFF000000
#define CPU_SCS_NVIC_IPR0_PRI_3_S                                           24

// Field: [23:16] PRI_2
//
// Priority of interrupt 2 (See EVENT:CPUIRQSEL2.EV for details).
#define CPU_SCS_NVIC_IPR0_PRI_2_W                                            8
#define CPU_SCS_NVIC_IPR0_PRI_2_M                                   0x00FF0000
#define CPU_SCS_NVIC_IPR0_PRI_2_S                                           16

// Field:  [15:8] PRI_1
//
// Priority of interrupt 1 (See EVENT:CPUIRQSEL1.EV for details).
#define CPU_SCS_NVIC_IPR0_PRI_1_W                                            8
#define CPU_SCS_NVIC_IPR0_PRI_1_M                                   0x0000FF00
#define CPU_SCS_NVIC_IPR0_PRI_1_S                                            8

// Field:   [7:0] PRI_0
//
// Priority of interrupt 0 (See EVENT:CPUIRQSEL0.EV for details).
#define CPU_SCS_NVIC_IPR0_PRI_0_W                                            8
#define CPU_SCS_NVIC_IPR0_PRI_0_M                                   0x000000FF
#define CPU_SCS_NVIC_IPR0_PRI_0_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR1
//
//*****************************************************************************
// Field: [31:24] PRI_7
//
// Priority of interrupt 7 (See EVENT:CPUIRQSEL7.EV for details).
#define CPU_SCS_NVIC_IPR1_PRI_7_W                                            8
#define CPU_SCS_NVIC_IPR1_PRI_7_M                                   0xFF000000
#define CPU_SCS_NVIC_IPR1_PRI_7_S                                           24

// Field: [23:16] PRI_6
//
// Priority of interrupt 6 (See EVENT:CPUIRQSEL6.EV for details).
#define CPU_SCS_NVIC_IPR1_PRI_6_W                                            8
#define CPU_SCS_NVIC_IPR1_PRI_6_M                                   0x00FF0000
#define CPU_SCS_NVIC_IPR1_PRI_6_S                                           16

// Field:  [15:8] PRI_5
//
// Priority of interrupt 5 (See EVENT:CPUIRQSEL5.EV for details).
#define CPU_SCS_NVIC_IPR1_PRI_5_W                                            8
#define CPU_SCS_NVIC_IPR1_PRI_5_M                                   0x0000FF00
#define CPU_SCS_NVIC_IPR1_PRI_5_S                                            8

// Field:   [7:0] PRI_4
//
// Priority of interrupt 4 (See EVENT:CPUIRQSEL4.EV for details).
#define CPU_SCS_NVIC_IPR1_PRI_4_W                                            8
#define CPU_SCS_NVIC_IPR1_PRI_4_M                                   0x000000FF
#define CPU_SCS_NVIC_IPR1_PRI_4_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR2
//
//*****************************************************************************
// Field: [31:24] PRI_11
//
// Priority of interrupt 11 (See EVENT:CPUIRQSEL11.EV for details).
#define CPU_SCS_NVIC_IPR2_PRI_11_W                                           8
#define CPU_SCS_NVIC_IPR2_PRI_11_M                                  0xFF000000
#define CPU_SCS_NVIC_IPR2_PRI_11_S                                          24

// Field: [23:16] PRI_10
//
// Priority of interrupt 10 (See EVENT:CPUIRQSEL10.EV for details).
#define CPU_SCS_NVIC_IPR2_PRI_10_W                                           8
#define CPU_SCS_NVIC_IPR2_PRI_10_M                                  0x00FF0000
#define CPU_SCS_NVIC_IPR2_PRI_10_S                                          16

// Field:  [15:8] PRI_9
//
// Priority of interrupt 9 (See EVENT:CPUIRQSEL9.EV for details).
#define CPU_SCS_NVIC_IPR2_PRI_9_W                                            8
#define CPU_SCS_NVIC_IPR2_PRI_9_M                                   0x0000FF00
#define CPU_SCS_NVIC_IPR2_PRI_9_S                                            8

// Field:   [7:0] PRI_8
//
// Priority of interrupt 8 (See EVENT:CPUIRQSEL8.EV for details).
#define CPU_SCS_NVIC_IPR2_PRI_8_W                                            8
#define CPU_SCS_NVIC_IPR2_PRI_8_M                                   0x000000FF
#define CPU_SCS_NVIC_IPR2_PRI_8_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR3
//
//*****************************************************************************
// Field: [31:24] PRI_15
//
// Priority of interrupt 15 (See EVENT:CPUIRQSEL15.EV for details).
#define CPU_SCS_NVIC_IPR3_PRI_15_W                                           8
#define CPU_SCS_NVIC_IPR3_PRI_15_M                                  0xFF000000
#define CPU_SCS_NVIC_IPR3_PRI_15_S                                          24

// Field: [23:16] PRI_14
//
// Priority of interrupt 14 (See EVENT:CPUIRQSEL14.EV for details).
#define CPU_SCS_NVIC_IPR3_PRI_14_W                                           8
#define CPU_SCS_NVIC_IPR3_PRI_14_M                                  0x00FF0000
#define CPU_SCS_NVIC_IPR3_PRI_14_S                                          16

// Field:  [15:8] PRI_13
//
// Priority of interrupt 13 (See EVENT:CPUIRQSEL13.EV for details).
#define CPU_SCS_NVIC_IPR3_PRI_13_W                                           8
#define CPU_SCS_NVIC_IPR3_PRI_13_M                                  0x0000FF00
#define CPU_SCS_NVIC_IPR3_PRI_13_S                                           8

// Field:   [7:0] PRI_12
//
// Priority of interrupt 12 (See EVENT:CPUIRQSEL12.EV for details).
#define CPU_SCS_NVIC_IPR3_PRI_12_W                                           8
#define CPU_SCS_NVIC_IPR3_PRI_12_M                                  0x000000FF
#define CPU_SCS_NVIC_IPR3_PRI_12_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR4
//
//*****************************************************************************
// Field: [31:24] PRI_19
//
// Priority of interrupt 19 (See EVENT:CPUIRQSEL19.EV for details).
#define CPU_SCS_NVIC_IPR4_PRI_19_W                                           8
#define CPU_SCS_NVIC_IPR4_PRI_19_M                                  0xFF000000
#define CPU_SCS_NVIC_IPR4_PRI_19_S                                          24

// Field: [23:16] PRI_18
//
// Priority of interrupt 18 (See EVENT:CPUIRQSEL18.EV for details).
#define CPU_SCS_NVIC_IPR4_PRI_18_W                                           8
#define CPU_SCS_NVIC_IPR4_PRI_18_M                                  0x00FF0000
#define CPU_SCS_NVIC_IPR4_PRI_18_S                                          16

// Field:  [15:8] PRI_17
//
// Priority of interrupt 17 (See EVENT:CPUIRQSEL17.EV for details).
#define CPU_SCS_NVIC_IPR4_PRI_17_W                                           8
#define CPU_SCS_NVIC_IPR4_PRI_17_M                                  0x0000FF00
#define CPU_SCS_NVIC_IPR4_PRI_17_S                                           8

// Field:   [7:0] PRI_16
//
// Priority of interrupt 16 (See EVENT:CPUIRQSEL16.EV for details).
#define CPU_SCS_NVIC_IPR4_PRI_16_W                                           8
#define CPU_SCS_NVIC_IPR4_PRI_16_M                                  0x000000FF
#define CPU_SCS_NVIC_IPR4_PRI_16_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR5
//
//*****************************************************************************
// Field: [31:24] PRI_23
//
// Priority of interrupt 23 (See EVENT:CPUIRQSEL23.EV for details).
#define CPU_SCS_NVIC_IPR5_PRI_23_W                                           8
#define CPU_SCS_NVIC_IPR5_PRI_23_M                                  0xFF000000
#define CPU_SCS_NVIC_IPR5_PRI_23_S                                          24

// Field: [23:16] PRI_22
//
// Priority of interrupt 22 (See EVENT:CPUIRQSEL22.EV for details).
#define CPU_SCS_NVIC_IPR5_PRI_22_W                                           8
#define CPU_SCS_NVIC_IPR5_PRI_22_M                                  0x00FF0000
#define CPU_SCS_NVIC_IPR5_PRI_22_S                                          16

// Field:  [15:8] PRI_21
//
// Priority of interrupt 21 (See EVENT:CPUIRQSEL21.EV for details).
#define CPU_SCS_NVIC_IPR5_PRI_21_W                                           8
#define CPU_SCS_NVIC_IPR5_PRI_21_M                                  0x0000FF00
#define CPU_SCS_NVIC_IPR5_PRI_21_S                                           8

// Field:   [7:0] PRI_20
//
// Priority of interrupt 20 (See EVENT:CPUIRQSEL20.EV for details).
#define CPU_SCS_NVIC_IPR5_PRI_20_W                                           8
#define CPU_SCS_NVIC_IPR5_PRI_20_M                                  0x000000FF
#define CPU_SCS_NVIC_IPR5_PRI_20_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR6
//
//*****************************************************************************
// Field: [31:24] PRI_27
//
// Priority of interrupt 27 (See EVENT:CPUIRQSEL27.EV for details).
#define CPU_SCS_NVIC_IPR6_PRI_27_W                                           8
#define CPU_SCS_NVIC_IPR6_PRI_27_M                                  0xFF000000
#define CPU_SCS_NVIC_IPR6_PRI_27_S                                          24

// Field: [23:16] PRI_26
//
// Priority of interrupt 26 (See EVENT:CPUIRQSEL26.EV for details).
#define CPU_SCS_NVIC_IPR6_PRI_26_W                                           8
#define CPU_SCS_NVIC_IPR6_PRI_26_M                                  0x00FF0000
#define CPU_SCS_NVIC_IPR6_PRI_26_S                                          16

// Field:  [15:8] PRI_25
//
// Priority of interrupt 25 (See EVENT:CPUIRQSEL25.EV for details).
#define CPU_SCS_NVIC_IPR6_PRI_25_W                                           8
#define CPU_SCS_NVIC_IPR6_PRI_25_M                                  0x0000FF00
#define CPU_SCS_NVIC_IPR6_PRI_25_S                                           8

// Field:   [7:0] PRI_24
//
// Priority of interrupt 24 (See EVENT:CPUIRQSEL24.EV for details).
#define CPU_SCS_NVIC_IPR6_PRI_24_W                                           8
#define CPU_SCS_NVIC_IPR6_PRI_24_M                                  0x000000FF
#define CPU_SCS_NVIC_IPR6_PRI_24_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR7
//
//*****************************************************************************
// Field: [31:24] PRI_31
//
// Priority of interrupt 31 (See EVENT:CPUIRQSEL31.EV for details).
#define CPU_SCS_NVIC_IPR7_PRI_31_W                                           8
#define CPU_SCS_NVIC_IPR7_PRI_31_M                                  0xFF000000
#define CPU_SCS_NVIC_IPR7_PRI_31_S                                          24

// Field: [23:16] PRI_30
//
// Priority of interrupt 30 (See EVENT:CPUIRQSEL30.EV for details).
#define CPU_SCS_NVIC_IPR7_PRI_30_W                                           8
#define CPU_SCS_NVIC_IPR7_PRI_30_M                                  0x00FF0000
#define CPU_SCS_NVIC_IPR7_PRI_30_S                                          16

// Field:  [15:8] PRI_29
//
// Priority of interrupt 29 (See EVENT:CPUIRQSEL29.EV for details).
#define CPU_SCS_NVIC_IPR7_PRI_29_W                                           8
#define CPU_SCS_NVIC_IPR7_PRI_29_M                                  0x0000FF00
#define CPU_SCS_NVIC_IPR7_PRI_29_S                                           8

// Field:   [7:0] PRI_28
//
// Priority of interrupt 28 (See EVENT:CPUIRQSEL28.EV for details).
#define CPU_SCS_NVIC_IPR7_PRI_28_W                                           8
#define CPU_SCS_NVIC_IPR7_PRI_28_M                                  0x000000FF
#define CPU_SCS_NVIC_IPR7_PRI_28_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR8
//
//*****************************************************************************
// Field: [31:24] PRI_35
//
// Priority of interrupt 35 (See EVENT:CPUIRQSEL35.EV for details).
#define CPU_SCS_NVIC_IPR8_PRI_35_W                                           8
#define CPU_SCS_NVIC_IPR8_PRI_35_M                                  0xFF000000
#define CPU_SCS_NVIC_IPR8_PRI_35_S                                          24

// Field: [23:16] PRI_34
//
// Priority of interrupt 34 (See EVENT:CPUIRQSEL34.EV for details).
#define CPU_SCS_NVIC_IPR8_PRI_34_W                                           8
#define CPU_SCS_NVIC_IPR8_PRI_34_M                                  0x00FF0000
#define CPU_SCS_NVIC_IPR8_PRI_34_S                                          16

// Field:  [15:8] PRI_33
//
// Priority of interrupt 33 (See EVENT:CPUIRQSEL33.EV for details).
#define CPU_SCS_NVIC_IPR8_PRI_33_W                                           8
#define CPU_SCS_NVIC_IPR8_PRI_33_M                                  0x0000FF00
#define CPU_SCS_NVIC_IPR8_PRI_33_S                                           8

// Field:   [7:0] PRI_32
//
// Priority of interrupt 32 (See EVENT:CPUIRQSEL32.EV for details).
#define CPU_SCS_NVIC_IPR8_PRI_32_W                                           8
#define CPU_SCS_NVIC_IPR8_PRI_32_M                                  0x000000FF
#define CPU_SCS_NVIC_IPR8_PRI_32_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_NVIC_IPR9
//
//*****************************************************************************
// Field:  [15:8] PRI_37
//
// Priority of interrupt 37 (See EVENT:CPUIRQSEL37.EV for details).
#define CPU_SCS_NVIC_IPR9_PRI_37_W                                           8
#define CPU_SCS_NVIC_IPR9_PRI_37_M                                  0x0000FF00
#define CPU_SCS_NVIC_IPR9_PRI_37_S                                           8

// Field:   [7:0] PRI_36
//
// Priority of interrupt 36 (See EVENT:CPUIRQSEL36.EV for details).
#define CPU_SCS_NVIC_IPR9_PRI_36_W                                           8
#define CPU_SCS_NVIC_IPR9_PRI_36_M                                  0x000000FF
#define CPU_SCS_NVIC_IPR9_PRI_36_S                                           0

//*****************************************************************************
//
// Register: CPU_SCS_O_CPUID
//
//*****************************************************************************
// Field: [31:24] IMPLEMENTER
//
// Implementor code.
#define CPU_SCS_CPUID_IMPLEMENTER_W                                          8
#define CPU_SCS_CPUID_IMPLEMENTER_M                                 0xFF000000
#define CPU_SCS_CPUID_IMPLEMENTER_S                                         24

// Field: [23:20] VARIANT
//
// Implementation defined variant number.
#define CPU_SCS_CPUID_VARIANT_W                                              4
#define CPU_SCS_CPUID_VARIANT_M                                     0x00F00000
#define CPU_SCS_CPUID_VARIANT_S                                             20

// Field: [19:16] CONSTANT
//
// Reads as 0xF
#define CPU_SCS_CPUID_CONSTANT_W                                             4
#define CPU_SCS_CPUID_CONSTANT_M                                    0x000F0000
#define CPU_SCS_CPUID_CONSTANT_S                                            16

// Field:  [15:4] PARTNO
//
// Number of processor within family.
#define CPU_SCS_CPUID_PARTNO_W                                              12
#define CPU_SCS_CPUID_PARTNO_M                                      0x0000FFF0
#define CPU_SCS_CPUID_PARTNO_S                                               4

// Field:   [3:0] REVISION
//
// Implementation defined revision number.
#define CPU_SCS_CPUID_REVISION_W                                             4
#define CPU_SCS_CPUID_REVISION_M                                    0x0000000F
#define CPU_SCS_CPUID_REVISION_S                                             0

//*****************************************************************************
//
// Register: CPU_SCS_O_ICSR
//
//*****************************************************************************
// Field:    [31] NMIPENDSET
//
// Set pending NMI bit. Setting this bit pends and activates an NMI. Because
// NMI is the highest-priority interrupt, it takes effect as soon as it
// registers.
//
// 0: No action
// 1: Set pending NMI
#define CPU_SCS_ICSR_NMIPENDSET                                     0x80000000
#define CPU_SCS_ICSR_NMIPENDSET_BITN                                        31
#define CPU_SCS_ICSR_NMIPENDSET_M                                   0x80000000
#define CPU_SCS_ICSR_NMIPENDSET_S                                           31

// Field:    [28] PENDSVSET
//
// Set pending pendSV bit.
//
// 0: No action
// 1: Set pending PendSV
#define CPU_SCS_ICSR_PENDSVSET                                      0x10000000
#define CPU_SCS_ICSR_PENDSVSET_BITN                                         28
#define CPU_SCS_ICSR_PENDSVSET_M                                    0x10000000
#define CPU_SCS_ICSR_PENDSVSET_S                                            28

// Field:    [27] PENDSVCLR
//
// Clear pending pendSV bit
//
// 0: No action
// 1: Clear pending pendSV
#define CPU_SCS_ICSR_PENDSVCLR                                      0x08000000
#define CPU_SCS_ICSR_PENDSVCLR_BITN                                         27
#define CPU_SCS_ICSR_PENDSVCLR_M                                    0x08000000
#define CPU_SCS_ICSR_PENDSVCLR_S                                            27

// Field:    [26] PENDSTSET
//
// Set a pending SysTick bit.
//
// 0: No action
// 1: Set pending SysTick
#define CPU_SCS_ICSR_PENDSTSET                                      0x04000000
#define CPU_SCS_ICSR_PENDSTSET_BITN                                         26
#define CPU_SCS_ICSR_PENDSTSET_M                                    0x04000000
#define CPU_SCS_ICSR_PENDSTSET_S                                            26

// Field:    [25] PENDSTCLR
//
// Clear pending SysTick bit
//
// 0: No action
// 1: Clear pending SysTick
#define CPU_SCS_ICSR_PENDSTCLR                                      0x02000000
#define CPU_SCS_ICSR_PENDSTCLR_BITN                                         25
#define CPU_SCS_ICSR_PENDSTCLR_M                                    0x02000000
#define CPU_SCS_ICSR_PENDSTCLR_S                                            25

// Field:    [23] ISRPREEMPT
//
// This field can only be used at debug time. It indicates that a pending
// interrupt is to be taken in the next running cycle. If DHCSR.C_MASKINTS= 0,
// the interrupt is serviced.
//
// 0: A pending exception is not serviced.
// 1: A pending exception is serviced on exit from the debug halt state
#define CPU_SCS_ICSR_ISRPREEMPT                                     0x00800000
#define CPU_SCS_ICSR_ISRPREEMPT_BITN                                        23
#define CPU_SCS_ICSR_ISRPREEMPT_M                                   0x00800000
#define CPU_SCS_ICSR_ISRPREEMPT_S                                           23

// Field:    [22] ISRPENDING
//
// Interrupt pending flag. Excludes NMI and faults.
//
// 0x0: Interrupt not pending
// 0x1: Interrupt pending
#define CPU_SCS_ICSR_ISRPENDING                                     0x00400000
#define CPU_SCS_ICSR_ISRPENDING_BITN                                        22
#define CPU_SCS_ICSR_ISRPENDING_M                                   0x00400000
#define CPU_SCS_ICSR_ISRPENDING_S                                           22

// Field: [17:12] VECTPENDING
//
// Pending ISR number field. This field contains the interrupt number of the
// highest priority pending ISR.
#define CPU_SCS_ICSR_VECTPENDING_W                                           6
#define CPU_SCS_ICSR_VECTPENDING_M                                  0x0003F000
#define CPU_SCS_ICSR_VECTPENDING_S                                          12

// Field:    [11] RETTOBASE
//
// Indicates whether there are preempted active exceptions:
//
// 0: There are preempted active exceptions to execute
// 1: There are no active exceptions, or the currently-executing exception is
// the only active exception.
#define CPU_SCS_ICSR_RETTOBASE                                      0x00000800
#define CPU_SCS_ICSR_RETTOBASE_BITN                                         11
#define CPU_SCS_ICSR_RETTOBASE_M                                    0x00000800
#define CPU_SCS_ICSR_RETTOBASE_S                                            11

// Field:   [8:0] VECTACTIVE
//
// Active ISR number field. Reset clears this field.
#define CPU_SCS_ICSR_VECTACTIVE_W                                            9
#define CPU_SCS_ICSR_VECTACTIVE_M                                   0x000001FF
#define CPU_SCS_ICSR_VECTACTIVE_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_VTOR
//
//*****************************************************************************
// Field:  [29:7] TBLOFF
//
// Bits 29 down to 7 of the vector table base offset.
#define CPU_SCS_VTOR_TBLOFF_W                                               23
#define CPU_SCS_VTOR_TBLOFF_M                                       0x3FFFFF80
#define CPU_SCS_VTOR_TBLOFF_S                                                7

//*****************************************************************************
//
// Register: CPU_SCS_O_AIRCR
//
//*****************************************************************************
// Field: [31:16] VECTKEY
//
// Register key. Writing to this register (AIRCR) requires 0x05FA in VECTKEY.
// Otherwise the write value is ignored. Read always returns 0xFA05.
#define CPU_SCS_AIRCR_VECTKEY_W                                             16
#define CPU_SCS_AIRCR_VECTKEY_M                                     0xFFFF0000
#define CPU_SCS_AIRCR_VECTKEY_S                                             16

// Field:    [15] ENDIANESS
//
// Data endianness bit
// ENUMs:
// BIG                      Big endian
// LITTLE                   Little endian
#define CPU_SCS_AIRCR_ENDIANESS                                     0x00008000
#define CPU_SCS_AIRCR_ENDIANESS_BITN                                        15
#define CPU_SCS_AIRCR_ENDIANESS_M                                   0x00008000
#define CPU_SCS_AIRCR_ENDIANESS_S                                           15
#define CPU_SCS_AIRCR_ENDIANESS_BIG                                 0x00008000
#define CPU_SCS_AIRCR_ENDIANESS_LITTLE                              0x00000000

// Field:  [10:8] PRIGROUP
//
// Interrupt priority grouping field. This field is a binary point position
// indicator for creating subpriorities for exceptions that share the same
// pre-emption level. It divides the PRI_n field in the Interrupt Priority
// Registers (NVIC_IPR0, NVIC_IPR1,..., and  NVIC_IPR8) into a pre-emption
// level and a subpriority level. The binary point is a left-of value. This
// means that the PRIGROUP value represents a point starting at the left of the
// Least Significant Bit (LSB). The lowest value might not be 0 depending on
// the number of bits allocated for priorities, and implementation choices.
#define CPU_SCS_AIRCR_PRIGROUP_W                                             3
#define CPU_SCS_AIRCR_PRIGROUP_M                                    0x00000700
#define CPU_SCS_AIRCR_PRIGROUP_S                                             8

// Field:     [2] SYSRESETREQ
//
// Requests a warm reset. Setting this bit does not prevent Halting Debug from
// running.
#define CPU_SCS_AIRCR_SYSRESETREQ                                   0x00000004
#define CPU_SCS_AIRCR_SYSRESETREQ_BITN                                       2
#define CPU_SCS_AIRCR_SYSRESETREQ_M                                 0x00000004
#define CPU_SCS_AIRCR_SYSRESETREQ_S                                          2

// Field:     [1] VECTCLRACTIVE
//
// Clears all active state information for active NMI, fault, and interrupts.
// It is the responsibility of the application to reinitialize the stack. This
// bit is for returning to a known state during debug. The bit self-clears.
// IPSR is not cleared by this operation. So, if used by an application, it
// must only be used at the base level of activation, or within a system
// handler whose active bit can be set.
#define CPU_SCS_AIRCR_VECTCLRACTIVE                                 0x00000002
#define CPU_SCS_AIRCR_VECTCLRACTIVE_BITN                                     1
#define CPU_SCS_AIRCR_VECTCLRACTIVE_M                               0x00000002
#define CPU_SCS_AIRCR_VECTCLRACTIVE_S                                        1

// Field:     [0] VECTRESET
//
// System Reset bit. Resets the system, with the exception of debug components.
// This bit is reserved for debug use and can be written to 1 only when the
// core is halted. The bit self-clears. Writing this bit to 1 while core is not
// halted may result in unpredictable behavior.
#define CPU_SCS_AIRCR_VECTRESET                                     0x00000001
#define CPU_SCS_AIRCR_VECTRESET_BITN                                         0
#define CPU_SCS_AIRCR_VECTRESET_M                                   0x00000001
#define CPU_SCS_AIRCR_VECTRESET_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_SCR
//
//*****************************************************************************
// Field:     [4] SEVONPEND
//
// Send Event on Pending bit:
//
// 0: Only enabled interrupts or events can wakeup the processor, disabled
// interrupts are excluded
// 1: Enabled events and all interrupts, including disabled interrupts, can
// wakeup the processor.
//
// When an event or interrupt enters pending state, the event signal wakes up
// the processor from WFE. If
// the processor is not waiting for an event, the event is registered and
// affects the next WFE.
// The processor also wakes up on execution of an SEV instruction.
#define CPU_SCS_SCR_SEVONPEND                                       0x00000010
#define CPU_SCS_SCR_SEVONPEND_BITN                                           4
#define CPU_SCS_SCR_SEVONPEND_M                                     0x00000010
#define CPU_SCS_SCR_SEVONPEND_S                                              4

// Field:     [2] SLEEPDEEP
//
// Controls whether the processor uses sleep or deep sleep as its low power
// mode
// ENUMs:
// DEEPSLEEP                Deep sleep
// SLEEP                    Sleep
#define CPU_SCS_SCR_SLEEPDEEP                                       0x00000004
#define CPU_SCS_SCR_SLEEPDEEP_BITN                                           2
#define CPU_SCS_SCR_SLEEPDEEP_M                                     0x00000004
#define CPU_SCS_SCR_SLEEPDEEP_S                                              2
#define CPU_SCS_SCR_SLEEPDEEP_DEEPSLEEP                             0x00000004
#define CPU_SCS_SCR_SLEEPDEEP_SLEEP                                 0x00000000

// Field:     [1] SLEEPONEXIT
//
// Sleep on exit when returning from Handler mode to Thread mode. Enables
// interrupt driven applications to avoid returning to empty main application.
//
// 0: Do not sleep when returning to thread mode
// 1: Sleep on ISR exit
#define CPU_SCS_SCR_SLEEPONEXIT                                     0x00000002
#define CPU_SCS_SCR_SLEEPONEXIT_BITN                                         1
#define CPU_SCS_SCR_SLEEPONEXIT_M                                   0x00000002
#define CPU_SCS_SCR_SLEEPONEXIT_S                                            1

//*****************************************************************************
//
// Register: CPU_SCS_O_CCR
//
//*****************************************************************************
// Field:     [9] STKALIGN
//
// Stack alignment bit.
//
// 0: Only 4-byte alignment is guaranteed for the SP used prior to the
// exception on exception entry.
// 1: On exception entry, the SP used prior to the exception is adjusted to be
// 8-byte aligned and the context to restore it is saved. The SP is restored on
// the associated exception return.
#define CPU_SCS_CCR_STKALIGN                                        0x00000200
#define CPU_SCS_CCR_STKALIGN_BITN                                            9
#define CPU_SCS_CCR_STKALIGN_M                                      0x00000200
#define CPU_SCS_CCR_STKALIGN_S                                               9

// Field:     [8] BFHFNMIGN
//
// Enables handlers with priority -1 or -2 to ignore data BusFaults caused by
// load and store instructions. This applies to the HardFault, NMI, and
// FAULTMASK escalated handlers:
//
// 0: Data BusFaults caused by load and store instructions cause a lock-up
// 1: Data BusFaults caused by load and store instructions are ignored.
//
// Set this bit to 1 only when the handler and its data are in absolutely safe
// memory. The normal use
// of this bit is to probe system devices and bridges to detect problems.
#define CPU_SCS_CCR_BFHFNMIGN                                       0x00000100
#define CPU_SCS_CCR_BFHFNMIGN_BITN                                           8
#define CPU_SCS_CCR_BFHFNMIGN_M                                     0x00000100
#define CPU_SCS_CCR_BFHFNMIGN_S                                              8

// Field:     [4] DIV_0_TRP
//
// Enables faulting or halting when the processor executes an SDIV or UDIV
// instruction with a divisor of 0:
//
// 0: Do not trap divide by 0. In this mode, a divide by zero returns a
// quotient of 0.
// 1: Trap divide by 0. The relevant Usage Fault Status Register bit is
// CFSR.DIVBYZERO.
#define CPU_SCS_CCR_DIV_0_TRP                                       0x00000010
#define CPU_SCS_CCR_DIV_0_TRP_BITN                                           4
#define CPU_SCS_CCR_DIV_0_TRP_M                                     0x00000010
#define CPU_SCS_CCR_DIV_0_TRP_S                                              4

// Field:     [3] UNALIGN_TRP
//
// Enables unaligned access traps:
//
// 0: Do not trap unaligned halfword and word accesses
// 1: Trap unaligned halfword and word accesses. The relevant Usage Fault
// Status Register bit is CFSR.UNALIGNED.
//
// If this bit is set to 1, an unaligned access generates a UsageFault.
// Unaligned LDM, STM, LDRD, and STRD instructions always fault regardless of
// the value in UNALIGN_TRP.
#define CPU_SCS_CCR_UNALIGN_TRP                                     0x00000008
#define CPU_SCS_CCR_UNALIGN_TRP_BITN                                         3
#define CPU_SCS_CCR_UNALIGN_TRP_M                                   0x00000008
#define CPU_SCS_CCR_UNALIGN_TRP_S                                            3

// Field:     [1] USERSETMPEND
//
// Enables unprivileged software access to STIR:
//
// 0: User code is not allowed to write to the Software Trigger Interrupt
// register (STIR).
// 1: User code can write the Software Trigger Interrupt register (STIR) to
// trigger (pend) a Main exception, which is associated with the Main stack
// pointer.
#define CPU_SCS_CCR_USERSETMPEND                                    0x00000002
#define CPU_SCS_CCR_USERSETMPEND_BITN                                        1
#define CPU_SCS_CCR_USERSETMPEND_M                                  0x00000002
#define CPU_SCS_CCR_USERSETMPEND_S                                           1

// Field:     [0] NONBASETHREDENA
//
// Indicates how the processor enters Thread mode:
//
// 0: Processor can enter Thread mode only when no exception is active.
// 1: Processor can enter Thread mode from any level using the appropriate
// return value (EXC_RETURN).
//
// Exception returns occur when one of the following instructions loads a value
// of 0xFXXXXXXX into the PC while in Handler mode:
// - POP/LDM which includes loading the PC.
// - LDR with PC as a destination.
// - BX with any register.
// The value written to the PC is intercepted and is referred to as the
// EXC_RETURN value.
#define CPU_SCS_CCR_NONBASETHREDENA                                 0x00000001
#define CPU_SCS_CCR_NONBASETHREDENA_BITN                                     0
#define CPU_SCS_CCR_NONBASETHREDENA_M                               0x00000001
#define CPU_SCS_CCR_NONBASETHREDENA_S                                        0

//*****************************************************************************
//
// Register: CPU_SCS_O_SHPR1
//
//*****************************************************************************
// Field: [23:16] PRI_6
//
// Priority of system handler 6. UsageFault
#define CPU_SCS_SHPR1_PRI_6_W                                                8
#define CPU_SCS_SHPR1_PRI_6_M                                       0x00FF0000
#define CPU_SCS_SHPR1_PRI_6_S                                               16

// Field:  [15:8] PRI_5
//
// Priority of system handler 5: BusFault
#define CPU_SCS_SHPR1_PRI_5_W                                                8
#define CPU_SCS_SHPR1_PRI_5_M                                       0x0000FF00
#define CPU_SCS_SHPR1_PRI_5_S                                                8

// Field:   [7:0] PRI_4
//
// Priority of system handler 4: MemManage
#define CPU_SCS_SHPR1_PRI_4_W                                                8
#define CPU_SCS_SHPR1_PRI_4_M                                       0x000000FF
#define CPU_SCS_SHPR1_PRI_4_S                                                0

//*****************************************************************************
//
// Register: CPU_SCS_O_SHPR2
//
//*****************************************************************************
// Field: [31:24] PRI_11
//
// Priority of system handler 11. SVCall
#define CPU_SCS_SHPR2_PRI_11_W                                               8
#define CPU_SCS_SHPR2_PRI_11_M                                      0xFF000000
#define CPU_SCS_SHPR2_PRI_11_S                                              24

//*****************************************************************************
//
// Register: CPU_SCS_O_SHPR3
//
//*****************************************************************************
// Field: [31:24] PRI_15
//
// Priority of system handler 15. SysTick exception
#define CPU_SCS_SHPR3_PRI_15_W                                               8
#define CPU_SCS_SHPR3_PRI_15_M                                      0xFF000000
#define CPU_SCS_SHPR3_PRI_15_S                                              24

// Field: [23:16] PRI_14
//
// Priority of system handler 14. Pend SV
#define CPU_SCS_SHPR3_PRI_14_W                                               8
#define CPU_SCS_SHPR3_PRI_14_M                                      0x00FF0000
#define CPU_SCS_SHPR3_PRI_14_S                                              16

// Field:   [7:0] PRI_12
//
// Priority of system handler 12. Debug Monitor
#define CPU_SCS_SHPR3_PRI_12_W                                               8
#define CPU_SCS_SHPR3_PRI_12_M                                      0x000000FF
#define CPU_SCS_SHPR3_PRI_12_S                                               0

//*****************************************************************************
//
// Register: CPU_SCS_O_SHCSR
//
//*****************************************************************************
// Field:    [18] USGFAULTENA
//
// Usage fault system handler enable
// ENUMs:
// EN                       Exception enabled
// DIS                      Exception disabled
#define CPU_SCS_SHCSR_USGFAULTENA                                   0x00040000
#define CPU_SCS_SHCSR_USGFAULTENA_BITN                                      18
#define CPU_SCS_SHCSR_USGFAULTENA_M                                 0x00040000
#define CPU_SCS_SHCSR_USGFAULTENA_S                                         18
#define CPU_SCS_SHCSR_USGFAULTENA_EN                                0x00040000
#define CPU_SCS_SHCSR_USGFAULTENA_DIS                               0x00000000

// Field:    [17] BUSFAULTENA
//
// Bus fault system handler enable
// ENUMs:
// EN                       Exception enabled
// DIS                      Exception disabled
#define CPU_SCS_SHCSR_BUSFAULTENA                                   0x00020000
#define CPU_SCS_SHCSR_BUSFAULTENA_BITN                                      17
#define CPU_SCS_SHCSR_BUSFAULTENA_M                                 0x00020000
#define CPU_SCS_SHCSR_BUSFAULTENA_S                                         17
#define CPU_SCS_SHCSR_BUSFAULTENA_EN                                0x00020000
#define CPU_SCS_SHCSR_BUSFAULTENA_DIS                               0x00000000

// Field:    [16] MEMFAULTENA
//
// MemManage fault system handler enable
// ENUMs:
// EN                       Exception enabled
// DIS                      Exception disabled
#define CPU_SCS_SHCSR_MEMFAULTENA                                   0x00010000
#define CPU_SCS_SHCSR_MEMFAULTENA_BITN                                      16
#define CPU_SCS_SHCSR_MEMFAULTENA_M                                 0x00010000
#define CPU_SCS_SHCSR_MEMFAULTENA_S                                         16
#define CPU_SCS_SHCSR_MEMFAULTENA_EN                                0x00010000
#define CPU_SCS_SHCSR_MEMFAULTENA_DIS                               0x00000000

// Field:    [15] SVCALLPENDED
//
// SVCall pending
// ENUMs:
// PENDING                  Exception is pending.
// NOTPENDING               Exception is not active
#define CPU_SCS_SHCSR_SVCALLPENDED                                  0x00008000
#define CPU_SCS_SHCSR_SVCALLPENDED_BITN                                     15
#define CPU_SCS_SHCSR_SVCALLPENDED_M                                0x00008000
#define CPU_SCS_SHCSR_SVCALLPENDED_S                                        15
#define CPU_SCS_SHCSR_SVCALLPENDED_PENDING                          0x00008000
#define CPU_SCS_SHCSR_SVCALLPENDED_NOTPENDING                       0x00000000

// Field:    [14] BUSFAULTPENDED
//
// BusFault pending
// ENUMs:
// PENDING                  Exception is pending.
// NOTPENDING               Exception is not active
#define CPU_SCS_SHCSR_BUSFAULTPENDED                                0x00004000
#define CPU_SCS_SHCSR_BUSFAULTPENDED_BITN                                   14
#define CPU_SCS_SHCSR_BUSFAULTPENDED_M                              0x00004000
#define CPU_SCS_SHCSR_BUSFAULTPENDED_S                                      14
#define CPU_SCS_SHCSR_BUSFAULTPENDED_PENDING                        0x00004000
#define CPU_SCS_SHCSR_BUSFAULTPENDED_NOTPENDING                     0x00000000

// Field:    [13] MEMFAULTPENDED
//
// MemManage exception pending
// ENUMs:
// PENDING                  Exception is pending.
// NOTPENDING               Exception is not active
#define CPU_SCS_SHCSR_MEMFAULTPENDED                                0x00002000
#define CPU_SCS_SHCSR_MEMFAULTPENDED_BITN                                   13
#define CPU_SCS_SHCSR_MEMFAULTPENDED_M                              0x00002000
#define CPU_SCS_SHCSR_MEMFAULTPENDED_S                                      13
#define CPU_SCS_SHCSR_MEMFAULTPENDED_PENDING                        0x00002000
#define CPU_SCS_SHCSR_MEMFAULTPENDED_NOTPENDING                     0x00000000

// Field:    [12] USGFAULTPENDED
//
// Usage fault pending
// ENUMs:
// PENDING                  Exception is pending.
// NOTPENDING               Exception is not active
#define CPU_SCS_SHCSR_USGFAULTPENDED                                0x00001000
#define CPU_SCS_SHCSR_USGFAULTPENDED_BITN                                   12
#define CPU_SCS_SHCSR_USGFAULTPENDED_M                              0x00001000
#define CPU_SCS_SHCSR_USGFAULTPENDED_S                                      12
#define CPU_SCS_SHCSR_USGFAULTPENDED_PENDING                        0x00001000
#define CPU_SCS_SHCSR_USGFAULTPENDED_NOTPENDING                     0x00000000

// Field:    [11] SYSTICKACT
//
// SysTick active flag.
//
// 0x0: Not active
// 0x1: Active
// ENUMs:
// ACTIVE                   Exception is active
// NOTACTIVE                Exception is not active
#define CPU_SCS_SHCSR_SYSTICKACT                                    0x00000800
#define CPU_SCS_SHCSR_SYSTICKACT_BITN                                       11
#define CPU_SCS_SHCSR_SYSTICKACT_M                                  0x00000800
#define CPU_SCS_SHCSR_SYSTICKACT_S                                          11
#define CPU_SCS_SHCSR_SYSTICKACT_ACTIVE                             0x00000800
#define CPU_SCS_SHCSR_SYSTICKACT_NOTACTIVE                          0x00000000

// Field:    [10] PENDSVACT
//
// PendSV active
//
// 0x0: Not active
// 0x1: Active
#define CPU_SCS_SHCSR_PENDSVACT                                     0x00000400
#define CPU_SCS_SHCSR_PENDSVACT_BITN                                        10
#define CPU_SCS_SHCSR_PENDSVACT_M                                   0x00000400
#define CPU_SCS_SHCSR_PENDSVACT_S                                           10

// Field:     [8] MONITORACT
//
// Debug monitor active
// ENUMs:
// ACTIVE                   Exception is active
// NOTACTIVE                Exception is not active
#define CPU_SCS_SHCSR_MONITORACT                                    0x00000100
#define CPU_SCS_SHCSR_MONITORACT_BITN                                        8
#define CPU_SCS_SHCSR_MONITORACT_M                                  0x00000100
#define CPU_SCS_SHCSR_MONITORACT_S                                           8
#define CPU_SCS_SHCSR_MONITORACT_ACTIVE                             0x00000100
#define CPU_SCS_SHCSR_MONITORACT_NOTACTIVE                          0x00000000

// Field:     [7] SVCALLACT
//
// SVCall active
// ENUMs:
// ACTIVE                   Exception is active
// NOTACTIVE                Exception is not active
#define CPU_SCS_SHCSR_SVCALLACT                                     0x00000080
#define CPU_SCS_SHCSR_SVCALLACT_BITN                                         7
#define CPU_SCS_SHCSR_SVCALLACT_M                                   0x00000080
#define CPU_SCS_SHCSR_SVCALLACT_S                                            7
#define CPU_SCS_SHCSR_SVCALLACT_ACTIVE                              0x00000080
#define CPU_SCS_SHCSR_SVCALLACT_NOTACTIVE                           0x00000000

// Field:     [3] USGFAULTACT
//
// UsageFault exception active
// ENUMs:
// ACTIVE                   Exception is active
// NOTACTIVE                Exception is not active
#define CPU_SCS_SHCSR_USGFAULTACT                                   0x00000008
#define CPU_SCS_SHCSR_USGFAULTACT_BITN                                       3
#define CPU_SCS_SHCSR_USGFAULTACT_M                                 0x00000008
#define CPU_SCS_SHCSR_USGFAULTACT_S                                          3
#define CPU_SCS_SHCSR_USGFAULTACT_ACTIVE                            0x00000008
#define CPU_SCS_SHCSR_USGFAULTACT_NOTACTIVE                         0x00000000

// Field:     [1] BUSFAULTACT
//
// BusFault exception active
// ENUMs:
// ACTIVE                   Exception is active
// NOTACTIVE                Exception is not active
#define CPU_SCS_SHCSR_BUSFAULTACT                                   0x00000002
#define CPU_SCS_SHCSR_BUSFAULTACT_BITN                                       1
#define CPU_SCS_SHCSR_BUSFAULTACT_M                                 0x00000002
#define CPU_SCS_SHCSR_BUSFAULTACT_S                                          1
#define CPU_SCS_SHCSR_BUSFAULTACT_ACTIVE                            0x00000002
#define CPU_SCS_SHCSR_BUSFAULTACT_NOTACTIVE                         0x00000000

// Field:     [0] MEMFAULTACT
//
// MemManage exception active
// ENUMs:
// ACTIVE                   Exception is active
// NOTACTIVE                Exception is not active
#define CPU_SCS_SHCSR_MEMFAULTACT                                   0x00000001
#define CPU_SCS_SHCSR_MEMFAULTACT_BITN                                       0
#define CPU_SCS_SHCSR_MEMFAULTACT_M                                 0x00000001
#define CPU_SCS_SHCSR_MEMFAULTACT_S                                          0
#define CPU_SCS_SHCSR_MEMFAULTACT_ACTIVE                            0x00000001
#define CPU_SCS_SHCSR_MEMFAULTACT_NOTACTIVE                         0x00000000

//*****************************************************************************
//
// Register: CPU_SCS_O_CFSR
//
//*****************************************************************************
// Field:    [25] DIVBYZERO
//
// When CCR.DIV_0_TRP (see Configuration Control Register on page 8-26) is
// enabled and an SDIV or UDIV instruction is used with a divisor of 0, this
// fault occurs The instruction is executed and the return PC points to it. If
// CCR.DIV_0_TRP is not set, then the divide returns a quotient of 0.
#define CPU_SCS_CFSR_DIVBYZERO                                      0x02000000
#define CPU_SCS_CFSR_DIVBYZERO_BITN                                         25
#define CPU_SCS_CFSR_DIVBYZERO_M                                    0x02000000
#define CPU_SCS_CFSR_DIVBYZERO_S                                            25

// Field:    [24] UNALIGNED
//
// When CCR.UNALIGN_TRP is enabled, and there is an attempt to make an
// unaligned memory access, then this fault occurs. Unaligned LDM/STM/LDRD/STRD
// instructions always fault irrespective of the setting of CCR.UNALIGN_TRP.
#define CPU_SCS_CFSR_UNALIGNED                                      0x01000000
#define CPU_SCS_CFSR_UNALIGNED_BITN                                         24
#define CPU_SCS_CFSR_UNALIGNED_M                                    0x01000000
#define CPU_SCS_CFSR_UNALIGNED_S                                            24

// Field:    [19] NOCP
//
// Attempt to use a coprocessor instruction. The processor does not support
// coprocessor instructions.
#define CPU_SCS_CFSR_NOCP                                           0x00080000
#define CPU_SCS_CFSR_NOCP_BITN                                              19
#define CPU_SCS_CFSR_NOCP_M                                         0x00080000
#define CPU_SCS_CFSR_NOCP_S                                                 19

// Field:    [18] INVPC
//
// Attempt to load EXC_RETURN into PC illegally. Invalid instruction, invalid
// context, invalid value. The return PC points to the instruction that tried
// to set the PC.
#define CPU_SCS_CFSR_INVPC                                          0x00040000
#define CPU_SCS_CFSR_INVPC_BITN                                             18
#define CPU_SCS_CFSR_INVPC_M                                        0x00040000
#define CPU_SCS_CFSR_INVPC_S                                                18

// Field:    [17] INVSTATE
//
// Indicates an attempt to execute in an invalid EPSR state (e.g. after a BX
// type instruction has changed state). This includes state change after entry
// to or return from exception, as well as from inter-working instructions.
// Return PC points to faulting instruction, with the invalid state.
#define CPU_SCS_CFSR_INVSTATE                                       0x00020000
#define CPU_SCS_CFSR_INVSTATE_BITN                                          17
#define CPU_SCS_CFSR_INVSTATE_M                                     0x00020000
#define CPU_SCS_CFSR_INVSTATE_S                                             17

// Field:    [16] UNDEFINSTR
//
// This bit is set when the processor attempts to execute an undefined
// instruction. This is an instruction that the processor cannot decode. The
// return PC points to the undefined instruction.
#define CPU_SCS_CFSR_UNDEFINSTR                                     0x00010000
#define CPU_SCS_CFSR_UNDEFINSTR_BITN                                        16
#define CPU_SCS_CFSR_UNDEFINSTR_M                                   0x00010000
#define CPU_SCS_CFSR_UNDEFINSTR_S                                           16

// Field:    [15] BFARVALID
//
// This bit is set if the Bus Fault Address Register (BFAR) contains a valid
// address. This is true after a bus fault where the address is known. Other
// faults can clear this bit, such as a Mem Manage fault occurring later. If a
// Bus fault occurs that is escalated to a Hard Fault because of priority, the
// Hard Fault handler must clear this bit. This prevents problems if returning
// to a stacked active Bus fault handler whose BFAR value has been overwritten.
#define CPU_SCS_CFSR_BFARVALID                                      0x00008000
#define CPU_SCS_CFSR_BFARVALID_BITN                                         15
#define CPU_SCS_CFSR_BFARVALID_M                                    0x00008000
#define CPU_SCS_CFSR_BFARVALID_S                                            15

// Field:    [12] STKERR
//
// Stacking from exception has caused one or more bus faults. The SP is still
// adjusted and the values in the context area on the stack might be incorrect.
// BFAR is not written.
#define CPU_SCS_CFSR_STKERR                                         0x00001000
#define CPU_SCS_CFSR_STKERR_BITN                                            12
#define CPU_SCS_CFSR_STKERR_M                                       0x00001000
#define CPU_SCS_CFSR_STKERR_S                                               12

// Field:    [11] UNSTKERR
//
// Unstack from exception return has caused one or more bus faults. This is
// chained to the handler, so that the original return stack is still present.
// SP is not adjusted from failing return and new save is not performed. BFAR
// is not written.
#define CPU_SCS_CFSR_UNSTKERR                                       0x00000800
#define CPU_SCS_CFSR_UNSTKERR_BITN                                          11
#define CPU_SCS_CFSR_UNSTKERR_M                                     0x00000800
#define CPU_SCS_CFSR_UNSTKERR_S                                             11

// Field:    [10] IMPRECISERR
//
// Imprecise data bus error. It is a BusFault, but the Return PC is not related
// to the causing instruction. This is not a synchronous fault. So, if detected
// when the priority of the current activation is higher than the Bus Fault, it
// only pends. Bus fault activates when returning to a lower priority
// activation. If a precise fault occurs before returning to a lower priority
// exception, the handler detects both IMPRECISERR set and one of the precise
// fault status bits set at the same time. BFAR is not written.
#define CPU_SCS_CFSR_IMPRECISERR                                    0x00000400
#define CPU_SCS_CFSR_IMPRECISERR_BITN                                       10
#define CPU_SCS_CFSR_IMPRECISERR_M                                  0x00000400
#define CPU_SCS_CFSR_IMPRECISERR_S                                          10

// Field:     [9] PRECISERR
//
// Precise data bus error return.
#define CPU_SCS_CFSR_PRECISERR                                      0x00000200
#define CPU_SCS_CFSR_PRECISERR_BITN                                          9
#define CPU_SCS_CFSR_PRECISERR_M                                    0x00000200
#define CPU_SCS_CFSR_PRECISERR_S                                             9

// Field:     [8] IBUSERR
//
// Instruction bus error flag. This flag is set by a prefetch error. The fault
// stops on the instruction, so if the error occurs under a branch shadow, no
// fault occurs. BFAR is not written.
#define CPU_SCS_CFSR_IBUSERR                                        0x00000100
#define CPU_SCS_CFSR_IBUSERR_BITN                                            8
#define CPU_SCS_CFSR_IBUSERR_M                                      0x00000100
#define CPU_SCS_CFSR_IBUSERR_S                                               8

// Field:     [7] MMARVALID
//
// Memory Manage Address Register (MMFAR) address valid flag. A later-arriving
// fault, such as a bus fault, can clear a memory manage fault.. If a MemManage
// fault occurs that is escalated to a Hard Fault because of priority, the Hard
// Fault handler must clear this bit. This prevents problems on return to a
// stacked active MemManage handler whose MMFAR value has been overwritten.
#define CPU_SCS_CFSR_MMARVALID                                      0x00000080
#define CPU_SCS_CFSR_MMARVALID_BITN                                          7
#define CPU_SCS_CFSR_MMARVALID_M                                    0x00000080
#define CPU_SCS_CFSR_MMARVALID_S                                             7

// Field:     [4] MSTKERR
//
// Stacking from exception has caused one or more access violations. The SP is
// still adjusted and the values in the context area on the stack might be
// incorrect. MMFAR is not written.
#define CPU_SCS_CFSR_MSTKERR                                        0x00000010
#define CPU_SCS_CFSR_MSTKERR_BITN                                            4
#define CPU_SCS_CFSR_MSTKERR_M                                      0x00000010
#define CPU_SCS_CFSR_MSTKERR_S                                               4

// Field:     [3] MUNSTKERR
//
// Unstack from exception return has caused one or more access violations. This
// is chained to the handler, so that the original return stack is still
// present. SP is not adjusted from failing return and new save is not
// performed. MMFAR is not written.
#define CPU_SCS_CFSR_MUNSTKERR                                      0x00000008
#define CPU_SCS_CFSR_MUNSTKERR_BITN                                          3
#define CPU_SCS_CFSR_MUNSTKERR_M                                    0x00000008
#define CPU_SCS_CFSR_MUNSTKERR_S                                             3

// Field:     [1] DACCVIOL
//
// Data access violation flag. Attempting to load or store at a location that
// does not permit the operation sets this flag. The return PC points to the
// faulting instruction. This error loads MMFAR with the address of the
// attempted access.
#define CPU_SCS_CFSR_DACCVIOL                                       0x00000002
#define CPU_SCS_CFSR_DACCVIOL_BITN                                           1
#define CPU_SCS_CFSR_DACCVIOL_M                                     0x00000002
#define CPU_SCS_CFSR_DACCVIOL_S                                              1

// Field:     [0] IACCVIOL
//
// Instruction access violation flag. Attempting to fetch an instruction from a
// location that does not permit execution sets this flag. This occurs on any
// access to an XN region, even when the MPU is disabled or not present. The
// return PC points to the faulting instruction. MMFAR is not written.
#define CPU_SCS_CFSR_IACCVIOL                                       0x00000001
#define CPU_SCS_CFSR_IACCVIOL_BITN                                           0
#define CPU_SCS_CFSR_IACCVIOL_M                                     0x00000001
#define CPU_SCS_CFSR_IACCVIOL_S                                              0

//*****************************************************************************
//
// Register: CPU_SCS_O_HFSR
//
//*****************************************************************************
// Field:    [31] DEBUGEVT
//
// This bit is set if there is a fault related to debug. This is only possible
// when halting debug is not enabled. For monitor enabled debug, it only
// happens for BKPT when the current priority is higher than the monitor. When
// both halting and monitor debug are disabled, it only happens for debug
// events that are not ignored (minimally, BKPT). The Debug Fault Status
// Register is updated.
#define CPU_SCS_HFSR_DEBUGEVT                                       0x80000000
#define CPU_SCS_HFSR_DEBUGEVT_BITN                                          31
#define CPU_SCS_HFSR_DEBUGEVT_M                                     0x80000000
#define CPU_SCS_HFSR_DEBUGEVT_S                                             31

// Field:    [30] FORCED
//
// Hard Fault activated because a Configurable Fault was received and cannot
// activate because of priority or because the Configurable Fault is disabled.
// The Hard Fault handler then has to read the other fault status registers to
// determine cause.
#define CPU_SCS_HFSR_FORCED                                         0x40000000
#define CPU_SCS_HFSR_FORCED_BITN                                            30
#define CPU_SCS_HFSR_FORCED_M                                       0x40000000
#define CPU_SCS_HFSR_FORCED_S                                               30

// Field:     [1] VECTTBL
//
// This bit is set if there is a fault because of vector table read on
// exception processing (Bus Fault). This case is always a Hard Fault. The
// return PC points to the pre-empted instruction.
#define CPU_SCS_HFSR_VECTTBL                                        0x00000002
#define CPU_SCS_HFSR_VECTTBL_BITN                                            1
#define CPU_SCS_HFSR_VECTTBL_M                                      0x00000002
#define CPU_SCS_HFSR_VECTTBL_S                                               1

//*****************************************************************************
//
// Register: CPU_SCS_O_DFSR
//
//*****************************************************************************
// Field:     [4] EXTERNAL
//
// External debug request flag. The processor stops on next instruction
// boundary.
//
// 0x0: External debug request signal not asserted
// 0x1: External debug request signal asserted
#define CPU_SCS_DFSR_EXTERNAL                                       0x00000010
#define CPU_SCS_DFSR_EXTERNAL_BITN                                           4
#define CPU_SCS_DFSR_EXTERNAL_M                                     0x00000010
#define CPU_SCS_DFSR_EXTERNAL_S                                              4

// Field:     [3] VCATCH
//
// Vector catch flag. When this flag is set, a flag in one of the local fault
// status registers is also set to indicate the type of fault.
//
// 0x0: No vector catch occurred
// 0x1: Vector catch occurred
#define CPU_SCS_DFSR_VCATCH                                         0x00000008
#define CPU_SCS_DFSR_VCATCH_BITN                                             3
#define CPU_SCS_DFSR_VCATCH_M                                       0x00000008
#define CPU_SCS_DFSR_VCATCH_S                                                3

// Field:     [2] DWTTRAP
//
// Data Watchpoint and Trace (DWT) flag. The processor stops at the current
// instruction or at the next instruction.
//
// 0x0: No DWT match
// 0x1: DWT match
#define CPU_SCS_DFSR_DWTTRAP                                        0x00000004
#define CPU_SCS_DFSR_DWTTRAP_BITN                                            2
#define CPU_SCS_DFSR_DWTTRAP_M                                      0x00000004
#define CPU_SCS_DFSR_DWTTRAP_S                                               2

// Field:     [1] BKPT
//
// BKPT flag. The BKPT flag is set by a BKPT instruction in flash patch code,
// and also by normal code. Return PC points to breakpoint containing
// instruction.
//
// 0x0: No BKPT instruction execution
// 0x1: BKPT instruction execution
#define CPU_SCS_DFSR_BKPT                                           0x00000002
#define CPU_SCS_DFSR_BKPT_BITN                                               1
#define CPU_SCS_DFSR_BKPT_M                                         0x00000002
#define CPU_SCS_DFSR_BKPT_S                                                  1

// Field:     [0] HALTED
//
// Halt request flag. The processor is halted on the next instruction.
//
// 0x0: No halt request
// 0x1: Halt requested by NVIC, including step
#define CPU_SCS_DFSR_HALTED                                         0x00000001
#define CPU_SCS_DFSR_HALTED_BITN                                             0
#define CPU_SCS_DFSR_HALTED_M                                       0x00000001
#define CPU_SCS_DFSR_HALTED_S                                                0

//*****************************************************************************
//
// Register: CPU_SCS_O_MMFAR
//
//*****************************************************************************
// Field:  [31:0] ADDRESS
//
// Mem Manage fault address field.
// This field is the data address of a faulted load or store attempt. When an
// unaligned access faults, the address is the actual address that faulted.
// Because an access can be split into multiple parts, each aligned, this
// address can be any offset in the range of the requested size. Flags
// CFSR.IACCVIOL, CFSR.DACCVIOL ,CFSR.MUNSTKERR and CFSR.MSTKERR in combination
// with CFSR.MMARVALIDindicate the cause of the fault.
#define CPU_SCS_MMFAR_ADDRESS_W                                             32
#define CPU_SCS_MMFAR_ADDRESS_M                                     0xFFFFFFFF
#define CPU_SCS_MMFAR_ADDRESS_S                                              0

//*****************************************************************************
//
// Register: CPU_SCS_O_BFAR
//
//*****************************************************************************
// Field:  [31:0] ADDRESS
//
// Bus fault address field. This field is the data address of a faulted load or
// store attempt. When an unaligned access faults, the address is the address
// requested by the instruction, even if that is not the address that faulted.
// Flags CFSR.IBUSERR, CFSR.PRECISERR, CFSR.IMPRECISERR, CFSR.UNSTKERR and
// CFSR.STKERR in combination with CFSR.BFARVALID indicate the cause of the
// fault.
#define CPU_SCS_BFAR_ADDRESS_W                                              32
#define CPU_SCS_BFAR_ADDRESS_M                                      0xFFFFFFFF
#define CPU_SCS_BFAR_ADDRESS_S                                               0

//*****************************************************************************
//
// Register: CPU_SCS_O_AFSR
//
//*****************************************************************************
// Field:  [31:0] IMPDEF
//
// Implementation defined. The bits map directly onto the signal assignment to
// the auxiliary fault inputs. Tied to 0
#define CPU_SCS_AFSR_IMPDEF_W                                               32
#define CPU_SCS_AFSR_IMPDEF_M                                       0xFFFFFFFF
#define CPU_SCS_AFSR_IMPDEF_S                                                0

//*****************************************************************************
//
// Register: CPU_SCS_O_ID_PFR0
//
//*****************************************************************************
// Field:   [7:4] STATE1
//
// State1 (T-bit == 1)
//
// 0x0: N/A
// 0x1: N/A
// 0x2: Thumb-2 encoding with the 16-bit basic instructions plus 32-bit
// Buncond/BL but no other 32-bit basic instructions (Note non-basic 32-bit
// instructions can be added using the appropriate instruction attribute, but
// other 32-bit basic instructions cannot.)
// 0x3: Thumb-2 encoding with all Thumb-2 basic instructions
#define CPU_SCS_ID_PFR0_STATE1_W                                             4
#define CPU_SCS_ID_PFR0_STATE1_M                                    0x000000F0
#define CPU_SCS_ID_PFR0_STATE1_S                                             4

// Field:   [3:0] STATE0
//
// State0 (T-bit == 0)
//
// 0x0: No ARM encoding
// 0x1: N/A
#define CPU_SCS_ID_PFR0_STATE0_W                                             4
#define CPU_SCS_ID_PFR0_STATE0_M                                    0x0000000F
#define CPU_SCS_ID_PFR0_STATE0_S                                             0

//*****************************************************************************
//
// Register: CPU_SCS_O_ID_PFR1
//
//*****************************************************************************
// Field:  [11:8] MICROCONTROLLER_PROGRAMMERS_MODEL
//
// Microcontroller programmer's model
//
// 0x0: Not supported
// 0x2: Two-stack support
#define CPU_SCS_ID_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_W                  4
#define CPU_SCS_ID_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_M         0x00000F00
#define CPU_SCS_ID_PFR1_MICROCONTROLLER_PROGRAMMERS_MODEL_S                  8

//*****************************************************************************
//
// Register: CPU_SCS_O_ID_DFR0
//
//*****************************************************************************
// Field: [23:20] MICROCONTROLLER_DEBUG_MODEL
//
// Microcontroller Debug Model - memory mapped
//
// 0x0: Not supported
// 0x1: Microcontroller debug v1 (ITMv1 and DWTv1)
#define CPU_SCS_ID_DFR0_MICROCONTROLLER_DEBUG_MODEL_W                        4
#define CPU_SCS_ID_DFR0_MICROCONTROLLER_DEBUG_MODEL_M               0x00F00000
#define CPU_SCS_ID_DFR0_MICROCONTROLLER_DEBUG_MODEL_S                       20

//*****************************************************************************
//
// Register: CPU_SCS_O_ID_AFR0
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_ID_MMFR0
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_ID_MMFR1
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_ID_MMFR2
//
//*****************************************************************************
// Field:    [24] WAIT_FOR_INTERRUPT_STALLING
//
// wait for interrupt stalling
//
// 0x0: Not supported
// 0x1: Wait for interrupt supported
#define CPU_SCS_ID_MMFR2_WAIT_FOR_INTERRUPT_STALLING                0x01000000
#define CPU_SCS_ID_MMFR2_WAIT_FOR_INTERRUPT_STALLING_BITN                   24
#define CPU_SCS_ID_MMFR2_WAIT_FOR_INTERRUPT_STALLING_M              0x01000000
#define CPU_SCS_ID_MMFR2_WAIT_FOR_INTERRUPT_STALLING_S                      24

//*****************************************************************************
//
// Register: CPU_SCS_O_ID_MMFR3
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_ID_ISAR0
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_ID_ISAR1
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_ID_ISAR2
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_ID_ISAR3
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_ID_ISAR4
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_CPACR
//
//*****************************************************************************
//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_TYPE
//
//*****************************************************************************
// Field: [23:16] IREGION
//
// The processor core uses only a unified MPU, this field always reads 0x0.
#define CPU_SCS_MPU_TYPE_IREGION_W                                           8
#define CPU_SCS_MPU_TYPE_IREGION_M                                  0x00FF0000
#define CPU_SCS_MPU_TYPE_IREGION_S                                          16

// Field:  [15:8] DREGION
//
// Number of supported MPU regions field. This field reads 0x08 indicating
// eight MPU regions.
#define CPU_SCS_MPU_TYPE_DREGION_W                                           8
#define CPU_SCS_MPU_TYPE_DREGION_M                                  0x0000FF00
#define CPU_SCS_MPU_TYPE_DREGION_S                                           8

// Field:     [0] SEPARATE
//
// The processor core uses only a unified MPU, thus this field is always 0.
#define CPU_SCS_MPU_TYPE_SEPARATE                                   0x00000001
#define CPU_SCS_MPU_TYPE_SEPARATE_BITN                                       0
#define CPU_SCS_MPU_TYPE_SEPARATE_M                                 0x00000001
#define CPU_SCS_MPU_TYPE_SEPARATE_S                                          0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_CTRL
//
//*****************************************************************************
// Field:     [2] PRIVDEFENA
//
// This bit enables the default memory map for privileged access, as a
// background region, when the MPU is enabled. The background region acts as if
// it was region number 1 before any settable regions. Any region that is set
// up overlays this default map, and overrides it. If this bit is not set, the
// default memory map is disabled, and memory not covered by a region faults.
// This applies to memory type, Execute Never (XN), cache and shareable rules.
// However, this only applies to privileged mode (fetch and data access). User
// mode code faults unless a region has been set up for its code and data. When
// the MPU is disabled, the default map acts on both privileged and user mode
// code. XN and SO rules always apply to the system partition whether this
// enable is set or not. If the MPU is disabled, this bit is ignored.
#define CPU_SCS_MPU_CTRL_PRIVDEFENA                                 0x00000004
#define CPU_SCS_MPU_CTRL_PRIVDEFENA_BITN                                     2
#define CPU_SCS_MPU_CTRL_PRIVDEFENA_M                               0x00000004
#define CPU_SCS_MPU_CTRL_PRIVDEFENA_S                                        2

// Field:     [1] HFNMIENA
//
// This bit enables the MPU when in Hard Fault, NMI, and FAULTMASK escalated
// handlers. If this bit and ENABLE are set, the MPU is enabled when in these
// handlers. If this bit is not set, the MPU is disabled when in these
// handlers, regardless of the value of ENABLE bit. If this bit is set and
// ENABLE is not set, behavior is unpredictable.
#define CPU_SCS_MPU_CTRL_HFNMIENA                                   0x00000002
#define CPU_SCS_MPU_CTRL_HFNMIENA_BITN                                       1
#define CPU_SCS_MPU_CTRL_HFNMIENA_M                                 0x00000002
#define CPU_SCS_MPU_CTRL_HFNMIENA_S                                          1

// Field:     [0] ENABLE
//
// Enable MPU
//
// 0: MPU disabled
// 1: MPU enabled
#define CPU_SCS_MPU_CTRL_ENABLE                                     0x00000001
#define CPU_SCS_MPU_CTRL_ENABLE_BITN                                         0
#define CPU_SCS_MPU_CTRL_ENABLE_M                                   0x00000001
#define CPU_SCS_MPU_CTRL_ENABLE_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RNR
//
//*****************************************************************************
// Field:   [7:0] REGION
//
// Region select field.
// This field selects the region to operate on when using the MPU_RASR and
// MPU_RBAR. It must be written first except when the address MPU_RBAR.VALID
// and MPU_RBAR.REGION fields are written, which overwrites this.
#define CPU_SCS_MPU_RNR_REGION_W                                             8
#define CPU_SCS_MPU_RNR_REGION_M                                    0x000000FF
#define CPU_SCS_MPU_RNR_REGION_S                                             0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RBAR
//
//*****************************************************************************
// Field:  [31:5] ADDR
//
// Region base address field.
// The position of the LSB depends on the region size, so that the base address
// is aligned according to an even multiple of size. The power of 2 size
// specified by the SZENABLE field of the MPU Region Attribute and Size
// Register defines how many bits of base address are used.
#define CPU_SCS_MPU_RBAR_ADDR_W                                             27
#define CPU_SCS_MPU_RBAR_ADDR_M                                     0xFFFFFFE0
#define CPU_SCS_MPU_RBAR_ADDR_S                                              5

// Field:     [4] VALID
//
// MPU region number valid:
// 0: MPU_RNR remains unchanged and is interpreted.
// 1: MPU_RNR is overwritten by REGION.
#define CPU_SCS_MPU_RBAR_VALID                                      0x00000010
#define CPU_SCS_MPU_RBAR_VALID_BITN                                          4
#define CPU_SCS_MPU_RBAR_VALID_M                                    0x00000010
#define CPU_SCS_MPU_RBAR_VALID_S                                             4

// Field:   [3:0] REGION
//
// MPU region override field
#define CPU_SCS_MPU_RBAR_REGION_W                                            4
#define CPU_SCS_MPU_RBAR_REGION_M                                   0x0000000F
#define CPU_SCS_MPU_RBAR_REGION_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RASR
//
//*****************************************************************************
// Field:    [28] XN
//
// Instruction access disable:
// 0: Enable instruction fetches
// 1: Disable instruction fetches
#define CPU_SCS_MPU_RASR_XN                                         0x10000000
#define CPU_SCS_MPU_RASR_XN_BITN                                            28
#define CPU_SCS_MPU_RASR_XN_M                                       0x10000000
#define CPU_SCS_MPU_RASR_XN_S                                               28

// Field: [26:24] AP
//
// Data access permission:
// 0x0: Priviliged permissions: No access. User permissions: No access.
// 0x1: Priviliged permissions: Read-write. User permissions: No access.
// 0x2: Priviliged permissions: Read-write. User permissions: Read-only.
// 0x3: Priviliged permissions: Read-write. User permissions: Read-write.
// 0x4: Reserved
// 0x5: Priviliged permissions: Read-only. User permissions: No access.
// 0x6: Priviliged permissions: Read-only. User permissions: Read-only.
// 0x7: Priviliged permissions: Read-only. User permissions: Read-only.
#define CPU_SCS_MPU_RASR_AP_W                                                3
#define CPU_SCS_MPU_RASR_AP_M                                       0x07000000
#define CPU_SCS_MPU_RASR_AP_S                                               24

// Field: [21:19] TEX
//
// Type extension
#define CPU_SCS_MPU_RASR_TEX_W                                               3
#define CPU_SCS_MPU_RASR_TEX_M                                      0x00380000
#define CPU_SCS_MPU_RASR_TEX_S                                              19

// Field:    [18] S
//
// Shareable bit:
// 0: Not shareable
// 1: Shareable
#define CPU_SCS_MPU_RASR_S                                          0x00040000
#define CPU_SCS_MPU_RASR_S_BITN                                             18
#define CPU_SCS_MPU_RASR_S_M                                        0x00040000
#define CPU_SCS_MPU_RASR_S_S                                                18

// Field:    [17] C
//
// Cacheable bit:
// 0: Not cacheable
// 1: Cacheable
#define CPU_SCS_MPU_RASR_C                                          0x00020000
#define CPU_SCS_MPU_RASR_C_BITN                                             17
#define CPU_SCS_MPU_RASR_C_M                                        0x00020000
#define CPU_SCS_MPU_RASR_C_S                                                17

// Field:    [16] B
//
// Bufferable bit:
// 0: Not bufferable
// 1: Bufferable
#define CPU_SCS_MPU_RASR_B                                          0x00010000
#define CPU_SCS_MPU_RASR_B_BITN                                             16
#define CPU_SCS_MPU_RASR_B_M                                        0x00010000
#define CPU_SCS_MPU_RASR_B_S                                                16

// Field:  [15:8] SRD
//
// Sub-Region Disable field:
// Setting a bit in this field disables the corresponding sub-region. Regions
// are split into eight equal-sized sub-regions. Sub-regions are not supported
// for region sizes of 128 bytes and less.
#define CPU_SCS_MPU_RASR_SRD_W                                               8
#define CPU_SCS_MPU_RASR_SRD_M                                      0x0000FF00
#define CPU_SCS_MPU_RASR_SRD_S                                               8

// Field:   [5:1] SIZE
//
// MPU Protection Region Size Field:
// 0x04: 32B
// 0x05: 64B
// 0x06: 128B
// 0x07: 256B
// 0x08: 512B
// 0x09: 1KB
// 0x0A: 2KB
// 0x0B: 4KB
// 0x0C: 8KB
// 0x0D: 16KB
// 0x0E: 32KB
// 0x0F: 64KB
// 0x10: 128KB
// 0x11: 256KB
// 0x12: 512KB
// 0x13: 1MB
// 0x14: 2MB
// 0x15: 4MB
// 0x16: 8MB
// 0x17: 16MB
// 0x18: 32MB
// 0x19: 64MB
// 0x1A: 128MB
// 0x1B: 256MB
// 0x1C: 512MB
// 0x1D: 1GB
// 0x1E: 2GB
// 0x1F: 4GB
#define CPU_SCS_MPU_RASR_SIZE_W                                              5
#define CPU_SCS_MPU_RASR_SIZE_M                                     0x0000003E
#define CPU_SCS_MPU_RASR_SIZE_S                                              1

// Field:     [0] ENABLE
//
// Region enable bit:
// 0: Disable region
// 1: Enable region
#define CPU_SCS_MPU_RASR_ENABLE                                     0x00000001
#define CPU_SCS_MPU_RASR_ENABLE_BITN                                         0
#define CPU_SCS_MPU_RASR_ENABLE_M                                   0x00000001
#define CPU_SCS_MPU_RASR_ENABLE_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RBAR_A1
//
//*****************************************************************************
// Field:  [31:0] MPU_RBAR_A1
//
// Alias for MPU_RBAR
#define CPU_SCS_MPU_RBAR_A1_MPU_RBAR_A1_W                                   32
#define CPU_SCS_MPU_RBAR_A1_MPU_RBAR_A1_M                           0xFFFFFFFF
#define CPU_SCS_MPU_RBAR_A1_MPU_RBAR_A1_S                                    0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RASR_A1
//
//*****************************************************************************
// Field:  [31:0] MPU_RASR_A1
//
// Alias for MPU_RASR
#define CPU_SCS_MPU_RASR_A1_MPU_RASR_A1_W                                   32
#define CPU_SCS_MPU_RASR_A1_MPU_RASR_A1_M                           0xFFFFFFFF
#define CPU_SCS_MPU_RASR_A1_MPU_RASR_A1_S                                    0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RBAR_A2
//
//*****************************************************************************
// Field:  [31:0] MPU_RBAR_A2
//
// Alias for MPU_RBAR
#define CPU_SCS_MPU_RBAR_A2_MPU_RBAR_A2_W                                   32
#define CPU_SCS_MPU_RBAR_A2_MPU_RBAR_A2_M                           0xFFFFFFFF
#define CPU_SCS_MPU_RBAR_A2_MPU_RBAR_A2_S                                    0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RASR_A2
//
//*****************************************************************************
// Field:  [31:0] MPU_RASR_A2
//
// Alias for MPU_RASR
#define CPU_SCS_MPU_RASR_A2_MPU_RASR_A2_W                                   32
#define CPU_SCS_MPU_RASR_A2_MPU_RASR_A2_M                           0xFFFFFFFF
#define CPU_SCS_MPU_RASR_A2_MPU_RASR_A2_S                                    0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RBAR_A3
//
//*****************************************************************************
// Field:  [31:0] MPU_RBAR_A3
//
// Alias for MPU_RBAR
#define CPU_SCS_MPU_RBAR_A3_MPU_RBAR_A3_W                                   32
#define CPU_SCS_MPU_RBAR_A3_MPU_RBAR_A3_M                           0xFFFFFFFF
#define CPU_SCS_MPU_RBAR_A3_MPU_RBAR_A3_S                                    0

//*****************************************************************************
//
// Register: CPU_SCS_O_MPU_RASR_A3
//
//*****************************************************************************
// Field:  [31:0] MPU_RASR_A3
//
// Alias for MPU_RASR
#define CPU_SCS_MPU_RASR_A3_MPU_RASR_A3_W                                   32
#define CPU_SCS_MPU_RASR_A3_MPU_RASR_A3_M                           0xFFFFFFFF
#define CPU_SCS_MPU_RASR_A3_MPU_RASR_A3_S                                    0

//*****************************************************************************
//
// Register: CPU_SCS_O_DHCSR
//
//*****************************************************************************
// Field:    [25] S_RESET_ST
//
// Indicates that the core has been reset, or is now being reset, since the
// last time this bit was read. This a sticky bit that clears on read. So,
// reading twice and getting 1 then 0 means it was reset in the past. Reading
// twice and getting 1 both times means that it is being reset now (held in
// reset still).
// When writing to this register, 0 must be written this bit-field, otherwise
// the write operation is ignored and no bits are written into the register.
#define CPU_SCS_DHCSR_S_RESET_ST                                    0x02000000
#define CPU_SCS_DHCSR_S_RESET_ST_BITN                                       25
#define CPU_SCS_DHCSR_S_RESET_ST_M                                  0x02000000
#define CPU_SCS_DHCSR_S_RESET_ST_S                                          25

// Field:    [24] S_RETIRE_ST
//
// Indicates that an instruction has completed since last read. This is a
// sticky bit that clears on read. This determines if the core is stalled on a
// load/store or fetch.
// When writing to this register, 0 must be written this bit-field, otherwise
// the write operation is ignored and no bits are written into the register.
#define CPU_SCS_DHCSR_S_RETIRE_ST                                   0x01000000
#define CPU_SCS_DHCSR_S_RETIRE_ST_BITN                                      24
#define CPU_SCS_DHCSR_S_RETIRE_ST_M                                 0x01000000
#define CPU_SCS_DHCSR_S_RETIRE_ST_S                                         24

// Field:    [19] S_LOCKUP
//
// Reads as one if the core is running (not halted) and a lockup condition is
// present.
// When writing to this register, 1 must be written this bit-field, otherwise
// the write operation is ignored and no bits are written into the register.
#define CPU_SCS_DHCSR_S_LOCKUP                                      0x00080000
#define CPU_SCS_DHCSR_S_LOCKUP_BITN                                         19
#define CPU_SCS_DHCSR_S_LOCKUP_M                                    0x00080000
#define CPU_SCS_DHCSR_S_LOCKUP_S                                            19

// Field:    [18] S_SLEEP
//
// Indicates that the core is sleeping (WFI, WFE, or **SLEEP-ON-EXIT**). Must
// use C_HALT to gain control or wait for interrupt to wake-up.
// When writing to this register, 1 must be written this bit-field, otherwise
// the write operation is ignored and no bits are written into the register.
#define CPU_SCS_DHCSR_S_SLEEP                                       0x00040000
#define CPU_SCS_DHCSR_S_SLEEP_BITN                                          18
#define CPU_SCS_DHCSR_S_SLEEP_M                                     0x00040000
#define CPU_SCS_DHCSR_S_SLEEP_S                                             18

// Field:    [17] S_HALT
//
// The core is in debug state when this bit is set.
// When writing to this register, 1 must be written this bit-field, otherwise
// the write operation is ignored and no bits are written into the register.
#define CPU_SCS_DHCSR_S_HALT                                        0x00020000
#define CPU_SCS_DHCSR_S_HALT_BITN                                           17
#define CPU_SCS_DHCSR_S_HALT_M                                      0x00020000
#define CPU_SCS_DHCSR_S_HALT_S                                              17

// Field:    [16] S_REGRDY
//
// Register Read/Write on the Debug Core Register Selector register is
// available. Last transfer is complete.
// When writing to this register, 1 must be written this bit-field, otherwise
// the write operation is ignored and no bits are written into the register.
#define CPU_SCS_DHCSR_S_REGRDY                                      0x00010000
#define CPU_SCS_DHCSR_S_REGRDY_BITN                                         16
#define CPU_SCS_DHCSR_S_REGRDY_M                                    0x00010000
#define CPU_SCS_DHCSR_S_REGRDY_S                                            16

// Field:     [5] C_SNAPSTALL
//
// If the core is stalled on a load/store operation the stall ceases and the
// instruction is forced to complete. This enables Halting debug to gain
// control of the core. It can only be set if: C_DEBUGEN = 1 and C_HALT = 1.
// The core reads S_RETIRE_ST as 0. This indicates that no instruction has
// advanced. This prevents misuse. The bus state is Unpredictable when this is
// used. S_RETIRE_ST can detect core stalls on load/store operations.
#define CPU_SCS_DHCSR_C_SNAPSTALL                                   0x00000020
#define CPU_SCS_DHCSR_C_SNAPSTALL_BITN                                       5
#define CPU_SCS_DHCSR_C_SNAPSTALL_M                                 0x00000020
#define CPU_SCS_DHCSR_C_SNAPSTALL_S                                          5

// Field:     [3] C_MASKINTS
//
// Mask interrupts when stepping or running in halted debug. This masking does
// not affect NMI, fault exceptions and SVC caused by execution of the
// instructions. This bit must only be modified when the processor is halted
// (S_HALT == 1).  C_MASKINTS must be set or cleared before halt is released
// (i.e., the writes to set or clear C_MASKINTS and to set or clear C_HALT must
// be separate). Modifying C_MASKINTS while the system is running with halting
// debug support enabled (C_DEBUGEN = 1, S_HALT = 0) may cause unpredictable
// behavior.
#define CPU_SCS_DHCSR_C_MASKINTS                                    0x00000008
#define CPU_SCS_DHCSR_C_MASKINTS_BITN                                        3
#define CPU_SCS_DHCSR_C_MASKINTS_M                                  0x00000008
#define CPU_SCS_DHCSR_C_MASKINTS_S                                           3

// Field:     [2] C_STEP
//
// Steps the core in halted debug. When C_DEBUGEN = 0, this bit has no effect.
// Must only be modified when the processor is halted (S_HALT == 1).
// Modifying C_STEP while the system is running with halting debug support
// enabled (C_DEBUGEN = 1, S_HALT = 0) may cause unpredictable behavior.
#define CPU_SCS_DHCSR_C_STEP                                        0x00000004
#define CPU_SCS_DHCSR_C_STEP_BITN                                            2
#define CPU_SCS_DHCSR_C_STEP_M                                      0x00000004
#define CPU_SCS_DHCSR_C_STEP_S                                               2

// Field:     [1] C_HALT
//
// Halts the core. This bit is set automatically when the core Halts. For
// example Breakpoint. This bit clears on core reset.
#define CPU_SCS_DHCSR_C_HALT                                        0x00000002
#define CPU_SCS_DHCSR_C_HALT_BITN                                            1
#define CPU_SCS_DHCSR_C_HALT_M                                      0x00000002
#define CPU_SCS_DHCSR_C_HALT_S                                               1

// Field:     [0] C_DEBUGEN
//
// Enables debug. This can only be written by AHB-AP and not by the core. It is
// ignored when written by the core, which cannot set or clear it. The core
// must write a 1 to it when writing C_HALT to halt itself.
// The values of C_HALT, C_STEP and C_MASKINTS are ignored by hardware when
// C_DEBUGEN = 0. The read values for C_HALT, C_STEP and C_MASKINTS fields will
// be unknown to software when C_DEBUGEN = 0.
#define CPU_SCS_DHCSR_C_DEBUGEN                                     0x00000001
#define CPU_SCS_DHCSR_C_DEBUGEN_BITN                                         0
#define CPU_SCS_DHCSR_C_DEBUGEN_M                                   0x00000001
#define CPU_SCS_DHCSR_C_DEBUGEN_S                                            0

//*****************************************************************************
//
// Register: CPU_SCS_O_DCRSR
//
//*****************************************************************************
// Field:    [16] REGWNR
//
// 1: Write
// 0: Read
#define CPU_SCS_DCRSR_REGWNR                                        0x00010000
#define CPU_SCS_DCRSR_REGWNR_BITN                                           16
#define CPU_SCS_DCRSR_REGWNR_M                                      0x00010000
#define CPU_SCS_DCRSR_REGWNR_S                                              16

// Field:   [4:0] REGSEL
//
// Register select
//
// 0x00: R0
// 0x01: R1
// 0x02: R2
// 0x03: R3
// 0x04: R4
// 0x05: R5
// 0x06: R6
// 0x07: R7
// 0x08: R8
// 0x09: R9
// 0x0A: R10
// 0x0B: R11
// 0x0C: R12
// 0x0D: Current SP
// 0x0E: LR
// 0x0F: DebugReturnAddress
// 0x10: XPSR/flags, execution state information, and exception number
// 0x11: MSP (Main SP)
// 0x12: PSP (Process SP)
// 0x14: CONTROL<<24 | FAULTMASK<<16 | BASEPRI<<8 | PRIMASK
#define CPU_SCS_DCRSR_REGSEL_W                                               5
#define CPU_SCS_DCRSR_REGSEL_M                                      0x0000001F
#define CPU_SCS_DCRSR_REGSEL_S                                               0

//*****************************************************************************
//
// Register: CPU_SCS_O_DCRDR
//
//*****************************************************************************
// Field:  [31:0] DCRDR
//
// This register holds data for reading and writing registers to and from the
// processor. This is the data value written to the register selected by DCRSR.
// When the processor receives a request from DCRSR, this register is read or
// written by the processor using a normal load-store unit operation. If core
// register transfers are not being performed, software-based debug monitors
// can use this register for communication in non-halting debug. This enables
// flags and bits to acknowledge state and indicate if commands have been
// accepted to, replied to, or accepted and replied to.
#define CPU_SCS_DCRDR_DCRDR_W                                               32
#define CPU_SCS_DCRDR_DCRDR_M                                       0xFFFFFFFF
#define CPU_SCS_DCRDR_DCRDR_S                                                0

//*****************************************************************************
//
// Register: CPU_SCS_O_DEMCR
//
//*****************************************************************************
// Field:    [24] TRCENA
//
// This bit must be set to 1 to enable use of the trace and debug blocks: DWT,
// ITM, ETM and TPIU. This enables control of power usage unless tracing is
// required. The application can enable this, for ITM use, or use by a
// debugger.
#define CPU_SCS_DEMCR_TRCENA                                        0x01000000
#define CPU_SCS_DEMCR_TRCENA_BITN                                           24
#define CPU_SCS_DEMCR_TRCENA_M                                      0x01000000
#define CPU_SCS_DEMCR_TRCENA_S                                              24

// Field:    [19] MON_REQ
//
// This enables the monitor to identify how it wakes up. This bit clears on a
// Core Reset.
//
// 0x0: Woken up by debug exception.
// 0x1: Woken up by MON_PEND
#define CPU_SCS_DEMCR_MON_REQ                                       0x00080000
#define CPU_SCS_DEMCR_MON_REQ_BITN                                          19
#define CPU_SCS_DEMCR_MON_REQ_M                                     0x00080000
#define CPU_SCS_DEMCR_MON_REQ_S                                             19

// Field:    [18] MON_STEP
//
// When MON_EN = 1, this steps the core. When MON_EN = 0, this bit is ignored.
// This is the equivalent to DHCSR.C_STEP. Interrupts are only stepped
// according to the priority of the monitor and settings of PRIMASK, FAULTMASK,
// or BASEPRI.
#define CPU_SCS_DEMCR_MON_STEP                                      0x00040000
#define CPU_SCS_DEMCR_MON_STEP_BITN                                         18
#define CPU_SCS_DEMCR_MON_STEP_M                                    0x00040000
#define CPU_SCS_DEMCR_MON_STEP_S                                            18

// Field:    [17] MON_PEND
//
// Pend the monitor to activate when priority permits. This can wake up the
// monitor through the AHB-AP port. It is the equivalent to DHCSR.C_HALT for
// Monitor debug. This register does not reset on a system reset. It is only
// reset by a power-on reset. Software in the reset handler or later, or by the
// DAP must enable the debug monitor.
#define CPU_SCS_DEMCR_MON_PEND                                      0x00020000
#define CPU_SCS_DEMCR_MON_PEND_BITN                                         17
#define CPU_SCS_DEMCR_MON_PEND_M                                    0x00020000
#define CPU_SCS_DEMCR_MON_PEND_S                                            17

// Field:    [16] MON_EN
//
// Enable the debug monitor.
// When enabled, the System handler priority register controls its priority
// level. If disabled, then all debug events go to Hard fault. DHCSR.C_DEBUGEN
// overrides this bit. Vector catching is semi-synchronous. When a matching
// event is seen, a Halt is requested. Because the processor can only halt on
// an instruction boundary, it must wait until the next instruction boundary.
// As a result, it stops on the first instruction of the exception handler.
// However, two special cases exist when a vector catch has triggered: 1. If a
// fault is taken during vectoring, vector read or stack push error, the halt
// occurs on the corresponding fault handler, for the vector error or stack
// push. 2. If a late arriving interrupt comes in during vectoring, it is not
// taken. That is, an implementation that supports the late arrival
// optimization must suppress it in this case.
#define CPU_SCS_DEMCR_MON_EN                                        0x00010000
#define CPU_SCS_DEMCR_MON_EN_BITN                                           16
#define CPU_SCS_DEMCR_MON_EN_M                                      0x00010000
#define CPU_SCS_DEMCR_MON_EN_S                                              16

// Field:    [10] VC_HARDERR
//
// Debug trap on Hard Fault. Ignored when DHCSR.C_DEBUGEN is cleared.
#define CPU_SCS_DEMCR_VC_HARDERR                                    0x00000400
#define CPU_SCS_DEMCR_VC_HARDERR_BITN                                       10
#define CPU_SCS_DEMCR_VC_HARDERR_M                                  0x00000400
#define CPU_SCS_DEMCR_VC_HARDERR_S                                          10

// Field:     [9] VC_INTERR
//
// Debug trap on a fault occurring during an exception entry or return
// sequence. Ignored when DHCSR.C_DEBUGEN is cleared.
#define CPU_SCS_DEMCR_VC_INTERR                                     0x00000200
#define CPU_SCS_DEMCR_VC_INTERR_BITN                                         9
#define CPU_SCS_DEMCR_VC_INTERR_M                                   0x00000200
#define CPU_SCS_DEMCR_VC_INTERR_S                                            9

// Field:     [8] VC_BUSERR
//
// Debug Trap on normal Bus error. Ignored when DHCSR.C_DEBUGEN is cleared.
#define CPU_SCS_DEMCR_VC_BUSERR                                     0x00000100
#define CPU_SCS_DEMCR_VC_BUSERR_BITN                                         8
#define CPU_SCS_DEMCR_VC_BUSERR_M                                   0x00000100
#define CPU_SCS_DEMCR_VC_BUSERR_S                                            8

// Field:     [7] VC_STATERR
//
// Debug trap on Usage Fault state errors. Ignored when DHCSR.C_DEBUGEN is
// cleared.
#define CPU_SCS_DEMCR_VC_STATERR                                    0x00000080
#define CPU_SCS_DEMCR_VC_STATERR_BITN                                        7
#define CPU_SCS_DEMCR_VC_STATERR_M                                  0x00000080
#define CPU_SCS_DEMCR_VC_STATERR_S                                           7

// Field:     [6] VC_CHKERR
//
// Debug trap on Usage Fault enabled checking errors. Ignored when
// DHCSR.C_DEBUGEN is cleared.
#define CPU_SCS_DEMCR_VC_CHKERR                                     0x00000040
#define CPU_SCS_DEMCR_VC_CHKERR_BITN                                         6
#define CPU_SCS_DEMCR_VC_CHKERR_M                                   0x00000040
#define CPU_SCS_DEMCR_VC_CHKERR_S                                            6

// Field:     [5] VC_NOCPERR
//
// Debug trap on a UsageFault access to a Coprocessor. Ignored when
// DHCSR.C_DEBUGEN is cleared.
#define CPU_SCS_DEMCR_VC_NOCPERR                                    0x00000020
#define CPU_SCS_DEMCR_VC_NOCPERR_BITN                                        5
#define CPU_SCS_DEMCR_VC_NOCPERR_M                                  0x00000020
#define CPU_SCS_DEMCR_VC_NOCPERR_S                                           5

// Field:     [4] VC_MMERR
//
// Debug trap on Memory Management faults. Ignored when DHCSR.C_DEBUGEN is
// cleared.
#define CPU_SCS_DEMCR_VC_MMERR                                      0x00000010
#define CPU_SCS_DEMCR_VC_MMERR_BITN                                          4
#define CPU_SCS_DEMCR_VC_MMERR_M                                    0x00000010
#define CPU_SCS_DEMCR_VC_MMERR_S                                             4

// Field:     [0] VC_CORERESET
//
// Reset Vector Catch. Halt running system if Core reset occurs. Ignored when
// DHCSR.C_DEBUGEN is cleared.
#define CPU_SCS_DEMCR_VC_CORERESET                                  0x00000001
#define CPU_SCS_DEMCR_VC_CORERESET_BITN                                      0
#define CPU_SCS_DEMCR_VC_CORERESET_M                                0x00000001
#define CPU_SCS_DEMCR_VC_CORERESET_S                                         0

//*****************************************************************************
//
// Register: CPU_SCS_O_STIR
//
//*****************************************************************************
// Field:   [8:0] INTID
//
// Interrupt ID field. Writing a value to this bit-field is the same as
// manually pending an interrupt by setting the corresponding interrupt bit in
// an Interrupt Set Pending Register in NVIC_ISPR0 or NVIC_ISPR1.
#define CPU_SCS_STIR_INTID_W                                                 9
#define CPU_SCS_STIR_INTID_M                                        0x000001FF
#define CPU_SCS_STIR_INTID_S                                                 0

//*****************************************************************************
//
// Register: CPU_SCS_O_FPCCR
//
//*****************************************************************************
// Field:    [31] ASPEN
//
// Automatic State Preservation enable.
// When this bit is set is will cause bit [2] of the Special CONTROL register
// to be set (FPCA) on execution of a floating point instruction which results
// in the floating point state automatically being preserved on exception
// entry.
#define CPU_SCS_FPCCR_ASPEN                                         0x80000000
#define CPU_SCS_FPCCR_ASPEN_BITN                                            31
#define CPU_SCS_FPCCR_ASPEN_M                                       0x80000000
#define CPU_SCS_FPCCR_ASPEN_S                                               31

// Field:    [30] LSPEN
//
// Lazy State Preservation enable.
// Lazy state preservation is when the processor performs a context save, space
// on the stack is reserved for the floating point state but it is not stacked
// until the new context performs a floating point operation.
// 0: Disable automatic lazy state preservation for floating-point context.
// 1: Enable automatic lazy state preservation for floating-point context.
#define CPU_SCS_FPCCR_LSPEN                                         0x40000000
#define CPU_SCS_FPCCR_LSPEN_BITN                                            30
#define CPU_SCS_FPCCR_LSPEN_M                                       0x40000000
#define CPU_SCS_FPCCR_LSPEN_S                                               30

// Field:     [8] MONRDY
//
// Indicates whether the the software executing when the processor allocated
// the FP stack frame was able to set the DebugMonitor exception to pending.
// 0: DebugMonitor is disabled or priority did not permit setting
// DEMCR.MON_PEND when the floating-point stack frame was allocated.
// 1: DebugMonitor is enabled and priority permits setting DEMCR.MON_PEND when
// the floating-point stack frame was allocated.
#define CPU_SCS_FPCCR_MONRDY                                        0x00000100
#define CPU_SCS_FPCCR_MONRDY_BITN                                            8
#define CPU_SCS_FPCCR_MONRDY_M                                      0x00000100
#define CPU_SCS_FPCCR_MONRDY_S                                               8

// Field:     [6] BFRDY
//
// Indicates whether the software executing when the processor allocated the FP
// stack frame was able to set the BusFault exception to pending.
// 0: BusFault is disabled or priority did not permit setting the BusFault
// handler to the pending state when the floating-point stack frame was
// allocated.
// 1: BusFault is enabled and priority permitted setting the BusFault handler
// to the pending state when the floating-point stack frame was allocated.
#define CPU_SCS_FPCCR_BFRDY                                         0x00000040
#define CPU_SCS_FPCCR_BFRDY_BITN                                             6
#define CPU_SCS_FPCCR_BFRDY_M                                       0x00000040
#define CPU_SCS_FPCCR_BFRDY_S                                                6

// Field:     [5] MMRDY
//
// Indicates whether the software executing when the processor allocated the FP
// stack frame was able to set the MemManage exception to pending.
// 0: MemManage is disabled or priority did not permit setting the MemManage
// handler to the pending state when the floating-point stack frame was
// allocated.
// 1: MemManage is enabled and priority permitted setting the MemManage handler
// to the pending state when the floating-point stack frame was allocated.
#define CPU_SCS_FPCCR_MMRDY                                         0x00000020
#define CPU_SCS_FPCCR_MMRDY_BITN                                             5
#define CPU_SCS_FPCCR_MMRDY_M                                       0x00000020
#define CPU_SCS_FPCCR_MMRDY_S                                                5

// Field:     [4] HFRDY
//
// Indicates whether the software executing when the processor allocated the FP
// stack frame was able to set the HardFault exception to pending.
// 0: Priority did not permit setting the HardFault handler to the pending
// state when the floating-point stack frame was allocated.
// 1: Priority permitted setting the HardFault handler to the pending state
// when the floating-point stack frame was allocated.
#define CPU_SCS_FPCCR_HFRDY                                         0x00000010
#define CPU_SCS_FPCCR_HFRDY_BITN                                             4
#define CPU_SCS_FPCCR_HFRDY_M                                       0x00000010
#define CPU_SCS_FPCCR_HFRDY_S                                                4

// Field:     [3] THREAD
//
// Indicates the processor mode was Thread when it allocated the FP stack
// frame.
// 0: Mode was not Thread Mode when the floating-point stack frame was
// allocated.
// 1: Mode was Thread Mode when the floating-point stack frame was allocated.
#define CPU_SCS_FPCCR_THREAD                                        0x00000008
#define CPU_SCS_FPCCR_THREAD_BITN                                            3
#define CPU_SCS_FPCCR_THREAD_M                                      0x00000008
#define CPU_SCS_FPCCR_THREAD_S                                               3

// Field:     [1] USER
//
// Indicates the privilege level of the software executing was User
// (Unpriviledged) when the processor allocated the FP stack frame:
// 0: Privilege level was not user when the floating-point stack frame was
// allocated.
// 1: Privilege level was user when the floating-point stack frame was
// allocated.
#define CPU_SCS_FPCCR_USER                                          0x00000002
#define CPU_SCS_FPCCR_USER_BITN                                              1
#define CPU_SCS_FPCCR_USER_M                                        0x00000002
#define CPU_SCS_FPCCR_USER_S                                                 1

// Field:     [0] LSPACT
//
// Indicates whether Lazy preservation of the FP state is active:
// 0: Lazy state preservation is not active.
// 1: Lazy state preservation is active. floating-point stack frame has been
// allocated but saving state to it has been deferred.
#define CPU_SCS_FPCCR_LSPACT                                        0x00000001
#define CPU_SCS_FPCCR_LSPACT_BITN                                            0
#define CPU_SCS_FPCCR_LSPACT_M                                      0x00000001
#define CPU_SCS_FPCCR_LSPACT_S                                               0

//*****************************************************************************
//
// Register: CPU_SCS_O_FPCAR
//
//*****************************************************************************
// Field:  [31:2] ADDRESS
//
// Holds the (double-word-aligned) location of the unpopulated floating-point
// register space allocated on an exception stack frame.
#define CPU_SCS_FPCAR_ADDRESS_W                                             30
#define CPU_SCS_FPCAR_ADDRESS_M                                     0xFFFFFFFC
#define CPU_SCS_FPCAR_ADDRESS_S                                              2

//*****************************************************************************
//
// Register: CPU_SCS_O_FPDSCR
//
//*****************************************************************************
// Field:    [26] AHP
//
// Default value for Alternative Half Precision bit. (If this bit is set to 1
// then Alternative half-precision format is selected).
#define CPU_SCS_FPDSCR_AHP                                          0x04000000
#define CPU_SCS_FPDSCR_AHP_BITN                                             26
#define CPU_SCS_FPDSCR_AHP_M                                        0x04000000
#define CPU_SCS_FPDSCR_AHP_S                                                26

// Field:    [25] DN
//
// Default value for Default NaN mode bit. (If this bit is set to 1 then any
// operation involving one or more NaNs returns the Default NaN).
#define CPU_SCS_FPDSCR_DN                                           0x02000000
#define CPU_SCS_FPDSCR_DN_BITN                                              25
#define CPU_SCS_FPDSCR_DN_M                                         0x02000000
#define CPU_SCS_FPDSCR_DN_S                                                 25

// Field:    [24] FZ
//
// Default value for Flush-to-Zero mode bit. (If this bit is set to 1 then
// Flush-to-zero mode is enabled).
#define CPU_SCS_FPDSCR_FZ                                           0x01000000
#define CPU_SCS_FPDSCR_FZ_BITN                                              24
#define CPU_SCS_FPDSCR_FZ_M                                         0x01000000
#define CPU_SCS_FPDSCR_FZ_S                                                 24

// Field: [23:22] RMODE
//
// Default value for Rounding Mode control field. (The encoding for this field
// is:
// 0b00 Round to Nearest (RN) mode
// 0b01 Round towards Plus Infinity (RP) mode
// 0b10 Round towards Minus Infinity (RM) mode
// 0b11 Round towards Zero (RZ) mode.
// The specified rounding mode is used by almost all floating-point
// instructions).
#define CPU_SCS_FPDSCR_RMODE_W                                               2
#define CPU_SCS_FPDSCR_RMODE_M                                      0x00C00000
#define CPU_SCS_FPDSCR_RMODE_S                                              22

//*****************************************************************************
//
// Register: CPU_SCS_O_MVFR0
//
//*****************************************************************************
// Field: [31:28] FP_ROUNDING_MODES
//
// Indicates the rounding modes supported by the FP floating-point hardware.
// The value of this field is: 0b0001 - all rounding modes supported.
#define CPU_SCS_MVFR0_FP_ROUNDING_MODES_W                                    4
#define CPU_SCS_MVFR0_FP_ROUNDING_MODES_M                           0xF0000000
#define CPU_SCS_MVFR0_FP_ROUNDING_MODES_S                                   28

// Field: [27:24] SHORT_VECTORS
//
// Indicates the hardware support for FP short vectors. The value of this field
// is: 0b0000 - not supported.
#define CPU_SCS_MVFR0_SHORT_VECTORS_W                                        4
#define CPU_SCS_MVFR0_SHORT_VECTORS_M                               0x0F000000
#define CPU_SCS_MVFR0_SHORT_VECTORS_S                                       24

// Field: [23:20] SQUARE_ROOT
//
// Indicates the hardware support for FP square root operations. The value of
// this field is: 0b0001 - supported.
#define CPU_SCS_MVFR0_SQUARE_ROOT_W                                          4
#define CPU_SCS_MVFR0_SQUARE_ROOT_M                                 0x00F00000
#define CPU_SCS_MVFR0_SQUARE_ROOT_S                                         20

// Field: [19:16] DIVIDE
//
// Indicates the hardware support for FP divide operations. The value of this
// field is: 0b0001 - supported.
#define CPU_SCS_MVFR0_DIVIDE_W                                               4
#define CPU_SCS_MVFR0_DIVIDE_M                                      0x000F0000
#define CPU_SCS_MVFR0_DIVIDE_S                                              16

// Field: [15:12] FP_EXCEPTION_TRAPPING
//
// Indicates whether the FP hardware implementation supports exception
// trapping. The value of this field is: 0b0000 - not supported.
#define CPU_SCS_MVFR0_FP_EXCEPTION_TRAPPING_W                                4
#define CPU_SCS_MVFR0_FP_EXCEPTION_TRAPPING_M                       0x0000F000
#define CPU_SCS_MVFR0_FP_EXCEPTION_TRAPPING_S                               12

// Field:  [11:8] DOUBLE_PRECISION
//
// Indicates the hardware support for FP double-precision operations. The value
// of this field is: 0b0000 - not supported.
#define CPU_SCS_MVFR0_DOUBLE_PRECISION_W                                     4
#define CPU_SCS_MVFR0_DOUBLE_PRECISION_M                            0x00000F00
#define CPU_SCS_MVFR0_DOUBLE_PRECISION_S                                     8

// Field:   [7:4] SINGLE_PRECISION
//
// Indicates the hardware support for FP single-precision operations. The value
// of this field is: 0b0010 - supported.
#define CPU_SCS_MVFR0_SINGLE_PRECISION_W                                     4
#define CPU_SCS_MVFR0_SINGLE_PRECISION_M                            0x000000F0
#define CPU_SCS_MVFR0_SINGLE_PRECISION_S                                     4

// Field:   [3:0] A_SIMD
//
// Indicates the size of the FP register bank. The value of this field is:
// 0b0001 - supported, 16 x 64-bit registers.
#define CPU_SCS_MVFR0_A_SIMD_W                                               4
#define CPU_SCS_MVFR0_A_SIMD_M                                      0x0000000F
#define CPU_SCS_MVFR0_A_SIMD_S                                               0

//*****************************************************************************
//
// Register: CPU_SCS_O_MVFR1
//
//*****************************************************************************
// Field: [31:28] FP_FUSED_MAC
//
// Indicates whether the FP supports fused multiply accumulate operations. The
// value of this field is: 0b0001 - supported.
#define CPU_SCS_MVFR1_FP_FUSED_MAC_W                                         4
#define CPU_SCS_MVFR1_FP_FUSED_MAC_M                                0xF0000000
#define CPU_SCS_MVFR1_FP_FUSED_MAC_S                                        28

// Field: [27:24] FP_HPFP
//
// Indicates whether the FP supports half-precision floating-point conversion
// operations. The value of this field is: 0b0001 - supported.
#define CPU_SCS_MVFR1_FP_HPFP_W                                              4
#define CPU_SCS_MVFR1_FP_HPFP_M                                     0x0F000000
#define CPU_SCS_MVFR1_FP_HPFP_S                                             24

// Field:   [7:4] D_NAN_MODE
//
// Indicates whether the FP hardware implementation supports only the Default
// NaN mode. The value of this field is: 0b0001 - hardware supports propagation
// of NaN values.
#define CPU_SCS_MVFR1_D_NAN_MODE_W                                           4
#define CPU_SCS_MVFR1_D_NAN_MODE_M                                  0x000000F0
#define CPU_SCS_MVFR1_D_NAN_MODE_S                                           4

// Field:   [3:0] FTZ_MODE
//
// Indicates whether the FP hardware implementation supports only the
// Flush-to-Zero mode of operation. The value of this field is: 0b0001 -
// hardware supports full denormalized number arithmetic.
#define CPU_SCS_MVFR1_FTZ_MODE_W                                             4
#define CPU_SCS_MVFR1_FTZ_MODE_M                                    0x0000000F
#define CPU_SCS_MVFR1_FTZ_MODE_S                                             0


#endif // __CPU_SCS__
