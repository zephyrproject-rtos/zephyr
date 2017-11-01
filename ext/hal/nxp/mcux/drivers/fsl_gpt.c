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

#include "fsl_gpt.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief Pointers to GPT bases for each instance. */
static GPT_Type *const s_gptBases[] = GPT_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to GPT clocks for each instance. */
static const clock_ip_name_t s_gptClocks[] = GPT_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*******************************************************************************
 * Code
 ******************************************************************************/
static uint32_t GPT_GetInstance(GPT_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0U; instance < ARRAY_SIZE(s_gptBases); instance++)
    {
        if (s_gptBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_gptBases));

    return instance;
}

void GPT_Init(GPT_Type *base, const gpt_config_t *initConfig)
{
    assert(initConfig);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate the GPT clock*/
    CLOCK_EnableClock(s_gptClocks[GPT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    base->CR = 0U;

    GPT_SoftwareReset(base);

    base->CR =
        (initConfig->enableFreeRun ? GPT_CR_FRR_MASK : 0U) | (initConfig->enableRunInWait ? GPT_CR_WAITEN_MASK : 0U) |
        (initConfig->enableRunInStop ? GPT_CR_STOPEN_MASK : 0U) |
        (initConfig->enableRunInDoze ? GPT_CR_DOZEEN_MASK : 0U) |
        (initConfig->enableRunInDbg ? GPT_CR_DBGEN_MASK : 0U) | (initConfig->enableMode ? GPT_CR_ENMOD_MASK : 0U);

    GPT_SetClockSource(base, initConfig->clockSource);
    GPT_SetClockDivider(base, initConfig->divider);
}

void GPT_Deinit(GPT_Type *base)
{
    /* Disable GPT timers */
    base->CR = 0U;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate the GPT clock*/
    CLOCK_DisableClock(s_gptClocks[GPT_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void GPT_GetDefaultConfig(gpt_config_t *config)
{
    assert(config);

    config->clockSource = kGPT_ClockSource_Periph;
    config->divider = 1U;
    config->enableRunInStop = true;
    config->enableRunInWait = true;
    config->enableRunInDoze = false;
    config->enableRunInDbg = false;
    config->enableFreeRun = false;
    config->enableMode = true;
}
