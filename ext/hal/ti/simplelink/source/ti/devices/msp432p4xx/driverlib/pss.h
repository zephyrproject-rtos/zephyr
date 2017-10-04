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
#ifndef __PSS_H__
#define __PSS_H__

//*****************************************************************************
//
//! \addtogroup pss_api
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
#include <ti/devices/msp432p4xx/inc/msp.h>
#include <stdbool.h>

//*****************************************************************************
//
// Control specific variables
//
//*****************************************************************************
#define PSS_KEY_VALUE PSS_KEY_KEY_VAL

#define PSS_SVSMH PSS_IE_SVSMHIE

#define PSS_FULL_PERFORMANCE_MODE   0x01
#define PSS_NORMAL_PERFORMANCE_MODE 0x00

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! Enables output of the High Side interrupt flag on the device \b SVMHOUT pin
//!
//! \param activeLow True if the signal should be logic low when SVSMHIFG
//!     is set. False if signal should be high when \b SVSMHIFG is set.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_enableHighSidePinToggle(bool activeLow);

//*****************************************************************************
//
//! Disables output of the High Side interrupt flag on the device \b SVMHOUT pin
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_disableHighSidePinToggle(void);

//*****************************************************************************
//
//! Enables high side voltage supervisor/monitor.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_enableHighSide(void);

//*****************************************************************************
//
//! Disables high side voltage supervisor/monitor.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_disableHighSide(void);

//*****************************************************************************
//
//! Sets the performance mode of the high side regulator. Full performance
//! mode allows for the best response times while normal performance mode is
//! optimized for the lowest possible current consumption.
//!
//! \param powerMode is the performance mode to set. Valid values are one of
//! the following:
//!     - \b PSS_FULL_PERFORMANCE_MODE,
//!     - \b PSS_NORMAL_PERFORMANCE_MODE
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_setHighSidePerformanceMode(uint_fast8_t powerMode);

//*****************************************************************************
//
//! Gets the performance mode of the high side voltage regulator. Refer to the
//! user's guide for specific information about information about the different
//! performance modes.
//!
//! \return Performance mode of the voltage regulator
//
//*****************************************************************************
extern uint_fast8_t PSS_getHighSidePerformanceMode(void);

//*****************************************************************************
//
//! Sets the high side voltage supervisor to monitor mode
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_enableHighSideMonitor(void);

//*****************************************************************************
//
//! Switches the high side of the power supply system to be a supervisor instead
//! of a monitor
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_disableHighSideMonitor(void);

//*****************************************************************************
//
//! Sets the voltage level at which the high side of the device voltage
//! regulator triggers a reset. This value is represented as an unsigned eight
//! bit integer where only the lowest three bits are most significant.
//!
//! \param triggerVoltage Voltage level in which high side supervisor/monitor
//!         triggers a reset. See the device specific data sheet for details
//!         on these voltage levels.
//!
//! Typical values will vary from part to part (so it is very important to
//! check the SVSH section of the data sheet. For reference only, the typical
//! MSP432 101 values are listed below:
//!     - 0 --> 1.57V
//!     - 1 --> 1.62V
//!     - 2 --> 1.83V
//!     - 3 --> 2V
//!     - 4 --> 2.25V
//!     - 5 --> 2.4V
//!     - 6 --> 2.6V
//!     - 7 --> 2.8V
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_setHighSideVoltageTrigger(uint_fast8_t triggerVoltage);

//*****************************************************************************
//
//! Returns the voltage level at which the high side of the device voltage
//! regulator triggers a reset.
//!
//! \return The voltage level that the high side voltage supervisor/monitor
//! triggers a reset. This value is represented as an unsigned eight
//! bit integer where only the lowest three bits are most significant.
//! See \link PSS_setHighSideVoltageTrigger \endlink for information regarding
//! the return value
//
//*****************************************************************************
extern uint_fast8_t PSS_getHighSideVoltageTrigger(void);

//*****************************************************************************
//
//! Enables the power supply system interrupt source.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_enableInterrupt(void);

//*****************************************************************************
//
//! Disables the power supply system interrupt source.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_disableInterrupt(void);

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \return The current interrupt status ( \b PSS_SVSMH )
//!
//*****************************************************************************
extern uint32_t PSS_getInterruptStatus(void);

//*****************************************************************************
//
//! Clears power supply system interrupt source.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_clearInterruptFlag(void);


//*****************************************************************************
//
//! Enables the "forced" mode of the DCDC regulator. In this mode, the fail
//! safe mechanism that disables the regulator to LDO mode when the supply
//! voltage falls below the minimum supply voltage required for DCDC operation
//! is turned off.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_enableForcedDCDCOperation(void);


//*****************************************************************************
//
//! Disables the "forced" mode of the DCDC regulator. In this mode, the fail
//! safe mechanism that disables the regulator to LDO mode when the supply
//! voltage falls below the minimum supply voltage required for DCDC operation
//! is turned on.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_disableForcedDCDCOperation(void);

//*****************************************************************************
//
//! Registers an interrupt handler for the power supply system interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the power
//! supply system interrupt occurs.
//!
//! This function registers the handler to be called when a power supply system
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific PSS interrupts must be enabled
//! via PSS_enableInterrupt().  It is the interrupt handler's responsibility to
//! clear the interrupt source via PSS_clearInterruptFlag().
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the power supply system
//!
//! This function unregisters the handler to be called when a power supply
//! system interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void PSS_unregisterInterrupt(void);

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

#endif // __PSS_H__
