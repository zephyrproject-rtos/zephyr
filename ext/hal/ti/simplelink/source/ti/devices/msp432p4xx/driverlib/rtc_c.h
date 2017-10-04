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
#ifndef RTC_C_H_
#define RTC_C_H_

//*****************************************************************************
//
//! \addtogroup rtc_api
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
//The following is a struct that can be passed to RTC_CalendarInit() in the
//CalendarTime parameter, as well as returned by RTC_getCalendarTime()
//
//*****************************************************************************
typedef struct _RTC_C_Calendar
{
    uint_fast8_t seconds;
    uint_fast8_t minutes;
    uint_fast8_t hours;
    uint_fast8_t dayOfWeek;
    uint_fast8_t dayOfmonth;
    uint_fast8_t month;
    uint_fast16_t year;
} RTC_C_Calendar;

//*****************************************************************************
//
//The following are values that can be passed to RTC_setCalibrationData()
//
//*****************************************************************************
#define RTC_C_CALIBRATIONFREQ_OFF   (RTC_C_CTL13_CALF_0)
#define RTC_C_CALIBRATIONFREQ_512HZ (RTC_C_CTL13_CALF_1)
#define RTC_C_CALIBRATIONFREQ_256HZ (RTC_C_CTL13_CALF_2)
#define RTC_C_CALIBRATIONFREQ_1HZ   (RTC_C_CTL13_CALF_3)

//*****************************************************************************
//
//The following are values that can be passed to RTC_setCalibrationData()
//
//*****************************************************************************
#define RTC_C_CALIBRATION_DOWN1PPM  ( !(RTC_C_OCAL_OCALS) )
#define RTC_C_CALIBRATION_UP1PPM    (RTC_C_OCAL_OCALS)

//*****************************************************************************
//
//The following are values that can be passed to
//RTC_setTemperatureCompensation()
//
//*****************************************************************************
#define RTC_C_COMPENSATION_DOWN1PPM  ( !(RTC_C_TCMP_TCMPS) )
#define RTC_C_COMPENSATION_UP1PPM    (RTC_C_TCMP_TCMPS)

//*****************************************************************************
//
//The following are values that can be passed to RTC_iniRTC_Calendar()
//
//*****************************************************************************
#define RTC_C_FORMAT_BINARY  ( !(RTC_C_CTL13_BCD) )
#define RTC_C_FORMAT_BCD     (RTC_C_CTL13_BCD)

//*****************************************************************************
//
//The following is a value that can be passed to RTC_seRTC_CalendarAlarm()
//
//*****************************************************************************
#define RTC_C_ALARMCONDITION_OFF  (0x80)

//*****************************************************************************
//
//The following are values that can be passed to RTC_seRTC_CalendarEvent()
//in the eventSelect parameter.
//
//*****************************************************************************
#define RTC_C_CALENDAREVENT_MINUTECHANGE  (RTC_C_CTL13_TEV_0)
#define RTC_C_CALENDAREVENT_HOURCHANGE    (RTC_C_CTL13_TEV_1)
#define RTC_C_CALENDAREVENT_NOON          (RTC_C_CTL13_TEV_2)
#define RTC_C_CALENDAREVENT_MIDNIGHT      (RTC_C_CTL13_TEV_3)

//*****************************************************************************
//
//The following are values that can be passed to RTC_definePrescaleEvent()
//
//*****************************************************************************
#define RTC_C_PRESCALE_0  (0x0)
#define RTC_C_PRESCALE_1  (0x1)

//*****************************************************************************
//
//The following are values that can be passed to RTC_definePrescaleEvent()
//in the prescaleEventDivider parameter.
//
//*****************************************************************************
#define RTC_C_PSEVENTDIVIDER_2   (RTC_C_PS0CTL_RT0IP_0)
#define RTC_C_PSEVENTDIVIDER_4   (RTC_C_PS0CTL_RT0IP_1)
#define RTC_C_PSEVENTDIVIDER_8   (RTC_C_PS0CTL_RT0IP_2)
#define RTC_C_PSEVENTDIVIDER_16  (RTC_C_PS0CTL_RT0IP_3)
#define RTC_C_PSEVENTDIVIDER_32  (RTC_C_PS0CTL_RT0IP_4)
#define RTC_C_PSEVENTDIVIDER_64  (RTC_C_PS0CTL_RT0IP_5)
#define RTC_C_PSEVENTDIVIDER_128 (RTC_C_PS0CTL_RT0IP_6)
#define RTC_C_PSEVENTDIVIDER_256 (RTC_C_PS0CTL_RT0IP_7)

