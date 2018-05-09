/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
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
#ifndef _FSL_COP_H_
#define _FSL_COP_H_

#include "fsl_common.h"

/*!
 * @addtogroup cop
 * @{
 */


/*******************************************************************************
 * Definitions
 *******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief COP driver version 2.0.0. */
#define FSL_COP_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*! @name COP refresh sequence. */
/*@{*/
#define COP_FIRST_BYTE_OF_REFRESH (0x55U)  /*!< First byte of refresh sequence */
#define COP_SECOND_BYTE_OF_REFRESH (0xAAU) /*!< Second byte of refresh sequence */
/*@}*/

/*! @brief COP clock source selection. */
typedef enum _cop_clock_source
{
    kCOP_LpoClock = 0U, /*!< COP clock sourced from LPO */
#if defined(FSL_FEATURE_COP_HAS_MORE_CLKSRC) && FSL_FEATURE_COP_HAS_MORE_CLKSRC
    kCOP_McgIrClock = 1U, /*!< COP clock sourced from MCGIRCLK */
    kCOP_OscErClock = 2U, /*!< COP clock sourced from OSCERCLK */
#endif                    /* FSL_FEATURE_COP_HAS_MORE_CLKSRC */
    kCOP_BusClock = 3U,   /*!< COP clock sourced from Bus clock */
} cop_clock_source_t;

/*! @brief Define the COP timeout cycles. */
typedef enum _cop_timeout_cycles
{
    kCOP_2Power5CyclesOr2Power13Cycles = 1U,  /*!< 2^5 or 2^13 clock cycles */
    kCOP_2Power8CyclesOr2Power16Cycles = 2U,  /*!< 2^8 or 2^16 clock cycles */
    kCOP_2Power10CyclesOr2Power18Cycles = 3U, /*!< 2^10 or 2^18 clock cycles */
} cop_timeout_cycles_t;

#if defined(FSL_FEATURE_COP_HAS_LONGTIME_MODE) && FSL_FEATURE_COP_HAS_LONGTIME_MODE
/*! @breif Define the COP timeout mode. */
typedef enum _cop_timeout_mode
{
    kCOP_ShortTimeoutMode = 0U, /*!< COP selects long timeout */
    kCOP_LongTimeoutMode = 1U,  /*!< COP selects short timeout */
} cop_timeout_mode_t;
#endif /* FSL_FEATURE_COP_HAS_LONGTIME_MODE */

/*! @brief Describes COP configuration structure. */
typedef struct _cop_config
{
    bool enableWindowMode; /*!< COP run mode: window mode or normal mode */
#if defined(FSL_FEATURE_COP_HAS_LONGTIME_MODE) && FSL_FEATURE_COP_HAS_LONGTIME_MODE
    cop_timeout_mode_t timeoutMode;     /*!< COP timeout mode: long timeout or short timeout */
    bool enableStop;                    /*!< Enable or disable COP in STOP mode */
    bool enableDebug;                   /*!< Enable or disable COP in DEBUG mode */
#endif                                  /* FSL_FEATURE_COP_HAS_LONGTIME_MODE */
    cop_clock_source_t clockSource;     /*!< Set COP clock source */
    cop_timeout_cycles_t timeoutCycles; /*!< Set COP timeout value */
} cop_config_t;

/*******************************************************************************
 * API
 *******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*!
 * @name COP Functional Operation
 * @{
 */

/*!
 * @brief Initializes the COP configuration structure.
 *
 * This function initializes the COP configuration structure to default values. The default
 * values are:
 * @code
 *   copConfig->enableWindowMode = false;
 *   copConfig->timeoutMode = kCOP_LongTimeoutMode;
 *   copConfig->enableStop = false;
 *   copConfig->enableDebug = false;
 *   copConfig->clockSource = kCOP_LpoClock;
 *   copConfig->timeoutCycles = kCOP_2Power10CyclesOr2Power18Cycles;
 * @endcode
 *
 * @param config Pointer to the COP configuration structure.
 * @see cop_config_t
 */
void COP_GetDefaultConfig(cop_config_t *config);

/*!
 * @brief Initializes the COP module.
 *
 * This function configures the COP. After it is called, the COP
 * starts running according to the configuration.
 * Because all COP control registers are write-once only, the COP_Init function
 * and the COP_Disable function can be called only once. A second call has no effect.
 *
 * Example:
 * @code
 *  cop_config_t config;
 *  COP_GetDefaultConfig(&config);
 *  config.timeoutCycles = kCOP_2Power8CyclesOr2Power16Cycles;
 *  COP_Init(sim_base,&config);
 * @endcode
 *
 * @param base   SIM peripheral base address.
 * @param config The configuration of COP.
 */
void COP_Init(SIM_Type *base, const cop_config_t *config);

/*!
 * @brief De-initializes the COP module.
 * This dedicated function is not provided. Instead, the COP_Disable function can be used to disable the COP.
 */

/*!
 * @brief Disables the COP module.
 *
 * This function disables the COP Watchdog.
 * Note: The COP configuration register is a write-once after reset.
 * To disable the COP Watchdog, call this function first.
 *
 * @param base  SIM peripheral base address.
 */
static inline void COP_Disable(SIM_Type *base)
{
    base->COPC &= ~SIM_COPC_COPT_MASK;
}

/*!
 * @brief Refreshes the COP timer
 *
 * This function feeds the COP.
 *
 * @param base  SIM peripheral base address.
 */
void COP_Refresh(SIM_Type *base);

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_COP_H_ */
