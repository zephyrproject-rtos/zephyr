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
 * DISCLAIMED. IN NO EVENT SDRVL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _FSL_GPIO_H_
#define _FSL_GPIO_H_

#include "fsl_common.h"

/*!
 * @addtogroup gpio
 * @{
 */

/*! @file */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief GPIO driver version 2.1.0. */
#define FSL_GPIO_DRIVER_VERSION (MAKE_VERSION(2, 1, 0))
/*@}*/

/*! @brief GPIO direction definition*/
typedef enum _gpio_pin_direction
{
    kGPIO_DigitalInput = 0U,  /*!< Set current pin as digital input*/
    kGPIO_DigitalOutput = 1U, /*!< Set current pin as digital output*/
} gpio_pin_direction_t;

/*!
 * @brief The GPIO pin configuration structure.
 *
 * Every pin can only be configured as either output pin or input pin at a time.
 * If configured as a input pin, then leave the outputConfig unused
 * Note : In some cases, the corresponding port property should be configured in advance
 *        with the PORT_SetPinConfig()
 */
typedef struct _gpio_pin_config
{
    gpio_pin_direction_t pinDirection; /*!< gpio direction, input or output */
    /* Output configurations, please ignore if configured as a input one */
    uint8_t outputLogic; /*!< Set default output logic, no use in input */
} gpio_pin_config_t;

/*! @} */

/*******************************************************************************
 * API
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif

/*!
 * @addtogroup gpio_driver
 * @{
 */

/*! @name GPIO Configuration */
/*@{*/

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
 * @param base   GPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param pin    GPIO port pin number
 * @param config GPIO pin configuration pointer
 */
void GPIO_PinInit(GPIO_Type *base, uint32_t pin, const gpio_pin_config_t *config);

/*@}*/

/*! @name GPIO Output Operations */
/*@{*/

/*!
 * @brief Sets the output level of the multiple GPIO pins to the logic 1 or 0.
 *
 * @param base    GPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param pin     GPIO pin's number
 * @param output  GPIO pin output logic level.
 *        - 0: corresponding pin output low logic level.
 *        - 1: corresponding pin output high logic level.
 */
static inline void GPIO_WritePinOutput(GPIO_Type *base, uint32_t pin, uint8_t output)
{
    if (output == 0U)
    {
        base->PCOR = 1 << pin;
    }
    else
    {
        base->PSOR = 1 << pin;
    }
}

/*!
 * @brief Sets the output level of the multiple GPIO pins to the logic 1.
 *
 * @param base GPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pins' numbers macro
 */
static inline void GPIO_SetPinsOutput(GPIO_Type *base, uint32_t mask)
{
    base->PSOR = mask;
}

/*!
 * @brief Sets the output level of the multiple GPIO pins to the logic 0.
 *
 * @param base GPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pins' numbers macro
 */
static inline void GPIO_ClearPinsOutput(GPIO_Type *base, uint32_t mask)
{
    base->PCOR = mask;
}

/*!
 * @brief Reverses current output logic of the multiple GPIO pins.
 *
 * @param base GPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pins' numbers macro
 */
static inline void GPIO_TogglePinsOutput(GPIO_Type *base, uint32_t mask)
{
    base->PTOR = mask;
}
/*@}*/

/*! @name GPIO Input Operations */
/*@{*/

/*!
 * @brief Reads the current input value of the whole GPIO port.
 *
 * @param base GPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param pin     GPIO pin's number
 * @retval GPIO port input value
 *        - 0: corresponding pin input low logic level.
 *        - 1: corresponding pin input high logic level.
 */
static inline uint32_t GPIO_ReadPinInput(GPIO_Type *base, uint32_t pin)
{
    return (((base->PDIR) >> pin) & 0x01U);
}
/*@}*/

/*! @name GPIO Interrupt */
/*@{*/

/*!
 * @brief Reads whole GPIO port interrupt status flag.
 *
 * If a pin is configured to generate the DMA request, the corresponding flag
 * is cleared automatically at the completion of the requested DMA transfer.
 * Otherwise, the flag remains set until a logic one is written to that flag.
 * If configured for a level sensitive interrupt that remains asserted, the flag
 * is set again immediately.
 *
 * @param base GPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @retval Current GPIO port interrupt status flag, for example, 0x00010001 means the
 *         pin 0 and 17 have the interrupt.
 */
uint32_t GPIO_GetPinsInterruptFlags(GPIO_Type *base);

/*!
 * @brief Clears multiple GPIO pins' interrupt status flag.
 *
 * @param base GPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pins' numbers macro
 */
