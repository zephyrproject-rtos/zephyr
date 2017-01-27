/* Copyright (c) 2016-2017 Nordic Semiconductor ASA
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

#ifndef NRF_USBD_H__
#define NRF_USBD_H__

/**
 * @ingroup nrf_drivers
 * @defgroup nrf_usbd_hal USBD HAL
 * @{
 *
 * @brief @tagAPI52840 Hardware access layer for Two Wire Interface Slave with EasyDMA
 * (USBD) peripheral.
 */

#include "nrf_peripherals.h"
#include "nrf.h"
#include "nrf_assert.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/**
 * @brief USBD tasks
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_USBD_TASK_STARTEPIN0    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[0] ), /**< Captures the EPIN[0].PTR, EPIN[0].MAXCNT and EPIN[0].CONFIG registers values, and enables control endpoint IN 0 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPIN1    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[1] ), /**< Captures the EPIN[1].PTR, EPIN[1].MAXCNT and EPIN[1].CONFIG registers values, and enables data endpoint IN 1 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPIN2    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[2] ), /**< Captures the EPIN[2].PTR, EPIN[2].MAXCNT and EPIN[2].CONFIG registers values, and enables data endpoint IN 2 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPIN3    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[3] ), /**< Captures the EPIN[3].PTR, EPIN[3].MAXCNT and EPIN[3].CONFIG registers values, and enables data endpoint IN 3 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPIN4    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[4] ), /**< Captures the EPIN[4].PTR, EPIN[4].MAXCNT and EPIN[4].CONFIG registers values, and enables data endpoint IN 4 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPIN5    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[5] ), /**< Captures the EPIN[5].PTR, EPIN[5].MAXCNT and EPIN[5].CONFIG registers values, and enables data endpoint IN 5 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPIN6    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[6] ), /**< Captures the EPIN[6].PTR, EPIN[6].MAXCNT and EPIN[6].CONFIG registers values, and enables data endpoint IN 6 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPIN7    = offsetof(NRF_USBD_Type, TASKS_STARTEPIN[7] ), /**< Captures the EPIN[7].PTR, EPIN[7].MAXCNT and EPIN[7].CONFIG registers values, and enables data endpoint IN 7 to respond to traffic from host */
    NRF_USBD_TASK_STARTISOIN    = offsetof(NRF_USBD_Type, TASKS_STARTISOIN   ), /**< Captures the ISOIN.PTR, ISOIN.MAXCNT and ISOIN.CONFIG registers values, and enables sending data on iso endpoint 8 */
    NRF_USBD_TASK_STARTEPOUT0   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[0]), /**< Captures the EPOUT[0].PTR, EPOUT[0].MAXCNT and EPOUT[0].CONFIG registers values, and enables control endpoint 0 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPOUT1   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[1]), /**< Captures the EPOUT[1].PTR, EPOUT[1].MAXCNT and EPOUT[1].CONFIG registers values, and enables data endpoint 1 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPOUT2   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[2]), /**< Captures the EPOUT[2].PTR, EPOUT[2].MAXCNT and EPOUT[2].CONFIG registers values, and enables data endpoint 2 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPOUT3   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[3]), /**< Captures the EPOUT[3].PTR, EPOUT[3].MAXCNT and EPOUT[3].CONFIG registers values, and enables data endpoint 3 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPOUT4   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[4]), /**< Captures the EPOUT[4].PTR, EPOUT[4].MAXCNT and EPOUT[4].CONFIG registers values, and enables data endpoint 4 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPOUT5   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[5]), /**< Captures the EPOUT[5].PTR, EPOUT[5].MAXCNT and EPOUT[5].CONFIG registers values, and enables data endpoint 5 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPOUT6   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[6]), /**< Captures the EPOUT[6].PTR, EPOUT[6].MAXCNT and EPOUT[6].CONFIG registers values, and enables data endpoint 6 to respond to traffic from host */
    NRF_USBD_TASK_STARTEPOUT7   = offsetof(NRF_USBD_Type, TASKS_STARTEPOUT[7]), /**< Captures the EPOUT[7].PTR, EPOUT[7].MAXCNT and EPOUT[7].CONFIG registers values, and enables data endpoint 7 to respond to traffic from host */
    NRF_USBD_TASK_STARTISOOUT   = offsetof(NRF_USBD_Type, TASKS_STARTISOOUT  ), /**< Captures the ISOOUT.PTR, ISOOUT.MAXCNT and ISOOUT.CONFIG registers values, and enables receiving of data on iso endpoint 8 */
    NRF_USBD_TASK_EP0RCVOUT     = offsetof(NRF_USBD_Type, TASKS_EP0RCVOUT    ), /**< Allows OUT data stage on control endpoint 0 */
    NRF_USBD_TASK_EP0STATUS     = offsetof(NRF_USBD_Type, TASKS_EP0STATUS    ), /**< Allows status stage on control endpoint 0 */
    NRF_USBD_TASK_EP0STALL      = offsetof(NRF_USBD_Type, TASKS_EP0STALL     ), /**< STALLs data and status stage on control endpoint 0 */
    NRF_USBD_TASK_DRIVEDPDM     = offsetof(NRF_USBD_Type, TASKS_DPDMDRIVE    ), /**< Forces D+ and D-lines to the state defined in the DPDMVALUE register */
    NRF_USBD_TASK_NODRIVEDPDM   = offsetof(NRF_USBD_Type, TASKS_DPDMNODRIVE  ), /**< Stops forcing D+ and D- lines to any state (USB engine takes control) */
    /*lint -restore*/
}nrf_usbd_task_t;

/**
 * @brief USBD events
 */
