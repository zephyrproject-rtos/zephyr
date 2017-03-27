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

#include "fsl_spi_dma.h"

/*******************************************************************************
 * Definitons
 ******************************************************************************/
/*<! Structure definition for spi_dma_private_handle_t. The structure is private. */
typedef struct _spi_dma_private_handle
{
    SPI_Type *base;
    spi_dma_handle_t *handle;
} spi_dma_private_handle_t;

/*! @brief SPI transfer state, which is used for SPI transactiaonl APIs' internal state. */
enum _spi_dma_states_t
{
    kSPI_Idle = 0x0, /*!< SPI is idle state */
    kSPI_Busy        /*!< SPI is busy tranferring data. */
};

/*<! Private handle only used for internally. */
static spi_dma_private_handle_t s_dmaPrivateHandle[FSL_FEATURE_SOC_SPI_COUNT];
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get the instance for SPI module.
 *
 * @param base SPI base address
 */
extern uint32_t SPI_GetInstance(SPI_Type *base);

/*!
 * @brief DMA callback function for SPI send transfer.
 *
 * @param handle DMA handle pointer.
 * @param userData User data for DMA callback function.
 */
static void SPI_TxDMACallback(dma_handle_t *handle, void *userData);

/*!
 * @brief DMA callback function for SPI receive transfer.
 *
 * @param handle DMA handle pointer.
 * @param userData User data for DMA callback function.
 */
static void SPI_RxDMACallback(dma_handle_t *handle, void *userData);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Dummy data used to send */
static const uint8_t s_dummyData = SPI_DUMMYDATA;

/*******************************************************************************
* Code
******************************************************************************/
static void SPI_TxDMACallback(dma_handle_t *handle, void *userData)
{
    spi_dma_private_handle_t *privHandle = (spi_dma_private_handle_t *)userData;
    spi_dma_handle_t *spiHandle = privHandle->handle;
    SPI_Type *base = privHandle->base;

    /* Disable Tx dma */
    SPI_EnableDMA(base, kSPI_TxDmaEnable, false);

    /* Stop DMA tranfer */
    DMA_StopTransfer(spiHandle->txHandle);

    /* change the state */
    spiHandle->txInProgress = false;

    /* All finished, call the callback */
    if ((spiHandle->txInProgress == false) && (spiHandle->rxInProgress == false))
    {
        spiHandle->state = kSPI_Idle;
        if (spiHandle->callback)
        {
            (spiHandle->callback)(base, spiHandle, kStatus_Success, spiHandle->userData);
        }
    }
}

static void SPI_RxDMACallback(dma_handle_t *handle, void *userData)
{
    spi_dma_private_handle_t *privHandle = (spi_dma_private_handle_t *)userData;
    spi_dma_handle_t *spiHandle = privHandle->handle;
    SPI_Type *base = privHandle->base;

    /* Disable Tx dma */
    SPI_EnableDMA(base, kSPI_RxDmaEnable, false);

    /* Stop DMA tranfer */
    DMA_StopTransfer(spiHandle->rxHandle);

    /* change the state */
    spiHandle->rxInProgress = false;

    /* All finished, call the callback */
    if ((spiHandle->txInProgress == false) && (spiHandle->rxInProgress == false))
    {
        spiHandle->state = kSPI_Idle;
        if (spiHandle->callback)
        {
            (spiHandle->callback)(base, spiHandle, kStatus_Success, spiHandle->userData);
        }
    }
}

void SPI_MasterTransferCreateHandleDMA(SPI_Type *base,
                                       spi_dma_handle_t *handle,
                                       spi_dma_callback_t callback,
                                       void *userData,
                                       dma_handle_t *txHandle,
                                       dma_handle_t *rxHandle)
{
    assert(handle);
    dma_transfer_config_t config = {0};
    uint32_t instance = SPI_GetInstance(base);

    /* Zero the handle */
    memset(handle, 0, sizeof(*handle));

    /* Set spi base to handle */
    handle->txHandle = txHandle;
    handle->rxHandle = rxHandle;
    handle->callback = callback;
    handle->userData = userData;

    /* Set SPI state to idle */
    handle->state = kSPI_Idle;

    /* Set handle to global state */
    s_dmaPrivateHandle[instance].base = base;
    s_dmaPrivateHandle[instance].handle = handle;

/* Compute internal state */
#if defined(FSL_FEATURE_SPI_16BIT_TRANSFERS) && (FSL_FEATURE_SPI_16BIT_TRANSFERS)
    handle->bytesPerFrame = ((base->C2 & SPI_C2_SPIMODE_MASK) >> SPI_C2_SPIMODE_SHIFT) + 1U;
#else
    handle->bytesPerFrame = 1U;
#endif /* FSL_FEATURE_SPI_16BIT_TRANSFERS */

#if defined(FSL_FEATURE_SPI_HAS_FIFO) && (FSL_FEATURE_SPI_HAS_FIFO)
    /* If using DMA, disable FIFO, as the FIFO may cause data loss if the data size is not integer
       times of 2bytes. As SPI cannot set watermark to 0, only can set to 1/2 FIFO size or 3/4 FIFO
       size. */
    if (FSL_FEATURE_SPI_FIFO_SIZEn(base) != 0)
    {
        base->C3 &= ~SPI_C3_FIFOMODE_MASK;
    }

#endif /* FSL_FEATURE_SPI_HAS_FIFO */

    /* Set the non-change attribute for Tx DMA transfer, to improve efficiency */
    config.destAddr = SPI_GetDataRegisterAddress(base);
    config.enableDestIncrement = false;
    config.enableSrcIncrement = true;
    if (handle->bytesPerFrame == 1U)
    {
        config.srcSize = kDMA_Transfersize8bits;
        config.destSize = kDMA_Transfersize8bits;
    }
    else
    {
        config.srcSize = kDMA_Transfersize16bits;
        config.destSize = kDMA_Transfersize16bits;
    }

    DMA_SubmitTransfer(handle->txHandle, &config, true);

    /* Set non-change attribute for Rx DMA */
    config.srcAddr = SPI_GetDataRegisterAddress(base);
    config.destAddr = 0U;
    config.enableDestIncrement = true;
    config.enableSrcIncrement = false;
    DMA_SubmitTransfer(handle->rxHandle, &config, true);

    /* Install callback for Tx dma channel */
    DMA_SetCallback(handle->txHandle, SPI_TxDMACallback, &s_dmaPrivateHandle[instance]);
    DMA_SetCallback(handle->rxHandle, SPI_RxDMACallback, &s_dmaPrivateHandle[instance]);
}

