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
#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

//*****************************************************************************
//
//! \addtogroup interrupt_api
//! @{
//
//*****************************************************************************


//*****************************************************************************
//
// If building with a C++ compiler, make all of the definitions in this header
// have a C binding.
//
//*****************************************************************************
#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <stdbool.h>
#include <ti/devices/msp432p4xx/inc/msp.h>

/******************************************************************************
* NVIC interrupts                                                             *
******************************************************************************/
/* System exceptions */
#define FAULT_NMI                                       ( 2) /* NMI fault */
#define FAULT_HARD                                      ( 3) /* Hard fault */
#define FAULT_MPU                                       ( 4) /* MPU fault */
#define FAULT_BUS                                       ( 5) /* Bus fault */
#define FAULT_USAGE                                     ( 6) /* Usage fault */
#define FAULT_SVCALL                                    (11) /* SVCall */
#define FAULT_DEBUG                                     (12) /* Debug monitor */
#define FAULT_PENDSV                                    (14) /* PendSV */
#define FAULT_SYSTICK                                   (15) /* System Tick */

/* External interrupts */
#define INT_PSS                                         (16) /* PSS IRQ */
#define INT_CS                                          (17) /* CS IRQ */
#define INT_PCM                                         (18) /* PCM IRQ */
#define INT_WDT_A                                       (19) /* WDT_A IRQ */
#define INT_FPU                                         (20) /* FPU IRQ */
#define INT_FLCTL                                       (21) /* FLCTL IRQ */
#define INT_COMP_E0                                     (22) /* COMP_E0 IRQ */
#define INT_COMP_E1                                     (23) /* COMP_E1 IRQ */
#define INT_TA0_0                                       (24) /* TA0_0 IRQ */
#define INT_TA0_N                                       (25) /* TA0_N IRQ */
#define INT_TA1_0                                       (26) /* TA1_0 IRQ */
#define INT_TA1_N                                       (27) /* TA1_N IRQ */
#define INT_TA2_0                                       (28) /* TA2_0 IRQ */
#define INT_TA2_N                                       (29) /* TA2_N IRQ */
#define INT_TA3_0                                       (30) /* TA3_0 IRQ */
#define INT_TA3_N                                       (31) /* TA3_N IRQ */
#define INT_EUSCIA0                                     (32) /* EUSCIA0 IRQ */
#define INT_EUSCIA1                                     (33) /* EUSCIA1 IRQ */
#define INT_EUSCIA2                                     (34) /* EUSCIA2 IRQ */
#define INT_EUSCIA3                                     (35) /* EUSCIA3 IRQ */
#define INT_EUSCIB0                                     (36) /* EUSCIB0 IRQ */
#define INT_EUSCIB1                                     (37) /* EUSCIB1 IRQ */
#define INT_EUSCIB2                                     (38) /* EUSCIB2 IRQ */
#define INT_EUSCIB3                                     (39) /* EUSCIB3 IRQ */
#define INT_ADC14                                       (40) /* ADC14 IRQ */
#define INT_T32_INT1                                    (41) /* T32_INT1 IRQ */
#define INT_T32_INT2                                    (42) /* T32_INT2 IRQ */
#define INT_T32_INTC                                    (43) /* T32_INTC IRQ */
#define INT_AES256                                      (44) /* AES256 IRQ */
#define INT_RTC_C                                       (45) /* RTC_C IRQ */
#define INT_DMA_ERR                                     (46) /* DMA_ERR IRQ */
#define INT_DMA_INT3                                    (47) /* DMA_INT3 IRQ */
#define INT_DMA_INT2                                    (48) /* DMA_INT2 IRQ */
#define INT_DMA_INT1                                    (49) /* DMA_INT1 IRQ */
#define INT_DMA_INT0                                    (50) /* DMA_INT0 IRQ */
#define INT_PORT1                                       (51) /* PORT1 IRQ */
#define INT_PORT2                                       (52) /* PORT2 IRQ */
#define INT_PORT3                                       (53) /* PORT3 IRQ */
#define INT_PORT4                                       (54) /* PORT4 IRQ */
#define INT_PORT5                                       (55) /* PORT5 IRQ */
#define INT_PORT6                                       (56) /* PORT6 IRQ */
#define INT_LCD_F                                       (57) /* PORT6 IRQ */

