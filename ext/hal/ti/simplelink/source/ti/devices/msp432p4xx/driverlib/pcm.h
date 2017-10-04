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
#ifndef __PCM_H__
#define __PCM_H__

//*****************************************************************************
//
//! \addtogroup pcm_api
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
#define PCM_KEY 0x695A0000

/* Power Modes */
#define PCM_AM_LDO_VCORE0      0x00
#define PCM_AM_LDO_VCORE1      0x01
#define PCM_AM_DCDC_VCORE0     0x04
#define PCM_AM_DCDC_VCORE1     0x05
#define PCM_AM_LF_VCORE0      0x08
#define PCM_AM_LF_VCORE1      0x09
#define PCM_LPM0_LDO_VCORE0    0x10
#define PCM_LPM0_LDO_VCORE1    0x11
#define PCM_LPM0_DCDC_VCORE0   0x14
#define PCM_LPM0_DCDC_VCORE1   0x15
#define PCM_LPM0_LF_VCORE0    0x18
#define PCM_LPM0_LF_VCORE1    0x19
#define PCM_LPM3               0x20
#define PCM_LPM4               0x21
#define PCM_LPM35_VCORE0       0xC0
#define PCM_LPM45              0xA0

#define PCM_VCORE0     0x00
#define PCM_VCORE1     0x01
#define PCM_VCORELPM3   0x02

#define PCM_LDO_MODE   0x00
#define PCM_DCDC_MODE  0x01
#define PCM_LF_MODE   0x02

#define PCM_SHUTDOWN_PARTIAL PCM_LPM35_VCORE0
#define PCM_SHUTDOWN_COMPLETE PCM_LPM45

#define PCM_DCDCERROR PCM_IE_DCDC_ERROR_IE
#define PCM_AM_INVALIDTRANSITION PCM_IE_AM_INVALID_TR_IE
#define PCM_SM_INVALIDCLOCK PCM_IE_LPM_INVALID_CLK_IE
#define PCM_SM_INVALIDTRANSITION PCM_IE_LPM_INVALID_TR_IE

//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//******************************************************************************
//
//! Sets the core voltage level (Vcore). The function will take care of all
//! power state transitions needed to shift between core voltage levels.
//! Because transitions between voltage levels may require changes power modes,
//! the power mode might temporarily be change. The power mode will be returned
//! to the original state (with the new voltage level) at the end of a
//! successful execution of this function.
//!
//! Refer to the device specific data sheet for specifics about core voltage
//! levels.
//!
//! \param voltageLevel The voltage level to be shifted to.
//!           - \b PCM_VCORE0,
//!           - \b PCM_VCORE1
//!
//! \return true if voltage level set, false otherwise.
//
//******************************************************************************
extern bool PCM_setCoreVoltageLevel(uint_fast8_t voltageLevel);

//******************************************************************************
//
//! Returns the current powers state of the system see the
//! PCM_setCoreVoltageLevel function for specific information about the modes.
//!
//! \return The current voltage of the system
//!
//! Possible return values include:
//!     - \b PCM_VCORE0
//!     - \b PCM_VCORE1
//!     - \b PCM_VCORELPM3
//!
//
//******************************************************************************
extern uint8_t PCM_getCoreVoltageLevel(void);

//******************************************************************************
//
//! Sets the core voltage level (Vcore). This function will take care of all
//! power state transitions needed to shift between core voltage levels.
//! Because transitions between voltage levels may require changes power modes,
//! the power mode might temporarily be change. The power mode will be returned
//! to the original state (with the new voltage level) at the end of a
//! successful execution of this function.
//!
//! This function is similar to PCMSetCoreVoltageLevel, however a timeout
//! mechanism is used.
//!
//! Refer to the device specific data sheet for specifics about core voltage
//! levels.
//!
//! \param voltageLevel The voltage level to be shifted to.
//!           - \b PCM_VCORE0,
//!           - \b PCM_VCORE1
//!
//! \param timeOut Number of loop iterations to timeout when checking for
//!         power state transitions. This should be used for debugging initial
//!         power/hardware configurations. After a stable hardware base is
//!         established, the PCMSetCoreVoltageLevel function should be used
//!
//! \return true if voltage level set, false otherwise.
//
//******************************************************************************
extern bool PCM_setCoreVoltageLevelWithTimeout(uint_fast8_t voltageLevel,
        uint32_t timeOut);

