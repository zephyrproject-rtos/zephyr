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

#include "rdc.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_SetMrAccess
 * Description   : Set RDC memory region access permission for RDC domains
 *
 *END**************************************************************************/
void RDC_SetMrAccess(RDC_Type * base, uint32_t mr, uint32_t startAddr, uint32_t endAddr,
                     uint8_t perm, bool enable, bool lock)
{
    base->MR[mr].MRSA = startAddr;
    base->MR[mr].MREA = endAddr;
    base->MR[mr].MRC = perm | (enable ? RDC_MRC_ENA_MASK : 0) | (lock ? RDC_MRC_LCK_MASK : 0);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_GetMrAccess
 * Description   : Get RDC memory region access permission for RDC domains
 *
 *END**************************************************************************/
uint8_t RDC_GetMrAccess(RDC_Type * base, uint32_t mr, uint32_t *startAddr, uint32_t *endAddr)
{
    if (startAddr)
        *startAddr = base->MR[mr].MRSA;
    if (endAddr)
        *endAddr = base->MR[mr].MREA;

    return base->MR[mr].MRC & 0xFF;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_GetViolationStatus
 * Description   : Get RDC memory violation status
 *
 *END**************************************************************************/
bool RDC_GetViolationStatus(RDC_Type * base, uint32_t mr, uint32_t *violationAddr, uint32_t *violationDomain)
{
    uint32_t mrvs;

    mrvs = base->MR[mr].MRVS;

    if (violationAddr)
        *violationAddr = mrvs & RDC_MRVS_VADR_MASK;
    if (violationDomain)
        *violationDomain = (mrvs & RDC_MRVS_VDID_MASK) >> RDC_MRVS_VDID_SHIFT;

    return (bool)(mrvs & RDC_MRVS_AD_MASK);
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
