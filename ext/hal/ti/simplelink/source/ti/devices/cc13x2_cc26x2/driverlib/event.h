/******************************************************************************
*  Filename:       event.h
*  Revised:        2016-09-19 10:36:17 +0200 (Mon, 19 Sep 2016)
*  Revision:       47179
*
*  Description:    Defines and prototypes for the Event Handler.
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

//*****************************************************************************
//
//! \addtogroup peripheral_group
//! @{
//! \addtogroup event_api
//! @{
//
//*****************************************************************************

#ifndef __EVENT_H__
#define __EVENT_H__

//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdint.h>
#include "../inc/hw_types.h"
#include "../inc/hw_memmap.h"
#include "../inc/hw_event.h"
#include "debug.h"


//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Connects an event to an event subscriber via Event Fabric.
//!
//! This function connects event sources to event subscribers.
//!
//! It is not possible to read event status in this module (except software events).
//! Event status must be read in the module that contains the event source. How a
//! specific event subscriber reacts to an event is configured and documented in
//! the respective modules.
//!
//! For a full list of configurable and constant mapped event sources to event
//! subscribers see the register descriptions for
//! <a href="../register_descriptions/CPU_MMAP/EVENT.html" target="_blank">Event Fabric</a>.
//!
//! Defines for event subscriber argument (\c ui32EventSubscriber) have the format:
//! - \ti_code{EVENT_O_[subscriber_name]}
//!
//! Defines for event source argument (\c ui32EventSource) must have the
//! following format where valid \c event_enum values are found in the
//! register description :
//! - \ti_code{EVENT_[subscriber_name]_EV_[event_enum]}
//!
//! Examples of valid defines for \c ui32EventSource:
//! - EVENT_CPUIRQSEL30_EV_AUX_TDC_DONE
//! - EVENT_RFCSEL9_EV_AUX_COMPA
//! - EVENT_GPT0ACAPTSEL_EV_AON_RTC_UPD
//!
//! \note Each event subscriber can only receive a sub-set of the event sources!
//!
//! \note Switching the event source is not glitch free, so it is imperative
//! that the subscriber is disabled for interrupts when switching the event
//! source. The behavior is undefined if not disabled.
//!
//! \param ui32EventSubscriber is the \b configurable event subscriber to receive the event.
//! Click the event subscriber to see the list of valid event sources in the
//! register description.
//! - EVENT_O_CPUIRQSEL30  : System CPU interrupt 30
//! - EVENT_O_RFCSEL9      : RF Core event 9
//! - EVENT_O_GPT0ACAPTSEL : GPT 0A capture event
//! - EVENT_O_GPT0BCAPTSEL : GPT 0B capture event
//! - EVENT_O_GPT1ACAPTSEL : GPT 1A capture event
//! - EVENT_O_GPT1BCAPTSEL : GPT 1B capture event
//! - EVENT_O_GPT2ACAPTSEL : GPT 2A capture event
//! - EVENT_O_GPT2BCAPTSEL : GPT 2B capture event
//! - EVENT_O_GPT3ACAPTSEL : GPT 3A capture event
//! - EVENT_O_GPT3BCAPTSEL : GPT 3B capture event
//! - EVENT_O_UDMACH9SSEL  : uDMA channel 9 single request
//! - EVENT_O_UDMACH9BSEL  : uDMA channel 9 burst request
//! - EVENT_O_UDMACH10SSEL : uDMA channel 10 single request
//! - EVENT_O_UDMACH10BSEL : uDMA channel 10 burst request
//! - EVENT_O_UDMACH11SSEL : uDMA channel 11 single request
//! - EVENT_O_UDMACH11BSEL : uDMA channel 11 burst request
//! - EVENT_O_UDMACH12SSEL : uDMA channel 12 single request
//! - EVENT_O_UDMACH12BSEL : uDMA channel 12 burst request
//! - EVENT_O_UDMACH14BSEL : uDMA channel 14 single request
//! - EVENT_O_AUXSEL0      : AUX
//! - EVENT_O_I2SSTMPSEL0  : I2S
//! - EVENT_O_FRZSEL0      : Freeze modules (some modules can freeze on CPU Halt)
//! \param ui32EventSource is the specific event that must be acted upon.
//! - Format: \ti_code{EVENT_[subscriber_name]_EV_[event_enum]} (see explanation above)
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
EventRegister(uint32_t ui32EventSubscriber, uint32_t ui32EventSource)
{
    // Check the arguments.
    ASSERT(( ui32EventSubscriber == EVENT_O_CPUIRQSEL30  ) ||
           ( ui32EventSubscriber == EVENT_O_RFCSEL9      ) ||
           ( ui32EventSubscriber == EVENT_O_GPT0ACAPTSEL ) ||
           ( ui32EventSubscriber == EVENT_O_GPT0BCAPTSEL ) ||
           ( ui32EventSubscriber == EVENT_O_GPT1ACAPTSEL ) ||
           ( ui32EventSubscriber == EVENT_O_GPT1BCAPTSEL ) ||
           ( ui32EventSubscriber == EVENT_O_GPT2ACAPTSEL ) ||
           ( ui32EventSubscriber == EVENT_O_GPT2BCAPTSEL ) ||
           ( ui32EventSubscriber == EVENT_O_GPT3ACAPTSEL ) ||
           ( ui32EventSubscriber == EVENT_O_GPT3BCAPTSEL ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH9SSEL  ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH9BSEL  ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH10SSEL ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH10BSEL ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH11SSEL ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH11BSEL ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH12SSEL ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH12BSEL ) ||
           ( ui32EventSubscriber == EVENT_O_UDMACH14BSEL ) ||
           ( ui32EventSubscriber == EVENT_O_AUXSEL0      ) ||
           ( ui32EventSubscriber == EVENT_O_I2SSTMPSEL0  ) ||
           ( ui32EventSubscriber == EVENT_O_FRZSEL0      ) );

    // Map the event source to the event subscriber
    HWREG(EVENT_BASE + ui32EventSubscriber) = ui32EventSource;
}

//*****************************************************************************
//
//! \brief Sets software event.
//!
//! Setting a software event triggers the event if the value was 0 before.
//!
//! \note The software event must be cleared manually after the event has
//! triggered the event subscriber.
//!
//! \param ui32SwEvent is the software event number.
//! - 0 : SW Event 0
//! - 1 : SW Event 1
//! - 2 : SW Event 2
//! - 3 : SW Event 3
//!
//! \return None
//!
//! \sa \ref EventSwEventClear()
//
//*****************************************************************************
__STATIC_INLINE void
EventSwEventSet(uint32_t ui32SwEvent)
{
    // Check the arguments.
    ASSERT( ui32SwEvent <= 3 );

    // Each software event is byte accessible
    HWREGB(EVENT_BASE + EVENT_O_SWEV + ui32SwEvent) = 1;
}

//*****************************************************************************
//
//! \brief Clears software event.
//!
//! \param ui32SwEvent is the software event number.
//! - 0 : SW Event 0
//! - 1 : SW Event 1
//! - 2 : SW Event 2
//! - 3 : SW Event 3
//!
//! \return None
//
//*****************************************************************************
__STATIC_INLINE void
EventSwEventClear(uint32_t ui32SwEvent)
{
    // Check the arguments.
    ASSERT( ui32SwEvent <= 3 );

    // Each software event is byte accessible
    HWREGB(EVENT_BASE + EVENT_O_SWEV + ui32SwEvent) = 0;
}

//*****************************************************************************
//
//! \brief Gets software event status.
//!
//! \param ui32SwEvent is the software event number.
//! - 0 : SW Event 0
//! - 1 : SW Event 1
//! - 2 : SW Event 2
//! - 3 : SW Event 3
//!
//! \return Returns current value of requested software event.
//! - 0 : Software event is de-asserted.
//! - 1 : Software event is asserted.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
EventSwEventGet(uint32_t ui32SwEvent)
{
    // Check the arguments.
    ASSERT( ui32SwEvent <= 3 );

    // Each software event is byte accessible
    return( HWREGB(EVENT_BASE + EVENT_O_SWEV + ui32SwEvent));
}

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __EVENT_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
