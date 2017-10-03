/*
 * Copyright (c) 2015-2017, Texas Instruments Incorporated
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
 */
/*! ============================================================================
 *  @file       PWMTimerCC32XX.h
 *
 *  @brief      PWM driver implementation using CC32XX General Purpose Timers.
 *
 *  The PWM header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/PWM.h>
 *  #include <ti/drivers/pwm/PWMTimerCC32XX.h>
 *  @endcode
 *
 *  Refer to @ref PWM.h for a complete description of the PWM
 *  driver APIs provided and examples of their use.
 *
 *  ## Overview #
 *  This driver configures a CC32XX General Purpose Timer (GPT) in PWM mode.
 *  When in PWM mode, each GPT is divided into 2 PWM outputs.  This driver
 *  manages each output as an independent PWM instance.  The timer is
 *  automatically configured in count-down mode using the system clock as
 *  the source.
 *
 *  The timers operate at the system clock frequency (80 MHz). So each timer
 *  tick is 12.5 ns. The period and duty registers are 16 bits wide; thus
 *  8-bit prescalars are used to extend period and duty registers.  The
 *  maximum value supported is 16777215 timer counts ((2^24) - 1) or
 *  209715 microseconds.  Updates to a PWM's period or duty will occur
 *  instantaneously (GPT peripherals do not have shadow registers).
 *
 *  When stopped, the driver will configure the pin in GPIO mode & set the
 *  output to the PWM_IdleLevel specified in the params used during open.  Users
 *  need be aware that while PIN 19 can be used for PWM it is not GPIO capable,
 *  so it cannot be set to the PWM_IdleLevel.  Output voltage will be PWM output
 *  at the moment it is stopped.
 *
 *  Finally, when this driver is opened, it automatically changes the
 *  PWM pin's parking configuration (used when entering low power modes) to
 *  correspond with the PWM_IDLE_LEVEL set in the PWM_params.  However, this
 *  setting is not reverted once the driver is closed, it is the users
 *  responsibility to change the parking configuration if necessary.
 *
 *  ### CC32xx PWM Driver Configuration #
 *
 *  In order to use the PWM APIs, the application is required
 *  to define 4 configuration items in the application Board.c file:
 *
 *  1.  An array of PWMTimerCC32XX_Object elements, which will be used by
 *  by the driver to maintain instance state.
 *  Below is an example PWMTimerCC32XX_Object array appropriate for the CC3220SF Launchpad
 *  board:
 *  @code
 *    #include <ti/drivers/PWM.h>
 *    #include <ti/drivers/pwm/PWMTimerCC32XX.h>
 *
 *    PWMTimerCC32XX_Object pwmTimerCC3220SObjects[CC3220SF_LAUNCHXL_PWMCOUNT];
 *  @endcode
 *
 *  2.  An array of PWMTimerCC32XX_HWAttrsV2 elements that defines which
 *  pin will be used by the corresponding PWM instance
 *  (see @ref pwmPinIdentifiersCC32XX).
 *  Below is an example PWMTimerCC32XX_HWAttrsV2 array appropriate for the CC3220SF Launchpad
 *  board:
 *  @code
 *    const PWMTimerCC32XX_HWAttrsV2 pwmTimerCC3220SHWAttrs[CC3220SF_LAUNCHXL_PWMCOUNT] = {
 *        {
 *            .pwmPin = PWMTimerCC32XX_PIN_01
 *        },
 *        {
 *            .pwmPin = PWMTimerCC32XX_PIN_02
 *        }
 *    };
 *  @endcode
 *
 *  3.  An array of @ref PWM_Config elements, one for each PWM instance. Each
 *  element of this array identifies the device-specific API function table,
 *  the device specific PWM object instance, and the device specific Hardware
 *  Attributes to be used for each PWM channel.
 *  Below is an example @ref PWM_Config array appropriate for the CC3220SF Launchpad
 *  board:
 *  @code
 *    const PWM_Config PWM_config[CC3220SF_LAUNCHXL_PWMCOUNT] = {
 *        {
 *            .fxnTablePtr = &PWMTimerCC32XX_fxnTable,
 *            .object = &pwmTimerCC3220SObjects[CC3220SF_LAUNCHXL_PWM6],
 *            .hwAttrs = &pwmTimerCC3220SHWAttrs[CC3220SF_LAUNCHXL_PWM6]
 *        },
 *        {
 *            .fxnTablePtr = &PWMTimerCC32XX_fxnTable,
 *            .object = &pwmTimerCC3220SObjects[CC3220SF_LAUNCHXL_PWM7],
 *            .hwAttrs = &pwmTimerCC3220SHWAttrs[CC3220SF_LAUNCHXL_PWM7]
 *        }
 *    };
 *  @endcode
 *
 *  4.  A global variable, PWM_count, that informs the driver how many PWM
 *  instances are defined:
 *  @code
 *    const uint_least8_t PWM_count = CC3220SF_LAUNCHXL_PWMCOUNT;
 *  @endcode
 *
 * ### Power Management #
 * The TI-RTOS power management framework will try to put the device into the most
 * power efficient mode whenever possible. Please see the technical reference
 * manual for further details on each power mode.
 *
 *  The PWMTimerCC32XX driver explicitly sets a power constraint when the
 *  PWM is running to prevent LPDS.
 *  The following statements are valid:
 *    - After PWM_open():  Clocks are enabled to the timer resource and the
 *                         configured pwmPin. The device is still allowed
 *                         to enter LPDS.
 *    - After PWM_start(): LPDS is disabled when PWM is running.
 *    - After PWM_stop():  Conditions are equal as for after PWM_open
 *    - After PWM_close(): The underlying GPTimer is turned off, and the clocks
 *                         to the timer and pin are disabled..
 *
 * =============================================================================
 */

