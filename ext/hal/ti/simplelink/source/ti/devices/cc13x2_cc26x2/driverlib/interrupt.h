/******************************************************************************
*  Filename:       interrupt.h
*  Revised:        2017-11-14 15:26:03 +0100 (Tue, 14 Nov 2017)
*  Revision:       50272
*
*  Description:    Defines and prototypes for the NVIC Interrupt Controller
*
*  Copyright (c) 2015 - 2017, Texas Instruments Incorporated
*  All rights reserved.
*
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*
*  1) Redistributions of source code must retain the above copyright notice,
*     this list of conditions and the following disclaimer.
*
*  2) Redistributions in binary form must reproduce the above copyright notice,
*     this list of conditions and the following disclaimer in the documentation
*     and/or other materials provided with the distribution.
*
*  3) Neither the name of the ORGANIZATION nor the names of its contributors may
*     be used to endorse or promote products derived from this software without
*     specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
*  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
*  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
*  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
*  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
*  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
*  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
*  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
*  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
*  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
*  POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************/

//*****************************************************************************
//
//! \addtogroup system_cpu_group
//! @{
//! \addtogroup interrupt_api
//! @{
//
//*****************************************************************************

#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

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

#include <stdbool.h>
#include <stdint.h>
#include "../inc/hw_ints.h"
#include "../inc/hw_types.h"
#include "../inc/hw_nvic.h"
#include "debug.h"
#include "cpu.h"

//*****************************************************************************
//
// Support for DriverLib in ROM:
// This section renames all functions that are not "static inline", so that
// calling these functions will default to implementation in flash. At the end
// of this file a second renaming will change the defaults to implementation in
// ROM for available functions.
//
// To force use of the implementation in flash, e.g. for debugging:
// - Globally: Define DRIVERLIB_NOROM at project level
// - Per function: Use prefix "NOROM_" when calling the function
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #define IntRegister                     NOROM_IntRegister
    #define IntUnregister                   NOROM_IntUnregister
    #define IntPriorityGroupingSet          NOROM_IntPriorityGroupingSet
    #define IntPriorityGroupingGet          NOROM_IntPriorityGroupingGet
    #define IntPrioritySet                  NOROM_IntPrioritySet
    #define IntPriorityGet                  NOROM_IntPriorityGet
    #define IntEnable                       NOROM_IntEnable
    #define IntDisable                      NOROM_IntDisable
    #define IntPendSet                      NOROM_IntPendSet
    #define IntPendGet                      NOROM_IntPendGet
    #define IntPendClear                    NOROM_IntPendClear
#endif

//*****************************************************************************
//
// Macro to generate an interrupt priority mask based on the number of bits
// of priority supported by the hardware. For CC26xx the number of priority
// bit is 3 as defined in <tt>hw_types.h</tt>. The priority mask is
// defined as
//
// INT_PRIORITY_MASK = ((0xFF << (8 - NUM_PRIORITY_BITS)) & 0xFF)
//
//*****************************************************************************
#define INT_PRIORITY_MASK       0x000000E0
#define INT_PRI_LEVEL0          0x00000000
#define INT_PRI_LEVEL1          0x00000020
#define INT_PRI_LEVEL2          0x00000040
#define INT_PRI_LEVEL3          0x00000060
#define INT_PRI_LEVEL4          0x00000080
#define INT_PRI_LEVEL5          0x000000A0
#define INT_PRI_LEVEL6          0x000000C0
#define INT_PRI_LEVEL7          0x000000E0

//*****************************************************************************
//
// API Functions and prototypes
//
//*****************************************************************************

