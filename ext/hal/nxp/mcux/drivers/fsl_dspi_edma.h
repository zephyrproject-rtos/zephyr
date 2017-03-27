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
#ifndef _FSL_DSPI_EDMA_H_
#define _FSL_DSPI_EDMA_H_

#include "fsl_dspi.h"
#include "fsl_edma.h"
/*!
 * @addtogroup dspi_edma_driver
 * @{
 */

/***********************************************************************************************************************
 * Definitions
 **********************************************************************************************************************/

/*!
* @brief Forward declaration of the DSPI eDMA master handle typedefs.
*/
typedef struct _dspi_master_edma_handle dspi_master_edma_handle_t;

/*!
* @brief Forward declaration of the DSPI eDMA slave handle typedefs.
*/
typedef struct _dspi_slave_edma_handle dspi_slave_edma_handle_t;

/*!
 * @brief Completion callback function pointer type.
 *
 * @param base DSPI peripheral base address.
 * @param handle A pointer to the handle for the DSPI master.
 * @param status Success or error code describing whether the transfer completed.
 * @param userData An arbitrary pointer-dataSized value passed from the application.
 */
typedef void (*dspi_master_edma_transfer_callback_t)(SPI_Type *base,
                                                     dspi_master_edma_handle_t *handle,
                                                     status_t status,
                                                     void *userData);
/*!
 * @brief Completion callback function pointer type.
 *
 * @param base DSPI peripheral base address.
 * @param handle A pointer to the handle for the DSPI slave.
 * @param status Success or error code describing whether the transfer completed.
 * @param userData An arbitrary pointer-dataSized value passed from the application.
 */
typedef void (*dspi_slave_edma_transfer_callback_t)(SPI_Type *base,
                                                    dspi_slave_edma_handle_t *handle,
                                                    status_t status,
                                                    void *userData);

/*! @brief DSPI master eDMA transfer handle structure used for the transactional API. */
struct _dspi_master_edma_handle
{
    uint32_t bitsPerFrame;         /*!< The desired number of bits per frame. */
    volatile uint32_t command;     /*!< The desired data command. */
    volatile uint32_t lastCommand; /*!< The desired last data command. */

    uint8_t fifoSize; /*!< FIFO dataSize. */

    volatile bool
        isPcsActiveAfterTransfer; /*!< Indicates whether the PCS signal keeps active after the last frame transfer.*/

    uint8_t nbytes;         /*!< eDMA minor byte transfer count initially configured. */
    volatile uint8_t state; /*!< DSPI transfer state , _dspi_transfer_state.*/

    uint8_t *volatile txData;                  /*!< Send buffer. */
    uint8_t *volatile rxData;                  /*!< Receive buffer. */
    volatile size_t remainingSendByteCount;    /*!< A number of bytes remaining to send.*/
    volatile size_t remainingReceiveByteCount; /*!< A number of bytes remaining to receive.*/
    size_t totalByteCount;                     /*!< A number of transfer bytes*/

    uint32_t rxBuffIfNull; /*!< Used if there is not rxData for DMA purpose.*/
    uint32_t txBuffIfNull; /*!< Used if there is not txData for DMA purpose.*/

    dspi_master_edma_transfer_callback_t callback; /*!< Completion callback. */
    void *userData;                                /*!< Callback user data. */

    edma_handle_t *edmaRxRegToRxDataHandle;        /*!<edma_handle_t handle point used for RxReg to RxData buff*/
    edma_handle_t *edmaTxDataToIntermediaryHandle; /*!<edma_handle_t handle point used for TxData to Intermediary*/
    edma_handle_t *edmaIntermediaryToTxRegHandle;  /*!<edma_handle_t handle point used for Intermediary to TxReg*/

    edma_tcd_t dspiSoftwareTCD[2]; /*!<SoftwareTCD , internal used*/
};

/*! @brief DSPI slave eDMA transfer handle structure used for the transactional API.*/
struct _dspi_slave_edma_handle
{
    uint32_t bitsPerFrame; /*!< The desired number of bits per frame. */