//*****************************************************************************
//
//The following are values that can be passed to the interrupt functions
//
//*****************************************************************************
#define RTC_C_OSCILLATOR_FAULT_INTERRUPT  RTC_C_CTL0_OFIE
#define RTC_C_TIME_EVENT_INTERRUPT        RTC_C_CTL0_TEVIE
#define RTC_C_CLOCK_ALARM_INTERRUPT       RTC_C_CTL0_AIE
#define RTC_C_CLOCK_READ_READY_INTERRUPT  RTC_C_CTL0_RDYIE
#define RTC_C_PRESCALE_TIMER0_INTERRUPT   0x02
#define RTC_C_PRESCALE_TIMER1_INTERRUPT   0x01

//*****************************************************************************
//
//! Starts the RTC.
//!
//! This function clears the RTC main hold bit to allow the RTC to function.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_startClock(void);

//*****************************************************************************
//
//! Holds the RTC.
//!
//! This function sets the RTC main hold bit to disable RTC functionality.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_holdClock(void);

//*****************************************************************************
//
//! Allows and Sets the frequency output to RTCLK pin for calibration
//! measurement.
//!
//! \param frequencySelect is the frequency output to RTCLK.
//!        Valid values are
//!        - \b   RTC_C_CALIBRATIONFREQ_OFF - turn off calibration
//!                                                 output [Default]
//!        - \b   RTC_C_CALIBRATIONFREQ_512HZ - output signal at 512Hz
//!                                                     for calibration
//!        - \b   RTC_C_CALIBRATIONFREQ_256HZ - output signal at 256Hz
//!                                                     for calibration
//!        - \b   RTC_C_CALIBRATIONFREQ_1HZ - output signal at 1Hz
//!                                                     for calibration
//!
//! This function sets a frequency to measure at the RTCLK output pin. After
//! testing the set frequency, the calibration could be set accordingly.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_setCalibrationFrequency(uint_fast16_t frequencySelect);

//*****************************************************************************
//
//! Sets the specified calibration for the RTC.
//!
//! \param offsetDirection is the direction that the calibration offset will
//!        go. Valid values are
//!        - \b   RTC_C_CALIBRATION_DOWN1PPM - calibrate at steps of -1
//!        - \b   RTC_C_CALIBRATION_UP1PPM - calibrat at steps of +1
//! \param offsetValue is the value that the offset will be a factor of; a
//!       valid value is any integer from 1-240.
//!
//! This function sets the calibration offset to make the RTC as accurate as
//! possible. The offsetDirection can be either +1-ppm or -1-ppm, and the
//! offsetValue should be from 1-240 and is multiplied by the direction setting
//! (i.e. +1-ppm * 8 (offsetValue) = +8-ppm).
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_setCalibrationData(uint_fast8_t offsetDirection,
        uint_fast8_t offsetValue);

//*****************************************************************************
//
//! Sets the specified temperature compensation for the RTC.
//!
//! \param offsetDirection is the direction that the calibration offset will
//!        go. Valid values are
//!        - \b   RTC_C_COMPENSATION_DOWN1PPM - calibrate at steps of -1
//!        - \b   RTC_C_COMPENSATION_UP1PPM - calibrate at steps of +1
//! \param offsetValue is the value that the offset will be a factor of; a
//!       value is any integer from 1-240.
//!
//! This function sets the calibration offset to make the RTC as accurate as
//! possible. The offsetDirection can be either +1-ppm or -1-ppm, and the
//! offsetValue should be from 1-240 and is multiplied by the direction setting
//! (i.e. +1-ppm * 8 (offsetValue) = +8-ppm).
//!
//! \return true if calibration was set, false if it could not be set
//!
//
//*****************************************************************************
extern bool RTC_C_setTemperatureCompensation(uint_fast16_t offsetDirection,
        uint_fast8_t offsetValue);

