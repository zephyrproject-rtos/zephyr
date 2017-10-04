/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#include <ti/devices/msp432p4xx/driverlib/reset.h>
#include <ti/devices/msp432p4xx/driverlib/debug.h>

void ResetCtl_initiateSoftReset(void)
{
    RSTCTL->RESET_REQ |= (RESET_KEY | RESET_SOFT_RESET);
}

void ResetCtl_initiateSoftResetWithSource(uint32_t source)
{
    RSTCTL->SOFTRESET_SET |= (source);
}

uint32_t ResetCtl_getSoftResetSource(void)
{
    return RSTCTL->SOFTRESET_STAT;
}

void ResetCtl_clearSoftResetSource(uint32_t mask)
{
    RSTCTL->SOFTRESET_CLR |= mask;
}

void ResetCtl_initiateHardReset(void)
{
    RSTCTL->RESET_REQ |= (RESET_KEY | RESET_HARD_RESET);
}

void ResetCtl_initiateHardResetWithSource(uint32_t source)
{
    RSTCTL->HARDRESET_SET |= (source);
}

uint32_t ResetCtl_getHardResetSource(void)
{
    return RSTCTL->HARDRESET_STAT;
}

void ResetCtl_clearHardResetSource(uint32_t mask)
{
    RSTCTL->HARDRESET_CLR |= mask;
}

uint32_t ResetCtl_getPSSSource(void)
{
    return RSTCTL->PSSRESET_STAT;
}

void ResetCtl_clearPSSFlags(void)
{
    RSTCTL->PSSRESET_CLR |= RSTCTL_PSSRESET_CLR_CLR;
}

uint32_t ResetCtl_getPCMSource(void)
{
    return RSTCTL->PCMRESET_STAT;
}

void ResetCtl_clearPCMFlags(void)
{
    RSTCTL->PCMRESET_CLR |= RSTCTL_PCMRESET_CLR_CLR;
}

