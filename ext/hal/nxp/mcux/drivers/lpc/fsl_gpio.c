/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_gpio.h"

/* Component ID definition, used by tools. */
#ifndef FSL_COMPONENT_ID
#define FSL_COMPONENT_ID "platform.drivers.lpc_gpio"
#endif

/*******************************************************************************
 * Variables
 ******************************************************************************/
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Array to map FGPIO instance number to clock name. */
static const clock_ip_name_t s_gpioClockName[] = GPIO_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#if !(defined(FSL_FEATURE_GPIO_HAS_NO_RESET) && FSL_FEATURE_GPIO_HAS_NO_RESET)
/*! @brief Pointers to GPIO resets for each instance. */
static const reset_ip_name_t s_gpioResets[] = GPIO_RSTS_N;
#endif
/*******************************************************************************
* Prototypes
************ ******************************************************************/

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * brief Initializes the GPIO peripheral.
 *
 * This function ungates the GPIO clock.
 *
 * param base   GPIO peripheral base pointer.
 * param port   GPIO port number.
 */
void GPIO_PortInit(GPIO_Type *base, uint32_t port)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    assert(port < ARRAY_SIZE(s_gpioClockName));

    /* Upgate the GPIO clock */
    CLOCK_EnableClock(s_gpioClockName[port]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
#if !(defined(FSL_FEATURE_GPIO_HAS_NO_RESET) && FSL_FEATURE_GPIO_HAS_NO_RESET)
    /* Reset the GPIO module */
    RESET_PeripheralReset(s_gpioResets[port]);
#endif
}

/*!
 * brief Initializes a GPIO pin used by the board.
 *
 * To initialize the GPIO, define a pin configuration, either input or output, in the user file.
 * Then, call the GPIO_PinInit() function.
 *
 * This is an example to define an input pin or output pin configuration:
 * code
 * // Define a digital input pin configuration,
 * gpio_pin_config_t config =
 * {
 *   kGPIO_DigitalInput,
 *   0,
 * }
 * //Define a digital output pin configuration,
 * gpio_pin_config_t config =
 * {
 *   kGPIO_DigitalOutput,
 *   0,
 * }
 * endcode
 *
 * param base   GPIO peripheral base pointer(Typically GPIO)
 * param port   GPIO port number
 * param pin    GPIO pin number
 * param config GPIO pin configuration pointer
 */
void GPIO_PinInit(GPIO_Type *base, uint32_t port, uint32_t pin, const gpio_pin_config_t *config)
{
    if (config->pinDirection == kGPIO_DigitalInput)
    {
#if defined(FSL_FEATURE_GPIO_DIRSET_AND_DIRCLR) && (FSL_FEATURE_GPIO_DIRSET_AND_DIRCLR)
        base->DIRCLR[port] = 1U << pin;
#else
        base->DIR[port] &= ~(1U << pin);
#endif /*FSL_FEATURE_GPIO_DIRSET_AND_DIRCLR*/
    }
    else
    {
        /* Set default output value */
        if (config->outputLogic == 0U)
        {
            base->CLR[port] = (1U << pin);
        }
        else
        {
            base->SET[port] = (1U << pin);
        }
/* Set pin direction */
#if defined(FSL_FEATURE_GPIO_DIRSET_AND_DIRCLR) && (FSL_FEATURE_GPIO_DIRSET_AND_DIRCLR)
        base->DIRSET[port] = 1U << pin;
#else
        base->DIR[port] |= 1U << pin;
#endif /*FSL_FEATURE_GPIO_DIRSET_AND_DIRCLR*/
    }
}
