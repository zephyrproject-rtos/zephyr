/******************************************************************************
*  Filename:       hw_aux_sce_h
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

#ifndef __HW_AUX_SCE_H__
#define __HW_AUX_SCE_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AUX_SCE component
//
//*****************************************************************************
// Internal
#define AUX_SCE_O_CTL                                               0x00000000

// Internal
#define AUX_SCE_O_FETCHSTAT                                         0x00000004

// Internal
#define AUX_SCE_O_CPUSTAT                                           0x00000008

// Internal
#define AUX_SCE_O_WUSTAT                                            0x0000000C

// Internal
#define AUX_SCE_O_REG1_0                                            0x00000010

// Internal
#define AUX_SCE_O_REG3_2                                            0x00000014

// Internal
#define AUX_SCE_O_REG5_4                                            0x00000018

// Internal
#define AUX_SCE_O_REG7_6                                            0x0000001C

// Internal
#define AUX_SCE_O_LOOPADDR                                          0x00000020

// Internal
#define AUX_SCE_O_LOOPCNT                                           0x00000024

//*****************************************************************************
//
// Register: AUX_SCE_O_CTL
//
//*****************************************************************************
// Field: [31:24] FORCE_EV_LOW
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_FORCE_EV_LOW_W                                           8
#define AUX_SCE_CTL_FORCE_EV_LOW_M                                  0xFF000000
#define AUX_SCE_CTL_FORCE_EV_LOW_S                                          24

// Field: [23:16] FORCE_EV_HIGH
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_FORCE_EV_HIGH_W                                          8
#define AUX_SCE_CTL_FORCE_EV_HIGH_M                                 0x00FF0000
#define AUX_SCE_CTL_FORCE_EV_HIGH_S                                         16

// Field:  [15:8] RESET_VECTOR
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_RESET_VECTOR_W                                           8
#define AUX_SCE_CTL_RESET_VECTOR_M                                  0x0000FF00
#define AUX_SCE_CTL_RESET_VECTOR_S                                           8

// Field:     [6] DBG_FREEZE_EN
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_DBG_FREEZE_EN                                   0x00000040
#define AUX_SCE_CTL_DBG_FREEZE_EN_BITN                                       6
#define AUX_SCE_CTL_DBG_FREEZE_EN_M                                 0x00000040
#define AUX_SCE_CTL_DBG_FREEZE_EN_S                                          6

// Field:     [5] FORCE_WU_LOW
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_FORCE_WU_LOW                                    0x00000020
#define AUX_SCE_CTL_FORCE_WU_LOW_BITN                                        5
#define AUX_SCE_CTL_FORCE_WU_LOW_M                                  0x00000020
#define AUX_SCE_CTL_FORCE_WU_LOW_S                                           5

// Field:     [4] FORCE_WU_HIGH
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_FORCE_WU_HIGH                                   0x00000010
#define AUX_SCE_CTL_FORCE_WU_HIGH_BITN                                       4
#define AUX_SCE_CTL_FORCE_WU_HIGH_M                                 0x00000010
#define AUX_SCE_CTL_FORCE_WU_HIGH_S                                          4

// Field:     [3] RESTART
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_RESTART                                         0x00000008
#define AUX_SCE_CTL_RESTART_BITN                                             3
#define AUX_SCE_CTL_RESTART_M                                       0x00000008
#define AUX_SCE_CTL_RESTART_S                                                3

// Field:     [2] SINGLE_STEP
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_SINGLE_STEP                                     0x00000004
#define AUX_SCE_CTL_SINGLE_STEP_BITN                                         2
#define AUX_SCE_CTL_SINGLE_STEP_M                                   0x00000004
#define AUX_SCE_CTL_SINGLE_STEP_S                                            2

// Field:     [1] SUSPEND
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_SUSPEND                                         0x00000002
#define AUX_SCE_CTL_SUSPEND_BITN                                             1
#define AUX_SCE_CTL_SUSPEND_M                                       0x00000002
#define AUX_SCE_CTL_SUSPEND_S                                                1

// Field:     [0] CLK_EN
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CTL_CLK_EN                                          0x00000001
#define AUX_SCE_CTL_CLK_EN_BITN                                              0
#define AUX_SCE_CTL_CLK_EN_M                                        0x00000001
#define AUX_SCE_CTL_CLK_EN_S                                                 0

//*****************************************************************************
//
// Register: AUX_SCE_O_FETCHSTAT
//
//*****************************************************************************
// Field: [31:16] OPCODE
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_FETCHSTAT_OPCODE_W                                          16
#define AUX_SCE_FETCHSTAT_OPCODE_M                                  0xFFFF0000
#define AUX_SCE_FETCHSTAT_OPCODE_S                                          16

// Field:  [15:0] PC
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_FETCHSTAT_PC_W                                              16
#define AUX_SCE_FETCHSTAT_PC_M                                      0x0000FFFF
#define AUX_SCE_FETCHSTAT_PC_S                                               0

//*****************************************************************************
//
// Register: AUX_SCE_O_CPUSTAT
//
//*****************************************************************************
// Field:    [11] BUS_ERROR
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CPUSTAT_BUS_ERROR                                   0x00000800
#define AUX_SCE_CPUSTAT_BUS_ERROR_BITN                                      11
#define AUX_SCE_CPUSTAT_BUS_ERROR_M                                 0x00000800
#define AUX_SCE_CPUSTAT_BUS_ERROR_S                                         11

// Field:    [10] SLEEP
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CPUSTAT_SLEEP                                       0x00000400
#define AUX_SCE_CPUSTAT_SLEEP_BITN                                          10
#define AUX_SCE_CPUSTAT_SLEEP_M                                     0x00000400
#define AUX_SCE_CPUSTAT_SLEEP_S                                             10

// Field:     [9] WEV
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CPUSTAT_WEV                                         0x00000200
#define AUX_SCE_CPUSTAT_WEV_BITN                                             9
#define AUX_SCE_CPUSTAT_WEV_M                                       0x00000200
#define AUX_SCE_CPUSTAT_WEV_S                                                9

// Field:     [8] HALTED
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CPUSTAT_HALTED                                      0x00000100
#define AUX_SCE_CPUSTAT_HALTED_BITN                                          8
#define AUX_SCE_CPUSTAT_HALTED_M                                    0x00000100
#define AUX_SCE_CPUSTAT_HALTED_S                                             8

// Field:     [3] V_FLAG
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CPUSTAT_V_FLAG                                      0x00000008
#define AUX_SCE_CPUSTAT_V_FLAG_BITN                                          3
#define AUX_SCE_CPUSTAT_V_FLAG_M                                    0x00000008
#define AUX_SCE_CPUSTAT_V_FLAG_S                                             3

// Field:     [2] C_FLAG
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CPUSTAT_C_FLAG                                      0x00000004
#define AUX_SCE_CPUSTAT_C_FLAG_BITN                                          2
#define AUX_SCE_CPUSTAT_C_FLAG_M                                    0x00000004
#define AUX_SCE_CPUSTAT_C_FLAG_S                                             2

// Field:     [1] N_FLAG
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CPUSTAT_N_FLAG                                      0x00000002
#define AUX_SCE_CPUSTAT_N_FLAG_BITN                                          1
#define AUX_SCE_CPUSTAT_N_FLAG_M                                    0x00000002
#define AUX_SCE_CPUSTAT_N_FLAG_S                                             1

// Field:     [0] Z_FLAG
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_CPUSTAT_Z_FLAG                                      0x00000001
#define AUX_SCE_CPUSTAT_Z_FLAG_BITN                                          0
#define AUX_SCE_CPUSTAT_Z_FLAG_M                                    0x00000001
#define AUX_SCE_CPUSTAT_Z_FLAG_S                                             0

//*****************************************************************************
//
// Register: AUX_SCE_O_WUSTAT
//
//*****************************************************************************
// Field: [18:16] EXC_VECTOR
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_WUSTAT_EXC_VECTOR_W                                          3
#define AUX_SCE_WUSTAT_EXC_VECTOR_M                                 0x00070000
#define AUX_SCE_WUSTAT_EXC_VECTOR_S                                         16

// Field:     [8] WU_SIGNAL
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_WUSTAT_WU_SIGNAL                                    0x00000100
#define AUX_SCE_WUSTAT_WU_SIGNAL_BITN                                        8
#define AUX_SCE_WUSTAT_WU_SIGNAL_M                                  0x00000100
#define AUX_SCE_WUSTAT_WU_SIGNAL_S                                           8

// Field:   [7:0] EV_SIGNALS
//
// Internal. Only to be used through TI provided API.
// ENUMs:
// SCEWEV_PROG              Internal. Only to be used through TI provided API.
// AUX_ADC_FIFO_NOT_EMPTY   Internal. Only to be used through TI provided API.
// AUX_TIMER1_EV_OR_IDLE    Internal. Only to be used through TI provided API.
// AUX_TIMER0_EV_OR_IDLE    Internal. Only to be used through TI provided API.
// AUX_TDC_DONE             Internal. Only to be used through TI provided API.
// AUX_COMPB                Internal. Only to be used through TI provided API.
// AUX_COMPA                Internal. Only to be used through TI provided API.
// AUX_PROG_DLY_IDLE        Internal. Only to be used through TI provided API.
#define AUX_SCE_WUSTAT_EV_SIGNALS_W                                          8
#define AUX_SCE_WUSTAT_EV_SIGNALS_M                                 0x000000FF
#define AUX_SCE_WUSTAT_EV_SIGNALS_S                                          0
#define AUX_SCE_WUSTAT_EV_SIGNALS_SCEWEV_PROG                       0x00000080
#define AUX_SCE_WUSTAT_EV_SIGNALS_AUX_ADC_FIFO_NOT_EMPTY            0x00000040
#define AUX_SCE_WUSTAT_EV_SIGNALS_AUX_TIMER1_EV_OR_IDLE             0x00000020
#define AUX_SCE_WUSTAT_EV_SIGNALS_AUX_TIMER0_EV_OR_IDLE             0x00000010
#define AUX_SCE_WUSTAT_EV_SIGNALS_AUX_TDC_DONE                      0x00000008
#define AUX_SCE_WUSTAT_EV_SIGNALS_AUX_COMPB                         0x00000004
#define AUX_SCE_WUSTAT_EV_SIGNALS_AUX_COMPA                         0x00000002
#define AUX_SCE_WUSTAT_EV_SIGNALS_AUX_PROG_DLY_IDLE                 0x00000001

//*****************************************************************************
//
// Register: AUX_SCE_O_REG1_0
//
//*****************************************************************************
// Field: [31:16] REG1
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_REG1_0_REG1_W                                               16
#define AUX_SCE_REG1_0_REG1_M                                       0xFFFF0000
#define AUX_SCE_REG1_0_REG1_S                                               16

// Field:  [15:0] REG0
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_REG1_0_REG0_W                                               16
#define AUX_SCE_REG1_0_REG0_M                                       0x0000FFFF
#define AUX_SCE_REG1_0_REG0_S                                                0

//*****************************************************************************
//
// Register: AUX_SCE_O_REG3_2
//
//*****************************************************************************
// Field: [31:16] REG3
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_REG3_2_REG3_W                                               16
#define AUX_SCE_REG3_2_REG3_M                                       0xFFFF0000
#define AUX_SCE_REG3_2_REG3_S                                               16

// Field:  [15:0] REG2
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_REG3_2_REG2_W                                               16
#define AUX_SCE_REG3_2_REG2_M                                       0x0000FFFF
#define AUX_SCE_REG3_2_REG2_S                                                0

//*****************************************************************************
//
// Register: AUX_SCE_O_REG5_4
//
//*****************************************************************************
// Field: [31:16] REG5
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_REG5_4_REG5_W                                               16
#define AUX_SCE_REG5_4_REG5_M                                       0xFFFF0000
#define AUX_SCE_REG5_4_REG5_S                                               16

// Field:  [15:0] REG4
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_REG5_4_REG4_W                                               16
#define AUX_SCE_REG5_4_REG4_M                                       0x0000FFFF
#define AUX_SCE_REG5_4_REG4_S                                                0

//*****************************************************************************
//
// Register: AUX_SCE_O_REG7_6
//
//*****************************************************************************
// Field: [31:16] REG7
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_REG7_6_REG7_W                                               16
#define AUX_SCE_REG7_6_REG7_M                                       0xFFFF0000
#define AUX_SCE_REG7_6_REG7_S                                               16

// Field:  [15:0] REG6
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_REG7_6_REG6_W                                               16
#define AUX_SCE_REG7_6_REG6_M                                       0x0000FFFF
#define AUX_SCE_REG7_6_REG6_S                                                0

//*****************************************************************************
//
// Register: AUX_SCE_O_LOOPADDR
//
//*****************************************************************************
// Field: [31:16] STOP
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_LOOPADDR_STOP_W                                             16
#define AUX_SCE_LOOPADDR_STOP_M                                     0xFFFF0000
#define AUX_SCE_LOOPADDR_STOP_S                                             16

// Field:  [15:0] START
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_LOOPADDR_START_W                                            16
#define AUX_SCE_LOOPADDR_START_M                                    0x0000FFFF
#define AUX_SCE_LOOPADDR_START_S                                             0

//*****************************************************************************
//
// Register: AUX_SCE_O_LOOPCNT
//
//*****************************************************************************
// Field:   [7:0] ITER_LEFT
//
// Internal. Only to be used through TI provided API.
#define AUX_SCE_LOOPCNT_ITER_LEFT_W                                          8
#define AUX_SCE_LOOPCNT_ITER_LEFT_M                                 0x000000FF
#define AUX_SCE_LOOPCNT_ITER_LEFT_S                                          0


#endif // __AUX_SCE__
