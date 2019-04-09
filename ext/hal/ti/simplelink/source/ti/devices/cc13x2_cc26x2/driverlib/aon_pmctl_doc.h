/******************************************************************************
*  Filename:       aon_pmctl_doc.h
*  Revised:        2017-11-02 15:41:14 +0100 (Thu, 02 Nov 2017)
*  Revision:       50165
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/
//! \addtogroup aonpmctl_api
//! @{
//! \section sec_aonpmctl Introduction
//!
//! This API provides a set of functions for using the AON Power Management
//! Controller module (AON_PMCTL).
//!
//! The AON_PMCTL module contains the following functional options:
//! - Selection of voltage regulator for the digital domain.
//! - Control of retention of MCU SRAM banks during power off of the BUS power domain.
//! - Control of power and retention of AUX SRAM.
//! - Control of power, reset, and clock for the following domains:
//!   - MCU_VD
//!   - JTAG_PD
//!   - AUX
//! - Control of the recharging of VDDR while in uLDO state.
//! - Control of the generation of a periodic request to the OSCDIG to initiate
//! an XOSC_HF amplitude calibration sequence.
//!
//! The main clock for the AON_PMCTL module is the 2 MHz SCLK MF clock.
//!
//! AON_PMCTL supports the MCU_voltage domain with a 48 MHz clock (SCLK_HF) that is divided
//! and gated by the PRCM module before being distributed to all modules in the
//! MCU voltage domain.
//!
//! The AON_PMCTL controls the SCLK_HF clock to ensure that it is available in the
//! Active and Idle power modes, and disabled for all other modes. SCLK_HF is not
//! allowed in uLDO state since it uses too much power.
//! The SCLK_HF clock is also available for the AUX module in the Active and Idle
//! power modes.
//!
//! The AON_PMCTL selects the clock source for the AUX domain in the different
//! power modes.
//!
//! Main functionality to control power management of the JTAG power domain is
//! supported. Note that no clock control is supported, as the JTAG is clocked
//! on the TCK clock.
//!
//!
//! \section sec_aonpmctl_api API
//!
//! The API functions can be grouped like this:
//!
//! Functions to perform status report:
//! - \ref AONPMCTLPowerStatusGet()
//!
//!
//! Functions to perform device configuration:
//! - \ref AONPMCTLJtagPowerOff()
//! - \ref AONPMCTLMcuSRamRetConfig()
//!
//! Please note that due to legacy software compatibility some functionalities controlled
//! by the AON Power Management Controller module are supported through the APIs of
//! the [System Controller](@ref sysctrl_api) and [Power Controller](@ref pwrctrl_api). Relevant functions are:
//! - \ref PowerCtrlSourceGet()
//! - \ref PowerCtrlSourceSet()
//! - \ref PowerCtrlResetSourceGet()
//! - \ref SysCtrl_DCDC_VoltageConditionalControl()
//! - \ref SysCtrlClockLossResetDisable()
//! - \ref SysCtrlClockLossResetEnable()
//! - \ref SysCtrlSystemReset()
//! - \ref SysCtrlResetSourceGet()
//!
//! @}