typedef enum
{
    /*lint -save -e30*/
    NRF_USBD_EVENT_USBRESET      = offsetof(NRF_USBD_Type, EVENTS_USBRESET   ), /**< Signals that a USB reset condition has been detected on the USB lines */
    NRF_USBD_EVENT_STARTED       = offsetof(NRF_USBD_Type, EVENTS_STARTED    ), /**< Confirms that the EPIN[n].PTR, EPIN[n].MAXCNT, EPIN[n].CONFIG, or EPOUT[n].PTR, EPOUT[n].MAXCNT and EPOUT[n].CONFIG registers have been captured on all endpoints reported in the EPSTATUS register */
    NRF_USBD_EVENT_ENDEPIN0      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[0] ), /**< The whole EPIN[0] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN1      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[1] ), /**< The whole EPIN[1] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN2      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[2] ), /**< The whole EPIN[2] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN3      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[3] ), /**< The whole EPIN[3] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN4      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[4] ), /**< The whole EPIN[4] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN5      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[5] ), /**< The whole EPIN[5] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN6      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[6] ), /**< The whole EPIN[6] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPIN7      = offsetof(NRF_USBD_Type, EVENTS_ENDEPIN[7] ), /**< The whole EPIN[7] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_EP0DATADONE   = offsetof(NRF_USBD_Type, EVENTS_EP0DATADONE), /**< An acknowledged data transfer has taken place on the control endpoint */
    NRF_USBD_EVENT_ENDISOIN0     = offsetof(NRF_USBD_Type, EVENTS_ENDISOIN   ), /**< The whole ISOIN buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT0     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[0]), /**< The whole EPOUT[0] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT1     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[1]), /**< The whole EPOUT[1] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT2     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[2]), /**< The whole EPOUT[2] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT3     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[3]), /**< The whole EPOUT[3] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT4     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[4]), /**< The whole EPOUT[4] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT5     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[5]), /**< The whole EPOUT[5] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT6     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[6]), /**< The whole EPOUT[6] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDEPOUT7     = offsetof(NRF_USBD_Type, EVENTS_ENDEPOUT[7]), /**< The whole EPOUT[7] buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_ENDISOOUT0    = offsetof(NRF_USBD_Type, EVENTS_ENDISOOUT  ), /**< The whole ISOOUT buffer has been consumed. The RAM buffer can be accessed safely by software. */
    NRF_USBD_EVENT_SOF           = offsetof(NRF_USBD_Type, EVENTS_SOF        ), /**< Signals that a SOF (start of frame) condition has been detected on the USB lines */
    NRF_USBD_EVENT_USBEVENT      = offsetof(NRF_USBD_Type, EVENTS_USBEVENT   ), /**< An event or an error not covered by specific events has occurred, check EVENTCAUSE register to find the cause */
    NRF_USBD_EVENT_EP0SETUP      = offsetof(NRF_USBD_Type, EVENTS_EP0SETUP   ), /**< A valid SETUP token has been received (and acknowledged) on the control endpoint */
    NRF_USBD_EVENT_DATAEP        = offsetof(NRF_USBD_Type, EVENTS_EPDATA     ), /**< A data transfer has occurred on a data endpoint, indicated by the EPDATASTATUS register */
    NRF_USBD_EVENT_ACCESSFAULT   = offsetof(NRF_USBD_Type, EVENTS_ACCESSFAULT), /**< >Access to an unavailable USB register has been attempted (software or EasyDMA) */
    /*lint -restore*/
}nrf_usbd_event_t;

/**
 * @brief USBD shorts
 */
typedef enum
{
    NRF_USBD_SHORT_EP0DATADONE_STARTEPIN0_MASK  = USBD_SHORTS_EP0DATADONE_STARTEPIN0_Msk , /**< Shortcut between EP0DATADONE event and STARTEPIN0 task */
    NRF_USBD_SHORT_EP0DATADONE_STARTEPOUT0_MASK = USBD_SHORTS_EP0DATADONE_STARTEPOUT0_Msk, /**< Shortcut between EP0DATADONE event and STARTEPOUT0 task */
    NRF_USBD_SHORT_EP0DATADONE_EP0STATUS_MASK   = USBD_SHORTS_EP0DATADONE_EP0STATUS_Msk  , /**< Shortcut between EP0DATADONE event and EP0STATUS task */
    NRF_USBD_SHORT_ENDEPOUT0_EP0STATUS_MASK     = USBD_SHORTS_ENDEPOUT0_EP0STATUS_Msk    , /**< Shortcut between ENDEPOUT[0] event and EP0STATUS task */
    NRF_USBD_SHORT_ENDEPOUT0_EP0RCVOUT_MASK     = USBD_SHORTS_ENDEPOUT0_EP0RCVOUT_Msk    , /**< Shortcut between ENDEPOUT[0] event and EP0RCVOUT task */
}nrf_usbd_short_mask_t;

/**
 * @brief USBD interrupts
 */
typedef enum
{
    NRF_USBD_INT_USBRESET_MASK    = USBD_INTEN_USBRESET_Msk   , /**< Enable or disable interrupt for USBRESET event */
    NRF_USBD_INT_STARTED_MASK     = USBD_INTEN_STARTED_Msk    , /**< Enable or disable interrupt for STARTED event */
    NRF_USBD_INT_ENDEPIN0_MASK    = USBD_INTEN_ENDEPIN0_Msk   , /**< Enable or disable interrupt for ENDEPIN[0] event */
    NRF_USBD_INT_ENDEPIN1_MASK    = USBD_INTEN_ENDEPIN1_Msk   , /**< Enable or disable interrupt for ENDEPIN[1] event */
    NRF_USBD_INT_ENDEPIN2_MASK    = USBD_INTEN_ENDEPIN2_Msk   , /**< Enable or disable interrupt for ENDEPIN[2] event */
    NRF_USBD_INT_ENDEPIN3_MASK    = USBD_INTEN_ENDEPIN3_Msk   , /**< Enable or disable interrupt for ENDEPIN[3] event */
    NRF_USBD_INT_ENDEPIN4_MASK    = USBD_INTEN_ENDEPIN4_Msk   , /**< Enable or disable interrupt for ENDEPIN[4] event */
    NRF_USBD_INT_ENDEPIN5_MASK    = USBD_INTEN_ENDEPIN5_Msk   , /**< Enable or disable interrupt for ENDEPIN[5] event */
    NRF_USBD_INT_ENDEPIN6_MASK    = USBD_INTEN_ENDEPIN6_Msk   , /**< Enable or disable interrupt for ENDEPIN[6] event */
    NRF_USBD_INT_ENDEPIN7_MASK    = USBD_INTEN_ENDEPIN7_Msk   , /**< Enable or disable interrupt for ENDEPIN[7] event */
    NRF_USBD_INT_EP0DATADONE_MASK = USBD_INTEN_EP0DATADONE_Msk, /**< Enable or disable interrupt for EP0DATADONE event */
    NRF_USBD_INT_ENDISOIN0_MASK   = USBD_INTEN_ENDISOIN_Msk   , /**< Enable or disable interrupt for ENDISOIN[0] event */
    NRF_USBD_INT_ENDEPOUT0_MASK   = USBD_INTEN_ENDEPOUT0_Msk  , /**< Enable or disable interrupt for ENDEPOUT[0] event */
    NRF_USBD_INT_ENDEPOUT1_MASK   = USBD_INTEN_ENDEPOUT1_Msk  , /**< Enable or disable interrupt for ENDEPOUT[1] event */
    NRF_USBD_INT_ENDEPOUT2_MASK   = USBD_INTEN_ENDEPOUT2_Msk  , /**< Enable or disable interrupt for ENDEPOUT[2] event */
    NRF_USBD_INT_ENDEPOUT3_MASK   = USBD_INTEN_ENDEPOUT3_Msk  , /**< Enable or disable interrupt for ENDEPOUT[3] event */
    NRF_USBD_INT_ENDEPOUT4_MASK   = USBD_INTEN_ENDEPOUT4_Msk  , /**< Enable or disable interrupt for ENDEPOUT[4] event */
    NRF_USBD_INT_ENDEPOUT5_MASK   = USBD_INTEN_ENDEPOUT5_Msk  , /**< Enable or disable interrupt for ENDEPOUT[5] event */
    NRF_USBD_INT_ENDEPOUT6_MASK   = USBD_INTEN_ENDEPOUT6_Msk  , /**< Enable or disable interrupt for ENDEPOUT[6] event */
    NRF_USBD_INT_ENDEPOUT7_MASK   = USBD_INTEN_ENDEPOUT7_Msk  , /**< Enable or disable interrupt for ENDEPOUT[7] event */
    NRF_USBD_INT_ENDISOOUT0_MASK  = USBD_INTEN_ENDISOOUT_Msk  , /**< Enable or disable interrupt for ENDISOOUT[0] event */
    NRF_USBD_INT_SOF_MASK         = USBD_INTEN_SOF_Msk        , /**< Enable or disable interrupt for SOF event */
    NRF_USBD_INT_USBEVENT_MASK    = USBD_INTEN_USBEVENT_Msk   , /**< Enable or disable interrupt for USBEVENT event */
    NRF_USBD_INT_EP0SETUP_MASK    = USBD_INTEN_EP0SETUP_Msk   , /**< Enable or disable interrupt for EP0SETUP event */
    NRF_USBD_INT_DATAEP_MASK      = USBD_INTEN_EPDATA_Msk     , /**< Enable or disable interrupt for EPDATA event */
    NRF_USBD_INT_ACCESSFAULT_MASK = USBD_INTEN_ACCESSFAULT_Msk, /**< Enable or disable interrupt for ACCESSFAULT event */
}nrf_usbd_int_mask_t;


