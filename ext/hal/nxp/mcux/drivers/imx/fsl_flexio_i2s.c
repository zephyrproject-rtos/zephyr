/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
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

#include "fsl_flexio_i2s.h"

/*******************************************************************************
* Definitations
******************************************************************************/
enum _sai_transfer_state
{
    kFLEXIO_I2S_Busy = 0x0U, /*!< FLEXIO_I2S is busy */
    kFLEXIO_I2S_Idle,        /*!< Transfer is done. */
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

extern uint32_t FLEXIO_GetInstance(FLEXIO_Type *base);

/*!
 * @brief Receive a piece of data in non-blocking way.
 *
 * @param base FLEXIO I2S base pointer
 * @param bitWidth How many bits in a audio word, usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be read.
 * @param size Bytes to be read.
 */
static void FLEXIO_I2S_ReadNonBlocking(FLEXIO_I2S_Type *base, uint8_t bitWidth, uint8_t *rxData, size_t size);

/*!
 * @brief sends a piece of data in non-blocking way.
 *
 * @param base FLEXIO I2S base pointer
 * @param bitWidth How many bits in a audio word, usually 8/16/24/32 bits.
 * @param buffer Pointer to the data to be written.
 * @param size Bytes to be written.
 */
static void FLEXIO_I2S_WriteNonBlocking(FLEXIO_I2S_Type *base, uint8_t bitWidth, uint8_t *txData, size_t size);
/*******************************************************************************
 * Variables
 ******************************************************************************/

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
extern const clock_ip_name_t s_flexioClocks[];
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

extern FLEXIO_Type *const s_flexioBases[];

/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t FLEXIO_I2S_GetInstance(FLEXIO_I2S_Type *base)
{
    return FLEXIO_GetInstance(base->flexioBase);
}

static void FLEXIO_I2S_WriteNonBlocking(FLEXIO_I2S_Type *base, uint8_t bitWidth, uint8_t *txData, size_t size)
{
    uint32_t i = 0;
    uint8_t j = 0;
    uint8_t bytesPerWord = bitWidth / 8U;
    uint32_t data = 0;
    uint32_t temp = 0;

    for (i = 0; i < size / bytesPerWord; i++)
    {
        for (j = 0; j < bytesPerWord; j++)
        {
            temp = (uint32_t)(*txData);
            data |= (temp << (8U * j));
            txData++;
        }
        base->flexioBase->SHIFTBUFBIS[base->txShifterIndex] = (data << (32U - bitWidth));
        data = 0;
    }
}

static void FLEXIO_I2S_ReadNonBlocking(FLEXIO_I2S_Type *base, uint8_t bitWidth, uint8_t *rxData, size_t size)
{
    uint32_t i = 0;
    uint8_t j = 0;
    uint8_t bytesPerWord = bitWidth / 8U;
    uint32_t data = 0;

    for (i = 0; i < size / bytesPerWord; i++)
    {
        data = (base->flexioBase->SHIFTBUFBIS[base->rxShifterIndex] >> (32U - bitWidth));
        for (j = 0; j < bytesPerWord; j++)
        {
            *rxData = (data >> (8U * j)) & 0xFF;
            rxData++;
        }
    }
}

void FLEXIO_I2S_Init(FLEXIO_I2S_Type *base, const flexio_i2s_config_t *config)
{
    assert(base && config);

    flexio_shifter_config_t shifterConfig = {0};
    flexio_timer_config_t timerConfig = {0};

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate flexio clock. */
    CLOCK_EnableClock(s_flexioClocks[FLEXIO_I2S_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Set shifter for I2S Tx data */
    shifterConfig.timerSelect = base->bclkTimerIndex;
    shifterConfig.pinSelect = base->txPinIndex;
    shifterConfig.timerPolarity = config->txTimerPolarity;
    shifterConfig.pinConfig = kFLEXIO_PinConfigOutput;
    shifterConfig.pinPolarity = config->txPinPolarity;
    shifterConfig.shifterMode = kFLEXIO_ShifterModeTransmit;
    shifterConfig.inputSource = kFLEXIO_ShifterInputFromPin;
    shifterConfig.shifterStop = kFLEXIO_ShifterStopBitDisable;
    if (config->masterSlave == kFLEXIO_I2S_Master)
    {
        shifterConfig.shifterStart = kFLEXIO_ShifterStartBitDisabledLoadDataOnShift;
    }
    else
    {
        shifterConfig.shifterStart = kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable;
    }

    FLEXIO_SetShifterConfig(base->flexioBase, base->txShifterIndex, &shifterConfig);

    /* Set shifter for I2S Rx Data */
    shifterConfig.timerSelect = base->bclkTimerIndex;
    shifterConfig.pinSelect = base->rxPinIndex;
    shifterConfig.timerPolarity = config->rxTimerPolarity;
    shifterConfig.pinConfig = kFLEXIO_PinConfigOutputDisabled;
    shifterConfig.pinPolarity = config->rxPinPolarity;
    shifterConfig.shifterMode = kFLEXIO_ShifterModeReceive;
    shifterConfig.inputSource = kFLEXIO_ShifterInputFromPin;
    shifterConfig.shifterStop = kFLEXIO_ShifterStopBitDisable;
    shifterConfig.shifterStart = kFLEXIO_ShifterStartBitDisabledLoadDataOnEnable;

    FLEXIO_SetShifterConfig(base->flexioBase, base->rxShifterIndex, &shifterConfig);

    /* Set Timer to I2S frame sync */
    if (config->masterSlave == kFLEXIO_I2S_Master)
    {
        timerConfig.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(base->txPinIndex);
        timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh;
        timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceExternal;
        timerConfig.pinConfig = kFLEXIO_PinConfigOutput;
        timerConfig.pinSelect = base->fsPinIndex;
        timerConfig.pinPolarity = config->fsPinPolarity;
        timerConfig.timerMode = kFLEXIO_TimerModeSingle16Bit;
        timerConfig.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
        timerConfig.timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
        timerConfig.timerReset = kFLEXIO_TimerResetNever;
        timerConfig.timerDisable = kFLEXIO_TimerDisableNever;
        timerConfig.timerEnable = kFLEXIO_TimerEnableOnPrevTimerEnable;
        timerConfig.timerStart = kFLEXIO_TimerStartBitDisabled;
        timerConfig.timerStop = kFLEXIO_TimerStopBitDisabled;
    }
    else
    {
        timerConfig.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(base->bclkPinIndex);
        timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh;
        timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
        timerConfig.pinConfig = kFLEXIO_PinConfigOutputDisabled;
        timerConfig.pinSelect = base->fsPinIndex;
        timerConfig.pinPolarity = config->fsPinPolarity;
        timerConfig.timerMode = kFLEXIO_TimerModeSingle16Bit;
        timerConfig.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
        timerConfig.timerDecrement = kFLEXIO_TimerDecSrcOnTriggerInputShiftTriggerInput;
        timerConfig.timerReset = kFLEXIO_TimerResetNever;
        timerConfig.timerDisable = kFLEXIO_TimerDisableOnTimerCompare;
        timerConfig.timerEnable = kFLEXIO_TimerEnableOnPinRisingEdge;
        timerConfig.timerStart = kFLEXIO_TimerStartBitDisabled;
        timerConfig.timerStop = kFLEXIO_TimerStopBitDisabled;
    }
    FLEXIO_SetTimerConfig(base->flexioBase, base->fsTimerIndex, &timerConfig);

    /* Set Timer to I2S bit clock */
    if (config->masterSlave == kFLEXIO_I2S_Master)
    {
        timerConfig.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(base->txShifterIndex);
        timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveLow;
        timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
        timerConfig.pinSelect = base->bclkPinIndex;
        timerConfig.pinConfig = kFLEXIO_PinConfigOutput;
        timerConfig.pinPolarity = config->bclkPinPolarity;
        timerConfig.timerMode = kFLEXIO_TimerModeDual8BitBaudBit;
        timerConfig.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
        timerConfig.timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
        timerConfig.timerReset = kFLEXIO_TimerResetNever;
        timerConfig.timerDisable = kFLEXIO_TimerDisableNever;
        timerConfig.timerEnable = kFLEXIO_TimerEnableOnTriggerHigh;
        timerConfig.timerStart = kFLEXIO_TimerStartBitEnabled;
        timerConfig.timerStop = kFLEXIO_TimerStopBitDisabled;
    }
    else
    {
        timerConfig.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_TIMn(base->fsTimerIndex);
        timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh;
        timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
        timerConfig.pinSelect = base->bclkPinIndex;
        timerConfig.pinConfig = kFLEXIO_PinConfigOutputDisabled;
        timerConfig.pinPolarity = config->bclkPinPolarity;
        timerConfig.timerMode = kFLEXIO_TimerModeSingle16Bit;
        timerConfig.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
        timerConfig.timerDecrement = kFLEXIO_TimerDecSrcOnPinInputShiftPinInput;
        timerConfig.timerReset = kFLEXIO_TimerResetNever;
        timerConfig.timerDisable = kFLEXIO_TimerDisableOnTimerCompareTriggerLow;
        timerConfig.timerEnable = kFLEXIO_TimerEnableOnPinRisingEdgeTriggerHigh;
        timerConfig.timerStart = kFLEXIO_TimerStartBitDisabled;
        timerConfig.timerStop = kFLEXIO_TimerStopBitDisabled;
    }
    FLEXIO_SetTimerConfig(base->flexioBase, base->bclkTimerIndex, &timerConfig);

    /* If enable flexio I2S */
    if (config->enableI2S)
    {
        base->flexioBase->CTRL |= FLEXIO_CTRL_FLEXEN_MASK;
    }
    else
    {
        base->flexioBase->CTRL &= ~FLEXIO_CTRL_FLEXEN_MASK;
    }
}

void FLEXIO_I2S_GetDefaultConfig(flexio_i2s_config_t *config)
{
    config->masterSlave = kFLEXIO_I2S_Master;
    config->enableI2S = true;
    config->txPinPolarity = kFLEXIO_PinActiveHigh;
    config->rxPinPolarity = kFLEXIO_PinActiveHigh;
    config->bclkPinPolarity = kFLEXIO_PinActiveHigh;
    config->fsPinPolarity = kFLEXIO_PinActiveLow;
    config->txTimerPolarity = kFLEXIO_ShifterTimerPolarityOnPositive;
    config->rxTimerPolarity = kFLEXIO_ShifterTimerPolarityOnNegitive;
}

void FLEXIO_I2S_Deinit(FLEXIO_I2S_Type *base)
{
    base->flexioBase->SHIFTCFG[base->txShifterIndex] = 0;
    base->flexioBase->SHIFTCTL[base->txShifterIndex] = 0;
    base->flexioBase->SHIFTCFG[base->rxShifterIndex] = 0;
    base->flexioBase->SHIFTCTL[base->rxShifterIndex] = 0;
    base->flexioBase->TIMCFG[base->fsTimerIndex] = 0;
    base->flexioBase->TIMCMP[base->fsTimerIndex] = 0;
    base->flexioBase->TIMCTL[base->fsTimerIndex] = 0;
    base->flexioBase->TIMCFG[base->bclkTimerIndex] = 0;
    base->flexioBase->TIMCMP[base->bclkTimerIndex] = 0;
    base->flexioBase->TIMCTL[base->bclkTimerIndex] = 0;
}

void FLEXIO_I2S_EnableInterrupts(FLEXIO_I2S_Type *base, uint32_t mask)
{
    if (mask & kFLEXIO_I2S_TxDataRegEmptyInterruptEnable)
    {
        FLEXIO_EnableShifterStatusInterrupts(base->flexioBase, 1U << base->txShifterIndex);
    }
    if (mask & kFLEXIO_I2S_RxDataRegFullInterruptEnable)
    {
        FLEXIO_EnableShifterStatusInterrupts(base->flexioBase, 1U << base->rxShifterIndex);
    }
}

uint32_t FLEXIO_I2S_GetStatusFlags(FLEXIO_I2S_Type *base)
{
    uint32_t status = 0;
    status = ((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->txShifterIndex)) >> base->txShifterIndex);
    status |=
        (((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->rxShifterIndex)) >> (base->rxShifterIndex))
         << 1U);
    return status;
}

void FLEXIO_I2S_DisableInterrupts(FLEXIO_I2S_Type *base, uint32_t mask)
{
    if (mask & kFLEXIO_I2S_TxDataRegEmptyInterruptEnable)
    {
        FLEXIO_DisableShifterStatusInterrupts(base->flexioBase, 1U << base->txShifterIndex);
    }
    if (mask & kFLEXIO_I2S_RxDataRegFullInterruptEnable)
    {
        FLEXIO_DisableShifterStatusInterrupts(base->flexioBase, 1U << base->rxShifterIndex);
    }
}

void FLEXIO_I2S_MasterSetFormat(FLEXIO_I2S_Type *base, flexio_i2s_format_t *format, uint32_t srcClock_Hz)
{
    uint32_t timDiv = srcClock_Hz / (format->sampleRate_Hz * 32U * 2U);
    uint32_t bclkDiv = 0;

    /* Shall keep bclk and fs div an integer */
    if (timDiv % 2)
    {
        timDiv += 1U;
    }
    /* Set Frame sync timer cmp */
    base->flexioBase->TIMCMP[base->fsTimerIndex] = FLEXIO_TIMCMP_CMP(32U * timDiv - 1U);

    /* Set bit clock timer cmp */
    bclkDiv = ((timDiv / 2U - 1U) | (63U << 8U));
    base->flexioBase->TIMCMP[base->bclkTimerIndex] = FLEXIO_TIMCMP_CMP(bclkDiv);
}

void FLEXIO_I2S_SlaveSetFormat(FLEXIO_I2S_Type *base, flexio_i2s_format_t *format)
{
    /* Set Frame sync timer cmp */
    base->flexioBase->TIMCMP[base->fsTimerIndex] = FLEXIO_TIMCMP_CMP(32U * 4U - 3U);

    /* Set bit clock timer cmp */
    base->flexioBase->TIMCMP[base->bclkTimerIndex] = FLEXIO_TIMCMP_CMP(32U * 2U - 1U);
}

void FLEXIO_I2S_WriteBlocking(FLEXIO_I2S_Type *base, uint8_t bitWidth, uint8_t *txData, size_t size)
{
    uint32_t i = 0;
    uint8_t bytesPerWord = bitWidth / 8U;

    for (i = 0; i < size / bytesPerWord; i++)
    {
        /* Wait until it can write data */
        while ((FLEXIO_I2S_GetStatusFlags(base) & kFLEXIO_I2S_TxDataRegEmptyFlag) == 0)
        {
        }

        FLEXIO_I2S_WriteNonBlocking(base, bitWidth, txData, bytesPerWord);
        txData += bytesPerWord;
    }

    /* Wait until the last data is sent */
    while ((FLEXIO_I2S_GetStatusFlags(base) & kFLEXIO_I2S_TxDataRegEmptyFlag) == 0)
    {
    }
}

void FLEXIO_I2S_ReadBlocking(FLEXIO_I2S_Type *base, uint8_t bitWidth, uint8_t *rxData, size_t size)
{
    uint32_t i = 0;
    uint8_t bytesPerWord = bitWidth / 8U;

    for (i = 0; i < size / bytesPerWord; i++)
    {
        /* Wait until data is received */
        while (!(FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->rxShifterIndex)))
        {
        }

        FLEXIO_I2S_ReadNonBlocking(base, bitWidth, rxData, bytesPerWord);
        rxData += bytesPerWord;
    }
}

