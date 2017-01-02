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
#ifndef _FSL_RCM_H_
#define _FSL_RCM_H_

#include "fsl_common.h"

/*! @addtogroup rcm */
/*! @{*/

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief RCM driver version 2.0.0. */
#define FSL_RCM_DRIVER_VERSION (MAKE_VERSION(2, 0, 0))
/*@}*/

/*!
 * @brief System Reset Source Name definitions
 */
typedef enum _rcm_reset_source
{
#if (defined(FSL_FEATURE_RCM_REG_WIDTH) && (FSL_FEATURE_RCM_REG_WIDTH == 32))
/* RCM register bit width is 32. */
#if (defined(FSL_FEATURE_RCM_HAS_WAKEUP) && FSL_FEATURE_RCM_HAS_WAKEUP)
    kRCM_SourceWakeup = RCM_SRS_WAKEUP_MASK, /*!< Low-leakage wakeup reset */
#endif
    kRCM_SourceLvd = RCM_SRS_LVD_MASK, /*!< low voltage detect reset */
#if (defined(FSL_FEATURE_RCM_HAS_LOC) && FSL_FEATURE_RCM_HAS_LOC)
    kRCM_SourceLoc = RCM_SRS_LOC_MASK, /*!< Loss of clock reset */
#endif                                 /* FSL_FEATURE_RCM_HAS_LOC */
#if (defined(FSL_FEATURE_RCM_HAS_LOL) && FSL_FEATURE_RCM_HAS_LOL)
    kRCM_SourceLol = RCM_SRS_LOL_MASK,   /*!< Loss of lock reset */
#endif                                   /* FSL_FEATURE_RCM_HAS_LOL */
    kRCM_SourceWdog = RCM_SRS_WDOG_MASK, /*!< Watchdog reset */
    kRCM_SourcePin = RCM_SRS_PIN_MASK,   /*!< External pin reset */
    kRCM_SourcePor = RCM_SRS_POR_MASK,   /*!< Power on reset */
#if (defined(FSL_FEATURE_RCM_HAS_JTAG) && FSL_FEATURE_RCM_HAS_JTAG)
    kRCM_SourceJtag = RCM_SRS_JTAG_MASK,     /*!< JTAG generated reset */
#endif                                       /* FSL_FEATURE_RCM_HAS_JTAG */
    kRCM_SourceLockup = RCM_SRS_LOCKUP_MASK, /*!< Core lock up reset */
    kRCM_SourceSw = RCM_SRS_SW_MASK,         /*!< Software reset */
#if (defined(FSL_FEATURE_RCM_HAS_MDM_AP) && FSL_FEATURE_RCM_HAS_MDM_AP)
    kRCM_SourceMdmap = RCM_SRS_MDM_AP_MASK, /*!< MDM-AP system reset */
#endif                                      /* FSL_FEATURE_RCM_HAS_MDM_AP */
#if (defined(FSL_FEATURE_RCM_HAS_EZPORT) && FSL_FEATURE_RCM_HAS_EZPORT)
    kRCM_SourceEzpt = RCM_SRS_EZPT_MASK,       /*!< EzPort reset */
#endif                                         /* FSL_FEATURE_RCM_HAS_EZPORT */
    kRCM_SourceSackerr = RCM_SRS_SACKERR_MASK, /*!< Parameter could get all reset flags */

#else /* (FSL_FEATURE_RCM_REG_WIDTH == 32) */
/* RCM register bit width is 8. */
#if (defined(FSL_FEATURE_RCM_HAS_WAKEUP) && FSL_FEATURE_RCM_HAS_WAKEUP)
    kRCM_SourceWakeup = RCM_SRS0_WAKEUP_MASK, /*!< Low-leakage wakeup reset */
#endif
    kRCM_SourceLvd = RCM_SRS0_LVD_MASK, /*!< low voltage detect reset */
#if (defined(FSL_FEATURE_RCM_HAS_LOC) && FSL_FEATURE_RCM_HAS_LOC)
    kRCM_SourceLoc = RCM_SRS0_LOC_MASK,   /*!< Loss of clock reset */
#endif /* FSL_FEATURE_RCM_HAS_LOC */
#if (defined(FSL_FEATURE_RCM_HAS_LOL) && FSL_FEATURE_RCM_HAS_LOL)
    kRCM_SourceLol = RCM_SRS0_LOL_MASK,   /*!< Loss of lock reset */
#endif /* FSL_FEATURE_RCM_HAS_LOL */
    kRCM_SourceWdog = RCM_SRS0_WDOG_MASK, /*!< Watchdog reset */
    kRCM_SourcePin = RCM_SRS0_PIN_MASK,   /*!< External pin reset */
    kRCM_SourcePor = RCM_SRS0_POR_MASK, /*!< Power on reset */
#if (defined(FSL_FEATURE_RCM_HAS_JTAG) && FSL_FEATURE_RCM_HAS_JTAG)
    kRCM_SourceJtag = RCM_SRS1_JTAG_MASK << 8U,     /*!< JTAG generated reset */
#endif /* FSL_FEATURE_RCM_HAS_JTAG */
    kRCM_SourceLockup = RCM_SRS1_LOCKUP_MASK << 8U, /*!< Core lock up reset */
    kRCM_SourceSw = RCM_SRS1_SW_MASK, /*!< Software reset */
#if (defined(FSL_FEATURE_RCM_HAS_MDM_AP) && FSL_FEATURE_RCM_HAS_MDM_AP)
    kRCM_SourceMdmap = RCM_SRS1_MDM_AP_MASK << 8U,    /*!< MDM-AP system reset */
#endif /* FSL_FEATURE_RCM_HAS_MDM_AP */
#if (defined(FSL_FEATURE_RCM_HAS_EZPORT) && FSL_FEATURE_RCM_HAS_EZPORT)
    kRCM_SourceEzpt = RCM_SRS1_EZPT_MASK << 8U,       /*!< EzPort reset */
#endif /* FSL_FEATURE_RCM_HAS_EZPORT */
    kRCM_SourceSackerr = RCM_SRS1_SACKERR_MASK << 8U, /*!< Parameter could get all reset flags */
#endif /* (FSL_FEATURE_RCM_REG_WIDTH == 32) */
    kRCM_SourceAll = 0xffffffffU,
} rcm_reset_source_t;

