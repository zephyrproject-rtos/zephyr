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
#ifndef _FSL_FLEXIO_SPI_EDMA_H_
#define _FSL_FLEXIO_SPI_EDMA_H_

#include "fsl_flexio_spi.h"
#include "fsl_edma.h"

/*!
 * @addtogroup flexio_edma_spi
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief  typedef for flexio_spi_master_edma_handle_t in advance. */
typedef struct _flexio_spi_master_edma_handle flexio_spi_master_edma_handle_t;

/*! @brief  Slave handle is the same with master handle. */
typedef flexio_spi_master_edma_handle_t flexio_spi_slave_edma_handle_t;

/*! @brief FlexIO SPI master callback for finished transmit */
typedef void (*flexio_spi_master_edma_transfer_callback_t)(FLEXIO_SPI_Type *base,
                                                           flexio_spi_master_edma_handle_t *handle,
                                                           status_t status,
                                                           void *userData);

/*! @brief FlexIO SPI slave callback for finished transmit */
typedef void (*flexio_spi_slave_edma_transfer_callback_t)(FLEXIO_SPI_Type *base,
                                                          flexio_spi_slave_edma_handle_t *handle,
                                                          status_t status,
                                                          void *userData);

/*! @brief FlexIO SPI eDMA transfer handle, users should not touch the content of the handle.*/
struct _flexio_spi_master_edma_handle
{
    size_t transferSize;                                 /*!< Total bytes to be transferred. */
    uint8_t nbytes;                                      /*!< eDMA minor byte transfer count initially configured. */
    bool txInProgress;                                   /*!< Send transfer in progress */
    bool rxInProgress;                                   /*!< Receive transfer in progress */
    edma_handle_t *txHandle;                             /*!< DMA handler for SPI send */
    edma_handle_t *rxHandle;                             /*!< DMA handler for SPI receive */
    flexio_spi_master_edma_transfer_callback_t callback; /*!< Callback for SPI DMA transfer */
    void *userData;                                      /*!< User Data for SPI DMA callback */
};

/*******************************************************************************
 * APIs
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name eDMA Transactional
 * @{
 */

/*!
 * @brief Initializes the FlexIO SPI master eDMA handle.
 *
 * This function initializes the FlexIO SPI master eDMA handle which can be used for other FlexIO SPI master transactional
 * APIs.
 * For a specified FlexIO SPI instance, call this API once to get the initialized handle.
 *
 * @param base Pointer to FLEXIO_SPI_Type structure.
 * @param handle Pointer to flexio_spi_master_edma_handle_t structure to store the transfer state.
 * @param callback SPI callback, NULL means no callback.
 * @param userData callback function parameter.
 * @param txHandle User requested eDMA handle for FlexIO SPI RX eDMA transfer.
 * @param rxHandle User requested eDMA handle for FlexIO SPI TX eDMA transfer.
 * @retval kStatus_Success Successfully create the handle.
 * @retval kStatus_OutOfRange The FlexIO SPI eDMA type/handle table out of range.
 */
status_t FLEXIO_SPI_MasterTransferCreateHandleEDMA(FLEXIO_SPI_Type *base,
                                                   flexio_spi_master_edma_handle_t *handle,
                                                   flexio_spi_master_edma_transfer_callback_t callback,
                                                   void *userData,
                                                   edma_handle_t *txHandle,
                                                   edma_handle_t *rxHandle);

/*!
 * @brief Performs a non-blocking FlexIO SPI transfer using eDMA.
 *
 * @note This interface returns immediately after transfer initiates. Call
 * FLEXIO_SPI_MasterGetTransferCountEDMA to poll the transfer status and check
 * whether the FlexIO SPI transfer is finished.
 *
 * @param base Pointer to FLEXIO_SPI_Type structure.
 * @param handle Pointer to flexio_spi_master_edma_handle_t structure to store the transfer state.
 * @param xfer Pointer to FlexIO SPI transfer structure.
 * @retval kStatus_Success Successfully start a transfer.
 * @retval kStatus_InvalidArgument Input argument is invalid.
 * @retval kStatus_FLEXIO_SPI_Busy FlexIO SPI is not idle, is running another transfer.
 */
status_t FLEXIO_SPI_MasterTransferEDMA(FLEXIO_SPI_Type *base,
                                       flexio_spi_master_edma_handle_t *handle,
                                       flexio_spi_transfer_t *xfer);

