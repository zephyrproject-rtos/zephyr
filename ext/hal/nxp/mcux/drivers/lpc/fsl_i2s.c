/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_i2s.h"
#include "fsl_flexcomm.h"
#include <string.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flexcomm_i2s"
#endif

/* TODO - absent in device header files, should be there */
#define I2S_FIFOCFG_TXI2SE0_MASK (0x4U)
#define I2S_FIFOCFG_TXI2SE0_SHIFT (2U)
#define I2S_FIFOCFG_TXI2SE0(x) (((uint32_t)(((uint32_t)(x)) << I2S_FIFOCFG_TXI2SE0_SHIFT)) & I2S_FIFOCFG_TXI2SE0_MASK)
#define I2S_FIFOCFG_PACK48_MASK (0x8U)
#define I2S_FIFOCFG_PACK48_SHIFT (3U)
#define I2S_FIFOCFG_PACK48(x) (((uint32_t)(((uint32_t)(x)) << I2S_FIFOCFG_PACK48_SHIFT)) & I2S_FIFOCFG_PACK48_MASK)

/*! @brief I2S states. */
enum _i2s_state
{
    kI2S_StateIdle = 0x0,             /*!< Not performing transfer */
    kI2S_StateTx,                     /*!< Performing transmit */
    kI2S_StateTxWaitToWriteDummyData, /*!< Wait on FIFO in order to write final dummy data there */
    kI2S_StateTxWaitForEmptyFifo,     /*!< Wait for FIFO to be flushed */
    kI2S_StateRx,                     /*!< Performing receive */
};

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void I2S_Config(I2S_Type *base, const i2s_config_t *config);
static void I2S_TxEnable(I2S_Type *base, bool enable);
static void I2S_RxEnable(I2S_Type *base, bool enable);
static status_t I2S_ValidateBuffer(i2s_handle_t *handle, i2s_transfer_t *transfer);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*! @brief Array to map i2c instance number to base address. */
static const uint32_t s_i2sBaseAddrs[FSL_FEATURE_SOC_I2S_COUNT] = I2S_BASE_ADDRS;

/*! @brief IRQ name array */
static const IRQn_Type s_i2sIRQ[] = I2S_IRQS;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief Returns an instance number given a base address.
 *
 * If an invalid base address is passed, debug builds will assert. Release builds will just return
 * instance number 0.
 *
 * param base The I2C peripheral base address.
 * return I2C instance number starting from 0.
 */
uint32_t I2S_GetInstance(I2S_Type *base)
{
    int i;
    for (i = 0; i < FSL_FEATURE_SOC_I2S_COUNT; i++)
    {
        if ((uint32_t)base == s_i2sBaseAddrs[i])
        {
            return i;
        }
    }
    assert(false);
    return 0;
}
/*!
 * brief Initializes the FLEXCOMM peripheral for I2S transmit functionality.
 *
 * Ungates the FLEXCOMM clock and configures the module
 * for I2S transmission using a configuration structure.
 * The configuration structure can be custom filled or set with default values by
 * I2S_TxGetDefaultConfig().
 *
 * note This API should be called at the beginning of the application to use
 * the I2S driver.
 *
 * param base I2S base pointer.
 * param config pointer to I2S configuration structure.
 */
void I2S_TxInit(I2S_Type *base, const i2s_config_t *config)
{
    uint32_t cfg = 0U;
    uint32_t trig = 0U;

    FLEXCOMM_Init(base, FLEXCOMM_PERIPH_I2S_TX);
    I2S_Config(base, config);

    /* Configure FIFO */

    cfg |= I2S_FIFOCFG_ENABLETX(1U);                 /* enable TX FIFO */
    cfg |= I2S_FIFOCFG_EMPTYTX(1U);                  /* empty TX FIFO */
    cfg |= I2S_FIFOCFG_TXI2SE0(config->txEmptyZero); /* transmit zero when buffer becomes empty or last item */
    cfg |= I2S_FIFOCFG_PACK48(config->pack48);       /* set pack 48-bit format or not */
    trig |= I2S_FIFOTRIG_TXLVLENA(1U);               /* enable TX FIFO trigger */
    trig |= I2S_FIFOTRIG_TXLVL(config->watermark);   /* set TX FIFO trigger level */

    base->FIFOCFG = cfg;
    base->FIFOTRIG = trig;
}

