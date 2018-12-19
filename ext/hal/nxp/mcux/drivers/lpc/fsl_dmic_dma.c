/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_dmic_dma.h"
#include "fsl_dmic.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.dmic_dma"
#endif

#define DMIC_HANDLE_ARRAY_SIZE 1

/*<! Structure definition for dmic_dma_handle_t. The structure is private. */
typedef struct _dmic_dma_private_handle
{
    DMIC_Type *base;
    dmic_dma_handle_t *handle;
} dmic_dma_private_handle_t;

/*! @brief DMIC transfer state, which is used for DMIC transactiaonl APIs' internal state. */
enum _dmic_dma_states_t
{
    kDMIC_Idle = 0x0, /*!< DMIC is idle state */
    kDMIC_Busy        /*!< DMIC is busy tranferring data. */
};
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*<! Private handle only used for internally. */
static dmic_dma_private_handle_t s_dmaPrivateHandle[DMIC_HANDLE_ARRAY_SIZE];

/*******************************************************************************
 * Code
********************************************************************************/

static void DMIC_TransferReceiveDMACallback(dma_handle_t *handle, void *param, bool transferDone, uint32_t intmode)
{
    assert(handle);
    assert(param);

    dmic_dma_private_handle_t *dmicPrivateHandle = (dmic_dma_private_handle_t *)param;
    dmicPrivateHandle->handle->state = kDMIC_Idle;

    if (dmicPrivateHandle->handle->callback)
    {
        dmicPrivateHandle->handle->callback(dmicPrivateHandle->base, dmicPrivateHandle->handle, kStatus_DMIC_Idle,
                                            dmicPrivateHandle->handle->userData);
    }
}

/*!
 * brief Initializes the DMIC handle which is used in transactional functions.
 * param base DMIC peripheral base address.
 * param handle Pointer to dmic_dma_handle_t structure.
 * param callback Callback function.
 * param userData User data.
 * param rxDmaHandle User-requested DMA handle for RX DMA transfer.
 */
status_t DMIC_TransferCreateHandleDMA(DMIC_Type *base,
                                      dmic_dma_handle_t *handle,
                                      dmic_dma_transfer_callback_t callback,
                                      void *userData,
                                      dma_handle_t *rxDmaHandle)
{
    int32_t instance = 0;

    /* check 'base' */
    assert(!(NULL == base));
    if (NULL == base)
    {
        return kStatus_InvalidArgument;
    }
    /* check 'handle' */
    assert(!(NULL == handle));
    if (NULL == handle)
    {
        return kStatus_InvalidArgument;
    }
    /* check DMIC instance by 'base'*/
    instance = DMIC_GetInstance(base);
    assert(!(instance < 0));
    if (instance < 0)
    {
        return kStatus_InvalidArgument;
    }

    memset(handle, 0, sizeof(*handle));
    /* assign 'base' and 'handle' */
    s_dmaPrivateHandle[instance].base = base;
    s_dmaPrivateHandle[instance].handle = handle;

    handle->callback = callback;
    handle->userData = userData;

    handle->rxDmaHandle = rxDmaHandle;

    /* Set DMIC state to idle */
    handle->state = kDMIC_Idle;
    /* Configure RX. */
    if (rxDmaHandle)
    {
        DMA_SetCallback(rxDmaHandle, DMIC_TransferReceiveDMACallback, &s_dmaPrivateHandle[instance]);
    }

    return kStatus_Success;
}

/*!
 * brief Receives data using DMA.
 *
 * This function receives data using DMA. This is a non-blocking function, which returns
 * right away. When all data is received, the receive callback function is called.
 *
 * param base USART peripheral base address.
 * param handle Pointer to usart_dma_handle_t structure.
 * param xfer DMIC DMA transfer structure. See #dmic_transfer_t.
 * param dmic_channel DMIC channel
 * retval kStatus_Success
 */
status_t DMIC_TransferReceiveDMA(DMIC_Type *base,
                                 dmic_dma_handle_t *handle,
                                 dmic_transfer_t *xfer,
                                 uint32_t dmic_channel)
{
    assert(handle);
    assert(handle->rxDmaHandle);
    assert(xfer);
    assert(xfer->data);
    assert(xfer->dataSize);

    dma_transfer_config_t xferConfig;
    status_t status;
    uint32_t srcAddr = (uint32_t)(&base->CHANNEL[dmic_channel].FIFO_DATA);

    /* Check if the device is busy. If previous RX not finished.*/
    if (handle->state == kDMIC_Busy)
    {
        status = kStatus_DMIC_Busy;
    }
    else
    {
        handle->state = kDMIC_Busy;
        handle->transferSize = xfer->dataSize;

        /* Prepare transfer. */
        DMA_PrepareTransfer(&xferConfig, (void *)srcAddr, xfer->data, sizeof(uint16_t), xfer->dataSize,
                            kDMA_PeripheralToMemory, NULL);

        /* Submit transfer. */
        DMA_SubmitTransfer(handle->rxDmaHandle, &xferConfig);

        DMA_StartTransfer(handle->rxDmaHandle);

        status = kStatus_Success;
    }
    return status;
}

/*!
 * brief Aborts the received data using DMA.
 *
 * This function aborts the received data using DMA.
 *
 * param base DMIC peripheral base address
 * param handle Pointer to dmic_dma_handle_t structure
 */
void DMIC_TransferAbortReceiveDMA(DMIC_Type *base, dmic_dma_handle_t *handle)
{
    assert(NULL != handle);
    assert(NULL != handle->rxDmaHandle);

    /* Stop transfer. */
    DMA_AbortTransfer(handle->rxDmaHandle);
    handle->state = kDMIC_Idle;
}

/*!
 * brief Get the number of bytes that have been received.
 *
 * This function gets the number of bytes that have been received.
 *
 * param base DMIC peripheral base address.
 * param handle DMIC handle pointer.
 * param count Receive bytes count.
 * retval kStatus_NoTransferInProgress No receive in progress.
 * retval kStatus_InvalidArgument Parameter is invalid.
 * retval kStatus_Success Get successfully through the parameter count;
 */
status_t DMIC_TransferGetReceiveCountDMA(DMIC_Type *base, dmic_dma_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(handle->rxDmaHandle);
    assert(count);

    if (kDMIC_Idle == handle->state)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->transferSize - DMA_GetRemainingBytes(handle->rxDmaHandle->base, handle->rxDmaHandle->channel);

    return kStatus_Success;
}
