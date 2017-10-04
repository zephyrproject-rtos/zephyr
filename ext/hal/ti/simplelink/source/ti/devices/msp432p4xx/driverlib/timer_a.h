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
#ifndef TIMERA_H_
#define TIMERA_H_

//*****************************************************************************
//
//! \addtogroup timera_api
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
// Timer_A Specific Parameters
//
//*****************************************************************************
#define TIMER_A_CMSIS(x) ((Timer_A_Type *) x)

#define TIMER_A_CCR0_INTERRUPT              0x00
#define TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT 0x01

//*****************************************************************************
//
//!     ypedef Timer_A_ContinuousModeConfig
//! \brief Type definition for \link _Timer_A_ContinuousModeConfig \endlink
//!     structure
//!
//! \struct _Timer_A_ContinuousModeConfig
//! \brief Configuration structure for continuous mode in the \b Timer_A module.
//!         See \link Timer_A_configureContinuousMode \endlink for parameter
//!                 documentation.
//
//*****************************************************************************
typedef struct _Timer_A_ContinuousModeConfig
{
    uint_fast16_t clockSource;
    uint_fast16_t clockSourceDivider;
    uint_fast16_t timerInterruptEnable_TAIE;
    uint_fast16_t timerClear;
} Timer_A_ContinuousModeConfig;

//*****************************************************************************
//
//!     ypedef Timer_A_UpModeConfig
//! \brief Type definition for \link _Timer_A_UpModeConfig \endlink
//!     structure
//!
//! \struct _Timer_A_UpModeConfig
//! \brief Configuration structure for Up mode in the \b Timer_A module. See
//!          \link Timer_A_configureUpMode \endlink for parameter
//!                 documentation.
//
//*****************************************************************************
typedef struct _Timer_A_UpModeConfig
{
    uint_fast16_t clockSource;
    uint_fast16_t clockSourceDivider;
    uint_fast16_t timerPeriod;
    uint_fast16_t timerInterruptEnable_TAIE;
    uint_fast16_t captureCompareInterruptEnable_CCR0_CCIE;
    uint_fast16_t timerClear;
} Timer_A_UpModeConfig;

//*****************************************************************************
//
//!     ypedef Timer_A_UpDownModeConfig
//! \brief Type definition for \link _Timer_A_UpDownModeConfig \endlink
//!     structure
//!
//! \struct _Timer_A_UpDownModeConfig
//! \brief Configuration structure for UpDown mode in the \b Timer_A module. See
//!          \link Timer_A_configureUpDownMode \endlink for parameter
//!                 documentation.
//
//*****************************************************************************
typedef struct _Timer_A_UpDownModeConfig
{
    uint_fast16_t clockSource;
    uint_fast16_t clockSourceDivider;
    uint_fast16_t timerPeriod;
    uint_fast16_t timerInterruptEnable_TAIE;
    uint_fast16_t captureCompareInterruptEnable_CCR0_CCIE;
    uint_fast16_t timerClear;
} Timer_A_UpDownModeConfig;

//*****************************************************************************
//
//!     ypedef Timer_A_CaptureModeConfig
//! \brief Type definition for \link _Timer_A_CaptureModeConfig \endlink
//!     structure
//!
//! \struct _Timer_A_CaptureModeConfig
//! \brief Configuration structure for capture mode in the \b Timer_A module.
//!         See  \link Timer_A_initCapture \endlink for parameter
//!                 documentation.
//
//*****************************************************************************
typedef struct _Timer_A_CaptureModeConfig
{
    uint_fast16_t captureRegister;
    uint_fast16_t captureMode;
    uint_fast16_t captureInputSelect;
    uint_fast16_t synchronizeCaptureSource;
    uint_fast8_t captureInterruptEnable;
    uint_fast16_t captureOutputMode;
} Timer_A_CaptureModeConfig;

//*****************************************************************************
//
//!     ypedef Timer_A_CompareModeConfig
//! \brief Type definition for \link _Timer_A_CompareModeConfig \endlink
//!     structure
//!
//! \struct _Timer_A_CompareModeConfig
//! \brief Configuration structure for compare mode in the \b Timer_A module.
//!          See \link Timer_A_initCompare \endlink for parameter
//!                 documentation.
//
//*****************************************************************************
typedef struct _Timer_A_CompareModeConfig
{
    uint_fast16_t compareRegister;
    uint_fast16_t compareInterruptEnable;
    uint_fast16_t compareOutputMode;
    uint_fast16_t compareValue;
} Timer_A_CompareModeConfig;

