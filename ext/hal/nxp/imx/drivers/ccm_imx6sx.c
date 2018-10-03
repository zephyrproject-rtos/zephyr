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

#include "ccm_imx6sx.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_SetClockEnableSignalOverrided
 * Description   : Override or do not override clock enable signal from module.
 *
 *END**************************************************************************/
void CCM_SetClockEnableSignalOverrided(CCM_Type * base, uint32_t signal, bool control)
{
    if(control)
        CCM_CMEOR_REG(base) |= signal;
    else
        CCM_CMEOR_REG(base) &= ~signal;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : CCM_SetMmdcHandshakeMask
 * Description   : Set handshake mask of MMDC module.
 *
 *END**************************************************************************/
void CCM_SetMmdcHandshakeMask(CCM_Type * base, bool mask)
{
    if(mask)
        CCM_CCDR_REG(base) |= CCM_CCDR_mmdc_mask_MASK;
    else
        CCM_CCDR_REG(base) &= ~CCM_CCDR_mmdc_mask_MASK;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
