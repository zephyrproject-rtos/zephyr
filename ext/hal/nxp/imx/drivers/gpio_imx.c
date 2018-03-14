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

#include "gpio_imx.h"

/*******************************************************************************
 * Code
 ******************************************************************************/

/*******************************************************************************
 * GPIO Initialization and Configuration functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : GPIO_Init
 * Description   : Initializes the GPIO module according to the specified
 *                 parameters in the initConfig.
 *
 *END**************************************************************************/
void GPIO_Init(GPIO_Type* base, const gpio_init_config_t* initConfig)
{
    uint32_t pin;
    volatile uint32_t *icr;

    /* Register reset to default value */
    GPIO_IMR_REG(base) = 0;
	GPIO_EDGE_SEL_REG(base) = 0;

    /* Get pin number */
    pin = initConfig->pin;

    /* Configure GPIO pin direction */
    if (initConfig->direction == gpioDigitalOutput)
        GPIO_GDIR_REG(base) |= (1U << pin);
    else
        GPIO_GDIR_REG(base) &= ~(1U << pin);

    /* Configure GPIO pin interrupt mode */
    if(pin < 16)
        icr = &GPIO_ICR1_REG(base);
    else
    {
        icr = &GPIO_ICR2_REG(base);
        pin -= 16;
    }
    switch(initConfig->interruptMode)
    {
        case(gpioIntLowLevel):
        {
            *icr &= ~(0x3<<(2*pin));
            break;
        }
        case(gpioIntHighLevel):
        {
            *icr = (*icr & (~(0x3<<(2*pin)))) | (0x1<<(2*pin));
            break;
        }
        case(gpioIntRisingEdge):
        {
            *icr = (*icr & (~(0x3<<(2*pin)))) | (0x2<<(2*pin));
            break;
        }
        case(gpioIntFallingEdge):
        {
            *icr |= (0x3<<(2*pin));
            break;
        }
        case(gpioNoIntmode):
        {
            break;
        }
    }
}

/*******************************************************************************
 * GPIO Read and Write Functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : GPIO_WritePinOutput
 * Description   : Sets the output level of the individual GPIO pin.
 *
 *END**************************************************************************/
void GPIO_WritePinOutput(GPIO_Type* base, uint32_t pin, gpio_pin_action_t pinVal)
{
    assert(pin < 32);
    if (pinVal == gpioPinSet)
    {
        GPIO_DR_REG(base) |= (1U << pin);  /* Set pin output to high level.*/
    }
    else
    {
        GPIO_DR_REG(base) &= ~(1U << pin);  /* Set pin output to low level.*/
    }
}

/*******************************************************************************
 * Interrupts and flags management functions
 ******************************************************************************/
/*FUNCTION**********************************************************************
 *
 * Function Name : GPIO_SetPinIntMode
 * Description   : Enable or Disable the specific pin interrupt.
 *
 *END**************************************************************************/
void GPIO_SetPinIntMode(GPIO_Type* base, uint32_t pin, bool enable)
{
    assert(pin < 32);

    if(enable)
        GPIO_IMR_REG(base) |= (1U << pin);
    else
        GPIO_IMR_REG(base) &= ~(1U << pin);
}

/*FUNCTION**********************************************************************
 *
 * Function Name : GPIO_SetIntEdgeSelect
 * Description   : Enable or Disable the specific pin interrupt.
 *
 *END**************************************************************************/

void GPIO_SetIntEdgeSelect(GPIO_Type* base, uint32_t pin, bool enable)
{
    assert(pin < 32);

    if(enable)
        GPIO_EDGE_SEL_REG(base) |= (1U << pin);
    else
        GPIO_EDGE_SEL_REG(base) &= ~(1U << pin);
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
