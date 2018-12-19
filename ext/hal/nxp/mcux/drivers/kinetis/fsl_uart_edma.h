/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_UART_EDMA_H_
#define _FSL_UART_EDMA_H_

#include "fsl_uart.h"
#include "fsl_edma.h"

/*!
 * @addtogroup uart_edma_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief UART EDMA driver version 2.1.6. */
#define FSL_UART_EDMA_DRIVER_VERSION (MAKE_VERSION(2, 1, 6))
/*@}*/

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
