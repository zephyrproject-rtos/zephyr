/*
 * The Clear BSD License
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 *  that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
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

#include "fsl_rtwdog.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

void RTWDOG_ClearStatusFlags(RTWDOG_Type *base, uint32_t mask)
{
    if (mask & kRTWDOG_InterruptFlag)
    {
        base->CS |= RTWDOG_CS_FLG_MASK;
    }
}

void RTWDOG_GetDefaultConfig(rtwdog_config_t *config)
{
    assert(config);

    config->enableRtwdog = true;
    config->clockSource = kRTWDOG_ClockSource1;
    config->prescaler = kRTWDOG_ClockPrescalerDivide1;
    config->workMode.enableWait = true;
    config->workMode.enableStop = false;
    config->workMode.enableDebug = false;
    config->testMode = kRTWDOG_TestModeDisabled;
    config->enableUpdate = true;
    config->enableInterrupt = false;
    config->enableWindowMode = false;
    config->windowValue = 0U;
    config->timeoutValue = 0xFFFFU;
}

void RTWDOG_Init(RTWDOG_Type *base, const rtwdog_config_t *config)
{
    assert(config);

    uint32_t value = 0U;
    uint32_t primaskValue = 0U;
 
    value = RTWDOG_CS_EN(config->enableRtwdog) | RTWDOG_CS_CLK(config->clockSource) | RTWDOG_CS_INT(config->enableInterrupt) |
            RTWDOG_CS_WIN(config->enableWindowMode) | RTWDOG_CS_UPDATE(config->enableUpdate) |
            RTWDOG_CS_DBG(config->workMode.enableDebug) | RTWDOG_CS_STOP(config->workMode.enableStop) |
            RTWDOG_CS_WAIT(config->workMode.enableWait) | RTWDOG_CS_PRES(config->prescaler) | RTWDOG_CS_CMD32EN(true) |
            RTWDOG_CS_TST(config->testMode);

    /* Disable the global interrupts. Otherwise, an interrupt could effectively invalidate the unlock sequence
     * and the WCT may expire. After the configuration finishes, re-enable the global interrupts. */
    primaskValue = DisableGlobalIRQ();
    RTWDOG_Unlock(base);
    base->WIN = config->windowValue;
    base->TOVAL = config->timeoutValue;
    base->CS = value;
    EnableGlobalIRQ(primaskValue);
}

void RTWDOG_Deinit(RTWDOG_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupts */
    primaskValue = DisableGlobalIRQ();
    RTWDOG_Unlock(base);
    RTWDOG_Disable(base);
    EnableGlobalIRQ(primaskValue);
}
