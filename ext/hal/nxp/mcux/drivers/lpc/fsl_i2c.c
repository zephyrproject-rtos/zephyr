/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_i2c.h"
#include "fsl_flexcomm.h"
#include <stdlib.h>
#include <string.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flexcomm_i2c"
#endif

/*! @brief Common sets of flags used by the driver. */
enum _i2c_flag_constants
{
    kI2C_MasterIrqFlags = I2C_INTSTAT_MSTPENDING_MASK | I2C_INTSTAT_MSTARBLOSS_MASK | I2C_INTSTAT_MSTSTSTPERR_MASK,
    kI2C_SlaveIrqFlags = I2C_INTSTAT_SLVPENDING_MASK | I2C_INTSTAT_SLVDESEL_MASK,
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static status_t I2C_InitTransferStateMachine(I2C_Type *base, i2c_master_handle_t *handle, i2c_master_transfer_t *xfer);
static void I2C_SlaveInternalStateMachineReset(I2C_Type *base);
static status_t I2C_SlaveDivVal(uint32_t srcClock_Hz, i2c_slave_bus_speed_t busSpeed, uint32_t *divVal);
static uint32_t I2C_SlavePollPending(I2C_Type *base);
static void I2C_SlaveInvokeEvent(I2C_Type *base, i2c_slave_handle_t *handle, i2c_slave_transfer_event_t event);
static bool I2C_SlaveAddressIRQ(I2C_Type *base, i2c_slave_handle_t *handle);
static status_t I2C_SlaveTransferNonBlockingInternal(I2C_Type *base,
                                                     i2c_slave_handle_t *handle,
                                                     const void *txData,
                                                     size_t txSize,
                                                     void *rxData,
                                                     size_t rxSize,
                                                     uint32_t eventMask);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Array to map i2c instance number to base address. */
static const uint32_t s_i2cBaseAddrs[FSL_FEATURE_SOC_I2C_COUNT] = I2C_BASE_ADDRS;

/*! @brief IRQ name array */
static const IRQn_Type s_i2cIRQ[] = I2C_IRQS;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Returns an instance number given a base address.
 *
 * If an invalid base address is passed, debug builds will assert. Release builds will just return
 * instance number 0.
 *
 * @param base The I2C peripheral base address.
 * @return I2C instance number starting from 0.
 */
/*!
 * brief Returns an instance number given a base address.
 *
 * If an invalid base address is passed, debug builds will assert. Release builds will just return
 * instance number 0.
 *
 * param base The I2C peripheral base address.
 * return I2C instance number starting from 0.
 */
uint32_t I2C_GetInstance(I2C_Type *base)
{
    int i;
    for (i = 0; i < FSL_FEATURE_SOC_I2C_COUNT; i++)
    {
        if ((uint32_t)base == s_i2cBaseAddrs[i])
        {
            return i;
        }
    }
    assert(false);
    return 0;
}

/*!
 * brief Provides a default configuration for the I2C master peripheral.
 *
 * This function provides the following default configuration for the I2C master peripheral:
 * code
 *  masterConfig->enableMaster            = true;
 *  masterConfig->baudRate_Bps            = 100000U;
 *  masterConfig->enableTimeout           = false;
 * endcode
 *
 * After calling this function, you can override any settings in order to customize the configuration,
 * prior to initializing the master driver with I2C_MasterInit().
 *
 * param[out] masterConfig User provided configuration structure for default values. Refer to #i2c_master_config_t.
 */
void I2C_MasterGetDefaultConfig(i2c_master_config_t *masterConfig)
{
    /* Initializes the configure structure to zero. */
    memset(masterConfig, 0, sizeof(*masterConfig));

    masterConfig->enableMaster = true;
    masterConfig->baudRate_Bps = 100000U;
    masterConfig->enableTimeout = false;
}

/*!
 * brief Initializes the I2C master peripheral.
 *
 * This function enables the peripheral clock and initializes the I2C master peripheral as described by the user
 * provided configuration. A software reset is performed prior to configuration.
 *
 * param base The I2C peripheral base address.
 * param masterConfig User provided peripheral configuration. Use I2C_MasterGetDefaultConfig() to get a set of
 * defaults
 *      that you can override.
 * param srcClock_Hz Frequency in Hertz of the I2C functional clock. Used to calculate the baud rate divisors,
 *      filter widths, and timeout periods.
 */
void I2C_MasterInit(I2C_Type *base, const i2c_master_config_t *masterConfig, uint32_t srcClock_Hz)
{
    FLEXCOMM_Init(base, FLEXCOMM_PERIPH_I2C);
    I2C_MasterEnable(base, masterConfig->enableMaster);
    I2C_MasterSetBaudRate(base, masterConfig->baudRate_Bps, srcClock_Hz);
}

/*!
* brief Deinitializes the I2C master peripheral.
*
 * This function disables the I2C master peripheral and gates the clock. It also performs a software
 * reset to restore the peripheral to reset conditions.
 *
 * param base The I2C peripheral base address.
 */
void I2C_MasterDeinit(I2C_Type *base)
{
    I2C_MasterEnable(base, false);
}

/*!
 * brief Sets the I2C bus frequency for master transactions.
 *
 * The I2C master is automatically disabled and re-enabled as necessary to configure the baud
 * rate. Do not call this function during a transfer, or the transfer is aborted.
 *
 * param base The I2C peripheral base address.
 * param srcClock_Hz I2C functional clock frequency in Hertz.
 * param baudRate_Bps Requested bus frequency in bits per second.
 */
void I2C_MasterSetBaudRate(I2C_Type *base, uint32_t baudRate_Bps, uint32_t srcClock_Hz)
{
    uint32_t scl, divider;
    uint32_t best_scl, best_div;
    uint32_t err, best_err;

    best_err = 0;

    for (scl = 9; scl >= 2; scl--)
    {
        /* calculated ideal divider value for given scl */
        divider = srcClock_Hz / (baudRate_Bps * scl * 2u);

        /* adjust it if it is out of range */
        divider = (divider > 0x10000u) ? 0x10000 : divider;

        /* calculate error */
        err = srcClock_Hz - (baudRate_Bps * scl * 2u * divider);
        if ((err < best_err) || (best_err == 0))
        {
            best_div = divider;
            best_scl = scl;
            best_err = err;
        }

        if ((err == 0) || (divider >= 0x10000u))
        {
            /* either exact value was found
               or divider is at its max (it would even greater in the next iteration for sure) */
            break;
        }
    }

    base->CLKDIV = I2C_CLKDIV_DIVVAL(best_div - 1);
    base->MSTTIME = I2C_MSTTIME_MSTSCLLOW(best_scl - 2u) | I2C_MSTTIME_MSTSCLHIGH(best_scl - 2u);
}

static uint32_t I2C_PendingStatusWait(I2C_Type *base)
{
    uint32_t status;

#if I2C_WAIT_TIMEOUT
    uint32_t waitTimes = I2C_WAIT_TIMEOUT;
#endif

    do
    {
        status = I2C_GetStatusFlags(base);
#if I2C_WAIT_TIMEOUT
    } while (((status & I2C_STAT_MSTPENDING_MASK) == 0) && (--waitTimes));

    if (waitTimes == 0)
    {
        return kStatus_I2C_Timeout;
    }
#else
    } while ((status & I2C_STAT_MSTPENDING_MASK) == 0);
#endif

    /* Clear controller state. */
    I2C_MasterClearStatusFlags(base, I2C_STAT_MSTARBLOSS_MASK | I2C_STAT_MSTSTSTPERR_MASK);

    return status;
}

/*!
 * brief Sends a START on the I2C bus.
 *
 * This function is used to initiate a new master mode transfer by sending the START signal.
 * The slave address is sent following the I2C START signal.
 *
 * param base I2C peripheral base pointer
 * param address 7-bit slave device address.
 * param direction Master transfer directions(transmit/receive).
 * retval kStatus_Success Successfully send the start signal.
 * retval kStatus_I2C_Busy Current bus is busy.
 */
