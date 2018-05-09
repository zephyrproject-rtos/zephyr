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

typedef struct _spi_dma_txdummy
{
    uint32_t lastWord;
    uint32_t word;
} spi_dma_txdummy_t;

/*<! Private handle only used for internally. */
static spi_dma_private_handle_t s_dmaPrivateHandle[FSL_FEATURE_SOC_SPI_COUNT];
/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
* @brief SPI private function to return SPI configuration
*
* @param base SPI base address.
*/
void *SPI_GetConfig(SPI_Type *base);

/*!
 * @brief DMA callback function for SPI send transfer.
 *
 * @param handle DMA handle pointer.
 * @param userData User data for DMA callback function.
 */
static void SPI_TxDMACallback(dma_handle_t *handle, void *userData, bool transferDone, uint32_t intmode);

/*!
 * @brief DMA callback function for SPI receive transfer.
 *
 * @param handle DMA handle pointer.
 * @param userData User data for DMA callback function.
 */
static void SPI_RxDMACallback(dma_handle_t *handle, void *userData, bool transferDone, uint32_t intmode);

/*******************************************************************************
 * Variables
 ******************************************************************************/
#if defined(__ICCARM__)
#pragma data_alignment = 4
static spi_dma_txdummy_t s_txDummy[FSL_FEATURE_SOC_SPI_COUNT] = {0};
#elif defined(__CC_ARM)
__attribute__((aligned(4))) static spi_dma_txdummy_t s_txDummy[FSL_FEATURE_SOC_SPI_COUNT] = {0};
#elif defined(__GNUC__)
__attribute__((aligned(4))) static spi_dma_txdummy_t s_txDummy[FSL_FEATURE_SOC_SPI_COUNT] = {0};
#endif

#if defined(__ICCARM__)
#pragma data_alignment = 4
static uint16_t s_rxDummy;
#elif defined(__CC_ARM)
__attribute__((aligned(4))) static uint16_t s_rxDummy;
#elif defined(__GNUC__)
__attribute__((aligned(4))) static uint16_t s_rxDummy;
#endif

#if defined(__ICCARM__)
#pragma data_alignment = 16
static dma_descriptor_t s_spi_descriptor_table[FSL_FEATURE_SOC_SPI_COUNT] = {0};
#elif defined(__CC_ARM)
__attribute__((aligned(16))) static dma_descriptor_t s_spi_descriptor_table[FSL_FEATURE_SOC_SPI_COUNT] = {0};
#elif defined(__GNUC__)
__attribute__((aligned(16))) static dma_descriptor_t s_spi_descriptor_table[FSL_FEATURE_SOC_SPI_COUNT] = {0};
#endif

/*******************************************************************************
* Code
******************************************************************************/

static void XferToFifoWR(spi_transfer_t *xfer, uint32_t *fifowr)
{
    *fifowr |= xfer->configFlags & (uint32_t)kSPI_FrameDelay ? (uint32_t)kSPI_FrameDelay : 0;
    *fifowr |= xfer->configFlags & (uint32_t)kSPI_FrameAssert ? (uint32_t)kSPI_FrameAssert : 0;
}

static void SpiConfigToFifoWR(spi_config_t *config, uint32_t *fifowr)
{
    *fifowr |= (SPI_DEASSERT_ALL & (~SPI_DEASSERTNUM_SSEL(config->sselNum)));
    /* set width of data - range asserted at entry */
    *fifowr |= SPI_FIFOWR_LEN(config->dataWidth);
}

static void PrepareTxFIFO(uint32_t *fifo, uint32_t count, uint32_t ctrl)
{
    assert(!(fifo == NULL));
    if (fifo == NULL)
    {
        return;
    }
    /* CS deassert and CS delay are relevant only for last word */
    uint32_t tx_ctrl = ctrl & (~(SPI_FIFOWR_EOT_MASK | SPI_FIFOWR_EOF_MASK));
    uint32_t i = 0;
    for (; i + 1 < count; i++)
    {
        fifo[i] = (fifo[i] & 0xFFFFU) | (tx_ctrl & 0xFFFF0000U);
    }
    if (i < count)
    {
        fifo[i] = (fifo[i] & 0xFFFFU) | (ctrl & 0xFFFF0000U);
    }
}

