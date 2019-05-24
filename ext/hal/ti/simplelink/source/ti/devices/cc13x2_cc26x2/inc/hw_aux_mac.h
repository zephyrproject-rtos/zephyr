/******************************************************************************
*  Filename:       hw_aux_mac_h
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

#ifndef __HW_AUX_MAC_H__
#define __HW_AUX_MAC_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_MAC component
//
//*****************************************************************************
// Signed Operand 0
#define AUX_MAC_O_OP0S                                              0x00000000

// Unsigned Operand 0
#define AUX_MAC_O_OP0U                                              0x00000004

// Signed Operand 1 and Multiply
#define AUX_MAC_O_OP1SMUL                                           0x00000008

// Unsigned Operand 1 and Multiply
#define AUX_MAC_O_OP1UMUL                                           0x0000000C

// Signed Operand 1 and Multiply-Accumulate
#define AUX_MAC_O_OP1SMAC                                           0x00000010

// Unsigned Operand 1 and Multiply-Accumulate
#define AUX_MAC_O_OP1UMAC                                           0x00000014

// Signed Operand 1 and 16-bit Addition
#define AUX_MAC_O_OP1SADD16                                         0x00000018

// Unsigned Operand 1 and 16-bit Addition
#define AUX_MAC_O_OP1UADD16                                         0x0000001C

// Signed Operand 1 and 32-bit Addition
#define AUX_MAC_O_OP1SADD32                                         0x00000020

// Unsigned Operand 1 and 32-bit Addition
#define AUX_MAC_O_OP1UADD32                                         0x00000024

// Count Leading Zero
#define AUX_MAC_O_CLZ                                               0x00000028

// Count Leading Sign
#define AUX_MAC_O_CLS                                               0x0000002C

// Accumulator Shift
#define AUX_MAC_O_ACCSHIFT                                          0x00000030

// Accumulator Reset
#define AUX_MAC_O_ACCRESET                                          0x00000034

// Accumulator Bits 15:0
#define AUX_MAC_O_ACC15_0                                           0x00000038

// Accumulator Bits 16:1
#define AUX_MAC_O_ACC16_1                                           0x0000003C

// Accumulator Bits 17:2
#define AUX_MAC_O_ACC17_2                                           0x00000040

// Accumulator Bits 18:3
#define AUX_MAC_O_ACC18_3                                           0x00000044

// Accumulator Bits 19:4
#define AUX_MAC_O_ACC19_4                                           0x00000048

// Accumulator Bits 20:5
#define AUX_MAC_O_ACC20_5                                           0x0000004C

// Accumulator Bits 21:6
#define AUX_MAC_O_ACC21_6                                           0x00000050

// Accumulator Bits 22:7
#define AUX_MAC_O_ACC22_7                                           0x00000054

// Accumulator Bits 23:8
#define AUX_MAC_O_ACC23_8                                           0x00000058

// Accumulator Bits 24:9
#define AUX_MAC_O_ACC24_9                                           0x0000005C

// Accumulator Bits 25:10
#define AUX_MAC_O_ACC25_10                                          0x00000060

// Accumulator Bits 26:11
#define AUX_MAC_O_ACC26_11                                          0x00000064

// Accumulator Bits 27:12
#define AUX_MAC_O_ACC27_12                                          0x00000068

// Accumulator Bits 28:13
#define AUX_MAC_O_ACC28_13                                          0x0000006C

// Accumulator Bits 29:14
#define AUX_MAC_O_ACC29_14                                          0x00000070

// Accumulator Bits 30:15
#define AUX_MAC_O_ACC30_15                                          0x00000074

// Accumulator Bits 31:16
#define AUX_MAC_O_ACC31_16                                          0x00000078

// Accumulator Bits 32:17
#define AUX_MAC_O_ACC32_17                                          0x0000007C

// Accumulator Bits 33:18
#define AUX_MAC_O_ACC33_18                                          0x00000080

// Accumulator Bits 34:19
#define AUX_MAC_O_ACC34_19                                          0x00000084

// Accumulator Bits 35:20
#define AUX_MAC_O_ACC35_20                                          0x00000088

// Accumulator Bits 36:21
#define AUX_MAC_O_ACC36_21                                          0x0000008C

// Accumulator Bits 37:22
#define AUX_MAC_O_ACC37_22                                          0x00000090

// Accumulator Bits 38:23
#define AUX_MAC_O_ACC38_23                                          0x00000094

// Accumulator Bits 39:24
#define AUX_MAC_O_ACC39_24                                          0x00000098

// Accumulator Bits 39:32
#define AUX_MAC_O_ACC39_32                                          0x0000009C

//*****************************************************************************
//
// Register: AUX_MAC_O_OP0S
//
//*****************************************************************************
// Field:  [15:0] OP0_VALUE
//
// Signed operand 0.
//
// Operand for multiply, multiply-and-accumulate, or 32-bit add operations.
#define AUX_MAC_OP0S_OP0_VALUE_W                                            16
#define AUX_MAC_OP0S_OP0_VALUE_M                                    0x0000FFFF
#define AUX_MAC_OP0S_OP0_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP0U
//
//*****************************************************************************
// Field:  [15:0] OP0_VALUE
//
// Unsigned operand 0.
//
// Operand for multiply, multiply-and-accumulate, or 32-bit add operations.
#define AUX_MAC_OP0U_OP0_VALUE_W                                            16
#define AUX_MAC_OP0U_OP0_VALUE_M                                    0x0000FFFF
#define AUX_MAC_OP0U_OP0_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP1SMUL
//
//*****************************************************************************
// Field:  [15:0] OP1_VALUE
//
// Signed operand 1 and multiplication trigger.
//
// Write OP1_VALUE to set signed operand 1 and trigger the following operation:
//
// When operand 0 was written to OP0S.OP0_VALUE: ACC = OP1_VALUE *
// OP0S.OP0_VALUE.
// When operand 0 was written to OP0U.OP0_VALUE: ACC = OP1_VALUE *
// OP0U.OP0_VALUE.
#define AUX_MAC_OP1SMUL_OP1_VALUE_W                                         16
#define AUX_MAC_OP1SMUL_OP1_VALUE_M                                 0x0000FFFF
#define AUX_MAC_OP1SMUL_OP1_VALUE_S                                          0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP1UMUL
//
//*****************************************************************************
// Field:  [15:0] OP1_VALUE
//
// Unsigned operand 1 and multiplication trigger.
//
// Write OP1_VALUE to set unsigned operand 1 and trigger the following
// operation:
//
// When operand 0 was written to OP0S.OP0_VALUE: ACC = OP1_VALUE *
// OP0S.OP0_VALUE.
// When operand 0 was written to OP0U.OP0_VALUE: ACC = OP1_VALUE *
// OP0U.OP0_VALUE.
#define AUX_MAC_OP1UMUL_OP1_VALUE_W                                         16
#define AUX_MAC_OP1UMUL_OP1_VALUE_M                                 0x0000FFFF
#define AUX_MAC_OP1UMUL_OP1_VALUE_S                                          0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP1SMAC
//
//*****************************************************************************
// Field:  [15:0] OP1_VALUE
//
// Signed operand 1 and multiply-accumulation trigger.
//
// Write OP1_VALUE to set signed operand 1 and trigger the following operation:
//
// When operand 0 was written to OP0S.OP0_VALUE:  ACC = ACC + ( OP1_VALUE *
// OP0S.OP0_VALUE ).
// When operand 0 was written to OP0U.OP0_VALUE:  ACC = ACC + ( OP1_VALUE *
// OP0U.OP0_VALUE ).
#define AUX_MAC_OP1SMAC_OP1_VALUE_W                                         16
#define AUX_MAC_OP1SMAC_OP1_VALUE_M                                 0x0000FFFF
#define AUX_MAC_OP1SMAC_OP1_VALUE_S                                          0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP1UMAC
//
//*****************************************************************************
// Field:  [15:0] OP1_VALUE
//
// Unsigned operand 1 and multiply-accumulation trigger.
//
// Write OP1_VALUE to set unsigned operand 1 and trigger the following
// operation:
//
// When operand 0 was written to OP0S.OP0_VALUE:  ACC = ACC + ( OP1_VALUE *
// OP0S.OP0_VALUE ).
// When operand 0 was written to OP0U.OP0_VALUE:  ACC = ACC + ( OP1_VALUE *
// OP0U.OP0_VALUE ).
#define AUX_MAC_OP1UMAC_OP1_VALUE_W                                         16
#define AUX_MAC_OP1UMAC_OP1_VALUE_M                                 0x0000FFFF
#define AUX_MAC_OP1UMAC_OP1_VALUE_S                                          0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP1SADD16
//
//*****************************************************************************
// Field:  [15:0] OP1_VALUE
//
// Signed operand 1 and 16-bit addition trigger.
//
// Write OP1_VALUE to set signed operand 1 and trigger the following operation:
//
// ACC = ACC + OP1_VALUE.
#define AUX_MAC_OP1SADD16_OP1_VALUE_W                                       16
#define AUX_MAC_OP1SADD16_OP1_VALUE_M                               0x0000FFFF
#define AUX_MAC_OP1SADD16_OP1_VALUE_S                                        0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP1UADD16
//
//*****************************************************************************
// Field:  [15:0] OP1_VALUE
//
// Unsigned operand 1 and 16-bit addition trigger.
//
// Write OP1_VALUE to set unsigned operand 1 and trigger the following
// operation:
//
// ACC = ACC + OP1_VALUE.
#define AUX_MAC_OP1UADD16_OP1_VALUE_W                                       16
#define AUX_MAC_OP1UADD16_OP1_VALUE_M                               0x0000FFFF
#define AUX_MAC_OP1UADD16_OP1_VALUE_S                                        0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP1SADD32
//
//*****************************************************************************
// Field:  [15:0] OP1_VALUE
//
// Upper half of signed 32-bit operand and addition trigger.
//
// Write OP1_VALUE to set upper half of signed 32-bit operand and trigger the
// following operation:
//
// When lower half of 32-bit operand was written to OP0S.OP0_VALUE: ACC = ACC +
// (( OP1_VALUE << 16) | OP0S.OP0_VALUE ).
// When lower half of 32-bit operand was written to OP0U.OP0_VALUE: ACC = ACC +
// (( OP1_VALUE << 16) | OP0U.OP0_VALUE ).
#define AUX_MAC_OP1SADD32_OP1_VALUE_W                                       16
#define AUX_MAC_OP1SADD32_OP1_VALUE_M                               0x0000FFFF
#define AUX_MAC_OP1SADD32_OP1_VALUE_S                                        0

//*****************************************************************************
//
// Register: AUX_MAC_O_OP1UADD32
//
//*****************************************************************************
// Field:  [15:0] OP1_VALUE
//
// Upper half of unsigned 32-bit operand and addition trigger.
//
// Write OP1_VALUE to set upper half of unsigned 32-bit operand and trigger the
// following operation:
//
// When lower half of 32-bit operand was written to OP0S.OP0_VALUE: ACC = ACC +
// (( OP1_VALUE << 16) | OP0S.OP0_VALUE ).
// When lower half of 32-bit operand was written to OP0U.OP0_VALUE: ACC = ACC +
// (( OP1_VALUE << 16) | OP0U.OP0_VALUE ).
#define AUX_MAC_OP1UADD32_OP1_VALUE_W                                       16
#define AUX_MAC_OP1UADD32_OP1_VALUE_M                               0x0000FFFF
#define AUX_MAC_OP1UADD32_OP1_VALUE_S                                        0

//*****************************************************************************
//
// Register: AUX_MAC_O_CLZ
//
//*****************************************************************************
// Field:   [5:0] VALUE
//
// Number of leading zero bits in the accumulator:
//
// 0x00: 0 leading zeros.
// 0x01: 1 leading zero.
// ...
// 0x28: 40 leading zeros (accumulator value is 0).
#define AUX_MAC_CLZ_VALUE_W                                                  6
#define AUX_MAC_CLZ_VALUE_M                                         0x0000003F
#define AUX_MAC_CLZ_VALUE_S                                                  0

//*****************************************************************************
//
// Register: AUX_MAC_O_CLS
//
//*****************************************************************************
// Field:   [5:0] VALUE
//
// Number of leading sign bits in the accumulator.
//
// When MSB of accumulator is 0, VALUE is number of leading zeros, MSB
// included.
// When MSB of accumulator is 1, VALUE is number of leading ones, MSB included.
//
// VALUE range is 1 thru 40.
#define AUX_MAC_CLS_VALUE_W                                                  6
#define AUX_MAC_CLS_VALUE_M                                         0x0000003F
#define AUX_MAC_CLS_VALUE_S                                                  0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACCSHIFT
//
//*****************************************************************************
// Field:     [2] LSL1
//
// Logic shift left by 1 bit.
//
// Write 1 to shift the accumulator one bit to the left, 0 inserted at bit 0.
#define AUX_MAC_ACCSHIFT_LSL1                                       0x00000004
#define AUX_MAC_ACCSHIFT_LSL1_BITN                                           2
#define AUX_MAC_ACCSHIFT_LSL1_M                                     0x00000004
#define AUX_MAC_ACCSHIFT_LSL1_S                                              2

// Field:     [1] LSR1
//
// Logic shift right by 1 bit.
//
// Write 1 to shift the accumulator one bit to the right, 0 inserted at bit 39.
#define AUX_MAC_ACCSHIFT_LSR1                                       0x00000002
#define AUX_MAC_ACCSHIFT_LSR1_BITN                                           1
#define AUX_MAC_ACCSHIFT_LSR1_M                                     0x00000002
#define AUX_MAC_ACCSHIFT_LSR1_S                                              1

// Field:     [0] ASR1
//
// Arithmetic shift right by 1 bit.
//
// Write 1 to shift the accumulator one bit to the right, previous sign bit
// inserted at bit 39.
#define AUX_MAC_ACCSHIFT_ASR1                                       0x00000001
#define AUX_MAC_ACCSHIFT_ASR1_BITN                                           0
#define AUX_MAC_ACCSHIFT_ASR1_M                                     0x00000001
#define AUX_MAC_ACCSHIFT_ASR1_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACCRESET
//
//*****************************************************************************
// Field:  [15:0] TRG
//
// Write any value to this register to trigger a reset of all bits in the
// accumulator.
#define AUX_MAC_ACCRESET_TRG_W                                              16
#define AUX_MAC_ACCRESET_TRG_M                                      0x0000FFFF
#define AUX_MAC_ACCRESET_TRG_S                                               0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC15_0
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 15:0.
//
// Write VALUE to initialize bits 15:0 of accumulator.
#define AUX_MAC_ACC15_0_VALUE_W                                             16
#define AUX_MAC_ACC15_0_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC15_0_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC16_1
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 16:1.
#define AUX_MAC_ACC16_1_VALUE_W                                             16
#define AUX_MAC_ACC16_1_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC16_1_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC17_2
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 17:2.
#define AUX_MAC_ACC17_2_VALUE_W                                             16
#define AUX_MAC_ACC17_2_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC17_2_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC18_3
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 18:3.
#define AUX_MAC_ACC18_3_VALUE_W                                             16
#define AUX_MAC_ACC18_3_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC18_3_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC19_4
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 19:4.
#define AUX_MAC_ACC19_4_VALUE_W                                             16
#define AUX_MAC_ACC19_4_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC19_4_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC20_5
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 20:5.
#define AUX_MAC_ACC20_5_VALUE_W                                             16
#define AUX_MAC_ACC20_5_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC20_5_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC21_6
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 21:6.
#define AUX_MAC_ACC21_6_VALUE_W                                             16
#define AUX_MAC_ACC21_6_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC21_6_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC22_7
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 22:7.
#define AUX_MAC_ACC22_7_VALUE_W                                             16
#define AUX_MAC_ACC22_7_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC22_7_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC23_8
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 23:8.
#define AUX_MAC_ACC23_8_VALUE_W                                             16
#define AUX_MAC_ACC23_8_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC23_8_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC24_9
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 24:9.
#define AUX_MAC_ACC24_9_VALUE_W                                             16
#define AUX_MAC_ACC24_9_VALUE_M                                     0x0000FFFF
#define AUX_MAC_ACC24_9_VALUE_S                                              0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC25_10
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 25:10.
#define AUX_MAC_ACC25_10_VALUE_W                                            16
#define AUX_MAC_ACC25_10_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC25_10_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC26_11
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 26:11.
#define AUX_MAC_ACC26_11_VALUE_W                                            16
#define AUX_MAC_ACC26_11_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC26_11_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC27_12
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 27:12.
#define AUX_MAC_ACC27_12_VALUE_W                                            16
#define AUX_MAC_ACC27_12_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC27_12_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC28_13
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 28:13.
#define AUX_MAC_ACC28_13_VALUE_W                                            16
#define AUX_MAC_ACC28_13_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC28_13_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC29_14
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 29:14.
#define AUX_MAC_ACC29_14_VALUE_W                                            16
#define AUX_MAC_ACC29_14_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC29_14_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC30_15
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 30:15.
#define AUX_MAC_ACC30_15_VALUE_W                                            16
#define AUX_MAC_ACC30_15_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC30_15_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC31_16
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 31:16.
//
// Write VALUE to initialize bits 31:16 of accumulator.
#define AUX_MAC_ACC31_16_VALUE_W                                            16
#define AUX_MAC_ACC31_16_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC31_16_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC32_17
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 32:17.
#define AUX_MAC_ACC32_17_VALUE_W                                            16
#define AUX_MAC_ACC32_17_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC32_17_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC33_18
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 33:18.
#define AUX_MAC_ACC33_18_VALUE_W                                            16
#define AUX_MAC_ACC33_18_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC33_18_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC34_19
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 34:19.
#define AUX_MAC_ACC34_19_VALUE_W                                            16
#define AUX_MAC_ACC34_19_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC34_19_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC35_20
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 35:20.
#define AUX_MAC_ACC35_20_VALUE_W                                            16
#define AUX_MAC_ACC35_20_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC35_20_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC36_21
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 36:21.
#define AUX_MAC_ACC36_21_VALUE_W                                            16
#define AUX_MAC_ACC36_21_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC36_21_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC37_22
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 37:22.
#define AUX_MAC_ACC37_22_VALUE_W                                            16
#define AUX_MAC_ACC37_22_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC37_22_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC38_23
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 38:23.
#define AUX_MAC_ACC38_23_VALUE_W                                            16
#define AUX_MAC_ACC38_23_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC38_23_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC39_24
//
//*****************************************************************************
// Field:  [15:0] VALUE
//
// Value of the accumulator, bits 39:24.
#define AUX_MAC_ACC39_24_VALUE_W                                            16
#define AUX_MAC_ACC39_24_VALUE_M                                    0x0000FFFF
#define AUX_MAC_ACC39_24_VALUE_S                                             0

//*****************************************************************************
//
// Register: AUX_MAC_O_ACC39_32
//
//*****************************************************************************
// Field:   [7:0] VALUE
//
// Value of the accumulator, bits 39:32.
//
// Write VALUE to initialize bits 39:32 of accumulator.
#define AUX_MAC_ACC39_32_VALUE_W                                             8
#define AUX_MAC_ACC39_32_VALUE_M                                    0x000000FF
#define AUX_MAC_ACC39_32_VALUE_S                                             0


#endif // __AUX_MAC__
