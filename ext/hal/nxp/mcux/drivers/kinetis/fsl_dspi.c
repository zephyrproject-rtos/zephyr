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

#include "fsl_dspi.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @brief Typedef for master interrupt handler. */
typedef void (*dspi_master_isr_t)(SPI_Type *base, dspi_master_handle_t *handle);

/*! @brief Typedef for slave interrupt handler. */
typedef void (*dspi_slave_isr_t)(SPI_Type *base, dspi_slave_handle_t *handle);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief Get instance number for DSPI module.
 *
 * @param base DSPI peripheral base address.
 */
uint32_t DSPI_GetInstance(SPI_Type *base);

/*!
 * @brief Configures the DSPI peripheral chip select polarity.
 *
 * This function  takes in the desired peripheral chip select (Pcs) and it's corresponding desired polarity and
 * configures the Pcs signal to operate with the desired characteristic.
 *
 * @param base DSPI peripheral address.
 * @param pcs The particular peripheral chip select (parameter value is of type dspi_which_pcs_t) for which we wish to
 *            apply the active high or active low characteristic.
 * @param activeLowOrHigh The setting for either "active high, inactive low (0)"  or "active low, inactive high(1)" of
 *                        type dspi_pcs_polarity_config_t.
 */
static void DSPI_SetOnePcsPolarity(SPI_Type *base, dspi_which_pcs_t pcs, dspi_pcs_polarity_config_t activeLowOrHigh);

/*!
 * @brief Master fill up the TX FIFO with data.
 * This is not a public API.
 */
static void DSPI_MasterTransferFillUpTxFifo(SPI_Type *base, dspi_master_handle_t *handle);

/*!
 * @brief Master finish up a transfer.
 * It would call back if there is callback function and set the state to idle.
 * This is not a public API.
 */
static void DSPI_MasterTransferComplete(SPI_Type *base, dspi_master_handle_t *handle);

/*!
 * @brief Slave fill up the TX FIFO with data.
 * This is not a public API.
 */
static void DSPI_SlaveTransferFillUpTxFifo(SPI_Type *base, dspi_slave_handle_t *handle);

/*!
 * @brief Slave finish up a transfer.
 * It would call back if there is callback function and set the state to idle.
 * This is not a public API.
 */
static void DSPI_SlaveTransferComplete(SPI_Type *base, dspi_slave_handle_t *handle);

/*!
 * @brief DSPI common interrupt handler.
 *
 * @param base DSPI peripheral address.
 * @param handle pointer to g_dspiHandle which stores the transfer state.
 */
static void DSPI_CommonIRQHandler(SPI_Type *base, void *param);

/*!
 * @brief Master prepare the transfer.
 * Basically it set up dspi_master_handle .
 * This is not a public API.
 */
static void DSPI_MasterTransferPrepare(SPI_Type *base, dspi_master_handle_t *handle, dspi_transfer_t *transfer);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Defines constant value arrays for the baud rate pre-scalar and scalar divider values.*/
static const uint32_t s_baudratePrescaler[] = {2, 3, 5, 7};
static const uint32_t s_baudrateScaler[] = {2,   4,   6,    8,    16,   32,   64,    128,
                                            256, 512, 1024, 2048, 4096, 8192, 16384, 32768};

static const uint32_t s_delayPrescaler[] = {1, 3, 5, 7};
static const uint32_t s_delayScaler[] = {2,   4,    8,    16,   32,   64,    128,   256,
                                         512, 1024, 2048, 4096, 8192, 16384, 32768, 65536};

/*! @brief Pointers to dspi bases for each instance. */
static SPI_Type *const s_dspiBases[] = SPI_BASE_PTRS;

/*! @brief Pointers to dspi IRQ number for each instance. */
static IRQn_Type const s_dspiIRQ[] = SPI_IRQS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Pointers to dspi clocks for each instance. */
static clock_ip_name_t const s_dspiClock[] = DSPI_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/*! @brief Pointers to dspi handles for each instance. */
static void *g_dspiHandle[ARRAY_SIZE(s_dspiBases)];

/*! @brief Pointer to master IRQ handler for each instance. */
static dspi_master_isr_t s_dspiMasterIsr;

/*! @brief Pointer to slave IRQ handler for each instance. */
static dspi_slave_isr_t s_dspiSlaveIsr;

/**********************************************************************************************************************
* Code
*********************************************************************************************************************/
uint32_t DSPI_GetInstance(SPI_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_dspiBases); instance++)
    {
        if (s_dspiBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_dspiBases));

    return instance;
}

