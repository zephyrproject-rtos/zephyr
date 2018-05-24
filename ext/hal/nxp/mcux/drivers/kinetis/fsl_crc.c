/*
 * The Clear BSD License
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted (subject to the limitations in the disclaimer below) provided
 * that the following conditions are met:
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
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY THIS LICENSE.
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
#include "fsl_crc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
/*! @internal @brief Has data register with name CRC. */
#if defined(FSL_FEATURE_CRC_HAS_CRC_REG) && FSL_FEATURE_CRC_HAS_CRC_REG
#define DATA CRC
#define DATALL CRCLL
#endif

#if defined(CRC_DRIVER_USE_CRC16_CCIT_FALSE_AS_DEFAULT) && CRC_DRIVER_USE_CRC16_CCIT_FALSE_AS_DEFAULT
/* @brief Default user configuration structure for CRC-16-CCITT */
#define CRC_DRIVER_DEFAULT_POLYNOMIAL 0x1021U
/*< CRC-16-CCIT polynomial x**16 + x**12 + x**5 + x**0 */
#define CRC_DRIVER_DEFAULT_SEED 0xFFFFU
/*< Default initial checksum */
#define CRC_DRIVER_DEFAULT_REFLECT_IN false
/*< Default is no transpose */
#define CRC_DRIVER_DEFAULT_REFLECT_OUT false
/*< Default is transpose bytes */
#define CRC_DRIVER_DEFAULT_COMPLEMENT_CHECKSUM false
/*< Default is without complement of CRC data register read data */
#define CRC_DRIVER_DEFAULT_CRC_BITS kCrcBits16
/*< Default is 16-bit CRC protocol */
#define CRC_DRIVER_DEFAULT_CRC_RESULT kCrcFinalChecksum
/*< Default is resutl type is final checksum */
#endif /* CRC_DRIVER_USE_CRC16_CCIT_FALSE_AS_DEFAULT */

/*! @brief CRC type of transpose of read write data */
typedef enum _crc_transpose_type
{
    kCrcTransposeNone = 0U,         /*! No transpose  */
    kCrcTransposeBits = 1U,         /*! Tranpose bits in bytes  */
    kCrcTransposeBitsAndBytes = 2U, /*! Transpose bytes and bits in bytes */
    kCrcTransposeBytes = 3U,        /*! Transpose bytes */
} crc_transpose_type_t;

/*!
* @brief CRC module configuration.
*
* This structure holds the configuration for the CRC module.
*/
typedef struct _crc_module_config
{
    uint32_t polynomial;                 /*!< CRC Polynomial, MSBit first.@n
                                              Example polynomial: 0x1021 = 1_0000_0010_0001 = x^12+x^5+1 */
    uint32_t seed;                       /*!< Starting checksum value */
    crc_transpose_type_t readTranspose;  /*!< Type of transpose when reading CRC result. */
    crc_transpose_type_t writeTranspose; /*!< Type of transpose when writing CRC input data. */
    bool complementChecksum;             /*!< True if the result shall be complement of the actual checksum. */
    crc_bits_t crcBits;                  /*!< Selects 16- or 32- bit CRC protocol. */
} crc_module_config_t;

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Returns transpose type for CRC protocol reflect in parameter.
 *
 * This functions helps to set writeTranspose member of crc_config_t structure. Reflect in is CRC protocol parameter.
 *
 * @param enable True or false for the selected CRC protocol Reflect In (refin) parameter.
 */
static inline crc_transpose_type_t CRC_GetTransposeTypeFromReflectIn(bool enable)
{
    return ((enable) ? kCrcTransposeBitsAndBytes : kCrcTransposeBytes);
}

/*!
 * @brief Returns transpose type for CRC protocol reflect out parameter.
 *
 * This functions helps to set readTranspose member of crc_config_t structure. Reflect out is CRC protocol parameter.
 *
 * @param enable True or false for the selected CRC protocol Reflect Out (refout) parameter.
 */
static inline crc_transpose_type_t CRC_GetTransposeTypeFromReflectOut(bool enable)
{
    return ((enable) ? kCrcTransposeBitsAndBytes : kCrcTransposeNone);
}

/*!
 * @brief Starts checksum computation.
 *
 * Configures the CRC module for the specified CRC protocol. @n
 * Starts the checksum computation by writing the seed value
 *
 * @param base CRC peripheral address.
 * @param config Pointer to protocol configuration structure.
 */
static void CRC_ConfigureAndStart(CRC_Type *base, const crc_module_config_t *config)
{
    uint32_t crcControl;

    /* pre-compute value for CRC control registger based on user configuraton without WAS field */
    crcControl = 0 | CRC_CTRL_TOT(config->writeTranspose) | CRC_CTRL_TOTR(config->readTranspose) |
                 CRC_CTRL_FXOR(config->complementChecksum) | CRC_CTRL_TCRC(config->crcBits);

    /* make sure the control register is clear - WAS is deasserted, and protocol is set */
    base->CTRL = crcControl;

    /* write polynomial register */
    base->GPOLY = config->polynomial;

    /* write pre-computed control register value along with WAS to start checksum computation */
    base->CTRL = crcControl | CRC_CTRL_WAS(true);

    /* write seed (initial checksum) */
    base->DATA = config->seed;

    /* deassert WAS by writing pre-computed CRC control register value */
    base->CTRL = crcControl;
}

