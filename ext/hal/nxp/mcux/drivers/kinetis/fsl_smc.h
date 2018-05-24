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

#ifndef _FSL_SMC_H_
#define _FSL_SMC_H_

#include "fsl_common.h"

/*! @addtogroup smc */
/*! @{ */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief SMC driver version 2.0.3. */
#define FSL_SMC_DRIVER_VERSION (MAKE_VERSION(2, 0, 3))
/*@}*/

/*!
 * @brief Power Modes Protection
 */
typedef enum _smc_power_mode_protection
{
#if (defined(FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE)
    kSMC_AllowPowerModeVlls = SMC_PMPROT_AVLLS_MASK, /*!< Allow Very-low-leakage Stop Mode. */
#endif
#if (defined(FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE)
    kSMC_AllowPowerModeLls = SMC_PMPROT_ALLS_MASK, /*!< Allow Low-leakage Stop Mode.      */
#endif                                             /* FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE */
    kSMC_AllowPowerModeVlp = SMC_PMPROT_AVLP_MASK, /*!< Allow Very-Low-power Mode.        */
#if (defined(FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE) && FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE)
    kSMC_AllowPowerModeHsrun = SMC_PMPROT_AHSRUN_MASK, /*!< Allow High-speed Run mode.        */
#endif                                                 /* FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE */
    kSMC_AllowPowerModeAll = (0U
#if (defined(FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE)
                              |
                              SMC_PMPROT_AVLLS_MASK
#endif
#if (defined(FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE)
                              |
                              SMC_PMPROT_ALLS_MASK
#endif /* FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE */
                              |
                              SMC_PMPROT_AVLP_MASK
#if (defined(FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE) && FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE)
                              |
                              kSMC_AllowPowerModeHsrun
#endif                          /* FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE */
                              ) /*!< Allow all power mode.              */
} smc_power_mode_protection_t;

/*!
 * @brief Power Modes in PMSTAT
 */
typedef enum _smc_power_state
{
    kSMC_PowerStateRun = 0x01U << 0U,  /*!< 0000_0001 - Current power mode is RUN   */
    kSMC_PowerStateStop = 0x01U << 1U, /*!< 0000_0010 - Current power mode is STOP  */
    kSMC_PowerStateVlpr = 0x01U << 2U, /*!< 0000_0100 - Current power mode is VLPR  */
    kSMC_PowerStateVlpw = 0x01U << 3U, /*!< 0000_1000 - Current power mode is VLPW  */
    kSMC_PowerStateVlps = 0x01U << 4U, /*!< 0001_0000 - Current power mode is VLPS  */
#if (defined(FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE)
    kSMC_PowerStateLls = 0x01U << 5U, /*!< 0010_0000 - Current power mode is LLS   */
#endif                                /* FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE */
#if (defined(FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE)
    kSMC_PowerStateVlls = 0x01U << 6U, /*!< 0100_0000 - Current power mode is VLLS  */
#endif
#if (defined(FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE) && FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE)
    kSMC_PowerStateHsrun = 0x01U << 7U /*!< 1000_0000 - Current power mode is HSRUN */
#endif                                 /* FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE */
} smc_power_state_t;

/*!
 * @brief Run mode definition
 */
typedef enum _smc_run_mode
{
    kSMC_RunNormal = 0U, /*!< Normal RUN mode.             */
    kSMC_RunVlpr = 2U,   /*!< Very-low-power RUN mode.     */
#if (defined(FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE) && FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE)
    kSMC_Hsrun = 3U /*!< High-speed Run mode (HSRUN). */
#endif              /* FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE */
} smc_run_mode_t;

/*!
 * @brief Stop mode definition
 */
typedef enum _smc_stop_mode
{
    kSMC_StopNormal = 0U, /*!< Normal STOP mode.           */
    kSMC_StopVlps = 2U,   /*!< Very-low-power STOP mode.   */
#if (defined(FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE)
    kSMC_StopLls = 3U, /*!< Low-leakage Stop mode.      */
#endif                 /* FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE */
#if (defined(FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE)
    kSMC_StopVlls = 4U /*!< Very-low-leakage Stop mode. */
#endif
} smc_stop_mode_t;

