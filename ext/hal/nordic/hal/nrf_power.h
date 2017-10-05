/* Copyright (c) 2010-2017 Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice, this
 *      list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *
 *   3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *      contributors may be used to endorse or promote products derived from
 *      this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef NRF_POWER_H__
#define NRF_POWER_H__

/**
 * @ingroup nrf_power
 * @defgroup nrf_power_hal POWER HAL
 * @{
 *
 * Hardware access layer for (POWER) peripheral.
 */
#include "nrf.h"
#include "nrf_assert.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name The implemented functionality
 * @{
 *
 * Macros that defines functionality that is implemented into POWER peripheral.
 */
#if defined(POWER_INTENSET_SLEEPENTER_Msk) || defined(__SDK_DOXYGEN__)
/**
 * @brief The fact that sleep events are present
 *
 * In some MCUs there is possibility to process sleep entering and exiting
 * events.
 */
#define NRF_POWER_HAS_SLEEPEVT 1
#else
#define NRF_POWER_HAS_SLEEPEVT 0
#endif

#if defined(POWER_RAM_POWER_S0POWER_Msk) || defined(__SDK_DOXYGEN__)
/**
 * @brief The fact that RAMPOWER registers are present
 *
 * After nRF51, new way to manage RAM power was implemented.
 * Special registers, one for every RAM block that makes it possible to
 * power ON or OFF RAM segments and turn ON and OFF RAM retention in system OFF
 * state.
 */
#define NRF_POWER_HAS_RAMPOWER_REGS 1
#else
#define NRF_POWER_HAS_RAMPOWER_REGS 0
#endif

#if defined(POWER_POFCON_THRESHOLDVDDH_Msk) || defined(__SDK_DOXYGEN__)
/**
 * @brief Auxiliary definition to mark the fact that VDDH is present
 *
 * This definition can be used in a code to decide if the part with VDDH
 * related settings should be implemented.
 */
#define NRF_POWER_HAS_VDDH 1
#else
#define NRF_POWER_HAS_VDDH 0
#endif

#if defined(POWER_USBREGSTATUS_VBUSDETECT_Msk) || defined(__SDK_DOXYGEN__)
/**
 * @brief The fact that power module manages USB regulator
 *
 * In devices that have USB, power peripheral manages also connection
 * detection and USB power regulator, that converts 5&nbsp;V to 3.3&nbsp;V
 * used by USBD peripheral.
 */
#define NRF_POWER_HAS_USBREG 1
#else
#define NRF_POWER_HAS_USBREG 0
#endif
/** @} */

/* ------------------------------------------------------------------------------------------------
 *  Begin of automatically generated part
 * ------------------------------------------------------------------------------------------------
 */

/**
 * @brief POWER tasks
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_POWER_TASK_CONSTLAT  = offsetof(NRF_POWER_Type, TASKS_CONSTLAT), /**< Enable constant latency mode */
    NRF_POWER_TASK_LOWPWR    = offsetof(NRF_POWER_Type, TASKS_LOWPWR  ), /**< Enable low power mode (variable latency) */
}nrf_power_task_t; /*lint -restore */

/**
 * @brief POWER events
 */
typedef enum /*lint -save -e30 -esym(628,__INTADDR__) */
{
    NRF_POWER_EVENT_POFWARN      = offsetof(NRF_POWER_Type, EVENTS_POFWARN    ), /**< Power failure warning */
#if NRF_POWER_HAS_SLEEPEVT
    NRF_POWER_EVENT_SLEEPENTER   = offsetof(NRF_POWER_Type, EVENTS_SLEEPENTER ), /**< CPU entered WFI/WFE sleep */
    NRF_POWER_EVENT_SLEEPEXIT    = offsetof(NRF_POWER_Type, EVENTS_SLEEPEXIT  ), /**< CPU exited WFI/WFE sleep */
#endif
#if NRF_POWER_HAS_USBREG
    NRF_POWER_EVENT_USBDETECTED  = offsetof(NRF_POWER_Type, EVENTS_USBDETECTED), /**< Voltage supply detected on VBUS */
    NRF_POWER_EVENT_USBREMOVED   = offsetof(NRF_POWER_Type, EVENTS_USBREMOVED ), /**< Voltage supply removed from VBUS */
    NRF_POWER_EVENT_USBPWRRDY    = offsetof(NRF_POWER_Type, EVENTS_USBPWRRDY  ), /**< USB 3.3&nbsp;V supply ready */
#endif
}nrf_power_event_t; /*lint -restore */

/**
 * @brief POWER interrupts
 */
