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

#include "mu_imx.h"

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_TrySendMsg
 * Description   : Try to send message to the other core.
 *
 *END**************************************************************************/
mu_status_t MU_TrySendMsg(MU_Type * base, uint32_t regIndex, uint32_t msg)
{
    assert(regIndex < MU_TR_COUNT);

    // TX register is empty.
    if(MU_IsTxEmpty(base, regIndex))
    {
        base->TR[regIndex] = msg;
        return kStatus_MU_Success;
    }

    return kStatus_MU_TxNotEmpty;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_SendMsg
 * Description   : Wait and send message to the other core.
 *
 *END**************************************************************************/
void MU_SendMsg(MU_Type * base, uint32_t regIndex, uint32_t msg)
{
    assert(regIndex < MU_TR_COUNT);
    uint32_t mask = MU_SR_TE0_MASK >> regIndex;
    // Wait TX register to be empty.
    while (!(base->SR & mask)) { }
    base->TR[regIndex] = msg;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_TryReceiveMsg
 * Description   : Try to receive message from the other core.
 *
 *END**************************************************************************/
mu_status_t MU_TryReceiveMsg(MU_Type * base, uint32_t regIndex, uint32_t *msg)
{
    assert(regIndex < MU_RR_COUNT);

    // RX register is full.
    if(MU_IsRxFull(base, regIndex))
    {
        *msg = base->RR[regIndex];
        return kStatus_MU_Success;
    }

    return kStatus_MU_RxNotFull;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_ReceiveMsg
 * Description   : Wait to receive message from the other core.
 *
 *END**************************************************************************/
void MU_ReceiveMsg(MU_Type * base, uint32_t regIndex, uint32_t *msg)
{
    assert(regIndex < MU_TR_COUNT);
    uint32_t mask = MU_SR_RF0_MASK >> regIndex;

    // Wait RX register to be full.
    while (!(base->SR & mask)) { }
    *msg = base->RR[regIndex];
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_TriggerGeneralInt
 * Description   : Trigger general purpose interrupt to the other core.
 *
 *END**************************************************************************/
mu_status_t MU_TriggerGeneralInt(MU_Type * base, uint32_t index)
{
    // Previous interrupt has been accepted.
    if (MU_IsGeneralIntAccepted(base, index))
    {
        // All interrupts have been accepted, trigger now.
        base->CR = (base->CR & ~MU_CR_GIRn_MASK)  // Clear GIRn
                             | (MU_CR_GIR0_MASK>>index); // Set GIRn
        return kStatus_MU_Success;
    }

    return kStatus_MU_IntPending;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_TrySetFlags
 * Description   : Try to set some bits of the 3-bit flag.
 *
 *END**************************************************************************/
mu_status_t MU_TrySetFlags(MU_Type * base, uint32_t flags)
{
    if(MU_IsFlagPending(base))
    {
        return kStatus_MU_FlagPending;
    }

    base->CR = (base->CR & ~(MU_CR_GIRn_MASK | MU_CR_Fn_MASK)) | flags;
    return kStatus_MU_Success;
}

/*FUNCTION**********************************************************************
 *
 * Function Name : MU_SetFlags
 * Description   : Block to set some bits of the 3-bit flag.
 *
 *END**************************************************************************/
void MU_SetFlags(MU_Type * base, uint32_t flags)
{
    while (MU_IsFlagPending(base)) { }
    base->CR = (base->CR & ~(MU_CR_GIRn_MASK | MU_CR_Fn_MASK)) | flags;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