#if (defined(FSL_FEATURE_SMC_USE_VLLSCTRL_REG) && FSL_FEATURE_SMC_USE_VLLSCTRL_REG) ||     \
    (defined(FSL_FEATURE_SMC_USE_STOPCTRL_VLLSM) && FSL_FEATURE_SMC_USE_STOPCTRL_VLLSM) || \
    (defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE)
/*!
 * @brief VLLS/LLS stop sub mode definition
 */
typedef enum _smc_stop_submode
{
    kSMC_StopSub0 = 0U, /*!< Stop submode 0, for VLLS0/LLS0. */
    kSMC_StopSub1 = 1U, /*!< Stop submode 1, for VLLS1/LLS1. */
    kSMC_StopSub2 = 2U, /*!< Stop submode 2, for VLLS2/LLS2. */
    kSMC_StopSub3 = 3U  /*!< Stop submode 3, for VLLS3/LLS3. */
} smc_stop_submode_t;
#endif

/*!
 * @brief Partial STOP option
 */
typedef enum _smc_partial_stop_mode
{
    kSMC_PartialStop = 0U,  /*!< STOP - Normal Stop mode*/
    kSMC_PartialStop1 = 1U, /*!< Partial Stop with both system and bus clocks disabled*/
    kSMC_PartialStop2 = 2U, /*!< Partial Stop with system clock disabled and bus clock enabled*/
} smc_partial_stop_option_t;

/*!
 * @brief SMC configuration status.
 */
enum _smc_status
{
    kStatus_SMC_StopAbort = MAKE_STATUS(kStatusGroup_POWER, 0) /*!< Entering Stop mode is abort*/
};

#if (defined(FSL_FEATURE_SMC_HAS_VERID) && FSL_FEATURE_SMC_HAS_VERID)
/*!
 * @brief IP version ID definition.
 */
typedef struct _smc_version_id
{
    uint16_t feature; /*!< Feature Specification Number. */
    uint8_t minor;    /*!< Minor version number.         */
    uint8_t major;    /*!< Major version number.         */
} smc_version_id_t;
#endif /* FSL_FEATURE_SMC_HAS_VERID */

#if (defined(FSL_FEATURE_SMC_HAS_PARAM) && FSL_FEATURE_SMC_HAS_PARAM)
/*!
 * @brief IP parameter definition.
 */
typedef struct _smc_param
{
    bool hsrunEnable; /*!< HSRUN mode enable. */
    bool llsEnable;   /*!< LLS mode enable.   */
    bool lls2Enable;  /*!< LLS2 mode enable.  */
    bool vlls0Enable; /*!< VLLS0 mode enable. */
} smc_param_t;
#endif /* FSL_FEATURE_SMC_HAS_PARAM */

#if (defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE) || \
    (defined(FSL_FEATURE_SMC_HAS_LPOPO) && FSL_FEATURE_SMC_HAS_LPOPO)
/*!
 * @brief SMC Low-Leakage Stop power mode configuration.
 */
typedef struct _smc_power_mode_lls_config
{
#if (defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE)
    smc_stop_submode_t subMode; /*!< Low-leakage Stop sub-mode */
#endif
#if (defined(FSL_FEATURE_SMC_HAS_LPOPO) && FSL_FEATURE_SMC_HAS_LPOPO)
    bool enableLpoClock; /*!< Enable LPO clock in LLS mode */
#endif
} smc_power_mode_lls_config_t;
#endif /* (FSL_FEATURE_SMC_HAS_LLS_SUBMODE || FSL_FEATURE_SMC_HAS_LPOPO) */

#if (defined(FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE)
/*!
 * @brief SMC Very Low-Leakage Stop power mode configuration.
 */
