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
 *  @file       GPIOCC32XX.h
 *
 *  @brief      GPIO driver implementation for CC32xx devices
 *
 *  The GPIO header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/GPIO.h>
 *  #include <ti/drivers/gpio/GPIOCC32XX.h>
 *  @endcode
 *
 *  Refer to @ref GPIO.h for a complete description of the GPIO
 *  driver APIs provided and examples of their use.
 *
 *  ### CC32xx GPIO Driver Configuration #
 *
 *  In order to use the GPIO APIs, the application is required
 *  to provide 3 structures in the Board.c file:
 *
 *  1.  An array of @ref GPIO_PinConfig elements that defines the
 *  initial configuration of each pin used by the application. A
 *  pin is referenced in the application by its corresponding index in this
 *  array. The pin type (that is, INPUT/OUTPUT), its initial state (that is
 *  OUTPUT_HIGH or LOW), interrupt behavior (RISING/FALLING edge, etc.)
 *  (see @ref GPIO_PinConfigSettings), and
 *  device specific pin identification (see @ref GPIOCC32XX_PinConfigIds)
 *  are configured in each element of this array.
 *  Below is an CC32XX device specific example of the GPIO_PinConfig array:
 *  @code
 *  //
 *  // Array of Pin configurations
 *  // NOTE: The order of the pin configurations must coincide with what was
 *  //       defined in CC3220SF_LAUNCHXL.h
 *  // NOTE: Pins not used for interrupts should be placed at the end of the
 *  //       array.  Callback entries can be omitted from callbacks array to
 *  //       reduce memory usage.
 *  //
 *  GPIO_PinConfig gpioPinConfigs[] = {
 *      // input pins with callbacks
 *      // CC3220SF_LAUNCHXL_GPIO_SW2
 *      GPIOCC32XX_GPIO_22 | GPIO_CFG_INPUT | GPIO_CFG_IN_INT_RISING,
 *      // CC3220SF_LAUNCHXL_GPIO_SW3
 *      GPIOCC32XX_GPIO_13 | GPIO_CFG_INPUT | GPIO_CFG_IN_INT_RISING,
 *
 *      // output pins
 *      // CC3220SF_LAUNCHXL_GPIO_LED_D7
 *      GPIOCC32XX_GPIO_09 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
 *  };
 *  @endcode
 *
 *  2.  An array of @ref GPIO_CallbackFxn elements that is used to store
 *  callback function pointers for GPIO pins configured with interrupts.
 *  The indexes for these array elements correspond to the pins defined
 *  in the @ref GPIO_PinConfig array. These function pointers can be defined
 *  statically by referencing the callback function name in the array
 *  element, or dynamically, by setting the array element to NULL and using
 *  GPIO_setCallback() at runtime to plug the callback entry.
 *  Pins not used for interrupts can be omitted from the callback array to
 *  reduce memory usage (if they are placed at the end of the @ref
 *  GPIO_PinConfig array). The callback function syntax should match the
 *  following:
 *  @code
 *  void (*GPIO_CallbackFxn)(unsigned int index);
 *  @endcode
 *  The index parameter is the same index that was passed to
 *  GPIO_setCallback(). This allows the same callback function to be used
 *  for multiple GPIO interrupts, by using the index to identify the GPIO
 *  that caused the interrupt.
 *  Below is a CC32XX device specific example of the @ref GPIO_CallbackFxn
 *  array:
 *  @code
 *  //
 *  // Array of callback function pointers
 *  // NOTE: The order of the pin configurations must coincide with what was
 *  //       defined in CC3220SF_LAUNCHXL.h
 *  // NOTE: Pins not used for interrupts can be omitted from callbacks array to
 *  //       reduce memory usage (if placed at end of gpioPinConfigs array).
 *  //
 *  GPIO_CallbackFxn gpioCallbackFunctions[] = {
 *      NULL,  // CC3220SF_LAUNCHXL_GPIO_SW2
 *      NULL   // CC3220SF_LAUNCHXL_GPIO_SW3
 *  };
 *  @endcode
 *
 *  3.  The device specific GPIOCC32XX_Config structure that tells the GPIO
 *  driver where the two aforementioned arrays are and the number of elements
 *  in each. The interrupt priority of all pins configured to generate
 *  interrupts is also specified here. Values for the interrupt priority are
 *  device-specific. You should be well-acquainted with the interrupt
 *  controller used in your device before setting this parameter to a
 *  non-default value. The sentinel value of (~0) (the default value) is
 *  used to indicate that the lowest possible priority should be used.
 *  Below is an example of an initialized GPIOCC32XX_Config
 *  structure:
 *  @code
 *  const GPIOCC32XX_Config GPIOCC32XX_config = {
 *      .pinConfigs = (GPIO_PinConfig *)gpioPinConfigs,
 *      .callbacks = (GPIO_CallbackFxn *)gpioCallbackFunctions,
 *      .numberOfPinConfigs = sizeof(gpioPinConfigs)/sizeof(GPIO_PinConfig),
 *      .numberOfCallbacks = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
 *      .intPriority = (~0)
 *  };
 *  @endcode
 *
 *  ============================================================================
 */

