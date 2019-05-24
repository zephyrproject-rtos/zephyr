/*
 * Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRF_POWER_H__
#define NRF_POWER_H__

#include <nrfx.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrf_power_hal POWER HAL
 * @{
 * @ingroup nrf_power
 * @brief   Hardware access layer for managing the POWER peripheral.
 */

#if defined(POWER_INTENSET_SLEEPENTER_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether sleep events are present. */
#define NRF_POWER_HAS_SLEEPEVT 1
#else
#define NRF_POWER_HAS_SLEEPEVT 0
#endif // defined(POWER_INTENSET_SLEEPENTER_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(POWER_USBREGSTATUS_VBUSDETECT_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether the POWER peripheral controls the USB regulator. */
#define NRF_POWER_HAS_USBREG 1
#else
#define NRF_POWER_HAS_USBREG 0
#endif // defined(POWER_USBREGSTATUS_VBUSDETECT_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(POWER_POFCON_THRESHOLDVDDH_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether VDDH is present. */
#define NRF_POWER_HAS_VDDH 1
#else
#define NRF_POWER_HAS_VDDH 0
#endif // defined(POWER_POFCON_THRESHOLDVDDH_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(POWER_DCDCEN_DCDCEN_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether DCDCEN is present. */
#define NRF_POWER_HAS_DCDCEN 1
#else
#define NRF_POWER_HAS_DCDCEN 0
#endif

#if defined(POWER_POFCON_THRESHOLD_Msk) || defined(__NRFX_DOXYGEN__)
/** @brief Symbol indicating whether POFCON is present. */
#define NRF_POWER_HAS_POFCON 1
#else
#define NRF_POWER_HAS_POFCON 0
#endif

/** @brief POWER tasks. */
typedef enum
{
    NRF_POWER_TASK_CONSTLAT  = offsetof(NRF_POWER_Type, TASKS_CONSTLAT), /**< Enable constant latency mode. */
    NRF_POWER_TASK_LOWPWR    = offsetof(NRF_POWER_Type, TASKS_LOWPWR  ), /**< Enable low-power mode (variable latency). */
} nrf_power_task_t;

/** @brief POWER events. */
typedef enum
{
#if NRF_POWER_HAS_POFCON
    NRF_POWER_EVENT_POFWARN      = offsetof(NRF_POWER_Type, EVENTS_POFWARN    ), /**< Power failure warning. */
#endif
#if NRF_POWER_HAS_SLEEPEVT
    NRF_POWER_EVENT_SLEEPENTER   = offsetof(NRF_POWER_Type, EVENTS_SLEEPENTER ), /**< CPU entered WFI/WFE sleep. */
    NRF_POWER_EVENT_SLEEPEXIT    = offsetof(NRF_POWER_Type, EVENTS_SLEEPEXIT  ), /**< CPU exited WFI/WFE sleep. */
#endif
#if NRF_POWER_HAS_USBREG
    NRF_POWER_EVENT_USBDETECTED  = offsetof(NRF_POWER_Type, EVENTS_USBDETECTED), /**< Voltage supply detected on VBUS. */
    NRF_POWER_EVENT_USBREMOVED   = offsetof(NRF_POWER_Type, EVENTS_USBREMOVED ), /**< Voltage supply removed from VBUS. */
    NRF_POWER_EVENT_USBPWRRDY    = offsetof(NRF_POWER_Type, EVENTS_USBPWRRDY  ), /**< USB 3.3&nbsp;V supply ready. */
#endif
} nrf_power_event_t;

/** @brief POWER interrupts. */
typedef enum
{
#if NRF_POWER_HAS_POFCON
    NRF_POWER_INT_POFWARN_MASK     = POWER_INTENSET_POFWARN_Msk    , /**< Write '1' to Enable interrupt for POFWARN event. */
#endif
#if NRF_POWER_HAS_SLEEPEVT
    NRF_POWER_INT_SLEEPENTER_MASK  = POWER_INTENSET_SLEEPENTER_Msk , /**< Write '1' to Enable interrupt for SLEEPENTER event. */
    NRF_POWER_INT_SLEEPEXIT_MASK   = POWER_INTENSET_SLEEPEXIT_Msk  , /**< Write '1' to Enable interrupt for SLEEPEXIT event. */
#endif
#if NRF_POWER_HAS_USBREG
    NRF_POWER_INT_USBDETECTED_MASK = POWER_INTENSET_USBDETECTED_Msk, /**< Write '1' to Enable interrupt for USBDETECTED event. */
    NRF_POWER_INT_USBREMOVED_MASK  = POWER_INTENSET_USBREMOVED_Msk , /**< Write '1' to Enable interrupt for USBREMOVED event. */
    NRF_POWER_INT_USBPWRRDY_MASK   = POWER_INTENSET_USBPWRRDY_Msk  , /**< Write '1' to Enable interrupt for USBPWRRDY event. */
#endif
} nrf_power_int_mask_t;

