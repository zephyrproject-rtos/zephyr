/******************************************************************************
*  Filename:       hw_cpu_dwt_h
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

#ifndef __HW_CPU_DWT_H__
#define __HW_CPU_DWT_H__

//*****************************************************************************
//
// This section defines the register offsets of
// CPU_DWT component
//
//*****************************************************************************
// Control
#define CPU_DWT_O_CTRL                                              0x00000000

// Current PC Sampler Cycle Count
#define CPU_DWT_O_CYCCNT                                            0x00000004

// CPI Count
#define CPU_DWT_O_CPICNT                                            0x00000008

// Exception Overhead Count
#define CPU_DWT_O_EXCCNT                                            0x0000000C

// Sleep Count
#define CPU_DWT_O_SLEEPCNT                                          0x00000010

// LSU Count
#define CPU_DWT_O_LSUCNT                                            0x00000014

// Fold Count
#define CPU_DWT_O_FOLDCNT                                           0x00000018

// Program Counter Sample
#define CPU_DWT_O_PCSR                                              0x0000001C

// Comparator 0
#define CPU_DWT_O_COMP0                                             0x00000020

// Mask 0
#define CPU_DWT_O_MASK0                                             0x00000024

// Function 0
#define CPU_DWT_O_FUNCTION0                                         0x00000028

// Comparator 1
#define CPU_DWT_O_COMP1                                             0x00000030

// Mask 1
#define CPU_DWT_O_MASK1                                             0x00000034

// Function 1
#define CPU_DWT_O_FUNCTION1                                         0x00000038

// Comparator 2
#define CPU_DWT_O_COMP2                                             0x00000040

// Mask 2
#define CPU_DWT_O_MASK2                                             0x00000044

// Function 2
#define CPU_DWT_O_FUNCTION2                                         0x00000048

// Comparator 3
#define CPU_DWT_O_COMP3                                             0x00000050

// Mask 3
#define CPU_DWT_O_MASK3                                             0x00000054

// Function 3
#define CPU_DWT_O_FUNCTION3                                         0x00000058

//*****************************************************************************
//
// Register: CPU_DWT_O_CTRL
//
//*****************************************************************************
// Field:    [25] NOCYCCNT
//
// When set, CYCCNT is not supported.
#define CPU_DWT_CTRL_NOCYCCNT                                       0x02000000
#define CPU_DWT_CTRL_NOCYCCNT_BITN                                          25
#define CPU_DWT_CTRL_NOCYCCNT_M                                     0x02000000
#define CPU_DWT_CTRL_NOCYCCNT_S                                             25

// Field:    [24] NOPRFCNT
//
// When set, FOLDCNT, LSUCNT, SLEEPCNT, EXCCNT, and CPICNT are not supported.
#define CPU_DWT_CTRL_NOPRFCNT                                       0x01000000
#define CPU_DWT_CTRL_NOPRFCNT_BITN                                          24
#define CPU_DWT_CTRL_NOPRFCNT_M                                     0x01000000
#define CPU_DWT_CTRL_NOPRFCNT_S                                             24

// Field:    [22] CYCEVTENA
//
// Enables Cycle count event. Emits an event when the POSTCNT counter triggers
// it. See CYCTAP and POSTPRESET for details. This event is only emitted if
// PCSAMPLEENA is disabled. PCSAMPLEENA overrides the setting of this bit.
//
// 0: Cycle count events disabled
// 1: Cycle count events enabled
#define CPU_DWT_CTRL_CYCEVTENA                                      0x00400000
#define CPU_DWT_CTRL_CYCEVTENA_BITN                                         22
#define CPU_DWT_CTRL_CYCEVTENA_M                                    0x00400000
#define CPU_DWT_CTRL_CYCEVTENA_S                                            22

// Field:    [21] FOLDEVTENA
//
// Enables Folded instruction count event. Emits an event when FOLDCNT
// overflows (every 256 cycles of folded instructions). A folded instruction is
// one that does not incur even one cycle to execute. For example, an IT
// instruction is folded away and so does not use up one cycle.
//
// 0: Folded instruction count events disabled.
// 1: Folded instruction count events enabled.
#define CPU_DWT_CTRL_FOLDEVTENA                                     0x00200000
#define CPU_DWT_CTRL_FOLDEVTENA_BITN                                        21
#define CPU_DWT_CTRL_FOLDEVTENA_M                                   0x00200000
#define CPU_DWT_CTRL_FOLDEVTENA_S                                           21

// Field:    [20] LSUEVTENA
//
// Enables LSU count event. Emits an event when LSUCNT overflows (every 256
// cycles of LSU operation). LSU counts include all LSU costs after the initial
// cycle for the instruction.
//
// 0: LSU count events disabled.
// 1: LSU count events enabled.
#define CPU_DWT_CTRL_LSUEVTENA                                      0x00100000
#define CPU_DWT_CTRL_LSUEVTENA_BITN                                         20
#define CPU_DWT_CTRL_LSUEVTENA_M                                    0x00100000
#define CPU_DWT_CTRL_LSUEVTENA_S                                            20

// Field:    [19] SLEEPEVTENA
//
// Enables Sleep count event. Emits an event when SLEEPCNT overflows (every 256
// cycles that the processor is sleeping).
//
// 0: Sleep count events disabled.
// 1: Sleep count events enabled.
#define CPU_DWT_CTRL_SLEEPEVTENA                                    0x00080000
#define CPU_DWT_CTRL_SLEEPEVTENA_BITN                                       19
#define CPU_DWT_CTRL_SLEEPEVTENA_M                                  0x00080000
#define CPU_DWT_CTRL_SLEEPEVTENA_S                                          19

// Field:    [18] EXCEVTENA
//
// Enables Interrupt overhead event. Emits an event when EXCCNT overflows
// (every 256 cycles of interrupt overhead).
//
// 0x0: Interrupt overhead event disabled.
// 0x1: Interrupt overhead event enabled.
#define CPU_DWT_CTRL_EXCEVTENA                                      0x00040000
#define CPU_DWT_CTRL_EXCEVTENA_BITN                                         18
#define CPU_DWT_CTRL_EXCEVTENA_M                                    0x00040000
#define CPU_DWT_CTRL_EXCEVTENA_S                                            18

// Field:    [17] CPIEVTENA
//
// Enables CPI count event. Emits an event when CPICNT overflows (every 256
// cycles of multi-cycle instructions).
//
// 0: CPI counter events disabled.
// 1: CPI counter events enabled.
#define CPU_DWT_CTRL_CPIEVTENA                                      0x00020000
#define CPU_DWT_CTRL_CPIEVTENA_BITN                                         17
#define CPU_DWT_CTRL_CPIEVTENA_M                                    0x00020000
#define CPU_DWT_CTRL_CPIEVTENA_S                                            17

// Field:    [16] EXCTRCENA
//
// Enables Interrupt event tracing.
//
// 0: Interrupt event trace disabled.
// 1: Interrupt event trace enabled.
#define CPU_DWT_CTRL_EXCTRCENA                                      0x00010000
#define CPU_DWT_CTRL_EXCTRCENA_BITN                                         16
#define CPU_DWT_CTRL_EXCTRCENA_M                                    0x00010000
#define CPU_DWT_CTRL_EXCTRCENA_S                                            16

// Field:    [12] PCSAMPLEENA
//
// Enables PC Sampling event. A PC sample event is emitted when the POSTCNT
// counter triggers it. See CYCTAP and POSTPRESET for details. Enabling this
// bit overrides CYCEVTENA.
//
// 0: PC Sampling event disabled.
// 1: Sampling event enabled.
#define CPU_DWT_CTRL_PCSAMPLEENA                                    0x00001000
#define CPU_DWT_CTRL_PCSAMPLEENA_BITN                                       12
#define CPU_DWT_CTRL_PCSAMPLEENA_M                                  0x00001000
#define CPU_DWT_CTRL_PCSAMPLEENA_S                                          12

// Field: [11:10] SYNCTAP
//
// Selects a synchronization packet rate. CYCCNTENA and CPU_ITM:TCR.SYNCENA
// must also be enabled for this feature.
// Synchronization packets (if enabled) are generated on tap transitions (0 to1
// or 1 to 0).
// ENUMs:
// BIT28                    Tap at bit 28 of CYCCNT
// BIT26                    Tap at bit 26 of CYCCNT
// BIT24                    Tap at bit 24 of CYCCNT
// DIS                      Disabled. No synchronization packets
#define CPU_DWT_CTRL_SYNCTAP_W                                               2
#define CPU_DWT_CTRL_SYNCTAP_M                                      0x00000C00
#define CPU_DWT_CTRL_SYNCTAP_S                                              10
#define CPU_DWT_CTRL_SYNCTAP_BIT28                                  0x00000C00
#define CPU_DWT_CTRL_SYNCTAP_BIT26                                  0x00000800
#define CPU_DWT_CTRL_SYNCTAP_BIT24                                  0x00000400
#define CPU_DWT_CTRL_SYNCTAP_DIS                                    0x00000000

// Field:     [9] CYCTAP
//
// Selects a tap on CYCCNT. These are spaced at bits [6] and [10]. When the
// selected bit in CYCCNT changes from 0 to 1 or 1 to 0, it emits into the
// POSTCNT, post-scalar counter. That counter then counts down. On a bit change
// when post-scalar is 0, it triggers an event for PC sampling or cycle count
// event (see details in CYCEVTENA).
// ENUMs:
// BIT10                    Selects bit [10] to tap
// BIT6                     Selects bit [6] to tap
#define CPU_DWT_CTRL_CYCTAP                                         0x00000200
#define CPU_DWT_CTRL_CYCTAP_BITN                                             9
#define CPU_DWT_CTRL_CYCTAP_M                                       0x00000200
#define CPU_DWT_CTRL_CYCTAP_S                                                9
#define CPU_DWT_CTRL_CYCTAP_BIT10                                   0x00000200
#define CPU_DWT_CTRL_CYCTAP_BIT6                                    0x00000000

// Field:   [8:5] POSTCNT
//
// Post-scalar counter for CYCTAP. When the selected tapped bit changes from 0
// to 1 or 1 to 0, the post scalar counter is down-counted when not 0. If 0, it
// triggers an event for PCSAMPLEENA or CYCEVTENA use. It also reloads with the
// value from POSTPRESET.
#define CPU_DWT_CTRL_POSTCNT_W                                               4
#define CPU_DWT_CTRL_POSTCNT_M                                      0x000001E0
#define CPU_DWT_CTRL_POSTCNT_S                                               5

// Field:   [4:1] POSTPRESET
//
// Reload value for post-scalar counter POSTCNT. When 0, events are triggered
// on each tap change (a power of 2). If this field has a non-0 value, it forms
// a count-down value, to be reloaded into POSTCNT each time it reaches 0. For
// example, a value 1 in this register means an event is formed every other tap
// change.
#define CPU_DWT_CTRL_POSTPRESET_W                                            4
#define CPU_DWT_CTRL_POSTPRESET_M                                   0x0000001E
#define CPU_DWT_CTRL_POSTPRESET_S                                            1

// Field:     [0] CYCCNTENA
//
// Enable CYCCNT, allowing it to increment and generate synchronization and
// count events. If NOCYCCNT = 1, this bit reads zero and ignore writes.
#define CPU_DWT_CTRL_CYCCNTENA                                      0x00000001
#define CPU_DWT_CTRL_CYCCNTENA_BITN                                          0
#define CPU_DWT_CTRL_CYCCNTENA_M                                    0x00000001
#define CPU_DWT_CTRL_CYCCNTENA_S                                             0

//*****************************************************************************
//
// Register: CPU_DWT_O_CYCCNT
//
//*****************************************************************************
// Field:  [31:0] CYCCNT
//
// Current PC Sampler Cycle Counter count value. When enabled, this counter
// counts the number of core cycles, except when the core is halted. The cycle
// counter is a free running counter, counting upwards (this counter will not
// advance in power modes where free-running clock to CPU stops). It wraps
// around to 0 on overflow. The debugger must initialize this to 0 when first
// enabling.
#define CPU_DWT_CYCCNT_CYCCNT_W                                             32
#define CPU_DWT_CYCCNT_CYCCNT_M                                     0xFFFFFFFF
#define CPU_DWT_CYCCNT_CYCCNT_S                                              0

//*****************************************************************************
//
// Register: CPU_DWT_O_CPICNT
//
//*****************************************************************************
// Field:   [7:0] CPICNT
//
// Current CPI counter value. Increments on the additional cycles (the first
// cycle is not counted) required to execute all instructions except those
// recorded by LSUCNT. This counter also increments on all instruction fetch
// stalls. If CTRL.CPIEVTENA is set, an event is emitted when the counter
// overflows. This counter initializes to 0 when it is enabled using
// CTRL.CPIEVTENA.
#define CPU_DWT_CPICNT_CPICNT_W                                              8
#define CPU_DWT_CPICNT_CPICNT_M                                     0x000000FF
#define CPU_DWT_CPICNT_CPICNT_S                                              0

//*****************************************************************************
//
// Register: CPU_DWT_O_EXCCNT
//
//*****************************************************************************
// Field:   [7:0] EXCCNT
//
// Current interrupt overhead counter value. Counts the total cycles spent in
// interrupt processing (for example entry stacking, return unstacking,
// pre-emption). An event is emitted on counter overflow (every 256 cycles).
// This counter initializes to 0 when it is enabled using CTRL.EXCEVTENA.
#define CPU_DWT_EXCCNT_EXCCNT_W                                              8
#define CPU_DWT_EXCCNT_EXCCNT_M                                     0x000000FF
#define CPU_DWT_EXCCNT_EXCCNT_S                                              0

//*****************************************************************************
//
// Register: CPU_DWT_O_SLEEPCNT
//
//*****************************************************************************
// Field:   [7:0] SLEEPCNT
//
// Sleep counter. Counts the number of cycles during which the processor is
// sleeping. An event is emitted on counter overflow (every 256 cycles). This
// counter initializes to 0 when it is enabled using CTRL.SLEEPEVTENA. Note
// that the sleep counter is clocked using CPU's free-running clock. In some
// power modes the free-running clock to CPU is gated to minimize power
// consumption. This means that the sleep counter will be invalid in these
// power modes.
#define CPU_DWT_SLEEPCNT_SLEEPCNT_W                                          8
#define CPU_DWT_SLEEPCNT_SLEEPCNT_M                                 0x000000FF
#define CPU_DWT_SLEEPCNT_SLEEPCNT_S                                          0

//*****************************************************************************
//
// Register: CPU_DWT_O_LSUCNT
//
//*****************************************************************************
// Field:   [7:0] LSUCNT
//
// LSU counter. This counts the total number of cycles that the processor is
// processing an LSU operation. The initial execution cost of the instruction
// is not counted. For example, an LDR that takes two cycles to complete
// increments this counter one cycle. Equivalently, an LDR that stalls for two
// cycles (i.e. takes four cycles to execute), increments this counter three
// times. An event is emitted on counter overflow (every 256 cycles). This
// counter initializes to 0 when it is enabled using CTRL.LSUEVTENA.
#define CPU_DWT_LSUCNT_LSUCNT_W                                              8
#define CPU_DWT_LSUCNT_LSUCNT_M                                     0x000000FF
#define CPU_DWT_LSUCNT_LSUCNT_S                                              0

//*****************************************************************************
//
// Register: CPU_DWT_O_FOLDCNT
//
//*****************************************************************************
// Field:   [7:0] FOLDCNT
//
// This counts the total number folded instructions. This counter initializes
// to 0 when it is enabled using CTRL.FOLDEVTENA.
#define CPU_DWT_FOLDCNT_FOLDCNT_W                                            8
#define CPU_DWT_FOLDCNT_FOLDCNT_M                                   0x000000FF
#define CPU_DWT_FOLDCNT_FOLDCNT_S                                            0

//*****************************************************************************
//
// Register: CPU_DWT_O_PCSR
//
//*****************************************************************************
// Field:  [31:0] EIASAMPLE
//
// Execution instruction address sample, or 0xFFFFFFFF if the core is halted.
#define CPU_DWT_PCSR_EIASAMPLE_W                                            32
#define CPU_DWT_PCSR_EIASAMPLE_M                                    0xFFFFFFFF
#define CPU_DWT_PCSR_EIASAMPLE_S                                             0

//*****************************************************************************
//
// Register: CPU_DWT_O_COMP0
//
//*****************************************************************************
// Field:  [31:0] COMP
//
// Reference value to compare against PC or the data address as given by
// FUNCTION0. Comparator 0 can also compare against the value of the PC Sampler
// Counter (CYCCNT).
#define CPU_DWT_COMP0_COMP_W                                                32
#define CPU_DWT_COMP0_COMP_M                                        0xFFFFFFFF
#define CPU_DWT_COMP0_COMP_S                                                 0

//*****************************************************************************
//
// Register: CPU_DWT_O_MASK0
//
//*****************************************************************************
// Field:   [3:0] MASK
//
// Mask on data address when matching against COMP0. This is the size of the
// ignore mask. That is, DWT matching is performed as:(ADDR ANDed with (0xFFFF
// left bit-shifted by MASK)) == COMP0. However, the actual comparison is
// slightly more complex to enable matching an address wherever it appears on a
// bus. So, if COMP0 is 3, this matches a word access of 0, because 3 would be
// within the word.
#define CPU_DWT_MASK0_MASK_W                                                 4
#define CPU_DWT_MASK0_MASK_M                                        0x0000000F
#define CPU_DWT_MASK0_MASK_S                                                 0

//*****************************************************************************
//
// Register: CPU_DWT_O_FUNCTION0
//
//*****************************************************************************
// Field:    [24] MATCHED
//
// This bit is set when the comparator matches, and indicates that the
// operation defined by FUNCTION has occurred since this bit was last read.
// This bit is cleared on read.
#define CPU_DWT_FUNCTION0_MATCHED                                   0x01000000
#define CPU_DWT_FUNCTION0_MATCHED_BITN                                      24
#define CPU_DWT_FUNCTION0_MATCHED_M                                 0x01000000
#define CPU_DWT_FUNCTION0_MATCHED_S                                         24

// Field:     [7] CYCMATCH
//
// This bit is only available in comparator 0. When set, COMP0 will compare
// against the cycle counter (CYCCNT).
#define CPU_DWT_FUNCTION0_CYCMATCH                                  0x00000080
#define CPU_DWT_FUNCTION0_CYCMATCH_BITN                                      7
#define CPU_DWT_FUNCTION0_CYCMATCH_M                                0x00000080
#define CPU_DWT_FUNCTION0_CYCMATCH_S                                         7

// Field:     [5] EMITRANGE
//
// Emit range field. This bit permits emitting offset when range match occurs.
// PC sampling is not supported when emit range is enabled.
// This field only applies for: FUNCTION = 1, 2, 3, 12, 13, 14, and 15.
#define CPU_DWT_FUNCTION0_EMITRANGE                                 0x00000020
#define CPU_DWT_FUNCTION0_EMITRANGE_BITN                                     5
#define CPU_DWT_FUNCTION0_EMITRANGE_M                               0x00000020
#define CPU_DWT_FUNCTION0_EMITRANGE_S                                        5

// Field:   [3:0] FUNCTION
//
// Function settings.
//
// 0x0: Disabled
// 0x1: EMITRANGE = 0, sample and emit PC through ITM. EMITRANGE = 1, emit
// address offset through ITM
// 0x2: EMITRANGE = 0, emit data through ITM on read and write. EMITRANGE = 1,
// emit data and address offset through ITM on read or write.
// 0x3: EMITRANGE = 0, sample PC and data value through ITM on read or write.
// EMITRANGE = 1, emit address offset and data value through ITM on read or
// write.
// 0x4: Watchpoint on PC match.
// 0x5: Watchpoint on read.
// 0x6: Watchpoint on write.
// 0x7: Watchpoint on read or write.
// 0x8: ETM trigger on PC match
// 0x9: ETM trigger on read
// 0xA: ETM trigger on write
// 0xB: ETM trigger on read or write
// 0xC: EMITRANGE = 0, sample data for read transfers. EMITRANGE = 1, sample
// Daddr (lower 16 bits) for read transfers
// 0xD: EMITRANGE = 0, sample data for write transfers. EMITRANGE = 1, sample
// Daddr (lower 16 bits) for write transfers
// 0xE: EMITRANGE = 0, sample PC + data for read transfers. EMITRANGE = 1,
// sample Daddr (lower 16 bits) + data for read transfers
// 0xF: EMITRANGE = 0, sample PC + data for write transfers. EMITRANGE = 1,
// sample Daddr (lower 16 bits) + data for write transfers
//
// Note 1: If the ETM is not fitted, then ETM trigger is not possible.
// Note 2: Data value is only sampled for accesses that do not fault (MPU or
// bus fault). The PC is sampled irrespective of any faults. The PC is only
// sampled for the first address of a burst.
// Note 3: PC match is not recommended for watchpoints because it stops after
// the instruction. It mainly guards and triggers the ETM.
#define CPU_DWT_FUNCTION0_FUNCTION_W                                         4
#define CPU_DWT_FUNCTION0_FUNCTION_M                                0x0000000F
#define CPU_DWT_FUNCTION0_FUNCTION_S                                         0

//*****************************************************************************
//
// Register: CPU_DWT_O_COMP1
//
//*****************************************************************************
// Field:  [31:0] COMP
//
// Reference value to compare against PC or the data address as given by
// FUNCTION1.
// Comparator 1 can also compare data values. So this register can contain
// reference values for data matching.
#define CPU_DWT_COMP1_COMP_W                                                32
#define CPU_DWT_COMP1_COMP_M                                        0xFFFFFFFF
#define CPU_DWT_COMP1_COMP_S                                                 0

//*****************************************************************************
//
// Register: CPU_DWT_O_MASK1
//
//*****************************************************************************
// Field:   [3:0] MASK
//
// Mask on data address when matching against COMP1. This is the size of the
// ignore mask. That is, DWT matching is performed as:(ADDR ANDed with (0xFFFF
// left bit-shifted by MASK)) == COMP1. However, the actual comparison is
// slightly more complex to enable matching an address wherever it appears on a
// bus. So, if COMP1 is 3, this matches a word access of 0, because 3 would be
// within the word.
#define CPU_DWT_MASK1_MASK_W                                                 4
#define CPU_DWT_MASK1_MASK_M                                        0x0000000F
#define CPU_DWT_MASK1_MASK_S                                                 0

//*****************************************************************************
//
// Register: CPU_DWT_O_FUNCTION1
//
//*****************************************************************************
// Field:    [24] MATCHED
//
// This bit is set when the comparator matches, and indicates that the
// operation defined by FUNCTION has occurred since this bit was last read.
// This bit is cleared on read.
#define CPU_DWT_FUNCTION1_MATCHED                                   0x01000000
#define CPU_DWT_FUNCTION1_MATCHED_BITN                                      24
#define CPU_DWT_FUNCTION1_MATCHED_M                                 0x01000000
#define CPU_DWT_FUNCTION1_MATCHED_S                                         24

// Field: [19:16] DATAVADDR1
//
// Identity of a second linked address comparator for data value matching when
// DATAVMATCH == 1 and LNK1ENA == 1.
#define CPU_DWT_FUNCTION1_DATAVADDR1_W                                       4
#define CPU_DWT_FUNCTION1_DATAVADDR1_M                              0x000F0000
#define CPU_DWT_FUNCTION1_DATAVADDR1_S                                      16

// Field: [15:12] DATAVADDR0
//
// Identity of a linked address comparator for data value matching when
// DATAVMATCH == 1.
#define CPU_DWT_FUNCTION1_DATAVADDR0_W                                       4
#define CPU_DWT_FUNCTION1_DATAVADDR0_M                              0x0000F000
#define CPU_DWT_FUNCTION1_DATAVADDR0_S                                      12

// Field: [11:10] DATAVSIZE
//
// Defines the size of the data in the COMP1 register that is to be matched:
//
// 0x0: Byte
// 0x1: Halfword
// 0x2: Word
// 0x3: Unpredictable.
#define CPU_DWT_FUNCTION1_DATAVSIZE_W                                        2
#define CPU_DWT_FUNCTION1_DATAVSIZE_M                               0x00000C00
#define CPU_DWT_FUNCTION1_DATAVSIZE_S                                       10

// Field:     [9] LNK1ENA
//
// Read only bit-field only supported in comparator 1.
//
// 0: DATAVADDR1 not supported
// 1: DATAVADDR1 supported (enabled)
#define CPU_DWT_FUNCTION1_LNK1ENA                                   0x00000200
#define CPU_DWT_FUNCTION1_LNK1ENA_BITN                                       9
#define CPU_DWT_FUNCTION1_LNK1ENA_M                                 0x00000200
#define CPU_DWT_FUNCTION1_LNK1ENA_S                                          9

// Field:     [8] DATAVMATCH
//
// Data match feature:
//
// 0: Perform address comparison
// 1: Perform data value compare. The comparators given by DATAVADDR0 and
// DATAVADDR1 provide the address for the data comparison. The FUNCTION setting
// for the comparators given by DATAVADDR0 and DATAVADDR1 are overridden and
// those comparators only provide the address match for the data comparison.
//
// This bit is only available in comparator 1.
#define CPU_DWT_FUNCTION1_DATAVMATCH                                0x00000100
#define CPU_DWT_FUNCTION1_DATAVMATCH_BITN                                    8
#define CPU_DWT_FUNCTION1_DATAVMATCH_M                              0x00000100
#define CPU_DWT_FUNCTION1_DATAVMATCH_S                                       8

// Field:     [5] EMITRANGE
//
// Emit range field. This bit permits emitting offset when range match occurs.
// PC sampling is not supported when emit range is enabled.
// This field only applies for: FUNCTION = 1, 2, 3, 12, 13, 14, and 15.
#define CPU_DWT_FUNCTION1_EMITRANGE                                 0x00000020
#define CPU_DWT_FUNCTION1_EMITRANGE_BITN                                     5
#define CPU_DWT_FUNCTION1_EMITRANGE_M                               0x00000020
#define CPU_DWT_FUNCTION1_EMITRANGE_S                                        5

// Field:   [3:0] FUNCTION
//
// Function settings:
//
// 0x0: Disabled
// 0x1: EMITRANGE = 0, sample and emit PC through ITM. EMITRANGE = 1, emit
// address offset through ITM
// 0x2: EMITRANGE = 0, emit data through ITM on read and write. EMITRANGE = 1,
// emit data and address offset through ITM on read or write.
// 0x3: EMITRANGE = 0, sample PC and data value through ITM on read or write.
// EMITRANGE = 1, emit address offset and data value through ITM on read or
// write.
// 0x4: Watchpoint on PC match.
// 0x5: Watchpoint on read.
// 0x6: Watchpoint on write.
// 0x7: Watchpoint on read or write.
// 0x8: ETM trigger on PC match
// 0x9: ETM trigger on read
// 0xA: ETM trigger on write
// 0xB: ETM trigger on read or write
// 0xC: EMITRANGE = 0, sample data for read transfers. EMITRANGE = 1, sample
// Daddr (lower 16 bits) for read transfers
// 0xD: EMITRANGE = 0, sample data for write transfers. EMITRANGE = 1, sample
// Daddr (lower 16 bits) for write transfers
// 0xE: EMITRANGE = 0, sample PC + data for read transfers. EMITRANGE = 1,
// sample Daddr (lower 16 bits) + data for read transfers
// 0xF: EMITRANGE = 0, sample PC + data for write transfers. EMITRANGE = 1,
// sample Daddr (lower 16 bits) + data for write transfers
//
// Note 1: If the ETM is not fitted, then ETM trigger is not possible.
// Note 2: Data value is only sampled for accesses that do not fault (MPU or
// bus fault). The PC is sampled irrespective of any faults. The PC is only
// sampled for the first address of a burst.
// Note 3: FUNCTION is overridden for comparators given by DATAVADDR0 and
// DATAVADDR1 if DATAVMATCH is also set. The comparators given by DATAVADDR0
// and DATAVADDR1 can then only perform address comparator matches for
// comparator 1 data matches.
// Note 4: If the data matching functionality is not included during
// implementation it is not possible to set DATAVADDR0, DATAVADDR1, or
// DATAVMATCH. This means that the data matching functionality is not available
// in the implementation. Test the availability of data matching by writing and
// reading DATAVMATCH. If it is not settable then data matching is unavailable.
// Note 5: PC match is not recommended for watchpoints because it stops after
// the instruction. It mainly guards and triggers the ETM.
#define CPU_DWT_FUNCTION1_FUNCTION_W                                         4
#define CPU_DWT_FUNCTION1_FUNCTION_M                                0x0000000F
#define CPU_DWT_FUNCTION1_FUNCTION_S                                         0

//*****************************************************************************
//
// Register: CPU_DWT_O_COMP2
//
//*****************************************************************************
// Field:  [31:0] COMP
//
// Reference value to compare against PC or the data address as given by
// FUNCTION2.
#define CPU_DWT_COMP2_COMP_W                                                32
#define CPU_DWT_COMP2_COMP_M                                        0xFFFFFFFF
#define CPU_DWT_COMP2_COMP_S                                                 0

//*****************************************************************************
//
// Register: CPU_DWT_O_MASK2
//
//*****************************************************************************
// Field:   [3:0] MASK
//
// Mask on data address when matching against COMP2. This is the size of the
// ignore mask. That is, DWT matching is performed as:(ADDR ANDed with (0xFFFF
// left bit-shifted by MASK)) == COMP2. However, the actual comparison is
// slightly more complex to enable matching an address wherever it appears on a
// bus. So, if COMP2 is 3, this matches a word access of 0, because 3 would be
// within the word.
#define CPU_DWT_MASK2_MASK_W                                                 4
#define CPU_DWT_MASK2_MASK_M                                        0x0000000F
#define CPU_DWT_MASK2_MASK_S                                                 0

//*****************************************************************************
//
// Register: CPU_DWT_O_FUNCTION2
//
//*****************************************************************************
// Field:    [24] MATCHED
//
// This bit is set when the comparator matches, and indicates that the
// operation defined by FUNCTION has occurred since this bit was last read.
// This bit is cleared on read.
#define CPU_DWT_FUNCTION2_MATCHED                                   0x01000000
#define CPU_DWT_FUNCTION2_MATCHED_BITN                                      24
#define CPU_DWT_FUNCTION2_MATCHED_M                                 0x01000000
#define CPU_DWT_FUNCTION2_MATCHED_S                                         24

// Field:     [5] EMITRANGE
//
// Emit range field. This bit permits emitting offset when range match occurs.
// PC sampling is not supported when emit range is enabled.
// This field only applies for: FUNCTION = 1, 2, 3, 12, 13, 14, and 15.
#define CPU_DWT_FUNCTION2_EMITRANGE                                 0x00000020
#define CPU_DWT_FUNCTION2_EMITRANGE_BITN                                     5
#define CPU_DWT_FUNCTION2_EMITRANGE_M                               0x00000020
#define CPU_DWT_FUNCTION2_EMITRANGE_S                                        5

// Field:   [3:0] FUNCTION
//
// Function settings.
//
// 0x0: Disabled
// 0x1: EMITRANGE = 0, sample and emit PC through ITM. EMITRANGE = 1, emit
// address offset through ITM
// 0x2: EMITRANGE = 0, emit data through ITM on read and write. EMITRANGE = 1,
// emit data and address offset through ITM on read or write.
// 0x3: EMITRANGE = 0, sample PC and data value through ITM on read or write.
// EMITRANGE = 1, emit address offset and data value through ITM on read or
// write.
// 0x4: Watchpoint on PC match.
// 0x5: Watchpoint on read.
// 0x6: Watchpoint on write.
// 0x7: Watchpoint on read or write.
// 0x8: ETM trigger on PC match
// 0x9: ETM trigger on read
// 0xA: ETM trigger on write
// 0xB: ETM trigger on read or write
// 0xC: EMITRANGE = 0, sample data for read transfers. EMITRANGE = 1, sample
// Daddr (lower 16 bits) for read transfers
// 0xD: EMITRANGE = 0, sample data for write transfers. EMITRANGE = 1, sample
// Daddr (lower 16 bits) for write transfers
// 0xE: EMITRANGE = 0, sample PC + data for read transfers. EMITRANGE = 1,
// sample Daddr (lower 16 bits) + data for read transfers
// 0xF: EMITRANGE = 0, sample PC + data for write transfers. EMITRANGE = 1,
// sample Daddr (lower 16 bits) + data for write transfers
//
// Note 1: If the ETM is not fitted, then ETM trigger is not possible.
// Note 2: Data value is only sampled for accesses that do not fault (MPU or
// bus fault). The PC is sampled irrespective of any faults. The PC is only
// sampled for the first address of a burst.
// Note 3: PC match is not recommended for watchpoints because it stops after
// the instruction. It mainly guards and triggers the ETM.
#define CPU_DWT_FUNCTION2_FUNCTION_W                                         4
#define CPU_DWT_FUNCTION2_FUNCTION_M                                0x0000000F
#define CPU_DWT_FUNCTION2_FUNCTION_S                                         0

//*****************************************************************************
//
// Register: CPU_DWT_O_COMP3
//
//*****************************************************************************
// Field:  [31:0] COMP
//
// Reference value to compare against PC or the data address as given by
// FUNCTION3.
#define CPU_DWT_COMP3_COMP_W                                                32
#define CPU_DWT_COMP3_COMP_M                                        0xFFFFFFFF
#define CPU_DWT_COMP3_COMP_S                                                 0

//*****************************************************************************
//
// Register: CPU_DWT_O_MASK3
//
//*****************************************************************************
// Field:   [3:0] MASK
//
// Mask on data address when matching against COMP3. This is the size of the
// ignore mask. That is, DWT matching is performed as:(ADDR ANDed with (0xFFFF
// left bit-shifted by MASK)) == COMP3. However, the actual comparison is
// slightly more complex to enable matching an address wherever it appears on a
// bus. So, if COMP3 is 3, this matches a word access of 0, because 3 would be
// within the word.
#define CPU_DWT_MASK3_MASK_W                                                 4
#define CPU_DWT_MASK3_MASK_M                                        0x0000000F
#define CPU_DWT_MASK3_MASK_S                                                 0

//*****************************************************************************
//
// Register: CPU_DWT_O_FUNCTION3
//
//*****************************************************************************
// Field:    [24] MATCHED
//
// This bit is set when the comparator matches, and indicates that the
// operation defined by FUNCTION has occurred since this bit was last read.
// This bit is cleared on read.
#define CPU_DWT_FUNCTION3_MATCHED                                   0x01000000
#define CPU_DWT_FUNCTION3_MATCHED_BITN                                      24
#define CPU_DWT_FUNCTION3_MATCHED_M                                 0x01000000
#define CPU_DWT_FUNCTION3_MATCHED_S                                         24

// Field:     [5] EMITRANGE
//
// Emit range field. This bit permits emitting offset when range match occurs.
// PC sampling is not supported when emit range is enabled.
// This field only applies for: FUNCTION = 1, 2, 3, 12, 13, 14, and 15.
#define CPU_DWT_FUNCTION3_EMITRANGE                                 0x00000020
#define CPU_DWT_FUNCTION3_EMITRANGE_BITN                                     5
#define CPU_DWT_FUNCTION3_EMITRANGE_M                               0x00000020
#define CPU_DWT_FUNCTION3_EMITRANGE_S                                        5

// Field:   [3:0] FUNCTION
//
// Function settings.
//
// 0x0: Disabled
// 0x1: EMITRANGE = 0, sample and emit PC through ITM. EMITRANGE = 1, emit
// address offset through ITM
// 0x2: EMITRANGE = 0, emit data through ITM on read and write. EMITRANGE = 1,
// emit data and address offset through ITM on read or write.
// 0x3: EMITRANGE = 0, sample PC and data value through ITM on read or write.
// EMITRANGE = 1, emit address offset and data value through ITM on read or
// write.
// 0x4: Watchpoint on PC match.
// 0x5: Watchpoint on read.
// 0x6: Watchpoint on write.
// 0x7: Watchpoint on read or write.
// 0x8: ETM trigger on PC match
// 0x9: ETM trigger on read
// 0xA: ETM trigger on write
// 0xB: ETM trigger on read or write
// 0xC: EMITRANGE = 0, sample data for read transfers. EMITRANGE = 1, sample
// Daddr (lower 16 bits) for read transfers
// 0xD: EMITRANGE = 0, sample data for write transfers. EMITRANGE = 1, sample
// Daddr (lower 16 bits) for write transfers
// 0xE: EMITRANGE = 0, sample PC + data for read transfers. EMITRANGE = 1,
// sample Daddr (lower 16 bits) + data for read transfers
// 0xF: EMITRANGE = 0, sample PC + data for write transfers. EMITRANGE = 1,
// sample Daddr (lower 16 bits) + data for write transfers
//
// Note 1: If the ETM is not fitted, then ETM trigger is not possible.
// Note 2: Data value is only sampled for accesses that do not fault (MPU or
// bus fault). The PC is sampled irrespective of any faults. The PC is only
// sampled for the first address of a burst.
// Note 3: PC match is not recommended for watchpoints because it stops after
// the instruction. It mainly guards and triggers the ETM.
#define CPU_DWT_FUNCTION3_FUNCTION_W                                         4
#define CPU_DWT_FUNCTION3_FUNCTION_M                                0x0000000F
#define CPU_DWT_FUNCTION3_FUNCTION_S                                         0


#endif // __CPU_DWT__
