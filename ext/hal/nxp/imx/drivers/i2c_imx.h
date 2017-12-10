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

#ifndef __I2C_IMX_H__
#define __I2C_IMX_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup i2c_imx_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief I2C module initialization structure. */
typedef struct _i2c_init_config
{
    uint32_t clockRate;    /*!< Current I2C module clock freq. */
    uint32_t baudRate;     /*!< Desired I2C baud rate. */
    uint8_t  slaveAddress; /*!< I2C module's own address when addressed as slave device. */
} i2c_init_config_t;

/*! @brief Flag for I2C interrupt status check or polling status. */
enum _i2c_status_flag
{
    i2cStatusTransferComplete = I2C_I2SR_ICF_MASK,  /*!< Data Transfer complete flag. */
    i2cStatusAddressedAsSlave = I2C_I2SR_IAAS_MASK, /*!< Addressed as a slave flag. */
    i2cStatusBusBusy          = I2C_I2SR_IBB_MASK,  /*!< Bus is busy flag. */
    i2cStatusArbitrationLost  = I2C_I2SR_IAL_MASK,  /*!< Arbitration is lost flag. */
    i2cStatusSlaveReadWrite   = I2C_I2SR_SRW_MASK,  /*!< Master reading from slave flag(De-assert if master writing to slave). */
    i2cStatusInterrupt        = I2C_I2SR_IIF_MASK,  /*!< An interrupt is pending flag. */
    i2cStatusReceivedAck      = I2C_I2SR_RXAK_MASK, /*!< No acknowledge detected flag. */
};

/*! @brief I2C Bus role of this module. */
enum _i2c_work_mode
{
    i2cModeSlave  = 0x0,                /*!< This module works as I2C Slave. */
    i2cModeMaster = I2C_I2CR_MSTA_MASK, /*!< This module works as I2C Master. */
};