//*****************************************************************************
//
//! \brief Registers a function as an interrupt handler in the dynamic vector table.
//!
//! \note Only use this function if you want to use the dynamic vector table (in SRAM)!
//!
//! This function writes a function pointer to the dynamic interrupt vector table
//! in SRAM to register the function as an interrupt handler (ISR). When the corresponding
//! interrupt occurs, and it has been enabled (see \ref IntEnable()), the function
//! pointer is fetched from the dynamic vector table, and the System CPU will
//! execute the interrupt handler.
//!
//! \note The first call to this function (directly or indirectly via a peripheral
//! driver interrupt register function) copies the interrupt vector table from
//! Flash to SRAM. NVIC uses the static vector table (in Flash) until this function
//! is called.
//!
//! \param ui32Interrupt specifies the index in the vector table to modify.
//! - System exceptions (vectors 0 to 15):
//!   - INT_NMI_FAULT
//!   - INT_HARD_FAULT
//!   - INT_MEMMANAGE_FAULT
//!   - INT_BUS_FAULT
//!   - INT_USAGE_FAULT
//!   - INT_SVCALL
//!   - INT_DEBUG
//!   - INT_PENDSV
//!   - INT_SYSTICK
//! - Interrupts (vectors >15):
//!   - INT_AON_GPIO_EDGE
//!   - INT_I2C_IRQ
//!   - INT_RFC_CPE_1
//!   - INT_PKA_IRQ
//!   - INT_AON_RTC_COMB
//!   - INT_UART0_COMB
//!   - INT_AUX_SWEV0
//!   - INT_SSI0_COMB
//!   - INT_SSI1_COMB
//!   - INT_RFC_CPE_0
//!   - INT_RFC_HW_COMB
//!   - INT_RFC_CMD_ACK
//!   - INT_I2S_IRQ
//!   - INT_AUX_SWEV1
//!   - INT_WDT_IRQ
//!   - INT_GPT0A
//!   - INT_GPT0B
//!   - INT_GPT1A
//!   - INT_GPT1B
//!   - INT_GPT2A
//!   - INT_GPT2B
//!   - INT_GPT3A
//!   - INT_GPT3B
//!   - INT_CRYPTO_RESULT_AVAIL_IRQ
//!   - INT_DMA_DONE_COMB
//!   - INT_DMA_ERR
//!   - INT_FLASH
//!   - INT_SWEV0
//!   - INT_AUX_COMB
//!   - INT_AON_PROG0
//!   - INT_PROG0 (Programmable interrupt, see \ref EventRegister())
//!   - INT_AUX_COMPA
//!   - INT_AUX_ADC_IRQ
//!   - INT_TRNG_IRQ
//!   - INT_OSC_COMB
//!   - INT_AUX_TIMER2_EV0
//!   - INT_UART1_COMB
//!   - INT_BATMON_COMB
//! \param pfnHandler is a pointer to the function to register as interrupt handler.
//!
//! \return None.
//!
//! \sa \ref IntUnregister(), \ref IntEnable()
//
//*****************************************************************************
extern void IntRegister(uint32_t ui32Interrupt, void (*pfnHandler)(void));

//*****************************************************************************
//
//! \brief Unregisters an interrupt handler in the dynamic vector table.
//!
//! This function removes an interrupt handler from the dynamic vector table and
//! replaces it with the default interrupt handler \ref IntDefaultHandler().
//!
//! \note Remember to disable the interrupt before removing its interrupt handler
//! from the vector table.
//!
//! \param ui32Interrupt specifies the index in the vector table to modify.
//! - See \ref IntRegister() for list of valid arguments.
//!
//! \return None.
//!
//! \sa \ref IntRegister(), \ref IntDisable()
//
//*****************************************************************************
extern void IntUnregister(uint32_t ui32Interrupt);

//*****************************************************************************
//
//! \brief Sets the priority grouping of the interrupt controller.
//!
//! This function specifies the split between preemptable priority levels and
//! subpriority levels in the interrupt priority specification.
//!
//! Three bits are available for hardware interrupt prioritization thus priority
//! grouping values of three through seven have the same effect.
//!
//! \param ui32Bits specifies the number of bits of preemptable priority.
//! - 0   : No pre-emption priority, eight bits of subpriority.
//! - 1   : One bit of pre-emption priority, seven bits of subpriority
//! - 2   : Two bits of pre-emption priority, six bits of subpriority
//! - 3-7 : Three bits of pre-emption priority, five bits of subpriority
//!
//! \return None
//!
//! \sa \ref IntPrioritySet()
//
//*****************************************************************************
extern void IntPriorityGroupingSet(uint32_t ui32Bits);