/*!
 * @brief Reset pin filter select in Run and Wait modes
 */
typedef enum _rcm_run_wait_filter_mode
{
    kRCM_FilterDisable = 0U,  /*!< All filtering disabled */
    kRCM_FilterBusClock = 1U, /*!< Bus clock filter enabled */
    kRCM_FilterLpoClock = 2U  /*!< LPO clock filter enabled */
} rcm_run_wait_filter_mode_t;

#if (defined(FSL_FEATURE_RCM_HAS_BOOTROM) && FSL_FEATURE_RCM_HAS_BOOTROM)
/*!
 * @brief Boot from ROM configuration.
 */
typedef enum _rcm_boot_rom_config
{
    kRCM_BootFlash = 0U,   /*!< Boot from flash */
    kRCM_BootRomCfg0 = 1U, /*!< Boot from boot ROM due to BOOTCFG0 */
    kRCM_BootRomFopt = 2U, /*!< Boot from boot ROM due to FOPT[7] */
    kRCM_BootRomBoth = 3U  /*!< Boot from boot ROM due to both BOOTCFG0 and FOPT[7] */
} rcm_boot_rom_config_t;
#endif /* FSL_FEATURE_RCM_HAS_BOOTROM */

#if (defined(FSL_FEATURE_RCM_HAS_SRIE) && FSL_FEATURE_RCM_HAS_SRIE)
/*!
 * @brief Max delay time from interrupt asserts to system reset.
 */
typedef enum _rcm_reset_delay
{
    kRCM_ResetDelay8Lpo = 0U,   /*!< Delay 8 LPO cycles.   */
    kRCM_ResetDelay32Lpo = 1U,  /*!< Delay 32 LPO cycles.  */
    kRCM_ResetDelay128Lpo = 2U, /*!< Delay 128 LPO cycles. */
    kRCM_ResetDelay512Lpo = 3U  /*!< Delay 512 LPO cycles. */
} rcm_reset_delay_t;