status_t SPI_MasterTransferDMA(SPI_Type *base, spi_dma_handle_t *handle, spi_transfer_t *xfer)
{
    assert(handle && xfer);

    dma_transfer_config_t config = {0};

    /* Check if the device is busy */
    if (handle->state == kSPI_Busy)
    {
        return kStatus_SPI_Busy;
    }

    /* Check if input parameter invalid */
    if (((xfer->txData == NULL) && (xfer->rxData == NULL)) || (xfer->dataSize == 0U))
    {
        return kStatus_InvalidArgument;
    }
    
    /* Disable SPI and then enable it, this is used to clear S register*/
    SPI_Enable(base, false);
    SPI_Enable(base, true);
    
    /* Configure tx transfer DMA */
    config.destAddr = SPI_GetDataRegisterAddress(base);
    config.enableDestIncrement = false;
    if (handle->bytesPerFrame == 1U)
    {
        config.srcSize = kDMA_Transfersize8bits;
        config.destSize = kDMA_Transfersize8bits;
    }
    else
    {
        config.srcSize = kDMA_Transfersize16bits;
        config.destSize = kDMA_Transfersize16bits;
    }
    config.transferSize = xfer->dataSize;
    /* Configure DMA channel */
    if (xfer->txData)
    {
        config.enableSrcIncrement = true;
        config.srcAddr = (uint32_t)(xfer->txData);
    }
    else
    {
        /* Disable the source increasement and source set to dummyData */
        config.enableSrcIncrement = false;
        config.srcAddr = (uint32_t)(&s_dummyData);
    }
    DMA_SubmitTransfer(handle->txHandle, &config, true);

    /* Handle rx transfer */
    if (xfer->rxData)
    {
        /* Set the source address */
        DMA_SetDestinationAddress(handle->rxHandle->base, handle->rxHandle->channel, (uint32_t)(xfer->rxData));

        /* Set the transfer size */
        DMA_SetTransferSize(handle->rxHandle->base, handle->rxHandle->channel, xfer->dataSize);
    }

    /* Change the state of handle */
    handle->transferSize = xfer->dataSize;
    handle->state = kSPI_Busy;

    /* Start Rx transfer if needed */
    if (xfer->rxData)
    {
        handle->rxInProgress = true;
        SPI_EnableDMA(base, kSPI_RxDmaEnable, true);
        DMA_StartTransfer(handle->rxHandle);
    }

    /* Always start Tx transfer */
    handle->txInProgress = true;
    SPI_EnableDMA(base, kSPI_TxDmaEnable, true);
    DMA_StartTransfer(handle->txHandle);

    return kStatus_Success;
}

status_t SPI_MasterTransferGetCountDMA(SPI_Type *base, spi_dma_handle_t *handle, size_t *count)
{
    assert(handle);

    status_t status = kStatus_Success;

    if (handle->state != kSPI_Busy)
    {
        status = kStatus_NoTransferInProgress;
    }
    else
    {
        if (handle->rxInProgress)
        {
            *count = handle->transferSize - DMA_GetRemainingBytes(handle->rxHandle->base, handle->rxHandle->channel);
        }
        else
        {
            *count = handle->transferSize - DMA_GetRemainingBytes(handle->txHandle->base, handle->txHandle->channel);
        }
    }

    return status;
}

void SPI_MasterTransferAbortDMA(SPI_Type *base, spi_dma_handle_t *handle)
{
    assert(handle);

    /* Disable dma */
    DMA_StopTransfer(handle->txHandle);
    DMA_StopTransfer(handle->rxHandle);

    /* Disable DMA enable bit */
    SPI_EnableDMA(base, kSPI_DmaAllEnable, false);

    /* Set the handle state */
    handle->txInProgress = false;
    handle->rxInProgress = false;
    handle->state = kSPI_Idle;
}
