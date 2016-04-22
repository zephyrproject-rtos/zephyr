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
#ifndef _FSL_I2C_H_
#define _FSL_I2C_H_

#include "fsl_common.h"

/*!
 * @addtogroup i2c_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief I2C driver version 2.0.0. */
#define FSL_I2C_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

#if (defined(FSL_FEATURE_I2C_HAS_START_STOP_DETECT) && FSL_FEATURE_I2C_HAS_START_STOP_DETECT || \
     defined(FSL_FEATURE_I2C_HAS_STOP_DETECT) && FSL_FEATURE_I2C_HAS_STOP_DETECT)
#define I2C_HAS_STOP_DETECT
#endif /* FSL_FEATURE_I2C_HAS_START_STOP_DETECT / FSL_FEATURE_I2C_HAS_STOP_DETECT */

/*! @brief  I2C status return codes. */
enum _i2c_status
{
    kStatus_I2C_Busy = MAKE_STATUS(kStatusGroup_I2C, 0),            /*!< I2C is busy with current transfer. */
    kStatus_I2C_Idle = MAKE_STATUS(kStatusGroup_I2C, 1),            /*!< Bus is Idle. */
    kStatus_I2C_Nak = MAKE_STATUS(kStatusGroup_I2C, 2),             /*!< NAK received during transfer. */
    kStatus_I2C_ArbitrationLost = MAKE_STATUS(kStatusGroup_I2C, 3), /*!< Arbitration lost during transfer. */
    kStatus_I2C_Timeout = MAKE_STATUS(kStatusGroup_I2C, 4),         /*!< Wait event timeout. */
};

/*!
 * @brief I2C peripheral flags
 *
 * The following status register flags can be cleared:
 * - #kI2C_ArbitrationLostFlag
 * - #kI2C_IntPendingFlag
 * - #kI2C_StartDetectFlag
 * - #kI2C_StopDetectFlag
 *
 * @note These enumerations are meant to be OR'd together to form a bit mask.
 *
 */
enum _i2c_flags
{
    kI2C_ReceiveNakFlag = I2C_S_RXAK_MASK,       /*!< I2C receive NAK flag. */
    kI2C_IntPendingFlag = I2C_S_IICIF_MASK,      /*!< I2C interrupt pending flag. */
    kI2C_TransferDirectionFlag = I2C_S_SRW_MASK, /*!< I2C transfer direction flag. */
    kI2C_RangeAddressMatchFlag = I2C_S_RAM_MASK, /*!< I2C range address match flag. */
    kI2C_ArbitrationLostFlag = I2C_S_ARBL_MASK,  /*!< I2C arbitration lost flag. */
    kI2C_BusBusyFlag = I2C_S_BUSY_MASK,          /*!< I2C bus busy flag. */
    kI2C_AddressMatchFlag = I2C_S_IAAS_MASK,     /*!< I2C address match flag. */
    kI2C_TransferCompleteFlag = I2C_S_TCF_MASK,  /*!< I2C transfer complete flag. */
#ifdef I2C_HAS_STOP_DETECT
    kI2C_StopDetectFlag = I2C_FLT_STOPF_MASK << 8, /*!< I2C stop detect flag. */
#endif /* FSL_FEATURE_I2C_HAS_START_STOP_DETECT / FSL_FEATURE_I2C_HAS_STOP_DETECT */

#if defined(FSL_FEATURE_I2C_HAS_START_STOP_DETECT) && FSL_FEATURE_I2C_HAS_START_STOP_DETECT
    kI2C_StartDetectFlag = I2C_FLT_STARTF_MASK << 8, /*!< I2C start detect flag. */
#endif                                               /* FSL_FEATURE_I2C_HAS_START_STOP_DETECT */
};