void FLEXIO_I2S_TransferTxCreateHandle(FLEXIO_I2S_Type *base,
                                       flexio_i2s_handle_t *handle,
                                       flexio_i2s_callback_t callback,
                                       void *userData)
{
    assert(handle);

    IRQn_Type flexio_irqs[] = FLEXIO_IRQS;

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Store callback and user data. */
    handle->callback = callback;
    handle->userData = userData;

    /* Save the context in global variables to support the double weak mechanism. */
    FLEXIO_RegisterHandleIRQ(base, handle, FLEXIO_I2S_TransferTxHandleIRQ);

    /* Set the TX/RX state. */
    handle->state = kFLEXIO_I2S_Idle;

    /* Enable interrupt in NVIC. */
    EnableIRQ(flexio_irqs[FLEXIO_I2S_GetInstance(base)]);
}

void FLEXIO_I2S_TransferRxCreateHandle(FLEXIO_I2S_Type *base,
                                       flexio_i2s_handle_t *handle,
                                       flexio_i2s_callback_t callback,
                                       void *userData)
{
    assert(handle);

    IRQn_Type flexio_irqs[] = FLEXIO_IRQS;

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Store callback and user data. */
    handle->callback = callback;
    handle->userData = userData;

    /* Save the context in global variables to support the double weak mechanism. */
    FLEXIO_RegisterHandleIRQ(base, handle, FLEXIO_I2S_TransferRxHandleIRQ);

    /* Set the TX/RX state. */
    handle->state = kFLEXIO_I2S_Idle;

    /* Enable interrupt in NVIC. */
    EnableIRQ(flexio_irqs[FLEXIO_I2S_GetInstance(base)]);
}

