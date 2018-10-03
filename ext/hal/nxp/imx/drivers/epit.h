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

#ifndef __EPIT_H__
#define __EPIT_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup epit_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief Clock source. */
enum _epit_clock_source
{
    epitClockSourceOff      = 0U, /*!< EPIT Clock Source Off.*/
    epitClockSourcePeriph   = 1U, /*!< EPIT Clock Source from Peripheral Clock.*/
    epitClockSourceHighFreq = 2U, /*!< EPIT Clock Source from High Frequency Reference Clock.*/
    epitClockSourceLowFreq  = 3U, /*!< EPIT Clock Source from Low Frequency Reference Clock.*/
};

/*! @brief Output compare operation mode. */
enum _epit_output_operation_mode
{
    epitOutputOperationDisconnected = 0U, /*!< EPIT Output Operation: Disconnected from pad.*/
    epitOutputOperationToggle       = 1U, /*!< EPIT Output Operation: Toggle output pin.*/
    epitOutputOperationClear        = 2U, /*!< EPIT Output Operation: Clear output pin.*/
    epitOutputOperationSet          = 3U, /*!< EPIT Output Operation: Set putput pin.*/
};

/*! @brief Structure to configure the running mode. */
typedef struct _epit_init_config
{
    bool freeRun;    /*!< true: set-and-forget mode, false: free-running mode. */
    bool waitEnable; /*!< EPIT enabled in wait mode. */
    bool stopEnable; /*!< EPIT enabled in stop mode. */
    bool dbgEnable;  /*!< EPIT enabled in debug mode. */
    bool enableMode; /*!< true: counter starts counting from load value (RLD=1) or 0xFFFF_FFFF (If RLD=0) when enabled,
	                      false: counter restores the value that it was disabled when enabled. */
} epit_init_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name EPIT State Control
 * @{
 */

/*!
 * @brief Initialize EPIT to reset state and initialize running mode.
 *
 * @param base EPIT base pointer.
 * @param initConfig EPIT initialize structure.
 */
void EPIT_Init(EPIT_Type* base, const epit_init_config_t* initConfig);

/*!
 * @brief Software reset of EPIT module.
 *
 * @param base EPIT base pointer.
 */
static inline void EPIT_SoftReset(EPIT_Type* base)
{
    EPIT_CR_REG(base) |= EPIT_CR_SWR_MASK;
    /* Wait reset finished. */
    while (EPIT_CR_REG(base) & EPIT_CR_SWR_MASK) { }
}

/*!
 * @brief Set clock source of EPIT.
 *
 * @param base EPIT base pointer.
 * @param source clock source (see @ref _epit_clock_source enumeration).
 */
static inline void EPIT_SetClockSource(EPIT_Type* base, uint32_t source)
{
    EPIT_CR_REG(base) = (EPIT_CR_REG(base) & ~EPIT_CR_CLKSRC_MASK) | EPIT_CR_CLKSRC(source);
}

/*!
 * @brief Get clock source of EPIT.
 *
 * @param base EPIT base pointer.
 * @return clock source (see @ref _epit_clock_source enumeration).
 */
static inline uint32_t EPIT_GetClockSource(EPIT_Type* base)
{
    return (EPIT_CR_REG(base) & EPIT_CR_CLKSRC_MASK) >> EPIT_CR_CLKSRC_SHIFT;
}

/*!
 * @brief Set pre scaler of EPIT.
 *
 * @param base EPIT base pointer.
 * @param prescaler Pre-scaler of EPIT (0-4095, divider = prescaler + 1).
 */
static inline void EPIT_SetPrescaler(EPIT_Type* base, uint32_t prescaler)
{
    assert(prescaler <= (EPIT_CR_PRESCALAR_MASK >> EPIT_CR_PRESCALAR_SHIFT));
    EPIT_CR_REG(base) = (EPIT_CR_REG(base) & ~EPIT_CR_PRESCALAR_MASK) | EPIT_CR_PRESCALAR(prescaler);
}

/*!
 * @brief Get pre scaler of EPIT.
 *
 * @param base EPIT base pointer.
 * @return Pre-scaler of EPIT (0-4095).
 */
static inline uint32_t EPIT_GetPrescaler(EPIT_Type* base)
{
    return (EPIT_CR_REG(base) & EPIT_CR_PRESCALAR_MASK) >> EPIT_CR_PRESCALAR_SHIFT;
}

/*!
 * @brief Enable EPIT module.
 *
 * @param base EPIT base pointer.
 */
static inline void EPIT_Enable(EPIT_Type* base)
{
    EPIT_CR_REG(base) |= EPIT_CR_EN_MASK;
}

/*!
 * @brief Disable EPIT module.
 *
 * @param base EPIT base pointer.
 */
static inline void EPIT_Disable(EPIT_Type* base)
{
    EPIT_CR_REG(base) &= ~EPIT_CR_EN_MASK;
}

/*!
 * @brief Get EPIT counter value.
 *
 * @param base EPIT base pointer.
 * @return EPIT counter value.
 */
static inline uint32_t EPIT_ReadCounter(EPIT_Type* base)
{
    return EPIT_CNR_REG(base);
}

/*@}*/

/*!
 * @name EPIT Output Signal Control
 * @{
 */

/*!
 * @brief Set EPIT output compare operation mode.
 *
 * @param base EPIT base pointer.
 * @param mode EPIT output compare operation mode (see @ref _epit_output_operation_mode enumeration).
 */
static inline void EPIT_SetOutputOperationMode(EPIT_Type* base, uint32_t mode)
{
    EPIT_CR_REG(base) = (EPIT_CR_REG(base) & ~EPIT_CR_OM_MASK) | EPIT_CR_OM(mode);
}

/*!
 * @brief Get EPIT output compare operation mode.
 *
 * @param base EPIT base pointer.
 * @return EPIT output operation mode (see @ref _epit_output_operation_mode enumeration).
 */
static inline uint32_t EPIT_GetOutputOperationMode(EPIT_Type* base)
{
    return (EPIT_CR_REG(base) & EPIT_CR_OM_MASK) >> EPIT_CR_OM_SHIFT;
}

/*!
 * @brief Set EPIT output compare value.
 *
 * @param base EPIT base pointer.
 * @param value EPIT output compare value.
 */
static inline void EPIT_SetOutputCompareValue(EPIT_Type* base, uint32_t value)
{
    EPIT_CMPR_REG(base) = value;
}

/*!
 * @brief Get EPIT output compare value.
 *
 * @param base EPIT base pointer.
 * @return EPIT output compare value.
 */
static inline uint32_t EPIT_GetOutputCompareValue(EPIT_Type* base)
{
    return EPIT_CMPR_REG(base);
}

/*@}*/

/*!
 * @name EPIT Data Load Control
 * @{
 */

/*!
 * @brief Set the value that is to be loaded into counter register.
 *
 * @param base EPIT base pointer.
 * @param value Counter load value.
 */
static inline void EPIT_SetCounterLoadValue(EPIT_Type* base, uint32_t value)
{
    EPIT_LR_REG(base) = value;
}

/*!
 * @brief Get the value that loaded into counter register.
 *
 * @param base EPIT base pointer.
 * @return The counter load value.
 */
static inline uint32_t EPIT_GetCounterLoadValue(EPIT_Type* base)
{
    return EPIT_LR_REG(base);
}

/*!
 * @brief Enable or disable EPIT overwrite counter immediately.
 *
 * @param base EPIT base pointer.
 * @param enable Enable/Disable EPIT overwrite counter immediately.
 *               - true: Enable overwrite counter immediately.
 *               - false: Disable overwrite counter immediately.
 */
void EPIT_SetOverwriteCounter(EPIT_Type* base, bool enable);

/*@}*/

/*!
 * @name EPIT Interrupt and Status Control
 * @{
 */

/*!
 * @brief Get EPIT status of output compare interrupt flag.
 *
 * @param base EPIT base pointer.
 * @return EPIT status of output compare interrupt flag.
 */
static inline uint32_t EPIT_GetStatusFlag(EPIT_Type* base)
{
    return EPIT_SR_REG(base) & EPIT_SR_OCIF_MASK;
}

/*!
 * @brief Clear EPIT Output compare interrupt flag.
 *
 * @param base EPIT base pointer.
 */
static inline void EPIT_ClearStatusFlag(EPIT_Type* base)
{
    EPIT_SR_REG(base) = EPIT_SR_OCIF_MASK;
}

/*!
 * @brief Enable or disable EPIT interrupt.
 *
 * @param base EPIT base pointer.
 * @param enable Enable/Disable EPIT interrupt.
 *        - true: Enable interrupt.
 *        - false: Disable interrupt.
 */
void EPIT_SetIntCmd(EPIT_Type* base, bool enable);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /*__EPIT_H__*/

/*******************************************************************************
 * EOF
 ******************************************************************************/
