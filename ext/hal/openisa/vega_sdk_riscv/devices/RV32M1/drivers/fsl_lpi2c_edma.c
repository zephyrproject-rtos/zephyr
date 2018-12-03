/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_lpi2c_edma.h"
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* @brief Mask to align an address to 32 bytes. */
#define ALIGN_32_MASK (0x1fU)

/*! @brief Common sets of flags used by the driver. */
enum _lpi2c_flag_constants
{
    /*! All flags which are cleared by the driver upon starting a transfer. */
    kMasterClearFlags = kLPI2C_MasterEndOfPacketFlag | kLPI2C_MasterStopDetectFlag | kLPI2C_MasterNackDetectFlag |
                        kLPI2C_MasterArbitrationLostFlag | kLPI2C_MasterFifoErrFlag | kLPI2C_MasterPinLowTimeoutFlag |
                        kLPI2C_MasterDataMatchFlag,

    /*! IRQ sources enabled by the non-blocking transactional API. */
    kMasterIrqFlags = kLPI2C_MasterArbitrationLostFlag | kLPI2C_MasterTxReadyFlag | kLPI2C_MasterRxReadyFlag |
                      kLPI2C_MasterStopDetectFlag | kLPI2C_MasterNackDetectFlag | kLPI2C_MasterPinLowTimeoutFlag |
                      kLPI2C_MasterFifoErrFlag,

    /*! Errors to check for. */
    kMasterErrorFlags = kLPI2C_MasterNackDetectFlag | kLPI2C_MasterArbitrationLostFlag | kLPI2C_MasterFifoErrFlag |
                        kLPI2C_MasterPinLowTimeoutFlag,

    /*! All flags which are cleared by the driver upon starting a transfer. */
    kSlaveClearFlags = kLPI2C_SlaveRepeatedStartDetectFlag | kLPI2C_SlaveStopDetectFlag | kLPI2C_SlaveBitErrFlag |
                       kLPI2C_SlaveFifoErrFlag,

    /*! IRQ sources enabled by the non-blocking transactional API. */
    kSlaveIrqFlags = kLPI2C_SlaveTxReadyFlag | kLPI2C_SlaveRxReadyFlag | kLPI2C_SlaveStopDetectFlag |
                     kLPI2C_SlaveRepeatedStartDetectFlag | kLPI2C_SlaveFifoErrFlag | kLPI2C_SlaveBitErrFlag |
                     kLPI2C_SlaveTransmitAckFlag | kLPI2C_SlaveAddressValidFlag,

    /*! Errors to check for. */
    kSlaveErrorFlags = kLPI2C_SlaveFifoErrFlag | kLPI2C_SlaveBitErrFlag,
};

/* ! @brief LPI2C master fifo commands. */
enum _lpi2c_master_fifo_cmd
{
    kTxDataCmd = LPI2C_MTDR_CMD(0x0U), /*!< Transmit DATA[7:0] */
    kRxDataCmd = LPI2C_MTDR_CMD(0X1U), /*!< Receive (DATA[7:0] + 1) bytes */
    kStopCmd = LPI2C_MTDR_CMD(0x2U),   /*!< Generate STOP condition */
    kStartCmd = LPI2C_MTDR_CMD(0x4U),  /*!< Generate(repeated) START and transmit address in DATA[[7:0] */
};

/*! @brief States for the state machine used by transactional APIs. */
enum _lpi2c_transfer_states
{
    kIdleState = 0,
    kSendCommandState,
    kIssueReadCommandState,
    kTransferDataState,
    kStopState,
    kWaitForCompletionState,
};

/*! @brief Typedef for interrupt handler. */
typedef void (*lpi2c_isr_t)(LPI2C_Type *base, void *handle);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/* Defined in fsl_lpi2c.c. */
extern status_t LPI2C_CheckForBusyBus(LPI2C_Type *base);

/* Defined in fsl_lpi2c.c. */
extern status_t LPI2C_MasterCheckAndClearError(LPI2C_Type *base, uint32_t status);

static uint32_t LPI2C_GenerateCommands(lpi2c_master_edma_handle_t *handle);

static void LPI2C_MasterEDMACallback(edma_handle_t *dmaHandle, void *userData, bool isTransferDone, uint32_t tcds);

/*******************************************************************************
 * Code
 ******************************************************************************/

