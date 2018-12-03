/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_gpio.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
static PORT_Type *const s_portBases[] = PORT_BASE_PTRS;
static GPIO_Type *const s_gpioBases[] = GPIO_BASE_PTRS;

#if defined(FSL_FEATURE_SOC_FGPIO_COUNT) && FSL_FEATURE_SOC_FGPIO_COUNT

#if defined(FSL_FEATURE_PCC_HAS_FGPIO_CLOCK_GATE_CONTROL) && FSL_FEATURE_PCC_HAS_FGPIO_CLOCK_GATE_CONTROL

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/*! @brief Array to map FGPIO instance number to clock name. */
static const clock_ip_name_t s_fgpioClockName[] = FGPIO_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

#endif /* FSL_FEATURE_PCC_HAS_FGPIO_CLOCK_GATE_CONTROL */

#endif /* FSL_FEATURE_SOC_FGPIO_COUNT */

/*******************************************************************************
* Prototypes
******************************************************************************/

/*!
* @brief Gets the GPIO instance according to the GPIO base
*
* @param base    GPIO peripheral base pointer(PTA, PTB, PTC, etc.)
* @retval GPIO instance
*/
static uint32_t GPIO_GetInstance(GPIO_Type *base);

/*******************************************************************************
 * Code
 ******************************************************************************/

static uint32_t GPIO_GetInstance(GPIO_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_gpioBases); instance++)
    {
        if (s_gpioBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_gpioBases));

    return instance;
}

void GPIO_PinInit(GPIO_Type *base, uint32_t pin, const gpio_pin_config_t *config)
{
    assert(config);

    if (config->pinDirection == kGPIO_DigitalInput)
    {
        base->PDDR &= ~(1U << pin);
    }
    else
    {
        GPIO_WritePinOutput(base, pin, config->outputLogic);
        base->PDDR |= (1U << pin);
    }
}

uint32_t GPIO_GetPinsInterruptFlags(GPIO_Type *base)
{
    uint8_t instance;
    PORT_Type *portBase;
    instance = GPIO_GetInstance(base);
    portBase = s_portBases[instance];
    return portBase->ISFR;
}

void GPIO_ClearPinsInterruptFlags(GPIO_Type *base, uint32_t mask)
{
    uint8_t instance;
    PORT_Type *portBase;
    instance = GPIO_GetInstance(base);
    portBase = s_portBases[instance];
    portBase->ISFR = mask;
}

#if defined(FSL_FEATURE_GPIO_HAS_ATTRIBUTE_CHECKER) && FSL_FEATURE_GPIO_HAS_ATTRIBUTE_CHECKER
void GPIO_CheckAttributeBytes(GPIO_Type *base, gpio_checker_attribute_t attribute)
{
    base->GACR = ((uint32_t)attribute << GPIO_GACR_ACB0_SHIFT) | ((uint32_t)attribute << GPIO_GACR_ACB1_SHIFT) |
                 ((uint32_t)attribute << GPIO_GACR_ACB2_SHIFT) | ((uint32_t)attribute << GPIO_GACR_ACB3_SHIFT);
}
#endif

#if defined(FSL_FEATURE_SOC_FGPIO_COUNT) && FSL_FEATURE_SOC_FGPIO_COUNT

/*******************************************************************************
 * Variables
 ******************************************************************************/
static FGPIO_Type *const s_fgpioBases[] = FGPIO_BASE_PTRS;

/*******************************************************************************
* Prototypes
******************************************************************************/
/*!
* @brief Gets the FGPIO instance according to the GPIO base
*
* @param base    FGPIO peripheral base pointer(PTA, PTB, PTC, etc.)
* @retval FGPIO instance
*/
static uint32_t FGPIO_GetInstance(FGPIO_Type *base);

/*******************************************************************************
 * Code
 ******************************************************************************/

static uint32_t FGPIO_GetInstance(FGPIO_Type *base)
{
    uint32_t instance;

    /* Find the instance index from base address mappings. */
    for (instance = 0; instance < ARRAY_SIZE(s_fgpioBases); instance++)
    {
        if (s_fgpioBases[instance] == base)
        {
            break;
        }
    }

    assert(instance < ARRAY_SIZE(s_fgpioBases));

    return instance;
}

#if defined(FSL_FEATURE_PCC_HAS_FGPIO_CLOCK_GATE_CONTROL) && FSL_FEATURE_PCC_HAS_FGPIO_CLOCK_GATE_CONTROL
void FGPIO_Init(FGPIO_Type *base)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Ungate FGPIO periphral clock */
    CLOCK_EnableClock(s_fgpioClockName[FGPIO_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */
}
#endif /* FSL_FEATURE_PCC_HAS_FGPIO_CLOCK_GATE_CONTROL */

void FGPIO_PinInit(FGPIO_Type *base, uint32_t pin, const gpio_pin_config_t *config)
{
    assert(config);

    if (config->pinDirection == kGPIO_DigitalInput)
    {
        base->PDDR &= ~(1U << pin);
    }
    else
    {
        FGPIO_WritePinOutput(base, pin, config->outputLogic);
        base->PDDR |= (1U << pin);
    }
}

uint32_t FGPIO_GetPinsInterruptFlags(FGPIO_Type *base)
{
    uint8_t instance;
    instance = FGPIO_GetInstance(base);
    PORT_Type *portBase;
    portBase = s_portBases[instance];
    return portBase->ISFR;
}

void FGPIO_ClearPinsInterruptFlags(FGPIO_Type *base, uint32_t mask)
{
    uint8_t instance;
    instance = FGPIO_GetInstance(base);
    PORT_Type *portBase;
    portBase = s_portBases[instance];
    portBase->ISFR = mask;
}

#if defined(FSL_FEATURE_FGPIO_HAS_ATTRIBUTE_CHECKER) && FSL_FEATURE_FGPIO_HAS_ATTRIBUTE_CHECKER
void FGPIO_CheckAttributeBytes(FGPIO_Type *base, gpio_checker_attribute_t attribute)
{
    base->GACR = (attribute << FGPIO_GACR_ACB0_SHIFT) | (attribute << FGPIO_GACR_ACB1_SHIFT) |
                 (attribute << FGPIO_GACR_ACB2_SHIFT) | (attribute << FGPIO_GACR_ACB3_SHIFT);
}
#endif

#endif /* FSL_FEATURE_SOC_FGPIO_COUNT */