status_t I2C_MasterStart(I2C_Type *base, uint8_t address, i2c_direction_t direction)
{
    status_t result;
    result = I2C_PendingStatusWait(base);
    if (result == kStatus_I2C_Timeout)
    {
        return kStatus_I2C_Timeout;
    }

    /* Write Address and RW bit to data register */
    base->MSTDAT = ((uint32_t)address << 1) | ((uint32_t)direction & 1u);
    /* Start the transfer */
    base->MSTCTL = I2C_MSTCTL_MSTSTART_MASK;

    return kStatus_Success;
}

/*!
 * brief Sends a STOP signal on the I2C bus.
 *
 * retval kStatus_Success Successfully send the stop signal.
 * retval kStatus_I2C_Timeout Send stop signal failed, timeout.
 */
status_t I2C_MasterStop(I2C_Type *base)
{
    status_t result;
    result = I2C_PendingStatusWait(base);
    if (result == kStatus_I2C_Timeout)
    {
        return kStatus_I2C_Timeout;
    }

    base->MSTCTL = I2C_MSTCTL_MSTSTOP_MASK;
    return kStatus_Success;
}

/*!
 * brief Performs a polling send transfer on the I2C bus.
 *
 * Sends up to a txSize number of bytes to the previously addressed slave device. The slave may
 * reply with a NAK to any byte in order to terminate the transfer early. If this happens, this
 * function returns #kStatus_I2C_Nak.
 *
 * param base  The I2C peripheral base address.
 * param txBuff The pointer to the data to be transferred.
 * param txSize The length in bytes of the data to be transferred.
 * param flags Transfer control flag to control special behavior like suppressing start or stop, for normal transfers
 * use kI2C_TransferDefaultFlag
 * retval kStatus_Success Data was sent successfully.
 * retval #kStatus_I2C_Busy Another master is currently utilizing the bus.
 * retval #kStatus_I2C_Nak The slave device sent a NAK in response to a byte.
 * retval #kStatus_I2C_ArbitrationLost Arbitration lost error.
 */
status_t I2C_MasterWriteBlocking(I2C_Type *base, const void *txBuff, size_t txSize, uint32_t flags)
{
    uint32_t status;
    uint32_t master_state;
    status_t err;

    const uint8_t *buf = (const uint8_t *)(uintptr_t)txBuff;

    assert(txBuff);

    err = kStatus_Success;
    while (txSize)
    {
        status = I2C_PendingStatusWait(base);

#if I2C_WAIT_TIMEOUT
        if (status == kStatus_I2C_Timeout)
        {
            return kStatus_I2C_Timeout;
        }
#endif

        if (status & I2C_STAT_MSTARBLOSS_MASK)
        {
            return kStatus_I2C_ArbitrationLost;
        }

        if (status & I2C_STAT_MSTSTSTPERR_MASK)
        {
            return kStatus_I2C_StartStopError;
        }

        master_state = (status & I2C_STAT_MSTSTATE_MASK) >> I2C_STAT_MSTSTATE_SHIFT;
        switch (master_state)
        {
            case I2C_STAT_MSTCODE_TXREADY:
                /* ready to send next byte */
                base->MSTDAT = *buf++;
                txSize--;
                base->MSTCTL = I2C_MSTCTL_MSTCONTINUE_MASK;
                break;

            case I2C_STAT_MSTCODE_NACKADR:
            case I2C_STAT_MSTCODE_NACKDAT:
                /* slave nacked the last byte */
                err = kStatus_I2C_Nak;
                break;

            default:
                /* unexpected state */
                err = kStatus_I2C_UnexpectedState;
                break;
        }

        if (err != kStatus_Success)
        {
            return err;
        }
    }

    status = I2C_PendingStatusWait(base);

#if I2C_WAIT_TIMEOUT
    if (status == kStatus_I2C_Timeout)
    {
        return kStatus_I2C_Timeout;
    }
#endif

    if ((status & (I2C_STAT_MSTARBLOSS_MASK | I2C_STAT_MSTSTSTPERR_MASK)) == 0)
    {
        if (!(flags & kI2C_TransferNoStopFlag))
        {
            /* Initiate stop */
            base->MSTCTL = I2C_MSTCTL_MSTSTOP_MASK;
            status = I2C_PendingStatusWait(base);
            if (status == kStatus_I2C_Timeout)
            {
                return kStatus_I2C_Timeout;
            }
        }
    }

    if (status & I2C_STAT_MSTARBLOSS_MASK)
    {
        return kStatus_I2C_ArbitrationLost;
    }

    if (status & I2C_STAT_MSTSTSTPERR_MASK)
    {
        return kStatus_I2C_StartStopError;
    }

    return kStatus_Success;
}

/*!
 * brief Performs a polling receive transfer on the I2C bus.
 *
 * param base  The I2C peripheral base address.
 * param rxBuff The pointer to the data to be transferred.
 * param rxSize The length in bytes of the data to be transferred.
 * param flags Transfer control flag to control special behavior like suppressing start or stop, for normal transfers
 * use kI2C_TransferDefaultFlag
 * retval kStatus_Success Data was received successfully.
 * retval #kStatus_I2C_Busy Another master is currently utilizing the bus.
 * retval #kStatus_I2C_Nak The slave device sent a NAK in response to a byte.
 * retval #kStatus_I2C_ArbitrationLost Arbitration lost error.
 */
status_t I2C_MasterReadBlocking(I2C_Type *base, void *rxBuff, size_t rxSize, uint32_t flags)
{
    uint32_t status = 0;
    uint32_t master_state;
    status_t err;

    uint8_t *buf = (uint8_t *)(rxBuff);

    assert(rxBuff);

    err = kStatus_Success;
    while (rxSize)
    {
        status = I2C_PendingStatusWait(base);

#if I2C_WAIT_TIMEOUT
        if (status == kStatus_I2C_Timeout)
        {
            return kStatus_I2C_Timeout;
        }
#endif

        if (status & (I2C_STAT_MSTARBLOSS_MASK | I2C_STAT_MSTSTSTPERR_MASK))
        {
            break;
        }

        master_state = (status & I2C_STAT_MSTSTATE_MASK) >> I2C_STAT_MSTSTATE_SHIFT;
        switch (master_state)
        {
            case I2C_STAT_MSTCODE_RXREADY:
                /* ready to send next byte */
                *(buf++) = base->MSTDAT;
                if (--rxSize)
                {
                    base->MSTCTL = I2C_MSTCTL_MSTCONTINUE_MASK;
                }
                else
                {
                    if ((flags & kI2C_TransferNoStopFlag) == 0)
                    {
                        /* initiate NAK and stop */
                        base->MSTCTL = I2C_MSTCTL_MSTSTOP_MASK;
                        status = I2C_PendingStatusWait(base);

#if I2C_WAIT_TIMEOUT
                        if (status == kStatus_I2C_Timeout)
                        {
                            return kStatus_I2C_Timeout;
                        }
#endif
                    }
                }
                break;

            case I2C_STAT_MSTCODE_NACKADR:
            case I2C_STAT_MSTCODE_NACKDAT:
                /* slave nacked the last byte */
                err = kStatus_I2C_Nak;
                break;

            default:
                /* unexpected state */
                err = kStatus_I2C_UnexpectedState;
                break;
        }

        if (err != kStatus_Success)
        {
            return err;
        }
    }

    if (status & I2C_STAT_MSTARBLOSS_MASK)
    {
        return kStatus_I2C_ArbitrationLost;
    }

    if (status & I2C_STAT_MSTSTSTPERR_MASK)
    {
        return kStatus_I2C_StartStopError;
    }

    return kStatus_Success;
}