void DSPI_MasterInit(SPI_Type *base, const dspi_master_config_t *masterConfig, uint32_t srcClock_Hz)
{
    assert(masterConfig);

    uint32_t temp;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* enable DSPI clock */
    CLOCK_EnableClock(s_dspiClock[DSPI_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    DSPI_Enable(base, true);
    DSPI_StopTransfer(base);

    DSPI_SetMasterSlaveMode(base, kDSPI_Master);

    temp = base->MCR & (~(SPI_MCR_CONT_SCKE_MASK | SPI_MCR_MTFE_MASK | SPI_MCR_ROOE_MASK | SPI_MCR_SMPL_PT_MASK |
                          SPI_MCR_DIS_TXF_MASK | SPI_MCR_DIS_RXF_MASK));

    base->MCR = temp | SPI_MCR_CONT_SCKE(masterConfig->enableContinuousSCK) |
                SPI_MCR_MTFE(masterConfig->enableModifiedTimingFormat) |
                SPI_MCR_ROOE(masterConfig->enableRxFifoOverWrite) | SPI_MCR_SMPL_PT(masterConfig->samplePoint) |
                SPI_MCR_DIS_TXF(false) | SPI_MCR_DIS_RXF(false);

    DSPI_SetOnePcsPolarity(base, masterConfig->whichPcs, masterConfig->pcsActiveHighOrLow);

    if (0 == DSPI_MasterSetBaudRate(base, masterConfig->whichCtar, masterConfig->ctarConfig.baudRate, srcClock_Hz))
    {
        assert(false);
    }

    temp = base->CTAR[masterConfig->whichCtar] &
           ~(SPI_CTAR_FMSZ_MASK | SPI_CTAR_CPOL_MASK | SPI_CTAR_CPHA_MASK | SPI_CTAR_LSBFE_MASK);

    base->CTAR[masterConfig->whichCtar] =
        temp | SPI_CTAR_FMSZ(masterConfig->ctarConfig.bitsPerFrame - 1) | SPI_CTAR_CPOL(masterConfig->ctarConfig.cpol) |
        SPI_CTAR_CPHA(masterConfig->ctarConfig.cpha) | SPI_CTAR_LSBFE(masterConfig->ctarConfig.direction);

    DSPI_MasterSetDelayTimes(base, masterConfig->whichCtar, kDSPI_PcsToSck, srcClock_Hz,
                             masterConfig->ctarConfig.pcsToSckDelayInNanoSec);
    DSPI_MasterSetDelayTimes(base, masterConfig->whichCtar, kDSPI_LastSckToPcs, srcClock_Hz,
                             masterConfig->ctarConfig.lastSckToPcsDelayInNanoSec);
    DSPI_MasterSetDelayTimes(base, masterConfig->whichCtar, kDSPI_BetweenTransfer, srcClock_Hz,
                             masterConfig->ctarConfig.betweenTransferDelayInNanoSec);

    DSPI_StartTransfer(base);
}

void DSPI_MasterGetDefaultConfig(dspi_master_config_t *masterConfig)
{
    assert(masterConfig);

    masterConfig->whichCtar = kDSPI_Ctar0;
    masterConfig->ctarConfig.baudRate = 500000;
    masterConfig->ctarConfig.bitsPerFrame = 8;
    masterConfig->ctarConfig.cpol = kDSPI_ClockPolarityActiveHigh;
    masterConfig->ctarConfig.cpha = kDSPI_ClockPhaseFirstEdge;
    masterConfig->ctarConfig.direction = kDSPI_MsbFirst;

    masterConfig->ctarConfig.pcsToSckDelayInNanoSec = 1000;
    masterConfig->ctarConfig.lastSckToPcsDelayInNanoSec = 1000;
    masterConfig->ctarConfig.betweenTransferDelayInNanoSec = 1000;

    masterConfig->whichPcs = kDSPI_Pcs0;
    masterConfig->pcsActiveHighOrLow = kDSPI_PcsActiveLow;

    masterConfig->enableContinuousSCK = false;
    masterConfig->enableRxFifoOverWrite = false;
    masterConfig->enableModifiedTimingFormat = false;
    masterConfig->samplePoint = kDSPI_SckToSin0Clock;
}

void DSPI_SlaveInit(SPI_Type *base, const dspi_slave_config_t *slaveConfig)
{
    assert(slaveConfig);

    uint32_t temp = 0;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* enable DSPI clock */
    CLOCK_EnableClock(s_dspiClock[DSPI_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    DSPI_Enable(base, true);
    DSPI_StopTransfer(base);

    DSPI_SetMasterSlaveMode(base, kDSPI_Slave);

    temp = base->MCR & (~(SPI_MCR_CONT_SCKE_MASK | SPI_MCR_MTFE_MASK | SPI_MCR_ROOE_MASK | SPI_MCR_SMPL_PT_MASK |
                          SPI_MCR_DIS_TXF_MASK | SPI_MCR_DIS_RXF_MASK));

    base->MCR = temp | SPI_MCR_CONT_SCKE(slaveConfig->enableContinuousSCK) |
                SPI_MCR_MTFE(slaveConfig->enableModifiedTimingFormat) |
                SPI_MCR_ROOE(slaveConfig->enableRxFifoOverWrite) | SPI_MCR_SMPL_PT(slaveConfig->samplePoint) |
                SPI_MCR_DIS_TXF(false) | SPI_MCR_DIS_RXF(false);

    DSPI_SetOnePcsPolarity(base, kDSPI_Pcs0, kDSPI_PcsActiveLow);

    temp = base->CTAR[slaveConfig->whichCtar] &
           ~(SPI_CTAR_FMSZ_MASK | SPI_CTAR_CPOL_MASK | SPI_CTAR_CPHA_MASK | SPI_CTAR_LSBFE_MASK);

    base->CTAR[slaveConfig->whichCtar] = temp | SPI_CTAR_SLAVE_FMSZ(slaveConfig->ctarConfig.bitsPerFrame - 1) |
                                         SPI_CTAR_SLAVE_CPOL(slaveConfig->ctarConfig.cpol) |
                                         SPI_CTAR_SLAVE_CPHA(slaveConfig->ctarConfig.cpha);

    DSPI_StartTransfer(base);
}

void DSPI_SlaveGetDefaultConfig(dspi_slave_config_t *slaveConfig)
{
    assert(slaveConfig);

    slaveConfig->whichCtar = kDSPI_Ctar0;
    slaveConfig->ctarConfig.bitsPerFrame = 8;
    slaveConfig->ctarConfig.cpol = kDSPI_ClockPolarityActiveHigh;
    slaveConfig->ctarConfig.cpha = kDSPI_ClockPhaseFirstEdge;

    slaveConfig->enableContinuousSCK = false;
    slaveConfig->enableRxFifoOverWrite = false;
    slaveConfig->enableModifiedTimingFormat = false;
    slaveConfig->samplePoint = kDSPI_SckToSin0Clock;
}

void DSPI_Deinit(SPI_Type *base)
{
    DSPI_StopTransfer(base);
    DSPI_Enable(base, false);

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* disable DSPI clock */
    CLOCK_DisableClock(s_dspiClock[DSPI_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

static void DSPI_SetOnePcsPolarity(SPI_Type *base, dspi_which_pcs_t pcs, dspi_pcs_polarity_config_t activeLowOrHigh)
{
    uint32_t temp;

    temp = base->MCR;

    if (activeLowOrHigh == kDSPI_PcsActiveLow)
    {
        temp |= SPI_MCR_PCSIS(pcs);
    }
    else
    {
        temp &= ~SPI_MCR_PCSIS(pcs);
    }

    base->MCR = temp;
}

uint32_t DSPI_MasterSetBaudRate(SPI_Type *base,
                                dspi_ctar_selection_t whichCtar,
                                uint32_t baudRate_Bps,
                                uint32_t srcClock_Hz)
{
    /* for master mode configuration, if slave mode detected, return 0*/
    if (!DSPI_IsMaster(base))
    {
        return 0;
    }
    uint32_t temp;
    uint32_t prescaler, bestPrescaler;
    uint32_t scaler, bestScaler;
    uint32_t dbr, bestDbr;
    uint32_t realBaudrate, bestBaudrate;
    uint32_t diff, min_diff;
    uint32_t baudrate = baudRate_Bps;

    /* find combination of prescaler and scaler resulting in baudrate closest to the requested value */
    min_diff = 0xFFFFFFFFU;
    bestPrescaler = 0;
    bestScaler = 0;
    bestDbr = 1;
    bestBaudrate = 0; /* required to avoid compilation warning */

    /* In all for loops, if min_diff = 0, the exit for loop*/
    for (prescaler = 0; (prescaler < 4) && min_diff; prescaler++)
    {
        for (scaler = 0; (scaler < 16) && min_diff; scaler++)
        {
            for (dbr = 1; (dbr < 3) && min_diff; dbr++)
            {
                realBaudrate = ((srcClock_Hz * dbr) / (s_baudratePrescaler[prescaler] * (s_baudrateScaler[scaler])));

                /* calculate the baud rate difference based on the conditional statement that states that the calculated
                * baud rate must not exceed the desired baud rate.
                */
                if (baudrate >= realBaudrate)
                {
                    diff = baudrate - realBaudrate;
                    if (min_diff > diff)
                    {
                        /* a better match found */
                        min_diff = diff;
                        bestPrescaler = prescaler;
                        bestScaler = scaler;
                        bestBaudrate = realBaudrate;
                        bestDbr = dbr;
                    }
                }
            }
        }
    }

    /* write the best dbr, prescalar, and baud rate scalar to the CTAR */
    temp = base->CTAR[whichCtar] & ~(SPI_CTAR_DBR_MASK | SPI_CTAR_PBR_MASK | SPI_CTAR_BR_MASK);

    base->CTAR[whichCtar] = temp | ((bestDbr - 1) << SPI_CTAR_DBR_SHIFT) | (bestPrescaler << SPI_CTAR_PBR_SHIFT) |
                            (bestScaler << SPI_CTAR_BR_SHIFT);

    /* return the actual calculated baud rate */
    return bestBaudrate;
}

void DSPI_MasterSetDelayScaler(
    SPI_Type *base, dspi_ctar_selection_t whichCtar, uint32_t prescaler, uint32_t scaler, dspi_delay_type_t whichDelay)
{
    /* these settings are only relevant in master mode */
    if (DSPI_IsMaster(base))
    {
        switch (whichDelay)
        {
            case kDSPI_PcsToSck:
                base->CTAR[whichCtar] = (base->CTAR[whichCtar] & (~SPI_CTAR_PCSSCK_MASK) & (~SPI_CTAR_CSSCK_MASK)) |
                                        SPI_CTAR_PCSSCK(prescaler) | SPI_CTAR_CSSCK(scaler);
                break;
            case kDSPI_LastSckToPcs:
                base->CTAR[whichCtar] = (base->CTAR[whichCtar] & (~SPI_CTAR_PASC_MASK) & (~SPI_CTAR_ASC_MASK)) |
                                        SPI_CTAR_PASC(prescaler) | SPI_CTAR_ASC(scaler);
                break;
            case kDSPI_BetweenTransfer:
                base->CTAR[whichCtar] = (base->CTAR[whichCtar] & (~SPI_CTAR_PDT_MASK) & (~SPI_CTAR_DT_MASK)) |
                                        SPI_CTAR_PDT(prescaler) | SPI_CTAR_DT(scaler);
                break;
            default:
                break;
        }
    }
}

uint32_t DSPI_MasterSetDelayTimes(SPI_Type *base,
                                  dspi_ctar_selection_t whichCtar,
                                  dspi_delay_type_t whichDelay,
                                  uint32_t srcClock_Hz,
                                  uint32_t delayTimeInNanoSec)
{
    /* for master mode configuration, if slave mode detected, return 0 */
    if (!DSPI_IsMaster(base))
    {
        return 0;
    }

    uint32_t prescaler, bestPrescaler;
    uint32_t scaler, bestScaler;
    uint32_t realDelay, bestDelay;
    uint32_t diff, min_diff;
    uint32_t initialDelayNanoSec;

    /* find combination of prescaler and scaler resulting in the delay closest to the
    * requested value
    */
    min_diff = 0xFFFFFFFFU;
    /* Initialize prescaler and scaler to their max values to generate the max delay */
    bestPrescaler = 0x3;
    bestScaler = 0xF;
    bestDelay = (((1000000000U * 4) / srcClock_Hz) * s_delayPrescaler[bestPrescaler] * s_delayScaler[bestScaler]) / 4;

    /* First calculate the initial, default delay */
    initialDelayNanoSec = 1000000000U / srcClock_Hz * 2;

    /* If the initial, default delay is already greater than the desired delay, then
    * set the delays to their initial value (0) and return the delay. In other words,
    * there is no way to decrease the delay value further.
    */
    if (initialDelayNanoSec >= delayTimeInNanoSec)
    {
        DSPI_MasterSetDelayScaler(base, whichCtar, 0, 0, whichDelay);
        return initialDelayNanoSec;
    }

    /* In all for loops, if min_diff = 0, the exit for loop */
    for (prescaler = 0; (prescaler < 4) && min_diff; prescaler++)
    {
        for (scaler = 0; (scaler < 16) && min_diff; scaler++)
        {
            realDelay = ((4000000000U / srcClock_Hz) * s_delayPrescaler[prescaler] * s_delayScaler[scaler]) / 4;

            /* calculate the delay difference based on the conditional statement
            * that states that the calculated delay must not be less then the desired delay
            */
            if (realDelay >= delayTimeInNanoSec)
            {
                diff = realDelay - delayTimeInNanoSec;
                if (min_diff > diff)
                {
                    /* a better match found */
                    min_diff = diff;
                    bestPrescaler = prescaler;
                    bestScaler = scaler;
                    bestDelay = realDelay;
                }
            }
        }
    }

    /* write the best dbr, prescalar, and baud rate scalar to the CTAR */
    DSPI_MasterSetDelayScaler(base, whichCtar, bestPrescaler, bestScaler, whichDelay);

    /* return the actual calculated baud rate */
    return bestDelay;
}

void DSPI_GetDefaultDataCommandConfig(dspi_command_data_config_t *command)
{
    assert(command);

    command->isPcsContinuous = false;
    command->whichCtar = kDSPI_Ctar0;
    command->whichPcs = kDSPI_Pcs0;
    command->isEndOfQueue = false;
    command->clearTransferCount = false;
}

void DSPI_MasterWriteDataBlocking(SPI_Type *base, dspi_command_data_config_t *command, uint16_t data)
{
    assert(command);

    /* First, clear Transmit Complete Flag (TCF) */
    DSPI_ClearStatusFlags(base, kDSPI_TxCompleteFlag);

    while (!(DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag))
    {
        DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
    }

    base->PUSHR = SPI_PUSHR_CONT(command->isPcsContinuous) | SPI_PUSHR_CTAS(command->whichCtar) |
                  SPI_PUSHR_PCS(command->whichPcs) | SPI_PUSHR_EOQ(command->isEndOfQueue) |
                  SPI_PUSHR_CTCNT(command->clearTransferCount) | SPI_PUSHR_TXDATA(data);
    DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

    /* Wait till TCF sets */
    while (!(DSPI_GetStatusFlags(base) & kDSPI_TxCompleteFlag))
    {
    }
}

void DSPI_MasterWriteCommandDataBlocking(SPI_Type *base, uint32_t data)
{
    /* First, clear Transmit Complete Flag (TCF) */
    DSPI_ClearStatusFlags(base, kDSPI_TxCompleteFlag);

    while (!(DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag))
    {
        DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
    }

    base->PUSHR = data;

    DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

    /* Wait till TCF sets */
    while (!(DSPI_GetStatusFlags(base) & kDSPI_TxCompleteFlag))
    {
    }
}

void DSPI_SlaveWriteDataBlocking(SPI_Type *base, uint32_t data)
{
    /* First, clear Transmit Complete Flag (TCF) */
    DSPI_ClearStatusFlags(base, kDSPI_TxCompleteFlag);

    while (!(DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag))
    {
        DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
    }

    base->PUSHR_SLAVE = data;

    DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

    /* Wait till TCF sets */
    while (!(DSPI_GetStatusFlags(base) & kDSPI_TxCompleteFlag))
    {
    }
}

void DSPI_EnableInterrupts(SPI_Type *base, uint32_t mask)
{
    if (mask & SPI_RSER_TFFF_RE_MASK)
    {
        base->RSER &= ~SPI_RSER_TFFF_DIRS_MASK;
    }
    if (mask & SPI_RSER_RFDF_RE_MASK)
    {
        base->RSER &= ~SPI_RSER_RFDF_DIRS_MASK;
    }
    base->RSER |= mask;
}

/*Transactional APIs -- Master*/

void DSPI_MasterTransferCreateHandle(SPI_Type *base,
                                     dspi_master_handle_t *handle,
                                     dspi_master_transfer_callback_t callback,
                                     void *userData)
{
    assert(handle);

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    g_dspiHandle[DSPI_GetInstance(base)] = handle;

    handle->callback = callback;
    handle->userData = userData;
}

status_t DSPI_MasterTransferBlocking(SPI_Type *base, dspi_transfer_t *transfer)
{
    assert(transfer);

    uint16_t wordToSend = 0;
    uint16_t wordReceived = 0;
    uint8_t dummyData = DSPI_DUMMY_DATA;
    uint8_t bitsPerFrame;

    uint32_t command;
    uint32_t lastCommand;

    uint8_t *txData;
    uint8_t *rxData;
    uint32_t remainingSendByteCount;
    uint32_t remainingReceiveByteCount;

    uint32_t fifoSize;
    dspi_command_data_config_t commandStruct;

    /* If the transfer count is zero, then return immediately.*/
    if (transfer->dataSize == 0)
    {
        return kStatus_InvalidArgument;
    }

    DSPI_StopTransfer(base);
    DSPI_DisableInterrupts(base, kDSPI_AllInterruptEnable);
    DSPI_FlushFifo(base, true, true);
    DSPI_ClearStatusFlags(base, kDSPI_AllStatusFlag);

    /*Calculate the command and lastCommand*/
    commandStruct.whichPcs =
        (dspi_which_pcs_t)(1U << ((transfer->configFlags & DSPI_MASTER_PCS_MASK) >> DSPI_MASTER_PCS_SHIFT));
    commandStruct.isEndOfQueue = false;
    commandStruct.clearTransferCount = false;
    commandStruct.whichCtar =
        (dspi_ctar_selection_t)((transfer->configFlags & DSPI_MASTER_CTAR_MASK) >> DSPI_MASTER_CTAR_SHIFT);
    commandStruct.isPcsContinuous = (bool)(transfer->configFlags & kDSPI_MasterPcsContinuous);

    command = DSPI_MasterGetFormattedCommand(&(commandStruct));

    commandStruct.isEndOfQueue = true;
    commandStruct.isPcsContinuous = (bool)(transfer->configFlags & kDSPI_MasterActiveAfterTransfer);
    lastCommand = DSPI_MasterGetFormattedCommand(&(commandStruct));

    /*Calculate the bitsPerFrame*/
    bitsPerFrame = ((base->CTAR[commandStruct.whichCtar] & SPI_CTAR_FMSZ_MASK) >> SPI_CTAR_FMSZ_SHIFT) + 1;

    txData = transfer->txData;
    rxData = transfer->rxData;
    remainingSendByteCount = transfer->dataSize;
    remainingReceiveByteCount = transfer->dataSize;

    if ((base->MCR & SPI_MCR_DIS_RXF_MASK) || (base->MCR & SPI_MCR_DIS_TXF_MASK))
    {
        fifoSize = 1;
    }
    else
    {
        fifoSize = FSL_FEATURE_DSPI_FIFO_SIZEn(base);
    }

    DSPI_StartTransfer(base);

    if (bitsPerFrame <= 8)
    {
        while (remainingSendByteCount > 0)
        {
            if (remainingSendByteCount == 1)
            {
                while (!(DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag))
                {
                    DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
                }

                if (txData != NULL)
                {
                    base->PUSHR = (*txData) | (lastCommand);
                    txData++;
                }
                else
                {
                    base->PUSHR = (lastCommand) | (dummyData);
                }
                DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
                remainingSendByteCount--;

                while (remainingReceiveByteCount > 0)
                {
                    if (DSPI_GetStatusFlags(base) & kDSPI_RxFifoDrainRequestFlag)
                    {
                        if (rxData != NULL)
                        {
                            /* Read data from POPR*/
                            *(rxData) = DSPI_ReadData(base);
                            rxData++;
                        }
                        else
                        {
                            DSPI_ReadData(base);
                        }
                        remainingReceiveByteCount--;

                        DSPI_ClearStatusFlags(base, kDSPI_RxFifoDrainRequestFlag);
                    }
                }
            }
            else
            {
                /*Wait until Tx Fifo is not full*/
                while (!(DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag))
                {
                    DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
                }
                if (txData != NULL)
                {
                    base->PUSHR = command | (uint16_t)(*txData);
                    txData++;
                }
                else
                {
                    base->PUSHR = command | dummyData;
                }
                remainingSendByteCount--;

                DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

                while ((remainingReceiveByteCount - remainingSendByteCount) >= fifoSize)
                {
                    if (DSPI_GetStatusFlags(base) & kDSPI_RxFifoDrainRequestFlag)
                    {
                        if (rxData != NULL)
                        {
                            *(rxData) = DSPI_ReadData(base);
                            rxData++;
                        }
                        else
                        {
                            DSPI_ReadData(base);
                        }
                        remainingReceiveByteCount--;

                        DSPI_ClearStatusFlags(base, kDSPI_RxFifoDrainRequestFlag);
                    }
                }
            }
        }
    }
    else
    {
        while (remainingSendByteCount > 0)
        {
            if (remainingSendByteCount <= 2)
            {
                while (!(DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag))
                {
                    DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
                }

                if (txData != NULL)
                {
                    wordToSend = *(txData);
                    ++txData;

                    if (remainingSendByteCount > 1)
                    {
                        wordToSend |= (unsigned)(*(txData)) << 8U;
                        ++txData;
                    }
                }
                else
                {
                    wordToSend = dummyData;
                }

                base->PUSHR = lastCommand | wordToSend;

                DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
                remainingSendByteCount = 0;

                while (remainingReceiveByteCount > 0)
                {
                    if (DSPI_GetStatusFlags(base) & kDSPI_RxFifoDrainRequestFlag)
                    {
                        wordReceived = DSPI_ReadData(base);

                        if (remainingReceiveByteCount != 1)
                        {
                            if (rxData != NULL)
                            {
                                *(rxData) = wordReceived;
                                ++rxData;
                                *(rxData) = wordReceived >> 8;
                                ++rxData;
                            }
                            remainingReceiveByteCount -= 2;
                        }
                        else
                        {
                            if (rxData != NULL)
                            {
                                *(rxData) = wordReceived;
                                ++rxData;
                            }
                            remainingReceiveByteCount--;
                        }
                        DSPI_ClearStatusFlags(base, kDSPI_RxFifoDrainRequestFlag);
                    }
                }
            }
            else
            {
                /*Wait until Tx Fifo is not full*/
                while (!(DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag))
                {
                    DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
                }

                if (txData != NULL)
                {
                    wordToSend = *(txData);
                    ++txData;
                    wordToSend |= (unsigned)(*(txData)) << 8U;
                    ++txData;
                }
                else
                {
                    wordToSend = dummyData;
                }
                base->PUSHR = command | wordToSend;
                remainingSendByteCount -= 2;

                DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

                while (((remainingReceiveByteCount - remainingSendByteCount) / 2) >= fifoSize)
                {
                    if (DSPI_GetStatusFlags(base) & kDSPI_RxFifoDrainRequestFlag)
                    {
                        wordReceived = DSPI_ReadData(base);

                        if (rxData != NULL)
                        {
                            *rxData = wordReceived;
                            ++rxData;
                            *rxData = wordReceived >> 8;
                            ++rxData;
                        }
                        remainingReceiveByteCount -= 2;

                        DSPI_ClearStatusFlags(base, kDSPI_RxFifoDrainRequestFlag);
                    }
                }
            }
        }
    }

    return kStatus_Success;
}

static void DSPI_MasterTransferPrepare(SPI_Type *base, dspi_master_handle_t *handle, dspi_transfer_t *transfer)
{
    assert(handle);
    assert(transfer);

    dspi_command_data_config_t commandStruct;

    DSPI_StopTransfer(base);
    DSPI_FlushFifo(base, true, true);
    DSPI_ClearStatusFlags(base, kDSPI_AllStatusFlag);

    commandStruct.whichPcs =
        (dspi_which_pcs_t)(1U << ((transfer->configFlags & DSPI_MASTER_PCS_MASK) >> DSPI_MASTER_PCS_SHIFT));
    commandStruct.isEndOfQueue = false;
    commandStruct.clearTransferCount = false;
    commandStruct.whichCtar =
        (dspi_ctar_selection_t)((transfer->configFlags & DSPI_MASTER_CTAR_MASK) >> DSPI_MASTER_CTAR_SHIFT);
    commandStruct.isPcsContinuous = (bool)(transfer->configFlags & kDSPI_MasterPcsContinuous);
    handle->command = DSPI_MasterGetFormattedCommand(&(commandStruct));

    commandStruct.isEndOfQueue = true;
    commandStruct.isPcsContinuous = (bool)(transfer->configFlags & kDSPI_MasterActiveAfterTransfer);
    handle->lastCommand = DSPI_MasterGetFormattedCommand(&(commandStruct));

    handle->bitsPerFrame = ((base->CTAR[commandStruct.whichCtar] & SPI_CTAR_FMSZ_MASK) >> SPI_CTAR_FMSZ_SHIFT) + 1;

    if ((base->MCR & SPI_MCR_DIS_RXF_MASK) || (base->MCR & SPI_MCR_DIS_TXF_MASK))
    {
        handle->fifoSize = 1;
    }
    else
    {
        handle->fifoSize = FSL_FEATURE_DSPI_FIFO_SIZEn(base);
    }
    handle->txData = transfer->txData;
    handle->rxData = transfer->rxData;
    handle->remainingSendByteCount = transfer->dataSize;
    handle->remainingReceiveByteCount = transfer->dataSize;
    handle->totalByteCount = transfer->dataSize;
}

status_t DSPI_MasterTransferNonBlocking(SPI_Type *base, dspi_master_handle_t *handle, dspi_transfer_t *transfer)
{
    assert(handle);
    assert(transfer);

    /* If the transfer count is zero, then return immediately.*/
    if (transfer->dataSize == 0)
    {
        return kStatus_InvalidArgument;
    }

    /* Check that we're not busy.*/
    if (handle->state == kDSPI_Busy)
    {
        return kStatus_DSPI_Busy;
    }

    handle->state = kDSPI_Busy;

    DSPI_MasterTransferPrepare(base, handle, transfer);
    DSPI_StartTransfer(base);

    /* Enable the NVIC for DSPI peripheral. */
    EnableIRQ(s_dspiIRQ[DSPI_GetInstance(base)]);

    DSPI_MasterTransferFillUpTxFifo(base, handle);

    /* RX FIFO Drain request: RFDF_RE to enable RFDF interrupt
    * Since SPI is a synchronous interface, we only need to enable the RX interrupt.
    * The IRQ handler will get the status of RX and TX interrupt flags.
    */
    s_dspiMasterIsr = DSPI_MasterTransferHandleIRQ;

    DSPI_EnableInterrupts(base, kDSPI_RxFifoDrainRequestInterruptEnable);

    return kStatus_Success;
}

status_t DSPI_MasterTransferGetCount(SPI_Type *base, dspi_master_handle_t *handle, size_t *count)
{
    assert(handle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    /* Catch when there is not an active transfer. */
    if (handle->state != kDSPI_Busy)
    {
        *count = 0;
        return kStatus_NoTransferInProgress;
    }

    *count = handle->totalByteCount - handle->remainingReceiveByteCount;
    return kStatus_Success;
}

static void DSPI_MasterTransferComplete(SPI_Type *base, dspi_master_handle_t *handle)
{
    assert(handle);

    /* Disable interrupt requests*/
    DSPI_DisableInterrupts(base, kDSPI_RxFifoDrainRequestInterruptEnable | kDSPI_TxFifoFillRequestInterruptEnable);

    status_t status = 0;
    if (handle->state == kDSPI_Error)
    {
        status = kStatus_DSPI_Error;
    }
    else
    {
        status = kStatus_Success;
    }

    handle->state = kDSPI_Idle;

    if (handle->callback)
    {
        handle->callback(base, handle, status, handle->userData);
    }
}

static void DSPI_MasterTransferFillUpTxFifo(SPI_Type *base, dspi_master_handle_t *handle)
{
    assert(handle);

    uint16_t wordToSend = 0;
    uint8_t dummyData = DSPI_DUMMY_DATA;

    /* If bits/frame is greater than one byte */
    if (handle->bitsPerFrame > 8)
    {
        /* Fill the fifo until it is full or until the send word count is 0 or until the difference
        * between the remainingReceiveByteCount and remainingSendByteCount equals the FIFO depth.
        * The reason for checking the difference is to ensure we only send as much as the
        * RX FIFO can receive.
        * For this case where bitsPerFrame > 8, each entry in the FIFO contains 2 bytes of the
        * send data, hence the difference between the remainingReceiveByteCount and
        * remainingSendByteCount must be divided by 2 to convert this difference into a
        * 16-bit (2 byte) value.
        */
        while ((DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag) &&
               ((handle->remainingReceiveByteCount - handle->remainingSendByteCount) / 2 < handle->fifoSize))
        {
            if (handle->remainingSendByteCount <= 2)
            {
                if (handle->txData)
                {
                    if (handle->remainingSendByteCount == 1)
                    {
                        wordToSend = *(handle->txData);
                    }
                    else
                    {
                        wordToSend = *(handle->txData);
                        ++handle->txData; /* increment to next data byte */
                        wordToSend |= (unsigned)(*(handle->txData)) << 8U;
                    }
                }
                else
                {
                    wordToSend = dummyData;
                }
                handle->remainingSendByteCount = 0;
                base->PUSHR = handle->lastCommand | wordToSend;
            }
            /* For all words except the last word */
            else
            {
                if (handle->txData)
                {
                    wordToSend = *(handle->txData);
                    ++handle->txData; /* increment to next data byte */
                    wordToSend |= (unsigned)(*(handle->txData)) << 8U;
                    ++handle->txData; /* increment to next data byte */
                }
                else
                {
                    wordToSend = dummyData;
                }
                handle->remainingSendByteCount -= 2; /* decrement remainingSendByteCount by 2 */
                base->PUSHR = handle->command | wordToSend;
            }

            /* Try to clear the TFFF; if the TX FIFO is full this will clear */
            DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

            /* exit loop if send count is zero, else update local variables for next loop */
            if (handle->remainingSendByteCount == 0)
            {
                break;
            }
        } /* End of TX FIFO fill while loop */
    }
    /* Optimized for bits/frame less than or equal to one byte. */
    else
    {
        /* Fill the fifo until it is full or until the send word count is 0 or until the difference
        * between the remainingReceiveByteCount and remainingSendByteCount equals the FIFO depth.
        * The reason for checking the difference is to ensure we only send as much as the
        * RX FIFO can receive.
        */
        while ((DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag) &&
               ((handle->remainingReceiveByteCount - handle->remainingSendByteCount) < handle->fifoSize))
        {
            if (handle->txData)
            {
                wordToSend = *(handle->txData);
                ++handle->txData;
            }
            else
            {
                wordToSend = dummyData;
            }

            if (handle->remainingSendByteCount == 1)
            {
                base->PUSHR = handle->lastCommand | wordToSend;
            }
            else
            {
                base->PUSHR = handle->command | wordToSend;
            }

            /* Try to clear the TFFF; if the TX FIFO is full this will clear */
            DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

            --handle->remainingSendByteCount;

            /* exit loop if send count is zero, else update local variables for next loop */
            if (handle->remainingSendByteCount == 0)
            {
                break;
            }
        }
    }
}

void DSPI_MasterTransferAbort(SPI_Type *base, dspi_master_handle_t *handle)
{
    assert(handle);

    DSPI_StopTransfer(base);

    /* Disable interrupt requests*/
    DSPI_DisableInterrupts(base, kDSPI_RxFifoDrainRequestInterruptEnable | kDSPI_TxFifoFillRequestInterruptEnable);

    handle->state = kDSPI_Idle;
}

void DSPI_MasterTransferHandleIRQ(SPI_Type *base, dspi_master_handle_t *handle)
{
    assert(handle);

    /* RECEIVE IRQ handler: Check read buffer only if there are remaining bytes to read. */
    if (handle->remainingReceiveByteCount)
    {
        /* Check read buffer.*/
        uint16_t wordReceived; /* Maximum supported data bit length in master mode is 16-bits */

        /* If bits/frame is greater than one byte */
        if (handle->bitsPerFrame > 8)
        {
            while (DSPI_GetStatusFlags(base) & kDSPI_RxFifoDrainRequestFlag)
            {
                wordReceived = DSPI_ReadData(base);
                /* clear the rx fifo drain request, needed for non-DMA applications as this flag
                * will remain set even if the rx fifo is empty. By manually clearing this flag, it
                * either remain clear if no more data is in the fifo, or it will set if there is
                * more data in the fifo.
                */
                DSPI_ClearStatusFlags(base, kDSPI_RxFifoDrainRequestFlag);

                /* Store read bytes into rx buffer only if a buffer pointer was provided */
                if (handle->rxData)
                {
                    /* For the last word received, if there is an extra byte due to the odd transfer
                    * byte count, only save the the last byte and discard the upper byte
                    */
                    if (handle->remainingReceiveByteCount == 1)
                    {
                        *handle->rxData = wordReceived; /* Write first data byte */
                        --handle->remainingReceiveByteCount;
                    }
                    else
                    {
                        *handle->rxData = wordReceived;      /* Write first data byte */
                        ++handle->rxData;                    /* increment to next data byte */
                        *handle->rxData = wordReceived >> 8; /* Write second data byte */
                        ++handle->rxData;                    /* increment to next data byte */
                        handle->remainingReceiveByteCount -= 2;
                    }
                }
                else
                {
                    if (handle->remainingReceiveByteCount == 1)
                    {
                        --handle->remainingReceiveByteCount;
                    }
                    else
                    {
                        handle->remainingReceiveByteCount -= 2;
                    }
                }
                if (handle->remainingReceiveByteCount == 0)
                {
                    break;
                }
            } /* End of RX FIFO drain while loop */
        }
        /* Optimized for bits/frame less than or equal to one byte. */
        else
        {
            while (DSPI_GetStatusFlags(base) & kDSPI_RxFifoDrainRequestFlag)
            {
                wordReceived = DSPI_ReadData(base);
                /* clear the rx fifo drain request, needed for non-DMA applications as this flag
                * will remain set even if the rx fifo is empty. By manually clearing this flag, it
                * either remain clear if no more data is in the fifo, or it will set if there is
                * more data in the fifo.
                */
                DSPI_ClearStatusFlags(base, kDSPI_RxFifoDrainRequestFlag);

                /* Store read bytes into rx buffer only if a buffer pointer was provided */
                if (handle->rxData)
                {
                    *handle->rxData = wordReceived;
                    ++handle->rxData;
                }

                --handle->remainingReceiveByteCount;

                if (handle->remainingReceiveByteCount == 0)
                {
                    break;
                }
            } /* End of RX FIFO drain while loop */
        }
    }

    /* Check write buffer. We always have to send a word in order to keep the transfer
    * moving. So if the caller didn't provide a send buffer, we just send a zero.
    */
    if (handle->remainingSendByteCount)
    {
        DSPI_MasterTransferFillUpTxFifo(base, handle);
    }

    /* Check if we're done with this transfer.*/
    if ((handle->remainingSendByteCount == 0) && (handle->remainingReceiveByteCount == 0))
    {
        /* Complete the transfer and disable the interrupts */
        DSPI_MasterTransferComplete(base, handle);
    }
}

/*Transactional APIs -- Slave*/
void DSPI_SlaveTransferCreateHandle(SPI_Type *base,
                                    dspi_slave_handle_t *handle,
                                    dspi_slave_transfer_callback_t callback,
                                    void *userData)
{
    assert(handle);

    /* Zero the handle. */
    memset(handle, 0, sizeof(*handle));

    g_dspiHandle[DSPI_GetInstance(base)] = handle;

    handle->callback = callback;
    handle->userData = userData;
}

status_t DSPI_SlaveTransferNonBlocking(SPI_Type *base, dspi_slave_handle_t *handle, dspi_transfer_t *transfer)
{
    assert(handle);
    assert(transfer);

    /* If receive length is zero */
    if (transfer->dataSize == 0)
    {
        return kStatus_InvalidArgument;
    }

    /* If both send buffer and receive buffer is null */
    if ((!(transfer->txData)) && (!(transfer->rxData)))
    {
        return kStatus_InvalidArgument;
    }

    /* Check that we're not busy.*/
    if (handle->state == kDSPI_Busy)
    {
        return kStatus_DSPI_Busy;
    }
    handle->state = kDSPI_Busy;

    /* Enable the NVIC for DSPI peripheral. */
    EnableIRQ(s_dspiIRQ[DSPI_GetInstance(base)]);

    /* Store transfer information */
    handle->txData = transfer->txData;
    handle->rxData = transfer->rxData;
    handle->remainingSendByteCount = transfer->dataSize;
    handle->remainingReceiveByteCount = transfer->dataSize;
    handle->totalByteCount = transfer->dataSize;

    handle->errorCount = 0;

    uint8_t whichCtar = (transfer->configFlags & DSPI_SLAVE_CTAR_MASK) >> DSPI_SLAVE_CTAR_SHIFT;
    handle->bitsPerFrame =
        (((base->CTAR_SLAVE[whichCtar]) & SPI_CTAR_SLAVE_FMSZ_MASK) >> SPI_CTAR_SLAVE_FMSZ_SHIFT) + 1;

    DSPI_StopTransfer(base);

    DSPI_FlushFifo(base, true, true);
    DSPI_ClearStatusFlags(base, kDSPI_AllStatusFlag);

    DSPI_StartTransfer(base);

    /* Prepare data to transmit */
    DSPI_SlaveTransferFillUpTxFifo(base, handle);

    s_dspiSlaveIsr = DSPI_SlaveTransferHandleIRQ;

    /* Enable RX FIFO drain request, the slave only use this interrupt */
    DSPI_EnableInterrupts(base, kDSPI_RxFifoDrainRequestInterruptEnable);

    if (handle->rxData)
    {
        /* RX FIFO overflow request enable */
        DSPI_EnableInterrupts(base, kDSPI_RxFifoOverflowInterruptEnable);
    }
    if (handle->txData)
    {
        /* TX FIFO underflow request enable */
        DSPI_EnableInterrupts(base, kDSPI_TxFifoUnderflowInterruptEnable);
    }

    return kStatus_Success;
}

status_t DSPI_SlaveTransferGetCount(SPI_Type *base, dspi_slave_handle_t *handle, size_t *count)
{
    assert(handle);

    if (!count)
    {
        return kStatus_InvalidArgument;
    }

    /* Catch when there is not an active transfer. */
    if (handle->state != kDSPI_Busy)
    {
        *count = 0;
        return kStatus_NoTransferInProgress;
    }

    *count = handle->totalByteCount - handle->remainingReceiveByteCount;
    return kStatus_Success;
}

static void DSPI_SlaveTransferFillUpTxFifo(SPI_Type *base, dspi_slave_handle_t *handle)
{
    assert(handle);

    uint16_t transmitData = 0;
    uint8_t dummyPattern = DSPI_DUMMY_DATA;

    /* Service the transmitter, if transmit buffer provided, transmit the data,
    * else transmit dummy pattern
    */
    while (DSPI_GetStatusFlags(base) & kDSPI_TxFifoFillRequestFlag)
    {
        /* Transmit data */
        if (handle->remainingSendByteCount > 0)
        {
            /* Have data to transmit, update the transmit data and push to FIFO */
            if (handle->bitsPerFrame <= 8)
            {
                /* bits/frame is 1 byte */
                if (handle->txData)
                {
                    /* Update transmit data and transmit pointer */
                    transmitData = *handle->txData;
                    handle->txData++;
                }
                else
                {
                    transmitData = dummyPattern;
                }

                /* Decrease remaining dataSize */
                --handle->remainingSendByteCount;
            }
            /* bits/frame is 2 bytes */
            else
            {
                /* With multibytes per frame transmission, the transmit frame contains data from
                * transmit buffer until sent dataSize matches user request. Other bytes will set to
                * dummy pattern value.
                */
                if (handle->txData)
                {
                    /* Update first byte of transmit data and transmit pointer */
                    transmitData = *handle->txData;
                    handle->txData++;

                    if (handle->remainingSendByteCount == 1)
                    {
                        /* Decrease remaining dataSize */
                        --handle->remainingSendByteCount;
                        /* Update second byte of transmit data to second byte of dummy pattern */
                        transmitData = transmitData | (uint16_t)(((uint16_t)dummyPattern) << 8);
                    }
                    else
                    {
                        /* Update second byte of transmit data and transmit pointer */
                        transmitData = transmitData | (uint16_t)((uint16_t)(*handle->txData) << 8);
                        handle->txData++;
                        handle->remainingSendByteCount -= 2;
                    }
                }
                else
                {
                    if (handle->remainingSendByteCount == 1)
                    {
                        --handle->remainingSendByteCount;
                    }
                    else
                    {
                        handle->remainingSendByteCount -= 2;
                    }
                    transmitData = (uint16_t)((uint16_t)(dummyPattern) << 8) | dummyPattern;
                }
            }
        }
        else
        {
            break;
        }

        /* Write the data to the DSPI data register */
        base->PUSHR_SLAVE = transmitData;

        /* Try to clear TFFF by writing a one to it; it will not clear if TX FIFO not full */
        DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);
    }
}

static void DSPI_SlaveTransferComplete(SPI_Type *base, dspi_slave_handle_t *handle)
{
    assert(handle);

    /* Disable interrupt requests */
    DSPI_DisableInterrupts(base, kDSPI_TxFifoUnderflowInterruptEnable | kDSPI_TxFifoFillRequestInterruptEnable |
                                     kDSPI_RxFifoOverflowInterruptEnable | kDSPI_RxFifoDrainRequestInterruptEnable);

    /* The transfer is complete. */
    handle->txData = NULL;
    handle->rxData = NULL;
    handle->remainingReceiveByteCount = 0;
    handle->remainingSendByteCount = 0;

    status_t status = 0;
    if (handle->state == kDSPI_Error)
    {
        status = kStatus_DSPI_Error;
    }
    else
    {
        status = kStatus_Success;
    }

    handle->state = kDSPI_Idle;

    if (handle->callback)
    {
        handle->callback(base, handle, status, handle->userData);
    }
}

void DSPI_SlaveTransferAbort(SPI_Type *base, dspi_slave_handle_t *handle)
{
    assert(handle);

    DSPI_StopTransfer(base);

    /* Disable interrupt requests */
    DSPI_DisableInterrupts(base, kDSPI_TxFifoUnderflowInterruptEnable | kDSPI_TxFifoFillRequestInterruptEnable |
                                     kDSPI_RxFifoOverflowInterruptEnable | kDSPI_RxFifoDrainRequestInterruptEnable);

    handle->state = kDSPI_Idle;
    handle->remainingSendByteCount = 0;
    handle->remainingReceiveByteCount = 0;
}

void DSPI_SlaveTransferHandleIRQ(SPI_Type *base, dspi_slave_handle_t *handle)
{
    assert(handle);

    uint8_t dummyPattern = DSPI_DUMMY_DATA;
    uint32_t dataReceived;
    uint32_t dataSend = 0;

    /* Because SPI protocol is synchronous, the number of bytes that that slave received from the
    * master is the actual number of bytes that the slave transmitted to the master. So we only
    * monitor the received dataSize to know when the transfer is complete.
    */
    if (handle->remainingReceiveByteCount > 0)
    {
        while (DSPI_GetStatusFlags(base) & kDSPI_RxFifoDrainRequestFlag)
        {
            /* Have received data in the buffer. */
            dataReceived = base->POPR;
            /*Clear the rx fifo drain request, needed for non-DMA applications as this flag
            * will remain set even if the rx fifo is empty. By manually clearing this flag, it
            * either remain clear if no more data is in the fifo, or it will set if there is
            * more data in the fifo.
            */
            DSPI_ClearStatusFlags(base, kDSPI_RxFifoDrainRequestFlag);

            /* If bits/frame is one byte */
            if (handle->bitsPerFrame <= 8)
            {
                if (handle->rxData)
                {
                    /* Receive buffer is not null, store data into it */
                    *handle->rxData = dataReceived;
                    ++handle->rxData;
                }
                /* Descrease remaining receive byte count */
                --handle->remainingReceiveByteCount;

                if (handle->remainingSendByteCount > 0)
                {
                    if (handle->txData)
                    {
                        dataSend = *handle->txData;
                        ++handle->txData;
                    }
                    else
                    {
                        dataSend = dummyPattern;
                    }

                    --handle->remainingSendByteCount;
                    /* Write the data to the DSPI data register */
                    base->PUSHR_SLAVE = dataSend;
                }
            }
            else /* If bits/frame is 2 bytes */
            {
                /* With multibytes frame receiving, we only receive till the received dataSize
                * matches user request. Other bytes will be ignored.
                */
                if (handle->rxData)
                {
                    /* Receive buffer is not null, store first byte into it */
                    *handle->rxData = dataReceived;
                    ++handle->rxData;

                    if (handle->remainingReceiveByteCount == 1)
                    {
                        /* Decrease remaining receive byte count */
                        --handle->remainingReceiveByteCount;
                    }
                    else
                    {
                        /* Receive buffer is not null, store second byte into it */
                        *handle->rxData = dataReceived >> 8;
                        ++handle->rxData;
                        handle->remainingReceiveByteCount -= 2;
                    }
                }
                /* If no handle->rxData*/
                else
                {
                    if (handle->remainingReceiveByteCount == 1)
                    {
                        /* Decrease remaining receive byte count */
                        --handle->remainingReceiveByteCount;
                    }
                    else
                    {
                        handle->remainingReceiveByteCount -= 2;
                    }
                }

                if (handle->remainingSendByteCount > 0)
                {
                    if (handle->txData)
                    {
                        dataSend = *handle->txData;
                        ++handle->txData;

                        if (handle->remainingSendByteCount == 1)
                        {
                            --handle->remainingSendByteCount;
                            dataSend |= (uint16_t)((uint16_t)(dummyPattern) << 8);
                        }
                        else
                        {
                            dataSend |= (uint32_t)(*handle->txData) << 8;
                            ++handle->txData;
                            handle->remainingSendByteCount -= 2;
                        }
                    }
                    /* If no handle->txData*/
                    else
                    {
                        if (handle->remainingSendByteCount == 1)
                        {
                            --handle->remainingSendByteCount;
                        }
                        else
                        {
                            handle->remainingSendByteCount -= 2;
                        }
                        dataSend = (uint16_t)((uint16_t)(dummyPattern) << 8) | dummyPattern;
                    }
                    /* Write the data to the DSPI data register */
                    base->PUSHR_SLAVE = dataSend;
                }
            }
            /* Try to clear TFFF by writing a one to it; it will not clear if TX FIFO not full */
            DSPI_ClearStatusFlags(base, kDSPI_TxFifoFillRequestFlag);

            if (handle->remainingReceiveByteCount == 0)
            {
                break;
            }
        }
    }
    /* Check if remaining receive byte count matches user request */
    if ((handle->remainingReceiveByteCount == 0) || (handle->state == kDSPI_Error))
    {
        /* Other cases, stop the transfer. */
        DSPI_SlaveTransferComplete(base, handle);
        return;
    }

    /* Catch tx fifo underflow conditions, service only if tx under flow interrupt enabled */
    if ((DSPI_GetStatusFlags(base) & kDSPI_TxFifoUnderflowFlag) && (base->RSER & SPI_RSER_TFUF_RE_MASK))
    {
        DSPI_ClearStatusFlags(base, kDSPI_TxFifoUnderflowFlag);
        /* Change state to error and clear flag */
        if (handle->txData)
        {
            handle->state = kDSPI_Error;
        }
        handle->errorCount++;
    }
    /* Catch rx fifo overflow conditions, service only if rx over flow interrupt enabled */
    if ((DSPI_GetStatusFlags(base) & kDSPI_RxFifoOverflowFlag) && (base->RSER & SPI_RSER_RFOF_RE_MASK))
    {
        DSPI_ClearStatusFlags(base, kDSPI_RxFifoOverflowFlag);
        /* Change state to error and clear flag */
        if (handle->txData)
        {
            handle->state = kDSPI_Error;
        }
        handle->errorCount++;
    }
}

static void DSPI_CommonIRQHandler(SPI_Type *base, void *param)
{
    if (DSPI_IsMaster(base))
    {
        s_dspiMasterIsr(base, (dspi_master_handle_t *)param);
    }
    else
    {
        s_dspiSlaveIsr(base, (dspi_slave_handle_t *)param);
    }
}

#if defined(SPI0)
void SPI0_DriverIRQHandler(void)
{
    assert(g_dspiHandle[0]);
    DSPI_CommonIRQHandler(SPI0, g_dspiHandle[0]);
}
#endif

#if defined(SPI1)
void SPI1_DriverIRQHandler(void)
{
    assert(g_dspiHandle[1]);
    DSPI_CommonIRQHandler(SPI1, g_dspiHandle[1]);
}
#endif

#if defined(SPI2)
void SPI2_DriverIRQHandler(void)
{
    assert(g_dspiHandle[2]);
    DSPI_CommonIRQHandler(SPI2, g_dspiHandle[2]);
}
#endif

#if defined(SPI3)
void SPI3_DriverIRQHandler(void)
{
    assert(g_dspiHandle[3]);
    DSPI_CommonIRQHandler(SPI3, g_dspiHandle[3]);
}
#endif

#if defined(SPI4)
void SPI4_DriverIRQHandler(void)
{
    assert(g_dspiHandle[4]);
    DSPI_CommonIRQHandler(SPI4, g_dspiHandle[4]);
}
#endif

#if defined(SPI5)
void SPI5_DriverIRQHandler(void)
{
    assert(g_dspiHandle[5]);
    DSPI_CommonIRQHandler(SPI5, g_dspiHandle[5]);
}
#endif

#if (FSL_FEATURE_SOC_DSPI_COUNT > 6)
#error "Should write the SPIx_DriverIRQHandler function that instance greater than 5 !"
#endif