typedef enum
{
    NRF_POWER_INT_POFWARN_MASK     = POWER_INTENSET_POFWARN_Msk    , /**< Write '1' to Enable interrupt for POFWARN event */
#if NRF_POWER_HAS_SLEEPEVT
    NRF_POWER_INT_SLEEPENTER_MASK  = POWER_INTENSET_SLEEPENTER_Msk , /**< Write '1' to Enable interrupt for SLEEPENTER event */
    NRF_POWER_INT_SLEEPEXIT_MASK   = POWER_INTENSET_SLEEPEXIT_Msk  , /**< Write '1' to Enable interrupt for SLEEPEXIT event */
#endif
#if NRF_POWER_HAS_USBREG
    NRF_POWER_INT_USBDETECTED_MASK = POWER_INTENSET_USBDETECTED_Msk, /**< Write '1' to Enable interrupt for USBDETECTED event */
    NRF_POWER_INT_USBREMOVED_MASK  = POWER_INTENSET_USBREMOVED_Msk , /**< Write '1' to Enable interrupt for USBREMOVED event */
    NRF_POWER_INT_USBPWRRDY_MASK   = POWER_INTENSET_USBPWRRDY_Msk  , /**< Write '1' to Enable interrupt for USBPWRRDY event */
#endif
}nrf_power_int_mask_t;

/**
 * @brief Function for activating a specific POWER task.
 *
 * @param task Task.
 */
__STATIC_INLINE void nrf_power_task_trigger(nrf_power_task_t task);

/**
 * @brief Function for returning the address of a specific POWER task register.
 *
 * @param task Task.
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrf_power_task_address_get(nrf_power_task_t task);

/**
 * @brief Function for clearing a specific event.
 *
 * @param event Event.
 */
__STATIC_INLINE void nrf_power_event_clear(nrf_power_event_t event);

/**
 * @brief Function for returning the state of a specific event.
 *
 * @param event Event.
 *
 * @retval true If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_power_event_check(nrf_power_event_t event);

/**
 * @brief Function for getting and clearing the state of specific event
 *
 * This function checks the state of the event and clears it.
 *
 * @param event Event.
 *
 * @retval true If the event was set.
 * @retval false If the event was not set.
 */
__STATIC_INLINE bool nrf_power_event_get_and_clear(nrf_power_event_t event);

/**
 * @brief Function for returning the address of a specific POWER event register.
 *
 * @param     event  Event.
 *
 * @return Address.
 */
__STATIC_INLINE uint32_t nrf_power_event_address_get(nrf_power_event_t event);

/**
 * @brief Function for enabling selected interrupts.
 *
 * @param     int_mask Interrupts mask.
 */
__STATIC_INLINE void nrf_power_int_enable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of selected interrupts.
 *
 * @param     int_mask Interrupts mask.
 *
 * @retval true If any of selected interrupts is enabled.
 * @retval false If none of selected interrupts is enabled.
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
 * @param     int_mask Interrupts mask.
 */
__STATIC_INLINE void nrf_power_int_disable(uint32_t int_mask);


/** @} */ /*  End of nrf_power_hal */


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

/* ------------------------------------------------------------------------------------------------
 *  Internal functions
 */

/**
 * @internal
 * @brief Internal function for getting task/event register address
 *
 * @oaram offset Offset of the register from the instance beginning
 *
 * @attention offset has to be modulo 4 value. In other case we can get hardware fault.
 * @return Pointer to the register
 */
__STATIC_INLINE volatile uint32_t * nrf_power_regptr_get(uint32_t offset)
{
    return (volatile uint32_t *)(((uint8_t *)NRF_POWER) + (uint32_t)offset);
}

/**
 * @internal
 * @brief Internal function for getting task/event register address - constant version
 *
 * @oaram offset Offset of the register from the instance beginning
 *
 * @attention offset has to be modulo 4 value. In other case we can get hardware fault.
 * @return Pointer to the register
 */
__STATIC_INLINE volatile const uint32_t * nrf_power_regptr_get_c(
    uint32_t offset)
{
    return (volatile const uint32_t *)(((uint8_t *)NRF_POWER) +
        (uint32_t)offset);
}

/* ------------------------------------------------------------------------------------------------
 *  Interface functions definitions
 */

void nrf_power_task_trigger(nrf_power_task_t task)
{
    *(nrf_power_regptr_get((uint32_t)task)) = 1UL;
}

uint32_t nrf_power_task_address_get(nrf_power_task_t task)
{
    return (uint32_t)nrf_power_regptr_get_c((uint32_t)task);
}

void nrf_power_event_clear(nrf_power_event_t event)
{
    *(nrf_power_regptr_get((uint32_t)event)) = 0UL;
}

bool nrf_power_event_check(nrf_power_event_t event)
{
    return (bool)*nrf_power_regptr_get_c((uint32_t)event);
}

bool nrf_power_event_get_and_clear(nrf_power_event_t event)
{
    bool ret = nrf_power_event_check(event);
    if(ret)
    {
        nrf_power_event_clear(event);
    }
    return ret;
}

uint32_t nrf_power_event_address_get(nrf_power_event_t event)
{
    return (uint32_t)nrf_power_regptr_get_c((uint32_t)event);
}

void nrf_power_int_enable(uint32_t int_mask)
{
    NRF_POWER->INTENSET = int_mask;
}

bool nrf_power_int_enable_check(uint32_t int_mask)
{
    return !!(NRF_POWER->INTENSET & int_mask);
}

uint32_t nrf_power_int_enable_get(void)
{
    return NRF_POWER->INTENSET;
}

