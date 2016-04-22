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
#ifndef _FSL_UART_H_
#define _FSL_UART_H_

#include "fsl_common.h"

/*!
 * @addtogroup uart_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief UART driver version 2.1.0. */
#define FSL_UART_DRIVER_VERSION (MAKE_VERSION(2, 1, 0))
/*@}*/

/*! @brief Error codes for the UART driver. */
enum _uart_status
{
    kStatus_UART_TxBusy = MAKE_STATUS(kStatusGroup_UART, 0),              /*!< Transmitter is busy. */
    kStatus_UART_RxBusy = MAKE_STATUS(kStatusGroup_UART, 1),              /*!< Receiver is busy. */
    kStatus_UART_TxIdle = MAKE_STATUS(kStatusGroup_UART, 2),              /*!< UART transmitter is idle. */
    kStatus_UART_RxIdle = MAKE_STATUS(kStatusGroup_UART, 3),              /*!< UART receiver is idle. */
    kStatus_UART_TxWatermarkTooLarge = MAKE_STATUS(kStatusGroup_UART, 4), /*!< TX FIFO watermark too large  */
    kStatus_UART_RxWatermarkTooLarge = MAKE_STATUS(kStatusGroup_UART, 5), /*!< RX FIFO watermark too large  */
    kStatus_UART_FlagCannotClearManually =
        MAKE_STATUS(kStatusGroup_UART, 6),                                /*!< UART flag can't be manually cleared. */
    kStatus_UART_Error = MAKE_STATUS(kStatusGroup_UART, 7),               /*!< Error happens on UART. */
    kStatus_UART_RxRingBufferOverrun = MAKE_STATUS(kStatusGroup_UART, 8), /*!< UART RX software ring buffer overrun. */
    kStatus_UART_RxHardwareOverrun = MAKE_STATUS(kStatusGroup_UART, 9),   /*!< UART RX receiver overrun. */
    kStatus_UART_NoiseError = MAKE_STATUS(kStatusGroup_UART, 10),         /*!< UART noise error. */
    kStatus_UART_FramingError = MAKE_STATUS(kStatusGroup_UART, 11),       /*!< UART framing error. */
    kStatus_UART_ParityError = MAKE_STATUS(kStatusGroup_UART, 12),        /*!< UART parity error. */
};

/*! @brief UART parity mode. */
typedef enum _uart_parity_mode
{
    kUART_ParityDisabled = 0x0U, /*!< Parity disabled */
    kUART_ParityEven = 0x2U,     /*!< Parity enabled, type even, bit setting: PE|PT = 10 */
    kUART_ParityOdd = 0x3U,      /*!< Parity enabled, type odd,  bit setting: PE|PT = 11 */
} uart_parity_mode_t;

/*! @brief UART stop bit count. */
typedef enum _uart_stop_bit_count
{
    kUART_OneStopBit = 0U, /*!< One stop bit */
    kUART_TwoStopBit = 1U, /*!< Two stop bits */
} uart_stop_bit_count_t;

/*!
 * @brief UART interrupt configuration structure, default settings all disabled.
 *
 * This structure contains the settings for all of the UART interrupt configurations.
 */
enum _uart_interrupt_enable
{
#if defined(FSL_FEATURE_UART_HAS_LIN_BREAK_DETECT) && FSL_FEATURE_UART_HAS_LIN_BREAK_DETECT
    kUART_LinBreakInterruptEnable = (UART_BDH_LBKDIE_MASK), /*!< LIN break detect interrupt. */
#endif
    kUART_RxActiveEdgeInterruptEnable = (UART_BDH_RXEDGIE_MASK),   /*!< RX active edge interrupt. */
    kUART_TxDataRegEmptyInterruptEnable = (UART_C2_TIE_MASK << 8), /*!< Transmit data register empty interrupt. */
    kUART_TransmissionCompleteInterruptEnable = (UART_C2_TCIE_MASK << 8), /*!< Transmission complete interrupt. */
    kUART_RxDataRegFullInterruptEnable = (UART_C2_RIE_MASK << 8),         /*!< Receiver data register full interrupt. */
    kUART_IdleLineInterruptEnable = (UART_C2_ILIE_MASK << 8),             /*!< Idle line interrupt. */
    kUART_RxOverrunInterruptEnable = (UART_C3_ORIE_MASK << 16),           /*!< Receiver overrun interrupt. */
    kUART_NoiseErrorInterruptEnable = (UART_C3_NEIE_MASK << 16),          /*!< Noise error flag interrupt. */
    kUART_FramingErrorInterruptEnable = (UART_C3_FEIE_MASK << 16),        /*!< Framing error flag interrupt. */
    kUART_ParityErrorInterruptEnable = (UART_C3_PEIE_MASK << 16),         /*!< Parity error flag interrupt. */
#if defined(FSL_FEATURE_UART_HAS_FIFO) && FSL_FEATURE_UART_HAS_FIFO
    kUART_RxFifoOverflowInterruptEnable = (UART_CFIFO_TXOFE_MASK << 24),  /*!< TX FIFO overflow interrupt. */
    kUART_TxFifoOverflowInterruptEnable = (UART_CFIFO_RXUFE_MASK << 24),  /*!< RX FIFO underflow interrupt. */
    kUART_RxFifoUnderflowInterruptEnable = (UART_CFIFO_RXUFE_MASK << 24), /*!< RX FIFO underflow interrupt. */
#endif
};

