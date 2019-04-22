/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_FLEXCAN_EDMA_H_
#define _FSL_FLEXCAN_EDMA_H_

#include "fsl_flexcan.h"
#include "fsl_edma.h"

/*!
 * @addtogroup flexcan_edma_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief FlexCAN EDMA driver version 2.2.4. */
#define FSL_FLEXCAN_EDMA_DRIVER_VERSION (MAKE_VERSION(2, 2, 4))
/*@}*/

/* Forward declaration of the handle typedef. */
typedef struct _flexcan_edma_handle flexcan_edma_handle_t;

/*! @brief FlexCAN transfer callback function. */
typedef void (*flexcan_edma_transfer_callback_t)(CAN_Type *base,
                                                 flexcan_edma_handle_t *handle,
                                                 status_t status,
                                                 void *userData);

/*!
* @brief FlexCAN eDMA handle
*/
struct _flexcan_edma_handle
{
    flexcan_edma_transfer_callback_t callback; /*!< Callback function. */
    void *userData;                            /*!< FlexCAN callback function parameter.*/
    edma_handle_t *rxFifoEdmaHandle;           /*!< The EDMA Rx FIFO channel used. */
    volatile uint8_t rxFifoState;              /*!< Rx FIFO transfer state. */
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
 * @brief Initializes the FlexCAN handle, which is used in transactional functions.
 *
 * @param base FlexCAN peripheral base address.
 * @param handle Pointer to flexcan_edma_handle_t structure.
 * @param callback The callback function.
 * @param userData The parameter of the callback function.
 * @param rxFifoEdmaHandle User-requested DMA handle for Rx FIFO DMA transfer.
 */
void FLEXCAN_TransferCreateHandleEDMA(CAN_Type *base,
                                      flexcan_edma_handle_t *handle,
                                      flexcan_edma_transfer_callback_t callback,
                                      void *userData,
                                      edma_handle_t *rxFifoEdmaHandle);

/*!
 * @brief Receives the CAN Message from the Rx FIFO using eDMA.
 *
 * This function receives the CAN Message using eDMA. This is a non-blocking function, which returns
 * right away. After the CAN Message is received, the receive callback function is called.
 *
 * @param base FlexCAN peripheral base address.
 * @param handle Pointer to flexcan_edma_handle_t structure.
 * @param xfer FlexCAN Rx FIFO EDMA transfer structure, see #flexcan_fifo_transfer_t.
 * @retval kStatus_Success if succeed, others failed.
 * @retval kStatus_FLEXCAN_RxFifoBusy Previous transfer ongoing.
 */
status_t FLEXCAN_TransferReceiveFifoEDMA(CAN_Type *base, flexcan_edma_handle_t *handle, flexcan_fifo_transfer_t *xfer);

/*!
 * @brief Aborts the receive process which used eDMA.
 *
 * This function aborts the receive process which used eDMA.
 *
 * @param base FlexCAN peripheral base address.
 * @param handle Pointer to flexcan_edma_handle_t structure.
 */
void FLEXCAN_TransferAbortReceiveFifoEDMA(CAN_Type *base, flexcan_edma_handle_t *handle);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_FLEXCAN_EDMA_H_ */