#ifndef ti_driver_pwm_PWMTimerCC32XX__include
#define ti_driver_pwm_PWMTimerCC32XX__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <ti/drivers/PWM.h>

/*! \cond */
/*
 *  PWMTimer port/pin defines for pin configuration.
 *
 *  The timer id (0, 1, 2, or 3) is stored in bits 31 - 28
 *  The timer half (0 = A, 1 = B) is stored in bits 27 - 24
 *  The GPIO port (0, 1, 2, or 3) is stored in bits 23 - 20
 *  The GPIO pin index within the port (0 - 7) is stored in bits 19 - 16
 *  The pin mode is stored in bits 11 - 8
 *  The pin number is stored in bits 7 - 0
 *
 *
 *    31 - 28     27 - 24      23 - 20        19 - 16       11 - 8   7 - 0
 *  -----------------------------------------------------------------------
 *  | Timer id | Timer half | GPIO port | GPIO pin index | pin mode | pin |
 *  -----------------------------------------------------------------------
 *
 *  The CC32XX has fixed GPIO assignments and pin modes for a given pin.
 *  A PWM pin mode for a given pin has a fixed timer/timer-half.
 */
#define PWMTimerCC32XX_T0A  (0x00 << 24)
#define PWMTimerCC32XX_T0B  (0x01 << 24)
#define PWMTimerCC32XX_T1A  (0x10 << 24)
#define PWMTimerCC32XX_T1B  (0x11 << 24)
#define PWMTimerCC32XX_T2A  (0x20 << 24)
#define PWMTimerCC32XX_T2B  (0x21 << 24)
#define PWMTimerCC32XX_T3A  (0x30 << 24)
#define PWMTimerCC32XX_T3B  (0x31 << 24)

#define PWMTimerCC32XX_GPIO9   (0x11 << 16)
#define PWMTimerCC32XX_GPIO10  (0x12 << 16)
#define PWMTimerCC32XX_GPIO11  (0x13 << 16)
#define PWMTimerCC32XX_GPIO24  (0x30 << 16)
#define PWMTimerCC32XX_GPIO25  (0x31 << 16)

#define PWMTimerCC32XX_GPIONONE  (0xFF << 16)
/*! \endcond */

/*!
 *  \defgroup pwmPinIdentifiersCC32XX PWMTimerCC32XX_HWAttrs 'pwmPin' field options
 *  @{
 */
/*!
 *  @name PIN 01, GPIO10, uses Timer3A for PWM.
 *  @{
 */
