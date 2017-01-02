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

#include "fsl_wdog.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

void WDOG_GetDefaultConfig(wdog_config_t *config)
{
    assert(config);

    config->enableWdog = true;
    config->clockSource = kWDOG_LpoClockSource;
    config->prescaler = kWDOG_ClockPrescalerDivide1;
#if defined(FSL_FEATURE_WDOG_HAS_WAITEN) && FSL_FEATURE_WDOG_HAS_WAITEN
    config->workMode.enableWait = true;
#endif /* FSL_FEATURE_WDOG_HAS_WAITEN */
    config->workMode.enableStop = false;
    config->workMode.enableDebug = false;
    config->enableUpdate = true;
    config->enableInterrupt = false;
    config->enableWindowMode = false;
    config->windowValue = 0U;
    config->timeoutValue = 0xFFFFU;
}

void WDOG_Init(WDOG_Type *base, const wdog_config_t *config)
{
    assert(config);

    uint32_t value = 0U;
    uint32_t primaskValue = 0U;

    value = WDOG_STCTRLH_WDOGEN(config->enableWdog) | WDOG_STCTRLH_CLKSRC(config->clockSource) |
            WDOG_STCTRLH_IRQRSTEN(config->enableInterrupt) | WDOG_STCTRLH_WINEN(config->enableWindowMode) |
            WDOG_STCTRLH_ALLOWUPDATE(config->enableUpdate) | WDOG_STCTRLH_DBGEN(config->workMode.enableDebug) |
            WDOG_STCTRLH_STOPEN(config->workMode.enableStop) |
#if defined(FSL_FEATURE_WDOG_HAS_WAITEN) && FSL_FEATURE_WDOG_HAS_WAITEN
            WDOG_STCTRLH_WAITEN(config->workMode.enableWait) |
#endif /* FSL_FEATURE_WDOG_HAS_WAITEN */
            WDOG_STCTRLH_DISTESTWDOG(1U);

    /* Disable the global interrupts. Otherwise, an interrupt could effectively invalidate the unlock sequence
     * and the WCT may expire. After the configuration finishes, re-enable the global interrupts. */
    primaskValue = DisableGlobalIRQ();
    WDOG_Unlock(base);
    /* Wait one bus clock cycle */
    base->RSTCNT = 0U;
    /* Set configruation */
    base->PRESC = WDOG_PRESC_PRESCVAL(config->prescaler);
    base->WINH = (uint16_t)((config->windowValue >> 16U) & 0xFFFFU);
    base->WINL = (uint16_t)((config->windowValue) & 0xFFFFU);
    base->TOVALH = (uint16_t)((config->timeoutValue >> 16U) & 0xFFFFU);
    base->TOVALL = (uint16_t)((config->timeoutValue) & 0xFFFFU);
    base->STCTRLH = value;
    EnableGlobalIRQ(primaskValue);
}

void WDOG_Deinit(WDOG_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupts */
    primaskValue = DisableGlobalIRQ();
    WDOG_Unlock(base);
    /* Wait one bus clock cycle */
    base->RSTCNT = 0U;
    WDOG_Disable(base);
    EnableGlobalIRQ(primaskValue);
    WDOG_ClearResetCount(base);
}

void WDOG_SetTestModeConfig(WDOG_Type *base, wdog_test_config_t *config)
{
    assert(config);

    uint32_t value = 0U;
    uint32_t primaskValue = 0U;

    value = WDOG_STCTRLH_DISTESTWDOG(0U) | WDOG_STCTRLH_TESTWDOG(1U) | WDOG_STCTRLH_TESTSEL(config->testMode) |
            WDOG_STCTRLH_BYTESEL(config->testedByte) | WDOG_STCTRLH_IRQRSTEN(0U) | WDOG_STCTRLH_WDOGEN(1U) |
            WDOG_STCTRLH_ALLOWUPDATE(1U);

    /* Disable the global interrupts. Otherwise, an interrupt could effectively invalidate the unlock sequence
     * and the WCT may expire. After the configuration finishes, re-enable the global interrupts. */
    primaskValue = DisableGlobalIRQ();
    WDOG_Unlock(base);
    /* Wait one bus clock cycle */
    base->RSTCNT = 0U;
    /* Set configruation */
    base->TOVALH = (uint16_t)((config->timeoutValue >> 16U) & 0xFFFFU);
    base->TOVALL = (uint16_t)((config->timeoutValue) & 0xFFFFU);
    base->STCTRLH = value;
    EnableGlobalIRQ(primaskValue);
}

uint32_t WDOG_GetStatusFlags(WDOG_Type *base)
{
    uint32_t status_flag = 0U;

    status_flag |= (base->STCTRLH & WDOG_STCTRLH_WDOGEN_MASK);
    status_flag |= (base->STCTRLL & WDOG_STCTRLL_INTFLG_MASK);

    return status_flag;
}

void WDOG_ClearStatusFlags(WDOG_Type *base, uint32_t mask)
{
    if (mask & kWDOG_TimeoutFlag)
    {
        base->STCTRLL |= WDOG_STCTRLL_INTFLG_MASK;
    }
}

void WDOG_Refresh(WDOG_Type *base)
{
    uint32_t primaskValue = 0U;

    /* Disable the global interrupt to protect refresh sequence */
    primaskValue = DisableGlobalIRQ();
    base->REFRESH = WDOG_FIRST_WORD_OF_REFRESH;
    base->REFRESH = WDOG_SECOND_WORD_OF_REFRESH;
    EnableGlobalIRQ(primaskValue);
}