/*!
 * brief Initializes the FLEXCOMM peripheral for I2S receive functionality.
 *
 * Ungates the FLEXCOMM clock and configures the module
 * for I2S receive using a configuration structure.
 * The configuration structure can be custom filled or set with default values by
 * I2S_RxGetDefaultConfig().
 *
 * note This API should be called at the beginning of the application to use
 * the I2S driver.
 *
 * param base I2S base pointer.
 * param config pointer to I2S configuration structure.
 */
void I2S_RxInit(I2S_Type *base, const i2s_config_t *config)
{
    uint32_t cfg = 0U;
    uint32_t trig = 0U;

    FLEXCOMM_Init(base, FLEXCOMM_PERIPH_I2S_RX);
    I2S_Config(base, config);

    /* Configure FIFO */

    cfg |= I2S_FIFOCFG_ENABLERX(1U);               /* enable RX FIFO */
    cfg |= I2S_FIFOCFG_EMPTYRX(1U);                /* empty RX FIFO */
    cfg |= I2S_FIFOCFG_PACK48(config->pack48);     /* set pack 48-bit format or not */
    trig |= I2S_FIFOTRIG_RXLVLENA(1U);             /* enable RX FIFO trigger */
    trig |= I2S_FIFOTRIG_RXLVL(config->watermark); /* set RX FIFO trigger level */

    base->FIFOCFG = cfg;
    base->FIFOTRIG = trig;
}

/*!
 * brief Sets the I2S Tx configuration structure to default values.
 *
 * This API initializes the configuration structure for use in I2S_TxInit().
 * The initialized structure can remain unchanged in I2S_TxInit(), or it can be modified
 * before calling I2S_TxInit().
 * Example:
   code
   i2s_config_t config;
   I2S_TxGetDefaultConfig(&config);
   endcode
 *
 * Default values:
 * code
 *   config->masterSlave = kI2S_MasterSlaveNormalMaster;
 *   config->mode = kI2S_ModeI2sClassic;
 *   config->rightLow = false;
 *   config->leftJust = false;
 *   config->pdmData = false;
 *   config->sckPol = false;
 *   config->wsPol = false;
 *   config->divider = 1;
 *   config->oneChannel = false;
 *   config->dataLength = 16;
 *   config->frameLength = 32;
 *   config->position = 0;
 *   config->watermark = 4;
 *   config->txEmptyZero = true;
 *   config->pack48 = false;
 * endcode
 *
 * param config pointer to I2S configuration structure.
 */
void I2S_TxGetDefaultConfig(i2s_config_t *config)
{
    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->masterSlave = kI2S_MasterSlaveNormalMaster;
    config->mode = kI2S_ModeI2sClassic;
    config->rightLow = false;
    config->leftJust = false;
#if defined(I2S_CFG1_PDMDATA)
    config->pdmData = false;
#endif
    config->sckPol = false;
    config->wsPol = false;
    config->divider = 1U;
    config->oneChannel = false;
    config->dataLength = 16U;
    config->frameLength = 32U;
    config->position = 0U;
    config->watermark = 4U;
    config->txEmptyZero = true;
    config->pack48 = false;
}

/*!
 * brief Sets the I2S Rx configuration structure to default values.
 *
 * This API initializes the configuration structure for use in I2S_RxInit().
 * The initialized structure can remain unchanged in I2S_RxInit(), or it can be modified
 * before calling I2S_RxInit().
 * Example:
   code
   i2s_config_t config;
   I2S_RxGetDefaultConfig(&config);
   endcode
 *
 * Default values:
 * code
 *   config->masterSlave = kI2S_MasterSlaveNormalSlave;
 *   config->mode = kI2S_ModeI2sClassic;
 *   config->rightLow = false;
 *   config->leftJust = false;
 *   config->pdmData = false;
 *   config->sckPol = false;
 *   config->wsPol = false;
 *   config->divider = 1;
 *   config->oneChannel = false;
 *   config->dataLength = 16;
 *   config->frameLength = 32;
 *   config->position = 0;
 *   config->watermark = 4;
 *   config->txEmptyZero = false;
 *   config->pack48 = false;
 * endcode
 *
 * param config pointer to I2S configuration structure.
 */
void I2S_RxGetDefaultConfig(i2s_config_t *config)
{
    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->masterSlave = kI2S_MasterSlaveNormalSlave;
    config->mode = kI2S_ModeI2sClassic;
    config->rightLow = false;
    config->leftJust = false;
#if defined(I2S_CFG1_PDMDATA)
    config->pdmData = false;
#endif
    config->sckPol = false;
    config->wsPol = false;
    config->divider = 1U;
    config->oneChannel = false;
    config->dataLength = 16U;
    config->frameLength = 32U;
    config->position = 0U;
    config->watermark = 4U;
    config->txEmptyZero = false;
    config->pack48 = false;
}