/*!
 * brief Performs a master polling transfer on the I2C bus.
 *
 * note The API does not return until the transfer succeeds or fails due
 * to arbitration lost or receiving a NAK.
 *
 * param base I2C peripheral base address.
 * param xfer Pointer to the transfer structure.
 * retval kStatus_Success Successfully complete the data transmission.
 * retval kStatus_I2C_Busy Previous transmission still not finished.
 * retval kStatus_I2C_Timeout Transfer error, wait signal timeout.
 * retval kStatus_I2C_ArbitrationLost Transfer error, arbitration lost.
 * retval kStataus_I2C_Nak Transfer error, receive NAK during transfer.
 */
status_t I2C_MasterTransferBlocking(I2C_Type *base, i2c_master_transfer_t *xfer)
{
    status_t result = kStatus_Success;
    uint32_t subaddress;
    uint8_t subaddrBuf[4];
    int i;

    assert(xfer);

    /* If repeated start is requested, send repeated start. */
    if (!(xfer->flags & kI2C_TransferNoStartFlag))
    {
        if (xfer->subaddressSize)
        {
            result = I2C_MasterStart(base, xfer->slaveAddress, kI2C_Write);
            if (result == kStatus_Success)
            {
                /* Prepare subaddress transmit buffer, most significant byte is stored at the lowest address */
                subaddress = xfer->subaddress;
                for (i = xfer->subaddressSize - 1; i >= 0; i--)
                {
                    subaddrBuf[i] = subaddress & 0xff;
                    subaddress >>= 8;
                }
                /* Send subaddress. */
                result = I2C_MasterWriteBlocking(base, subaddrBuf, xfer->subaddressSize, kI2C_TransferNoStopFlag);
                if ((result == kStatus_Success) && (xfer->direction == kI2C_Read))
                {
                    result = I2C_MasterRepeatedStart(base, xfer->slaveAddress, xfer->direction);
                }
            }
        }
        else if (xfer->flags & kI2C_TransferRepeatedStartFlag)
        {
            result = I2C_MasterRepeatedStart(base, xfer->slaveAddress, xfer->direction);
        }
        else
        {
            result = I2C_MasterStart(base, xfer->slaveAddress, xfer->direction);
        }
    }

    if (result == kStatus_Success)
    {
        if ((xfer->direction == kI2C_Write) && (xfer->dataSize > 0))
        {
            /* Transmit data. */
            result = I2C_MasterWriteBlocking(base, xfer->data, xfer->dataSize, xfer->flags);
        }
        else
        {
            if ((xfer->direction == kI2C_Read) && (xfer->dataSize > 0))
            {
                /* Receive Data. */
                result = I2C_MasterReadBlocking(base, xfer->data, xfer->dataSize, xfer->flags);
            }
        }
    }

    if (result == kStatus_I2C_Nak)
    {
        I2C_MasterStop(base);
    }

    return result;
}

/*!
 * brief Creates a new handle for the I2C master non-blocking APIs.
 *
 * The creation of a handle is for use with the non-blocking APIs. Once a handle
 * is created, there is not a corresponding destroy handle. If the user wants to
 * terminate a transfer, the I2C_MasterTransferAbort() API shall be called.
 *
 * param base The I2C peripheral base address.
 * param[out] handle Pointer to the I2C master driver handle.
 * param callback User provided pointer to the asynchronous callback function.
 * param userData User provided pointer to the application callback data.
 */
void I2C_MasterTransferCreateHandle(I2C_Type *base,
                                    i2c_master_handle_t *handle,
                                    i2c_master_transfer_callback_t callback,
                                    void *userData)
{
    uint32_t instance;

    assert(handle);

    /* Clear out the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Look up instance number */
    instance = I2C_GetInstance(base);

    /* Save base and instance. */
    handle->completionCallback = callback;
    handle->userData = userData;

    FLEXCOMM_SetIRQHandler(base, (flexcomm_irq_handler_t)I2C_MasterTransferHandleIRQ, handle);

    /* Clear internal IRQ enables and enable NVIC IRQ. */
    I2C_DisableInterrupts(base, kI2C_MasterIrqFlags);
    EnableIRQ(s_i2cIRQ[instance]);
}

/*!
 * brief Performs a non-blocking transaction on the I2C bus.
 *
 * param base The I2C peripheral base address.
 * param handle Pointer to the I2C master driver handle.
 * param xfer The pointer to the transfer descriptor.
 * retval kStatus_Success The transaction was started successfully.
 * retval #kStatus_I2C_Busy Either another master is currently utilizing the bus, or a non-blocking
 *      transaction is already in progress.
 */
status_t I2C_MasterTransferNonBlocking(I2C_Type *base, i2c_master_handle_t *handle, i2c_master_transfer_t *xfer)
{
    status_t result;

    assert(handle);
    assert(xfer);
    assert(xfer->subaddressSize <= sizeof(xfer->subaddress));

    /* Return busy if another transaction is in progress. */
    if (handle->state != kIdleState)
    {
        return kStatus_I2C_Busy;
    }

    /* Disable I2C IRQ sources while we configure stuff. */
    I2C_DisableInterrupts(base, kI2C_MasterIrqFlags);

    /* Prepare transfer state machine. */
    result = I2C_InitTransferStateMachine(base, handle, xfer);

    /* Clear error flags. */
    I2C_MasterClearStatusFlags(base, I2C_STAT_MSTARBLOSS_MASK | I2C_STAT_MSTSTSTPERR_MASK);

    /* Enable I2C internal IRQ sources. */
    I2C_EnableInterrupts(base, kI2C_MasterIrqFlags);

    return result;
}

/*!
 * brief Returns number of bytes transferred so far.
 * param base The I2C peripheral base address.
 * param handle Pointer to the I2C master driver handle.
 * param[out] count Number of bytes transferred so far by the non-blocking transaction.
 * retval kStatus_Success
 * retval #kStatus_I2C_Busy
 */