typedef struct _smc_power_mode_vlls_config
{
#if (defined(FSL_FEATURE_SMC_USE_VLLSCTRL_REG) && FSL_FEATURE_SMC_USE_VLLSCTRL_REG) ||     \
    (defined(FSL_FEATURE_SMC_USE_STOPCTRL_VLLSM) && FSL_FEATURE_SMC_USE_STOPCTRL_VLLSM) || \
    (defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE)
    smc_stop_submode_t subMode; /*!< Very Low-leakage Stop sub-mode */
#endif
#if (defined(FSL_FEATURE_SMC_HAS_PORPO) && FSL_FEATURE_SMC_HAS_PORPO)
    bool enablePorDetectInVlls0; /*!< Enable Power on reset detect in VLLS mode */
#endif
#if (defined(FSL_FEATURE_SMC_HAS_RAM2_POWER_OPTION) && FSL_FEATURE_SMC_HAS_RAM2_POWER_OPTION)
    bool enableRam2InVlls2; /*!< Enable RAM2 power in VLLS2 */
#endif
#if (defined(FSL_FEATURE_SMC_HAS_LPOPO) && FSL_FEATURE_SMC_HAS_LPOPO)
    bool enableLpoClock; /*!< Enable LPO clock in VLLS mode */
#endif
} smc_power_mode_vlls_config_t;
#endif

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*! @name System mode controller APIs*/
/*@{*/

#if (defined(FSL_FEATURE_SMC_HAS_VERID) && FSL_FEATURE_SMC_HAS_VERID)
/*!
 * @brief Gets the SMC version ID.
 *
 * This function gets the SMC version ID, including major version number,
 * minor version number, and feature specification number.
 *
 * @param base SMC peripheral base address.
 * @param versionId     Pointer to the version ID structure.
 */
static inline void SMC_GetVersionId(SMC_Type *base, smc_version_id_t *versionId)
{
    *((uint32_t *)versionId) = base->VERID;
}
#endif /* FSL_FEATURE_SMC_HAS_VERID */

#if (defined(FSL_FEATURE_SMC_HAS_PARAM) && FSL_FEATURE_SMC_HAS_PARAM)
/*!
 * @brief Gets the SMC parameter.
 *
 * This function gets the SMC parameter including the enabled power mdoes.
 *
 * @param base SMC peripheral base address.
 * @param param         Pointer to the SMC param structure.
 */
void SMC_GetParam(SMC_Type *base, smc_param_t *param);
#endif

/*!
 * @brief Configures all power mode protection settings.
 *
 * This function  configures the power mode protection settings for
 * supported power modes in the specified chip family. The available power modes
 * are defined in the smc_power_mode_protection_t. This should be done at an early
 * system level initialization stage. See the reference manual for details.
 * This register can only write once after the power reset.
 *
 * The allowed modes are passed as bit map. For example, to allow LLS and VLLS,
 * use SMC_SetPowerModeProtection(kSMC_AllowPowerModeVlls | kSMC_AllowPowerModeVlps).
 * To allow all modes, use SMC_SetPowerModeProtection(kSMC_AllowPowerModeAll).
 *
 * @param base SMC peripheral base address.
 * @param allowedModes Bitmap of the allowed power modes.
 */
static inline void SMC_SetPowerModeProtection(SMC_Type *base, uint8_t allowedModes)
{
    base->PMPROT = allowedModes;
}

/*!
 * @brief Gets the current power mode status.
 *
 * This function  returns the current power mode status. After the application
 * switches the power mode, it should always check the status to check whether it
 * runs into the specified mode or not. The application  should  check
 * this mode before switching to a different mode. The system  requires that
 * only certain modes can switch to other specific modes. See the
 * reference manual for details and the smc_power_state_t for information about
 * the power status.
 *
 * @param base SMC peripheral base address.
 * @return Current power mode status.
 */
static inline smc_power_state_t SMC_GetPowerModeState(SMC_Type *base)
{
    return (smc_power_state_t)base->PMSTAT;
}

