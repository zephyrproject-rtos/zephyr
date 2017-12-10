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

#include "wdog_imx.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : WDOG_Enable
 * Description   : Configure WDOG funtions, call once only
 *
 *END**************************************************************************/
void WDOG_Enable(WDOG_Type *base, uint8_t timeout)
{
    uint16_t wcr = base->WCR & (~WDOG_WCR_WT_MASK);
    base->WCR = wcr | WDOG_WCR_WT(timeout) | WDOG_WCR_WDE_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : WDOG_Reset
 * Description   : Assert WDOG reset signal
 *
 *END**************************************************************************/
void WDOG_Reset(WDOG_Type *base, bool wda, bool srs)
{
    uint16_t wcr = base->WCR;

    if (wda)
        wcr &= ~WDOG_WCR_WDA_MASK;
    if (srs)
        wcr &= ~WDOG_WCR_SRS_MASK;

    base->WCR = wcr;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : WDOG_Refresh
 * Description   : Refresh the WDOG to prevent timeout
 *
 *END**************************************************************************/
void WDOG_Refresh(WDOG_Type *base)
{
    base->WSR = 0x5555;
    base->WSR = 0xAAAA;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