status_t I2C_MasterTransferGetCount(I2C_Type *base, i2c_master_handle_t *handle, size_t *count)
{
    assert(handle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    /* Catch when there is not an active transfer. */
    if (handle->state == kIdleState)
    {
        *count = 0;
        return kStatus_NoTransferInProgress;
    }

    /* There is no necessity to disable interrupts as we read a single integer value */
    *count = handle->transferCount;
    return kStatus_Success;
}

/*!
 * brief Terminates a non-blocking I2C master transmission early.
 *
 * note It is not safe to call this function from an IRQ handler that has a higher priority than the
 *      I2C peripheral's IRQ priority.
 *
 * param base The I2C peripheral base address.
 * param handle Pointer to the I2C master driver handle.
 * retval kStatus_Success A transaction was successfully aborted.
 * retval #kStatus_I2C_Timeout Timeout during polling for flags.
 */
status_t I2C_MasterTransferAbort(I2C_Type *base, i2c_master_handle_t *handle)
{
    uint32_t status;
    uint32_t master_state;

    if (handle->state != kIdleState)
    {
        /* Disable internal IRQ enables. */
        I2C_DisableInterrupts(base, kI2C_MasterIrqFlags);

        /* Wait until module is ready */
        status = I2C_PendingStatusWait(base);

#if I2C_WAIT_TIMEOUT
        if (status == kStatus_I2C_Timeout)
        {
            /* Reset handle to idle state. */
            handle->state = kIdleState;
            return kStatus_I2C_Timeout;
        }
#endif

        /* Get the state of the I2C module */
        master_state = (status & I2C_STAT_MSTSTATE_MASK) >> I2C_STAT_MSTSTATE_SHIFT;

        if (master_state != I2C_STAT_MSTCODE_IDLE)
        {
            /* Send a stop command to finalize the transfer. */
            base->MSTCTL = I2C_MSTCTL_MSTSTOP_MASK;

            /* Wait until the STOP is completed */
            status = I2C_PendingStatusWait(base);
            if (status == kStatus_I2C_Timeout)
            {
                return kStatus_I2C_Timeout;
            }
        }

        /* Reset handle. */
        handle->state = kIdleState;
    }
    return kStatus_Success;
}

/*!
 * @brief Prepares the transfer state machine and fills in the command buffer.
 * @param handle Master nonblocking driver handle.
 */
static status_t I2C_InitTransferStateMachine(I2C_Type *base, i2c_master_handle_t *handle, i2c_master_transfer_t *xfer)
{
    struct _i2c_master_transfer *transfer;

    handle->transfer = *xfer;
    transfer = &(handle->transfer);

    handle->transferCount = 0;
    handle->remainingBytes = transfer->dataSize;
    handle->buf = (uint8_t *)transfer->data;
    handle->remainingSubaddr = 0;

    if (transfer->flags & kI2C_TransferNoStartFlag)
    {
        /* Start condition shall be ommited, switch directly to next phase */
        if (transfer->dataSize == 0)
        {
            handle->state = kStopState;
        }
        else if (handle->transfer.direction == kI2C_Write)
        {
            handle->state = kTransmitDataState;
        }
        else if (handle->transfer.direction == kI2C_Read)
        {
            handle->state = kReceiveDataState;
        }
        else
        {
            return kStatus_I2C_InvalidParameter;
        }
    }
    else
    {
        if (transfer->subaddressSize != 0)
        {
            int i;
            uint32_t subaddress;

            if (transfer->subaddressSize > sizeof(handle->subaddrBuf))
            {
                return kStatus_I2C_InvalidParameter;
            }

            /* Prepare subaddress transmit buffer, most significant byte is stored at the lowest address */
            subaddress = xfer->subaddress;
            for (i = xfer->subaddressSize - 1; i >= 0; i--)
            {
                handle->subaddrBuf[i] = subaddress & 0xff;
                subaddress >>= 8;
            }
            handle->remainingSubaddr = transfer->subaddressSize;
        }
        handle->state = kStartState;
    }

    return kStatus_Success;
}

/*!
 * @brief Execute states until FIFOs are exhausted.
 * @param handle Master nonblocking driver handle.
 * @param[out] isDone Set to true if the transfer has completed.
 * @retval #kStatus_Success
 * @retval #kStatus_I2C_ArbitrationLost
 * @retval #kStatus_I2C_Nak
 */
static status_t I2C_RunTransferStateMachine(I2C_Type *base, i2c_master_handle_t *handle, bool *isDone)
{
    uint32_t status;
    uint32_t master_state;
    struct _i2c_master_transfer *transfer;
    status_t err;

    transfer = &(handle->transfer);
    bool ignoreNak = ((handle->state == kStopState) && (handle->remainingBytes == 0U)) ||
                     ((handle->state == kWaitForCompletionState) && (handle->remainingBytes == 0U));

    *isDone = false;

    status = I2C_GetStatusFlags(base);

    if (status & I2C_STAT_MSTARBLOSS_MASK)
    {
        I2C_MasterClearStatusFlags(base, I2C_STAT_MSTARBLOSS_MASK);
        return kStatus_I2C_ArbitrationLost;
    }

    if (status & I2C_STAT_MSTSTSTPERR_MASK)
    {
        I2C_MasterClearStatusFlags(base, I2C_STAT_MSTSTSTPERR_MASK);
        return kStatus_I2C_StartStopError;
    }

    if ((status & I2C_STAT_MSTPENDING_MASK) == 0)
    {
        return kStatus_I2C_Busy;
    }

    /* Get the state of the I2C module */
    master_state = (status & I2C_STAT_MSTSTATE_MASK) >> I2C_STAT_MSTSTATE_SHIFT;

    if (((master_state == I2C_STAT_MSTCODE_NACKADR) || (master_state == I2C_STAT_MSTCODE_NACKDAT)) && (ignoreNak != true))
    {
        /* Slave NACKed last byte, issue stop and return error */
        base->MSTCTL = I2C_MSTCTL_MSTSTOP_MASK;
        handle->state = kWaitForCompletionState;
        return kStatus_I2C_Nak;
    }

    err = kStatus_Success;
    switch (handle->state)
    {
        case kStartState:
            if (handle->remainingSubaddr)
            {
                /* Subaddress takes precedence over the data transfer, direction is always "write" in this case */
                base->MSTDAT = (uint32_t)transfer->slaveAddress << 1;
                handle->state = kTransmitSubaddrState;
            }
            else if (transfer->direction == kI2C_Write)
            {
                base->MSTDAT = (uint32_t)transfer->slaveAddress << 1;
                handle->state = handle->remainingBytes ? kTransmitDataState : kStopState;
            }
            else
            {
                base->MSTDAT = ((uint32_t)transfer->slaveAddress << 1) | 1u;
                handle->state = handle->remainingBytes ? kReceiveDataState : kStopState;
            }
            /* Send start condition */
            base->MSTCTL = I2C_MSTCTL_MSTSTART_MASK;
            break;

        case kTransmitSubaddrState:
            if (master_state != I2C_STAT_MSTCODE_TXREADY)
            {
                return kStatus_I2C_UnexpectedState;
            }

            /* Most significant subaddress byte comes first */
            base->MSTDAT = handle->subaddrBuf[handle->transfer.subaddressSize - handle->remainingSubaddr];
            base->MSTCTL = I2C_MSTCTL_MSTCONTINUE_MASK;
            if (--(handle->remainingSubaddr))
            {
                /* There are still subaddress bytes to be transmitted */
                break;
            }
            if (handle->remainingBytes)
            {
                /* There is data to be transferred, if there is write to read turnaround it is necessary to perform
                 * repeated start */
                handle->state = (transfer->direction == kI2C_Read) ? kStartState : kTransmitDataState;
            }
            else
            {
                /* No more data, schedule stop condition */
                handle->state = kStopState;
            }
            break;

        case kTransmitDataState:
            if (master_state != I2C_STAT_MSTCODE_TXREADY)
            {
                return kStatus_I2C_UnexpectedState;
            }
            base->MSTDAT = *(handle->buf)++;
            base->MSTCTL = I2C_MSTCTL_MSTCONTINUE_MASK;
            if (--handle->remainingBytes == 0)
            {
                /* No more data, schedule stop condition */
                handle->state = kStopState;
            }
            handle->transferCount++;
            break;

        case kReceiveDataState:
            if (master_state != I2C_STAT_MSTCODE_RXREADY)
            {
                return kStatus_I2C_UnexpectedState;
            }
            *(handle->buf)++ = base->MSTDAT;
            if (--handle->remainingBytes)
            {
                base->MSTCTL = I2C_MSTCTL_MSTCONTINUE_MASK;
            }
            else
            {
                /* No more data expected, issue NACK and STOP right away */
                base->MSTCTL = I2C_MSTCTL_MSTSTOP_MASK;
                handle->state = kWaitForCompletionState;
            }
            handle->transferCount++;
            break;

        case kStopState:
            if (transfer->flags & kI2C_TransferNoStopFlag)
            {
                /* Stop condition is omitted, we are done */
                *isDone = true;
                handle->state = kIdleState;
                break;
            }
            /* Send stop condition */
            base->MSTCTL = I2C_MSTCTL_MSTSTOP_MASK;
            handle->state = kWaitForCompletionState;
            break;

        case kWaitForCompletionState:
            *isDone = true;
            handle->state = kIdleState;
            break;

        case kIdleState:
        default:
            /* State machine shall not be invoked again once it enters the idle state */
            err = kStatus_I2C_UnexpectedState;
            break;
    }

    return err;
}

/*!
 * brief Reusable routine to handle master interrupts.
 * note This function does not need to be called unless you are reimplementing the
 *  nonblocking API's interrupt handler routines to add special functionality.
 * param base The I2C peripheral base address.
 * param handle Pointer to the I2C master driver handle.
 */
void I2C_MasterTransferHandleIRQ(I2C_Type *base, i2c_master_handle_t *handle)
{
    bool isDone;
    status_t result;

    /* Don't do anything if we don't have a valid handle. */
    if (!handle)
    {
        return;
    }

    result = I2C_RunTransferStateMachine(base, handle, &isDone);

    if (isDone || (result != kStatus_Success))
    {
        /* Restore handle to idle state. */
        handle->state = kIdleState;

        /* Disable internal IRQ enables. */
        I2C_DisableInterrupts(base, kI2C_MasterIrqFlags);

        /* Invoke callback. */
        if (handle->completionCallback)
        {
            handle->completionCallback(base, handle, result, handle->userData);
        }
    }
}

/*!
 * @brief Sets the hardware slave state machine to reset
 *
 * Per documentation, the only the state machine is reset, the configuration settings remain.
 *
 * @param base The I2C peripheral base address.
 */
static void I2C_SlaveInternalStateMachineReset(I2C_Type *base)
{
    I2C_SlaveEnable(base, false); /* clear SLVEN Slave enable bit */
}

/*!
 * @brief Compute CLKDIV
 *
 * This function computes CLKDIV value according to the given bus speed and Flexcomm source clock frequency.
 * This setting is used by hardware during slave clock stretching.
 *
 * @param base The I2C peripheral base address.
 * @return status of the operation
 */
static status_t I2C_SlaveDivVal(uint32_t srcClock_Hz, i2c_slave_bus_speed_t busSpeed, uint32_t *divVal)
{
    uint32_t dataSetupTime_ns;

    switch (busSpeed)
    {
        case kI2C_SlaveStandardMode:
            dataSetupTime_ns = 250u;
            break;

        case kI2C_SlaveFastMode:
            dataSetupTime_ns = 100u;
            break;

        case kI2C_SlaveFastModePlus:
            dataSetupTime_ns = 50u;
            break;

        case kI2C_SlaveHsMode:
            dataSetupTime_ns = 10u;
            break;

        default:
            dataSetupTime_ns = 0;
            break;
    }

    if (0 == dataSetupTime_ns)
    {
        return kStatus_InvalidArgument;
    }

    /* divVal = (sourceClock_Hz / 1000000) * (dataSetupTime_ns / 1000) */
    *divVal = srcClock_Hz / 1000u;
    *divVal = (*divVal) * dataSetupTime_ns;
    *divVal = (*divVal) / 1000000u;

    if ((*divVal) > I2C_CLKDIV_DIVVAL_MASK)
    {
        *divVal = I2C_CLKDIV_DIVVAL_MASK;
    }

    return kStatus_Success;
}

/*!
 * @brief Poll wait for the SLVPENDING flag.
 *
 * Wait for the pending status to be set (SLVPENDING = 1) by polling the STAT register.
 *
 * @param base The I2C peripheral base address.
 * @return status register at time the SLVPENDING bit is read as set
 */
static uint32_t I2C_SlavePollPending(I2C_Type *base)
{
    uint32_t stat;

#if I2C_WAIT_TIMEOUT
    uint32_t waitTimes = I2C_WAIT_TIMEOUT;
#endif
    do
    {
        stat = base->STAT;
#if I2C_WAIT_TIMEOUT
    } while ((0u == (stat & I2C_STAT_SLVPENDING_MASK)) && (--waitTimes));

    if (waitTimes == 0u)
    {
        return kStatus_I2C_Timeout;
    }
#else
    } while (0u == (stat & I2C_STAT_SLVPENDING_MASK));
#endif

    return stat;
}