#define PWMTimerCC32XX_PIN_01  (PWMTimerCC32XX_T3A | PWMTimerCC32XX_GPIO10 | 0x0300) /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 02, GPIO11, uses Timer3B for PWM.
 *  @{
 */
#define PWMTimerCC32XX_PIN_02  (PWMTimerCC32XX_T3B | PWMTimerCC32XX_GPIO11 | 0x0301) /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 17, GPIO24, uses Timer0A for PWM.
 *  @{
 */
#define PWMTimerCC32XX_PIN_17  (PWMTimerCC32XX_T0A | PWMTimerCC32XX_GPIO24 | 0x0510) /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 19, uses Timer1B for PWM.
 *  @{
 */
#define PWMTimerCC32XX_PIN_19  (PWMTimerCC32XX_T1B | PWMTimerCC32XX_GPIONONE | 0x0812) /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 21, GPIO25, uses Timer1A for PWM.
 *  @{
 */
#define PWMTimerCC32XX_PIN_21  (PWMTimerCC32XX_T1A | PWMTimerCC32XX_GPIO25 | 0x0914) /*!< @hideinitializer */
/*! @} */
/*!
 *  @name PIN 64, GPIO9, uses Timer2B for PWM.
 *  @{
 */
#define PWMTimerCC32XX_PIN_64  (PWMTimerCC32XX_T2B | PWMTimerCC32XX_GPIO9 | 0x033F)  /*!< @hideinitializer */
/*! @} */
/*! @} */

/**
 *  @addtogroup PWM_STATUS
 *  PWMTimerCC32XX_STATUS_* macros are command codes only defined in the
 *  PWMTimerCC32XX.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/pwm/PWMTimerCC32XX.h>
 *  @endcode
 *  @{
 */

/* Add PWMTimerCC32XX_STATUS_* macros here */

/** @}*/

/**
 *  @addtogroup PWM_CMD
 *  PWMTimerCC32XX_CMD_* macros are command codes only defined in the
 *  PWMTimerCC32XX.h driver implementation and need to:
 *  @code
 *  #include <ti/drivers/pwm/PWMTimerCC32XX.h>
 *  @endcode
 *  @{
 */

/* Add PWMTimerCC32XX_CMD_* macros here */

/** @}*/

/* PWM function table pointer */
extern const PWM_FxnTable PWMTimerCC32XX_fxnTable;

/*!
 *  @brief  PWMTimerCC32XX Hardware attributes
 *
 *  The 'pwmPin' field identifies which physical pin to use for a
 *  particular PWM channel as well as the corresponding Timer resource used
 *  to source the PWM signal. The encoded pin identifier macros for
 *  initializing the 'pwmPin' field must be selected from the
 *  @ref pwmPinIdentifiersCC32XX macros.
 *
 *  A sample structure is shown below:
 *  @code
 *  const PWMTimerCC32XX_HWAttrsV2 pwmTimerCC32XXHWAttrs[] = {
 *      {
 *          .pwmPin = PWMTimerCC32XX_PIN_01,
 *      },
 *      {
 *          .pwmPin = PWMTimerCC32XX_PIN_02,
 *      }
 *  };
 *  @endcode
 */
typedef struct PWMTimerCC32XX_HWAttrsV2 {
    uint32_t pwmPin;                    /*!< Pin to output PWM signal on
                                             (see @ref pwmPinIdentifiersCC32XX) */
} PWMTimerCC32XX_HWAttrsV2;

/*!
 *  @brief  PWMTimerCC32XX Object
 *
 *  The application must not access any member variables of this structure!
 */
typedef struct PWMTimerCC32XX_Object {
    Power_NotifyObj  postNotify;
    uint32_t         duty;              /* Current duty cycle in Duty_Unites */
    uint32_t         period;            /* Current period PERIOD_Units */
    PWM_Duty_Units   dutyUnits;         /* Current duty cycle unit */
    PWM_Period_Units periodUnits;       /* Current period unit */
    PWM_IdleLevel    idleLevel;         /* PWM idle level when stopped / not started */
    bool             pwmStarted;        /* Used to gate Power_set/releaseConstraint() calls */
    bool             isOpen;            /* open flag used to check if PWM is opened */
} PWMTimerCC32XX_Object;

#ifdef __cplusplus
}
#endif

#endif /* ti_driver_pwm_PWMTimerCC32XX__include */