#ifndef ti_drivers_GPIOCC32XX__include
#define ti_drivers_GPIOCC32XX__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <ti/drivers/GPIO.h>

/*!
 *  @brief  GPIO device specific driver configuration structure
 *
 *  The device specific GPIOCC32XX_Config structure that tells the GPIO
 *  driver where the two aforementioned arrays are and the number of elements
 *  in each. The interrupt priority of all pins configured to generate
 *  interrupts is also specified here. Values for the interrupt priority are
 *  device-specific. You should be well-acquainted with the interrupt
 *  controller used in your device before setting this parameter to a
 *  non-default value. The sentinel value of (~0) (the default value) is
 *  used to indicate that the lowest possible priority should be used.
 *
 *  Below is an example of an initialized GPIOCC32XX_Config
 *  structure:
 *  @code
 *  const GPIOCC32XX_Config GPIOCC32XX_config = {
 *      .pinConfigs = (GPIO_PinConfig *)gpioPinConfigs,
 *      .callbacks = (GPIO_CallbackFxn *)gpioCallbackFunctions,
 *      .numberOfPinConfigs = sizeof(gpioPinConfigs)/sizeof(GPIO_PinConfig),
 *      .numberOfCallbacks = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
 *      .intPriority = (~0)
 *  };
 *  @endcode
 */
typedef struct GPIOCC32XX_Config {
    /*! Pointer to the board's GPIO_PinConfig array */
    GPIO_PinConfig  *pinConfigs;

    /*! Pointer to the board's GPIO_CallbackFxn array */
    GPIO_CallbackFxn  *callbacks;

    /*! Number of GPIO_PinConfigs defined */
    uint32_t numberOfPinConfigs;

    /*! Number of GPIO_Callbacks defined */
    uint32_t numberOfCallbacks;

    /*!
     *  Interrupt priority used for call back interrupts.
     *
     *  intPriority is the interrupt priority, as defined by the
     *  underlying OS.  It is passed unmodified to the underlying OS's
     *  interrupt handler creation code, so you need to refer to the OS
     *  documentation for usage.  If the driver uses the ti.dpl
     *  interface instead of making OS calls directly, then the HwiP port
     *  handles the interrupt priority in an OS specific way.  In the case
     *  of the SYS/BIOS port, intPriority is passed unmodified to Hwi_create().
     *
     *  Setting ~0 will configure the lowest possible priority
     */
    uint32_t intPriority;
} GPIOCC32XX_Config;

/*!
 *  \defgroup GPIOCC32XX_PinConfigIds GPIO pin identification macros used to configure GPIO pins
 *  @{
 */
/**
 *  @name Device specific GPIO port/pin identifiers to be used within the board's GPIO_PinConfig table.
 *  @{
*/
#define GPIOCC32XX_EMPTY_PIN  0x0000    /*!< @hideinitializer */

#define GPIOCC32XX_GPIO_00    0x0001    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_01    0x0002    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_02    0x0004    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_03    0x0008    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_04    0x0010    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_05    0x0020    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_06    0x0040    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_07    0x0080    /*!< @hideinitializer */

#define GPIOCC32XX_GPIO_08    0x0101    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_09    0x0102    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_10    0x0104    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_11    0x0108    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_12    0x0110    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_13    0x0120    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_14    0x0140    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_15    0x0180    /*!< @hideinitializer */

#define GPIOCC32XX_GPIO_16    0x0201    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_17    0x0202    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_22    0x0240    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_23    0x0280    /*!< @hideinitializer */

#define GPIOCC32XX_GPIO_24    0x0301    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_25    0x0302    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_28    0x0310    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_29    0x0320    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_30    0x0340    /*!< @hideinitializer */
#define GPIOCC32XX_GPIO_31    0x0380    /*!< @hideinitializer */

#define GPIOCC32XX_GPIO_32    0x0401    /*!< @hideinitializer */
/** @} */

/**
 *  @name CC32xx device specific GPIO_PinConfig macros
 *  @{
 */
#define GPIOCC32XX_USE_STATIC 0x8000    /*!< @hideinitializer use statically-defined parking state */
/** @} */

/** @} end of GPIOCC32XX_PinConfigIds group */

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_GPIOCC32XX__include */