//*****************************************************************************
//
//!     ypedef Timer_A_PWMConfig
//! \brief Type definition for \link _Timer_A_PWMConfig \endlink
//!     structure
//!
//! \struct _Timer_A_PWMConfig
//! \brief Configuration structure for PWM mode in the \b Timer_A module. See
//!          \link Timer_A_generatePWM \endlink for parameter
//!                 documentation.
//
//*****************************************************************************
typedef struct _Timer_A_PWMConfig
{
    uint_fast16_t clockSource;
    uint_fast16_t clockSourceDivider;
    uint_fast16_t timerPeriod;
    uint_fast16_t compareRegister;
    uint_fast16_t compareOutputMode;
    uint_fast16_t dutyCycle;
} Timer_A_PWMConfig;


//*****************************************************************************
//
// The following is a parameter determines the maximum difference in counts of
// the TAxR register for a majority vote
//
//*****************************************************************************
#define TIMER_A_THRESHOLD                                                     50

//*****************************************************************************
//
// The following are values that can be passed to the clockSourceDivider
// parameter
//
//*****************************************************************************
#define TIMER_A_CLOCKSOURCE_DIVIDER_1                                      0x01
#define TIMER_A_CLOCKSOURCE_DIVIDER_2                                      0x02
#define TIMER_A_CLOCKSOURCE_DIVIDER_4                                      0x04
#define TIMER_A_CLOCKSOURCE_DIVIDER_8                                      0x08
#define TIMER_A_CLOCKSOURCE_DIVIDER_3                                      0x03
#define TIMER_A_CLOCKSOURCE_DIVIDER_5                                      0x05
#define TIMER_A_CLOCKSOURCE_DIVIDER_6                                      0x06
#define TIMER_A_CLOCKSOURCE_DIVIDER_7                                      0x07
#define TIMER_A_CLOCKSOURCE_DIVIDER_10                                     0x0A
#define TIMER_A_CLOCKSOURCE_DIVIDER_12                                     0x0C
#define TIMER_A_CLOCKSOURCE_DIVIDER_14                                     0x0E
#define TIMER_A_CLOCKSOURCE_DIVIDER_16                                     0x10
#define TIMER_A_CLOCKSOURCE_DIVIDER_20                                     0x14
#define TIMER_A_CLOCKSOURCE_DIVIDER_24                                     0x18
#define TIMER_A_CLOCKSOURCE_DIVIDER_28                                     0x1C
#define TIMER_A_CLOCKSOURCE_DIVIDER_32                                     0x20
#define TIMER_A_CLOCKSOURCE_DIVIDER_40                                     0x28
#define TIMER_A_CLOCKSOURCE_DIVIDER_48                                     0x30
#define TIMER_A_CLOCKSOURCE_DIVIDER_56                                     0x38
#define TIMER_A_CLOCKSOURCE_DIVIDER_64                                     0x40

//*****************************************************************************
//
// The following are values that can be passed to the timerMode parameter
//
//*****************************************************************************
#define TIMER_A_STOP_MODE                                                  TIMER_A_CTL_MC_0
#define TIMER_A_UP_MODE                                                    TIMER_A_CTL_MC_1
#define TIMER_A_CONTINUOUS_MODE                                            TIMER_A_CTL_MC_2
#define TIMER_A_UPDOWN_MODE                                                TIMER_A_CTL_MC_3

//*****************************************************************************
//
// The following are values that can be passed to the timerClear parameter
//
//*****************************************************************************
#define TIMER_A_DO_CLEAR                                                  TIMER_A_CTL_CLR
#define TIMER_A_SKIP_CLEAR                                                 0x00

//*****************************************************************************
//
// The following are values that can be passed to the clockSource parameter
//
//*****************************************************************************
#define TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK                        TIMER_A_CTL_SSEL__TACLK
#define TIMER_A_CLOCKSOURCE_ACLK                                  TIMER_A_CTL_SSEL__ACLK
#define TIMER_A_CLOCKSOURCE_SMCLK                                 TIMER_A_CTL_SSEL__SMCLK
#define TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK               TIMER_A_CTL_SSEL__INCLK

//*****************************************************************************
//
// The following are values that can be passed to the timerInterruptEnable_TAIE
// parameter
//
//*****************************************************************************
#define TIMER_A_TAIE_INTERRUPT_ENABLE                                      TIMER_A_CTL_IE
#define TIMER_A_TAIE_INTERRUPT_DISABLE                                     0x00

//*****************************************************************************
//
// The following are values that can be passed to the
// captureCompareInterruptEnable_CCR0_CCIE parameter
//
//*****************************************************************************
#define TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE                                 TIMER_A_CCTLN_CCIE
#define TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE                                0x00