//*****************************************************************************
//
//! Initializes the settings to operate the RTC in Calendar mode.
//!
//! \param calendarTime is the structure containing the values for the Calendar
//!       to be initialized to.
//!        Valid values should be of type Calendar and should contain the
//!        following members and corresponding values:
//!        - \b   seconds between 0-59
//!        - \b   minutes between 0-59
//!        - \b   hours between 0-23
//!        - \b   dayOfWeek between 0-6
//!        - \b   dayOfmonth between 1-31
//!        - \b   month between 1-12
//!        - \b   year between 0-4095
//!        \note Values beyond the ones specified may result in erratic behavior.
//! \param formatSelect is the format for the Calendar registers to use.
//!        Valid values are
//!        - \b   RTC_C_FORMAT_BINARY [Default]
//!        - \b   RTC_C_FORMAT_BCD
//!
//! This function initializes the Calendar mode of the RTC module.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_initCalendar(const RTC_C_Calendar *calendarTime,
        uint_fast16_t formatSelect);

//*****************************************************************************
//
//! Returns the Calendar Time stored in the Calendar registers of the RTC.
//!
//!
//! This function returns the current Calendar time in the form of a Calendar
//! structure.
//!
//! \return A Calendar structure containing the current time.
//
//*****************************************************************************
extern RTC_C_Calendar RTC_C_getCalendarTime(void);

//*****************************************************************************
//
//! Sets and Enables the desired Calendar Alarm settings.
//!
//! \param minutesAlarm is the alarm condition for the minutes.
//!        Valid values are
//!        - An integer between 0-59, OR
//!        - \b   RTC_C_ALARMCONDITION_OFF [Default]
//! \param hoursAlarm is the alarm condition for the hours.
//!        Valid values are
//!        - An integer between 0-24, OR
//!        - \b   RTC_C_ALARMCONDITION_OFF [Default]
//! \param dayOfWeekAlarm is the alarm condition for the day of week.
//!        Valid values are
//!        - An integer between 0-6, OR
//!        - \b   RTC_C_ALARMCONDITION_OFF [Default]
//! \param dayOfmonthAlarm is the alarm condition for the day of the month.
//!        Valid values are
//!        - An integer between 0-31, OR
//!        - \b   RTC_C_ALARMCONDITION_OFF [Default]
//!
//! This function sets a Calendar interrupt condition to assert the RTCAIFG
//! interrupt flag. The condition is a logical and of all of the parameters.
//! For example if the minutes and hours alarm is set, then the interrupt will
//! only assert when the minutes AND the hours change to the specified setting.
//! Use the RTC_ALARM_OFF for any alarm settings that should not be apart of
//! the alarm condition.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_setCalendarAlarm(uint_fast8_t minutesAlarm,
        uint_fast8_t hoursAlarm, uint_fast8_t dayOfWeekAlarm,
        uint_fast8_t dayOfmonthAlarm);

//*****************************************************************************
//
//! Sets a single specified Calendar interrupt condition.
//!
//! \param eventSelect is the condition selected.
//!        Valid values are
//!        - \b   RTC_C_CALENDAREVENT_MINUTECHANGE - assert interrupt on every
//!             minute
//!        - \b   RTC_C_CALENDAREVENT_HOURCHANGE - assert interrupt on every hour
//!        - \b   RTC_C_CALENDAREVENT_NOON - assert interrupt when hour is 12
//!        - \b   RTC_C_CALENDAREVENT_MIDNIGHT - assert interrupt when hour is 0
//!
//! This function sets a specified event to assert the RTCTEVIFG interrupt. This
//! interrupt is independent from the Calendar alarm interrupt.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_setCalendarEvent(uint_fast16_t eventSelect);

