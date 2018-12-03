/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef _FSL_LLWU_H_
#define _FSL_LLWU_H_

#include "fsl_common.h"

/*! @addtogroup llwu */
/*! @{ */

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
    kLLWU_ExternalPinDisable = 0U,     /*!< Pin disabled as a wakeup input.           */
    kLLWU_ExternalPinRisingEdge = 1U,  /*!< Pin enabled with the rising edge detection. */
    kLLWU_ExternalPinFallingEdge = 2U, /*!< Pin enabled with the falling edge detection.*/
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
    uint16_t feature; /*!< A feature specification number. */
    uint8_t minor;    /*!< The minor version number.         */
    uint8_t major;    /*!< The major version number.         */
} llwu_version_id_t;
#endif /* FSL_FEATURE_LLWU_HAS_VERID */

#if (defined(FSL_FEATURE_LLWU_HAS_PARAM) && FSL_FEATURE_LLWU_HAS_PARAM)
/*!
 * @brief IP parameter definition.
 */
typedef struct _llwu_param
{
    uint8_t filters; /*!< A number of the pin filter.      */
    uint8_t dmas;    /*!< A number of the wakeup DMA.      */
    uint8_t modules; /*!< A number of the wakeup module.   */
    uint8_t pins;    /*!< A number of the wake up pin.     */
} llwu_param_t;
#endif /* FSL_FEATURE_LLWU_HAS_PARAM */

#if (defined(FSL_FEATURE_LLWU_HAS_PIN_FILTER) && FSL_FEATURE_LLWU_HAS_PIN_FILTER)
/*!
 * @brief An external input pin filter control structure
 */
