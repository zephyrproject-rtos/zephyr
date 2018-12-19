/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_spi.h"
#include "fsl_flexcomm.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.flexcomm_spi"
#endif

/* Note:  FIFOCFG[SIZE] has always value 1 = 8 items depth */
#define SPI_FIFO_DEPTH(base) ((((base)->FIFOCFG & SPI_FIFOCFG_SIZE_MASK) >> SPI_FIFOCFG_SIZE_SHIFT) << 3)

/* Convert transfer count to transfer bytes. dataWidth is a
 * range <0,15>. Range <8,15> represents 2B transfer */
#define SPI_COUNT_TO_BYTES(dataWidth, count) ((count) << ((dataWidth) >> 3U))
#define SPI_BYTES_TO_COUNT(dataWidth, bytes) ((bytes) >> ((dataWidth) >> 3U))
#define SPI_SSELPOL_MASK ((SPI_CFG_SPOL0_MASK) | (SPI_CFG_SPOL1_MASK) | (SPI_CFG_SPOL2_MASK) | (SPI_CFG_SPOL3_MASK))

/*******************************************************************************
 * Variables
 ******************************************************************************/
/*! @brief internal SPI config array */
static spi_config_t g_configs[FSL_FEATURE_SOC_SPI_COUNT] = {(spi_data_width_t)0};

/*! @brief Array to map SPI instance number to base address. */
static const uint32_t s_spiBaseAddrs[FSL_FEATURE_SOC_SPI_COUNT] = SPI_BASE_ADDRS;

/*! @brief IRQ name array */
static const IRQn_Type s_spiIRQ[] = SPI_IRQS;

/* @brief Dummy data for each instance. This data is used when user's tx buffer is NULL*/
volatile uint8_t s_dummyData[FSL_FEATURE_SOC_SPI_COUNT] = {0};
/*******************************************************************************
 * Code
 ******************************************************************************/

/* Get the index corresponding to the FLEXCOMM */
/*! brief Returns instance number for SPI peripheral base address. */
uint32_t SPI_GetInstance(SPI_Type *base)
{
    int i;

    for (i = 0; i < FSL_FEATURE_SOC_SPI_COUNT; i++)
    {
        if ((uint32_t)base == s_spiBaseAddrs[i])
        {
            return i;
        }
    }

    assert(false);
    return 0;
}

/*!
 * brief Set up the dummy data.
 *
 * param base SPI peripheral address.
 * param dummyData Data to be transferred when tx buffer is NULL.
 */
void SPI_SetDummyData(SPI_Type *base, uint8_t dummyData)
{
    uint32_t instance = SPI_GetInstance(base);
    s_dummyData[instance] = dummyData;
}

/*!
 * brief Returns the configurations.
 *
 * param base SPI peripheral address.
 * return return configurations which contain datawidth and SSEL numbers.
 *         return data type is a pointer of spi_config_t.
 */
void *SPI_GetConfig(SPI_Type *base)
{
    int32_t instance;
    instance = SPI_GetInstance(base);
    if (instance < 0)
    {
        return NULL;
    }
    return &g_configs[instance];
}

/*!
 * brief  Sets the SPI master configuration structure to default values.
 *
 * The purpose of this API is to get the configuration structure initialized for use in SPI_MasterInit().
 * User may use the initialized structure unchanged in SPI_MasterInit(), or modify
 * some fields of the structure before calling SPI_MasterInit(). After calling this API,
 * the master is ready to transfer.
 * Example:
   code
   spi_master_config_t config;
   SPI_MasterGetDefaultConfig(&config);
   endcode
 *
 * param config pointer to master config structure
 */
void SPI_MasterGetDefaultConfig(spi_master_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->enableLoopback = false;
    config->enableMaster = true;
    config->polarity = kSPI_ClockPolarityActiveHigh;
    config->phase = kSPI_ClockPhaseFirstEdge;
    config->direction = kSPI_MsbFirst;
    config->baudRate_Bps = 500000U;
    config->dataWidth = kSPI_Data8Bits;
    config->sselNum = kSPI_Ssel0;
    config->txWatermark = kSPI_TxFifo0;
    config->rxWatermark = kSPI_RxFifo1;
    config->sselPol = kSPI_SpolActiveAllLow;
    config->delayConfig.preDelay = 0U;
    config->delayConfig.postDelay = 0U;
    config->delayConfig.frameDelay = 0U;
    config->delayConfig.transferDelay = 0U;
}

/*!
 * brief Initializes the SPI with master configuration.
 *
 * The configuration structure can be filled by user from scratch, or be set with default
 * values by SPI_MasterGetDefaultConfig(). After calling this API, the slave is ready to transfer.
 * Example
   code
   spi_master_config_t config = {
   .baudRate_Bps = 400000,
   ...
   };
   SPI_MasterInit(SPI0, &config);
   endcode
 *
 * param base SPI base pointer
 * param config pointer to master configuration structure
 * param srcClock_Hz Source clock frequency.
 */
