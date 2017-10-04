/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * --/COPYRIGHT--*/
#ifndef TIMER32_H_
#define TIMER32_H_

//*****************************************************************************
//
//! \addtogroup timer32_api
//! @{
//
//*****************************************************************************

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
#include <stdint.h>
#include <stdbool.h>
#include <ti/devices/msp432p4xx/inc/msp.h>

//*****************************************************************************
//
// Control specific variables
//
//*****************************************************************************
#define TIMER32_CMSIS(x) ((Timer32_Type *) x)

#define TIMER_OFFSET    0x020

#define TIMER32_0_BASE   (uint32_t)TIMER32_1
#define TIMER32_1_BASE   (uint32_t)TIMER32_2

#define TIMER32_0_INTERRUPT         INT_T32_INT1
#define TIMER32_1_INTERRUPT         INT_T32_INT2
#define TIMER32_COMBINED_INTERRUPT  INT_T32_INTC

#define TIMER32_16BIT            0x00
#define TIMER32_32BIT            0x01

#define TIMER32_PRESCALER_1      0x00
#define TIMER32_PRESCALER_16     0x04
#define TIMER32_PRESCALER_256    0x08

#define TIMER32_FREE_RUN_MODE   0x00
#define TIMER32_PERIODIC_MODE   0x01

//*****************************************************************************
//
// API Function prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! Initializes the Timer32 module
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//!
//! \param preScaler is the prescaler (or divider) to apply to the clock
//! source given to the Timer32 module.
//!         Valid values are
//!         - \b TIMER32_PRESCALER_1 [DEFAULT]
//!         - \b TIMER32_PRESCALER_16
//!         - \b TIMER32_PRESCALER_256
//! \param resolution is the bit resolution of the Timer32 module.
//!         Valid values are
//!         - \b TIMER32_16BIT [DEFAULT]
//!         - \b TIMER32_32BIT
//! \param mode selects between free run and periodic mode. In free run
//! mode, the value of the timer is reset to UINT16_MAX (for 16-bit mode) or
//! UINT32_MAX (for 16-bit mode) when the timer reaches zero. In periodic mode,
//! the timer is reset to the value set by the Timer32_setCount function.
//!         Valid values are
//!         - \b TIMER32_FREE_RUN_MODE [DEFAULT]
//!         - \b TIMER32_PERIODIC_MODE
//!
//!
//! \return None.
//
//*****************************************************************************
extern void Timer32_initModule(uint32_t timer, uint32_t preScaler,
        uint32_t resolution, uint32_t mode);

//*****************************************************************************
//
//! Sets the count of the timer and resets the current value to the value
//! passed. This value is set on the next rising edge of the clock provided to
//! the timer module
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//! \param count Value of the timer to set. Note that
//! if the timer is in 16-bit mode and a value is passed in that exceeds
//! UINT16_MAX, the value will be truncated to UINT16_MAX.
//!
//! Also note that if the timer is operating in periodic mode, the value passed
//! into this function will represent the new period of the timer (the value
//! which is reloaded into the timer each time it reaches a zero value).
//!
//! \return None
//
//*****************************************************************************
extern void Timer32_setCount(uint32_t timer, uint32_t count);

//*****************************************************************************
//
//! Sets the count of the timer without resetting the current value. When the
//! current value of the timer reaches zero, the value passed into this function
//! will be set as the new count value.
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//! \param count Value of the timer to set in the background. Note that
//! if the timer is in 16-bit mode and a value is passed in that exceeds
//! UINT16_MAX, the value will be truncated to UINT16_MAX.
//!
//! Also note that if the timer is operating in periodic mode, the value passed
//! into this function will represent the new period of the timer (the value
//! which is reloaded into the timer each time it reaches a zero value).
//!
//! \return None
//
//*****************************************************************************
extern void Timer32_setCountInBackground(uint32_t timer, uint32_t count);

//*****************************************************************************
//
//! Returns the current value of the timer.
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//!
//! \return The current count of the timer.
//
//*****************************************************************************
extern uint32_t Timer32_getValue(uint32_t timer);