void nrf_power_int_disable(uint32_t int_mask)
{
    NRF_POWER->INTENCLR = int_mask;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

/* ------------------------------------------------------------------------------------------------
 *  End of automatically generated part
 * ------------------------------------------------------------------------------------------------
 */
/**
 * @ingroup nrf_power_hal
 * @{
 */

/**
 * @brief Reset reason
 */
typedef enum
{
    NRF_POWER_RESETREAS_RESETPIN_MASK = POWER_RESETREAS_RESETPIN_Msk, /*!< Bit mask of RESETPIN field. *///!< NRF_POWER_RESETREAS_RESETPIN_MASK
    NRF_POWER_RESETREAS_DOG_MASK      = POWER_RESETREAS_DOG_Msk     , /*!< Bit mask of DOG field. */     //!< NRF_POWER_RESETREAS_DOG_MASK
    NRF_POWER_RESETREAS_SREQ_MASK     = POWER_RESETREAS_SREQ_Msk    , /*!< Bit mask of SREQ field. */    //!< NRF_POWER_RESETREAS_SREQ_MASK
    NRF_POWER_RESETREAS_LOCKUP_MASK   = POWER_RESETREAS_LOCKUP_Msk  , /*!< Bit mask of LOCKUP field. */  //!< NRF_POWER_RESETREAS_LOCKUP_MASK
    NRF_POWER_RESETREAS_OFF_MASK      = POWER_RESETREAS_OFF_Msk     , /*!< Bit mask of OFF field. */     //!< NRF_POWER_RESETREAS_OFF_MASK
    NRF_POWER_RESETREAS_LPCOMP_MASK   = POWER_RESETREAS_LPCOMP_Msk  , /*!< Bit mask of LPCOMP field. */  //!< NRF_POWER_RESETREAS_LPCOMP_MASK
    NRF_POWER_RESETREAS_DIF_MASK      = POWER_RESETREAS_DIF_Msk     , /*!< Bit mask of DIF field. */     //!< NRF_POWER_RESETREAS_DIF_MASK
#if defined(POWER_RESETREAS_NFC_Msk) || defined(__SDK_DOXYGEN__)
    NRF_POWER_RESETREAS_NFC_MASK      = POWER_RESETREAS_NFC_Msk     , /*!< Bit mask of NFC field. */
#endif
#if defined(POWER_RESETREAS_VBUS_Msk) || defined(__SDK_DOXYGEN__)
    NRF_POWER_RESETREAS_VBUS_MASK     = POWER_RESETREAS_VBUS_Msk    , /*!< Bit mask of VBUS field. */
#endif
}nrf_power_resetreas_mask_t;

#if NRF_POWER_HAS_USBREG
/**
 * @brief USBREGSTATUS register bit masks
 *
 * @sa nrf_power_usbregstatus_get
 */
typedef enum
{
    NRF_POWER_USBREGSTATUS_VBUSDETECT_MASK = POWER_USBREGSTATUS_VBUSDETECT_Msk, /**< USB detected or removed     */
    NRF_POWER_USBREGSTATUS_OUTPUTRDY_MASK  = POWER_USBREGSTATUS_OUTPUTRDY_Msk   /**< USB 3.3&nbsp;V supply ready */
}nrf_power_usbregstatus_mask_t;
#endif

/**
 * @brief RAM blocks numbers
 *
 * @sa nrf_power_ramblock_mask_t
 * @note
 * Ram blocks has to been used in nrf51.
 * In new CPU ram is divided into segments and this functionality is depreciated.
 * For the newer MCU see the PS for mapping between internal RAM and RAM blocks,
 * because this mapping is not 1:1, and functions related to old style blocks
 * should not be used.
 */
typedef enum
{
    NRF_POWER_RAMBLOCK0 = POWER_RAMSTATUS_RAMBLOCK0_Pos,
    NRF_POWER_RAMBLOCK1 = POWER_RAMSTATUS_RAMBLOCK1_Pos,
    NRF_POWER_RAMBLOCK2 = POWER_RAMSTATUS_RAMBLOCK2_Pos,
    NRF_POWER_RAMBLOCK3 = POWER_RAMSTATUS_RAMBLOCK3_Pos
}nrf_power_ramblock_t;

/**
 * @brief RAM blocks masks
 *
 * @sa nrf_power_ramblock_t
 */
typedef enum
{
    NRF_POWER_RAMBLOCK0_MASK = POWER_RAMSTATUS_RAMBLOCK0_Msk,
    NRF_POWER_RAMBLOCK1_MASK = POWER_RAMSTATUS_RAMBLOCK1_Msk,
    NRF_POWER_RAMBLOCK2_MASK = POWER_RAMSTATUS_RAMBLOCK2_Msk,
    NRF_POWER_RAMBLOCK3_MASK = POWER_RAMSTATUS_RAMBLOCK3_Msk
}nrf_power_ramblock_mask_t;

/**
 * @brief RAM power state position of the bits
 *
 * @sa nrf_power_onoffram_mask_t
 */
typedef enum
{
    NRF_POWER_ONRAM0,  /**< Keep RAM block 0 on or off in system ON Mode                 */
    NRF_POWER_OFFRAM0, /**< Keep retention on RAM block 0 when RAM block is switched off */
    NRF_POWER_ONRAM1,  /**< Keep RAM block 1 on or off in system ON Mode                 */
    NRF_POWER_OFFRAM1, /**< Keep retention on RAM block 1 when RAM block is switched off */
    NRF_POWER_ONRAM2,  /**< Keep RAM block 2 on or off in system ON Mode                 */
    NRF_POWER_OFFRAM2, /**< Keep retention on RAM block 2 when RAM block is switched off */
    NRF_POWER_ONRAM3,  /**< Keep RAM block 3 on or off in system ON Mode                 */
    NRF_POWER_OFFRAM3, /**< Keep retention on RAM block 3 when RAM block is switched off */
}nrf_power_onoffram_t;

/**
 * @brief RAM power state bit masks
 *
 * @sa nrf_power_onoffram_t
 */
typedef enum
{
    NRF_POWER_ONRAM0_MASK  = 1U << NRF_POWER_ONRAM0,  /**< Keep RAM block 0 on or off in system ON Mode                 */
    NRF_POWER_OFFRAM0_MASK = 1U << NRF_POWER_OFFRAM0, /**< Keep retention on RAM block 0 when RAM block is switched off */
    NRF_POWER_ONRAM1_MASK  = 1U << NRF_POWER_ONRAM1,  /**< Keep RAM block 1 on or off in system ON Mode                 */
    NRF_POWER_OFFRAM1_MASK = 1U << NRF_POWER_OFFRAM1, /**< Keep retention on RAM block 1 when RAM block is switched off */
    NRF_POWER_ONRAM2_MASK  = 1U << NRF_POWER_ONRAM2,  /**< Keep RAM block 2 on or off in system ON Mode                 */
    NRF_POWER_OFFRAM2_MASK = 1U << NRF_POWER_OFFRAM2, /**< Keep retention on RAM block 2 when RAM block is switched off */
    NRF_POWER_ONRAM3_MASK  = 1U << NRF_POWER_ONRAM3,  /**< Keep RAM block 3 on or off in system ON Mode                 */
    NRF_POWER_OFFRAM3_MASK = 1U << NRF_POWER_OFFRAM3, /**< Keep retention on RAM block 3 when RAM block is switched off */
}nrf_power_onoffram_mask_t;

/**
 * @brief Power failure comparator thresholds
 */
typedef enum
{
    NRF_POWER_POFTHR_V21 = POWER_POFCON_THRESHOLD_V21, /**< Set threshold to 2.1&nbsp;V */
    NRF_POWER_POFTHR_V23 = POWER_POFCON_THRESHOLD_V23, /**< Set threshold to 2.3&nbsp;V */
    NRF_POWER_POFTHR_V25 = POWER_POFCON_THRESHOLD_V25, /**< Set threshold to 2.5&nbsp;V */
    NRF_POWER_POFTHR_V27 = POWER_POFCON_THRESHOLD_V27, /**< Set threshold to 2.7&nbsp;V */
#if defined(POWER_POFCON_THRESHOLD_V17) || defined(__SDK_DOXYGEN__)
    NRF_POWER_POFTHR_V17 = POWER_POFCON_THRESHOLD_V17, /**< Set threshold to 1.7&nbsp;V */
    NRF_POWER_POFTHR_V18 = POWER_POFCON_THRESHOLD_V18, /**< Set threshold to 1.8&nbsp;V */
    NRF_POWER_POFTHR_V19 = POWER_POFCON_THRESHOLD_V19, /**< Set threshold to 1.9&nbsp;V */
    NRF_POWER_POFTHR_V20 = POWER_POFCON_THRESHOLD_V20, /**< Set threshold to 2.0&nbsp;V */
    NRF_POWER_POFTHR_V22 = POWER_POFCON_THRESHOLD_V22, /**< Set threshold to 2.2&nbsp;V */
    NRF_POWER_POFTHR_V24 = POWER_POFCON_THRESHOLD_V24, /**< Set threshold to 2.4&nbsp;V */
    NRF_POWER_POFTHR_V26 = POWER_POFCON_THRESHOLD_V26, /**< Set threshold to 2.6&nbsp;V */
    NRF_POWER_POFTHR_V28 = POWER_POFCON_THRESHOLD_V28, /**< Set threshold to 2.8&nbsp;V */
#endif
}nrf_power_pof_thr_t;

#if NRF_POWER_HAS_VDDH
/**
 * @brief Power failure comparator thresholds for VDDH
 */
typedef enum
{
    NRF_POWER_POFTHRVDDH_V27 = POWER_POFCON_THRESHOLDVDDH_V27, /**< Set threshold to 2.7&nbsp;V */
    NRF_POWER_POFTHRVDDH_V28 = POWER_POFCON_THRESHOLDVDDH_V28, /**< Set threshold to 2.8&nbsp;V */
    NRF_POWER_POFTHRVDDH_V29 = POWER_POFCON_THRESHOLDVDDH_V29, /**< Set threshold to 2.9&nbsp;V */
    NRF_POWER_POFTHRVDDH_V30 = POWER_POFCON_THRESHOLDVDDH_V30, /**< Set threshold to 3.0&nbsp;V */
    NRF_POWER_POFTHRVDDH_V31 = POWER_POFCON_THRESHOLDVDDH_V31, /**< Set threshold to 3.1&nbsp;V */
    NRF_POWER_POFTHRVDDH_V32 = POWER_POFCON_THRESHOLDVDDH_V32, /**< Set threshold to 3.2&nbsp;V */
    NRF_POWER_POFTHRVDDH_V33 = POWER_POFCON_THRESHOLDVDDH_V33, /**< Set threshold to 3.3&nbsp;V */
    NRF_POWER_POFTHRVDDH_V34 = POWER_POFCON_THRESHOLDVDDH_V34, /**< Set threshold to 3.4&nbsp;V */
    NRF_POWER_POFTHRVDDH_V35 = POWER_POFCON_THRESHOLDVDDH_V35, /**< Set threshold to 3.5&nbsp;V */
    NRF_POWER_POFTHRVDDH_V36 = POWER_POFCON_THRESHOLDVDDH_V36, /**< Set threshold to 3.6&nbsp;V */
    NRF_POWER_POFTHRVDDH_V37 = POWER_POFCON_THRESHOLDVDDH_V37, /**< Set threshold to 3.7&nbsp;V */
    NRF_POWER_POFTHRVDDH_V38 = POWER_POFCON_THRESHOLDVDDH_V38, /**< Set threshold to 3.8&nbsp;V */
    NRF_POWER_POFTHRVDDH_V39 = POWER_POFCON_THRESHOLDVDDH_V39, /**< Set threshold to 3.9&nbsp;V */
    NRF_POWER_POFTHRVDDH_V40 = POWER_POFCON_THRESHOLDVDDH_V40, /**< Set threshold to 4.0&nbsp;V */
    NRF_POWER_POFTHRVDDH_V41 = POWER_POFCON_THRESHOLDVDDH_V41, /**< Set threshold to 4.1&nbsp;V */
    NRF_POWER_POFTHRVDDH_V42 = POWER_POFCON_THRESHOLDVDDH_V42, /**< Set threshold to 4.2&nbsp;V */
}nrf_power_pof_thrvddh_t;

/**
 * @brief Main regulator status
 */
typedef enum
{
    NRF_POWER_MAINREGSTATUS_NORMAL = POWER_MAINREGSTATUS_MAINREGSTATUS_Normal, /**< Normal voltage mode. Voltage supplied on VDD. */
    NRF_POWER_MAINREGSTATUS_HIGH   = POWER_MAINREGSTATUS_MAINREGSTATUS_High    /**< High voltage mode. Voltage supplied on VDDH.  */
}nrf_power_mainregstatus_t;

#endif /* NRF_POWER_HAS_VDDH */

#if NRF_POWER_HAS_RAMPOWER_REGS
/**
 * @brief Bit positions for RAMPOWER register
 *
 * All possible bits described, even if they are not used in selected MCU.
 */
typedef enum
{
    /** Keep RAM section S0 ON in System ON mode */
    NRF_POWER_RAMPOWER_S0POWER = POWER_RAM_POWER_S0POWER_Pos,
    NRF_POWER_RAMPOWER_S1POWER,  /**< Keep RAM section S1 ON in System ON mode */
    NRF_POWER_RAMPOWER_S2POWER,  /**< Keep RAM section S2 ON in System ON mode */
    NRF_POWER_RAMPOWER_S3POWER,  /**< Keep RAM section S3 ON in System ON mode */
    NRF_POWER_RAMPOWER_S4POWER,  /**< Keep RAM section S4 ON in System ON mode */
    NRF_POWER_RAMPOWER_S5POWER,  /**< Keep RAM section S5 ON in System ON mode */
    NRF_POWER_RAMPOWER_S6POWER,  /**< Keep RAM section S6 ON in System ON mode */
    NRF_POWER_RAMPOWER_S7POWER,  /**< Keep RAM section S7 ON in System ON mode */
    NRF_POWER_RAMPOWER_S8POWER,  /**< Keep RAM section S8 ON in System ON mode */
    NRF_POWER_RAMPOWER_S9POWER,  /**< Keep RAM section S9 ON in System ON mode */
    NRF_POWER_RAMPOWER_S10POWER, /**< Keep RAM section S10 ON in System ON mode */
    NRF_POWER_RAMPOWER_S11POWER, /**< Keep RAM section S11 ON in System ON mode */
    NRF_POWER_RAMPOWER_S12POWER, /**< Keep RAM section S12 ON in System ON mode */
    NRF_POWER_RAMPOWER_S13POWER, /**< Keep RAM section S13 ON in System ON mode */
    NRF_POWER_RAMPOWER_S14POWER, /**< Keep RAM section S14 ON in System ON mode */
    NRF_POWER_RAMPOWER_S15POWER, /**< Keep RAM section S15 ON in System ON mode */

    /** Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S0RETENTION = POWER_RAM_POWER_S0RETENTION_Pos,
    NRF_POWER_RAMPOWER_S1RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S2RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S3RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S4RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S5RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S6RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S7RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S8RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S9RETENTION,  /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S10RETENTION, /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S11RETENTION, /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S12RETENTION, /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S13RETENTION, /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S14RETENTION, /**< Keep section retention in OFF mode when section is OFF */
    NRF_POWER_RAMPOWER_S15RETENTION, /**< Keep section retention in OFF mode when section is OFF */
}nrf_power_rampower_t;

#if defined ( __CC_ARM )
#pragma push
#pragma diag_suppress 66
#endif
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
    NRF_POWER_RAMPOWER_S15RETENTION_MASK = 1UL << NRF_POWER_RAMPOWER_S15RETENTION,
}nrf_power_rampower_mask_t;
#if defined ( __CC_ARM )
#pragma pop
#endif
#endif /* NRF_POWER_HAS_RAMPOWER_REGS */