status_t SPI_MasterInit(SPI_Type *base, const spi_master_config_t *config, uint32_t srcClock_Hz)
{
    int32_t result = 0, instance = 0;
    uint32_t tmp;

    /* assert params */
    assert(!((NULL == base) || (NULL == config) || (0 == srcClock_Hz)));
    if ((NULL == base) || (NULL == config) || (0 == srcClock_Hz))
    {
        return kStatus_InvalidArgument;
    }

    /* initialize flexcomm to SPI mode */
    result = FLEXCOMM_Init(base, FLEXCOMM_PERIPH_SPI);
    assert(kStatus_Success == result);
    if (kStatus_Success != result)
    {
        return result;
    }

    /* set divider */
    result = SPI_MasterSetBaud(base, config->baudRate_Bps, srcClock_Hz);
    if (kStatus_Success != result)
    {
        return result;
    }
    /* get instance number */
    instance = SPI_GetInstance(base);
    assert(instance >= 0);

    /* configure SPI mode */
    tmp = base->CFG;
    tmp &= ~(SPI_CFG_MASTER_MASK | SPI_CFG_LSBF_MASK | SPI_CFG_CPHA_MASK | SPI_CFG_CPOL_MASK | SPI_CFG_LOOP_MASK |
             SPI_CFG_ENABLE_MASK | SPI_SSELPOL_MASK);
    /* phase */
    tmp |= SPI_CFG_CPHA(config->phase);
    /* polarity */
    tmp |= SPI_CFG_CPOL(config->polarity);
    /* direction */
    tmp |= SPI_CFG_LSBF(config->direction);
    /* master mode */
    tmp |= SPI_CFG_MASTER(1);
    /* loopback */
    tmp |= SPI_CFG_LOOP(config->enableLoopback);
    /* configure active level for all CS */
    tmp |= ((uint32_t)config->sselPol & (SPI_SSELPOL_MASK));
    base->CFG = tmp;

    /* store configuration */
    g_configs[instance].dataWidth = config->dataWidth;
    g_configs[instance].sselNum = config->sselNum;
    /* enable FIFOs */
    base->FIFOCFG |= SPI_FIFOCFG_EMPTYTX_MASK | SPI_FIFOCFG_EMPTYRX_MASK;
    base->FIFOCFG |= SPI_FIFOCFG_ENABLETX_MASK | SPI_FIFOCFG_ENABLERX_MASK;
    /* trigger level - empty txFIFO, one item in rxFIFO */
    tmp = base->FIFOTRIG & (~(SPI_FIFOTRIG_RXLVL_MASK | SPI_FIFOTRIG_TXLVL_MASK));
    tmp |= SPI_FIFOTRIG_TXLVL(config->txWatermark) | SPI_FIFOTRIG_RXLVL(config->rxWatermark);
    /* enable generating interrupts for FIFOTRIG levels */
    tmp |= SPI_FIFOTRIG_TXLVLENA_MASK | SPI_FIFOTRIG_RXLVLENA_MASK;
    /* set FIFOTRIG */
    base->FIFOTRIG = tmp;

    /* Set the delay configuration. */
    SPI_SetTransferDelay(base, &config->delayConfig);
    /* Set the dummy data. */
    SPI_SetDummyData(base, (uint8_t)SPI_DUMMYDATA);

    SPI_Enable(base, config->enableMaster);
    return kStatus_Success;
}

/*!
 * brief  Sets the SPI slave configuration structure to default values.
 *
 * The purpose of this API is to get the configuration structure initialized for use in SPI_SlaveInit().
 * Modify some fields of the structure before calling SPI_SlaveInit().
 * Example:
   code
   spi_slave_config_t config;
   SPI_SlaveGetDefaultConfig(&config);
   endcode
 *
 * param config pointer to slave configuration structure
 */
void SPI_SlaveGetDefaultConfig(spi_slave_config_t *config)
{
    assert(NULL != config);

    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    config->enableSlave = true;
    config->polarity = kSPI_ClockPolarityActiveHigh;
    config->phase = kSPI_ClockPhaseFirstEdge;
    config->direction = kSPI_MsbFirst;
    config->dataWidth = kSPI_Data8Bits;
    config->txWatermark = kSPI_TxFifo0;
    config->rxWatermark = kSPI_RxFifo1;
    config->sselPol = kSPI_SpolActiveAllLow;
}

/*!
 * brief Initializes the SPI with slave configuration.
 *
 * The configuration structure can be filled by user from scratch or be set with
 * default values by SPI_SlaveGetDefaultConfig().
 * After calling this API, the slave is ready to transfer.
 * Example
   code
    spi_slave_config_t config = {
    .polarity = flexSPIClockPolarity_ActiveHigh;
    .phase = flexSPIClockPhase_FirstEdge;
    .direction = flexSPIMsbFirst;
    ...
    };
    SPI_SlaveInit(SPI0, &config);
   endcode
 *
 * param base SPI base pointer
 * param config pointer to slave configuration structure
 */