//*****************************************************************************
//
// The following are values that can be passed to the captureInterruptEnable
// parameter
//
//*****************************************************************************
#define TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE                           0x00
#define TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE                            TIMER_A_CCTLN_CCIE

//*****************************************************************************
//
// The following are values that can be passed to the captureInputSelect
// parameter
//
//*****************************************************************************
#define TIMER_A_CAPTURE_INPUTSELECT_CCIxA                                TIMER_A_CCTLN_CCIS_0
#define TIMER_A_CAPTURE_INPUTSELECT_CCIxB                                TIMER_A_CCTLN_CCIS_1
#define TIMER_A_CAPTURE_INPUTSELECT_GND                                  TIMER_A_CCTLN_CCIS_2
#define TIMER_A_CAPTURE_INPUTSELECT_Vcc                                  TIMER_A_CCTLN_CCIS_3

//*****************************************************************************
//
// The following are values that can be passed to the compareOutputMode
// parameter
//
//*****************************************************************************
#define TIMER_A_OUTPUTMODE_OUTBITVALUE                                 TIMER_A_CCTLN_OUTMOD_0
#define TIMER_A_OUTPUTMODE_SET                                         TIMER_A_CCTLN_OUTMOD_1
#define TIMER_A_OUTPUTMODE_TOGGLE_RESET                                TIMER_A_CCTLN_OUTMOD_2
#define TIMER_A_OUTPUTMODE_SET_RESET                                   TIMER_A_CCTLN_OUTMOD_3
#define TIMER_A_OUTPUTMODE_TOGGLE                                      TIMER_A_CCTLN_OUTMOD_4
#define TIMER_A_OUTPUTMODE_RESET                                       TIMER_A_CCTLN_OUTMOD_5
#define TIMER_A_OUTPUTMODE_TOGGLE_SET                                  TIMER_A_CCTLN_OUTMOD_6
#define TIMER_A_OUTPUTMODE_RESET_SET                                   TIMER_A_CCTLN_OUTMOD_7

//*****************************************************************************
//
// The following are values that can be passed to the compareRegister parameter
//
//*****************************************************************************
#define TIMER_A_CAPTURECOMPARE_REGISTER_0                                  0x02
#define TIMER_A_CAPTURECOMPARE_REGISTER_1                                  0x04
#define TIMER_A_CAPTURECOMPARE_REGISTER_2                                  0x06
#define TIMER_A_CAPTURECOMPARE_REGISTER_3                                  0x08
#define TIMER_A_CAPTURECOMPARE_REGISTER_4                                  0x0A
#define TIMER_A_CAPTURECOMPARE_REGISTER_5                                  0x0C
#define TIMER_A_CAPTURECOMPARE_REGISTER_6                                  0x0E

//*****************************************************************************
//
// The following are values that can be passed to the captureMode parameter
//
//*****************************************************************************
#define TIMER_A_CAPTUREMODE_NO_CAPTURE                                     TIMER_A_CCTLN_CM_0
#define TIMER_A_CAPTUREMODE_RISING_EDGE                                    TIMER_A_CCTLN_CM_1
#define TIMER_A_CAPTUREMODE_FALLING_EDGE                                   TIMER_A_CCTLN_CM_2
#define TIMER_A_CAPTUREMODE_RISING_AND_FALLING_EDGE                        TIMER_A_CCTLN_CM_3

//*****************************************************************************
//
// The following are values that can be passed to the synchronizeCaptureSource
// parameter
//
//*****************************************************************************
#define TIMER_A_CAPTURE_ASYNCHRONOUS                                       0x00
#define TIMER_A_CAPTURE_SYNCHRONOUS                                        TIMER_A_CCTLN_SCS

//*****************************************************************************
//
// The following are values that can be passed to the mask parameter
//
//*****************************************************************************
#define TIMER_A_CAPTURE_OVERFLOW                                          TIMER_A_CCTLN_COV
#define TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG                             TIMER_A_CCTLN_CCIFG

//*****************************************************************************
//
// The following are values that can be passed to the synchronized parameter
//
//*****************************************************************************
#define TIMER_A_READ_SYNCHRONIZED_CAPTURECOMPAREINPUT                      TIMER_A_CCTLN_SCCI
#define TIMER_A_READ_CAPTURE_COMPARE_INPUT                                 TIMER_A_CCTLN_CCI


#define TIMER_A_CAPTURECOMPARE_INPUT_HIGH                                  0x01
#define TIMER_A_CAPTURECOMPARE_INPUT_LOW                                   0x00