/**
 * @brief Get reset reason mask
 *
 * Function returns the reset reason.
 * Unless cleared, the RESETREAS register is cumulative.
 * A field is cleared by writing '1' to it (see @ref nrf_power_resetreas_clear).
 * If none of the reset sources are flagged,
 * this indicates that the chip was reset from the on-chip reset generator,
 * which indicates a power-on-reset or a brown out reset.
 *
 * @return The mask of reset reasons constructed with @ref nrf_power_resetreas_mask_t.
 */
__STATIC_INLINE uint32_t nrf_power_resetreas_get(void);

/**
 * @brief Clear selected reset reason field
 *
 * Function clears selected reset reason fields.
 *
 * @param[in] mask The mask constructed from @ref nrf_power_resetreas_mask_t enumerator values.
 * @sa nrf_power_resetreas_get
 */
__STATIC_INLINE void nrf_power_resetreas_clear(uint32_t mask);

/**
 * @brief Get RAMSTATUS register
 *
 * Returns the masks of RAM blocks that are powered ON.
 *
 * @return Value with bits sets according to masks in @ref nrf_power_ramblock_mask_t.
 */
__STATIC_INLINE uint32_t nrf_power_ramstatus_get(void);

/**
 * @brief Go to system OFF
 *
 * This function puts the CPU into system off mode.
 * The only way to wake up the CPU is by reset.
 *
 * @note This function never returns.
 */