//*****************************************************************************
//
//! \brief Gets the priority grouping of the interrupt controller.
//!
//! This function returns the split between preemptable priority levels and
//! subpriority levels in the interrupt priority specification.
//!
//! \return Returns the number of bits of preemptable priority.
//! - 0   : No pre-emption priority, eight bits of subpriority.
//! - 1   : One bit of pre-emption priority, seven bits of subpriority
//! - 2   : Two bits of pre-emption priority, six bits of subpriority
//! - 3-7 : Three bits of pre-emption priority, five bits of subpriority
//
//*****************************************************************************
extern uint32_t IntPriorityGroupingGet(void);

//*****************************************************************************
//
//! \brief Sets the priority of an interrupt.
//!
//! This function sets the priority of an interrupt, including system exceptions.
//! When multiple interrupts are asserted simultaneously, the ones with the highest
//! priority are processed before the lower priority interrupts. Smaller numbers
//! correspond to higher interrupt priorities thus priority 0 is the highest
//! interrupt priority.
//!
//! \warning This function does not support setting priority of interrupt vectors
//! one through three which are:
//! - 1: Reset handler
//! - 2: NMI handler
//! - 3: Hard fault handler
//!
//! \param ui32Interrupt specifies the index in the vector table to change priority for.
//! - System exceptions:
//!   - INT_MEMMANAGE_FAULT
//!   - INT_BUS_FAULT
//!   - INT_USAGE_FAULT
//!   - INT_SVCALL
//!   - INT_DEBUG
//!   - INT_PENDSV
//!   - INT_SYSTICK
//! - Interrupts:
//!   - INT_AON_GPIO_EDGE
//!   - INT_I2C_IRQ
//!   - INT_RFC_CPE_1
//!   - INT_PKA_IRQ
//!   - INT_AON_RTC_COMB
//!   - INT_UART0_COMB
//!   - INT_AUX_SWEV0
//!   - INT_SSI0_COMB
//!   - INT_SSI1_COMB
//!   - INT_RFC_CPE_0
//!   - INT_RFC_HW_COMB
//!   - INT_RFC_CMD_ACK
//!   - INT_I2S_IRQ
//!   - INT_AUX_SWEV1
//!   - INT_WDT_IRQ
//!   - INT_GPT0A
//!   - INT_GPT0B
//!   - INT_GPT1A
//!   - INT_GPT1B
//!   - INT_GPT2A
//!   - INT_GPT2B
//!   - INT_GPT3A
//!   - INT_GPT3B
//!   - INT_CRYPTO_RESULT_AVAIL_IRQ
//!   - INT_DMA_DONE_COMB
//!   - INT_DMA_ERR
//!   - INT_FLASH
//!   - INT_SWEV0
//!   - INT_AUX_COMB
//!   - INT_AON_PROG0
//!   - INT_PROG0 (Programmable interrupt, see \ref EventRegister())
//!   - INT_AUX_COMPA
//!   - INT_AUX_ADC_IRQ
//!   - INT_TRNG_IRQ
//!   - INT_OSC_COMB
//!   - INT_AUX_TIMER2_EV0
//!   - INT_UART1_COMB
//!   - INT_BATMON_COMB
//! \param ui8Priority specifies the priority of the interrupt.
//! - \ref INT_PRI_LEVEL0 : Highest priority.
//! - \ref INT_PRI_LEVEL1
//! - \ref INT_PRI_LEVEL2
//! - \ref INT_PRI_LEVEL3
//! - \ref INT_PRI_LEVEL4
//! - \ref INT_PRI_LEVEL5
//! - \ref INT_PRI_LEVEL6
//! - \ref INT_PRI_LEVEL7 : Lowest priority.
//!
//! \return None
//!
//! \sa \ref IntPriorityGroupingSet()
//
//*****************************************************************************
extern void IntPrioritySet(uint32_t ui32Interrupt, uint8_t ui8Priority);

//*****************************************************************************
//
//! \brief Gets the priority of an interrupt.
//!
//! This function gets the priority of an interrupt.
//!
//! \warning This function does not support getting priority of interrupt vectors
//! one through three which are:
//! - 1: Reset handler
//! - 2: NMI handler
//! - 3: Hard fault handler
//!
//! \param ui32Interrupt specifies the index in the vector table to read priority of.
//! - See \ref IntPrioritySet() for list of valid arguments.
//!
//! \return Returns the interrupt priority:
//! - \ref INT_PRI_LEVEL0 : Highest priority.
//! - \ref INT_PRI_LEVEL1
//! - \ref INT_PRI_LEVEL2
//! - \ref INT_PRI_LEVEL3
//! - \ref INT_PRI_LEVEL4
//! - \ref INT_PRI_LEVEL5
//! - \ref INT_PRI_LEVEL6
//! - \ref INT_PRI_LEVEL7 : Lowest priority.
//
//*****************************************************************************
extern int32_t IntPriorityGet(uint32_t ui32Interrupt);