status_t SPI_SlaveInit(SPI_Type *base, const spi_slave_config_t *config)
{
    int32_t result = 0, instance;
    uint32_t tmp;

    /* assert params */
    assert(!((NULL == base) || (NULL == config)));
    if ((NULL == base) || (NULL == config))
    {
        return kStatus_InvalidArgument;
    }
    /* configure flexcomm to SPI, enable clock gate */
    result = FLEXCOMM_Init(base, FLEXCOMM_PERIPH_SPI);
    assert(kStatus_Success == result);
    if (kStatus_Success != result)
    {
        return result;
    }

    instance = SPI_GetInstance(base);

    /* configure SPI mode */
    tmp = base->CFG;
    tmp &= ~(SPI_CFG_MASTER_MASK | SPI_CFG_LSBF_MASK | SPI_CFG_CPHA_MASK | SPI_CFG_CPOL_MASK | SPI_CFG_ENABLE_MASK |
             SPI_SSELPOL_MASK);
    /* phase */
    tmp |= SPI_CFG_CPHA(config->phase);
    /* polarity */
    tmp |= SPI_CFG_CPOL(config->polarity);
    /* direction */
    tmp |= SPI_CFG_LSBF(config->direction);
    /* configure active level for all CS */
    tmp |= ((uint32_t)config->sselPol & (SPI_SSELPOL_MASK));
    base->CFG = tmp;

    /* store configuration */
    g_configs[instance].dataWidth = config->dataWidth;
    /* empty and enable FIFOs */
    base->FIFOCFG |= SPI_FIFOCFG_EMPTYTX_MASK | SPI_FIFOCFG_EMPTYRX_MASK;
    base->FIFOCFG |= SPI_FIFOCFG_ENABLETX_MASK | SPI_FIFOCFG_ENABLERX_MASK;
    /* trigger level - empty txFIFO, one item in rxFIFO */
    tmp = base->FIFOTRIG & (~(SPI_FIFOTRIG_RXLVL_MASK | SPI_FIFOTRIG_TXLVL_MASK));
    tmp |= SPI_FIFOTRIG_TXLVL(config->txWatermark) | SPI_FIFOTRIG_RXLVL(config->rxWatermark);
    /* enable generating interrupts for FIFOTRIG levels */
    tmp |= SPI_FIFOTRIG_TXLVLENA_MASK | SPI_FIFOTRIG_RXLVLENA_MASK;
    /* set FIFOTRIG */
    base->FIFOTRIG = tmp;

    SPI_SetDummyData(base, (uint8_t)SPI_DUMMYDATA);

    SPI_Enable(base, config->enableSlave);
    return kStatus_Success;
}

/*!
 * brief De-initializes the SPI.
 *
 * Calling this API resets the SPI module, gates the SPI clock.
 * The SPI module can't work unless calling the SPI_MasterInit/SPI_SlaveInit to initialize module.
 *
 * param base SPI base pointer
 */
void SPI_Deinit(SPI_Type *base)
{
    /* Assert arguments */
    assert(NULL != base);
    /* Disable interrupts, disable dma requests, disable peripheral */
    base->FIFOINTENCLR = SPI_FIFOINTENCLR_TXERR_MASK | SPI_FIFOINTENCLR_RXERR_MASK | SPI_FIFOINTENCLR_TXLVL_MASK |
                         SPI_FIFOINTENCLR_RXLVL_MASK;
    base->FIFOCFG &= ~(SPI_FIFOCFG_DMATX_MASK | SPI_FIFOCFG_DMARX_MASK);
    base->CFG &= ~(SPI_CFG_ENABLE_MASK);
}

/*!
 * brief Enables the DMA request from SPI txFIFO.
 *
 * param base SPI base pointer
 * param enable True means enable DMA, false means disable DMA
 */
void SPI_EnableTxDMA(SPI_Type *base, bool enable)
{
    if (enable)
    {
        base->FIFOCFG |= SPI_FIFOCFG_DMATX_MASK;
    }
    else
    {
        base->FIFOCFG &= ~SPI_FIFOCFG_DMATX_MASK;
    }
}

/*!
 * brief Enables the DMA request from SPI rxFIFO.
 *
 * param base SPI base pointer
 * param enable True means enable DMA, false means disable DMA
 */
void SPI_EnableRxDMA(SPI_Type *base, bool enable)
{
    if (enable)
    {
        base->FIFOCFG |= SPI_FIFOCFG_DMARX_MASK;
    }
    else
    {
        base->FIFOCFG &= ~SPI_FIFOCFG_DMARX_MASK;
    }
}

/*!
 * brief Sets the baud rate for SPI transfer. This is only used in master.
 *
 * param base SPI base pointer
 * param baudrate_Bps baud rate needed in Hz.
 * param srcClock_Hz SPI source clock frequency in Hz.
 */
