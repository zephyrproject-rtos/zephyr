/******************************************************************************
*  Filename:       interrupt.c
*  Revised:        2017-05-19 11:31:39 +0200 (Fri, 19 May 2017)
*  Revision:       49017
*
*  Description:    Driver for the NVIC Interrupt Controller.
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

#include "interrupt.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  IntRegister
    #define IntRegister                     NOROM_IntRegister
    #undef  IntUnregister
    #define IntUnregister                   NOROM_IntUnregister
    #undef  IntPriorityGroupingSet
    #define IntPriorityGroupingSet          NOROM_IntPriorityGroupingSet
    #undef  IntPriorityGroupingGet
    #define IntPriorityGroupingGet          NOROM_IntPriorityGroupingGet
    #undef  IntPrioritySet
    #define IntPrioritySet                  NOROM_IntPrioritySet
    #undef  IntPriorityGet
    #define IntPriorityGet                  NOROM_IntPriorityGet
    #undef  IntEnable
    #define IntEnable                       NOROM_IntEnable
    #undef  IntDisable
    #define IntDisable                      NOROM_IntDisable
    #undef  IntPendSet
    #define IntPendSet                      NOROM_IntPendSet
    #undef  IntPendGet
    #define IntPendGet                      NOROM_IntPendGet
    #undef  IntPendClear
    #define IntPendClear                    NOROM_IntPendClear
#endif

//*****************************************************************************
//
//! This is a mapping between priority grouping encodings and the number of
//! preemption priority bits.
//
//*****************************************************************************
static const uint32_t g_pui32Priority[] =
{
    NVIC_APINT_PRIGROUP_0_8, NVIC_APINT_PRIGROUP_1_7, NVIC_APINT_PRIGROUP_2_6,
    NVIC_APINT_PRIGROUP_3_5, NVIC_APINT_PRIGROUP_4_4, NVIC_APINT_PRIGROUP_5_3,
    NVIC_APINT_PRIGROUP_6_2, NVIC_APINT_PRIGROUP_7_1
};

//*****************************************************************************
//
//! This is a mapping between interrupt number and the register that contains
//! the priority encoding for that interrupt.
//
//*****************************************************************************
static const uint32_t g_pui32Regs[] =
{
    0, NVIC_SYS_PRI1, NVIC_SYS_PRI2, NVIC_SYS_PRI3, NVIC_PRI0, NVIC_PRI1,
    NVIC_PRI2, NVIC_PRI3, NVIC_PRI4, NVIC_PRI5, NVIC_PRI6, NVIC_PRI7,
    NVIC_PRI8, NVIC_PRI9, NVIC_PRI10, NVIC_PRI11, NVIC_PRI12, NVIC_PRI13
};

//*****************************************************************************
//
//! \brief The default interrupt handler.
//!
//! This is the default interrupt handler for all interrupts. It simply loops
//! forever so that the system state is preserved for observation by a
//! debugger. Since interrupts should be disabled before unregistering the
//! corresponding handler, this should never be called.
//!
//! \return None
//
//*****************************************************************************
static void
IntDefaultHandler(void)
{
    // Go into an infinite loop.
    while(1)
    {
    }
}

//*****************************************************************************
//
//! \brief Global pointer to the (dynamic) interrupt vector table when placed in SRAM.
//!
//! Interrupt vector table is placed at "vtable_ram" defined in the linker file
//! provided by Texas Instruments. By default, this is at the beginning of SRAM.
//!
//! \note See \ti_code{interrupt.c} for compiler specific implementation!
//
//*****************************************************************************
#if defined(DOXYGEN)
// Dummy void pointer used as placeholder to generate Doxygen documentation.
void (*g_pfnRAMVectors[NUM_INTERRUPTS])(void);
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=256
static __no_init void (*g_pfnRAMVectors[NUM_INTERRUPTS])(void) @ ".vtable_ram";
#elif defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(g_pfnRAMVectors, 256)
#pragma DATA_SECTION(g_pfnRAMVectors, ".vtable_ram")
void (*g_pfnRAMVectors[NUM_INTERRUPTS])(void);
#elif defined (__CC_ARM)
static __attribute__((section("vtable_ram")))
void (*g_pfnRAMVectors[NUM_INTERRUPTS])(void) __attribute__((aligned(256)));
#else
static __attribute__((section("vtable_ram")))
void (*g_pfnRAMVectors[NUM_INTERRUPTS])(void) __attribute__((aligned(256)));
#endif

//*****************************************************************************
//
// Registers a function to be called when an interrupt occurs.
//
//*****************************************************************************
void
IntRegister(uint32_t ui32Interrupt, void (*pfnHandler)(void))
{
    uint32_t ui32Idx, ui32Value;

    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Make sure that the RAM vector table is correctly aligned.
    ASSERT(((uint32_t)g_pfnRAMVectors & 0x000000ff) == 0);

    // See if the RAM vector table has been initialized.
    if(HWREG(NVIC_VTABLE) != (uint32_t)g_pfnRAMVectors)
    {
        // Copy the vector table from the beginning of FLASH to the RAM vector
        // table.
        ui32Value = HWREG(NVIC_VTABLE);
        for(ui32Idx = 0; ui32Idx < NUM_INTERRUPTS; ui32Idx++)
        {
            g_pfnRAMVectors[ui32Idx] = (void (*)(void))HWREG((ui32Idx * 4) +
                                       ui32Value);
        }

        // Point NVIC at the RAM vector table.
        HWREG(NVIC_VTABLE) = (uint32_t)g_pfnRAMVectors;
    }

    // Save the interrupt handler.
    g_pfnRAMVectors[ui32Interrupt] = pfnHandler;
}

//*****************************************************************************
//
// Unregisters the function to be called when an interrupt occurs.
//
//*****************************************************************************
void
IntUnregister(uint32_t ui32Interrupt)
{
    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Reset the interrupt handler.
    g_pfnRAMVectors[ui32Interrupt] = IntDefaultHandler;
}

//*****************************************************************************
//
// Sets the priority grouping of the interrupt controller.
//
//*****************************************************************************
void
IntPriorityGroupingSet(uint32_t ui32Bits)
{
    // Check the arguments.
    ASSERT(ui32Bits < NUM_PRIORITY);

    // Set the priority grouping.
    HWREG(NVIC_APINT) = NVIC_APINT_VECTKEY | g_pui32Priority[ui32Bits];
}

//*****************************************************************************
//
// Gets the priority grouping of the interrupt controller
//
//*****************************************************************************
uint32_t
IntPriorityGroupingGet(void)
{
    uint32_t ui32Loop, ui32Value;

    // Read the priority grouping.
    ui32Value = HWREG(NVIC_APINT) & NVIC_APINT_PRIGROUP_M;

    // Loop through the priority grouping values.
    for(ui32Loop = 0; ui32Loop < NUM_PRIORITY; ui32Loop++)
    {
        // Stop looping if this value matches.
        if(ui32Value == g_pui32Priority[ui32Loop])
        {
            break;
        }
    }

    // Return the number of priority bits.
    return(ui32Loop);
}

//*****************************************************************************
//
// Sets the priority of an interrupt
//
//*****************************************************************************
void
IntPrioritySet(uint32_t ui32Interrupt, uint8_t ui8Priority)
{
    uint32_t ui32Temp;

    // Check the arguments.
    ASSERT((ui32Interrupt >= 4) && (ui32Interrupt < NUM_INTERRUPTS));
    ASSERT(ui8Priority <= INT_PRI_LEVEL7);

    // Set the interrupt priority.
    ui32Temp = HWREG(g_pui32Regs[ui32Interrupt >> 2]);
    ui32Temp &= ~(0xFF << (8 * (ui32Interrupt & 3)));
    ui32Temp |= ui8Priority << (8 * (ui32Interrupt & 3));
    HWREG(g_pui32Regs[ui32Interrupt >> 2]) = ui32Temp;
}

//*****************************************************************************
//
// Gets the priority of an interrupt
//
//*****************************************************************************
int32_t
IntPriorityGet(uint32_t ui32Interrupt)
{
    // Check the arguments.
    ASSERT((ui32Interrupt >= 4) && (ui32Interrupt < NUM_INTERRUPTS));

    // Return the interrupt priority.
    return((HWREG(g_pui32Regs[ui32Interrupt >> 2]) >> (8 * (ui32Interrupt & 3))) &
           0xFF);
}

//*****************************************************************************
//
// Enables an interrupt
//
//*****************************************************************************
void
IntEnable(uint32_t ui32Interrupt)
{
    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Determine the interrupt to enable.
    if(ui32Interrupt == INT_MEMMANAGE_FAULT)
    {
        // Enable the MemManage interrupt.
        HWREG(NVIC_SYS_HND_CTRL) |= NVIC_SYS_HND_CTRL_MEM;
    }
    else if(ui32Interrupt == INT_BUS_FAULT)
    {
        // Enable the bus fault interrupt.
        HWREG(NVIC_SYS_HND_CTRL) |= NVIC_SYS_HND_CTRL_BUS;
    }
    else if(ui32Interrupt == INT_USAGE_FAULT)
    {
        // Enable the usage fault interrupt.
        HWREG(NVIC_SYS_HND_CTRL) |= NVIC_SYS_HND_CTRL_USAGE;
    }
    else if(ui32Interrupt == INT_SYSTICK)
    {
        // Enable the System Tick interrupt.
        HWREG(NVIC_ST_CTRL) |= NVIC_ST_CTRL_INTEN;
    }
    else if((ui32Interrupt >= 16) && (ui32Interrupt <= 47))
    {
        // Enable the general interrupt.
        HWREG(NVIC_EN0) = 1 << (ui32Interrupt - 16);
    }
    else if(ui32Interrupt >= 48)
    {
        // Enable the general interrupt.
        HWREG(NVIC_EN1) = 1 << (ui32Interrupt - 48);
    }
}

//*****************************************************************************
//
// Disables an interrupt
//
//*****************************************************************************
void
IntDisable(uint32_t ui32Interrupt)
{
    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Determine the interrupt to disable.
    if(ui32Interrupt == INT_MEMMANAGE_FAULT)
    {
        // Disable the MemManage interrupt.
        HWREG(NVIC_SYS_HND_CTRL) &= ~(NVIC_SYS_HND_CTRL_MEM);
    }
    else if(ui32Interrupt == INT_BUS_FAULT)
    {
        // Disable the bus fault interrupt.
        HWREG(NVIC_SYS_HND_CTRL) &= ~(NVIC_SYS_HND_CTRL_BUS);
    }
    else if(ui32Interrupt == INT_USAGE_FAULT)
    {
        // Disable the usage fault interrupt.
        HWREG(NVIC_SYS_HND_CTRL) &= ~(NVIC_SYS_HND_CTRL_USAGE);
    }
    else if(ui32Interrupt == INT_SYSTICK)
    {
        // Disable the System Tick interrupt.
        HWREG(NVIC_ST_CTRL) &= ~(NVIC_ST_CTRL_INTEN);
    }
    else if((ui32Interrupt >= 16) && (ui32Interrupt <= 47))
    {
        // Disable the general interrupt.
        HWREG(NVIC_DIS0) = 1 << (ui32Interrupt - 16);
    }
    else if(ui32Interrupt >= 48)
    {
        // Disable the general interrupt.
        HWREG(NVIC_DIS1) = 1 << (ui32Interrupt - 48);
    }
}

//*****************************************************************************
//
// Pends an interrupt
//
//*****************************************************************************
void
IntPendSet(uint32_t ui32Interrupt)
{
    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Determine the interrupt to pend.
    if(ui32Interrupt == INT_NMI_FAULT)
    {
        // Pend the NMI interrupt.
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_NMI_SET;
    }
    else if(ui32Interrupt == INT_PENDSV)
    {
        // Pend the PendSV interrupt.
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PEND_SV;
    }
    else if(ui32Interrupt == INT_SYSTICK)
    {
        // Pend the SysTick interrupt.
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PENDSTSET;
    }
    else if((ui32Interrupt >= 16) && (ui32Interrupt <= 47))
    {
        // Pend the general interrupt.
        HWREG(NVIC_PEND0) = 1 << (ui32Interrupt - 16);
    }
    else if(ui32Interrupt >= 48)
    {
        // Pend the general interrupt.
        HWREG(NVIC_PEND1) = 1 << (ui32Interrupt - 48);
    }
}

//*****************************************************************************
//
// Query whether an interrupt is pending
//
//*****************************************************************************
bool
IntPendGet(uint32_t ui32Interrupt)
{
    uint32_t ui32IntPending;

    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Assume no interrupts are pending.
    ui32IntPending = 0;

    // The lower 16 IRQ vectors are unsupported by this function
    if (ui32Interrupt < 16)
    {

       return 0;
    }

    // Subtract lower 16 irq vectors
    ui32Interrupt -= 16;

    // Check if the interrupt is pending
    ui32IntPending = HWREG(NVIC_PEND0 + (ui32Interrupt / 32));
    ui32IntPending &= (1 << (ui32Interrupt & 31));

    return ui32IntPending ? true : false;
}

//*****************************************************************************
//
// Unpends an interrupt
//
//*****************************************************************************
void
IntPendClear(uint32_t ui32Interrupt)
{
    // Check the arguments.
    ASSERT(ui32Interrupt < NUM_INTERRUPTS);

    // Determine the interrupt to unpend.
    if(ui32Interrupt == INT_PENDSV)
    {
        // Unpend the PendSV interrupt.
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_UNPEND_SV;
    }
    else if(ui32Interrupt == INT_SYSTICK)
    {
        // Unpend the SysTick interrupt.
        HWREG(NVIC_INT_CTRL) |= NVIC_INT_CTRL_PENDSTCLR;
    }
    else if((ui32Interrupt >= 16) && (ui32Interrupt <= 47))
    {
        // Unpend the general interrupt.
        HWREG(NVIC_UNPEND0) = 1 << (ui32Interrupt - 16);
    }
    else if(ui32Interrupt >= 48)
    {
        // Unpend the general interrupt.
        HWREG(NVIC_UNPEND1) = 1 << (ui32Interrupt - 48);
    }
}
