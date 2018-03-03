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

#include "fsl_lpuart_edma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*<! Structure definition for lpuart_edma_private_handle_t. The structure is private. */
typedef struct _lpuart_edma_private_handle
{
    LPUART_Type *base;
    lpuart_edma_handle_t *handle;
} lpuart_edma_private_handle_t;

/* LPUART EDMA transfer handle. */
enum _lpuart_edma_tansfer_states
{
    kLPUART_TxIdle, /* TX idle. */
    kLPUART_TxBusy, /* TX busy. */
    kLPUART_RxIdle, /* RX idle. */
    kLPUART_RxBusy  /* RX busy. */
};

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Array of LPUART handle. */
#if (defined(LPUART8))
#define LPUART_HANDLE_ARRAY_SIZE 9
#else /* LPUART8 */
#if (defined(LPUART7))
#define LPUART_HANDLE_ARRAY_SIZE 8
#else /* LPUART7 */
#if (defined(LPUART6))
#define LPUART_HANDLE_ARRAY_SIZE 7
#else /* LPUART6 */
#if (defined(LPUART5))
#define LPUART_HANDLE_ARRAY_SIZE 6
#else /* LPUART5 */
#if (defined(LPUART4))
#define LPUART_HANDLE_ARRAY_SIZE 5
#else /* LPUART4 */
#if (defined(LPUART3))
#define LPUART_HANDLE_ARRAY_SIZE 4
#else /* LPUART3 */
#if (defined(LPUART2))
#define LPUART_HANDLE_ARRAY_SIZE 3
#else /* LPUART2 */
#if (defined(LPUART1))
#define LPUART_HANDLE_ARRAY_SIZE 2
#else /* LPUART1 */
#if (defined(LPUART0))
#define LPUART_HANDLE_ARRAY_SIZE 1
#else /* LPUART0 */
#define LPUART_HANDLE_ARRAY_SIZE FSL_FEATURE_SOC_LPUART_COUNT
#endif /* LPUART 0 */
#endif /* LPUART 1 */
#endif /* LPUART 2 */
#endif /* LPUART 3 */
#endif /* LPUART 4 */
#endif /* LPUART 5 */
#endif /* LPUART 6 */
#endif /* LPUART 7 */
#endif /* LPUART 8 */

/*<! Private handle only used for internally. */
static lpuart_edma_private_handle_t s_edmaPrivateHandle[LPUART_HANDLE_ARRAY_SIZE];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief LPUART EDMA send finished callback function.
 *
 * This function is called when LPUART EDMA send finished. It disables the LPUART
 * TX EDMA request and sends @ref kStatus_LPUART_TxIdle to LPUART callback.
 *
 * @param handle The EDMA handle.
 * @param param Callback function parameter.
 */
static void LPUART_SendEDMACallback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds);

/*!
 * @brief LPUART EDMA receive finished callback function.
 *
 * This function is called when LPUART EDMA receive finished. It disables the LPUART
 * RX EDMA request and sends @ref kStatus_LPUART_RxIdle to LPUART callback.
 *
 * @param handle The EDMA handle.
 * @param param Callback function parameter.
 */
static void LPUART_ReceiveEDMACallback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds);

/*!
 * @brief Get the LPUART instance from peripheral base address.
 *
 * @param base LPUART peripheral base address.
 * @return LPUART instance.
 */
extern uint32_t LPUART_GetInstance(LPUART_Type *base);

/*******************************************************************************
 * Code
 ******************************************************************************/

static void LPUART_SendEDMACallback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
    assert(param);

    lpuart_edma_private_handle_t *lpuartPrivateHandle = (lpuart_edma_private_handle_t *)param;

    /* Avoid the warning for unused variables. */
    handle = handle;
    tcds = tcds;

    if (transferDone)
    {
        LPUART_TransferAbortSendEDMA(lpuartPrivateHandle->base, lpuartPrivateHandle->handle);

        if (lpuartPrivateHandle->handle->callback)
        {
            lpuartPrivateHandle->handle->callback(lpuartPrivateHandle->base, lpuartPrivateHandle->handle,
                                                  kStatus_LPUART_TxIdle, lpuartPrivateHandle->handle->userData);
        }
    }
}

static void LPUART_ReceiveEDMACallback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
    assert(param);

    lpuart_edma_private_handle_t *lpuartPrivateHandle = (lpuart_edma_private_handle_t *)param;

    /* Avoid warning for unused parameters. */
    handle = handle;
    tcds = tcds;

    if (transferDone)
    {
        /* Disable transfer. */
        LPUART_TransferAbortReceiveEDMA(lpuartPrivateHandle->base, lpuartPrivateHandle->handle);

        if (lpuartPrivateHandle->handle->callback)
        {
            lpuartPrivateHandle->handle->callback(lpuartPrivateHandle->base, lpuartPrivateHandle->handle,
                                                  kStatus_LPUART_RxIdle, lpuartPrivateHandle->handle->userData);
        }
    }
}

