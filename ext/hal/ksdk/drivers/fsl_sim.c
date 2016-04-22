/*
* Copyright (c) 2015, Freescale Semiconductor, Inc.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*
* o Redistributions of source code must retain the above copyright notice, this list
*   of conditions and the following disclaimer.
*
* o Redistributions in binary form must reproduce the above copyright notice, this
*   list of conditions and the following disclaimer in the documentation and/or
*   other materials provided with the distribution.
*
* o Neither the name of Freescale Semiconductor, Inc. nor the names of its
*   contributors may be used to endorse or promote products derived from this
*   software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "fsl_sim.h"

/*******************************************************************************
 * Codes
 ******************************************************************************/
#if (defined(FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR) && FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR)
void SIM_SetUsbVoltRegulatorEnableMode(uint32_t mask)
{
    SIM->SOPT1CFG |= (SIM_SOPT1CFG_URWE_MASK | SIM_SOPT1CFG_UVSWE_MASK | SIM_SOPT1CFG_USSWE_MASK);

    SIM->SOPT1 = (SIM->SOPT1 & ~kSIM_UsbVoltRegEnableInAllModes) | mask;
}
#endif /* FSL_FEATURE_SIM_OPT_HAS_USB_VOLTAGE_REGULATOR */

void SIM_GetUniqueId(sim_uid_t *uid)
{
#if defined(SIM_UIDH)
    uid->H = SIM->UIDH;
#endif
    uid->MH = SIM->UIDMH;
    uid->ML = SIM->UIDML;
    uid->L = SIM->UIDL;
}