status_t SPI_MasterSetBaud(SPI_Type *base, uint32_t baudrate_Bps, uint32_t srcClock_Hz)
{
    uint32_t tmp;

    /* assert params */
    assert(!((NULL == base) || (0 == baudrate_Bps) || (0 == srcClock_Hz)));
    if ((NULL == base) || (0 == baudrate_Bps) || (0 == srcClock_Hz))
    {
        return kStatus_InvalidArgument;
    }

    /* calculate baudrate */
    tmp = (srcClock_Hz / baudrate_Bps) - 1;
    if (tmp > 0xFFFF)
    {
        return kStatus_SPI_BaudrateNotSupport;
    }
    base->DIV &= ~SPI_DIV_DIVVAL_MASK;
    base->DIV |= SPI_DIV_DIVVAL(tmp);
    return kStatus_Success;
}

/*!
 * brief Writes a data into the SPI data register.
 *
 * param base SPI base pointer
 * param data needs to be write.
 * param configFlags transfer configuration options ref spi_xfer_option_t
 */
void SPI_WriteData(SPI_Type *base, uint16_t data, uint32_t configFlags)
{
    uint32_t control = 0;
    int32_t instance;

    /* check params */
    assert(NULL != base);
    /* get and check instance */
    instance = SPI_GetInstance(base);
    assert(!(instance < 0));
    if (instance < 0)
    {
        return;
    }

    /* set data width */
    control |= SPI_FIFOWR_LEN(g_configs[instance].dataWidth);
    /* set sssel */
    control |= (SPI_DEASSERT_ALL & (~SPI_DEASSERTNUM_SSEL(g_configs[instance].sselNum)));
    /* mask configFlags */
    control |= (configFlags & SPI_FIFOWR_FLAGS_MASK);
    /* control should not affect lower 16 bits */
    assert(!(control & 0xFFFF));
    base->FIFOWR = data | control;
}

/*!
 * brief Initializes the SPI master handle.
 *
 * This function initializes the SPI master handle which can be used for other SPI master transactional APIs. Usually,
 * for a specified SPI instance, call this API once to get the initialized handle.
 *
 * param base SPI peripheral base address.
 * param handle SPI handle pointer.
 * param callback Callback function.
 * param userData User data.
 */
status_t SPI_MasterTransferCreateHandle(SPI_Type *base,
                                        spi_master_handle_t *handle,
                                        spi_master_callback_t callback,
                                        void *userData)
{
    int32_t instance = 0;

    /* check 'base' */
    assert(!(NULL == base));
    if (NULL == base)
    {
        return kStatus_InvalidArgument;
    }
    /* check 'handle' */
    assert(!(NULL == handle));
    if (NULL == handle)
    {
        return kStatus_InvalidArgument;
    }
    /* get flexcomm instance by 'base' param */
    instance = SPI_GetInstance(base);
    assert(!(instance < 0));
    if (instance < 0)
    {
        return kStatus_InvalidArgument;
    }

    memset(handle, 0, sizeof(*handle));
    /* Initialize the handle */
    if (base->CFG & SPI_CFG_MASTER_MASK)
    {
        FLEXCOMM_SetIRQHandler(base, (flexcomm_irq_handler_t)SPI_MasterTransferHandleIRQ, handle);
    }
    else
    {
        FLEXCOMM_SetIRQHandler(base, (flexcomm_irq_handler_t)SPI_SlaveTransferHandleIRQ, handle);
    }

    handle->dataWidth = g_configs[instance].dataWidth;
    /* in slave mode, the sselNum is not important */
    handle->sselNum = g_configs[instance].sselNum;
    handle->txWatermark = (spi_txfifo_watermark_t)SPI_FIFOTRIG_TXLVL_GET(base);
    handle->rxWatermark = (spi_rxfifo_watermark_t)SPI_FIFOTRIG_RXLVL_GET(base);
    handle->callback = callback;
    handle->userData = userData;

    /* Enable SPI NVIC */
    EnableIRQ(s_spiIRQ[instance]);

    return kStatus_Success;
}

/*!
 * brief Transfers a block of data using a polling method.
 *
 * param base SPI base pointer
 * param xfer pointer to spi_xfer_config_t structure
 * retval kStatus_Success Successfully start a transfer.
 * retval kStatus_InvalidArgument Input argument is invalid.
 */
