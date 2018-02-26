/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
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
 * o Neither the name of the copyright holder nor the names of its
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

#include "fsl_gpio.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/

/* Array of GPIO peripheral base address. */
static GPIO_Type *const s_gpioBases[] = GPIO_BASE_PTRS;

#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
/* Array of GPIO clock name. */
static const clock_ip_name_t s_gpioClock[] = GPIO_CLOCKS;
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

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

void GPIO_PinInit(GPIO_Type* base, uint32_t pin, const gpio_pin_config_t* Config)
{
#if !(defined(FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL) && FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL)
    /* Enable GPIO clock. */
    CLOCK_EnableClock(s_gpioClock[GPIO_GetInstance(base)]);
#endif /* FSL_SDK_DISABLE_DRIVER_CLOCK_CONTROL */

    /* Register reset to default value */
    base->IMR &= ~(1U << pin);

    /* Configure GPIO pin direction */
    if (Config->direction == kGPIO_DigitalInput)
    {
        base->GDIR &= ~(1U << pin);
    }
    else
    {
        GPIO_WritePinOutput(base, pin, Config->outputLogic);
        base->GDIR |= (1U << pin);
    }

    /* Configure GPIO pin interrupt mode */
    GPIO_SetPinInterruptConfig(base, pin, Config->interruptMode);
}

void GPIO_PinWrite(GPIO_Type* base, uint32_t pin, uint8_t output)
{
    assert(pin < 32);
    if (output == 0U)
    {
        base->DR &= ~(1U << pin);  /* Set pin output to low level.*/
    }
    else
    {
        base->DR |= (1U << pin);  /* Set pin output to high level.*/
    }
}

void GPIO_PinSetInterruptConfig(GPIO_Type* base, uint32_t pin, gpio_interrupt_mode_t pinInterruptMode)
{
    volatile uint32_t *icr;
    uint32_t icrShift;

    icrShift = pin;

    /* Register reset to default value */
    base->EDGE_SEL &= ~(1U << pin);

    if(pin < 16)
    {
        icr = &(base->ICR1);
    }
    else
    {
        icr = &(base->ICR2);
        icrShift -= 16;
    }
    switch(pinInterruptMode)
    {
        case(kGPIO_IntLowLevel):
            *icr &= ~(3U << (2 * icrShift));
            break;
        case(kGPIO_IntHighLevel):
            *icr = (*icr & (~(3U << (2 * icrShift)))) | (1U << (2 * icrShift));
            break;
        case(kGPIO_IntRisingEdge):
            *icr = (*icr & (~(3U << (2 * icrShift)))) | (2U << (2 * icrShift));
            break;
        case(kGPIO_IntFallingEdge):
            *icr |= (3U << (2 * icrShift));
            break;
        case(kGPIO_IntRisingOrFallingEdge):
            base->EDGE_SEL |= (1U << pin);
            break;
        default:
            break;
    }
}
