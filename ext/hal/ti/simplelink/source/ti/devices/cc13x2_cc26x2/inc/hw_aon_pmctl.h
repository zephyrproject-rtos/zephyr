/******************************************************************************
*  Filename:       hw_aon_pmctl_h
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

#ifndef __HW_AON_PMCTL_H__
#define __HW_AON_PMCTL_H__

//*****************************************************************************
//
// This section defines the register offsets of
// AON_PMCTL component
//
//*****************************************************************************
// AUX SCE Clock Management
#define AON_PMCTL_O_AUXSCECLK                                       0x00000004

// RAM Configuration
#define AON_PMCTL_O_RAMCFG                                          0x00000008

// Power Management Control
#define AON_PMCTL_O_PWRCTL                                          0x00000010

// AON Power and Reset Status
#define AON_PMCTL_O_PWRSTAT                                         0x00000014

// Shutdown Control
#define AON_PMCTL_O_SHUTDOWN                                        0x00000018

// Recharge Controller Configuration
#define AON_PMCTL_O_RECHARGECFG                                     0x0000001C

// Recharge Controller Status
#define AON_PMCTL_O_RECHARGESTAT                                    0x00000020

// Oscillator Configuration
#define AON_PMCTL_O_OSCCFG                                          0x00000024

// Reset Management
#define AON_PMCTL_O_RESETCTL                                        0x00000028

// Sleep Control
#define AON_PMCTL_O_SLEEPCTL                                        0x0000002C

// JTAG Configuration
#define AON_PMCTL_O_JTAGCFG                                         0x00000034

// JTAG USERCODE
#define AON_PMCTL_O_JTAGUSERCODE                                    0x0000003C

//*****************************************************************************
//
// Register: AON_PMCTL_O_AUXSCECLK
//
//*****************************************************************************
// Field:     [8] PD_SRC
//
// Selects the clock source for the AUX domain when AUX is in powerdown mode.
// Note: Switching the clock source is guaranteed to be glitch-free
// ENUMs:
// SCLK_LF                  LF clock (SCLK_LF )
// NO_CLOCK                 No clock
#define AON_PMCTL_AUXSCECLK_PD_SRC                                  0x00000100
#define AON_PMCTL_AUXSCECLK_PD_SRC_BITN                                      8
#define AON_PMCTL_AUXSCECLK_PD_SRC_M                                0x00000100
#define AON_PMCTL_AUXSCECLK_PD_SRC_S                                         8
#define AON_PMCTL_AUXSCECLK_PD_SRC_SCLK_LF                          0x00000100
#define AON_PMCTL_AUXSCECLK_PD_SRC_NO_CLOCK                         0x00000000

// Field:     [0] SRC
//
// Selects the clock source for the AUX domain when AUX is in active mode.
// Note: Switching the clock source is guaranteed to be glitch-free
// ENUMs:
// SCLK_MF                  MF Clock (SCLK_MF)
// SCLK_HFDIV2              HF Clock divided by 2 (SCLK_HFDIV2)
#define AON_PMCTL_AUXSCECLK_SRC                                     0x00000001
#define AON_PMCTL_AUXSCECLK_SRC_BITN                                         0
#define AON_PMCTL_AUXSCECLK_SRC_M                                   0x00000001
#define AON_PMCTL_AUXSCECLK_SRC_S                                            0
#define AON_PMCTL_AUXSCECLK_SRC_SCLK_MF                             0x00000001
#define AON_PMCTL_AUXSCECLK_SRC_SCLK_HFDIV2                         0x00000000

//*****************************************************************************
//
// Register: AON_PMCTL_O_RAMCFG
//
//*****************************************************************************
// Field:    [17] AUX_SRAM_PWR_OFF
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RAMCFG_AUX_SRAM_PWR_OFF                           0x00020000
#define AON_PMCTL_RAMCFG_AUX_SRAM_PWR_OFF_BITN                              17
#define AON_PMCTL_RAMCFG_AUX_SRAM_PWR_OFF_M                         0x00020000
#define AON_PMCTL_RAMCFG_AUX_SRAM_PWR_OFF_S                                 17

// Field:    [16] AUX_SRAM_RET_EN
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RAMCFG_AUX_SRAM_RET_EN                            0x00010000
#define AON_PMCTL_RAMCFG_AUX_SRAM_RET_EN_BITN                               16
#define AON_PMCTL_RAMCFG_AUX_SRAM_RET_EN_M                          0x00010000
#define AON_PMCTL_RAMCFG_AUX_SRAM_RET_EN_S                                  16

// Field:   [3:0] BUS_SRAM_RET_EN
//
// MCU SRAM is partitioned into 5  banks . This register controls which of the
// banks that has retention during MCU Bus domain power off
// ENUMs:
// RET_FULL                 Retention on for all banks SRAM:BANK0, SRAM:BANK1
//                          ,SRAM:BANK2,  SRAM:BANK3  and SRAM:BANK4
// RET_LEVEL3               Retention on for SRAM:BANK0, SRAM:BANK1
//                          ,SRAM:BANK2 and SRAM:BANK3
// RET_LEVEL2               Retention on for SRAM:BANK0, SRAM:BANK1 and
//                          SRAM:BANK2
// RET_LEVEL1               Retention on for SRAM:BANK0 and  SRAM:BANK1
// RET_NONE                 Retention is disabled
#define AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_W                                   4
#define AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_M                          0x0000000F
#define AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_S                                   0
#define AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_FULL                   0x0000000F
#define AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_LEVEL3                 0x00000007
#define AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_LEVEL2                 0x00000003
#define AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_LEVEL1                 0x00000001
#define AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_NONE                   0x00000000

//*****************************************************************************
//
// Register: AON_PMCTL_O_PWRCTL
//
//*****************************************************************************
// Field:     [2] DCDC_ACTIVE
//
// Select to use DCDC regulator for VDDR in active mode
//
// 0: Use GLDO for regulation of VDDR in active mode.
// 1: Use DCDC for regulation of VDDR in active mode.
//
// DCDC_EN must also be set for DCDC to be used as regulator for VDDR in active
// mode
#define AON_PMCTL_PWRCTL_DCDC_ACTIVE                                0x00000004
#define AON_PMCTL_PWRCTL_DCDC_ACTIVE_BITN                                    2
#define AON_PMCTL_PWRCTL_DCDC_ACTIVE_M                              0x00000004
#define AON_PMCTL_PWRCTL_DCDC_ACTIVE_S                                       2

// Field:     [1] EXT_REG_MODE
//
// Status of source for VDDRsupply:
//
// 0: DCDC or GLDO are generating VDDR
// 1: DCDC and GLDO are bypassed and an external regulator supplies VDDR
#define AON_PMCTL_PWRCTL_EXT_REG_MODE                               0x00000002
#define AON_PMCTL_PWRCTL_EXT_REG_MODE_BITN                                   1
#define AON_PMCTL_PWRCTL_EXT_REG_MODE_M                             0x00000002
#define AON_PMCTL_PWRCTL_EXT_REG_MODE_S                                      1

// Field:     [0] DCDC_EN
//
// Select to use DCDC regulator during recharge of VDDR
//
// 0: Use GLDO for recharge of VDDR
// 1: Use DCDC for recharge of VDDR
//
// Note: This bitfield should be set to the same as DCDC_ACTIVE
#define AON_PMCTL_PWRCTL_DCDC_EN                                    0x00000001
#define AON_PMCTL_PWRCTL_DCDC_EN_BITN                                        0
#define AON_PMCTL_PWRCTL_DCDC_EN_M                                  0x00000001
#define AON_PMCTL_PWRCTL_DCDC_EN_S                                           0

//*****************************************************************************
//
// Register: AON_PMCTL_O_PWRSTAT
//
//*****************************************************************************
// Field:     [2] JTAG_PD_ON
//
// Indicates JTAG power state:
//
// 0: JTAG is powered off
// 1: JTAG is powered on
#define AON_PMCTL_PWRSTAT_JTAG_PD_ON                                0x00000004
#define AON_PMCTL_PWRSTAT_JTAG_PD_ON_BITN                                    2
#define AON_PMCTL_PWRSTAT_JTAG_PD_ON_M                              0x00000004
#define AON_PMCTL_PWRSTAT_JTAG_PD_ON_S                                       2

// Field:     [1] AUX_BUS_RESET_DONE
//
// Indicates Reset Done from AUX Bus:
//
// 0: AUX Bus is being reset
// 1: AUX Bus reset is released
#define AON_PMCTL_PWRSTAT_AUX_BUS_RESET_DONE                        0x00000002
#define AON_PMCTL_PWRSTAT_AUX_BUS_RESET_DONE_BITN                            1
#define AON_PMCTL_PWRSTAT_AUX_BUS_RESET_DONE_M                      0x00000002
#define AON_PMCTL_PWRSTAT_AUX_BUS_RESET_DONE_S                               1

// Field:     [0] AUX_RESET_DONE
//
// Indicates Reset Done from AUX:
//
// 0: AUX is being reset
// 1: AUX reset is released
#define AON_PMCTL_PWRSTAT_AUX_RESET_DONE                            0x00000001
#define AON_PMCTL_PWRSTAT_AUX_RESET_DONE_BITN                                0
#define AON_PMCTL_PWRSTAT_AUX_RESET_DONE_M                          0x00000001
#define AON_PMCTL_PWRSTAT_AUX_RESET_DONE_S                                   0

//*****************************************************************************
//
// Register: AON_PMCTL_O_SHUTDOWN
//
//*****************************************************************************
// Field:     [0] EN
//
// Shutdown control.
//
// 0: Do not write 0 to this bit.
// 1: Immediately start the process to enter shutdown mode
#define AON_PMCTL_SHUTDOWN_EN                                       0x00000001
#define AON_PMCTL_SHUTDOWN_EN_BITN                                           0
#define AON_PMCTL_SHUTDOWN_EN_M                                     0x00000001
#define AON_PMCTL_SHUTDOWN_EN_S                                              0

//*****************************************************************************
//
// Register: AON_PMCTL_O_RECHARGECFG
//
//*****************************************************************************
// Field: [31:30] MODE
//
// Selects recharge algorithm for VDDR when the system is running on the uLDO
// ENUMs:
// COMPARATOR               External recharge comparator.
//                          Note that the clock to
//                          the recharge comparator must be enabled,
//
// [ANATOP_MMAP:ADI_3_REFSYS:CTL_RECHARGE_CMP0:COMP_CLK_DISABLE],
//                          before selecting  this recharge algorithm.
// ADAPTIVE                 Adaptive timer
// STATIC                   Static timer
// OFF                      Recharge disabled
#define AON_PMCTL_RECHARGECFG_MODE_W                                         2
#define AON_PMCTL_RECHARGECFG_MODE_M                                0xC0000000
#define AON_PMCTL_RECHARGECFG_MODE_S                                        30
#define AON_PMCTL_RECHARGECFG_MODE_COMPARATOR                       0xC0000000
#define AON_PMCTL_RECHARGECFG_MODE_ADAPTIVE                         0x80000000
#define AON_PMCTL_RECHARGECFG_MODE_STATIC                           0x40000000
#define AON_PMCTL_RECHARGECFG_MODE_OFF                              0x00000000

// Field: [23:20] C2
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RECHARGECFG_C2_W                                           4
#define AON_PMCTL_RECHARGECFG_C2_M                                  0x00F00000
#define AON_PMCTL_RECHARGECFG_C2_S                                          20

// Field: [19:16] C1
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RECHARGECFG_C1_W                                           4
#define AON_PMCTL_RECHARGECFG_C1_M                                  0x000F0000
#define AON_PMCTL_RECHARGECFG_C1_S                                          16

// Field: [15:11] MAX_PER_M
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RECHARGECFG_MAX_PER_M_W                                    5
#define AON_PMCTL_RECHARGECFG_MAX_PER_M_M                           0x0000F800
#define AON_PMCTL_RECHARGECFG_MAX_PER_M_S                                   11

// Field:  [10:8] MAX_PER_E
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RECHARGECFG_MAX_PER_E_W                                    3
#define AON_PMCTL_RECHARGECFG_MAX_PER_E_M                           0x00000700
#define AON_PMCTL_RECHARGECFG_MAX_PER_E_S                                    8

// Field:   [7:3] PER_M
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RECHARGECFG_PER_M_W                                        5
#define AON_PMCTL_RECHARGECFG_PER_M_M                               0x000000F8
#define AON_PMCTL_RECHARGECFG_PER_M_S                                        3

// Field:   [2:0] PER_E
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RECHARGECFG_PER_E_W                                        3
#define AON_PMCTL_RECHARGECFG_PER_E_M                               0x00000007
#define AON_PMCTL_RECHARGECFG_PER_E_S                                        0

//*****************************************************************************
//
// Register: AON_PMCTL_O_RECHARGESTAT
//
//*****************************************************************************
// Field: [19:16] VDDR_SMPLS
//
// The last 4 VDDR samples.
//
// For each bit:
// 0: VDDR was below VDDR_OK threshold when recharge started
// 1: VDDR was above VDDR_OK threshold when recharge started
//
// The register is updated prior to every recharge period with a shift left,
// and bit 0 is updated with the last VDDR sample.
#define AON_PMCTL_RECHARGESTAT_VDDR_SMPLS_W                                  4
#define AON_PMCTL_RECHARGESTAT_VDDR_SMPLS_M                         0x000F0000
#define AON_PMCTL_RECHARGESTAT_VDDR_SMPLS_S                                 16

// Field:  [15:0] MAX_USED_PER
//
// Shows the maximum number of 32kHz periods that have separated two recharge
// cycles and VDDR still was above VDDR_OK threshold when the latter recharge
// started. This register can be used as an indication of the leakage current
// during standby.
//
// This bitfield is cleared to 0 when writing this register.
#define AON_PMCTL_RECHARGESTAT_MAX_USED_PER_W                               16
#define AON_PMCTL_RECHARGESTAT_MAX_USED_PER_M                       0x0000FFFF
#define AON_PMCTL_RECHARGESTAT_MAX_USED_PER_S                                0

//*****************************************************************************
//
// Register: AON_PMCTL_O_OSCCFG
//
//*****************************************************************************
// Field:   [7:3] PER_M
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_OSCCFG_PER_M_W                                             5
#define AON_PMCTL_OSCCFG_PER_M_M                                    0x000000F8
#define AON_PMCTL_OSCCFG_PER_M_S                                             3

// Field:   [2:0] PER_E
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_OSCCFG_PER_E_W                                             3
#define AON_PMCTL_OSCCFG_PER_E_M                                    0x00000007
#define AON_PMCTL_OSCCFG_PER_E_S                                             0

//*****************************************************************************
//
// Register: AON_PMCTL_O_RESETCTL
//
//*****************************************************************************
// Field:    [31] SYSRESET
//
// Cold reset register. Writing 1 to this bitfield will reset the entire chip
// and cause boot code to run again.
//
// 0: No effect
// 1: Generate system reset. Appears as SYSRESET in RESET_SRC
#define AON_PMCTL_RESETCTL_SYSRESET                                 0x80000000
#define AON_PMCTL_RESETCTL_SYSRESET_BITN                                    31
#define AON_PMCTL_RESETCTL_SYSRESET_M                               0x80000000
#define AON_PMCTL_RESETCTL_SYSRESET_S                                       31

// Field:    [25] BOOT_DET_1_CLR
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RESETCTL_BOOT_DET_1_CLR                           0x02000000
#define AON_PMCTL_RESETCTL_BOOT_DET_1_CLR_BITN                              25
#define AON_PMCTL_RESETCTL_BOOT_DET_1_CLR_M                         0x02000000
#define AON_PMCTL_RESETCTL_BOOT_DET_1_CLR_S                                 25

// Field:    [24] BOOT_DET_0_CLR
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RESETCTL_BOOT_DET_0_CLR                           0x01000000
#define AON_PMCTL_RESETCTL_BOOT_DET_0_CLR_BITN                              24
#define AON_PMCTL_RESETCTL_BOOT_DET_0_CLR_M                         0x01000000
#define AON_PMCTL_RESETCTL_BOOT_DET_0_CLR_S                                 24

// Field:    [17] BOOT_DET_1_SET
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RESETCTL_BOOT_DET_1_SET                           0x00020000
#define AON_PMCTL_RESETCTL_BOOT_DET_1_SET_BITN                              17
#define AON_PMCTL_RESETCTL_BOOT_DET_1_SET_M                         0x00020000
#define AON_PMCTL_RESETCTL_BOOT_DET_1_SET_S                                 17

// Field:    [16] BOOT_DET_0_SET
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RESETCTL_BOOT_DET_0_SET                           0x00010000
#define AON_PMCTL_RESETCTL_BOOT_DET_0_SET_BITN                              16
#define AON_PMCTL_RESETCTL_BOOT_DET_0_SET_M                         0x00010000
#define AON_PMCTL_RESETCTL_BOOT_DET_0_SET_S                                 16

// Field:    [15] WU_FROM_SD
//
// A Wakeup from SHUTDOWN on an IO event has occurred, or a wakeup from
// SHUTDOWN has occurred as a result of the debugger being attached.. (TCK pin
// being forced low)
//
// Please refer to IOC:IOCFGn.WU_CFG for configuring the IO's as wakeup
// sources.
//
// 0: Wakeup occurred from cold reset or brown out as seen in RESET_SRC
// 1: A wakeup has occurred from SHUTDOWN
//
// Note: This flag will be cleared when SLEEPCTL.IO_PAD_SLEEP_DIS is asserted.
#define AON_PMCTL_RESETCTL_WU_FROM_SD                               0x00008000
#define AON_PMCTL_RESETCTL_WU_FROM_SD_BITN                                  15
#define AON_PMCTL_RESETCTL_WU_FROM_SD_M                             0x00008000
#define AON_PMCTL_RESETCTL_WU_FROM_SD_S                                     15

// Field:    [14] GPIO_WU_FROM_SD
//
// A wakeup from SHUTDOWN on an IO event has occurred
//
// Please refer to IOC:IOCFGn.WU_CFG for configuring the IO's as wakeup
// sources.
//
// 0: The wakeup did not occur from SHUTDOWN on an IO event
// 1: A wakeup from SHUTDOWN occurred from an IO event
//
// The case where WU_FROM_SD is asserted but this bitfield is not asserted will
// only occur in a debug session. The boot code will not proceed with wakeup
// from SHUTDOWN procedure until this bitfield is asserted as well.
//
// Note: This flag will be cleared when  SLEEPCTL.IO_PAD_SLEEP_DIS is asserted.
#define AON_PMCTL_RESETCTL_GPIO_WU_FROM_SD                          0x00004000
#define AON_PMCTL_RESETCTL_GPIO_WU_FROM_SD_BITN                             14
#define AON_PMCTL_RESETCTL_GPIO_WU_FROM_SD_M                        0x00004000
#define AON_PMCTL_RESETCTL_GPIO_WU_FROM_SD_S                                14

// Field:    [13] BOOT_DET_1
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RESETCTL_BOOT_DET_1                               0x00002000
#define AON_PMCTL_RESETCTL_BOOT_DET_1_BITN                                  13
#define AON_PMCTL_RESETCTL_BOOT_DET_1_M                             0x00002000
#define AON_PMCTL_RESETCTL_BOOT_DET_1_S                                     13

// Field:    [12] BOOT_DET_0
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RESETCTL_BOOT_DET_0                               0x00001000
#define AON_PMCTL_RESETCTL_BOOT_DET_0_BITN                                  12
#define AON_PMCTL_RESETCTL_BOOT_DET_0_M                             0x00001000
#define AON_PMCTL_RESETCTL_BOOT_DET_0_S                                     12

// Field:     [8] VDDS_LOSS_EN
//
// Controls reset generation in case VDDS is lost
//
// 0: Brown out detect of VDDS is ignored, unless VDDS_LOSS_EN_OVR=1
// 1: Brown out detect of VDDS generates system reset
#define AON_PMCTL_RESETCTL_VDDS_LOSS_EN                             0x00000100
#define AON_PMCTL_RESETCTL_VDDS_LOSS_EN_BITN                                 8
#define AON_PMCTL_RESETCTL_VDDS_LOSS_EN_M                           0x00000100
#define AON_PMCTL_RESETCTL_VDDS_LOSS_EN_S                                    8

// Field:     [7] VDDR_LOSS_EN
//
// Controls reset generation in case VDDR is lost
//
// 0: Brown out detect of VDDR is ignored, unless VDDR_LOSS_EN_OVR=1
// 1: Brown out detect of VDDR generates system reset
#define AON_PMCTL_RESETCTL_VDDR_LOSS_EN                             0x00000080
#define AON_PMCTL_RESETCTL_VDDR_LOSS_EN_BITN                                 7
#define AON_PMCTL_RESETCTL_VDDR_LOSS_EN_M                           0x00000080
#define AON_PMCTL_RESETCTL_VDDR_LOSS_EN_S                                    7

// Field:     [6] VDD_LOSS_EN
//
// Controls reset generation in case VDD is lost
//
// 0: Brown out detect of VDD is ignored, unless VDD_LOSS_EN_OVR=1
// 1: Brown out detect of VDD generates system reset
#define AON_PMCTL_RESETCTL_VDD_LOSS_EN                              0x00000040
#define AON_PMCTL_RESETCTL_VDD_LOSS_EN_BITN                                  6
#define AON_PMCTL_RESETCTL_VDD_LOSS_EN_M                            0x00000040
#define AON_PMCTL_RESETCTL_VDD_LOSS_EN_S                                     6

// Field:     [5] CLK_LOSS_EN
//
// Controls reset generation in case SCLK_LF, SCLK_MF or SCLK_HF is lost when
// clock loss detection is enabled by [ANATOP_MMAP:DDI_0_OSC:CTL0.CLK_LOSS_EN]
//
// 0: Clock loss is ignored
// 1: Clock loss generates system reset
//
// Note: Clock loss reset generation must be disabled when changing clock
// source for   SCLK_LF. Failure to do so may result in a spurious system
// reset. Clock loss reset generation is controlled by
// [ANATOP_MMAP:DDI_0_OSC:CTL0.CLK_LOSS_EN]
#define AON_PMCTL_RESETCTL_CLK_LOSS_EN                              0x00000020
#define AON_PMCTL_RESETCTL_CLK_LOSS_EN_BITN                                  5
#define AON_PMCTL_RESETCTL_CLK_LOSS_EN_M                            0x00000020
#define AON_PMCTL_RESETCTL_CLK_LOSS_EN_S                                     5

// Field:     [4] MCU_WARM_RESET
//
// Internal. Only to be used through TI provided API.
#define AON_PMCTL_RESETCTL_MCU_WARM_RESET                           0x00000010
#define AON_PMCTL_RESETCTL_MCU_WARM_RESET_BITN                               4
#define AON_PMCTL_RESETCTL_MCU_WARM_RESET_M                         0x00000010
#define AON_PMCTL_RESETCTL_MCU_WARM_RESET_S                                  4

// Field:   [3:1] RESET_SRC
//
// Shows the root cause of the last system reset. More than the reported reset
// source can have been active during the last system reset but only the root
// cause is reported.
//
// The capture feature is not rearmed until all off the possible reset sources
// have been released and the result has been copied to AON_PMCTL. During the
// copy and rearm process it is one 2MHz period in which and eventual new
// system reset will be reported as Power on reset regardless of the root
// cause.
// ENUMs:
// WARMRESET                Software reset via PRCM warm reset request
// SYSRESET                 Software reset via SYSRESET or hardware power
//                          management timeout detection.
//
//                          Note: The hardware power
//                          management timeout circuit is always enabled.
// CLK_LOSS                 SCLK_LF, SCLK_MF or SCLK_HF clock loss detect
// VDDR_LOSS                Brown out detect on VDDR
// VDDS_LOSS                Brown out detect on VDDS
// PIN_RESET                Reset pin
// PWR_ON                   Power on reset
#define AON_PMCTL_RESETCTL_RESET_SRC_W                                       3
#define AON_PMCTL_RESETCTL_RESET_SRC_M                              0x0000000E
#define AON_PMCTL_RESETCTL_RESET_SRC_S                                       1
#define AON_PMCTL_RESETCTL_RESET_SRC_WARMRESET                      0x0000000E
#define AON_PMCTL_RESETCTL_RESET_SRC_SYSRESET                       0x0000000C
#define AON_PMCTL_RESETCTL_RESET_SRC_CLK_LOSS                       0x0000000A
#define AON_PMCTL_RESETCTL_RESET_SRC_VDDR_LOSS                      0x00000008
#define AON_PMCTL_RESETCTL_RESET_SRC_VDDS_LOSS                      0x00000004
#define AON_PMCTL_RESETCTL_RESET_SRC_PIN_RESET                      0x00000002
#define AON_PMCTL_RESETCTL_RESET_SRC_PWR_ON                         0x00000000

//*****************************************************************************
//
// Register: AON_PMCTL_O_SLEEPCTL
//
//*****************************************************************************
// Field:     [0] IO_PAD_SLEEP_DIS
//
// Controls the I/O pad sleep mode. The boot code will set this bitfield
// automatically unless waking up from a SHUTDOWN ( RESETCTL.WU_FROM_SD is
// set).
//
// 0: I/O pad sleep mode is enabled, meaning all outputs and pad configurations
// are latched. Inputs are transparent if pad is configured as input before
// IO_PAD_SLEEP_DIS is set to 1
// 1: I/O pad sleep mode is disabled
//
// Application software must reconfigure the state for all IO's before setting
// this bitfield upon waking up from a SHUTDOWN to avoid glitches on pins.
#define AON_PMCTL_SLEEPCTL_IO_PAD_SLEEP_DIS                         0x00000001
#define AON_PMCTL_SLEEPCTL_IO_PAD_SLEEP_DIS_BITN                             0
#define AON_PMCTL_SLEEPCTL_IO_PAD_SLEEP_DIS_M                       0x00000001
#define AON_PMCTL_SLEEPCTL_IO_PAD_SLEEP_DIS_S                                0

//*****************************************************************************
//
// Register: AON_PMCTL_O_JTAGCFG
//
//*****************************************************************************
// Field:     [8] JTAG_PD_FORCE_ON
//
// Controls JTAG Power domain power state:
//
// 0: Controlled exclusively by debug subsystem. (JTAG Power domain will be
// powered off unless a debugger is attached)
// 1: JTAG Power Domain is forced on, independent of debug subsystem.
//
// Note: The reset value causes JTAG Power domain to be powered on by default.
// Software must clear this bit to turn off the JTAG Power domain
#define AON_PMCTL_JTAGCFG_JTAG_PD_FORCE_ON                          0x00000100
#define AON_PMCTL_JTAGCFG_JTAG_PD_FORCE_ON_BITN                              8
#define AON_PMCTL_JTAGCFG_JTAG_PD_FORCE_ON_M                        0x00000100
#define AON_PMCTL_JTAGCFG_JTAG_PD_FORCE_ON_S                                 8

//*****************************************************************************
//
// Register: AON_PMCTL_O_JTAGUSERCODE
//
//*****************************************************************************
// Field:  [31:0] USER_CODE
//
// 32-bit JTAG USERCODE register feeding main JTAG TAP
// Note: This field can be locked by LOCKCFG.LOCK
#define AON_PMCTL_JTAGUSERCODE_USER_CODE_W                                  32
#define AON_PMCTL_JTAGUSERCODE_USER_CODE_M                          0xFFFFFFFF
#define AON_PMCTL_JTAGUSERCODE_USER_CODE_S                                   0


#endif // __AON_PMCTL__
