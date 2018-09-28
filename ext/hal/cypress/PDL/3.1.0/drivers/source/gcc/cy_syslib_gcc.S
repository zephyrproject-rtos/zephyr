/***************************************************************************//**
* \file cy_syslib_core_armcc.s
* \version 2.10.1
*
* \brief Assembly routines for GNU as.
*
********************************************************************************
* \copyright
* Copyright 2016-2018, Cypress Semiconductor Corporation.  All rights reserved.
* SPDX-License-Identifier: Apache-2.0



*******************************************************************************/

.syntax unified
.text
.thumb


/*******************************************************************************
* Function Name: Cy_SysLib_DelayCycles
****************************************************************************//**
*
* Delays for the specified number of cycles.
*
* \param uint32_t cycles: The number of cycles to delay.
*
*******************************************************************************/
/* void Cy_SysLib_DelayCycles(uint32_t cycles) */
.align 3                    /* Align to 8 byte boundary (2^n) */
#ifndef __clang__
.global Cy_SysLib_DelayCycles
.func Cy_SysLib_DelayCycles, Cy_SysLib_DelayCycles
.type Cy_SysLib_DelayCycles, %function
.thumb_func
Cy_SysLib_DelayCycles:      /* cycles bytes */

#else
.global _Cy_SysLib_DelayCycles
.type _Cy_SysLib_DelayCycles, %function
.thumb_func

_Cy_SysLib_DelayCycles:      /* cycles bytes */
#endif

    ADDS r0, r0, #2         /*    1    2    Round to nearest multiple of 4 */
    LSRS r0, r0, #2         /*    1    2    Divide by 4 and set flags */
    BEQ Cy_DelayCycles_done /*    2    2    Skip if 0 */
Cy_DelayCycles_loop:
/* For CM0+ branch instruction takes 2 CPU cycles */
    ADDS r0, r0, #1         /*    1    2    Increment counter */
    SUBS r0, r0, #2         /*    1    2    Decrement counter by 2 */
    BNE Cy_DelayCycles_loop /*    2    2    2 CPU cycles (if branch is taken)*/
    NOP                     /*    1    2    Loop alignment padding */
Cy_DelayCycles_done:
    NOP                     /*    1    2    Loop alignment padding */
    BX lr                   /*    3    2 */

#ifndef __clang__    
.endfunc
#endif

/*******************************************************************************
* Function Name: Cy_SysLib_EnterCriticalSection
****************************************************************************//**
*
* Cy_SysLib_EnterCriticalSection disables interrupts and returns a value
* indicating whether interrupts were previously enabled.
*
* Note Implementation of Cy_SysLib_EnterCriticalSection manipulates the IRQ
* enable bit with interrupts still enabled.
*
* \return Returns 0 if interrupts were previously enabled or 1 if interrupts
* were previously disabled.
*
*******************************************************************************/
/* uint8_t Cy_SysLib_EnterCriticalSection(void) */
#ifndef __clang__
.global Cy_SysLib_EnterCriticalSection
.func Cy_SysLib_EnterCriticalSection, Cy_SysLib_EnterCriticalSection
.type Cy_SysLib_EnterCriticalSection, %function
.thumb_func

Cy_SysLib_EnterCriticalSection:
#else
.global _Cy_SysLib_EnterCriticalSection
.type _Cy_SysLib_EnterCriticalSection, %function
.thumb_func

_Cy_SysLib_EnterCriticalSection:
#endif

    MRS r0, PRIMASK         /* Save and return interrupt state */
    
    /* Disable interrupts */
#ifndef __clang__
    CPSID I
#else
    CPSID i
#endif
    BX lr

#ifndef __clang__
.endfunc
#endif


/*******************************************************************************
* Function Name: Cy_SysLib_ExitCriticalSection
****************************************************************************//**
*
*  Re-enables interrupts if they were enabled before
*  Cy_SysLib_EnterCriticalSection() was called. The argument should be the value
*  returned from \ref Cy_SysLib_EnterCriticalSection().
*
*  \param uint8_t savedIntrStatus:
*   Saved interrupt status returned by the \ref Cy_SysLib_EnterCriticalSection().
*
*******************************************************************************/
/* void Cy_SysLib_ExitCriticalSection(uint8_t savedIntrStatus) */
#ifndef __clang__
.global Cy_SysLib_ExitCriticalSection
.func Cy_SysLib_ExitCriticalSection, Cy_SysLib_ExitCriticalSection
.type Cy_SysLib_ExitCriticalSection, %function
.thumb_func
Cy_SysLib_ExitCriticalSection:
#else
.global _Cy_SysLib_ExitCriticalSection
.type _Cy_SysLib_ExitCriticalSection, %function
.thumb_func

_Cy_SysLib_ExitCriticalSection:
#endif

    MSR PRIMASK, r0         /* Restore interrupt state */
    BX lr

#ifndef __clang__
.endfunc
#endif

.end

/* [] END OF FILE */