static void I2S_Config(I2S_Type *base, const i2s_config_t *config)
{
    assert(config);

    uint32_t cfg1 = 0U;
    uint32_t cfg2 = 0U;

    /* set master/slave configuration */
    cfg1 |= I2S_CFG1_MSTSLVCFG(config->masterSlave);

    /* set I2S mode */
    cfg1 |= I2S_CFG1_MODE(config->mode);

    /* set right low (channel swap) */
    cfg1 |= I2S_CFG1_RIGHTLOW(config->rightLow);

    /* set data justification */
    cfg1 |= I2S_CFG1_LEFTJUST(config->leftJust);

#if defined(I2S_CFG1_PDMDATA)
    /* set source to PDM dmic */
    cfg1 |= I2S_CFG1_PDMDATA(config->pdmData);
#endif

    /* set SCLK polarity */
    cfg1 |= I2S_CFG1_SCK_POL(config->sckPol);

    /* set WS polarity */
    cfg1 |= I2S_CFG1_WS_POL(config->wsPol);

    /* set mono mode */
    cfg1 |= I2S_CFG1_ONECHANNEL(config->oneChannel);

    /* set data length */
    cfg1 |= I2S_CFG1_DATALEN(config->dataLength - 1U);

    /* set frame length */
    cfg2 |= I2S_CFG2_FRAMELEN(config->frameLength - 1U);

    /* set data position of this channel pair within the frame */
    cfg2 |= I2S_CFG2_POSITION(config->position);

    /* write to registers */
    base->CFG1 = cfg1;
    base->CFG2 = cfg2;

    /* set the clock divider */
    base->DIV = I2S_DIV_DIV(config->divider - 1U);
}

/*!
 * brief De-initializes the I2S peripheral.
 *
 * This API gates the FLEXCOMM clock. The I2S module can't operate unless I2S_TxInit
 * or I2S_RxInit is called to enable the clock.
 *
 * param base I2S base pointer.
 */
void I2S_Deinit(I2S_Type *base)
{
    /* TODO gate FLEXCOMM clock via FLEXCOMM driver */
}

static void I2S_TxEnable(I2S_Type *base, bool enable)
{
    if (enable)
    {
        I2S_EnableInterrupts(base, kI2S_TxErrorFlag | kI2S_TxLevelFlag);
        I2S_Enable(base);
    }
    else
    {
        I2S_DisableInterrupts(base, kI2S_TxErrorFlag | kI2S_TxLevelFlag);
        I2S_Disable(base);
        base->FIFOCFG |= I2S_FIFOCFG_EMPTYTX_MASK;
    }
}

static void I2S_RxEnable(I2S_Type *base, bool enable)
{
    if (enable)
    {
        I2S_EnableInterrupts(base, kI2S_RxErrorFlag | kI2S_RxLevelFlag);
        I2S_Enable(base);
    }
    else
    {
        I2S_DisableInterrupts(base, kI2S_RxErrorFlag | kI2S_RxLevelFlag);
        I2S_Disable(base);
        base->FIFOCFG |= I2S_FIFOCFG_EMPTYRX_MASK;
    }
}

