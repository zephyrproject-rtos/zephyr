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

#include "fsl_flexio_i2s_edma.h"

/*******************************************************************************
 * Definitations
 ******************************************************************************/
/* Used for 32byte aligned */
#define STCD_ADDR(address) (edma_tcd_t *)(((uint32_t)address + 32) & ~0x1FU)

/*<! Structure definition for flexio_i2s_edma_private_handle_t. The structure is private. */
typedef struct _flexio_i2s_edma_private_handle
{
    FLEXIO_I2S_Type *base;
    flexio_i2s_edma_handle_t *handle;
} flexio_i2s_edma_private_handle_t;

enum _flexio_i2s_edma_transfer_state
{
    kFLEXIO_I2S_Busy = 0x0U, /*!< FLEXIO I2S is busy */
    kFLEXIO_I2S_Idle,        /*!< Transfer is done. */
};

/*<! Private handle only used for internally. */
static flexio_i2s_edma_private_handle_t s_edmaPrivateHandle[2];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief FLEXIO I2S EDMA callback for send.
 *
 * @param handle pointer to flexio_i2s_edma_handle_t structure which stores the transfer state.
 * @param userData Parameter for user callback.
 * @param done If the DMA transfer finished.
 * @param tcds The TCD index.
 */
static void FLEXIO_I2S_TxEDMACallback(edma_handle_t *handle, void *userData, bool done, uint32_t tcds);

/*!
 * @brief FLEXIO I2S EDMA callback for receive.
 *
 * @param handle pointer to flexio_i2s_edma_handle_t structure which stores the transfer state.
 * @param userData Parameter for user callback.
 * @param done If the DMA transfer finished.
 * @param tcds The TCD index.
 */
static void FLEXIO_I2S_RxEDMACallback(edma_handle_t *handle, void *userData, bool done, uint32_t tcds);

/*******************************************************************************
* Code
******************************************************************************/
static void FLEXIO_I2S_TxEDMACallback(edma_handle_t *handle, void *userData, bool done, uint32_t tcds)
{
    flexio_i2s_edma_private_handle_t *privHandle = (flexio_i2s_edma_private_handle_t *)userData;
    flexio_i2s_edma_handle_t *flexio_i2sHandle = privHandle->handle;

    /* If finished a blcok, call the callback function */
    memset(&flexio_i2sHandle->queue[flexio_i2sHandle->queueDriver], 0, sizeof(flexio_i2s_transfer_t));
    flexio_i2sHandle->queueDriver = (flexio_i2sHandle->queueDriver + 1) % FLEXIO_I2S_XFER_QUEUE_SIZE;
    if (flexio_i2sHandle->callback)
    {
        (flexio_i2sHandle->callback)(privHandle->base, flexio_i2sHandle, kStatus_Success, flexio_i2sHandle->userData);
    }

    /* If all data finished, just stop the transfer */
    if (flexio_i2sHandle->queue[flexio_i2sHandle->queueDriver].data == NULL)
    {
        FLEXIO_I2S_TransferAbortSendEDMA(privHandle->base, flexio_i2sHandle);
    }
}

static void FLEXIO_I2S_RxEDMACallback(edma_handle_t *handle, void *userData, bool done, uint32_t tcds)
{
    flexio_i2s_edma_private_handle_t *privHandle = (flexio_i2s_edma_private_handle_t *)userData;
    flexio_i2s_edma_handle_t *flexio_i2sHandle = privHandle->handle;

    /* If finished a blcok, call the callback function */
    memset(&flexio_i2sHandle->queue[flexio_i2sHandle->queueDriver], 0, sizeof(flexio_i2s_transfer_t));
    flexio_i2sHandle->queueDriver = (flexio_i2sHandle->queueDriver + 1) % FLEXIO_I2S_XFER_QUEUE_SIZE;
    if (flexio_i2sHandle->callback)
    {
        (flexio_i2sHandle->callback)(privHandle->base, flexio_i2sHandle, kStatus_Success, flexio_i2sHandle->userData);
    }

    /* If all data finished, just stop the transfer */
    if (flexio_i2sHandle->queue[flexio_i2sHandle->queueDriver].data == NULL)
    {
        FLEXIO_I2S_TransferAbortReceiveEDMA(privHandle->base, flexio_i2sHandle);
    }
}