/** @brief Reset reason. */
typedef enum
{
    NRF_POWER_RESETREAS_RESETPIN_MASK = POWER_RESETREAS_RESETPIN_Msk, /*!< Bit mask of RESETPIN field. *///!< NRF_POWER_RESETREAS_RESETPIN_MASK
    NRF_POWER_RESETREAS_DOG_MASK      = POWER_RESETREAS_DOG_Msk     , /*!< Bit mask of DOG field. */     //!< NRF_POWER_RESETREAS_DOG_MASK
    NRF_POWER_RESETREAS_SREQ_MASK     = POWER_RESETREAS_SREQ_Msk    , /*!< Bit mask of SREQ field. */    //!< NRF_POWER_RESETREAS_SREQ_MASK
    NRF_POWER_RESETREAS_LOCKUP_MASK   = POWER_RESETREAS_LOCKUP_Msk  , /*!< Bit mask of LOCKUP field. */  //!< NRF_POWER_RESETREAS_LOCKUP_MASK
    NRF_POWER_RESETREAS_OFF_MASK      = POWER_RESETREAS_OFF_Msk     , /*!< Bit mask of OFF field. */     //!< NRF_POWER_RESETREAS_OFF_MASK
#if defined(POWER_RESETREAS_LPCOMP_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_POWER_RESETREAS_LPCOMP_MASK   = POWER_RESETREAS_LPCOMP_Msk  , /*!< Bit mask of LPCOMP field. */  //!< NRF_POWER_RESETREAS_LPCOMP_MASK
#endif
    NRF_POWER_RESETREAS_DIF_MASK      = POWER_RESETREAS_DIF_Msk     , /*!< Bit mask of DIF field. */     //!< NRF_POWER_RESETREAS_DIF_MASK
#if defined(POWER_RESETREAS_NFC_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_POWER_RESETREAS_NFC_MASK      = POWER_RESETREAS_NFC_Msk     , /*!< Bit mask of NFC field. */
#endif
#if defined(POWER_RESETREAS_VBUS_Msk) || defined(__NRFX_DOXYGEN__)
    NRF_POWER_RESETREAS_VBUS_MASK     = POWER_RESETREAS_VBUS_Msk    , /*!< Bit mask of VBUS field. */
#endif
} nrf_power_resetreas_mask_t;

#if NRF_POWER_HAS_USBREG
/**
 * @brief USBREGSTATUS register bit masks
 *
 * @sa nrf_power_usbregstatus_get
 */
typedef enum
{
    NRF_POWER_USBREGSTATUS_VBUSDETECT_MASK = POWER_USBREGSTATUS_VBUSDETECT_Msk, /**< USB detected or removed.     */
    NRF_POWER_USBREGSTATUS_OUTPUTRDY_MASK  = POWER_USBREGSTATUS_OUTPUTRDY_Msk   /**< USB 3.3&nbsp;V supply ready. */
} nrf_power_usbregstatus_mask_t;
#endif // NRF_POWER_HAS_USBREG

#if defined(POWER_RAMSTATUS_RAMBLOCK0_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Numbers of RAM blocks
 *
 * @sa nrf_power_ramblock_mask_t
 * @note
 * RAM blocks must be used in nRF51.
 * In newer SoCs, RAM is divided into segments and this functionality is not supported.
 * See the PS for mapping between the internal RAM and RAM blocks, because this
 * mapping is not 1:1, and functions related to old style blocks must not be used.
 */
typedef enum
{
    NRF_POWER_RAMBLOCK0 = POWER_RAMSTATUS_RAMBLOCK0_Pos,
    NRF_POWER_RAMBLOCK1 = POWER_RAMSTATUS_RAMBLOCK1_Pos,
    NRF_POWER_RAMBLOCK2 = POWER_RAMSTATUS_RAMBLOCK2_Pos,
    NRF_POWER_RAMBLOCK3 = POWER_RAMSTATUS_RAMBLOCK3_Pos
} nrf_power_ramblock_t;

/**
 * @brief Masks of RAM blocks.
 *
 * @sa nrf_power_ramblock_t
 */
typedef enum
{
    NRF_POWER_RAMBLOCK0_MASK = POWER_RAMSTATUS_RAMBLOCK0_Msk,
    NRF_POWER_RAMBLOCK1_MASK = POWER_RAMSTATUS_RAMBLOCK1_Msk,
    NRF_POWER_RAMBLOCK2_MASK = POWER_RAMSTATUS_RAMBLOCK2_Msk,
    NRF_POWER_RAMBLOCK3_MASK = POWER_RAMSTATUS_RAMBLOCK3_Msk
} nrf_power_ramblock_mask_t;
#endif // defined(POWER_RAMSTATUS_RAMBLOCK0_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief RAM power state position of the bits
 *
 * @sa nrf_power_onoffram_mask_t
 */
typedef enum
{
    NRF_POWER_ONRAM0,  /**< Keep RAM block 0 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM0, /**< Keep retention on RAM block 0 when RAM block is switched OFF. */
    NRF_POWER_ONRAM1,  /**< Keep RAM block 1 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM1, /**< Keep retention on RAM block 1 when RAM block is switched OFF. */
    NRF_POWER_ONRAM2,  /**< Keep RAM block 2 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM2, /**< Keep retention on RAM block 2 when RAM block is switched OFF. */
    NRF_POWER_ONRAM3,  /**< Keep RAM block 3 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM3, /**< Keep retention on RAM block 3 when RAM block is switched OFF. */
}nrf_power_onoffram_t;

/**
 * @brief RAM power state bit masks
 *
 * @sa nrf_power_onoffram_t
 */
typedef enum
{
    NRF_POWER_ONRAM0_MASK  = 1U << NRF_POWER_ONRAM0,  /**< Keep RAM block 0 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM0_MASK = 1U << NRF_POWER_OFFRAM0, /**< Keep retention on RAM block 0 when RAM block is switched OFF. */
    NRF_POWER_ONRAM1_MASK  = 1U << NRF_POWER_ONRAM1,  /**< Keep RAM block 1 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM1_MASK = 1U << NRF_POWER_OFFRAM1, /**< Keep retention on RAM block 1 when RAM block is switched OFF. */
    NRF_POWER_ONRAM2_MASK  = 1U << NRF_POWER_ONRAM2,  /**< Keep RAM block 2 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM2_MASK = 1U << NRF_POWER_OFFRAM2, /**< Keep retention on RAM block 2 when RAM block is switched OFF. */
    NRF_POWER_ONRAM3_MASK  = 1U << NRF_POWER_ONRAM3,  /**< Keep RAM block 3 ON or OFF in System ON mode.                 */
    NRF_POWER_OFFRAM3_MASK = 1U << NRF_POWER_OFFRAM3, /**< Keep retention on RAM block 3 when RAM block is switched OFF. */
}nrf_power_onoffram_mask_t;

