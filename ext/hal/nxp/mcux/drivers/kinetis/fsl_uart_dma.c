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

#include "fsl_uart_dma.h"
#include "fsl_dmamux.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Array of UART handle. */
#if (defined(UART5))
#define UART_HANDLE_ARRAY_SIZE 6
#else /* UART5 */
#if (defined(UART4))
#define UART_HANDLE_ARRAY_SIZE 5
#else /* UART4 */
#if (defined(UART3))
#define UART_HANDLE_ARRAY_SIZE 4
#else /* UART3 */
#if (defined(UART2))
#define UART_HANDLE_ARRAY_SIZE 3
#else /* UART2 */
#if (defined(UART1))
#define UART_HANDLE_ARRAY_SIZE 2
#else /* UART1 */
#if (defined(UART0))
#define UART_HANDLE_ARRAY_SIZE 1
#else /* UART0 */
#error No UART instance.
#endif /* UART 0 */
#endif /* UART 1 */
#endif /* UART 2 */
#endif /* UART 3 */
#endif /* UART 4 */
#endif /* UART 5 */

/*<! Structure definition for uart_dma_handle_t. The structure is private. */
typedef struct _uart_dma_private_handle
{
    UART_Type *base;
    uart_dma_handle_t *handle;
} uart_dma_private_handle_t;

/* UART DMA transfer handle. */
enum _uart_dma_tansfer_states
{
    kUART_TxIdle, /* TX idle. */
    kUART_TxBusy, /* TX busy. */
    kUART_RxIdle, /* RX idle. */
    kUART_RxBusy  /* RX busy. */
};

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*<! Private handle only used for internally. */
static uart_dma_private_handle_t s_dmaPrivateHandle[UART_HANDLE_ARRAY_SIZE];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief UART DMA send finished callback function.
 *
 * This function is called when UART DMA send finished. It disables the UART
 * TX DMA request and sends @ref kStatus_UART_TxIdle to UART callback.
 *
 * @param handle The DMA handle.
 * @param param Callback function parameter.
 */
static void UART_TransferSendDMACallback(dma_handle_t *handle, void *param);

/*!
 * @brief UART DMA receive finished callback function.
 *
 * This function is called when UART DMA receive finished. It disables the UART
 * RX DMA request and sends @ref kStatus_UART_RxIdle to UART callback.
 *
 * @param handle The DMA handle.
 * @param param Callback function parameter.
 */
static void UART_TransferReceiveDMACallback(dma_handle_t *handle, void *param);

/*!
 * @brief Get the UART instance from peripheral base address.
 *
 * @param base UART peripheral base address.
 * @return UART instance.
 */
extern uint32_t UART_GetInstance(UART_Type *base);

/*******************************************************************************
 * Code
 ******************************************************************************/

static void UART_TransferSendDMACallback(dma_handle_t *handle, void *param)
{
    assert(handle);
    assert(param);

    uart_dma_private_handle_t *uartPrivateHandle = (uart_dma_private_handle_t *)param;

    /* Disable UART TX DMA. */
    UART_EnableTxDMA(uartPrivateHandle->base, false);

    /* Disable interrupt. */
    DMA_DisableInterrupts(handle->base, handle->channel);

    uartPrivateHandle->handle->txState = kUART_TxIdle;

    if (uartPrivateHandle->handle->callback)
    {
        uartPrivateHandle->handle->callback(uartPrivateHandle->base, uartPrivateHandle->handle, kStatus_UART_TxIdle,
                                            uartPrivateHandle->handle->userData);
    }
}

static void UART_TransferReceiveDMACallback(dma_handle_t *handle, void *param)
{
    assert(handle);
    assert(param);

    uart_dma_private_handle_t *uartPrivateHandle = (uart_dma_private_handle_t *)param;

    /* Disable UART RX DMA. */
    UART_EnableRxDMA(uartPrivateHandle->base, false);

    /* Disable interrupt. */
    DMA_DisableInterrupts(handle->base, handle->channel);

    uartPrivateHandle->handle->rxState = kUART_RxIdle;

    if (uartPrivateHandle->handle->callback)
    {
        uartPrivateHandle->handle->callback(uartPrivateHandle->base, uartPrivateHandle->handle, kStatus_UART_RxIdle,
                                            uartPrivateHandle->handle->userData);
    }
}

void UART_TransferCreateHandleDMA(UART_Type *base,
                                  uart_dma_handle_t *handle,
                                  uart_dma_transfer_callback_t callback,
                                  void *userData,
                                  dma_handle_t *txDmaHandle,
                                  dma_handle_t *rxDmaHandle)
{
    assert(handle);

    uint32_t instance = UART_GetInstance(base);

    memset(handle, 0, sizeof(*handle));

    s_dmaPrivateHandle[instance].base = base;
    s_dmaPrivateHandle[instance].handle = handle;

    handle->rxState = kUART_RxIdle;
    handle->txState = kUART_TxIdle;

    handle->callback = callback;
    handle->userData = userData;

#if defined(FSL_FEATURE_UART_HAS_FIFO) && FSL_FEATURE_UART_HAS_FIFO
    /* Note:
       Take care of the RX FIFO, DMA request only assert when received bytes
       equal or more than RX water mark, there is potential issue if RX water
       mark larger than 1.
       For example, if RX FIFO water mark is 2, upper layer needs 5 bytes and
       5 bytes are received. the last byte will be saved in FIFO but not trigger
       DMA transfer because the water mark is 2.
     */
    if (rxDmaHandle)
    {
        base->RWFIFO = 1U;
    }
#endif

    handle->rxDmaHandle = rxDmaHandle;
    handle->txDmaHandle = txDmaHandle;

    /* Configure TX. */
    if (txDmaHandle)
    {
        DMA_SetCallback(txDmaHandle, UART_TransferSendDMACallback, &s_dmaPrivateHandle[instance]);
    }

    /* Configure RX. */
    if (rxDmaHandle)
    {
        DMA_SetCallback(rxDmaHandle, UART_TransferReceiveDMACallback, &s_dmaPrivateHandle[instance]);
    }
}