//*****************************************************************************
//
// The following are values that can be passed to the outputModeOutBitValue
// parameter
//
//*****************************************************************************
#define TIMER_A_OUTPUTMODE_OUTBITVALUE_HIGH                                TIMER_A_CCTLN_OUT
#define TIMER_A_OUTPUTMODE_OUTBITVALUE_LOW                                 0x00

//*****************************************************************************
//
// The following are values that can be passed toThe following are values that
// can be returned by the interrupt functions
//
//*****************************************************************************
#define TIMER_A_INTERRUPT_NOT_PENDING                                      0x00
#define TIMER_A_INTERRUPT_PENDING                                          0x01


/* Convenience function for setting the PWM Duty Cycle */
#define Timer_A_setDutyCycle(timer,dutyCycle) \
                Timer_A_setCompareValue(timer,dutyCycle)

//*****************************************************************************
//
//Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! Starts Timer_A counter
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param timerMode selects Clock source. Valid values are
//!       - \b TIMER_A_CONTINUOUS_MODE [Default value]
//!       - \b TIMER_A_UPDOWN_MODE
//!       - \b TIMER_A_UP_MODE
//!
//! \note This function assumes that the timer has been previously configured
//! using Timer_A_configureContinuousMode,  Timer_A_configureUpMode or
//! Timer_A_configureUpDownMode.
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_startCounter(uint32_t timer, uint_fast16_t timerMode);

//*****************************************************************************
//
//! Configures Timer_A in continuous mode.
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param config Configuration structure for Timer_A continuous mode
//!
//! <hr>
//! <b>Configuration options for \link Timer_A_ContinuousModeConfig \endlink
//!         structure.</b>
//! <hr>
//!
//! \param clockSource selects Clock source. Valid values are
//!       - \b TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK [Default value]
//!       - \b TIMER_A_CLOCKSOURCE_ACLK
//!       - \b TIMER_A_CLOCKSOURCE_SMCLK
//!       - \b TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
//! \param timerInterruptEnable_TAIE is the divider for Clock source.
//!       Valid values are:
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_1 [Default value]
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_2
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_4
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_8
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_3
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_5
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_6
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_7
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_10
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_12
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_14
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_16
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_20
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_24
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_28
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_32
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_40
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_48
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_56
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_64
//! \param timerInterruptEnable_TAIE is to enable or disable Timer_A
//!        interrupt. Valid values are
//!      - \b TIMER_A_TAIE_INTERRUPT_ENABLE
//!      - \b TIMER_A_TAIE_INTERRUPT_DISABLE [Default value]
//! \param timerClear decides if Timer_A clock divider, count direction,
//!        count need to be reset. Valid values are
//!      - \b TIMER_A_DO_CLEAR
//!      - \b TIMER_A_SKIP_CLEAR [Default value]
//!
//! \note This API does not start the timer. Timer needs to be started when
//! required using the Timer_A_startCounter API.
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_configureContinuousMode(uint32_t timer,
        const Timer_A_ContinuousModeConfig *config);

//*****************************************************************************
//
//! Configures Timer_A in up mode.
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param config Configuration structure for Timer_A Up mode
//!
//! <hr>
//! <b>Configuration options for \link Timer_A_UpModeConfig \endlink
//!         structure.</b>
//! <hr>
//! \param clockSource selects Clock source. Valid values are
//!       - \b TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK [Default value]
//!       - \b TIMER_A_CLOCKSOURCE_ACLK
//!       - \b TIMER_A_CLOCKSOURCE_SMCLK
//!       - \b TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
//! \param clockSourceDivider is the divider for Clock source. Valid values
//!         are:
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_1 [Default value]
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_2
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_4
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_8
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_3
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_5
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_6
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_7
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_10
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_12
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_14
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_16
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_20
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_24
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_28
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_32
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_40
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_48
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_56
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_64
//! \param timerPeriod is the specified Timer_A period. This is the value
//!         that gets written into the CCR0. Limited to 16 bits[uint16_t]
//! \param timerInterruptEnable_TAIE is to enable or disable Timer_A
//!        interrupt. Valid values are:
//!      - \b TIMER_A_TAIE_INTERRUPT_ENABLE and
//!      - \b TIMER_A_TAIE_INTERRUPT_DISABLE [Default value]
//! \param captureCompareInterruptEnable_CCR0_CCIE is to enable or disable
//!         Timer_A CCR0 captureComapre interrupt. Valid values are
//!      - \b TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE and
//!      - \b TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE [Default value]
//! \param timerClear decides if Timer_A clock divider, count direction,
//!        count need to be reset. Valid values are
//!      - \b TIMER_A_DO_CLEAR
//!      - \b TIMER_A_SKIP_CLEAR [Default value]
//!
//!\note This API does not start the timer. Timer needs to be started when
//!required using the Timer_A_startCounter API.
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_configureUpMode(uint32_t timer,
        const Timer_A_UpModeConfig *config);

