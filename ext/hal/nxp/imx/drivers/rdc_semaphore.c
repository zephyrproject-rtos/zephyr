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
#include "rdc_semaphore.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * Private Functions
 ******************************************************************************/
static RDC_SEMAPHORE_Type *RDC_SEMAPHORE_GetGate(uint32_t *pdap)
{
    RDC_SEMAPHORE_Type *semaphore;

    if (*pdap < 64)
        semaphore = RDC_SEMAPHORE1;
    else
    {
        semaphore = RDC_SEMAPHORE2;
        *pdap -= 64;
    }

    return semaphore;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_SEMAPHORE_TryLock
 * Description   : Lock RDC semaphore for shared peripheral access
 *
 *END**************************************************************************/
rdc_semaphore_status_t RDC_SEMAPHORE_TryLock(uint32_t pdap)
{
    RDC_SEMAPHORE_Type *semaphore;
    uint32_t index = pdap;

    semaphore = RDC_SEMAPHORE_GetGate(&index);

    semaphore->GATE[index] = RDC_SEMAPHORE_GATE_GTFSM(RDC_SEMAPHORE_MASTER_SELF + 1);

    return ((semaphore->GATE[index] & RDC_SEMAPHORE_GATE_GTFSM_MASK) ==
           RDC_SEMAPHORE_GATE_GTFSM(RDC_SEMAPHORE_MASTER_SELF + 1)) ?
           statusRdcSemaphoreSuccess : statusRdcSemaphoreBusy;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_SEMAPHORE_Lock
 * Description   : Lock RDC semaphore for shared peripheral access, polling until
 *                 success.
 *
 *END**************************************************************************/
void RDC_SEMAPHORE_Lock(uint32_t pdap)
{
    RDC_SEMAPHORE_Type *semaphore;
    uint32_t index = pdap;

    semaphore = RDC_SEMAPHORE_GetGate(&index);

    do {
        /* Wait gate status free */
        while (semaphore->GATE[index] & RDC_SEMAPHORE_GATE_GTFSM_MASK) { }
        semaphore->GATE[index] = RDC_SEMAPHORE_GATE_GTFSM(RDC_SEMAPHORE_MASTER_SELF + 1);
    } while ((semaphore->GATE[index] & RDC_SEMAPHORE_GATE_GTFSM_MASK) !=
             RDC_SEMAPHORE_GATE_GTFSM(RDC_SEMAPHORE_MASTER_SELF + 1));
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_SEMAPHORE_Unlock
 * Description   : Unlock RDC semaphore
 *
 *END**************************************************************************/
void RDC_SEMAPHORE_Unlock(uint32_t pdap)
{
    RDC_SEMAPHORE_Type *semaphore;
    uint32_t index = pdap;

    semaphore = RDC_SEMAPHORE_GetGate(&index);

    semaphore->GATE[index] = RDC_SEMAPHORE_GATE_GTFSM(0);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_SEMAPHORE_GetLockDomainID
 * Description   : Get domain ID which locks the semaphore
 *
 *END**************************************************************************/
uint32_t RDC_SEMAPHORE_GetLockDomainID(uint32_t pdap)
{
    RDC_SEMAPHORE_Type *semaphore;
    uint32_t index = pdap;

    semaphore = RDC_SEMAPHORE_GetGate(&index);

    return (semaphore->GATE[index] & RDC_SEMAPHORE_GATE_LDOM_MASK) >> RDC_SEMAPHORE_GATE_LDOM_SHIFT;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_SEMAPHORE_GetLockMaster
 * Description   : Get master index which locks the semaphore
 *
 *END**************************************************************************/
uint32_t RDC_SEMAPHORE_GetLockMaster(uint32_t pdap)
{
    RDC_SEMAPHORE_Type *semaphore;
    uint32_t index = pdap;
    uint8_t master;

    semaphore = RDC_SEMAPHORE_GetGate(&index);

    master = (semaphore->GATE[index] & RDC_SEMAPHORE_GATE_GTFSM_MASK) >> RDC_SEMAPHORE_GATE_GTFSM_SHIFT;

    return master == 0 ? RDC_SEMAPHORE_MASTER_NONE : master - 1;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_SEMAPHORE_Reset
 * Description   : Reset RDC semaphore to unlocked status
 *
 *END**************************************************************************/
void RDC_SEMAPHORE_Reset(uint32_t pdap)
{
    RDC_SEMAPHORE_Type *semaphore;
    uint32_t index = pdap;

    semaphore = RDC_SEMAPHORE_GetGate(&index);

    /* The reset state machine must be in idle state */
    assert ((semaphore->RSTGT_R & RDC_SEMAPHORE_RSTGT_R_RSTGSM_MASK) == 0);

    semaphore->RSTGT_W = 0xE2;
    semaphore->RSTGT_W = 0x1D | RDC_SEMAPHORE_RSTGT_W_RSTGTN(index);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : RDC_SEMAPHORE_ResetAll
 * Description   : Reset all RDC semaphores to unlocked status for certain
 *                 RDC_SEMAPHORE instance
 *
 *END**************************************************************************/
void RDC_SEMAPHORE_ResetAll(RDC_SEMAPHORE_Type *base)
{
    /* The reset state machine must be in idle state */
    assert ((base->RSTGT_R & RDC_SEMAPHORE_RSTGT_R_RSTGSM_MASK) == 0);

    base->RSTGT_W = 0xE2;
    base->RSTGT_W = 0x1D | RDC_SEMAPHORE_RSTGT_W_RSTGTN_MASK;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