static status_t I2S_ValidateBuffer(i2s_handle_t *handle, i2s_transfer_t *transfer)
{
    assert(transfer->data);
    if (!transfer->data)
    {
        return kStatus_InvalidArgument;
    }

    assert(transfer->dataSize > 0U);
    if (transfer->dataSize <= 0U)
    {
        return kStatus_InvalidArgument;
    }

    if (handle->dataLength == 4U)
    {
        /* No alignment and data length requirements */
    }
    else if ((handle->dataLength >= 5U) && (handle->dataLength <= 8U))
    {
        assert((((uint32_t)transfer->data) % 2U) == 0U);
        if ((((uint32_t)transfer->data) % 2U) != 0U)
        {
            /* Data not 2-bytes aligned */
            return kStatus_InvalidArgument;
        }

        assert((transfer->dataSize % 2U) == 0U);
        if ((transfer->dataSize % 2U) != 0U)
        {
            /* Data not in pairs of left/right channel bytes */
            return kStatus_InvalidArgument;
        }
    }
    else if ((handle->dataLength >= 9U) && (handle->dataLength <= 16U))
    {
        assert((((uint32_t)transfer->data) % 4U) == 0U);
        if ((((uint32_t)transfer->data) % 4U) != 0U)
        {
            /* Data not 4-bytes aligned */
            return kStatus_InvalidArgument;
        }

        assert((transfer->dataSize % 4U) == 0U);
        if ((transfer->dataSize % 4U) != 0U)
        {
            /* Data lenght not multiply of 4 */
            return kStatus_InvalidArgument;
        }
    }
    else if ((handle->dataLength >= 17U) && (handle->dataLength <= 24U))
    {
        assert((transfer->dataSize % 6U) == 0U);
        if ((transfer->dataSize % 6U) != 0U)
        {
            /* Data lenght not multiply of 6 */
            return kStatus_InvalidArgument;
        }

        assert(!((handle->pack48) && ((((uint32_t)transfer->data) % 4U) != 0U)));
        if ((handle->pack48) && ((((uint32_t)transfer->data) % 4U) != 0U))
        {
            /* Data not 4-bytes aligned */
            return kStatus_InvalidArgument;
        }
    }
    else /* if (handle->dataLength >= 25U) */
    {
        assert((((uint32_t)transfer->data) % 4U) == 0U);
        if ((((uint32_t)transfer->data) % 4U) != 0U)
        {
            /* Data not 4-bytes aligned */
            return kStatus_InvalidArgument;
        }

        if (handle->oneChannel)
        {
            assert((transfer->dataSize % 4U) == 0U);
            if ((transfer->dataSize % 4U) != 0U)
            {
                /* Data lenght not multiply of 4 */
                return kStatus_InvalidArgument;
            }
        }
        else
        {
            assert((transfer->dataSize % 8U) == 0U);
            if ((transfer->dataSize % 8U) != 0U)
            {
                /* Data lenght not multiply of 8 */
                return kStatus_InvalidArgument;
            }
        }
    }

    return kStatus_Success;
}

/*!
 * brief Initializes handle for transfer of audio data.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 * param callback function to be called back when transfer is done or fails.
 * param userData pointer to data passed to callback.
 */
void I2S_TxTransferCreateHandle(I2S_Type *base, i2s_handle_t *handle, i2s_transfer_callback_t callback, void *userData)
{
    uint32_t instance;

    assert(handle);

    /* Clear out the handle */
    memset(handle, 0U, sizeof(*handle));

    /* Look up instance number */
    instance = I2S_GetInstance(base);

    /* Save callback and user data */
    handle->completionCallback = callback;
    handle->userData = userData;

    /* Remember some items set previously by configuration */
    handle->watermark = ((base->FIFOTRIG & I2S_FIFOTRIG_TXLVL_MASK) >> I2S_FIFOTRIG_TXLVL_SHIFT);
    handle->oneChannel = ((base->CFG1 & I2S_CFG1_ONECHANNEL_MASK) >> I2S_CFG1_ONECHANNEL_SHIFT);
    handle->dataLength = ((base->CFG1 & I2S_CFG1_DATALEN_MASK) >> I2S_CFG1_DATALEN_SHIFT) + 1U;
    handle->pack48 = ((base->FIFOCFG & I2S_FIFOCFG_PACK48_MASK) >> I2S_FIFOCFG_PACK48_SHIFT);

    handle->useFifo48H = false;

    /* Register IRQ handling */
    FLEXCOMM_SetIRQHandler(base, (flexcomm_irq_handler_t)I2S_TxHandleIRQ, handle);

    /* enable NVIC IRQ. */
    EnableIRQ(s_i2sIRQ[instance]);
}

/*!
 * brief Begins or queue sending of the given data.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 * param transfer data buffer.
 *
 * retval kStatus_Success
 * retval kStatus_I2S_Busy if all queue slots are occupied with unsent buffers.
 */