void GPIO_ClearPinsInterruptFlags(GPIO_Type *base, uint32_t mask);

/*@}*/
/*! @} */

/*!
 * @addtogroup fgpio_driver
 * @{
 */

/*
 * Introduce the FGPIO feature.
 *
 * The FGPIO features are only support on some of Kinetis chips. The FGPIO registers are aliased to the IOPORT
 * interface. Accesses via the IOPORT interface occur in parallel with any instruction fetches and will therefore
 * complete in a single cycle. This aliased Fast GPIO memory map is called FGPIO.
 */

#if defined(FSL_FEATURE_SOC_FGPIO_COUNT) && FSL_FEATURE_SOC_FGPIO_COUNT

/*! @name FGPIO Configuration */
/*@{*/

/*!
 * @brief Initializes a FGPIO pin used by the board.
 *
 * To initialize the FGPIO driver, define a pin configuration, either input or output, in the user file.
 * Then, call the FGPIO_PinInit() function.
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
 * @param base   FGPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param pin    FGPIO port pin number
 * @param config FGPIO pin configuration pointer
 */
void FGPIO_PinInit(FGPIO_Type *base, uint32_t pin, const gpio_pin_config_t *config);

/*@}*/

/*! @name FGPIO Output Operations */
/*@{*/

/*!
 * @brief Sets the output level of the multiple FGPIO pins to the logic 1 or 0.
 *
 * @param base    FGPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param pin     FGPIO pin's number
 * @param output  FGPIOpin output logic level.
 *        - 0: corresponding pin output low logic level.
 *        - 1: corresponding pin output high logic level.
 */
static inline void FGPIO_WritePinOutput(FGPIO_Type *base, uint32_t pin, uint8_t output)
{
    if (output == 0U)
    {
        base->PCOR = 1 << pin;
    }
    else
    {
        base->PSOR = 1 << pin;
    }
}

/*!
 * @brief Sets the output level of the multiple FGPIO pins to the logic 1.
 *
 * @param base FGPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask FGPIO pins' numbers macro
 */
static inline void FGPIO_SetPinsOutput(FGPIO_Type *base, uint32_t mask)
{
    base->PSOR = mask;
}

/*!
 * @brief Sets the output level of the multiple FGPIO pins to the logic 0.
 *
 * @param base FGPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask FGPIO pins' numbers macro
 */
static inline void FGPIO_ClearPinsOutput(FGPIO_Type *base, uint32_t mask)
{
    base->PCOR = mask;
}

/*!
 * @brief Reverses current output logic of the multiple FGPIO pins.
 *
 * @param base FGPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask FGPIO pins' numbers macro
 */
static inline void FGPIO_TogglePinsOutput(FGPIO_Type *base, uint32_t mask)
{
    base->PTOR = mask;
}
/*@}*/

/*! @name FGPIO Input Operations */
/*@{*/

/*!
 * @brief Reads the current input value of the whole FGPIO port.
 *
 * @param base FGPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param pin  FGPIO pin's number
 * @retval FGPIO port input value
 *        - 0: corresponding pin input low logic level.
 *        - 1: corresponding pin input high logic level.
 */
static inline uint32_t FGPIO_ReadPinInput(FGPIO_Type *base, uint32_t pin)
{
    return (((base->PDIR) >> pin) & 0x01U);
}
/*@}*/

/*! @name FGPIO Interrupt */
/*@{*/

/*!
 * @brief Reads the whole FGPIO port interrupt status flag.
 *
 * If a pin is configured to generate the DMA request,  the corresponding flag
 * is cleared automatically at the completion of the requested DMA transfer.
 * Otherwise, the flag remains set until a logic one is written to that flag.
 * If configured for a level sensitive interrupt that remains asserted, the flag
 * is set again immediately.
 *
 * @param base FGPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @retval Current FGPIO port interrupt status flags, for example, 0x00010001 means the
 *         pin 0 and 17 have the interrupt.
 */
uint32_t FGPIO_GetPinsInterruptFlags(FGPIO_Type *base);

/*!
 * @brief Clears the multiple FGPIO pins' interrupt status flag.
 *
 * @param base FGPIO peripheral base pointer(GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask FGPIO pins' numbers macro
 */
void FGPIO_ClearPinsInterruptFlags(FGPIO_Type *base, uint32_t mask);

/*@}*/

#endif /* FSL_FEATURE_SOC_FGPIO_COUNT */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */

#endif /* _FSL_GPIO_H_*/