//*****************************************************************************
//
//! Configures Timer_A in up down mode.
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param config Configuration structure for Timer_A UpDown mode
//!
//! <hr>
//! <b>Configuration options for \link Timer_A_UpDownModeConfig \endlink
//!         structure.</b>
//! <hr>
//! \param clockSource selects Clock source. Valid values are
//!       - \b TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK [Default value]
//!       - \b TIMER_A_CLOCKSOURCE_ACLK
//!       - \b TIMER_A_CLOCKSOURCE_SMCLK
//!       - \b TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
//! \param clockSourceDivider is the divider for Clock source. Valid values
//!         are:
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_1 [Default value]
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_2
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_4
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_8
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_3
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_5
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_6
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_7
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_10
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_12
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_14
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_16
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_20
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_24
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_28
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_32
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_40
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_48
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_56
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_64
//! \param timerPeriod is the specified Timer_A period
//! \param timerInterruptEnable_TAIE is to enable or disable Timer_A
//!         interrupt.
//!        Valid values are
//!      - \b TIMER_A_TAIE_INTERRUPT_ENABLE
//!      - \b TIMER_A_TAIE_INTERRUPT_DISABLE [Default value]
//! \param captureCompareInterruptEnable_CCR0_CCIE is to enable or disable
//!         Timer_A CCR0 captureComapre interrupt. Valid values are
//!      - \b TIMER_A_CCIE_CCR0_INTERRUPT_ENABLE and
//!      - \b TIMER_A_CCIE_CCR0_INTERRUPT_DISABLE [Default value]
//! \param timerClear decides if Timer_A clock divider, count direction, count
//!        need to be reset. Valid values are
//!      - \b TIMER_A_DO_CLEAR
//!      - \b TIMER_A_SKIP_CLEAR [Default value]
//!
//!This API does not start the timer. Timer needs to be started when required
//!using the Timer_A_startCounter API.
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_configureUpDownMode(uint32_t timer,
        const Timer_A_UpDownModeConfig *config);

//*****************************************************************************
//
//! Initializes Capture Mode
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param config Configuration structure for Timer_A capture mode
//!
//! <hr>
//! <b>Configuration options for \link Timer_A_CaptureModeConfig \endlink
//!         structure.</b>
//! <hr>
//! \param captureRegister selects the Capture register being used. Valid
//!     values are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    \n Refer to datasheet to ensure the device has the capture compare
//!    register being used
//! \param captureMode is the capture mode selected. Valid values are
//!      - \b TIMER_A_CAPTUREMODE_NO_CAPTURE [Default value]
//!      - \b TIMER_A_CAPTUREMODE_RISING_EDGE
//!      - \b TIMER_A_CAPTUREMODE_FALLING_EDGE
//!      - \b TIMER_A_CAPTUREMODE_RISING_AND_FALLING_EDGE
//! \param captureInputSelect decides the Input Select
//!      - \b TIMER_A_CAPTURE_INPUTSELECT_CCIxA [Default value]
//!      - \b TIMER_A_CAPTURE_INPUTSELECT_CCIxB
//!      - \b TIMER_A_CAPTURE_INPUTSELECT_GND
//!      - \b TIMER_A_CAPTURE_INPUTSELECT_Vcc
//! \param synchronizeCaptureSource decides if capture source should be
//!         synchronized with timer clock
//!        Valid values are
//!      - \b TIMER_A_CAPTURE_ASYNCHRONOUS [Default value]
//!      - \b TIMER_A_CAPTURE_SYNCHRONOUS
//! \param captureInterruptEnable is to enable or disable
//!         timer captureComapre interrupt. Valid values are
//!      - \b TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE [Default value]
//!      - \b TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE
//! \param captureOutputMode specifies the ouput mode. Valid values are
//!      - \b TIMER_A_OUTPUTMODE_OUTBITVALUE [Default value],
//!      - \b TIMER_A_OUTPUTMODE_SET,
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE_RESET,
//!      - \b TIMER_A_OUTPUTMODE_SET_RESET
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE,
//!      - \b TIMER_A_OUTPUTMODE_RESET,
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE_SET,
//!      - \b TIMER_A_OUTPUTMODE_RESET_SET
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_initCapture(uint32_t timer,
        const Timer_A_CaptureModeConfig *config);