static void SPI_SetupDummy(uint32_t *dummy, spi_transfer_t *xfer, spi_config_t *spi_config_p)
{
    *dummy = SPI_DUMMYDATA;
    XferToFifoWR(xfer, dummy);
    SpiConfigToFifoWR(spi_config_p, dummy);
}

status_t SPI_MasterTransferCreateHandleDMA(SPI_Type *base,
                                           spi_dma_handle_t *handle,
                                           spi_dma_callback_t callback,
                                           void *userData,
                                           dma_handle_t *txHandle,
                                           dma_handle_t *rxHandle)
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

    instance = SPI_GetInstance(base);

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

    /* Install callback for Tx dma channel */
    DMA_SetCallback(handle->txHandle, SPI_TxDMACallback, &s_dmaPrivateHandle[instance]);
    DMA_SetCallback(handle->rxHandle, SPI_RxDMACallback, &s_dmaPrivateHandle[instance]);

    return kStatus_Success;
}

status_t SPI_MasterTransferDMA(SPI_Type *base, spi_dma_handle_t *handle, spi_transfer_t *xfer)
{
    int32_t instance;
    status_t result = kStatus_Success;
    spi_config_t *spi_config_p;

    assert(!((NULL == handle) || (NULL == xfer)));
    if ((NULL == handle) || (NULL == xfer))
    {
        return kStatus_InvalidArgument;
    }
    /* txData set and not aligned to sizeof(uint32_t) */
    assert(!((NULL != xfer->txData) && ((uint32_t)xfer->txData % sizeof(uint32_t))));
    if ((NULL != xfer->txData) && ((uint32_t)xfer->txData % sizeof(uint32_t)))
    {
        return kStatus_InvalidArgument;
    }
    /* rxData set and not aligned to sizeof(uint32_t) */
    assert(!((NULL != xfer->rxData) && ((uint32_t)xfer->rxData % sizeof(uint32_t))));
    if ((NULL != xfer->rxData) && ((uint32_t)xfer->rxData % sizeof(uint32_t)))
    {
        return kStatus_InvalidArgument;
    }
    /* byte size is zero or not aligned to sizeof(uint32_t) */
    assert(!((xfer->dataSize == 0) || (xfer->dataSize % sizeof(uint32_t))));
    if ((xfer->dataSize == 0) || (xfer->dataSize % sizeof(uint32_t)))
    {
        return kStatus_InvalidArgument;
    }
    /* cannot get instance from base address */
    instance = SPI_GetInstance(base);
    assert(!(instance < 0));
    if (instance < 0)
    {
        return kStatus_InvalidArgument;
    }

    /* Check if the device is busy */
    if (handle->state == kSPI_Busy)
    {
        return kStatus_SPI_Busy;
    }
    else
    {
        uint32_t tmp;
        dma_transfer_config_t xferConfig = {0};
        spi_config_p = (spi_config_t *)SPI_GetConfig(base);

        handle->state = kStatus_SPI_Busy;
        handle->transferSize = xfer->dataSize;

        /* receive */
        SPI_EnableRxDMA(base, true);
        if (xfer->rxData)
        {
            DMA_PrepareTransfer(&xferConfig, (void *)&base->FIFORD, xfer->rxData, sizeof(uint32_t), xfer->dataSize,
                                kDMA_PeripheralToMemory, NULL);
        }
        else
        {
            DMA_PrepareTransfer(&xferConfig, (void *)&base->FIFORD, &s_rxDummy, sizeof(uint32_t), xfer->dataSize,
                                kDMA_StaticToStatic, NULL);
        }
        DMA_SubmitTransfer(handle->rxHandle, &xferConfig);
        handle->rxInProgress = true;
        DMA_StartTransfer(handle->rxHandle);

        /* transmit */
        SPI_EnableTxDMA(base, true);
        if (xfer->txData)
        {
            tmp = 0;
            XferToFifoWR(xfer, &tmp);
            SpiConfigToFifoWR(spi_config_p, &tmp);
            PrepareTxFIFO((uint32_t *)xfer->txData, xfer->dataSize / sizeof(uint32_t), tmp);
            DMA_PrepareTransfer(&xferConfig, xfer->txData, (void *)&base->FIFOWR, sizeof(uint32_t), xfer->dataSize,
                                kDMA_MemoryToPeripheral, NULL);
            DMA_SubmitTransfer(handle->txHandle, &xferConfig);
        }
        else
        {
            if ((xfer->configFlags & kSPI_FrameAssert) && (xfer->dataSize > sizeof(uint32_t)))
            {
                dma_xfercfg_t tmp_xfercfg = { 0 };
                tmp_xfercfg.valid = true;
                tmp_xfercfg.swtrig = true;
                tmp_xfercfg.intA = true;
                tmp_xfercfg.byteWidth = sizeof(uint32_t);
                tmp_xfercfg.srcInc = 0;
                tmp_xfercfg.dstInc = 0;
                tmp_xfercfg.transferCount = 1;
                /* create chained descriptor to transmit last word */
                SPI_SetupDummy(&s_txDummy[instance].lastWord, xfer, spi_config_p);
                DMA_CreateDescriptor(&s_spi_descriptor_table[instance], &tmp_xfercfg, &s_txDummy[instance].lastWord,
                                     (uint32_t *)&base->FIFOWR, NULL);
                /* use common API to setup first descriptor */
                SPI_SetupDummy(&s_txDummy[instance].word, NULL, spi_config_p);
                DMA_PrepareTransfer(&xferConfig, &s_txDummy[instance].word, (void *)&base->FIFOWR, sizeof(uint32_t),
                                    xfer->dataSize - sizeof(uint32_t), kDMA_StaticToStatic,
                                    &s_spi_descriptor_table[instance]);
                /* disable interrupts for first descriptor
                 * to avoid calling callback twice */
                xferConfig.xfercfg.intA = false;
                xferConfig.xfercfg.intB = false;
                result = DMA_SubmitTransfer(handle->txHandle, &xferConfig);
                if (result != kStatus_Success)
                {
                    return result;
                }
            }
            else
            {
                SPI_SetupDummy(&s_txDummy[instance].word, xfer, spi_config_p);
                DMA_PrepareTransfer(&xferConfig, &s_txDummy[instance].word, (void *)&base->FIFOWR, sizeof(uint32_t),
                                    xfer->dataSize, kDMA_StaticToStatic, NULL);
                result = DMA_SubmitTransfer(handle->txHandle, &xferConfig);
                if (result != kStatus_Success)
                {
                    return result;
                }
            }
        }
        handle->txInProgress = true;
        DMA_StartTransfer(handle->txHandle);
    }

    return result;
}