//*****************************************************************************
//
//! \brief Enables an interrupt or system exception.
//!
//! This function enables the specified interrupt in the interrupt controller.
//!
//! \note If a fault condition occurs while the corresponding system exception
//! is disabled, the fault is treated as a Hard Fault.
//!
//! \param ui32Interrupt specifies the index in the vector table to enable.
//! - System exceptions:
//!   - INT_MEMMANAGE_FAULT
//!   - INT_BUS_FAULT
//!   - INT_USAGE_FAULT
//!   - INT_SYSTICK
//! - Interrupts:
//!   - INT_AON_GPIO_EDGE
//!   - INT_I2C_IRQ
//!   - INT_RFC_CPE_1
//!   - INT_PKA_IRQ
//!   - INT_AON_RTC_COMB
//!   - INT_UART0_COMB
//!   - INT_AUX_SWEV0
//!   - INT_SSI0_COMB
//!   - INT_SSI1_COMB
//!   - INT_RFC_CPE_0
//!   - INT_RFC_HW_COMB
//!   - INT_RFC_CMD_ACK
//!   - INT_I2S_IRQ
//!   - INT_AUX_SWEV1
//!   - INT_WDT_IRQ
//!   - INT_GPT0A
//!   - INT_GPT0B
//!   - INT_GPT1A
//!   - INT_GPT1B
//!   - INT_GPT2A
//!   - INT_GPT2B
//!   - INT_GPT3A
//!   - INT_GPT3B
//!   - INT_CRYPTO_RESULT_AVAIL_IRQ
//!   - INT_DMA_DONE_COMB
//!   - INT_DMA_ERR
//!   - INT_FLASH
//!   - INT_SWEV0
//!   - INT_AUX_COMB
//!   - INT_AON_PROG0
//!   - INT_PROG0 (Programmable interrupt, see \ref EventRegister())
//!   - INT_AUX_COMPA
//!   - INT_AUX_ADC_IRQ
//!   - INT_TRNG_IRQ
//!   - INT_OSC_COMB
//!   - INT_AUX_TIMER2_EV0
//!   - INT_UART1_COMB
//!   - INT_BATMON_COMB
//!
//! \return None
//!
//! \sa \ref IntDisable()
//
//*****************************************************************************
extern void IntEnable(uint32_t ui32Interrupt);

//*****************************************************************************
//
//! \brief Disables an interrupt or system exception.
//!
//! This function disables the specified interrupt in the interrupt controller.
//!
//! \param ui32Interrupt specifies the index in the vector table to disable.
//! - See \ref IntEnable() for list of valid arguments.
//!
//! \return None
//!
//! \sa \ref IntEnable()
//
//*****************************************************************************
extern void IntDisable(uint32_t ui32Interrupt);