//*****************************************************************************
//
//! Initializes Compare Mode
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param config Configuration structure for Timer_A compare mode
//!
//! <hr>
//! <b>Configuration options for \link Timer_A_CompareModeConfig \endlink
//!         structure.</b>
//! <hr>
//! \param compareRegister selects the Capture register being used. Valid
//!     values are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    \n Refer to datasheet to ensure the device has the capture compare
//!    register being used
//! \param compareInterruptEnable is to enable or disable
//!         timer captureComapre interrupt. Valid values are
//!      - \b TIMER_A_CAPTURECOMPARE_INTERRUPT_ENABLE and
//!      - \b TIMER_A_CAPTURECOMPARE_INTERRUPT_DISABLE [Default value]
//! \param compareOutputMode specifies the output mode. Valid values are
//!      - \b TIMER_A_OUTPUTMODE_OUTBITVALUE [Default value],
//!      - \b TIMER_A_OUTPUTMODE_SET,
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE_RESET,
//!      - \b TIMER_A_OUTPUTMODE_SET_RESET
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE,
//!      - \b TIMER_A_OUTPUTMODE_RESET,
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE_SET,
//!      - \b TIMER_A_OUTPUTMODE_RESET_SET
//! \param compareValue is the count to be compared with in compare mode
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_initCompare(uint32_t timer,
        const Timer_A_CompareModeConfig *config);

//*****************************************************************************
//
//! Reset/Clear the timer clock divider, count direction, count
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \returns None
//
//*****************************************************************************
extern void Timer_A_clearTimer(uint32_t timer);

//*****************************************************************************
//
//! Get synchronized capture compare input
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister selects the Capture register being used.
//!     Valid values are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    \n Refer to datasheet to ensure the device has the capture compare
//!    register being used
//! \param synchronizedSetting is to select type of capture compare input.
//!         Valid values are
//!      - \b TIMER_A_READ_CAPTURE_COMPARE_INPUT
//!      - \b TIMER_A_READ_SYNCHRONIZED_CAPTURECOMPAREINPUT
//!
//! \return \b TIMER_A_CAPTURECOMPARE_INPUT_HIGH or
//!       - \b TIMER_A_CAPTURECOMPARE_INPUT_LOW
//
//*****************************************************************************
extern uint_fast8_t Timer_A_getSynchronizedCaptureCompareInput(uint32_t timer,
        uint_fast16_t captureCompareRegister,
        uint_fast16_t synchronizedSetting);

//*****************************************************************************
//
//! Get ouput bit for output mode
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister selects the Capture register being used.
//!     Valid values are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    \n Refer to datasheet to ensure the device has the capture compare
//!    register being used
//!
//! \return \b TIMER_A_OUTPUTMODE_OUTBITVALUE_HIGH or
//!       - \b TIMER_A_OUTPUTMODE_OUTBITVALUE_LOW
//
//*****************************************************************************
extern uint_fast8_t Timer_A_getOutputForOutputModeOutBitValue(uint32_t timer,
        uint_fast16_t captureCompareRegister);

//*****************************************************************************
//
//! Get current capture compare count
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister selects the Capture register being used.
//!     Valid values are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    \n Refer to datasheet to ensure the device has the capture compare
//!    register being used
//!
//! \return current count as uint16_t
//
//*****************************************************************************
extern uint_fast16_t Timer_A_getCaptureCompareCount(uint32_t timer,
        uint_fast16_t captureCompareRegister);

//*****************************************************************************
//
//! Set ouput bit for output mode
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister selects the Capture register being used.
//!     are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    \n Refer to datasheet to ensure the device has the capture compare
//!    register being used
//! \param outputModeOutBitValue the value to be set for out bit.
//!     Valid values are:
//!                    - \b TIMER_A_OUTPUTMODE_OUTBITVALUE_HIGH
//!                    - \b TIMER_A_OUTPUTMODE_OUTBITVALUE_LOW
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_setOutputForOutputModeOutBitValue(uint32_t timer,
        uint_fast16_t captureCompareRegister,
        uint_fast8_t outputModeOutBitValue);

