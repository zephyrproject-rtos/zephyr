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
#ifndef _FSL_PMC_H_
#define _FSL_PMC_H_

#include "fsl_common.h"

/*! @addtogroup pmc */
/*! @{ */


/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief PMC driver version */
#define FSL_PMC_DRIVER_VERSION (MAKE_VERSION(2, 0, 0)) /*!< Version 2.0.0. */
/*@}*/

#if (defined(FSL_FEATURE_PMC_HAS_LVDV) && FSL_FEATURE_PMC_HAS_LVDV)
/*!
 * @brief Low-voltage Detect Voltage Select
 */
typedef enum _pmc_low_volt_detect_volt_select
{
    kPMC_LowVoltDetectLowTrip = 0U, /*!< Low-trip point selected (VLVD = VLVDL )*/
    kPMC_LowVoltDetectHighTrip = 1U /*!< High-trip point selected (VLVD = VLVDH )*/
} pmc_low_volt_detect_volt_select_t;
#endif

#if (defined(FSL_FEATURE_PMC_HAS_LVWV) && FSL_FEATURE_PMC_HAS_LVWV)
/*!
 * @brief Low-voltage Warning Voltage Select
 */
typedef enum _pmc_low_volt_warning_volt_select
{
    kPMC_LowVoltWarningLowTrip = 0U,  /*!< Low-trip point selected (VLVW = VLVW1)*/
    kPMC_LowVoltWarningMid1Trip = 1U, /*!< Mid 1 trip point selected (VLVW = VLVW2)*/
    kPMC_LowVoltWarningMid2Trip = 2U, /*!< Mid 2 trip point selected (VLVW = VLVW3)*/
    kPMC_LowVoltWarningHighTrip = 3U  /*!< High-trip point selected (VLVW = VLVW4)*/
} pmc_low_volt_warning_volt_select_t;
#endif

#if (defined(FSL_FEATURE_PMC_HAS_HVDSC1) && FSL_FEATURE_PMC_HAS_HVDSC1)
/*!
 * @brief High-voltage Detect Voltage Select
 */
typedef enum _pmc_high_volt_detect_volt_select
{
    kPMC_HighVoltDetectLowTrip = 0U, /*!< Low-trip point selected (VHVD = VHVDL )*/
    kPMC_HighVoltDetectHighTrip = 1U /*!< High-trip point selected (VHVD = VHVDH )*/
} pmc_high_volt_detect_volt_select_t;
#endif /* FSL_FEATURE_PMC_HAS_HVDSC1 */

#if (defined(FSL_FEATURE_PMC_HAS_BGBDS) && FSL_FEATURE_PMC_HAS_BGBDS)
/*!
 * @brief Bandgap Buffer Drive Select.
 */
typedef enum _pmc_bandgap_buffer_drive_select
{
    kPMC_BandgapBufferDriveLow = 0U, /*!< Low-drive.  */
    kPMC_BandgapBufferDriveHigh = 1U /*!< High-drive. */
} pmc_bandgap_buffer_drive_select_t;
#endif /* FSL_FEATURE_PMC_HAS_BGBDS */

#if (defined(FSL_FEATURE_PMC_HAS_VLPO) && FSL_FEATURE_PMC_HAS_VLPO)
/*!
 * @brief VLPx Option
 */
typedef enum _pmc_vlp_freq_option
{
    kPMC_FreqRestrict = 0U,  /*!< Frequency is restricted in VLPx mode. */
    kPMC_FreqUnrestrict = 1U /*!< Frequency is unrestricted in VLPx mode. */
} pmc_vlp_freq_mode_t;
#endif /* FSL_FEATURE_PMC_HAS_VLPO */

#if (defined(FSL_FEATURE_PMC_HAS_VERID) && FSL_FEATURE_PMC_HAS_VERID)
/*!
 @brief IP version ID definition.
 */
typedef struct _pmc_version_id
{
    uint16_t feature; /*!< Feature Specification Number. */
    uint8_t minor;    /*!< Minor version number.         */
    uint8_t major;    /*!< Major version number.         */
} pmc_version_id_t;
#endif /* FSL_FEATURE_PMC_HAS_VERID */