#define NUM_INTERRUPTS                                  (57)
//*****************************************************************************
//
// Macro to generate an interrupt priority mask based on the number of bits
// of priority supported by the hardware.
//
//*****************************************************************************
#define INT_PRIORITY_MASK       ((0xFF << (8 - NUM_PRIORITY_BITS)) & 0xFF)
#define NUM_PRIORITY            8

#define NVIC_APINT_PRIGROUP_M   0x00000700  // Interrupt Priority Grouping
#define NVIC_APINT_PRIGROUP_7_1 0x00000000  // Priority group 7.1 split
#define NVIC_APINT_PRIGROUP_6_2 0x00000100  // Priority group 6.2 split
#define NVIC_APINT_PRIGROUP_5_3 0x00000200  // Priority group 5.3 split
#define NVIC_APINT_PRIGROUP_4_4 0x00000300  // Priority group 4.4 split
#define NVIC_APINT_PRIGROUP_3_5 0x00000400  // Priority group 3.5 split
#define NVIC_APINT_PRIGROUP_2_6 0x00000500  // Priority group 2.6 split
#define NVIC_APINT_PRIGROUP_1_7 0x00000600  // Priority group 1.7 split
#define NVIC_APINT_PRIGROUP_0_8 0x00000700  // Priority group 0.8 split
#define NVIC_SYS_PRI1_R         0xE000ED18  // System Handler Priority 1
#define NVIC_SYS_PRI2_R         0xE000ED1C  // System Handler Priority 2
#define NVIC_SYS_PRI3_R         0xE000ED20  // System Handler Priority 3
#define NVIC_PRI0_R             0xE000E400  // Interrupt 0-3 Priority
#define NVIC_PRI1_R             0xE000E404  // Interrupt 4-7 Priority
#define NVIC_PRI2_R             0xE000E408  // Interrupt 8-11 Priority
#define NVIC_PRI3_R             0xE000E40C  // Interrupt 12-15 Priority
#define NVIC_PRI4_R             0xE000E410  // Interrupt 16-19 Priority
#define NVIC_PRI5_R             0xE000E414  // Interrupt 20-23 Priority
#define NVIC_PRI6_R             0xE000E418  // Interrupt 24-27 Priority
#define NVIC_PRI7_R             0xE000E41C  // Interrupt 28-31 Priority
#define NVIC_PRI8_R             0xE000E420  // Interrupt 32-35 Priority
#define NVIC_PRI9_R             0xE000E424  // Interrupt 36-39 Priority
#define NVIC_PRI10_R            0xE000E428  // Interrupt 40-43 Priority
#define NVIC_PRI11_R            0xE000E42C  // Interrupt 44-47 Priority
#define NVIC_PRI12_R            0xE000E430  // Interrupt 48-51 Priority
#define NVIC_PRI13_R            0xE000E434  // Interrupt 52-55 Priority
#define NVIC_PRI14_R            0xE000E438  // Interrupt 56-59 Priority
#define NVIC_PRI15_R            0xE000E43C  // Interrupt 60-63 Priority
#define NVIC_EN0_R              0xE000E100  // Interrupt 0-31 Set Enable
#define NVIC_EN1_R              0xE000E104  // Interrupt 32-54 Set Enable
#define NVIC_DIS0_R             0xE000E180  // Interrupt 0-31 Clear Enable
#define NVIC_DIS1_R             0xE000E184  // Interrupt 32-54 Clear Enable
#define NVIC_PEND0_R            0xE000E200  // Interrupt 0-31 Set Pending
#define NVIC_PEND1_R            0xE000E204  // Interrupt 32-54 Set Pending
#define NVIC_UNPEND0_R          0xE000E280  // Interrupt 0-31 Clear Pending
#define NVIC_UNPEND1_R          0xE000E284  // Interrupt 32-54 Clear Pending
//*****************************************************************************
//
// Prototypes for the APIs.
//
//*****************************************************************************

