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

#include "fsl_lpsci_dma.h"
#include "fsl_dmamux.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*<! Structure definition for lpsci_dma_handle_t. The structure is private. */
typedef struct _lpsci_dma_private_handle
{
    UART0_Type *base;
    lpsci_dma_handle_t *handle;
} lpsci_dma_private_handle_t;

/* LPSCI DMA transfer handle. */
enum _lpsci_dma_tansfer_states
{
    kLPSCI_TxIdle, /* TX idle. */
    kLPSCI_TxBusy, /* TX busy. */
    kLPSCI_RxIdle, /* RX idle. */
    kLPSCI_RxBusy  /* RX busy. */
};

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*<! Private handle only used for internally. */
static lpsci_dma_private_handle_t s_dmaPrivateHandle[FSL_FEATURE_SOC_LPSCI_COUNT];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief LPSCI DMA send finished callback function.
 *
 * This function is called when LPSCI DMA send finished. It disables the LPSCI
 * TX DMA request and sends @ref kStatus_LPSCI_TxIdle to LPSCI callback.
 *
 * @param handle The DMA handle.
 * @param param Callback function parameter.
 */
static void LPSCI_TransferSendDMACallback(dma_handle_t *handle, void *param);

/*!
 * @brief LPSCI DMA receive finished callback function.
 *
 * This function is called when LPSCI DMA receive finished. It disables the LPSCI
 * RX DMA request and sends @ref kStatus_LPSCI_RxIdle to LPSCI callback.
 *
 * @param handle The DMA handle.
 * @param param Callback function parameter.
 */
static void LPSCI_TransferReceiveDMACallback(dma_handle_t *handle, void *param);

/*!
 * @brief Get the LPSCI instance from peripheral base address.
 *
 * @param base LPSCI peripheral base address.
 * @return LPSCI instance.
 */
extern uint32_t LPSCI_GetInstance(UART0_Type *base);

/*******************************************************************************
 * Code
 ******************************************************************************/
static void LPSCI_TransferSendDMACallback(dma_handle_t *handle, void *param)
{
    assert(handle);
    assert(param);

    lpsci_dma_private_handle_t *lpsciPrivateHandle = (lpsci_dma_private_handle_t *)param;

    /* Disable LPSCI TX DMA. */
    LPSCI_EnableTxDMA(lpsciPrivateHandle->base, false);

    /* Disable interrupt. */
    DMA_DisableInterrupts(handle->base, handle->channel);

    lpsciPrivateHandle->handle->txState = kLPSCI_TxIdle;

    if (lpsciPrivateHandle->handle->callback)
    {
        lpsciPrivateHandle->handle->callback(lpsciPrivateHandle->base, lpsciPrivateHandle->handle, kStatus_LPSCI_TxIdle,
                                             lpsciPrivateHandle->handle->userData);
    }
}

static void LPSCI_TransferReceiveDMACallback(dma_handle_t *handle, void *param)
{
    assert(handle);
    assert(param);

    lpsci_dma_private_handle_t *lpsciPrivateHandle = (lpsci_dma_private_handle_t *)param;

    /* Disable LPSCI RX DMA. */
    LPSCI_EnableRxDMA(lpsciPrivateHandle->base, false);

    /* Disable interrupt. */
    DMA_DisableInterrupts(handle->base, handle->channel);

    lpsciPrivateHandle->handle->rxState = kLPSCI_RxIdle;

    if (lpsciPrivateHandle->handle->callback)
    {
        lpsciPrivateHandle->handle->callback(lpsciPrivateHandle->base, lpsciPrivateHandle->handle, kStatus_LPSCI_RxIdle,
                                             lpsciPrivateHandle->handle->userData);
    }
}

void LPSCI_TransferCreateHandleDMA(UART0_Type *base,
                                   lpsci_dma_handle_t *handle,
                                   lpsci_dma_transfer_callback_t callback,
                                   void *userData,
                                   dma_handle_t *txDmaHandle,
                                   dma_handle_t *rxDmaHandle)
{
    assert(handle);

    uint32_t instance = LPSCI_GetInstance(base);

    memset(handle, 0, sizeof(lpsci_dma_handle_t));

    s_dmaPrivateHandle[instance].base = base;
    s_dmaPrivateHandle[instance].handle = handle;

    handle->rxState = kLPSCI_RxIdle;
    handle->txState = kLPSCI_TxIdle;

    handle->callback = callback;
    handle->userData = userData;

#if defined(FSL_FEATURE_LPSCI_HAS_FIFO) && FSL_FEATURE_LPSCI_HAS_FIFO
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
        DMA_SetCallback(txDmaHandle, LPSCI_TransferSendDMACallback, &s_dmaPrivateHandle[instance]);
    }

    /* Configure RX. */
    if (rxDmaHandle)
    {
        DMA_SetCallback(rxDmaHandle, LPSCI_TransferReceiveDMACallback, &s_dmaPrivateHandle[instance]);
    }
}

