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
/* Standard Includes */
#include <stdint.h>

/* DriverLib Includes */
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/cpu.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>


//*****************************************************************************
//
// This is a mapping between priority grouping encodings and the number of
// preemption priority bits.
//
//*****************************************************************************
static const uint32_t g_pulPriority[] =
{ NVIC_APINT_PRIGROUP_0_8, NVIC_APINT_PRIGROUP_1_7, NVIC_APINT_PRIGROUP_2_6,
NVIC_APINT_PRIGROUP_3_5, NVIC_APINT_PRIGROUP_4_4,
NVIC_APINT_PRIGROUP_5_3, NVIC_APINT_PRIGROUP_6_2,
NVIC_APINT_PRIGROUP_7_1 };

//*****************************************************************************
//
// This is a mapping between interrupt number and the register that contains
// the priority encoding for that interrupt.
//
//*****************************************************************************
static const uint32_t g_pulRegs[] =
{ 0, NVIC_SYS_PRI1_R, NVIC_SYS_PRI2_R, NVIC_SYS_PRI3_R, NVIC_PRI0_R,
NVIC_PRI1_R, NVIC_PRI2_R, NVIC_PRI3_R, NVIC_PRI4_R, NVIC_PRI5_R,
NVIC_PRI6_R, NVIC_PRI7_R, NVIC_PRI8_R, NVIC_PRI9_R, NVIC_PRI10_R,
NVIC_PRI11_R, NVIC_PRI12_R, NVIC_PRI13_R, NVIC_PRI14_R, NVIC_PRI15_R };

//*****************************************************************************
//
// This is a mapping between interrupt number (for the peripheral interrupts
// only) and the register that contains the interrupt enable for that
// interrupt.
//
//*****************************************************************************
static const uint32_t g_pulEnRegs[] =
{ NVIC_EN0_R, NVIC_EN1_R };

//*****************************************************************************
//
// This is a mapping between interrupt number (for the peripheral interrupts
// only) and the register that contains the interrupt disable for that
// interrupt.
//
//*****************************************************************************
static const uint32_t g_pulDisRegs[] =
{ NVIC_DIS0_R, NVIC_DIS1_R };

//*****************************************************************************
//
// This is a mapping between interrupt number (for the peripheral interrupts
// only) and the register that contains the interrupt pend for that interrupt.
//
//*****************************************************************************
static const uint32_t g_pulPendRegs[] =
{ NVIC_PEND0_R, NVIC_PEND1_R };

//*****************************************************************************
//
// This is a mapping between interrupt number (for the peripheral interrupts
// only) and the register that contains the interrupt unpend for that
// interrupt.
//
//*****************************************************************************
static const uint32_t g_pulUnpendRegs[] =
{ NVIC_UNPEND0_R, NVIC_UNPEND1_R };

//*****************************************************************************
//
//! \internal
//! The default interrupt handler.
//!
//! This is the default interrupt handler for all interrupts.  It simply loops
//! forever so that the system state is preserved for observation by a
//! debugger.  Since interrupts should be disabled before unregistering the
//! corresponding handler, this should never be called.
//!
//! \return None.
//
//*****************************************************************************
static void IntDefaultHandler(void)
{
    //
    // Go into an infinite loop.
    //
    while (1)
    {
    }
}

//*****************************************************************************
//
// The processor vector table.
//
// This contains a list of the handlers for the various interrupt sources in
// the system.  The layout of this list is defined by the hardware; assertion
// of an interrupt causes the processor to start executing directly at the
// address given in the corresponding location in this list.
//
//*****************************************************************************
#if defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment=1024
static __no_init void (*g_pfnRAMVectors[NUM_INTERRUPTS+1])(void) @ "VTABLE";
#elif defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN(g_pfnRAMVectors, 1024)
#pragma DATA_SECTION(g_pfnRAMVectors, ".vtable")
void (*g_pfnRAMVectors[NUM_INTERRUPTS + 1])(void);
#else
static __attribute__((section("vtable")))
void (*g_pfnRAMVectors[NUM_INTERRUPTS+1])(void) __attribute__((aligned(1024)));
#endif

bool Interrupt_enableMaster(void)
{
    //
    // Enable processor interrupts.
    //
    return (CPU_cpsie());
}

bool Interrupt_disableMaster(void)
{
    //
    // Disable processor interrupts.
    //
    return (CPU_cpsid());
}