__STATIC_INLINE void nrf_power_system_off(void);

/**
 * @brief Set power failure comparator configuration
 *
 * Sets power failure comparator threshold and enable/disable flag.
 *
 * @param enabled Set to true if power failure comparator should be enabled.
 * @param thr     Set the voltage threshold value.
 *
 * @note
 * If VDDH settings is present in the device, this function would
 * clear it settings (set to the lowest voltage).
 * Use @ref nrf_power_pofcon_vddh_set function to set new value.
 */
__STATIC_INLINE void nrf_power_pofcon_set(bool enabled, nrf_power_pof_thr_t thr);

/**
 * @brief Get power failure comparator configuration
 *
 * Get power failure comparator threshold and enable bit.
 *
 * @param[out] p_enabled Function would set this boolean variable to true
 *                       if power failure comparator is enabled.
 *                       The pointer can be NULL if we do not need this information.
 * @return Threshold setting for power failure comparator
 */
__STATIC_INLINE nrf_power_pof_thr_t nrf_power_pofcon_get(bool * p_enabled);

#if NRF_POWER_HAS_VDDH
/**
 * @brief Set VDDH power failure comparator threshold
 *
 * @param thr Threshold to be set
 */
__STATIC_INLINE void nrf_power_pofcon_vddh_set(nrf_power_pof_thrvddh_t thr);