/*!
 * @brief Invoke event from I2C_SlaveTransferHandleIRQ().
 *
 * Sets the event type to transfer structure and invokes the event callback, if it has been
 * enabled by eventMask.
 *
 * @param base The I2C peripheral base address.
 * @param handle The I2C slave handle for non-blocking APIs.
 * @param event The I2C slave event to invoke.
 */
static void I2C_SlaveInvokeEvent(I2C_Type *base, i2c_slave_handle_t *handle, i2c_slave_transfer_event_t event)
{
    handle->transfer.event = event;
    if ((handle->callback) && (handle->transfer.eventMask & event))
    {
        handle->callback(base, &handle->transfer, handle->userData);

        /* if after event callback we have data buffer (callback func has added new data), keep transfer busy */
        if (false == handle->isBusy)
        {
            if (((handle->transfer.txData) && (handle->transfer.txSize)) ||
                ((handle->transfer.rxData) && (handle->transfer.rxSize)))
            {
                handle->isBusy = true;
            }
        }

        /* Clear the transferred count now that we have a new buffer. */
        if ((event == kI2C_SlaveReceiveEvent) || (event == kI2C_SlaveTransmitEvent))
        {
            handle->transfer.transferredCount = 0;
        }
    }
}

/*!
 * @brief Handle slave address match event.
 *
 * Called by Slave interrupt routine to ACK or NACK the matched address.
 * It also determines master direction (read or write).
 *
 * @param base The I2C peripheral base address.
 * @return true if the matched address is ACK'ed
 * @return false if the matched address is NACK'ed
 */
static bool I2C_SlaveAddressIRQ(I2C_Type *base, i2c_slave_handle_t *handle)
{
    uint8_t addressByte0;

    addressByte0 = (uint8_t)base->SLVDAT;

    /* store the matched address */
    handle->transfer.receivedAddress = addressByte0;

    /* R/nW */
    if (addressByte0 & 1u)
    {
        /* if we have no data in this transfer, call callback to get new */
        if ((handle->transfer.txData == NULL) || (handle->transfer.txSize == 0))
        {
            I2C_SlaveInvokeEvent(base, handle, kI2C_SlaveTransmitEvent);
        }

        /* NACK if we have no data in this transfer. */
        if ((handle->transfer.txData == NULL) || (handle->transfer.txSize == 0))
        {
            base->SLVCTL = I2C_SLVCTL_SLVNACK_MASK;
            return false;
        }

        /* master wants to read, so slave transmit is next state */
        handle->slaveFsm = kI2C_SlaveFsmTransmit;
    }
    else
    {
        /* if we have no receive buffer in this transfer, call callback to get new */
        if ((handle->transfer.rxData == NULL) || (handle->transfer.rxSize == 0))
        {
            I2C_SlaveInvokeEvent(base, handle, kI2C_SlaveReceiveEvent);
        }

        /* NACK if we have no data in this transfer */
        if ((handle->transfer.rxData == NULL) || (handle->transfer.rxSize == 0))
        {
            base->SLVCTL = I2C_SLVCTL_SLVNACK_MASK;
            return false;
        }

        /* master wants write, so slave receive is next state */
        handle->slaveFsm = kI2C_SlaveFsmReceive;
    }

    /* continue transaction */
    base->SLVCTL = I2C_SLVCTL_SLVCONTINUE_MASK;

    return true;
}

/*!
 * @brief Starts accepting slave transfers.
 *
 * Call this API after calling I2C_SlaveInit() and I2C_SlaveTransferCreateHandle() to start processing
 * transactions driven by an I2C master. The slave monitors the I2C bus and pass events to the
 * callback that was passed into the call to I2C_SlaveTransferCreateHandle(). The callback is always invoked
 * from the interrupt context.
 *
 * @param base The I2C peripheral base address.
 * @param handle Pointer to #i2c_slave_handle_t structure which stores the transfer state.
 * @param txData Data to be transmitted to master in response to master read from slave requests. NULL if slave RX only.
 * @param txSize Size of txData buffer in bytes.
 * @param rxData Data where received data from master will be stored in response to master write to slave requests. NULL
 *               if slave TX only.
 * @param rxSize Size of rxData buffer in bytes.
 *
 * @retval #kStatus_Success Slave transfers were successfully started.
 * @retval #kStatus_I2C_Busy Slave transfers have already been started on this handle.
 */
