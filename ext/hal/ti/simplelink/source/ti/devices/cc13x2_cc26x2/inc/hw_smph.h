/******************************************************************************
*  Filename:       hw_smph_h
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

#ifndef __HW_SMPH_H__
#define __HW_SMPH_H__

//*****************************************************************************
//
// This section defines the register offsets of
// SMPH component
//
//*****************************************************************************
// MCU SEMAPHORE 0
#define SMPH_O_SMPH0                                                0x00000000

// MCU SEMAPHORE 1
#define SMPH_O_SMPH1                                                0x00000004

// MCU SEMAPHORE 2
#define SMPH_O_SMPH2                                                0x00000008

// MCU SEMAPHORE 3
#define SMPH_O_SMPH3                                                0x0000000C

// MCU SEMAPHORE 4
#define SMPH_O_SMPH4                                                0x00000010

// MCU SEMAPHORE 5
#define SMPH_O_SMPH5                                                0x00000014

// MCU SEMAPHORE 6
#define SMPH_O_SMPH6                                                0x00000018

// MCU SEMAPHORE 7
#define SMPH_O_SMPH7                                                0x0000001C

// MCU SEMAPHORE 8
#define SMPH_O_SMPH8                                                0x00000020

// MCU SEMAPHORE 9
#define SMPH_O_SMPH9                                                0x00000024

// MCU SEMAPHORE 10
#define SMPH_O_SMPH10                                               0x00000028

// MCU SEMAPHORE 11
#define SMPH_O_SMPH11                                               0x0000002C

// MCU SEMAPHORE 12
#define SMPH_O_SMPH12                                               0x00000030

// MCU SEMAPHORE 13
#define SMPH_O_SMPH13                                               0x00000034

// MCU SEMAPHORE 14
#define SMPH_O_SMPH14                                               0x00000038

// MCU SEMAPHORE 15
#define SMPH_O_SMPH15                                               0x0000003C

// MCU SEMAPHORE 16
#define SMPH_O_SMPH16                                               0x00000040

// MCU SEMAPHORE 17
#define SMPH_O_SMPH17                                               0x00000044

// MCU SEMAPHORE 18
#define SMPH_O_SMPH18                                               0x00000048

// MCU SEMAPHORE 19
#define SMPH_O_SMPH19                                               0x0000004C

// MCU SEMAPHORE 20
#define SMPH_O_SMPH20                                               0x00000050

// MCU SEMAPHORE 21
#define SMPH_O_SMPH21                                               0x00000054

// MCU SEMAPHORE 22
#define SMPH_O_SMPH22                                               0x00000058

// MCU SEMAPHORE 23
#define SMPH_O_SMPH23                                               0x0000005C

// MCU SEMAPHORE 24
#define SMPH_O_SMPH24                                               0x00000060

// MCU SEMAPHORE 25
#define SMPH_O_SMPH25                                               0x00000064

// MCU SEMAPHORE 26
#define SMPH_O_SMPH26                                               0x00000068

// MCU SEMAPHORE 27
#define SMPH_O_SMPH27                                               0x0000006C

// MCU SEMAPHORE 28
#define SMPH_O_SMPH28                                               0x00000070

// MCU SEMAPHORE 29
#define SMPH_O_SMPH29                                               0x00000074

// MCU SEMAPHORE 30
#define SMPH_O_SMPH30                                               0x00000078

// MCU SEMAPHORE 31
#define SMPH_O_SMPH31                                               0x0000007C

// MCU SEMAPHORE 0 ALIAS
#define SMPH_O_PEEK0                                                0x00000800

// MCU SEMAPHORE 1 ALIAS
#define SMPH_O_PEEK1                                                0x00000804

// MCU SEMAPHORE 2 ALIAS
#define SMPH_O_PEEK2                                                0x00000808

// MCU SEMAPHORE 3 ALIAS
#define SMPH_O_PEEK3                                                0x0000080C

// MCU SEMAPHORE 4 ALIAS
#define SMPH_O_PEEK4                                                0x00000810

// MCU SEMAPHORE 5 ALIAS
#define SMPH_O_PEEK5                                                0x00000814

// MCU SEMAPHORE 6 ALIAS
#define SMPH_O_PEEK6                                                0x00000818

// MCU SEMAPHORE 7 ALIAS
#define SMPH_O_PEEK7                                                0x0000081C

// MCU SEMAPHORE 8 ALIAS
#define SMPH_O_PEEK8                                                0x00000820

// MCU SEMAPHORE 9 ALIAS
#define SMPH_O_PEEK9                                                0x00000824

// MCU SEMAPHORE 10 ALIAS
#define SMPH_O_PEEK10                                               0x00000828

// MCU SEMAPHORE 11 ALIAS
#define SMPH_O_PEEK11                                               0x0000082C

// MCU SEMAPHORE 12 ALIAS
#define SMPH_O_PEEK12                                               0x00000830

// MCU SEMAPHORE 13 ALIAS
#define SMPH_O_PEEK13                                               0x00000834

// MCU SEMAPHORE 14 ALIAS
#define SMPH_O_PEEK14                                               0x00000838

// MCU SEMAPHORE 15 ALIAS
#define SMPH_O_PEEK15                                               0x0000083C

// MCU SEMAPHORE 16 ALIAS
#define SMPH_O_PEEK16                                               0x00000840

// MCU SEMAPHORE 17 ALIAS
#define SMPH_O_PEEK17                                               0x00000844

// MCU SEMAPHORE 18 ALIAS
#define SMPH_O_PEEK18                                               0x00000848

// MCU SEMAPHORE 19 ALIAS
#define SMPH_O_PEEK19                                               0x0000084C

// MCU SEMAPHORE 20 ALIAS
#define SMPH_O_PEEK20                                               0x00000850

// MCU SEMAPHORE 21 ALIAS
#define SMPH_O_PEEK21                                               0x00000854

// MCU SEMAPHORE 22 ALIAS
#define SMPH_O_PEEK22                                               0x00000858

// MCU SEMAPHORE 23 ALIAS
#define SMPH_O_PEEK23                                               0x0000085C

// MCU SEMAPHORE 24 ALIAS
#define SMPH_O_PEEK24                                               0x00000860

// MCU SEMAPHORE 25 ALIAS
#define SMPH_O_PEEK25                                               0x00000864

// MCU SEMAPHORE 26 ALIAS
#define SMPH_O_PEEK26                                               0x00000868

// MCU SEMAPHORE 27 ALIAS
#define SMPH_O_PEEK27                                               0x0000086C

// MCU SEMAPHORE 28 ALIAS
#define SMPH_O_PEEK28                                               0x00000870

// MCU SEMAPHORE 29 ALIAS
#define SMPH_O_PEEK29                                               0x00000874

// MCU SEMAPHORE 30 ALIAS
#define SMPH_O_PEEK30                                               0x00000878

// MCU SEMAPHORE 31 ALIAS
#define SMPH_O_PEEK31                                               0x0000087C

//*****************************************************************************
//
// Register: SMPH_O_SMPH0
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH0_STAT                                             0x00000001
#define SMPH_SMPH0_STAT_BITN                                                 0
#define SMPH_SMPH0_STAT_M                                           0x00000001
#define SMPH_SMPH0_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH1
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH1_STAT                                             0x00000001
#define SMPH_SMPH1_STAT_BITN                                                 0
#define SMPH_SMPH1_STAT_M                                           0x00000001
#define SMPH_SMPH1_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH2
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH2_STAT                                             0x00000001
#define SMPH_SMPH2_STAT_BITN                                                 0
#define SMPH_SMPH2_STAT_M                                           0x00000001
#define SMPH_SMPH2_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH3
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH3_STAT                                             0x00000001
#define SMPH_SMPH3_STAT_BITN                                                 0
#define SMPH_SMPH3_STAT_M                                           0x00000001
#define SMPH_SMPH3_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH4
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH4_STAT                                             0x00000001
#define SMPH_SMPH4_STAT_BITN                                                 0
#define SMPH_SMPH4_STAT_M                                           0x00000001
#define SMPH_SMPH4_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH5
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH5_STAT                                             0x00000001
#define SMPH_SMPH5_STAT_BITN                                                 0
#define SMPH_SMPH5_STAT_M                                           0x00000001
#define SMPH_SMPH5_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH6
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH6_STAT                                             0x00000001
#define SMPH_SMPH6_STAT_BITN                                                 0
#define SMPH_SMPH6_STAT_M                                           0x00000001
#define SMPH_SMPH6_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH7
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH7_STAT                                             0x00000001
#define SMPH_SMPH7_STAT_BITN                                                 0
#define SMPH_SMPH7_STAT_M                                           0x00000001
#define SMPH_SMPH7_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH8
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH8_STAT                                             0x00000001
#define SMPH_SMPH8_STAT_BITN                                                 0
#define SMPH_SMPH8_STAT_M                                           0x00000001
#define SMPH_SMPH8_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH9
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH9_STAT                                             0x00000001
#define SMPH_SMPH9_STAT_BITN                                                 0
#define SMPH_SMPH9_STAT_M                                           0x00000001
#define SMPH_SMPH9_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_SMPH10
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH10_STAT                                            0x00000001
#define SMPH_SMPH10_STAT_BITN                                                0
#define SMPH_SMPH10_STAT_M                                          0x00000001
#define SMPH_SMPH10_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH11
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH11_STAT                                            0x00000001
#define SMPH_SMPH11_STAT_BITN                                                0
#define SMPH_SMPH11_STAT_M                                          0x00000001
#define SMPH_SMPH11_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH12
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH12_STAT                                            0x00000001
#define SMPH_SMPH12_STAT_BITN                                                0
#define SMPH_SMPH12_STAT_M                                          0x00000001
#define SMPH_SMPH12_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH13
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH13_STAT                                            0x00000001
#define SMPH_SMPH13_STAT_BITN                                                0
#define SMPH_SMPH13_STAT_M                                          0x00000001
#define SMPH_SMPH13_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH14
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH14_STAT                                            0x00000001
#define SMPH_SMPH14_STAT_BITN                                                0
#define SMPH_SMPH14_STAT_M                                          0x00000001
#define SMPH_SMPH14_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH15
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH15_STAT                                            0x00000001
#define SMPH_SMPH15_STAT_BITN                                                0
#define SMPH_SMPH15_STAT_M                                          0x00000001
#define SMPH_SMPH15_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH16
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH16_STAT                                            0x00000001
#define SMPH_SMPH16_STAT_BITN                                                0
#define SMPH_SMPH16_STAT_M                                          0x00000001
#define SMPH_SMPH16_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH17
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH17_STAT                                            0x00000001
#define SMPH_SMPH17_STAT_BITN                                                0
#define SMPH_SMPH17_STAT_M                                          0x00000001
#define SMPH_SMPH17_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH18
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH18_STAT                                            0x00000001
#define SMPH_SMPH18_STAT_BITN                                                0
#define SMPH_SMPH18_STAT_M                                          0x00000001
#define SMPH_SMPH18_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH19
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH19_STAT                                            0x00000001
#define SMPH_SMPH19_STAT_BITN                                                0
#define SMPH_SMPH19_STAT_M                                          0x00000001
#define SMPH_SMPH19_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH20
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH20_STAT                                            0x00000001
#define SMPH_SMPH20_STAT_BITN                                                0
#define SMPH_SMPH20_STAT_M                                          0x00000001
#define SMPH_SMPH20_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH21
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH21_STAT                                            0x00000001
#define SMPH_SMPH21_STAT_BITN                                                0
#define SMPH_SMPH21_STAT_M                                          0x00000001
#define SMPH_SMPH21_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH22
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH22_STAT                                            0x00000001
#define SMPH_SMPH22_STAT_BITN                                                0
#define SMPH_SMPH22_STAT_M                                          0x00000001
#define SMPH_SMPH22_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH23
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH23_STAT                                            0x00000001
#define SMPH_SMPH23_STAT_BITN                                                0
#define SMPH_SMPH23_STAT_M                                          0x00000001
#define SMPH_SMPH23_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH24
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH24_STAT                                            0x00000001
#define SMPH_SMPH24_STAT_BITN                                                0
#define SMPH_SMPH24_STAT_M                                          0x00000001
#define SMPH_SMPH24_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH25
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH25_STAT                                            0x00000001
#define SMPH_SMPH25_STAT_BITN                                                0
#define SMPH_SMPH25_STAT_M                                          0x00000001
#define SMPH_SMPH25_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH26
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH26_STAT                                            0x00000001
#define SMPH_SMPH26_STAT_BITN                                                0
#define SMPH_SMPH26_STAT_M                                          0x00000001
#define SMPH_SMPH26_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH27
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH27_STAT                                            0x00000001
#define SMPH_SMPH27_STAT_BITN                                                0
#define SMPH_SMPH27_STAT_M                                          0x00000001
#define SMPH_SMPH27_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH28
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH28_STAT                                            0x00000001
#define SMPH_SMPH28_STAT_BITN                                                0
#define SMPH_SMPH28_STAT_M                                          0x00000001
#define SMPH_SMPH28_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH29
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH29_STAT                                            0x00000001
#define SMPH_SMPH29_STAT_BITN                                                0
#define SMPH_SMPH29_STAT_M                                          0x00000001
#define SMPH_SMPH29_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH30
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH30_STAT                                            0x00000001
#define SMPH_SMPH30_STAT_BITN                                                0
#define SMPH_SMPH30_STAT_M                                          0x00000001
#define SMPH_SMPH30_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_SMPH31
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Reading the register causes it to change value to 0. Releasing the semaphore
// is done by writing 1.
#define SMPH_SMPH31_STAT                                            0x00000001
#define SMPH_SMPH31_STAT_BITN                                                0
#define SMPH_SMPH31_STAT_M                                          0x00000001
#define SMPH_SMPH31_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK0
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK0_STAT                                             0x00000001
#define SMPH_PEEK0_STAT_BITN                                                 0
#define SMPH_PEEK0_STAT_M                                           0x00000001
#define SMPH_PEEK0_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK1
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK1_STAT                                             0x00000001
#define SMPH_PEEK1_STAT_BITN                                                 0
#define SMPH_PEEK1_STAT_M                                           0x00000001
#define SMPH_PEEK1_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK2
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK2_STAT                                             0x00000001
#define SMPH_PEEK2_STAT_BITN                                                 0
#define SMPH_PEEK2_STAT_M                                           0x00000001
#define SMPH_PEEK2_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK3
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK3_STAT                                             0x00000001
#define SMPH_PEEK3_STAT_BITN                                                 0
#define SMPH_PEEK3_STAT_M                                           0x00000001
#define SMPH_PEEK3_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK4
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK4_STAT                                             0x00000001
#define SMPH_PEEK4_STAT_BITN                                                 0
#define SMPH_PEEK4_STAT_M                                           0x00000001
#define SMPH_PEEK4_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK5
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK5_STAT                                             0x00000001
#define SMPH_PEEK5_STAT_BITN                                                 0
#define SMPH_PEEK5_STAT_M                                           0x00000001
#define SMPH_PEEK5_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK6
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK6_STAT                                             0x00000001
#define SMPH_PEEK6_STAT_BITN                                                 0
#define SMPH_PEEK6_STAT_M                                           0x00000001
#define SMPH_PEEK6_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK7
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK7_STAT                                             0x00000001
#define SMPH_PEEK7_STAT_BITN                                                 0
#define SMPH_PEEK7_STAT_M                                           0x00000001
#define SMPH_PEEK7_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK8
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK8_STAT                                             0x00000001
#define SMPH_PEEK8_STAT_BITN                                                 0
#define SMPH_PEEK8_STAT_M                                           0x00000001
#define SMPH_PEEK8_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK9
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK9_STAT                                             0x00000001
#define SMPH_PEEK9_STAT_BITN                                                 0
#define SMPH_PEEK9_STAT_M                                           0x00000001
#define SMPH_PEEK9_STAT_S                                                    0

//*****************************************************************************
//
// Register: SMPH_O_PEEK10
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK10_STAT                                            0x00000001
#define SMPH_PEEK10_STAT_BITN                                                0
#define SMPH_PEEK10_STAT_M                                          0x00000001
#define SMPH_PEEK10_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK11
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK11_STAT                                            0x00000001
#define SMPH_PEEK11_STAT_BITN                                                0
#define SMPH_PEEK11_STAT_M                                          0x00000001
#define SMPH_PEEK11_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK12
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK12_STAT                                            0x00000001
#define SMPH_PEEK12_STAT_BITN                                                0
#define SMPH_PEEK12_STAT_M                                          0x00000001
#define SMPH_PEEK12_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK13
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK13_STAT                                            0x00000001
#define SMPH_PEEK13_STAT_BITN                                                0
#define SMPH_PEEK13_STAT_M                                          0x00000001
#define SMPH_PEEK13_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK14
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK14_STAT                                            0x00000001
#define SMPH_PEEK14_STAT_BITN                                                0
#define SMPH_PEEK14_STAT_M                                          0x00000001
#define SMPH_PEEK14_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK15
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK15_STAT                                            0x00000001
#define SMPH_PEEK15_STAT_BITN                                                0
#define SMPH_PEEK15_STAT_M                                          0x00000001
#define SMPH_PEEK15_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK16
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK16_STAT                                            0x00000001
#define SMPH_PEEK16_STAT_BITN                                                0
#define SMPH_PEEK16_STAT_M                                          0x00000001
#define SMPH_PEEK16_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK17
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK17_STAT                                            0x00000001
#define SMPH_PEEK17_STAT_BITN                                                0
#define SMPH_PEEK17_STAT_M                                          0x00000001
#define SMPH_PEEK17_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK18
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK18_STAT                                            0x00000001
#define SMPH_PEEK18_STAT_BITN                                                0
#define SMPH_PEEK18_STAT_M                                          0x00000001
#define SMPH_PEEK18_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK19
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK19_STAT                                            0x00000001
#define SMPH_PEEK19_STAT_BITN                                                0
#define SMPH_PEEK19_STAT_M                                          0x00000001
#define SMPH_PEEK19_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK20
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK20_STAT                                            0x00000001
#define SMPH_PEEK20_STAT_BITN                                                0
#define SMPH_PEEK20_STAT_M                                          0x00000001
#define SMPH_PEEK20_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK21
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK21_STAT                                            0x00000001
#define SMPH_PEEK21_STAT_BITN                                                0
#define SMPH_PEEK21_STAT_M                                          0x00000001
#define SMPH_PEEK21_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK22
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK22_STAT                                            0x00000001
#define SMPH_PEEK22_STAT_BITN                                                0
#define SMPH_PEEK22_STAT_M                                          0x00000001
#define SMPH_PEEK22_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK23
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK23_STAT                                            0x00000001
#define SMPH_PEEK23_STAT_BITN                                                0
#define SMPH_PEEK23_STAT_M                                          0x00000001
#define SMPH_PEEK23_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK24
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK24_STAT                                            0x00000001
#define SMPH_PEEK24_STAT_BITN                                                0
#define SMPH_PEEK24_STAT_M                                          0x00000001
#define SMPH_PEEK24_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK25
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK25_STAT                                            0x00000001
#define SMPH_PEEK25_STAT_BITN                                                0
#define SMPH_PEEK25_STAT_M                                          0x00000001
#define SMPH_PEEK25_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK26
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK26_STAT                                            0x00000001
#define SMPH_PEEK26_STAT_BITN                                                0
#define SMPH_PEEK26_STAT_M                                          0x00000001
#define SMPH_PEEK26_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK27
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK27_STAT                                            0x00000001
#define SMPH_PEEK27_STAT_BITN                                                0
#define SMPH_PEEK27_STAT_M                                          0x00000001
#define SMPH_PEEK27_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK28
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK28_STAT                                            0x00000001
#define SMPH_PEEK28_STAT_BITN                                                0
#define SMPH_PEEK28_STAT_M                                          0x00000001
#define SMPH_PEEK28_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK29
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK29_STAT                                            0x00000001
#define SMPH_PEEK29_STAT_BITN                                                0
#define SMPH_PEEK29_STAT_M                                          0x00000001
#define SMPH_PEEK29_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK30
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK30_STAT                                            0x00000001
#define SMPH_PEEK30_STAT_BITN                                                0
#define SMPH_PEEK30_STAT_M                                          0x00000001
#define SMPH_PEEK30_STAT_S                                                   0

//*****************************************************************************
//
// Register: SMPH_O_PEEK31
//
//*****************************************************************************
// Field:     [0] STAT
//
// Status when reading:
//
// 0: Semaphore is taken
// 1: Semaphore is available
//
// Used for semaphore debugging. A read operation will not change register
// value. Register writing is not possible.
#define SMPH_PEEK31_STAT                                            0x00000001
#define SMPH_PEEK31_STAT_BITN                                                0
#define SMPH_PEEK31_STAT_M                                          0x00000001
#define SMPH_PEEK31_STAT_S                                                   0


#endif // __SMPH__
