/***************************************************************************//**
* \file cy_wdt.c
* \version 1.10
*
*  This file provides the source code to the API for the WDT driver.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation. All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

#include "cy_wdt.h"
#include "cy_syslib.h"

#if defined(__cplusplus)
extern "C" {
#endif

static bool Cy_WDT_Locked(void);


/*******************************************************************************
* Function Name: Cy_WDT_Init
****************************************************************************//**
*
* Initializes the Watchdog timer to its default state.
*
* The given default setting of the WDT:
* The WDT is unlocked and disabled.
* The WDT match value is 4096.
* None of ignore bits are set: the whole WDT counter bits are checked against 
* the match value.
*
*******************************************************************************/
void Cy_WDT_Init(void)
{
    uint32_t interruptState;
    interruptState = Cy_SysLib_EnterCriticalSection();

    /* Unlock the WDT by two writes */
    SRSS_WDT_CTL = ((SRSS_WDT_CTL & (uint32_t)(~SRSS_WDT_CTL_WDT_LOCK_Msk)) | CY_SRSS_WDT_LOCK_BIT0);

    SRSS_WDT_CTL |= CY_SRSS_WDT_LOCK_BIT1;

    Cy_WDT_Disable();

    Cy_WDT_SetMatch(CY_SRSS_WDT_DEFAULT_MATCH_VALUE);

    Cy_WDT_SetIgnoreBits(CY_SRSS_WDT_DEFAULT_IGNORE_BITS);

    Cy_SysLib_ExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: Cy_WDT_Lock
****************************************************************************//**
*
* Locks out configuration changes to the Watchdog Timer register.
*
* After this function is called, the WDT configuration cannot be changed until 
* Cy_WDT_Unlock() is called.
*
*******************************************************************************/
void Cy_WDT_Lock(void)
{
    uint32_t interruptState;
    interruptState = Cy_SysLib_EnterCriticalSection();

    SRSS_WDT_CTL |= _VAL2FLD(SRSS_WDT_CTL_WDT_LOCK, CY_SRSS_WDT_LOCK_BITS);

    Cy_SysLib_ExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: Cy_WDT_Locked
****************************************************************************//**
* \internal
* Reports the WDT lock state.
*
* \return true - if WDT is locked, and false - if WDT is unlocked.
* \endinternal
*******************************************************************************/
static bool Cy_WDT_Locked(void)
{
    /* Prohibits writing to the WDT registers and LFCLK */
    return (0u != _FLD2VAL(SRSS_WDT_CTL_WDT_LOCK, SRSS_WDT_CTL));
}


/*******************************************************************************
* Function Name: Cy_WDT_Unlock
****************************************************************************//**
*
* Unlocks the Watchdog Timer configuration register.
*
*******************************************************************************/
void Cy_WDT_Unlock(void)
{
    uint32_t interruptState;
    interruptState = Cy_SysLib_EnterCriticalSection();

    /* The WDT lock is to be removed by two writes */
    SRSS_WDT_CTL = ((SRSS_WDT_CTL & (uint32_t)(~SRSS_WDT_CTL_WDT_LOCK_Msk)) | CY_SRSS_WDT_LOCK_BIT0);

    SRSS_WDT_CTL |= CY_SRSS_WDT_LOCK_BIT1;

    Cy_SysLib_ExitCriticalSection(interruptState);
}


/*******************************************************************************
* Function Name: Cy_WDT_SetMatch
****************************************************************************//**
*
* Configures the WDT counter match comparison value. The Watchdog timer 
* should be unlocked before changing the match value. Call the Cy_WDT_Unlock() 
* function to unlock the WDT.
*
* \param match
* The valid valid range is [0-65535]. The value to be used to match 
* against the counter.
*
*******************************************************************************/
void Cy_WDT_SetMatch(uint32_t match)
{
    CY_ASSERT_L3(CY_WDT_IS_MATCH_VAL_VALID(match));
    
    if (false == Cy_WDT_Locked())
    {
        SRSS_WDT_MATCH = _CLR_SET_FLD32U((SRSS_WDT_MATCH), SRSS_WDT_MATCH_MATCH, match);
    }
}


/*******************************************************************************
* Function Name: Cy_WDT_SetIgnoreBits
****************************************************************************//**
*
* Configures the number of the most significant bits of the Watchdog timer that 
* are not checked against the match. Unlock the Watchdog timer before 
* ignoring the bits setting. Call the Cy_WDT_Unlock() API to unlock the WDT.
*
* \param bitsNum
* The number of the most significant bits. The valid range is [0-15].
* The bitsNum over 12 are considered as 12.
*
* \details The value of bitsNum controls the time-to-reset of the Watchdog timer
* This happens after 3 successive matches.
*
* \warning This function changes the WDT interrupt period, therefore 
* the device can go into an infinite WDT reset loop. This may happen
* if a WDT reset occurs faster that a device start-up.
*
*******************************************************************************/
void Cy_WDT_SetIgnoreBits(uint32_t bitsNum)
{
    CY_ASSERT_L3(CY_WDT_IS_IGNORE_BITS_VALID(bitsNum));

    if (false == Cy_WDT_Locked())
    {
        SRSS_WDT_MATCH = _CLR_SET_FLD32U((SRSS_WDT_MATCH), SRSS_WDT_MATCH_IGNORE_BITS, bitsNum);
    }
}


/*******************************************************************************
* Function Name: Cy_WDT_ClearInterrupt
****************************************************************************//**
*
* Clears the WDT match flag which is set every time the WDT counter reaches a 
* WDT match value. Two unserviced interrupts lead to a system reset 
* (i.e. at the third match).
*
*******************************************************************************/
void Cy_WDT_ClearInterrupt(void)
{
    SRSS_SRSS_INTR = _VAL2FLD(SRSS_SRSS_INTR_WDT_MATCH, 1u);

    /* Read the interrupt register to ensure that the initial clearing write has
    * been flushed out to the hardware.
    */
    (void) SRSS_SRSS_INTR;
}


/*******************************************************************************
* Function Name: Cy_WDT_ClearWatchdog
****************************************************************************//**
*
* Clears ("feeds") the watchdog, to prevent a XRES device reset.
* This function simply call Cy_WDT_ClearInterrupt() function.
*
*******************************************************************************/
void Cy_WDT_ClearWatchdog(void)
{
    Cy_WDT_ClearInterrupt();
}

#if defined(__cplusplus)
}
#endif


/* [] END OF FILE */