/*!
 * @brief System reset interrupt enable bit definitions.
 */
typedef enum _rcm_interrupt_enable
{
    kRCM_IntNone = 0U,                              /*!< No interrupt enabled.           */
    kRCM_IntLossOfClk = RCM_SRIE_LOC_MASK,          /*!< Loss of clock interrupt.        */
    kRCM_IntLossOfLock = RCM_SRIE_LOL_MASK,         /*!< Loss of lock interrupt.         */
    kRCM_IntWatchDog = RCM_SRIE_WDOG_MASK,          /*!< Watch dog interrupt.            */
    kRCM_IntExternalPin = RCM_SRIE_PIN_MASK,        /*!< External pin interrupt.         */
    kRCM_IntGlobal = RCM_SRIE_GIE_MASK,             /*!< Global interrupts.              */
    kRCM_IntCoreLockup = RCM_SRIE_LOCKUP_MASK,      /*!< Core lock up interrupt           */
    kRCM_IntSoftware = RCM_SRIE_SW_MASK,            /*!< software interrupt              */
    kRCM_IntStopModeAckErr = RCM_SRIE_SACKERR_MASK, /*!< Stop mode ACK error interrupt.  */
#if (defined(FSL_FEATURE_RCM_HAS_CORE1) && FSL_FEATURE_RCM_HAS_CORE1)
    kRCM_IntCore1 = RCM_SRIE_CORE1_MASK, /*!< Core 1 interrupt.               */
#endif
    kRCM_IntAll = RCM_SRIE_LOC_MASK /*!< Enable all interrupts.          */
                  |
                  RCM_SRIE_LOL_MASK | RCM_SRIE_WDOG_MASK | RCM_SRIE_PIN_MASK | RCM_SRIE_GIE_MASK |
                  RCM_SRIE_LOCKUP_MASK | RCM_SRIE_SW_MASK | RCM_SRIE_SACKERR_MASK
#if (defined(FSL_FEATURE_RCM_HAS_CORE1) && FSL_FEATURE_RCM_HAS_CORE1)
                  |
                  RCM_SRIE_CORE1_MASK
#endif
} rcm_interrupt_enable_t;
#endif /* FSL_FEATURE_RCM_HAS_SRIE */

#if (defined(FSL_FEATURE_RCM_HAS_VERID) && FSL_FEATURE_RCM_HAS_VERID)
/*!
 * @brief IP version ID definition.
 */
typedef struct _rcm_version_id
{
    uint16_t feature; /*!< Feature Specification Number. */
    uint8_t minor;    /*!< Minor version number.         */
    uint8_t major;    /*!< Major version number.         */
} rcm_version_id_t;
#endif

/*!
 * @brief Reset pin filter configuration
 */
typedef struct _rcm_reset_pin_filter_config
{
    bool enableFilterInStop;                    /*!< Reset pin filter select in stop mode. */
    rcm_run_wait_filter_mode_t filterInRunWait; /*!< Reset pin filter in run/wait mode. */
    uint8_t busClockFilterCount;                /*!< Reset pin bus clock filter width.  */
} rcm_reset_pin_filter_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus*/

/*! @name Reset Control Module APIs*/
/*@{*/

#if (defined(FSL_FEATURE_RCM_HAS_VERID) && FSL_FEATURE_RCM_HAS_VERID)
/*!
 * @brief Gets the RCM version ID.
 *
 * This function gets the RCM version ID including the major version number,
 * the minor version number, and the feature specification number.
 *
 * @param base RCM peripheral base address.
 * @param versionId     Pointer to version ID structure.
 */
static inline void RCM_GetVersionId(RCM_Type *base, rcm_version_id_t *versionId)
{
    *((uint32_t *)versionId) = base->VERID;
}
#endif

