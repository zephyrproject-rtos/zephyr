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
#ifndef _FSL_LLWU_H_
#define _FSL_LLWU_H_

#include "fsl_common.h"

/*! @addtogroup llwu */
/*! @{ */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief LLWU driver version 2.0.1. */
#define FSL_LLWU_DRIVER_VERSION (MAKE_VERSION(2, 0, 1))
/*@}*/

/*!
 * @brief External input pin control modes
 */
typedef enum _llwu_external_pin_mode
{
    kLLWU_ExternalPinDisable = 0U,     /*!< Pin disabled as wakeup input.           */
    kLLWU_ExternalPinRisingEdge = 1U,  /*!< Pin enabled with rising edge detection. */
    kLLWU_ExternalPinFallingEdge = 2U, /*!< Pin enabled with falling edge detection.*/
    kLLWU_ExternalPinAnyEdge = 3U      /*!< Pin enabled with any change detection.  */
} llwu_external_pin_mode_t;

/*!
 * @brief Digital filter control modes
 */
typedef enum _llwu_pin_filter_mode
{
    kLLWU_PinFilterDisable = 0U,     /*!< Filter disabled.               */
    kLLWU_PinFilterRisingEdge = 1U,  /*!< Filter positive edge detection.*/
    kLLWU_PinFilterFallingEdge = 2U, /*!< Filter negative edge detection.*/
    kLLWU_PinFilterAnyEdge = 3U      /*!< Filter any edge detection.     */
} llwu_pin_filter_mode_t;

#if (defined(FSL_FEATURE_LLWU_HAS_VERID) && FSL_FEATURE_LLWU_HAS_VERID)
/*!
 * @brief IP version ID definition.
 */
typedef struct _llwu_version_id
{
    uint16_t feature; /*!< Feature Specification Number. */
    uint8_t minor;    /*!< Minor version number.         */
    uint8_t major;    /*!< Major version number.         */
} llwu_version_id_t;
#endif /* FSL_FEATURE_LLWU_HAS_VERID */

#if (defined(FSL_FEATURE_LLWU_HAS_PARAM) && FSL_FEATURE_LLWU_HAS_PARAM)
/*!
 * @brief IP parameter definition.
 */
typedef struct _llwu_param
{
    uint8_t filters; /*!< Number of pin filter.      */
    uint8_t dmas;    /*!< Number of wakeup DMA.      */
    uint8_t modules; /*!< Number of wakeup module.   */
    uint8_t pins;    /*!< Number of wake up pin.     */
} llwu_param_t;
#endif /* FSL_FEATURE_LLWU_HAS_PARAM */

#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && FSL_FEATURE_LLWU_HAS_PIN_FILTER)
/*!
 * @brief External input pin filter control structure
 */
typedef struct _llwu_external_pin_filter_mode
{
    uint32_t pinIndex;                 /*!< Pin number  */
    llwu_pin_filter_mode_t filterMode; /*!< Filter mode */
} llwu_external_pin_filter_mode_t;
#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name Low-Leakage Wakeup Unit Control APIs
 * @{
 */

#if (defined(FSL_FEATURE_LLWU_HAS_VERID) && FSL_FEATURE_LLWU_HAS_VERID)
/*!
 * @brief Gets the LLWU version ID.
 *
 * This function gets the LLWU version ID, including major version number,
 * minor version number, and feature specification number.
 *
 * @param base LLWU peripheral base address.
 * @param versionId     Pointer to version ID structure.
 */
static inline void LLWU_GetVersionId(LLWU_Type *base, llwu_version_id_t *versionId)
{
    *((uint32_t *)versionId) = base->VERID;
}
#endif /* FSL_FEATURE_LLWU_HAS_VERID */

#if (defined(FSL_FEATURE_LLWU_HAS_PARAM) && FSL_FEATURE_LLWU_HAS_PARAM)
/*!
 * @brief Gets the LLWU parameter.
 *
 * This function gets the LLWU parameter, including wakeup pin number, module
 * number, DMA number, and pin filter number.
 *
 * @param base LLWU peripheral base address.
 * @param param         Pointer to LLWU param structure.
 */
static inline void LLWU_GetParam(LLWU_Type *base, llwu_param_t *param)
{
    *((uint32_t *)param) = base->PARAM;
}
#endif /* FSL_FEATURE_LLWU_HAS_PARAM */

#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN)
/*!
 * @brief Sets the external input pin source mode.
 *
 * This function sets the external input pin source mode that is used
 * as a wake up source.
 *
 * @param base LLWU peripheral base address.
 * @param pinIndex pin index which to be enabled as external wakeup source, start from 1.
 * @param pinMode pin configuration mode defined in llwu_external_pin_modes_t
 */
void LLWU_SetExternalWakeupPinMode(LLWU_Type *base, uint32_t pinIndex, llwu_external_pin_mode_t pinMode);

/*!
 * @brief Gets the external wakeup source flag.
 *
 * This function checks the external pin flag to detect whether the MCU is
 * woke up by the specific pin.
 *
 * @param base LLWU peripheral base address.
 * @param pinIndex     pin index, start from 1.
 * @return true if the specific pin is wake up source.
 */
bool LLWU_GetExternalWakeupPinFlag(LLWU_Type *base, uint32_t pinIndex);

/*!
 * @brief Clears the external wakeup source flag.
 *
 * This function clears the external wakeup source flag for a specific pin.
 *
 * @param base LLWU peripheral base address.
 * @param pinIndex pin index, start from 1.
 */
void LLWU_ClearExternalWakeupPinFlag(LLWU_Type *base, uint32_t pinIndex);
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */

#if (defined(FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE) && FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE)
/*!
 * @brief Enables/disables the internal module source.
 *
 * This function enables/disables the internal module source mode that is used
 * as a wake up source.
 *
 * @param base LLWU peripheral base address.
 * @param moduleIndex   module index which to be enabled as internal wakeup source, start from 1.
 * @param enable        enable or disable setting
 */
static inline void LLWU_EnableInternalModuleInterruptWakup(LLWU_Type *base, uint32_t moduleIndex, bool enable)
{
    if (enable)
    {
        base->ME |= 1U << moduleIndex;
    }
    else
    {
        base->ME &= ~(1U << moduleIndex);
    }
}

/*!
 * @brief Gets the external wakeup source flag.
 *
 * This function checks the external pin flag to detect whether the system is
 * woke up by the specific pin.
 *
 * @param base LLWU peripheral base address.
 * @param moduleIndex  module index, start from 1.
 * @return true if the specific pin is wake up source.
 */
static inline bool LLWU_GetInternalWakeupModuleFlag(LLWU_Type *base, uint32_t moduleIndex)
{
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32))
    return (bool)(base->MF & (1U << moduleIndex));
