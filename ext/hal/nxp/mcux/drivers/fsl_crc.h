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

#ifndef _FSL_CRC_H_
#define _FSL_CRC_H_

#include "fsl_common.h"

/*!
 * @addtogroup crc_driver
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief CRC driver version. Version 2.0.0. */
#define FSL_CRC_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*! @internal @brief Has data register with name CRC. */
#if defined(FSL_FEATURE_CRC_HAS_CRC_REG) && FSL_FEATURE_CRC_HAS_CRC_REG
#define DATA CRC
#define DATALL CRCLL
#endif

#ifndef CRC_DRIVER_CUSTOM_DEFAULTS
/*! @brief Default configuration structure filled by CRC_GetDefaultConfig(). Use CRC16-CCIT-FALSE as defeault. */
#define CRC_DRIVER_USE_CRC16_CCIT_FALSE_AS_DEFAULT 1
#endif

/*! @brief CRC bit width */
typedef enum _crc_bits
{
    kCrcBits16 = 0U, /*!< Generate 16-bit CRC code  */
    kCrcBits32 = 1U  /*!< Generate 32-bit CRC code  */
} crc_bits_t;

/*! @brief CRC result type */
typedef enum _crc_result
{
    kCrcFinalChecksum = 0U,       /*!< CRC data register read value is the final checksum.
                                      Reflect out and final xor protocol features are applied. */
    kCrcIntermediateChecksum = 1U /*!< CRC data register read value is intermediate checksum (raw value).
                                      Reflect out and final xor protocol feature are not applied.
                                      Intermediate checksum can be used as a seed for CRC_Init()
                                      to continue adding data to this checksum. */
} crc_result_t;

/*!
* @brief CRC protocol configuration.
*
* This structure holds the configuration for the CRC protocol.
*
*/
typedef struct _crc_config
{
    uint32_t polynomial;     /*!< CRC Polynomial, MSBit first.
                                  Example polynomial: 0x1021 = 1_0000_0010_0001 = x^12+x^5+1 */
    uint32_t seed;           /*!< Starting checksum value */
    bool reflectIn;          /*!< Reflect bits on input. */
    bool reflectOut;         /*!< Reflect bits on output. */
    bool complementChecksum; /*!< True if the result shall be complement of the actual checksum. */
    crc_bits_t crcBits;      /*!< Selects 16- or 32- bit CRC protocol. */
    crc_result_t crcResult;  /*!< Selects final or intermediate checksum return from CRC_Get16bitResult() or
                                CRC_Get32bitResult() */
} crc_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Enables and configures the CRC peripheral module.
 *
 * This functions enables the clock gate in the Kinetis SIM module for the CRC peripheral.
 * It also configures the CRC module and starts checksum computation by writing the seed.
 *
 * @param base CRC peripheral address.
 * @param config CRC module configuration structure
 */
void CRC_Init(CRC_Type *base, const crc_config_t *config);

/*!
 * @brief Disables the CRC peripheral module.
 *
 * This functions disables the clock gate in the Kinetis SIM module for the CRC peripheral.
 *
 * @param base CRC peripheral address.
 */
static inline void CRC_Deinit(CRC_Type *base)
{
    /* gate clock */
    CLOCK_DisableClock(kCLOCK_Crc0);
}

/*!
 * @brief Loads default values to CRC protocol configuration structure.
 *
 * Loads default values to CRC protocol configuration structure. The default values are:
 * @code
 *   config->polynomial = 0x1021;
 *   config->seed = 0xFFFF;
 *   config->reflectIn = false;
 *   config->reflectOut = false;
 *   config->complementChecksum = false;
 *   config->crcBits = kCrcBits16;
 *   config->crcResult = kCrcFinalChecksum;
 * @endcode
 *
 * @param config CRC protocol configuration structure
 */
void CRC_GetDefaultConfig(crc_config_t *config);

/*!
 * @brief Writes data to the CRC module.
 *
 * Writes input data buffer bytes to CRC data register.
 * The configured type of transpose is applied.
 *
 * @param base CRC peripheral address.
 * @param data Input data stream, MSByte in data[0].
 * @param dataSize Size in bytes of the input data buffer.
 */
void CRC_WriteData(CRC_Type *base, const uint8_t *data, size_t dataSize);

/*!
 * @brief Reads 32-bit checksum from the CRC module.
 *
 * Reads CRC data register (intermediate or final checksum).
 * The configured type of transpose and complement are applied.
 *
 * @param base CRC peripheral address.
 * @return intermediate or final 32-bit checksum, after configured transpose and complement operations.
 */
static inline uint32_t CRC_Get32bitResult(CRC_Type *base)
{
    return base->DATA;
}

/*!
 * @brief Reads 16-bit checksum from the CRC module.
 *
 * Reads CRC data register (intermediate or final checksum).
 * The configured type of transpose and complement are applied.
 *
 * @param base CRC peripheral address.
 * @return intermediate or final 16-bit checksum, after configured transpose and complement operations.
 */
uint16_t CRC_Get16bitResult(CRC_Type *base);

#if defined(__cplusplus)
}
#endif

/*!
 *@}
 */

#endif /* _FSL_CRC_H_ */
