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

#ifndef _FSL_VREF_H_
#define _FSL_VREF_H_

#include "fsl_common.h"

/*!
 * @addtogroup vref
 * @{
 */


/******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
#define FSL_VREF_DRIVER_VERSION (MAKE_VERSION(2, 1, 0)) /*!< Version 2.1.0. */
/*@}*/

/* Those macros below defined to support SoC family which have VREFL (0.4V) reference */
#if defined(FSL_FEATURE_VREF_HAS_LOW_REFERENCE) && FSL_FEATURE_VREF_HAS_LOW_REFERENCE
#define VREF_SC_MODE_LV VREF_VREFH_SC_MODE_LV
#define VREF_SC_REGEN VREF_VREFH_SC_REGEN
#define VREF_SC_VREFEN VREF_VREFH_SC_VREFEN
#define VREF_SC_ICOMPEN VREF_VREFH_SC_ICOMPEN
#define VREF_SC_REGEN_MASK VREF_VREFH_SC_REGEN_MASK
#define VREF_SC_VREFST_MASK VREF_VREFH_SC_VREFST_MASK
#define VREF_SC_VREFEN_MASK VREF_VREFH_SC_VREFEN_MASK
#define VREF_SC_MODE_LV_MASK VREF_VREFH_SC_MODE_LV_MASK
#define VREF_SC_ICOMPEN_MASK VREF_VREFH_SC_ICOMPEN_MASK
#define TRM VREFH_TRM
#define VREF_TRM_TRIM VREF_VREFH_TRM_TRIM
#define VREF_TRM_CHOPEN_MASK VREF_VREFH_TRM_CHOPEN_MASK
#define VREF_TRM_TRIM_MASK VREF_VREFH_TRM_TRIM_MASK
#define VREF_TRM_CHOPEN_SHIFT VREF_VREFH_TRM_CHOPEN_SHIFT
#define VREF_TRM_TRIM_SHIFT VREF_VREFH_TRM_TRIM_SHIFT
#define VREF_SC_MODE_LV_SHIFT VREF_VREFH_SC_MODE_LV_SHIFT
#define VREF_SC_REGEN_SHIFT VREF_VREFH_SC_REGEN_SHIFT
#define VREF_SC_VREFST_SHIFT VREF_VREFH_SC_VREFST_SHIFT
#define VREF_SC_ICOMPEN_SHIFT VREF_VREFH_SC_ICOMPEN_SHIFT
#endif /* FSL_FEATURE_VREF_HAS_LOW_REFERENCE */

/*!
 * @brief VREF modes.
 */
typedef enum _vref_buffer_mode
{
    kVREF_ModeBandgapOnly = 0U, /*!< Bandgap on only, for stabilization and startup */
#if defined(FSL_FEATURE_VREF_MODE_LV_TYPE) && FSL_FEATURE_VREF_MODE_LV_TYPE
    kVREF_ModeHighPowerBuffer = 1U, /*!< High-power buffer mode enabled */
    kVREF_ModeLowPowerBuffer = 2U   /*!< Low-power buffer mode enabled */
#else
    kVREF_ModeTightRegulationBuffer = 2U /*!< Tight regulation buffer enabled */
#endif /* FSL_FEATURE_VREF_MODE_LV_TYPE */
} vref_buffer_mode_t;

/*!
 * @brief The description structure for the VREF module.
 */
typedef struct _vref_config
{
    vref_buffer_mode_t bufferMode; /*!< Buffer mode selection */
#if defined(FSL_FEATURE_VREF_HAS_LOW_REFERENCE) && FSL_FEATURE_VREF_HAS_LOW_REFERENCE
    bool enableLowRef;          /*!< Set VREFL (0.4 V) reference buffer enable or disable */
    bool enableExternalVoltRef; /*!< Select external voltage reference or not (internal) */
#endif                          /* FSL_FEATURE_VREF_HAS_LOW_REFERENCE */
#if defined(FSL_FEATURE_VREF_HAS_TRM4) && FSL_FEATURE_VREF_HAS_TRM4
    bool enable2V1VoltRef; /*!< Enable Internal Voltage Reference (2.1V) */
#endif                     /* FSL_FEATURE_VREF_HAS_TRM4 */
} vref_config_t;

/******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @name VREF functional operation
 * @{
 */