void LPUART_TransferCreateHandleEDMA(LPUART_Type *base,
                                     lpuart_edma_handle_t *handle,
                                     lpuart_edma_transfer_callback_t callback,
                                     void *userData,
                                     edma_handle_t *txEdmaHandle,
                                     edma_handle_t *rxEdmaHandle)
{
    assert(handle);

    uint32_t instance = LPUART_GetInstance(base);

    s_edmaPrivateHandle[instance].base = base;
    s_edmaPrivateHandle[instance].handle = handle;

    memset(handle, 0, sizeof(*handle));

    handle->rxState = kLPUART_RxIdle;
    handle->txState = kLPUART_TxIdle;

    handle->rxEdmaHandle = rxEdmaHandle;
    handle->txEdmaHandle = txEdmaHandle;

    handle->callback = callback;
    handle->userData = userData;

#if defined(FSL_FEATURE_LPUART_HAS_FIFO) && FSL_FEATURE_LPUART_HAS_FIFO
    /* Note:
       Take care of the RX FIFO, EDMA request only assert when received bytes
       equal or more than RX water mark, there is potential issue if RX water
       mark larger than 1.
       For example, if RX FIFO water mark is 2, upper layer needs 5 bytes and
       5 bytes are received. the last byte will be saved in FIFO but not trigger
       EDMA transfer because the water mark is 2.
     */
    if (rxEdmaHandle)
    {
        base->WATER &= (~LPUART_WATER_RXWATER_MASK);
    }
#endif

    /* Configure TX. */
    if (txEdmaHandle)
    {
        EDMA_SetCallback(handle->txEdmaHandle, LPUART_SendEDMACallback, &s_edmaPrivateHandle[instance]);
    }

    /* Configure RX. */
    if (rxEdmaHandle)
    {
        EDMA_SetCallback(handle->rxEdmaHandle, LPUART_ReceiveEDMACallback, &s_edmaPrivateHandle[instance]);
    }
}

status_t LPUART_SendEDMA(LPUART_Type *base, lpuart_edma_handle_t *handle, lpuart_transfer_t *xfer)
{
    assert(handle);
    assert(handle->txEdmaHandle);
    assert(xfer);
    assert(xfer->data);
    assert(xfer->dataSize);

    edma_transfer_config_t xferConfig;
    status_t status;

    /* If previous TX not finished. */
    if (kLPUART_TxBusy == handle->txState)
    {
        status = kStatus_LPUART_TxBusy;
    }
    else
    {
        handle->txState = kLPUART_TxBusy;
        handle->txDataSizeAll = xfer->dataSize;

        /* Prepare transfer. */
        EDMA_PrepareTransfer(&xferConfig, xfer->data, sizeof(uint8_t), (void *)LPUART_GetDataRegisterAddress(base),
                             sizeof(uint8_t), sizeof(uint8_t), xfer->dataSize, kEDMA_MemoryToPeripheral);

        /* Store the initially configured eDMA minor byte transfer count into the LPUART handle */
        handle->nbytes = sizeof(uint8_t);

        /* Submit transfer. */
        EDMA_SubmitTransfer(handle->txEdmaHandle, &xferConfig);
        EDMA_StartTransfer(handle->txEdmaHandle);

        /* Enable LPUART TX EDMA. */
        LPUART_EnableTxDMA(base, true);

        status = kStatus_Success;
    }

    return status;
}

status_t LPUART_ReceiveEDMA(LPUART_Type *base, lpuart_edma_handle_t *handle, lpuart_transfer_t *xfer)
{
    assert(handle);
    assert(handle->rxEdmaHandle);
    assert(xfer);
    assert(xfer->data);
    assert(xfer->dataSize);

    edma_transfer_config_t xferConfig;
    status_t status;

    /* If previous RX not finished. */
    if (kLPUART_RxBusy == handle->rxState)
    {
        status = kStatus_LPUART_RxBusy;
    }
    else
    {
        handle->rxState = kLPUART_RxBusy;
        handle->rxDataSizeAll = xfer->dataSize;

        /* Prepare transfer. */
        EDMA_PrepareTransfer(&xferConfig, (void *)LPUART_GetDataRegisterAddress(base), sizeof(uint8_t), xfer->data,
                             sizeof(uint8_t), sizeof(uint8_t), xfer->dataSize, kEDMA_PeripheralToMemory);

        /* Store the initially configured eDMA minor byte transfer count into the LPUART handle */
        handle->nbytes = sizeof(uint8_t);

        /* Submit transfer. */
        EDMA_SubmitTransfer(handle->rxEdmaHandle, &xferConfig);
        EDMA_StartTransfer(handle->rxEdmaHandle);

        /* Enable LPUART RX EDMA. */
        LPUART_EnableRxDMA(base, true);

        status = kStatus_Success;
    }

    return status;
}

void LPUART_TransferAbortSendEDMA(LPUART_Type *base, lpuart_edma_handle_t *handle)
{
    assert(handle);
    assert(handle->txEdmaHandle);

    /* Disable LPUART TX EDMA. */
    LPUART_EnableTxDMA(base, false);

    /* Stop transfer. */
    EDMA_AbortTransfer(handle->txEdmaHandle);

    handle->txState = kLPUART_TxIdle;
}

void LPUART_TransferAbortReceiveEDMA(LPUART_Type *base, lpuart_edma_handle_t *handle)
{
    assert(handle);
    assert(handle->rxEdmaHandle);

    /* Disable LPUART RX EDMA. */
    LPUART_EnableRxDMA(base, false);

    /* Stop transfer. */
    EDMA_AbortTransfer(handle->rxEdmaHandle);

    handle->rxState = kLPUART_RxIdle;
}

status_t LPUART_TransferGetReceiveCountEDMA(LPUART_Type *base, lpuart_edma_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(handle->rxEdmaHandle);
    assert(count);

    if (kLPUART_RxIdle == handle->rxState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->rxDataSizeAll -
             (uint32_t)handle->nbytes *
                 EDMA_GetRemainingMajorLoopCount(handle->rxEdmaHandle->base, handle->rxEdmaHandle->channel);

    return kStatus_Success;
}

status_t LPUART_TransferGetSendCountEDMA(LPUART_Type *base, lpuart_edma_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(handle->txEdmaHandle);
    assert(count);

    if (kLPUART_TxIdle == handle->txState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->txDataSizeAll -
             (uint32_t)handle->nbytes *
                 EDMA_GetRemainingMajorLoopCount(handle->txEdmaHandle->base, handle->txEdmaHandle->channel);

    return kStatus_Success;
}
