/**
 * Copyright (c) 2017, Nordic Semiconductor ASA
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
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_POWER_H__
#define NRFX_POWER_H__

#include <nrfx.h>
#include <hal/nrf_power.h>
#include <nrfx_power_clock.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_power POWER driver
 * @{
 * @ingroup nrf_power
 * @brief   POWER peripheral driver.
 */

/**
 * @brief Power mode possible configurations
 */
typedef enum
{
    NRFX_POWER_MODE_CONSTLAT, /**< Constant latency mode */
    NRFX_POWER_MODE_LOWPWR    /**< Low power mode        */
}nrfx_power_mode_t;

#if NRF_POWER_HAS_SLEEPEVT || defined(__NRFX_DOXYGEN__)
/**
 * @brief Events from power system
 */
typedef enum
{
    NRFX_POWER_SLEEP_EVT_ENTER, /**< CPU entered WFI/WFE sleep
                                 *
                                 * Keep in mind that if this interrupt is enabled,
                                 * it means that CPU was waken up just after WFI by this interrupt.
                                 */
    NRFX_POWER_SLEEP_EVT_EXIT   /**< CPU exited WFI/WFE sleep */
}nrfx_power_sleep_evt_t;
#endif /* NRF_POWER_HAS_SLEEPEVT */

#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
/**
 * @brief Events from USB power system
 */
typedef enum
{
    NRFX_POWER_USB_EVT_DETECTED, /**< USB power detected on the connector (plugged in). */
    NRFX_POWER_USB_EVT_REMOVED,  /**< USB power removed from the connector. */
    NRFX_POWER_USB_EVT_READY     /**< USB power regulator ready. */
}nrfx_power_usb_evt_t;

/**
 * @brief USB power state
 *
 * The single enumerator that holds all data about current state of USB
 * related POWER.
 *
 * Organized this way that higher power state has higher numeric value
 */
typedef enum
{
    NRFX_POWER_USB_STATE_DISCONNECTED, /**< No power on USB lines detected */
    NRFX_POWER_USB_STATE_CONNECTED,    /**< The USB power is detected, but USB power regulator is not ready */
    NRFX_POWER_USB_STATE_READY         /**< From the power point of view USB is ready for working */
}nrfx_power_usb_state_t;
#endif /* NRF_POWER_HAS_USBREG */

/**
 * @name Callback types
 *
 * Defined types of callback functions
 * @{
 */
/**
 * @brief Event handler for power failure warning
 */
typedef void (*nrfx_power_pofwarn_event_handler_t)(void);

#if NRF_POWER_HAS_SLEEPEVT || defined(__NRFX_DOXYGEN__)
/**
 * @brief Event handler for entering/exiting sleep
 *
 * @param event Event type
 */
typedef void (*nrfx_power_sleep_event_handler_t)(nrfx_power_sleep_evt_t event);
#endif

#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
/**
 * @brief Event handler for USB related power events
 *
 * @param event Event type
 */
typedef void (*nrfx_power_usb_event_handler_t)(nrfx_power_usb_evt_t event);
#endif
/** @} */

/**
 * @brief General power configuration
 *
 * Parameters required to initialize power driver.
 */
typedef struct
{
    /**
     * @brief Enable main DCDC regulator
     *
     * This bit only informs the driver that elements for DCDC regulator
     * are installed and regulator can be used.
     * The regulator would be enabled or disabled automatically
     * by the hardware, basing on current power requirement.
     */
    bool dcdcen:1;

#if NRF_POWER_HAS_VDDH || defined(__NRFX_DOXYGEN__)
    /**
     * @brief Enable HV DCDC regulator
     *
     * This bit only informs the driver that elements for DCDC regulator
     * are installed and regulator can be used.
     * The regulator would be enabled or disabled automatically
     * by the hardware, basing on current power requirement.
     */
    bool dcdcenhv: 1;
#endif
}nrfx_power_config_t;

/**
 * @brief The configuration for power failure comparator
 *
 * Configuration used to enable and configure power failure comparator
 */
typedef struct
{
    nrfx_power_pofwarn_event_handler_t handler; //!< Event handler
    nrf_power_pof_thr_t                thr;     //!< Threshold for power failure detection
#if NRF_POWER_HAS_VDDH || defined(__NRFX_DOXYGEN__)
    nrf_power_pof_thrvddh_t            thrvddh; //!< Threshold for power failure detection on VDDH pin
#endif
}nrfx_power_pofwarn_config_t;

#if NRF_POWER_HAS_SLEEPEVT || defined(__NRFX_DOXYGEN__)
/**
 * @brief The configuration of sleep event processing
 *
 * Configuration used to enable and configure sleep event handling
 */
typedef struct
{
    nrfx_power_sleep_event_handler_t handler;    //!< Event handler
    bool                             en_enter:1; //!< Enable event on sleep entering
    bool                             en_exit :1; //!< Enable event on sleep exiting
}nrfx_power_sleepevt_config_t;
#endif

#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
/**
 * @brief The configuration of USB related power events
 *
 * Configuration used to enable and configure USB power event handling
 */
typedef struct
{
    nrfx_power_usb_event_handler_t handler; //!< Event processing
}nrfx_power_usbevt_config_t;
#endif /* NRF_POWER_HAS_USBREG */