//*****************************************************************************
//
//! Sets up an interrupt condition for the selected Prescaler.
//!
//! \param prescaleSelect is the prescaler to define an interrupt for.
//!        Valid values are
//!        - \b   RTC_C_PRESCALE_0
//!        - \b   RTC_C_PRESCALE_1
//! \param prescaleEventDivider is a divider to specify when an interrupt can
//!       occur based on the clock source of the selected prescaler.
//!       (Does not affect timer of the selected prescaler).
//!       Valid values are
//!       - \b   RTC_C_PSEVENTDIVIDER_2 [Default]
//!       - \b   RTC_C_PSEVENTDIVIDER_4
//!       - \b   RTC_C_PSEVENTDIVIDER_8
//!       - \b   RTC_C_PSEVENTDIVIDER_16
//!       - \b   RTC_C_PSEVENTDIVIDER_32
//!       - \b   RTC_C_PSEVENTDIVIDER_64
//!       - \b   RTC_C_PSEVENTDIVIDER_128
//!       - \b   RTC_C_PSEVENTDIVIDER_256
//!
//! This function sets the condition for an interrupt to assert based on the
//! individual prescalers.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_definePrescaleEvent(uint_fast8_t prescaleSelect,
        uint_fast8_t prescaleEventDivider);

//*****************************************************************************
//
//! Returns the selected Prescaler value.
//!
//! \param prescaleSelect is the prescaler to obtain the value of.
//!        Valid values are
//!        - \b   RTC_C_PRESCALE_0
//!        - \b   RTC_C_PRESCALE_1
//!
//! This function returns the value of the selected prescale counter register.
//! The counter should be held before reading. If in counter mode, the
//! individual prescaler can be held, while in Calendar mode the whole RTC must
//! be held.
//!
//! \return The value of the specified Prescaler count register
//
//*****************************************************************************
extern uint_fast8_t RTC_C_getPrescaleValue(uint_fast8_t prescaleSelect);

//*****************************************************************************
//
//! Sets the selected Prescaler value.
//!
//! \param prescaleSelect is the prescaler to set the value for.
//!        Valid values are
//!        - \b   RTC_C_PRESCALE_0
//!        - \b   RTC_C_PRESCALE_1
//! \param prescaleCounterValue is the specified value to set the prescaler to;
//!       a valid value is any integer from 0-255.
//!
//! This function sets the prescale counter value. Before setting the prescale
//! counter, it should be held.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_setPrescaleValue(uint_fast8_t prescaleSelect,
        uint_fast8_t prescaleCounterValue);

//*****************************************************************************
//
//! Returns the given BCD value in Binary Format
//!
//! \param valueToConvert is the raw value in BCD format to convert to
//!        Binary.
//!
//! This function converts BCD values to Binary format.
//!
//! \return The Binary version of the valueToConvert parameter.
//
//*****************************************************************************
extern uint16_t RTC_C_convertBCDToBinary(uint16_t valueToConvert);

//*****************************************************************************
//
//! Returns the given Binary value in BCD Format
//!

//! \param valueToConvert is the raw value in Binary format to convert to
//!        BCD.
//!
//! This function converts Binary values to BCD format.
//!
//! \return The BCD version of the valueToConvert parameter.
//
//*****************************************************************************
extern uint16_t RTC_C_convertBinaryToBCD(uint16_t valueToConvert);

//*****************************************************************************
//
//! Enables selected RTC interrupt sources.
//!
//! \param interruptMask is a bit mask of the interrupts to enable.
//!        Mask Value is the logical OR of any of the following
//!        - \b  RTC_C_TIME_EVENT_INTERRUPT - asserts when counter overflows in
//!             counter mode or when Calendar event condition defined by
//!             setCalendarEvent() is met.
//!        - \b  RTC_C_CLOCK_ALARM_INTERRUPT - asserts when alarm condition in
//!             Calendar mode is met.
//!        - \b  RTC_C_CLOCK_READ_READY_INTERRUPT - asserts when Calendar
//!             registers are settled.
//!        - \b  RTC_C_PRESCALE_TIMER0_INTERRUPT - asserts when Prescaler 0
//!             event condition is met.
//!        - \b  RTC_C_PRESCALE_TIMER1_INTERRUPT - asserts when Prescaler 1
//!             event condition is met.
//!        - \b  RTC_C_OSCILLATOR_FAULT_INTERRUPT - asserts if there is
//!             a problem with the 32kHz oscillator, while the RTC is running.
//!
//! This function enables the selected RTC interrupt source.  Only the sources
//! that are enabled can be reflected to the processor interrupt; disabled
//! sources have no effect on the processor.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_enableInterrupt(uint8_t interruptMask);

