/******************************************************************************
*  Filename:       hw_cpu_rom_table_h
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

#ifndef __HW_CPU_ROM_TABLE_H__
#define __HW_CPU_ROM_TABLE_H__

//*****************************************************************************
//
// This section defines the register offsets of
// CPU_ROM_TABLE component
//
//*****************************************************************************
// System Control Space Component
#define CPU_ROM_TABLE_O_SCS                                         0x00000000

// Data Watchpoint and Trace Component
#define CPU_ROM_TABLE_O_DWT                                         0x00000004

// Flash Patch and Breakpoint Component
#define CPU_ROM_TABLE_O_FPB                                         0x00000008

// Instrumentation Trace Component
#define CPU_ROM_TABLE_O_ITM                                         0x0000000C

// Trace Port Interface Component
#define CPU_ROM_TABLE_O_TPIU                                        0x00000010

// Enhanced Trace Component
#define CPU_ROM_TABLE_O_ETM                                         0x00000014

// End Marker
#define CPU_ROM_TABLE_O_END                                         0x00000018

// System Memory Map Access for DAP
#define CPU_ROM_TABLE_O_SYSTEM_ACCESS                               0x00000FCC

//*****************************************************************************
//
// Register: CPU_ROM_TABLE_O_SCS
//
//*****************************************************************************
// Field:  [31:0] SCS
//
// Points to the SCS at 0xE000E000.
// (SCS + Base address for ROM_TABLE) & 0x0FFFFFFF0 = 0xE000E000.
#define CPU_ROM_TABLE_SCS_SCS_W                                             32
#define CPU_ROM_TABLE_SCS_SCS_M                                     0xFFFFFFFF
#define CPU_ROM_TABLE_SCS_SCS_S                                              0

//*****************************************************************************
//
// Register: CPU_ROM_TABLE_O_DWT
//
//*****************************************************************************
// Field:  [31:1] DWT
//
// Points to the Data Watchpoint and Trace block at 0xE0001000.
// (2*DWT + Base address for ROM_TABLE) & 0x0FFFFFFF0 = 0xE0001000.
#define CPU_ROM_TABLE_DWT_DWT_W                                             31
#define CPU_ROM_TABLE_DWT_DWT_M                                     0xFFFFFFFE
#define CPU_ROM_TABLE_DWT_DWT_S                                              1

// Field:     [0] DWT_PRESENT
//
// 0: DWT is not present
// 1: DWT is present.
#define CPU_ROM_TABLE_DWT_DWT_PRESENT                               0x00000001
#define CPU_ROM_TABLE_DWT_DWT_PRESENT_BITN                                   0
#define CPU_ROM_TABLE_DWT_DWT_PRESENT_M                             0x00000001
#define CPU_ROM_TABLE_DWT_DWT_PRESENT_S                                      0

//*****************************************************************************
//
// Register: CPU_ROM_TABLE_O_FPB
//
//*****************************************************************************
// Field:  [31:1] FPB
//
// Points to the Flash Patch and Breakpoint block at 0xE0002000.
// (2*FPB + Base address for ROM_TABLE) & 0x0FFFFFFF0 = 0xE0002000.
#define CPU_ROM_TABLE_FPB_FPB_W                                             31
#define CPU_ROM_TABLE_FPB_FPB_M                                     0xFFFFFFFE
#define CPU_ROM_TABLE_FPB_FPB_S                                              1

// Field:     [0] FPB_PRESENT
//
// 0: FPB is not present
// 1: FPB is present.
#define CPU_ROM_TABLE_FPB_FPB_PRESENT                               0x00000001
#define CPU_ROM_TABLE_FPB_FPB_PRESENT_BITN                                   0
#define CPU_ROM_TABLE_FPB_FPB_PRESENT_M                             0x00000001
#define CPU_ROM_TABLE_FPB_FPB_PRESENT_S                                      0

//*****************************************************************************
//
// Register: CPU_ROM_TABLE_O_ITM
//
//*****************************************************************************
// Field:  [31:1] ITM
//
// Points to the Instrumentation Trace block at 0xE0000000.
// (2*ITM + Base address for ROM_TABLE) & 0x0FFFFFFF0 = 0xE0000000.
#define CPU_ROM_TABLE_ITM_ITM_W                                             31
#define CPU_ROM_TABLE_ITM_ITM_M                                     0xFFFFFFFE
#define CPU_ROM_TABLE_ITM_ITM_S                                              1

// Field:     [0] ITM_PRESENT
//
// 0: ITM is not present
// 1: ITM is present.
#define CPU_ROM_TABLE_ITM_ITM_PRESENT                               0x00000001
#define CPU_ROM_TABLE_ITM_ITM_PRESENT_BITN                                   0
#define CPU_ROM_TABLE_ITM_ITM_PRESENT_M                             0x00000001
#define CPU_ROM_TABLE_ITM_ITM_PRESENT_S                                      0

//*****************************************************************************
//
// Register: CPU_ROM_TABLE_O_TPIU
//
//*****************************************************************************
// Field:  [31:1] TPIU
//
// Points to the TPIU. TPIU is at 0xE0040000.
// (2*TPIU + Base address for ROM_TABLE) & 0x0FFFFFFF0 = 0xE0040000.
#define CPU_ROM_TABLE_TPIU_TPIU_W                                           31
#define CPU_ROM_TABLE_TPIU_TPIU_M                                   0xFFFFFFFE
#define CPU_ROM_TABLE_TPIU_TPIU_S                                            1

// Field:     [0] TPIU_PRESENT
//
// 0: TPIU is not present
// 1: TPIU is present.
#define CPU_ROM_TABLE_TPIU_TPIU_PRESENT                             0x00000001
#define CPU_ROM_TABLE_TPIU_TPIU_PRESENT_BITN                                 0
#define CPU_ROM_TABLE_TPIU_TPIU_PRESENT_M                           0x00000001
#define CPU_ROM_TABLE_TPIU_TPIU_PRESENT_S                                    0

//*****************************************************************************
//
// Register: CPU_ROM_TABLE_O_ETM
//
//*****************************************************************************
// Field:  [31:1] ETM
//
// Points to the ETM. ETM is at 0xE0041000.
// (2*ETM + Base address for ROM_TABLE) & 0x0FFFFFFF0 = 0xE0041000.
#define CPU_ROM_TABLE_ETM_ETM_W                                             31
#define CPU_ROM_TABLE_ETM_ETM_M                                     0xFFFFFFFE
#define CPU_ROM_TABLE_ETM_ETM_S                                              1

// Field:     [0] ETM_PRESENT
//
// 0: ETM is not present
// 1: ETM is present.
#define CPU_ROM_TABLE_ETM_ETM_PRESENT                               0x00000001
#define CPU_ROM_TABLE_ETM_ETM_PRESENT_BITN                                   0
#define CPU_ROM_TABLE_ETM_ETM_PRESENT_M                             0x00000001
#define CPU_ROM_TABLE_ETM_ETM_PRESENT_S                                      0

//*****************************************************************************
//
// Register: CPU_ROM_TABLE_O_END
//
//*****************************************************************************
// Field:  [31:0] END
//
// End of the ROM table
#define CPU_ROM_TABLE_END_END_W                                             32
#define CPU_ROM_TABLE_END_END_M                                     0xFFFFFFFF
#define CPU_ROM_TABLE_END_END_S                                              0

//*****************************************************************************
//
// Register: CPU_ROM_TABLE_O_SYSTEM_ACCESS
//
//*****************************************************************************
// Field:     [0] SYSTEM_ACCESS
//
// 1: The system memory map is accessible using the DAP
// 0: Only debug resources are accessible using the DAP
#define CPU_ROM_TABLE_SYSTEM_ACCESS_SYSTEM_ACCESS                   0x00000001
#define CPU_ROM_TABLE_SYSTEM_ACCESS_SYSTEM_ACCESS_BITN                       0
#define CPU_ROM_TABLE_SYSTEM_ACCESS_SYSTEM_ACCESS_M                 0x00000001
#define CPU_ROM_TABLE_SYSTEM_ACCESS_SYSTEM_ACCESS_S                          0


#endif // __CPU_ROM_TABLE__