#if (defined(FSL_FEATURE_RCM_HAS_PARAM) && FSL_FEATURE_RCM_HAS_PARAM)
/*!
 * @brief Gets the reset source implemented status.
 *
 * This function gets the RCM parameter that indicates whether the corresponding reset source is implemented.
 * Use source masks defined in the rcm_reset_source_t to get the desired source status.
 *
 * Example:
   @code
   uint32_t status;

   // To test whether the MCU is reset using Watchdog.
   status = RCM_GetResetSourceImplementedStatus(RCM) & (kRCM_SourceWdog | kRCM_SourcePin);
   @endcode
 *
 * @param base RCM peripheral base address.
 * @return All reset source implemented status bit map.
 */
static inline uint32_t RCM_GetResetSourceImplementedStatus(RCM_Type *base)
{
    return base->PARAM;
}
#endif /* FSL_FEATURE_RCM_HAS_PARAM */

/*!
 * @brief Gets the reset source status which caused a previous reset.
 *
 * This function gets the current reset source status. Use source masks
 * defined in the rcm_reset_source_t to get the desired source status.
 *
 * Example:
   @code
   uint32_t resetStatus;

   // To get all reset source statuses.
   resetStatus = RCM_GetPreviousResetSources(RCM) & kRCM_SourceAll;

   // To test whether the MCU is reset using Watchdog.
   resetStatus = RCM_GetPreviousResetSources(RCM) & kRCM_SourceWdog;

   // To test multiple reset sources.
   resetStatus = RCM_GetPreviousResetSources(RCM) & (kRCM_SourceWdog | kRCM_SourcePin);
   @endcode
 *
 * @param base RCM peripheral base address.
 * @return All reset source status bit map.
 */
static inline uint32_t RCM_GetPreviousResetSources(RCM_Type *base)
{
#if (defined(FSL_FEATURE_RCM_REG_WIDTH) && (FSL_FEATURE_RCM_REG_WIDTH == 32))
    return base->SRS;
#else
    return (uint32_t)((uint32_t)base->SRS0 | ((uint32_t)base->SRS1 << 8U));
#endif /* (FSL_FEATURE_RCM_REG_WIDTH == 32) */
}

#if (defined(FSL_FEATURE_RCM_HAS_SSRS) && FSL_FEATURE_RCM_HAS_SSRS)
/*!
 * @brief Gets the sticky reset source status.
 *
 * This function gets the current reset source status that has not been cleared
 * by software for some specific source.
 *
 * Example:
   @code
   uint32_t resetStatus;

   // To get all reset source statuses.
   resetStatus = RCM_GetStickyResetSources(RCM) & kRCM_SourceAll;

   // To test whether the MCU is reset using Watchdog.
   resetStatus = RCM_GetStickyResetSources(RCM) & kRCM_SourceWdog;

   // To test multiple reset sources.
   resetStatus = RCM_GetStickyResetSources(RCM) & (kRCM_SourceWdog | kRCM_SourcePin);
   @endcode
 *
 * @param base RCM peripheral base address.
 * @return All reset source status bit map.
 */
static inline uint32_t RCM_GetStickyResetSources(RCM_Type *base)
{
#if (defined(FSL_FEATURE_RCM_REG_WIDTH) && (FSL_FEATURE_RCM_REG_WIDTH == 32))
    return base->SSRS;
#else
    return (base->SSRS0 | ((uint32_t)base->SSRS1 << 8U));
#endif /* (FSL_FEATURE_RCM_REG_WIDTH == 32) */
}

/*!
 * @brief Clears the sticky reset source status.
 *
 * This function clears the sticky system reset flags indicated by source masks.
 *
 * Example:
   @code
   // Clears multiple reset sources.
   RCM_ClearStickyResetSources(kRCM_SourceWdog | kRCM_SourcePin);
   @endcode
 *
 * @param base RCM peripheral base address.
 * @param sourceMasks reset source status bit map
 */