void FLEXIO_I2S_TransferSetFormat(FLEXIO_I2S_Type *base,
                                  flexio_i2s_handle_t *handle,
                                  flexio_i2s_format_t *format,
                                  uint32_t srcClock_Hz)
{
    assert(handle && format);

    /* Set the bitWidth to handle */
    handle->bitWidth = format->bitWidth;

    /* Set sample rate */
    if (srcClock_Hz != 0)
    {
        /* It is master */
        FLEXIO_I2S_MasterSetFormat(base, format, srcClock_Hz);
    }
    else
    {
        FLEXIO_I2S_SlaveSetFormat(base, format);
    }
}

status_t FLEXIO_I2S_TransferSendNonBlocking(FLEXIO_I2S_Type *base,
                                            flexio_i2s_handle_t *handle,
                                            flexio_i2s_transfer_t *xfer)
{
    assert(handle);

    /* Check if the queue is full */
    if (handle->queue[handle->queueUser].data)
    {
        return kStatus_FLEXIO_I2S_QueueFull;
    }
    if ((xfer->dataSize == 0) || (xfer->data == NULL))
    {
        return kStatus_InvalidArgument;
    }

    /* Add into queue */
    handle->queue[handle->queueUser].data = xfer->data;
    handle->queue[handle->queueUser].dataSize = xfer->dataSize;
    handle->transferSize[handle->queueUser] = xfer->dataSize;
    handle->queueUser = (handle->queueUser + 1) % FLEXIO_I2S_XFER_QUEUE_SIZE;

    /* Set the state to busy */
    handle->state = kFLEXIO_I2S_Busy;

    FLEXIO_I2S_EnableInterrupts(base, kFLEXIO_I2S_TxDataRegEmptyInterruptEnable);

    /* Enable Tx transfer */
    FLEXIO_I2S_Enable(base, true);

    return kStatus_Success;
}