//*****************************************************************************
//
//! \brief Pends an interrupt.
//!
//! This function pends the specified interrupt in the interrupt controller.
//! This causes the interrupt controller to execute the corresponding interrupt
//! handler at the next available time, based on the current interrupt state
//! priorities.
//!
//! This interrupt controller automatically clears the pending interrupt once the
//! interrupt handler is executed.
//!
//! \param ui32Interrupt specifies the index in the vector table to pend.
//! - System exceptions:
//!   - INT_NMI_FAULT
//!   - INT_PENDSV
//!   - INT_SYSTICK
//! - Interrupts:
//!   - INT_AON_GPIO_EDGE
//!   - INT_I2C_IRQ
//!   - INT_RFC_CPE_1
//!   - INT_PKA_IRQ
//!   - INT_AON_RTC_COMB
//!   - INT_UART0_COMB
//!   - INT_AUX_SWEV0
//!   - INT_SSI0_COMB
//!   - INT_SSI1_COMB
//!   - INT_RFC_CPE_0
//!   - INT_RFC_HW_COMB
//!   - INT_RFC_CMD_ACK
//!   - INT_I2S_IRQ
//!   - INT_AUX_SWEV1
//!   - INT_WDT_IRQ
//!   - INT_GPT0A
//!   - INT_GPT0B
//!   - INT_GPT1A
//!   - INT_GPT1B
//!   - INT_GPT2A
//!   - INT_GPT2B
//!   - INT_GPT3A
//!   - INT_GPT3B
//!   - INT_CRYPTO_RESULT_AVAIL_IRQ
//!   - INT_DMA_DONE_COMB
//!   - INT_DMA_ERR
//!   - INT_FLASH
//!   - INT_SWEV0
//!   - INT_AUX_COMB
//!   - INT_AON_PROG0
//!   - INT_PROG0 (Programmable interrupt, see \ref EventRegister())
//!   - INT_AUX_COMPA
//!   - INT_AUX_ADC_IRQ
//!   - INT_TRNG_IRQ
//!   - INT_OSC_COMB
//!   - INT_AUX_TIMER2_EV0
//!   - INT_UART1_COMB
//!   - INT_BATMON_COMB
//!
//! \return None
//!
//! \sa \ref IntEnable()
//
//*****************************************************************************
extern void IntPendSet(uint32_t ui32Interrupt);

//*****************************************************************************
//
//! \brief Checks if an interrupt is pending.
//!
//! This function checks the interrupt controller to see if an interrupt is pending.
//!
//! The interrupt must be enabled in order for the corresponding interrupt handler
//! to be executed, so an interrupt can be pending waiting to be enabled or waiting
//! for an interrupt of higher priority to be done executing.
//!
//! \note This function does not support reading pending status for system exceptions
//! (vector table indexes <16).
//!
//! \param ui32Interrupt specifies the index in the vector table to check pending
//! status for.
//! - See \ref IntPendSet() for list of valid arguments (except system exceptions).
//!
//! \return Returns:
//! - \c true  : Specified interrupt is pending.
//! - \c false : Specified interrupt is not pending.
//
//*****************************************************************************
extern bool IntPendGet(uint32_t ui32Interrupt);

//*****************************************************************************
//
//! \brief Unpends an interrupt.
//!
//! This function unpends the specified interrupt in the interrupt controller.
//! This causes any previously generated interrupts that have not been handled yet
//! (due to higher priority interrupts or the interrupt no having been enabled
//! yet) to be discarded.
//!
//! \note It is not possible to unpend the NMI because it takes effect
//! immediately when being pended.
//!
//! \param ui32Interrupt specifies the index in the vector table to unpend.
//! - See \ref IntPendSet() for list of valid arguments (except NMI).
//!
//! \return None
//
//*****************************************************************************
extern void IntPendClear(uint32_t ui32Interrupt);

//*****************************************************************************
//
//! \brief Enables the CPU interrupt.
//!
//! Allows the CPU to respond to interrupts.
//!
//! \return Returns:
//! - \c true  : Interrupts were disabled and are now enabled.
//! - \c false : Interrupts were already enabled when the function was called.
//
//*****************************************************************************
__STATIC_INLINE bool
IntMasterEnable(void)
{
    // Enable CPU interrupts.
    return(CPUcpsie());
}

//*****************************************************************************
//
//! \brief Disables the CPU interrupts with configurable priority.
//!
//! Prevents the CPU from receiving interrupts except NMI and hard fault. This
//! does not affect the set of interrupts enabled in the interrupt controller;
//! it just gates the interrupt from the interrupt controller to the CPU.
//!
//! \return Returns:
//! - \c true  : Interrupts were already disabled when the function was called.
//! - \c false : Interrupts were enabled and are now disabled.
//
//*****************************************************************************
__STATIC_INLINE bool
IntMasterDisable(void)
{
    // Disable CPU interrupts.
    return(CPUcpsid());
}

