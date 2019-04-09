/******************************************************************************
*  Filename:       aon_pmctl.h
*  Revised:        2017-11-02 14:16:14 +0100 (Thu, 02 Nov 2017)
*  Revision:       50156
*
*  Description:    Defines and prototypes for the AON Power-Management Controller
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

//*****************************************************************************
//
//! \addtogroup aon_group
//! @{
//! \addtogroup aonpmctl_api
//! @{
//
//*****************************************************************************

#ifndef __AON_PMCTL_H__
#define __AON_PMCTL_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_aon_pmctl.h"
#include "debug.h"

//*****************************************************************************
//
// Defines that can be be used to enable/disable the retention on the SRAM
// banks during power off of the MCU BUS domain. The defines can be passed to
// AONPMCTLMcuSRamConfig) .
//
//*****************************************************************************
#define MCU_RAM_RET_NONE        AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_NONE
#define MCU_RAM_RET_LVL1        AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_LEVEL1
#define MCU_RAM_RET_LVL2        AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_LEVEL2
#define MCU_RAM_RET_LVL3        AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_LEVEL3
#define MCU_RAM_RET_FULL        AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_RET_FULL

//*****************************************************************************
//
// Defines for all the different power modes available through
// AONPMCTLPowerStatusGet() .
//
//*****************************************************************************
#define AONPMCTL_JTAG_POWER_ON  AON_PMCTL_PWRSTAT_JTAG_PD_ON

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Configure the retention on the block SRAM in the MCU BUS domain.
//!
//! MCU SRAM is partitioned into 5 banks of 16 KB each. The SRAM supports
//! retention on all 5 banks during MCU BUS domain power off. The retention
//! on the SRAM can be turned on and off. Use this function to enable the
//! retention on the banks.
//!
//! If a group of banks is not represented in the parameter \c ui32Retention
//! then the retention will be disabled for that bank group during MCU BUS
//! domain power off.
//!
//! \note Retention on all SRAM banks is enabled by default. Configuration of
//! individual SRAM banks is not supported. Configuration is only supported
//! on bank group level.
//!
//! \param ui32Retention defines which groups of SRAM banks to enable/disable
//! retention on:
//! - \ref MCU_RAM_RET_NONE   Retention is disabled
//! - \ref MCU_RAM_RET_LVL1   Retention on for banks 0 and 1
//! - \ref MCU_RAM_RET_LVL2   Retention on for banks 0, 1 and 2
//! - \ref MCU_RAM_RET_LVL3   Retention on for banks 0, 1, 2 and 3
//! - \ref MCU_RAM_RET_FULL   Retention on for all five banks
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
AONPMCTLMcuSRamRetConfig(uint32_t ui32Retention)
{
    uint32_t ui32Reg;

    // Check the arguments.
    ASSERT((ui32Retention == MCU_RAM_RET_NONE) ||
           (ui32Retention == MCU_RAM_RET_LVL1) ||
           (ui32Retention == MCU_RAM_RET_LVL2) ||
           (ui32Retention == MCU_RAM_RET_LVL3) ||
           (ui32Retention == MCU_RAM_RET_FULL));

    // Configure the retention.
    ui32Reg = HWREG(AON_PMCTL_BASE + AON_PMCTL_O_RAMCFG) & ~AON_PMCTL_RAMCFG_BUS_SRAM_RET_EN_M;
    ui32Reg |= ui32Retention;
    HWREG(AON_PMCTL_BASE + AON_PMCTL_O_RAMCFG) = ui32Reg;
}

//*****************************************************************************
//
//! \brief Get the power status of the Always On (AON) domain.
//!
//! This function reports the power management status in AON.
//!
//! \return Returns the current power status of the device as a bitwise OR'ed
//! combination of these values:
//! - \ref AONPMCTL_JTAG_POWER_ON
//
//*****************************************************************************
__STATIC_INLINE uint32_t
AONPMCTLPowerStatusGet(void)
{
    // Return the power status.
    return (HWREG(AON_PMCTL_BASE + AON_PMCTL_O_PWRSTAT));
}


//*****************************************************************************
//
//! \brief Request power off of the JTAG domain.
//!
//! The JTAG domain is automatically powered up on if a debugger is connected.
//! If a debugger is not connected this function can be used to power off the
//! JTAG domain.
//!
//! \note Achieving the lowest power modes (shutdown/powerdown) requires the
//! JTAG domain to be turned off. In general the JTAG domain should never be
//! powered in production code.
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
AONPMCTLJtagPowerOff(void)
{
    // Request the power off of the JTAG domain
    HWREG(AON_PMCTL_BASE + AON_PMCTL_O_JTAGCFG) = 0;
}


//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __AON_PMCTL_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
