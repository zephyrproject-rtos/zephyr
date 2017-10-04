/* --COPYRIGHT--,BSD
 * Copyright (c) 2017, Texas Instruments Incorporated
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
 * --/COPYRIGHT--*/
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/systick.h>

void SysTick_enableModule(void)
{
    //
    // Enable SysTick.
    //
    SysTick->CTRL |= SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
}

void SysTick_disableModule(void)
{
    //
    // Disable SysTick.
    //
    SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);
}

void SysTick_registerInterrupt(void (*intHandler)(void))
{
    //
    // Register the interrupt handler, returning an error if an error occurs.
    //
    Interrupt_registerInterrupt(FAULT_SYSTICK, intHandler);

}

void SysTick_unregisterInterrupt(void)
{

    //
    // Unregister the interrupt handler.
    //
    Interrupt_unregisterInterrupt(FAULT_SYSTICK);
}

void SysTick_enableInterrupt(void)
{
    //
    // Enable the SysTick interrupt.
    //
    SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;
}

void SysTick_disableInterrupt(void)
{
    //
    // Disable the SysTick interrupt.
    //
    SysTick->CTRL &= ~(SysTick_CTRL_TICKINT_Msk);
}

void SysTick_setPeriod(uint32_t period)
{
    //
    // Check the arguments.
    //
    ASSERT((period > 0) && (period <= 16777216));

    //
    // Set the period of the SysTick counter.
    //
    SysTick->LOAD = period - 1;
}

uint32_t SysTick_getPeriod(void)
{
    //
    // Return the period of the SysTick counter.
    //
    return (SysTick->LOAD + 1);
}

uint32_t SysTick_getValue(void)
{
    //
    // Return the current value of the SysTick counter.
    //
    return (SysTick->VAL);
}