#if NRF_POWER_HAS_POFCON
/** @brief Power failure comparator thresholds. */
typedef enum
{
    NRF_POWER_POFTHR_V21 = POWER_POFCON_THRESHOLD_V21, /**< Set threshold to 2.1&nbsp;V. */
    NRF_POWER_POFTHR_V23 = POWER_POFCON_THRESHOLD_V23, /**< Set threshold to 2.3&nbsp;V. */
    NRF_POWER_POFTHR_V25 = POWER_POFCON_THRESHOLD_V25, /**< Set threshold to 2.5&nbsp;V. */
    NRF_POWER_POFTHR_V27 = POWER_POFCON_THRESHOLD_V27, /**< Set threshold to 2.7&nbsp;V. */
#if defined(POWER_POFCON_THRESHOLD_V17) || defined(__NRFX_DOXYGEN__)
    NRF_POWER_POFTHR_V17 = POWER_POFCON_THRESHOLD_V17, /**< Set threshold to 1.7&nbsp;V. */
    NRF_POWER_POFTHR_V18 = POWER_POFCON_THRESHOLD_V18, /**< Set threshold to 1.8&nbsp;V. */
    NRF_POWER_POFTHR_V19 = POWER_POFCON_THRESHOLD_V19, /**< Set threshold to 1.9&nbsp;V. */
    NRF_POWER_POFTHR_V20 = POWER_POFCON_THRESHOLD_V20, /**< Set threshold to 2.0&nbsp;V. */
    NRF_POWER_POFTHR_V22 = POWER_POFCON_THRESHOLD_V22, /**< Set threshold to 2.2&nbsp;V. */
    NRF_POWER_POFTHR_V24 = POWER_POFCON_THRESHOLD_V24, /**< Set threshold to 2.4&nbsp;V. */
    NRF_POWER_POFTHR_V26 = POWER_POFCON_THRESHOLD_V26, /**< Set threshold to 2.6&nbsp;V. */
    NRF_POWER_POFTHR_V28 = POWER_POFCON_THRESHOLD_V28, /**< Set threshold to 2.8&nbsp;V. */
#endif // defined(POWER_POFCON_THRESHOLD_V17) || defined(__NRFX_DOXYGEN__)
} nrf_power_pof_thr_t;
#endif // NRF_POWER_HAS_POFCON

#if NRF_POWER_HAS_VDDH
/** @brief Power failure comparator thresholds for VDDH. */
typedef enum
{
    NRF_POWER_POFTHRVDDH_V27 = POWER_POFCON_THRESHOLDVDDH_V27, /**< Set threshold to 2.7&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V28 = POWER_POFCON_THRESHOLDVDDH_V28, /**< Set threshold to 2.8&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V29 = POWER_POFCON_THRESHOLDVDDH_V29, /**< Set threshold to 2.9&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V30 = POWER_POFCON_THRESHOLDVDDH_V30, /**< Set threshold to 3.0&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V31 = POWER_POFCON_THRESHOLDVDDH_V31, /**< Set threshold to 3.1&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V32 = POWER_POFCON_THRESHOLDVDDH_V32, /**< Set threshold to 3.2&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V33 = POWER_POFCON_THRESHOLDVDDH_V33, /**< Set threshold to 3.3&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V34 = POWER_POFCON_THRESHOLDVDDH_V34, /**< Set threshold to 3.4&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V35 = POWER_POFCON_THRESHOLDVDDH_V35, /**< Set threshold to 3.5&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V36 = POWER_POFCON_THRESHOLDVDDH_V36, /**< Set threshold to 3.6&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V37 = POWER_POFCON_THRESHOLDVDDH_V37, /**< Set threshold to 3.7&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V38 = POWER_POFCON_THRESHOLDVDDH_V38, /**< Set threshold to 3.8&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V39 = POWER_POFCON_THRESHOLDVDDH_V39, /**< Set threshold to 3.9&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V40 = POWER_POFCON_THRESHOLDVDDH_V40, /**< Set threshold to 4.0&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V41 = POWER_POFCON_THRESHOLDVDDH_V41, /**< Set threshold to 4.1&nbsp;V. */
    NRF_POWER_POFTHRVDDH_V42 = POWER_POFCON_THRESHOLDVDDH_V42, /**< Set threshold to 4.2&nbsp;V. */
} nrf_power_pof_thrvddh_t;

/** @brief Main regulator status. */
typedef enum
{
    NRF_POWER_MAINREGSTATUS_NORMAL = POWER_MAINREGSTATUS_MAINREGSTATUS_Normal, /**< Normal voltage mode. Voltage supplied on VDD. */
    NRF_POWER_MAINREGSTATUS_HIGH   = POWER_MAINREGSTATUS_MAINREGSTATUS_High    /**< High voltage mode. Voltage supplied on VDDH.  */
} nrf_power_mainregstatus_t;

#endif // NRF_POWER_HAS_VDDH

#if defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Bit positions for RAMPOWER register
 *
 * All possible bits described, even if they are not used in selected MCU.
 */
typedef enum
{
    /** Keep RAM section S0 ON in System ON mode */
    NRF_POWER_RAMPOWER_S0POWER = POWER_RAM_POWER_S0POWER_Pos,
    NRF_POWER_RAMPOWER_S1POWER,  /**< Keep RAM section S1 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S2POWER,  /**< Keep RAM section S2 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S3POWER,  /**< Keep RAM section S3 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S4POWER,  /**< Keep RAM section S4 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S5POWER,  /**< Keep RAM section S5 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S6POWER,  /**< Keep RAM section S6 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S7POWER,  /**< Keep RAM section S7 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S8POWER,  /**< Keep RAM section S8 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S9POWER,  /**< Keep RAM section S9 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S10POWER, /**< Keep RAM section S10 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S11POWER, /**< Keep RAM section S11 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S12POWER, /**< Keep RAM section S12 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S13POWER, /**< Keep RAM section S13 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S14POWER, /**< Keep RAM section S14 ON in System ON mode. */
    NRF_POWER_RAMPOWER_S15POWER, /**< Keep RAM section S15 ON in System ON mode. */

    /** Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S0RETENTION = POWER_RAM_POWER_S0RETENTION_Pos,
    NRF_POWER_RAMPOWER_S1RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S2RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S3RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S4RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S5RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S6RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S7RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S8RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S9RETENTION,  /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S10RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S11RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S12RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S13RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S14RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
    NRF_POWER_RAMPOWER_S15RETENTION, /**< Keep section retention in OFF mode when section is OFF. */
} nrf_power_rampower_t;