//*****************************************************************************
//
//! Generate a PWM with timer running in up mode
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param config Configuration structure for Timer_A PWM mode
//!
//! <hr>
//! <b>Configuration options for \link Timer_A_PWMConfig \endlink
//!         structure.</b>
//! <hr>
//! \param clockSource selects Clock source. Valid values are
//!       - \b TIMER_A_CLOCKSOURCE_EXTERNAL_TXCLK
//!       - \b TIMER_A_CLOCKSOURCE_ACLK
//!       - \b TIMER_A_CLOCKSOURCE_SMCLK
//!       - \b TIMER_A_CLOCKSOURCE_INVERTED_EXTERNAL_TXCLK
//! \param clockSourceDivider is the divider for Clock source. Valid values
//!         are
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_1
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_2
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_4
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_8
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_3
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_5
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_6
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_7
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_10
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_12
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_14
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_16
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_20
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_24
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_28
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_32
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_40
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_48
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_56
//!      - \b TIMER_A_CLOCKSOURCE_DIVIDER_64
//! \param timerPeriod selects the desired timer period
//! \param compareRegister selects the compare register being used.
//!     Valid values are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    <br>\n Refer to datasheet to ensure the device has the capture compare
//!    register being used
//! \param compareOutputMode specifies the ouput mode. Valid values are:
//!      - \b TIMER_A_OUTPUTMODE_OUTBITVALUE,
//!      - \b TIMER_A_OUTPUTMODE_SET,
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE_RESET,
//!      - \b TIMER_A_OUTPUTMODE_SET_RESET
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE,
//!      - \b TIMER_A_OUTPUTMODE_RESET,
//!      - \b TIMER_A_OUTPUTMODE_TOGGLE_SET,
//!      - \b TIMER_A_OUTPUTMODE_RESET_SET
//! \param dutyCycle specifies the dutycycle for the generated waveform
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_generatePWM(uint32_t timer,
                                const Timer_A_PWMConfig *config);

//*****************************************************************************
//
//! Stops the timer
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \returns None
//
//*****************************************************************************
extern void Timer_A_stopTimer(uint32_t timer);

//*****************************************************************************
//
//! Sets the value of the capture-compare register
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param compareRegister selects the Capture register being used. Valid
//!     values are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    <br>\n Refer to datasheet to ensure the device has the capture compare
//!     register being used
//! \param compareValue is the count to be compared with in compare mode
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_setCompareValue(uint32_t timer,
        uint_fast16_t compareRegister, uint_fast16_t compareValue);

//*****************************************************************************
//
//! Returns the current value of the specified timer. Note that according to
//! the Timer A user guide, reading the value of the counter is unreliable
//! if the system clock is asynchronous from the timer clock. The API addresses
//! this concern by reading the timer count register twice and then determining
//! the integrity of the value. If the two values are within 10 timer counts
//! of each other, the value is deemed safe and returned. If not, the process
//! is repeated until a reliable timer value is determined.
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \returns The value of the specified timer
//
//*****************************************************************************
extern uint16_t Timer_A_getCounterValue(uint32_t timer);

//*****************************************************************************
//
//! Clears the Timer TAIFG interrupt flag
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_clearInterruptFlag(uint32_t timer);

//*****************************************************************************
//
//! Clears the capture-compare interrupt flag
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister selects the Capture-compare register being
//! used. Valid values are
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_0
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_1
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_2
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_3
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_4
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_5
//!   - \b TIMER_A_CAPTURECOMPARE_REGISTER_6
//!    <br>Refer to the datasheet to ensure the device has the capture compare
//!    register being used
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_clearCaptureCompareInterrupt(uint32_t timer,
        uint_fast16_t captureCompareRegister);

//*****************************************************************************
//
//! Enable timer interrupt
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_enableInterrupt(uint32_t timer);

//*****************************************************************************
//
//! Disable timer interrupt
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_disableInterrupt(uint32_t timer);

//*****************************************************************************
//
//! Get timer interrupt status
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \return uint32_t. Return interrupt status. Valid values are
//!       - \b TIMER_A_INTERRUPT_PENDING
//!       - \b TIMER_A_INTERRUPT_NOT_PENDING
//
//*****************************************************************************
extern uint32_t Timer_A_getInterruptStatus(uint32_t timer);

//*****************************************************************************
//
//! Get timer interrupt status masked with the enabled interrupts.
//! This function is useful to call in ISRs to get a list of pending
//! interrupts that are actually enabled and could have caused
//! the ISR.
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \return uint32_t. Return interrupt status. Valid values are
//!       - \b TIMER_A_INTERRUPT_PENDING
//!       - \b TIMER_A_INTERRUPT_NOT_PENDING
//
//*****************************************************************************
extern uint32_t Timer_A_getEnabledInterruptStatus(uint32_t timer);

//*****************************************************************************
//
//! Enable capture compare interrupt
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister is the selected capture compare register
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_enableCaptureCompareInterrupt(uint32_t timer,
        uint_fast16_t captureCompareRegister);