/*!
 * @brief Prepares to enter stop modes.
 *
 * This function should be called before entering STOP/VLPS/LLS/VLLS modes.
 */
void SMC_PreEnterStopModes(void);

/*!
 * @brief Recovers after wake up from stop modes.
 *
 * This function should be called after wake up from STOP/VLPS/LLS/VLLS modes.
 * It is used with @ref SMC_PreEnterStopModes.
 */
void SMC_PostExitStopModes(void);

/*!
 * @brief Prepares to enter wait modes.
 *
 * This function should be called before entering WAIT/VLPW modes.
 */
void SMC_PreEnterWaitModes(void);

/*!
 * @brief Recovers after wake up from stop modes.
 *
 * This function should be called after wake up from WAIT/VLPW modes.
 * It is used with @ref SMC_PreEnterWaitModes.
 */
void SMC_PostExitWaitModes(void);

/*!
 * @brief Configures the system to RUN power mode.
 *
 * @param base SMC peripheral base address.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeRun(SMC_Type *base);

#if (defined(FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE) && FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE)
/*!
 * @brief Configures the system to HSRUN power mode.
 *
 * @param base SMC peripheral base address.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeHsrun(SMC_Type *base);
#endif /* FSL_FEATURE_SMC_HAS_HIGH_SPEED_RUN_MODE */

/*!
 * @brief Configures the system to WAIT power mode.
 *
 * @param base SMC peripheral base address.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeWait(SMC_Type *base);

/*!
 * @brief Configures the system to Stop power mode.
 *
 * @param base SMC peripheral base address.
 * @param  option Partial Stop mode option.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeStop(SMC_Type *base, smc_partial_stop_option_t option);

#if (defined(FSL_FEATURE_SMC_HAS_LPWUI) && FSL_FEATURE_SMC_HAS_LPWUI)
/*!
 * @brief Configures the system to VLPR power mode.
 *
 * @param base SMC peripheral base address.
 * @param  wakeupMode Enter Normal Run mode if true, else stay in VLPR mode.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlpr(SMC_Type *base, bool wakeupMode);
#else
/*!
 * @brief Configures the system to VLPR power mode.
 *
 * @param base SMC peripheral base address.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlpr(SMC_Type *base);
#endif /* FSL_FEATURE_SMC_HAS_LPWUI */

/*!
 * @brief Configures the system to VLPW power mode.
 *
 * @param base SMC peripheral base address.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlpw(SMC_Type *base);

/*!
 * @brief Configures the system to VLPS power mode.
 *
 * @param base SMC peripheral base address.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlps(SMC_Type *base);

#if (defined(FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE)
#if ((defined(FSL_FEATURE_SMC_HAS_LLS_SUBMODE) && FSL_FEATURE_SMC_HAS_LLS_SUBMODE) || \
     (defined(FSL_FEATURE_SMC_HAS_LPOPO) && FSL_FEATURE_SMC_HAS_LPOPO))
/*!
 * @brief Configures the system to LLS power mode.
 *
 * @param base SMC peripheral base address.
 * @param  config The LLS power mode configuration structure
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeLls(SMC_Type *base, const smc_power_mode_lls_config_t *config);
#else
/*!
 * @brief Configures the system to LLS power mode.
 *
 * @param base SMC peripheral base address.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeLls(SMC_Type *base);
#endif
#endif /* FSL_FEATURE_SMC_HAS_LOW_LEAKAGE_STOP_MODE */

#if (defined(FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE) && FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE)
/*!
 * @brief Configures the system to VLLS power mode.
 *
 * @param base SMC peripheral base address.
 * @param  config The VLLS power mode configuration structure.
 * @return SMC configuration error code.
 */
status_t SMC_SetPowerModeVlls(SMC_Type *base, const smc_power_mode_vlls_config_t *config);
#endif /* FSL_FEATURE_SMC_HAS_VERY_LOW_LEAKAGE_STOP_MODE */

/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*! @}*/

#endif /* _FSL_SMC_H_ */