//*****************************************************************************
//
//! Starts the timer. The Timer32_initModule function should be called (in
//! conjunction with Timer32_setCount if periodic mode is desired) prior to
//  starting the timer.
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//!
//! \param oneShot sets whether the Timer32 module operates in one shot
//!  or continuous mode. In one shot mode, the timer will halt when a zero is
//!  reached and stay halted until either:
//!         - The user calls the Timer32PeriodSet function
//!         - The Timer32_initModule is called to reinitialize the timer with one-shot
//!             mode disabled.
//!
//! A true value will cause the timer to operate in one shot mode while a false
//! value will cause the timer to operate in continuous mode
//!
//! \return None
//
//*****************************************************************************
extern void Timer32_startTimer(uint32_t timer, bool oneShot);

//*****************************************************************************
//
//! Halts the timer. Current count and setting values are preserved.
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//!
//! \return None
//
//*****************************************************************************
extern void Timer32_haltTimer(uint32_t timer);

//*****************************************************************************
//
//! Enables a Timer32 interrupt source.
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//!
//! Enables the indicated Timer32 interrupt source.
//!
//! \return None.
//
//*****************************************************************************
extern void Timer32_enableInterrupt(uint32_t timer);

//*****************************************************************************
//
//! Disables a Timer32 interrupt source.
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//!
//! Disables the indicated Timer32 interrupt source.
//!
//! \return None.
//
//*****************************************************************************
extern void Timer32_disableInterrupt(uint32_t timer);

//*****************************************************************************
//
//! Clears Timer32 interrupt source.
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//!
//! The Timer32 interrupt source is cleared, so that it no longer asserts.
//!
//! \return None.
//
//*****************************************************************************
extern void Timer32_clearInterruptFlag(uint32_t timer);

//*****************************************************************************
//
//! Gets the current Timer32 interrupt status.
//!
//! \param timer is the instance of the Timer32 module.
//! Valid parameters must be one of the following values:
//!         - \b TIMER32_0_BASE
//!         - \b TIMER32_1_BASE
//!
//! This returns the interrupt status for the Timer32 module. A positive value
//! will indicate that an interrupt is pending while a zero value will indicate
//! that no interrupt is pending.
//!
//! \return The current interrupt status
//
//*****************************************************************************
extern uint32_t Timer32_getInterruptStatus(uint32_t timer);

//*****************************************************************************
//
//! Registers an interrupt handler for Timer32 interrupts.
//!
//! \param timerInterrupt is the specific interrupt to register. For the
//! Timer32 module, there are a total of three different interrupts: one
//! interrupt for each two Timer32 modules, and a "combined" interrupt which
//! is a logical OR of each individual Timer32 interrupt.
//!         - \b TIMER32_0_INTERRUPT
//!         - \b TIMER32_1_INTERRUPT
//!         - \b TIMER32_COMBINED_INTERRUPT
//!
//! \param intHandler is a pointer to the function to be called when the
//! Timer32 interrupt occurs.
//!
//! This function registers the handler to be called when an Timer32
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific Timer32 interrupts must be enabled
//! via Timer32_enableInterrupt().  It is the interrupt handler's
//! responsibility to clear the interrupt source
//! via Timer32_clearInterruptFlag().
//!
//! \return None.
//
//*****************************************************************************
extern void Timer32_registerInterrupt(uint32_t timerInterrupt,
        void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the Timer32 interrupt.
//!
//! \param timerInterrupt is the specific interrupt to register. For the
//! Timer32 module, there are a total of three different interrupts: one
//! interrupt for each two Timer32 modules, and a "combined" interrupt which
//! is a logical OR of each individual Timer32 interrupt.
//!         - \b TIMER32_0_INTERRUPT
//!         - \b TIMER32_1_INTERRUPT
//!         - \b TIMER32_COMBINED_INTERRUPT
//!
//! This function unregisters the handler to be called when a Timer32
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void Timer32_unregisterInterrupt(uint32_t timerInterrupt);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif /* TIMER32_H_ */