status_t SPI_MasterTransferBlocking(SPI_Type *base, spi_transfer_t *xfer)
{
    int32_t instance;
    uint32_t tx_ctrl = 0, last_ctrl = 0;
    uint32_t tmp32, rxRemainingBytes, txRemainingBytes, dataWidth;
    uint32_t toReceiveCount = 0;
    uint8_t *txData, *rxData;
    uint32_t fifoDepth;

    /* check params */
    assert(!((NULL == base) || (NULL == xfer) || ((NULL == xfer->txData) && (NULL == xfer->rxData))));
    if ((NULL == base) || (NULL == xfer) || ((NULL == xfer->txData) && (NULL == xfer->rxData)))
    {
        return kStatus_InvalidArgument;
    }

    fifoDepth = SPI_FIFO_DEPTH(base);
    txData = xfer->txData;
    rxData = xfer->rxData;
    txRemainingBytes = txData ? xfer->dataSize : 0;
    rxRemainingBytes = rxData ? xfer->dataSize : 0;

    instance = SPI_GetInstance(base);
    assert(instance >= 0);
    dataWidth = g_configs[instance].dataWidth;

    /* dataSize (in bytes) is not aligned to 16bit (2B) transfer */
    assert(!((dataWidth > kSPI_Data8Bits) && (xfer->dataSize & 0x1)));
    if ((dataWidth > kSPI_Data8Bits) && (xfer->dataSize & 0x1))
    {
        return kStatus_InvalidArgument;
    }

    /* clear tx/rx errors and empty FIFOs */
    base->FIFOCFG |= SPI_FIFOCFG_EMPTYTX_MASK | SPI_FIFOCFG_EMPTYRX_MASK;
    base->FIFOSTAT |= SPI_FIFOSTAT_TXERR_MASK | SPI_FIFOSTAT_RXERR_MASK;
    /* select slave to talk with */
    tx_ctrl |= (SPI_DEASSERT_ALL & (~SPI_DEASSERTNUM_SSEL(g_configs[instance].sselNum)));
    /* set width of data - range asserted at entry */
    tx_ctrl |= SPI_FIFOWR_LEN(dataWidth);
    /* delay for frames */
    tx_ctrl |= (xfer->configFlags & (uint32_t)kSPI_FrameDelay) ? (uint32_t)kSPI_FrameDelay : 0;
    /* end of transfer */
    last_ctrl |= (xfer->configFlags & (uint32_t)kSPI_FrameAssert) ? (uint32_t)kSPI_FrameAssert : 0;
    /* last index of loop */
    while (txRemainingBytes || rxRemainingBytes || toReceiveCount)
    {
        /* if rxFIFO is not empty */
        if (base->FIFOSTAT & SPI_FIFOSTAT_RXNOTEMPTY_MASK)
        {
            tmp32 = base->FIFORD;
            /* rxBuffer is not empty */
            if (rxRemainingBytes)
            {
                *(rxData++) = tmp32;
                rxRemainingBytes--;
                /* read 16 bits at once */
                if (dataWidth > 8)
                {
                    *(rxData++) = tmp32 >> 8;
                    rxRemainingBytes--;
                }
            }
            /* decrease number of data expected to receive */
            toReceiveCount -= 1;
        }
        /* transmit if txFIFO is not full and data to receive does not exceed FIFO depth */
        if ((base->FIFOSTAT & SPI_FIFOSTAT_TXNOTFULL_MASK) && (toReceiveCount < fifoDepth) &&
            ((txRemainingBytes) || (rxRemainingBytes >= SPI_COUNT_TO_BYTES(dataWidth, toReceiveCount + 1))))
        {
            /* txBuffer is not empty */
            if (txRemainingBytes)
            {
                tmp32 = *(txData++);
                txRemainingBytes--;
                /* write 16 bit at once */
                if (dataWidth > 8)
                {
                    tmp32 |= ((uint32_t)(*(txData++))) << 8U;
                    txRemainingBytes--;
                }
                if (!txRemainingBytes)
                {
                    tx_ctrl |= last_ctrl;
                }
            }
            else
            {
                tmp32 = ((uint32_t)s_dummyData[instance] << 8U | (s_dummyData[instance]));
                /* last transfer */
                if (rxRemainingBytes == SPI_COUNT_TO_BYTES(dataWidth, toReceiveCount + 1))
                {
                    tx_ctrl |= last_ctrl;
                }
            }
            /* send data */
            tmp32 = tx_ctrl | tmp32;
            base->FIFOWR = tmp32;
            toReceiveCount += 1;
        }
    }
    /* wait if TX FIFO of previous transfer is not empty */
    while (!(base->FIFOSTAT & SPI_FIFOSTAT_TXEMPTY_MASK))
    {
    }
    return kStatus_Success;
}

/*!
 * brief Performs a non-blocking SPI interrupt transfer.
 *
 * param base SPI peripheral base address.
 * param handle pointer to spi_master_handle_t structure which stores the transfer state
 * param xfer pointer to spi_xfer_config_t structure
 * retval kStatus_Success Successfully start a transfer.
 * retval kStatus_InvalidArgument Input argument is invalid.
 * retval kStatus_SPI_Busy SPI is not idle, is running another transfer.
 */
