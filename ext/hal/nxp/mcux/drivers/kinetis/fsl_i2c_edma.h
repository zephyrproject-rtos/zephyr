/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_I2C_DMA_H_
#define _FSL_I2C_DMA_H_

#include "fsl_i2c.h"
#include "fsl_edma.h"

/*!
 * @addtogroup i2c_edma_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief I2C EDMA driver version 2.0.5. */
#define FSL_I2C_EDMA_DRIVER_VERSION (MAKE_VERSION(2, 0, 5))
/*@}*/

/*! @brief I2C master eDMA handle typedef. */
typedef struct _i2c_master_edma_handle i2c_master_edma_handle_t;

/*! @brief I2C master eDMA transfer callback typedef. */
typedef void (*i2c_master_edma_transfer_callback_t)(I2C_Type *base,
                                                    i2c_master_edma_handle_t *handle,
                                                    status_t status,
                                                    void *userData);

/*! @brief I2C master eDMA transfer structure. */
struct _i2c_master_edma_handle
{
    i2c_master_transfer_t transfer; /*!< I2C master transfer structure. */
    size_t transferSize;            /*!< Total bytes to be transferred. */
    uint8_t nbytes;                 /*!< eDMA minor byte transfer count initially configured. */
    uint8_t state;                  /*!< I2C master transfer status. */
    edma_handle_t *dmaHandle;       /*!< The eDMA handler used. */
    i2c_master_edma_transfer_callback_t
        completionCallback; /*!< A callback function called after the eDMA transfer is finished. */
    void *userData;         /*!< A callback parameter passed to the callback function. */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus. */

/*!
 * @name I2C Block eDMA Transfer Operation
 * @{
 */

/*!
 * @brief Initializes the I2C handle which is used in transcational functions.
 *
 * @param base I2C peripheral base address.
 * @param handle A pointer to the i2c_master_edma_handle_t structure.
 * @param callback A pointer to the user callback function.
 * @param userData A user parameter passed to the callback function.
 * @param edmaHandle eDMA handle pointer.
 */
void I2C_MasterCreateEDMAHandle(I2C_Type *base,
                                i2c_master_edma_handle_t *handle,
                                i2c_master_edma_transfer_callback_t callback,
                                void *userData,
                                edma_handle_t *edmaHandle);

/*!
 * @brief Performs a master eDMA non-blocking transfer on the I2C bus.
 *
 * @param base I2C peripheral base address.
 * @param handle A pointer to the i2c_master_edma_handle_t structure.
 * @param xfer A pointer to the transfer structure of i2c_master_transfer_t.
 * @retval kStatus_Success Sucessfully completed the data transmission.
 * @retval kStatus_I2C_Busy A previous transmission is still not finished.
 * @retval kStatus_I2C_Timeout Transfer error, waits for a signal timeout.
 * @retval kStatus_I2C_ArbitrationLost Transfer error, arbitration lost.
 * @retval kStataus_I2C_Nak Transfer error, receive NAK during transfer.
 */
status_t I2C_MasterTransferEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle, i2c_master_transfer_t *xfer);

/*!
 * @brief Gets a master transfer status during the eDMA non-blocking transfer.
 *
 * @param base I2C peripheral base address.
 * @param handle A pointer to the i2c_master_edma_handle_t structure.
 * @param count A number of bytes transferred by the non-blocking transaction.
 */
status_t I2C_MasterTransferGetCountEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle, size_t *count);

/*!
 * @brief Aborts a master eDMA non-blocking transfer early.
 *
 * @param base I2C peripheral base address.
 * @param handle A pointer to the i2c_master_edma_handle_t structure.
 */
void I2C_MasterTransferAbortEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle);

/* @} */
#if defined(__cplusplus)
}
#endif /*_cplusplus. */
/*@}*/
#endif /*_FSL_I2C_DMA_H_*/