/*!
 * @brief Starts final checksum computation.
 *
 * Configures the CRC module for the specified CRC protocol. @n
 * Starts final checksum computation by writing the seed value.
 * @note CRC_Get16bitResult() or CRC_Get32bitResult() return final checksum
 *       (output reflection and xor functions are applied).
 *
 * @param base CRC peripheral address.
 * @param protocolConfig Pointer to protocol configuration structure.
 */
static void CRC_SetProtocolConfig(CRC_Type *base, const crc_config_t *protocolConfig)
{
    crc_module_config_t moduleConfig;
    /* convert protocol to CRC peripheral module configuration, prepare for final checksum */
    moduleConfig.polynomial = protocolConfig->polynomial;
    moduleConfig.seed = protocolConfig->seed;
    moduleConfig.readTranspose = CRC_GetTransposeTypeFromReflectOut(protocolConfig->reflectOut);
    moduleConfig.writeTranspose = CRC_GetTransposeTypeFromReflectIn(protocolConfig->reflectIn);
    moduleConfig.complementChecksum = protocolConfig->complementChecksum;
    moduleConfig.crcBits = protocolConfig->crcBits;

    CRC_ConfigureAndStart(base, &moduleConfig);
}

/*!
 * @brief Starts intermediate checksum computation.
 *
 * Configures the CRC module for the specified CRC protocol. @n
 * Starts intermediate checksum computation by writing the seed value.
 * @note CRC_Get16bitResult() or CRC_Get32bitResult() return intermediate checksum (raw data register value).
 *
 * @param base CRC peripheral address.
 * @param protocolConfig Pointer to protocol configuration structure.
 */
static void CRC_SetRawProtocolConfig(CRC_Type *base, const crc_config_t *protocolConfig)
{
    crc_module_config_t moduleConfig;
    /* convert protocol to CRC peripheral module configuration, prepare for intermediate checksum */
    moduleConfig.polynomial = protocolConfig->polynomial;
    moduleConfig.seed = protocolConfig->seed;
    moduleConfig.readTranspose =
        kCrcTransposeNone; /* intermediate checksum does no transpose of data register read value */
    moduleConfig.writeTranspose = CRC_GetTransposeTypeFromReflectIn(protocolConfig->reflectIn);
    moduleConfig.complementChecksum = false; /* intermediate checksum does no xor of data register read value */
    moduleConfig.crcBits = protocolConfig->crcBits;

    CRC_ConfigureAndStart(base, &moduleConfig);
}

void CRC_Init(CRC_Type *base, const crc_config_t *config)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* ungate clock */
    CLOCK_EnableClock(kCLOCK_Crc0);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
    /* configure CRC module and write the seed */
    if (config->crcResult == kCrcFinalChecksum)
    {
        CRC_SetProtocolConfig(base, config);
    }
    else
    {
        CRC_SetRawProtocolConfig(base, config);
    }
}

void CRC_GetDefaultConfig(crc_config_t *config)
{
    static const crc_config_t crc16ccit = {
        CRC_DRIVER_DEFAULT_POLYNOMIAL,          CRC_DRIVER_DEFAULT_SEED,
        CRC_DRIVER_DEFAULT_REFLECT_IN,          CRC_DRIVER_DEFAULT_REFLECT_OUT,
        CRC_DRIVER_DEFAULT_COMPLEMENT_CHECKSUM, CRC_DRIVER_DEFAULT_CRC_BITS,
        CRC_DRIVER_DEFAULT_CRC_RESULT,
    };

    *config = crc16ccit;
}

void CRC_WriteData(CRC_Type *base, const uint8_t *data, size_t dataSize)
{
    const uint32_t *data32;

    /* 8-bit reads and writes till source address is aligned 4 bytes */
    while ((dataSize) && ((uint32_t)data & 3U))
    {
        base->ACCESS8BIT.DATALL = *data;
        data++;
        dataSize--;
    }

    /* use 32-bit reads and writes as long as possible */
    data32 = (const uint32_t *)data;
    while (dataSize >= sizeof(uint32_t))
    {
        base->DATA = *data32;
        data32++;
        dataSize -= sizeof(uint32_t);
    }

    data = (const uint8_t *)data32;

    /* 8-bit reads and writes till end of data buffer */
    while (dataSize)
    {
        base->ACCESS8BIT.DATALL = *data;
        data++;
        dataSize--;
    }
}

uint32_t CRC_Get32bitResult(CRC_Type *base)
{
    return base->DATA;
}

uint16_t CRC_Get16bitResult(CRC_Type *base)
{
    uint32_t retval;
    uint32_t totr; /* type of transpose read bitfield */

    retval = base->DATA;
    totr = (base->CTRL & CRC_CTRL_TOTR_MASK) >> CRC_CTRL_TOTR_SHIFT;

    /* check transpose type to get 16-bit out of 32-bit register */
    if (totr >= 2U)
    {
        /* transpose of bytes for read is set, the result CRC is in CRC_DATA[HU:HL] */
        retval &= 0xFFFF0000U;
        retval = retval >> 16U;
    }
    else
    {
        /* no transpose of bytes for read, the result CRC is in CRC_DATA[LU:LL] */
        retval &= 0x0000FFFFU;
    }
    return (uint16_t)retval;
}
