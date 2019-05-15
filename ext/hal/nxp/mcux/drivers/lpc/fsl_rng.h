/*
 * Copyright 2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_RNG_DRIVER_H_
#define _FSL_RNG_DRIVER_H_

#include "fsl_common.h"

/*!
 * @addtogroup rng
 * @{
 */

/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief RNG driver version. Version 2.0.0.
 *
 * Current version: 2.0.0
 *
 * Change log:
 * - Version 2.0.0
 *   - Initial version
 */
#define FSL_RNG_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Initializes the RNG.
 *
 * This function initializes the RNG.
 * When called, the RNG module and ring oscillator is enabled.
 *
 * @param base  RNG base address
 * @param userConfig    Pointer to the initialization configuration structure.
 * @return If successful, returns the kStatus_RNG_Success. Otherwise, it returns an error.
 */
void RNG_Init(RNG_Type *base);

/*!
 * @brief Shuts down the RNG.
 *
 * This function shuts down the RNG.
 *
 * @param base  RNG base address.
 */
void RNG_Deinit(RNG_Type *base);

/*!
 * @brief Gets random data.
 *
 * This function gets random data from the RNG.
 *
 * @param base  RNG base address.
 * @param data  Pointer address used to store random data.
 * @param dataSize  Size of the buffer pointed by the data parameter.
 * @return random data
 */
status_t RNG_GetRandomData(RNG_Type *base, void *data, size_t data_size);

/*!
 * @brief Returns random 32-bit number.
 *
 * This function gets random number from the RNG.
 *
 * @param base  RNG base address.
 * @return random number
 */
static inline uint32_t RNG_GetRandomWord(RNG_Type *base)
{
    return base->RANDOM_NUMBER;
}

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /*_FSL_RNG_H_*/