/*!
 * @brief UART status flags.
 *
 * This provides constants for the UART status flags for use in the UART functions.
 */
enum _uart_flags
{
    kUART_TxDataRegEmptyFlag = (UART_S1_TDRE_MASK),     /*!< TX data register empty flag. */
    kUART_TransmissionCompleteFlag = (UART_S1_TC_MASK), /*!< Transmission complete flag. */
    kUART_RxDataRegFullFlag = (UART_S1_RDRF_MASK),      /*!< RX data register full flag. */
    kUART_IdleLineFlag = (UART_S1_IDLE_MASK),           /*!< Idle line detect flag. */
    kUART_RxOverrunFlag = (UART_S1_OR_MASK),            /*!< RX overrun flag. */
    kUART_NoiseErrorFlag = (UART_S1_NF_MASK),           /*!< RX takes 3 samples of each received bit.
                                                             If any of these samples differ, noise flag sets */
    kUART_FramingErrorFlag = (UART_S1_FE_MASK),         /*!< Frame error flag, sets if logic 0 was detected
                                                             where stop bit expected */
    kUART_ParityErrorFlag = (UART_S1_PF_MASK),          /*!< If parity enabled, sets upon parity error detection */
#if defined(FSL_FEATURE_UART_HAS_LIN_BREAK_DETECT) && FSL_FEATURE_UART_HAS_LIN_BREAK_DETECT
    kUART_LinBreakFlag =
        (UART_S2_LBKDIF_MASK << 8), /*!< LIN break detect interrupt flag, sets when
                                                           LIN break char detected and LIN circuit enabled */
#endif
    kUART_RxActiveEdgeFlag = (UART_S2_RXEDGIF_MASK << 8), /*!< RX pin active edge interrupt flag,
                                                                                 sets when active edge detected */
    kUART_RxActiveFlag = (UART_S2_RAF_MASK << 8),         /*!< Receiver Active Flag (RAF),
                                                                                 sets at beginning of valid start bit */
#if defined(FSL_FEATURE_UART_HAS_EXTENDED_DATA_REGISTER_FLAGS) && FSL_FEATURE_UART_HAS_EXTENDED_DATA_REGISTER_FLAGS
    kUART_NoiseErrorInRxDataRegFlag = (UART_ED_NOISY_MASK << 16),    /*!< Noisy bit, sets if noise detected. */
    kUART_ParityErrorInRxDataRegFlag = (UART_ED_PARITYE_MASK << 16), /*!< Paritye bit, sets if parity error detected. */
#endif
#if defined(FSL_FEATURE_UART_HAS_FIFO) && FSL_FEATURE_UART_HAS_FIFO
    kUART_TxFifoEmptyFlag = (UART_SFIFO_TXEMPT_MASK << 24),   /*!< TXEMPT bit, sets if TX buffer is empty */
    kUART_RxFifoEmptyFlag = (UART_SFIFO_RXEMPT_MASK << 24),   /*!< RXEMPT bit, sets if RX buffer is empty */
    kUART_TxFifoOverflowFlag = (UART_SFIFO_TXOF_MASK << 24),  /*!< TXOF bit, sets if TX buffer overflow occurred */
    kUART_RxFifoOverflowFlag = (UART_SFIFO_RXOF_MASK << 24),  /*!< RXOF bit, sets if receive buffer overflow */
    kUART_RxFifoUnderflowFlag = (UART_SFIFO_RXUF_MASK << 24), /*!< RXUF bit, sets if receive buffer underflow */
#endif
};