//*****************************************************************************
//
//! Enables the processor interrupt.
//!
//! This function allows the processor to respond to interrupts.  This function
//! does not affect the set of interrupts enabled in the interrupt controller;
//! it just gates the single interrupt from the controller to the processor.
//!
//! \return Returns \b true if interrupts were disabled when the function was
//! called or \b false if they were initially enabled.
//
//*****************************************************************************
extern bool Interrupt_enableMaster(void);

//*****************************************************************************
//
//! Disables the processor interrupt.
//!
//! This function prevents the processor from receiving interrupts.  This
//! function does not affect the set of interrupts enabled in the interrupt
//! controller; it just gates the single interrupt from the controller to the
//! processor.
//!
//! \return Returns \b true if interrupts were already disabled when the
//! function was called or \b false if they were initially enabled.
//
//*****************************************************************************
extern bool Interrupt_disableMaster(void);

//*****************************************************************************
//
//! Registers a function to be called when an interrupt occurs.
//!
//! \param interruptNumber specifies the interrupt in question.
//! \param intHandler is a pointer to the function to be called.
//!
//! \note The use of this function (directly or indirectly via a peripheral
//! driver interrupt register function) moves the interrupt vector table from
//! flash to SRAM.  Therefore, care must be taken when linking the application
//! to ensure that the SRAM vector table is located at the beginning of SRAM;
//! otherwise the NVIC does not look in the correct portion of memory for the
//! vector table (it requires the vector table be on a 1 kB memory alignment).
//! Normally, the SRAM vector table is so placed via the use of linker scripts.
//! See the discussion of compile-time versus run-time interrupt handler
//! registration in the introduction to this chapter.
//!
//! \note This function is only used if the customer wants to specify the
//!  interrupt handler at run time. In most cases, this is done through means
//!  of the user setting the ISR function pointer in the startup file. Refer
//!  Refer to the Module Operation section for more details.
//!
//! See \link Interrupt_enableInterrupt \endlink for details about the interrupt
//! parameter
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_registerInterrupt(uint32_t interruptNumber,
        void (*intHandler)(void));

//*****************************************************************************
//
//! Unregisters the function to be called when an interrupt occurs.
//!
//! \param interruptNumber specifies the interrupt in question.
//!
//! This function is used to indicate that no handler should be called when the
//! given interrupt is asserted to the processor.  The interrupt source is
//! automatically disabled (via Interrupt_disableInterrupt()) if necessary.
//!
//! \sa Interrupt_registerInterrupt() for important information about
//! registering interrupt handlers.
//!
//! See \link Interrupt_enableInterrupt \endlink for details about the interrupt
//! parameter
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_unregisterInterrupt(uint32_t interruptNumber);

//*****************************************************************************
//
//! Sets the priority grouping of the interrupt controller.
//!
//! \param bits specifies the number of bits of preemptable priority.
//!
//! This function specifies the split between preemptable priority levels and
//! sub-priority levels in the interrupt priority specification.  The range of
//! the grouping values are dependent upon the hardware implementation; on
//! the MSP432 family, three bits are available for hardware interrupt
//! prioritization and therefore priority grouping values of three through
//! seven have the same effect.
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_setPriorityGrouping(uint32_t bits);

//*****************************************************************************
//
//! Gets the priority grouping of the interrupt controller.
//!
//! This function returns the split between preemptable priority levels and
//! sub-priority levels in the interrupt priority specification.
//!
//! \return The number of bits of preemptable priority.
//
//*****************************************************************************
extern uint32_t Interrupt_getPriorityGrouping(void);

//*****************************************************************************
//
//! Sets the priority of an interrupt.
//!
//! \param interruptNumber specifies the interrupt in question.
//! \param priority specifies the priority of the interrupt.
//!
//! This function is used to set the priority of an interrupt.  When multiple
//! interrupts are asserted simultaneously, the ones with the highest priority
//! are processed before the lower priority interrupts.  Smaller numbers
//! correspond to higher interrupt priorities; priority 0 is the highest
//! interrupt priority.
//!
//! The hardware priority mechanism only looks at the upper N bits of the
//! priority level (where N is 3 for the MSP432 family), so any
//! prioritization must be performed in those bits.  The remaining bits can be
//! used to sub-prioritize the interrupt sources, and may be used by the
//! hardware priority mechanism on a future part.  This arrangement allows
//! priorities to migrate to different NVIC implementations without changing
//! the gross prioritization of the interrupts.
//!
//! See \link Interrupt_enableInterrupt \endlink for details about the interrupt
//! parameter
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_setPriority(uint32_t interruptNumber, uint8_t priority);

