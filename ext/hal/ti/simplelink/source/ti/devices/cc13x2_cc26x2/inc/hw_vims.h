/******************************************************************************
*  Filename:       hw_vims_h
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

#ifndef __HW_VIMS_H__
#define __HW_VIMS_H__

//*****************************************************************************
//
// This section defines the register offsets of
// VIMS component
//
//*****************************************************************************
// Status
#define VIMS_O_STAT                                                 0x00000000

// Control
#define VIMS_O_CTL                                                  0x00000004

//*****************************************************************************
//
// Register: VIMS_O_STAT
//
//*****************************************************************************
// Field:     [5] IDCODE_LB_DIS
//
// Icode/Dcode flash line buffer status
//
// 0: Enabled or in transition to disabled
// 1: Disabled and flushed
#define VIMS_STAT_IDCODE_LB_DIS                                     0x00000020
#define VIMS_STAT_IDCODE_LB_DIS_BITN                                         5
#define VIMS_STAT_IDCODE_LB_DIS_M                                   0x00000020
#define VIMS_STAT_IDCODE_LB_DIS_S                                            5

// Field:     [4] SYSBUS_LB_DIS
//
// Sysbus flash line buffer control
//
// 0: Enabled or in transition to disabled
// 1: Disabled and flushed
#define VIMS_STAT_SYSBUS_LB_DIS                                     0x00000010
#define VIMS_STAT_SYSBUS_LB_DIS_BITN                                         4
#define VIMS_STAT_SYSBUS_LB_DIS_M                                   0x00000010
#define VIMS_STAT_SYSBUS_LB_DIS_S                                            4

// Field:     [3] MODE_CHANGING
//
// VIMS mode change status
//
// 0: VIMS is in the mode defined by MODE
// 1: VIMS is in the process of changing to the mode given in CTL.MODE
#define VIMS_STAT_MODE_CHANGING                                     0x00000008
#define VIMS_STAT_MODE_CHANGING_BITN                                         3
#define VIMS_STAT_MODE_CHANGING_M                                   0x00000008
#define VIMS_STAT_MODE_CHANGING_S                                            3

// Field:     [2] INV
//
// This bit is set when invalidation of the cache memory is active / ongoing
#define VIMS_STAT_INV                                               0x00000004
#define VIMS_STAT_INV_BITN                                                   2
#define VIMS_STAT_INV_M                                             0x00000004
#define VIMS_STAT_INV_S                                                      2

// Field:   [1:0] MODE
//
// Current VIMS mode
// ENUMs:
// OFF                      VIMS Off mode
// CACHE                    VIMS Cache mode
// GPRAM                    VIMS GPRAM mode
#define VIMS_STAT_MODE_W                                                     2
#define VIMS_STAT_MODE_M                                            0x00000003
#define VIMS_STAT_MODE_S                                                     0
#define VIMS_STAT_MODE_OFF                                          0x00000003
#define VIMS_STAT_MODE_CACHE                                        0x00000001
#define VIMS_STAT_MODE_GPRAM                                        0x00000000

//*****************************************************************************
//
// Register: VIMS_O_CTL
//
//*****************************************************************************
// Field:    [31] STATS_CLR
//
// Set this bit to clear statistic counters.
#define VIMS_CTL_STATS_CLR                                          0x80000000
#define VIMS_CTL_STATS_CLR_BITN                                             31
#define VIMS_CTL_STATS_CLR_M                                        0x80000000
#define VIMS_CTL_STATS_CLR_S                                                31

// Field:    [30] STATS_EN
//
// Set this bit to enable statistic counters.
#define VIMS_CTL_STATS_EN                                           0x40000000
#define VIMS_CTL_STATS_EN_BITN                                              30
#define VIMS_CTL_STATS_EN_M                                         0x40000000
#define VIMS_CTL_STATS_EN_S                                                 30

// Field:    [29] DYN_CG_EN
//
// 0: The in-built clock gate functionality is bypassed.
// 1: The in-built clock gate functionality is enabled, automatically gating
// the clock when not needed.
#define VIMS_CTL_DYN_CG_EN                                          0x20000000
#define VIMS_CTL_DYN_CG_EN_BITN                                             29
#define VIMS_CTL_DYN_CG_EN_M                                        0x20000000
#define VIMS_CTL_DYN_CG_EN_S                                                29

// Field:     [5] IDCODE_LB_DIS
//
// Icode/Dcode flash line buffer control
//
// 0: Enable
// 1: Disable
#define VIMS_CTL_IDCODE_LB_DIS                                      0x00000020
#define VIMS_CTL_IDCODE_LB_DIS_BITN                                          5
#define VIMS_CTL_IDCODE_LB_DIS_M                                    0x00000020
#define VIMS_CTL_IDCODE_LB_DIS_S                                             5

// Field:     [4] SYSBUS_LB_DIS
//
// Sysbus flash line buffer control
//
// 0: Enable
// 1: Disable
#define VIMS_CTL_SYSBUS_LB_DIS                                      0x00000010
#define VIMS_CTL_SYSBUS_LB_DIS_BITN                                          4
#define VIMS_CTL_SYSBUS_LB_DIS_M                                    0x00000010
#define VIMS_CTL_SYSBUS_LB_DIS_S                                             4

// Field:     [3] ARB_CFG
//
// Icode/Dcode and sysbus arbitation scheme
//
// 0: Static arbitration  (icode/docde > sysbus)
// 1: Round-robin arbitration
#define VIMS_CTL_ARB_CFG                                            0x00000008
#define VIMS_CTL_ARB_CFG_BITN                                                3
#define VIMS_CTL_ARB_CFG_M                                          0x00000008
#define VIMS_CTL_ARB_CFG_S                                                   3

// Field:     [2] PREF_EN
//
// Tag prefetch control
//
// 0: Disabled
// 1: Enabled
#define VIMS_CTL_PREF_EN                                            0x00000004
#define VIMS_CTL_PREF_EN_BITN                                                2
#define VIMS_CTL_PREF_EN_M                                          0x00000004
#define VIMS_CTL_PREF_EN_S                                                   2

// Field:   [1:0] MODE
//
// VIMS mode request.
// Write accesses to this field will be blocked while STAT.MODE_CHANGING is set
// to 1.
// ENUMs:
// OFF                      VIMS Off mode
// CACHE                    VIMS Cache mode
// GPRAM                    VIMS GPRAM mode
#define VIMS_CTL_MODE_W                                                      2
#define VIMS_CTL_MODE_M                                             0x00000003
#define VIMS_CTL_MODE_S                                                      0
#define VIMS_CTL_MODE_OFF                                           0x00000003
#define VIMS_CTL_MODE_CACHE                                         0x00000001
#define VIMS_CTL_MODE_GPRAM                                         0x00000000


#endif // __VIMS__