/*! @brief UART configuration structure. */
typedef struct _uart_config
{
    uint32_t baudRate_Bps;         /*!< UART baud rate  */
    uart_parity_mode_t parityMode; /*!< Parity mode, disabled (default), even, odd */
#if defined(FSL_FEATURE_UART_HAS_STOP_BIT_CONFIG_SUPPORT) && FSL_FEATURE_UART_HAS_STOP_BIT_CONFIG_SUPPORT
    uart_stop_bit_count_t stopBitCount; /*!< Number of stop bits, 1 stop bit (default) or 2 stop bits  */
#endif
#if defined(FSL_FEATURE_UART_HAS_FIFO) && FSL_FEATURE_UART_HAS_FIFO
    uint8_t txFifoWatermark; /*!< TX FIFO watermark */
    uint8_t rxFifoWatermark; /*!< RX FIFO watermark */
#endif
    bool enableTx; /*!< Enable TX */
    bool enableRx; /*!< Enable RX */
} uart_config_t;

/*! @brief UART transfer structure. */
typedef struct _uart_transfer
{
    uint8_t *data;   /*!< The buffer of data to be transfer.*/
    size_t dataSize; /*!< The byte count to be transfer. */
} uart_transfer_t;

/* Forward declaration of the handle typedef. */
typedef struct _uart_handle uart_handle_t;

/*! @brief UART transfer callback function. */
typedef void (*uart_transfer_callback_t)(UART_Type *base, uart_handle_t *handle, status_t status, void *userData);

/*! @brief UART handle structure. */
struct _uart_handle
{
    uint8_t *volatile txData;   /*!< Address of remaining data to send. */
    volatile size_t txDataSize; /*!< Size of the remaining data to send. */
    size_t txDataSizeAll;       /*!< Size of the data to send out. */
    uint8_t *volatile rxData;   /*!< Address of remaining data to receive. */
    volatile size_t rxDataSize; /*!< Size of the remaining data to receive. */
    size_t rxDataSizeAll;       /*!< Size of the data to receive. */

    uint8_t *rxRingBuffer;              /*!< Start address of the receiver ring buffer. */
    size_t rxRingBufferSize;            /*!< Size of the ring buffer. */
    volatile uint16_t rxRingBufferHead; /*!< Index for the driver to store received data into ring buffer. */
    volatile uint16_t rxRingBufferTail; /*!< Index for the user to get data from the ring buffer. */

    uart_transfer_callback_t callback; /*!< Callback function. */
    void *userData;                    /*!< UART callback function parameter.*/