/*! @brief I2C feature interrupt source. */
enum _i2c_interrupt_enable
{
    kI2C_GlobalInterruptEnable = I2C_C1_IICIE_MASK, /*!< I2C global interrupt. */

#if defined(FSL_FEATURE_I2C_HAS_STOP_DETECT) && FSL_FEATURE_I2C_HAS_STOP_DETECT
    kI2C_StopDetectInterruptEnable = I2C_FLT_STOPIE_MASK, /*!< I2C stop detect interrupt. */
#endif                                                    /* FSL_FEATURE_I2C_HAS_STOP_DETECT */

#if defined(FSL_FEATURE_I2C_HAS_START_STOP_DETECT) && FSL_FEATURE_I2C_HAS_START_STOP_DETECT
    kI2C_StartStopDetectInterruptEnable = I2C_FLT_SSIE_MASK, /*!< I2C start&stop detect interrupt. */
#endif                                                       /* FSL_FEATURE_I2C_HAS_START_STOP_DETECT */
};

/*! @brief Direction of master and slave transfers. */
typedef enum _i2c_direction
{
    kI2C_Write = 0x0U, /*!< Master transmit to slave. */
    kI2C_Read = 0x1U,  /*!< Master receive from slave. */
} i2c_direction_t;

/*! @brief Addressing mode. */
typedef enum _i2c_slave_address_mode
{
    kI2C_Address7bit = 0x0U, /*!< 7-bit addressing mode. */
    kI2C_RangeMatch = 0X2U,  /*!< Range address match addressing mode. */
} i2c_slave_address_mode_t;

/*! @brief I2C transfer control flag. */
enum _i2c_master_transfer_flags
{
    kI2C_TransferDefaultFlag = 0x0U,       /*!< Transfer starts with a start signal, stops with a stop signal. */
    kI2C_TransferNoStartFlag = 0x1U,       /*!< Transfer starts without a start signal. */
    kI2C_TransferRepeatedStartFlag = 0x2U, /*!< Transfer starts with a repeated start signal. */
    kI2C_TransferNoStopFlag = 0x4U,        /*!< Transfer ends without a stop signal. */
};

/*!
 * @brief Set of events sent to the callback for nonblocking slave transfers.
 *
 * These event enumerations are used for two related purposes. First, a bit mask created by OR'ing together
 * events is passed to I2C_SlaveTransferNonBlocking() in order to specify which events to enable.
 * Then, when the slave callback is invoked, it is passed the current event through its @a transfer
 * parameter.
 *
 * @note These enumerations are meant to be OR'd together to form a bit mask of events.
 */
typedef enum _i2c_slave_transfer_event
{
    kI2C_SlaveAddressMatchEvent = 0x01U, /*!< Received the slave address after a start or repeated start. */
    kI2C_SlaveTransmitEvent = 0x02U,     /*!< Callback is requested to provide data to transmit
                                                (slave-transmitter role). */
    kI2C_SlaveReceiveEvent = 0x04U,      /*!< Callback is requested to provide a buffer in which to place received
                                                 data (slave-receiver role). */
    kI2C_SlaveTransmitAckEvent = 0x08U,  /*!< Callback needs to either transmit an ACK or NACK. */
#if defined(FSL_FEATURE_I2C_HAS_START_STOP_DETECT) && FSL_FEATURE_I2C_HAS_START_STOP_DETECT
    kI2C_SlaveRepeatedStartEvent = 0x10U, /*!< A repeated start was detected. */
#endif
    kI2C_SlaveCompletionEvent = 0x20U, /*!< A stop was detected or finished transfer, completing the transfer. */

    /*! Bit mask of all available events. */
    kI2C_SlaveAllEvents = kI2C_SlaveAddressMatchEvent | kI2C_SlaveTransmitEvent | kI2C_SlaveReceiveEvent |
#if defined(FSL_FEATURE_I2C_HAS_START_STOP_DETECT) && FSL_FEATURE_I2C_HAS_START_STOP_DETECT
                          kI2C_SlaveRepeatedStartEvent |
#endif
                          kI2C_SlaveCompletionEvent,
} i2c_slave_transfer_event_t;

