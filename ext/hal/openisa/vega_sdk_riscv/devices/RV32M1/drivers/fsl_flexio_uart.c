/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flexio_uart.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*<! @brief uart transfer state. */
enum _flexio_uart_transfer_states
{
    kFLEXIO_UART_TxIdle, /* TX idle. */
    kFLEXIO_UART_TxBusy, /* TX busy. */
    kFLEXIO_UART_RxIdle, /* RX idle. */
    kFLEXIO_UART_RxBusy  /* RX busy. */
};

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
extern const clock_ip_name_t s_flexioClocks[];
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

extern FLEXIO_Type *const s_flexioBases[];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

extern uint32_t FLEXIO_GetInstance(FLEXIO_Type *base);

/*!
 * @brief Get the length of received data in RX ring buffer.
 *
 * @param handle FLEXIO UART handle pointer.
 * @return Length of received data in RX ring buffer.
 */
static size_t FLEXIO_UART_TransferGetRxRingBufferLength(flexio_uart_handle_t *handle);

/*!
 * @brief Check whether the RX ring buffer is full.
 *
 * @param handle FLEXIO UART handle pointer.
 * @retval true  RX ring buffer is full.
 * @retval false RX ring buffer is not full.
 */
static bool FLEXIO_UART_TransferIsRxRingBufferFull(flexio_uart_handle_t *handle);

/*******************************************************************************
 * Codes
 ******************************************************************************/

uint32_t FLEXIO_UART_GetInstance(FLEXIO_UART_Type *base)
{
    return FLEXIO_GetInstance(base->flexioBase);
}

static size_t FLEXIO_UART_TransferGetRxRingBufferLength(flexio_uart_handle_t *handle)
{
    size_t size;

    if (handle->rxRingBufferTail > handle->rxRingBufferHead)
    {
        size = (size_t)(handle->rxRingBufferHead + handle->rxRingBufferSize - handle->rxRingBufferTail);
    }
    else
    {
        size = (size_t)(handle->rxRingBufferHead - handle->rxRingBufferTail);
    }

    return size;
}

static bool FLEXIO_UART_TransferIsRxRingBufferFull(flexio_uart_handle_t *handle)
{
    bool full;

    if (FLEXIO_UART_TransferGetRxRingBufferLength(handle) == (handle->rxRingBufferSize - 1U))
    {
        full = true;
    }
    else
    {
        full = false;
    }

    return full;
}

