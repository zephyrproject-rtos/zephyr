/******************************************************************************
*  Filename:       pwr_ctrl.c
*  Revised:        2017-06-05 12:13:49 +0200 (Mon, 05 Jun 2017)
*  Revision:       49096
*
*  Description:    Power Control driver.
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

#include "pwr_ctrl.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  PowerCtrlSourceSet
    #define PowerCtrlSourceSet              NOROM_PowerCtrlSourceSet
#endif


//*****************************************************************************
//
// Set (Request) the main power source
//
//*****************************************************************************
void
PowerCtrlSourceSet(uint32_t ui32PowerConfig)
{
    // Check the arguments.
    ASSERT((ui32PowerConfig == PWRCTRL_PWRSRC_DCDC) ||
           (ui32PowerConfig == PWRCTRL_PWRSRC_GLDO) ||
           (ui32PowerConfig == PWRCTRL_PWRSRC_ULDO));

    // Configure the power.
    if(ui32PowerConfig == PWRCTRL_PWRSRC_DCDC) {
        HWREG(AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL) |=
            (AON_PMCTL_PWRCTL_DCDC_EN | AON_PMCTL_PWRCTL_DCDC_ACTIVE);
    }
    else if (ui32PowerConfig == PWRCTRL_PWRSRC_GLDO)
    {
        HWREG(AON_PMCTL_BASE + AON_PMCTL_O_PWRCTL) &=
            ~(AON_PMCTL_PWRCTL_DCDC_EN | AON_PMCTL_PWRCTL_DCDC_ACTIVE);
    }
    else
    {
        PRCMMcuUldoConfigure(true);
    }
}