/**
 * @brief Get VDDH power failure comparator threshold
 *
 * @return VDDH threshold currently configured
 */
__STATIC_INLINE nrf_power_pof_thrvddh_t nrf_power_pofcon_vddh_get(void);
#endif

/**
 * @brief Set general purpose retention register
 *
 * @param val Value to be set in the register
 */
__STATIC_INLINE void nrf_power_gpregret_set(uint8_t val);

/**
 * @brief Get general purpose retention register
 *
 * @return The value from the register
 */
__STATIC_INLINE uint8_t nrf_power_gpregret_get(void);

#if defined(POWER_GPREGRET2_GPREGRET_Msk) || defined(__SDK_DOXYGEN__)
/**
 * @brief Set general purpose retention register 2
 *
 * @param val Value to be set in the register
 * @note This register is not available in nrf51 MCU family
 */
__STATIC_INLINE void nrf_power_gpregret2_set(uint8_t val);

/**
 * @brief Get general purpose retention register 2
 *
 * @return The value from the register
 * @note This register is not available in all MCUs.
 */
__STATIC_INLINE uint8_t nrf_power_gpregret2_get(void);
#endif

/**
 * @brief Enable or disable DCDC converter
 *
 * @param enable Set true to enable or false to disable DCDC converter.
 *
 * @note
 * If the device consist of high voltage power input (VDDH) this setting
 * would relate to the converter on low voltage side (1.3&nbsp;V output).
 */
__STATIC_INLINE void nrf_power_dcdcen_set(bool enable);