/*! @brief I2C master user configuration. */
typedef struct _i2c_master_config
{
    bool enableMaster; /*!< Enables the I2C peripheral at initialization time. */
#if defined(FSL_FEATURE_I2C_HAS_HIGH_DRIVE_SELECTION) && FSL_FEATURE_I2C_HAS_HIGH_DRIVE_SELECTION
    bool enableHighDrive; /*!< Controls the drive capability of the I2C pads. */
#endif
#if defined(FSL_FEATURE_I2C_HAS_STOP_HOLD_OFF) && FSL_FEATURE_I2C_HAS_STOP_HOLD_OFF
    bool enableStopHold; /*!< Controls the stop hold enable. */
#endif
    uint32_t baudRate_Bps;     /*!< Baud rate configuration of I2C peripheral. */
    uint8_t glitchFilterWidth; /*!< Controls the width of the glitch. */
} i2c_master_config_t;

/*! @brief I2C slave user configuration. */
typedef struct _i2c_slave_config
{
    bool enableSlave;       /*!< Enables the I2C peripheral at initialization time. */
    bool enableGeneralCall; /*!< Enable general call addressing mode. */
    bool enableWakeUp;      /*!< Enables/disables waking up MCU from low power mode. */
#if defined(FSL_FEATURE_I2C_HAS_HIGH_DRIVE_SELECTION) && FSL_FEATURE_I2C_HAS_HIGH_DRIVE_SELECTION
    bool enableHighDrive; /*!< Controls the drive capability of the I2C pads. */
#endif
    bool enableBaudRateCtl; /*!< Enables/disables independent slave baud rate on SCL in very fast I2C modes. */
    uint16_t slaveAddress;  /*!< Slave address configuration. */
    uint16_t upperAddress;  /*!< Maximum boundary slave address used in range matching mode. */
    i2c_slave_address_mode_t addressingMode; /*!< Addressing mode configuration of i2c_slave_address_mode_config_t. */
} i2c_slave_config_t;

/*! @brief I2C master handle typedef. */
typedef struct _i2c_master_handle i2c_master_handle_t;

/*! @brief I2C master transfer callback typedef. */
typedef void (*i2c_master_transfer_callback_t)(I2C_Type *base,
                                               i2c_master_handle_t *handle,
                                               status_t status,
                                               void *userData);

/*! @brief I2C slave handle typedef. */
typedef struct _i2c_slave_handle i2c_slave_handle_t;

/*! @brief I2C master transfer structure. */
typedef struct _i2c_master_transfer
{
    uint32_t flags;            /*!< Transfer flag which controls the transfer. */
    uint8_t slaveAddress;      /*!< 7-bit slave address. */
    i2c_direction_t direction; /*!< Transfer direction, read or write. */
    uint32_t subaddress;       /*!< Sub address. Transferred MSB first. */
    uint8_t subaddressSize;    /*!< Size of command buffer. */
    uint8_t *volatile data;    /*!< Transfer buffer. */
    volatile size_t dataSize;  /*!< Transfer size. */
} i2c_master_transfer_t;

/*! @brief I2C master handle structure. */
struct _i2c_master_handle
{
    i2c_master_transfer_t transfer;                    /*!< I2C master transfer copy. */
    size_t transferSize;                               /*!< Total bytes to be transferred. */
    uint8_t state;                                     /*!< Transfer state maintained during transfer. */
    i2c_master_transfer_callback_t completionCallback; /*!< Callback function called when transfer finished. */
    void *userData;                                    /*!< Callback parameter passed to callback function. */
};

/*! @brief I2C slave transfer structure. */
typedef struct _i2c_slave_transfer
{
    i2c_slave_transfer_event_t event; /*!< Reason the callback is being invoked. */
    uint8_t *volatile data;           /*!< Transfer buffer. */
    volatile size_t dataSize;         /*!< Transfer size. */
    status_t completionStatus;        /*!< Success or error code describing how the transfer completed. Only applies for
                                         #kI2C_SlaveCompletionEvent. */
    size_t transferredCount;          /*!< Number of bytes actually transferred since start or last repeated start. */
} i2c_slave_transfer_t;

/*! @brief I2C slave transfer callback typedef. */
typedef void (*i2c_slave_transfer_callback_t)(I2C_Type *base, i2c_slave_transfer_t *xfer, void *userData);

