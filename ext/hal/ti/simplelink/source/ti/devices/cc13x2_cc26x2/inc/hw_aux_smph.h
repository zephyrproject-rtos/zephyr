/******************************************************************************
*  Filename:       hw_aux_smph_h
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

#ifndef __HW_AUX_SMPH_H__
#define __HW_AUX_SMPH_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_SMPH component
//
//*****************************************************************************
// Semaphore 0
#define AUX_SMPH_O_SMPH0                                            0x00000000

// Semaphore 1
#define AUX_SMPH_O_SMPH1                                            0x00000004

// Semaphore 2
#define AUX_SMPH_O_SMPH2                                            0x00000008

// Semaphore 3
#define AUX_SMPH_O_SMPH3                                            0x0000000C

// Semaphore 4
#define AUX_SMPH_O_SMPH4                                            0x00000010

// Semaphore 5
#define AUX_SMPH_O_SMPH5                                            0x00000014

// Semaphore 6
#define AUX_SMPH_O_SMPH6                                            0x00000018

// Semaphore 7
#define AUX_SMPH_O_SMPH7                                            0x0000001C

// Auto Take
#define AUX_SMPH_O_AUTOTAKE                                         0x00000020

//*****************************************************************************
//
// Register: AUX_SMPH_O_SMPH0
//
//*****************************************************************************
// Field:     [0] STAT
//
// Request or release of semaphore.
//
// Request by read:
//
// 0: Semaphore not available.
// 1: Semaphore granted.
//
// Release by write:
//
// 0: Do not use.
// 1: Release semaphore.
#define AUX_SMPH_SMPH0_STAT                                         0x00000001
#define AUX_SMPH_SMPH0_STAT_BITN                                             0
#define AUX_SMPH_SMPH0_STAT_M                                       0x00000001
#define AUX_SMPH_SMPH0_STAT_S                                                0

//*****************************************************************************
//
// Register: AUX_SMPH_O_SMPH1
//
//*****************************************************************************
// Field:     [0] STAT
//
// Request or release of semaphore.
//
// Request by read:
//
// 0: Semaphore not available.
// 1: Semaphore granted.
//
// Release by write:
//
// 0: Do not use.
// 1: Release semaphore.
#define AUX_SMPH_SMPH1_STAT                                         0x00000001
#define AUX_SMPH_SMPH1_STAT_BITN                                             0
#define AUX_SMPH_SMPH1_STAT_M                                       0x00000001
#define AUX_SMPH_SMPH1_STAT_S                                                0

//*****************************************************************************
//
// Register: AUX_SMPH_O_SMPH2
//
//*****************************************************************************
// Field:     [0] STAT
//
// Request or release of semaphore.
//
// Request by read:
//
// 0: Semaphore not available.
// 1: Semaphore granted.
//
// Release by write:
//
// 0: Do not use.
// 1: Release semaphore.
#define AUX_SMPH_SMPH2_STAT                                         0x00000001
#define AUX_SMPH_SMPH2_STAT_BITN                                             0
#define AUX_SMPH_SMPH2_STAT_M                                       0x00000001
#define AUX_SMPH_SMPH2_STAT_S                                                0

//*****************************************************************************
//
// Register: AUX_SMPH_O_SMPH3
//
//*****************************************************************************
// Field:     [0] STAT
//
// Request or release of semaphore.
//
// Request by read:
//
// 0: Semaphore not available.
// 1: Semaphore granted.
//
// Release by write:
//
// 0: Do not use.
// 1: Release semaphore.
#define AUX_SMPH_SMPH3_STAT                                         0x00000001
#define AUX_SMPH_SMPH3_STAT_BITN                                             0
#define AUX_SMPH_SMPH3_STAT_M                                       0x00000001
#define AUX_SMPH_SMPH3_STAT_S                                                0

//*****************************************************************************
//
// Register: AUX_SMPH_O_SMPH4
//
//*****************************************************************************
// Field:     [0] STAT
//
// Request or release of semaphore.
//
// Request by read:
//
// 0: Semaphore not available.
// 1: Semaphore granted.
//
// Release by write:
//
// 0: Do not use.
// 1: Release semaphore.
#define AUX_SMPH_SMPH4_STAT                                         0x00000001
#define AUX_SMPH_SMPH4_STAT_BITN                                             0
#define AUX_SMPH_SMPH4_STAT_M                                       0x00000001
#define AUX_SMPH_SMPH4_STAT_S                                                0

//*****************************************************************************
//
// Register: AUX_SMPH_O_SMPH5
//
//*****************************************************************************
// Field:     [0] STAT
//
// Request or release of semaphore.
//
// Request by read:
//
// 0: Semaphore not available.
// 1: Semaphore granted.
//
// Release by write:
//
// 0: Do not use.
// 1: Release semaphore.
#define AUX_SMPH_SMPH5_STAT                                         0x00000001
#define AUX_SMPH_SMPH5_STAT_BITN                                             0
#define AUX_SMPH_SMPH5_STAT_M                                       0x00000001
#define AUX_SMPH_SMPH5_STAT_S                                                0

//*****************************************************************************
//
// Register: AUX_SMPH_O_SMPH6
//
//*****************************************************************************
// Field:     [0] STAT
//
// Request or release of semaphore.
//
// Request by read:
//
// 0: Semaphore not available.
// 1: Semaphore granted.
//
// Release by write:
//
// 0: Do not use.
// 1: Release semaphore.
#define AUX_SMPH_SMPH6_STAT                                         0x00000001
#define AUX_SMPH_SMPH6_STAT_BITN                                             0
#define AUX_SMPH_SMPH6_STAT_M                                       0x00000001
#define AUX_SMPH_SMPH6_STAT_S                                                0

//*****************************************************************************
//
// Register: AUX_SMPH_O_SMPH7
//
//*****************************************************************************
// Field:     [0] STAT
//
// Request or release of semaphore.
//
// Request by read:
//
// 0: Semaphore not available.
// 1: Semaphore granted.
//
// Release by write:
//
// 0: Do not use.
// 1: Release semaphore.
#define AUX_SMPH_SMPH7_STAT                                         0x00000001
#define AUX_SMPH_SMPH7_STAT_BITN                                             0
#define AUX_SMPH_SMPH7_STAT_M                                       0x00000001
#define AUX_SMPH_SMPH7_STAT_S                                                0

//*****************************************************************************
//
// Register: AUX_SMPH_O_AUTOTAKE
//
//*****************************************************************************
// Field:   [2:0] SMPH_ID
//
// Write the semaphore ID,0x0-0x7, to SMPH_ID to request this semaphore until
// it is granted.
//
// When semaphore SMPH_ID is granted, event
// AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE becomes 1.
//
// The event becomes 0 when software releases the semaphore or writes a new
// value to SMPH_ID.
//
// To avoid corrupted semaphores:
// - Usage of this functionality must be restricted to one CPU core.
// - Software must wait until AUX_EVCTL:EVSTAT3.AUX_SMPH_AUTOTAKE_DONE is 1
// before it writes a new value to SMPH_ID.
#define AUX_SMPH_AUTOTAKE_SMPH_ID_W                                          3
#define AUX_SMPH_AUTOTAKE_SMPH_ID_M                                 0x00000007
#define AUX_SMPH_AUTOTAKE_SMPH_ID_S                                          0


#endif // __AUX_SMPH__
