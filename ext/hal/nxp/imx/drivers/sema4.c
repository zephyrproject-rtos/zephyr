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

#include <assert.h>
#include "sema4.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_TryLock
 * Description   : Lock SEMA4 gate for exclusive access between multicore
 *
 *END**************************************************************************/
sema4_status_t SEMA4_TryLock(SEMA4_Type *base, uint32_t gateIndex)
{
    __IO uint8_t *gate;

    assert(gateIndex < 16);

    gate = &base->GATE00 + gateIndex;

    *gate = SEMA4_GATE00_GTFSM(SEMA4_PROCESSOR_SELF + 1);

    return ((*gate & SEMA4_GATE00_GTFSM_MASK) == SEMA4_GATE00_GTFSM(SEMA4_PROCESSOR_SELF + 1)) ?
           statusSema4Success : statusSema4Busy;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_Lock
 * Description   : Lock SEMA4 gate for exclusive access between multicore,
 *                 polling until success
 *
 *END**************************************************************************/
void SEMA4_Lock(SEMA4_Type *base, uint32_t gateIndex)
{
    __IO uint8_t *gate;

    assert(gateIndex < 16);

    gate = &base->GATE00 + gateIndex;

    do {
        /* Wait gate status free */
        while (*gate & SEMA4_GATE00_GTFSM_MASK) { }
        *gate = SEMA4_GATE00_GTFSM(SEMA4_PROCESSOR_SELF + 1);
    } while ((*gate & SEMA4_GATE00_GTFSM_MASK) != SEMA4_GATE00_GTFSM(SEMA4_PROCESSOR_SELF + 1));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_Unlock
 * Description   : Unlock SEMA4 gate
 *
 *END**************************************************************************/
void SEMA4_Unlock(SEMA4_Type *base, uint32_t gateIndex)
{
    __IO uint8_t *gate;

    assert(gateIndex < 16);

    gate = &base->GATE00 + gateIndex;

    *gate = SEMA4_GATE00_GTFSM(0);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_GetLockProcessor
 * Description   : Get master index which locks the semaphore
 *
 *END**************************************************************************/
uint32_t SEMA4_GetLockProcessor(SEMA4_Type *base, uint32_t gateIndex)
{
    __IO uint8_t *gate;
    uint8_t proc;

    assert(gateIndex < 16);

    gate = &base->GATE00 + gateIndex;

    proc = (*gate & SEMA4_GATE00_GTFSM_MASK) >> SEMA4_GATE00_GTFSM_SHIFT;

    return proc == 0 ? SEMA4_PROCESSOR_NONE : proc - 1;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_ResetGate
 * Description   : Reset SEMA4 gate to unlocked status
 *
 *END**************************************************************************/
void SEMA4_ResetGate(SEMA4_Type *base, uint32_t gateIndex)
{
    assert(gateIndex < 16);

    /* The reset state machine must be in idle state */
    assert ((base->RSTGT & 0x30) == 0);

    base->RSTGT = 0xE2;
    base->RSTGT = 0x1D | SEMA4_RSTGT_RSTGTN(gateIndex);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_ResetAllGates
 * Description   : Reset all SEMA4 gates to unlocked status for certain
 *                 SEMA4 instance
 *
 *END**************************************************************************/
void SEMA4_ResetAllGates(SEMA4_Type *base)
{
    /* The reset state machine must be in idle state */
    assert ((base->RSTGT & 0x30) == 0);

    base->RSTGT = 0xE2;
    base->RSTGT = 0x1D | SEMA4_RSTGT_RSTGTN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_ResetNotification
 * Description   : Reset SEMA4 IRQ notifications
 *
 *END**************************************************************************/
void SEMA4_ResetNotification(SEMA4_Type *base, uint32_t gateIndex)
{
    assert(gateIndex < 16);

    /* The reset state machine must be in idle state */
    assert ((base->RSTNTF & 0x30) == 0);

    base->RSTNTF = 0x47;
    base->RSTNTF = 0xB8 | SEMA4_RSTNTF_RSTNTN(gateIndex);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_ResetAllNotifications
 * Description   : Reset all SEMA4 gates to unlocked status for certain
 *                 SEMA4 instance
 *
 *END**************************************************************************/
void SEMA4_ResetAllNotifications(SEMA4_Type *base)
{
    /* The reset state machine must be in idle state */
    assert ((base->RSTNTF & 0x30) == 0);

    base->RSTNTF = 0x47;
    base->RSTNTF = 0xB8 | SEMA4_RSTNTF_RSTNTN_MASK;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : SEMA4_SetIntCmd
 * Description   : Enable or disable SEMA4 IRQ notification.
 *
 *END**************************************************************************/
void SEMA4_SetIntCmd(SEMA4_Type * base, uint16_t intMask, bool enable)
{
    if (enable)
        base->CPnINE[SEMA4_PROCESSOR_SELF].INE |= intMask;
    else
        base->CPnINE[SEMA4_PROCESSOR_SELF].INE &= ~intMask;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