void FLEXIO_I2S_TransferTxCreateHandleEDMA(FLEXIO_I2S_Type *base,
                                           flexio_i2s_edma_handle_t *handle,
                                           flexio_i2s_edma_callback_t callback,
                                           void *userData,
                                           edma_handle_t *dmaHandle)
{
    assert(handle && dmaHandle);

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Set flexio_i2s base to handle */
    handle->dmaHandle = dmaHandle;
    handle->callback = callback;
    handle->userData = userData;

    /* Set FLEXIO I2S state to idle */
    handle->state = kFLEXIO_I2S_Idle;

    s_edmaPrivateHandle[0].base = base;
    s_edmaPrivateHandle[0].handle = handle;

    /* Need to use scatter gather */
    EDMA_InstallTCDMemory(dmaHandle, STCD_ADDR(handle->tcd), FLEXIO_I2S_XFER_QUEUE_SIZE);

    /* Install callback for Tx dma channel */
    EDMA_SetCallback(dmaHandle, FLEXIO_I2S_TxEDMACallback, &s_edmaPrivateHandle[0]);
}

void FLEXIO_I2S_TransferRxCreateHandleEDMA(FLEXIO_I2S_Type *base,
                                           flexio_i2s_edma_handle_t *handle,
                                           flexio_i2s_edma_callback_t callback,
                                           void *userData,
                                           edma_handle_t *dmaHandle)
{
    assert(handle && dmaHandle);

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Set flexio_i2s base to handle */
    handle->dmaHandle = dmaHandle;
    handle->callback = callback;
    handle->userData = userData;

    /* Set FLEXIO I2S state to idle */
    handle->state = kFLEXIO_I2S_Idle;

    s_edmaPrivateHandle[1].base = base;
    s_edmaPrivateHandle[1].handle = handle;

    /* Need to use scatter gather */
    EDMA_InstallTCDMemory(dmaHandle, STCD_ADDR(handle->tcd), FLEXIO_I2S_XFER_QUEUE_SIZE);

    /* Install callback for Tx dma channel */
    EDMA_SetCallback(dmaHandle, FLEXIO_I2S_RxEDMACallback, &s_edmaPrivateHandle[1]);
}

void FLEXIO_I2S_TransferSetFormatEDMA(FLEXIO_I2S_Type *base,
                                      flexio_i2s_edma_handle_t *handle,
                                      flexio_i2s_format_t *format,
                                      uint32_t srcClock_Hz)
{
    assert(handle && format);

    /* Configure the audio format to FLEXIO I2S registers */
    if (srcClock_Hz != 0)
    {
        /* It is master */
        FLEXIO_I2S_MasterSetFormat(base, format, srcClock_Hz);
    }
    else
    {
        FLEXIO_I2S_SlaveSetFormat(base, format);
    }

    /* Get the tranfer size from format, this should be used in EDMA configuration */
    handle->bytesPerFrame = format->bitWidth / 8U;
}

status_t FLEXIO_I2S_TransferSendEDMA(FLEXIO_I2S_Type *base,
                                     flexio_i2s_edma_handle_t *handle,
                                     flexio_i2s_transfer_t *xfer)
{
    assert(handle && xfer);

    edma_transfer_config_t config = {0};
    uint32_t destAddr = FLEXIO_I2S_TxGetDataRegisterAddress(base) + (4U - handle->bytesPerFrame);

    /* Check if input parameter invalid */
    if ((xfer->data == NULL) || (xfer->dataSize == 0U))
    {
        return kStatus_InvalidArgument;
    }

    if (handle->queue[handle->queueUser].data)
    {
        return kStatus_FLEXIO_I2S_QueueFull;
    }

    /* Change the state of handle */
    handle->state = kFLEXIO_I2S_Busy;

    /* Update the queue state */
    handle->queue[handle->queueUser].data = xfer->data;
    handle->queue[handle->queueUser].dataSize = xfer->dataSize;
    handle->transferSize[handle->queueUser] = xfer->dataSize;
    handle->queueUser = (handle->queueUser + 1) % FLEXIO_I2S_XFER_QUEUE_SIZE;

    /* Prepare edma configure */
    EDMA_PrepareTransfer(&config, xfer->data, handle->bytesPerFrame, (void *)destAddr, handle->bytesPerFrame,
                         handle->bytesPerFrame, xfer->dataSize, kEDMA_MemoryToPeripheral);

    /* Store the initially configured eDMA minor byte transfer count into the FLEXIO I2S handle */
    handle->nbytes = handle->bytesPerFrame;

    EDMA_SubmitTransfer(handle->dmaHandle, &config);

    /* Start DMA transfer */
    EDMA_StartTransfer(handle->dmaHandle);

    /* Enable DMA enable bit */
    FLEXIO_I2S_TxEnableDMA(base, true);

    /* Enable FLEXIO I2S Tx clock */
    FLEXIO_I2S_Enable(base, true);

    return kStatus_Success;
}

