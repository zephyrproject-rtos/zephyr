/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __GPIO_IMX_H__
#define __GPIO_IMX_H__

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "device_imx.h"

/*!
 * @addtogroup gpio_driver
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @brief GPIO direction definition. */
typedef enum _gpio_pin_direction
{
    gpioDigitalInput  = 0U, /*!< Set current pin as digital input.*/
    gpioDigitalOutput = 1U, /*!< Set current pin as digital output.*/
} gpio_pin_direction_t;

/*! @brief GPIO interrupt mode definition. */
typedef enum _gpio_interrupt_mode
{
    gpioIntLowLevel    = 0U, /*!< Set current pin interrupt is low-level sensitive.*/
    gpioIntHighLevel   = 1U, /*!< Set current pin interrupt is high-level sensitive.*/
    gpioIntRisingEdge  = 2U, /*!< Set current pin interrupt is rising-edge sensitive.*/
    gpioIntFallingEdge = 3U, /*!< Set current pin interrupt is falling-edge sensitive.*/
    gpioNoIntmode      = 4U, /*!< Set current pin general IO functionality. */
} gpio_interrupt_mode_t;

/*! @brief GPIO pin(bit) value definition. */
typedef enum _gpio_pin_action
{
    gpioPinClear = 0U, /*!< Clear GPIO Pin.*/
    gpioPinSet   = 1U, /*!< Set GPIO Pin.*/
} gpio_pin_action_t;

/*! @brief GPIO Init structure definition. */
typedef struct _gpio_init_config
{
    uint32_t              pin;           /*!< Specifies the pin number. */
    gpio_pin_direction_t  direction;     /*!< Specifies the pin direction. */
    gpio_interrupt_mode_t interruptMode; /*!< Specifies the pin interrupt mode, a value of @ref gpio_interrupt_mode_t. */
} gpio_init_config_t;

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @name GPIO Initialization and Configuration functions
 * @{
 */

/*!
 * @brief Initializes the GPIO peripheral according to the specified
 *        parameters in the initConfig.
 *
 * @param base GPIO base pointer.
 * @param initConfig pointer to a @ref gpio_init_config_t structure that
 *        contains the configuration information.
 */
void GPIO_Init(GPIO_Type* base, const gpio_init_config_t* initConfig);

/*@}*/

/*!
 * @name GPIO Read and Write Functions
 * @{
 */

/*!
 * @brief Reads the current input value of the pin when pin's direction is configured as input.
 *
 * @param base GPIO base pointer.
 * @param pin GPIO port pin number.
 * @return GPIO pin input value.
 */
static inline uint8_t GPIO_ReadPinInput(GPIO_Type* base, uint32_t pin)
{
    assert(pin < 32);

    return (uint8_t)((GPIO_DR_REG(base) >> pin) & 1U);
}

/*!
 * @brief Reads the current input value of a specific GPIO port when port's direction are all configured as input.
 *        This function  gets all 32-pin input as a 32-bit integer.
 *
 * @param base GPIO base pointer.
 * @return GPIO port input data.
 */
static inline uint32_t GPIO_ReadPortInput(GPIO_Type* base)
{
    return GPIO_DR_REG(base);
}

/*!
 * @brief Reads the current pin output.
 *
 * @param base GPIO base pointer.
 * @param pin GPIO port pin number.
 * @return Current pin output value.
 */
static inline uint8_t GPIO_ReadPinOutput(GPIO_Type* base, uint32_t pin)
{
    assert(pin < 32);

    return (uint8_t)((GPIO_DR_REG(base) >> pin) & 0x1U);
}

/*!
 * @brief Reads out all pin output status of the current port.
 *        This function operates all 32 port pins.
 *
 * @param base GPIO base pointer.
 * @return Current port output status.
 */
static inline uint32_t GPIO_ReadPortOutput(GPIO_Type* base)
{
    return GPIO_DR_REG(base);
}

/*!
 * @brief Sets the output level of the individual GPIO pin to logic 1 or 0.
 *
 * @param base GPIO base pointer.
 * @param pin GPIO port pin number.
 * @param pinVal pin output value (See @ref gpio_pin_action_t structure).
 */
void GPIO_WritePinOutput(GPIO_Type* base, uint32_t pin, gpio_pin_action_t pinVal);

/*!
 * @brief Sets the output of the GPIO port pins to a specific logic value.
 *         This function  operates all 32 port pins.
 *
 * @param base GPIO base pointer.
 * @param portVal data to configure the GPIO output.
 */
static inline void GPIO_WritePortOutput(GPIO_Type* base, uint32_t portVal)
{
    GPIO_DR_REG(base) = portVal;
}

/*@}*/

/*!
 * @name GPIO Read Pad Status Functions
 * @{
 */

 /*!
 * @brief Reads the current GPIO pin pad status.
 *
 * @param base GPIO base pointer.
 * @param pin GPIO port pin number.
 * @return GPIO pin pad status value.
 */
static inline uint8_t GPIO_ReadPadStatus(GPIO_Type* base, uint32_t pin)
{
    assert(pin < 32);

    return (uint8_t)((GPIO_PSR_REG(base) >> pin) & 1U);
}

/*@}*/

/*!
 * @name Interrupts and flags management functions
 * @{
 */

/*!
 * @brief Enable or Disable the specific pin interrupt.
 *
 * @param base GPIO base pointer.
 * @param pin GPIO pin number.
 * @param enable Enable or disable interrupt.
 *               - true: Enable GPIO interrupt.
 *               - false: Disable GPIO interrupt.
 */
void GPIO_SetPinIntMode(GPIO_Type* base, uint32_t pin, bool enable);

/*!
 * @brief Check individual pin interrupt status.
 *
 * @param base GPIO base pointer.
 * @param pin GPIO port pin number.
 * @return current pin interrupt status flag.
 */
static inline bool GPIO_IsIntPending(GPIO_Type* base, uint32_t pin)
{
    assert(pin < 32);

    return (bool)((GPIO_ISR_REG(base) >> pin) & 1U);
}

/*!
 * @brief Clear pin interrupt flag. Status flags are cleared by
 *        writing a 1 to the corresponding bit position.
 *
 * @param base GPIO base pointer.
 * @param pin GPIO port pin number.
 */
static inline void GPIO_ClearStatusFlag(GPIO_Type* base, uint32_t pin)
{
    assert(pin < 32);

    GPIO_ISR_REG(base) = (1U << pin);
}

/*!
 * @brief Enable or disable the edge select bit to override
 *        the ICR register's configuration.
 *
 * @param base GPIO base pointer.
 * @param pin GPIO port pin number.
 * @param enable Enable or disable edge select bit.
 */
void GPIO_SetIntEdgeSelect(GPIO_Type* base, uint32_t pin, bool enable);

/*@}*/

#if defined(__cplusplus)
}
#endif

/*! @} */

#endif /* __GPIO_IMX_H__*/

/*******************************************************************************
 * EOF
 ******************************************************************************/