/**
 * @brief Bit masks for RAMPOWER register
 *
 * All possible bits described, even if they are not used in selected MCU.
 */
typedef enum
{
    NRF_POWER_RAMPOWER_S0POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S0POWER ,
    NRF_POWER_RAMPOWER_S1POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S1POWER ,
    NRF_POWER_RAMPOWER_S2POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S2POWER ,
    NRF_POWER_RAMPOWER_S3POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S3POWER ,
    NRF_POWER_RAMPOWER_S4POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S4POWER ,
    NRF_POWER_RAMPOWER_S5POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S5POWER ,
    NRF_POWER_RAMPOWER_S7POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S7POWER ,
    NRF_POWER_RAMPOWER_S8POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S8POWER ,
    NRF_POWER_RAMPOWER_S9POWER_MASK  = 1UL << NRF_POWER_RAMPOWER_S9POWER ,
    NRF_POWER_RAMPOWER_S10POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S10POWER,
    NRF_POWER_RAMPOWER_S11POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S11POWER,
    NRF_POWER_RAMPOWER_S12POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S12POWER,
    NRF_POWER_RAMPOWER_S13POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S13POWER,
    NRF_POWER_RAMPOWER_S14POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S14POWER,
    NRF_POWER_RAMPOWER_S15POWER_MASK = 1UL << NRF_POWER_RAMPOWER_S15POWER,

    NRF_POWER_RAMPOWER_S0RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S0RETENTION ,
    NRF_POWER_RAMPOWER_S1RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S1RETENTION ,
    NRF_POWER_RAMPOWER_S2RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S2RETENTION ,
    NRF_POWER_RAMPOWER_S3RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S3RETENTION ,
    NRF_POWER_RAMPOWER_S4RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S4RETENTION ,
    NRF_POWER_RAMPOWER_S5RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S5RETENTION ,
    NRF_POWER_RAMPOWER_S7RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S7RETENTION ,
    NRF_POWER_RAMPOWER_S8RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S8RETENTION ,
    NRF_POWER_RAMPOWER_S9RETENTION_MASK  = 1UL << NRF_POWER_RAMPOWER_S9RETENTION ,
    NRF_POWER_RAMPOWER_S10RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S10RETENTION,
    NRF_POWER_RAMPOWER_S11RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S11RETENTION,
    NRF_POWER_RAMPOWER_S12RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S12RETENTION,
    NRF_POWER_RAMPOWER_S13RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S13RETENTION,
    NRF_POWER_RAMPOWER_S14RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S14RETENTION,
    NRF_POWER_RAMPOWER_S15RETENTION_MASK = (int)(1UL << NRF_POWER_RAMPOWER_S15RETENTION),
} nrf_power_rampower_mask_t;
#endif // defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for activating a specific POWER task.
 *
 * @param[in] task Task.
 */
__STATIC_INLINE void nrf_power_task_trigger(nrf_power_task_t task);

/**
 * @brief Function for returning the address of a specific POWER task register.
 *
 * @param[in] task Task.
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrf_power_task_address_get(nrf_power_task_t task);

/**
 * @brief Function for clearing a specific event.
 *
 * @param[in] event Event.
 */
__STATIC_INLINE void nrf_power_event_clear(nrf_power_event_t event);

/**
 * @brief Function for retrieving the state of the POWER event.
 *
 * @param[in] event Event to be checked.
 *
 * @retval true  The event has been generated.
 * @retval false The event has not been generated.
 */
__STATIC_INLINE bool nrf_power_event_check(nrf_power_event_t event);

/**
 * @brief Function for getting and clearing the state of specific event
 *
 * This function checks the state of the event and clears it.
 *
 * @param[in] event Event.
 *
 * @retval true  The event was set.
 * @retval false The event was not set.
 */
__STATIC_INLINE bool nrf_power_event_get_and_clear(nrf_power_event_t event);

/**
 * @brief Function for returning the address of a specific POWER event register.
 *
 * @param[in] event Event.
 *
 * @return Address.
 */
__STATIC_INLINE uint32_t nrf_power_event_address_get(nrf_power_event_t event);

/**
 * @brief Function for enabling selected interrupts.
 *
 * @param[in] int_mask Interrupts mask.
 */
__STATIC_INLINE void nrf_power_int_enable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of selected interrupts.
 *
 * @param[in] int_mask Interrupts mask.
 *
 * @retval true  Any of selected interrupts is enabled.
 * @retval false None of selected interrupts is enabled.
 */
__STATIC_INLINE bool nrf_power_int_enable_check(uint32_t int_mask);

/**
 * @brief Function for retrieving the information about enabled interrupts.
 *
 * @return The flags of enabled interrupts.
 */
__STATIC_INLINE uint32_t nrf_power_int_enable_get(void);

/**
 * @brief Function for disabling selected interrupts.
 *
 * @param[in] int_mask Interrupts mask.
 */
__STATIC_INLINE void nrf_power_int_disable(uint32_t int_mask);

#if defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the subscribe configuration for a given
 *        POWER task.
 *
 * @param[in] task    Task for which to set the configuration.
 * @param[in] channel Channel through which to subscribe events.
 */
__STATIC_INLINE void nrf_power_subscribe_set(nrf_power_task_t task,
                                             uint8_t          channel);

/**
 * @brief Function for clearing the subscribe configuration for a given
 *        POWER task.
 *
 * @param[in] task Task for which to clear the configuration.
 */