status_t FLEXIO_I2S_TransferReceiveNonBlocking(FLEXIO_I2S_Type *base,
                                               flexio_i2s_handle_t *handle,
                                               flexio_i2s_transfer_t *xfer)
{
    assert(handle);

    /* Check if the queue is full */
    if (handle->queue[handle->queueUser].data)
    {
        return kStatus_FLEXIO_I2S_QueueFull;
    }

    if ((xfer->dataSize == 0) || (xfer->data == NULL))
    {
        return kStatus_InvalidArgument;
    }

    /* Add into queue */
    handle->queue[handle->queueUser].data = xfer->data;
    handle->queue[handle->queueUser].dataSize = xfer->dataSize;
    handle->transferSize[handle->queueUser] = xfer->dataSize;
    handle->queueUser = (handle->queueUser + 1) % FLEXIO_I2S_XFER_QUEUE_SIZE;

    /* Set state to busy */
    handle->state = kFLEXIO_I2S_Busy;

    /* Enable interrupt */
    FLEXIO_I2S_EnableInterrupts(base, kFLEXIO_I2S_RxDataRegFullInterruptEnable);

    /* Enable Rx transfer */
    FLEXIO_I2S_Enable(base, true);

    return kStatus_Success;
}

void FLEXIO_I2S_TransferAbortSend(FLEXIO_I2S_Type *base, flexio_i2s_handle_t *handle)
{
    assert(handle);

    /* Stop Tx transfer and disable interrupt */
    FLEXIO_I2S_DisableInterrupts(base, kFLEXIO_I2S_TxDataRegEmptyInterruptEnable);
    handle->state = kFLEXIO_I2S_Idle;

    /* Clear the queue */
    memset(handle->queue, 0, sizeof(flexio_i2s_transfer_t) * FLEXIO_I2S_XFER_QUEUE_SIZE);
    handle->queueDriver = 0;
    handle->queueUser = 0;
}

