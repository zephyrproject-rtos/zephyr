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

#include "gpt.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : GPT_Init
 * Description   : Initialize GPT to reset state and initialize running mode
 *
 *END**************************************************************************/
void GPT_Init(GPT_Type* base, const gpt_init_config_t* initConfig)
{
    assert(initConfig);

    base->CR = 0;

    GPT_SoftReset(base);

    base->CR = (initConfig->freeRun ? GPT_CR_FRR_MASK : 0)       |
               (initConfig->waitEnable ? GPT_CR_WAITEN_MASK : 0) |
               (initConfig->stopEnable ? GPT_CR_STOPEN_MASK : 0) |
               (initConfig->dozeEnable ? GPT_CR_DOZEEN_MASK : 0) |
               (initConfig->dbgEnable ? GPT_CR_DBGEN_MASK : 0)   |
               (initConfig->enableMode ? GPT_CR_ENMOD_MASK : 0);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : GPT_SetClockSource
 * Description   : Set clock source of GPT
 *
 *END**************************************************************************/
void GPT_SetClockSource(GPT_Type* base, uint32_t source)
{
    assert(source <= gptClockSourceOsc);

    if (source == gptClockSourceOsc)
        base->CR = (base->CR & ~GPT_CR_CLKSRC_MASK) | GPT_CR_EN_24M_MASK | GPT_CR_CLKSRC(source);
    else
        base->CR = (base->CR & ~(GPT_CR_CLKSRC_MASK | GPT_CR_EN_24M_MASK)) | GPT_CR_CLKSRC(source);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : GPT_SetIntCmd
 * Description   : Enable or disable GPT interrupts
 *
 *END**************************************************************************/
void GPT_SetIntCmd(GPT_Type* base, uint32_t flags, bool enable)
{
    if (enable)
        base->IR |= flags;
    else
        base->IR &= ~flags;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
