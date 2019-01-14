/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_USART_DMA_H_
#define _FSL_USART_DMA_H_

#include "fsl_common.h"
#include "fsl_dma.h"
#include "fsl_usart.h"

/*!
 * @addtogroup usart_dma_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief USART dma driver version 2.0.1. */
#define FSL_USART_DMA_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/* Forward declaration of the handle typedef. */
typedef struct _usart_dma_handle usart_dma_handle_t;

/*! @brief UART transfer callback function. */
typedef void (*usart_dma_transfer_callback_t)(USART_Type *base,
                                              usart_dma_handle_t *handle,
                                              status_t status,
                                              void *userData);

/*!
* @brief UART DMA handle
*/
struct _usart_dma_handle
{
    USART_Type *base; /*!< UART peripheral base address. */

    usart_dma_transfer_callback_t callback; /*!< Callback function. */
    void *userData;                         /*!< UART callback function parameter.*/
    size_t rxDataSizeAll;                   /*!< Size of the data to receive. */
    size_t txDataSizeAll;                   /*!< Size of the data to send out. */

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
#endif /* _cplusplus */

/*!
 * @name DMA transactional
 * @{
 */

/*!
 * @brief Initializes the USART handle which is used in transactional functions.
 * @param base USART peripheral base address.
 * @param handle Pointer to usart_dma_handle_t structure.
 * @param callback Callback function.
 * @param userData User data.
 * @param txDmaHandle User-requested DMA handle for TX DMA transfer.
 * @param rxDmaHandle User-requested DMA handle for RX DMA transfer.
 */
status_t USART_TransferCreateHandleDMA(USART_Type *base,
                                       usart_dma_handle_t *handle,
                                       usart_dma_transfer_callback_t callback,
                                       void *userData,
                                       dma_handle_t *txDmaHandle,
                                       dma_handle_t *rxDmaHandle);

/*!
 * @brief Sends data using DMA.
 *
 * This function sends data using DMA. This is a non-blocking function, which returns
 * right away. When all data is sent, the send callback function is called.
 *
 * @param base USART peripheral base address.
 * @param handle USART handle pointer.
 * @param xfer USART DMA transfer structure. See #usart_transfer_t.
 * @retval kStatus_Success if succeed, others failed.
 * @retval kStatus_USART_TxBusy Previous transfer on going.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t USART_TransferSendDMA(USART_Type *base, usart_dma_handle_t *handle, usart_transfer_t *xfer);

/*!
 * @brief Receives data using DMA.
 *
 * This function receives data using DMA. This is a non-blocking function, which returns
 * right away. When all data is received, the receive callback function is called.
 *
 * @param base USART peripheral base address.
 * @param handle Pointer to usart_dma_handle_t structure.
 * @param xfer USART DMA transfer structure. See #usart_transfer_t.
 * @retval kStatus_Success if succeed, others failed.
 * @retval kStatus_USART_RxBusy Previous transfer on going.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t USART_TransferReceiveDMA(USART_Type *base, usart_dma_handle_t *handle, usart_transfer_t *xfer);

/*!
 * @brief Aborts the sent data using DMA.
 *
 * This function aborts send data using DMA.
 *
 * @param base USART peripheral base address
 * @param handle Pointer to usart_dma_handle_t structure
 */
void USART_TransferAbortSendDMA(USART_Type *base, usart_dma_handle_t *handle);

/*!
 * @brief Aborts the received data using DMA.
 *
 * This function aborts the received data using DMA.
 *
 * @param base USART peripheral base address
 * @param handle Pointer to usart_dma_handle_t structure
 */
void USART_TransferAbortReceiveDMA(USART_Type *base, usart_dma_handle_t *handle);

/*!
 * @brief Get the number of bytes that have been received.
 *
 * This function gets the number of bytes that have been received.
 *
 * @param base USART peripheral base address.
 * @param handle USART handle pointer.
 * @param count Receive bytes count.
 * @retval kStatus_NoTransferInProgress No receive in progress.
 * @retval kStatus_InvalidArgument Parameter is invalid.
 * @retval kStatus_Success Get successfully through the parameter \p count;
 */
status_t USART_TransferGetReceiveCountDMA(USART_Type *base, usart_dma_handle_t *handle, uint32_t *count);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_USART_DMA_H_ */