/*!
 * @brief Enables the clock gate and configures the VREF module according to the configuration structure.
 *
 * This function must be called before calling all other VREF driver functions,
 * read/write registers, and configurations with user-defined settings.
 * The example below shows how to set up  vref_config_t parameters and
 * how to call the VREF_Init function by passing in these parameters.
 * This is an example.
 * @code
 *   vref_config_t vrefConfig;
 *   vrefConfig.bufferMode = kVREF_ModeHighPowerBuffer;
 *   vrefConfig.enableExternalVoltRef = false;
 *   vrefConfig.enableLowRef = false;
 *   VREF_Init(VREF, &vrefConfig);
 * @endcode
 *
 * @param base VREF peripheral address.
 * @param config Pointer to the configuration structure.
 */
void VREF_Init(VREF_Type *base, const vref_config_t *config);

/*!
 * @brief Stops and disables the clock for the VREF module.
 *
 * This function should be called to shut down the module.
 * This is an example.
 * @code
 *   vref_config_t vrefUserConfig;
 *   VREF_Init(VREF);
 *   VREF_GetDefaultConfig(&vrefUserConfig);
 *   ...
 *   VREF_Deinit(VREF);
 * @endcode
 *
 * @param base VREF peripheral address.
 */
void VREF_Deinit(VREF_Type *base);

/*!
 * @brief Initializes the VREF configuration structure.
 *
 * This function initializes the VREF configuration structure to default values.
 * This is an example.
 * @code
 *   vrefConfig->bufferMode = kVREF_ModeHighPowerBuffer;
 *   vrefConfig->enableExternalVoltRef = false;
 *   vrefConfig->enableLowRef = false;
 * @endcode
 *
 * @param config Pointer to the initialization structure.
 */
void VREF_GetDefaultConfig(vref_config_t *config);

/*!
 * @brief Sets a TRIM value for the reference voltage.
 *
 * This function sets a TRIM value for the reference voltage.
 * Note that the TRIM value maximum is 0x3F.
 *
 * @param base VREF peripheral address.
 * @param trimValue Value of the trim register to set the output reference voltage (maximum 0x3F (6-bit)).
 */
void VREF_SetTrimVal(VREF_Type *base, uint8_t trimValue);

/*!
 * @brief Reads the value of the TRIM meaning output voltage.
 *
 * This function gets the TRIM value from the TRM register.
 *
 * @param base VREF peripheral address.
 * @return Six-bit value of trim setting.
 */
static inline uint8_t VREF_GetTrimVal(VREF_Type *base)
{
    return (base->TRM & VREF_TRM_TRIM_MASK);
}

#if defined(FSL_FEATURE_VREF_HAS_TRM4) && FSL_FEATURE_VREF_HAS_TRM4
/*!
 * @brief Sets a TRIM value for the reference voltage (2V1).
 *
 * This function sets a TRIM value for the reference voltage (2V1).
 * Note that the TRIM value maximum is 0x3F.
 *
 * @param base VREF peripheral address.
 * @param trimValue Value of the trim register to set the output reference voltage (maximum 0x3F (6-bit)).
 */
void VREF_SetTrim2V1Val(VREF_Type *base, uint8_t trimValue);

/*!
 * @brief Reads the value of the TRIM meaning output voltage (2V1).
 *
 * This function gets the TRIM value from the VREF_TRM4 register.
 *
 * @param base VREF peripheral address.
 * @return Six-bit value of trim setting.
 */
static inline uint8_t VREF_GetTrim2V1Val(VREF_Type *base)
{
    return (base->TRM4 & VREF_TRM4_TRIM2V1_MASK);
}
#endif /* FSL_FEATURE_VREF_HAS_TRM4 */

#if defined(FSL_FEATURE_VREF_HAS_LOW_REFERENCE) && FSL_FEATURE_VREF_HAS_LOW_REFERENCE

/*!
 * @brief Sets the TRIM value for the low voltage reference.
 *
 * This function sets the TRIM value for low reference voltage.
 * Note the following.
 *      - The TRIM value maximum is 0x05U
 *      - The values 111b and 110b are not valid/allowed.
 *
 * @param base VREF peripheral address.
 * @param trimValue Value of the trim register to set output low reference voltage (maximum 0x05U (3-bit)).
 */
void VREF_SetLowReferenceTrimVal(VREF_Type *base, uint8_t trimValue);

/*!
 * @brief Reads the value of the TRIM meaning output voltage.
 *
 * This function gets the TRIM value from the VREFL_TRM register.
 *
 * @param base VREF peripheral address.
 * @return Three-bit value of the trim setting.
 */
static inline uint8_t VREF_GetLowReferenceTrimVal(VREF_Type *base)
{
    return (base->VREFL_TRM & VREF_VREFL_TRM_VREFL_TRIM_MASK);
}
#endif /* FSL_FEATURE_VREF_HAS_LOW_REFERENCE */

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_VREF_H_ */