//******************************************************************************
//
//! Sets the core voltage level (Vcore).  This function is similar to
//! PCM_setCoreVoltageLevel, however there are no polling flags to ensure
//! a state has changed. Execution is returned back to the calling program
//  and it is up to the user to ensure proper state transitions happen
//! correctly. For MSP432, changing into different power modes/states
//! require very specific logic. This function will initiate only one state
//! transition and then return. It is up to the user to keep calling this
//! function until the correct power state has been achieved.
//!
//! Refer to the device specific data sheet for specifics about core voltage
//! levels.
//!
//! \param voltageLevel The voltage level to be shifted to.
//!           - \b PCM_VCORE0,
//!           - \b PCM_VCORE1
//!
//! \return true if voltage level set, false otherwise.
//
//******************************************************************************
extern bool PCM_setCoreVoltageLevelNonBlocking(uint_fast8_t voltageLevel);

//******************************************************************************
//
//! Switches between power modes. This function will take care of all
//! power state transitions needed to shift between power modes. Note for
//! changing to DCDC mode, specific hardware considerations are required.
//!
//! Refer to the device specific data sheet for specifics about power modes.
//!
//! \param powerMode The voltage modes to be shifted to. Valid values are:
//!           - \b PCM_LDO_MODE,
//!           - \b PCM_DCDC_MODE,
//!           - \b PCM_LF_MODE
//!
//! \return true if power mode is set, false otherwise.
//
//******************************************************************************
extern bool PCM_setPowerMode(uint_fast8_t powerMode);

//******************************************************************************
//
//! Switches between power modes. This function will take care of all
//! power state transitions needed to shift between power modes. Note for
//! changing to DCDC mode, specific hardware considerations are required.
//!
//! This function is similar to PCMSetPowerMode, however a timeout
//! mechanism is used.
//!
//! Refer to the device specific data sheet for specifics about power modes.
//!
//! \param powerMode The voltage modes to be shifted to. Valid values are:
//!           - \b PCM_LDO_MODE,
//!           - \b PCM_DCDC_MODE,
//!           - \b PCM_LF_MODE
//!
//! \param timeOut Number of loop iterations to timeout when checking for
//!         power state transitions. This should be used for debugging initial
//!         power/hardware configurations. After a stable hardware base is
//!         established, the PCMSetPowerMode function should be used
//!
//! \return true if power mode is set, false otherwise.
//
//******************************************************************************
extern bool PCM_setPowerModeWithTimeout(uint_fast8_t powerMode,
        uint32_t timeOut);

//******************************************************************************
//
//! Sets the core voltage level (Vcore).  This function is similar to
//! PCM_setPowerMode, however there are no polling flags to ensure
//! a state has changed. Execution is returned back to the calling program
//  and it is up to the user to ensure proper state transitions happen
//! correctly. For MSP432, changing into different power modes/states
//! require very specific logic. This function will initiate only one state
//! transition and then return. It is up to the user to keep calling this
//! function until the correct power state has been achieved.
//!
//! Refer to the device specific data sheet for specifics about core voltage
//! levels.
//!
//! \param powerMode The voltage modes to be shifted to. Valid values are:
//!           - \b PCM_LDO_MODE,
//!           - \b PCM_DCDC_MODE,
//!           - \b PCM_LF_MODE
//!
//! \return true if power mode change was initiated, false otherwise
//
//******************************************************************************
extern bool PCM_setPowerModeNonBlocking(uint_fast8_t powerMode);

//******************************************************************************
//
//! Returns the current powers state of the system see the \b PCM_setPowerState
//! function for specific information about the modes.
//!
//! \return The current power mode of the system
//!
//
//******************************************************************************
extern uint8_t PCM_getPowerMode(void);

