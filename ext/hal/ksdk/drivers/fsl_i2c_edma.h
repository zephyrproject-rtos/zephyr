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
#ifndef _FSL_I2C_DMA_H_
#define _FSL_I2C_DMA_H_

#include "fsl_i2c.h"
#include "fsl_dmamux.h"
#include "fsl_edma.h"

/*!
 * @addtogroup i2c_edma_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief I2C master edma handle typedef. */
typedef struct _i2c_master_edma_handle i2c_master_edma_handle_t;

/*! @brief I2C master edma transfer callback typedef. */
typedef void (*i2c_master_edma_transfer_callback_t)(I2C_Type *base,
                                                    i2c_master_edma_handle_t *handle,
                                                    status_t status,
                                                    void *userData);

/*! @brief I2C master edma transfer structure. */
struct _i2c_master_edma_handle
{
    i2c_master_transfer_t transfer; /*!< I2C master transfer struct. */
    size_t transferSize;            /*!< Total bytes to be transferred. */
    uint8_t state;                  /*!< I2C master transfer status. */
    edma_handle_t *dmaHandle;       /*!< The eDMA handler used. */
    i2c_master_edma_transfer_callback_t
        completionCallback; /*!< Callback function called after edma transfer finished. */
    void *userData;         /*!< Callback parameter passed to callback function. */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus. */

/*!
 * @name I2C Block EDMA Transfer Operation
 * @{
 */

/*!
 * @brief Init the I2C handle which is used in transcational functions.
 *
 * @param base I2C peripheral base address.
 * @param handle pointer to i2c_master_edma_handle_t structure.
 * @param callback pointer to user callback function.
 * @param userData user param passed to the callback function.
 * @param edmaHandle EDMA handle pointer.
 */
void I2C_MasterCreateEDMAHandle(I2C_Type *base,
                                i2c_master_edma_handle_t *handle,
                                i2c_master_edma_transfer_callback_t callback,
                                void *userData,
                                edma_handle_t *edmaHandle);

/*!
 * @brief Performs a master edma non-blocking transfer on the I2C bus.
 *
 * @param base I2C peripheral base address.
 * @param handle pointer to i2c_master_edma_handle_t structure.
 * @param xfer pointer to transfer structure of i2c_master_transfer_t.
 * @retval kStatus_Success Sucessully complete the data transmission.
 * @retval kStatus_I2C_Busy Previous transmission still not finished.
 * @retval kStatus_I2C_Timeout Transfer error, wait signal timeout.
 * @retval kStatus_I2C_ArbitrationLost Transfer error, arbitration lost.
 * @retval kStataus_I2C_Nak Transfer error, receive Nak during transfer.
 */
status_t I2C_MasterTransferEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle, i2c_master_transfer_t *xfer);

/*!
 * @brief Get master transfer status during a edma non-blocking transfer.
 *
 * @param base I2C peripheral base address.
 * @param handle pointer to i2c_master_edma_handle_t structure.
 * @param count Number of bytes transferred so far by the non-blocking transaction.
 */
status_t I2C_MasterTransferGetCountEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle, size_t *count);

/*!
 * @brief Abort a master edma non-blocking transfer in a early time.
 *
 * @param base I2C peripheral base address.
 * @param handle pointer to i2c_master_edma_handle_t structure.
 */
void I2C_MasterTransferAbortEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle);

/* @} */
#if defined(__cplusplus)
}
#endif /*_cplusplus. */
/*@}*/
#endif /*_FSL_I2C_DMA_H_*/
