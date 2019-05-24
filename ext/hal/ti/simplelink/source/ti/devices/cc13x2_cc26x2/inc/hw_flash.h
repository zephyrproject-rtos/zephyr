/******************************************************************************
*  Filename:       hw_flash_h
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

#ifndef __HW_FLASH_H__
#define __HW_FLASH_H__

//*****************************************************************************
//
// This section defines the register offsets of
// FLASH component
//
//*****************************************************************************
// FMC and Efuse Status
#define FLASH_O_STAT                                                0x0000001C

// Internal
#define FLASH_O_CFG                                                 0x00000024

// Internal
#define FLASH_O_SYSCODE_START                                       0x00000028

// Internal
#define FLASH_O_FLASH_SIZE                                          0x0000002C

// Internal
#define FLASH_O_FWLOCK                                              0x0000003C

// Internal
#define FLASH_O_FWFLAG                                              0x00000040

// Internal
#define FLASH_O_EFUSE                                               0x00001000

// Internal
#define FLASH_O_EFUSEADDR                                           0x00001004

// Internal
#define FLASH_O_DATAUPPER                                           0x00001008

// Internal
#define FLASH_O_DATALOWER                                           0x0000100C

// Internal
#define FLASH_O_EFUSECFG                                            0x00001010

// Internal
#define FLASH_O_EFUSESTAT                                           0x00001014

// Internal
#define FLASH_O_ACC                                                 0x00001018

// Internal
#define FLASH_O_BOUNDARY                                            0x0000101C

// Internal
#define FLASH_O_EFUSEFLAG                                           0x00001020

// Internal
#define FLASH_O_EFUSEKEY                                            0x00001024

// Internal
#define FLASH_O_EFUSERELEASE                                        0x00001028

// Internal
#define FLASH_O_EFUSEPINS                                           0x0000102C

// Internal
#define FLASH_O_EFUSECRA                                            0x00001030

// Internal
#define FLASH_O_EFUSEREAD                                           0x00001034

// Internal
#define FLASH_O_EFUSEPROGRAM                                        0x00001038

// Internal
#define FLASH_O_EFUSEERROR                                          0x0000103C

// Internal
#define FLASH_O_SINGLEBIT                                           0x00001040

// Internal
#define FLASH_O_TWOBIT                                              0x00001044

// Internal
#define FLASH_O_SELFTESTCYC                                         0x00001048

// Internal
#define FLASH_O_SELFTESTSIGN                                        0x0000104C

// Internal
#define FLASH_O_FRDCTL                                              0x00002000

// Internal
#define FLASH_O_FSPRD                                               0x00002004

// Internal
#define FLASH_O_FEDACCTL1                                           0x00002008

// Internal
#define FLASH_O_FEDACSTAT                                           0x0000201C

// Internal
#define FLASH_O_FBPROT                                              0x00002030

// Internal
#define FLASH_O_FBSE                                                0x00002034

// Internal
#define FLASH_O_FBBUSY                                              0x00002038

// Internal
#define FLASH_O_FBAC                                                0x0000203C

// Internal
#define FLASH_O_FBFALLBACK                                          0x00002040

// Internal
#define FLASH_O_FBPRDY                                              0x00002044

// Internal
#define FLASH_O_FPAC1                                               0x00002048

// Internal
#define FLASH_O_FPAC2                                               0x0000204C

// Internal
#define FLASH_O_FMAC                                                0x00002050

// Internal
#define FLASH_O_FMSTAT                                              0x00002054

// Internal
#define FLASH_O_FLOCK                                               0x00002064

// Internal
#define FLASH_O_FVREADCT                                            0x00002080

// Internal
#define FLASH_O_FVHVCT1                                             0x00002084

// Internal
#define FLASH_O_FVHVCT2                                             0x00002088

// Internal
#define FLASH_O_FVHVCT3                                             0x0000208C

// Internal
#define FLASH_O_FVNVCT                                              0x00002090

// Internal
#define FLASH_O_FVSLP                                               0x00002094

// Internal
#define FLASH_O_FVWLCT                                              0x00002098

// Internal
#define FLASH_O_FEFUSECTL                                           0x0000209C

// Internal
#define FLASH_O_FEFUSESTAT                                          0x000020A0

// Internal
#define FLASH_O_FEFUSEDATA                                          0x000020A4

// Internal
#define FLASH_O_FSEQPMP                                             0x000020A8

// Internal
#define FLASH_O_FBSTROBES                                           0x00002100

// Internal
#define FLASH_O_FPSTROBES                                           0x00002104

// Internal
#define FLASH_O_FBMODE                                              0x00002108

// Internal
#define FLASH_O_FTCR                                                0x0000210C

// Internal
#define FLASH_O_FADDR                                               0x00002110

// Internal
#define FLASH_O_FTCTL                                               0x0000211C

// Internal
#define FLASH_O_FWPWRITE0                                           0x00002120

// Internal
#define FLASH_O_FWPWRITE1                                           0x00002124

// Internal
#define FLASH_O_FWPWRITE2                                           0x00002128

// Internal
#define FLASH_O_FWPWRITE3                                           0x0000212C

// Internal
#define FLASH_O_FWPWRITE4                                           0x00002130

// Internal
#define FLASH_O_FWPWRITE5                                           0x00002134

// Internal
#define FLASH_O_FWPWRITE6                                           0x00002138

// Internal
#define FLASH_O_FWPWRITE7                                           0x0000213C

// Internal
#define FLASH_O_FWPWRITE_ECC                                        0x00002140

// Internal
#define FLASH_O_FSWSTAT                                             0x00002144

// Internal
#define FLASH_O_FSM_GLBCTL                                          0x00002200

// Internal
#define FLASH_O_FSM_STATE                                           0x00002204

// Internal
#define FLASH_O_FSM_STAT                                            0x00002208

// Internal
#define FLASH_O_FSM_CMD                                             0x0000220C

// Internal
#define FLASH_O_FSM_PE_OSU                                          0x00002210

// Internal
#define FLASH_O_FSM_VSTAT                                           0x00002214

// Internal
#define FLASH_O_FSM_PE_VSU                                          0x00002218

// Internal
#define FLASH_O_FSM_CMP_VSU                                         0x0000221C

// Internal
#define FLASH_O_FSM_EX_VAL                                          0x00002220

// Internal
#define FLASH_O_FSM_RD_H                                            0x00002224

// Internal
#define FLASH_O_FSM_P_OH                                            0x00002228

// Internal
#define FLASH_O_FSM_ERA_OH                                          0x0000222C

// Internal
#define FLASH_O_FSM_SAV_PPUL                                        0x00002230

// Internal
#define FLASH_O_FSM_PE_VH                                           0x00002234

// Internal
#define FLASH_O_FSM_PRG_PW                                          0x00002240

// Internal
#define FLASH_O_FSM_ERA_PW                                          0x00002244

// Internal
#define FLASH_O_FSM_SAV_ERA_PUL                                     0x00002254

// Internal
#define FLASH_O_FSM_TIMER                                           0x00002258

// Internal
#define FLASH_O_FSM_MODE                                            0x0000225C

// Internal
#define FLASH_O_FSM_PGM                                             0x00002260

// Internal
#define FLASH_O_FSM_ERA                                             0x00002264

// Internal
#define FLASH_O_FSM_PRG_PUL                                         0x00002268

// Internal
#define FLASH_O_FSM_ERA_PUL                                         0x0000226C

// Internal
#define FLASH_O_FSM_STEP_SIZE                                       0x00002270

// Internal
#define FLASH_O_FSM_PUL_CNTR                                        0x00002274

// Internal
#define FLASH_O_FSM_EC_STEP_HEIGHT                                  0x00002278

// Internal
#define FLASH_O_FSM_ST_MACHINE                                      0x0000227C

// Internal
#define FLASH_O_FSM_FLES                                            0x00002280

// Internal
#define FLASH_O_FSM_WR_ENA                                          0x00002288

// Internal
#define FLASH_O_FSM_ACC_PP                                          0x0000228C

// Internal
#define FLASH_O_FSM_ACC_EP                                          0x00002290

// Internal
#define FLASH_O_FSM_ADDR                                            0x000022A0

// Internal
#define FLASH_O_FSM_SECTOR                                          0x000022A4

// Internal
#define FLASH_O_FMC_REV_ID                                          0x000022A8

// Internal
#define FLASH_O_FSM_ERR_ADDR                                        0x000022AC

// Internal
#define FLASH_O_FSM_PGM_MAXPUL                                      0x000022B0

// Internal
#define FLASH_O_FSM_EXECUTE                                         0x000022B4

// Internal
#define FLASH_O_FSM_SECTOR1                                         0x000022C0

// Internal
#define FLASH_O_FSM_SECTOR2                                         0x000022C4

// Internal
#define FLASH_O_FSM_BSLE0                                           0x000022E0

// Internal
#define FLASH_O_FSM_BSLE1                                           0x000022E4

// Internal
#define FLASH_O_FSM_BSLP0                                           0x000022F0

// Internal
#define FLASH_O_FSM_BSLP1                                           0x000022F4

// FMC FSM Enable 128-bit Wide Programming
#define FLASH_O_FSM_PGM128                                          0x000022F8

// Internal
#define FLASH_O_FCFG_BANK                                           0x00002400

// Internal
#define FLASH_O_FCFG_WRAPPER                                        0x00002404

// Internal
#define FLASH_O_FCFG_BNK_TYPE                                       0x00002408

// Internal
#define FLASH_O_FCFG_B0_START                                       0x00002410

// Internal
#define FLASH_O_FCFG_B1_START                                       0x00002414

// Internal
#define FLASH_O_FCFG_B2_START                                       0x00002418

// Internal
#define FLASH_O_FCFG_B3_START                                       0x0000241C

// Internal
#define FLASH_O_FCFG_B4_START                                       0x00002420

// Internal
#define FLASH_O_FCFG_B5_START                                       0x00002424

// Internal
#define FLASH_O_FCFG_B6_START                                       0x00002428

// Internal
#define FLASH_O_FCFG_B7_START                                       0x0000242C

// Internal
#define FLASH_O_FCFG_B0_SSIZE0                                      0x00002430

//*****************************************************************************
//
// Register: FLASH_O_STAT
//
//*****************************************************************************
// Field:    [15] EFUSE_BLANK
//
// Efuse scanning detected if fuse ROM is blank:
// 0 : Not blank
// 1 : Blank
#define FLASH_STAT_EFUSE_BLANK                                      0x00008000
#define FLASH_STAT_EFUSE_BLANK_BITN                                         15
#define FLASH_STAT_EFUSE_BLANK_M                                    0x00008000
#define FLASH_STAT_EFUSE_BLANK_S                                            15

// Field:    [14] EFUSE_TIMEOUT
//
// Efuse scanning resulted in timeout error.
// 0 : No Timeout error
// 1 : Timeout Error
#define FLASH_STAT_EFUSE_TIMEOUT                                    0x00004000
#define FLASH_STAT_EFUSE_TIMEOUT_BITN                                       14
#define FLASH_STAT_EFUSE_TIMEOUT_M                                  0x00004000
#define FLASH_STAT_EFUSE_TIMEOUT_S                                          14

// Field:    [13] SPRS_BYTE_NOT_OK
//
// Efuse scanning resulted in scan chain Sparse byte error.
// 0 : No Sparse error
// 1 : Sparse Error
#define FLASH_STAT_SPRS_BYTE_NOT_OK                                 0x00002000
#define FLASH_STAT_SPRS_BYTE_NOT_OK_BITN                                    13
#define FLASH_STAT_SPRS_BYTE_NOT_OK_M                               0x00002000
#define FLASH_STAT_SPRS_BYTE_NOT_OK_S                                       13

// Field:  [12:8] EFUSE_ERRCODE
//
// Same as EFUSEERROR.CODE
#define FLASH_STAT_EFUSE_ERRCODE_W                                           5
#define FLASH_STAT_EFUSE_ERRCODE_M                                  0x00001F00
#define FLASH_STAT_EFUSE_ERRCODE_S                                           8

// Field:     [2] SAMHOLD_DIS
//
// Status indicator of flash sample and hold sequencing logic. This bit will go
// to 1 some delay after CFG.DIS_IDLE is set to 1.
// 0: Not disabled
// 1: Sample and hold disabled and stable
#define FLASH_STAT_SAMHOLD_DIS                                      0x00000004
#define FLASH_STAT_SAMHOLD_DIS_BITN                                          2
#define FLASH_STAT_SAMHOLD_DIS_M                                    0x00000004
#define FLASH_STAT_SAMHOLD_DIS_S                                             2

// Field:     [1] BUSY
//
// Fast version of the FMC FMSTAT.BUSY bit.
// This flag is valid immediately after the operation setting it (FMSTAT.BUSY
// is delayed some cycles)
// 0 : Not busy
// 1 : Busy
#define FLASH_STAT_BUSY                                             0x00000002
#define FLASH_STAT_BUSY_BITN                                                 1
#define FLASH_STAT_BUSY_M                                           0x00000002
#define FLASH_STAT_BUSY_S                                                    1

// Field:     [0] POWER_MODE
//
// Power state of the flash sub-system.
// 0 : Active
// 1 : Low power
#define FLASH_STAT_POWER_MODE                                       0x00000001
#define FLASH_STAT_POWER_MODE_BITN                                           0
#define FLASH_STAT_POWER_MODE_M                                     0x00000001
#define FLASH_STAT_POWER_MODE_S                                              0

//*****************************************************************************
//
// Register: FLASH_O_CFG
//
//*****************************************************************************
// Field:     [8] STANDBY_MODE_SEL
//
// Internal. Only to be used through TI provided API.
#define FLASH_CFG_STANDBY_MODE_SEL                                  0x00000100
#define FLASH_CFG_STANDBY_MODE_SEL_BITN                                      8
#define FLASH_CFG_STANDBY_MODE_SEL_M                                0x00000100
#define FLASH_CFG_STANDBY_MODE_SEL_S                                         8

// Field:   [7:6] STANDBY_PW_SEL
//
// Internal. Only to be used through TI provided API.
#define FLASH_CFG_STANDBY_PW_SEL_W                                           2
#define FLASH_CFG_STANDBY_PW_SEL_M                                  0x000000C0
#define FLASH_CFG_STANDBY_PW_SEL_S                                           6

// Field:     [5] DIS_EFUSECLK
//
// Internal. Only to be used through TI provided API.
#define FLASH_CFG_DIS_EFUSECLK                                      0x00000020
#define FLASH_CFG_DIS_EFUSECLK_BITN                                          5
#define FLASH_CFG_DIS_EFUSECLK_M                                    0x00000020
#define FLASH_CFG_DIS_EFUSECLK_S                                             5

// Field:     [4] DIS_READACCESS
//
// Internal. Only to be used through TI provided API.
#define FLASH_CFG_DIS_READACCESS                                    0x00000010
#define FLASH_CFG_DIS_READACCESS_BITN                                        4
#define FLASH_CFG_DIS_READACCESS_M                                  0x00000010
#define FLASH_CFG_DIS_READACCESS_S                                           4

// Field:     [3] ENABLE_SWINTF
//
// Internal. Only to be used through TI provided API.
#define FLASH_CFG_ENABLE_SWINTF                                     0x00000008
#define FLASH_CFG_ENABLE_SWINTF_BITN                                         3
#define FLASH_CFG_ENABLE_SWINTF_M                                   0x00000008
#define FLASH_CFG_ENABLE_SWINTF_S                                            3

// Field:     [1] DIS_STANDBY
//
// Internal. Only to be used through TI provided API.
#define FLASH_CFG_DIS_STANDBY                                       0x00000002
#define FLASH_CFG_DIS_STANDBY_BITN                                           1
#define FLASH_CFG_DIS_STANDBY_M                                     0x00000002
#define FLASH_CFG_DIS_STANDBY_S                                              1

// Field:     [0] DIS_IDLE
//
// Internal. Only to be used through TI provided API.
#define FLASH_CFG_DIS_IDLE                                          0x00000001
#define FLASH_CFG_DIS_IDLE_BITN                                              0
#define FLASH_CFG_DIS_IDLE_M                                        0x00000001
#define FLASH_CFG_DIS_IDLE_S                                                 0

//*****************************************************************************
//
// Register: FLASH_O_SYSCODE_START
//
//*****************************************************************************
// Field:   [5:0] SYSCODE_START
//
// Internal. Only to be used through TI provided API.
#define FLASH_SYSCODE_START_SYSCODE_START_W                                  6
#define FLASH_SYSCODE_START_SYSCODE_START_M                         0x0000003F
#define FLASH_SYSCODE_START_SYSCODE_START_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FLASH_SIZE
//
//*****************************************************************************
// Field:   [7:0] SECTORS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FLASH_SIZE_SECTORS_W                                           8
#define FLASH_FLASH_SIZE_SECTORS_M                                  0x000000FF
#define FLASH_FLASH_SIZE_SECTORS_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FWLOCK
//
//*****************************************************************************
// Field:   [2:0] FWLOCK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWLOCK_FWLOCK_W                                                3
#define FLASH_FWLOCK_FWLOCK_M                                       0x00000007
#define FLASH_FWLOCK_FWLOCK_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_FWFLAG
//
//*****************************************************************************
// Field:   [2:0] FWFLAG
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWFLAG_FWFLAG_W                                                3
#define FLASH_FWFLAG_FWFLAG_M                                       0x00000007
#define FLASH_FWFLAG_FWFLAG_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_EFUSE
//
//*****************************************************************************
// Field: [28:24] INSTRUCTION
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSE_INSTRUCTION_W                                            5
#define FLASH_EFUSE_INSTRUCTION_M                                   0x1F000000
#define FLASH_EFUSE_INSTRUCTION_S                                           24

// Field:  [15:0] DUMPWORD
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSE_DUMPWORD_W                                              16
#define FLASH_EFUSE_DUMPWORD_M                                      0x0000FFFF
#define FLASH_EFUSE_DUMPWORD_S                                               0

//*****************************************************************************
//
// Register: FLASH_O_EFUSEADDR
//
//*****************************************************************************
// Field: [15:11] BLOCK
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEADDR_BLOCK_W                                              5
#define FLASH_EFUSEADDR_BLOCK_M                                     0x0000F800
#define FLASH_EFUSEADDR_BLOCK_S                                             11

// Field:  [10:0] ROW
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEADDR_ROW_W                                               11
#define FLASH_EFUSEADDR_ROW_M                                       0x000007FF
#define FLASH_EFUSEADDR_ROW_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_DATAUPPER
//
//*****************************************************************************
// Field:   [7:3] SPARE
//
// Internal. Only to be used through TI provided API.
#define FLASH_DATAUPPER_SPARE_W                                              5
#define FLASH_DATAUPPER_SPARE_M                                     0x000000F8
#define FLASH_DATAUPPER_SPARE_S                                              3

// Field:     [2] P
//
// Internal. Only to be used through TI provided API.
#define FLASH_DATAUPPER_P                                           0x00000004
#define FLASH_DATAUPPER_P_BITN                                               2
#define FLASH_DATAUPPER_P_M                                         0x00000004
#define FLASH_DATAUPPER_P_S                                                  2

// Field:     [1] R
//
// Internal. Only to be used through TI provided API.
#define FLASH_DATAUPPER_R                                           0x00000002
#define FLASH_DATAUPPER_R_BITN                                               1
#define FLASH_DATAUPPER_R_M                                         0x00000002
#define FLASH_DATAUPPER_R_S                                                  1

// Field:     [0] EEN
//
// Internal. Only to be used through TI provided API.
#define FLASH_DATAUPPER_EEN                                         0x00000001
#define FLASH_DATAUPPER_EEN_BITN                                             0
#define FLASH_DATAUPPER_EEN_M                                       0x00000001
#define FLASH_DATAUPPER_EEN_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_DATALOWER
//
//*****************************************************************************
// Field:  [31:0] DATA
//
// Internal. Only to be used through TI provided API.
#define FLASH_DATALOWER_DATA_W                                              32
#define FLASH_DATALOWER_DATA_M                                      0xFFFFFFFF
#define FLASH_DATALOWER_DATA_S                                               0

//*****************************************************************************
//
// Register: FLASH_O_EFUSECFG
//
//*****************************************************************************
// Field:     [8] IDLEGATING
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSECFG_IDLEGATING                                   0x00000100
#define FLASH_EFUSECFG_IDLEGATING_BITN                                       8
#define FLASH_EFUSECFG_IDLEGATING_M                                 0x00000100
#define FLASH_EFUSECFG_IDLEGATING_S                                          8

// Field:   [4:3] SLAVEPOWER
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSECFG_SLAVEPOWER_W                                          2
#define FLASH_EFUSECFG_SLAVEPOWER_M                                 0x00000018
#define FLASH_EFUSECFG_SLAVEPOWER_S                                          3

// Field:     [0] GATING
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSECFG_GATING                                       0x00000001
#define FLASH_EFUSECFG_GATING_BITN                                           0
#define FLASH_EFUSECFG_GATING_M                                     0x00000001
#define FLASH_EFUSECFG_GATING_S                                              0

//*****************************************************************************
//
// Register: FLASH_O_EFUSESTAT
//
//*****************************************************************************
// Field:     [0] RESETDONE
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSESTAT_RESETDONE                                   0x00000001
#define FLASH_EFUSESTAT_RESETDONE_BITN                                       0
#define FLASH_EFUSESTAT_RESETDONE_M                                 0x00000001
#define FLASH_EFUSESTAT_RESETDONE_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_ACC
//
//*****************************************************************************
// Field:  [23:0] ACCUMULATOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_ACC_ACCUMULATOR_W                                             24
#define FLASH_ACC_ACCUMULATOR_M                                     0x00FFFFFF
#define FLASH_ACC_ACCUMULATOR_S                                              0

//*****************************************************************************
//
// Register: FLASH_O_BOUNDARY
//
//*****************************************************************************
// Field:    [23] DISROW0
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_DISROW0                                      0x00800000
#define FLASH_BOUNDARY_DISROW0_BITN                                         23
#define FLASH_BOUNDARY_DISROW0_M                                    0x00800000
#define FLASH_BOUNDARY_DISROW0_S                                            23

// Field:    [22] SPARE
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_SPARE                                        0x00400000
#define FLASH_BOUNDARY_SPARE_BITN                                           22
#define FLASH_BOUNDARY_SPARE_M                                      0x00400000
#define FLASH_BOUNDARY_SPARE_S                                              22

// Field:    [21] EFC_SELF_TEST_ERROR
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_EFC_SELF_TEST_ERROR                          0x00200000
#define FLASH_BOUNDARY_EFC_SELF_TEST_ERROR_BITN                             21
#define FLASH_BOUNDARY_EFC_SELF_TEST_ERROR_M                        0x00200000
#define FLASH_BOUNDARY_EFC_SELF_TEST_ERROR_S                                21

// Field:    [20] EFC_INSTRUCTION_INFO
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_EFC_INSTRUCTION_INFO                         0x00100000
#define FLASH_BOUNDARY_EFC_INSTRUCTION_INFO_BITN                            20
#define FLASH_BOUNDARY_EFC_INSTRUCTION_INFO_M                       0x00100000
#define FLASH_BOUNDARY_EFC_INSTRUCTION_INFO_S                               20

// Field:    [19] EFC_INSTRUCTION_ERROR
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_EFC_INSTRUCTION_ERROR                        0x00080000
#define FLASH_BOUNDARY_EFC_INSTRUCTION_ERROR_BITN                           19
#define FLASH_BOUNDARY_EFC_INSTRUCTION_ERROR_M                      0x00080000
#define FLASH_BOUNDARY_EFC_INSTRUCTION_ERROR_S                              19

// Field:    [18] EFC_AUTOLOAD_ERROR
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_EFC_AUTOLOAD_ERROR                           0x00040000
#define FLASH_BOUNDARY_EFC_AUTOLOAD_ERROR_BITN                              18
#define FLASH_BOUNDARY_EFC_AUTOLOAD_ERROR_M                         0x00040000
#define FLASH_BOUNDARY_EFC_AUTOLOAD_ERROR_S                                 18

// Field: [17:14] OUTPUTENABLE
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_OUTPUTENABLE_W                                        4
#define FLASH_BOUNDARY_OUTPUTENABLE_M                               0x0003C000
#define FLASH_BOUNDARY_OUTPUTENABLE_S                                       14

// Field:    [13] SYS_ECC_SELF_TEST_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_SYS_ECC_SELF_TEST_EN                         0x00002000
#define FLASH_BOUNDARY_SYS_ECC_SELF_TEST_EN_BITN                            13
#define FLASH_BOUNDARY_SYS_ECC_SELF_TEST_EN_M                       0x00002000
#define FLASH_BOUNDARY_SYS_ECC_SELF_TEST_EN_S                               13

// Field:    [12] SYS_ECC_OVERRIDE_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_SYS_ECC_OVERRIDE_EN                          0x00001000
#define FLASH_BOUNDARY_SYS_ECC_OVERRIDE_EN_BITN                             12
#define FLASH_BOUNDARY_SYS_ECC_OVERRIDE_EN_M                        0x00001000
#define FLASH_BOUNDARY_SYS_ECC_OVERRIDE_EN_S                                12

// Field:    [11] EFC_FDI
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_EFC_FDI                                      0x00000800
#define FLASH_BOUNDARY_EFC_FDI_BITN                                         11
#define FLASH_BOUNDARY_EFC_FDI_M                                    0x00000800
#define FLASH_BOUNDARY_EFC_FDI_S                                            11

// Field:    [10] SYS_DIEID_AUTOLOAD_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_SYS_DIEID_AUTOLOAD_EN                        0x00000400
#define FLASH_BOUNDARY_SYS_DIEID_AUTOLOAD_EN_BITN                           10
#define FLASH_BOUNDARY_SYS_DIEID_AUTOLOAD_EN_M                      0x00000400
#define FLASH_BOUNDARY_SYS_DIEID_AUTOLOAD_EN_S                              10

// Field:   [9:8] SYS_REPAIR_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_SYS_REPAIR_EN_W                                       2
#define FLASH_BOUNDARY_SYS_REPAIR_EN_M                              0x00000300
#define FLASH_BOUNDARY_SYS_REPAIR_EN_S                                       8

// Field:   [7:4] SYS_WS_READ_STATES
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_SYS_WS_READ_STATES_W                                  4
#define FLASH_BOUNDARY_SYS_WS_READ_STATES_M                         0x000000F0
#define FLASH_BOUNDARY_SYS_WS_READ_STATES_S                                  4

// Field:   [3:0] INPUTENABLE
//
// Internal. Only to be used through TI provided API.
#define FLASH_BOUNDARY_INPUTENABLE_W                                         4
#define FLASH_BOUNDARY_INPUTENABLE_M                                0x0000000F
#define FLASH_BOUNDARY_INPUTENABLE_S                                         0

//*****************************************************************************
//
// Register: FLASH_O_EFUSEFLAG
//
//*****************************************************************************
// Field:     [0] KEY
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEFLAG_KEY                                         0x00000001
#define FLASH_EFUSEFLAG_KEY_BITN                                             0
#define FLASH_EFUSEFLAG_KEY_M                                       0x00000001
#define FLASH_EFUSEFLAG_KEY_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_EFUSEKEY
//
//*****************************************************************************
// Field:  [31:0] CODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEKEY_CODE_W                                               32
#define FLASH_EFUSEKEY_CODE_M                                       0xFFFFFFFF
#define FLASH_EFUSEKEY_CODE_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_EFUSERELEASE
//
//*****************************************************************************
// Field: [31:25] ODPYEAR
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSERELEASE_ODPYEAR_W                                         7
#define FLASH_EFUSERELEASE_ODPYEAR_M                                0xFE000000
#define FLASH_EFUSERELEASE_ODPYEAR_S                                        25

// Field: [24:21] ODPMONTH
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSERELEASE_ODPMONTH_W                                        4
#define FLASH_EFUSERELEASE_ODPMONTH_M                               0x01E00000
#define FLASH_EFUSERELEASE_ODPMONTH_S                                       21

// Field: [20:16] ODPDAY
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSERELEASE_ODPDAY_W                                          5
#define FLASH_EFUSERELEASE_ODPDAY_M                                 0x001F0000
#define FLASH_EFUSERELEASE_ODPDAY_S                                         16

// Field:  [15:9] EFUSEYEAR
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSERELEASE_EFUSEYEAR_W                                       7
#define FLASH_EFUSERELEASE_EFUSEYEAR_M                              0x0000FE00
#define FLASH_EFUSERELEASE_EFUSEYEAR_S                                       9

// Field:   [8:5] EFUSEMONTH
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSERELEASE_EFUSEMONTH_W                                      4
#define FLASH_EFUSERELEASE_EFUSEMONTH_M                             0x000001E0
#define FLASH_EFUSERELEASE_EFUSEMONTH_S                                      5

// Field:   [4:0] EFUSEDAY
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSERELEASE_EFUSEDAY_W                                        5
#define FLASH_EFUSERELEASE_EFUSEDAY_M                               0x0000001F
#define FLASH_EFUSERELEASE_EFUSEDAY_S                                        0

//*****************************************************************************
//
// Register: FLASH_O_EFUSEPINS
//
//*****************************************************************************
// Field:    [15] EFC_SELF_TEST_DONE
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_EFC_SELF_TEST_DONE                          0x00008000
#define FLASH_EFUSEPINS_EFC_SELF_TEST_DONE_BITN                             15
#define FLASH_EFUSEPINS_EFC_SELF_TEST_DONE_M                        0x00008000
#define FLASH_EFUSEPINS_EFC_SELF_TEST_DONE_S                                15

// Field:    [14] EFC_SELF_TEST_ERROR
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_EFC_SELF_TEST_ERROR                         0x00004000
#define FLASH_EFUSEPINS_EFC_SELF_TEST_ERROR_BITN                            14
#define FLASH_EFUSEPINS_EFC_SELF_TEST_ERROR_M                       0x00004000
#define FLASH_EFUSEPINS_EFC_SELF_TEST_ERROR_S                               14

// Field:    [13] SYS_ECC_SELF_TEST_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_SYS_ECC_SELF_TEST_EN                        0x00002000
#define FLASH_EFUSEPINS_SYS_ECC_SELF_TEST_EN_BITN                           13
#define FLASH_EFUSEPINS_SYS_ECC_SELF_TEST_EN_M                      0x00002000
#define FLASH_EFUSEPINS_SYS_ECC_SELF_TEST_EN_S                              13

// Field:    [12] EFC_INSTRUCTION_INFO
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_EFC_INSTRUCTION_INFO                        0x00001000
#define FLASH_EFUSEPINS_EFC_INSTRUCTION_INFO_BITN                           12
#define FLASH_EFUSEPINS_EFC_INSTRUCTION_INFO_M                      0x00001000
#define FLASH_EFUSEPINS_EFC_INSTRUCTION_INFO_S                              12

// Field:    [11] EFC_INSTRUCTION_ERROR
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_EFC_INSTRUCTION_ERROR                       0x00000800
#define FLASH_EFUSEPINS_EFC_INSTRUCTION_ERROR_BITN                          11
#define FLASH_EFUSEPINS_EFC_INSTRUCTION_ERROR_M                     0x00000800
#define FLASH_EFUSEPINS_EFC_INSTRUCTION_ERROR_S                             11

// Field:    [10] EFC_AUTOLOAD_ERROR
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_EFC_AUTOLOAD_ERROR                          0x00000400
#define FLASH_EFUSEPINS_EFC_AUTOLOAD_ERROR_BITN                             10
#define FLASH_EFUSEPINS_EFC_AUTOLOAD_ERROR_M                        0x00000400
#define FLASH_EFUSEPINS_EFC_AUTOLOAD_ERROR_S                                10

// Field:     [9] SYS_ECC_OVERRIDE_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_SYS_ECC_OVERRIDE_EN                         0x00000200
#define FLASH_EFUSEPINS_SYS_ECC_OVERRIDE_EN_BITN                             9
#define FLASH_EFUSEPINS_SYS_ECC_OVERRIDE_EN_M                       0x00000200
#define FLASH_EFUSEPINS_SYS_ECC_OVERRIDE_EN_S                                9

// Field:     [8] EFC_READY
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_EFC_READY                                   0x00000100
#define FLASH_EFUSEPINS_EFC_READY_BITN                                       8
#define FLASH_EFUSEPINS_EFC_READY_M                                 0x00000100
#define FLASH_EFUSEPINS_EFC_READY_S                                          8

// Field:     [7] EFC_FCLRZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_EFC_FCLRZ                                   0x00000080
#define FLASH_EFUSEPINS_EFC_FCLRZ_BITN                                       7
#define FLASH_EFUSEPINS_EFC_FCLRZ_M                                 0x00000080
#define FLASH_EFUSEPINS_EFC_FCLRZ_S                                          7

// Field:     [6] SYS_DIEID_AUTOLOAD_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_SYS_DIEID_AUTOLOAD_EN                       0x00000040
#define FLASH_EFUSEPINS_SYS_DIEID_AUTOLOAD_EN_BITN                           6
#define FLASH_EFUSEPINS_SYS_DIEID_AUTOLOAD_EN_M                     0x00000040
#define FLASH_EFUSEPINS_SYS_DIEID_AUTOLOAD_EN_S                              6

// Field:   [5:4] SYS_REPAIR_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_SYS_REPAIR_EN_W                                      2
#define FLASH_EFUSEPINS_SYS_REPAIR_EN_M                             0x00000030
#define FLASH_EFUSEPINS_SYS_REPAIR_EN_S                                      4

// Field:   [3:0] SYS_WS_READ_STATES
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPINS_SYS_WS_READ_STATES_W                                 4
#define FLASH_EFUSEPINS_SYS_WS_READ_STATES_M                        0x0000000F
#define FLASH_EFUSEPINS_SYS_WS_READ_STATES_S                                 0

//*****************************************************************************
//
// Register: FLASH_O_EFUSECRA
//
//*****************************************************************************
// Field:   [5:0] DATA
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSECRA_DATA_W                                                6
#define FLASH_EFUSECRA_DATA_M                                       0x0000003F
#define FLASH_EFUSECRA_DATA_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_EFUSEREAD
//
//*****************************************************************************
// Field:   [9:8] DATABIT
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEREAD_DATABIT_W                                            2
#define FLASH_EFUSEREAD_DATABIT_M                                   0x00000300
#define FLASH_EFUSEREAD_DATABIT_S                                            8

// Field:   [7:4] READCLOCK
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEREAD_READCLOCK_W                                          4
#define FLASH_EFUSEREAD_READCLOCK_M                                 0x000000F0
#define FLASH_EFUSEREAD_READCLOCK_S                                          4

// Field:     [3] DEBUG
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEREAD_DEBUG                                       0x00000008
#define FLASH_EFUSEREAD_DEBUG_BITN                                           3
#define FLASH_EFUSEREAD_DEBUG_M                                     0x00000008
#define FLASH_EFUSEREAD_DEBUG_S                                              3

// Field:     [2] SPARE
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEREAD_SPARE                                       0x00000004
#define FLASH_EFUSEREAD_SPARE_BITN                                           2
#define FLASH_EFUSEREAD_SPARE_M                                     0x00000004
#define FLASH_EFUSEREAD_SPARE_S                                              2

// Field:   [1:0] MARGIN
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEREAD_MARGIN_W                                             2
#define FLASH_EFUSEREAD_MARGIN_M                                    0x00000003
#define FLASH_EFUSEREAD_MARGIN_S                                             0

//*****************************************************************************
//
// Register: FLASH_O_EFUSEPROGRAM
//
//*****************************************************************************
// Field:    [30] COMPAREDISABLE
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPROGRAM_COMPAREDISABLE                           0x40000000
#define FLASH_EFUSEPROGRAM_COMPAREDISABLE_BITN                              30
#define FLASH_EFUSEPROGRAM_COMPAREDISABLE_M                         0x40000000
#define FLASH_EFUSEPROGRAM_COMPAREDISABLE_S                                 30

// Field: [29:14] CLOCKSTALL
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPROGRAM_CLOCKSTALL_W                                     16
#define FLASH_EFUSEPROGRAM_CLOCKSTALL_M                             0x3FFFC000
#define FLASH_EFUSEPROGRAM_CLOCKSTALL_S                                     14

// Field:    [13] VPPTOVDD
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPROGRAM_VPPTOVDD                                 0x00002000
#define FLASH_EFUSEPROGRAM_VPPTOVDD_BITN                                    13
#define FLASH_EFUSEPROGRAM_VPPTOVDD_M                               0x00002000
#define FLASH_EFUSEPROGRAM_VPPTOVDD_S                                       13

// Field:  [12:9] ITERATIONS
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPROGRAM_ITERATIONS_W                                      4
#define FLASH_EFUSEPROGRAM_ITERATIONS_M                             0x00001E00
#define FLASH_EFUSEPROGRAM_ITERATIONS_S                                      9

// Field:   [8:0] WRITECLOCK
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEPROGRAM_WRITECLOCK_W                                      9
#define FLASH_EFUSEPROGRAM_WRITECLOCK_M                             0x000001FF
#define FLASH_EFUSEPROGRAM_WRITECLOCK_S                                      0

//*****************************************************************************
//
// Register: FLASH_O_EFUSEERROR
//
//*****************************************************************************
// Field:     [5] DONE
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEERROR_DONE                                       0x00000020
#define FLASH_EFUSEERROR_DONE_BITN                                           5
#define FLASH_EFUSEERROR_DONE_M                                     0x00000020
#define FLASH_EFUSEERROR_DONE_S                                              5

// Field:   [4:0] CODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_EFUSEERROR_CODE_W                                              5
#define FLASH_EFUSEERROR_CODE_M                                     0x0000001F
#define FLASH_EFUSEERROR_CODE_S                                              0

//*****************************************************************************
//
// Register: FLASH_O_SINGLEBIT
//
//*****************************************************************************
// Field:  [31:1] FROMN
//
// Internal. Only to be used through TI provided API.
#define FLASH_SINGLEBIT_FROMN_W                                             31
#define FLASH_SINGLEBIT_FROMN_M                                     0xFFFFFFFE
#define FLASH_SINGLEBIT_FROMN_S                                              1

// Field:     [0] FROM0
//
// Internal. Only to be used through TI provided API.
#define FLASH_SINGLEBIT_FROM0                                       0x00000001
#define FLASH_SINGLEBIT_FROM0_BITN                                           0
#define FLASH_SINGLEBIT_FROM0_M                                     0x00000001
#define FLASH_SINGLEBIT_FROM0_S                                              0

//*****************************************************************************
//
// Register: FLASH_O_TWOBIT
//
//*****************************************************************************
// Field:  [31:1] FROMN
//
// Internal. Only to be used through TI provided API.
#define FLASH_TWOBIT_FROMN_W                                                31
#define FLASH_TWOBIT_FROMN_M                                        0xFFFFFFFE
#define FLASH_TWOBIT_FROMN_S                                                 1

// Field:     [0] FROM0
//
// Internal. Only to be used through TI provided API.
#define FLASH_TWOBIT_FROM0                                          0x00000001
#define FLASH_TWOBIT_FROM0_BITN                                              0
#define FLASH_TWOBIT_FROM0_M                                        0x00000001
#define FLASH_TWOBIT_FROM0_S                                                 0

//*****************************************************************************
//
// Register: FLASH_O_SELFTESTCYC
//
//*****************************************************************************
// Field:  [31:0] CYCLES
//
// Internal. Only to be used through TI provided API.
#define FLASH_SELFTESTCYC_CYCLES_W                                          32
#define FLASH_SELFTESTCYC_CYCLES_M                                  0xFFFFFFFF
#define FLASH_SELFTESTCYC_CYCLES_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_SELFTESTSIGN
//
//*****************************************************************************
// Field:  [31:0] SIGNATURE
//
// Internal. Only to be used through TI provided API.
#define FLASH_SELFTESTSIGN_SIGNATURE_W                                      32
#define FLASH_SELFTESTSIGN_SIGNATURE_M                              0xFFFFFFFF
#define FLASH_SELFTESTSIGN_SIGNATURE_S                                       0

//*****************************************************************************
//
// Register: FLASH_O_FRDCTL
//
//*****************************************************************************
// Field:  [11:8] RWAIT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FRDCTL_RWAIT_W                                                 4
#define FLASH_FRDCTL_RWAIT_M                                        0x00000F00
#define FLASH_FRDCTL_RWAIT_S                                                 8

//*****************************************************************************
//
// Register: FLASH_O_FSPRD
//
//*****************************************************************************
// Field:  [15:8] RMBSEM
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSPRD_RMBSEM_W                                                 8
#define FLASH_FSPRD_RMBSEM_M                                        0x0000FF00
#define FLASH_FSPRD_RMBSEM_S                                                 8

// Field:     [1] RM1
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSPRD_RM1                                             0x00000002
#define FLASH_FSPRD_RM1_BITN                                                 1
#define FLASH_FSPRD_RM1_M                                           0x00000002
#define FLASH_FSPRD_RM1_S                                                    1

// Field:     [0] RM0
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSPRD_RM0                                             0x00000001
#define FLASH_FSPRD_RM0_BITN                                                 0
#define FLASH_FSPRD_RM0_M                                           0x00000001
#define FLASH_FSPRD_RM0_S                                                    0

//*****************************************************************************
//
// Register: FLASH_O_FEDACCTL1
//
//*****************************************************************************
// Field:    [24] SUSP_IGNR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEDACCTL1_SUSP_IGNR                                   0x01000000
#define FLASH_FEDACCTL1_SUSP_IGNR_BITN                                      24
#define FLASH_FEDACCTL1_SUSP_IGNR_M                                 0x01000000
#define FLASH_FEDACCTL1_SUSP_IGNR_S                                         24

//*****************************************************************************
//
// Register: FLASH_O_FEDACSTAT
//
//*****************************************************************************
// Field:    [25] RVF_INT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEDACSTAT_RVF_INT                                     0x02000000
#define FLASH_FEDACSTAT_RVF_INT_BITN                                        25
#define FLASH_FEDACSTAT_RVF_INT_M                                   0x02000000
#define FLASH_FEDACSTAT_RVF_INT_S                                           25

// Field:    [24] FSM_DONE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEDACSTAT_FSM_DONE                                    0x01000000
#define FLASH_FEDACSTAT_FSM_DONE_BITN                                       24
#define FLASH_FEDACSTAT_FSM_DONE_M                                  0x01000000
#define FLASH_FEDACSTAT_FSM_DONE_S                                          24

//*****************************************************************************
//
// Register: FLASH_O_FBPROT
//
//*****************************************************************************
// Field:     [0] PROTL1DIS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBPROT_PROTL1DIS                                      0x00000001
#define FLASH_FBPROT_PROTL1DIS_BITN                                          0
#define FLASH_FBPROT_PROTL1DIS_M                                    0x00000001
#define FLASH_FBPROT_PROTL1DIS_S                                             0

//*****************************************************************************
//
// Register: FLASH_O_FBSE
//
//*****************************************************************************
// Field:  [15:0] BSE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSE_BSE_W                                                    16
#define FLASH_FBSE_BSE_M                                            0x0000FFFF
#define FLASH_FBSE_BSE_S                                                     0

//*****************************************************************************
//
// Register: FLASH_O_FBBUSY
//
//*****************************************************************************
// Field:   [7:0] BUSY
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBBUSY_BUSY_W                                                  8
#define FLASH_FBBUSY_BUSY_M                                         0x000000FF
#define FLASH_FBBUSY_BUSY_S                                                  0

//*****************************************************************************
//
// Register: FLASH_O_FBAC
//
//*****************************************************************************
// Field:    [16] OTPPROTDIS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBAC_OTPPROTDIS                                       0x00010000
#define FLASH_FBAC_OTPPROTDIS_BITN                                          16
#define FLASH_FBAC_OTPPROTDIS_M                                     0x00010000
#define FLASH_FBAC_OTPPROTDIS_S                                             16

// Field:  [15:8] BAGP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBAC_BAGP_W                                                    8
#define FLASH_FBAC_BAGP_M                                           0x0000FF00
#define FLASH_FBAC_BAGP_S                                                    8

// Field:   [7:0] VREADS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBAC_VREADS_W                                                  8
#define FLASH_FBAC_VREADS_M                                         0x000000FF
#define FLASH_FBAC_VREADS_S                                                  0

//*****************************************************************************
//
// Register: FLASH_O_FBFALLBACK
//
//*****************************************************************************
// Field: [27:24] FSM_PWRSAV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_FSM_PWRSAV_W                                        4
#define FLASH_FBFALLBACK_FSM_PWRSAV_M                               0x0F000000
#define FLASH_FBFALLBACK_FSM_PWRSAV_S                                       24

// Field: [19:16] REG_PWRSAV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_REG_PWRSAV_W                                        4
#define FLASH_FBFALLBACK_REG_PWRSAV_M                               0x000F0000
#define FLASH_FBFALLBACK_REG_PWRSAV_S                                       16

// Field: [15:14] BANKPWR7
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_BANKPWR7_W                                          2
#define FLASH_FBFALLBACK_BANKPWR7_M                                 0x0000C000
#define FLASH_FBFALLBACK_BANKPWR7_S                                         14

// Field: [13:12] BANKPWR6
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_BANKPWR6_W                                          2
#define FLASH_FBFALLBACK_BANKPWR6_M                                 0x00003000
#define FLASH_FBFALLBACK_BANKPWR6_S                                         12

// Field: [11:10] BANKPWR5
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_BANKPWR5_W                                          2
#define FLASH_FBFALLBACK_BANKPWR5_M                                 0x00000C00
#define FLASH_FBFALLBACK_BANKPWR5_S                                         10

// Field:   [9:8] BANKPWR4
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_BANKPWR4_W                                          2
#define FLASH_FBFALLBACK_BANKPWR4_M                                 0x00000300
#define FLASH_FBFALLBACK_BANKPWR4_S                                          8

// Field:   [7:6] BANKPWR3
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_BANKPWR3_W                                          2
#define FLASH_FBFALLBACK_BANKPWR3_M                                 0x000000C0
#define FLASH_FBFALLBACK_BANKPWR3_S                                          6

// Field:   [5:4] BANKPWR2
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_BANKPWR2_W                                          2
#define FLASH_FBFALLBACK_BANKPWR2_M                                 0x00000030
#define FLASH_FBFALLBACK_BANKPWR2_S                                          4

// Field:   [3:2] BANKPWR1
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_BANKPWR1_W                                          2
#define FLASH_FBFALLBACK_BANKPWR1_M                                 0x0000000C
#define FLASH_FBFALLBACK_BANKPWR1_S                                          2

// Field:   [1:0] BANKPWR0
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBFALLBACK_BANKPWR0_W                                          2
#define FLASH_FBFALLBACK_BANKPWR0_M                                 0x00000003
#define FLASH_FBFALLBACK_BANKPWR0_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FBPRDY
//
//*****************************************************************************
// Field:    [16] BANKBUSY
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBPRDY_BANKBUSY                                       0x00010000
#define FLASH_FBPRDY_BANKBUSY_BITN                                          16
#define FLASH_FBPRDY_BANKBUSY_M                                     0x00010000
#define FLASH_FBPRDY_BANKBUSY_S                                             16

// Field:    [15] PUMPRDY
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBPRDY_PUMPRDY                                        0x00008000
#define FLASH_FBPRDY_PUMPRDY_BITN                                           15
#define FLASH_FBPRDY_PUMPRDY_M                                      0x00008000
#define FLASH_FBPRDY_PUMPRDY_S                                              15

// Field:     [0] BANKRDY
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBPRDY_BANKRDY                                        0x00000001
#define FLASH_FBPRDY_BANKRDY_BITN                                            0
#define FLASH_FBPRDY_BANKRDY_M                                      0x00000001
#define FLASH_FBPRDY_BANKRDY_S                                               0

//*****************************************************************************
//
// Register: FLASH_O_FPAC1
//
//*****************************************************************************
// Field: [27:16] PSLEEPTDIS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FPAC1_PSLEEPTDIS_W                                            12
#define FLASH_FPAC1_PSLEEPTDIS_M                                    0x0FFF0000
#define FLASH_FPAC1_PSLEEPTDIS_S                                            16

// Field:  [15:4] PUMPRESET_PW
//
// Internal. Only to be used through TI provided API.
#define FLASH_FPAC1_PUMPRESET_PW_W                                          12
#define FLASH_FPAC1_PUMPRESET_PW_M                                  0x0000FFF0
#define FLASH_FPAC1_PUMPRESET_PW_S                                           4

// Field:   [1:0] PUMPPWR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FPAC1_PUMPPWR_W                                                2
#define FLASH_FPAC1_PUMPPWR_M                                       0x00000003
#define FLASH_FPAC1_PUMPPWR_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_FPAC2
//
//*****************************************************************************
// Field:  [15:0] PAGP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FPAC2_PAGP_W                                                  16
#define FLASH_FPAC2_PAGP_M                                          0x0000FFFF
#define FLASH_FPAC2_PAGP_S                                                   0

//*****************************************************************************
//
// Register: FLASH_O_FMAC
//
//*****************************************************************************
// Field:   [2:0] BANK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMAC_BANK_W                                                    3
#define FLASH_FMAC_BANK_M                                           0x00000007
#define FLASH_FMAC_BANK_S                                                    0

//*****************************************************************************
//
// Register: FLASH_O_FMSTAT
//
//*****************************************************************************
// Field:    [17] RVSUSP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_RVSUSP                                         0x00020000
#define FLASH_FMSTAT_RVSUSP_BITN                                            17
#define FLASH_FMSTAT_RVSUSP_M                                       0x00020000
#define FLASH_FMSTAT_RVSUSP_S                                               17

// Field:    [16] RDVER
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_RDVER                                          0x00010000
#define FLASH_FMSTAT_RDVER_BITN                                             16
#define FLASH_FMSTAT_RDVER_M                                        0x00010000
#define FLASH_FMSTAT_RDVER_S                                                16

// Field:    [15] RVF
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_RVF                                            0x00008000
#define FLASH_FMSTAT_RVF_BITN                                               15
#define FLASH_FMSTAT_RVF_M                                          0x00008000
#define FLASH_FMSTAT_RVF_S                                                  15

// Field:    [14] ILA
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_ILA                                            0x00004000
#define FLASH_FMSTAT_ILA_BITN                                               14
#define FLASH_FMSTAT_ILA_M                                          0x00004000
#define FLASH_FMSTAT_ILA_S                                                  14

// Field:    [13] DBF
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_DBF                                            0x00002000
#define FLASH_FMSTAT_DBF_BITN                                               13
#define FLASH_FMSTAT_DBF_M                                          0x00002000
#define FLASH_FMSTAT_DBF_S                                                  13

// Field:    [12] PGV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_PGV                                            0x00001000
#define FLASH_FMSTAT_PGV_BITN                                               12
#define FLASH_FMSTAT_PGV_M                                          0x00001000
#define FLASH_FMSTAT_PGV_S                                                  12

// Field:    [11] PCV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_PCV                                            0x00000800
#define FLASH_FMSTAT_PCV_BITN                                               11
#define FLASH_FMSTAT_PCV_M                                          0x00000800
#define FLASH_FMSTAT_PCV_S                                                  11

// Field:    [10] EV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_EV                                             0x00000400
#define FLASH_FMSTAT_EV_BITN                                                10
#define FLASH_FMSTAT_EV_M                                           0x00000400
#define FLASH_FMSTAT_EV_S                                                   10

// Field:     [9] CV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_CV                                             0x00000200
#define FLASH_FMSTAT_CV_BITN                                                 9
#define FLASH_FMSTAT_CV_M                                           0x00000200
#define FLASH_FMSTAT_CV_S                                                    9

// Field:     [8] BUSY
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_BUSY                                           0x00000100
#define FLASH_FMSTAT_BUSY_BITN                                               8
#define FLASH_FMSTAT_BUSY_M                                         0x00000100
#define FLASH_FMSTAT_BUSY_S                                                  8

// Field:     [7] ERS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_ERS                                            0x00000080
#define FLASH_FMSTAT_ERS_BITN                                                7
#define FLASH_FMSTAT_ERS_M                                          0x00000080
#define FLASH_FMSTAT_ERS_S                                                   7

// Field:     [6] PGM
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_PGM                                            0x00000040
#define FLASH_FMSTAT_PGM_BITN                                                6
#define FLASH_FMSTAT_PGM_M                                          0x00000040
#define FLASH_FMSTAT_PGM_S                                                   6

// Field:     [5] INVDAT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_INVDAT                                         0x00000020
#define FLASH_FMSTAT_INVDAT_BITN                                             5
#define FLASH_FMSTAT_INVDAT_M                                       0x00000020
#define FLASH_FMSTAT_INVDAT_S                                                5

// Field:     [4] CSTAT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_CSTAT                                          0x00000010
#define FLASH_FMSTAT_CSTAT_BITN                                              4
#define FLASH_FMSTAT_CSTAT_M                                        0x00000010
#define FLASH_FMSTAT_CSTAT_S                                                 4

// Field:     [3] VOLSTAT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_VOLSTAT                                        0x00000008
#define FLASH_FMSTAT_VOLSTAT_BITN                                            3
#define FLASH_FMSTAT_VOLSTAT_M                                      0x00000008
#define FLASH_FMSTAT_VOLSTAT_S                                               3

// Field:     [2] ESUSP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_ESUSP                                          0x00000004
#define FLASH_FMSTAT_ESUSP_BITN                                              2
#define FLASH_FMSTAT_ESUSP_M                                        0x00000004
#define FLASH_FMSTAT_ESUSP_S                                                 2

// Field:     [1] PSUSP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_PSUSP                                          0x00000002
#define FLASH_FMSTAT_PSUSP_BITN                                              1
#define FLASH_FMSTAT_PSUSP_M                                        0x00000002
#define FLASH_FMSTAT_PSUSP_S                                                 1

// Field:     [0] SLOCK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMSTAT_SLOCK                                          0x00000001
#define FLASH_FMSTAT_SLOCK_BITN                                              0
#define FLASH_FMSTAT_SLOCK_M                                        0x00000001
#define FLASH_FMSTAT_SLOCK_S                                                 0

//*****************************************************************************
//
// Register: FLASH_O_FLOCK
//
//*****************************************************************************
// Field:  [15:0] ENCOM
//
// Internal. Only to be used through TI provided API.
#define FLASH_FLOCK_ENCOM_W                                                 16
#define FLASH_FLOCK_ENCOM_M                                         0x0000FFFF
#define FLASH_FLOCK_ENCOM_S                                                  0

//*****************************************************************************
//
// Register: FLASH_O_FVREADCT
//
//*****************************************************************************
// Field:   [3:0] VREADCT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVREADCT_VREADCT_W                                             4
#define FLASH_FVREADCT_VREADCT_M                                    0x0000000F
#define FLASH_FVREADCT_VREADCT_S                                             0

//*****************************************************************************
//
// Register: FLASH_O_FVHVCT1
//
//*****************************************************************************
// Field: [23:20] TRIM13_E
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVHVCT1_TRIM13_E_W                                             4
#define FLASH_FVHVCT1_TRIM13_E_M                                    0x00F00000
#define FLASH_FVHVCT1_TRIM13_E_S                                            20

// Field: [19:16] VHVCT_E
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVHVCT1_VHVCT_E_W                                              4
#define FLASH_FVHVCT1_VHVCT_E_M                                     0x000F0000
#define FLASH_FVHVCT1_VHVCT_E_S                                             16

// Field:   [7:4] TRIM13_PV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVHVCT1_TRIM13_PV_W                                            4
#define FLASH_FVHVCT1_TRIM13_PV_M                                   0x000000F0
#define FLASH_FVHVCT1_TRIM13_PV_S                                            4

// Field:   [3:0] VHVCT_PV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVHVCT1_VHVCT_PV_W                                             4
#define FLASH_FVHVCT1_VHVCT_PV_M                                    0x0000000F
#define FLASH_FVHVCT1_VHVCT_PV_S                                             0

//*****************************************************************************
//
// Register: FLASH_O_FVHVCT2
//
//*****************************************************************************
// Field: [23:20] TRIM13_P
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVHVCT2_TRIM13_P_W                                             4
#define FLASH_FVHVCT2_TRIM13_P_M                                    0x00F00000
#define FLASH_FVHVCT2_TRIM13_P_S                                            20

// Field: [19:16] VHVCT_P
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVHVCT2_VHVCT_P_W                                              4
#define FLASH_FVHVCT2_VHVCT_P_M                                     0x000F0000
#define FLASH_FVHVCT2_VHVCT_P_S                                             16

//*****************************************************************************
//
// Register: FLASH_O_FVHVCT3
//
//*****************************************************************************
// Field: [19:16] WCT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVHVCT3_WCT_W                                                  4
#define FLASH_FVHVCT3_WCT_M                                         0x000F0000
#define FLASH_FVHVCT3_WCT_S                                                 16

// Field:   [3:0] VHVCT_READ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVHVCT3_VHVCT_READ_W                                           4
#define FLASH_FVHVCT3_VHVCT_READ_M                                  0x0000000F
#define FLASH_FVHVCT3_VHVCT_READ_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FVNVCT
//
//*****************************************************************************
// Field:  [12:8] VCG2P5CT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVNVCT_VCG2P5CT_W                                              5
#define FLASH_FVNVCT_VCG2P5CT_M                                     0x00001F00
#define FLASH_FVNVCT_VCG2P5CT_S                                              8

// Field:   [4:0] VIN_CT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVNVCT_VIN_CT_W                                                5
#define FLASH_FVNVCT_VIN_CT_M                                       0x0000001F
#define FLASH_FVNVCT_VIN_CT_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_FVSLP
//
//*****************************************************************************
// Field: [15:12] VSL_P
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVSLP_VSL_P_W                                                  4
#define FLASH_FVSLP_VSL_P_M                                         0x0000F000
#define FLASH_FVSLP_VSL_P_S                                                 12

//*****************************************************************************
//
// Register: FLASH_O_FVWLCT
//
//*****************************************************************************
// Field:   [4:0] VWLCT_P
//
// Internal. Only to be used through TI provided API.
#define FLASH_FVWLCT_VWLCT_P_W                                               5
#define FLASH_FVWLCT_VWLCT_P_M                                      0x0000001F
#define FLASH_FVWLCT_VWLCT_P_S                                               0

//*****************************************************************************
//
// Register: FLASH_O_FEFUSECTL
//
//*****************************************************************************
// Field: [26:24] CHAIN_SEL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEFUSECTL_CHAIN_SEL_W                                          3
#define FLASH_FEFUSECTL_CHAIN_SEL_M                                 0x07000000
#define FLASH_FEFUSECTL_CHAIN_SEL_S                                         24

// Field:    [17] WRITE_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEFUSECTL_WRITE_EN                                    0x00020000
#define FLASH_FEFUSECTL_WRITE_EN_BITN                                       17
#define FLASH_FEFUSECTL_WRITE_EN_M                                  0x00020000
#define FLASH_FEFUSECTL_WRITE_EN_S                                          17

// Field:    [16] BP_SEL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEFUSECTL_BP_SEL                                      0x00010000
#define FLASH_FEFUSECTL_BP_SEL_BITN                                         16
#define FLASH_FEFUSECTL_BP_SEL_M                                    0x00010000
#define FLASH_FEFUSECTL_BP_SEL_S                                            16

// Field:     [8] EF_CLRZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEFUSECTL_EF_CLRZ                                     0x00000100
#define FLASH_FEFUSECTL_EF_CLRZ_BITN                                         8
#define FLASH_FEFUSECTL_EF_CLRZ_M                                   0x00000100
#define FLASH_FEFUSECTL_EF_CLRZ_S                                            8

// Field:     [4] EF_TEST
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEFUSECTL_EF_TEST                                     0x00000010
#define FLASH_FEFUSECTL_EF_TEST_BITN                                         4
#define FLASH_FEFUSECTL_EF_TEST_M                                   0x00000010
#define FLASH_FEFUSECTL_EF_TEST_S                                            4

// Field:   [3:0] EFUSE_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEFUSECTL_EFUSE_EN_W                                           4
#define FLASH_FEFUSECTL_EFUSE_EN_M                                  0x0000000F
#define FLASH_FEFUSECTL_EFUSE_EN_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FEFUSESTAT
//
//*****************************************************************************
// Field:     [0] SHIFT_DONE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEFUSESTAT_SHIFT_DONE                                 0x00000001
#define FLASH_FEFUSESTAT_SHIFT_DONE_BITN                                     0
#define FLASH_FEFUSESTAT_SHIFT_DONE_M                               0x00000001
#define FLASH_FEFUSESTAT_SHIFT_DONE_S                                        0

//*****************************************************************************
//
// Register: FLASH_O_FEFUSEDATA
//
//*****************************************************************************
// Field:  [31:0] FEFUSEDATA
//
// Internal. Only to be used through TI provided API.
#define FLASH_FEFUSEDATA_FEFUSEDATA_W                                       32
#define FLASH_FEFUSEDATA_FEFUSEDATA_M                               0xFFFFFFFF
#define FLASH_FEFUSEDATA_FEFUSEDATA_S                                        0

//*****************************************************************************
//
// Register: FLASH_O_FSEQPMP
//
//*****************************************************************************
// Field: [27:24] TRIM_3P4
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSEQPMP_TRIM_3P4_W                                             4
#define FLASH_FSEQPMP_TRIM_3P4_M                                    0x0F000000
#define FLASH_FSEQPMP_TRIM_3P4_S                                            24

// Field: [21:20] TRIM_1P7
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSEQPMP_TRIM_1P7_W                                             2
#define FLASH_FSEQPMP_TRIM_1P7_M                                    0x00300000
#define FLASH_FSEQPMP_TRIM_1P7_S                                            20

// Field: [19:16] TRIM_0P8
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSEQPMP_TRIM_0P8_W                                             4
#define FLASH_FSEQPMP_TRIM_0P8_M                                    0x000F0000
#define FLASH_FSEQPMP_TRIM_0P8_S                                            16

// Field: [14:12] VIN_AT_X
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSEQPMP_VIN_AT_X_W                                             3
#define FLASH_FSEQPMP_VIN_AT_X_M                                    0x00007000
#define FLASH_FSEQPMP_VIN_AT_X_S                                            12

// Field:     [8] VIN_BY_PASS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSEQPMP_VIN_BY_PASS                                   0x00000100
#define FLASH_FSEQPMP_VIN_BY_PASS_BITN                                       8
#define FLASH_FSEQPMP_VIN_BY_PASS_M                                 0x00000100
#define FLASH_FSEQPMP_VIN_BY_PASS_S                                          8

//*****************************************************************************
//
// Register: FLASH_O_FBSTROBES
//
//*****************************************************************************
// Field:    [24] ECBIT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_ECBIT                                       0x01000000
#define FLASH_FBSTROBES_ECBIT_BITN                                          24
#define FLASH_FBSTROBES_ECBIT_M                                     0x01000000
#define FLASH_FBSTROBES_ECBIT_S                                             24

// Field:    [18] RWAIT2_FLCLK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_RWAIT2_FLCLK                                0x00040000
#define FLASH_FBSTROBES_RWAIT2_FLCLK_BITN                                   18
#define FLASH_FBSTROBES_RWAIT2_FLCLK_M                              0x00040000
#define FLASH_FBSTROBES_RWAIT2_FLCLK_S                                      18

// Field:    [17] RWAIT_FLCLK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_RWAIT_FLCLK                                 0x00020000
#define FLASH_FBSTROBES_RWAIT_FLCLK_BITN                                    17
#define FLASH_FBSTROBES_RWAIT_FLCLK_M                               0x00020000
#define FLASH_FBSTROBES_RWAIT_FLCLK_S                                       17

// Field:    [16] FLCLKEN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_FLCLKEN                                     0x00010000
#define FLASH_FBSTROBES_FLCLKEN_BITN                                        16
#define FLASH_FBSTROBES_FLCLKEN_M                                   0x00010000
#define FLASH_FBSTROBES_FLCLKEN_S                                           16

// Field:     [8] CTRLENZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_CTRLENZ                                     0x00000100
#define FLASH_FBSTROBES_CTRLENZ_BITN                                         8
#define FLASH_FBSTROBES_CTRLENZ_M                                   0x00000100
#define FLASH_FBSTROBES_CTRLENZ_S                                            8

// Field:     [6] NOCOLRED
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_NOCOLRED                                    0x00000040
#define FLASH_FBSTROBES_NOCOLRED_BITN                                        6
#define FLASH_FBSTROBES_NOCOLRED_M                                  0x00000040
#define FLASH_FBSTROBES_NOCOLRED_S                                           6

// Field:     [5] PRECOL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_PRECOL                                      0x00000020
#define FLASH_FBSTROBES_PRECOL_BITN                                          5
#define FLASH_FBSTROBES_PRECOL_M                                    0x00000020
#define FLASH_FBSTROBES_PRECOL_S                                             5

// Field:     [4] TI_OTP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_TI_OTP                                      0x00000010
#define FLASH_FBSTROBES_TI_OTP_BITN                                          4
#define FLASH_FBSTROBES_TI_OTP_M                                    0x00000010
#define FLASH_FBSTROBES_TI_OTP_S                                             4

// Field:     [3] OTP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_OTP                                         0x00000008
#define FLASH_FBSTROBES_OTP_BITN                                             3
#define FLASH_FBSTROBES_OTP_M                                       0x00000008
#define FLASH_FBSTROBES_OTP_S                                                3

// Field:     [2] TEZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBSTROBES_TEZ                                         0x00000004
#define FLASH_FBSTROBES_TEZ_BITN                                             2
#define FLASH_FBSTROBES_TEZ_M                                       0x00000004
#define FLASH_FBSTROBES_TEZ_S                                                2

//*****************************************************************************
//
// Register: FLASH_O_FPSTROBES
//
//*****************************************************************************
// Field:     [8] EXECUTEZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FPSTROBES_EXECUTEZ                                    0x00000100
#define FLASH_FPSTROBES_EXECUTEZ_BITN                                        8
#define FLASH_FPSTROBES_EXECUTEZ_M                                  0x00000100
#define FLASH_FPSTROBES_EXECUTEZ_S                                           8

// Field:     [1] V3PWRDNZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FPSTROBES_V3PWRDNZ                                    0x00000002
#define FLASH_FPSTROBES_V3PWRDNZ_BITN                                        1
#define FLASH_FPSTROBES_V3PWRDNZ_M                                  0x00000002
#define FLASH_FPSTROBES_V3PWRDNZ_S                                           1

// Field:     [0] V5PWRDNZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FPSTROBES_V5PWRDNZ                                    0x00000001
#define FLASH_FPSTROBES_V5PWRDNZ_BITN                                        0
#define FLASH_FPSTROBES_V5PWRDNZ_M                                  0x00000001
#define FLASH_FPSTROBES_V5PWRDNZ_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FBMODE
//
//*****************************************************************************
// Field:   [2:0] MODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FBMODE_MODE_W                                                  3
#define FLASH_FBMODE_MODE_M                                         0x00000007
#define FLASH_FBMODE_MODE_S                                                  0

//*****************************************************************************
//
// Register: FLASH_O_FTCR
//
//*****************************************************************************
// Field:   [6:0] TCR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FTCR_TCR_W                                                     7
#define FLASH_FTCR_TCR_M                                            0x0000007F
#define FLASH_FTCR_TCR_S                                                     0

//*****************************************************************************
//
// Register: FLASH_O_FADDR
//
//*****************************************************************************
// Field:  [31:0] FADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FADDR_FADDR_W                                                 32
#define FLASH_FADDR_FADDR_M                                         0xFFFFFFFF
#define FLASH_FADDR_FADDR_S                                                  0

//*****************************************************************************
//
// Register: FLASH_O_FTCTL
//
//*****************************************************************************
// Field:    [16] WDATA_BLK_CLR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FTCTL_WDATA_BLK_CLR                                   0x00010000
#define FLASH_FTCTL_WDATA_BLK_CLR_BITN                                      16
#define FLASH_FTCTL_WDATA_BLK_CLR_M                                 0x00010000
#define FLASH_FTCTL_WDATA_BLK_CLR_S                                         16

// Field:     [1] TEST_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FTCTL_TEST_EN                                         0x00000002
#define FLASH_FTCTL_TEST_EN_BITN                                             1
#define FLASH_FTCTL_TEST_EN_M                                       0x00000002
#define FLASH_FTCTL_TEST_EN_S                                                1

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE0
//
//*****************************************************************************
// Field:  [31:0] FWPWRITE0
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE0_FWPWRITE0_W                                         32
#define FLASH_FWPWRITE0_FWPWRITE0_M                                 0xFFFFFFFF
#define FLASH_FWPWRITE0_FWPWRITE0_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE1
//
//*****************************************************************************
// Field:  [31:0] FWPWRITE1
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE1_FWPWRITE1_W                                         32
#define FLASH_FWPWRITE1_FWPWRITE1_M                                 0xFFFFFFFF
#define FLASH_FWPWRITE1_FWPWRITE1_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE2
//
//*****************************************************************************
// Field:  [31:0] FWPWRITE2
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE2_FWPWRITE2_W                                         32
#define FLASH_FWPWRITE2_FWPWRITE2_M                                 0xFFFFFFFF
#define FLASH_FWPWRITE2_FWPWRITE2_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE3
//
//*****************************************************************************
// Field:  [31:0] FWPWRITE3
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE3_FWPWRITE3_W                                         32
#define FLASH_FWPWRITE3_FWPWRITE3_M                                 0xFFFFFFFF
#define FLASH_FWPWRITE3_FWPWRITE3_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE4
//
//*****************************************************************************
// Field:  [31:0] FWPWRITE4
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE4_FWPWRITE4_W                                         32
#define FLASH_FWPWRITE4_FWPWRITE4_M                                 0xFFFFFFFF
#define FLASH_FWPWRITE4_FWPWRITE4_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE5
//
//*****************************************************************************
// Field:  [31:0] FWPWRITE5
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE5_FWPWRITE5_W                                         32
#define FLASH_FWPWRITE5_FWPWRITE5_M                                 0xFFFFFFFF
#define FLASH_FWPWRITE5_FWPWRITE5_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE6
//
//*****************************************************************************
// Field:  [31:0] FWPWRITE6
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE6_FWPWRITE6_W                                         32
#define FLASH_FWPWRITE6_FWPWRITE6_M                                 0xFFFFFFFF
#define FLASH_FWPWRITE6_FWPWRITE6_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE7
//
//*****************************************************************************
// Field:  [31:0] FWPWRITE7
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE7_FWPWRITE7_W                                         32
#define FLASH_FWPWRITE7_FWPWRITE7_M                                 0xFFFFFFFF
#define FLASH_FWPWRITE7_FWPWRITE7_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FWPWRITE_ECC
//
//*****************************************************************************
// Field: [31:24] ECCBYTES07_00
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE_ECC_ECCBYTES07_00_W                                   8
#define FLASH_FWPWRITE_ECC_ECCBYTES07_00_M                          0xFF000000
#define FLASH_FWPWRITE_ECC_ECCBYTES07_00_S                                  24

// Field: [23:16] ECCBYTES15_08
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE_ECC_ECCBYTES15_08_W                                   8
#define FLASH_FWPWRITE_ECC_ECCBYTES15_08_M                          0x00FF0000
#define FLASH_FWPWRITE_ECC_ECCBYTES15_08_S                                  16

// Field:  [15:8] ECCBYTES23_16
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE_ECC_ECCBYTES23_16_W                                   8
#define FLASH_FWPWRITE_ECC_ECCBYTES23_16_M                          0x0000FF00
#define FLASH_FWPWRITE_ECC_ECCBYTES23_16_S                                   8

// Field:   [7:0] ECCBYTES31_24
//
// Internal. Only to be used through TI provided API.
#define FLASH_FWPWRITE_ECC_ECCBYTES31_24_W                                   8
#define FLASH_FWPWRITE_ECC_ECCBYTES31_24_M                          0x000000FF
#define FLASH_FWPWRITE_ECC_ECCBYTES31_24_S                                   0

//*****************************************************************************
//
// Register: FLASH_O_FSWSTAT
//
//*****************************************************************************
// Field:     [0] SAFELV
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSWSTAT_SAFELV                                        0x00000001
#define FLASH_FSWSTAT_SAFELV_BITN                                            0
#define FLASH_FSWSTAT_SAFELV_M                                      0x00000001
#define FLASH_FSWSTAT_SAFELV_S                                               0

//*****************************************************************************
//
// Register: FLASH_O_FSM_GLBCTL
//
//*****************************************************************************
// Field:     [0] CLKSEL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_GLBCTL_CLKSEL                                     0x00000001
#define FLASH_FSM_GLBCTL_CLKSEL_BITN                                         0
#define FLASH_FSM_GLBCTL_CLKSEL_M                                   0x00000001
#define FLASH_FSM_GLBCTL_CLKSEL_S                                            0

//*****************************************************************************
//
// Register: FLASH_O_FSM_STATE
//
//*****************************************************************************
// Field:    [11] CTRLENZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STATE_CTRLENZ                                     0x00000800
#define FLASH_FSM_STATE_CTRLENZ_BITN                                        11
#define FLASH_FSM_STATE_CTRLENZ_M                                   0x00000800
#define FLASH_FSM_STATE_CTRLENZ_S                                           11

// Field:    [10] EXECUTEZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STATE_EXECUTEZ                                    0x00000400
#define FLASH_FSM_STATE_EXECUTEZ_BITN                                       10
#define FLASH_FSM_STATE_EXECUTEZ_M                                  0x00000400
#define FLASH_FSM_STATE_EXECUTEZ_S                                          10

// Field:     [8] FSM_ACT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STATE_FSM_ACT                                     0x00000100
#define FLASH_FSM_STATE_FSM_ACT_BITN                                         8
#define FLASH_FSM_STATE_FSM_ACT_M                                   0x00000100
#define FLASH_FSM_STATE_FSM_ACT_S                                            8

// Field:     [7] TIOTP_ACT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STATE_TIOTP_ACT                                   0x00000080
#define FLASH_FSM_STATE_TIOTP_ACT_BITN                                       7
#define FLASH_FSM_STATE_TIOTP_ACT_M                                 0x00000080
#define FLASH_FSM_STATE_TIOTP_ACT_S                                          7

// Field:     [6] OTP_ACT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STATE_OTP_ACT                                     0x00000040
#define FLASH_FSM_STATE_OTP_ACT_BITN                                         6
#define FLASH_FSM_STATE_OTP_ACT_M                                   0x00000040
#define FLASH_FSM_STATE_OTP_ACT_S                                            6

//*****************************************************************************
//
// Register: FLASH_O_FSM_STAT
//
//*****************************************************************************
// Field:     [2] NON_OP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STAT_NON_OP                                       0x00000004
#define FLASH_FSM_STAT_NON_OP_BITN                                           2
#define FLASH_FSM_STAT_NON_OP_M                                     0x00000004
#define FLASH_FSM_STAT_NON_OP_S                                              2

// Field:     [1] OVR_PUL_CNT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STAT_OVR_PUL_CNT                                  0x00000002
#define FLASH_FSM_STAT_OVR_PUL_CNT_BITN                                      1
#define FLASH_FSM_STAT_OVR_PUL_CNT_M                                0x00000002
#define FLASH_FSM_STAT_OVR_PUL_CNT_S                                         1

// Field:     [0] INV_DAT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STAT_INV_DAT                                      0x00000001
#define FLASH_FSM_STAT_INV_DAT_BITN                                          0
#define FLASH_FSM_STAT_INV_DAT_M                                    0x00000001
#define FLASH_FSM_STAT_INV_DAT_S                                             0

//*****************************************************************************
//
// Register: FLASH_O_FSM_CMD
//
//*****************************************************************************
// Field:   [5:0] FSMCMD
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_CMD_FSMCMD_W                                               6
#define FLASH_FSM_CMD_FSMCMD_M                                      0x0000003F
#define FLASH_FSM_CMD_FSMCMD_S                                               0

//*****************************************************************************
//
// Register: FLASH_O_FSM_PE_OSU
//
//*****************************************************************************
// Field:  [15:8] PGM_OSU
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PE_OSU_PGM_OSU_W                                           8
#define FLASH_FSM_PE_OSU_PGM_OSU_M                                  0x0000FF00
#define FLASH_FSM_PE_OSU_PGM_OSU_S                                           8

// Field:   [7:0] ERA_OSU
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PE_OSU_ERA_OSU_W                                           8
#define FLASH_FSM_PE_OSU_ERA_OSU_M                                  0x000000FF
#define FLASH_FSM_PE_OSU_ERA_OSU_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FSM_VSTAT
//
//*****************************************************************************
// Field: [15:12] VSTAT_CNT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_VSTAT_VSTAT_CNT_W                                          4
#define FLASH_FSM_VSTAT_VSTAT_CNT_M                                 0x0000F000
#define FLASH_FSM_VSTAT_VSTAT_CNT_S                                         12

//*****************************************************************************
//
// Register: FLASH_O_FSM_PE_VSU
//
//*****************************************************************************
// Field:  [15:8] PGM_VSU
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PE_VSU_PGM_VSU_W                                           8
#define FLASH_FSM_PE_VSU_PGM_VSU_M                                  0x0000FF00
#define FLASH_FSM_PE_VSU_PGM_VSU_S                                           8

// Field:   [7:0] ERA_VSU
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PE_VSU_ERA_VSU_W                                           8
#define FLASH_FSM_PE_VSU_ERA_VSU_M                                  0x000000FF
#define FLASH_FSM_PE_VSU_ERA_VSU_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FSM_CMP_VSU
//
//*****************************************************************************
// Field: [15:12] ADD_EXZ
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_CMP_VSU_ADD_EXZ_W                                          4
#define FLASH_FSM_CMP_VSU_ADD_EXZ_M                                 0x0000F000
#define FLASH_FSM_CMP_VSU_ADD_EXZ_S                                         12

//*****************************************************************************
//
// Register: FLASH_O_FSM_EX_VAL
//
//*****************************************************************************
// Field:  [15:8] REP_VSU
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_EX_VAL_REP_VSU_W                                           8
#define FLASH_FSM_EX_VAL_REP_VSU_M                                  0x0000FF00
#define FLASH_FSM_EX_VAL_REP_VSU_S                                           8

// Field:   [7:0] EXE_VALD
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_EX_VAL_EXE_VALD_W                                          8
#define FLASH_FSM_EX_VAL_EXE_VALD_M                                 0x000000FF
#define FLASH_FSM_EX_VAL_EXE_VALD_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FSM_RD_H
//
//*****************************************************************************
// Field:   [7:0] RD_H
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_RD_H_RD_H_W                                                8
#define FLASH_FSM_RD_H_RD_H_M                                       0x000000FF
#define FLASH_FSM_RD_H_RD_H_S                                                0

//*****************************************************************************
//
// Register: FLASH_O_FSM_P_OH
//
//*****************************************************************************
// Field:  [15:8] PGM_OH
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_P_OH_PGM_OH_W                                              8
#define FLASH_FSM_P_OH_PGM_OH_M                                     0x0000FF00
#define FLASH_FSM_P_OH_PGM_OH_S                                              8

//*****************************************************************************
//
// Register: FLASH_O_FSM_ERA_OH
//
//*****************************************************************************
// Field:  [15:0] ERA_OH
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ERA_OH_ERA_OH_W                                           16
#define FLASH_FSM_ERA_OH_ERA_OH_M                                   0x0000FFFF
#define FLASH_FSM_ERA_OH_ERA_OH_S                                            0

//*****************************************************************************
//
// Register: FLASH_O_FSM_SAV_PPUL
//
//*****************************************************************************
// Field:  [11:0] SAV_P_PUL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_SAV_PPUL_SAV_P_PUL_W                                      12
#define FLASH_FSM_SAV_PPUL_SAV_P_PUL_M                              0x00000FFF
#define FLASH_FSM_SAV_PPUL_SAV_P_PUL_S                                       0

//*****************************************************************************
//
// Register: FLASH_O_FSM_PE_VH
//
//*****************************************************************************
// Field:  [15:8] PGM_VH
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PE_VH_PGM_VH_W                                             8
#define FLASH_FSM_PE_VH_PGM_VH_M                                    0x0000FF00
#define FLASH_FSM_PE_VH_PGM_VH_S                                             8

//*****************************************************************************
//
// Register: FLASH_O_FSM_PRG_PW
//
//*****************************************************************************
// Field:  [15:0] PROG_PUL_WIDTH
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PRG_PW_PROG_PUL_WIDTH_W                                   16
#define FLASH_FSM_PRG_PW_PROG_PUL_WIDTH_M                           0x0000FFFF
#define FLASH_FSM_PRG_PW_PROG_PUL_WIDTH_S                                    0

//*****************************************************************************
//
// Register: FLASH_O_FSM_ERA_PW
//
//*****************************************************************************
// Field:  [31:0] FSM_ERA_PW
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ERA_PW_FSM_ERA_PW_W                                       32
#define FLASH_FSM_ERA_PW_FSM_ERA_PW_M                               0xFFFFFFFF
#define FLASH_FSM_ERA_PW_FSM_ERA_PW_S                                        0

//*****************************************************************************
//
// Register: FLASH_O_FSM_SAV_ERA_PUL
//
//*****************************************************************************
// Field:  [11:0] SAV_ERA_PUL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_SAV_ERA_PUL_SAV_ERA_PUL_W                                 12
#define FLASH_FSM_SAV_ERA_PUL_SAV_ERA_PUL_M                         0x00000FFF
#define FLASH_FSM_SAV_ERA_PUL_SAV_ERA_PUL_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FSM_TIMER
//
//*****************************************************************************
// Field:  [31:0] FSM_TIMER
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_TIMER_FSM_TIMER_W                                         32
#define FLASH_FSM_TIMER_FSM_TIMER_M                                 0xFFFFFFFF
#define FLASH_FSM_TIMER_FSM_TIMER_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FSM_MODE
//
//*****************************************************************************
// Field: [19:18] RDV_SUBMODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_MODE_RDV_SUBMODE_W                                         2
#define FLASH_FSM_MODE_RDV_SUBMODE_M                                0x000C0000
#define FLASH_FSM_MODE_RDV_SUBMODE_S                                        18

// Field: [17:16] PGM_SUBMODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_MODE_PGM_SUBMODE_W                                         2
#define FLASH_FSM_MODE_PGM_SUBMODE_M                                0x00030000
#define FLASH_FSM_MODE_PGM_SUBMODE_S                                        16

// Field: [15:14] ERA_SUBMODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_MODE_ERA_SUBMODE_W                                         2
#define FLASH_FSM_MODE_ERA_SUBMODE_M                                0x0000C000
#define FLASH_FSM_MODE_ERA_SUBMODE_S                                        14

// Field: [13:12] SUBMODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_MODE_SUBMODE_W                                             2
#define FLASH_FSM_MODE_SUBMODE_M                                    0x00003000
#define FLASH_FSM_MODE_SUBMODE_S                                            12

// Field:  [11:9] SAV_PGM_CMD
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_MODE_SAV_PGM_CMD_W                                         3
#define FLASH_FSM_MODE_SAV_PGM_CMD_M                                0x00000E00
#define FLASH_FSM_MODE_SAV_PGM_CMD_S                                         9

// Field:   [8:6] SAV_ERA_MODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_MODE_SAV_ERA_MODE_W                                        3
#define FLASH_FSM_MODE_SAV_ERA_MODE_M                               0x000001C0
#define FLASH_FSM_MODE_SAV_ERA_MODE_S                                        6

// Field:   [5:3] MODE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_MODE_MODE_W                                                3
#define FLASH_FSM_MODE_MODE_M                                       0x00000038
#define FLASH_FSM_MODE_MODE_S                                                3

// Field:   [2:0] CMD
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_MODE_CMD_W                                                 3
#define FLASH_FSM_MODE_CMD_M                                        0x00000007
#define FLASH_FSM_MODE_CMD_S                                                 0

//*****************************************************************************
//
// Register: FLASH_O_FSM_PGM
//
//*****************************************************************************
// Field: [25:23] PGM_BANK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PGM_PGM_BANK_W                                             3
#define FLASH_FSM_PGM_PGM_BANK_M                                    0x03800000
#define FLASH_FSM_PGM_PGM_BANK_S                                            23

// Field:  [22:0] PGM_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PGM_PGM_ADDR_W                                            23
#define FLASH_FSM_PGM_PGM_ADDR_M                                    0x007FFFFF
#define FLASH_FSM_PGM_PGM_ADDR_S                                             0

//*****************************************************************************
//
// Register: FLASH_O_FSM_ERA
//
//*****************************************************************************
// Field: [25:23] ERA_BANK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ERA_ERA_BANK_W                                             3
#define FLASH_FSM_ERA_ERA_BANK_M                                    0x03800000
#define FLASH_FSM_ERA_ERA_BANK_S                                            23

// Field:  [22:0] ERA_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ERA_ERA_ADDR_W                                            23
#define FLASH_FSM_ERA_ERA_ADDR_M                                    0x007FFFFF
#define FLASH_FSM_ERA_ERA_ADDR_S                                             0

//*****************************************************************************
//
// Register: FLASH_O_FSM_PRG_PUL
//
//*****************************************************************************
// Field: [19:16] BEG_EC_LEVEL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PRG_PUL_BEG_EC_LEVEL_W                                     4
#define FLASH_FSM_PRG_PUL_BEG_EC_LEVEL_M                            0x000F0000
#define FLASH_FSM_PRG_PUL_BEG_EC_LEVEL_S                                    16

// Field:  [11:0] MAX_PRG_PUL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PRG_PUL_MAX_PRG_PUL_W                                     12
#define FLASH_FSM_PRG_PUL_MAX_PRG_PUL_M                             0x00000FFF
#define FLASH_FSM_PRG_PUL_MAX_PRG_PUL_S                                      0

//*****************************************************************************
//
// Register: FLASH_O_FSM_ERA_PUL
//
//*****************************************************************************
// Field: [19:16] MAX_EC_LEVEL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ERA_PUL_MAX_EC_LEVEL_W                                     4
#define FLASH_FSM_ERA_PUL_MAX_EC_LEVEL_M                            0x000F0000
#define FLASH_FSM_ERA_PUL_MAX_EC_LEVEL_S                                    16

// Field:  [11:0] MAX_ERA_PUL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ERA_PUL_MAX_ERA_PUL_W                                     12
#define FLASH_FSM_ERA_PUL_MAX_ERA_PUL_M                             0x00000FFF
#define FLASH_FSM_ERA_PUL_MAX_ERA_PUL_S                                      0

//*****************************************************************************
//
// Register: FLASH_O_FSM_STEP_SIZE
//
//*****************************************************************************
// Field: [24:16] EC_STEP_SIZE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_STEP_SIZE_EC_STEP_SIZE_W                                   9
#define FLASH_FSM_STEP_SIZE_EC_STEP_SIZE_M                          0x01FF0000
#define FLASH_FSM_STEP_SIZE_EC_STEP_SIZE_S                                  16

//*****************************************************************************
//
// Register: FLASH_O_FSM_PUL_CNTR
//
//*****************************************************************************
// Field: [24:16] CUR_EC_LEVEL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PUL_CNTR_CUR_EC_LEVEL_W                                    9
#define FLASH_FSM_PUL_CNTR_CUR_EC_LEVEL_M                           0x01FF0000
#define FLASH_FSM_PUL_CNTR_CUR_EC_LEVEL_S                                   16

// Field:  [11:0] PUL_CNTR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PUL_CNTR_PUL_CNTR_W                                       12
#define FLASH_FSM_PUL_CNTR_PUL_CNTR_M                               0x00000FFF
#define FLASH_FSM_PUL_CNTR_PUL_CNTR_S                                        0

//*****************************************************************************
//
// Register: FLASH_O_FSM_EC_STEP_HEIGHT
//
//*****************************************************************************
// Field:   [3:0] EC_STEP_HEIGHT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_EC_STEP_HEIGHT_EC_STEP_HEIGHT_W                            4
#define FLASH_FSM_EC_STEP_HEIGHT_EC_STEP_HEIGHT_M                   0x0000000F
#define FLASH_FSM_EC_STEP_HEIGHT_EC_STEP_HEIGHT_S                            0

//*****************************************************************************
//
// Register: FLASH_O_FSM_ST_MACHINE
//
//*****************************************************************************
// Field:    [23] DO_PRECOND
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_DO_PRECOND                             0x00800000
#define FLASH_FSM_ST_MACHINE_DO_PRECOND_BITN                                23
#define FLASH_FSM_ST_MACHINE_DO_PRECOND_M                           0x00800000
#define FLASH_FSM_ST_MACHINE_DO_PRECOND_S                                   23

// Field:    [22] FSM_INT_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_FSM_INT_EN                             0x00400000
#define FLASH_FSM_ST_MACHINE_FSM_INT_EN_BITN                                22
#define FLASH_FSM_ST_MACHINE_FSM_INT_EN_M                           0x00400000
#define FLASH_FSM_ST_MACHINE_FSM_INT_EN_S                                   22

// Field:    [21] ALL_BANKS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_ALL_BANKS                              0x00200000
#define FLASH_FSM_ST_MACHINE_ALL_BANKS_BITN                                 21
#define FLASH_FSM_ST_MACHINE_ALL_BANKS_M                            0x00200000
#define FLASH_FSM_ST_MACHINE_ALL_BANKS_S                                    21

// Field:    [20] CMPV_ALLOWED
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_CMPV_ALLOWED                           0x00100000
#define FLASH_FSM_ST_MACHINE_CMPV_ALLOWED_BITN                              20
#define FLASH_FSM_ST_MACHINE_CMPV_ALLOWED_M                         0x00100000
#define FLASH_FSM_ST_MACHINE_CMPV_ALLOWED_S                                 20

// Field:    [19] RANDOM
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_RANDOM                                 0x00080000
#define FLASH_FSM_ST_MACHINE_RANDOM_BITN                                    19
#define FLASH_FSM_ST_MACHINE_RANDOM_M                               0x00080000
#define FLASH_FSM_ST_MACHINE_RANDOM_S                                       19

// Field:    [18] RV_SEC_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_RV_SEC_EN                              0x00040000
#define FLASH_FSM_ST_MACHINE_RV_SEC_EN_BITN                                 18
#define FLASH_FSM_ST_MACHINE_RV_SEC_EN_M                            0x00040000
#define FLASH_FSM_ST_MACHINE_RV_SEC_EN_S                                    18

// Field:    [17] RV_RES
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_RV_RES                                 0x00020000
#define FLASH_FSM_ST_MACHINE_RV_RES_BITN                                    17
#define FLASH_FSM_ST_MACHINE_RV_RES_M                               0x00020000
#define FLASH_FSM_ST_MACHINE_RV_RES_S                                       17

// Field:    [16] RV_INT_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_RV_INT_EN                              0x00010000
#define FLASH_FSM_ST_MACHINE_RV_INT_EN_BITN                                 16
#define FLASH_FSM_ST_MACHINE_RV_INT_EN_M                            0x00010000
#define FLASH_FSM_ST_MACHINE_RV_INT_EN_S                                    16

// Field:    [14] ONE_TIME_GOOD
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_ONE_TIME_GOOD                          0x00004000
#define FLASH_FSM_ST_MACHINE_ONE_TIME_GOOD_BITN                             14
#define FLASH_FSM_ST_MACHINE_ONE_TIME_GOOD_M                        0x00004000
#define FLASH_FSM_ST_MACHINE_ONE_TIME_GOOD_S                                14

// Field:    [11] DO_REDU_COL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_DO_REDU_COL                            0x00000800
#define FLASH_FSM_ST_MACHINE_DO_REDU_COL_BITN                               11
#define FLASH_FSM_ST_MACHINE_DO_REDU_COL_M                          0x00000800
#define FLASH_FSM_ST_MACHINE_DO_REDU_COL_S                                  11

// Field:  [10:7] DBG_SHORT_ROW
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_DBG_SHORT_ROW_W                                 4
#define FLASH_FSM_ST_MACHINE_DBG_SHORT_ROW_M                        0x00000780
#define FLASH_FSM_ST_MACHINE_DBG_SHORT_ROW_S                                 7

// Field:     [5] PGM_SEC_COF_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_PGM_SEC_COF_EN                         0x00000020
#define FLASH_FSM_ST_MACHINE_PGM_SEC_COF_EN_BITN                             5
#define FLASH_FSM_ST_MACHINE_PGM_SEC_COF_EN_M                       0x00000020
#define FLASH_FSM_ST_MACHINE_PGM_SEC_COF_EN_S                                5

// Field:     [4] PREC_STOP_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_PREC_STOP_EN                           0x00000010
#define FLASH_FSM_ST_MACHINE_PREC_STOP_EN_BITN                               4
#define FLASH_FSM_ST_MACHINE_PREC_STOP_EN_M                         0x00000010
#define FLASH_FSM_ST_MACHINE_PREC_STOP_EN_S                                  4

// Field:     [3] DIS_TST_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_DIS_TST_EN                             0x00000008
#define FLASH_FSM_ST_MACHINE_DIS_TST_EN_BITN                                 3
#define FLASH_FSM_ST_MACHINE_DIS_TST_EN_M                           0x00000008
#define FLASH_FSM_ST_MACHINE_DIS_TST_EN_S                                    3

// Field:     [2] CMD_EN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_CMD_EN                                 0x00000004
#define FLASH_FSM_ST_MACHINE_CMD_EN_BITN                                     2
#define FLASH_FSM_ST_MACHINE_CMD_EN_M                               0x00000004
#define FLASH_FSM_ST_MACHINE_CMD_EN_S                                        2

// Field:     [1] INV_DATA
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_INV_DATA                               0x00000002
#define FLASH_FSM_ST_MACHINE_INV_DATA_BITN                                   1
#define FLASH_FSM_ST_MACHINE_INV_DATA_M                             0x00000002
#define FLASH_FSM_ST_MACHINE_INV_DATA_S                                      1

// Field:     [0] OVERRIDE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ST_MACHINE_OVERRIDE                               0x00000001
#define FLASH_FSM_ST_MACHINE_OVERRIDE_BITN                                   0
#define FLASH_FSM_ST_MACHINE_OVERRIDE_M                             0x00000001
#define FLASH_FSM_ST_MACHINE_OVERRIDE_S                                      0

//*****************************************************************************
//
// Register: FLASH_O_FSM_FLES
//
//*****************************************************************************
// Field:  [11:8] BLK_TIOTP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_FLES_BLK_TIOTP_W                                           4
#define FLASH_FSM_FLES_BLK_TIOTP_M                                  0x00000F00
#define FLASH_FSM_FLES_BLK_TIOTP_S                                           8

// Field:   [7:0] BLK_OTP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_FLES_BLK_OTP_W                                             8
#define FLASH_FSM_FLES_BLK_OTP_M                                    0x000000FF
#define FLASH_FSM_FLES_BLK_OTP_S                                             0

//*****************************************************************************
//
// Register: FLASH_O_FSM_WR_ENA
//
//*****************************************************************************
// Field:   [2:0] WR_ENA
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_WR_ENA_WR_ENA_W                                            3
#define FLASH_FSM_WR_ENA_WR_ENA_M                                   0x00000007
#define FLASH_FSM_WR_ENA_WR_ENA_S                                            0

//*****************************************************************************
//
// Register: FLASH_O_FSM_ACC_PP
//
//*****************************************************************************
// Field:  [31:0] FSM_ACC_PP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ACC_PP_FSM_ACC_PP_W                                       32
#define FLASH_FSM_ACC_PP_FSM_ACC_PP_M                               0xFFFFFFFF
#define FLASH_FSM_ACC_PP_FSM_ACC_PP_S                                        0

//*****************************************************************************
//
// Register: FLASH_O_FSM_ACC_EP
//
//*****************************************************************************
// Field:  [15:0] ACC_EP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ACC_EP_ACC_EP_W                                           16
#define FLASH_FSM_ACC_EP_ACC_EP_M                                   0x0000FFFF
#define FLASH_FSM_ACC_EP_ACC_EP_S                                            0

//*****************************************************************************
//
// Register: FLASH_O_FSM_ADDR
//
//*****************************************************************************
// Field: [30:28] BANK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ADDR_BANK_W                                                3
#define FLASH_FSM_ADDR_BANK_M                                       0x70000000
#define FLASH_FSM_ADDR_BANK_S                                               28

// Field:  [27:0] CUR_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ADDR_CUR_ADDR_W                                           28
#define FLASH_FSM_ADDR_CUR_ADDR_M                                   0x0FFFFFFF
#define FLASH_FSM_ADDR_CUR_ADDR_S                                            0

//*****************************************************************************
//
// Register: FLASH_O_FSM_SECTOR
//
//*****************************************************************************
// Field: [31:16] SECT_ERASED
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_SECTOR_SECT_ERASED_W                                      16
#define FLASH_FSM_SECTOR_SECT_ERASED_M                              0xFFFF0000
#define FLASH_FSM_SECTOR_SECT_ERASED_S                                      16

// Field:  [15:8] FSM_SECTOR_EXTENSION
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_SECTOR_FSM_SECTOR_EXTENSION_W                              8
#define FLASH_FSM_SECTOR_FSM_SECTOR_EXTENSION_M                     0x0000FF00
#define FLASH_FSM_SECTOR_FSM_SECTOR_EXTENSION_S                              8

// Field:   [7:4] SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_SECTOR_SECTOR_W                                            4
#define FLASH_FSM_SECTOR_SECTOR_M                                   0x000000F0
#define FLASH_FSM_SECTOR_SECTOR_S                                            4

// Field:   [3:0] SEC_OUT
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_SECTOR_SEC_OUT_W                                           4
#define FLASH_FSM_SECTOR_SEC_OUT_M                                  0x0000000F
#define FLASH_FSM_SECTOR_SEC_OUT_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FMC_REV_ID
//
//*****************************************************************************
// Field: [31:12] MOD_VERSION
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMC_REV_ID_MOD_VERSION_W                                      20
#define FLASH_FMC_REV_ID_MOD_VERSION_M                              0xFFFFF000
#define FLASH_FMC_REV_ID_MOD_VERSION_S                                      12

// Field:  [11:0] CONFIG_CRC
//
// Internal. Only to be used through TI provided API.
#define FLASH_FMC_REV_ID_CONFIG_CRC_W                                       12
#define FLASH_FMC_REV_ID_CONFIG_CRC_M                               0x00000FFF
#define FLASH_FMC_REV_ID_CONFIG_CRC_S                                        0

//*****************************************************************************
//
// Register: FLASH_O_FSM_ERR_ADDR
//
//*****************************************************************************
// Field:  [31:8] FSM_ERR_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ERR_ADDR_FSM_ERR_ADDR_W                                   24
#define FLASH_FSM_ERR_ADDR_FSM_ERR_ADDR_M                           0xFFFFFF00
#define FLASH_FSM_ERR_ADDR_FSM_ERR_ADDR_S                                    8

// Field:   [3:0] FSM_ERR_BANK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_ERR_ADDR_FSM_ERR_BANK_W                                    4
#define FLASH_FSM_ERR_ADDR_FSM_ERR_BANK_M                           0x0000000F
#define FLASH_FSM_ERR_ADDR_FSM_ERR_BANK_S                                    0

//*****************************************************************************
//
// Register: FLASH_O_FSM_PGM_MAXPUL
//
//*****************************************************************************
// Field:  [11:0] FSM_PGM_MAXPUL
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_PGM_MAXPUL_FSM_PGM_MAXPUL_W                               12
#define FLASH_FSM_PGM_MAXPUL_FSM_PGM_MAXPUL_M                       0x00000FFF
#define FLASH_FSM_PGM_MAXPUL_FSM_PGM_MAXPUL_S                                0

//*****************************************************************************
//
// Register: FLASH_O_FSM_EXECUTE
//
//*****************************************************************************
// Field: [19:16] SUSPEND_NOW
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_EXECUTE_SUSPEND_NOW_W                                      4
#define FLASH_FSM_EXECUTE_SUSPEND_NOW_M                             0x000F0000
#define FLASH_FSM_EXECUTE_SUSPEND_NOW_S                                     16

// Field:   [4:0] FSMEXECUTE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_EXECUTE_FSMEXECUTE_W                                       5
#define FLASH_FSM_EXECUTE_FSMEXECUTE_M                              0x0000001F
#define FLASH_FSM_EXECUTE_FSMEXECUTE_S                                       0

//*****************************************************************************
//
// Register: FLASH_O_FSM_SECTOR1
//
//*****************************************************************************
// Field:  [31:0] FSM_SECTOR1
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_SECTOR1_FSM_SECTOR1_W                                     32
#define FLASH_FSM_SECTOR1_FSM_SECTOR1_M                             0xFFFFFFFF
#define FLASH_FSM_SECTOR1_FSM_SECTOR1_S                                      0

//*****************************************************************************
//
// Register: FLASH_O_FSM_SECTOR2
//
//*****************************************************************************
// Field:  [31:0] FSM_SECTOR2
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_SECTOR2_FSM_SECTOR2_W                                     32
#define FLASH_FSM_SECTOR2_FSM_SECTOR2_M                             0xFFFFFFFF
#define FLASH_FSM_SECTOR2_FSM_SECTOR2_S                                      0

//*****************************************************************************
//
// Register: FLASH_O_FSM_BSLE0
//
//*****************************************************************************
// Field:  [31:0] FSM_BSLE0
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_BSLE0_FSM_BSLE0_W                                         32
#define FLASH_FSM_BSLE0_FSM_BSLE0_M                                 0xFFFFFFFF
#define FLASH_FSM_BSLE0_FSM_BSLE0_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FSM_BSLE1
//
//*****************************************************************************
// Field:  [31:0] FSM_BSL1
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_BSLE1_FSM_BSL1_W                                          32
#define FLASH_FSM_BSLE1_FSM_BSL1_M                                  0xFFFFFFFF
#define FLASH_FSM_BSLE1_FSM_BSL1_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FSM_BSLP0
//
//*****************************************************************************
// Field:  [31:0] FSM_BSLP0
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_BSLP0_FSM_BSLP0_W                                         32
#define FLASH_FSM_BSLP0_FSM_BSLP0_M                                 0xFFFFFFFF
#define FLASH_FSM_BSLP0_FSM_BSLP0_S                                          0

//*****************************************************************************
//
// Register: FLASH_O_FSM_BSLP1
//
//*****************************************************************************
// Field:  [31:0] FSM_BSL1
//
// Internal. Only to be used through TI provided API.
#define FLASH_FSM_BSLP1_FSM_BSL1_W                                          32
#define FLASH_FSM_BSLP1_FSM_BSL1_M                                  0xFFFFFFFF
#define FLASH_FSM_BSLP1_FSM_BSL1_S                                           0

//*****************************************************************************
//
// Register: FLASH_O_FSM_PGM128
//
//*****************************************************************************
// Field:     [0] EN_PGM128
//
// 1: Enables 128-bit wide programming.  This mode requires programming supply
// voltage to be greater than 2.5v at the Flash Pump.  The primary use case for
// this mode is manufacturing test for test time reduction.
//
// 0: 64-bit wide programming.  Valid at any programming voltage.  A 128-bit
// word is divided into two 64-bit words for programming. [default]
//
// This register is write protected with the FSM_WR_ENA register.
#define FLASH_FSM_PGM128_EN_PGM128                                  0x00000001
#define FLASH_FSM_PGM128_EN_PGM128_BITN                                      0
#define FLASH_FSM_PGM128_EN_PGM128_M                                0x00000001
#define FLASH_FSM_PGM128_EN_PGM128_S                                         0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_BANK
//
//*****************************************************************************
// Field: [31:20] EE_BANK_WIDTH
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BANK_EE_BANK_WIDTH_W                                     12
#define FLASH_FCFG_BANK_EE_BANK_WIDTH_M                             0xFFF00000
#define FLASH_FCFG_BANK_EE_BANK_WIDTH_S                                     20

// Field: [19:16] EE_NUM_BANK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BANK_EE_NUM_BANK_W                                        4
#define FLASH_FCFG_BANK_EE_NUM_BANK_M                               0x000F0000
#define FLASH_FCFG_BANK_EE_NUM_BANK_S                                       16

// Field:  [15:4] MAIN_BANK_WIDTH
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BANK_MAIN_BANK_WIDTH_W                                   12
#define FLASH_FCFG_BANK_MAIN_BANK_WIDTH_M                           0x0000FFF0
#define FLASH_FCFG_BANK_MAIN_BANK_WIDTH_S                                    4

// Field:   [3:0] MAIN_NUM_BANK
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BANK_MAIN_NUM_BANK_W                                      4
#define FLASH_FCFG_BANK_MAIN_NUM_BANK_M                             0x0000000F
#define FLASH_FCFG_BANK_MAIN_NUM_BANK_S                                      0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_WRAPPER
//
//*****************************************************************************
// Field: [31:24] FAMILY_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_FAMILY_TYPE_W                                     8
#define FLASH_FCFG_WRAPPER_FAMILY_TYPE_M                            0xFF000000
#define FLASH_FCFG_WRAPPER_FAMILY_TYPE_S                                    24

// Field:    [20] MEM_MAP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_MEM_MAP                                  0x00100000
#define FLASH_FCFG_WRAPPER_MEM_MAP_BITN                                     20
#define FLASH_FCFG_WRAPPER_MEM_MAP_M                                0x00100000
#define FLASH_FCFG_WRAPPER_MEM_MAP_S                                        20

// Field: [19:16] CPU2
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_CPU2_W                                            4
#define FLASH_FCFG_WRAPPER_CPU2_M                                   0x000F0000
#define FLASH_FCFG_WRAPPER_CPU2_S                                           16

// Field: [15:12] EE_IN_MAIN
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_EE_IN_MAIN_W                                      4
#define FLASH_FCFG_WRAPPER_EE_IN_MAIN_M                             0x0000F000
#define FLASH_FCFG_WRAPPER_EE_IN_MAIN_S                                     12

// Field:    [11] ROM
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_ROM                                      0x00000800
#define FLASH_FCFG_WRAPPER_ROM_BITN                                         11
#define FLASH_FCFG_WRAPPER_ROM_M                                    0x00000800
#define FLASH_FCFG_WRAPPER_ROM_S                                            11

// Field:    [10] IFLUSH
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_IFLUSH                                   0x00000400
#define FLASH_FCFG_WRAPPER_IFLUSH_BITN                                      10
#define FLASH_FCFG_WRAPPER_IFLUSH_M                                 0x00000400
#define FLASH_FCFG_WRAPPER_IFLUSH_S                                         10

// Field:     [9] SIL3
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_SIL3                                     0x00000200
#define FLASH_FCFG_WRAPPER_SIL3_BITN                                         9
#define FLASH_FCFG_WRAPPER_SIL3_M                                   0x00000200
#define FLASH_FCFG_WRAPPER_SIL3_S                                            9

// Field:     [8] ECCA
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_ECCA                                     0x00000100
#define FLASH_FCFG_WRAPPER_ECCA_BITN                                         8
#define FLASH_FCFG_WRAPPER_ECCA_M                                   0x00000100
#define FLASH_FCFG_WRAPPER_ECCA_S                                            8

// Field:   [7:6] AUTO_SUSP
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_AUTO_SUSP_W                                       2
#define FLASH_FCFG_WRAPPER_AUTO_SUSP_M                              0x000000C0
#define FLASH_FCFG_WRAPPER_AUTO_SUSP_S                                       6

// Field:   [5:4] UERR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_UERR_W                                            2
#define FLASH_FCFG_WRAPPER_UERR_M                                   0x00000030
#define FLASH_FCFG_WRAPPER_UERR_S                                            4

// Field:   [3:0] CPU_TYPE1
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_WRAPPER_CPU_TYPE1_W                                       4
#define FLASH_FCFG_WRAPPER_CPU_TYPE1_M                              0x0000000F
#define FLASH_FCFG_WRAPPER_CPU_TYPE1_S                                       0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_BNK_TYPE
//
//*****************************************************************************
// Field: [31:28] B7_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BNK_TYPE_B7_TYPE_W                                        4
#define FLASH_FCFG_BNK_TYPE_B7_TYPE_M                               0xF0000000
#define FLASH_FCFG_BNK_TYPE_B7_TYPE_S                                       28

// Field: [27:24] B6_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BNK_TYPE_B6_TYPE_W                                        4
#define FLASH_FCFG_BNK_TYPE_B6_TYPE_M                               0x0F000000
#define FLASH_FCFG_BNK_TYPE_B6_TYPE_S                                       24

// Field: [23:20] B5_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BNK_TYPE_B5_TYPE_W                                        4
#define FLASH_FCFG_BNK_TYPE_B5_TYPE_M                               0x00F00000
#define FLASH_FCFG_BNK_TYPE_B5_TYPE_S                                       20

// Field: [19:16] B4_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BNK_TYPE_B4_TYPE_W                                        4
#define FLASH_FCFG_BNK_TYPE_B4_TYPE_M                               0x000F0000
#define FLASH_FCFG_BNK_TYPE_B4_TYPE_S                                       16

// Field: [15:12] B3_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BNK_TYPE_B3_TYPE_W                                        4
#define FLASH_FCFG_BNK_TYPE_B3_TYPE_M                               0x0000F000
#define FLASH_FCFG_BNK_TYPE_B3_TYPE_S                                       12

// Field:  [11:8] B2_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BNK_TYPE_B2_TYPE_W                                        4
#define FLASH_FCFG_BNK_TYPE_B2_TYPE_M                               0x00000F00
#define FLASH_FCFG_BNK_TYPE_B2_TYPE_S                                        8

// Field:   [7:4] B1_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BNK_TYPE_B1_TYPE_W                                        4
#define FLASH_FCFG_BNK_TYPE_B1_TYPE_M                               0x000000F0
#define FLASH_FCFG_BNK_TYPE_B1_TYPE_S                                        4

// Field:   [3:0] B0_TYPE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_BNK_TYPE_B0_TYPE_W                                        4
#define FLASH_FCFG_BNK_TYPE_B0_TYPE_M                               0x0000000F
#define FLASH_FCFG_BNK_TYPE_B0_TYPE_S                                        0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B0_START
//
//*****************************************************************************
// Field: [31:28] B0_MAX_SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B0_START_B0_MAX_SECTOR_W                                  4
#define FLASH_FCFG_B0_START_B0_MAX_SECTOR_M                         0xF0000000
#define FLASH_FCFG_B0_START_B0_MAX_SECTOR_S                                 28

// Field: [27:24] B0_MUX_FACTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B0_START_B0_MUX_FACTOR_W                                  4
#define FLASH_FCFG_B0_START_B0_MUX_FACTOR_M                         0x0F000000
#define FLASH_FCFG_B0_START_B0_MUX_FACTOR_S                                 24

// Field:  [23:0] B0_START_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B0_START_B0_START_ADDR_W                                 24
#define FLASH_FCFG_B0_START_B0_START_ADDR_M                         0x00FFFFFF
#define FLASH_FCFG_B0_START_B0_START_ADDR_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B1_START
//
//*****************************************************************************
// Field: [31:28] B1_MAX_SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B1_START_B1_MAX_SECTOR_W                                  4
#define FLASH_FCFG_B1_START_B1_MAX_SECTOR_M                         0xF0000000
#define FLASH_FCFG_B1_START_B1_MAX_SECTOR_S                                 28

// Field: [27:24] B1_MUX_FACTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B1_START_B1_MUX_FACTOR_W                                  4
#define FLASH_FCFG_B1_START_B1_MUX_FACTOR_M                         0x0F000000
#define FLASH_FCFG_B1_START_B1_MUX_FACTOR_S                                 24

// Field:  [23:0] B1_START_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B1_START_B1_START_ADDR_W                                 24
#define FLASH_FCFG_B1_START_B1_START_ADDR_M                         0x00FFFFFF
#define FLASH_FCFG_B1_START_B1_START_ADDR_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B2_START
//
//*****************************************************************************
// Field: [31:28] B2_MAX_SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B2_START_B2_MAX_SECTOR_W                                  4
#define FLASH_FCFG_B2_START_B2_MAX_SECTOR_M                         0xF0000000
#define FLASH_FCFG_B2_START_B2_MAX_SECTOR_S                                 28

// Field: [27:24] B2_MUX_FACTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B2_START_B2_MUX_FACTOR_W                                  4
#define FLASH_FCFG_B2_START_B2_MUX_FACTOR_M                         0x0F000000
#define FLASH_FCFG_B2_START_B2_MUX_FACTOR_S                                 24

// Field:  [23:0] B2_START_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B2_START_B2_START_ADDR_W                                 24
#define FLASH_FCFG_B2_START_B2_START_ADDR_M                         0x00FFFFFF
#define FLASH_FCFG_B2_START_B2_START_ADDR_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B3_START
//
//*****************************************************************************
// Field: [31:28] B3_MAX_SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B3_START_B3_MAX_SECTOR_W                                  4
#define FLASH_FCFG_B3_START_B3_MAX_SECTOR_M                         0xF0000000
#define FLASH_FCFG_B3_START_B3_MAX_SECTOR_S                                 28

// Field: [27:24] B3_MUX_FACTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B3_START_B3_MUX_FACTOR_W                                  4
#define FLASH_FCFG_B3_START_B3_MUX_FACTOR_M                         0x0F000000
#define FLASH_FCFG_B3_START_B3_MUX_FACTOR_S                                 24

// Field:  [23:0] B3_START_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B3_START_B3_START_ADDR_W                                 24
#define FLASH_FCFG_B3_START_B3_START_ADDR_M                         0x00FFFFFF
#define FLASH_FCFG_B3_START_B3_START_ADDR_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B4_START
//
//*****************************************************************************
// Field: [31:28] B4_MAX_SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B4_START_B4_MAX_SECTOR_W                                  4
#define FLASH_FCFG_B4_START_B4_MAX_SECTOR_M                         0xF0000000
#define FLASH_FCFG_B4_START_B4_MAX_SECTOR_S                                 28

// Field: [27:24] B4_MUX_FACTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B4_START_B4_MUX_FACTOR_W                                  4
#define FLASH_FCFG_B4_START_B4_MUX_FACTOR_M                         0x0F000000
#define FLASH_FCFG_B4_START_B4_MUX_FACTOR_S                                 24

// Field:  [23:0] B4_START_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B4_START_B4_START_ADDR_W                                 24
#define FLASH_FCFG_B4_START_B4_START_ADDR_M                         0x00FFFFFF
#define FLASH_FCFG_B4_START_B4_START_ADDR_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B5_START
//
//*****************************************************************************
// Field: [31:28] B5_MAX_SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B5_START_B5_MAX_SECTOR_W                                  4
#define FLASH_FCFG_B5_START_B5_MAX_SECTOR_M                         0xF0000000
#define FLASH_FCFG_B5_START_B5_MAX_SECTOR_S                                 28

// Field: [27:24] B5_MUX_FACTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B5_START_B5_MUX_FACTOR_W                                  4
#define FLASH_FCFG_B5_START_B5_MUX_FACTOR_M                         0x0F000000
#define FLASH_FCFG_B5_START_B5_MUX_FACTOR_S                                 24

// Field:  [23:0] B5_START_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B5_START_B5_START_ADDR_W                                 24
#define FLASH_FCFG_B5_START_B5_START_ADDR_M                         0x00FFFFFF
#define FLASH_FCFG_B5_START_B5_START_ADDR_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B6_START
//
//*****************************************************************************
// Field: [31:28] B6_MAX_SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B6_START_B6_MAX_SECTOR_W                                  4
#define FLASH_FCFG_B6_START_B6_MAX_SECTOR_M                         0xF0000000
#define FLASH_FCFG_B6_START_B6_MAX_SECTOR_S                                 28

// Field: [27:24] B6_MUX_FACTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B6_START_B6_MUX_FACTOR_W                                  4
#define FLASH_FCFG_B6_START_B6_MUX_FACTOR_M                         0x0F000000
#define FLASH_FCFG_B6_START_B6_MUX_FACTOR_S                                 24

// Field:  [23:0] B6_START_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B6_START_B6_START_ADDR_W                                 24
#define FLASH_FCFG_B6_START_B6_START_ADDR_M                         0x00FFFFFF
#define FLASH_FCFG_B6_START_B6_START_ADDR_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B7_START
//
//*****************************************************************************
// Field: [31:28] B7_MAX_SECTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B7_START_B7_MAX_SECTOR_W                                  4
#define FLASH_FCFG_B7_START_B7_MAX_SECTOR_M                         0xF0000000
#define FLASH_FCFG_B7_START_B7_MAX_SECTOR_S                                 28

// Field: [27:24] B7_MUX_FACTOR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B7_START_B7_MUX_FACTOR_W                                  4
#define FLASH_FCFG_B7_START_B7_MUX_FACTOR_M                         0x0F000000
#define FLASH_FCFG_B7_START_B7_MUX_FACTOR_S                                 24

// Field:  [23:0] B7_START_ADDR
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B7_START_B7_START_ADDR_W                                 24
#define FLASH_FCFG_B7_START_B7_START_ADDR_M                         0x00FFFFFF
#define FLASH_FCFG_B7_START_B7_START_ADDR_S                                  0

//*****************************************************************************
//
// Register: FLASH_O_FCFG_B0_SSIZE0
//
//*****************************************************************************
// Field: [27:16] B0_NUM_SECTORS
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B0_SSIZE0_B0_NUM_SECTORS_W                               12
#define FLASH_FCFG_B0_SSIZE0_B0_NUM_SECTORS_M                       0x0FFF0000
#define FLASH_FCFG_B0_SSIZE0_B0_NUM_SECTORS_S                               16

// Field:   [3:0] B0_SECT_SIZE
//
// Internal. Only to be used through TI provided API.
#define FLASH_FCFG_B0_SSIZE0_B0_SECT_SIZE_W                                  4
#define FLASH_FCFG_B0_SSIZE0_B0_SECT_SIZE_M                         0x0000000F
#define FLASH_FCFG_B0_SSIZE0_B0_SECT_SIZE_S                                  0


#endif // __FLASH__