//******************************************************************************
//
//! Switches between power states. This is a convenience function that combines
//! the functionality of PCM_setPowerMode and PCM_setCoreVoltageLevel as well as
//! the LPM0/LPM3 functions.
//!
//! Refer to the device specific data sheet for specifics about power states.
//!
//! \param powerState The voltage modes to be shifted to. Valid values are:
//!           - \b PCM_AM_LDO_VCORE0,      [Active Mode, LDO, VCORE0]
//!           - \b PCM_AM_LDO_VCORE1,      [Active Mode, LDO, VCORE1]
//!           - \b PCM_AM_DCDC_VCORE0,     [Active Mode, DCDC, VCORE0]
//!           - \b PCM_AM_DCDC_VCORE1,     [Active Mode, DCDC, VCORE1]
//!           - \b PCM_AM_LF_VCORE0,       [Active Mode, Low Frequency, VCORE0]
//!           - \b PCM_AM_LF_VCORE1,       [Active Mode, Low Frequency, VCORE1]
//!           - \b PCM_LPM0_LDO_VCORE0,    [LMP0, LDO, VCORE0]
//!           - \b PCM_LPM0_LDO_VCORE1,    [LMP0, LDO, VCORE1]
//!           - \b PCM_LPM0_DCDC_VCORE0,   [LMP0, DCDC, VCORE0]
//!           - \b PCM_LPM0_DCDC_VCORE1,   [LMP0, DCDC, VCORE1]
//!           - \b PCM_LPM0_LF_VCORE0,     [LMP0, Low Frequency, VCORE0]
//!           - \b PCM_LPM0_LF_VCORE1,     [LMP0, Low Frequency, VCORE1]
//!           - \b PCM_LPM3,               [LPM3]
//!           - \b PCM_LPM35_VCORE0,       [LPM3.5 VCORE 0]
//!           - \b PCM_LPM4,               [LPM4]
//!           - \b PCM_LPM45,              [LPM4.5]
//!
//! \return true if power state is set, false otherwise.
//
//******************************************************************************
extern bool PCM_setPowerState(uint_fast8_t powerState);

//******************************************************************************
//
//! Switches between power states. This is a convenience function that combines
//! the functionality of PCM_setPowerMode and PCM_setCoreVoltageLevel as well as
//! the LPM modes.
//!
//! This function is similar to PCM_setPowerState, however a timeout
//! mechanism is used.
//!
//! Refer to the device specific data sheet for specifics about power states.
//!
//! \param powerState The voltage modes to be shifted to. Valid values are:
//!           - \b PCM_AM_LDO_VCORE0,      [Active Mode, LDO, VCORE0]
//!           - \b PCM_AM_LDO_VCORE1,      [Active Mode, LDO, VCORE1]
//!           - \b PCM_AM_DCDC_VCORE0,     [Active Mode, DCDC, VCORE0]
//!           - \b PCM_AM_DCDC_VCORE1,     [Active Mode, DCDC, VCORE1]
//!           - \b PCM_AM_LF_VCORE0,       [Active Mode, Low Frequency, VCORE0]
//!           - \b PCM_AM_LF_VCORE1,       [Active Mode, Low Frequency, VCORE1]
//!           - \b PCM_LPM0_LDO_VCORE0,    [LMP0, LDO, VCORE0]
//!           - \b PCM_LPM0_LDO_VCORE1,    [LMP0, LDO, VCORE1]
//!           - \b PCM_LPM0_DCDC_VCORE0,   [LMP0, DCDC, VCORE0]
//!           - \b PCM_LPM0_DCDC_VCORE1,   [LMP0, DCDC, VCORE1]
//!           - \b PCM_LPM0_LF_VCORE0,     [LMP0, Low Frequency, VCORE0]
//!           - \b PCM_LPM0_LF_VCORE1,     [LMP0, Low Frequency, VCORE1]
//!           - \b PCM_LPM3,               [LPM3]
//!           - \b PCM_LPM35_VCORE0,       [LPM3.5 VCORE 0]
//!           - \b PCM_LPM4,               [LPM4]
//!           - \b PCM_LPM45,              [LPM4.5]
//!
//! \param timeout Number of loop iterations to timeout when checking for
//!         power state transitions. This should be used for debugging initial
//!         power/hardware configurations. After a stable hardware base is
//!         established, the PCMSetPowerMode function should be used
//!
//! \return true if power state is set, false otherwise. It is important to
//!         note that if a timeout occurs, false will be returned, however the
//!         power state at this point is not guaranteed to be the same as the
//!         state prior to the function call
//
//******************************************************************************
extern bool PCM_setPowerStateWithTimeout(uint_fast8_t powerState,
        uint32_t timeout);

