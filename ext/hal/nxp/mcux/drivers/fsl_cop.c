/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
 * o Neither the name of the copyright holder nor the names of its
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

#include "fsl_cop.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

void COP_GetDefaultConfig(cop_config_t *config)
{
    assert(config);

    config->enableWindowMode = false;
#if defined(FSL_FEATURE_COP_HAS_LONGTIME_MODE) && FSL_FEATURE_COP_HAS_LONGTIME_MODE
    config->timeoutMode = kCOP_LongTimeoutMode;
    config->enableStop = false;
    config->enableDebug = false;
#endif /* FSL_FEATURE_COP_HAS_LONGTIME_MODE */
    config->clockSource = kCOP_LpoClock;
    config->timeoutCycles = kCOP_2Power10CyclesOr2Power18Cycles;
}

void COP_Init(SIM_Type *base, const cop_config_t *config)
{
    assert(config);

    uint32_t value = 0U;

#if defined(FSL_FEATURE_COP_HAS_LONGTIME_MODE) && FSL_FEATURE_COP_HAS_LONGTIME_MODE
    value = SIM_COPC_COPW(config->enableWindowMode) | SIM_COPC_COPCLKS(config->timeoutMode) |
            SIM_COPC_COPT(config->timeoutCycles) | SIM_COPC_COPSTPEN(config->enableStop) |
            SIM_COPC_COPDBGEN(config->enableDebug) | SIM_COPC_COPCLKSEL(config->clockSource);
#else
    value = SIM_COPC_COPW(config->enableWindowMode) | SIM_COPC_COPCLKS(config->clockSource) |
            SIM_COPC_COPT(config->timeoutCycles);
#endif /* FSL_FEATURE_COP_HAS_LONGTIME_MODE */
    base->COPC = value;
}

void COP_Refresh(SIM_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupt to protect refresh sequence */
    primaskValue = DisableGlobalIRQ();
    base->SRVCOP = COP_FIRST_BYTE_OF_REFRESH;
    base->SRVCOP = COP_SECOND_BYTE_OF_REFRESH;
    EnableGlobalIRQ(primaskValue);
}