status_t FLEXIO_UART_Init(FLEXIO_UART_Type *base, const flexio_uart_config_t *userConfig, uint32_t srcClock_Hz)
{
    assert(base && userConfig);

    flexio_shifter_config_t shifterConfig;
    flexio_timer_config_t timerConfig;
    uint32_t ctrlReg = 0;
    uint16_t timerDiv = 0;
    uint16_t timerCmp = 0;
    status_t result = kStatus_Success;

    /* Clear the shifterConfig & timerConfig struct. */
    memset(&shifterConfig, 0, sizeof(shifterConfig));
    memset(&timerConfig, 0, sizeof(timerConfig));

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate flexio clock. */
    CLOCK_EnableClock(s_flexioClocks[FLEXIO_UART_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Reset FLEXIO before configuration. */
    FLEXIO_Reset(base->flexioBase);

    /* Configure FLEXIO UART */
    ctrlReg = base->flexioBase->CTRL;
    ctrlReg &= ~(FLEXIO_CTRL_DOZEN_MASK | FLEXIO_CTRL_DBGE_MASK | FLEXIO_CTRL_FASTACC_MASK | FLEXIO_CTRL_FLEXEN_MASK);
    ctrlReg |= (FLEXIO_CTRL_DBGE(userConfig->enableInDebug) | FLEXIO_CTRL_FASTACC(userConfig->enableFastAccess) |
                FLEXIO_CTRL_FLEXEN(userConfig->enableUart));
    if (!userConfig->enableInDoze)
    {
        ctrlReg |= FLEXIO_CTRL_DOZEN_MASK;
    }

    base->flexioBase->CTRL = ctrlReg;

    /* Do hardware configuration. */
    /* 1. Configure the shifter 0 for tx. */
    shifterConfig.timerSelect = base->timerIndex[0];
    shifterConfig.timerPolarity = kFLEXIO_ShifterTimerPolarityOnPositive;
    shifterConfig.pinConfig = kFLEXIO_PinConfigOutput;
    shifterConfig.pinSelect = base->TxPinIndex;
    shifterConfig.pinPolarity = kFLEXIO_PinActiveHigh;
    shifterConfig.shifterMode = kFLEXIO_ShifterModeTransmit;
    shifterConfig.inputSource = kFLEXIO_ShifterInputFromPin;
    shifterConfig.shifterStop = kFLEXIO_ShifterStopBitHigh;
    shifterConfig.shifterStart = kFLEXIO_ShifterStartBitLow;

    FLEXIO_SetShifterConfig(base->flexioBase, base->shifterIndex[0], &shifterConfig);

    /*2. Configure the timer 0 for tx. */
    timerConfig.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(base->shifterIndex[0]);
    timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveLow;
    timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
    timerConfig.pinConfig = kFLEXIO_PinConfigOutputDisabled;
    timerConfig.pinSelect = base->TxPinIndex;
    timerConfig.pinPolarity = kFLEXIO_PinActiveHigh;
    timerConfig.timerMode = kFLEXIO_TimerModeDual8BitBaudBit;
    timerConfig.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
    timerConfig.timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
    timerConfig.timerReset = kFLEXIO_TimerResetNever;
    timerConfig.timerDisable = kFLEXIO_TimerDisableOnTimerCompare;
    timerConfig.timerEnable = kFLEXIO_TimerEnableOnTriggerHigh;
    timerConfig.timerStop = kFLEXIO_TimerStopBitEnableOnTimerDisable;
    timerConfig.timerStart = kFLEXIO_TimerStartBitEnabled;

    timerDiv = srcClock_Hz / userConfig->baudRate_Bps;
    timerDiv = timerDiv / 2 - 1;

    if (timerDiv > 0xFFU)
    {
        result = kStatus_InvalidArgument;
    }

    timerCmp = ((uint32_t)(userConfig->bitCountPerChar * 2 - 1)) << 8U;
    timerCmp |= timerDiv;

    timerConfig.timerCompare = timerCmp;

    FLEXIO_SetTimerConfig(base->flexioBase, base->timerIndex[0], &timerConfig);

    /* 3. Configure the shifter 1 for rx. */
    shifterConfig.timerSelect = base->timerIndex[1];
    shifterConfig.timerPolarity = kFLEXIO_ShifterTimerPolarityOnNegitive;
    shifterConfig.pinConfig = kFLEXIO_PinConfigOutputDisabled;
    shifterConfig.pinSelect = base->RxPinIndex;
    shifterConfig.pinPolarity = kFLEXIO_PinActiveHigh;
    shifterConfig.shifterMode = kFLEXIO_ShifterModeReceive;
    shifterConfig.inputSource = kFLEXIO_ShifterInputFromPin;
    shifterConfig.shifterStop = kFLEXIO_ShifterStopBitHigh;
    shifterConfig.shifterStart = kFLEXIO_ShifterStartBitLow;

    FLEXIO_SetShifterConfig(base->flexioBase, base->shifterIndex[1], &shifterConfig);

    /* 4. Configure the timer 1 for rx. */
    timerConfig.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(base->RxPinIndex);
    timerConfig.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh;
    timerConfig.triggerSource = kFLEXIO_TimerTriggerSourceExternal;
    timerConfig.pinConfig = kFLEXIO_PinConfigOutputDisabled;
    timerConfig.pinSelect = base->RxPinIndex;
    timerConfig.pinPolarity = kFLEXIO_PinActiveLow;
    timerConfig.timerMode = kFLEXIO_TimerModeDual8BitBaudBit;
    timerConfig.timerOutput = kFLEXIO_TimerOutputOneAffectedByReset;
    timerConfig.timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
    timerConfig.timerReset = kFLEXIO_TimerResetOnTimerPinRisingEdge;
    timerConfig.timerDisable = kFLEXIO_TimerDisableOnTimerCompare;
    timerConfig.timerEnable = kFLEXIO_TimerEnableOnPinRisingEdge;
    timerConfig.timerStop = kFLEXIO_TimerStopBitEnableOnTimerDisable;
    timerConfig.timerStart = kFLEXIO_TimerStartBitEnabled;

    timerConfig.timerCompare = timerCmp;

    FLEXIO_SetTimerConfig(base->flexioBase, base->timerIndex[1], &timerConfig);

    return result;
}

void FLEXIO_UART_Deinit(FLEXIO_UART_Type *base)
{
    /* Disable FLEXIO UART module. */
    FLEXIO_UART_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Gate flexio clock. */
    CLOCK_DisableClock(kCLOCK_Flexio0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void FLEXIO_UART_GetDefaultConfig(flexio_uart_config_t *userConfig)
{
    assert(userConfig);

    userConfig->enableUart = true;
    userConfig->enableInDoze = false;
    userConfig->enableInDebug = true;
    userConfig->enableFastAccess = false;
    /* Default baud rate 115200. */
    userConfig->baudRate_Bps = 115200U;
    /* Default bit count at 8. */
    userConfig->bitCountPerChar = kFLEXIO_UART_8BitsPerChar;
}

void FLEXIO_UART_EnableInterrupts(FLEXIO_UART_Type *base, uint32_t mask)
{
    if (mask & kFLEXIO_UART_TxDataRegEmptyInterruptEnable)
    {
        FLEXIO_EnableShifterStatusInterrupts(base->flexioBase, 1U << base->shifterIndex[0]);
    }
    if (mask & kFLEXIO_UART_RxDataRegFullInterruptEnable)
    {
        FLEXIO_EnableShifterStatusInterrupts(base->flexioBase, 1U << base->shifterIndex[1]);
    }
}

void FLEXIO_UART_DisableInterrupts(FLEXIO_UART_Type *base, uint32_t mask)
{
    if (mask & kFLEXIO_UART_TxDataRegEmptyInterruptEnable)
    {
        FLEXIO_DisableShifterStatusInterrupts(base->flexioBase, 1U << base->shifterIndex[0]);
    }
    if (mask & kFLEXIO_UART_RxDataRegFullInterruptEnable)
    {
        FLEXIO_DisableShifterStatusInterrupts(base->flexioBase, 1U << base->shifterIndex[1]);
    }
}

uint32_t FLEXIO_UART_GetStatusFlags(FLEXIO_UART_Type *base)
{
    uint32_t status = 0;
    status =
        ((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[0])) >> base->shifterIndex[0]);
    status |=
        (((FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[1])) >> (base->shifterIndex[1]))
         << 1U);
    status |=
        (((FLEXIO_GetShifterErrorFlags(base->flexioBase) & (1U << base->shifterIndex[1])) >> (base->shifterIndex[1]))
         << 2U);
    return status;
}

void FLEXIO_UART_ClearStatusFlags(FLEXIO_UART_Type *base, uint32_t mask)
{
    if (mask & kFLEXIO_UART_TxDataRegEmptyFlag)
    {
        FLEXIO_ClearShifterStatusFlags(base->flexioBase, 1U << base->shifterIndex[0]);
    }
    if (mask & kFLEXIO_UART_RxDataRegFullFlag)
    {
        FLEXIO_ClearShifterStatusFlags(base->flexioBase, 1U << base->shifterIndex[1]);
    }
    if (mask & kFLEXIO_UART_RxOverRunFlag)
    {
        FLEXIO_ClearShifterErrorFlags(base->flexioBase, 1U << base->shifterIndex[1]);
    }
}

void FLEXIO_UART_WriteBlocking(FLEXIO_UART_Type *base, const uint8_t *txData, size_t txSize)
{
    assert(txData);
    assert(txSize);

    while (txSize--)
    {
        /* Wait until data transfer complete. */
        while (!(FLEXIO_GetShifterStatusFlags(base->flexioBase) & (1U << base->shifterIndex[0])))
        {
        }

        base->flexioBase->SHIFTBUF[base->shifterIndex[0]] = *txData++;
    }
}

void FLEXIO_UART_ReadBlocking(FLEXIO_UART_Type *base, uint8_t *rxData, size_t rxSize)
{
    assert(rxData);
    assert(rxSize);

    while (rxSize--)
    {
        /* Wait until data transfer complete. */
        while (!(FLEXIO_UART_GetStatusFlags(base) & kFLEXIO_UART_RxDataRegFullFlag))
        {
        }

        *rxData++ = base->flexioBase->SHIFTBUFBYS[base->shifterIndex[1]];
    }
}

status_t FLEXIO_UART_TransferCreateHandle(FLEXIO_UART_Type *base,
                                          flexio_uart_handle_t *handle,
                                          flexio_uart_transfer_callback_t callback,
                                          void *userData)
{
    assert(handle);

    IRQn_Type flexio_irqs[] = FLEXIO_IRQS;

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Set the TX/RX state. */
    handle->rxState = kFLEXIO_UART_RxIdle;
    handle->txState = kFLEXIO_UART_TxIdle;

    /* Set the callback and user data. */
    handle->callback = callback;
    handle->userData = userData;

    /* Enable interrupt in NVIC. */
    EnableIRQ(flexio_irqs[FLEXIO_UART_GetInstance(base)]);

    /* Save the context in global variables to support the double weak mechanism. */
    return FLEXIO_RegisterHandleIRQ(base, handle, FLEXIO_UART_TransferHandleIRQ);
}

void FLEXIO_UART_TransferStartRingBuffer(FLEXIO_UART_Type *base,
                                         flexio_uart_handle_t *handle,
                                         uint8_t *ringBuffer,
                                         size_t ringBufferSize)
{
    assert(handle);

    /* Setup the ringbuffer address */
    if (ringBuffer)
    {
        handle->rxRingBuffer = ringBuffer;
        handle->rxRingBufferSize = ringBufferSize;
        handle->rxRingBufferHead = 0U;
        handle->rxRingBufferTail = 0U;

        /* Enable the interrupt to accept the data when user need the ring buffer. */
        FLEXIO_UART_EnableInterrupts(base, kFLEXIO_UART_RxDataRegFullInterruptEnable);
    }
}

void FLEXIO_UART_TransferStopRingBuffer(FLEXIO_UART_Type *base, flexio_uart_handle_t *handle)
{
    assert(handle);

    if (handle->rxState == kFLEXIO_UART_RxIdle)
    {
        FLEXIO_UART_DisableInterrupts(base, kFLEXIO_UART_RxDataRegFullInterruptEnable);
    }

    handle->rxRingBuffer = NULL;
    handle->rxRingBufferSize = 0U;
    handle->rxRingBufferHead = 0U;
    handle->rxRingBufferTail = 0U;
}

status_t FLEXIO_UART_TransferSendNonBlocking(FLEXIO_UART_Type *base,
                                             flexio_uart_handle_t *handle,
                                             flexio_uart_transfer_t *xfer)
{
    status_t status;

    /* Return error if xfer invalid. */
    if ((0U == xfer->dataSize) || (NULL == xfer->data))
    {
        return kStatus_InvalidArgument;
    }

    /* Return error if current TX busy. */
    if (kFLEXIO_UART_TxBusy == handle->txState)
    {
        status = kStatus_FLEXIO_UART_TxBusy;
    }
    else
    {
        handle->txData = xfer->data;
        handle->txDataSize = xfer->dataSize;
        handle->txDataSizeAll = xfer->dataSize;
        handle->txState = kFLEXIO_UART_TxBusy;

        /* Enable transmiter interrupt. */
        FLEXIO_UART_EnableInterrupts(base, kFLEXIO_UART_TxDataRegEmptyInterruptEnable);

        status = kStatus_Success;
    }

    return status;
}

void FLEXIO_UART_TransferAbortSend(FLEXIO_UART_Type *base, flexio_uart_handle_t *handle)
{
    /* Disable the transmitter and disable the interrupt. */
    FLEXIO_UART_DisableInterrupts(base, kFLEXIO_UART_TxDataRegEmptyInterruptEnable);

    handle->txDataSize = 0;
    handle->txState = kFLEXIO_UART_TxIdle;
}

status_t FLEXIO_UART_TransferGetSendCount(FLEXIO_UART_Type *base, flexio_uart_handle_t *handle, size_t *count)
{
    assert(handle);
    assert(count);

    if (kFLEXIO_UART_TxIdle == handle->txState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->txDataSizeAll - handle->txDataSize;

    return kStatus_Success;
}

status_t FLEXIO_UART_TransferReceiveNonBlocking(FLEXIO_UART_Type *base,
                                                flexio_uart_handle_t *handle,
                                                flexio_uart_transfer_t *xfer,
                                                size_t *receivedBytes)
{
    uint32_t i;
    status_t status;
    /* How many bytes to copy from ring buffer to user memory. */
    size_t bytesToCopy = 0U;
    /* How many bytes to receive. */
    size_t bytesToReceive;
    /* How many bytes currently have received. */
    size_t bytesCurrentReceived;

    /* Return error if xfer invalid. */
    if ((0U == xfer->dataSize) || (NULL == xfer->data))
    {
        return kStatus_InvalidArgument;
    }

    /* How to get data:
       1. If RX ring buffer is not enabled, then save xfer->data and xfer->dataSize
          to uart handle, enable interrupt to store received data to xfer->data. When
          all data received, trigger callback.
       2. If RX ring buffer is enabled and not empty, get data from ring buffer first.
          If there are enough data in ring buffer, copy them to xfer->data and return.
          If there are not enough data in ring buffer, copy all of them to xfer->data,
          save the xfer->data remained empty space to uart handle, receive data
          to this empty space and trigger callback when finished. */

    if (kFLEXIO_UART_RxBusy == handle->rxState)
    {
        status = kStatus_FLEXIO_UART_RxBusy;
    }
    else
    {
        bytesToReceive = xfer->dataSize;
        bytesCurrentReceived = 0U;

        /* If RX ring buffer is used. */
        if (handle->rxRingBuffer)
        {
            /* Disable FLEXIO_UART RX IRQ, protect ring buffer. */
            FLEXIO_UART_DisableInterrupts(base, kFLEXIO_UART_RxDataRegFullInterruptEnable);

            /* How many bytes in RX ring buffer currently. */
            bytesToCopy = FLEXIO_UART_TransferGetRxRingBufferLength(handle);

            if (bytesToCopy)
            {
                bytesToCopy = MIN(bytesToReceive, bytesToCopy);

                bytesToReceive -= bytesToCopy;

                /* Copy data from ring buffer to user memory. */
                for (i = 0U; i < bytesToCopy; i++)
                {
                    xfer->data[bytesCurrentReceived++] = handle->rxRingBuffer[handle->rxRingBufferTail];

                    /* Wrap to 0. Not use modulo (%) because it might be large and slow. */
                    if (handle->rxRingBufferTail + 1U == handle->rxRingBufferSize)
                    {
                        handle->rxRingBufferTail = 0U;
                    }
                    else
                    {
                        handle->rxRingBufferTail++;
                    }
                }
            }

            /* If ring buffer does not have enough data, still need to read more data. */
            if (bytesToReceive)
            {
                /* No data in ring buffer, save the request to UART handle. */
                handle->rxData = xfer->data + bytesCurrentReceived;
                handle->rxDataSize = bytesToReceive;
                handle->rxDataSizeAll = bytesToReceive;
                handle->rxState = kFLEXIO_UART_RxBusy;
            }

            /* Enable FLEXIO_UART RX IRQ if previously enabled. */
            FLEXIO_UART_EnableInterrupts(base, kFLEXIO_UART_RxDataRegFullInterruptEnable);
        }
        /* Ring buffer not used. */
        else
        {
            handle->rxData = xfer->data + bytesCurrentReceived;
            handle->rxDataSize = bytesToReceive;
            handle->rxDataSizeAll = bytesToReceive;
            handle->rxState = kFLEXIO_UART_RxBusy;

            /* Enable RX interrupt. */
            FLEXIO_UART_EnableInterrupts(base, kFLEXIO_UART_RxDataRegFullInterruptEnable);
        }

        /* Return the how many bytes have read. */
        if (receivedBytes)
        {
            *receivedBytes = bytesCurrentReceived;
        }

        status = kStatus_Success;
    }

    return status;
}

void FLEXIO_UART_TransferAbortReceive(FLEXIO_UART_Type *base, flexio_uart_handle_t *handle)
{
    /* Only abort the receive to handle->rxData, the RX ring buffer is still working. */
    if (!handle->rxRingBuffer)
    {
        /* Disable RX interrupt. */
        FLEXIO_UART_DisableInterrupts(base, kFLEXIO_UART_RxDataRegFullInterruptEnable);
    }

    handle->rxDataSize = 0U;
    handle->rxState = kFLEXIO_UART_RxIdle;
}

status_t FLEXIO_UART_TransferGetReceiveCount(FLEXIO_UART_Type *base, flexio_uart_handle_t *handle, size_t *count)
{
    assert(handle);
    assert(count);

    if (kFLEXIO_UART_RxIdle == handle->rxState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->rxDataSizeAll - handle->rxDataSize;

    return kStatus_Success;
}

void FLEXIO_UART_TransferHandleIRQ(void *uartType, void *uartHandle)
{
    uint8_t count = 1;
    FLEXIO_UART_Type *base = (FLEXIO_UART_Type *)uartType;
    flexio_uart_handle_t *handle = (flexio_uart_handle_t *)uartHandle;

    /* Read the status back. */
    uint8_t status = FLEXIO_UART_GetStatusFlags(base);

    /* If RX overrun. */
    if (kFLEXIO_UART_RxOverRunFlag & status)
    {
        /* Clear Overrun flag. */
        FLEXIO_UART_ClearStatusFlags(base, kFLEXIO_UART_RxOverRunFlag);

        /* Trigger callback. */
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_FLEXIO_UART_RxHardwareOverrun, handle->userData);
        }
    }

    /* Receive data register full */
    if ((kFLEXIO_UART_RxDataRegFullFlag & status) && (base->flexioBase->SHIFTSIEN & (1U << base->shifterIndex[1])))
    {
        /* If handle->rxDataSize is not 0, first save data to handle->rxData. */
        if (handle->rxDataSize)
        {
            /* Using non block API to read the data from the registers. */
            FLEXIO_UART_ReadByte(base, handle->rxData);
            handle->rxDataSize--;
            handle->rxData++;
            count--;

            /* If all the data required for upper layer is ready, trigger callback. */
            if (!handle->rxDataSize)
            {
                handle->rxState = kFLEXIO_UART_RxIdle;

                if (handle->callback)
                {
                    handle->callback(base, handle, kStatus_FLEXIO_UART_RxIdle, handle->userData);
                }
            }
        }

        if (handle->rxRingBuffer)
        {
            if (count)
            {
                /* If RX ring buffer is full, trigger callback to notify over run. */
                if (FLEXIO_UART_TransferIsRxRingBufferFull(handle))
                {
                    if (handle->callback)
                    {
                        handle->callback(base, handle, kStatus_FLEXIO_UART_RxRingBufferOverrun, handle->userData);
                    }
                }

                /* If ring buffer is still full after callback function, the oldest data is overrided. */
                if (FLEXIO_UART_TransferIsRxRingBufferFull(handle))
                {
                    /* Increase handle->rxRingBufferTail to make room for new data. */
                    if (handle->rxRingBufferTail + 1U == handle->rxRingBufferSize)
                    {
                        handle->rxRingBufferTail = 0U;
                    }
                    else
                    {
                        handle->rxRingBufferTail++;
                    }
                }

                /* Read data. */
                handle->rxRingBuffer[handle->rxRingBufferHead] = base->flexioBase->SHIFTBUFBYS[base->shifterIndex[1]];

                /* Increase handle->rxRingBufferHead. */
                if (handle->rxRingBufferHead + 1U == handle->rxRingBufferSize)
                {
                    handle->rxRingBufferHead = 0U;
                }
                else
                {
                    handle->rxRingBufferHead++;
                }
            }
        }
        /* If no receive requst pending, stop RX interrupt. */
        else if (!handle->rxDataSize)
        {
            FLEXIO_UART_DisableInterrupts(base, kFLEXIO_UART_RxDataRegFullInterruptEnable);
        }
        else
        {
        }
    }

    /* Send data register empty and the interrupt is enabled. */
    if ((kFLEXIO_UART_TxDataRegEmptyFlag & status) && (base->flexioBase->SHIFTSIEN & (1U << base->shifterIndex[0])))
    {
        if (handle->txDataSize)
        {
            /* Using non block API to write the data to the registers. */
            FLEXIO_UART_WriteByte(base, handle->txData);
            handle->txData++;
            handle->txDataSize--;
            count--;

            /* If all the data are written to data register, TX finished. */
            if (!handle->txDataSize)
            {
                handle->txState = kFLEXIO_UART_TxIdle;

                /* Disable TX register empty interrupt. */
                FLEXIO_UART_DisableInterrupts(base, kFLEXIO_UART_TxDataRegEmptyInterruptEnable);

                /* Trigger callback. */
                if (handle->callback)
                {
                    handle->callback(base, handle, kStatus_FLEXIO_UART_TxIdle, handle->userData);
                }
            }
        }
    }
}