status_t I2S_TxTransferNonBlocking(I2S_Type *base, i2s_handle_t *handle, i2s_transfer_t transfer)
{
    assert(handle);
    if (!handle)
    {
        return kStatus_InvalidArgument;
    }

    status_t result;

    result = I2S_ValidateBuffer(handle, &transfer);
    if (result != kStatus_Success)
    {
        return result;
    }

    if (handle->i2sQueue[handle->queueUser].dataSize)
    {
        /* Previously prepared buffers not processed yet */
        return kStatus_I2S_Busy;
    }

    handle->state = kI2S_StateTx;
    handle->i2sQueue[handle->queueUser].data = transfer.data;
    handle->i2sQueue[handle->queueUser].dataSize = transfer.dataSize;
    handle->queueUser = (handle->queueUser + 1U) % I2S_NUM_BUFFERS;

    base->FIFOTRIG = (base->FIFOTRIG & (~I2S_FIFOTRIG_TXLVL_MASK)) | I2S_FIFOTRIG_TXLVL(handle->watermark);
    I2S_TxEnable(base, true);

    return kStatus_Success;
}

/*!
 * brief Aborts sending of data.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 */
void I2S_TxTransferAbort(I2S_Type *base, i2s_handle_t *handle)
{
    assert(handle);

    /* Disable I2S operation and interrupts */
    I2S_TxEnable(base, false);

    /* Reset state */
    handle->state = kI2S_StateIdle;

    /* Clear transfer queue */
    memset((void *)&handle->i2sQueue, 0U, sizeof(i2s_transfer_t) * I2S_NUM_BUFFERS);
    handle->queueDriver = 0U;
    handle->queueUser = 0U;
}

/*!
 * brief Initializes handle for reception of audio data.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 * param callback function to be called back when transfer is done or fails.
 * param userData pointer to data passed to callback.
 */
void I2S_RxTransferCreateHandle(I2S_Type *base, i2s_handle_t *handle, i2s_transfer_callback_t callback, void *userData)
{
    uint32_t instance;

    assert(handle);

    /* Clear out the handle */
    memset(handle, 0U, sizeof(*handle));

    /* Look up instance number */
    instance = I2S_GetInstance(base);

    /* Save callback and user data */
    handle->completionCallback = callback;
    handle->userData = userData;

    /* Remember some items set previously by configuration */
    handle->watermark = ((base->FIFOTRIG & I2S_FIFOTRIG_RXLVL_MASK) >> I2S_FIFOTRIG_RXLVL_SHIFT);
    handle->oneChannel = ((base->CFG1 & I2S_CFG1_ONECHANNEL_MASK) >> I2S_CFG1_ONECHANNEL_SHIFT);
    handle->dataLength = ((base->CFG1 & I2S_CFG1_DATALEN_MASK) >> I2S_CFG1_DATALEN_SHIFT) + 1U;
    handle->pack48 = ((base->FIFOCFG & I2S_FIFOCFG_PACK48_MASK) >> I2S_FIFOCFG_PACK48_SHIFT);

    handle->useFifo48H = false;

    /* Register IRQ handling */
    FLEXCOMM_SetIRQHandler(base, (flexcomm_irq_handler_t)I2S_RxHandleIRQ, handle);

    /* enable NVIC IRQ. */
    EnableIRQ(s_i2sIRQ[instance]);
}

/*!
 * brief Begins or queue reception of data into given buffer.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 * param transfer data buffer.
 *
 * retval kStatus_Success
 * retval kStatus_I2S_Busy if all queue slots are occupied with buffers which are not full.
 */
status_t I2S_RxTransferNonBlocking(I2S_Type *base, i2s_handle_t *handle, i2s_transfer_t transfer)
{
    assert(handle);
    if (!handle)
    {
        return kStatus_InvalidArgument;
    }

    status_t result;

    result = I2S_ValidateBuffer(handle, &transfer);
    if (result != kStatus_Success)
    {
        return result;
    }

    if (handle->i2sQueue[handle->queueUser].dataSize)
    {
        /* Previously prepared buffers not processed yet */
        return kStatus_I2S_Busy;
    }

    handle->state = kI2S_StateRx;
    handle->i2sQueue[handle->queueUser].data = transfer.data;
    handle->i2sQueue[handle->queueUser].dataSize = transfer.dataSize;
    handle->queueUser = (handle->queueUser + 1U) % I2S_NUM_BUFFERS;

    base->FIFOTRIG = (base->FIFOTRIG & (~I2S_FIFOTRIG_RXLVL_MASK)) | I2S_FIFOTRIG_RXLVL(handle->watermark);
    I2S_RxEnable(base, true);

    return kStatus_Success;
}

/*!
 * brief Aborts receiving of data.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 */
