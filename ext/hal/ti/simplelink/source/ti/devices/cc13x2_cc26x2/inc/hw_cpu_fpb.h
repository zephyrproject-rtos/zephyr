/******************************************************************************
*  Filename:       hw_cpu_fpb_h
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

#ifndef __HW_CPU_FPB_H__
#define __HW_CPU_FPB_H__

//*****************************************************************************
//
// This section defines the register offsets of
// CPU_FPB component
//
//*****************************************************************************
// Control
#define CPU_FPB_O_CTRL                                              0x00000000

// Remap
#define CPU_FPB_O_REMAP                                             0x00000004

// Comparator 0
#define CPU_FPB_O_COMP0                                             0x00000008

// Comparator 1
#define CPU_FPB_O_COMP1                                             0x0000000C

// Comparator 2
#define CPU_FPB_O_COMP2                                             0x00000010

// Comparator 3
#define CPU_FPB_O_COMP3                                             0x00000014

// Comparator 4
#define CPU_FPB_O_COMP4                                             0x00000018

// Comparator 5
#define CPU_FPB_O_COMP5                                             0x0000001C

// Comparator 6
#define CPU_FPB_O_COMP6                                             0x00000020

// Comparator 7
#define CPU_FPB_O_COMP7                                             0x00000024

//*****************************************************************************
//
// Register: CPU_FPB_O_CTRL
//
//*****************************************************************************
// Field: [13:12] NUM_CODE2
//
// Number of full banks of code comparators, sixteen comparators per bank.
// Where less than sixteen code comparators are provided, the bank count is
// zero, and the number present indicated by NUM_CODE1. This read only field
// contains 3'b000 to indicate 0 banks for Cortex-M processor.
#define CPU_FPB_CTRL_NUM_CODE2_W                                             2
#define CPU_FPB_CTRL_NUM_CODE2_M                                    0x00003000
#define CPU_FPB_CTRL_NUM_CODE2_S                                            12

// Field:  [11:8] NUM_LIT
//
// Number of literal slots field.
//
// 0x0: No literal slots
// 0x2: Two literal slots
#define CPU_FPB_CTRL_NUM_LIT_W                                               4
#define CPU_FPB_CTRL_NUM_LIT_M                                      0x00000F00
#define CPU_FPB_CTRL_NUM_LIT_S                                               8

// Field:   [7:4] NUM_CODE1
//
// Number of code slots field.
//
// 0x0: No code slots
// 0x2: Two code slots
// 0x6: Six code slots
#define CPU_FPB_CTRL_NUM_CODE1_W                                             4
#define CPU_FPB_CTRL_NUM_CODE1_M                                    0x000000F0
#define CPU_FPB_CTRL_NUM_CODE1_S                                             4

// Field:     [1] KEY
//
// Key field. In order to write to this register, this bit-field must be
// written to '1'. This bit always reads 0.
#define CPU_FPB_CTRL_KEY                                            0x00000002
#define CPU_FPB_CTRL_KEY_BITN                                                1
#define CPU_FPB_CTRL_KEY_M                                          0x00000002
#define CPU_FPB_CTRL_KEY_S                                                   1

// Field:     [0] ENABLE
//
// Flash patch unit enable bit
//
// 0x0: Flash patch unit disabled
// 0x1: Flash patch unit enabled
#define CPU_FPB_CTRL_ENABLE                                         0x00000001
#define CPU_FPB_CTRL_ENABLE_BITN                                             0
#define CPU_FPB_CTRL_ENABLE_M                                       0x00000001
#define CPU_FPB_CTRL_ENABLE_S                                                0

//*****************************************************************************
//
// Register: CPU_FPB_O_REMAP
//
//*****************************************************************************
// Field:  [28:5] REMAP
//
// Remap base address field.
#define CPU_FPB_REMAP_REMAP_W                                               24
#define CPU_FPB_REMAP_REMAP_M                                       0x1FFFFFE0
#define CPU_FPB_REMAP_REMAP_S                                                5

//*****************************************************************************
//
// Register: CPU_FPB_O_COMP0
//
//*****************************************************************************
// Field: [31:30] REPLACE
//
// This selects what happens when the COMP address is matched. Address
// remapping only takes place for the 0x0 setting.
//
// 0x0: Remap to remap address. See REMAP.REMAP
// 0x1: Set BKPT on lower halfword, upper is unaffected
// 0x2: Set BKPT on upper halfword, lower is unaffected
// 0x3: Set BKPT on both lower and upper halfwords.
#define CPU_FPB_COMP0_REPLACE_W                                              2
#define CPU_FPB_COMP0_REPLACE_M                                     0xC0000000
#define CPU_FPB_COMP0_REPLACE_S                                             30

// Field:  [28:2] COMP
//
// Comparison address.
#define CPU_FPB_COMP0_COMP_W                                                27
#define CPU_FPB_COMP0_COMP_M                                        0x1FFFFFFC
#define CPU_FPB_COMP0_COMP_S                                                 2

// Field:     [0] ENABLE
//
// Compare and remap enable comparator 0. CTRL.ENABLE must also be set to
// enable comparisons.
//
// 0x0: Compare and remap for comparator 0 disabled
// 0x1: Compare and remap for comparator 0 enabled
#define CPU_FPB_COMP0_ENABLE                                        0x00000001
#define CPU_FPB_COMP0_ENABLE_BITN                                            0
#define CPU_FPB_COMP0_ENABLE_M                                      0x00000001
#define CPU_FPB_COMP0_ENABLE_S                                               0

//*****************************************************************************
//
// Register: CPU_FPB_O_COMP1
//
//*****************************************************************************
// Field: [31:30] REPLACE
//
// This selects what happens when the COMP address is matched. Address
// remapping only takes place for the 0x0 setting.
//
// 0x0: Remap to remap address. See REMAP.REMAP
// 0x1: Set BKPT on lower halfword, upper is unaffected
// 0x2: Set BKPT on upper halfword, lower is unaffected
// 0x3: Set BKPT on both lower and upper halfwords.
#define CPU_FPB_COMP1_REPLACE_W                                              2
#define CPU_FPB_COMP1_REPLACE_M                                     0xC0000000
#define CPU_FPB_COMP1_REPLACE_S                                             30

// Field:  [28:2] COMP
//
// Comparison address.
#define CPU_FPB_COMP1_COMP_W                                                27
#define CPU_FPB_COMP1_COMP_M                                        0x1FFFFFFC
#define CPU_FPB_COMP1_COMP_S                                                 2

// Field:     [0] ENABLE
//
// Compare and remap enable comparator 1. CTRL.ENABLE must also be set to
// enable comparisons.
//
// 0x0: Compare and remap for comparator 1 disabled
// 0x1: Compare and remap for comparator 1 enabled
#define CPU_FPB_COMP1_ENABLE                                        0x00000001
#define CPU_FPB_COMP1_ENABLE_BITN                                            0
#define CPU_FPB_COMP1_ENABLE_M                                      0x00000001
#define CPU_FPB_COMP1_ENABLE_S                                               0

//*****************************************************************************
//
// Register: CPU_FPB_O_COMP2
//
//*****************************************************************************
// Field: [31:30] REPLACE
//
// This selects what happens when the COMP address is matched. Address
// remapping only takes place for the 0x0 setting.
//
// 0x0: Remap to remap address. See REMAP.REMAP
// 0x1: Set BKPT on lower halfword, upper is unaffected
// 0x2: Set BKPT on upper halfword, lower is unaffected
// 0x3: Set BKPT on both lower and upper halfwords.
#define CPU_FPB_COMP2_REPLACE_W                                              2
#define CPU_FPB_COMP2_REPLACE_M                                     0xC0000000
#define CPU_FPB_COMP2_REPLACE_S                                             30

// Field:  [28:2] COMP
//
// Comparison address.
#define CPU_FPB_COMP2_COMP_W                                                27
#define CPU_FPB_COMP2_COMP_M                                        0x1FFFFFFC
#define CPU_FPB_COMP2_COMP_S                                                 2

// Field:     [0] ENABLE
//
// Compare and remap enable comparator 2. CTRL.ENABLE must also be set to
// enable comparisons.
//
// 0x0: Compare and remap for comparator 2 disabled
// 0x1: Compare and remap for comparator 2 enabled
#define CPU_FPB_COMP2_ENABLE                                        0x00000001
#define CPU_FPB_COMP2_ENABLE_BITN                                            0
#define CPU_FPB_COMP2_ENABLE_M                                      0x00000001
#define CPU_FPB_COMP2_ENABLE_S                                               0

//*****************************************************************************
//
// Register: CPU_FPB_O_COMP3
//
//*****************************************************************************
// Field: [31:30] REPLACE
//
// This selects what happens when the COMP address is matched. Address
// remapping only takes place for the 0x0 setting.
//
// 0x0: Remap to remap address. See REMAP.REMAP
// 0x1: Set BKPT on lower halfword, upper is unaffected
// 0x2: Set BKPT on upper halfword, lower is unaffected
// 0x3: Set BKPT on both lower and upper halfwords.
#define CPU_FPB_COMP3_REPLACE_W                                              2
#define CPU_FPB_COMP3_REPLACE_M                                     0xC0000000
#define CPU_FPB_COMP3_REPLACE_S                                             30

// Field:  [28:2] COMP
//
// Comparison address.
#define CPU_FPB_COMP3_COMP_W                                                27
#define CPU_FPB_COMP3_COMP_M                                        0x1FFFFFFC
#define CPU_FPB_COMP3_COMP_S                                                 2

// Field:     [0] ENABLE
//
// Compare and remap enable comparator 3. CTRL.ENABLE must also be set to
// enable comparisons.
//
// 0x0: Compare and remap for comparator 3 disabled
// 0x1: Compare and remap for comparator 3 enabled
#define CPU_FPB_COMP3_ENABLE                                        0x00000001
#define CPU_FPB_COMP3_ENABLE_BITN                                            0
#define CPU_FPB_COMP3_ENABLE_M                                      0x00000001
#define CPU_FPB_COMP3_ENABLE_S                                               0

//*****************************************************************************
//
// Register: CPU_FPB_O_COMP4
//
//*****************************************************************************
// Field: [31:30] REPLACE
//
// This selects what happens when the COMP address is matched. Address
// remapping only takes place for the 0x0 setting.
//
// 0x0: Remap to remap address. See REMAP.REMAP
// 0x1: Set BKPT on lower halfword, upper is unaffected
// 0x2: Set BKPT on upper halfword, lower is unaffected
// 0x3: Set BKPT on both lower and upper halfwords.
#define CPU_FPB_COMP4_REPLACE_W                                              2
#define CPU_FPB_COMP4_REPLACE_M                                     0xC0000000
#define CPU_FPB_COMP4_REPLACE_S                                             30

// Field:  [28:2] COMP
//
// Comparison address.
#define CPU_FPB_COMP4_COMP_W                                                27
#define CPU_FPB_COMP4_COMP_M                                        0x1FFFFFFC
#define CPU_FPB_COMP4_COMP_S                                                 2

// Field:     [0] ENABLE
//
// Compare and remap enable comparator 4. CTRL.ENABLE must also be set to
// enable comparisons.
//
// 0x0: Compare and remap for comparator 4 disabled
// 0x1: Compare and remap for comparator 4 enabled
#define CPU_FPB_COMP4_ENABLE                                        0x00000001
#define CPU_FPB_COMP4_ENABLE_BITN                                            0
#define CPU_FPB_COMP4_ENABLE_M                                      0x00000001
#define CPU_FPB_COMP4_ENABLE_S                                               0

//*****************************************************************************
//
// Register: CPU_FPB_O_COMP5
//
//*****************************************************************************
// Field: [31:30] REPLACE
//
// This selects what happens when the COMP address is matched. Address
// remapping only takes place for the 0x0 setting.
//
// 0x0: Remap to remap address. See REMAP.REMAP
// 0x1: Set BKPT on lower halfword, upper is unaffected
// 0x2: Set BKPT on upper halfword, lower is unaffected
// 0x3: Set BKPT on both lower and upper halfwords.
#define CPU_FPB_COMP5_REPLACE_W                                              2
#define CPU_FPB_COMP5_REPLACE_M                                     0xC0000000
#define CPU_FPB_COMP5_REPLACE_S                                             30

// Field:  [28:2] COMP
//
// Comparison address.
#define CPU_FPB_COMP5_COMP_W                                                27
#define CPU_FPB_COMP5_COMP_M                                        0x1FFFFFFC
#define CPU_FPB_COMP5_COMP_S                                                 2

// Field:     [0] ENABLE
//
// Compare and remap enable comparator 5. CTRL.ENABLE must also be set to
// enable comparisons.
//
// 0x0: Compare and remap for comparator 5 disabled
// 0x1: Compare and remap for comparator 5 enabled
#define CPU_FPB_COMP5_ENABLE                                        0x00000001
#define CPU_FPB_COMP5_ENABLE_BITN                                            0
#define CPU_FPB_COMP5_ENABLE_M                                      0x00000001
#define CPU_FPB_COMP5_ENABLE_S                                               0

//*****************************************************************************
//
// Register: CPU_FPB_O_COMP6
//
//*****************************************************************************
// Field: [31:30] REPLACE
//
// This selects what happens when the COMP address is matched. Comparator 6 is
// a literal comparator and the only supported setting is 0x0. Other settings
// will be ignored.
//
// 0x0: Remap to remap address. See REMAP.REMAP
// 0x1: Set BKPT on lower halfword, upper is unaffected
// 0x2: Set BKPT on upper halfword, lower is unaffected
// 0x3: Set BKPT on both lower and upper halfwords.
#define CPU_FPB_COMP6_REPLACE_W                                              2
#define CPU_FPB_COMP6_REPLACE_M                                     0xC0000000
#define CPU_FPB_COMP6_REPLACE_S                                             30

// Field:  [28:2] COMP
//
// Comparison address.
#define CPU_FPB_COMP6_COMP_W                                                27
#define CPU_FPB_COMP6_COMP_M                                        0x1FFFFFFC
#define CPU_FPB_COMP6_COMP_S                                                 2

// Field:     [0] ENABLE
//
// Compare and remap enable comparator 6. CTRL.ENABLE must also be set to
// enable comparisons.
//
// 0x0: Compare and remap for comparator 6 disabled
// 0x1: Compare and remap for comparator 6 enabled
#define CPU_FPB_COMP6_ENABLE                                        0x00000001
#define CPU_FPB_COMP6_ENABLE_BITN                                            0
#define CPU_FPB_COMP6_ENABLE_M                                      0x00000001
#define CPU_FPB_COMP6_ENABLE_S                                               0

//*****************************************************************************
//
// Register: CPU_FPB_O_COMP7
//
//*****************************************************************************
// Field: [31:30] REPLACE
//
// This selects what happens when the COMP address is matched. Comparator 7 is
// a literal comparator and the only supported setting is 0x0. Other settings
// will be ignored.
//
// 0x0: Remap to remap address. See REMAP.REMAP
// 0x1: Set BKPT on lower halfword, upper is unaffected
// 0x2: Set BKPT on upper halfword, lower is unaffected
// 0x3: Set BKPT on both lower and upper halfwords.
#define CPU_FPB_COMP7_REPLACE_W                                              2
#define CPU_FPB_COMP7_REPLACE_M                                     0xC0000000
#define CPU_FPB_COMP7_REPLACE_S                                             30

// Field:  [28:2] COMP
//
// Comparison address.
#define CPU_FPB_COMP7_COMP_W                                                27
#define CPU_FPB_COMP7_COMP_M                                        0x1FFFFFFC
#define CPU_FPB_COMP7_COMP_S                                                 2

// Field:     [0] ENABLE
//
// Compare and remap enable comparator 7. CTRL.ENABLE must also be set to
// enable comparisons.
//
// 0x0: Compare and remap for comparator 7 disabled
// 0x1: Compare and remap for comparator 7 enabled
#define CPU_FPB_COMP7_ENABLE                                        0x00000001
#define CPU_FPB_COMP7_ENABLE_BITN                                            0
#define CPU_FPB_COMP7_ENABLE_M                                      0x00000001
#define CPU_FPB_COMP7_ENABLE_S                                               0


#endif // __CPU_FPB__