/**
 * @brief Function for getting the handler of the power failure comparator.
 * @return Handler of the power failure comparator.
 */
nrfx_power_pofwarn_event_handler_t nrfx_power_pof_handler_get(void);

#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
/**
 * @brief Function for getting the handler of the USB power.
 * @return Handler of the USB power.
 */
nrfx_power_usb_event_handler_t nrfx_power_usb_handler_get(void);
#endif

/**
 * @brief Initialize power module driver
 *
 * Enabled power module driver would process all the interrupts from power system.
 *
 * @param[in] p_config Pointer to the structure with initial configuration.
 *
 * @retval NRFX_SUCCESS                   Successfully initialized.
 * @retval NRFX_ERROR_ALREADY_INITIALIZED Module was already initialized.
 */
nrfx_err_t nrfx_power_init(nrfx_power_config_t const * p_config);

/**
 * @brief Unintialize power module driver
 *
 * Disables all the interrupt handling in the module.
 *
 * @sa nrfx_power_init
 */
void nrfx_power_uninit(void);

/**
 * @brief Initialize power failure comparator
 *
 * Configures the power failure comparator. This function does not setup and enable it.
 * Those steps can be done with functions @ref nrfx_power_pof_enable and @ref nrfx_power_pof_disable
 * or with Softdevice API (when Softdevice is using).
 *
 * @param[in] p_config Configuration with values and event handler.
 *                     If event handler is set to NULL, interrupt would be disabled.
 */
void nrfx_power_pof_init(nrfx_power_pofwarn_config_t const * p_config);

/**
 * @brief Enable power failure comparator
 * Sets and enables interrupt of the power failure comparator. This functions cannot be using
 * when Softdevice is enabled. If event handler set in init function is set to NULL, interrupt
 * would be disabled.
 *
 * @param[in] p_config Configuration with values and event handler.
 */
void nrfx_power_pof_enable(nrfx_power_pofwarn_config_t const * p_config);

/**
 * @brief Disable the power failure comparator
 *
 * Disables the power failure comparator interrupt.
 */
void nrfx_power_pof_disable(void);

/**
 * @brief Clear the power failure comparator settings
 *
 * Clears the settings of the power failure comparator.
 */
void nrfx_power_pof_uninit(void);

#if NRF_POWER_HAS_SLEEPEVT || defined(__NRFX_DOXYGEN__)
/**
 * @brief Initialize sleep entering and exiting events processing
 *
 * Configures and setups the sleep event processing.
 *
 * @param[in] p_config Configuration with values and event handler.
 *
 * @sa nrfx_power_sleepevt_uninit
 *
 */
void nrfx_power_sleepevt_init(nrfx_power_sleepevt_config_t const * p_config);

/**
 * @brief Enable sleep entering and exiting events processing
 *
 * @param[in] p_config Configuration with values and event handler.
 */
void nrfx_power_sleepevt_enable(nrfx_power_sleepevt_config_t const * p_config);

/**
 * @brief Disable sleep entering and exiting events processing
 */
void nrfx_power_sleepevt_disable(void);

/**
 * @brief Uninitialize sleep entering and exiting events processing
 *
 * @sa nrfx_power_sleepevt_init
 */
void nrfx_power_sleepevt_uninit(void);
#endif /* NRF_POWER_HAS_SLEEPEVT */

#if NRF_POWER_HAS_USBREG || defined(__NRFX_DOXYGEN__)
/**
 * @brief Initialize USB power event processing
 *
 * Configures and setups the USB power event processing.
 *
 * @param[in] p_config Configuration with values and event handler.
 *
 * @sa nrfx_power_usbevt_uninit
 */
void nrfx_power_usbevt_init(nrfx_power_usbevt_config_t const * p_config);

/**
 * @brief Enable USB power event processing
 */
void nrfx_power_usbevt_enable(void);

/**
 * @brief Disable USB power event processing
 */
void nrfx_power_usbevt_disable(void);

/**
 * @brief Uninitalize USB power event processing
 *
 * @sa nrfx_power_usbevt_init
 */
void nrfx_power_usbevt_uninit(void);

/**
 *  @brief Get the status of USB power
 *
 *  @return Current USB power status
 */
__STATIC_INLINE nrfx_power_usb_state_t nrfx_power_usbstatus_get(void);

#endif /* NRF_POWER_HAS_USBREG */

#ifndef SUPPRESS_INLINE_IMPLEMENTATION

#if NRF_POWER_HAS_USBREG
__STATIC_INLINE nrfx_power_usb_state_t nrfx_power_usbstatus_get(void)
{
    uint32_t status = nrf_power_usbregstatus_get();
    if(0 == (status & NRF_POWER_USBREGSTATUS_VBUSDETECT_MASK))
    {
        return NRFX_POWER_USB_STATE_DISCONNECTED;
    }
    if(0 == (status & NRF_POWER_USBREGSTATUS_OUTPUTRDY_MASK))
    {
        return NRFX_POWER_USB_STATE_CONNECTED;
    }
    return NRFX_POWER_USB_STATE_READY;
}
#endif /* NRF_POWER_HAS_USBREG */

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */


void nrfx_power_irq_handler(void);


/** @} */

#ifdef __cplusplus
}
#endif

#endif /* NRFX_POWER_H__ */