void Interrupt_registerInterrupt(uint32_t interruptNumber,
        void (*intHandler)(void))
{
    uint32_t ulIdx, ulValue;

    //
    // Check the arguments.
    //
    ASSERT(interruptNumber < (NUM_INTERRUPTS+1));

    //
    // Make sure that the RAM vector table is correctly aligned.
    //
    ASSERT(((uint32_t) g_pfnRAMVectors & 0x000000ff) == 0);

    //
    // See if the RAM vector table has been initialized.
    //
    if (SCB->VTOR != (uint32_t) g_pfnRAMVectors)
    {
        //
        // Copy the vector table from the beginning of FLASH to the RAM vector
        // table.
        //
        ulValue = SCB->VTOR;
        for (ulIdx = 0; ulIdx < (NUM_INTERRUPTS + 1); ulIdx++)
        {
            g_pfnRAMVectors[ulIdx] = (void (*)(void)) HWREG32(
                    (ulIdx * 4) + ulValue);
        }

        //
        // Point the NVIC at the RAM vector table.
        //
        SCB->VTOR = (uint32_t) g_pfnRAMVectors;
    }

    //
    // Save the interrupt handler.
    //
    g_pfnRAMVectors[interruptNumber] = intHandler;
}

void Interrupt_unregisterInterrupt(uint32_t interruptNumber)
{
    //
    // Check the arguments.
    //
    ASSERT(interruptNumber < (NUM_INTERRUPTS+1));

    //
    // Reset the interrupt handler.
    //
    g_pfnRAMVectors[interruptNumber] = IntDefaultHandler;
}

void Interrupt_setPriorityGrouping(uint32_t bits)
{
    //
    // Check the arguments.
    //
    ASSERT(bits < NUM_PRIORITY);

    //
    // Set the priority grouping.
    //
    SCB->AIRCR = SCB_AIRCR_VECTKEY_Msk | g_pulPriority[bits];
}

uint32_t Interrupt_getPriorityGrouping(void)
{
    uint32_t ulLoop, ulValue;

    //
    // Read the priority grouping.
    //
    ulValue = SCB->AIRCR & NVIC_APINT_PRIGROUP_M;

    //
    // Loop through the priority grouping values.
    //
    for (ulLoop = 0; ulLoop < NUM_PRIORITY; ulLoop++)
    {
        //
        // Stop looping if this value matches.
        //
        if (ulValue == g_pulPriority[ulLoop])
        {
            break;
        }
    }

    //
    // Return the number of priority bits.
    //
    return (ulLoop);
}

void Interrupt_setPriority(uint32_t interruptNumber, uint8_t priority)
{
    uint32_t ulTemp;

    //
    // Check the arguments.
    //
    ASSERT((interruptNumber >= 4) && (interruptNumber < (NUM_INTERRUPTS+1)));

    //
    // Set the interrupt priority.
    //
    ulTemp = HWREG32(g_pulRegs[interruptNumber >> 2]);
    ulTemp &= ~(0xFF << (8 * (interruptNumber & 3)));
    ulTemp |= priority << (8 * (interruptNumber & 3));
    HWREG32 (g_pulRegs[interruptNumber >> 2]) = ulTemp;
}

uint8_t Interrupt_getPriority(uint32_t interruptNumber)
{
    //
    // Check the arguments.
    //
    ASSERT((interruptNumber >= 4) && (interruptNumber < (NUM_INTERRUPTS+1)));

    //
    // Return the interrupt priority.
    //
    return ((HWREG32(g_pulRegs[interruptNumber >> 2])
            >> (8 * (interruptNumber & 3))) & 0xFF);
}

void Interrupt_enableInterrupt(uint32_t interruptNumber)
{
    //
    // Check the arguments.
    //
    ASSERT(interruptNumber < (NUM_INTERRUPTS+1));

    //
    // Determine the interrupt to enable.
    //
    if (interruptNumber == FAULT_MPU)
    {
        //
        // Enable the MemManage interrupt.
        //
        SCB->SHCSR |= SCB_SHCSR_MEMFAULTENA_Msk;
    } else if (interruptNumber == FAULT_BUS)
    {
        //
        // Enable the bus fault interrupt.
        //
        SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
    } else if (interruptNumber == FAULT_USAGE)
    {
        //
        // Enable the usage fault interrupt.
        //
        SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk;
    } else if (interruptNumber == FAULT_SYSTICK)
    {
        //
        // Enable the System Tick interrupt.
        //
        SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    } else if (interruptNumber >= 16)
    {
        //
        // Enable the general interrupt.
        //
        HWREG32 (g_pulEnRegs[(interruptNumber - 16) / 32]) = 1
                << ((interruptNumber - 16) & 31);
    }
}