    uint8_t *volatile txData;                  /*!< Send buffer. */
    uint8_t *volatile rxData;                  /*!< Receive buffer. */
    volatile size_t remainingSendByteCount;    /*!< A number of bytes remaining to send.*/
    volatile size_t remainingReceiveByteCount; /*!< A number of bytes remaining to receive.*/
    size_t totalByteCount;                     /*!< A number of transfer bytes*/

    uint32_t rxBuffIfNull; /*!< Used if there is not rxData for DMA purpose.*/
    uint32_t txBuffIfNull; /*!< Used if there is not txData for DMA purpose.*/
    uint32_t txLastData;   /*!< Used if there is an extra byte when 16bits per frame for DMA purpose.*/

    uint8_t nbytes; /*!< eDMA minor byte transfer count initially configured. */

    volatile uint8_t state; /*!< DSPI transfer state.*/

    dspi_slave_edma_transfer_callback_t callback; /*!< Completion callback. */
    void *userData;                               /*!< Callback user data. */

    edma_handle_t *edmaRxRegToRxDataHandle; /*!<edma_handle_t handle point used for RxReg to RxData buff*/
    edma_handle_t *edmaTxDataToTxRegHandle; /*!<edma_handle_t handle point used for TxData to TxReg*/
};

/***********************************************************************************************************************
 * API
 **********************************************************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus*/

/*Transactional APIs*/

/*!
 * @brief Initializes the DSPI master eDMA handle.
 *
 * This function initializes the DSPI eDMA handle which can be used for other DSPI transactional APIs.  Usually, for a
 * specified DSPI instance, call this API once to get the initialized handle.
 *
 * Note that DSPI eDMA has separated (RX and TX as two sources) or shared (RX  and TX are the same source) DMA request
 * source.
 * (1) For the separated DMA request source, enable and set the RX DMAMUX source for edmaRxRegToRxDataHandle and
 * TX DMAMUX source for edmaIntermediaryToTxRegHandle.
 * (2) For the shared DMA request source, enable and set the RX/RX DMAMUX source for the edmaRxRegToRxDataHandle.
 *
 * @param base DSPI peripheral base address.
 * @param handle DSPI handle pointer to dspi_master_edma_handle_t.
 * @param callback DSPI callback.
 * @param userData A callback function parameter.
 * @param edmaRxRegToRxDataHandle edmaRxRegToRxDataHandle pointer to edma_handle_t.
 * @param edmaTxDataToIntermediaryHandle edmaTxDataToIntermediaryHandle pointer to edma_handle_t.
 * @param edmaIntermediaryToTxRegHandle edmaIntermediaryToTxRegHandle pointer to edma_handle_t.
 */
void DSPI_MasterTransferCreateHandleEDMA(SPI_Type *base,
                                         dspi_master_edma_handle_t *handle,
                                         dspi_master_edma_transfer_callback_t callback,
                                         void *userData,
                                         edma_handle_t *edmaRxRegToRxDataHandle,
                                         edma_handle_t *edmaTxDataToIntermediaryHandle,
                                         edma_handle_t *edmaIntermediaryToTxRegHandle);

/*!
 * @brief DSPI master transfer data using eDMA.
 *
 * This function transfers data using eDMA. This is a non-blocking function, which returns right away. When all data
 * is transferred, the callback function is called.
 *
 * @param base DSPI peripheral base address.
 * @param handle A pointer to the dspi_master_edma_handle_t structure which stores the transfer state.
 * @param transfer A pointer to the dspi_transfer_t structure.
 * @return status of status_t.
 */
status_t DSPI_MasterTransferEDMA(SPI_Type *base, dspi_master_edma_handle_t *handle, dspi_transfer_t *transfer);

/*!
 * @brief DSPI master aborts a transfer which is using eDMA.
 *
 * This function aborts a transfer which is using eDMA.
 *
 * @param base DSPI peripheral base address.
 * @param handle A pointer to the dspi_master_edma_handle_t structure which stores the transfer state.
 */