//*****************************************************************************
//
//! Gets the priority of an interrupt.
//!
//! \param interruptNumber specifies the interrupt in question.
//!
//! This function gets the priority of an interrupt.  See
//! Interrupt_setPriority() for a definition of the priority value.
//!
//! See \link Interrupt_enableInterrupt \endlink for details about the interrupt
//! parameter
//!
//! \return Returns the interrupt priority, or -1 if an invalid interrupt was
//! specified.
//
//*****************************************************************************
extern uint8_t Interrupt_getPriority(uint32_t interruptNumber);

//*****************************************************************************
//
//! Enables an interrupt.
//!
//! \param interruptNumber specifies the interrupt to be enabled.
//!
//! The specified interrupt is enabled in the interrupt controller.  Other
//! enables for the interrupt (such as at the peripheral level) are unaffected
//! by this function.
//!
//! Valid values will vary from part to part, so it is important to check the
//! device specific datasheet, however for MSP432 101 the following values can
//! be provided:
//!     - \b FAULT_NMI
//!     - \b FAULT_HARD
//!     - \b FAULT_MPU
//!     - \b FAULT_BUS
//!     - \b FAULT_USAGE
//!     - \b FAULT_SVCALL
//!     - \b FAULT_DEBUG
//!     - \b FAULT_PENDSV
//!     - \b FAULT_SYSTICK
//!     - \b INT_PSS
//!     - \b INT_CS
//!     - \b INT_PCM
//!     - \b INT_WDT_A
//!     - \b INT_FPU
//!     - \b INT_FLCTL
//!     - \b INT_COMP0
//!     - \b INT_COMP1
//!     - \b INT_TA0_0
//!     - \b INT_TA0_N
//!     - \b INT_TA1_0
//!     - \b INT_TA1_N
//!     - \b INT_TA2_0
//!     - \b INT_TA2_N
//!     - \b INT_TA3_0
//!     - \b INT_TA3_N
//!     - \b INT_EUSCIA0
//!     - \b INT_EUSCIA1
//!     - \b INT_EUSCIA2
//!     - \b INT_EUSCIA3
//!     - \b INT_EUSCIB0
//!     - \b INT_EUSCIB1
//!     - \b INT_EUSCIB2
//!     - \b INT_EUSCIB3
//!     - \b INT_ADC14
//!     - \b INT_T32_INT1
//!     - \b INT_T32_INT2
//!     - \b INT_T32_INTC
//!     - \b INT_AES
//!     - \b INT_RTCC
//!     - \b INT_DMA_ERR
//!     - \b INT_DMA_INT3
//!     - \b INT_DMA_INT2
//!     - \b INT_DMA_INT1
//!     - \b INT_DMA_INT0
//!     - \b INT_PORT1
//!     - \b INT_PORT2
//!     - \b INT_PORT3
//!     - \b INT_PORT4
//!     - \b INT_PORT5
//!     - \b INT_PORT6
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_enableInterrupt(uint32_t interruptNumber);

//*****************************************************************************
//
//! Disables an interrupt.
//!
//! \param interruptNumber specifies the interrupt to be disabled.
//!
//! The specified interrupt is disabled in the interrupt controller.  Other
//! enables for the interrupt (such as at the peripheral level) are unaffected
//! by this function.
//!
//! See \link Interrupt_enableInterrupt \endlink for details about the interrupt
//! parameter
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_disableInterrupt(uint32_t interruptNumber);

//*****************************************************************************
//
//! Returns if a peripheral interrupt is enabled.
//!
//! \param interruptNumber specifies the interrupt to check.
//!
//! This function checks if the specified interrupt is enabled in the interrupt
//! controller.
//!
//! See \link Interrupt_enableInterrupt \endlink for details about the interrupt
//! parameter
//!
//! \return A non-zero value if the interrupt is enabled.
//
//*****************************************************************************
extern bool Interrupt_isEnabled(uint32_t interruptNumber);