static status_t I2C_SlaveTransferNonBlockingInternal(I2C_Type *base,
                                                     i2c_slave_handle_t *handle,
                                                     const void *txData,
                                                     size_t txSize,
                                                     void *rxData,
                                                     size_t rxSize,
                                                     uint32_t eventMask)
{
    status_t status;

    assert(handle);

    status = kStatus_Success;

    /* Disable I2C IRQ sources while we configure stuff. */
    I2C_DisableInterrupts(base, kI2C_SlaveIrqFlags);

    /* Return busy if another transaction is in progress. */
    if (handle->isBusy)
    {
        status = kStatus_I2C_Busy;
    }

    /* Save transfer into handle. */
    handle->transfer.txData = (const uint8_t *)(uintptr_t)txData;
    handle->transfer.txSize = txSize;
    handle->transfer.rxData = (uint8_t *)rxData;
    handle->transfer.rxSize = rxSize;
    handle->transfer.transferredCount = 0;
    handle->transfer.eventMask = eventMask | kI2C_SlaveTransmitEvent | kI2C_SlaveReceiveEvent;
    handle->isBusy = true;

    /* Set the SLVEN bit to 1 in the CFG register. */
    I2C_SlaveEnable(base, true);

    /* Clear w1c flags. */
    base->STAT |= 0u;

    /* Enable I2C internal IRQ sources. */
    I2C_EnableInterrupts(base, kI2C_SlaveIrqFlags);

    return status;
}

/*!
 * brief Starts accepting master read from slave requests.
 *
 * The function can be called in response to #kI2C_SlaveTransmitEvent callback to start a new slave Tx transfer
 * from within the transfer callback.
 *
 * The set of events received by the callback is customizable. To do so, set the a eventMask parameter to
 * the OR'd combination of #i2c_slave_transfer_event_t enumerators for the events you wish to receive.
 * The #kI2C_SlaveTransmitEvent and #kI2C_SlaveReceiveEvent events are always enabled and do not need
 * to be included in the mask. Alternatively, you can pass 0 to get a default set of only the transmit and
 * receive events that are always enabled. In addition, the #kI2C_SlaveAllEvents constant is provided as
 * a convenient way to enable all events.
 *
 * param base The I2C peripheral base address.
 * param transfer Pointer to #i2c_slave_transfer_t structure.
 * param txData Pointer to data to send to master.
 * param txSize Size of txData in bytes.
 * param eventMask Bit mask formed by OR'ing together #i2c_slave_transfer_event_t enumerators to specify
 *      which events to send to the callback. Other accepted values are 0 to get a default set of
 *      only the transmit and receive events, and #kI2C_SlaveAllEvents to enable all events.
 *
 * retval kStatus_Success Slave transfers were successfully started.
 * retval #kStatus_I2C_Busy Slave transfers have already been started on this handle.
 */
status_t I2C_SlaveSetSendBuffer(
    I2C_Type *base, volatile i2c_slave_transfer_t *transfer, const void *txData, size_t txSize, uint32_t eventMask)
{
    return I2C_SlaveTransferNonBlockingInternal(base, transfer->handle, txData, txSize, NULL, 0u, eventMask);
}

/*!
 * brief Starts accepting master write to slave requests.
  *
 * The function can be called in response to #kI2C_SlaveReceiveEvent callback to start a new slave Rx transfer
 * from within the transfer callback.
 *
 * The set of events received by the callback is customizable. To do so, set the a eventMask parameter to
 * the OR'd combination of #i2c_slave_transfer_event_t enumerators for the events you wish to receive.
 * The #kI2C_SlaveTransmitEvent and #kI2C_SlaveReceiveEvent events are always enabled and do not need
 * to be included in the mask. Alternatively, you can pass 0 to get a default set of only the transmit and
 * receive events that are always enabled. In addition, the #kI2C_SlaveAllEvents constant is provided as
 * a convenient way to enable all events.
 *
 * param base The I2C peripheral base address.
 * param transfer Pointer to #i2c_slave_transfer_t structure.
 * param rxData Pointer to data to store data from master.
 * param rxSize Size of rxData in bytes.
 * param eventMask Bit mask formed by OR'ing together #i2c_slave_transfer_event_t enumerators to specify
 *      which events to send to the callback. Other accepted values are 0 to get a default set of
 *      only the transmit and receive events, and #kI2C_SlaveAllEvents to enable all events.
 *
 * retval kStatus_Success Slave transfers were successfully started.
 * retval #kStatus_I2C_Busy Slave transfers have already been started on this handle.
 */
status_t I2C_SlaveSetReceiveBuffer(
    I2C_Type *base, volatile i2c_slave_transfer_t *transfer, void *rxData, size_t rxSize, uint32_t eventMask)
{
    return I2C_SlaveTransferNonBlockingInternal(base, transfer->handle, NULL, 0u, rxData, rxSize, eventMask);
}

/*!
 * brief Configures Slave Address n register.
 *
 * This function writes new value to Slave Address register.
 *
 * param base The I2C peripheral base address.
 * param addressRegister The module supports multiple address registers. The parameter determines which one shall be
 * changed.
 * param address The slave address to be stored to the address register for matching.
 * param addressDisable Disable matching of the specified address register.
  */
void I2C_SlaveSetAddress(I2C_Type *base,
                         i2c_slave_address_register_t addressRegister,
                         uint8_t address,
                         bool addressDisable)
{
    base->SLVADR[addressRegister] = I2C_SLVADR_SLVADR(address) | I2C_SLVADR_SADISABLE(addressDisable);
}

/*!
 * brief Provides a default configuration for the I2C slave peripheral.
 *
 * This function provides the following default configuration for the I2C slave peripheral:
 * code
 *  slaveConfig->enableSlave = true;
 *  slaveConfig->address0.disable = false;
 *  slaveConfig->address0.address = 0u;
 *  slaveConfig->address1.disable = true;
 *  slaveConfig->address2.disable = true;
 *  slaveConfig->address3.disable = true;
 *  slaveConfig->busSpeed = kI2C_SlaveStandardMode;
 * endcode
 *
 * After calling this function, override any settings  to customize the configuration,
 * prior to initializing the master driver with I2C_SlaveInit(). Be sure to override at least the a
 * address0.address member of the configuration structure with the desired slave address.
 *
 * param[out] slaveConfig User provided configuration structure that is set to default values. Refer to
 *      #i2c_slave_config_t.
 */
void I2C_SlaveGetDefaultConfig(i2c_slave_config_t *slaveConfig)
{
    assert(slaveConfig);

    i2c_slave_config_t mySlaveConfig = {0};

    /* default config enables slave address 0 match to general I2C call address zero */
    mySlaveConfig.enableSlave = true;
    mySlaveConfig.address1.addressDisable = true;
    mySlaveConfig.address2.addressDisable = true;
    mySlaveConfig.address3.addressDisable = true;

    *slaveConfig = mySlaveConfig;
}

/*!
 * brief Initializes the I2C slave peripheral.
 *
 * This function enables the peripheral clock and initializes the I2C slave peripheral as described by the user
 * provided configuration.
 *
 * param base The I2C peripheral base address.
 * param slaveConfig User provided peripheral configuration. Use I2C_SlaveGetDefaultConfig() to get a set of defaults
 *      that you can override.
 * param srcClock_Hz Frequency in Hertz of the I2C functional clock. Used to calculate CLKDIV value to provide
 * enough
 *                       data setup time for master when slave stretches the clock.
 */