/**
 * @brief Function for activating a specific USBD task.
 *
 * @param task Task.
 */
__STATIC_INLINE void nrf_usbd_task_trigger(nrf_usbd_task_t task);

/**
 * @brief Function for returning the address of a specific USBD task register.
 *
 * @param task Task.
 *
 * @return Task address.
 */
__STATIC_INLINE uint32_t nrf_usbd_task_address_get(nrf_usbd_task_t task);

/**
 * @brief Function for clearing a specific event.
 *
 * @param event Event.
 */
__STATIC_INLINE void nrf_usbd_event_clear(nrf_usbd_event_t event);

/**
 * @brief Function for returning the state of a specific event.
 *
 * @param event Event.
 *
 * @retval true If the event is set.
 * @retval false If the event is not set.
 */
__STATIC_INLINE bool nrf_usbd_event_check(nrf_usbd_event_t event);

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
__STATIC_INLINE bool nrf_usbd_event_get_and_clear(nrf_usbd_event_t event);

/**
 * @brief Function for returning the address of a specific USBD event register.
 *
 * @param     event  Event.
 *
 * @return Address.
 */
__STATIC_INLINE uint32_t nrf_usbd_event_address_get(nrf_usbd_event_t event);

/**
 * @brief Function for setting a shortcut.
 *
 * @param     short_mask Shortcuts mask.
 */
__STATIC_INLINE void nrf_usbd_shorts_enable(uint32_t short_mask);

/**
 * @brief Function for clearing shortcuts.
 *
 * @param     short_mask Shortcuts mask.
 */
__STATIC_INLINE void nrf_usbd_shorts_disable(uint32_t short_mask);

/**
 * @brief Get the shorts mask
 *
 * Function returns shorts register.
 *
 * @return Flags of currently enabled shortcuts
 */
__STATIC_INLINE uint32_t nrf_usbd_shorts_get(void);

/**
 * @brief Function for enabling selected interrupts.
 *
 * @param     int_mask Interrupts mask.
 */
__STATIC_INLINE void nrf_usbd_int_enable(uint32_t int_mask);

/**
 * @brief Function for retrieving the state of selected interrupts.
 *
 * @param     int_mask Interrupts mask.
 *
 * @retval true If any of selected interrupts is enabled.
 * @retval false If none of selected interrupts is enabled.
 */
__STATIC_INLINE bool nrf_usbd_int_enable_check(uint32_t int_mask);

/**
 * @brief Function for retrieving the information about enabled interrupts.
 *
 * @return The flags of enabled interrupts.
 */
__STATIC_INLINE uint32_t nrf_usbd_int_enable_get(void);

/**
 * @brief Function for disabling selected interrupts.
 *
 * @param     int_mask Interrupts mask.
 */
__STATIC_INLINE void nrf_usbd_int_disable(uint32_t int_mask);


