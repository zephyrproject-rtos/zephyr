/***************************************************************************//**
* \file cy_systick.c
* \version 1.10.1
*
* Provides the API definitions of the SisTick driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_systick.h"
#include <stddef.h>     /* for NULL */


static Cy_SysTick_Callback Cy_SysTick_Callbacks[CY_SYS_SYST_NUM_OF_CALLBACKS];
static void Cy_SysTick_ServiceCallbacks(void);


/*******************************************************************************
* Function Name: Cy_SysTick_Init
****************************************************************************//**
*
* Initializes the SysTick driver:
* - Initializes the callback addresses with pointers to NULL
* - Associates the SysTick system vector with the callback functions
* - Sets the SysTick clock by calling \ref Cy_SysTick_SetClockSource()
* - Sets the SysTick reload interval by calling \ref Cy_SysTick_SetReload()
* - Clears the SysTick counter value by calling \ref Cy_SysTick_Clear()
* - Enables the SysTick by calling \ref Cy_SysTick_Enable(). Note the \ref
*   Cy_SysTick_Enable() function also enables the SysTick interrupt by calling
*   \ref Cy_SysTick_EnableInterrupt().
*
* \param clockSource The SysTick clock source \ref cy_en_systick_clock_source_t
* \param interval The SysTick reload value.
*
* \sideeffect Clears the SysTick count flag if it was set.
*
*******************************************************************************/
void Cy_SysTick_Init(cy_en_systick_clock_source_t clockSource, uint32_t interval)
{
    uint32_t i;

    for (i = 0u; i<CY_SYS_SYST_NUM_OF_CALLBACKS; i++)
    {
        Cy_SysTick_Callbacks[i] = NULL;
    }

    __ramVectors[CY_SYSTICK_IRQ_NUM] = &Cy_SysTick_ServiceCallbacks;
    Cy_SysTick_SetClockSource(clockSource);

    Cy_SysTick_SetReload(interval);
    Cy_SysTick_Clear();
    Cy_SysTick_Enable();
}


/*******************************************************************************
* Function Name: Cy_SysTick_Enable
****************************************************************************//**
*
* Enables the SysTick timer and its interrupt.
*
* \sideeffect Clears the SysTick count flag if it was set
*
*******************************************************************************/
void Cy_SysTick_Enable(void)
{
    Cy_SysTick_EnableInterrupt();
    SYSTICK_CTRL |= SysTick_CTRL_ENABLE_Msk;
}


/*******************************************************************************
* Function Name: Cy_SysTick_Disable
****************************************************************************//**
*
* Disables the SysTick timer and its interrupt.
*
* \sideeffect Clears the SysTick count flag if it was set
*
*******************************************************************************/
void Cy_SysTick_Disable(void)
{
    Cy_SysTick_DisableInterrupt();
    SYSTICK_CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}


/*******************************************************************************
* Function Name: Cy_SysTick_SetClockSource
****************************************************************************//**
*
* Sets the clock source for the SysTick counter.
*
* Clears the SysTick count flag if it was set. If the clock source is not ready
* this function call will have no effect. After changing the clock source to the
* low frequency clock, the counter and reload register values will remain
* unchanged so the time to the interrupt will be significantly longer and vice
* versa.
*
* Changing the SysTick clock source and/or its frequency will change
* the interrupt interval and Cy_SysTick_SetReload() should be
* called to compensate this change.
*
* \param clockSource \ref cy_en_systick_clock_source_t Clock source.
*
*******************************************************************************/
void Cy_SysTick_SetClockSource(cy_en_systick_clock_source_t clockSource)
{
    if (clockSource == CY_SYSTICK_CLOCK_SOURCE_CLK_CPU)
    {
        SYSTICK_CTRL |= SysTick_CTRL_CLKSOURCE_Msk;
    }
    else
    {
        CPUSS->SYSTICK_CTL = _VAL2FLD(CPUSS_SYSTICK_CTL_CLOCK_SOURCE, (uint32_t) clockSource);
        SYSTICK_CTRL &= ~SysTick_CTRL_CLKSOURCE_Msk;
    }
}


/*******************************************************************************
* Function Name: Cy_SysTick_GetClockSource
****************************************************************************//**
*
* Gets the clock source for the SysTick counter.
*
* \returns \ref cy_en_systick_clock_source_t Clock source
*
*******************************************************************************/
cy_en_systick_clock_source_t Cy_SysTick_GetClockSource(void)
{
    cy_en_systick_clock_source_t returnValue;

    if ((SYSTICK_CTRL & SysTick_CTRL_CLKSOURCE_Msk) != 0u)
    {
        returnValue = CY_SYSTICK_CLOCK_SOURCE_CLK_CPU;
    }
    else
    {
        returnValue =  (cy_en_systick_clock_source_t) ((uint32_t) _FLD2VAL(CPUSS_SYSTICK_CTL_CLOCK_SOURCE, CPUSS->SYSTICK_CTL));
    }

    return(returnValue);
}


/*******************************************************************************
* Function Name: Cy_SysTick_SetCallback
****************************************************************************//**
*
* Sets the callback function to the specified callback number.
*
* \param number The number of the callback function addresses to be set.
* The valid range is from 0 to \ref CY_SYS_SYST_NUM_OF_CALLBACKS - 1.
*
* \param function The pointer to the function that will be associated with the
* SysTick ISR for the specified number.
*
* \return Returns the address of the previous callback function.
* The NULL is returned if the specified address in not set or incorrect
* parameter is specified.

* \sideeffect
* The registered callback functions will be executed in the interrupt.
*
*******************************************************************************/
Cy_SysTick_Callback Cy_SysTick_SetCallback(uint32_t number, Cy_SysTick_Callback function)
{
    Cy_SysTick_Callback retVal;

    if (number < CY_SYS_SYST_NUM_OF_CALLBACKS)
    {
        retVal = Cy_SysTick_Callbacks[number];
        Cy_SysTick_Callbacks[number] = function;
    }
    else
    {
        retVal = NULL;
    }

    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_SysTick_GetCallback
****************************************************************************//**
*
* Gets the specified callback function address.
*
* \param number The number of the callback function address to get. The valid
* range is from 0 to \ref CY_SYS_SYST_NUM_OF_CALLBACKS - 1.
*
* \return Returns the address of the specified callback function.
* The NULL is returned if the specified address in not initialized or incorrect
* parameter is specified.
*
*******************************************************************************/
Cy_SysTick_Callback Cy_SysTick_GetCallback(uint32_t number)
{
    Cy_SysTick_Callback retVal;

    if (number < CY_SYS_SYST_NUM_OF_CALLBACKS)
    {
        retVal = Cy_SysTick_Callbacks[number];
    }
    else
    {
        retVal = NULL;
    }

    return (retVal);
}


/*******************************************************************************
* Function Name: Cy_SysTick_ServiceCallbacks
****************************************************************************//**
*
* The system Tick timer interrupt routine.
*
*******************************************************************************/
static void Cy_SysTick_ServiceCallbacks(void)
{
    uint32_t i;

    /* Verify that tick timer flag was set */
    if (0u != Cy_SysTick_GetCountFlag())
    {
        for (i=0u; i < CY_SYS_SYST_NUM_OF_CALLBACKS; i++)
        {
            if (Cy_SysTick_Callbacks[i] != NULL)
            {
                (void)(Cy_SysTick_Callbacks[i])();
            }
        }
    }
}


/* [] END OF FILE */