//*****************************************************************************
//
//! \brief Sets the priority masking level.
//!
//! This function sets the interrupt priority masking level so that all
//! interrupts at the specified or lesser priority level are masked. This
//! can be used to globally disable a set of interrupts with priority below
//! a predetermined threshold. A value of 0 disables priority
//! masking.
//!
//! Smaller numbers correspond to higher interrupt priorities. So for example
//! a priority level mask of 4 will allow interrupts of priority level 0-3,
//! and interrupts with a numerical priority of 4 and greater will be blocked.
//! The device supports priority levels 0 through 7.
//!
//! \param ui32PriorityMask is the priority level that will be masked.
//! - 0 : Disable priority masking.
//! - 1 : Allow priority 0 interrupts, mask interrupts with priority 1-7.
//! - 2 : Allow priority 0-1 interrupts, mask interrupts with priority 2-7.
//! - 3 : Allow priority 0-2 interrupts, mask interrupts with priority 3-7.
//! - 4 : Allow priority 0-3 interrupts, mask interrupts with priority 4-7.
//! - 5 : Allow priority 0-4 interrupts, mask interrupts with priority 5-7.
//! - 6 : Allow priority 0-5 interrupts, mask interrupts with priority 6-7.
//! - 7 : Allow priority 0-6 interrupts, mask interrupts with priority 7.
//!
//! \return None.
//
//*****************************************************************************
__STATIC_INLINE void
IntPriorityMaskSet(uint32_t ui32PriorityMask)
{
    CPUbasepriSet(ui32PriorityMask);
}

//*****************************************************************************
//
//! \brief Gets the priority masking level.
//!
//! This function gets the current setting of the interrupt priority masking
//! level. The value returned is the priority level such that all interrupts
//! of that and lesser priority are masked. A value of 0 means that priority
//! masking is disabled.
//!
//! Smaller numbers correspond to higher interrupt priorities. So for example
//! a priority level mask of 4 will allow interrupts of priority level 0-3,
//! and interrupts with a numerical priority of 4 and greater will be blocked.
//!
//! \return Returns the value of the interrupt priority level mask.
//
//*****************************************************************************
__STATIC_INLINE uint32_t
IntPriorityMaskGet(void)
{
    return(CPUbasepriGet());
}

//*****************************************************************************
//
// Support for DriverLib in ROM:
// Redirect to implementation in ROM when available.
//
//*****************************************************************************
#if !defined(DRIVERLIB_NOROM) && !defined(DOXYGEN)
    #include "../driverlib/rom.h"
    #ifdef ROM_IntRegister
        #undef  IntRegister
        #define IntRegister                     ROM_IntRegister
    #endif
    #ifdef ROM_IntUnregister
        #undef  IntUnregister
        #define IntUnregister                   ROM_IntUnregister
    #endif
    #ifdef ROM_IntPriorityGroupingSet
        #undef  IntPriorityGroupingSet
        #define IntPriorityGroupingSet          ROM_IntPriorityGroupingSet
    #endif
    #ifdef ROM_IntPriorityGroupingGet
        #undef  IntPriorityGroupingGet
        #define IntPriorityGroupingGet          ROM_IntPriorityGroupingGet
    #endif
    #ifdef ROM_IntPrioritySet
        #undef  IntPrioritySet
        #define IntPrioritySet                  ROM_IntPrioritySet
    #endif
    #ifdef ROM_IntPriorityGet
        #undef  IntPriorityGet
        #define IntPriorityGet                  ROM_IntPriorityGet
    #endif
    #ifdef ROM_IntEnable
        #undef  IntEnable
        #define IntEnable                       ROM_IntEnable
    #endif
    #ifdef ROM_IntDisable
        #undef  IntDisable
        #define IntDisable                      ROM_IntDisable
    #endif
    #ifdef ROM_IntPendSet
        #undef  IntPendSet
        #define IntPendSet                      ROM_IntPendSet
    #endif
    #ifdef ROM_IntPendGet
        #undef  IntPendGet
        #define IntPendGet                      ROM_IntPendGet
    #endif
    #ifdef ROM_IntPendClear
        #undef  IntPendClear
        #define IntPendClear                    ROM_IntPendClear
    #endif
#endif

//*****************************************************************************
//
// Mark the end of the C bindings section for C++ compilers.
//
//*****************************************************************************
#ifdef __cplusplus
}
#endif

#endif // __INTERRUPT_H__

//*****************************************************************************
//
//! Close the Doxygen group.
//! @}
//! @}
//
//*****************************************************************************
