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
/** ============================================================================
 *  @file       GPIO.h
 *
 *  @brief      GPIO driver
 *
 *  The GPIO header file should be included in an application as follows:
 *  @code
 *  #include <ti/drivers/GPIO.h>
 *  @endcode
 *
 *  # Overview #
 *  The GPIO module allows you to manage General Purpose I/O pins via simple
 *  and portable APIs. GPIO pin behavior is usually configured statically,
 *  but can also be configured or reconfigured at runtime.
 *
 *  Because of its simplicity, the GPIO driver does not follow the model of
 *  other TI-RTOS drivers in which a driver application interface has
 *  separate device-specific implementations. This difference is most
 *  apparent in the GPIOxxx_Config structure, which does not require you to
 *  specify a particular function table or object.
 *
 *  # Usage #
 *  The following code example demonstrates how
 *  to configure a GPIO pin to generate an interrupt and how to toggle an
 *  an LED on and off within the registered interrupt callback function.
 *
 *  @code
 *  #include <stdint.h>
 *  #include <stddef.h>
 *
 *  // Driver Header file
 *  #include <ti/drivers/GPIO.h>
 *
 *  // Example/Board Header file
 *  #include "Board.h"
 *
 *  main()
 *  {
 *      // Call GPIO driver init function
 *      GPIO_init();
 *
 *      // Turn on user LED
 *      GPIO_write(Board_GPIO_LED0, Board_GPIO_LED_ON);
 *
 *      // install Button callback
 *      GPIO_setCallback(Board_GPIO_BUTTON0, gpioButtonFxn0);
 *
 *      // Enable interrupts
 *      GPIO_enableInt(Board_GPIO_BUTTON0);
 *
 *      ...
 *  }
 *
 *  //
 *  //  ======== gpioButtonFxn0 ========
 *  //  Callback function for the GPIO interrupt on Board_GPIO_BUTTON0.
 *  //
 *  void gpioButtonFxn0(uint_least8_t index)
 *  {
 *      // Toggle the LED
 *      GPIO_toggle(Board_GPIO_LED0);
 *  }
 *
 *  @endcode
 *
 *  Details for the example code above are described in the following
 *  subsections.
 *
 *  ### GPIO Driver Configuration #
 *
 *  In order to use the GPIO APIs, the application is required
 *  to provide 3 structures in the Board.c file:
 *  1.  An array of @ref GPIO_PinConfig elements that defines the
 *  initial configuration of each pin used by the application. A
 *  pin is referenced in the application by its corresponding index in this
 *  array. The pin type (that is, INPUT/OUTPUT), its initial state (that is
 *  OUTPUT_HIGH or LOW), interrupt behavior (RISING/FALLING edge, etc.), and
 *  device specific pin identification are configured in each element
 *  of this array (see @ref GPIO_PinConfigSettings).
 *  Below is an MSP432 device specific example of the GPIO_PinConfig array:
 *  @code
 *  //
 *  // Array of Pin configurations
 *  // NOTE: The order of the pin configurations must coincide with what was
 *  //       defined in MSP_EXP432P401R.h
 *  // NOTE: Pins not used for interrupts should be placed at the end of the
 *  //       array.  Callback entries can be omitted from callbacks array to
 *  //       reduce memory usage.
 *  //
 *  GPIO_PinConfig gpioPinConfigs[] = {
 *      // Input pins
 *      // MSP_EXP432P401R_GPIO_S1
 *      GPIOMSP432_P1_1 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING,
 *      // MSP_EXP432P401R_GPIO_S2
 *      GPIOMSP432_P1_4 | GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING,
 *
 *      // Output pins
 *      // MSP_EXP432P401R_GPIO_LED1
 *      GPIOMSP432_P1_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
 *      // MSP_EXP432P401R_GPIO_LED_RED
 *      GPIOMSP432_P2_0 | GPIO_CFG_OUT_STD | GPIO_CFG_OUT_STR_HIGH | GPIO_CFG_OUT_LOW,
 *  };
 *  @endcode
 *
 *  2.  An array of @ref GPIO_CallbackFxn elements that is used to store
 *  callback function pointers for GPIO pins configured with interrupts.
 *  The indexes for these array elements correspond to the pins defined
 *  in the GPIO_pinConfig array. These function pointers can be defined
 *  statically by referencing the callback function name in the array
 *  element, or dynamically, by setting the array element to NULL and using
 *  GPIO_setCallback() at runtime to plug the callback entry.
 *  Pins not used for interrupts can be omitted from the callback array to
 *  reduce memory usage (if they are placed at the end of GPIO_pinConfig
 *  array). The callback function syntax should match the following:
 *  @code
 *  void (*GPIO_CallbackFxn)(uint_least8_t index);
 *  @endcode
 *  The index parameter is the same index that was passed to
 *  GPIO_setCallback(). This allows the same callback function to be used
 *  for multiple GPIO interrupts, by using the index to identify the GPIO
 *  that caused the interrupt.
 *  Keep in mind that the callback functions will be called in the context of
 *  an interrupt service routine and should be designed accordingly.  When an
 *  interrupt is triggered, the interrupt status of all (interrupt enabled) pins
 *  on a port will be read, cleared, and the respective callbacks will be
 *  executed.  Callbacks will be called in order from least significant bit to
 *  most significant bit.
 *  Below is an MSP432 device specific example of the GPIO_CallbackFxn array:
 *  @code
 *  //
 *  // Array of callback function pointers
 *  // NOTE: The order of the pin configurations must coincide with what was
 *  //       defined in MSP_EXP432P401R.h
 *  // NOTE: Pins not used for interrupts can be omitted from callbacks array
 *  //       to reduce memory usage (if placed at end of gpioPinConfigs
 *  //       array).
 *  //
 *  GPIO_CallbackFxn gpioCallbackFunctions[] = {
 *      // MSP_EXP432P401R_GPIO_S1
 *      NULL,
 *      // MSP_EXP432P401R_GPIO_S2
 *      NULL
 *  };
 *  @endcode
 *
 *  3.  A device specific GPIOxxx_Config structure that tells the GPIO
 *  driver where the two aforementioned arrays are and the number of elements
 *  in each. The interrupt priority of all pins configured to generate
 *  interrupts is also specified here. Values for the interrupt priority are
 *  device-specific. You should be well-acquainted with the interrupt
 *  controller used in your device before setting this parameter to a
 *  non-default value. The sentinel value of (~0) (the default value) is
 *  used to indicate that the lowest possible priority should be used.
 *  Below is an MSP432 device specific example of a GPIOxxx_Config
 *  structure:
 *  @code
 *  //
 *  // MSP432 specific GPIOxxx_Config structure
 *  //
 *  const GPIOMSP432_Config GPIOMSP432_config = {
 *      .pinConfigs = (GPIO_PinConfig *)gpioPinConfigs,
 *      .callbacks = (GPIO_CallbackFxn *)gpioCallbackFunctions,
 *      .numberOfPinConfigs = sizeof(gpioPinConfigs)/sizeof(GPIO_PinConfig),
 *      .numberOfCallbacks = sizeof(gpioCallbackFunctions)/sizeof(GPIO_CallbackFxn),
 *      .intPriority = (~0)
 *  };
 *  @endcode
 *
 *  ### Initializing the GPIO Driver #
 *
 *  GPIO_init() must be called before any other GPIO APIs.  This function
 *  configures each GPIO pin in the user-provided @ref GPIO_PinConfig
 *  array according to the defined settings. The user can also reconfigure
 *  a pin dynamically after GPIO_init() is called by using the
 *  GPIO_setConfig(), and GPIO_setCallback() APIs.
 *
 *  # Implementation #
 *
 *  Unlike most other TI-RTOS drivers, the GPIO driver has no generic function
 *  table with pointers to device-specific API implementations. All the generic
 *  GPIO APIs are implemented by the device-specific GPIO driver module.
 *  Additionally, there is no notion of an instance 'handle' with the GPIO driver.
 *  GPIO pins are referenced by their numeric index in the GPIO_PinConfig array.
 *  This design approach was used to enhance runtime and memory efficiency.
 *
 *  ============================================================================
 */