__STATIC_INLINE void nrf_power_subscribe_clear(nrf_power_task_t task);

/**
 * @brief Function for setting the publish configuration for a given
 *        POWER event.
 *
 * @param[in] event   Event for which to set the configuration.
 * @param[in] channel Channel through which to publish the event.
 */
__STATIC_INLINE void nrf_power_publish_set(nrf_power_event_t event,
                                           uint8_t           channel);

/**
 * @brief Function for clearing the publish configuration for a given
 *        POWER event.
 *
 * @param[in] event Event for which to clear the configuration.
 */
__STATIC_INLINE void nrf_power_publish_clear(nrf_power_event_t event);
#endif // defined(DPPI_PRESENT) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for getting the reset reason bitmask.
 *
 * This function returns the reset reason bitmask.
 * Unless cleared, the RESETREAS register is cumulative.
 * A field is cleared by writing '1' to it (see @ref nrf_power_resetreas_clear).
 * If none of the reset sources is flagged,
 * the chip was reset from the on-chip reset generator,
 * which indicates a power-on-reset or a brown out reset.
 *
 * @return The mask of reset reasons constructed with @ref nrf_power_resetreas_mask_t.
 */
__STATIC_INLINE uint32_t nrf_power_resetreas_get(void);

/**
 * @brief Function for clearing the selected reset reason field.
 *
 * This function clears the selected reset reason field.
 *
 * @param[in] mask The mask constructed from @ref nrf_power_resetreas_mask_t enumerator values.
 * @sa nrf_power_resetreas_get
 */
__STATIC_INLINE void nrf_power_resetreas_clear(uint32_t mask);

#if defined(POWER_POWERSTATUS_LTEMODEM_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting power status of the LTE Modem domain.
 *
 * @retval true  The LTE Modem domain is powered on.
 * @retval false The LTE Modem domain is powered off.
 */
__STATIC_INLINE bool nrf_power_powerstatus_get(void);
#endif

#if defined(POWER_RAMSTATUS_RAMBLOCK0_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting the RAMSTATUS register.
 *
 * @return Value with bits set according to the masks in @ref nrf_power_ramblock_mask_t.
 */
__STATIC_INLINE uint32_t nrf_power_ramstatus_get(void);
#endif // defined(POWER_RAMSTATUS_RAMBLOCK0_Msk) || defined(__NRFX_DOXYGEN__)

#if defined(POWER_SYSTEMOFF_SYSTEMOFF_Enter)
/**
 * @brief Function for going into System OFF mode.
 *
 * This function puts the CPU in System OFF mode.
 * The only way to wake up the CPU is by reset.
 *
 * @note This function never returns.
 */
__STATIC_INLINE void nrf_power_system_off(void);
#endif // defined(POWER_SYSTEMOFF_SYSTEMOFF_Enter)

#if NRF_POWER_HAS_POFCON
/**
 * @brief Function for setting the power failure comparator configuration.
 *
 * This function sets the power failure comparator threshold and enables or disables flag.
 * @note
 * If VDDH settings are present in the device, this function will
 * clear its settings (set to the lowest voltage).
 * Use @ref nrf_power_pofcon_vddh_set function to set new value.
 *
 * @param enabled Sets to true if power failure comparator is to be enabled.
 * @param thr     Sets the voltage threshold value.
 *
 */
__STATIC_INLINE void nrf_power_pofcon_set(bool enabled, nrf_power_pof_thr_t thr);

/**
 * @brief Function for getting the power failure comparator configuration.
 *
 * @param[out] p_enabled Function sets this boolean variable to true
 *                       if power failure comparator is enabled.
 *                       The pointer can be NULL if we do not need this information.
 *
 * @return Threshold setting for power failure comparator.
 */
__STATIC_INLINE nrf_power_pof_thr_t nrf_power_pofcon_get(bool * p_enabled);
#endif // NRF_POWER_HAS_POFCON

#if NRF_POWER_HAS_VDDH
/**
 * @brief Function for setting the VDDH power failure comparator threshold.
 *
 * @param thr Threshold to be set.
 */
__STATIC_INLINE void nrf_power_pofcon_vddh_set(nrf_power_pof_thrvddh_t thr);

/**
 * @brief Function for getting the VDDH power failure comparator threshold.
 *
 * @return VDDH threshold currently configured.
 */
__STATIC_INLINE nrf_power_pof_thrvddh_t nrf_power_pofcon_vddh_get(void);
#endif // NRF_POWER_HAS_VDDH

/**
 * @brief Function for setting the general purpose retention register.
 *
 * @param[in] val Value to be set in the register.
 */
__STATIC_INLINE void nrf_power_gpregret_set(uint8_t val);

/**
 * @brief Function for getting general purpose retention register.
 *
 * @return The value from the register.
 */
__STATIC_INLINE uint8_t nrf_power_gpregret_get(void);

#if defined(POWER_GPREGRET2_GPREGRET_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for setting the general purpose retention register 2.
 *
 * @note This register is not available in the nRF51 MCU family.
 *
 * @param[in] val Value to be set in the register.
 */
__STATIC_INLINE void nrf_power_gpregret2_set(uint8_t val);

/**
 * @brief Function for getting the general purpose retention register 2.
 *
 * @note This register is not available in all MCUs.
 *
 * @return The value from the register.
 */
__STATIC_INLINE uint8_t nrf_power_gpregret2_get(void);
#endif // defined(POWER_GPREGRET2_GPREGRET_Msk) || defined(__NRFX_DOXYGEN__)

/**
 * @brief Function for getting value of the particular general purpose retention register
 *
 * @param[in] reg_num General purpose retention register number.
 *
 * @return The value from the register
 */
__STATIC_INLINE uint8_t nrf_power_gpregret_ext_get(uint8_t reg_num);

/**
 * @brief Function for setting particular general purpose retention register.
 *
 * @param[in] reg_num General purpose retention register number.
 * @param[in] val     Value to be set in the register
 */