void DSPI_MasterTransferAbortEDMA(SPI_Type *base, dspi_master_edma_handle_t *handle);

/*!
 * @brief Gets the master eDMA transfer count.
 *
 * This function gets the master eDMA transfer count.
 *
 * @param base DSPI peripheral base address.
 * @param handle A pointer to the dspi_master_edma_handle_t structure which stores the transfer state.
 * @param count A number of bytes transferred by the non-blocking transaction.
 * @return status of status_t.
 */
status_t DSPI_MasterTransferGetCountEDMA(SPI_Type *base, dspi_master_edma_handle_t *handle, size_t *count);

/*!
 * @brief Initializes the DSPI slave eDMA handle.
 *
 * This function initializes the DSPI eDMA handle which can be used for other DSPI transactional APIs.  Usually, for a
 * specified DSPI instance, call this API once to get the initialized handle.
 *
 * Note that DSPI eDMA has separated (RN and TX in 2 sources) or shared (RX  and TX are the same source) DMA request
 * source.
 * (1)For the separated DMA request source, enable and set the RX DMAMUX source for edmaRxRegToRxDataHandle and
 * TX DMAMUX source for edmaTxDataToTxRegHandle.
 * (2)For the shared DMA request source,  enable and set the RX/RX DMAMUX source for the edmaRxRegToRxDataHandle.
 *
 * @param base DSPI peripheral base address.
 * @param handle DSPI handle pointer to dspi_slave_edma_handle_t.
 * @param callback DSPI callback.
 * @param userData A callback function parameter.
 * @param edmaRxRegToRxDataHandle edmaRxRegToRxDataHandle pointer to edma_handle_t.
 * @param edmaTxDataToTxRegHandle edmaTxDataToTxRegHandle pointer to edma_handle_t.
 */
void DSPI_SlaveTransferCreateHandleEDMA(SPI_Type *base,
                                        dspi_slave_edma_handle_t *handle,
                                        dspi_slave_edma_transfer_callback_t callback,
                                        void *userData,
                                        edma_handle_t *edmaRxRegToRxDataHandle,
                                        edma_handle_t *edmaTxDataToTxRegHandle);

/*!
 * @brief DSPI slave transfer data using eDMA.
 *
 * This function transfers data using eDMA. This is a non-blocking function, which returns right away. When all data
 * is transferred, the callback function is called.
 * Note that the slave eDMA transfer doesn't support transfer_size is 1 when the bitsPerFrame is greater
 * than eight.

 * @param base DSPI peripheral base address.
 * @param handle A pointer to the dspi_slave_edma_handle_t structure which stores the transfer state.
 * @param transfer A pointer to the dspi_transfer_t structure.
 * @return status of status_t.
 */
status_t DSPI_SlaveTransferEDMA(SPI_Type *base, dspi_slave_edma_handle_t *handle, dspi_transfer_t *transfer);

/*!
 * @brief DSPI slave aborts a transfer which is using eDMA.
 *
 * This function aborts a transfer which is using eDMA.
 *
 * @param base DSPI peripheral base address.
 * @param handle A pointer to the dspi_slave_edma_handle_t structure which stores the transfer state.
 */
void DSPI_SlaveTransferAbortEDMA(SPI_Type *base, dspi_slave_edma_handle_t *handle);

/*!
 * @brief Gets the slave eDMA transfer count.
 *
 * This function gets the slave eDMA transfer count.
 *
 * @param base DSPI peripheral base address.
 * @param handle A pointer to the dspi_slave_edma_handle_t structure which stores the transfer state.
 * @param count A number of bytes transferred so far by the non-blocking transaction.
 * @return status of status_t.
 */
status_t DSPI_SlaveTransferGetCountEDMA(SPI_Type *base, dspi_slave_edma_handle_t *handle, size_t *count);

#if defined(__cplusplus)
}
#endif /*_cplusplus*/
       /*!
        *@}
        */

#endif /*_FSL_DSPI_EDMA_H_*/