/**
 * @brief Get the state of DCDC converter
 *
 * @retval true  Converter is enabled
 * @retval false Converter is disabled
 *
 * @note
 * If the device consist of high voltage power input (VDDH) this setting
 * would relate to the converter on low voltage side (1.3&nbsp;V output).
 */
__STATIC_INLINE bool nrf_power_dcdcen_get(void);

#if NRF_POWER_HAS_RAMPOWER_REGS
/**
 * @brief Turn ON sections in selected RAM block.
 *
 * This function turns ON sections in block and also block retention.
 *
 * @sa nrf_power_rampower_mask_t
 * @sa nrf_power_rampower_mask_off
 *
 * @param block        RAM block index.
 * @param section_mask Mask of the sections created by merging
 *                     @ref nrf_power_rampower_mask_t flags.
 */
__STATIC_INLINE void nrf_power_rampower_mask_on(uint8_t block, uint32_t section_mask);

/**
 * @brief Turn ON sections in selected RAM block.
 *
 * This function turns OFF sections in block and also block retention.
 *
 * @sa nrf_power_rampower_mask_t
 * @sa nrf_power_rampower_mask_off
 *
 * @param block        RAM block index.
 * @param section_mask Mask of the sections created by merging
 *                     @ref nrf_power_rampower_mask_t flags.
 */
__STATIC_INLINE void nrf_power_rampower_mask_off(uint8_t block, uint32_t section_mask);

/**
 * @brief Get the mask of ON and retention sections in selected RAM block.
 *
 * @param block RAM block index.
 * @return Mask of sections state composed from @ref nrf_power_rampower_mask_t flags.
 */
__STATIC_INLINE uint32_t nrf_power_rampower_mask_get(uint8_t block);
#endif /* NRF_POWER_HAS_RAMPOWER_REGS */

#if NRF_POWER_HAS_VDDH
/**
 * @brief Enable of disable DCDC converter on VDDH
 *
 * @param enable Set true to enable or false to disable DCDC converter.
 */
__STATIC_INLINE void nrf_power_dcdcen_vddh_set(bool enable);

/**
 * @brief Get the state of DCDC converter on VDDH
 *
 * @retval true  Converter is enabled
 * @retval false Converter is disabled
 */
__STATIC_INLINE bool nrf_power_dcdcen_vddh_get(void);

/**
 * @brief Get main supply status
 *
 * @return Current main supply status
 */
__STATIC_INLINE nrf_power_mainregstatus_t nrf_power_mainregstatus_get(void);
#endif /* NRF_POWER_HAS_VDDH */

#if NRF_POWER_HAS_USBREG
/**
 *
 * @return Get the whole USBREGSTATUS register
 *
 * @return The USBREGSTATUS register value.
 *         Use @ref nrf_power_usbregstatus_mask_t values for bit masking.
 *
 * @sa nrf_power_usbregstatus_vbusdet_get
 * @sa nrf_power_usbregstatus_outrdy_get
 */
__STATIC_INLINE uint32_t nrf_power_usbregstatus_get(void);

/**
 * @brief VBUS input detection status
 *
 * USBDETECTED and USBREMOVED events are derived from this information
 *
 * @retval false VBUS voltage below valid threshold
 * @retval true  VBUS voltage above valid threshold
 *
 * @sa nrf_power_usbregstatus_get
 */
__STATIC_INLINE bool nrf_power_usbregstatus_vbusdet_get(void);

/**
 * @brief USB supply output settling time elapsed
 *
 * @retval false USBREG output settling time not elapsed
 * @retval true  USBREG output settling time elapsed
 *               (same information as USBPWRRDY event)
 *
 * @sa nrf_power_usbregstatus_get
 */
__STATIC_INLINE bool nrf_power_usbregstatus_outrdy_get(void);
#endif /* NRF_POWER_HAS_USBREG */

/** @} */

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

__STATIC_INLINE uint32_t nrf_power_resetreas_get(void)
{
    return NRF_POWER->RESETREAS;
}

__STATIC_INLINE void nrf_power_resetreas_clear(uint32_t mask)
{
    NRF_POWER->RESETREAS = mask;
}

__STATIC_INLINE uint32_t nrf_power_ramstatus_get(void)
{
    return NRF_POWER->RAMSTATUS;
}

__STATIC_INLINE void nrf_power_system_off(void)
{
    NRF_POWER->SYSTEMOFF = POWER_SYSTEMOFF_SYSTEMOFF_Enter;
    /* Solution for simulated System OFF in debug mode.
     * Also, because dead loop is placed here, we do not need to implement
     * any barriers here. */
    while(true)
    {
        /* Intentionally empty - we would be here only in debug mode */
    }
}

__STATIC_INLINE void nrf_power_pofcon_set(bool enabled, nrf_power_pof_thr_t thr)
{
    ASSERT(thr == (thr & (POWER_POFCON_THRESHOLD_Msk >> POWER_POFCON_THRESHOLD_Pos)));
    NRF_POWER->POFCON = (((uint32_t)thr) << POWER_POFCON_THRESHOLD_Pos) |
        (enabled ?
        (POWER_POFCON_POF_Enabled << POWER_POFCON_POF_Pos)
        :
        (POWER_POFCON_POF_Disabled << POWER_POFCON_POF_Pos));
}