#else
#if (defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 16))
#if (defined(FSL_FEATURE_LLWU_HAS_PF) && FSL_FEATURE_LLWU_HAS_PF)
    return (bool)(base->MF5 & (1U << moduleIndex));
#else
    return (bool)(base->F5 & (1U << moduleIndex));
#endif /* FSL_FEATURE_LLWU_HAS_PF */
#else
#if (defined(FSL_FEATURE_LLWU_HAS_PF) && FSL_FEATURE_LLWU_HAS_PF)
    return (bool)(base->PF3 & (1U << moduleIndex));
#else
    return (bool)(base->F3 & (1U << moduleIndex));
#endif
#endif /* FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN */
#endif /* FSL_FEATURE_LLWU_REG_BITWIDTH */
}
#endif /* FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE */

#if (defined(FSL_FEATURE_LLWU_HAS_DMA_ENABLE_REG) && FSL_FEATURE_LLWU_HAS_DMA_ENABLE_REG)
/*!
 * @brief Enables/disables the internal module DMA wakeup source.
 *
 * This function enables/disables the internal DMA that is used as a wake up source.
 *
 * @param base LLWU peripheral base address.
 * @param moduleIndex   Internal module index which used as DMA request source, start from 1.
 * @param enable        Enable or disable DMA request source
 */
static inline void LLWU_EnableInternalModuleDmaRequestWakup(LLWU_Type *base, uint32_t moduleIndex, bool enable)
{
    if (enable)
    {
        base->DE |= 1U << moduleIndex;
    }
    else
    {
        base->DE &= ~(1U << moduleIndex);
    }
}
#endif /* FSL_FEATURE_LLWU_HAS_DMA_ENABLE_REG */

#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && FSL_FEATURE_LLWU_HAS_PIN_FILTER)
/*!
 * @brief Sets the pin filter configuration.
 *
 * This function sets the pin filter configuration.
 *
 * @param base LLWU peripheral base address.
 * @param filterIndex pin filter index which used to enable/disable the digital filter, start from 1.
 * @param filterMode filter mode configuration
 */
void LLWU_SetPinFilterMode(LLWU_Type *base, uint32_t filterIndex, llwu_external_pin_filter_mode_t filterMode);

/*!
 * @brief Gets the pin filter configuration.
 *
 * This function gets the pin filter flag.
 *
 * @param base LLWU peripheral base address.
 * @param filterIndex pin filter index, start from 1.
 * @return true if the flag is a source of existing a low-leakage power mode.
 */
bool LLWU_GetPinFilterFlag(LLWU_Type *base, uint32_t filterIndex);

/*!
 * @brief Clear the pin filter configuration.
 *
 * This function clear the pin filter flag.
 *
 * @param base LLWU peripheral base address.
 * @param filterIndex pin filter index which to be clear the flag, start from 1.
 */
void LLWU_ClearPinFilterFlag(LLWU_Type *base, uint32_t filterIndex);

#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */

#if (defined(FSL_FEATURE_LLWU_HAS_RESET_ENABLE) && FSL_FEATURE_LLWU_HAS_RESET_ENABLE)
/*!
 * @brief Sets the reset pin mode.
 *
 * This function sets how the reset pin is used as a low leakage mode exit source.
 *
 * @param pinEnable       Enable reset pin filter
 * @param pinFilterEnable Specify whether pin filter is enabled in Low-Leakage power mode.
 */
void LLWU_SetResetPinMode(LLWU_Type *base, bool pinEnable, bool enableInLowLeakageMode);
#endif /* FSL_FEATURE_LLWU_HAS_RESET_ENABLE */

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/
#endif /* _FSL_LLWU_H_*/