void Interrupt_disableInterrupt(uint32_t interruptNumber)
{
    //
    // Check the arguments.
    //
    ASSERT(interruptNumber < (NUM_INTERRUPTS+1));

    //
    // Determine the interrupt to disable.
    //
    if (interruptNumber == FAULT_MPU)
    {
        //
        // Disable the MemManage interrupt.
        //
        SCB->SHCSR &= ~(SCB_SHCSR_MEMFAULTENA_Msk);
    } else if (interruptNumber == FAULT_BUS)
    {
        //
        // Disable the bus fault interrupt.
        //
        SCB->SHCSR &= ~(SCB_SHCSR_BUSFAULTENA_Msk);
    } else if (interruptNumber == FAULT_USAGE)
    {
        //
        // Disable the usage fault interrupt.
        //
        SCB->SHCSR &= ~(SCB_SHCSR_USGFAULTENA_Msk);
    } else if (interruptNumber == FAULT_SYSTICK)
    {
        //
        // Disable the System Tick interrupt.
        //
        SysTick->CTRL &= ~(SysTick_CTRL_ENABLE_Msk);
    } else if (interruptNumber >= 16)
    {
        //
        // Disable the general interrupt.
        //
        HWREG32 (g_pulDisRegs[(interruptNumber - 16) / 32]) = 1
                << ((interruptNumber - 16) & 31);
    }
}

bool Interrupt_isEnabled(uint32_t interruptNumber)
{
    uint32_t ulRet;

    //
    // Check the arguments.
    //
    ASSERT(interruptNumber < (NUM_INTERRUPTS+1));

    //
    // Initialize the return value.
    //
    ulRet = 0;

    //
    // Determine the interrupt to disable.
    //
    if (interruptNumber == FAULT_MPU)
    {
        //
        // Check the MemManage interrupt.
        //
        ulRet = SCB->SHCSR & SCB_SHCSR_MEMFAULTENA_Msk;
    } else if (interruptNumber == FAULT_BUS)
    {
        //
        // Check the bus fault interrupt.
        //
        ulRet = SCB->SHCSR & SCB_SHCSR_BUSFAULTENA_Msk;
    } else if (interruptNumber == FAULT_USAGE)
    {
        //
        // Check the usage fault interrupt.
        //
        ulRet = SCB->SHCSR & SCB_SHCSR_USGFAULTENA_Msk;
    } else if (interruptNumber == FAULT_SYSTICK)
    {
        //
        // Check the System Tick interrupt.
        //
        ulRet = SysTick->CTRL & SysTick_CTRL_ENABLE_Msk;
    } else if (interruptNumber >= 16)
    {
        //
        // Check the general interrupt.
        //
        ulRet = HWREG32(g_pulEnRegs[(interruptNumber - 16) / 32])
                & (1 << ((interruptNumber - 16) & 31));
    }
    return (ulRet);
}

void Interrupt_pendInterrupt(uint32_t interruptNumber)
{
    //
    // Check the arguments.
    //
    ASSERT(interruptNumber < (NUM_INTERRUPTS+1));

    //
    // Determine the interrupt to pend.
    //
    if (interruptNumber == FAULT_NMI)
    {
        //
        // Pend the NMI interrupt.
        //
        SCB->ICSR |= SCB_ICSR_NMIPENDSET_Msk;
    } else if (interruptNumber == FAULT_PENDSV)
    {
        //
        // Pend the PendSV interrupt.
        //
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    } else if (interruptNumber == FAULT_SYSTICK)
    {
        //
        // Pend the SysTick interrupt.
        //
        SCB->ICSR |= SCB_ICSR_PENDSTSET_Msk;
    } else if (interruptNumber >= 16)
    {
        //
        // Pend the general interrupt.
        //
        HWREG32 (g_pulPendRegs[(interruptNumber - 16) / 32]) = 1
                << ((interruptNumber - 16) & 31);
    }
}

void Interrupt_unpendInterrupt(uint32_t interruptNumber)
{
    //
    // Check the arguments.
    //
    ASSERT(interruptNumber < (NUM_INTERRUPTS+1));

    //
    // Determine the interrupt to unpend.
    //
    if (interruptNumber == FAULT_PENDSV)
    {
        //
        // Unpend the PendSV interrupt.
        //
        SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
    } else if (interruptNumber == FAULT_SYSTICK)
    {
        //
        // Unpend the SysTick interrupt.
        //
        SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;
    } else if (interruptNumber >= 16)
    {
        //
        // Unpend the general interrupt.
        //
        HWREG32 (g_pulUnpendRegs[(interruptNumber - 16) / 32]) = 1
                << ((interruptNumber - 16) & 31);
    }
}

void Interrupt_setPriorityMask(uint8_t priorityMask)
{
    CPU_basepriSet(priorityMask);
}

uint8_t Interrupt_getPriorityMask(void)
{
    return (CPU_basepriGet());
}

void Interrupt_setVectorTableAddress(uint32_t addr)
{
    SCB->VTOR = addr;
}

uint32_t Interrupt_getVectorTableAddress(void)
{
    return SCB->VTOR;
}

void Interrupt_enableSleepOnIsrExit(void)
{
    SCB->SCR |= SCB_SCR_SLEEPONEXIT_Msk;
}

void Interrupt_disableSleepOnIsrExit(void)
{
    SCB->SCR &= ~SCB_SCR_SLEEPONEXIT_Msk;
}