status_t SPI_MasterTransferNonBlocking(SPI_Type *base, spi_master_handle_t *handle, spi_transfer_t *xfer)
{
    /* check params */
    assert(
        !((NULL == base) || (NULL == handle) || (NULL == xfer) || ((NULL == xfer->txData) && (NULL == xfer->rxData))));
    if ((NULL == base) || (NULL == handle) || (NULL == xfer) || ((NULL == xfer->txData) && (NULL == xfer->rxData)))
    {
        return kStatus_InvalidArgument;
    }

    /* dataSize (in bytes) is not aligned to 16bit (2B) transfer */
    assert(!((handle->dataWidth > kSPI_Data8Bits) && (xfer->dataSize & 0x1)));
    if ((handle->dataWidth > kSPI_Data8Bits) && (xfer->dataSize & 0x1))
    {
        return kStatus_InvalidArgument;
    }

    /* Check if SPI is busy */
    if (handle->state == kStatus_SPI_Busy)
    {
        return kStatus_SPI_Busy;
    }

    /* Set the handle information */
    handle->txData = xfer->txData;
    handle->rxData = xfer->rxData;
    /* set count */
    handle->txRemainingBytes = xfer->txData ? xfer->dataSize : 0;
    handle->rxRemainingBytes = xfer->rxData ? xfer->dataSize : 0;
    handle->totalByteCount = xfer->dataSize;
    /* other options */
    handle->toReceiveCount = 0;
    handle->configFlags = xfer->configFlags;
    /* Set the SPI state to busy */
    handle->state = kStatus_SPI_Busy;
    /* clear FIFOs when transfer starts */
    base->FIFOCFG |= SPI_FIFOCFG_EMPTYTX_MASK | SPI_FIFOCFG_EMPTYRX_MASK;
    base->FIFOSTAT |= SPI_FIFOSTAT_TXERR_MASK | SPI_FIFOSTAT_RXERR_MASK;
    /* enable generating txIRQ and rxIRQ, first transfer is fired by empty txFIFO */
    base->FIFOINTENSET |= SPI_FIFOINTENSET_TXLVL_MASK | SPI_FIFOINTENSET_RXLVL_MASK;
    return kStatus_Success;
}

/*!
 * brief Transfers a block of data using a polling method.
 *
 * This function will do a half-duplex transfer for SPI master, This is a blocking function,
 * which does not retuen until all transfer have been completed. And data transfer mechanism is half-duplex,
 * users can set transmit first or receive first.
 *
 * param base SPI base pointer
 * param xfer pointer to spi_half_duplex_transfer_t structure
 * return status of status_t.
 */
status_t SPI_MasterHalfDuplexTransferBlocking(SPI_Type *base, spi_half_duplex_transfer_t *xfer)
{
    assert(xfer);

    spi_transfer_t tempXfer = {0};
    status_t status;

    if (xfer->isTransmitFirst)
    {
        tempXfer.txData = xfer->txData;
        tempXfer.rxData = NULL;
        tempXfer.dataSize = xfer->txDataSize;
    }
    else
    {
        tempXfer.txData = NULL;
        tempXfer.rxData = xfer->rxData;
        tempXfer.dataSize = xfer->rxDataSize;
    }
    /* If the pcs pin keep assert between transmit and receive. */
    if (xfer->isPcsAssertInTransfer)
    {
        tempXfer.configFlags = (xfer->configFlags) & (uint32_t)(~kSPI_FrameAssert);
    }
    else
    {
        tempXfer.configFlags = (xfer->configFlags) | kSPI_FrameAssert;
    }

    status = SPI_MasterTransferBlocking(base, &tempXfer);

    if (status != kStatus_Success)
    {
        return status;
    }

    if (xfer->isTransmitFirst)
    {
        tempXfer.txData = NULL;
        tempXfer.rxData = xfer->rxData;
        tempXfer.dataSize = xfer->rxDataSize;
    }
    else
    {
        tempXfer.txData = xfer->txData;
        tempXfer.rxData = NULL;
        tempXfer.dataSize = xfer->txDataSize;
    }
    tempXfer.configFlags = xfer->configFlags;

    /* SPI transfer blocking. */
    status = SPI_MasterTransferBlocking(base, &tempXfer);

    return status;
}

/*!
 * brief Performs a non-blocking SPI interrupt transfer.
 *
 * This function using polling way to do the first half transimission and using interrupts to
 * do the second half transimission, the transfer mechanism is half-duplex.
 * When do the second half transimission, code will return right away. When all data is transferred,
 * the callback function is called.
 *
 * param base SPI peripheral base address.
 * param handle pointer to spi_master_handle_t structure which stores the transfer state
 * param xfer pointer to spi_half_duplex_transfer_t structure
 * return status of status_t.
 */
status_t SPI_MasterHalfDuplexTransferNonBlocking(SPI_Type *base,
                                                 spi_master_handle_t *handle,
                                                 spi_half_duplex_transfer_t *xfer)
{
    assert(xfer);
    assert(handle);
    spi_transfer_t tempXfer = {0};
    status_t status;

    if (xfer->isTransmitFirst)
    {
        tempXfer.txData = xfer->txData;
        tempXfer.rxData = NULL;
        tempXfer.dataSize = xfer->txDataSize;
    }
    else
    {
        tempXfer.txData = NULL;
        tempXfer.rxData = xfer->rxData;
        tempXfer.dataSize = xfer->rxDataSize;
    }
    /* If the PCS pin keep assert between transmit and receive. */
    if (xfer->isPcsAssertInTransfer)
    {
        tempXfer.configFlags = (xfer->configFlags) & (uint32_t)(~kSPI_FrameAssert);
    }
    else
    {
        tempXfer.configFlags = (xfer->configFlags) | kSPI_FrameAssert;
    }

    status = SPI_MasterTransferBlocking(base, &tempXfer);
    if (status != kStatus_Success)
    {
        return status;
    }

    if (xfer->isTransmitFirst)
    {
        tempXfer.txData = NULL;
        tempXfer.rxData = xfer->rxData;
        tempXfer.dataSize = xfer->rxDataSize;
    }
    else
    {
        tempXfer.txData = xfer->txData;
        tempXfer.rxData = NULL;
        tempXfer.dataSize = xfer->txDataSize;
    }
    tempXfer.configFlags = xfer->configFlags;

    status = SPI_MasterTransferNonBlocking(base, handle, &tempXfer);

    return status;
}