status_t I2C_SlaveInit(I2C_Type *base, const i2c_slave_config_t *slaveConfig, uint32_t srcClock_Hz)
{
    status_t status;
    uint32_t divVal = 0;

    /* configure data setup time used when slave stretches clock */
    status = I2C_SlaveDivVal(srcClock_Hz, slaveConfig->busSpeed, &divVal);
    if (kStatus_Success != status)
    {
        return status;
    }

    FLEXCOMM_Init(base, FLEXCOMM_PERIPH_I2C);

    /* I2C Clock Divider register */
    base->CLKDIV = divVal;

    /* set Slave address */
    I2C_SlaveSetAddress(base, kI2C_SlaveAddressRegister0, slaveConfig->address0.address,
                        slaveConfig->address0.addressDisable);
    I2C_SlaveSetAddress(base, kI2C_SlaveAddressRegister1, slaveConfig->address1.address,
                        slaveConfig->address1.addressDisable);
    I2C_SlaveSetAddress(base, kI2C_SlaveAddressRegister2, slaveConfig->address2.address,
                        slaveConfig->address2.addressDisable);
    I2C_SlaveSetAddress(base, kI2C_SlaveAddressRegister3, slaveConfig->address3.address,
                        slaveConfig->address3.addressDisable);

    /* set Slave address 0 qual */
    base->SLVQUAL0 = I2C_SLVQUAL0_QUALMODE0(slaveConfig->qualMode) | I2C_SLVQUAL0_SLVQUAL0(slaveConfig->qualAddress);

    /* set Slave enable */
    base->CFG = I2C_CFG_SLVEN(slaveConfig->enableSlave);

    return status;
}

/*!
* brief Deinitializes the I2C slave peripheral.
*
 * This function disables the I2C slave peripheral and gates the clock. It also performs a software
 * reset to restore the peripheral to reset conditions.
 *
 * param base The I2C peripheral base address.
 */
void I2C_SlaveDeinit(I2C_Type *base)
{
    I2C_SlaveEnable(base, false);
}

/*!
 * brief Performs a polling send transfer on the I2C bus.
 *
 * The function executes blocking address phase and blocking data phase.
 *
 * param base  The I2C peripheral base address.
 * param txBuff The pointer to the data to be transferred.
 * param txSize The length in bytes of the data to be transferred.
 * return kStatus_Success Data has been sent.
 * return kStatus_Fail Unexpected slave state (master data write while master read from slave is expected).
 */
