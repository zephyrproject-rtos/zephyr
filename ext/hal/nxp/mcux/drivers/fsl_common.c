/*
 * Copyright (c) 2015-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of the copyright holder nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fsl_common.h"
#include "fsl_debug_console.h"

#ifndef NDEBUG
#if (defined(__CC_ARM)) || (defined(__ICCARM__))
void __aeabi_assert(const char *failedExpr, const char *file, int line)
{
    PRINTF("ASSERT ERROR \" %s \": file \"%s\" Line \"%d\" \n", failedExpr, file, line);
    for (;;)
    {
        __BKPT(0);
    }
}
#elif(defined(__REDLIB__))

#if SDK_DEBUGCONSOLE
void __assertion_failed(char *_Expr)
{
    PRINTF("%s\n", _Expr);
    for (;;)
    {
        __asm("bkpt #0");
    }
}
#endif

#elif(defined(__GNUC__))
void __assert_func(const char *file, int line, const char *func, const char *failedExpr)
{
    PRINTF("ASSERT ERROR \" %s \": file \"%s\" Line \"%d\" function name \"%s\" \n", failedExpr, file, line, func);
    for (;;)
    {
        __BKPT(0);
    }
}
#endif /* (defined(__CC_ARM)) ||  (defined (__ICCARM__)) */
#endif /* NDEBUG */

#ifndef __GIC_PRIO_BITS
uint32_t InstallIRQHandler(IRQn_Type irq, uint32_t irqHandler)
{
/* Addresses for VECTOR_TABLE and VECTOR_RAM come from the linker file */
#if defined(__CC_ARM)
    extern uint32_t Image$$VECTOR_ROM$$Base[];
    extern uint32_t Image$$VECTOR_RAM$$Base[];
    extern uint32_t Image$$RW_m_data$$Base[];

#define __VECTOR_TABLE Image$$VECTOR_ROM$$Base
#define __VECTOR_RAM Image$$VECTOR_RAM$$Base
#define __RAM_VECTOR_TABLE_SIZE (((uint32_t)Image$$RW_m_data$$Base - (uint32_t)Image$$VECTOR_RAM$$Base))
#elif defined(__ICCARM__)
    extern uint32_t __RAM_VECTOR_TABLE_SIZE[];
    extern uint32_t __VECTOR_TABLE[];
    extern uint32_t __VECTOR_RAM[];
#elif defined(__GNUC__)
    extern uint32_t __VECTOR_TABLE[];
    extern uint32_t __VECTOR_RAM[];
    extern uint32_t __RAM_VECTOR_TABLE_SIZE_BYTES[];
    uint32_t __RAM_VECTOR_TABLE_SIZE = (uint32_t)(__RAM_VECTOR_TABLE_SIZE_BYTES);
#endif /* defined(__CC_ARM) */
    uint32_t n;
    uint32_t ret;
    uint32_t irqMaskValue;

    irqMaskValue = DisableGlobalIRQ();
    if (SCB->VTOR != (uint32_t)__VECTOR_RAM)
    {
        /* Copy the vector table from ROM to RAM */
        for (n = 0; n < ((uint32_t)__RAM_VECTOR_TABLE_SIZE) / sizeof(uint32_t); n++)
        {
            __VECTOR_RAM[n] = __VECTOR_TABLE[n];
        }
        /* Point the VTOR to the position of vector table */
        SCB->VTOR = (uint32_t)__VECTOR_RAM;
    }

    ret = __VECTOR_RAM[irq + 16];
    /* make sure the __VECTOR_RAM is noncachable */
    __VECTOR_RAM[irq + 16] = irqHandler;

    EnableGlobalIRQ(irqMaskValue);

    return ret;
}
#endif

#ifndef CPU_QN908X
#if (defined(FSL_FEATURE_SOC_SYSCON_COUNT) && (FSL_FEATURE_SOC_SYSCON_COUNT > 0))

void EnableDeepSleepIRQ(IRQn_Type interrupt)
{
    uint32_t index = 0;
    uint32_t intNumber = (uint32_t)interrupt;
    while (intNumber >= 32u)
    {
        index++;
        intNumber -= 32u;
    }

    SYSCON->STARTERSET[index] = 1u << intNumber;
    EnableIRQ(interrupt); /* also enable interrupt at NVIC */
}

void DisableDeepSleepIRQ(IRQn_Type interrupt)
{
    uint32_t index = 0;
    uint32_t intNumber = (uint32_t)interrupt;
    while (intNumber >= 32u)
    {
        index++;
        intNumber -= 32u;
    }

    DisableIRQ(interrupt); /* also disable interrupt at NVIC */
    SYSCON->STARTERCLR[index] = 1u << intNumber;
}
#endif /* FSL_FEATURE_SOC_SYSCON_COUNT */
#else
void EnableDeepSleepIRQ(IRQn_Type interrupt)
{
    uint32_t index = 0;
    uint32_t intNumber = (uint32_t)interrupt;
    while (intNumber >= 32u)
    {
        index++;
        intNumber -= 32u;
    }

    /*   SYSCON->STARTERSET[index] = 1u << intNumber; */
    EnableIRQ(interrupt); /* also enable interrupt at NVIC */
}

void DisableDeepSleepIRQ(IRQn_Type interrupt)
{
    uint32_t index = 0;
    uint32_t intNumber = (uint32_t)interrupt;
    while (intNumber >= 32u)
    {
        index++;
        intNumber -= 32u;
    }

    DisableIRQ(interrupt); /* also disable interrupt at NVIC */
                           /*   SYSCON->STARTERCLR[index] = 1u << intNumber; */
}
#endif /*CPU_QN908X */
