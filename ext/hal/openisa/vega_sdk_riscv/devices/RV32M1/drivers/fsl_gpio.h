/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_GPIO_H_
#define _FSL_GPIO_H_

#include "fsl_common.h"

/*!
 * @addtogroup gpio
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/*! @name Driver version */
/*@{*/
/*! @brief GPIO driver version 2.2.0. */
#define FSL_GPIO_DRIVER_VERSION (MAKE_VERSION(2, 2, 0))
/*@}*/

/*! @brief GPIO direction definition */
typedef enum _gpio_pin_direction
{
    kGPIO_DigitalInput = 0U,  /*!< Set current pin as digital input*/
    kGPIO_DigitalOutput = 1U, /*!< Set current pin as digital output*/
} gpio_pin_direction_t;

#if defined(FSL_FEATURE_GPIO_HAS_ATTRIBUTE_CHECKER) && FSL_FEATURE_GPIO_HAS_ATTRIBUTE_CHECKER
/*! @brief GPIO checker attribute */
typedef enum _gpio_checker_attribute
{
    kGPIO_UsernonsecureRWUsersecureRWPrivilegedsecureRW =
        0x00U, /*!< User nonsecure:Read+Write; User Secure:Read+Write; Privileged Secure:Read+Write */
    kGPIO_UsernonsecureRUsersecureRWPrivilegedsecureRW =
        0x01U, /*!< User nonsecure:Read;       User Secure:Read+Write; Privileged Secure:Read+Write */
    kGPIO_UsernonsecureNUsersecureRWPrivilegedsecureRW =
        0x02U, /*!< User nonsecure:None;       User Secure:Read+Write; Privileged Secure:Read+Write */
    kGPIO_UsernonsecureRUsersecureRPrivilegedsecureRW =
        0x03U, /*!< User nonsecure:Read;       User Secure:Read;       Privileged Secure:Read+Write */
    kGPIO_UsernonsecureNUsersecureRPrivilegedsecureRW =
        0x04U, /*!< User nonsecure:None;       User Secure:Read;       Privileged Secure:Read+Write */
    kGPIO_UsernonsecureNUsersecureNPrivilegedsecureRW =
        0x05U, /*!< User nonsecure:None;       User Secure:None;       Privileged Secure:Read+Write */
    kGPIO_UsernonsecureNUsersecureNPrivilegedsecureR =
        0x06U, /*!< User nonsecure:None;       User Secure:None;       Privileged Secure:Read */
    kGPIO_UsernonsecureNUsersecureNPrivilegedsecureN =
        0x07U, /*!< User nonsecure:None;       User Secure:None;       Privileged Secure:None */
    kGPIO_IgnoreAttributeCheck = 0x80U, /*!< Ignores the attribute check */
} gpio_checker_attribute_t;
#endif

/*!
 * @brief The GPIO pin configuration structure.
 *
 * Each pin can only be configured as either an output pin or an input pin at a time.
 * If configured as an input pin, leave the outputConfig unused.
 * Note that in some use cases, the corresponding port property should be configured in advance
 *        with the PORT_SetPinConfig().
 */