status_t UART_TransferSendDMA(UART_Type *base, uart_dma_handle_t *handle, uart_transfer_t *xfer)
{
    assert(handle);
    assert(handle->txDmaHandle);
    assert(xfer);
    assert(xfer->data);
    assert(xfer->dataSize);

    dma_transfer_config_t xferConfig;
    status_t status;

    /* If previous TX not finished. */
    if (kUART_TxBusy == handle->txState)
    {
        status = kStatus_UART_TxBusy;
    }
    else
    {
        handle->txState = kUART_TxBusy;
        handle->txDataSizeAll = xfer->dataSize;

        /* Prepare transfer. */
        DMA_PrepareTransfer(&xferConfig, xfer->data, sizeof(uint8_t), (void *)UART_GetDataRegisterAddress(base),
                            sizeof(uint8_t), xfer->dataSize, kDMA_MemoryToPeripheral);

        /* Submit transfer. */
        DMA_SubmitTransfer(handle->txDmaHandle, &xferConfig, kDMA_EnableInterrupt);
        DMA_StartTransfer(handle->txDmaHandle);

        /* Enable UART TX DMA. */
        UART_EnableTxDMA(base, true);

        status = kStatus_Success;
    }

    return status;
}

status_t UART_TransferReceiveDMA(UART_Type *base, uart_dma_handle_t *handle, uart_transfer_t *xfer)
{
    assert(handle);
    assert(handle->rxDmaHandle);
    assert(xfer);
    assert(xfer->data);
    assert(xfer->dataSize);

    dma_transfer_config_t xferConfig;
    status_t status;

    /* If previous RX not finished. */
    if (kUART_RxBusy == handle->rxState)
    {
        status = kStatus_UART_RxBusy;
    }
    else
    {
        handle->rxState = kUART_RxBusy;
        handle->rxDataSizeAll = xfer->dataSize;

        /* Prepare transfer. */
        DMA_PrepareTransfer(&xferConfig, (void *)UART_GetDataRegisterAddress(base), sizeof(uint8_t), xfer->data,
                            sizeof(uint8_t), xfer->dataSize, kDMA_PeripheralToMemory);

        /* Submit transfer. */
        DMA_SubmitTransfer(handle->rxDmaHandle, &xferConfig, kDMA_EnableInterrupt);
        DMA_StartTransfer(handle->rxDmaHandle);

        /* Enable UART RX DMA. */
        UART_EnableRxDMA(base, true);

        status = kStatus_Success;
    }

    return status;
}

void UART_TransferAbortSendDMA(UART_Type *base, uart_dma_handle_t *handle)
{
    assert(handle);
    assert(handle->txDmaHandle);

    /* Disable UART TX DMA. */
    UART_EnableTxDMA(base, false);

    /* Stop transfer. */
    DMA_AbortTransfer(handle->txDmaHandle);

    /* Write DMA->DSR[DONE] to abort transfer and clear status. */
    DMA_ClearChannelStatusFlags(handle->txDmaHandle->base, handle->txDmaHandle->channel, kDMA_TransactionsDoneFlag);

    handle->txState = kUART_TxIdle;
}

void UART_TransferAbortReceiveDMA(UART_Type *base, uart_dma_handle_t *handle)
{
    assert(handle);
    assert(handle->rxDmaHandle);

    /* Disable UART RX DMA. */
    UART_EnableRxDMA(base, false);

    /* Stop transfer. */
    DMA_AbortTransfer(handle->rxDmaHandle);

    /* Write DMA->DSR[DONE] to abort transfer and clear status. */
    DMA_ClearChannelStatusFlags(handle->rxDmaHandle->base, handle->rxDmaHandle->channel, kDMA_TransactionsDoneFlag);

    handle->rxState = kUART_RxIdle;
}

status_t UART_TransferGetSendCountDMA(UART_Type *base, uart_dma_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(handle->txDmaHandle);
    assert(count);

    if (kUART_TxIdle == handle->txState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->txDataSizeAll - DMA_GetRemainingBytes(handle->txDmaHandle->base, handle->txDmaHandle->channel);

    return kStatus_Success;
}

status_t UART_TransferGetReceiveCountDMA(UART_Type *base, uart_dma_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(handle->rxDmaHandle);
    assert(count);

    if (kUART_RxIdle == handle->rxState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->rxDataSizeAll - DMA_GetRemainingBytes(handle->rxDmaHandle->base, handle->rxDmaHandle->channel);

    return kStatus_Success;
}