//*****************************************************************************
//
//! Disables selected RTC interrupt sources.
//!
//! \param interruptMask is a bit mask of the interrupts to disable.
//!        Mask Value is the logical OR of any of the following
//!        - \b  RTC_C_TIME_EVENT_INTERRUPT - asserts when counter overflows in
//!             counter mode or when Calendar event condition defined by
//!             setCalendarEvent() is met.
//!        - \b  RTC_C_CLOCK_ALARM_INTERRUPT - asserts when alarm condition in
//!             Calendar mode is met.
//!        - \b  RTC_CLOCK_READ_READY_INTERRUPT - asserts when Calendar
//!             registers are settled.
//!        - \b  RTC_C_PRESCALE_TIMER0_INTERRUPT - asserts when Prescaler 0
//!             event condition is met.
//!        - \b  RTC_C_PRESCALE_TIMER1_INTERRUPT - asserts when Prescaler 1
//!             event condition is met.
//!        - \b  RTC_C_OSCILLATOR_FAULT_INTERRUPT - asserts if there is a
//!             problem with the 32kHz oscillator, while the RTC is running.
//!
//! This function disables the selected RTC interrupt source.  Only the sources
//! that are enabled can be reflected to the processor interrupt; disabled
//! sources have no effect on the processor.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_disableInterrupt(uint8_t interruptMask);

//*****************************************************************************
//
//! Returns the status of the interrupts flags.
//!
//! \return A bit mask of the selected interrupt flag's status.
//!        Mask Value is the logical OR of any of the following
//!        - \b  RTC_C_TIME_EVENT_INTERRUPT - asserts when counter overflows in
//!             counter mode or when Calendar event condition defined by
//!             setCalendarEvent() is met.
//!        - \b  RTC_C_CLOCK_ALARM_INTERRUPT - asserts when alarm condition in
//!             Calendar mode is met.
//!        - \b  RTC_C_CLOCK_READ_READY_INTERRUPT - asserts when Calendar
//!             registers are settled.
//!        - \b  RTC_C_PRESCALE_TIMER0_INTERRUPT - asserts when Prescaler 0
//!             event condition is met.
//!        - \b  RTC_C_PRESCALE_TIMER1_INTERRUPT - asserts when Prescaler 1
//!             event condition is met.
//!        - \b  RTC_C_OSCILLATOR_FAULT_INTERRUPT - asserts if there is a
//!             problem with the 32kHz oscillator, while the RTC is running.
//
//*****************************************************************************
extern uint_fast8_t RTC_C_getInterruptStatus(void);

//*****************************************************************************
//
//! Returns the status of the interrupts flags masked with the enabled
//! interrupts.  This function is useful to call in ISRs to get a
//! list of pending interrupts that are actually enabled and could have caused
//! the ISR.
//!
//! \return A bit mask of the selected interrupt flag's status.
//!        Mask Value is the logical OR of any of the following
//!        - \b  RTC_TIME_EVENT_INTERRUPT - asserts when counter overflows in
//!             counter mode or when Calendar event condition defined by
//!             setCalendarEvent() is met.
//!        - \b  RTC_CLOCK_ALARM_INTERRUPT - asserts when alarm condition in
//!             Calendar mode is met.
//!        - \b  RTC_CLOCK_READ_READY_INTERRUPT - asserts when Calendar
//!             registers are settled.
//!        - \b  RTC_C_PRESCALE_TIMER0_INTERRUPT - asserts when Prescaler 0
//!             event condition is met.
//!        - \b  RTC_C_PRESCALE_TIMER1_INTERRUPT - asserts when Prescaler 1
//!             event condition is met.
//!        - \b  RTC_OSCILLATOR_FAULT_INTERRUPT - asserts if there is a problem
//!             with the 32kHz oscillator, while the RTC is running.
//
//*****************************************************************************
extern uint_fast8_t RTC_C_getEnabledInterruptStatus(void);