typedef struct z_gpio_pin_config
{
    gpio_pin_direction_t pinDirection; /*!< GPIO direction, input or output */
    /* Output configurations; ignore if configured as an input pin */
    uint8_t outputLogic; /*!< Set a default output logic, which has no use in input */
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
 * To initialize the GPIO, define a pin configuration, as either input or output, in the user file.
 * Then, call the GPIO_PinInit() function.
 *
 * This is an example to define an input pin or an output pin configuration.
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
 * @param base   GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
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
 * @param base    GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
 * @param pin     GPIO pin number
 * @param output  GPIO pin output logic level.
 *        - 0: corresponding pin output low-logic level.
 *        - 1: corresponding pin output high-logic level.
 */
static inline void GPIO_WritePinOutput(GPIO_Type *base, uint32_t pin, uint8_t output)
{
    if (output == 0U)
    {
        base->PCOR = 1U << pin;
    }
    else
    {
        base->PSOR = 1U << pin;
    }
}

/*!
 * @brief Sets the output level of the multiple GPIO pins to the logic 1.
 *
 * @param base GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pin number macro
 */
static inline void GPIO_SetPinsOutput(GPIO_Type *base, uint32_t mask)
{
    base->PSOR = mask;
}

/*!
 * @brief Sets the output level of the multiple GPIO pins to the logic 0.
 *
 * @param base GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pin number macro
 */
static inline void GPIO_ClearPinsOutput(GPIO_Type *base, uint32_t mask)
{
    base->PCOR = mask;
}

/*!
 * @brief Reverses the current output logic of the multiple GPIO pins.
 *
 * @param base GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pin number macro
 */
static inline void GPIO_TogglePinsOutput(GPIO_Type *base, uint32_t mask)
{
    base->PTOR = mask;
}
/*@}*/

/*! @name GPIO Input Operations */
/*@{*/

/*!
 * @brief Reads the current input value of the GPIO port.
 *
 * @param base GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
 * @param pin     GPIO pin number
 * @retval GPIO port input value
 *        - 0: corresponding pin input low-logic level.
 *        - 1: corresponding pin input high-logic level.
 */
static inline uint32_t GPIO_ReadPinInput(GPIO_Type *base, uint32_t pin)
{
    return (((base->PDIR) >> pin) & 0x01U);
}
/*@}*/

/*! @name GPIO Interrupt */
/*@{*/

/*!
 * @brief Reads the GPIO port interrupt status flag.
 *
 * If a pin is configured to generate the DMA request, the corresponding flag
 * is cleared automatically at the completion of the requested DMA transfer.
 * Otherwise, the flag remains set until a logic one is written to that flag.
 * If configured for a level sensitive interrupt that remains asserted, the flag
 * is set again immediately.
 *
 * @param base GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
 * @retval The current GPIO port interrupt status flag, for example, 0x00010001 means the
 *         pin 0 and 17 have the interrupt.
 */
uint32_t GPIO_GetPinsInterruptFlags(GPIO_Type *base);

/*!
 * @brief Clears multiple GPIO pin interrupt status flags.
 *
 * @param base GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pin number macro
 */
void GPIO_ClearPinsInterruptFlags(GPIO_Type *base, uint32_t mask);

#if defined(FSL_FEATURE_GPIO_HAS_ATTRIBUTE_CHECKER) && FSL_FEATURE_GPIO_HAS_ATTRIBUTE_CHECKER
/*!
 * @brief The GPIO module supports a device-specific number of data ports, organized as 32-bit
 * words. Each 32-bit data port includes a GACR register, which defines the byte-level
 * attributes required for a successful access to the GPIO programming model. The attribute controls for the 4 data
 * bytes in the GACR follow a standard little endian
 * data convention.
 *
 * @param base GPIO peripheral base pointer (GPIOA, GPIOB, GPIOC, and so on.)
 * @param mask GPIO pin number macro
 */
void GPIO_CheckAttributeBytes(GPIO_Type *base, gpio_checker_attribute_t attribute);
#endif

/*@}*/
/*! @} */

/*!
 * @addtogroup fgpio_driver
 * @{
 */

/*
 * Introduces the FGPIO feature.
 *
 * The FGPIO features are only support on some Kinetis MCUs. The FGPIO registers are aliased to the IOPORT
 * interface. Accesses via the IOPORT interface occur in parallel with any instruction fetches and
 * complete in a single cycle. This aliased Fast GPIO memory map is called FGPIO.
 */

#if defined(FSL_FEATURE_SOC_FGPIO_COUNT) && FSL_FEATURE_SOC_FGPIO_COUNT

/*! @name FGPIO Configuration */
/*@{*/

/*!
 * @brief Initializes the FGPIO peripheral.
 *
 * This function ungates the FGPIO clock.
 *
 * @param base   FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 */
 void FGPIO_Init(FGPIO_Type *base);

/*!
 * @brief Initializes a FGPIO pin used by the board.
 *
 * To initialize the FGPIO driver, define a pin configuration, as either input or output, in the user file.
 * Then, call the FGPIO_PinInit() function.
 *
 * This is an example to define an input pin or an output pin configuration:
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
 * @param base   FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
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
 * @param base    FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 * @param pin     FGPIO pin number
 * @param output  FGPIOpin output logic level.
 *        - 0: corresponding pin output low-logic level.
 *        - 1: corresponding pin output high-logic level.
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
 * @param base FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 * @param mask FGPIO pin number macro
 */
static inline void FGPIO_SetPinsOutput(FGPIO_Type *base, uint32_t mask)
{
    base->PSOR = mask;
}

/*!
 * @brief Sets the output level of the multiple FGPIO pins to the logic 0.
 *
 * @param base FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 * @param mask FGPIO pin number macro
 */
static inline void FGPIO_ClearPinsOutput(FGPIO_Type *base, uint32_t mask)
{
    base->PCOR = mask;
}

/*!
 * @brief Reverses the current output logic of the multiple FGPIO pins.
 *
 * @param base FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 * @param mask FGPIO pin number macro
 */
static inline void FGPIO_TogglePinsOutput(FGPIO_Type *base, uint32_t mask)
{
    base->PTOR = mask;
}
/*@}*/

/*! @name FGPIO Input Operations */
/*@{*/

/*!
 * @brief Reads the current input value of the FGPIO port.
 *
 * @param base FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 * @param pin  FGPIO pin number
 * @retval FGPIO port input value
 *        - 0: corresponding pin input low-logic level.
 *        - 1: corresponding pin input high-logic level.
 */
static inline uint32_t FGPIO_ReadPinInput(FGPIO_Type *base, uint32_t pin)
{
    return (((base->PDIR) >> pin) & 0x01U);
}
/*@}*/

/*! @name FGPIO Interrupt */
/*@{*/

/*!
 * @brief Reads the FGPIO port interrupt status flag.
 *
 * If a pin is configured to generate the DMA request, the corresponding flag
 * is cleared automatically at the completion of the requested DMA transfer.
 * Otherwise, the flag remains set until a logic one is written to that flag.
 * If configured for a level-sensitive interrupt that remains asserted, the flag
 * is set again immediately.
 *
 * @param base FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 * @retval The current FGPIO port interrupt status flags, for example, 0x00010001 means the
 *         pin 0 and 17 have the interrupt.
 */
uint32_t FGPIO_GetPinsInterruptFlags(FGPIO_Type *base);

/*!
 * @brief Clears the multiple FGPIO pin interrupt status flag.
 *
 * @param base FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 * @param mask FGPIO pin number macro
 */
void FGPIO_ClearPinsInterruptFlags(FGPIO_Type *base, uint32_t mask);

#if defined(FSL_FEATURE_GPIO_HAS_ATTRIBUTE_CHECKER) && FSL_FEATURE_GPIO_HAS_ATTRIBUTE_CHECKER
/*!
 * @brief The FGPIO module supports a device-specific number of data ports, organized as 32-bit
 * words. Each 32-bit data port includes a GACR register, which defines the byte-level
 * attributes required for a successful access to the GPIO programming model. The attribute controls for the 4 data
 * bytes in the GACR follow a standard little endian
 * data convention.
 *
 * @param base FGPIO peripheral base pointer (FGPIOA, FGPIOB, FGPIOC, and so on.)
 * @param mask FGPIO pin number macro
 */
void FGPIO_CheckAttributeBytes(FGPIO_Type *base, gpio_checker_attribute_t attribute);
#endif

/*@}*/

#endif /* FSL_FEATURE_SOC_FGPIO_COUNT */

#if defined(__cplusplus)
}
#endif

/*!
 * @}
 */

#endif /* _FSL_GPIO_H_*/