/*! @brief Data transfer direction. */
enum _i2c_direction_mode
{
    i2cDirectionReceive  = 0x0,               /*!< This module works at receive mode. */
    i2cDirectionTransmit = I2C_I2CR_MTX_MASK, /*!< This module works at transmit mode. */
};

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name I2C Initialization and Configuration functions
 * @{
 */

/*!
 * @brief Initialize I2C module with given initialization structure.
 *
 * @param base I2C base pointer.
 * @param initConfig I2C initialization structure (see @ref i2c_init_config_t).
 */
void I2C_Init(I2C_Type* base, const i2c_init_config_t* initConfig);

/*!
 * @brief This function reset I2C module register content to its default value.
 *
 * @param base I2C base pointer.
 */
void I2C_Deinit(I2C_Type* base);

/*!
 * @brief This function is used to Enable the I2C Module.
 *
 * @param base I2C base pointer.
 */
static inline void I2C_Enable(I2C_Type* base)
{
    I2C_I2CR_REG(base) |= I2C_I2CR_IEN_MASK;
}

/*!
 * @brief This function is used to Disable the I2C Module.
 *
 * @param base I2C base pointer.
 */
static inline void I2C_Disable(I2C_Type* base)
{
    I2C_I2CR_REG(base) &= ~I2C_I2CR_IEN_MASK;
}

/*!
 * @brief This function is used to set the baud rate of I2C Module.
 *
 * @param base I2C base pointer.
 * @param clockRate I2C module clock frequency.
 * @param baudRate Desired I2C module baud rate.
 */
void I2C_SetBaudRate(I2C_Type* base, uint32_t clockRate, uint32_t baudRate);

/*!
 * @brief This function is used to set the own I2C bus address when addressed as a slave.
 *
 * @param base I2C base pointer.
 * @param slaveAddress Own I2C Bus address.
 */
static inline void I2C_SetSlaveAddress(I2C_Type* base, uint8_t slaveAddress)
{
    assert(slaveAddress < 0x80);

    I2C_IADR_REG(base) = (I2C_IADR_REG(base) & ~I2C_IADR_ADR_MASK) | I2C_IADR_ADR(slaveAddress);
}

/*!
 * @name I2C Bus Control functions
 * @{
 */

/*!
 * @brief This function is used to Generate a Repeat Start Signal on I2C Bus.
 *
 * @param base I2C base pointer.
 */
static inline void I2C_SendRepeatStart(I2C_Type* base)
{
    I2C_I2CR_REG(base) |= I2C_I2CR_RSTA_MASK;
}

/*!
 * @brief This function is used to select the I2C bus role of this module,
 *        both I2C Bus Master and Slave can be select.
 *
 * @param base I2C base pointer.
 * @param mode I2C Bus role to set (see @ref _i2c_work_mode enumeration).
 */
static inline void I2C_SetWorkMode(I2C_Type* base, uint32_t mode)
{
    assert((mode == i2cModeMaster) || (mode == i2cModeSlave));

    I2C_I2CR_REG(base) = (I2C_I2CR_REG(base) & ~I2C_I2CR_MSTA_MASK) | mode;
}

/*!
 * @brief This function is used to select the data transfer direction of this module,
 *        both Transmit and Receive can be select.
 *
 * @param base I2C base pointer.
 * @param direction I2C Bus data transfer direction (see @ref _i2c_direction_mode enumeration).
 */
static inline void I2C_SetDirMode(I2C_Type* base, uint32_t direction)
{
    assert((direction == i2cDirectionReceive) || (direction == i2cDirectionTransmit));

    I2C_I2CR_REG(base) = (I2C_I2CR_REG(base) & ~I2C_I2CR_MTX_MASK) | direction;
}

/*!
 * @brief This function is used to set the Transmit Acknowledge action when receive
 *        data from other device.
 *
 * @param base I2C base pointer.
 * @param ack The ACK value answerback to remote I2C device.
 *            - true: An acknowledge signal is sent to the bus at the ninth clock bit.
 *            - false: No acknowledge signal response is sent.
 */
void I2C_SetAckBit(I2C_Type* base, bool ack);

/*!
 * @name Data transfers functions
 * @{
 */

/*!
 * @brief Writes one byte of data to the I2C bus.
 *
 * @param base I2C base pointer.
 * @param byte The byte of data to transmit.
 */
static inline void I2C_WriteByte(I2C_Type* base, uint8_t byte)
{
    I2C_I2DR_REG(base) = byte;
}

/*!
 * @brief Returns the last byte of data read from the bus and initiate another read.
 *
 * In a master receive mode, calling this function initiates receiving the next byte of data.
 *
 * @param base I2C base pointer.
 * @return This function returns the last byte received while the I2C module is configured in master
 *         receive or slave receive mode.
 */
static inline uint8_t I2C_ReadByte(I2C_Type* base)
{
    return (uint8_t)(I2C_I2DR_REG(base) & I2C_I2DR_DATA_MASK);
}

/*!
 * @name Interrupts and flags management functions
 * @{
 */

/*!
 * @brief Enable or disable I2C interrupt requests.
 *
 * @param base I2C base pointer.
 * @param enable Enable/Disbale I2C interrupt.
 *               - true: Enable I2C interrupt.
 *               - false: Disable I2C interrupt.
 */
void I2C_SetIntCmd(I2C_Type* base, bool enable);

/*!
 * @brief Gets the I2C status flag state.
 *
 * @param base I2C base pointer.
 * @param flags I2C status flag mask (see @ref _i2c_status_flag enumeration.)
 * @return I2C status, each bit represents one status flag
 */
static inline uint32_t I2C_GetStatusFlag(I2C_Type* base, uint32_t flags)
{
    return (I2C_I2SR_REG(base) & flags);
}

/*!
 * @brief Clear one or more I2C status flag state.
 *
 * @param base I2C base pointer.
 * @param flags I2C status flag mask (see @ref _i2c_status_flag enumeration.)
 */
static inline void I2C_ClearStatusFlag(I2C_Type* base, uint32_t flags)
{
    /* Write 0 to clear. */
    I2C_I2SR_REG(base) &= ~flags;
}

#ifdef __cplusplus
}
#endif

/*! @}*/

#endif /* __I2C_IMX_H__ */
/*******************************************************************************
 * EOF
 ******************************************************************************/