status_t LPSCI_TransferSendDMA(UART0_Type *base, lpsci_dma_handle_t *handle, lpsci_transfer_t *xfer)
{
    assert(handle);
    assert(handle->txDmaHandle);
    assert(xfer);
    assert(xfer->data);
    assert(xfer->dataSize);

    dma_transfer_config_t xferConfig;
    status_t status;

    /* If previous TX not finished. */
    if (kLPSCI_TxBusy == handle->txState)
    {
        status = kStatus_LPSCI_TxBusy;
    }
    else
    {
        handle->txState = kLPSCI_TxBusy;
        handle->txDataSizeAll = xfer->dataSize;

        /* Prepare transfer. */
        DMA_PrepareTransfer(&xferConfig, xfer->data, sizeof(uint8_t), (void *)LPSCI_GetDataRegisterAddress(base),
                            sizeof(uint8_t), xfer->dataSize, kDMA_MemoryToPeripheral);

        /* Submit transfer. */
        DMA_SubmitTransfer(handle->txDmaHandle, &xferConfig, kDMA_EnableInterrupt);
        DMA_StartTransfer(handle->txDmaHandle);

        /* Enable LPSCI TX DMA. */
        LPSCI_EnableTxDMA(base, true);

        status = kStatus_Success;
    }

    return status;
}

status_t LPSCI_TransferReceiveDMA(UART0_Type *base, lpsci_dma_handle_t *handle, lpsci_transfer_t *xfer)
{
    assert(handle);
    assert(handle->rxDmaHandle);
    assert(xfer);
    assert(xfer->data);
    assert(xfer->dataSize);

    dma_transfer_config_t xferConfig;
    status_t status;

    /* If previous RX not finished. */
    if (kLPSCI_RxBusy == handle->rxState)
    {
        status = kStatus_LPSCI_RxBusy;
    }
    else
    {
        handle->rxState = kLPSCI_RxBusy;
        handle->rxDataSizeAll = xfer->dataSize;

        /* Prepare transfer. */
        DMA_PrepareTransfer(&xferConfig, (void *)LPSCI_GetDataRegisterAddress(base), sizeof(uint8_t), xfer->data,
                            sizeof(uint8_t), xfer->dataSize, kDMA_PeripheralToMemory);

        /* Submit transfer. */
        DMA_SubmitTransfer(handle->rxDmaHandle, &xferConfig, kDMA_EnableInterrupt);
        DMA_StartTransfer(handle->rxDmaHandle);

        /* Enable LPSCI RX DMA. */
        LPSCI_EnableRxDMA(base, true);

        status = kStatus_Success;
    }

    return status;
}

void LPSCI_TransferAbortSendDMA(UART0_Type *base, lpsci_dma_handle_t *handle)
{
    assert(handle);
    assert(handle->txDmaHandle);

    /* Disable LPSCI TX DMA. */
    LPSCI_EnableTxDMA(base, false);

    /* Stop transfer. */
    DMA_AbortTransfer(handle->txDmaHandle);

    /* Write DMA->DSR[DONE] to abort transfer and clear status. */
    DMA_ClearChannelStatusFlags(handle->txDmaHandle->base, handle->txDmaHandle->channel, kDMA_TransactionsDoneFlag);

    handle->txState = kLPSCI_TxIdle;
}

void LPSCI_TransferAbortReceiveDMA(UART0_Type *base, lpsci_dma_handle_t *handle)
{
    assert(handle);
    assert(handle->rxDmaHandle);

    /* Disable LPSCI RX DMA. */
    LPSCI_EnableRxDMA(base, false);

    /* Stop transfer. */
    DMA_AbortTransfer(handle->rxDmaHandle);

    /* Write DMA->DSR[DONE] to abort transfer and clear status. */
    DMA_ClearChannelStatusFlags(handle->rxDmaHandle->base, handle->rxDmaHandle->channel, kDMA_TransactionsDoneFlag);

    handle->rxState = kLPSCI_RxIdle;
}

status_t LPSCI_TransferGetSendCountDMA(UART0_Type *base, lpsci_dma_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(handle->txDmaHandle);
    assert(count);

    if (kLPSCI_TxIdle == handle->txState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->txDataSizeAll - DMA_GetRemainingBytes(handle->txDmaHandle->base, handle->txDmaHandle->channel);

    return kStatus_Success;
}

status_t LPSCI_TransferGetReceiveCountDMA(UART0_Type *base, lpsci_dma_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(handle->rxDmaHandle);
    assert(count);

    if (kLPSCI_RxIdle == handle->rxState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->rxDataSizeAll - DMA_GetRemainingBytes(handle->rxDmaHandle->base, handle->rxDmaHandle->channel);

    return kStatus_Success;
}
