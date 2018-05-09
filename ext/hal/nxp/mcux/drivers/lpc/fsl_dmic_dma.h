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
#ifndef _FSL_DMIC_DMA_H_
#define _FSL_DMIC_DMA_H_

#include "fsl_common.h"
#include "fsl_dma.h"

/*!
 * @addtogroup dmic_dma_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief DMIC transfer structure. */
typedef struct _dmic_transfer
{
    uint16_t *data;  /*!< The buffer of data to be transfer.*/
    size_t dataSize; /*!< The byte count to be transfer. */
} dmic_transfer_t;

/* Forward declaration of the handle typedef. */
typedef struct _dmic_dma_handle dmic_dma_handle_t;

/*! @brief DMIC transfer callback function. */
typedef void (*dmic_dma_transfer_callback_t)(DMIC_Type *base,
                                             dmic_dma_handle_t *handle,
                                             status_t status,
                                             void *userData);

/*!
* @brief DMIC DMA handle
*/
struct _dmic_dma_handle
{
    DMIC_Type *base;                       /*!< DMIC peripheral base address. */
    dma_handle_t *rxDmaHandle;             /*!< The DMA RX channel used. */
    dmic_dma_transfer_callback_t callback; /*!< Callback function. */
    void *userData;                        /*!< DMIC callback function parameter.*/
    size_t transferSize;                   /*!< Size of the data to receive. */
    volatile uint8_t state;                /*!< Internal state of DMIC DMA transfer */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* _cplusplus */

/*!
 * @name DMA transactional
 * @{
 */

/*!
 * @brief Initializes the DMIC handle which is used in transactional functions.
 * @param base DMIC peripheral base address.
 * @param handle Pointer to dmic_dma_handle_t structure.
 * @param callback Callback function.
 * @param userData User data.
 * @param rxDmaHandle User-requested DMA handle for RX DMA transfer.
 */
status_t DMIC_TransferCreateHandleDMA(DMIC_Type *base,
                                      dmic_dma_handle_t *handle,
                                      dmic_dma_transfer_callback_t callback,
                                      void *userData,
                                      dma_handle_t *rxDmaHandle);

/*!
 * @brief Receives data using DMA.
 *
 * This function receives data using DMA. This is a non-blocking function, which returns
 * right away. When all data is received, the receive callback function is called.
 *
 * @param base USART peripheral base address.
 * @param handle Pointer to usart_dma_handle_t structure.
 * @param xfer DMIC DMA transfer structure. See #dmic_transfer_t.
 * @param dmic_channel DMIC channel 
 * @retval kStatus_Success
 */
status_t DMIC_TransferReceiveDMA(DMIC_Type *base,
                                 dmic_dma_handle_t *handle,
                                 dmic_transfer_t *xfer,
                                 uint32_t dmic_channel);

/*!
 * @brief Aborts the received data using DMA.
 *
 * This function aborts the received data using DMA.
 *
 * @param base DMIC peripheral base address
 * @param handle Pointer to dmic_dma_handle_t structure
 */
void DMIC_TransferAbortReceiveDMA(DMIC_Type *base, dmic_dma_handle_t *handle);

/*!
 * @brief Get the number of bytes that have been received.
 *
 * This function gets the number of bytes that have been received.
 *
 * @param base DMIC peripheral base address.
 * @param handle DMIC handle pointer.
 * @param count Receive bytes count.
 * @retval kStatus_NoTransferInProgress No receive in progress.
 * @retval kStatus_InvalidArgument Parameter is invalid.
 * @retval kStatus_Success Get successfully through the parameter count;
 */
status_t DMIC_TransferGetReceiveCountDMA(DMIC_Type *base, dmic_dma_handle_t *handle, uint32_t *count);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_DMIC_DMA_H_ */