__STATIC_INLINE void nrf_power_gpregret_ext_set(uint8_t          reg_num,
                                                uint8_t          val);

#if NRF_POWER_HAS_DCDCEN
/**
 * @brief Enable or disable DCDC converter
 *
 * @note
 * If the device consist of high voltage power input (VDDH), this setting
 * will relate to the converter on low voltage side (1.3&nbsp;V output).
 *
 * @param[in] enable Set true to enable the DCDC converter or false to disable the DCDC converter.
 */
__STATIC_INLINE void nrf_power_dcdcen_set(bool enable);

/**
 * @brief Function for getting the state of the DCDC converter.
 *
 * @note
 * If the device consist of high voltage power input (VDDH), this setting
 * will relate to the converter on low voltage side (1.3&nbsp;V output).
 *
 * @retval true  Converter is enabled.
 * @retval false Converter is disabled.
 */
__STATIC_INLINE bool nrf_power_dcdcen_get(void);
#endif // NRF_POWER_HAS_DCDCEN

#if defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__NRFX_DOXYGEN__)
/**
 * @brief Turn ON sections in the selected RAM block.
 *
 * This function turns ON several sections in one block and also block retention.
 *
 * @sa nrf_power_rampower_mask_t
 * @sa nrf_power_rampower_mask_off
 *
 * @param[in] block        RAM block index.
 * @param[in] section_mask Mask of the sections created by merging
 *                         @ref nrf_power_rampower_mask_t flags.
 */
__STATIC_INLINE void nrf_power_rampower_mask_on(uint8_t block, uint32_t section_mask);

/**
 * @brief Turn ON sections in the selected RAM block.
 *
 * This function turns OFF several sections in one block and also block retention.
 *
 * @sa nrf_power_rampower_mask_t
 * @sa nrf_power_rampower_mask_off
 *
 * @param[in] block        RAM block index.
 * @param[in] section_mask Mask of the sections created by merging
 *                         @ref nrf_power_rampower_mask_t flags.
 */
__STATIC_INLINE void nrf_power_rampower_mask_off(uint8_t block, uint32_t section_mask);

/**
 * @brief Function for getting the ON mask and retention sections in the selected RAM block.
 *
 * @param[in] block RAM block index.
 *
 * @return Mask of sections state composed from @ref nrf_power_rampower_mask_t flags.
 */
__STATIC_INLINE uint32_t nrf_power_rampower_mask_get(uint8_t block);
#endif /* defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__NRFX_DOXYGEN__) */

#if NRF_POWER_HAS_VDDH
/**
 * @brief Function for enabling or disabling the DCDC converter on VDDH.
 *
 * @param enable Set true to enable the DCDC converter or false to disable the DCDC converter.
 */
__STATIC_INLINE void nrf_power_dcdcen_vddh_set(bool enable);

/**
 * @brief Function for getting the state of DCDC converter on VDDH.
 *
 * @retval true  Converter is enabled.
 * @retval false Converter is disabled.
 */
__STATIC_INLINE bool nrf_power_dcdcen_vddh_get(void);

/**
 * @brief Function for getting the main supply status.
 *
 * @return The current main supply status.
 */
__STATIC_INLINE nrf_power_mainregstatus_t nrf_power_mainregstatus_get(void);
#endif // NRF_POWER_HAS_VDDH

#if NRF_POWER_HAS_USBREG
/**
 * @brief Function for getting the whole USBREGSTATUS register.
 *
 * @return The USBREGSTATUS register value.
 *         Use @ref nrf_power_usbregstatus_mask_t values for bit masking.
 *
 * @sa nrf_power_usbregstatus_vbusdet_get
 * @sa nrf_power_usbregstatus_outrdy_get
 */
__STATIC_INLINE uint32_t nrf_power_usbregstatus_get(void);

/**
 * @brief Function for getting the VBUS input detection status.
 *
 * USBDETECTED and USBREMOVED events are derived from this information
 *
 * @retval false VBUS voltage below valid threshold.
 * @retval true  VBUS voltage above valid threshold.
 *
 * @sa nrf_power_usbregstatus_get
 */
__STATIC_INLINE bool nrf_power_usbregstatus_vbusdet_get(void);

/**
 * @brief Function for getting the state of the elapsed time for the USB supply output settling.
 *
 * @retval false USBREG output settling time not elapsed.
 * @retval true  USBREG output settling time elapsed
 *               (same information as USBPWRRDY event).
 *
 * @sa nrf_power_usbregstatus_get
 */
__STATIC_INLINE bool nrf_power_usbregstatus_outrdy_get(void);
#endif // NRF_POWER_HAS_USBREG

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE void nrf_power_task_trigger(nrf_power_task_t task)
{
    *((volatile uint32_t *)((uint8_t *)NRF_POWER + (uint32_t)task)) = 0x1UL;
}

__STATIC_INLINE uint32_t nrf_power_task_address_get(nrf_power_task_t task)
{
    return ((uint32_t)NRF_POWER + (uint32_t)task);
}

__STATIC_INLINE void nrf_power_event_clear(nrf_power_event_t event)
{
    *((volatile uint32_t *)((uint8_t *)NRF_POWER + (uint32_t)event)) = 0x0UL;
#if __CORTEX_M == 0x04
    volatile uint32_t dummy = *((volatile uint32_t *)((uint8_t *)NRF_POWER + (uint32_t)event));
    (void)dummy;
#endif
}

__STATIC_INLINE bool nrf_power_event_check(nrf_power_event_t event)
{
    return (bool)*(volatile uint32_t *)((uint8_t *)NRF_POWER + (uint32_t)event);
}

__STATIC_INLINE bool nrf_power_event_get_and_clear(nrf_power_event_t event)
{
    bool ret = nrf_power_event_check(event);
    if (ret)
    {
        nrf_power_event_clear(event);
    }
    return ret;
}