void LPI2C_MasterCreateEDMAHandle(LPI2C_Type *base,
                                  lpi2c_master_edma_handle_t *handle,
                                  edma_handle_t *rxDmaHandle,
                                  edma_handle_t *txDmaHandle,
                                  lpi2c_master_edma_transfer_callback_t callback,
                                  void *userData)
{
    assert(handle);
    assert(rxDmaHandle);
    assert(txDmaHandle);

    /* Clear out the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Set up the handle. For combined rx/tx DMA requests, the tx channel handle is set to the rx handle */
    /* in order to make the transfer API code simpler. */
    handle->base = base;
    handle->completionCallback = callback;
    handle->userData = userData;
    handle->rx = rxDmaHandle;
    handle->tx = FSL_FEATURE_LPI2C_HAS_SEPARATE_DMA_RX_TX_REQn(base) ? txDmaHandle : rxDmaHandle;

    /* Set DMA channel completion callbacks. */
    EDMA_SetCallback(handle->rx, LPI2C_MasterEDMACallback, handle);
    if (FSL_FEATURE_LPI2C_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        EDMA_SetCallback(handle->tx, LPI2C_MasterEDMACallback, handle);
    }
}

/*!
 * @brief Prepares the command buffer with the sequence of commands needed to send the requested transaction.
 * @param handle Master DMA driver handle.
 * @return Number of command words.
 */
static uint32_t LPI2C_GenerateCommands(lpi2c_master_edma_handle_t *handle)
{
    lpi2c_master_transfer_t *xfer = &handle->transfer;
    uint16_t *cmd = (uint16_t *)&handle->commandBuffer;
    uint32_t cmdCount = 0;

    /* Handle no start option. */
    if (xfer->flags & kLPI2C_TransferNoStartFlag)
    {
        if (xfer->direction == kLPI2C_Read)
        {
            /* Need to issue read command first. */
            cmd[cmdCount++] = kRxDataCmd | LPI2C_MTDR_DATA(xfer->dataSize - 1);
        }
    }
    else
    {
        /*
         * Initial direction depends on whether a subaddress was provided, and of course the actual
         * data transfer direction.
         */
        lpi2c_direction_t direction = xfer->subaddressSize ? kLPI2C_Write : xfer->direction;

        /* Start command. */
        cmd[cmdCount++] =
            (uint16_t)kStartCmd | (uint16_t)((uint16_t)((uint16_t)xfer->slaveAddress << 1U) | (uint16_t)direction);

        /* Subaddress, MSB first. */
        if (xfer->subaddressSize)
        {
            uint32_t subaddressRemaining = xfer->subaddressSize;
            while (subaddressRemaining--)
            {
                uint8_t subaddressByte = (xfer->subaddress >> (8 * subaddressRemaining)) & 0xff;
                cmd[cmdCount++] = subaddressByte;
            }
        }

        /* Reads need special handling because we have to issue a read command and maybe a repeated start. */
        if ((xfer->dataSize) && (xfer->direction == kLPI2C_Read))
        {
            /* Need to send repeated start if switching directions to read. */
            if (direction == kLPI2C_Write)
            {
                cmd[cmdCount++] = (uint16_t)kStartCmd |
                                  (uint16_t)((uint16_t)((uint16_t)xfer->slaveAddress << 1U) | (uint16_t)kLPI2C_Read);
            }

            /* Read command. */
            cmd[cmdCount++] = kRxDataCmd | LPI2C_MTDR_DATA(xfer->dataSize - 1);
        }
    }

    return cmdCount;
}

