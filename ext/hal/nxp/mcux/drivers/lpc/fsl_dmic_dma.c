/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
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

#include "fsl_dmic_dma.h"
#include "fsl_dmic.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
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
/*!
 * @brief Get the DMIC instance from peripheral base address.
 *
 * @param base DMIC peripheral base address.
 * @return DMIC instance.
 */
extern uint32_t DMIC_GetInstance(DMIC_Type *base);
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
        DMA_PrepareTransfer(&xferConfig, (void *)&base->CHANNEL[dmic_channel].FIFO_DATA, xfer->data, sizeof(uint16_t),
                            xfer->dataSize, kDMA_PeripheralToMemory, NULL);

        /* Submit transfer. */
        DMA_SubmitTransfer(handle->rxDmaHandle, &xferConfig);

        DMA_StartTransfer(handle->rxDmaHandle);

        status = kStatus_Success;
    }
    return status;
}

void DMIC_TransferAbortReceiveDMA(DMIC_Type *base, dmic_dma_handle_t *handle)
{
    assert(NULL != handle);
    assert(NULL != handle->rxDmaHandle);

    /* Stop transfer. */
    DMA_AbortTransfer(handle->rxDmaHandle);
    handle->state = kDMIC_Idle;
}

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