typedef struct _llwu_external_pin_filter_mode
{
    uint32_t pinIndex;                 /*!< A pin number  */
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
 * This function gets the LLWU version ID, including the major version number,
 * the minor version number, and the feature specification number.
 *
 * @param base LLWU peripheral base address.
 * @param versionId     A pointer to the version ID structure.
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
 * This function gets the LLWU parameter, including a wakeup pin number, a module
 * number, a DMA number, and a pin filter number.
 *
 * @param base LLWU peripheral base address.
 * @param param         A pointer to the LLWU parameter structure.
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
 * @param pinIndex A pin index to be enabled as an external wakeup source starting from 1.
 * @param pinMode A pin configuration mode defined in the llwu_external_pin_modes_t.
 */
void LLWU_SetExternalWakeupPinMode(LLWU_Type *base, uint32_t pinIndex, llwu_external_pin_mode_t pinMode);

/*!
 * @brief Gets the external wakeup source flag.
 *
 * This function checks the external pin flag to detect whether the MCU is
 * woken up by the specific pin.
 *
 * @param base LLWU peripheral base address.
 * @param pinIndex     A pin index, which starts from 1.
 * @return True if the specific pin is a wakeup source.
 */
bool LLWU_GetExternalWakeupPinFlag(LLWU_Type *base, uint32_t pinIndex);

/*!
 * @brief Clears the external wakeup source flag.
 *
 * This function clears the external wakeup source flag for a specific pin.
 *
 * @param base LLWU peripheral base address.
 * @param pinIndex A pin index, which starts from 1.
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
 * @param moduleIndex   A module index to be enabled as an internal wakeup source starting from 1.
 * @param enable        An enable or a disable setting
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

#if (!(defined(FSL_FEATURE_LLWU_HAS_NO_INTERNAL_MODULE_WAKEUP_FLAG_REG) && \
       FSL_FEATURE_LLWU_HAS_NO_INTERNAL_MODULE_WAKEUP_FLAG_REG))
/* Re-define the register which includes the internal wakeup module flag. */
#if (defined(FSL_FEATURE_LLWU_REG_BITWIDTH) && (FSL_FEATURE_LLWU_REG_BITWIDTH == 32)) /* 32-bit LLWU. */
#if (defined(FSL_FEATURE_LLWU_HAS_MF) && FSL_FEATURE_LLWU_HAS_MF)
#define INTERNAL_WAKEUP_MODULE_FLAG_REG MF
#else
#error "Unsupported internal module flag register."
#endif
#else /* 8-bit LLUW. */
#if (defined(FSL_FEATURE_LLWU_HAS_MF) && FSL_FEATURE_LLWU_HAS_MF)
#define INTERNAL_WAKEUP_MODULE_FLAG_REG MF5
#elif(defined(FSL_FEATURE_LLWU_HAS_PF) && FSL_FEATURE_LLWU_HAS_PF)
#define INTERNAL_WAKEUP_MODULE_FLAG_REG PF3
#elif(!(defined(FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN) && (FSL_FEATURE_LLWU_HAS_EXTERNAL_PIN > 16)))
#define INTERNAL_WAKEUP_MODULE_FLAG_REG F3
#else
#error "Unsupported internal module flag register."
#endif
#endif /* FSL_FEATURE_LLWU_REG_BITWIDTH */

/*!
 * @brief Gets the external wakeup source flag.
 *
 * This function checks the external pin flag to detect whether the system is
 * woken up by the specific pin.
 *
 * @param base LLWU peripheral base address.
 * @param moduleIndex  A module index, which starts from 1.
 * @return True if the specific pin is a wake up source.
 */
static inline bool LLWU_GetInternalWakeupModuleFlag(LLWU_Type *base, uint32_t moduleIndex)
{
    return ((1U << moduleIndex) == (base->INTERNAL_WAKEUP_MODULE_FLAG_REG & (1U << moduleIndex)));
}
#endif /* FSL_FEATURE_LLWU_HAS_NO_INTERNAL_MODULE_WAKEUP_FLAG_REG */
#endif /* FSL_FEATURE_LLWU_HAS_INTERNAL_MODULE */

#if (defined(FSL_FEATURE_LLWU_HAS_DMA_ENABLE_REG) && FSL_FEATURE_LLWU_HAS_DMA_ENABLE_REG)
/*!
 * @brief Enables/disables the internal module DMA wakeup source.
 *
 * This function enables/disables the internal DMA that is used as a wake up source.
 *
 * @param base LLWU peripheral base address.
 * @param moduleIndex   An internal module index which is used as a DMA request source, starting from 1.
 * @param enable        Enable or disable the DMA request source
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
 * @param filterIndex A pin filter index used to enable/disable the digital filter, starting from 1.
 * @param filterMode A filter mode configuration
 */
void LLWU_SetPinFilterMode(LLWU_Type *base, uint32_t filterIndex, llwu_external_pin_filter_mode_t filterMode);

/*!
 * @brief Gets the pin filter configuration.
 *
 * This function gets the pin filter flag.
 *
 * @param base LLWU peripheral base address.
 * @param filterIndex A pin filter index, which starts from 1.
 * @return True if the flag is a source of the existing low-leakage power mode.
 */
bool LLWU_GetPinFilterFlag(LLWU_Type *base, uint32_t filterIndex);

/*!
 * @brief Clears the pin filter configuration.
 *
 * This function clears the pin filter flag.
 *
 * @param base LLWU peripheral base address.
 * @param filterIndex A pin filter index to clear the flag, starting from 1.
 */
void LLWU_ClearPinFilterFlag(LLWU_Type *base, uint32_t filterIndex);

#endif /* FSL_FEATURE_LLWU_HAS_PIN_FILTER */

#if (defined(FSL_FEATURE_LLWU_HAS_RESET_ENABLE) && FSL_FEATURE_LLWU_HAS_RESET_ENABLE)
/*!
 * @brief Sets the reset pin mode.
 *
 * This function determines how the reset pin is used as a low leakage mode exit source.
 *
 * @param pinEnable       Enable reset the pin filter
 * @param pinFilterEnable Specify whether the pin filter is enabled in Low-Leakage power mode.
 */
void LLWU_SetResetPinMode(LLWU_Type *base, bool pinEnable, bool enableInLowLeakageMode);
#endif /* FSL_FEATURE_LLWU_HAS_RESET_ENABLE */

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @}*/
#endif /* _FSL_LLWU_H_*/