//*****************************************************************************
//
//! Disable capture compare interrupt
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister is the selected capture compare register
//!
//! \return None
//
//*****************************************************************************
extern void Timer_A_disableCaptureCompareInterrupt(uint32_t timer,
        uint_fast16_t captureCompareRegister);

//*****************************************************************************
//
//! Return capture compare interrupt status
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister is the selected capture compare register
//!
//! \param mask is the mask for the interrupt status
//!        Mask value is the logical OR of any of the following:
//!        - \b TIMER_A_CAPTURE_OVERFLOW
//!        - \b TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG
//!
//! \returns uint32_t. The mask of the set flags.
//!         Valid values is an OR of
//!       - \b TIMER_A_CAPTURE_OVERFLOW,
//!       - \b TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG
//
//*****************************************************************************
extern uint32_t Timer_A_getCaptureCompareInterruptStatus(uint32_t timer,
        uint_fast16_t captureCompareRegister, uint_fast16_t mask);

//*****************************************************************************
//
//! Return capture compare interrupt status masked with the enabled interrupts.
//! This function is useful to call in ISRs to get a list of pending
//! interrupts that are actually enabled and could have caused
//! the ISR.
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//! \param captureCompareRegister is the selected capture compare register
//!
//! \returns uint32_t. The mask of the set flags.
//!         Valid values is an OR of
//!       - \b TIMER_A_CAPTURE_OVERFLOW,
//!       - \b TIMER_A_CAPTURECOMPARE_INTERRUPT_FLAG
//
//*****************************************************************************
extern uint32_t Timer_A_getCaptureCompareEnabledInterruptStatus(uint32_t timer,
        uint_fast16_t captureCompareRegister);

//*****************************************************************************
//
//! Registers an interrupt handler for the timer capture compare interrupt.
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \param interruptSelect Selects which timer interrupt handler to
//! register. For the timer module, there are two separate interrupt handlers
//! that can be registered:
//!         - \b TIMER_A_CCR0_INTERRUPT Corresponds to the interrupt for CCR0
//!         - \b TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT Corresponds to the
//!          interrupt for CCR1-6, as well as the overflow interrupt.
//!
//! \param intHandler is a pointer to the function to be called when the
//! timer capture compare interrupt occurs.
//!
//! This function registers the handler to be called when a timer
//! interrupt occurs. This function enables the global interrupt in the
//! interrupt controller; specific Timer_Ainterrupts must be enabled
//! via Timer_A_enableInterrupt().  It is the interrupt handler's
//! responsibility to clear the interrupt source
//! via Timer_A_clearCaptureCompareInterrupt().
//!
//! \return None.
//
//*****************************************************************************
extern void Timer_A_registerInterrupt(uint32_t timer,
        uint_fast8_t interruptSelect, void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the interrupt handler for the timer
//!
//! \param timer is the instance of the Timer_A module. Valid parameters
//! vary from part to part, but can include:
//!         - \b TIMER_A0_BASE
//!         - \b TIMER_A1_BASE
//!         - \b TIMER_A2_BASE
//!         - \b TIMER_A3_BASE
//!
//! \param interruptSelect Selects which timer interrupt handler to
//! register. For the timer module, there are two separate interrupt handlers
//! that can be registered:
//!         - \b TIMER_A_CCR0_INTERRUPT Corresponds to the interrupt for CCR0
//!         - \b TIMER_A_CCRX_AND_OVERFLOW_INTERRUPT Corresponds to the
//!          interrupt for CCR1-6, as well as the overflow interrupt.
//!
//! This function unregisters the handler to be called when timer
//! interrupt occurs.  This function also masks off the interrupt in the
//! interrupt controller so that the interrupt handler no longer is called.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! \return None.
//
//*****************************************************************************
extern void Timer_A_unregisterInterrupt(uint32_t timer,
        uint_fast8_t interruptSelect);

/* Backwards Compatibility Layer */
#define Timer_A_clearTimerInterrupt Timer_A_clearInterruptFlag
#define Timer_A_clear Timer_A_clearTimer
#define Timer_A_initCaptureMode Timer_A_initCapture
#define Timer_A_initCompareMode Timer_A_initCompare
#define Timer_A_initContinuousMode Timer_A_configureContinuousMode
#define Timer_A_initUpDownMode Timer_A_configureUpDownMode
#define Timer_A_initUpMode Timer_A_configureUpMode
#define Timer_A_outputPWM Timer_A_generatePWM
#define Timer_A_stop Timer_A_stopTimer

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

#endif /* TIMERA_H_ */