//*****************************************************************************
//
//! Pends an interrupt.
//!
//! \param interruptNumber specifies the interrupt to be pended.
//!
//! The specified interrupt is pended in the interrupt controller.  Pending an
//! interrupt causes the interrupt controller to execute the corresponding
//! interrupt handler at the next available time, based on the current
//! interrupt state priorities. For example, if called by a higher priority
//! interrupt handler, the specified interrupt handler is not called until
//! after the current interrupt handler has completed execution.  The interrupt
//! must have been enabled for it to be called.
//!
//! See \link Interrupt_enableInterrupt \endlink for details about the interrupt
//! parameter
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_pendInterrupt(uint32_t interruptNumber);

//*****************************************************************************
//
//! Un-pends an interrupt.
//!
//! \param interruptNumber specifies the interrupt to be un-pended.
//!
//! The specified interrupt is un-pended in the interrupt controller.  This
//! will cause any previously generated interrupts that have not been handled
//! yet (due to higher priority interrupts or the interrupt no having been
//! enabled yet) to be discarded.
//!
//! See \link Interrupt_enableInterrupt \endlink for details about the interrupt
//! parameter
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_unpendInterrupt(uint32_t interruptNumber);

//*****************************************************************************
//
//! Sets the priority masking level
//!
//! \param priorityMask is the priority level that is masked.
//!
//! This function sets the interrupt priority masking level so that all
//! interrupts at the specified or lesser priority level are masked.  Masking
//! interrupts can be used to globally disable a set of interrupts with
//! priority below a predetermined threshold.  A value of 0 disables priority
//! masking.
//!
//! Smaller numbers correspond to higher interrupt priorities.  So for example
//! a priority level mask of 4 allows interrupts of priority level 0-3,
//! and interrupts with a numerical priority of 4 and greater are blocked.
//!
//! The hardware priority mechanism only looks at the upper N bits of the
//! priority level (where N is 3 for the MSP432 family), so any
//! prioritization must be performed in those bits.
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_setPriorityMask(uint8_t priorityMask);

//*****************************************************************************
//
//! Gets the priority masking level
//!
//! This function gets the current setting of the interrupt priority masking
//! level.  The value returned is the priority level such that all interrupts
//! of that and lesser priority are masked.  A value of 0 means that priority
//! masking is disabled.
//!
//! Smaller numbers correspond to higher interrupt priorities.  So for example
//! a priority level mask of 4 allows interrupts of priority level 0-3,
//! and interrupts with a numerical priority of 4 and greater are blocked.
//!
//! The hardware priority mechanism only looks at the upper N bits of the
//! priority level (where N is 3 for the MSP432 family), so any
//! prioritization must be performed in those bits.
//!
//! \return Returns the value of the interrupt priority level mask.
//
//*****************************************************************************
extern uint8_t Interrupt_getPriorityMask(void);

//*****************************************************************************
//
//! Sets the address of the vector table. This function is for advanced users
//! who might want to switch between multiple instances of vector tables
//! (perhaps between flash/ram).
//!
//! \param addr is the new address of the vector table.
//!
//! \return None.
//
//*****************************************************************************
extern void Interrupt_setVectorTableAddress(uint32_t addr);

//*****************************************************************************
//
//! Returns the address of the interrupt vector table.
//!
//! \return Address of the vector table.
//
//*****************************************************************************
extern uint32_t Interrupt_getVectorTableAddress(void);

//*****************************************************************************
//
//! Enables the processor to sleep when exiting an ISR. For low power operation,
//! this is ideal as power cycles are not wasted with the processing required
//! for waking up from an ISR and going back to sleep.
//!
//! \return None
//
//*****************************************************************************
extern void Interrupt_enableSleepOnIsrExit(void);

//*****************************************************************************
//
//! Disables the processor to sleep when exiting an ISR.
//!
//! \return None
//
//*****************************************************************************
extern void Interrupt_disableSleepOnIsrExit(void);

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

//*****************************************************************************
//
// Close the Doxygen group.
//! @}
//
//*****************************************************************************

#endif // __INTERRUPT_H__