/** @} */ /*  End of nrf_usbd_hal */


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
__STATIC_INLINE volatile uint32_t* nrf_usbd_getRegPtr(uint32_t offset)
{
    return (volatile uint32_t*)(((uint8_t *)NRF_USBD) + (uint32_t)offset);
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
__STATIC_INLINE volatile const uint32_t* nrf_usbd_getRegPtr_c(uint32_t offset)
{
    return (volatile const uint32_t*)(((uint8_t *)NRF_USBD) + (uint32_t)offset);
}

/* ------------------------------------------------------------------------------------------------
 *  Interface functions definitions
 */

void nrf_usbd_task_trigger(nrf_usbd_task_t task)
{
    *(nrf_usbd_getRegPtr((uint32_t)task)) = 1UL;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_task_address_get(nrf_usbd_task_t task)
{
    return (uint32_t)nrf_usbd_getRegPtr_c((uint32_t)task);
}

void nrf_usbd_event_clear(nrf_usbd_event_t event)
{
    *(nrf_usbd_getRegPtr((uint32_t)event)) = 0UL;
    __ISB();
    __DSB();
}

bool nrf_usbd_event_check(nrf_usbd_event_t event)
{
    return (bool)*nrf_usbd_getRegPtr_c((uint32_t)event);
}

bool nrf_usbd_event_get_and_clear(nrf_usbd_event_t event)
{
    bool ret = nrf_usbd_event_check(event);
    if(ret)
    {
        nrf_usbd_event_clear(event);
    }
    return ret;
}

uint32_t nrf_usbd_event_address_get(nrf_usbd_event_t event)
{
    return (uint32_t)nrf_usbd_getRegPtr_c((uint32_t)event);
}

void nrf_usbd_shorts_enable(uint32_t short_mask)
{
    NRF_USBD->SHORTS |= short_mask;
}

void nrf_usbd_shorts_disable(uint32_t short_mask)
{
    if(~0U == short_mask)
    {
        /* Optimized version for "disable all" */
        NRF_USBD->SHORTS = 0;
    }
    else
    {
        NRF_USBD->SHORTS &= ~short_mask;
    }
}

uint32_t nrf_usbd_shorts_get(void)
{
    return NRF_USBD->SHORTS;
}

void nrf_usbd_int_enable(uint32_t int_mask)
{
    NRF_USBD->INTENSET = int_mask;
}

bool nrf_usbd_int_enable_check(uint32_t int_mask)
{
    return !!(NRF_USBD->INTENSET & int_mask);
}

uint32_t nrf_usbd_int_enable_get(void)
{
    return NRF_USBD->INTENSET;
}

void nrf_usbd_int_disable(uint32_t int_mask)
{
    NRF_USBD->INTENCLR = int_mask;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

/* ------------------------------------------------------------------------------------------------
 *  End of automatically generated part
 * ------------------------------------------------------------------------------------------------
 */
/**
 * @ingroup nrf_usbd_hal
 * @{
 */

/**
 * @brief Frame counter size
 *
 * The number of counts that can be fitted into frame counter
 */
#define NRF_USBD_FRAMECNTR_SIZE ( (USBD_FRAMECNTR_FRAMECNTR_Msk >> USBD_FRAMECNTR_FRAMECNTR_Pos) + 1UL )
#ifndef USBD_FRAMECNTR_FRAMECNTR_Msk
#error USBD_FRAMECNTR_FRAMECNTR_Msk should be changed into USBD_FRAMECNTR_FRAMECNTR_Msk
#endif

/**
 * @brief First isochronous endpoint number
 *
 * The number of the first isochronous endpoint
 */
#define NRF_USBD_EPISO_FIRST 8

/**
 * @brief Total number of IN endpoints
 *
 * Total number of IN endpoint (including ISOCHRONOUS).
 */
#define NRF_USBD_EPIN_CNT 9

/**
 * @brief Total number of OUT endpoints
 *
 * Total number of OUT endpoint (including ISOCHRONOUS).
 */
#define NRF_USBD_EPOUT_CNT 9

/**
 * @brief Mask of the direction bit in endpoint number
 */
#define NRF_USBD_EP_DIR_Msk (1U << 7)

/**
 * @brief The value of direction bit for IN endpoint direction
 */
#define NRF_USBD_EP_DIR_IN  (1U << 7)

/**
 * @brief The value of direction bit for OUT endpoint direction
 */
#define NRF_USBD_EP_DIR_OUT (0U << 7)

/**
 * @brief Make IN endpoint identifier from endpoint number
 *
 * Macro that sets direction bit to make IN endpoint
 * @param[in] epnr Endpoint number
 * @return IN Endpoint identifier
 */
#define NRF_USBD_EPIN(epnr)  (((uint8_t)(epnr)) | NRF_USBD_EP_DIR_IN)

/**
 * @brief Make OUT endpoint identifier from endpoint number
 *
 * Macro that sets direction bit to make OUT endpoint
 * @param[in] epnr Endpoint number
 * @return OUT Endpoint identifier
 */
#define NRF_USBD_EPOUT(epnr) (((uint8_t)(epnr)) | NRF_USBD_EP_DIR_OUT)

/**
 * @brief Extract the endpoint number from endpoint identifier
 *
 * Macro that strips out the information about endpoint direction.
 * @param[in] ep Endpoint identifier
 * @return Endpoint number
 */
#define NRF_USBD_EP_NR_GET(ep) ((uint8_t)(((uint8_t)(ep)) & 0xFU))

/**
 * @brief Macro for checking endpoint direction
 *
 * This macro checks if given endpoint has IN direction
 * @param ep Endpoint identifier
 * @retval true  If the endpoint direction is IN
 * @retval false If the endpoint direction is OUT
 */
#define NRF_USBD_EPIN_CHECK(ep)  ( (((uint8_t)(ep)) & NRF_USBD_EP_DIR_Msk) == NRF_USBD_EP_DIR_IN  )

/**
 * @brief Macro for checking endpoint direction
 *
 * This macro checks if given endpoint has OUT direction
 * @param ep Endpoint identifier
 * @retval true  If the endpoint direction is OUT
 * @retval false If the endpoint direction is IN
 */
#define NRF_USBD_EPOUT_CHECK(ep) ( (((uint8_t)(ep)) & NRF_USBD_EP_DIR_Msk) == NRF_USBD_EP_DIR_OUT )

/**
 * @brief Macro for checking if endpoint is isochronous
 *
 * @param ep It can be endpoint identifier or just endpoint number to check
 * @retval true  The endpoint is isochronous type
 * @retval false The endpoint is bulk of interrupt type
 */
#define NRF_USBD_EPISO_CHECK(ep) (NRF_USBD_EP_NR_GET(ep) >= NRF_USBD_EPISO_FIRST)


/**
 * @brief EVENTCAUSE register bit masks
 */
typedef enum
{
    NRF_USBD_EVENTCAUSE_ISOOUTCRC_MASK = USBD_EVENTCAUSE_ISOOUTCRC_Msk, /**< CRC error was detected on isochronous OUT endpoint 8. */
    NRF_USBD_EVENTCAUSE_SUSPEND_MASK   = USBD_EVENTCAUSE_SUSPEND_Msk  , /**< Signals that the USB lines have been seen idle long enough for the device to enter suspend. */
    NRF_USBD_EVENTCAUSE_RESUME_MASK    = USBD_EVENTCAUSE_RESUME_Msk   , /**< Signals that a RESUME condition (K state or activity restart) has been detected on the USB lines. */
    NRF_USBD_EVENTCAUSE_READY_MASK     = USBD_EVENTCAUSE_READY_Msk      /**< MAC is ready for normal operation, rised few us after USBD enabling */
}nrf_usbd_eventcause_mask_t;

/**
 * @brief BUSSTATE register bit masks
 */
typedef enum
{
    NRF_USBD_BUSSTATE_DM_MASK = USBD_BUSSTATE_DM_Msk, /**< Negative line mask */
    NRF_USBD_BUSSTATE_DP_MASK = USBD_BUSSTATE_DP_Msk, /**< Positive line mask */
    /** Both lines are low */
    NRF_USBD_BUSSTATE_DPDM_LL = (USBD_BUSSTATE_DM_Low  << USBD_BUSSTATE_DM_Pos) | (USBD_BUSSTATE_DP_Low  << USBD_BUSSTATE_DP_Pos),
    /** Positive line is high, negative line is low */
    NRF_USBD_BUSSTATE_DPDM_HL = (USBD_BUSSTATE_DM_Low  << USBD_BUSSTATE_DM_Pos) | (USBD_BUSSTATE_DP_High << USBD_BUSSTATE_DP_Pos),
    /** Positive line is low, negative line is high */
    NRF_USBD_BUSSTATE_DPDM_LH = (USBD_BUSSTATE_DM_High << USBD_BUSSTATE_DM_Pos) | (USBD_BUSSTATE_DP_Low  << USBD_BUSSTATE_DP_Pos),
    /** Both lines are high */
    NRF_USBD_BUSSTATE_DPDM_HH = (USBD_BUSSTATE_DM_High << USBD_BUSSTATE_DM_Pos) | (USBD_BUSSTATE_DP_High << USBD_BUSSTATE_DP_Pos),
    /** J state */
    NRF_USBD_BUSSTATE_J = NRF_USBD_BUSSTATE_DPDM_HL,
    /** K state */
    NRF_USBD_BUSSTATE_K = NRF_USBD_BUSSTATE_DPDM_LH,
    /** Single ended 0 */
    NRF_USBD_BUSSTATE_SE0 = NRF_USBD_BUSSTATE_DPDM_LL,
    /** Single ended 1 */
    NRF_USBD_BUSSTATE_SE1 = NRF_USBD_BUSSTATE_DPDM_HH
}nrf_usbd_busstate_t;

/**
 * @brief DPDMVALUE register
 */
typedef enum
{
    /**Generate Resume signal. Signal is generated for 50&nbsp;us or 5&nbsp;ms,
     * depending on bus state */
    NRF_USBD_DPDMVALUE_RESUME = USBD_DPDMVALUE_STATE_Resume,
    /** D+ Forced high, D- forced low (J state) */
    NRF_USBD_DPDMVALUE_J      = USBD_DPDMVALUE_STATE_J,
    /** D+ Forced low, D- forced high (K state) */
    NRF_USBD_DPMVALUE_K       = USBD_DPDMVALUE_STATE_K
}nrf_usbd_dpdmvalue_t;

/**
 * @brief Dtoggle value or operation
 */
typedef enum
{
    NRF_USBD_DTOGGLE_NOP   = USBD_DTOGGLE_VALUE_Nop,  /**< No operation - do not change current data toggle on selected endpoint */
    NRF_USBD_DTOGGLE_DATA0 = USBD_DTOGGLE_VALUE_Data0,/**< Data toggle is DATA0 on selected endpoint */
    NRF_USBD_DTOGGLE_DATA1 = USBD_DTOGGLE_VALUE_Data1 /**< Data toggle is DATA1 on selected endpoint */
}nrf_usbd_dtoggle_t;

/**
 * @brief EPSTATUS bit masks
 */
typedef enum
{
    NRF_USBD_EPSTATUS_EPIN0_MASK  = USBD_EPSTATUS_EPIN0_Msk,
    NRF_USBD_EPSTATUS_EPIN1_MASK  = USBD_EPSTATUS_EPIN1_Msk,
    NRF_USBD_EPSTATUS_EPIN2_MASK  = USBD_EPSTATUS_EPIN2_Msk,
    NRF_USBD_EPSTATUS_EPIN3_MASK  = USBD_EPSTATUS_EPIN3_Msk,
    NRF_USBD_EPSTATUS_EPIN4_MASK  = USBD_EPSTATUS_EPIN4_Msk,
    NRF_USBD_EPSTATUS_EPIN5_MASK  = USBD_EPSTATUS_EPIN5_Msk,
    NRF_USBD_EPSTATUS_EPIN6_MASK  = USBD_EPSTATUS_EPIN6_Msk,
    NRF_USBD_EPSTATUS_EPIN7_MASK  = USBD_EPSTATUS_EPIN7_Msk,

    NRF_USBD_EPSTATUS_EPOUT0_MASK = USBD_EPSTATUS_EPOUT0_Msk,
    NRF_USBD_EPSTATUS_EPOUT1_MASK = USBD_EPSTATUS_EPOUT1_Msk,
    NRF_USBD_EPSTATUS_EPOUT2_MASK = USBD_EPSTATUS_EPOUT2_Msk,
    NRF_USBD_EPSTATUS_EPOUT3_MASK = USBD_EPSTATUS_EPOUT3_Msk,
    NRF_USBD_EPSTATUS_EPOUT4_MASK = USBD_EPSTATUS_EPOUT4_Msk,
    NRF_USBD_EPSTATUS_EPOUT5_MASK = USBD_EPSTATUS_EPOUT5_Msk,
    NRF_USBD_EPSTATUS_EPOUT6_MASK = USBD_EPSTATUS_EPOUT6_Msk,
    NRF_USBD_EPSTATUS_EPOUT7_MASK = USBD_EPSTATUS_EPOUT7_Msk,
}nrf_usbd_epstatus_mask_t;

/**
 * @brief DATAEPSTATUS bit masks
 */
typedef enum
{
    NRF_USBD_EPDATASTATUS_EPIN1_MASK  = USBD_EPDATASTATUS_EPIN1_Msk,
    NRF_USBD_EPDATASTATUS_EPIN2_MASK  = USBD_EPDATASTATUS_EPIN2_Msk,
    NRF_USBD_EPDATASTATUS_EPIN3_MASK  = USBD_EPDATASTATUS_EPIN3_Msk,
    NRF_USBD_EPDATASTATUS_EPIN4_MASK  = USBD_EPDATASTATUS_EPIN4_Msk,
    NRF_USBD_EPDATASTATUS_EPIN5_MASK  = USBD_EPDATASTATUS_EPIN5_Msk,
    NRF_USBD_EPDATASTATUS_EPIN6_MASK  = USBD_EPDATASTATUS_EPIN6_Msk,
    NRF_USBD_EPDATASTATUS_EPIN7_MASK  = USBD_EPDATASTATUS_EPIN7_Msk,

    NRF_USBD_EPDATASTATUS_EPOUT1_MASK = USBD_EPDATASTATUS_EPOUT1_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT2_MASK = USBD_EPDATASTATUS_EPOUT2_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT3_MASK = USBD_EPDATASTATUS_EPOUT3_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT4_MASK = USBD_EPDATASTATUS_EPOUT4_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT5_MASK = USBD_EPDATASTATUS_EPOUT5_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT6_MASK = USBD_EPDATASTATUS_EPOUT6_Msk,
    NRF_USBD_EPDATASTATUS_EPOUT7_MASK = USBD_EPDATASTATUS_EPOUT7_Msk,
}nrf_usbd_dataepstatus_mask_t;

/**
 * @brief ISOSPLIT configurations
 */
typedef enum
{
    NRF_USBD_ISOSPLIT_OneDir = USBD_ISOSPLIT_SPLIT_OneDir, /**< Full buffer dedicated to either iso IN or OUT */
    NRF_USBD_ISOSPLIT_Half   = USBD_ISOSPLIT_SPLIT_HalfIN, /**< Buffer divided in half */

}nrf_usbd_isosplit_t;

/**
 * @brief Enable USBD
 */
__STATIC_INLINE void nrf_usbd_enable(void);

/**
 * @brief Disable USBD
 */
__STATIC_INLINE void nrf_usbd_disable(void);

/**
 * @brief Get EVENTCAUSE register
 *
 * @return Flag values defined in @ref nrf_usbd_eventcause_mask_t
 */
__STATIC_INLINE uint32_t nrf_usbd_eventcause_get(void);

/**
 * @brief Clear EVENTCAUSE flags
 *
 * @param flags Flags defined in @ref nrf_usbd_eventcause_mask_t
 */
__STATIC_INLINE void nrf_usbd_eventcause_clear(uint32_t flags);

/**
 * @brief Get EVENTCAUSE register and clear flags that are set
 *
 * The safest way to return current EVENTCAUSE register.
 * All the flags that are returned would be cleared inside EVENTCAUSE register.
 *
 * @return Flag values defined in @ref nrf_usbd_eventcause_mask_t
 */
__STATIC_INLINE uint32_t nrf_usbd_eventcause_get_and_clear(void);

/**
 * @brief Get BUSSTATE register value
 *
 * @return The value of BUSSTATE register
 */
__STATIC_INLINE nrf_usbd_busstate_t nrf_usbd_busstate_get(void);

/**
 * @brief Get HALTEDEPIN register value
 *
 * @param ep Endpoint number with IN/OUT flag
 *
 * @return The value of HALTEDEPIN or HALTEDOUT register for selected endpoint
 *
 * @note
 * Use this function for the response for GetStatus() request to endpoint.
 * To check if endpoint is stalled in the code use @ref nrf_usbd_ep_is_stall.
 */
__STATIC_INLINE uint32_t nrf_usbd_haltedep(uint8_t ep);

/**
 * @brief Check if selected endpoint is stalled
 *
 * Function to be used as a syntax sweeter for @ref nrf_usbd_haltedep.
 *
 * Also as the isochronous endpoint cannot be halted - it returns always false
 * if isochronous endpoint is checked.
 *
 * @param ep Endpoint number with IN/OUT flag
 *
 * @return The information if the enepoint is halted.
 */
__STATIC_INLINE bool nrf_usbd_ep_is_stall(uint8_t ep);

/**
 * @brief Get EPSTATUS register value
 *
 * @return Flag values defined in @ref nrf_usbd_epstatus_mask_t
 */
__STATIC_INLINE uint32_t nrf_usbd_epstatus_get(void);

/**
 * @brief Clear EPSTATUS register value
 *
 * @param flags Flags defined in @ref nrf_usbd_epstatus_mask_t
 */
__STATIC_INLINE void nrf_usbd_epstatus_clear(uint32_t flags);

/**
 * @brief Get and clear EPSTATUS register value
 *
 * Function clears all flags in register set before returning its value.
 * @return Flag values defined in @ref nrf_usbd_epstatus_mask_t
 */
__STATIC_INLINE uint32_t nrf_usbd_epstatus_get_and_clear(void);

/**
 * @brief Get DATAEPSTATUS register value
 *
 * @return Flag values defined in @ref nrf_usbd_dataepstatus_mask_t
 */
__STATIC_INLINE uint32_t nrf_usbd_epdatastatus_get(void);

/**
 * @brief Clear DATAEPSTATUS register value
 *
 * @param flags Flags defined in @ref nrf_usbd_dataepstatus_mask_t
 */
__STATIC_INLINE void nrf_usbd_epdatastatus_clear(uint32_t flags);

/**
 * @brief Get and clear DATAEPSTATUS register value
 *
 * Function clears all flags in register set before returning its value.
 * @return Flag values defined in @ref nrf_usbd_dataepstatus_mask_t
 */
__STATIC_INLINE uint32_t nrf_usbd_epdatastatus_get_and_clear(void);

/**
 * @name Setup command frame functions
 *
 * Functions for setup command frame parts access
 * @{
 */
    /**
     * @brief Read BMREQUESTTYPE - part of SETUP packet
     *
     * @return the value of BREQUESTTYPE on last received SETUP frame
     */
    __STATIC_INLINE uint8_t nrf_usbd_setup_bmrequesttype_get(void);

    /**
     * @brief Read BMREQUEST - part of SETUP packet
     *
     * @return the value of BREQUEST on last received SETUP frame
     */
    __STATIC_INLINE uint8_t nrf_usbd_setup_brequest_get(void);

    /**
     * @brief Read WVALUE - part of SETUP packet
     *
     * @return the value of WVALUE on last received SETUP frame
     */
    __STATIC_INLINE uint16_t nrf_usbd_setup_wvalue_get(void);

    /**
     * @brief Read WINDEX - part of SETUP packet
     *
     * @return the value of WINDEX on last received SETUP frame
     */
    __STATIC_INLINE uint16_t nrf_usbd_setup_windex_get(void);

    /**
     * @brief Read WLENGTH - part of SETUP packet
     *
     * @return the value of WLENGTH on last received SETUP frame
     */
    __STATIC_INLINE uint16_t nrf_usbd_setup_wlength_get(void);
/** @} */

/**
 * @brief Get number of received bytes on selected endpoint
 *
 * @param ep Endpoint identifier.
 *
 * @return Number of received bytes.
 *
 * @note This function may be used on Bulk/Interrupt and Isochronous endpoints.
 */
__STATIC_INLINE size_t nrf_usbd_epout_size_get(uint8_t ep);

/**
 * @brief Clear out endpoint to accept any new incoming traffic
 *
 * @param ep ep Endpoint identifier. Only OUT Interrupt/Bulk endpoints are accepted.
 */
__STATIC_INLINE void nrf_usbd_epout_clear(uint8_t ep);

/**
 * @brief Enable USB pullup
 */
__STATIC_INLINE void nrf_usbd_pullup_enable(void);

/**
 * @brief Disable USB pullup
 */
__STATIC_INLINE void nrf_usbd_pullup_disable(void);

/**
 * @brief Return USB pullup state
 */
__STATIC_INLINE bool nrf_usbd_pullup_check(void);

/**
 * @brief Configure the value to be forced on the bus on DRIVEDPDM task
 *
 * Selected state would be forced on the bus when @ref NRF_USBD_TASK_DRIVEDPM is set.
 * The state would be removed from the bus on @ref NRF_USBD_TASK_NODRIVEDPM and
 * the control would be returned to the USBD peripheral.
 * @param val State to be set
 */
__STATIC_INLINE void nrf_usbd_dpdmvalue_set(nrf_usbd_dpdmvalue_t val);

/**
 * @brief Data toggle set
 *
 * Configuration of current state of data toggling
 * @param ep Endpoint number with the information about its direction
 * @param op Operation to execute
 */
__STATIC_INLINE void nrf_usbd_dtoggle_set(uint8_t ep, nrf_usbd_dtoggle_t op);

/**
 * @brief Data toggle get
 *
 * Get the current state of data toggling
 * @param ep Endpoint number to return the information about current data toggling
 * @retval NRF_USBD_DTOGGLE_DATA0 Data toggle is DATA0 on selected endpoint
 * @retval NRF_USBD_DTOGGLE_DATA1 Data toggle is DATA1 on selected endpoint
 */
__STATIC_INLINE nrf_usbd_dtoggle_t nrf_usbd_dtoggle_get(uint8_t ep);

/**
 * @brief Enable selected endpoint
 *
 * Enabled endpoint responds for the tokens on the USB bus
 *
 * @param ep Endpoint id to enable
 */
__STATIC_INLINE void nrf_usbd_ep_enable(uint8_t ep);

/**
 * @brief Disable selected endpoint
 *
 * Disabled endpoint does not respond for the tokens on the USB bus
 *
 * @param ep Endpoint id to disable
 */
__STATIC_INLINE void nrf_usbd_ep_disable(uint8_t ep);

/**
 * @brief Disable all endpoints
 *
 * Auxiliary function to simply disable all aviable endpoints.
 * It lefts only EP0 IN and OUT enabled.
 */
__STATIC_INLINE void nrf_usbd_ep_all_disable(void);

/**
 * @brief Stall selected endpoint
 *
 * @param ep Endpoint identifier
 * @note This function cannot be called on isochronous endpoint
 */
__STATIC_INLINE void nrf_usbd_ep_stall(uint8_t ep);

/**
 * @brief Unstall selected endpoint
 *
 * @param ep Endpoint identifier
 * @note This function cannot be called on isochronous endpoint
 */
__STATIC_INLINE void nrf_usbd_ep_unstall(uint8_t ep);

/**
 * @brief Configure isochronous buffer splitting
 *
 * Configure isochronous buffer splitting between IN and OUT endpoints.
 *
 * @param split Required configuration
 */
__STATIC_INLINE void nrf_usbd_isosplit_set(nrf_usbd_isosplit_t split);

/**
 * @brief Get the isochronous buffer splitting configuration
 *
 * Get the current isochronous buffer splitting configuration.
 *
 * @return Current configuration
 */
__STATIC_INLINE nrf_usbd_isosplit_t nrf_usbd_isosplit_get(void);

/**
 * @brief Get current frame counter
 *
 * @return Current frame counter
 */
__STATIC_INLINE uint32_t nrf_usbd_framecntr_get(void);

/**
 * @brief Configure EasyDMA channel
 *
 * Configures EasyDMA for the transfer.
 *
 * @param ep     Endpoint identifier (with direction)
 * @param ptr    Pointer to the data
 * @param maxcnt Number of bytes to transfer
 */
__STATIC_INLINE void nrf_usbd_ep_easydma_set(uint8_t ep, uint32_t ptr, uint32_t maxcnt);

/**
 * @brief Get number of transferred bytes
 *
 * Get number of transferred bytes in the last transaction
 *
 * @param ep Endpoint identifier
 *
 * @return The content of the AMOUNT register
 */
__STATIC_INLINE uint32_t nrf_usbd_ep_amount_get(uint8_t ep);


#ifndef SUPPRESS_INLINE_IMPLEMENTATION

void nrf_usbd_enable(void)
{
#ifdef NRF_FPGA_IMPLEMENTATION
    *(volatile uint32_t *)0x400005F4 = 3;
    __ISB();
    __DSB();
    *(volatile uint32_t *)0x400005F0 = 3;
    __ISB();
    __DSB();
#endif

    NRF_USBD->ENABLE = USBD_ENABLE_ENABLE_Enabled << USBD_ENABLE_ENABLE_Pos;
    __ISB();
    __DSB();
}

void nrf_usbd_disable(void)
{
    NRF_USBD->ENABLE = USBD_ENABLE_ENABLE_Disabled << USBD_ENABLE_ENABLE_Pos;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_eventcause_get(void)
{
    return NRF_USBD->EVENTCAUSE;
}

void nrf_usbd_eventcause_clear(uint32_t flags)
{
    NRF_USBD->EVENTCAUSE = flags;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_eventcause_get_and_clear(void)
{
    uint32_t ret;
    ret = nrf_usbd_eventcause_get();
    nrf_usbd_eventcause_clear(ret);
    __ISB();
    __DSB();
    return ret;
}

nrf_usbd_busstate_t nrf_usbd_busstate_get(void)
{
    return (nrf_usbd_busstate_t)(NRF_USBD->BUSSTATE);
}

uint32_t nrf_usbd_haltedep(uint8_t ep)
{
    uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
    if(NRF_USBD_EPIN_CHECK(ep))
    {
        ASSERT(epnr < ARRAY_SIZE(NRF_USBD->HALTED.EPIN));
        return NRF_USBD->HALTED.EPIN[epnr];
    }
    else
    {
        ASSERT(epnr < ARRAY_SIZE(NRF_USBD->HALTED.EPOUT));
        return NRF_USBD->HALTED.EPOUT[epnr];
    }
}

bool nrf_usbd_ep_is_stall(uint8_t ep)
{
    if(NRF_USBD_EPISO_CHECK(ep))
        return false;
    return USBD_HALTED_EPOUT_GETSTATUS_Halted == nrf_usbd_haltedep(ep);
}

uint32_t nrf_usbd_epstatus_get(void)
{
    return NRF_USBD->EPSTATUS;
}

void nrf_usbd_epstatus_clear(uint32_t flags)
{
    NRF_USBD->EPSTATUS = flags;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_epstatus_get_and_clear(void)
{
    uint32_t ret;
    ret = nrf_usbd_epstatus_get();
    nrf_usbd_epstatus_clear(ret);
    return ret;
}

uint32_t nrf_usbd_epdatastatus_get(void)
{
    return NRF_USBD->EPDATASTATUS;
}

void nrf_usbd_epdatastatus_clear(uint32_t flags)
{
    NRF_USBD->EPDATASTATUS = flags;
    __ISB();
    __DSB();
}

uint32_t nrf_usbd_epdatastatus_get_and_clear(void)
{
    uint32_t ret;
    ret = nrf_usbd_epdatastatus_get();
    nrf_usbd_epdatastatus_clear(ret);
    __ISB();
    __DSB();
    return ret;
}

uint8_t nrf_usbd_setup_bmrequesttype_get(void)
{
    return (uint8_t)(NRF_USBD->BMREQUESTTYPE);
}

uint8_t nrf_usbd_setup_brequest_get(void)
{
    return (uint8_t)(NRF_USBD->BREQUEST);
}

uint16_t nrf_usbd_setup_wvalue_get(void)
{
    const uint16_t val = NRF_USBD->WVALUEL;
    return (uint16_t)(val | ((NRF_USBD->WVALUEH) << 8));
}

uint16_t nrf_usbd_setup_windex_get(void)
{
    const uint16_t val = NRF_USBD->WINDEXL;
    return (uint16_t)(val | ((NRF_USBD->WINDEXH) << 8));
}

uint16_t nrf_usbd_setup_wlength_get(void)
{
    const uint16_t val = NRF_USBD->WLENGTHL;
    return (uint16_t)(val | ((NRF_USBD->WLENGTHH) << 8));
}

size_t nrf_usbd_epout_size_get(uint8_t ep)
{
    ASSERT(NRF_USBD_EPOUT_CHECK(ep));
    if(NRF_USBD_EPISO_CHECK(ep))
    {
        ASSERT(NRF_USBD_EP_NR_GET(ep) == ARRAY_SIZE(NRF_USBD->SIZE.EPOUT)); /* Only single isochronous endpoint supported */
        return NRF_USBD->SIZE.ISOOUT;
    }

    ASSERT(NRF_USBD_EP_NR_GET(ep) < ARRAY_SIZE(NRF_USBD->SIZE.EPOUT));
    return NRF_USBD->SIZE.EPOUT[NRF_USBD_EP_NR_GET(ep)];
}

void nrf_usbd_epout_clear(uint8_t ep)
{
    ASSERT(NRF_USBD_EPOUT_CHECK(ep) && (NRF_USBD_EP_NR_GET(ep) < ARRAY_SIZE(NRF_USBD->SIZE.EPOUT)));
    NRF_USBD->SIZE.EPOUT[NRF_USBD_EP_NR_GET(ep)] = 0;
    __ISB();
    __DSB();
}

void nrf_usbd_pullup_enable(void)
{
    NRF_USBD->USBPULLUP = USBD_USBPULLUP_CONNECT_Enabled << USBD_USBPULLUP_CONNECT_Pos;
    __ISB();
    __DSB();
}

void nrf_usbd_pullup_disable(void)
{
    NRF_USBD->USBPULLUP = USBD_USBPULLUP_CONNECT_Disabled << USBD_USBPULLUP_CONNECT_Pos;
    __ISB();
    __DSB();
}

bool nrf_usbd_pullup_check(void)
{
    return NRF_USBD->USBPULLUP == (USBD_USBPULLUP_CONNECT_Enabled << USBD_USBPULLUP_CONNECT_Pos);
}

void nrf_usbd_dpdmvalue_set(nrf_usbd_dpdmvalue_t val)
{
    NRF_USBD->DPDMVALUE = ((uint32_t)val) << USBD_DPDMVALUE_STATE_Pos;
}

void nrf_usbd_dtoggle_set(uint8_t ep, nrf_usbd_dtoggle_t op)
{
    NRF_USBD->DTOGGLE = ep | (op << USBD_DTOGGLE_VALUE_Pos);
    __ISB();
    __DSB();
}

nrf_usbd_dtoggle_t nrf_usbd_dtoggle_get(uint8_t ep)
{
    uint32_t retval;
    /* Select the endpoint to read */
    nrf_usbd_dtoggle_set(ep, NRF_USBD_DTOGGLE_NOP);
    retval = ((NRF_USBD->DTOGGLE) & USBD_DTOGGLE_VALUE_Msk) >> USBD_DTOGGLE_VALUE_Pos;
    return (nrf_usbd_dtoggle_t)retval;
}

void nrf_usbd_ep_enable(uint8_t ep)
{
    uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
    ASSERT(epnr <= USBD_EPINEN_ISOIN_Pos);

    if(NRF_USBD_EPIN_CHECK(ep))
    {
        NRF_USBD->EPINEN |= 1UL<<epnr;
    }
    else
    {
        NRF_USBD->EPOUTEN |= 1UL<<epnr;
    }
    __ISB();
    __DSB();
}

void nrf_usbd_ep_disable(uint8_t ep)
{
    uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
    ASSERT(epnr <= USBD_EPINEN_ISOIN_Pos);

    if(NRF_USBD_EPIN_CHECK(ep))
    {
        NRF_USBD->EPINEN &= ~(1UL<<epnr);
    }
    else
    {
        NRF_USBD->EPOUTEN &= ~(1UL<<epnr);
    }
    __ISB();
    __DSB();
}

void nrf_usbd_ep_all_disable(void)
{
    NRF_USBD->EPINEN  = USBD_EPINEN_IN0_Enable << USBD_EPINEN_IN0_Pos;
    NRF_USBD->EPOUTEN = USBD_EPOUTEN_OUT0_Enable << USBD_EPOUTEN_OUT0_Pos;
    __ISB();
    __DSB();
}

void nrf_usbd_ep_stall(uint8_t ep)
{
    ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    NRF_USBD->EPSTALL = (USBD_EPSTALL_STALL_Stall << USBD_EPSTALL_STALL_Pos) | ep;
    __ISB();
    __DSB();
}

void nrf_usbd_ep_unstall(uint8_t ep)
{
    ASSERT(!NRF_USBD_EPISO_CHECK(ep));
    NRF_USBD->EPSTALL = (USBD_EPSTALL_STALL_UnStall << USBD_EPSTALL_STALL_Pos) | ep;
    __ISB();
    __DSB();
}

void nrf_usbd_isosplit_set(nrf_usbd_isosplit_t split)
{
    NRF_USBD->ISOSPLIT = split << USBD_ISOSPLIT_SPLIT_Pos;
}

nrf_usbd_isosplit_t nrf_usbd_isosplit_get(void)
{
    return (nrf_usbd_isosplit_t)(((NRF_USBD->ISOSPLIT) & USBD_ISOSPLIT_SPLIT_Msk) >> USBD_ISOSPLIT_SPLIT_Pos);
}

uint32_t nrf_usbd_framecntr_get(void)
{
    return NRF_USBD->FRAMECNTR;
}

void nrf_usbd_ep_easydma_set(uint8_t ep, uint32_t ptr, uint32_t maxcnt)
{
    if(NRF_USBD_EPIN_CHECK(ep))
    {
        if(NRF_USBD_EPISO_CHECK(ep))
        {
            NRF_USBD->ISOIN.PTR    = ptr;
            NRF_USBD->ISOIN.MAXCNT = maxcnt;
        }
        else
        {
            uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
            ASSERT(epnr < ARRAY_SIZE(NRF_USBD->EPIN));
            NRF_USBD->EPIN[epnr].PTR    = ptr;
            NRF_USBD->EPIN[epnr].MAXCNT = maxcnt;
        }
    }
    else
    {
        if(NRF_USBD_EPISO_CHECK(ep))
        {
            NRF_USBD->ISOOUT.PTR    = ptr;
            NRF_USBD->ISOOUT.MAXCNT = maxcnt;
        }
        else
        {
            uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
            ASSERT(epnr < ARRAY_SIZE(NRF_USBD->EPOUT));
            NRF_USBD->EPOUT[epnr].PTR    = ptr;
            NRF_USBD->EPOUT[epnr].MAXCNT = maxcnt;
        }
    }
}

uint32_t nrf_usbd_ep_amount_get(uint8_t ep)
{
    uint32_t ret;

    if(NRF_USBD_EPIN_CHECK(ep))
    {
        if(NRF_USBD_EPISO_CHECK(ep))
        {
            ret = NRF_USBD->ISOIN.AMOUNT;
        }
        else
        {
            uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
            ASSERT(epnr < ARRAY_SIZE(NRF_USBD->EPOUT));
            ret = NRF_USBD->EPIN[epnr].AMOUNT;
        }
    }
    else
    {
        if(NRF_USBD_EPISO_CHECK(ep))
        {
            ret = NRF_USBD->ISOOUT.AMOUNT;
        }
        else
        {
            uint8_t epnr = NRF_USBD_EP_NR_GET(ep);
            ASSERT(epnr < ARRAY_SIZE(NRF_USBD->EPOUT));
            ret = NRF_USBD->EPOUT[epnr].AMOUNT;
        }
    }

    return ret;
}

#endif /* SUPPRESS_INLINE_IMPLEMENTATION */

/** @} */
#endif /* NRF_USBD_H__ */