static void SPI_RxDMACallback(dma_handle_t *handle, void *userData, bool transferDone, uint32_t intmode)
{
    spi_dma_private_handle_t *privHandle = (spi_dma_private_handle_t *)userData;
    spi_dma_handle_t *spiHandle = privHandle->handle;
    SPI_Type *base = privHandle->base;

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

static void SPI_TxDMACallback(dma_handle_t *handle, void *userData, bool transferDone, uint32_t intmode)
{
    spi_dma_private_handle_t *privHandle = (spi_dma_private_handle_t *)userData;
    spi_dma_handle_t *spiHandle = privHandle->handle;
    SPI_Type *base = privHandle->base;

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

void SPI_MasterTransferAbortDMA(SPI_Type *base, spi_dma_handle_t *handle)
{
    assert(NULL != handle);

    /* Stop tx transfer first */
    DMA_AbortTransfer(handle->txHandle);
    /* Then rx transfer */
    DMA_AbortTransfer(handle->rxHandle);

    /* Set the handle state */
    handle->txInProgress = false;
    handle->rxInProgress = false;
    handle->state = kSPI_Idle;
}

status_t SPI_MasterTransferGetCountDMA(SPI_Type *base, spi_dma_handle_t *handle, size_t *count)
{
    assert(handle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    /* Catch when there is not an active transfer. */
    if (handle->state != kSPI_Busy)
    {
        *count = 0;
        return kStatus_NoTransferInProgress;
    }

    size_t bytes;

    bytes = DMA_GetRemainingBytes(handle->rxHandle->base, handle->rxHandle->channel);

    *count = handle->transferSize - bytes;

    return kStatus_Success;
}
