/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_flexcan_edma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flexcan_edma"
#endif

/*<! Structure definition for flexcan_edma_private_handle_t. The structure is private. */
typedef struct _flexcan_edma_private_handle
{
    CAN_Type *base;
    flexcan_edma_handle_t *handle;
} flexcan_edma_private_handle_t;

/* FlexCAN EDMA transfer handle. */
enum _flexcan_edma_tansfer_state
{
    KFLEXCAN_RxFifoIdle = 0U, /* Rx Fifo idle. */
    KFLEXCAN_RxFifoBusy = 1U, /* Rx Fifo busy. */
};

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*<! Private handle only used for internally. */
static flexcan_edma_private_handle_t s_edmaPrivateHandle[FSL_FEATURE_SOC_FLEXCAN_COUNT];

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief FlexCAN EDMA receive finished callback function.
 *
 * This function is called when FlexCAN Rx FIFO EDMA receive finished.
 * It disables the FlexCAN Rx FIFO EDMA request and sends
 * @ref kStatus_FLEXCAN_RxFifoIdle to FlexCAN EDMA callback.
 *
 * @param handle The EDMA handle.
 * @param param Callback function parameter.
 */
static void FLEXCAN_ReceiveFifoEDMACallback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds);

/*******************************************************************************
 * Code
 ******************************************************************************/

static void FLEXCAN_ReceiveFifoEDMACallback(edma_handle_t *handle, void *param, bool transferDone, uint32_t tcds)
{
    handle = handle;
    tcds = tcds;

    flexcan_edma_private_handle_t *flexcanPrivateHandle = (flexcan_edma_private_handle_t *)param;

    if (transferDone)
    {
        /* Disable transfer. */
        FLEXCAN_TransferAbortReceiveFifoEDMA(flexcanPrivateHandle->base, flexcanPrivateHandle->handle);

        if (flexcanPrivateHandle->handle->callback)
        {
            flexcanPrivateHandle->handle->callback(flexcanPrivateHandle->base, flexcanPrivateHandle->handle,
                                                   kStatus_FLEXCAN_RxFifoIdle, flexcanPrivateHandle->handle->userData);
        }
    }
}

/*!
 * brief Initializes the FlexCAN handle, which is used in transactional functions.
 *
 * param base FlexCAN peripheral base address.
 * param handle Pointer to flexcan_edma_handle_t structure.
 * param callback The callback function.
 * param userData The parameter of the callback function.
 * param rxFifoEdmaHandle User-requested DMA handle for Rx FIFO DMA transfer.
 */
void FLEXCAN_TransferCreateHandleEDMA(CAN_Type *base,
                                      flexcan_edma_handle_t *handle,
                                      flexcan_edma_transfer_callback_t callback,
                                      void *userData,
                                      edma_handle_t *rxFifoEdmaHandle)
{
    assert(handle);

    uint32_t instance = FLEXCAN_GetInstance(base);
    s_edmaPrivateHandle[instance].base = base;
    s_edmaPrivateHandle[instance].handle = handle;

    memset(handle, 0, sizeof(flexcan_edma_handle_t));

    handle->rxFifoState = KFLEXCAN_RxFifoIdle;
    handle->rxFifoEdmaHandle = rxFifoEdmaHandle;

    /* Register Callback. */
    handle->callback = callback;
    handle->userData = userData;

    /* Configure Rx FIFO DMA. */
    EDMA_SetCallback(handle->rxFifoEdmaHandle, FLEXCAN_ReceiveFifoEDMACallback, &s_edmaPrivateHandle[instance]);
}

/*!
 * brief Receives the CAN Message from the Rx FIFO using eDMA.
 *
 * This function receives the CAN Message using eDMA. This is a non-blocking function, which returns
 * right away. After the CAN Message is received, the receive callback function is called.
 *
 * param base FlexCAN peripheral base address.
 * param handle Pointer to flexcan_edma_handle_t structure.
 * param xfer FlexCAN Rx FIFO EDMA transfer structure, see #flexcan_fifo_transfer_t.
 * retval kStatus_Success if succeed, others failed.
 * retval kStatus_FLEXCAN_RxFifoBusy Previous transfer ongoing.
 */
status_t FLEXCAN_TransferReceiveFifoEDMA(CAN_Type *base, flexcan_edma_handle_t *handle, flexcan_fifo_transfer_t *xfer)
{
    assert(handle->rxFifoEdmaHandle);

    edma_transfer_config_t dmaXferConfig;
    status_t status;

    /* If previous Rx FIFO receive not finished. */
    if (KFLEXCAN_RxFifoBusy == handle->rxFifoState)
    {
        status = kStatus_FLEXCAN_RxFifoBusy;
    }
    else
    {
        handle->rxFifoState = KFLEXCAN_RxFifoBusy;

        /* Prepare transfer. */
        EDMA_PrepareTransfer(&dmaXferConfig, (void *)FLEXCAN_GetRxFifoHeadAddr(base), sizeof(flexcan_frame_t),
                             (void *)xfer->frame, sizeof(uint32_t), sizeof(flexcan_frame_t), sizeof(flexcan_frame_t),
                             kEDMA_PeripheralToMemory);
        /* Submit transfer. */
        EDMA_SubmitTransfer(handle->rxFifoEdmaHandle, &dmaXferConfig);
        EDMA_StartTransfer(handle->rxFifoEdmaHandle);

        /* Enable FlexCAN Rx FIFO EDMA. */
        FLEXCAN_EnableRxFifoDMA(base, true);

        status = kStatus_Success;
    }

    return status;
}

/*!
 * brief Aborts the receive process which used eDMA.
 *
 * This function aborts the receive process which used eDMA.
 *
 * param base FlexCAN peripheral base address.
 * param handle Pointer to flexcan_edma_handle_t structure.
 */
void FLEXCAN_TransferAbortReceiveFifoEDMA(CAN_Type *base, flexcan_edma_handle_t *handle)
{
    assert(handle->rxFifoEdmaHandle);

    /* Disable FlexCAN Rx FIFO EDMA. */
    FLEXCAN_EnableRxFifoDMA(base, false);

    /* Stop transfer. */
    EDMA_AbortTransfer(handle->rxFifoEdmaHandle);

    handle->rxFifoState = KFLEXCAN_RxFifoIdle;
}