/*!
 * brief Gets the master transfer count.
 *
 * This function gets the master transfer count.
 *
 * param base SPI peripheral base address.
 * param handle Pointer to the spi_master_handle_t structure which stores the transfer state.
 * param count The number of bytes transferred by using the non-blocking transaction.
 * return status of status_t.
 */
status_t SPI_MasterTransferGetCount(SPI_Type *base, spi_master_handle_t *handle, size_t *count)
{
    assert(NULL != handle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    /* Catch when there is not an active transfer. */
    if (handle->state != kStatus_SPI_Busy)
    {
        *count = 0;
        return kStatus_NoTransferInProgress;
    }

    *count = handle->totalByteCount - handle->rxRemainingBytes;
    return kStatus_Success;
}

/*!
 * brief SPI master aborts a transfer using an interrupt.
 *
 * This function aborts a transfer using an interrupt.
 *
 * param base SPI peripheral base address.
 * param handle Pointer to the spi_master_handle_t structure which stores the transfer state.
 */
void SPI_MasterTransferAbort(SPI_Type *base, spi_master_handle_t *handle)
{
    assert(NULL != handle);

    /* Disable interrupt requests*/
    base->FIFOINTENSET &= ~(SPI_FIFOINTENSET_TXLVL_MASK | SPI_FIFOINTENSET_RXLVL_MASK);
    /* Empty FIFOs */
    base->FIFOCFG |= SPI_FIFOCFG_EMPTYTX_MASK | SPI_FIFOCFG_EMPTYRX_MASK;

    handle->state = kStatus_SPI_Idle;
    handle->txRemainingBytes = 0;
    handle->rxRemainingBytes = 0;
}

static void SPI_TransferHandleIRQInternal(SPI_Type *base, spi_master_handle_t *handle)
{
    uint32_t tx_ctrl = 0, last_ctrl = 0, tmp32;
    bool loopContinue;
    uint32_t fifoDepth;
    /* Get flexcomm instance by 'base' param */
    uint32_t instance = SPI_GetInstance(base);

    /* check params */
    assert((NULL != base) && (NULL != handle) && ((NULL != handle->txData) || (NULL != handle->rxData)));

    fifoDepth = SPI_FIFO_DEPTH(base);
    /* select slave to talk with */
    tx_ctrl |= (SPI_DEASSERT_ALL & SPI_ASSERTNUM_SSEL(handle->sselNum));
    /* set width of data */
    tx_ctrl |= SPI_FIFOWR_LEN(handle->dataWidth);
    /* delay for frames */
    tx_ctrl |= (handle->configFlags & (uint32_t)kSPI_FrameDelay) ? (uint32_t)kSPI_FrameDelay : 0;
    /* end of transfer */
    last_ctrl |= (handle->configFlags & (uint32_t)kSPI_FrameAssert) ? (uint32_t)kSPI_FrameAssert : 0;
    do
    {
        loopContinue = false;

        /* rxFIFO is not empty */
        if (base->FIFOSTAT & SPI_FIFOSTAT_RXNOTEMPTY_MASK)
        {
            tmp32 = base->FIFORD;
            /* rxBuffer is not empty */
            if (handle->rxRemainingBytes)
            {
                /* low byte must go first */
                *(handle->rxData++) = tmp32;
                handle->rxRemainingBytes--;
                /* read 16 bits at once */
                if (handle->dataWidth > kSPI_Data8Bits)
                {
                    *(handle->rxData++) = tmp32 >> 8;
                    handle->rxRemainingBytes--;
                }
            }
            /* decrease number of data expected to receive */
            handle->toReceiveCount -= 1;
            loopContinue = true;
        }

        /* - txFIFO is not full
         * - we cannot cause rxFIFO overflow by sending more data than is the depth of FIFO
         * - txBuffer is not empty or the next 'toReceiveCount' data can fit into rxBuffer
         */
        if ((base->FIFOSTAT & SPI_FIFOSTAT_TXNOTFULL_MASK) && (handle->toReceiveCount < fifoDepth) &&
            ((handle->txRemainingBytes) ||
             (handle->rxRemainingBytes >= SPI_COUNT_TO_BYTES(handle->dataWidth, handle->toReceiveCount + 1))))
        {
            /* txBuffer is not empty */
            if (handle->txRemainingBytes)
            {
                /* low byte must go first */
                tmp32 = *(handle->txData++);
                handle->txRemainingBytes--;
                /* write 16 bit at once */
                if (handle->dataWidth > kSPI_Data8Bits)
                {
                    tmp32 |= ((uint32_t)(*(handle->txData++))) << 8U;
                    handle->txRemainingBytes--;
                }
                /* last transfer */
                if (!handle->txRemainingBytes)
                {
                    tx_ctrl |= last_ctrl;
                }
            }
            else
            {
                tmp32 = ((uint32_t)s_dummyData[instance] << 8U | (s_dummyData[instance]));
                /* last transfer */
                if (handle->rxRemainingBytes == SPI_COUNT_TO_BYTES(handle->dataWidth, handle->toReceiveCount + 1))
                {
                    tx_ctrl |= last_ctrl;
                }
            }
            /* send data */
            tmp32 = tx_ctrl | tmp32;
            base->FIFOWR = tmp32;
            /* increase number of expected data to receive */
            handle->toReceiveCount += 1;
            loopContinue = true;
        }
    } while (loopContinue);
}

/*!
 * brief Interrupts the handler for the SPI.
 *
 * param base SPI peripheral base address.
 * param handle pointer to spi_master_handle_t structure which stores the transfer state.
 */
void SPI_MasterTransferHandleIRQ(SPI_Type *base, spi_master_handle_t *handle)
{
    assert((NULL != base) && (NULL != handle));

    /* IRQ behaviour:
     * - first interrupt is triggered by empty txFIFO. The transfer function
     *   then tries empty rxFIFO and fill txFIFO interleaved that results to
     *   strategy to process as many items as possible.
     * - the next IRQs can be:
     *      rxIRQ from nonempty rxFIFO which requires to empty rxFIFO.
     *      txIRQ from empty txFIFO which requires to refill txFIFO.
     * - last interrupt is triggered by empty txFIFO. The last state is
     *   known by empty rxBuffer and txBuffer. If there is nothing to receive
     *   or send - both operations have been finished and interrupts can be
     *   disabled.
     */

    /* Data to send or read or expected to receive */
    if ((handle->txRemainingBytes) || (handle->rxRemainingBytes) || (handle->toReceiveCount))
    {
        /* Transmit or receive data */
        SPI_TransferHandleIRQInternal(base, handle);
        /* No data to send or read or receive. Transfer ends. Set txTrigger to 0 level and
         * enable txIRQ to confirm when txFIFO becomes empty */
        if ((!handle->txRemainingBytes) && (!handle->rxRemainingBytes) && (!handle->toReceiveCount))
        {
            base->FIFOTRIG = base->FIFOTRIG & (~SPI_FIFOTRIG_TXLVL_MASK);
            base->FIFOINTENSET |= SPI_FIFOINTENSET_TXLVL_MASK;
        }
        else
        {
            uint32_t rxRemainingCount = SPI_BYTES_TO_COUNT(handle->dataWidth, handle->rxRemainingBytes);
            /* If, there are no data to send or rxFIFO is already filled with necessary number of dummy data,
             * disable txIRQ. From this point only rxIRQ is used to receive data without any transmission */
            if ((!handle->txRemainingBytes) && (rxRemainingCount <= handle->toReceiveCount))
            {
                base->FIFOINTENCLR = SPI_FIFOINTENCLR_TXLVL_MASK;
            }
            /* Nothing to receive or transmit, but we still have pending data which are bellow rxLevel.
             * Cannot clear rxFIFO, txFIFO might be still active */
            if (rxRemainingCount == 0)
            {
                if ((handle->txRemainingBytes == 0) && (handle->toReceiveCount != 0) &&
                    (handle->toReceiveCount < SPI_FIFOTRIG_RXLVL_GET(base) + 1))
                {
                    base->FIFOTRIG =
                        (base->FIFOTRIG & (~SPI_FIFOTRIG_RXLVL_MASK)) | SPI_FIFOTRIG_RXLVL(handle->toReceiveCount - 1);
                }
            }
            /* Expected to receive less data than rxLevel value, we have to update rxLevel */
            else
            {
                if (rxRemainingCount < (SPI_FIFOTRIG_RXLVL_GET(base) + 1))
                {
                    base->FIFOTRIG =
                        (base->FIFOTRIG & (~SPI_FIFOTRIG_RXLVL_MASK)) | SPI_FIFOTRIG_RXLVL(rxRemainingCount - 1);
                }
            }
        }
    }
    else
    {
        /* Empty txFIFO is confirmed. Disable IRQs and restore triggers values */
        base->FIFOINTENCLR = SPI_FIFOINTENCLR_RXLVL_MASK | SPI_FIFOINTENCLR_TXLVL_MASK;
        base->FIFOTRIG = (base->FIFOTRIG & (~(SPI_FIFOTRIG_RXLVL_MASK | SPI_FIFOTRIG_RXLVL_MASK))) |
                         SPI_FIFOTRIG_RXLVL(handle->rxWatermark) | SPI_FIFOTRIG_TXLVL(handle->txWatermark);
        /* set idle state and call user callback */
        handle->state = kStatus_SPI_Idle;
        if (handle->callback)
        {
            (handle->callback)(base, handle, handle->state, handle->userData);
        }
    }
}
