/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

.section .text,"ax"
.text


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
.global Cy_SysLib_DelayCycles
.type Cy_SysLib_DelayCycles, %function
Cy_SysLib_DelayCycles:      /* cycles bytes */

    ADDS r0, r0, #2         /*    1    2    Round to nearest multiple of 4 */
    LSRS r0, r0, #2         /*    1    2    Divide by 4 and set flags */
    BEQ Cy_DelayCycles_done /*    2    2    Skip if 0 */

Cy_DelayCycles_loop:
    ADDS r0, r0, #1         /*    1    2    Increment counter */
    SUBS r0, r0, #2         /*    1    2    Decrement counter by 2 */
    BNE Cy_DelayCycles_loop /*   (1)2  2    2 CPU cycles (if branch is taken) */
    NOP                     /*    1    2    Loop alignment padding */

Cy_DelayCycles_done:
    NOP                     /*    1    2    Loop alignment padding */
    BX lr                   /*    3    2 */


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
.global Cy_SysLib_EnterCriticalSection
.type Cy_SysLib_EnterCriticalSection, %function

Cy_SysLib_EnterCriticalSection:
    MRS r0, PRIMASK         /* Save and return interrupt state */
    cpsid i                 /* Disable interrupts */
    BX lr



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
.global Cy_SysLib_ExitCriticalSection
.type Cy_SysLib_ExitCriticalSection, %function

Cy_SysLib_ExitCriticalSection:
    MSR PRIMASK, r0         /* Restore interrupt state */
    BX lr

.end


/* [] END OF FILE */
