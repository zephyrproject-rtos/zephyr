/******************************************************************************
*  Filename:       hw_cpu_tpiu_h
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

#ifndef __HW_CPU_TPIU_H__
#define __HW_CPU_TPIU_H__

//*****************************************************************************
//
// This section defines the register offsets of
// CPU_TPIU component
//
//*****************************************************************************
// Supported Sync Port Sizes
#define CPU_TPIU_O_SSPSR                                            0x00000000

// Current Sync Port Size
#define CPU_TPIU_O_CSPSR                                            0x00000004

// Async Clock Prescaler
#define CPU_TPIU_O_ACPR                                             0x00000010

// Selected Pin Protocol
#define CPU_TPIU_O_SPPR                                             0x000000F0

// Formatter and Flush Status
#define CPU_TPIU_O_FFSR                                             0x00000300

// Formatter and Flush Control
#define CPU_TPIU_O_FFCR                                             0x00000304

// Formatter Synchronization Counter
#define CPU_TPIU_O_FSCR                                             0x00000308

// Claim Tag Mask
#define CPU_TPIU_O_CLAIMMASK                                        0x00000FA0

// Claim Tag Set
#define CPU_TPIU_O_CLAIMSET                                         0x00000FA0

// Current Claim Tag
#define CPU_TPIU_O_CLAIMTAG                                         0x00000FA4

// Claim Tag Clear
#define CPU_TPIU_O_CLAIMCLR                                         0x00000FA4

// Device ID
#define CPU_TPIU_O_DEVID                                            0x00000FC8

//*****************************************************************************
//
// Register: CPU_TPIU_O_SSPSR
//
//*****************************************************************************
// Field:     [3] FOUR
//
// 4-bit port size support
//
// 0x0: Not supported
// 0x1: Supported
#define CPU_TPIU_SSPSR_FOUR                                         0x00000008
#define CPU_TPIU_SSPSR_FOUR_BITN                                             3
#define CPU_TPIU_SSPSR_FOUR_M                                       0x00000008
#define CPU_TPIU_SSPSR_FOUR_S                                                3

// Field:     [2] THREE
//
// 3-bit port size support
//
// 0x0: Not supported
// 0x1: Supported
#define CPU_TPIU_SSPSR_THREE                                        0x00000004
#define CPU_TPIU_SSPSR_THREE_BITN                                            2
#define CPU_TPIU_SSPSR_THREE_M                                      0x00000004
#define CPU_TPIU_SSPSR_THREE_S                                               2

// Field:     [1] TWO
//
// 2-bit port size support
//
// 0x0: Not supported
// 0x1: Supported
#define CPU_TPIU_SSPSR_TWO                                          0x00000002
#define CPU_TPIU_SSPSR_TWO_BITN                                              1
#define CPU_TPIU_SSPSR_TWO_M                                        0x00000002
#define CPU_TPIU_SSPSR_TWO_S                                                 1

// Field:     [0] ONE
//
// 1-bit port size support
//
// 0x0: Not supported
// 0x1: Supported
#define CPU_TPIU_SSPSR_ONE                                          0x00000001
#define CPU_TPIU_SSPSR_ONE_BITN                                              0
#define CPU_TPIU_SSPSR_ONE_M                                        0x00000001
#define CPU_TPIU_SSPSR_ONE_S                                                 0

//*****************************************************************************
//
// Register: CPU_TPIU_O_CSPSR
//
//*****************************************************************************
// Field:     [3] FOUR
//
// 4-bit port enable
// Writing values with more than one bit set in CSPSR, or setting a bit that is
// not indicated as supported in SSPSR can cause Unpredictable behavior.
#define CPU_TPIU_CSPSR_FOUR                                         0x00000008
#define CPU_TPIU_CSPSR_FOUR_BITN                                             3
#define CPU_TPIU_CSPSR_FOUR_M                                       0x00000008
#define CPU_TPIU_CSPSR_FOUR_S                                                3

// Field:     [2] THREE
//
// 3-bit port enable
// Writing values with more than one bit set in CSPSR, or setting a bit that is
// not indicated as supported in SSPSR can cause Unpredictable behavior.
#define CPU_TPIU_CSPSR_THREE                                        0x00000004
#define CPU_TPIU_CSPSR_THREE_BITN                                            2
#define CPU_TPIU_CSPSR_THREE_M                                      0x00000004
#define CPU_TPIU_CSPSR_THREE_S                                               2

// Field:     [1] TWO
//
// 2-bit port enable
// Writing values with more than one bit set in CSPSR, or setting a bit that is
// not indicated as supported in SSPSR can cause Unpredictable behavior.
#define CPU_TPIU_CSPSR_TWO                                          0x00000002
#define CPU_TPIU_CSPSR_TWO_BITN                                              1
#define CPU_TPIU_CSPSR_TWO_M                                        0x00000002
#define CPU_TPIU_CSPSR_TWO_S                                                 1

// Field:     [0] ONE
//
// 1-bit port enable
// Writing values with more than one bit set in CSPSR, or setting a bit that is
// not indicated as supported in SSPSR can cause Unpredictable behavior.
#define CPU_TPIU_CSPSR_ONE                                          0x00000001
#define CPU_TPIU_CSPSR_ONE_BITN                                              0
#define CPU_TPIU_CSPSR_ONE_M                                        0x00000001
#define CPU_TPIU_CSPSR_ONE_S                                                 0

//*****************************************************************************
//
// Register: CPU_TPIU_O_ACPR
//
//*****************************************************************************
// Field:  [12:0] PRESCALER
//
// Divisor for input trace clock is (PRESCALER + 1).
#define CPU_TPIU_ACPR_PRESCALER_W                                           13
#define CPU_TPIU_ACPR_PRESCALER_M                                   0x00001FFF
#define CPU_TPIU_ACPR_PRESCALER_S                                            0

//*****************************************************************************
//
// Register: CPU_TPIU_O_SPPR
//
//*****************************************************************************
// Field:   [1:0] PROTOCOL
//
// Trace output protocol
// ENUMs:
// SWO_NRZ                  SerialWire Output (NRZ)
// SWO_MANCHESTER           SerialWire Output (Manchester). This is the reset
//                          value.
// TRACEPORT                TracePort mode
#define CPU_TPIU_SPPR_PROTOCOL_W                                             2
#define CPU_TPIU_SPPR_PROTOCOL_M                                    0x00000003
#define CPU_TPIU_SPPR_PROTOCOL_S                                             0
#define CPU_TPIU_SPPR_PROTOCOL_SWO_NRZ                              0x00000002
#define CPU_TPIU_SPPR_PROTOCOL_SWO_MANCHESTER                       0x00000001
#define CPU_TPIU_SPPR_PROTOCOL_TRACEPORT                            0x00000000

//*****************************************************************************
//
// Register: CPU_TPIU_O_FFSR
//
//*****************************************************************************
// Field:     [3] FTNONSTOP
//
// 0: Formatter can be stopped
// 1: Formatter cannot be stopped
#define CPU_TPIU_FFSR_FTNONSTOP                                     0x00000008
#define CPU_TPIU_FFSR_FTNONSTOP_BITN                                         3
#define CPU_TPIU_FFSR_FTNONSTOP_M                                   0x00000008
#define CPU_TPIU_FFSR_FTNONSTOP_S                                            3

//*****************************************************************************
//
// Register: CPU_TPIU_O_FFCR
//
//*****************************************************************************
// Field:     [8] TRIGIN
//
// Indicates that triggers are inserted when a trigger pin is asserted.
#define CPU_TPIU_FFCR_TRIGIN                                        0x00000100
#define CPU_TPIU_FFCR_TRIGIN_BITN                                            8
#define CPU_TPIU_FFCR_TRIGIN_M                                      0x00000100
#define CPU_TPIU_FFCR_TRIGIN_S                                               8

// Field:     [1] ENFCONT
//
// Enable continuous formatting:
//
// 0: Continuous formatting disabled
// 1: Continuous formatting enabled
#define CPU_TPIU_FFCR_ENFCONT                                       0x00000002
#define CPU_TPIU_FFCR_ENFCONT_BITN                                           1
#define CPU_TPIU_FFCR_ENFCONT_M                                     0x00000002
#define CPU_TPIU_FFCR_ENFCONT_S                                              1

//*****************************************************************************
//
// Register: CPU_TPIU_O_FSCR
//
//*****************************************************************************
// Field:  [31:0] FSCR
//
// The global synchronization trigger is generated by the Program Counter (PC)
// Sampler block. This means that there is no synchronization counter in the
// TPIU.
#define CPU_TPIU_FSCR_FSCR_W                                                32
#define CPU_TPIU_FSCR_FSCR_M                                        0xFFFFFFFF
#define CPU_TPIU_FSCR_FSCR_S                                                 0

//*****************************************************************************
//
// Register: CPU_TPIU_O_CLAIMMASK
//
//*****************************************************************************
// Field:  [31:0] CLAIMMASK
//
// This register forms one half of the Claim Tag value. When reading this
// register returns the number of bits that can be set (each bit is considered
// separately):
//
// 0: This claim tag bit is not implemented
// 1: This claim tag bit is not implemented
//
// The behavior when writing to this register is described in CLAIMSET.
#define CPU_TPIU_CLAIMMASK_CLAIMMASK_W                                      32
#define CPU_TPIU_CLAIMMASK_CLAIMMASK_M                              0xFFFFFFFF
#define CPU_TPIU_CLAIMMASK_CLAIMMASK_S                                       0

//*****************************************************************************
//
// Register: CPU_TPIU_O_CLAIMSET
//
//*****************************************************************************
// Field:  [31:0] CLAIMSET
//
// This register forms one half of the Claim Tag value. Writing to this
// location allows individual bits to be set (each bit is considered
// separately):
//
// 0: No effect
// 1: Set this bit in the claim tag
//
// The behavior when reading from this location is described in CLAIMMASK.
#define CPU_TPIU_CLAIMSET_CLAIMSET_W                                        32
#define CPU_TPIU_CLAIMSET_CLAIMSET_M                                0xFFFFFFFF
#define CPU_TPIU_CLAIMSET_CLAIMSET_S                                         0

//*****************************************************************************
//
// Register: CPU_TPIU_O_CLAIMTAG
//
//*****************************************************************************
// Field:  [31:0] CLAIMTAG
//
// This register forms one half of the Claim Tag value. Reading this register
// returns the current Claim Tag value.
// Reading CLAIMMASK determines how many bits from this register must be used.
//
// The behavior when writing to this register is described in CLAIMCLR.
#define CPU_TPIU_CLAIMTAG_CLAIMTAG_W                                        32
#define CPU_TPIU_CLAIMTAG_CLAIMTAG_M                                0xFFFFFFFF
#define CPU_TPIU_CLAIMTAG_CLAIMTAG_S                                         0

//*****************************************************************************
//
// Register: CPU_TPIU_O_CLAIMCLR
//
//*****************************************************************************
// Field:  [31:0] CLAIMCLR
//
// This register forms one half of the Claim Tag value. Writing to this
// location enables individual bits to be cleared (each bit is considered
// separately):
//
// 0: No effect
// 1: Clear this bit in the claim tag.
//
// The behavior when reading from this location is described in CLAIMTAG.
#define CPU_TPIU_CLAIMCLR_CLAIMCLR_W                                        32
#define CPU_TPIU_CLAIMCLR_CLAIMCLR_M                                0xFFFFFFFF
#define CPU_TPIU_CLAIMCLR_CLAIMCLR_S                                         0

//*****************************************************************************
//
// Register: CPU_TPIU_O_DEVID
//
//*****************************************************************************
// Field:  [31:0] DEVID
//
// This field returns: 0xCA1 if there is an ETM present. 0xCA0 if there is no
// ETM present.
#define CPU_TPIU_DEVID_DEVID_W                                              32
#define CPU_TPIU_DEVID_DEVID_M                                      0xFFFFFFFF
#define CPU_TPIU_DEVID_DEVID_S                                               0


#endif // __CPU_TPIU__