void FLEXIO_I2S_TransferAbortReceive(FLEXIO_I2S_Type *base, flexio_i2s_handle_t *handle)
{
    assert(handle);

    /* Stop rx transfer and disable interrupt */
    FLEXIO_I2S_DisableInterrupts(base, kFLEXIO_I2S_RxDataRegFullInterruptEnable);
    handle->state = kFLEXIO_I2S_Idle;

    /* Clear the queue */
    memset(handle->queue, 0, sizeof(flexio_i2s_transfer_t) * FLEXIO_I2S_XFER_QUEUE_SIZE);
    handle->queueDriver = 0;
    handle->queueUser = 0;
}

status_t FLEXIO_I2S_TransferGetSendCount(FLEXIO_I2S_Type *base, flexio_i2s_handle_t *handle, size_t *count)
{
    assert(handle);

    status_t status = kStatus_Success;

    if (handle->state != kFLEXIO_I2S_Busy)
    {
        status = kStatus_NoTransferInProgress;
    }
    else
    {
        *count = (handle->transferSize[handle->queueDriver] - handle->queue[handle->queueDriver].dataSize);
    }

    return status;
}

status_t FLEXIO_I2S_TransferGetReceiveCount(FLEXIO_I2S_Type *base, flexio_i2s_handle_t *handle, size_t *count)
{
    assert(handle);

    status_t status = kStatus_Success;

    if (handle->state != kFLEXIO_I2S_Busy)
    {
        status = kStatus_NoTransferInProgress;
    }
    else
    {
        *count = (handle->transferSize[handle->queueDriver] - handle->queue[handle->queueDriver].dataSize);
    }

    return status;
}