status_t LPI2C_MasterTransferEDMA(LPI2C_Type *base,
                                  lpi2c_master_edma_handle_t *handle,
                                  lpi2c_master_transfer_t *transfer)
{
    status_t result;

    assert(handle);
    assert(transfer);
    assert(transfer->subaddressSize <= sizeof(transfer->subaddress));

    /* Return busy if another transaction is in progress. */
    if (handle->isBusy)
    {
        return kStatus_LPI2C_Busy;
    }

    /* Return an error if the bus is already in use not by us. */
    result = LPI2C_CheckForBusyBus(base);
    if (result)
    {
        return result;
    }

    /* We're now busy. */
    handle->isBusy = true;

    /* Disable LPI2C IRQ and DMA sources while we configure stuff. */
    LPI2C_MasterDisableInterrupts(base, kMasterIrqFlags);
    LPI2C_MasterEnableDMA(base, false, false);

    /* Clear all flags. */
    LPI2C_MasterClearStatusFlags(base, kMasterClearFlags);

    /* Save transfer into handle. */
    handle->transfer = *transfer;

    /* Generate commands to send. */
    uint32_t commandCount = LPI2C_GenerateCommands(handle);

    /* If the user is transmitting no data with no start or stop, then just go ahead and invoke the callback. */
    if ((!commandCount) && (transfer->dataSize == 0))
    {
        if (handle->completionCallback)
        {
            handle->completionCallback(base, handle, kStatus_Success, handle->userData);
        }
        return kStatus_Success;
    }

    /* Reset DMA channels. */
    EDMA_ResetChannel(handle->rx->base, handle->rx->channel);
    if (FSL_FEATURE_LPI2C_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        EDMA_ResetChannel(handle->tx->base, handle->tx->channel);
    }

    /* Get a 32-byte aligned TCD pointer. */
    edma_tcd_t *tcd = (edma_tcd_t *)((uint32_t)(&handle->tcds[1]) & (~ALIGN_32_MASK));

    bool hasSendData = (transfer->direction == kLPI2C_Write) && (transfer->dataSize);
    bool hasReceiveData = (transfer->direction == kLPI2C_Read) && (transfer->dataSize);

    edma_transfer_config_t transferConfig;
    edma_tcd_t *linkTcd = NULL;

    /* Set up data transmit. */
    if (hasSendData)
    {
        transferConfig.srcAddr = (uint32_t)transfer->data;
        transferConfig.destAddr = (uint32_t)LPI2C_MasterGetTxFifoAddress(base);
        transferConfig.srcTransferSize = kEDMA_TransferSize1Bytes;
        transferConfig.destTransferSize = kEDMA_TransferSize1Bytes;
        transferConfig.srcOffset = sizeof(uint8_t);
        transferConfig.destOffset = 0;
        transferConfig.minorLoopBytes = sizeof(uint8_t); /* TODO optimize to fill fifo */
        transferConfig.majorLoopCounts = transfer->dataSize;

        /* Store the initially configured eDMA minor byte transfer count into the LPI2C handle */
        handle->nbytes = transferConfig.minorLoopBytes;

        if (commandCount)
        {
            /* Create a software TCD, which will be chained after the commands. */
            EDMA_TcdReset(tcd);
            EDMA_TcdSetTransferConfig(tcd, &transferConfig, NULL);
            EDMA_TcdEnableInterrupts(tcd, kEDMA_MajorInterruptEnable);
            linkTcd = tcd;
        }
        else
        {
            /* User is only transmitting data with no required commands, so this transfer can stand alone. */
            EDMA_SetTransferConfig(handle->tx->base, handle->tx->channel, &transferConfig, NULL);
            EDMA_EnableChannelInterrupts(handle->tx->base, handle->tx->channel, kEDMA_MajorInterruptEnable);
        }
    }
    else if (hasReceiveData)
    {
        /* Set up data receive. */
        transferConfig.srcAddr = (uint32_t)LPI2C_MasterGetRxFifoAddress(base);
        transferConfig.destAddr = (uint32_t)transfer->data;
        transferConfig.srcTransferSize = kEDMA_TransferSize1Bytes;
        transferConfig.destTransferSize = kEDMA_TransferSize1Bytes;
        transferConfig.srcOffset = 0;
        transferConfig.destOffset = sizeof(uint8_t);
        transferConfig.minorLoopBytes = sizeof(uint8_t); /* TODO optimize to empty fifo */
        transferConfig.majorLoopCounts = transfer->dataSize;

        /* Store the initially configured eDMA minor byte transfer count into the LPI2C handle */
        handle->nbytes = transferConfig.minorLoopBytes;

        if (FSL_FEATURE_LPI2C_HAS_SEPARATE_DMA_RX_TX_REQn(base) || (!commandCount))
        {
            /* We can put this receive transfer on its own DMA channel. */
            EDMA_SetTransferConfig(handle->rx->base, handle->rx->channel, &transferConfig, NULL);
            EDMA_EnableChannelInterrupts(handle->rx->base, handle->rx->channel, kEDMA_MajorInterruptEnable);
        }
        else
        {
            /* For shared rx/tx DMA requests when there are commands, create a software TCD which will be */
            /* chained onto the commands transfer. */
            EDMA_TcdReset(tcd);
            EDMA_TcdSetTransferConfig(tcd, &transferConfig, NULL);
            EDMA_TcdEnableInterrupts(tcd, kEDMA_MajorInterruptEnable);
            linkTcd = tcd;
        }
    }
    else
    {
        /* No data to send */
    }

    /* Set up commands transfer. */
    if (commandCount)
    {
        transferConfig.srcAddr = (uint32_t)handle->commandBuffer;
        transferConfig.destAddr = (uint32_t)LPI2C_MasterGetTxFifoAddress(base);
        transferConfig.srcTransferSize = kEDMA_TransferSize2Bytes;
        transferConfig.destTransferSize = kEDMA_TransferSize2Bytes;
        transferConfig.srcOffset = sizeof(uint16_t);
        transferConfig.destOffset = 0;
        transferConfig.minorLoopBytes = sizeof(uint16_t); /* TODO optimize to fill fifo */
        transferConfig.majorLoopCounts = commandCount;

        EDMA_SetTransferConfig(handle->tx->base, handle->tx->channel, &transferConfig, linkTcd);
    }

    /* Start DMA transfer. */
    if (hasReceiveData || !FSL_FEATURE_LPI2C_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        EDMA_StartTransfer(handle->rx);
    }
    if ((hasSendData || commandCount) && FSL_FEATURE_LPI2C_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        EDMA_StartTransfer(handle->tx);
    }

    /* Enable DMA in both directions. This actually kicks of the transfer. */
    LPI2C_MasterEnableDMA(base, true, true);

    return result;
}