#if (defined(FSL_FEATURE_PMC_HAS_PARAM) && FSL_FEATURE_PMC_HAS_PARAM)
/*! @brief IP parameter definition. */
typedef struct _pmc_param
{
    bool vlpoEnable; /*!< VLPO enable. */
    bool hvdEnable;  /*!< HVD enable.  */
} pmc_param_t;
#endif /* FSL_FEATURE_PMC_HAS_PARAM */

/*!
 * @brief Low-voltage Detect Configuration Structure
 */
typedef struct _pmc_low_volt_detect_config
{
    bool enableInt;   /*!< Enable interrupt when Low-voltage detect*/
    bool enableReset; /*!< Enable system reset when Low-voltage detect*/
#if (defined(FSL_FEATURE_PMC_HAS_LVDV) && FSL_FEATURE_PMC_HAS_LVDV)
    pmc_low_volt_detect_volt_select_t voltSelect; /*!< Low-voltage detect trip point voltage selection*/
#endif
} pmc_low_volt_detect_config_t;

/*!
 * @brief Low-voltage Warning Configuration Structure
 */
typedef struct _pmc_low_volt_warning_config
{
    bool enableInt; /*!< Enable interrupt when low-voltage warning*/
#if (defined(FSL_FEATURE_PMC_HAS_LVWV) && FSL_FEATURE_PMC_HAS_LVWV)
    pmc_low_volt_warning_volt_select_t voltSelect; /*!< Low-voltage warning trip point voltage selection*/
#endif
} pmc_low_volt_warning_config_t;

#if (defined(FSL_FEATURE_PMC_HAS_HVDSC1) && FSL_FEATURE_PMC_HAS_HVDSC1)
/*!
 * @brief High-voltage Detect Configuration Structure
 */
typedef struct _pmc_high_volt_detect_config
{
    bool enableInt;                                /*!< Enable interrupt when high-voltage detect*/
    bool enableReset;                              /*!< Enable system reset when high-voltage detect*/
    pmc_high_volt_detect_volt_select_t voltSelect; /*!< High-voltage detect trip point voltage selection*/
} pmc_high_volt_detect_config_t;
#endif /* FSL_FEATURE_PMC_HAS_HVDSC1 */

#if ((defined(FSL_FEATURE_PMC_HAS_BGBE) && FSL_FEATURE_PMC_HAS_BGBE) || \
     (defined(FSL_FEATURE_PMC_HAS_BGEN) && FSL_FEATURE_PMC_HAS_BGEN) || \
     (defined(FSL_FEATURE_PMC_HAS_BGBDS) && FSL_FEATURE_PMC_HAS_BGBDS))
/*!
 * @brief Bandgap Buffer configuration.
 */
typedef struct _pmc_bandgap_buffer_config
{
#if (defined(FSL_FEATURE_PMC_HAS_BGBE) && FSL_FEATURE_PMC_HAS_BGBE)
    bool enable; /*!< Enable bandgap buffer.                   */
#endif
#if (defined(FSL_FEATURE_PMC_HAS_BGEN) && FSL_FEATURE_PMC_HAS_BGEN)
    bool enableInLowPowerMode; /*!< Enable bandgap buffer in low-power mode. */
#endif                         /* FSL_FEATURE_PMC_HAS_BGEN */
#if (defined(FSL_FEATURE_PMC_HAS_BGBDS) && FSL_FEATURE_PMC_HAS_BGBDS)
    pmc_bandgap_buffer_drive_select_t drive; /*!< Bandgap buffer drive select.             */
#endif                                       /* FSL_FEATURE_PMC_HAS_BGBDS */
} pmc_bandgap_buffer_config_t;
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*! @name Power Management Controller Control APIs*/
/*@{*/

#if (defined(FSL_FEATURE_PMC_HAS_VERID) && FSL_FEATURE_PMC_HAS_VERID)
/*!
 * @brief Gets the PMC version ID.
 *
 * This function gets the PMC version ID, including major version number,
 * minor version number, and a feature specification number.
 *
 * @param base PMC peripheral base address.
 * @param versionId     Pointer to version ID structure.
 */