static inline void RCM_ClearStickyResetSources(RCM_Type *base, uint32_t sourceMasks)
{
#if (defined(FSL_FEATURE_RCM_REG_WIDTH) && (FSL_FEATURE_RCM_REG_WIDTH == 32))
    base->SSRS = sourceMasks;
#else
    base->SSRS0 = (sourceMasks & 0xffU);
    base->SSRS1 = ((sourceMasks >> 8U) & 0xffU);
#endif /* (FSL_FEATURE_RCM_REG_WIDTH == 32) */
}
#endif /* FSL_FEATURE_RCM_HAS_SSRS */

/*!
 * @brief Configures the reset pin filter.
 *
 * This function sets the reset pin filter including the filter source, filter
 * width, and so on.
 *
 * @param base RCM peripheral base address.
 * @param config Pointer to the configuration structure.
 */
void RCM_ConfigureResetPinFilter(RCM_Type *base, const rcm_reset_pin_filter_config_t *config);

#if (defined(FSL_FEATURE_RCM_HAS_EZPMS) && FSL_FEATURE_RCM_HAS_EZPMS)
/*!
 * @brief Gets the EZP_MS_B pin assert status.
 *
 * This function gets the easy port mode status (EZP_MS_B) pin assert status.
 *
 * @param base RCM peripheral base address.
 * @return status  true - asserted, false - reasserted
 */
static inline bool RCM_GetEasyPortModePinStatus(RCM_Type *base)
{
    return (bool)(base->MR & RCM_MR_EZP_MS_MASK);
}
#endif /* FSL_FEATURE_RCM_HAS_EZPMS */

#if (defined(FSL_FEATURE_RCM_HAS_BOOTROM) && FSL_FEATURE_RCM_HAS_BOOTROM)
/*!
 * @brief Gets the ROM boot source.
 *
 * This function gets the ROM boot source during the last chip reset.
 *
 * @param base RCM peripheral base address.
 * @return The ROM boot source.
 */
static inline rcm_boot_rom_config_t RCM_GetBootRomSource(RCM_Type *base)
{
    return (rcm_boot_rom_config_t)((base->MR & RCM_MR_BOOTROM_MASK) >> RCM_MR_BOOTROM_SHIFT);
}

/*!
 * @brief Clears the ROM boot source flag.
 *
 * This function clears the ROM boot source flag.
 *
 * @param base     Register base address of RCM
 */
static inline void RCM_ClearBootRomSource(RCM_Type *base)
{
    base->MR |= RCM_MR_BOOTROM_MASK;
}

/*!
 * @brief Forces the boot from ROM.
 *
 * This function forces booting from ROM during all subsequent system resets.
 *
 * @param base RCM peripheral base address.
 * @param config   Boot configuration.
 */
void RCM_SetForceBootRomSource(RCM_Type *base, rcm_boot_rom_config_t config);
#endif /* FSL_FEATURE_RCM_HAS_BOOTROM */

#if (defined(FSL_FEATURE_RCM_HAS_SRIE) && FSL_FEATURE_RCM_HAS_SRIE)
/*!
 * @brief Sets the system reset interrupt configuration.
 *
 * For graceful shutdown, the RCM supports delaying the assertion of the system
 * reset for a period of time when the reset interrupt is generated. This function
 * can be used to enable the interrupt and the delay period. The interrupts
 * are passed in as bit mask. See rcm_int_t for details. For example, to
 * delay a reset for 512 LPO cycles after the WDOG timeout or loss-of-clock occurs,
 * configure as follows:
 * RCM_SetSystemResetInterruptConfig(kRCM_IntWatchDog | kRCM_IntLossOfClk, kRCM_ResetDelay512Lpo);
 *
 * @param base RCM peripheral base address.
 * @param intMask   Bit mask of the system reset interrupts to enable. See
 *                  rcm_interrupt_enable_t for details.
 * @param Delay     Bit mask of the system reset interrupts to enable.
 */
static inline void RCM_SetSystemResetInterruptConfig(RCM_Type *base, uint32_t intMask, rcm_reset_delay_t delay)
{
    base->SRIE = (intMask | delay);
}
#endif /* FSL_FEATURE_RCM_HAS_SRIE */
/*@}*/

#if defined(__cplusplus)
}
#endif /* __cplusplus*/

/*! @}*/

#endif /* _FSL_RCM_H_ */
