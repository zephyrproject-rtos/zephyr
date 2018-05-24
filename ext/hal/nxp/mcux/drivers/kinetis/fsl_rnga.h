/*
 * The Clear BSD License
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
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