    volatile uint8_t txState; /*!< TX transfer state. */
    volatile uint8_t rxState; /*!< RX transfer state */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* _cplusplus */

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Initializes a UART instance with user configuration structure and peripheral clock.
 *
 * This function configures the UART module with the user-defined settings. The user can configure the configuration
 * structure and also get the default configuration by using the UART_GetDefaultConfig() function.
 * Example below shows how to use this API to configure UART.
 * @code
 *  uart_config_t uartConfig;
 *  uartConfig.baudRate_Bps = 115200U;
 *  uartConfig.parityMode = kUART_ParityDisabled;
 *  uartConfig.stopBitCount = kUART_OneStopBit;
 *  uartConfig.txFifoWatermark = 0;
 *  uartConfig.rxFifoWatermark = 1;
 *  UART_Init(UART1, &uartConfig, 20000000U);
 * @endcode
 *
 * @param base UART peripheral base address.
 * @param config Pointer to user-defined configuration structure.
 * @param srcClock_Hz UART clock source frequency in HZ.
 */
void UART_Init(UART_Type *base, const uart_config_t *config, uint32_t srcClock_Hz);

/*!
 * @brief Deinitializes a UART instance.
 *
 * This function waits for TX complete, disables TX and RX, and disables the UART clock.
 *
 * @param base UART peripheral base address.
 */
void UART_Deinit(UART_Type *base);

/*!
 * @brief Gets the default configuration structure.
 *
 * This function initializes the UART configuration structure to a default value. The default
 * values are:
 *   uartConfig->baudRate_Bps = 115200U;
 *   uartConfig->bitCountPerChar = kUART_8BitsPerChar;
 *   uartConfig->parityMode = kUART_ParityDisabled;
 *   uartConfig->stopBitCount = kUART_OneStopBit;
 *   uartConfig->txFifoWatermark = 0;
 *   uartConfig->rxFifoWatermark = 1;
 *   uartConfig->enableTx = false;
 *   uartConfig->enableRx = false;
 *
 * @param config Pointer to configuration structure.
 */
void UART_GetDefaultConfig(uart_config_t *config);

/*!
 * @brief Sets the UART instance baud rate.
 *
 * This function configures the UART module baud rate. This function is used to update
 * the UART module baud rate after the UART module is initialized by the UART_Init.
 * @code
 *  UART_SetBaudRate(UART1, 115200U, 20000000U);
 * @endcode
 *
 * @param base UART peripheral base address.
 * @param baudRate_Bps UART baudrate to be set.
 * @param srcClock_Hz UART clock source freqency in HZ.
 */
void UART_SetBaudRate(UART_Type *base, uint32_t baudRate_Bps, uint32_t srcClock_Hz);

/* @} */

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Get UART status flags.
 *
 * This function get all UART status flags, the flags are returned as the logical
 * OR value of the enumerators @ref _uart_flags. To check specific status,
 * compare the return value with enumerators in @ref _uart_flags.
 * For example, to check whether the TX is empty:
 * @code
 *     if (kUART_TxDataRegEmptyFlag & UART_GetStatusFlags(UART1))
 *     {
 *         ...
 *     }
 * @endcode
 *
 * @param base UART peripheral base address.
 * @return UART status flags which are ORed by the enumerators in the _uart_flags.
 */
uint32_t UART_GetStatusFlags(UART_Type *base);

/*!
 * @brief Clears status flags with the provided mask.
 *
 * This function clears UART status flags with a provided mask. Automatically cleared flag
 * can't be cleared by this function.
 * Some flags can only be cleared or set by hardware itself. These flags are:
 *    kUART_TxDataRegEmptyFlag, kUART_TransmissionCompleteFlag, kUART_RxDataRegFullFlag,
 *    kUART_RxActiveFlag, kUART_NoiseErrorInRxDataRegFlag, kUART_ParityErrorInRxDataRegFlag,
 *    kUART_TxFifoEmptyFlag,kUART_RxFifoEmptyFlag
 * Note: This API should be called when the Tx/Rx is idle, otherwise it takes no effects.
 *
 * @param base UART peripheral base address.
 * @param mask The status flags to be cleared, it is logical OR value of @ref _uart_flags.
 * @retval kStatus_UART_FlagCannotClearManually The flag can't be cleared by this function but
 *         it is cleared automatically by hardware.
 * @retval kStatus_Success Status in the mask are cleared.
 */
status_t UART_ClearStatusFlags(UART_Type *base, uint32_t mask);

/* @} */

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enables UART interrupts according to the provided mask.
 *
 * This function enables the UART interrupts according to the provided mask. The mask
 * is a logical OR of enumeration members. See @ref _uart_interrupt_enable.
 * For example, to enable TX empty interrupt and RX full interrupt:
 * @code
 *     UART_EnableInterrupts(UART1,kUART_TxDataRegEmptyInterruptEnable | kUART_RxDataRegFullInterruptEnable);
 * @endcode
 *
 * @param base UART peripheral base address.
 * @param mask The interrupts to enable. Logical OR of @ref _uart_interrupt_enable.
 */
void UART_EnableInterrupts(UART_Type *base, uint32_t mask);

/*!
 * @brief Disables the UART interrupts according to the provided mask.
 *
 * This function disables the UART interrupts according to the provided mask. The mask
 * is a logical OR of enumeration members. See @ref _uart_interrupt_enable.
 * For example, to disable TX empty interrupt and RX full interrupt:
 * @code
 *     UART_DisableInterrupts(UART1,kUART_TxDataRegEmptyInterruptEnable | kUART_RxDataRegFullInterruptEnable);
 * @endcode
 *
 * @param base UART peripheral base address.
 * @param mask The interrupts to disable. Logical OR of @ref _uart_interrupt_enable.
 */
void UART_DisableInterrupts(UART_Type *base, uint32_t mask);

/*!
 * @brief Gets the enabled UART interrupts.
 *
 * This function gets the enabled UART interrupts. The enabled interrupts are returned
 * as the logical OR value of the enumerators @ref _uart_interrupt_enable. To check
 * specific interrupts enable status, compare the return value with enumerators
 * in @ref _uart_interrupt_enable.
 * For example, to check whether TX empty interrupt is enabled:
 * @code
 *     uint32_t enabledInterrupts = UART_GetEnabledInterrupts(UART1);
 *
 *     if (kUART_TxDataRegEmptyInterruptEnable & enabledInterrupts)
 *     {
 *         ...
 *     }
 * @endcode
 *
 * @param base UART peripheral base address.
 * @return UART interrupt flags which are logical OR of the enumerators in @ref _uart_interrupt_enable.
 */
uint32_t UART_GetEnabledInterrupts(UART_Type *base);

/* @} */

#if defined(FSL_FEATURE_UART_HAS_DMA_SELECT) && FSL_FEATURE_UART_HAS_DMA_SELECT
/*!
 * @name DMA Control
 * @{
 */

/*!
 * @brief Gets the UART data register address.
 *
 * This function returns the UART data register address, which is mainly used by DMA/eDMA.
 *
 * @param base UART peripheral base address.
 * @return UART data register address which are used both by transmitter and receiver.
 */
static inline uint32_t UART_GetDataRegisterAddress(UART_Type *base)
{
    return (uint32_t) & (base->D);
}

/*!
 * @brief Enables or disables the UART transmitter DMA request.
 *
 * This function enables or disables the transmit data register empty flag, S1[TDRE], to generate the DMA requests.
 *
 * @param base UART peripheral base address.
 * @param enable True to enable, false to disable.
 */
static inline void UART_EnableTxDMA(UART_Type *base, bool enable)
{
    if (enable)
    {
#if (defined(FSL_FEATURE_UART_IS_SCI) && FSL_FEATURE_UART_IS_SCI)
        base->C4 |= UART_C4_TDMAS_MASK;
#else
        base->C5 |= UART_C5_TDMAS_MASK;
#endif
        base->C2 |= UART_C2_TIE_MASK;
    }
    else
    {
#if (defined(FSL_FEATURE_UART_IS_SCI) && FSL_FEATURE_UART_IS_SCI)
        base->C4 &= ~UART_C4_TDMAS_MASK;
#else
        base->C5 &= ~UART_C5_TDMAS_MASK;
#endif
        base->C2 &= ~UART_C2_TIE_MASK;
    }
}

/*!
 * @brief Enables or disables the UART receiver DMA.
 *
 * This function enables or disables the receiver data register full flag, S1[RDRF], to generate DMA requests.
 *
 * @param base UART peripheral base address.
 * @param enable True to enable, false to disable.
 */
static inline void UART_EnableRxDMA(UART_Type *base, bool enable)
{
    if (enable)
    {
#if (defined(FSL_FEATURE_UART_IS_SCI) && FSL_FEATURE_UART_IS_SCI)
        base->C4 |= UART_C4_RDMAS_MASK;
#else
        base->C5 |= UART_C5_RDMAS_MASK;
#endif
        base->C2 |= UART_C2_RIE_MASK;
    }
    else
    {
#if (defined(FSL_FEATURE_UART_IS_SCI) && FSL_FEATURE_UART_IS_SCI)
        base->C4 &= ~UART_C4_RDMAS_MASK;
#else
        base->C5 &= ~UART_C5_RDMAS_MASK;
#endif
        base->C2 &= ~UART_C2_RIE_MASK;
    }
}

/* @} */
#endif /* FSL_FEATURE_UART_HAS_DMA_SELECT */

/*!
 * @name Bus Operations
 * @{
 */

/*!
 * @brief Enables or disables the UART transmitter.
 *
 * This function enables or disables the UART transmitter.
 *
 * @param base UART peripheral base address.
 * @param enable True to enable, false to disable.
 */
static inline void UART_EnableTx(UART_Type *base, bool enable)
{
    if (enable)
    {
        base->C2 |= UART_C2_TE_MASK;
    }
    else
    {
        base->C2 &= ~UART_C2_TE_MASK;
    }
}

/*!
 * @brief Enables or disables the UART receiver.
 *
 * This function enables or disables the UART receiver.
 *
 * @param base UART peripheral base address.
 * @param enable True to enable, false to disable.
 */
static inline void UART_EnableRx(UART_Type *base, bool enable)
{
    if (enable)
    {
        base->C2 |= UART_C2_RE_MASK;
    }
    else
    {
        base->C2 &= ~UART_C2_RE_MASK;
    }
}

/*!
 * @brief Writes to the TX register.
 *
 * This function writes data to the TX register directly. The upper layer must ensure
 * that the TX register is empty or TX FIFO has empty room before calling this function.
 *
 * @param base UART peripheral base address.
 * @param data The byte to write.
 */
static inline void UART_WriteByte(UART_Type *base, uint8_t data)
{
    base->D = data;
}

/*!
 * @brief Reads the RX register directly.
 *
 * This function reads data from the TX register directly. The upper layer must
 * ensure that the RX register is full or that the TX FIFO has data before calling this function.
 *
 * @param base UART peripheral base address.
 * @return The byte read from UART data register.
 */
static inline uint8_t UART_ReadByte(UART_Type *base)
{
    return base->D;
}

/*!
 * @brief Writes to the TX register using a blocking method.
 *
 * This function polls the TX register, waits for the TX register to be empty or for the TX FIFO
 * to have room and writes data to the TX buffer.
 *
 * @note This function does not check whether all the data has been sent out to the bus.
 * Before disabling the TX, check kUART_TransmissionCompleteFlag to ensure that the TX is
 * finished.
 *
 * @param base UART peripheral base address.
 * @param data Start address of the data to write.
 * @param length Size of the data to write.
 */
void UART_WriteBlocking(UART_Type *base, const uint8_t *data, size_t length);

/*!
 * @brief Read RX data register using a blocking method.
 *
 * This function polls the RX register, waits for the RX register to be full or for RX FIFO to
 * have data and read data from the TX register.
 *
 * @param base UART peripheral base address.
 * @param data Start address of the buffer to store the received data.
 * @param length Size of the buffer.
 * @retval kStatus_UART_RxHardwareOverrun Receiver overrun happened while receiving data.
 * @retval kStatus_UART_NoiseError Noise error happened while receiving data.
 * @retval kStatus_UART_FramingError Framing error happened while receiving data.
 * @retval kStatus_UART_ParityError Parity error happened while receiving data.
 * @retval kStatus_Success Successfully received all data.
 */
status_t UART_ReadBlocking(UART_Type *base, uint8_t *data, size_t length);

/* @} */

/*!
 * @name Transactional
 * @{
 */

/*!
 * @brief Initializes the UART handle.
 *
 * This function initializes the UART handle which can be used for other UART
 * transactional APIs. Usually, for a specified UART instance,
 * call this API once to get the initialized handle.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param callback The callback function.
 * @param userData The parameter of the callback function.
 */
void UART_TransferCreateHandle(UART_Type *base,
                               uart_handle_t *handle,
                               uart_transfer_callback_t callback,
                               void *userData);

/*!
 * @brief Sets up the RX ring buffer.
 *
 * This function sets up the RX ring buffer to a specific UART handle.
 *
 * When the RX ring buffer is used, data received are stored into the ring buffer even when the
 * user doesn't call the UART_TransferReceiveNonBlocking() API. If there is already data received
 * in the ring buffer, the user can get the received data from the ring buffer directly.
 *
 * @note When using the RX ring buffer, one byte is reserved for internal use. In other
 * words, if @p ringBufferSize is 32, then only 31 bytes are used for saving data.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param ringBuffer Start address of the ring buffer for background receiving. Pass NULL to disable the ring buffer.
 * @param ringBufferSize size of the ring buffer.
 */
void UART_TransferStartRingBuffer(UART_Type *base, uart_handle_t *handle, uint8_t *ringBuffer, size_t ringBufferSize);

/*!
 * @brief Aborts the background transfer and uninstalls the ring buffer.
 *
 * This function aborts the background transfer and uninstalls the ring buffer.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 */
void UART_TransferStopRingBuffer(UART_Type *base, uart_handle_t *handle);

/*!
 * @brief Transmits a buffer of data using the interrupt method.
 *
 * This function sends data using an interrupt method. This is a non-blocking function, which
 * returns directly without waiting for all data to be written to the TX register. When
 * all data is written to the TX register in the ISR, the UART driver calls the callback
 * function and passes the @ref kStatus_UART_TxIdle as status parameter.
 *
 * @note The kStatus_UART_TxIdle is passed to the upper layer when all data is written
 * to the TX register. However it does not ensure that all data are sent out. Before disabling the TX,
 * check the kUART_TransmissionCompleteFlag to ensure that the TX is finished.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param xfer UART transfer structure. See  #uart_transfer_t.
 * @retval kStatus_Success Successfully start the data transmission.
 * @retval kStatus_UART_TxBusy Previous transmission still not finished, data not all written to TX register yet.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t UART_TransferSendNonBlocking(UART_Type *base, uart_handle_t *handle, uart_transfer_t *xfer);

/*!
 * @brief Aborts the interrupt driven data transmit.
 *
 * This function aborts the interrupt driven data sending. The user can get the remainBytes to find out
 * how many bytes are still not sent out.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 */
void UART_TransferAbortSend(UART_Type *base, uart_handle_t *handle);

/*!
 * @brief Get the number of bytes that have been written to UART TX register.
 *
 * This function gets the number of bytes that have been written to UART TX
 * register by interrupt method.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param count Send bytes count.
 * @retval kStatus_NoTransferInProgress No send in progress.
 * @retval kStatus_InvalidArgument Parameter is invalid.
 * @retval kStatus_Success Get successfully through the parameter \p count;
 */
status_t UART_TransferGetSendCount(UART_Type *base, uart_handle_t *handle, uint32_t *count);

/*!
 * @brief Receives a buffer of data using an interrupt method.
 *
 * This function receives data using an interrupt method. This is a non-blocking function, which
 *  returns without waiting for all data to be received.
 * If the RX ring buffer is used and not empty, the data in the ring buffer is copied and
 * the parameter @p receivedBytes shows how many bytes are copied from the ring buffer.
 * After copying, if the data in the ring buffer is not enough to read, the receive
 * request is saved by the UART driver. When the new data arrives, the receive request
 * is serviced first. When all data is received, the UART driver notifies the upper layer
 * through a callback function and passes the status parameter @ref kStatus_UART_RxIdle.
 * For example, the upper layer needs 10 bytes but there are only 5 bytes in the ring buffer.
 * The 5 bytes are copied to the xfer->data and this function returns with the
 * parameter @p receivedBytes set to 5. For the left 5 bytes, newly arrived data is
 * saved from the xfer->data[5]. When 5 bytes are received, the UART driver notifies the upper layer.
 * If the RX ring buffer is not enabled, this function enables the RX and RX interrupt
 * to receive data to the xfer->data. When all data is received, the upper layer is notified.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param xfer UART transfer structure, refer to #uart_transfer_t.
 * @param receivedBytes Bytes received from the ring buffer directly.
 * @retval kStatus_Success Successfully queue the transfer into transmit queue.
 * @retval kStatus_UART_RxBusy Previous receive request is not finished.
 * @retval kStatus_InvalidArgument Invalid argument.
 */
status_t UART_TransferReceiveNonBlocking(UART_Type *base,
                                         uart_handle_t *handle,
                                         uart_transfer_t *xfer,
                                         size_t *receivedBytes);

/*!
 * @brief Aborts the interrupt-driven data receiving.
 *
 * This function aborts the interrupt-driven data receiving. The user can get the remainBytes to know
 * how many bytes not received yet.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 */
void UART_TransferAbortReceive(UART_Type *base, uart_handle_t *handle);

/*!
 * @brief Get the number of bytes that have been received.
 *
 * This function gets the number of bytes that have been received.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 * @param count Receive bytes count.
 * @retval kStatus_NoTransferInProgress No receive in progress.
 * @retval kStatus_InvalidArgument Parameter is invalid.
 * @retval kStatus_Success Get successfully through the parameter \p count;
 */
status_t UART_TransferGetReceiveCount(UART_Type *base, uart_handle_t *handle, uint32_t *count);

/*!
 * @brief UART IRQ handle function.
 *
 * This function handles the UART transmit and receive IRQ request.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 */
void UART_TransferHandleIRQ(UART_Type *base, uart_handle_t *handle);

/*!
 * @brief UART Error IRQ handle function.
 *
 * This function handle the UART error IRQ request.
 *
 * @param base UART peripheral base address.
 * @param handle UART handle pointer.
 */
void UART_TransferHandleErrorIRQ(UART_Type *base, uart_handle_t *handle);

/* @} */

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* _FSL_UART_H_ */