//*****************************************************************************
//
//! Clears selected RTC interrupt flags.
//!
//! \param interruptFlagMask is a bit mask of the interrupt flags to be
//!        cleared. Mask Value is the logical OR of any of the following
//!        - \b  RTC_C_TIME_EVENT_INTERRUPT - asserts when counter overflows in
//!             counter mode or when Calendar event condition defined by
//!             setCalendarEvent() is met.
//!        - \b  RTC_C_CLOCK_ALARM_INTERRUPT - asserts when alarm condition in
//!             Calendar mode is met.
//!        - \b  RTC_C_CLOCK_READ_READY_INTERRUPT - asserts when Calendar
//!             registers are settled.
//!        - \b  RTC_C_PRESCALE_TIMER0_INTERRUPT - asserts when Prescaler 0
//!             event condition is met.
//!        - \b  RTC_C_PRESCALE_TIMER1_INTERRUPT - asserts when Prescaler 1
//!             event condition is met.
//!        - \b  RTC_C_OSCILLATOR_FAULT_INTERRUPT - asserts if there is
//!             a problem with the 32kHz oscillator, while the RTC is running.
//!
//! This function clears the RTC interrupt flag is cleared, so that it no longer
//! asserts.
//!
//! \return None
//
//*****************************************************************************
extern void RTC_C_clearInterruptFlag(uint_fast8_t interruptFlagMask);

//*****************************************************************************
//
//! Registers an interrupt handler for the RTC interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the
//! RTC interrupt occurs.
//!
//! This function registers the handler to be called when a RTC
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific AES interrupts must be enabled
//! via RTC_enableInterrupt().  It is the interrupt handler's responsibility to
//! clear the interrupt source via RTC_clearInterruptFlag().
//!
//! \return None.
//
//*****************************************************************************
extern void RTC_C_registerInterrupt(void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the RTC interrupt
//!
//! This function unregisters the handler to be called when RTC
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void RTC_C_unregisterInterrupt(void);

/* Defines for future devices that might have multiple instances */
#define RTC_C_startClockMultipleInstance(a) RTC_C_startClock()
#define RTC_C_holdClockMultipleInstance(a) RTC_C_holdClock()
#define RTC_C_setCalibrationFrequencyMultipleInstance(a,b) RTC_C_setCalibrationFrequency(b)
#define RTC_C_setCalibrationDataMultipleInstance(a,b,c) RTC_C_setCalibrationData(b,c)
#define RTC_C_setTemperatureCompensationMultipleInstance(a,b,c) RTC_C_setTemperatureCompensation(b,c)
#define RTC_C_initCalendarMultipleInstance(a,b,c) RTC_C_initCalendar(b,c)
#define RTC_C_getCalendarTimeMultipleInstance(a) RTC_C_getCalendarTime()
#define RTC_C_setCalendarAlarmMultipleInstance(a,b,c,d,e) RTC_C_setCalendarAlarm(b,c,d,e)
#define RTC_C_setCalendarEventMultipleInstance(a,b) RTC_C_setCalendarEvent(b)
#define RTC_C_definePrescaleEventMultipleInstance(a,b,c) RTC_C_definePrescaleEvent(b,c)
#define RTC_C_getPrescaleValueMultipleInstance(a,b) RTC_C_getPrescaleValue(b)
#define RTC_C_setPrescaleValueMultipleInstance(a,b,c) RTC_C_setPrescaleValue(b,c)
#define RTC_C_convertBCDToBinaryMultipleInstance(a,b) RTC_C_convertBCDToBinary(b)
#define RTC_C_convertBinaryToBCDMultipleInstance(a,b) RTC_C_convertBinaryToBCD(b)
#define RTC_C_enableInterruptMultipleInstance(a,b) RTC_C_enableInterrupt(b)
#define RTC_C_disableInterruptMultipleInstance(a,b) RTC_C_disableInterrupt(b)
#define RTC_C_getInterruptStatusMultipleInstance(a) RTC_C_getInterruptStatus()
#define RTC_C_getEnabledInterruptStatusMultipleInstance(a) RTC_C_getEnabledInterruptStatus()
#define RTC_C_clearInterruptFlagMultipleInstance(a,b) RTC_C_clearInterruptFlag(b)
#define RTC_C_registerInterruptMultipleInstance(a,b) RTC_C_registerInterrupt(b)
#define RTC_C_unregisterInterruptMultipleInstance(a) RTC_C_unregisterInterrupt()

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

#endif /* RTC_H */
