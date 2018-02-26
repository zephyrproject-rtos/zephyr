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
#ifndef _FSL_UART_DMA_H_
#define _FSL_UART_DMA_H_

#include "fsl_uart.h"
#include "fsl_dmamux.h"
#include "fsl_dma.h"

/*!
 * @addtogroup uart_dma_driver
 * @{
 */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Forward declaration of the handle typedef. */
typedef struct _uart_dma_handle uart_dma_handle_t;

/*! @brief UART transfer callback function. */
typedef void (*uart_dma_transfer_callback_t)(UART_Type *base,
                                             uart_dma_handle_t *handle,
                                             status_t status,
                                             void *userData);

/*!
* @brief UART DMA handle
*/
struct _uart_dma_handle
{
    UART_Type *base; /*!< UART peripheral base address. */

    uart_dma_transfer_callback_t callback; /*!< Callback function. */
    void *userData;                        /*!< UART callback function parameter.*/
    size_t rxDataSizeAll;                  /*!< Size of the data to receive. */
    size_t txDataSizeAll;                  /*!< Size of the data to send out. */

    dma_handle_t *txDmaHandle; /*!< The DMA TX channel used. */
    dma_handle_t *rxDmaHandle; /*!< The DMA RX channel used. */

    volatile uint8_t txState; /*!< TX transfer state. */
    volatile uint8_t rxState; /*!< RX transfer state */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name eDMA transactional
 * @{
 */

/*!
 * @brief Initializes the UART handle which is used in transactional functions and sets the callback.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_dma_handle_t structure.
 * @param callback UART callback, NULL means no callback.
 * @param userData User callback function data.
 * @param rxDmaHandle User requested DMA handle for the RX DMA transfer.
 * @param txDmaHandle User requested DMA handle for the TX DMA transfer.
 */
void UART_TransferCreateHandleDMA(UART_Type *base,
                          uart_dma_handle_t *handle,
                          uart_dma_transfer_callback_t callback,
                          void *userData,
                          dma_handle_t *txDmaHandle,
                          dma_handle_t *rxDmaHandle);

/*!
 * @brief Sends data using DMA.
 *
 * This function sends data using DMA. This is non-blocking function, which returns
 * right away. When all data is sent, the send callback function is called.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param xfer UART DMA transfer structure. See #uart_transfer_t.
 * @retval kStatus_Success if succeeded; otherwise failed.
 * @retval kStatus_UART_TxBusy Previous transfer ongoing.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t UART_TransferSendDMA(UART_Type *base, uart_dma_handle_t *handle, uart_transfer_t *xfer);

/*!
 * @brief Receives data using DMA.
 *
 * This function receives data using DMA. This is non-blocking function, which returns
 * right away. When all data is received, the receive callback function is called.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_dma_handle_t structure.
 * @param xfer UART DMA transfer structure. See #uart_transfer_t.
 * @retval kStatus_Success if succeeded; otherwise failed.
 * @retval kStatus_UART_RxBusy Previous transfer on going.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t UART_TransferReceiveDMA(UART_Type *base, uart_dma_handle_t *handle, uart_transfer_t *xfer);

/*!
 * @brief Aborts the send data using DMA.
 *
 * This function aborts the sent data using DMA.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to uart_dma_handle_t structure.
 */
void UART_TransferAbortSendDMA(UART_Type *base, uart_dma_handle_t *handle);

/*!
 * @brief Aborts the received data using DMA.
 *
 * This function abort receive data which using DMA.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to uart_dma_handle_t structure.
 */
void UART_TransferAbortReceiveDMA(UART_Type *base, uart_dma_handle_t *handle);

/*!
 * @brief Gets the number of bytes written to UART TX register.
 *
 * This function gets the number of bytes written to UART TX
 * register by DMA.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param count Send bytes count.
 * @retval kStatus_NoTransferInProgress No send in progress.
 * @retval kStatus_InvalidArgument Parameter is invalid.
 * @retval kStatus_Success Get successfully through the parameter \p count;
 */
status_t UART_TransferGetSendCountDMA(UART_Type *base, uart_dma_handle_t *handle, uint32_t *count);

/*!
 * @brief Gets the number of bytes that have been received.
 *
 * This function gets the number of bytes that have been received.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param count Receive bytes count.
 * @retval kStatus_NoTransferInProgress No receive in progress.
 * @retval kStatus_InvalidArgument Parameter is invalid.
 * @retval kStatus_Success Get successfully through the parameter \p count;
 */
status_t UART_TransferGetReceiveCountDMA(UART_Type *base, uart_dma_handle_t *handle, uint32_t *count);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_UART_DMA_H_ */