void I2S_RxTransferAbort(I2S_Type *base, i2s_handle_t *handle)
{
    assert(handle);

    /* Disable I2S operation and interrupts */
    I2S_RxEnable(base, false);

    /* Reset state */
    handle->state = kI2S_StateIdle;

    /* Clear transfer queue */
    memset((void *)&handle->i2sQueue, 0U, sizeof(i2s_transfer_t) * I2S_NUM_BUFFERS);
    handle->queueDriver = 0U;
    handle->queueUser = 0U;
}

/*!
 * brief Returns number of bytes transferred so far.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 * param[out] count number of bytes transferred so far by the non-blocking transaction.
 *
 * retval kStatus_Success
 * retval kStatus_NoTransferInProgress there is no non-blocking transaction currently in progress.
 */
status_t I2S_TransferGetCount(I2S_Type *base, i2s_handle_t *handle, size_t *count)
{
    assert(handle);
    if (!handle)
    {
        return kStatus_InvalidArgument;
    }

    assert(count);
    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    if (handle->state == kI2S_StateIdle)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->transferCount;

    return kStatus_Success;
}

/*!
 * brief Returns number of buffer underruns or overruns.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 * param[out] count number of transmit errors encountered so far by the non-blocking transaction.
 *
 * retval kStatus_Success
 * retval kStatus_NoTransferInProgress there is no non-blocking transaction currently in progress.
 */
status_t I2S_TransferGetErrorCount(I2S_Type *base, i2s_handle_t *handle, size_t *count)
{
    assert(handle);
    if (!handle)
    {
        return kStatus_InvalidArgument;
    }

    assert(count);
    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    if (handle->state == kI2S_StateIdle)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->errorCount;

    return kStatus_Success;
}

/*!
 * brief Invoked from interrupt handler when transmit FIFO level decreases.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 */
