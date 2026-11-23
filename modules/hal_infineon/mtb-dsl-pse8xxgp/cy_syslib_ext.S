/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Each routine in this file must execute from RAM, so emit every function into
 * its own .text.<symbol> input section. This module's CMakeLists relocates them
 * into the SRAM code region with zephyr_code_relocate().
 */
.syntax unified
.thumb

.section .text.Cy_SysLib_DelayCycles,"ax",%progbits


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
.thumb_func
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
.section .text.Cy_SysLib_EnterCriticalSection,"ax",%progbits
.global Cy_SysLib_EnterCriticalSection
.type Cy_SysLib_EnterCriticalSection, %function
.thumb_func
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
.section .text.Cy_SysLib_ExitCriticalSection,"ax",%progbits
.global Cy_SysLib_ExitCriticalSection
.type Cy_SysLib_ExitCriticalSection, %function
.thumb_func
Cy_SysLib_ExitCriticalSection:
    MSR PRIMASK, r0         /* Restore interrupt state */
    BX lr

.end


/* [] END OF FILE */