static inline void PMC_GetVersionId(PMC_Type *base, pmc_version_id_t *versionId)
{
    *((uint32_t *)versionId) = base->VERID;
}
#endif /* FSL_FEATURE_PMC_HAS_VERID */

#if (defined(FSL_FEATURE_PMC_HAS_PARAM) && FSL_FEATURE_PMC_HAS_PARAM)
/*!
 * @brief Gets the PMC parameter.
 *
 * This function gets the PMC parameter including the VLPO enable and the HVD enable.
 *
 * @param base PMC peripheral base address.
 * @param param         Pointer to PMC param structure.
 */
void PMC_GetParam(PMC_Type *base, pmc_param_t *param);
#endif

/*!
 * @brief Configures the low-voltage detect setting.
 *
 * This function configures the low-voltage detect setting, including the trip
 * point voltage setting, enables or disables the interrupt, enables or disables the system reset.
 *
 * @param base PMC peripheral base address.
 * @param config  Low-voltage detect configuration structure.
 */
void PMC_ConfigureLowVoltDetect(PMC_Type *base, const pmc_low_volt_detect_config_t *config);

/*!
 * @brief Gets the Low-voltage Detect Flag status.
 *
 * This function  reads the current LVDF status. If it returns 1, a low-voltage event is detected.
 *
 * @param base PMC peripheral base address.
 * @return Current low-voltage detect flag
 *                - true: Low-voltage detected
 *                - false: Low-voltage not detected
 */
static inline bool PMC_GetLowVoltDetectFlag(PMC_Type *base)
{
    return (bool)(base->LVDSC1 & PMC_LVDSC1_LVDF_MASK);
}

/*!
 * @brief Acknowledges clearing the Low-voltage Detect flag.
 *
 * This function acknowledges the low-voltage detection errors (write 1 to
 * clear LVDF).
 *
 * @param base PMC peripheral base address.
 */
static inline void PMC_ClearLowVoltDetectFlag(PMC_Type *base)
{
    base->LVDSC1 |= PMC_LVDSC1_LVDACK_MASK;
}

/*!
 * @brief Configures the low-voltage warning setting.
 *
 * This function configures the low-voltage warning setting, including the trip
 * point voltage setting and enabling or disabling the interrupt.
 *
 * @param base PMC peripheral base address.
 * @param config  Low-voltage warning configuration structure.
 */
void PMC_ConfigureLowVoltWarning(PMC_Type *base, const pmc_low_volt_warning_config_t *config);

/*!
 * @brief Gets the Low-voltage Warning Flag status.
 *
 * This function polls the current LVWF status. When 1 is returned, it
 * indicates a low-voltage warning event. LVWF is set when V Supply transitions
 * below the trip point or after reset and V Supply is already below the V LVW.
 *
 * @param base PMC peripheral base address.
 * @return Current LVWF status
 *                  - true: Low-voltage Warning Flag is set.
 *                  - false: the  Low-voltage Warning does not happen.
 */
static inline bool PMC_GetLowVoltWarningFlag(PMC_Type *base)
{
    return (bool)(base->LVDSC2 & PMC_LVDSC2_LVWF_MASK);
}

/*!
 * @brief Acknowledges the Low-voltage Warning flag.
 *
 * This function acknowledges the low voltage warning errors (write 1 to
 * clear LVWF).
 *
 * @param base PMC peripheral base address.
 */
static inline void PMC_ClearLowVoltWarningFlag(PMC_Type *base)
{
    base->LVDSC2 |= PMC_LVDSC2_LVWACK_MASK;
}

#if (defined(FSL_FEATURE_PMC_HAS_HVDSC1) && FSL_FEATURE_PMC_HAS_HVDSC1)
/*!
 * @brief Configures the high-voltage detect setting.
 *
 * This function configures the high-voltage detect setting, including the trip
 * point voltage setting, enabling or disabling the interrupt, enabling or disabling the system reset.
 *
 * @param base PMC peripheral base address.
 * @param config  High-voltage detect configuration structure.
 */
void PMC_ConfigureHighVoltDetect(PMC_Type *base, const pmc_high_volt_detect_config_t *config);