void I2S_TxHandleIRQ(I2S_Type *base, i2s_handle_t *handle)
{
    uint32_t intstat = base->FIFOINTSTAT;
    uint32_t data;

    if (intstat & I2S_FIFOINTSTAT_TXERR_MASK)
    {
        handle->errorCount++;

        /* Clear TX error interrupt flag */
        base->FIFOSTAT = I2S_FIFOSTAT_TXERR(1U);
    }

    if (intstat & I2S_FIFOINTSTAT_TXLVL_MASK)
    {
        if (handle->state == kI2S_StateTx)
        {
            /* Send data */

            while ((base->FIFOSTAT & I2S_FIFOSTAT_TXNOTFULL_MASK) &&
                   (handle->i2sQueue[handle->queueDriver].dataSize > 0U))
            {
                /* Write output data */
                if (handle->dataLength == 4U)
                {
                    data = *(handle->i2sQueue[handle->queueDriver].data);
                    base->FIFOWR = ((data & 0xF0U) << 12U) | (data & 0xFU);
                    handle->i2sQueue[handle->queueDriver].data++;
                    handle->transferCount++;
                    handle->i2sQueue[handle->queueDriver].dataSize--;
                }
                else if (handle->dataLength <= 8U)
                {
                    data = *((volatile uint16_t *)handle->i2sQueue[handle->queueDriver].data);
                    base->FIFOWR = ((data & 0xFF00U) << 8U) | (data & 0xFFU);
                    handle->i2sQueue[handle->queueDriver].data += sizeof(uint16_t);
                    handle->transferCount += sizeof(uint16_t);
                    handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint16_t);
                }
                else if (handle->dataLength <= 16U)
                {
                    base->FIFOWR = *((volatile uint32_t *)(handle->i2sQueue[handle->queueDriver].data));
                    handle->i2sQueue[handle->queueDriver].data += sizeof(uint32_t);
                    handle->transferCount += sizeof(uint32_t);
                    handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint32_t);
                }
                else if (handle->dataLength <= 24U)
                {
                    if (handle->pack48)
                    {
                        if (handle->useFifo48H)
                        {
                            base->FIFOWR48H = *((volatile uint16_t *)(handle->i2sQueue[handle->queueDriver].data));
                            handle->i2sQueue[handle->queueDriver].data += sizeof(uint16_t);
                            handle->transferCount += sizeof(uint16_t);
                            handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint16_t);
                            handle->useFifo48H = false;
                        }
                        else
                        {
                            base->FIFOWR = *((volatile uint32_t *)(handle->i2sQueue[handle->queueDriver].data));
                            handle->i2sQueue[handle->queueDriver].data += sizeof(uint32_t);
                            handle->transferCount += sizeof(uint32_t);
                            handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint32_t);
                            handle->useFifo48H = true;
                        }
                    }
                    else
                    {
                        data = (uint32_t)(*(handle->i2sQueue[handle->queueDriver].data++));
                        data |= ((uint32_t)(*(handle->i2sQueue[handle->queueDriver].data++))) << 8U;
                        data |= ((uint32_t)(*(handle->i2sQueue[handle->queueDriver].data++))) << 16U;
                        if (handle->useFifo48H)
                        {
                            base->FIFOWR48H = data;
                            handle->useFifo48H = false;
                        }
                        else
                        {
                            base->FIFOWR = data;
                            handle->useFifo48H = true;
                        }
                        handle->transferCount += 3U;
                        handle->i2sQueue[handle->queueDriver].dataSize -= 3U;
                    }
                }
                else /* if (handle->dataLength <= 32U) */
                {
                    base->FIFOWR = *((volatile uint32_t *)(handle->i2sQueue[handle->queueDriver].data));
                    handle->i2sQueue[handle->queueDriver].data += sizeof(uint32_t);
                    handle->transferCount += sizeof(uint32_t);
                    handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint32_t);
                }

                if (handle->i2sQueue[handle->queueDriver].dataSize == 0U)
                {
                    /* Actual data buffer sent out, switch to a next one */
                    handle->queueDriver = (handle->queueDriver + 1U) % I2S_NUM_BUFFERS;

                    /* Notify user */
                    if (handle->completionCallback)
                    {
                        handle->completionCallback(base, handle, kStatus_I2S_BufferComplete, handle->userData);
                    }

                    /* Check if the next buffer contains anything to send */
                    if (handle->i2sQueue[handle->queueDriver].dataSize == 0U)
                    {
                        /* Everything has been written to FIFO */
                        handle->state = kI2S_StateTxWaitToWriteDummyData;
                        break;
                    }
                }
            }
        }
        else if (handle->state == kI2S_StateTxWaitToWriteDummyData)
        {
            /* Write dummy data */
            if ((handle->dataLength > 16U) && (handle->dataLength < 25U))
            {
                if (handle->useFifo48H)
                {
                    base->FIFOWR48H = 0U;
                    handle->useFifo48H = false;
                }
                else
                {
                    base->FIFOWR = 0U;
                    base->FIFOWR48H = 0U;
                }
            }
            else
            {
                base->FIFOWR = 0U;
            }

            /* Next time invoke this handler when FIFO becomes empty (TX level 0) */
            base->FIFOTRIG &= ~I2S_FIFOTRIG_TXLVL_MASK;
            handle->state = kI2S_StateTxWaitForEmptyFifo;
        }
        else if (handle->state == kI2S_StateTxWaitForEmptyFifo)
        {
            /* FIFO, including additional dummy data, has been emptied now,
             * all relevant data should have been output from peripheral */

            /* Stop transfer */
            I2S_Disable(base);
            I2S_DisableInterrupts(base, kI2S_TxErrorFlag | kI2S_TxLevelFlag);
            base->FIFOCFG |= I2S_FIFOCFG_EMPTYTX_MASK;

            /* Reset state */
            handle->state = kI2S_StateIdle;

            /* Notify user */
            if (handle->completionCallback)
            {
                handle->completionCallback(base, handle, kStatus_I2S_Done, handle->userData);
            }
        }
        else
        {
            /* Do nothing */
        }

        /* Clear TX level interrupt flag */
        base->FIFOSTAT = I2S_FIFOSTAT_TXLVL(1U);
    }
}

/*!
 * brief Invoked from interrupt handler when receive FIFO level decreases.
 *
 * param base I2S base pointer.
 * param handle pointer to handle structure.
 */
