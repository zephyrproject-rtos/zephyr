/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_RNGA_DRIVER_H_
#define _FSL_RNGA_DRIVER_H_

#include "fsl_common.h"

#if defined(FSL_FEATURE_SOC_RNG_COUNT) && FSL_FEATURE_SOC_RNG_COUNT
/*!
 * @addtogroup rnga
 * @{
 */


/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief RNGA driver version 2.0.1. */
#define FSL_RNGA_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*! @brief RNGA working mode */
typedef enum _rnga_mode
{
    kRNGA_ModeNormal = 0U, /*!< Normal Mode. The ring-oscillator clocks are active; RNGA generates entropy
                                           (randomness) from the clocks and stores it in shift registers.*/
    kRNGA_ModeSleep = 1U,  /*!< Sleep Mode. The ring-oscillator clocks are inactive; RNGA does not generate entropy.*/
} rnga_mode_t;

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @brief Initializes the RNGA.
 *
 * This function initializes the RNGA.
 * When called, the RNGA entropy generation starts immediately.
 *
 * @param base RNGA base address
 */
void RNGA_Init(RNG_Type *base);

/*!
 * @brief Shuts down the RNGA.
 *
 * This function shuts down the RNGA.
 *
 * @param base RNGA base address
 */
void RNGA_Deinit(RNG_Type *base);

/*!
 * @brief Gets random data.
 *
 * This function gets random data from the RNGA.
 *
 * @param base RNGA base address
 * @param data pointer to user buffer to be filled by random data
 * @param data_size size of data in bytes
 * @return RNGA status
 */
status_t RNGA_GetRandomData(RNG_Type *base, void *data, size_t data_size);

/*!
 * @brief Feeds the RNGA module.
 *
 * This function inputs an entropy value that the RNGA uses to seed its
 * pseudo-random algorithm.
 *
 * @param base RNGA base address
 * @param seed input seed value
 */
void RNGA_Seed(RNG_Type *base, uint32_t seed);

/*!
 * @brief Sets the RNGA in normal mode or sleep mode.
 *
 * This function sets the RNGA in sleep mode or normal mode.
 *
 * @param base RNGA base address
 * @param mode normal mode or sleep mode
 */
void RNGA_SetMode(RNG_Type *base, rnga_mode_t mode);

/*!
 * @brief Gets the RNGA working mode.
 *
 * This function gets the RNGA working mode.
 *
 * @param base RNGA base address
 * @return normal mode or sleep mode
 */
rnga_mode_t RNGA_GetMode(RNG_Type *base);

#if defined(__cplusplus)
}
#endif

/*! @}*/

#endif /* FSL_FEATURE_SOC_RNG_COUNT */
#endif /* _FSL_RNGA_H_*/