__STATIC_INLINE nrf_power_pof_thr_t nrf_power_pofcon_get(bool * p_enabled)
{
    uint32_t pofcon = NRF_POWER->POFCON;
    if(NULL != p_enabled)
    {
        (*p_enabled) = ((pofcon & POWER_POFCON_POF_Msk) >> POWER_POFCON_POF_Pos)
            == POWER_POFCON_POF_Enabled;
    }
    return (nrf_power_pof_thr_t)((pofcon & POWER_POFCON_THRESHOLD_Msk) >>
        POWER_POFCON_THRESHOLD_Pos);
}

#if NRF_POWER_HAS_VDDH
__STATIC_INLINE void nrf_power_pofcon_vddh_set(nrf_power_pof_thrvddh_t thr)
{
    ASSERT(thr == (thr & (POWER_POFCON_THRESHOLDVDDH_Msk >> POWER_POFCON_THRESHOLDVDDH_Pos)));
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
#endif /* NRF_POWER_HAS_VDDH */

__STATIC_INLINE void nrf_power_gpregret_set(uint8_t val)
{
    NRF_POWER->GPREGRET = val;
}

__STATIC_INLINE uint8_t nrf_power_gpregret_get(void)
{
    return NRF_POWER->GPREGRET;
}

#if defined(POWER_GPREGRET2_GPREGRET_Msk) || defined(__SDK_DOXYGEN__)
void nrf_power_gpregret2_set(uint8_t val)
{
    NRF_POWER->GPREGRET2 = val;
}

__STATIC_INLINE uint8_t nrf_power_gpregret2_get(void)
{
    return NRF_POWER->GPREGRET2;
}
#endif

__STATIC_INLINE void nrf_power_dcdcen_set(bool enable)
{
#if NRF_POWER_HAS_VDDH
    NRF_POWER->DCDCEN = (enable ?
        POWER_DCDCEN_DCDCEN_Enabled : POWER_DCDCEN_DCDCEN_Disabled) <<
            POWER_DCDCEN_DCDCEN_Pos;
#else
    NRF_POWER->DCDCEN = (enable ?
        POWER_DCDCEN_DCDCEN_Enabled : POWER_DCDCEN_DCDCEN_Disabled) <<
            POWER_DCDCEN_DCDCEN_Pos;
#endif
}

__STATIC_INLINE bool nrf_power_dcdcen_get(void)
{
#if NRF_POWER_HAS_VDDH
    return (NRF_POWER->DCDCEN & POWER_DCDCEN_DCDCEN_Msk)
            ==
           (POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos);
#else
    return (NRF_POWER->DCDCEN & POWER_DCDCEN_DCDCEN_Msk)
            ==
           (POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos);
#endif
}

#if NRF_POWER_HAS_RAMPOWER_REGS
__STATIC_INLINE void nrf_power_rampower_mask_on(uint8_t block, uint32_t section_mask)
{
    ASSERT(block < ARRAY_SIZE(NRF_POWER->RAM));
    NRF_POWER->RAM[block].POWERSET = section_mask;
}

__STATIC_INLINE void nrf_power_rampower_mask_off(uint8_t block, uint32_t section_mask)
{
    ASSERT(block < ARRAY_SIZE(NRF_POWER->RAM));
    NRF_POWER->RAM[block].POWERCLR = section_mask;
}

__STATIC_INLINE uint32_t nrf_power_rampower_mask_get(uint8_t block)
{
    ASSERT(block < ARRAY_SIZE(NRF_POWER->RAM));
    return NRF_POWER->RAM[block].POWER;
}
#endif /* NRF_POWER_HAS_RAMPOWER_REGS */

#if NRF_POWER_HAS_VDDH
__STATIC_INLINE void nrf_power_dcdcen_vddh_set(bool enable)
{
    NRF_POWER->DCDCEN0 = (enable ?
        POWER_DCDCEN0_DCDCEN_Enabled : POWER_DCDCEN0_DCDCEN_Disabled) <<
            POWER_DCDCEN0_DCDCEN_Pos;
}

bool nrf_power_dcdcen_vddh_get(void)
{
    return (NRF_POWER->DCDCEN0 & POWER_DCDCEN0_DCDCEN_Msk)
            ==
           (POWER_DCDCEN0_DCDCEN_Enabled << POWER_DCDCEN0_DCDCEN_Pos);
}

nrf_power_mainregstatus_t nrf_power_mainregstatus_get(void)
{
    return (nrf_power_mainregstatus_t)(((NRF_POWER->MAINREGSTATUS) &
        POWER_MAINREGSTATUS_MAINREGSTATUS_Msk) >>
        POWER_MAINREGSTATUS_MAINREGSTATUS_Pos);
}
#endif /* NRF_POWER_HAS_VDDH */

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
#endif /* NRF_POWER_HAS_USBREG */

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */


#ifdef __cplusplus
}
#endif

#endif /* NRF_POWER_H__ */