void I2S_RxHandleIRQ(I2S_Type *base, i2s_handle_t *handle)
{
    uint32_t intstat = base->FIFOINTSTAT;
    uint32_t data;

    if (intstat & I2S_FIFOINTSTAT_RXERR_MASK)
    {
        handle->errorCount++;

        /* Clear RX error interrupt flag */
        base->FIFOSTAT = I2S_FIFOSTAT_RXERR(1U);
    }

    if (intstat & I2S_FIFOINTSTAT_RXLVL_MASK)
    {
        while ((base->FIFOSTAT & I2S_FIFOSTAT_RXNOTEMPTY_MASK) && (handle->i2sQueue[handle->queueDriver].dataSize > 0U))
        {
            /* Read input data */
            if (handle->dataLength == 4U)
            {
                data = base->FIFORD;
                *(handle->i2sQueue[handle->queueDriver].data) = ((data & 0x000F0000U) >> 12U) | (data & 0x0000000FU);
                handle->i2sQueue[handle->queueDriver].data++;
                handle->transferCount++;
                handle->i2sQueue[handle->queueDriver].dataSize--;
            }
            else if (handle->dataLength <= 8U)
            {
                data = base->FIFORD;
                *((volatile uint16_t *)handle->i2sQueue[handle->queueDriver].data) =
                    ((data >> 8U) & 0xFF00U) | (data & 0xFFU);
                handle->i2sQueue[handle->queueDriver].data += sizeof(uint16_t);
                handle->transferCount += sizeof(uint16_t);
                handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint16_t);
            }
            else if (handle->dataLength <= 16U)
            {
                data = base->FIFORD;
                *((volatile uint32_t *)handle->i2sQueue[handle->queueDriver].data) = data;
                handle->i2sQueue[handle->queueDriver].data += sizeof(uint32_t);
                handle->transferCount += sizeof(uint32_t);
                handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint32_t);
            }
            else if (handle->dataLength <= 24U)
            {
                if (handle->pack48)
                {
                    if (handle->useFifo48H)
                    {
                        data = base->FIFORD48H;
                        handle->useFifo48H = false;

                        *((volatile uint16_t *)handle->i2sQueue[handle->queueDriver].data) = data;
                        handle->i2sQueue[handle->queueDriver].data += sizeof(uint16_t);
                        handle->transferCount += sizeof(uint16_t);
                        handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint16_t);
                    }
                    else
                    {
                        data = base->FIFORD;
                        handle->useFifo48H = true;

                        *((volatile uint32_t *)handle->i2sQueue[handle->queueDriver].data) = data;
                        handle->i2sQueue[handle->queueDriver].data += sizeof(uint32_t);
                        handle->transferCount += sizeof(uint32_t);
                        handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint32_t);
                    }
                }
                else
                {
                    if (handle->useFifo48H)
                    {
                        data = base->FIFORD48H;
                        handle->useFifo48H = false;
                    }
                    else
                    {
                        data = base->FIFORD;
                        handle->useFifo48H = true;
                    }

                    *(handle->i2sQueue[handle->queueDriver].data++) = data & 0xFFU;
                    *(handle->i2sQueue[handle->queueDriver].data++) = (data >> 8U) & 0xFFU;
                    *(handle->i2sQueue[handle->queueDriver].data++) = (data >> 16U) & 0xFFU;
                    handle->transferCount += 3U;
                    handle->i2sQueue[handle->queueDriver].dataSize -= 3U;
                }
            }
            else /* if (handle->dataLength <= 32U) */
            {
                data = base->FIFORD;
                *((volatile uint32_t *)handle->i2sQueue[handle->queueDriver].data) = data;
                handle->i2sQueue[handle->queueDriver].data += sizeof(uint32_t);
                handle->transferCount += sizeof(uint32_t);
                handle->i2sQueue[handle->queueDriver].dataSize -= sizeof(uint32_t);
            }

            if (handle->i2sQueue[handle->queueDriver].dataSize == 0U)
            {
                /* Actual data buffer filled with input data, switch to a next one */
                handle->queueDriver = (handle->queueDriver + 1U) % I2S_NUM_BUFFERS;

                /* Notify user */
                if (handle->completionCallback)
                {
                    handle->completionCallback(base, handle, kStatus_I2S_BufferComplete, handle->userData);
                }

                if (handle->i2sQueue[handle->queueDriver].dataSize == 0U)
                {
                    /* No other buffer prepared to receive data into */

                    /* Disable I2S operation and interrupts */
                    I2S_Disable(base);
                    I2S_DisableInterrupts(base, kI2S_RxErrorFlag | kI2S_RxLevelFlag);
                    base->FIFOCFG |= I2S_FIFOCFG_EMPTYRX_MASK;

                    /* Reset state */
                    handle->state = kI2S_StateIdle;

                    /* Notify user */
                    if (handle->completionCallback)
                    {
                        handle->completionCallback(base, handle, kStatus_I2S_Done, handle->userData);
                    }

                    /* Clear RX level interrupt flag */
                    base->FIFOSTAT = I2S_FIFOSTAT_RXLVL(1U);

                    return;
                }
            }
        }

        /* Clear RX level interrupt flag */
        base->FIFOSTAT = I2S_FIFOSTAT_RXLVL(1U);
    }
}
