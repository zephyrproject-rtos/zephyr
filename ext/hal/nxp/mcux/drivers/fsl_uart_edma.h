/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
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
#ifndef _FSL_UART_EDMA_H_
#define _FSL_UART_EDMA_H_

#include "fsl_uart.h"
#include "fsl_dmamux.h"
#include "fsl_edma.h"

/*!
 * @addtogroup uart_edma_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Forward declaration of the handle typedef. */
typedef struct _uart_edma_handle uart_edma_handle_t;

/*! @brief UART transfer callback function. */
typedef void (*uart_edma_transfer_callback_t)(UART_Type *base,
                                              uart_edma_handle_t *handle,
                                              status_t status,
                                              void *userData);

/*!
* @brief UART eDMA handle
*/
struct _uart_edma_handle
{
    uart_edma_transfer_callback_t callback; /*!< Callback function. */
    void *userData;                         /*!< UART callback function parameter.*/
    size_t rxDataSizeAll;                   /*!< Size of the data to receive. */
    size_t txDataSizeAll;                   /*!< Size of the data to send out. */

    edma_handle_t *txEdmaHandle; /*!< The eDMA TX channel used. */
    edma_handle_t *rxEdmaHandle; /*!< The eDMA RX channel used. */

    uint8_t nbytes; /*!< eDMA minor byte transfer count initially configured. */

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
 * @brief Initializes the UART handle which is used in transactional functions.
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_edma_handle_t structure.
 * @param callback UART callback, NULL means no callback.
 * @param userData User callback function data.
 * @param rxEdmaHandle User-requested DMA handle for RX DMA transfer.
 * @param txEdmaHandle User-requested DMA handle for TX DMA transfer.
 */
void UART_TransferCreateHandleEDMA(UART_Type *base,
                                   uart_edma_handle_t *handle,
                                   uart_edma_transfer_callback_t callback,
                                   void *userData,
                                   edma_handle_t *txEdmaHandle,
                                   edma_handle_t *rxEdmaHandle);

/*!
 * @brief Sends data using eDMA.
 *
 * This function sends data using eDMA. This is a non-blocking function, which returns
 * right away. When all data is sent, the send callback function is called.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param xfer UART eDMA transfer structure. See #uart_transfer_t.
 * @retval kStatus_Success if succeeded; otherwise failed.
 * @retval kStatus_UART_TxBusy Previous transfer ongoing.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t UART_SendEDMA(UART_Type *base, uart_edma_handle_t *handle, uart_transfer_t *xfer);

/*!
 * @brief Receives data using eDMA.
 *
 * This function receives data using eDMA. This is a non-blocking function, which returns
 * right away. When all data is received, the receive callback function is called.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_edma_handle_t structure.
 * @param xfer UART eDMA transfer structure. See #uart_transfer_t.
 * @retval kStatus_Success if succeeded; otherwise failed.
 * @retval kStatus_UART_RxBusy Previous transfer ongoing.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t UART_ReceiveEDMA(UART_Type *base, uart_edma_handle_t *handle, uart_transfer_t *xfer);

/*!
 * @brief Aborts the sent data using eDMA.
 *
 * This function aborts sent data using eDMA.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_edma_handle_t structure.
 */
void UART_TransferAbortSendEDMA(UART_Type *base, uart_edma_handle_t *handle);

/*!
 * @brief Aborts the receive data using eDMA.
 *
 * This function aborts receive data using eDMA.
 *
 * @param base UART peripheral base address.
 * @param handle Pointer to the uart_edma_handle_t structure.
 */
void UART_TransferAbortReceiveEDMA(UART_Type *base, uart_edma_handle_t *handle);

/*!
 * @brief Gets the number of bytes that have been written to UART TX register.
 *
 * This function gets the number of bytes that have been written to UART TX
 * register by DMA.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param count Send bytes count.
 * @retval kStatus_NoTransferInProgress No send in progress.
 * @retval kStatus_InvalidArgument Parameter is invalid.
 * @retval kStatus_Success Get successfully through the parameter \p count;
 */
status_t UART_TransferGetSendCountEDMA(UART_Type *base, uart_edma_handle_t *handle, uint32_t *count);

/*!
 * @brief Gets the number of received bytes.
 *
 * This function gets the number of received bytes.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param count Receive bytes count.
 * @retval kStatus_NoTransferInProgress No receive in progress.
 * @retval kStatus_InvalidArgument Parameter is invalid.
 * @retval kStatus_Success Get successfully through the parameter \p count;
 */
status_t UART_TransferGetReceiveCountEDMA(UART_Type *base, uart_edma_handle_t *handle, uint32_t *count);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_UART_EDMA_H_ */