__STATIC_INLINE uint32_t nrf_power_event_address_get(nrf_power_event_t event)
{
    return ((uint32_t)NRF_POWER + (uint32_t)event);
}

__STATIC_INLINE void nrf_power_int_enable(uint32_t int_mask)
{
    NRF_POWER->INTENSET = int_mask;
}

__STATIC_INLINE bool nrf_power_int_enable_check(uint32_t int_mask)
{
    return (bool)(NRF_POWER->INTENSET & int_mask);
}

__STATIC_INLINE uint32_t nrf_power_int_enable_get(void)
{
    return NRF_POWER->INTENSET;
}

__STATIC_INLINE void nrf_power_int_disable(uint32_t int_mask)
{
    NRF_POWER->INTENCLR = int_mask;
}

#if defined(DPPI_PRESENT)
__STATIC_INLINE void nrf_power_subscribe_set(nrf_power_task_t task,
                                             uint8_t          channel)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_POWER + (uint32_t) task + 0x80uL)) =
            ((uint32_t)channel | POWER_SUBSCRIBE_CONSTLAT_EN_Msk);
}

__STATIC_INLINE void nrf_power_subscribe_clear(nrf_power_task_t task)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_POWER + (uint32_t) task + 0x80uL)) = 0;
}

__STATIC_INLINE void nrf_power_publish_set(nrf_power_event_t event,
                                           uint8_t           channel)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_POWER + (uint32_t) event + 0x80uL)) =
            ((uint32_t)channel | POWER_PUBLISH_SLEEPENTER_EN_Msk);
}

__STATIC_INLINE void nrf_power_publish_clear(nrf_power_event_t event)
{
    *((volatile uint32_t *) ((uint8_t *) NRF_POWER + (uint32_t) event + 0x80uL)) = 0;
}
#endif // defined(DPPI_PRESENT)

__STATIC_INLINE uint32_t nrf_power_resetreas_get(void)
{
    return NRF_POWER->RESETREAS;
}

__STATIC_INLINE void nrf_power_resetreas_clear(uint32_t mask)
{
    NRF_POWER->RESETREAS = mask;
}

#if defined(POWER_POWERSTATUS_LTEMODEM_Msk)
__STATIC_INLINE bool nrf_power_powerstatus_get(void)
{
    return (NRF_POWER->POWERSTATUS & POWER_POWERSTATUS_LTEMODEM_Msk) ==
            (POWER_POWERSTATUS_LTEMODEM_ON << POWER_POWERSTATUS_LTEMODEM_Pos);
}
#endif // (POWER_POWERSTATUS_LTEMODEM_Msk)

#if defined(POWER_RAMSTATUS_RAMBLOCK0_Msk)
__STATIC_INLINE uint32_t nrf_power_ramstatus_get(void)
{
    return NRF_POWER->RAMSTATUS;
}
#endif // defined(POWER_RAMSTATUS_RAMBLOCK0_Msk)

#if defined(POWER_SYSTEMOFF_SYSTEMOFF_Enter)
__STATIC_INLINE void nrf_power_system_off(void)
{
    NRF_POWER->SYSTEMOFF = POWER_SYSTEMOFF_SYSTEMOFF_Enter;
    __DSB();

    /* Solution for simulated System OFF in debug mode */
    while (true)
    {
        __WFE();
    }
}
#endif // defined(POWER_SYSTEMOFF_SYSTEMOFF_Enter)

#if NRF_POWER_HAS_POFCON
__STATIC_INLINE void nrf_power_pofcon_set(bool enabled, nrf_power_pof_thr_t thr)
{
    NRFX_ASSERT(thr == (thr & (POWER_POFCON_THRESHOLD_Msk >> POWER_POFCON_THRESHOLD_Pos)));
#if NRF_POWER_HAS_VDDH
    uint32_t pofcon = NRF_POWER->POFCON;
    pofcon &= ~(POWER_POFCON_THRESHOLD_Msk | POWER_POFCON_POF_Msk);
    pofcon |=
#else // NRF_POWER_HAS_VDDH
    NRF_POWER->POFCON =
#endif
        (((uint32_t)thr) << POWER_POFCON_THRESHOLD_Pos) |
        (enabled ?
        (POWER_POFCON_POF_Enabled << POWER_POFCON_POF_Pos)
        :
        (POWER_POFCON_POF_Disabled << POWER_POFCON_POF_Pos));
#if NRF_POWER_HAS_VDDH
    NRF_POWER->POFCON = pofcon;
#endif
}

__STATIC_INLINE nrf_power_pof_thr_t nrf_power_pofcon_get(bool * p_enabled)
{
    uint32_t pofcon = NRF_POWER->POFCON;
    if (NULL != p_enabled)
    {
        (*p_enabled) = ((pofcon & POWER_POFCON_POF_Msk) >> POWER_POFCON_POF_Pos)
            == POWER_POFCON_POF_Enabled;
    }
    return (nrf_power_pof_thr_t)((pofcon & POWER_POFCON_THRESHOLD_Msk) >>
        POWER_POFCON_THRESHOLD_Pos);
}
#endif // NRF_POWER_HAS_POFCON

#if NRF_POWER_HAS_VDDH
__STATIC_INLINE void nrf_power_pofcon_vddh_set(nrf_power_pof_thrvddh_t thr)
{
    NRFX_ASSERT(thr == (thr & (POWER_POFCON_THRESHOLDVDDH_Msk >> POWER_POFCON_THRESHOLDVDDH_Pos)));
    uint32_t pofcon = NRF_POWER->POFCON;
    pofcon &= ~POWER_POFCON_THRESHOLDVDDH_Msk;
    pofcon |= (((uint32_t)thr) << POWER_POFCON_THRESHOLDVDDH_Pos);
    NRF_POWER->POFCON = pofcon;
}