#ifndef ti_drivers_GPIO__include
#define ti_drivers_GPIO__include

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 *  @name GPIO_STATUS_* macros are general status codes returned by GPIO driver APIs.
 *  @{
 */

/*!
 * @brief   Common GPIO status code reservation offset.
 *
 * GPIO driver implementations should offset status codes with
 * GPIO_STATUS_RESERVED growing negatively.
 *
 * Example implementation specific status codes:
 * @code
 * #define GPIOTXYZ_STATUS_ERROR1    GPIO_STATUS_RESERVED - 1
 * #define GPIOTXYZ_STATUS_ERROR0    GPIO_STATUS_RESERVED - 0
 * #define GPIOTXYZ_STATUS_ERROR2    GPIO_STATUS_RESERVED - 2
 * @endcode
 */
#define GPIO_STATUS_RESERVED        (-32)

/*!
 * @brief   Successful status code returned by GPI_setConfig().
 *
 * GPI_setConfig() returns GPIO_STATUS_SUCCESS if the API was executed
 * successfully.
 */
#define GPIO_STATUS_SUCCESS         (0)

/*!
 * @brief   Generic error status code returned by GPI_setConfig().
 *
 * GPI_setConfig() returns GPIO_STATUS_ERROR if the API was not executed
 * successfully.
 */
#define GPIO_STATUS_ERROR           (-1)
/** @}*/

/*!
 *  @brief  GPIO pin configuration settings
 *
 *  The upper 16 bits of the 32 bit PinConfig is reserved
 *  for pin configuration settings.
 *
 *  The lower 16 bits are reserved for device-specific
 *  port/pin identifications
 */
typedef uint32_t GPIO_PinConfig;

/*!
 *  @cond NODOC
 *  Internally used configuration bit access macros.
 */
#define GPIO_CFG_IO_MASK           0x00ff0000
#define GPIO_CFG_IO_LSB            16
#define GPIO_CFG_OUT_TYPE_MASK     0x00060000
#define GPIO_CFG_OUT_TYPE_LSB      17
#define GPIO_CFG_IN_TYPE_MASK      0x00060000
#define GPIO_CFG_IN_TYPE_LSB       17
#define GPIO_CFG_OUT_STRENGTH_MASK 0x00f00000
#define GPIO_CFG_OUT_STRENGTH_LSB  20
#define GPIO_CFG_INT_MASK          0x07000000
#define GPIO_CFG_INT_LSB           24
#define GPIO_CFG_OUT_BIT           19
/*! @endcond */

/*!
 *  \defgroup GPIO_PinConfigSettings Macros used to configure GPIO pins
 *  @{
 */
/** @name GPIO_PinConfig output pin configuration macros
 *  @{
 */
#define GPIO_CFG_OUTPUT            (((uint32_t) 0) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Pin is an output. */
#define GPIO_CFG_OUT_STD           (((uint32_t) 0) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Output pin is actively driven high and low */
#define GPIO_CFG_OUT_OD_NOPULL     (((uint32_t) 2) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Output pin is Open Drain */
#define GPIO_CFG_OUT_OD_PU         (((uint32_t) 4) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Output pin is Open Drain w/ pull up */
#define GPIO_CFG_OUT_OD_PD         (((uint32_t) 6) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Output pin is Open Drain w/ pull dn */

#define GPIO_CFG_OUT_STR_LOW       (((uint32_t) 0) << GPIO_CFG_OUT_STRENGTH_LSB) /*!< @hideinitializer Set output pin strengh to low */
#define GPIO_CFG_OUT_STR_MED       (((uint32_t) 1) << GPIO_CFG_OUT_STRENGTH_LSB) /*!< @hideinitializer Set output pin strengh to medium */
#define GPIO_CFG_OUT_STR_HIGH      (((uint32_t) 2) << GPIO_CFG_OUT_STRENGTH_LSB) /*!< @hideinitializer Set output pin strengh to high */

#define GPIO_CFG_OUT_HIGH          (((uint32_t) 1) << GPIO_CFG_OUT_BIT) /*!< @hideinitializer Set pin's output to 1. */
#define GPIO_CFG_OUT_LOW           (((uint32_t) 0) << GPIO_CFG_OUT_BIT) /*!< @hideinitializer Set pin's output to 0. */
/** @} */

/** @name GPIO_PinConfig input pin configuration macros
 *  @{
 */
#define GPIO_CFG_INPUT             (((uint32_t) 1) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Pin is an input. */
#define GPIO_CFG_IN_NOPULL         (((uint32_t) 1) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Input pin with no internal PU/PD */
#define GPIO_CFG_IN_PU             (((uint32_t) 3) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Input pin with internal PU */
#define GPIO_CFG_IN_PD             (((uint32_t) 5) << GPIO_CFG_IO_LSB) /*!< @hideinitializer Input pin with internal PD */
/** @} */

/** @name GPIO_PinConfig interrupt configuration macros
 *  @{
 */
#define GPIO_CFG_IN_INT_NONE       (((uint32_t) 0) << GPIO_CFG_INT_LSB)    /*!< @hideinitializer No Interrupt */
#define GPIO_CFG_IN_INT_FALLING    (((uint32_t) 1) << GPIO_CFG_INT_LSB)    /*!< @hideinitializer Interrupt on falling edge */
#define GPIO_CFG_IN_INT_RISING     (((uint32_t) 2) << GPIO_CFG_INT_LSB)    /*!< @hideinitializer Interrupt on rising edge */
#define GPIO_CFG_IN_INT_BOTH_EDGES (((uint32_t) 3) << GPIO_CFG_INT_LSB)    /*!< @hideinitializer Interrupt on both edges */
#define GPIO_CFG_IN_INT_LOW        (((uint32_t) 4) << GPIO_CFG_INT_LSB)    /*!< @hideinitializer Interrupt on low level */
#define GPIO_CFG_IN_INT_HIGH       (((uint32_t) 5) << GPIO_CFG_INT_LSB)    /*!< @hideinitializer Interrupt on high level */
/** @} */

/** @name Special GPIO_PinConfig configuration macros
 *  @{
 */

/*!
 *  @brief 'Or' in this @ref GPIO_PinConfig definition to inform GPIO_setConfig()
 *  to only configure the interrupt attributes of a GPIO input pin.
 */
#define GPIO_CFG_IN_INT_ONLY       (((uint32_t) 1) << 27)                  /*!< @hideinitializer configure interrupt only */

/*!
 *  @brief Use this @ref GPIO_PinConfig definition to inform GPIO_init()
 *  NOT to configure the corresponding pin
 */
#define GPIO_DO_NOT_CONFIG         0x40000000                              /*!< @hideinitializer Do not configure this Pin */

/** @} */
/** @} end of GPIO_PinConfigSettings group */

/*!
 *  @brief  GPIO callback function type
 *
 *  @param      index       GPIO index.  This is the same index that
 *                          was passed to GPIO_setCallback().  This allows
 *                          you to use the same callback function for multiple
 *                          GPIO interrupts, by using the index to identify
 *                          the GPIO that caused the interrupt.
 */
typedef void (*GPIO_CallbackFxn)(uint_least8_t index);

/*!
 *  @brief      Clear a GPIO pin interrupt flag
 *
 *  Clears the GPIO interrupt for the specified index.
 *
 *  Note: It is not necessary to call this API within a
 *  callback assigned to a pin.
 *
 *  @param      index       GPIO index
 */
extern void GPIO_clearInt(uint_least8_t index);

/*!
 *  @brief      Disable a GPIO pin interrupt
 *
 *  Disables interrupts for the specified GPIO index.
 *
 *  @param      index       GPIO index
 */
extern void GPIO_disableInt(uint_least8_t index);

/*!
 *  @brief      Enable a GPIO pin interrupt
 *
 *  Enables GPIO interrupts for the selected index to occur.
 *
 *  Note:  Prior to enabling a GPIO pin interrupt, make sure
 *  that a corresponding callback function has been provided.
 *  Use the GPIO_setCallback() API for this purpose at runtime.
 *  Alternatively, the callback function can be statically
 *  configured in the GPIO_CallbackFxn array provided.
 *
 *  @param      index       GPIO index
 */
extern void GPIO_enableInt(uint_least8_t index);

/*!
 *  @brief      Get the current configuration for a gpio pin
 *
 *  The pin configuration is provided in the static GPIO_PinConfig array,
 *  but can be changed with GPIO_setConfig().  GPIO_getConfig() gets the
 *  current pin configuration.
 *
 *  @param      index       GPIO index
 *  @param      pinConfig   Location to store device specific pin
 *                          configuration settings
 */
extern void GPIO_getConfig(uint_least8_t index, GPIO_PinConfig *pinConfig);

/*!
 *  @brief  Initializes the GPIO module
 *
 *  The pins defined in the application-provided *GPIOXXX_config* structure
 *  are initialized accordingly.
 *
 *  @pre    The GPIO_config structure must exist and be persistent before this
 *          function can be called. This function must also be called before
 *          any other GPIO driver APIs.
 */
extern void GPIO_init();

/*!
 *  @brief      Reads the value of a GPIO pin
 *
 *  The value returned will either be zero or one depending on the
 *  state of the pin.
 *
 *  @param      index  GPIO index
 *
 *  @return     0 or 1, depending on the state of the pin.
 */
extern uint_fast8_t GPIO_read(uint_least8_t index);

/*!
 *  @brief      Bind a callback function to a GPIO pin interrupt
 *
 *  Associate a callback function with a particular GPIO pin interrupt.
 *
 *  Callbacks can be changed at any time, making it easy to switch between
 *  efficient, state-specific interrupt handlers.
 *
 *  Note: The callback function is called within the context of an interrupt
 *  handler.
 *
 *  Note: This API does not enable the GPIO pin interrupt.
 *  Use GPIO_enableInt() and GPIO_disableInt() to enable
 *  and disable the pin interrupt as necessary.
 *
 *  Note: it is not necessary to call GPIO_clearInt() within a callback.
 *  That operation is performed internally before the callback is invoked.
 *
 *  @param      index       GPIO index
 *  @param      callback    address of the callback function
 */
extern void GPIO_setCallback(uint_least8_t index, GPIO_CallbackFxn callback);

/*!
 *  @brief      Configure the gpio pin
 *
 *  Dynamically configure a gpio pin to a device specific setting.
 *  For many applications, the pin configurations provided in the static
 *  GPIO_PinConfig array is sufficient.
 *
 *  For input pins with interrupt configurations, a corresponding interrupt
 *  object will be created as needed.
 *
 *  @param      index       GPIO index
 *  @param      pinConfig   device specific pin configuration settings
 */
extern int_fast16_t GPIO_setConfig(uint_least8_t index,
    GPIO_PinConfig pinConfig);

/*!
 *  @brief      Toggles the current state of a GPIO
 *
 *  @param      index  GPIO index
 */
extern void GPIO_toggle(uint_least8_t index);

/*!
 *  @brief     Writes the value to a GPIO pin
 *
 *  @param      index    GPIO index
 *  @param      value    must be either 0 or 1
 */
extern void GPIO_write(uint_least8_t index, unsigned int value);

#ifdef __cplusplus
}
#endif

#endif /* ti_drivers_GPIO__include */
