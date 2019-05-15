/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _LPC_GPIO_H_
#define _LPC_GPIO_H_

#include "fsl_common.h"

/*!
 * @addtogroup lpc_gpio
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief LPC GPIO driver version 2.1.3. */
#define FSL_GPIO_DRIVER_VERSION (MAKE_VERSION(2, 1, 3))
/*@}*/

/*! @brief LPC GPIO direction definition */
typedef enum _gpio_pin_direction
{
    kGPIO_DigitalInput = 0U,  /*!< Set current pin as digital input*/
    kGPIO_DigitalOutput = 1U, /*!< Set current pin as digital output*/
} gpio_pin_direction_t;

/*!
 * @brief The GPIO pin configuration structure.
 *
 * Every pin can only be configured as either output pin or input pin at a time.
 * If configured as a input pin, then leave the outputConfig unused.
 */
typedef struct _gpio_pin_config
{
    gpio_pin_direction_t pinDirection; /*!< GPIO direction, input or output */
    /* Output configurations, please ignore if configured as a input one */
    uint8_t outputLogic; /*!< Set default output logic, no use in input */
} gpio_pin_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/
#if defined(__cplusplus)
extern "C" {
#endif

/*! @name GPIO Configuration */
/*@{*/

/*!
 * @brief Initializes the GPIO peripheral.
 *
 * This function ungates the GPIO clock.
 *
 * @param base   GPIO peripheral base pointer.
 * @param port   GPIO port number.
 */
void GPIO_PortInit(GPIO_Type *base, uint32_t port);

/*!
 * @brief Initializes a GPIO pin used by the board.
 *
 * To initialize the GPIO, define a pin configuration, either input or output, in the user file.
 * Then, call the GPIO_PinInit() function.
 *
 * This is an example to define an input pin or output pin configuration:
 * @code
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
 * @endcode
 *
 * @param base   GPIO peripheral base pointer(Typically GPIO)
 * @param port   GPIO port number
 * @param pin    GPIO pin number
 * @param config GPIO pin configuration pointer
 */
void GPIO_PinInit(GPIO_Type *base, uint32_t port, uint32_t pin, const gpio_pin_config_t *config);

/*@}*/

/*! @name GPIO Output Operations */
/*@{*/

/*!
 * @brief Sets the output level of the one GPIO pin to the logic 1 or 0.
 *
 * @param base    GPIO peripheral base pointer(Typically GPIO)
 * @param port   GPIO port number
 * @param pin    GPIO pin number
 * @param output  GPIO pin output logic level.
 *        - 0: corresponding pin output low-logic level.
 *        - 1: corresponding pin output high-logic level.
 */
static inline void GPIO_PinWrite(GPIO_Type *base, uint32_t port, uint32_t pin, uint8_t output)
{
    base->B[port][pin] = output;
}

/*@}*/
/*! @name GPIO Input Operations */
/*@{*/

/*!
 * @brief Reads the current input value of the GPIO PIN.
 *
 * @param base GPIO peripheral base pointer(Typically GPIO)
 * @param port   GPIO port number
 * @param pin    GPIO pin number
 * @retval GPIO port input value
 *        - 0: corresponding pin input low-logic level.
 *        - 1: corresponding pin input high-logic level.
 */
static inline uint32_t GPIO_PinRead(GPIO_Type *base, uint32_t port, uint32_t pin)
{
    return (uint32_t)base->B[port][pin];
}

/*@}*/

/*!
 * @brief Sets the output level of the multiple GPIO pins to the logic 1.
 *
 * @param base GPIO peripheral base pointer(Typically GPIO)
 * @param port GPIO port number
 * @param mask GPIO pin number macro
 */
static inline void GPIO_PortSet(GPIO_Type *base, uint32_t port, uint32_t mask)
{
    base->SET[port] = mask;
}

/*!
 * @brief Sets the output level of the multiple GPIO pins to the logic 0.
 *
 * @param base GPIO peripheral base pointer(Typically GPIO)
 * @param port GPIO port number
 * @param mask GPIO pin number macro
 */
static inline void GPIO_PortClear(GPIO_Type *base, uint32_t port, uint32_t mask)
{
    base->CLR[port] = mask;
}

/*!
 * @brief Reverses current output logic of the multiple GPIO pins.
 *
 * @param base GPIO peripheral base pointer(Typically GPIO)
 * @param port GPIO port number
 * @param mask GPIO pin number macro
 */
static inline void GPIO_PortToggle(GPIO_Type *base, uint32_t port, uint32_t mask)
{
    base->NOT[port] = mask;
}

/*@}*/

/*!
 * @brief Reads the current input value of the whole GPIO port.
 *
 * @param base GPIO peripheral base pointer(Typically GPIO)
 * @param port GPIO port number
 */
static inline uint32_t GPIO_PortRead(GPIO_Type *base, uint32_t port)
{
    return (uint32_t)base->PIN[port];
}

/*@}*/
/*! @name GPIO Mask Operations */
/*@{*/

/*!
 * @brief Sets port mask, 0 - enable pin, 1 - disable pin.
 *
 * @param base GPIO peripheral base pointer(Typically GPIO)
 * @param port GPIO port number
 * @param mask GPIO pin number macro
 */
static inline void GPIO_PortMaskedSet(GPIO_Type *base, uint32_t port, uint32_t mask)
{
    base->MASK[port] = mask;
}

/*!
 * @brief Sets the output level of the masked GPIO port. Only pins enabled by GPIO_SetPortMask() will be affected.
 *
 * @param base    GPIO peripheral base pointer(Typically GPIO)
 * @param port   GPIO port number
 * @param output  GPIO port output value.
 */
static inline void GPIO_PortMaskedWrite(GPIO_Type *base, uint32_t port, uint32_t output)
{
    base->MPIN[port] = output;
}

/*!
 * @brief Reads the current input value of the masked GPIO port. Only pins enabled by GPIO_SetPortMask() will be
 * affected.
 *
 * @param base   GPIO peripheral base pointer(Typically GPIO)
 * @param port   GPIO port number
 * @retval       masked GPIO port value
 */
static inline uint32_t GPIO_PortMaskedRead(GPIO_Type *base, uint32_t port)
{
    return (uint32_t)base->MPIN[port];
}

/*@}*/

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */

#endif /* _LPC_GPIO_H_*/