/*!
 * @brief Aborts a FlexIO SPI transfer using eDMA.
 *
 * @param base Pointer to FLEXIO_SPI_Type structure.
 * @param handle FlexIO SPI eDMA handle pointer.
 */
void FLEXIO_SPI_MasterTransferAbortEDMA(FLEXIO_SPI_Type *base, flexio_spi_master_edma_handle_t *handle);

/*!
 * @brief Gets the remaining bytes for FlexIO SPI eDMA transfer.
 *
 * @param base Pointer to FLEXIO_SPI_Type structure.
 * @param handle FlexIO SPI eDMA handle pointer.
 * @param count Number of bytes transferred so far by the non-blocking transaction.
 */
status_t FLEXIO_SPI_MasterTransferGetCountEDMA(FLEXIO_SPI_Type *base,
                                               flexio_spi_master_edma_handle_t *handle,
                                               size_t *count);

/*!
 * @brief Initializes the FlexIO SPI slave eDMA handle.
 *
 * This function initializes the FlexIO SPI slave eDMA handle.
 *
 * @param base Pointer to FLEXIO_SPI_Type structure.
 * @param handle Pointer to flexio_spi_slave_edma_handle_t structure to store the transfer state.
 * @param callback SPI callback, NULL means no callback.
 * @param userData callback function parameter.
 * @param txHandle User requested eDMA handle for FlexIO SPI TX eDMA transfer.
 * @param rxHandle User requested eDMA handle for FlexIO SPI RX eDMA transfer.
 */
static inline void FLEXIO_SPI_SlaveTransferCreateHandleEDMA(FLEXIO_SPI_Type *base,
                                                            flexio_spi_slave_edma_handle_t *handle,
                                                            flexio_spi_slave_edma_transfer_callback_t callback,
                                                            void *userData,
                                                            edma_handle_t *txHandle,
                                                            edma_handle_t *rxHandle)
{
    FLEXIO_SPI_MasterTransferCreateHandleEDMA(base, handle, callback, userData, txHandle, rxHandle);
}

/*!
 * @brief Performs a non-blocking FlexIO SPI transfer using eDMA.
 *
 * @note This interface returns immediately after transfer initiates. Call
 * FLEXIO_SPI_SlaveGetTransferCountEDMA to poll the transfer status and
 * check whether the FlexIO SPI transfer is finished.
 *
 * @param base Pointer to FLEXIO_SPI_Type structure.
 * @param handle Pointer to flexio_spi_slave_edma_handle_t structure to store the transfer state.
 * @param xfer Pointer to FlexIO SPI transfer structure.
 * @retval kStatus_Success Successfully start a transfer.
 * @retval kStatus_InvalidArgument Input argument is invalid.
 * @retval kStatus_FLEXIO_SPI_Busy FlexIO SPI is not idle, is running another transfer.
 */
status_t FLEXIO_SPI_SlaveTransferEDMA(FLEXIO_SPI_Type *base,
                                      flexio_spi_slave_edma_handle_t *handle,
                                      flexio_spi_transfer_t *xfer);

/*!
 * @brief Aborts a FlexIO SPI transfer using eDMA.
 *
 * @param base Pointer to FLEXIO_SPI_Type structure.
 * @param handle Pointer to flexio_spi_slave_edma_handle_t structure to store the transfer state.
 */
static inline void FLEXIO_SPI_SlaveTransferAbortEDMA(FLEXIO_SPI_Type *base, flexio_spi_slave_edma_handle_t *handle)
{
    FLEXIO_SPI_MasterTransferAbortEDMA(base, handle);
}

/*!
 * @brief Gets the remaining bytes to be transferred for FlexIO SPI eDMA.
 *
 * @param base Pointer to FLEXIO_SPI_Type structure.
 * @param handle FlexIO SPI eDMA handle pointer.
 * @param count Number of bytes transferred so far by the non-blocking transaction.
 */
static inline status_t FLEXIO_SPI_SlaveTransferGetCountEDMA(FLEXIO_SPI_Type *base,
                                                            flexio_spi_slave_edma_handle_t *handle,
                                                            size_t *count)
{
    return FLEXIO_SPI_MasterTransferGetCountEDMA(base, handle, count);
}

/*! @} */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */
#endif