__STATIC_INLINE nrf_power_pof_thrvddh_t nrf_power_pofcon_vddh_get(void)
{
    return (nrf_power_pof_thrvddh_t)((NRF_POWER->POFCON &
        POWER_POFCON_THRESHOLDVDDH_Msk) >> POWER_POFCON_THRESHOLDVDDH_Pos);
}
#endif // NRF_POWER_HAS_VDDH

__STATIC_INLINE void nrf_power_gpregret_set(uint8_t val)
{
    volatile uint32_t * p_gpregret;
    if (sizeof(NRF_POWER->GPREGRET) > sizeof(uint32_t))
    {
        p_gpregret = &((volatile uint32_t *)NRF_POWER->GPREGRET)[0];
    }
    else
    {
        p_gpregret = &((volatile uint32_t *)&NRF_POWER->GPREGRET)[0];
    }
    *p_gpregret = val;
}

__STATIC_INLINE uint8_t nrf_power_gpregret_get(void)
{
    volatile uint32_t * p_gpregret;
    if (sizeof(NRF_POWER->GPREGRET) > sizeof(uint32_t))
    {
        p_gpregret = &((volatile uint32_t *)NRF_POWER->GPREGRET)[0];
    }
    else
    {
        p_gpregret = &((volatile uint32_t *)&NRF_POWER->GPREGRET)[0];
    }
    return *p_gpregret;
}

__STATIC_INLINE void nrf_power_gpregret_ext_set(uint8_t reg_num, uint8_t val)
{
#ifdef NRF91_SERIES
    NRF_POWER->GPREGRET[reg_num] = val;
#else
    NRFX_ASSERT(reg_num < 1);
    NRF_POWER->GPREGRET = val;
#endif
}

__STATIC_INLINE uint8_t nrf_power_gpregret_ext_get(uint8_t reg_num)
{
#ifdef NRF91_SERIES
    return NRF_POWER->GPREGRET[reg_num];
#else
    NRFX_ASSERT(reg_num < 1);
    return NRF_POWER->GPREGRET;
#endif
}

#if defined(POWER_GPREGRET2_GPREGRET_Msk)
__STATIC_INLINE void nrf_power_gpregret2_set(uint8_t val)
{
    NRF_POWER->GPREGRET2 = val;
}

__STATIC_INLINE uint8_t nrf_power_gpregret2_get(void)
{
    return NRF_POWER->GPREGRET2;
}
#endif

#if NRF_POWER_HAS_DCDCEN
__STATIC_INLINE void nrf_power_dcdcen_set(bool enable)
{
    NRF_POWER->DCDCEN = (enable ?
        POWER_DCDCEN_DCDCEN_Enabled : POWER_DCDCEN_DCDCEN_Disabled) <<
            POWER_DCDCEN_DCDCEN_Pos;
}

__STATIC_INLINE bool nrf_power_dcdcen_get(void)
{
    return (NRF_POWER->DCDCEN & POWER_DCDCEN_DCDCEN_Msk)
            ==
           (POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos);
}
#endif // NRF_POWER_HAS_DCDCEN

#if defined(POWER_RAM_POWER_S0POWER_Msk)
__STATIC_INLINE void nrf_power_rampower_mask_on(uint8_t block, uint32_t section_mask)
{
    NRFX_ASSERT(block < NRFX_ARRAY_SIZE(NRF_POWER->RAM));
    NRF_POWER->RAM[block].POWERSET = section_mask;
}

__STATIC_INLINE void nrf_power_rampower_mask_off(uint8_t block, uint32_t section_mask)
{
    NRFX_ASSERT(block < NRFX_ARRAY_SIZE(NRF_POWER->RAM));
    NRF_POWER->RAM[block].POWERCLR = section_mask;
}

__STATIC_INLINE uint32_t nrf_power_rampower_mask_get(uint8_t block)
{
    NRFX_ASSERT(block < NRFX_ARRAY_SIZE(NRF_POWER->RAM));
    return NRF_POWER->RAM[block].POWER;
}
#endif /* defined(POWER_RAM_POWER_S0POWER_Msk) */

#if NRF_POWER_HAS_VDDH
__STATIC_INLINE void nrf_power_dcdcen_vddh_set(bool enable)
{
    NRF_POWER->DCDCEN0 = (enable ?
        POWER_DCDCEN0_DCDCEN_Enabled : POWER_DCDCEN0_DCDCEN_Disabled) <<
            POWER_DCDCEN0_DCDCEN_Pos;
}

__STATIC_INLINE bool nrf_power_dcdcen_vddh_get(void)
{
    return (NRF_POWER->DCDCEN0 & POWER_DCDCEN0_DCDCEN_Msk)
            ==
           (POWER_DCDCEN0_DCDCEN_Enabled << POWER_DCDCEN0_DCDCEN_Pos);
}

__STATIC_INLINE nrf_power_mainregstatus_t nrf_power_mainregstatus_get(void)
{
    return (nrf_power_mainregstatus_t)(((NRF_POWER->MAINREGSTATUS) &
        POWER_MAINREGSTATUS_MAINREGSTATUS_Msk) >>
        POWER_MAINREGSTATUS_MAINREGSTATUS_Pos);
}
#endif // NRF_POWER_HAS_VDDH

#if NRF_POWER_HAS_USBREG
__STATIC_INLINE uint32_t nrf_power_usbregstatus_get(void)
{
    return NRF_POWER->USBREGSTATUS;
}

__STATIC_INLINE bool nrf_power_usbregstatus_vbusdet_get(void)
{
    return (nrf_power_usbregstatus_get() &
        NRF_POWER_USBREGSTATUS_VBUSDETECT_MASK) != 0;
}

__STATIC_INLINE bool nrf_power_usbregstatus_outrdy_get(void)
{
    return (nrf_power_usbregstatus_get() &
        NRF_POWER_USBREGSTATUS_OUTPUTRDY_MASK) != 0;
}
#endif // NRF_POWER_HAS_USBREG

#endif // SUPPRESS_INLINE_IMPLEMENTATION

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRF_POWER_H__