status_t LPI2C_MasterTransferGetCountEDMA(LPI2C_Type *base, lpi2c_master_edma_handle_t *handle, size_t *count)
{
    assert(handle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    /* Catch when there is not an active transfer. */
    if (!handle->isBusy)
    {
        *count = 0;
        return kStatus_NoTransferInProgress;
    }

    uint32_t remaining = handle->transfer.dataSize;

    /* If the DMA is still on a commands transfer that chains to the actual data transfer, */
    /* we do nothing and return the number of transferred bytes as zero. */
    if (handle->tx->base->TCD[handle->tx->channel].DLAST_SGA == 0)
    {
        if (handle->transfer.direction == kLPI2C_Write)
        {
            remaining =
                (uint32_t)handle->nbytes * EDMA_GetRemainingMajorLoopCount(handle->tx->base, handle->tx->channel);
        }
        else
        {
            remaining =
                (uint32_t)handle->nbytes * EDMA_GetRemainingMajorLoopCount(handle->rx->base, handle->rx->channel);
        }
    }

    *count = handle->transfer.dataSize - remaining;

    return kStatus_Success;
}

status_t LPI2C_MasterTransferAbortEDMA(LPI2C_Type *base, lpi2c_master_edma_handle_t *handle)
{
    /* Catch when there is not an active transfer. */
    if (!handle->isBusy)
    {
        return kStatus_LPI2C_Idle;
    }

    /* Terminate DMA transfers. */
    EDMA_AbortTransfer(handle->rx);
    if (FSL_FEATURE_LPI2C_HAS_SEPARATE_DMA_RX_TX_REQn(base))
    {
        EDMA_AbortTransfer(handle->tx);
    }

    /* Reset fifos. */
    base->MCR |= LPI2C_MCR_RRF_MASK | LPI2C_MCR_RTF_MASK;

    /* Send a stop command to finalize the transfer. */
    base->MTDR = kStopCmd;

    /* Reset handle. */
    handle->isBusy = false;

    return kStatus_Success;
}

/*!
 * @brief DMA completion callback.
 * @param dmaHandle DMA channel handle for the channel that completed.
 * @param userData User data associated with the channel handle. For this callback, the user data is the
 *      LPI2C DMA driver handle.
 * @param isTransferDone Whether the DMA transfer has completed.
 * @param tcds Number of TCDs that completed.
 */
static void LPI2C_MasterEDMACallback(edma_handle_t *dmaHandle, void *userData, bool isTransferDone, uint32_t tcds)
{
    lpi2c_master_edma_handle_t *handle = (lpi2c_master_edma_handle_t *)userData;
    if (!handle)
    {
        return;
    }

    /* Check for errors. */
    status_t result = LPI2C_MasterCheckAndClearError(handle->base, LPI2C_MasterGetStatusFlags(handle->base));

    /* Done with this transaction. */
    handle->isBusy = false;

    if (!(handle->transfer.flags & kLPI2C_TransferNoStopFlag))
    {
        /* Send a stop command to finalize the transfer. */
        handle->base->MTDR = kStopCmd;
    }

    /* Invoke callback. */
    if (handle->completionCallback)
    {
        handle->completionCallback(handle->base, handle, result, handle->userData);
    }
}