/*!
 * @brief Gets the High-voltage Detect Flag status.
 *
 * This function  reads the current HVDF status. If it returns 1, a low
 * voltage event is detected.
 *
 * @param base PMC peripheral base address.
 * @return Current high-voltage detect flag
 *                - true: High-voltage detected
 *                - false: High-voltage not detected
 */
static inline bool PMC_GetHighVoltDetectFlag(PMC_Type *base)
{
    return (bool)(base->HVDSC1 & PMC_HVDSC1_HVDF_MASK);
}

/*!
 * @brief Acknowledges clearing the High-voltage Detect flag.
 *
 * This function acknowledges the high-voltage detection errors (write 1 to
 * clear HVDF).
 *
 * @param base PMC peripheral base address.
 */
static inline void PMC_ClearHighVoltDetectFlag(PMC_Type *base)
{
    base->HVDSC1 |= PMC_HVDSC1_HVDACK_MASK;
}
#endif /* FSL_FEATURE_PMC_HAS_HVDSC1 */

#if ((defined(FSL_FEATURE_PMC_HAS_BGBE) && FSL_FEATURE_PMC_HAS_BGBE) || \
     (defined(FSL_FEATURE_PMC_HAS_BGEN) && FSL_FEATURE_PMC_HAS_BGEN) || \
     (defined(FSL_FEATURE_PMC_HAS_BGBDS) && FSL_FEATURE_PMC_HAS_BGBDS))
/*!
 * @brief Configures the PMC bandgap.
 *
 * This function configures the PMC bandgap, including the drive select and
 * behavior in low-power mode.
 *
 * @param base PMC peripheral base address.
 * @param config Pointer to the configuration structure
 */
void PMC_ConfigureBandgapBuffer(PMC_Type *base, const pmc_bandgap_buffer_config_t *config);
#endif

#if (defined(FSL_FEATURE_PMC_HAS_ACKISO) && FSL_FEATURE_PMC_HAS_ACKISO)
/*!
 * @brief Gets the acknowledge Peripherals and I/O pads isolation flag.
 *
 * This function  reads the Acknowledge Isolation setting that indicates
 * whether certain peripherals and the I/O pads are in a latched state as
 * a result of having been in the VLLS mode.
 *
 * @param base PMC peripheral base address.
 * @param base  Base address for current PMC instance.
 * @return ACK isolation
 *               0 - Peripherals and I/O pads are in a normal run state.
 *               1 - Certain peripherals and I/O pads are in an isolated and
 *                   latched state.
 */
static inline bool PMC_GetPeriphIOIsolationFlag(PMC_Type *base)
{
    return (bool)(base->REGSC & PMC_REGSC_ACKISO_MASK);
}

/*!
 * @brief Acknowledges the isolation flag to Peripherals and I/O pads.
 *
 * This function  clears the ACK Isolation flag. Writing one to this setting
 * when it is set releases the I/O pads and certain peripherals to their normal
 * run mode state.
 *
 * @param base PMC peripheral base address.
 */
static inline void PMC_ClearPeriphIOIsolationFlag(PMC_Type *base)
{
    base->REGSC |= PMC_REGSC_ACKISO_MASK;
}
#endif /* FSL_FEATURE_PMC_HAS_ACKISO */

#if (defined(FSL_FEATURE_PMC_HAS_REGONS) && FSL_FEATURE_PMC_HAS_REGONS)
/*!
 * @brief Gets the regulator regulation status.
 *
 * This function  returns the regulator to run a regulation status. It provides
 * the current status of the internal voltage regulator.
 *
 * @param base PMC peripheral base address.
 * @param base  Base address for current PMC instance.
 * @return Regulation status
 *               0 - Regulator is in a stop regulation or in transition to/from the regulation.
 *               1 - Regulator is in a run regulation.
 *
 */
static inline bool PMC_IsRegulatorInRunRegulation(PMC_Type *base)
{
    return (bool)(base->REGSC & PMC_REGSC_REGONS_MASK);
}
#endif /* FSL_FEATURE_PMC_HAS_REGONS */

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*! @}*/

#endif /* _FSL_PMC_H_*/
