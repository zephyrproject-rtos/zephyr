/******************************************************************************
*  Filename:       interrupt_doc.h
*  Revised:        2017-11-14 15:26:03 +0100 (Tue, 14 Nov 2017)
*  Revision:       50272
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
//! \addtogroup interrupt_api
//! @{
//! \section sec_interrupt Introduction
//!
//! The interrupt controller API provides a set of functions for dealing with the
//! Nested Vectored Interrupt Controller (NVIC). Functions are provided to enable
//! and disable interrupts, register interrupt handlers, and set the priority of
//! interrupts.
//!
//! The event sources that trigger the interrupt lines in the NVIC are controlled by
//! the MCU event fabric. All event sources are statically connected to the NVIC interrupt lines
//! except one which is programmable. For more information about the MCU event fabric, see the
//! [MCU event fabric API](\ref event_api).
//!
//! \section sec_interrupt_api API
//!
//! Interrupts and system exceptions must be individually enabled and disabled through:
//! - \ref IntEnable()
//! - \ref IntDisable()
//!
//! The global CPU interrupt can be enabled and disabled with the following functions:
//! - \ref IntMasterEnable()
//! - \ref IntMasterDisable()
//!
//! This does not affect the individual interrupt enable states. Masking of the CPU
//! interrupt can be used as a simple critical section (only an NMI can interrupt the
//! CPU while the CPU interrupt is disabled), although masking the CPU
//! interrupt can increase the interrupt response time.
//!
//! It is possible to access the NVIC to see if any interrupts are pending and manually
//! clear pending interrupts which have not yet been serviced or set a specific interrupt as
//! pending to be handled based on its priority. Pending interrupts are cleared automatically
//! when the interrupt is accepted and executed. However, the event source which caused the
//! interrupt might need to be cleared manually to avoid re-triggering the corresponding interrupt.
//! The functions to read, clear, and set pending interrupts are:
//! - \ref IntPendGet()
//! - \ref IntPendClear()
//! - \ref IntPendSet()
//!
//! The interrupt prioritization in the NVIC allows handling of higher priority interrupts
//! before lower priority interrupts, as well as allowing preemption of lower priority interrupt
//! handlers by higher priority interrupts.
//! The device supports eight priority levels from 0 to 7 with 0 being the highest priority.
//! The priority of each interrupt source can be set and examined using:
//! - \ref IntPrioritySet()
//! - \ref IntPriorityGet()
//!
//! Interrupts can be masked based on their priority such that interrupts with the same or lower
//! priority than the mask are effectively disabled. This can be configured with:
//! - \ref IntPriorityMaskSet()
//! - \ref IntPriorityMaskGet()
//!
//! Subprioritization is also possible. Instead of having three bits of preemptable
//! prioritization (eight levels), the NVIC can be configured for 3 - M bits of
//! preemptable prioritization and M bits of subpriority. In this scheme, two
//! interrupts with the same preemptable prioritization but different subpriorities
//! do not cause a preemption. Instead, tail chaining is used to process
//! the two interrupts back-to-back.
//! If two interrupts with the same priority (and subpriority if so configured) are
//! asserted at the same time, the one with the lower interrupt number is
//! processed first.
//! Subprioritization is handled by:
//! - \ref IntPriorityGroupingSet()
//! - \ref IntPriorityGroupingGet()
//!
//! \section sec_interrupt_table Interrupt Vector Table
//!
//! The interrupt vector table can be configured in one of two ways:
//! - Statically (at compile time): Vector table is placed in Flash and each entry has a fixed
//!   pointer to an interrupt handler (ISR).
//! - Dynamically (at runtime): Vector table is placed in SRAM and each entry can be changed
//!   (registered or unregistered) at runtime. This allows a single interrupt to trigger different
//!   interrupt handlers (ISRs) depending on which interrupt handler is registered at the time the
//!   System CPU responds to the interrupt.
//!
//! When configured, the interrupts must be explicitly enabled in the NVIC through \ref IntEnable()
//! before the CPU can respond to the interrupt (in addition to any interrupt enabling required
//! within the peripheral).
//!
//! \subsection sec_interrupt_table_static Static Vector Table
//!
//! Static registration of interrupt handlers is accomplished by editing the interrupt handler
//! table in the startup code of the application. Texas Instruments provides startup files for
//! each supported compiler ( \ti_code{startup_<compiler>.c} ) and these startup files include
//! a default static interrupt vector table.
//! All entries, except ResetISR, are declared as \c extern with weak assignment to a default
//! interrupt handler. This allows the user to declare and define a function (in the user's code)
//! with the same name as an entry in the vector table. At compile time, the linker then replaces
//! the pointer to the default interrupt handler in the vector table with the pointer to the
//! interrupt handler defined by the user.
//!
//! Statically configuring the interrupt table provides the fastest interrupt response time
//! because the stacking operation (a write to SRAM on the data bus) is performed in parallel
//! with the interrupt handler table fetch (a read from Flash on the instruction bus), as well
//! as the prefetch of the interrupt handler (assuming it is also in Flash).
//!
//! \subsection sec_interrupt_table_dynamic Dynamic Vector Table
//!
//! Alternatively, interrupts can be registered in the vector table at runtime, thus dynamically.
//! The dynamic vector table is placed in SRAM and the code can then modify the pointers to
//! interrupt handlers throughout the application.
//!
//! DriverLib uses these two functions to modify the dynamic vector table:
//! - \ref IntRegister() : Write a pointer to an interrupt handler into the vector table.
//! - \ref IntUnregister() : Write pointer to default interrupt handler into the vector table.
//!
//! \note First call to \ref IntRegister() initializes the vector table in SRAM by copying the
//! static vector table from Flash and forcing the NVIC to use the dynamic vector table from
//! this point forward. If using the dynamic vector table it is highly recommended to
//! initialize it during the setup phase of the application. The NVIC uses the static vector
//! table in Flash until the application initializes the dynamic vector table in SRAM.
//!
//! Runtime configuration of interrupts adds a small latency to the interrupt response time
//! because the stacking operation (a write to SRAM on the data bus) and the interrupt handler
//! fetch from the vector table (a read from SRAM on the instruction bus) must be performed
//! sequentially.
//!
//! The dynamic vector table, \ref g_pfnRAMVectors, is placed in SRAM in the section called
//! \c vtable_ram which is a section defined in the linker file. By default the linker file
//! places this section at the start of the SRAM but this is configurable by the user.
//!
//! \warning Runtime configuration of interrupt handlers requires that the interrupt
//! handler table is placed on a 256-byte boundary in SRAM (typically, this is
//! at the beginning of SRAM). Failure to do so results in an incorrect vector
//! address being fetched in response to an interrupt.
//!
//! @}
