/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "fsl_crc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.lpc_crc"
#endif

#if defined(CRC_DRIVER_USE_CRC16_CCITT_FALSE_AS_DEFAULT) && CRC_DRIVER_USE_CRC16_CCITT_FALSE_AS_DEFAULT
/* @brief Default user configuration structure for CRC-CCITT */
#define CRC_DRIVER_DEFAULT_POLYNOMIAL kCRC_Polynomial_CRC_CCITT
/*< CRC-CCIT polynomial x^16 + x^12 + x^5 + x^0 */
#define CRC_DRIVER_DEFAULT_REVERSE_IN false
/*< Default is no bit reverse */
#define CRC_DRIVER_DEFAULT_COMPLEMENT_IN false
/*< Default is without complement of written data */
#define CRC_DRIVER_DEFAULT_REVERSE_OUT false
/*< Default is no bit reverse */
#define CRC_DRIVER_DEFAULT_COMPLEMENT_OUT false
/*< Default is without complement of CRC data register read data */
#define CRC_DRIVER_DEFAULT_SEED 0xFFFFU
/*< Default initial checksum */
#endif /* CRC_DRIVER_USE_CRC16_CCITT_FALSE_AS_DEFAULT */

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * brief Enables and configures the CRC peripheral module.
 *
 * This functions enables the CRC peripheral clock in the LPC SYSCON block.
 * It also configures the CRC engine and starts checksum computation by writing the seed.
 *
 * param base   CRC peripheral address.
 * param config CRC module configuration structure.
 */
void CRC_Init(CRC_Type *base, const crc_config_t *config)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* enable clock to CRC */
    CLOCK_EnableClock(kCLOCK_Crc);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_FEATURE_CRC_HAS_NO_RESET) && FSL_FEATURE_CRC_HAS_NO_RESET)
    RESET_PeripheralReset(kCRC_RST_SHIFT_RSTn);
#endif

    /* configure CRC module and write the seed */
    base->MODE = 0 | CRC_MODE_CRC_POLY(config->polynomial) | CRC_MODE_BIT_RVS_WR(config->reverseIn) |
                 CRC_MODE_CMPL_WR(config->complementIn) | CRC_MODE_BIT_RVS_SUM(config->reverseOut) |
                 CRC_MODE_CMPL_SUM(config->complementOut);
    base->SEED = config->seed;
}

/*!
 * brief Loads default values to CRC protocol configuration structure.
 *
 * Loads default values to CRC protocol configuration structure. The default values are:
 * code
 *   config->polynomial = kCRC_Polynomial_CRC_CCITT;
 *   config->reverseIn = false;
 *   config->complementIn = false;
 *   config->reverseOut = false;
 *   config->complementOut = false;
 *   config->seed = 0xFFFFU;
 * endcode
 *
 * param config CRC protocol configuration structure
 */
void CRC_GetDefaultConfig(crc_config_t *config)
{
    /* Initializes the configure structure to zero. */
    memset(config, 0, sizeof(*config));

    static const crc_config_t default_config = {CRC_DRIVER_DEFAULT_POLYNOMIAL,     CRC_DRIVER_DEFAULT_REVERSE_IN,
                                                CRC_DRIVER_DEFAULT_COMPLEMENT_IN,  CRC_DRIVER_DEFAULT_REVERSE_OUT,
                                                CRC_DRIVER_DEFAULT_COMPLEMENT_OUT, CRC_DRIVER_DEFAULT_SEED};

    *config = default_config;
}

/*!
 * brief resets CRC peripheral module.
 *
 * param base   CRC peripheral address.
 */
void CRC_Reset(CRC_Type *base)
{
    crc_config_t config;
    CRC_GetDefaultConfig(&config);
    CRC_Init(base, &config);
}

/*!
 * brief Loads actual values configured in CRC peripheral to CRC protocol configuration structure.
 *
 * The values, including seed, can be used to resume CRC calculation later.

 * param base   CRC peripheral address.
 * param config CRC protocol configuration structure
 */
void CRC_GetConfig(CRC_Type *base, crc_config_t *config)
{
    /* extract CRC mode settings */
    uint32_t mode = base->MODE;
    config->polynomial = (crc_polynomial_t)((mode & CRC_MODE_CRC_POLY_MASK) >> CRC_MODE_CRC_POLY_SHIFT);
    config->reverseIn = (bool)(mode & CRC_MODE_BIT_RVS_WR_MASK);
    config->complementIn = (bool)(mode & CRC_MODE_CMPL_WR_MASK);
    config->reverseOut = (bool)(mode & CRC_MODE_BIT_RVS_SUM_MASK);
    config->complementOut = (bool)(mode & CRC_MODE_CMPL_SUM_MASK);

    /* reset CRC sum bit reverse and 1's complement setting, so its value can be used as a seed */
    base->MODE = mode & ~((1U << CRC_MODE_BIT_RVS_SUM_SHIFT) | (1U << CRC_MODE_CMPL_SUM_SHIFT));

    /* now we can obtain intermediate raw CRC sum value */
    config->seed = base->SUM;

    /* restore original CRC sum bit reverse and 1's complement setting */
    base->MODE = mode;
}

/*!
 * brief Writes data to the CRC module.
 *
 * Writes input data buffer bytes to CRC data register.
 *
 * param base     CRC peripheral address.
 * param data     Input data stream, MSByte in data[0].
 * param dataSize Size of the input data buffer in bytes.
 */
void CRC_WriteData(CRC_Type *base, const uint8_t *data, size_t dataSize)
{
    const uint32_t *data32;

    /* 8-bit reads and writes till source address is aligned 4 bytes */
    while ((dataSize) && ((uint32_t)data & 3U))
    {
        *((__O uint8_t *)&(base->WR_DATA)) = *data;
        data++;
        dataSize--;
    }

    /* use 32-bit reads and writes as long as possible */
    data32 = (const uint32_t *)data;
    while (dataSize >= sizeof(uint32_t))
    {
        *((__O uint32_t *)&(base->WR_DATA)) = *data32;
        data32++;
        dataSize -= sizeof(uint32_t);
    }

    data = (const uint8_t *)data32;

    /* 8-bit reads and writes till end of data buffer */
    while (dataSize)
    {
        *((__O uint8_t *)&(base->WR_DATA)) = *data;
        data++;
        dataSize--;
    }
}