/*! @brief I2C slave handle structure. */
struct _i2c_slave_handle
{
    bool isBusy;                            /*!< Whether transfer is busy. */
    i2c_slave_transfer_t transfer;          /*!< I2C slave transfer copy. */
    uint32_t eventMask;                     /*!< Mask of enabled events. */
    i2c_slave_transfer_callback_t callback; /*!< Callback function called at transfer event. */
    void *userData;                         /*!< Callback parameter passed to callback. */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /*_cplusplus. */

/*!
 * @name Initialization and deinitialization
 * @{
 */

/*!
 * @brief Initializes the I2C peripheral. Call this API to ungate the I2C clock
 * and configure the I2C with master configuration.
 *
 * @note This API should be called at the beginning of the application to use
 * the I2C driver, or any operation to the I2C module could cause hard fault
 * because clock is not enabled. The configuration structure can be filled by user
 * from scratch, or be set with default values by I2C_MasterGetDefaultConfig().
 * After calling this API, the master is ready to transfer.
 * Example:
 * @code
 * i2c_master_config_t config = {
 * .enableMaster = true,
 * .enableStopHold = false,
 * .highDrive = false,
 * .baudRate_Bps = 100000,
 * .glitchFilterWidth = 0
 * };
 * I2C_MasterInit(I2C0, &config, 12000000U);
 * @endcode
 *
 * @param base I2C base pointer
 * @param masterConfig pointer to master configuration structure
 * @param srcClock_Hz I2C peripheral clock frequency in Hz
 */
void I2C_MasterInit(I2C_Type *base, const i2c_master_config_t *masterConfig, uint32_t srcClock_Hz);

/*!
 * @brief Initializes the I2C peripheral. Call this API to ungate the I2C clock
 * and initializes the I2C with slave configuration.
 *
 * @note This API should be called at the beginning of the application to use
 * the I2C driver, or any operation to the I2C module can cause a hard fault
 * because the clock is not enabled. The configuration structure can partly be set
 * with default values by I2C_SlaveGetDefaultConfig(), or can be filled by the user.
 * Example
 * @code
 * i2c_slave_config_t config = {
 * .enableSlave = true,
 * .enableGeneralCall = false,
 * .addressingMode = kI2C_Address7bit,
 * .slaveAddress = 0x1DU,
 * .enableWakeUp = false,
 * .enablehighDrive = false,
 * .enableBaudRateCtl = false
 * };
 * I2C_SlaveInit(I2C0, &config);
 * @endcode
 *
 * @param base I2C base pointer
 * @param slaveConfig pointer to slave configuration structure
 */
void I2C_SlaveInit(I2C_Type *base, const i2c_slave_config_t *slaveConfig);

/*!
 * @brief De-initializes the I2C master peripheral. Call this API to gate the I2C clock.
 * The I2C master module can't work unless the I2C_MasterInit is called.
 * @param base I2C base pointer
 */
void I2C_MasterDeinit(I2C_Type *base);

/*!
 * @brief De-initializes the I2C slave peripheral. Calling this API gates the I2C clock.
 * The I2C slave module can't work unless the I2C_SlaveInit is called to enable the clock.
 * @param base I2C base pointer
 */
void I2C_SlaveDeinit(I2C_Type *base);

/*!
 * @brief  Sets the I2C master configuration structure to default values.
 *
 * The purpose of this API is to get the configuration structure initialized for use in the I2C_MasterConfigure().
 * Use the initialized structure unchanged in I2C_MasterConfigure(), or modify some fields of
 * the structure before calling I2C_MasterConfigure().
 * Example:
 * @code
 * i2c_master_config_t config;
 * I2C_MasterGetDefaultConfig(&config);
 * @endcode
 * @param masterConfig Pointer to the master configuration structure.
*/
void I2C_MasterGetDefaultConfig(i2c_master_config_t *masterConfig);

/*!
 * @brief  Sets the I2C slave configuration structure to default values.
 *
 * The purpose of this API is to get the configuration structure initialized for use in I2C_SlaveConfigure().
 * Modify fields of the structure before calling the I2C_SlaveConfigure().
 * Example:
 * @code
 * i2c_slave_config_t config;
 * I2C_SlaveGetDefaultConfig(&config);
 * @endcode
 * @param slaveConfig Pointer to the slave configuration structure.
 */
void I2C_SlaveGetDefaultConfig(i2c_slave_config_t *slaveConfig);

/*!
 * @brief Enables or disabless the I2C peripheral operation.
 *
 * @param base I2C base pointer
 * @param enable pass true to enable module, false to disable module
 */
static inline void I2C_Enable(I2C_Type *base, bool enable)
{
    if (enable)
    {
        base->C1 |= I2C_C1_IICEN_MASK;
    }
    else
    {
        base->C1 &= ~I2C_C1_IICEN_MASK;
    }
}

/* @} */

/*!
 * @name Status
 * @{
 */

/*!
 * @brief Gets the I2C status flags.
 *
 * @param base I2C base pointer
 * @return status flag, use status flag to AND #_i2c_flags could get the related status.
 */
uint32_t I2C_MasterGetStatusFlags(I2C_Type *base);

/*!
 * @brief Gets the I2C status flags.
 *
 * @param base I2C base pointer
 * @return status flag, use status flag to AND #_i2c_flags could get the related status.
 */
static inline uint32_t I2C_SlaveGetStatusFlags(I2C_Type *base)
{
    return I2C_MasterGetStatusFlags(base);
}

/*!
 * @brief Clears the I2C status flag state.
 *
 * The following status register flags can be cleared: kI2C_ArbitrationLostFlag and kI2C_IntPendingFlag
 *
 * @param base I2C base pointer
 * @param statusMask The status flag mask, defined in type i2c_status_flag_t.
 *      The parameter could be any combination of the following values:
 *          @arg kI2C_StartDetectFlag (if available)
 *          @arg kI2C_StopDetectFlag (if available)
 *          @arg kI2C_ArbitrationLostFlag
 *          @arg kI2C_IntPendingFlagFlag
 */
static inline void I2C_MasterClearStatusFlags(I2C_Type *base, uint32_t statusMask)
{
/* Must clear the STARTF / STOPF bits prior to clearing IICIF */
#if defined(FSL_FEATURE_I2C_HAS_START_STOP_DETECT) && FSL_FEATURE_I2C_HAS_START_STOP_DETECT
    if (statusMask & kI2C_StartDetectFlag)
    {
        /* Shift the odd-ball flags back into place. */
        base->FLT |= (uint8_t)(statusMask >> 8U);
    }
#endif

#ifdef I2C_HAS_STOP_DETECT
    if (statusMask & kI2C_StopDetectFlag)
    {
        /* Shift the odd-ball flags back into place. */
        base->FLT |= (uint8_t)(statusMask >> 8U);
    }
#endif

    base->S = (uint8_t)statusMask;
}

/*!
 * @brief Clears the I2C status flag state.
 *
 * The following status register flags can be cleared: kI2C_ArbitrationLostFlag and kI2C_IntPendingFlag
 *
  * @param base I2C base pointer
  * @param statusMask The status flag mask, defined in type i2c_status_flag_t.
 *      The parameter could be any combination of the following values:
 *          @arg kI2C_StartDetectFlag (if available)
 *          @arg kI2C_StopDetectFlag (if available)
 *          @arg kI2C_ArbitrationLostFlag
 *          @arg kI2C_IntPendingFlagFlag
 */
static inline void I2C_SlaveClearStatusFlags(I2C_Type *base, uint32_t statusMask)
{
    I2C_MasterClearStatusFlags(base, statusMask);
}

/* @} */

/*!
 * @name Interrupts
 * @{
 */

/*!
 * @brief Enables I2C interrupt requests.
 *
 * @param base I2C base pointer
 * @param mask interrupt source
 *     The parameter can be combination of the following source if defined:
 *     @arg kI2C_GlobalInterruptEnable
 *     @arg kI2C_StopDetectInterruptEnable/kI2C_StartDetectInterruptEnable
 *     @arg kI2C_SdaTimeoutInterruptEnable
 */
void I2C_EnableInterrupts(I2C_Type *base, uint32_t mask);

/*!
 * @brief Disables I2C interrupt requests.
 *
 * @param base I2C base pointer
 * @param mask interrupt source
 *     The parameter can be combination of the following source if defined:
 *     @arg kI2C_GlobalInterruptEnable
 *     @arg kI2C_StopDetectInterruptEnable/kI2C_StartDetectInterruptEnable
 *     @arg kI2C_SdaTimeoutInterruptEnable
 */
void I2C_DisableInterrupts(I2C_Type *base, uint32_t mask);

/*!
 * @name DMA Control
 * @{
 */
#if defined(FSL_FEATURE_I2C_HAS_DMA_SUPPORT) && FSL_FEATURE_I2C_HAS_DMA_SUPPORT
/*!
 * @brief Enables/disables the I2C DMA interrupt.
 *
 * @param base I2C base pointer
 * @param enable true to enable, false to disable
*/
static inline void I2C_EnableDMA(I2C_Type *base, bool enable)
{
    if (enable)
    {
        base->C1 |= I2C_C1_DMAEN_MASK;
    }
    else
    {
        base->C1 &= ~I2C_C1_DMAEN_MASK;
    }
}

#endif /* FSL_FEATURE_I2C_HAS_DMA_SUPPORT */

/*!
 * @brief  Gets the I2C tx/rx data register address. This API is used to provide a transfer address
 * for I2C DMA transfer configuration.
 *
 * @param base I2C base pointer
 * @return data register address
 */
static inline uint32_t I2C_GetDataRegAddr(I2C_Type *base)
{
    return (uint32_t)(&(base->D));
}

/* @} */
/*!
 * @name Bus Operations
 * @{
 */

/*!
 * @brief Sets the I2C master transfer baud rate.
 *
 * @param base I2C base pointer
 * @param baudRate_Bps the baud rate value in bps
 * @param srcClock_Hz Source clock
 */
void I2C_MasterSetBaudRate(I2C_Type *base, uint32_t baudRate_Bps, uint32_t srcClock_Hz);

/*!
 * @brief Sends a START on the I2C bus.
 *
 * This function is used to initiate a new master mode transfer by sending the START signal.
 * The slave address is sent following the I2C START signal.
 *
 * @param base I2C peripheral base pointer
 * @param address 7-bit slave device address.
 * @param direction Master transfer directions(transmit/receive).
 * @retval kStatus_Success Successfully send the start signal.
 * @retval kStatus_I2C_Busy Current bus is busy.
 */
status_t I2C_MasterStart(I2C_Type *base, uint8_t address, i2c_direction_t direction);

/*!
 * @brief Sends a STOP signal on the I2C bus.
 *
 * @retval kStatus_Success Successfully send the stop signal.
 * @retval kStatus_I2C_Timeout Send stop signal failed, timeout.
 */
status_t I2C_MasterStop(I2C_Type *base);

/*!
 * @brief Sends a REPEATED START on the I2C bus.
 *
 * @param base I2C peripheral base pointer
 * @param address 7-bit slave device address.
 * @param direction Master transfer directions(transmit/receive).
 * @retval kStatus_Success Successfully send the start signal.
 * @retval kStatus_I2C_Busy Current bus is busy but not occupied by current I2C master.
 */
status_t I2C_MasterRepeatedStart(I2C_Type *base, uint8_t address, i2c_direction_t direction);

/*!
 * @brief Performs a polling send transaction on the I2C bus without a STOP signal.
 *
 * @param base  The I2C peripheral base pointer.
 * @param txBuff The pointer to the data to be transferred.
 * @param txSize The length in bytes of the data to be transferred.
 * @retval kStatus_Success Successfully complete the data transmission.
 * @retval kStatus_I2C_ArbitrationLost Transfer error, arbitration lost.
 * @retval kStataus_I2C_Nak Transfer error, receive NAK during transfer.
 */
status_t I2C_MasterWriteBlocking(I2C_Type *base, const uint8_t *txBuff, size_t txSize);

/*!
 * @brief Performs a polling receive transaction on the I2C bus with a STOP signal.
 *
 * @note The I2C_MasterReadBlocking function stops the bus before reading the final byte.
 * Without stopping the bus prior for the final read, the bus issues another read, resulting
 * in garbage data being read into the data register.
 *
 * @param base I2C peripheral base pointer.
 * @param rxBuff The pointer to the data to store the received data.
 * @param rxSize The length in bytes of the data to be received.
 * @retval kStatus_Success Successfully complete the data transmission.
 * @retval kStatus_I2C_Timeout Send stop signal failed, timeout.
 */
status_t I2C_MasterReadBlocking(I2C_Type *base, uint8_t *rxBuff, size_t rxSize);

/*!
 * @brief Performs a polling send transaction on the I2C bus.
 *
 * @param base  The I2C peripheral base pointer.
 * @param txBuff The pointer to the data to be transferred.
 * @param txSize The length in bytes of the data to be transferred.
 * @retval kStatus_Success Successfully complete the data transmission.
 * @retval kStatus_I2C_ArbitrationLost Transfer error, arbitration lost.
 * @retval kStataus_I2C_Nak Transfer error, receive NAK during transfer.
 */
status_t I2C_SlaveWriteBlocking(I2C_Type *base, const uint8_t *txBuff, size_t txSize);

/*!
 * @brief Performs a polling receive transaction on the I2C bus.
 *
 * @param base I2C peripheral base pointer.
 * @param rxBuff The pointer to the data to store the received data.
 * @param rxSize The length in bytes of the data to be received.
 */
void I2C_SlaveReadBlocking(I2C_Type *base, uint8_t *rxBuff, size_t rxSize);

/*!
 * @brief Performs a master polling transfer on the I2C bus.
 *
 * @note The API does not return until the transfer succeeds or fails due
 * to arbitration lost or receiving a NAK.
 *
 * @param base I2C peripheral base address.
 * @param xfer Pointer to the transfer structure.
 * @retval kStatus_Success Successfully complete the data transmission.
 * @retval kStatus_I2C_Busy Previous transmission still not finished.
 * @retval kStatus_I2C_Timeout Transfer error, wait signal timeout.
 * @retval kStatus_I2C_ArbitrationLost Transfer error, arbitration lost.
 * @retval kStataus_I2C_Nak Transfer error, receive NAK during transfer.
 */
status_t I2C_MasterTransferBlocking(I2C_Type *base, i2c_master_transfer_t *xfer);

/* @} */

/*!
 * @name Transactional
 * @{
 */

/*!
 * @brief Initializes the I2C handle which is used in transactional functions.
 *
 * @param base I2C base pointer.
 * @param handle pointer to i2c_master_handle_t structure to store the transfer state.
 * @param callback pointer to user callback function.
 * @param userData user paramater passed to the callback function.
 */
void I2C_MasterTransferCreateHandle(I2C_Type *base,
                                    i2c_master_handle_t *handle,
                                    i2c_master_transfer_callback_t callback,
                                    void *userData);

/*!
 * @brief Performs a master interrupt non-blocking transfer on the I2C bus.
 *
 * @note Calling the API will return immediately after transfer initiates, user needs
 * to call I2C_MasterGetTransferCount to poll the transfer status to check whether
 * the transfer is finished, if the return status is not kStatus_I2C_Busy, the transfer
 * is finished.
 *
 * @param base I2C base pointer.
 * @param handle pointer to i2c_master_handle_t structure which stores the transfer state.
 * @param xfer pointer to i2c_master_transfer_t structure.
 * @retval kStatus_Success Sucessully start the data transmission.
 * @retval kStatus_I2C_Busy Previous transmission still not finished.
 * @retval kStatus_I2C_Timeout Transfer error, wait signal timeout.
 */
status_t I2C_MasterTransferNonBlocking(I2C_Type *base, i2c_master_handle_t *handle, i2c_master_transfer_t *xfer);

/*!
 * @brief Gets the master transfer status during a interrupt non-blocking transfer.
 *
 * @param base I2C base pointer.
 * @param handle pointer to i2c_master_handle_t structure which stores the transfer state.
 * @param count Number of bytes transferred so far by the non-blocking transaction.
 * @retval kStatus_InvalidArgument count is Invalid.
 * @retval kStatus_Success Successfully return the count.
 */
status_t I2C_MasterTransferGetCount(I2C_Type *base, i2c_master_handle_t *handle, size_t *count);

/*!
 * @brief Aborts an interrupt non-blocking transfer early.
 *
 * @note This API can be called at any time when an interrupt non-blocking transfer initiates
 * to abort the transfer early.
 *
 * @param base I2C base pointer.
 * @param handle pointer to i2c_master_handle_t structure which stores the transfer state
 */
void I2C_MasterTransferAbort(I2C_Type *base, i2c_master_handle_t *handle);

/*!
 * @brief Master interrupt handler.
 *
 * @param base I2C base pointer.
 * @param i2cHandle pointer to i2c_master_handle_t structure.
 */
void I2C_MasterTransferHandleIRQ(I2C_Type *base, void *i2cHandle);

/*!
 * @brief Initializes the I2C handle which is used in transactional functions.
 *
 * @param base I2C base pointer.
 * @param handle pointer to i2c_slave_handle_t structure to store the transfer state.
 * @param callback pointer to user callback function.
 * @param userData user parameter passed to the callback function.
 */
void I2C_SlaveTransferCreateHandle(I2C_Type *base,
                                   i2c_slave_handle_t *handle,
                                   i2c_slave_transfer_callback_t callback,
                                   void *userData);

/*!
 * @brief Starts accepting slave transfers.
 *
 * Call this API after calling the I2C_SlaveInit() and I2C_SlaveTransferCreateHandle() to start processing
 * transactions driven by an I2C master. The slave monitors the I2C bus and passes events to the
 * callback that was passed into the call to I2C_SlaveTransferCreateHandle(). The callback is always invoked
 * from the interrupt context.
 *
 * The set of events received by the callback is customizable. To do so, set the @a eventMask parameter to
 * the OR'd combination of #i2c_slave_transfer_event_t enumerators for the events you wish to receive.
 * The #kI2C_SlaveTransmitEvent and #kLPI2C_SlaveReceiveEvent events are always enabled and do not need
 * to be included in the mask. Alternatively, pass 0 to get a default set of only the transmit and
 * receive events that are always enabled. In addition, the #kI2C_SlaveAllEvents constant is provided as
 * a convenient way to enable all events.
 *
 * @param base The I2C peripheral base address.
 * @param handle Pointer to #i2c_slave_handle_t structure which stores the transfer state.
 * @param eventMask Bit mask formed by OR'ing together #i2c_slave_transfer_event_t enumerators to specify
 *      which events to send to the callback. Other accepted values are 0 to get a default set of
 *      only the transmit and receive events, and #kI2C_SlaveAllEvents to enable all events.
 *
 * @retval #kStatus_Success Slave transfers were successfully started.
 * @retval #kStatus_I2C_Busy Slave transfers have already been started on this handle.
 */
status_t I2C_SlaveTransferNonBlocking(I2C_Type *base, i2c_slave_handle_t *handle, uint32_t eventMask);

/*!
 * @brief Aborts the slave transfer.
 *
 * @note This API can be called at any time to stop slave for handling the bus events.
 *
 * @param base I2C base pointer.
 * @param handle pointer to i2c_slave_handle_t structure which stores the transfer state.
 */
void I2C_SlaveTransferAbort(I2C_Type *base, i2c_slave_handle_t *handle);

/*!
 * @brief Gets the slave transfer remaining bytes during a interrupt non-blocking transfer.
 *
 * @param base I2C base pointer.
 * @param handle pointer to i2c_slave_handle_t structure.
 * @param count Number of bytes transferred so far by the non-blocking transaction.
 * @retval kStatus_InvalidArgument count is Invalid.
 * @retval kStatus_Success Successfully return the count.
 */
status_t I2C_SlaveTransferGetCount(I2C_Type *base, i2c_slave_handle_t *handle, size_t *count);

/*!
 * @brief Slave interrupt handler.
 *
 * @param base I2C base pointer.
 * @param i2cHandle pointer to i2c_slave_handle_t structure which stores the transfer state
 */
void I2C_SlaveTransferHandleIRQ(I2C_Type *base, void *i2cHandle);

/* @} */
#if defined(__cplusplus)
}
#endif /*_cplusplus. */
/*@}*/

#endif /* _FSL_I2C_H_*/
