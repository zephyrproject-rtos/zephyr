/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
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

#include "fsl_lpsci.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* LPSCI transfer state. */
enum _lpsci_tansfer_state
{
    kLPSCI_TxIdle,         /*!< TX idle. */
    kLPSCI_TxBusy,         /*!< TX busy. */
    kLPSCI_RxIdle,         /*!< RX idle. */
    kLPSCI_RxBusy,         /*!< RX busy. */
    kLPSCI_RxFramingError, /* Rx framing error */
    kLPSCI_RxParityError   /* Rx parity error */
};

/* Typedef for interrupt handler. */
typedef void (*lpsci_isr_t)(UART0_Type *base, lpsci_handle_t *handle);

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*!
 * @brief Get the LPSCI instance from peripheral base address.
 *
 * @param base LPSCI peripheral base address.
 * @return LPSCI instance.
 */
uint32_t LPSCI_GetInstance(UART0_Type *base);

/*!
 * @brief Get the length of received data in RX ring buffer.
 *
 * @userData handle LPSCI handle pointer.
 * @return Length of received data in RX ring buffer.
 */
static size_t LPSCI_TransferGetRxRingBufferLength(lpsci_handle_t *handle);

/*!
 * @brief Check whether the RX ring buffer is full.
 *
 * @parram handle LPSCI handle pointer.
 * @retval true  RX ring buffer is full.
 * @retval false RX ring buffer is not full.
 */
static bool LPSCI_TransferIsRxRingBufferFull(lpsci_handle_t *handle);

/*!
 * @brief Write to TX register using non-blocking method.
 *
 * This function writes data to the TX register directly, upper layer must make
 * sure the TX register is empty before calling this function.
 *
 * @note This function does not check whether all the data has been sent out to bus,
 * so before disable TX, check kLPSCI_TransmissionCompleteFlag to ensure the TX is
 * finished.
 *
 * @param base LPSCI peripheral base address.
 * @param data Start addresss of the data to write.
 * @param length Size of the buffer to be sent.
 */
static void LPSCI_WriteNonBlocking(UART0_Type *base, const uint8_t *data, size_t length);

/*!
 * @brief Read RX data register using blocking method.
 *
 * This function polls the RX register, waits for the RX register full
 * then read data from TX register.
 *
 * @param base LPSCI peripheral base address.
 * @param data Start addresss of the buffer to store the received data.
 * @param length Size of the buffer.
 */
static void LPSCI_ReadNonBlocking(UART0_Type *base, uint8_t *data, size_t length);

/*******************************************************************************
 * Variables
 ******************************************************************************/
/* Array of LPSCI handle. */
static lpsci_handle_t *s_lpsciHandle[FSL_FEATURE_SOC_LPSCI_COUNT];

/* Array of LPSCI peripheral base address. */
static UART0_Type *const s_lpsciBases[] = UART0_BASE_PTRS;

/* Array of LPSCI IRQ number. */
static const IRQn_Type s_lpsciIRQ[] = UART0_RX_TX_IRQS;
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of LPSCI clock name. */
static const clock_ip_name_t s_lpsciClock[] = UART0_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

/* LPSCI ISR for transactional APIs. */
static lpsci_isr_t s_lpsciIsr;

/*******************************************************************************
 * Code
 ******************************************************************************/

uint32_t LPSCI_GetInstance(UART0_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_lpsciBases); instance++)
    {
        if (s_lpsciBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_lpsciBases));

    return instance;
}

static size_t LPSCI_TransferGetRxRingBufferLength(lpsci_handle_t *handle)
{
    assert(handle);

    size_t size;

    if (handle->rxRingBufferTail > handle->rxRingBufferHead)
    {
        size = (size_t)(handle->rxRingBufferHead + handle->rxRingBufferSize - handle->rxRingBufferTail);
    }
    else
    {
        size = (size_t)(handle->rxRingBufferHead - handle->rxRingBufferTail);
    }

    return size;
}

static bool LPSCI_TransferIsRxRingBufferFull(lpsci_handle_t *handle)
{
    assert(handle);

    bool full;

    if (LPSCI_TransferGetRxRingBufferLength(handle) == (handle->rxRingBufferSize - 1U))
    {
        full = true;
    }
    else
    {
        full = false;
    }

    return full;
}

static void LPSCI_ReadNonBlocking(UART0_Type *base, uint8_t *data, size_t length)
{
    assert(data);

    /* The Non Blocking read data API assume user have ensured there is enough space in
    peripheral to write. */
    size_t i;

    for (i = 0; i < length; i++)
    {
        data[i] = base->D;
    }
}