status_t FLEXIO_I2S_TransferReceiveEDMA(FLEXIO_I2S_Type *base,
                                        flexio_i2s_edma_handle_t *handle,
                                        flexio_i2s_transfer_t *xfer)
{
    assert(handle && xfer);

    edma_transfer_config_t config = {0};
    uint32_t srcAddr = FLEXIO_I2S_RxGetDataRegisterAddress(base) + (4U - handle->bytesPerFrame);

    /* Check if input parameter invalid */
    if ((xfer->data == NULL) || (xfer->dataSize == 0U))
    {
        return kStatus_InvalidArgument;
    }

    if (handle->queue[handle->queueUser].data)
    {
        return kStatus_FLEXIO_I2S_QueueFull;
    }

    /* Change the state of handle */
    handle->state = kFLEXIO_I2S_Busy;

    /* Update queue state  */
    handle->queue[handle->queueUser].data = xfer->data;
    handle->queue[handle->queueUser].dataSize = xfer->dataSize;
    handle->transferSize[handle->queueUser] = xfer->dataSize;
    handle->queueUser = (handle->queueUser + 1) % FLEXIO_I2S_XFER_QUEUE_SIZE;

    /* Prepare edma configure */
    EDMA_PrepareTransfer(&config, (void *)srcAddr, handle->bytesPerFrame, xfer->data, handle->bytesPerFrame,
                         handle->bytesPerFrame, xfer->dataSize, kEDMA_PeripheralToMemory);

    /* Store the initially configured eDMA minor byte transfer count into the FLEXIO I2S handle */
    handle->nbytes = handle->bytesPerFrame;

    EDMA_SubmitTransfer(handle->dmaHandle, &config);

    /* Start DMA transfer */
    EDMA_StartTransfer(handle->dmaHandle);

    /* Enable DMA enable bit */
    FLEXIO_I2S_RxEnableDMA(base, true);

    /* Enable FLEXIO I2S Rx clock */
    FLEXIO_I2S_Enable(base, true);

    return kStatus_Success;
}

void FLEXIO_I2S_TransferAbortSendEDMA(FLEXIO_I2S_Type *base, flexio_i2s_edma_handle_t *handle)
{
    assert(handle);

    /* Disable dma */
    EDMA_AbortTransfer(handle->dmaHandle);

    /* Disable DMA enable bit */
    FLEXIO_I2S_TxEnableDMA(base, false);

    /* Set the handle state */
    handle->state = kFLEXIO_I2S_Idle;
}

void FLEXIO_I2S_TransferAbortReceiveEDMA(FLEXIO_I2S_Type *base, flexio_i2s_edma_handle_t *handle)
{
    assert(handle);

    /* Disable dma */
    EDMA_AbortTransfer(handle->dmaHandle);

    /* Disable DMA enable bit */
    FLEXIO_I2S_RxEnableDMA(base, false);

    /* Set the handle state */
    handle->state = kFLEXIO_I2S_Idle;
}

status_t FLEXIO_I2S_TransferGetSendCountEDMA(FLEXIO_I2S_Type *base, flexio_i2s_edma_handle_t *handle, size_t *count)
{
    assert(handle);

    status_t status = kStatus_Success;

    if (handle->state != kFLEXIO_I2S_Busy)
    {
        status = kStatus_NoTransferInProgress;
    }
    else
    {
        *count = handle->transferSize[handle->queueDriver] -
                 (uint32_t)handle->nbytes *
                     EDMA_GetRemainingMajorLoopCount(handle->dmaHandle->base, handle->dmaHandle->channel);
    }

    return status;
}

status_t FLEXIO_I2S_TransferGetReceiveCountEDMA(FLEXIO_I2S_Type *base, flexio_i2s_edma_handle_t *handle, size_t *count)
{
    assert(handle);

    status_t status = kStatus_Success;

    if (handle->state != kFLEXIO_I2S_Busy)
    {
        status = kStatus_NoTransferInProgress;
    }
    else
    {
        *count = handle->transferSize[handle->queueDriver] -
                 (uint32_t)handle->nbytes *
                     EDMA_GetRemainingMajorLoopCount(handle->dmaHandle->base, handle->dmaHandle->channel);
    }

    return status;
}