//******************************************************************************
//
//! Returns the current powers state of the system see the PCMChangePowerState
//! function for specific information about the states.
//!
//! Refer to \link PCM_setPowerState \endlink for possible return values.
//!
//! \return The current power state of the system
//
//******************************************************************************
extern uint8_t PCM_getPowerState(void);

//******************************************************************************
//
//! Sets the power state of the part.  This function is similar to
//! PCM_getPowerState, however there are no polling flags to ensure
//! a state has changed. Execution is returned back to the calling program
//  and it is up to the user to ensure proper state transitions happen
//! correctly. For MSP432, changing into different power modes/states
//! require very specific logic. This function will initiate only one state
//! transition and then return. It is up to the user to keep calling this
//! function until the correct power state has been achieved.
//!
//! Refer to the device specific data sheet for specifics about core voltage
//! levels.
//!
//! \param powerState The voltage modes to be shifted to. Valid values are:
//!           - \b PCM_AM_LDO_VCORE0,      [Active Mode, LDO, VCORE0]
//!           - \b PCM_AM_LDO_VCORE1,      [Active Mode, LDO, VCORE1]
//!           - \b PCM_AM_DCDC_VCORE0,     [Active Mode, DCDC, VCORE0]
//!           - \b PCM_AM_DCDC_VCORE1,     [Active Mode, DCDC, VCORE1]
//!           - \b PCM_AM_LF_VCORE0,       [Active Mode, Low Frequency, VCORE0]
//!           - \b PCM_AM_LF_VCORE1,       [Active Mode, Low Frequency, VCORE1]
//!           - \b PCM_LPM0_LDO_VCORE0,    [LMP0, LDO, VCORE0]
//!           - \b PCM_LPM0_LDO_VCORE1,    [LMP0, LDO, VCORE1]
//!           - \b PCM_LPM0_DCDC_VCORE0,   [LMP0, DCDC, VCORE0]
//!           - \b PCM_LPM0_DCDC_VCORE1,   [LMP0, DCDC, VCORE1]
//!           - \b PCM_LPM0_LF_VCORE0,     [LMP0, Low Frequency, VCORE0]
//!           - \b PCM_LPM0_LF_VCORE1,     [LMP0, Low Frequency, VCORE1]
//!           - \b PCM_LPM3,               [LPM3]
//!           - \b PCM_LPM35_VCORE0,       [LPM3.5 VCORE 0]
//!           - \b PCM_LPM45,              [LPM4.5]
//!
//! \return true if power state change was initiated, false otherwise
//
//******************************************************************************
extern bool PCM_setPowerStateNonBlocking(uint_fast8_t powerState);

//******************************************************************************
//
//! Transitions the device into LPM3.5/LPM4.5 mode.
//!
//! Refer to the device specific data sheet for specifics about shutdown modes.
//!
//! The following events will cause a wake up from LPM3.5 mode:
//! - Device reset
//! - External reset RST
//! - Enabled RTC, WDT, and wake-up I/O only interrupt events
//!
//! The following events will cause a wake up from the LPM4.5 mode:
//! - Device reset
//! - External reset RST
//! - Wake-up I/O only interrupt events
//!
//! \param shutdownMode Specific mode to go to. Valid values are:
//!            - \b PCM_LPM35_VCORE0
//!            - \b PCM_LPM45
//!
//!
//! \return false if LPM state cannot be entered, true otherwise.
//
//******************************************************************************
extern bool PCM_shutdownDevice(uint32_t shutdownMode);

//******************************************************************************
//
//! Transitions the device into LPM0.
//!
//! Refer to the device specific data sheet for specifics about low power modes.
//!
//! \return false if LPM0 state cannot be entered, true otherwise.
//
//******************************************************************************
extern bool PCM_gotoLPM0(void);