status_t I2C_SlaveWriteBlocking(I2C_Type *base, const uint8_t *txBuff, size_t txSize)
{
    const uint8_t *buf = txBuff;
    uint32_t stat;
    bool slaveAddress;
    bool slaveTransmit;

    /* Set the SLVEN bit to 1 in the CFG register. */
    I2C_SlaveEnable(base, true);

    /* wait for SLVPENDING */
    stat = I2C_SlavePollPending(base);
    if (stat == kStatus_I2C_Timeout)
    {
        return kStatus_I2C_Timeout;
    }

    /* Get slave machine state */
    slaveAddress = (((stat & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_ADDR);
    slaveTransmit = (((stat & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_TX);

    /* in I2C_SlaveSend() it shall be either slaveAddress or slaveTransmit */
    if (!(slaveAddress || slaveTransmit))
    {
        I2C_SlaveInternalStateMachineReset(base);
        return kStatus_Fail;
    }

    if (slaveAddress)
    {
        /* Acknowledge (ack) the address by setting SLVCONTINUE = 1 in the slave control register */
        base->SLVCTL = I2C_SLVCTL_SLVCONTINUE_MASK;

        /* wait for SLVPENDING */
        stat = I2C_SlavePollPending(base);
        if (stat == kStatus_I2C_Timeout)
        {
            return kStatus_I2C_Timeout;
        }
    }

    /* send bytes up to txSize */
    while (txSize)
    {
        slaveTransmit = (((stat & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_TX);

        if (!slaveTransmit)
        {
            I2C_SlaveInternalStateMachineReset(base);
            return kStatus_Fail;
        }

        /* Write 8 bits of data to the SLVDAT register */
        base->SLVDAT = I2C_SLVDAT_DATA(*buf);

        /* continue transaction */
        base->SLVCTL = I2C_SLVCTL_SLVCONTINUE_MASK;

        /* advance counters and pointers for next data */
        buf++;
        txSize--;

        if (txSize)
        {
            /* wait for SLVPENDING */
            stat = I2C_SlavePollPending(base);
            if (stat == kStatus_I2C_Timeout)
            {
                return kStatus_I2C_Timeout;
            }
        }
    }

    return kStatus_Success;
}

/*!
 * brief Performs a polling receive transfer on the I2C bus.
 *
 * The function executes blocking address phase and blocking data phase.
 *
 * param base  The I2C peripheral base address.
 * param rxBuff The pointer to the data to be transferred.
 * param rxSize The length in bytes of the data to be transferred.
 * return kStatus_Success Data has been received.
 * return kStatus_Fail Unexpected slave state (master data read while master write to slave is expected).
 */
status_t I2C_SlaveReadBlocking(I2C_Type *base, uint8_t *rxBuff, size_t rxSize)
{
    uint8_t *buf = rxBuff;
    uint32_t stat;
    bool slaveAddress;
    bool slaveReceive;

    /* Set the SLVEN bit to 1 in the CFG register. */
    I2C_SlaveEnable(base, true);

    /* wait for SLVPENDING */
    stat = I2C_SlavePollPending(base);
    if (stat == kStatus_I2C_Timeout)
    {
        return kStatus_I2C_Timeout;
    }

    /* Get slave machine state */
    slaveAddress = (((stat & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_ADDR);
    slaveReceive = (((stat & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_RX);

    /* in I2C_SlaveReceive() it shall be either slaveAddress or slaveReceive */
    if (!(slaveAddress || slaveReceive))
    {
        I2C_SlaveInternalStateMachineReset(base);
        return kStatus_Fail;
    }

    if (slaveAddress)
    {
        /* Acknowledge (ack) the address by setting SLVCONTINUE = 1 in the slave control register */
        base->SLVCTL = I2C_SLVCTL_SLVCONTINUE_MASK;

        /* wait for SLVPENDING */
        stat = I2C_SlavePollPending(base);
        if (stat == kStatus_I2C_Timeout)
        {
            return kStatus_I2C_Timeout;
        }
    }

    /* receive bytes up to rxSize */
    while (rxSize)
    {
        slaveReceive = (((stat & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_RX);

        if (!slaveReceive)
        {
            I2C_SlaveInternalStateMachineReset(base);
            return kStatus_Fail;
        }

        /* Read 8 bits of data from the SLVDAT register */
        *buf = (uint8_t)base->SLVDAT;

        /* continue transaction */
        base->SLVCTL = I2C_SLVCTL_SLVCONTINUE_MASK;

        /* advance counters and pointers for next data */
        buf++;
        rxSize--;

        if (rxSize)
        {
            /* wait for SLVPENDING */
            stat = I2C_SlavePollPending(base);
            if (stat == kStatus_I2C_Timeout)
            {
                return kStatus_I2C_Timeout;
            }
        }
    }

    return kStatus_Success;
}

/*!
 * brief Creates a new handle for the I2C slave non-blocking APIs.
 *
 * The creation of a handle is for use with the non-blocking APIs. Once a handle
 * is created, there is not a corresponding destroy handle. If the user wants to
 * terminate a transfer, the I2C_SlaveTransferAbort() API shall be called.
 *
 * param base The I2C peripheral base address.
 * param[out] handle Pointer to the I2C slave driver handle.
 * param callback User provided pointer to the asynchronous callback function.
 * param userData User provided pointer to the application callback data.
 */
void I2C_SlaveTransferCreateHandle(I2C_Type *base,
                                   i2c_slave_handle_t *handle,
                                   i2c_slave_transfer_callback_t callback,
                                   void *userData)
{
    uint32_t instance;

    assert(handle);

    /* Clear out the handle. */
    memset(handle, 0, sizeof(*handle));

    /* Look up instance number */
    instance = I2C_GetInstance(base);

    /* Save base and instance. */
    handle->callback = callback;
    handle->userData = userData;

    /* initialize fsm */
    handle->slaveFsm = kI2C_SlaveFsmAddressMatch;

    /* store pointer to handle into transfer struct */
    handle->transfer.handle = handle;

    FLEXCOMM_SetIRQHandler(base, (flexcomm_irq_handler_t)I2C_SlaveTransferHandleIRQ, handle);

    /* Clear internal IRQ enables and enable NVIC IRQ. */
    I2C_DisableInterrupts(base, kI2C_SlaveIrqFlags);
    EnableIRQ(s_i2cIRQ[instance]);
}

/*!
 * brief Starts accepting slave transfers.
 *
 * Call this API after calling I2C_SlaveInit() and I2C_SlaveTransferCreateHandle() to start processing
 * transactions driven by an I2C master. The slave monitors the I2C bus and pass events to the
 * callback that was passed into the call to I2C_SlaveTransferCreateHandle(). The callback is always invoked
 * from the interrupt context.
 *
 * If no slave Tx transfer is busy, a master read from slave request invokes #kI2C_SlaveTransmitEvent callback.
 * If no slave Rx transfer is busy, a master write to slave request invokes #kI2C_SlaveReceiveEvent callback.
 *
 * The set of events received by the callback is customizable. To do so, set the a eventMask parameter to
 * the OR'd combination of #i2c_slave_transfer_event_t enumerators for the events you wish to receive.
 * The #kI2C_SlaveTransmitEvent and #kI2C_SlaveReceiveEvent events are always enabled and do not need
 * to be included in the mask. Alternatively, you can pass 0 to get a default set of only the transmit and
 * receive events that are always enabled. In addition, the #kI2C_SlaveAllEvents constant is provided as
 * a convenient way to enable all events.
 *
 * param base The I2C peripheral base address.
 * param handle Pointer to i2c_slave_handle_t structure which stores the transfer state.
 * param eventMask Bit mask formed by OR'ing together #i2c_slave_transfer_event_t enumerators to specify
 *      which events to send to the callback. Other accepted values are 0 to get a default set of
 *      only the transmit and receive events, and #kI2C_SlaveAllEvents to enable all events.
 *
 * retval kStatus_Success Slave transfers were successfully started.
 * retval #kStatus_I2C_Busy Slave transfers have already been started on this handle.
 */
status_t I2C_SlaveTransferNonBlocking(I2C_Type *base, i2c_slave_handle_t *handle, uint32_t eventMask)
{
    return I2C_SlaveTransferNonBlockingInternal(base, handle, NULL, 0u, NULL, 0u, eventMask);
}

/*!
 * brief Gets the slave transfer remaining bytes during a interrupt non-blocking transfer.
 *
 * param base I2C base pointer.
 * param handle pointer to i2c_slave_handle_t structure.
 * param count Number of bytes transferred so far by the non-blocking transaction.
 * retval kStatus_InvalidArgument count is Invalid.
 * retval kStatus_Success Successfully return the count.
 */
status_t I2C_SlaveTransferGetCount(I2C_Type *base, i2c_slave_handle_t *handle, size_t *count)
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

    /* For an active transfer, just return the count from the handle. */
    *count = handle->transfer.transferredCount;

    return kStatus_Success;
}

/*!
 * brief Aborts the slave non-blocking transfers.
 * note This API could be called at any time to stop slave for handling the bus events.
 * param base The I2C peripheral base address.
 * param handle Pointer to i2c_slave_handle_t structure which stores the transfer state.
 * retval kStatus_Success
 * retval #kStatus_I2C_Idle
 */
void I2C_SlaveTransferAbort(I2C_Type *base, i2c_slave_handle_t *handle)
{
    /* Disable I2C IRQ sources while we configure stuff. */
    I2C_DisableInterrupts(base, kI2C_SlaveIrqFlags);

    /* Set the SLVEN bit to 0 in the CFG register. */
    I2C_SlaveEnable(base, false);

    handle->isBusy = false;
    handle->transfer.txSize = 0;
    handle->transfer.rxSize = 0;
}

/*!
 * brief Reusable routine to handle slave interrupts.
 * note This function does not need to be called unless you are reimplementing the
 *  non blocking API's interrupt handler routines to add special functionality.
 * param base The I2C peripheral base address.
 * param handle Pointer to i2c_slave_handle_t structure which stores the transfer state.
 */
void I2C_SlaveTransferHandleIRQ(I2C_Type *base, i2c_slave_handle_t *handle)
{
    uint32_t i2cStatus = base->STAT;

    if (i2cStatus & I2C_STAT_SLVDESEL_MASK)
    {
        I2C_SlaveInvokeEvent(base, handle, kI2C_SlaveDeselectedEvent);
        I2C_SlaveClearStatusFlags(base, I2C_STAT_SLVDESEL_MASK);
    }

    /* SLVPENDING flag is cleared by writing I2C_SLVCTL_SLVCONTINUE_MASK to SLVCTL register */
    if (i2cStatus & I2C_STAT_SLVPENDING_MASK)
    {
        bool slaveAddress = (((i2cStatus & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_ADDR);

        if (slaveAddress)
        {
            I2C_SlaveAddressIRQ(base, handle);
            I2C_SlaveInvokeEvent(base, handle, kI2C_SlaveAddressMatchEvent);
        }
        else
        {
            switch (handle->slaveFsm)
            {
                case kI2C_SlaveFsmReceive:
                {
                    bool slaveReceive =
                        (((i2cStatus & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_RX);

                    if (slaveReceive)
                    {
                        /* if we have no receive buffer in this transfer, call callback to get new */
                        if ((handle->transfer.rxData == NULL) || (handle->transfer.rxSize == 0))
                        {
                            I2C_SlaveInvokeEvent(base, handle, kI2C_SlaveReceiveEvent);
                        }

                        /* receive a byte */
                        if ((handle->transfer.rxData) && (handle->transfer.rxSize))
                        {
                            /* continue transaction */
                            base->SLVCTL = I2C_SLVCTL_SLVCONTINUE_MASK;
                            *(handle->transfer.rxData) = (uint8_t)base->SLVDAT;
                            (handle->transfer.rxSize)--;
                            (handle->transfer.rxData)++;
                            (handle->transfer.transferredCount)++;
                        }

                        /* is this last transaction for this transfer? allow next transaction */
                        if ((0 == handle->transfer.rxSize) && (0 == handle->transfer.txSize))
                        {
                            handle->isBusy = false;
                            I2C_SlaveInvokeEvent(base, handle, kI2C_SlaveCompletionEvent);
                        }
                    }
                    else
                    {
                        base->SLVCTL = I2C_SLVCTL_SLVNACK_MASK;
                    }
                }
                break;

                case kI2C_SlaveFsmTransmit:
                {
                    bool slaveTransmit =
                        (((i2cStatus & I2C_STAT_SLVSTATE_MASK) >> I2C_STAT_SLVSTATE_SHIFT) == I2C_STAT_SLVST_TX);

                    if (slaveTransmit)
                    {
                        /* if we have no data in this transfer, call callback to get new */
                        if ((handle->transfer.txData == NULL) || (handle->transfer.txSize == 0))
                        {
                            I2C_SlaveInvokeEvent(base, handle, kI2C_SlaveTransmitEvent);
                        }

                        /* transmit a byte */
                        if ((handle->transfer.txData) && (handle->transfer.txSize))
                        {
                            base->SLVDAT = *(handle->transfer.txData);
                            /* continue transaction */
                            base->SLVCTL = I2C_SLVCTL_SLVCONTINUE_MASK;
                            (handle->transfer.txSize)--;
                            (handle->transfer.txData)++;
                            (handle->transfer.transferredCount)++;
                        }

                        /* is this last transaction for this transfer? allow next transaction */
                        if ((0 == handle->transfer.rxSize) && (0 == handle->transfer.txSize))
                        {
                            handle->isBusy = false;
                            I2C_SlaveInvokeEvent(base, handle, kI2C_SlaveCompletionEvent);
                        }
                    }
                    else
                    {
                        base->SLVCTL = I2C_SLVCTL_SLVNACK_MASK;
                    }
                }
                break;

                default:
                    /* incorrect state, slv_abort()? */
                    break;
            }
        }
    }
}