void FLEXIO_I2S_TransferTxHandleIRQ(void *i2sBase, void *i2sHandle)
{
    assert(i2sHandle);

    flexio_i2s_handle_t *handle = (flexio_i2s_handle_t *)i2sHandle;
    FLEXIO_I2S_Type *base = (FLEXIO_I2S_Type *)i2sBase;
    uint8_t *buffer = handle->queue[handle->queueDriver].data;
    uint8_t dataSize = handle->bitWidth / 8U;

    /* Handle error */
    if (FLEXIO_GetShifterErrorFlags(base->flexioBase) & (1U << base->txShifterIndex))
    {
        FLEXIO_ClearShifterErrorFlags(base->flexioBase, (1U << base->txShifterIndex));
    }
    /* Handle transfer */
    if (((FLEXIO_I2S_GetStatusFlags(base) & kFLEXIO_I2S_TxDataRegEmptyFlag) != 0) &&
        (handle->queue[handle->queueDriver].data != NULL))
    {
        FLEXIO_I2S_WriteNonBlocking(base, handle->bitWidth, buffer, dataSize);

        /* Update internal counter */
        handle->queue[handle->queueDriver].dataSize -= dataSize;
        handle->queue[handle->queueDriver].data += dataSize;
    }

    /* If finished a blcok, call the callback function */
    if ((handle->queue[handle->queueDriver].dataSize == 0U) && (handle->queue[handle->queueDriver].data != NULL))
    {
        memset(&handle->queue[handle->queueDriver], 0, sizeof(flexio_i2s_transfer_t));
        handle->queueDriver = (handle->queueDriver + 1) % FLEXIO_I2S_XFER_QUEUE_SIZE;
        if (handle->callback)
        {
            (handle->callback)(base, handle, kStatus_Success, handle->userData);
        }
    }

    /* If all data finished, just stop the transfer */
    if (handle->queue[handle->queueDriver].data == NULL)
    {
        FLEXIO_I2S_TransferAbortSend(base, handle);
    }
}

void FLEXIO_I2S_TransferRxHandleIRQ(void *i2sBase, void *i2sHandle)
{
    assert(i2sHandle);

    flexio_i2s_handle_t *handle = (flexio_i2s_handle_t *)i2sHandle;
    FLEXIO_I2S_Type *base = (FLEXIO_I2S_Type *)i2sBase;
    uint8_t *buffer = handle->queue[handle->queueDriver].data;
    uint8_t dataSize = handle->bitWidth / 8U;

    /* Handle transfer */
    if (((FLEXIO_I2S_GetStatusFlags(base) & kFLEXIO_I2S_RxDataRegFullFlag) != 0) &&
        (handle->queue[handle->queueDriver].data != NULL))
    {
        FLEXIO_I2S_ReadNonBlocking(base, handle->bitWidth, buffer, dataSize);

        /* Update internal state */
        handle->queue[handle->queueDriver].dataSize -= dataSize;
        handle->queue[handle->queueDriver].data += dataSize;
    }

    /* If finished a blcok, call the callback function */
    if ((handle->queue[handle->queueDriver].dataSize == 0U) && (handle->queue[handle->queueDriver].data != NULL))
    {
        memset(&handle->queue[handle->queueDriver], 0, sizeof(flexio_i2s_transfer_t));
        handle->queueDriver = (handle->queueDriver + 1) % FLEXIO_I2S_XFER_QUEUE_SIZE;
        if (handle->callback)
        {
            (handle->callback)(base, handle, kStatus_Success, handle->userData);
        }
    }

    /* If all data finished, just stop the transfer */
    if (handle->queue[handle->queueDriver].data == NULL)
    {
        FLEXIO_I2S_TransferAbortReceive(base, handle);
    }
}