//******************************************************************************
//
//! Transitions the device into LPM3
//!
//! Refer to the device specific data sheet for specifics about low power modes.
//! Note that since LPM3 cannot be entered from  a DCDC power modes, the
//! power mode is first switched to LDO operation (if in DCDC mode),
//! LPM3 is entered, and the DCDC mode is restored on wake up.
//!
//! \return false if LPM3 state cannot be entered, true otherwise.
//
//******************************************************************************
extern bool PCM_gotoLPM3(void);

//******************************************************************************
//
//! Transitions the device into LPM0 while maintaining a safe
//! interrupt handling mentality. This function is meant to be used in
//! situations where the user wants to go to LPM0, however does not want
//! to go to "miss" any interrupts due to the fact that going to LPM0 is not
//! an atomic operation. This function will modify the PRIMASK and on exit of
//! the program the master interrupts will be disabled.
//!
//! Refer to the device specific data sheet for specifics about low power modes.
//!
//! \return false if LPM0 state cannot be entered, true otherwise.
//
//******************************************************************************
extern bool PCM_gotoLPM0InterruptSafe(void);

//******************************************************************************
//
//! Transitions the device into LPM3 while maintaining a safe
//! interrupt handling mentality. This function is meant to be used in
//! situations where the user wants to go to LPM3, however does not want
//! to go to "miss" any interrupts due to the fact that going to LPM3 is not
//! an atomic operation. This function will modify the PRIMASK and on exit of
//! the program the master interrupts will be disabled.
//!
//! Refer to the device specific data sheet for specifics about low power modes.
//! Note that since LPM3 cannot be entered from  a DCDC power modes, the
//! power mode is first switched to LDO operation (if in DCDC mode), the LPM3
//! is entered, and the DCDC mode is restored on wake up.
//!
//! \return false if LPM3 cannot be entered, true otherwise.
//
//******************************************************************************
extern bool PCM_gotoLPM3InterruptSafe(void);

//******************************************************************************
//
//! Transitions the device into LPM4. LPM4 is the exact same with LPM3, just
//! with RTC_C and WDT_A disabled. When waking up, RTC_C and WDT_A will remain
//! disabled until reconfigured by the user.
//!
//! \return false if LPM4 state cannot be entered, true otherwise.
//
//******************************************************************************
extern bool PCM_gotoLPM4(void);

//******************************************************************************
//
//! Transitions the device into LPM4 while maintaining a safe
//! interrupt handling mentality. This function is meant to be used in
//! situations where the user wants to go to LPM4, however does not want
//! to go to "miss" any interrupts due to the fact that going to LPM4 is not
//! an atomic operation. This function will modify the PRIMASK and on exit of
//! the program the master interrupts will be disabled.
//!
//! Refer to the device specific data sheet for specifics about low power modes.
//! Note that since LPM3 cannot be entered from  a DCDC power modes, the
//! power mode is first switched to LDO operation (if in DCDC mode),
//! LPM4 is entered, and the DCDC mode is restored on wake up.
//!
//! \return false if LPM4 state cannot be entered, true otherwise.
//
//******************************************************************************
extern bool PCM_gotoLPM4InterruptSafe(void);

//******************************************************************************
//
//! Enables "rude mode" entry into LPM3 and shutdown modes. With this mode
//! enabled, an entry into shutdown or LPM3 will occur even if there are
//! clock systems active. The system will forcibly  turn off all clock/systems
//! when going into these modes.
//!
//! \return None
//
//******************************************************************************
extern void PCM_enableRudeMode(void);

//******************************************************************************
//
//! Disables "rude mode" entry into LPM3 and shutdown modes. With this
//! mode disabled, an entry into shutdown or LPM3 will wait for any
//! active clock requests to free up before going into LPM3 or shutdown.
//!
//! \return None
//
//******************************************************************************
extern void PCM_disableRudeMode(void);