static void LPSCI_WriteNonBlocking(UART0_Type *base, const uint8_t *data, size_t length)
{
    assert(data);

    /* The Non Blocking write data API assume user have ensured there is enough space in
    peripheral to write. */
    size_t i;

    for (i = 0; i < length; i++)
    {
        base->D = data[i];
    }
}

status_t LPSCI_Init(UART0_Type *base, const lpsci_config_t *config, uint32_t srcClock_Hz)
{
    assert(config);
    assert(config->baudRate_Bps);

    uint8_t temp;
    uint16_t sbr = 0;
    uint16_t sbrTemp;
    uint32_t osr = 0;
    uint32_t osrTemp;
    uint32_t tempDiff, calculatedBaud, baudDiff;

    /* This LPSCI instantiation uses a slightly different baud rate calculation
     * The idea is to use the best OSR (over-sampling rate) possible
     * Note, OSR is typically hard-set to 16 in other LPSCI instantiations
     * loop to find the best OSR value possible, one that generates minimum baudDiff
     * iterate through the rest of the supported values of OSR */

    baudDiff = config->baudRate_Bps;
    for (osrTemp = 4; osrTemp <= 32; osrTemp++)
    {
        /* calculate the temporary sbr value   */
        sbrTemp = (srcClock_Hz / (config->baudRate_Bps * osrTemp));
        /* set sbrTemp to 1 if the sourceClockInHz can not satisfy the desired baud rate */
        if (sbrTemp == 0)
        {
            sbrTemp = 1;
        }
        /* Calculate the baud rate based on the temporary OSR and SBR values */
        calculatedBaud = (srcClock_Hz / (osrTemp * sbrTemp));

        tempDiff = calculatedBaud - config->baudRate_Bps;

        /* Select the better value between srb and (sbr + 1) */
        if (tempDiff > (config->baudRate_Bps - (srcClock_Hz / (osrTemp * (sbrTemp + 1)))))
        {
            tempDiff = config->baudRate_Bps - (srcClock_Hz / (osrTemp * (sbrTemp + 1)));
            sbrTemp++;
        }

        if (tempDiff <= baudDiff)
        {
            baudDiff = tempDiff;
            osr = osrTemp; /* update and store the best OSR value calculated*/
            sbr = sbrTemp; /* update store the best SBR value calculated*/
        }
    }

    /* next, check to see if actual baud rate is within 3% of desired baud rate
     * based on the best calculate OSR value */
    if (baudDiff > ((config->baudRate_Bps / 100) * 3))
    {
        /* Unacceptable baud rate difference of more than 3%*/
        return kStatus_LPSCI_BaudrateNotSupport;
    }

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable LPSCI clock */
    CLOCK_EnableClock(s_lpsciClock[LPSCI_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Disable TX RX before setting. */
    base->C2 &= ~(UART0_C2_TE_MASK | UART0_C2_RE_MASK);

    /* Acceptable baud rate */
    /* Check if OSR is between 4x and 7x oversampling*/
    /* If so, then "BOTHEDGE" sampling must be turned on*/
    if ((osr > 3) && (osr < 8))
    {
        base->C5 |= UART0_C5_BOTHEDGE_MASK;
    }

    /* program the osr value (bit value is one less than actual value)*/
    base->C4 = ((base->C4 & ~UART0_C4_OSR_MASK) | (osr - 1));

    /* program the sbr (divider) value obtained above*/
    base->BDH = ((base->C4 & ~UART0_BDH_SBR_MASK) | (uint8_t)(sbr >> 8));
    base->BDL = (uint8_t)sbr;

    /* set parity mode */
    temp = base->C1 & ~(UART0_C1_PE_MASK | UART0_C1_PT_MASK | UART0_C1_M_MASK);

    if (kLPSCI_ParityDisabled != config->parityMode)
    {
        temp |= (uint8_t)config->parityMode | UART0_C1_M_MASK;
    }

    base->C1 = temp;

#if defined(FSL_FEATURE_LPSCI_HAS_STOP_BIT_CONFIG_SUPPORT) && FSL_FEATURE_LPSCI_HAS_STOP_BIT_CONFIG_SUPPORT
    /* set stop bit per char */
    base->BDH &= ~UART0_BDH_SBNS_MASK;
    base->BDH |= UART0_BDH_SBNS((uint8_t)config->stopBitCount);
#endif

    /* Enable TX/RX base on configure structure. */
    temp = base->C2;

    if (config->enableTx)
    {
        temp |= UART0_C2_TE_MASK;
    }

    if (config->enableRx)
    {
        temp |= UART0_C2_RE_MASK;
    }

    base->C2 = temp;

    return kStatus_Success;
}

void LPSCI_Deinit(UART0_Type *base)
{
    /* Wait last char out */
    while (0 == (base->S1 & UART0_S1_TC_MASK))
    {
    }

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Disable LPSCI clock */
    CLOCK_DisableClock(s_lpsciClock[LPSCI_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}

void LPSCI_GetDefaultConfig(lpsci_config_t *config)
{
    assert(config);

    config->baudRate_Bps = 115200U;
    config->parityMode = kLPSCI_ParityDisabled;
    config->stopBitCount = kLPSCI_OneStopBit;
    config->enableTx = false;
    config->enableRx = false;
}

status_t LPSCI_SetBaudRate(UART0_Type *base, uint32_t baudRate_Bps, uint32_t srcClock_Hz)
{
    assert(baudRate_Bps);

    uint16_t sbrTemp;
    uint32_t osr = 0, sbr = 0;
    uint8_t osrTemp;
    uint32_t tempDiff, calculatedBaud, baudDiff;
    uint8_t oldCtrl;

    /* This LPSCI instantiation uses a slightly different baud rate calculation
     * The idea is to use the best OSR (over-sampling rate) possible
     * Note, OSR is typically hard-set to 16 in other LPSCI instantiations
     * First calculate the baud rate using the minimum OSR possible (4). */
    baudDiff = baudRate_Bps;
    for (osrTemp = 4; osrTemp <= 32; osrTemp++)
    {
        /* calculate the temporary sbr value   */
        sbrTemp = (srcClock_Hz / (baudRate_Bps * osrTemp));
        /* set sbrTemp to 1 if the sourceClockInHz can not satisfy the desired baud rate */
        if (sbrTemp == 0)
        {
            sbrTemp = 1;
        }
        /* Calculate the baud rate based on the temporary OSR and SBR values */
        calculatedBaud = (srcClock_Hz / (osrTemp * sbrTemp));

        tempDiff = calculatedBaud - baudRate_Bps;

        /* Select the better value between srb and (sbr + 1) */
        if (tempDiff > (baudRate_Bps - (srcClock_Hz / (osrTemp * (sbrTemp + 1)))))
        {
            tempDiff = baudRate_Bps - (srcClock_Hz / (osrTemp * (sbrTemp + 1)));
            sbrTemp++;
        }

        if (tempDiff <= baudDiff)
        {
            baudDiff = tempDiff;
            osr = osrTemp; /* update and store the best OSR value calculated*/
            sbr = sbrTemp; /* update store the best SBR value calculated*/
        }
    }

    /* next, check to see if actual baud rate is within 3% of desired baud rate
     * based on the best calculate OSR value */
    if (baudDiff < ((baudRate_Bps / 100) * 3))
    {
        /* Store C2 before disable Tx and Rx */
        oldCtrl = base->C2;

        /* Disable LPSCI TX RX before setting. */
        base->C2 &= ~(UART0_C2_TE_MASK | UART0_C2_RE_MASK);

        /* Acceptable baud rate */
        /* Check if OSR is between 4x and 7x oversampling*/
        /* If so, then "BOTHEDGE" sampling must be turned on*/
        if ((osr > 3) && (osr < 8))
        {
            base->C5 |= UART0_C5_BOTHEDGE_MASK;
        }

        /* program the osr value (bit value is one less than actual value)*/
        base->C4 = ((base->C4 & ~UART0_C4_OSR_MASK) | (osr - 1));

        /* program the sbr (divider) value obtained above*/
        base->BDH = ((base->C4 & ~UART0_BDH_SBR_MASK) | (uint8_t)(sbr >> 8));
        base->BDL = (uint8_t)sbr;

        /* Restore C2. */
        base->C2 = oldCtrl;

        return kStatus_Success;
    }
    else
    {
        /* Unacceptable baud rate difference of more than 3%*/
        return kStatus_LPSCI_BaudrateNotSupport;
    }
}

void LPSCI_EnableInterrupts(UART0_Type *base, uint32_t mask)
{
    mask &= kLPSCI_AllInterruptsEnable;

    /* The interrupt mask is combined by control bits from several register: ((C3<<16) | (C2<<8) |(BDH))
     */
    base->BDH |= mask;
    base->C2 |= (mask >> 8);
    base->C3 |= (mask >> 16);
}

void LPSCI_DisableInterrupts(UART0_Type *base, uint32_t mask)
{
    mask &= kLPSCI_AllInterruptsEnable;

    /* The interrupt mask is combined by control bits from several register: ((C3<<16) | (C2<<8) |(BDH))
     */
    base->BDH &= ~mask;
    base->C2 &= ~(mask >> 8);
    base->C3 &= ~(mask >> 16);
}

uint32_t LPSCI_GetEnabledInterrupts(UART0_Type *base)
{
    uint32_t temp;
    temp = base->BDH | ((uint32_t)(base->C2) << 8) | ((uint32_t)(base->C3) << 16);

    return temp & kLPSCI_AllInterruptsEnable;
}

uint32_t LPSCI_GetStatusFlags(UART0_Type *base)
{
    uint32_t status_flag;
    status_flag = base->S1 | ((uint32_t)(base->S2) << 8);

#if defined(FSL_FEATURE_LPSCI_HAS_EXTENDED_DATA_REGISTER_FLAGS) && FSL_FEATURE_LPSCI_HAS_EXTENDED_DATA_REGISTER_FLAGS
    status_flag |= ((uint32_t)(base->ED) << 16);
#endif

    return status_flag;
}

status_t LPSCI_ClearStatusFlags(UART0_Type *base, uint32_t mask)
{
    volatile uint8_t dummy = 0;
    status_t status;
    dummy++; /* For unused variable warning */

#if defined(FSL_FEATURE_LPSCI_HAS_LIN_BREAK_DETECT) && FSL_FEATURE_LPSCI_HAS_LIN_BREAK_DETECT
    if (mask & kLPSCI_LinBreakFlag)
    {
        base->S2 = UART0_S2_LBKDIF_MASK;
        mask &= ~(uint32_t)kLPSCI_LinBreakFlag;
    }
#endif

    if (mask & kLPSCI_RxActiveEdgeFlag)
    {
        base->S2 = UART0_S2_RXEDGIF_MASK;
        mask &= ~(uint32_t)kLPSCI_RxActiveEdgeFlag;
    }

    if ((mask & (kLPSCI_IdleLineFlag | kLPSCI_RxOverrunFlag | kLPSCI_NoiseErrorFlag | kLPSCI_FramingErrorFlag |
                 kLPSCI_ParityErrorFlag)))
    {
        base->S1 = (mask & (kLPSCI_IdleLineFlag | kLPSCI_RxOverrunFlag | kLPSCI_NoiseErrorFlag |
                            kLPSCI_FramingErrorFlag | kLPSCI_ParityErrorFlag));
        mask &= ~(uint32_t)(kLPSCI_IdleLineFlag | kLPSCI_RxOverrunFlag | kLPSCI_NoiseErrorFlag |
                            kLPSCI_FramingErrorFlag | kLPSCI_ParityErrorFlag);
    }

    if (mask)
    {
        /* Some flags can only clear or set by the hardware itself, these flags are: kLPSCI_TxDataRegEmptyFlag,
        kLPSCI_TransmissionCompleteFlag, kLPSCI_RxDataRegFullFlag, kLPSCI_RxActiveFlag,
        kLPSCI_NoiseErrorInRxDataRegFlag,
        kLPSCI_ParityErrorInRxDataRegFlag*/

        status = kStatus_LPSCI_FlagCannotClearManually;
    }
    else
    {
        status = kStatus_Success;
    }

    return status;
}

void LPSCI_WriteBlocking(UART0_Type *base, const uint8_t *data, size_t length)
{
    assert(data);

    /* This API can only ensure that the data is written into the data buffer but can't
    ensure all data in the data buffer are sent into the transmit shift buffer. */
    while (length--)
    {
        while (!(base->S1 & UART0_S1_TDRE_MASK))
        {
        }
        base->D = *(data++);
    }
}

status_t LPSCI_ReadBlocking(UART0_Type *base, uint8_t *data, size_t length)
{
    assert(data);

    uint32_t statusFlag;

    while (length--)
    {
        while (!(base->S1 & UART0_S1_RDRF_MASK))
        {
            statusFlag = LPSCI_GetStatusFlags(base);

            if (statusFlag & kLPSCI_RxOverrunFlag)
            {
                LPSCI_ClearStatusFlags(base, kLPSCI_RxOverrunFlag);
                return kStatus_LPSCI_RxHardwareOverrun;
            }

            if (statusFlag & kLPSCI_NoiseErrorFlag)
            {
                LPSCI_ClearStatusFlags(base, kLPSCI_NoiseErrorFlag);
                return kStatus_LPSCI_NoiseError;
            }

            if (statusFlag & kLPSCI_FramingErrorFlag)
            {
                LPSCI_ClearStatusFlags(base, kLPSCI_FramingErrorFlag);
                return kStatus_LPSCI_FramingError;
            }

            if (statusFlag & kLPSCI_ParityErrorFlag)
            {
                LPSCI_ClearStatusFlags(base, kLPSCI_ParityErrorFlag);
                return kStatus_LPSCI_ParityError;
            }
        }
        *(data++) = base->D;
    }

    return kStatus_Success;
}

void LPSCI_TransferCreateHandle(UART0_Type *base,
                                lpsci_handle_t *handle,
                                lpsci_transfer_callback_t callback,
                                void *userData)
{
    assert(handle);

    uint32_t instance;

    /* Zero the handle. */
    memset(handle, 0, sizeof(lpsci_handle_t));

    /* Set the TX/RX state. */
    handle->rxState = kLPSCI_RxIdle;
    handle->txState = kLPSCI_TxIdle;

    /* Set the callback and user data. */
    handle->callback = callback;
    handle->userData = userData;

    /* Get instance from peripheral base address. */
    instance = LPSCI_GetInstance(base);

    /* Save the handle in global variables to support the double weak mechanism. */
    s_lpsciHandle[instance] = handle;

    s_lpsciIsr = LPSCI_TransferHandleIRQ;

    /* Enable interrupt in NVIC. */
    EnableIRQ(s_lpsciIRQ[instance]);
}

void LPSCI_TransferStartRingBuffer(UART0_Type *base, lpsci_handle_t *handle, uint8_t *ringBuffer, size_t ringBufferSize)
{
    assert(handle);
    assert(ringBuffer);

    /* Setup the ringbuffer address */
    handle->rxRingBuffer = ringBuffer;
    handle->rxRingBufferSize = ringBufferSize;
    handle->rxRingBufferHead = 0U;
    handle->rxRingBufferTail = 0U;

    /* Enable the interrupt to accept the data when user need the ring buffer. */
    LPSCI_EnableInterrupts(base, kLPSCI_RxDataRegFullInterruptEnable | kLPSCI_RxOverrunInterruptEnable |
                                     kLPSCI_FramingErrorInterruptEnable);
    /* Enable parity error interrupt when parity mode is enable*/
    if (UART0_C1_PE_MASK & base->C1)
    {
        LPSCI_EnableInterrupts(base, kLPSCI_ParityErrorInterruptEnable);
    }
}

void LPSCI_TransferStopRingBuffer(UART0_Type *base, lpsci_handle_t *handle)
{
    assert(handle);

    if (handle->rxState == kLPSCI_RxIdle)
    {
        LPSCI_DisableInterrupts(base, kLPSCI_RxDataRegFullInterruptEnable | kLPSCI_RxOverrunInterruptEnable |
                                          kLPSCI_FramingErrorInterruptEnable);

        /* Disable parity error interrupt when parity mode is enable*/
        if (UART0_C1_PE_MASK & base->C1)
        {
            LPSCI_DisableInterrupts(base, kLPSCI_ParityErrorInterruptEnable);
        }
    }

    handle->rxRingBuffer = NULL;
    handle->rxRingBufferSize = 0U;
    handle->rxRingBufferHead = 0U;
    handle->rxRingBufferTail = 0U;
}

status_t LPSCI_TransferSendNonBlocking(UART0_Type *base, lpsci_handle_t *handle, lpsci_transfer_t *xfer)
{
    assert(handle);
    assert(xfer);
    assert(xfer->dataSize);
    assert(xfer->data);

    status_t status;

    /* Return error if current TX busy. */
    if (kLPSCI_TxBusy == handle->txState)
    {
        status = kStatus_LPSCI_TxBusy;
    }
    else
    {
        handle->txData = xfer->data;
        handle->txDataSize = xfer->dataSize;
        handle->txDataSizeAll = xfer->dataSize;
        handle->txState = kLPSCI_TxBusy;

        /* Enable transmiter interrupt. */
        LPSCI_EnableInterrupts(base, kLPSCI_TxDataRegEmptyInterruptEnable);

        status = kStatus_Success;
    }

    return status;
}

void LPSCI_TransferAbortSend(UART0_Type *base, lpsci_handle_t *handle)
{
    assert(handle);

    LPSCI_DisableInterrupts(base, kLPSCI_TxDataRegEmptyInterruptEnable | kLPSCI_TransmissionCompleteInterruptEnable);

    handle->txDataSize = 0;
    handle->txState = kLPSCI_TxIdle;
}

status_t LPSCI_TransferGetSendCount(UART0_Type *base, lpsci_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(count);

    if (kLPSCI_TxIdle == handle->txState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->txDataSizeAll - handle->txDataSize;

    return kStatus_Success;
}

status_t LPSCI_TransferReceiveNonBlocking(UART0_Type *base,
                                          lpsci_handle_t *handle,
                                          lpsci_transfer_t *xfer,
                                          size_t *receivedBytes)
{
    assert(handle);
    assert(xfer);
    assert(xfer->dataSize);
    assert(xfer->data);

    uint32_t i;
    status_t status;
    /* How many bytes to copy from ring buffer to user memory. */
    size_t bytesToCopy = 0U;
    /* How many bytes to receive. */
    size_t bytesToReceive;
    /* How many bytes currently have received. */
    size_t bytesCurrentReceived;

    /* How to get data:
       1. If RX ring buffer is not enabled, then save xfer->data and xfer->dataSize
          to lpsci handle, enable interrupt to store received data to xfer->data. When
          all data received, trigger callback.
       2. If RX ring buffer is enabled and not empty, get data from ring buffer first.
          If there are enough data in ring buffer, copy them to xfer->data and return.
          If there are not enough data in ring buffer, copy all of them to xfer->data,
          save the xfer->data remained empty space to lpsci handle, receive data
          to this empty space and trigger callback when finished. */

    if (kLPSCI_RxBusy == handle->rxState)
    {
        status = kStatus_LPSCI_RxBusy;
    }
    else
    {
        bytesToReceive = xfer->dataSize;
        bytesCurrentReceived = 0U;

        /* If RX ring buffer is used. */
        if (handle->rxRingBuffer)
        {
            /* Disable LPSCI RX IRQ, protect ring buffer. */
            LPSCI_DisableInterrupts(base, kLPSCI_RxDataRegFullInterruptEnable);

            /* How many bytes in RX ring buffer currently. */
            bytesToCopy = LPSCI_TransferGetRxRingBufferLength(handle);

            if (bytesToCopy)
            {
                bytesToCopy = MIN(bytesToReceive, bytesToCopy);

                bytesToReceive -= bytesToCopy;

                /* Copy data from ring buffer to user memory. */
                for (i = 0U; i < bytesToCopy; i++)
                {
                    xfer->data[bytesCurrentReceived++] = handle->rxRingBuffer[handle->rxRingBufferTail];

                    /* Wrap to 0. Not use modulo (%) because it might be large and slow. */
                    if (handle->rxRingBufferTail + 1U == handle->rxRingBufferSize)
                    {
                        handle->rxRingBufferTail = 0U;
                    }
                    else
                    {
                        handle->rxRingBufferTail++;
                    }
                }
            }

            /* If ring buffer does not have enough data, still need to read more data. */
            if (bytesToReceive)
            {
                /* No data in ring buffer, save the request to lpsci handle. */
                handle->rxData = xfer->data + bytesCurrentReceived;
                handle->rxDataSize = bytesToReceive;
                handle->rxDataSizeAll = bytesToReceive;
                handle->rxState = kLPSCI_RxBusy;
            }

            /* Enable LPSCI RX IRQ if previously enabled. */
            LPSCI_EnableInterrupts(base, kLPSCI_RxDataRegFullInterruptEnable);

            /* Call user callback since all data are received. */
            if (0 == bytesToReceive)
            {
                if (handle->callback)
                {
                    handle->callback(base, handle, kStatus_LPSCI_RxIdle, handle->userData);
                }
            }
        }
        /* Ring buffer not used. */
        else
        {
            handle->rxData = xfer->data + bytesCurrentReceived;
            handle->rxDataSize = bytesToReceive;
            handle->rxDataSizeAll = bytesToReceive;
            handle->rxState = kLPSCI_RxBusy;

            /* Enable RX interrupt. */
            LPSCI_EnableInterrupts(base, kLPSCI_RxDataRegFullInterruptEnable | kLPSCI_RxOverrunInterruptEnable |
                                             kLPSCI_FramingErrorInterruptEnable);
            /* Enable parity error interrupt when parity mode is enable*/
            if (UART0_C1_PE_MASK & base->C1)
            {
                LPSCI_EnableInterrupts(base, kLPSCI_ParityErrorInterruptEnable);
            }
        }

        /* Return the how many bytes have read. */
        if (receivedBytes)
        {
            *receivedBytes = bytesCurrentReceived;
        }

        status = kStatus_Success;
    }

    return status;
}

void LPSCI_TransferAbortReceive(UART0_Type *base, lpsci_handle_t *handle)
{
    assert(handle);

    /* Only abort the receive to handle->rxData, the RX ring buffer is still working. */
    if (!handle->rxRingBuffer)
    {
        /* Disable RX interrupt. */
        LPSCI_DisableInterrupts(base, kLPSCI_RxDataRegFullInterruptEnable | kLPSCI_RxOverrunInterruptEnable |
                                          kLPSCI_FramingErrorInterruptEnable);
        /* Disable parity error interrupt when parity mode is enable*/
        if (UART0_C1_PE_MASK & base->C1)
        {
            LPSCI_DisableInterrupts(base, kLPSCI_ParityErrorInterruptEnable);
        }
    }

    handle->rxDataSize = 0U;
    handle->rxState = kLPSCI_RxIdle;
}

status_t LPSCI_TransferGetReceiveCount(UART0_Type *base, lpsci_handle_t *handle, uint32_t *count)
{
    assert(handle);
    assert(count);

    if (kLPSCI_RxIdle == handle->rxState)
    {
        return kStatus_NoTransferInProgress;
    }

    *count = handle->rxDataSizeAll - handle->rxDataSize;

    return kStatus_Success;
}

void LPSCI_TransferHandleIRQ(UART0_Type *base, lpsci_handle_t *handle)
{
    assert(handle);

    uint8_t count;
    uint8_t tempCount;

    /* If RX parity error */
    if (UART0_S1_PF_MASK & base->S1)
    {
        handle->rxState = kLPSCI_RxParityError;

        LPSCI_ClearStatusFlags(base, kLPSCI_ParityErrorFlag);
        /* Trigger callback. */
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_LPSCI_ParityError, handle->userData);
        }
    }

    /* If RX framing error */
    if (UART0_S1_FE_MASK & base->S1)
    {
        handle->rxState = kLPSCI_RxFramingError;

        LPSCI_ClearStatusFlags(base, kLPSCI_FramingErrorFlag);
        /* Trigger callback. */
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_LPSCI_FramingError, handle->userData);
        }
    }
    /* If RX overrun. */
    if (UART0_S1_OR_MASK & base->S1)
    {
        while (UART0_S1_RDRF_MASK & base->S1)
        {
            (void)base->D;
        }

        LPSCI_ClearStatusFlags(base, kLPSCI_RxOverrunFlag);
        /* Trigger callback. */
        if (handle->callback)
        {
            handle->callback(base, handle, kStatus_LPSCI_RxHardwareOverrun, handle->userData);
        }
    }

    /* Receive data register full */
    if ((UART0_S1_RDRF_MASK & base->S1) && (UART0_C2_RIE_MASK & base->C2))
    {
/* Get the size that can be stored into buffer for this interrupt. */
#if defined(FSL_FEATURE_LPSCI_HAS_FIFO) && FSL_FEATURE_LPSCI_HAS_FIFO
        count = base->RCFIFO;
#else
        count = 1;
#endif

        /* If handle->rxDataSize is not 0, first save data to handle->rxData. */
        while ((count) && (handle->rxDataSize))
        {
#if defined(FSL_FEATURE_LPSCI_HAS_FIFO) && FSL_FEATURE_LPSCI_HAS_FIFO
            tempCount = MIN(handle->rxDataSize, count);
#else
            tempCount = 1;
#endif

            /* Using non block API to read the data from the registers. */
            LPSCI_ReadNonBlocking(base, handle->rxData, tempCount);
            handle->rxData += tempCount;
            handle->rxDataSize -= tempCount;
            count -= tempCount;

            /* If all the data required for upper layer is ready, trigger callback. */
            if (!handle->rxDataSize)
            {
                handle->rxState = kLPSCI_RxIdle;

                if (handle->callback)
                {
                    handle->callback(base, handle, kStatus_LPSCI_RxIdle, handle->userData);
                }
            }
        }

        /* If use RX ring buffer, receive data to ring buffer. */
        if (handle->rxRingBuffer)
        {
            while (count--)
            {
                /* If RX ring buffer is full, trigger callback to notify over run. */
                if (LPSCI_TransferIsRxRingBufferFull(handle))
                {
                    if (handle->callback)
                    {
                        handle->callback(base, handle, kStatus_LPSCI_RxRingBufferOverrun, handle->userData);
                    }
                }

                /* If ring buffer is still full after callback function, the oldest data is overrided. */
                if (LPSCI_TransferIsRxRingBufferFull(handle))
                {
                    /* Increase handle->rxRingBufferTail to make room for new data. */
                    if (handle->rxRingBufferTail + 1U == handle->rxRingBufferSize)
                    {
                        handle->rxRingBufferTail = 0U;
                    }
                    else
                    {
                        handle->rxRingBufferTail++;
                    }
                }

                /* Read data. */
                handle->rxRingBuffer[handle->rxRingBufferHead] = base->D;

                /* Increase handle->rxRingBufferHead. */
                if (handle->rxRingBufferHead + 1U == handle->rxRingBufferSize)
                {
                    handle->rxRingBufferHead = 0U;
                }
                else
                {
                    handle->rxRingBufferHead++;
                }
            }
        }
        /* If no receive requst pending, stop RX interrupt. */
        else if (!handle->rxDataSize)
        {
            LPSCI_DisableInterrupts(base, kLPSCI_RxDataRegFullInterruptEnable | kLPSCI_RxOverrunInterruptEnable |
                                              kLPSCI_FramingErrorInterruptEnable);

            /* Disable parity error interrupt when parity mode is enable*/
            if (UART0_C1_PE_MASK & base->C1)
            {
                LPSCI_DisableInterrupts(base, kLPSCI_ParityErrorInterruptEnable);
            }
        }
        else
        {
        }
    }
    /* If framing error or parity error happened, stop the RX interrupt when ues no ring buffer */
    if (((handle->rxState == kLPSCI_RxFramingError) || (handle->rxState == kLPSCI_RxParityError)) &&
        (!handle->rxRingBuffer))
    {
        LPSCI_DisableInterrupts(base, kLPSCI_RxDataRegFullInterruptEnable | kLPSCI_RxOverrunInterruptEnable |
                                          kLPSCI_FramingErrorInterruptEnable);

        /* Disable parity error interrupt when parity mode is enable*/
        if (UART0_C1_PE_MASK & base->C1)
        {
            LPSCI_DisableInterrupts(base, kLPSCI_ParityErrorInterruptEnable);
        }
    }

    /* Send data register empty and the interrupt is enabled. */
    if ((base->S1 & UART0_S1_TDRE_MASK) && (base->C2 & UART0_C2_TIE_MASK))
    {
/* Get the bytes that available at this moment. */
#if defined(FSL_FEATURE_LPSCI_HAS_FIFO) && FSL_FEATURE_LPSCI_HAS_FIFO
        count = FSL_FEATURE_LPSCI_FIFO_SIZEn(base) - base->TCFIFO;
#else
        count = 1;
#endif

        while ((count) && (handle->txDataSize))
        {
#if defined(FSL_FEATURE_LPSCI_HAS_FIFO) && FSL_FEATURE_LPSCI_HAS_FIFO
            tempCount = MIN(handle->txDataSize, count);
#else
            tempCount = 1;
#endif

            /* Using non block API to write the data to the registers. */
            LPSCI_WriteNonBlocking(base, handle->txData, tempCount);
            handle->txData += tempCount;
            handle->txDataSize -= tempCount;
            count -= tempCount;

            /* If all the data are written to data register, enable TX complete interrupt. */
            if (!handle->txDataSize)
            {
                handle->txState = kLPSCI_TxIdle;

                /* Disable TX register empty interrupt. */
                base->C2 &= ~UART0_C2_TIE_MASK;

                /* Trigger callback. */
                if (handle->callback)
                {
                    handle->callback(base, handle, kStatus_LPSCI_TxIdle, handle->userData);
                }
            }
        }
    }
}

void LPSCI_TransferHandleErrorIRQ(UART0_Type *base, lpsci_handle_t *handle)
{
    /* To be implemented by User. */
}

#if defined(UART0)
void UART0_DriverIRQHandler(void)
{
    s_lpsciIsr(UART0, s_lpsciHandle[0]);
}

#endif
