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

#include "fsl_i2c_edma.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*<! @breif Structure definition for i2c_master_edma_private_handle_t. The structure is private. */
typedef struct _i2c_master_edma_private_handle
{
    I2C_Type *base;
    i2c_master_edma_handle_t *handle;
} i2c_master_edma_private_handle_t;

/*! @brief i2c master DMA transfer state. */
enum _i2c_master_dma_transfer_states
{
    kIdleState = 0x0U,         /*!< I2C bus idle. */
    kTransferDataState = 0x1U, /*!< 7-bit address check state. */
};

/*! @brief Common sets of flags used by the driver. */
enum _i2c_flag_constants
{
/*! All flags which are cleared by the driver upon starting a transfer. */
#if defined(FSL_FEATURE_I2C_HAS_START_STOP_DETECT) && FSL_FEATURE_I2C_HAS_START_STOP_DETECT
    kClearFlags = kI2C_ArbitrationLostFlag | kI2C_IntPendingFlag | kI2C_StartDetectFlag | kI2C_StopDetectFlag,
#elif defined(FSL_FEATURE_I2C_HAS_STOP_DETECT) && FSL_FEATURE_I2C_HAS_STOP_DETECT
    kClearFlags = kI2C_ArbitrationLostFlag | kI2C_IntPendingFlag | kI2C_StopDetectFlag,
#else
    kClearFlags = kI2C_ArbitrationLostFlag | kI2C_IntPendingFlag,
#endif
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief EDMA callback for I2C master EDMA driver.
 *
 * @param handle EDMA handler for I2C master EDMA driver
 * @param userData user param passed to the callback function
 */
static void I2C_MasterTransferCallbackEDMA(edma_handle_t *handle, void *userData, bool transferDone, uint32_t tcds);

/*!
 * @brief Check and clear status operation.
 *
 * @param base I2C peripheral base address.
 * @param status current i2c hardware status.
 * @retval kStatus_Success No error found.
 * @retval kStatus_I2C_ArbitrationLost Transfer error, arbitration lost.
 * @retval kStatus_I2C_Nak Received Nak error.
 */
static status_t I2C_CheckAndClearError(I2C_Type *base, uint32_t status);

/*!
 * @brief EDMA config for I2C master driver.
 *
 * @param base I2C peripheral base address.
 * @param handle pointer to i2c_master_edma_handle_t structure which stores the transfer state
 */
static void I2C_MasterTransferEDMAConfig(I2C_Type *base, i2c_master_edma_handle_t *handle);

/*!
 * @brief Set up master transfer, send slave address and sub address(if any), wait until the
 * wait until address sent status return.
 *
 * @param base I2C peripheral base address.
 * @param handle pointer to i2c_master_edma_handle_t structure which stores the transfer state
 * @param xfer pointer to i2c_master_transfer_t structure
 */
static status_t I2C_InitTransferStateMachineEDMA(I2C_Type *base,
                                                 i2c_master_edma_handle_t *handle,
                                                 i2c_master_transfer_t *xfer);

/*!
 * @brief Get the I2C instance from peripheral base address.
 *
 * @param base I2C peripheral base address.
 * @return I2C instance.
 */
extern uint32_t I2C_GetInstance(I2C_Type *base);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*<! Private handle only used for internally. */
static i2c_master_edma_private_handle_t s_edmaPrivateHandle[FSL_FEATURE_SOC_I2C_COUNT];

/*******************************************************************************
 * Codes
 ******************************************************************************/

static void I2C_MasterTransferCallbackEDMA(edma_handle_t *handle, void *userData, bool transferDone, uint32_t tcds)
{
    i2c_master_edma_private_handle_t *i2cPrivateHandle = (i2c_master_edma_private_handle_t *)userData;
    status_t result = kStatus_Success;

    /* Disable DMA. */
    I2C_EnableDMA(i2cPrivateHandle->base, false);

    /* Send stop if kI2C_TransferNoStop flag is not asserted. */
    if (!(i2cPrivateHandle->handle->transfer.flags & kI2C_TransferNoStopFlag))
    {
        if (i2cPrivateHandle->handle->transfer.direction == kI2C_Read)
        {
            /* Change to send NAK at the last byte. */
            i2cPrivateHandle->base->C1 |= I2C_C1_TXAK_MASK;

            /* Wait the last data to be received. */
            while (!(i2cPrivateHandle->base->S & kI2C_TransferCompleteFlag))
            {
            }

            /* Send stop signal. */
            result = I2C_MasterStop(i2cPrivateHandle->base);

            /* Read the last data byte. */
            *(i2cPrivateHandle->handle->transfer.data + i2cPrivateHandle->handle->transfer.dataSize - 1) =
                i2cPrivateHandle->base->D;
        }
        else
        {
            /* Wait the last data to be sent. */
            while (!(i2cPrivateHandle->base->S & kI2C_TransferCompleteFlag))
            {
            }

            /* Send stop signal. */
            result = I2C_MasterStop(i2cPrivateHandle->base);
        }
    }
    else
    {
        if (i2cPrivateHandle->handle->transfer.direction == kI2C_Read)
        {
            /* Change to send NAK at the last byte. */
            i2cPrivateHandle->base->C1 |= I2C_C1_TXAK_MASK;

            /* Wait the last data to be received. */
            while (!(i2cPrivateHandle->base->S & kI2C_TransferCompleteFlag))
            {
            }

            /* Change direction to send. */
            i2cPrivateHandle->base->C1 |= I2C_C1_TX_MASK;

            /* Read the last data byte. */
            *(i2cPrivateHandle->handle->transfer.data + i2cPrivateHandle->handle->transfer.dataSize - 1) =
                i2cPrivateHandle->base->D;
        }
    }

    i2cPrivateHandle->handle->state = kIdleState;

    if (i2cPrivateHandle->handle->completionCallback)
    {
        i2cPrivateHandle->handle->completionCallback(i2cPrivateHandle->base, i2cPrivateHandle->handle, result,
                                                     i2cPrivateHandle->handle->userData);
    }
}

static status_t I2C_CheckAndClearError(I2C_Type *base, uint32_t status)
{
    status_t result = kStatus_Success;

    /* Check arbitration lost. */
    if (status & kI2C_ArbitrationLostFlag)
    {
        /* Clear arbitration lost flag. */
        base->S = kI2C_ArbitrationLostFlag;
        result = kStatus_I2C_ArbitrationLost;
    }
    /* Check NAK */
    else if (status & kI2C_ReceiveNakFlag)
    {
        result = kStatus_I2C_Nak;
    }
    else
    {
    }

    return result;
}

static status_t I2C_InitTransferStateMachineEDMA(I2C_Type *base,
                                                 i2c_master_edma_handle_t *handle,
                                                 i2c_master_transfer_t *xfer)
{
    assert(handle);
    assert(xfer);

    status_t result = kStatus_Success;

    if (handle->state != kIdleState)
    {
        return kStatus_I2C_Busy;
    }
    else
    {
        i2c_direction_t direction = xfer->direction;

        /* Init the handle member. */
        handle->transfer = *xfer;

        /* Save total transfer size. */
        handle->transferSize = xfer->dataSize;

        handle->state = kTransferDataState;

        /* Clear all status before transfer. */
        I2C_MasterClearStatusFlags(base, kClearFlags);

        /* Change to send write address when it's a read operation with command. */
        if ((xfer->subaddressSize > 0) && (xfer->direction == kI2C_Read))
        {
            direction = kI2C_Write;
        }

        /* If repeated start is requested, send repeated start. */
        if (handle->transfer.flags & kI2C_TransferRepeatedStartFlag)
        {
            result = I2C_MasterRepeatedStart(base, handle->transfer.slaveAddress, direction);
        }
        else /* For normal transfer, send start. */
        {
            result = I2C_MasterStart(base, handle->transfer.slaveAddress, direction);
        }

        if (result)
        {
            return result;
        }

        while (!(base->S & kI2C_IntPendingFlag))
        {
        }

        /* Check if there's transfer error. */
        result = I2C_CheckAndClearError(base, base->S);

        /* Return if error. */
        if (result)
        {
            if (result == kStatus_I2C_Nak)
            {
                result = kStatus_I2C_Addr_Nak;

                if (I2C_MasterStop(base) != kStatus_Success)
                {
                    result = kStatus_I2C_Timeout;
                }

                if (handle->completionCallback)
                {
                    (handle->completionCallback)(base, handle, result, handle->userData);
                }
            }

            return result;
        }

        /* Send subaddress. */
        if (handle->transfer.subaddressSize)
        {
            do
            {
                /* Clear interrupt pending flag. */
                base->S = kI2C_IntPendingFlag;

                handle->transfer.subaddressSize--;
                base->D = ((handle->transfer.subaddress) >> (8 * handle->transfer.subaddressSize));

                /* Wait until data transfer complete. */
                while (!(base->S & kI2C_IntPendingFlag))
                {
                }

                /* Check if there's transfer error. */
                result = I2C_CheckAndClearError(base, base->S);

                if (result)
                {
                    return result;
                }

            } while ((handle->transfer.subaddressSize > 0) && (result == kStatus_Success));

            if (handle->transfer.direction == kI2C_Read)
            {
                /* Clear pending flag. */
                base->S = kI2C_IntPendingFlag;

                /* Send repeated start and slave address. */
                result = I2C_MasterRepeatedStart(base, handle->transfer.slaveAddress, kI2C_Read);

                if (result)
                {
                    return result;
                }

                /* Wait until data transfer complete. */
                while (!(base->S & kI2C_IntPendingFlag))
                {
                }

                /* Check if there's transfer error. */
                result = I2C_CheckAndClearError(base, base->S);

                if (result)
                {
                    return result;
                }
            }
        }

        /* Clear pending flag. */
        base->S = kI2C_IntPendingFlag;
    }

    return result;
}

static void I2C_MasterTransferEDMAConfig(I2C_Type *base, i2c_master_edma_handle_t *handle)
{
    edma_transfer_config_t transfer_config;

    if (handle->transfer.direction == kI2C_Read)
    {
        transfer_config.srcAddr = (uint32_t)I2C_GetDataRegAddr(base);
        transfer_config.destAddr = (uint32_t)(handle->transfer.data);
        transfer_config.majorLoopCounts = (handle->transfer.dataSize - 1);
        transfer_config.srcTransferSize = kEDMA_TransferSize1Bytes;
        transfer_config.srcOffset = 0;
        transfer_config.destTransferSize = kEDMA_TransferSize1Bytes;
        transfer_config.destOffset = 1;
        transfer_config.minorLoopBytes = 1;
    }
    else
    {
        transfer_config.srcAddr = (uint32_t)(handle->transfer.data + 1);
        transfer_config.destAddr = (uint32_t)I2C_GetDataRegAddr(base);
        transfer_config.majorLoopCounts = (handle->transfer.dataSize - 1);
        transfer_config.srcTransferSize = kEDMA_TransferSize1Bytes;
        transfer_config.srcOffset = 1;
        transfer_config.destTransferSize = kEDMA_TransferSize1Bytes;
        transfer_config.destOffset = 0;
        transfer_config.minorLoopBytes = 1;
    }

    /* Store the initially configured eDMA minor byte transfer count into the I2C handle */
    handle->nbytes = transfer_config.minorLoopBytes;

    EDMA_SubmitTransfer(handle->dmaHandle, &transfer_config);
    EDMA_StartTransfer(handle->dmaHandle);
}

void I2C_MasterCreateEDMAHandle(I2C_Type *base,
                                i2c_master_edma_handle_t *handle,
                                i2c_master_edma_transfer_callback_t callback,
                                void *userData,
                                edma_handle_t *edmaHandle)
{
    assert(handle);
    assert(edmaHandle);

    uint32_t instance = I2C_GetInstance(base);

    /* Zero handle. */
    memset(handle, 0, sizeof(*handle));

    /* Set the user callback and userData. */
    handle->completionCallback = callback;
    handle->userData = userData;

    /* Set the base for the handle. */
    base = base;

    /* Set the handle for EDMA. */
    handle->dmaHandle = edmaHandle;

    s_edmaPrivateHandle[instance].base = base;
    s_edmaPrivateHandle[instance].handle = handle;

    EDMA_SetCallback(edmaHandle, (edma_callback)I2C_MasterTransferCallbackEDMA, &s_edmaPrivateHandle[instance]);
}

status_t I2C_MasterTransferEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle, i2c_master_transfer_t *xfer)
{
    assert(handle);
    assert(xfer);

    status_t result;
    uint8_t tmpReg;
    volatile uint8_t dummy = 0;

    /* Add this to avoid build warning. */
    dummy++;

    /* Disable dma xfer. */
    I2C_EnableDMA(base, false);

    /* Send address and command buffer(if there is), until senddata phase or receive data phase. */
    result = I2C_InitTransferStateMachineEDMA(base, handle, xfer);

    if (result)
    {
        /* Send stop if received Nak. */
        if (result == kStatus_I2C_Nak)
        {
            if (I2C_MasterStop(base) != kStatus_Success)
            {
                result = kStatus_I2C_Timeout;
            }
        }

        /* Reset the state to idle state. */
        handle->state = kIdleState;

        return result;
    }

    /* Configure dma transfer. */
    /* For i2c send, need to send 1 byte first to trigger the dma, for i2c read,
    need to send stop before reading the last byte, so the dma transfer size should
    be (xSize - 1). */
    if (handle->transfer.dataSize > 1)
    {
        I2C_MasterTransferEDMAConfig(base, handle);
        if (handle->transfer.direction == kI2C_Read)
        {
            /* Change direction for receive. */
            base->C1 &= ~(I2C_C1_TX_MASK | I2C_C1_TXAK_MASK);

            /* Read dummy to release the bus. */
            dummy = base->D;

            /* Enabe dma transfer. */
            I2C_EnableDMA(base, true);
        }
        else
        {
            /* Enabe dma transfer. */
            I2C_EnableDMA(base, true);

            /* Send the first data. */
            base->D = *handle->transfer.data;
        }
    }
    else /* If transfer size is 1, use polling method. */
    {
        if (handle->transfer.direction == kI2C_Read)
        {
            tmpReg = base->C1;

            /* Change direction to Rx. */
            tmpReg &= ~I2C_C1_TX_MASK;

            /* Configure send NAK */
            tmpReg |= I2C_C1_TXAK_MASK;

            base->C1 = tmpReg;

            /* Read dummy to release the bus. */
            dummy = base->D;
        }
        else
        {
            base->D = *handle->transfer.data;
        }

        /* Wait until data transfer complete. */
        while (!(base->S & kI2C_IntPendingFlag))
        {
        }

        /* Clear pending flag. */
        base->S = kI2C_IntPendingFlag;

        /* Send stop if kI2C_TransferNoStop flag is not asserted. */
        if (!(handle->transfer.flags & kI2C_TransferNoStopFlag))
        {
            result = I2C_MasterStop(base);
        }
        else
        {
            /* Change direction to send. */
            base->C1 |= I2C_C1_TX_MASK;
        }

        /* Read the last byte of data. */
        if (handle->transfer.direction == kI2C_Read)
        {
            *handle->transfer.data = base->D;
        }

        /* Reset the state to idle. */
        handle->state = kIdleState;
    }

    return result;
}

status_t I2C_MasterTransferGetCountEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle, size_t *count)
{
    assert(handle->dmaHandle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    if (kIdleState != handle->state)
    {
        *count = (handle->transferSize -
                  (uint32_t)handle->nbytes *
                      EDMA_GetRemainingMajorLoopCount(handle->dmaHandle->base, handle->dmaHandle->channel));
    }
    else
    {
        *count = handle->transferSize;
    }

    return kStatus_Success;
}

void I2C_MasterTransferAbortEDMA(I2C_Type *base, i2c_master_edma_handle_t *handle)
{
    EDMA_AbortTransfer(handle->dmaHandle);

    /* Disable dma transfer. */
    I2C_EnableDMA(base, false);

    /* Reset the state to idle. */
    handle->state = kIdleState;
}