//*****************************************************************************
//
//! Enables individual power control interrupt sources.
//!
//! \param flags is a bit mask of the interrupt sources to be enabled.  Must
//! be a logical OR of:
//!         - \b PCM_DCDCERROR,
//!         - \b PCM_AM_INVALIDTRANSITION,
//!         - \b PCM_SM_INVALIDCLOCK,
//!         - \b PCM_SM_INVALIDTRANSITION
//!
//! This function enables the indicated power control interrupt sources.  Only
//! the sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//!
//! \return None.
//
//*****************************************************************************
extern void PCM_enableInterrupt(uint32_t flags);

//*****************************************************************************
//
//! Disables individual power control interrupt sources.
//!
//! \param flags is a bit mask of the interrupt sources to be enabled.  Must
//! be a logical OR of:
//!         - \b PCM_DCDCERROR,
//!         - \b PCM_AM_INVALIDTRANSITION,
//!         - \b PCM_SM_INVALIDCLOCK,
//!         - \b PCM_SM_INVALIDTRANSITION
//!
//! This function disables the indicated power control interrupt sources.  Only
//! the sources that are enabled can be reflected to the processor interrupt;
//! disabled sources have no effect on the processor.
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//!
//! \return None.
//
//*****************************************************************************
extern void PCM_disableInterrupt(uint32_t flags);

//*****************************************************************************
//
//! Gets the current interrupt status.
//!
//! \return The current interrupt status, enumerated as a bit field of:
//!         - \b PCM_DCDCERROR,
//!         - \b PCM_AM_INVALIDTRANSITION,
//!         - \b PCM_SM_INVALIDCLOCK,
//!         - \b PCM_SM_INVALIDTRANSITION
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//
//*****************************************************************************
extern uint32_t PCM_getInterruptStatus(void);

//*****************************************************************************
//
//! Gets the current interrupt status masked with the enabled interrupts.
//! This function is useful to call in ISRs to get a list of pending
//! interrupts that are actually enabled and could have caused
//! the ISR.
//!
//! \return The current interrupt status, enumerated as a bit field of:
//!         - \b PCM_DCDCERROR,
//!         - \b PCM_AM_INVALIDTRANSITION,
//!         - \b PCM_SM_INVALIDCLOCK,
//!         - \b PCM_SM_INVALIDTRANSITION
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//
//*****************************************************************************
extern uint32_t PCM_getEnabledInterruptStatus(void);

//*****************************************************************************
//
//! Clears power system interrupt sources.
//!
//! The specified power system interrupt sources are cleared, so that they no
//! longer assert.  This function must be called in the interrupt handler to
//! keep it from being called again immediately upon exit.
//!
//! \note Because there is a write buffer in the Cortex-M processor, it may
//! take several clock cycles before the interrupt source is actually cleared.
//! Therefore, it is recommended that the interrupt source be cleared early in
//! the interrupt handler (as opposed to the very last action) to avoid
//! returning from the interrupt handler before the interrupt source is
//! actually cleared.  Failure to do so may result in the interrupt handler
//! being immediately reentered (because the interrupt controller still sees
//! the interrupt source asserted).
//!
//! \param flags is a bit mask of the interrupt sources to be cleared.  Must
//! be a logical OR of
//!         - \b PCM_DCDCERROR,
//!         - \b PCM_AM_INVALIDTRANSITION,
//!         - \b PCM_SM_INVALIDCLOCK,
//!         - \b PCM_SM_INVALIDTRANSITION
//!
//! \note The interrupt sources vary based on the part in use.
//! Please consult the data sheet for the part you are using to determine
//! which interrupt sources are available.
//!
//! \return None.
//
//*****************************************************************************
extern void PCM_clearInterruptFlag(uint32_t flags);

//*****************************************************************************
//
//! Registers an interrupt handler for the power system interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the power
//! system interrupt occurs.
//!
//! This function registers the handler to be called when a clock system
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific PCM  interrupts must be enabled
//! via PCM_enableInterrupt().  It is the interrupt handler's responsibility to
//! clear the interrupt source via \link PCM_clearInterruptFlag \endlink .
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void PCM_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the power system.
//!
//! This function unregisters the handler to be called when a power system
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void PCM_unregisterInterrupt(void);

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

#endif // __PCM_H__
