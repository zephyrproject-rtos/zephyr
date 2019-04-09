/******************************************************************************
*  Filename:       cpu.c
*  Revised:        2018-05-08 10:04:01 +0200 (Tue, 08 May 2018)
*  Revision:       51972
*
*  Description:    Instruction wrappers for special CPU instructions needed by
*                  the drivers.
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

#include "cpu.h"

//*****************************************************************************
//
// Handle support for DriverLib in ROM:
// This section will undo prototype renaming made in the header file
//
//*****************************************************************************
#if !defined(DOXYGEN)
    #undef  CPUcpsid
    #define CPUcpsid                        NOROM_CPUcpsid
    #undef  CPUprimask
    #define CPUprimask                      NOROM_CPUprimask
    #undef  CPUcpsie
    #define CPUcpsie                        NOROM_CPUcpsie
    #undef  CPUbasepriGet
    #define CPUbasepriGet                   NOROM_CPUbasepriGet
    #undef  CPUdelay
    #define CPUdelay                        NOROM_CPUdelay
#endif

//*****************************************************************************
//
// Disable all external interrupts
//
//*****************************************************************************
#if defined(DOXYGEN)
uint32_t
CPUcpsid(void)
{
    // This function is written in assembly. See cpu.c for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
uint32_t
CPUcpsid(void)
{
    // Read PRIMASK and disable interrupts.
    __asm("    mrs     r0, PRIMASK\n"
          "    cpsid   i\n");

    // "Warning[Pe940]: missing return statement at end of non-void function"
    // is suppressed here to avoid putting a "bx lr" in the inline assembly
    // above and a superfluous return statement here.
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm uint32_t
CPUcpsid(void)
{
    // Read PRIMASK and disable interrupts.
    mrs     r0, PRIMASK;
    cpsid   i;
    bx      lr
}
#elif defined(__TI_COMPILER_VERSION__)
uint32_t
CPUcpsid(void)
{
    // Read PRIMASK and disable interrupts.
    __asm("    mrs     r0, PRIMASK\n"
          "    cpsid   i\n"
          "    bx      lr\n");

    // The following keeps the compiler happy, because it wants to see a
    // return value from this function.  It will generate code to return
    // a zero.  However, the real return is the "bx lr" above, so the
    // return(0) is never executed and the function returns with the value
    // you expect in R0.
    return(0);
}
#else
uint32_t __attribute__((naked))
CPUcpsid(void)
{
    uint32_t ui32Ret;

    // Read PRIMASK and disable interrupts
    __asm volatile ("    mrs     %0, PRIMASK\n"
                    "    cpsid   i\n"
                    "    bx      lr\n"
                    : "=r"(ui32Ret)
                   );

    // The return is handled in the inline assembly, but the compiler will
    // still complain if there is not an explicit return here (despite the fact
    // that this does not result in any code being produced because of the
    // naked attribute).
    return(ui32Ret);
}
#endif

//*****************************************************************************
//
// Get the current interrupt state
//
//*****************************************************************************
#if defined(DOXYGEN)
uint32_t
CPUprimask(void)
{
    // This function is written in assembly. See cpu.c for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
uint32_t
CPUprimask(void)
{
    // Read PRIMASK.
    __asm("    mrs     r0, PRIMASK\n");

    // "Warning[Pe940]: missing return statement at end of non-void function"
    // is suppressed here to avoid putting a "bx lr" in the inline assembly
    // above and a superfluous return statement here.
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm uint32_t
CPUprimask(void)
{
    // Read PRIMASK.
    mrs     r0, PRIMASK;
    bx      lr
}
#elif defined(__TI_COMPILER_VERSION__)
uint32_t
CPUprimask(void)
{
    // Read PRIMASK.
    __asm("    mrs     r0, PRIMASK\n"
          "    bx      lr\n");

    // The following keeps the compiler happy, because it wants to see a
    // return value from this function.  It will generate code to return
    // a zero.  However, the real return is the "bx lr" above, so the
    // return(0) is never executed and the function returns with the value
    // you expect in R0.
    return(0);
}
#else
uint32_t __attribute__((naked))
CPUprimask(void)
{
    uint32_t ui32Ret;

    // Read PRIMASK
    __asm volatile ("    mrs     %0, PRIMASK\n"
                    "    bx      lr\n"
                    : "=r"(ui32Ret)
                   );

    // The return is handled in the inline assembly, but the compiler will
    // still complain if there is not an explicit return here (despite the fact
    // that this does not result in any code being produced because of the
    // naked attribute).
    return(ui32Ret);
}
#endif

//*****************************************************************************
//
// Enable all external interrupts
//
//*****************************************************************************
#if defined(DOXYGEN)
uint32_t
CPUcpsie(void)
{
    // This function is written in assembly. See cpu.c for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
uint32_t
CPUcpsie(void)
{
    // Read PRIMASK and enable interrupts.
    __asm("    mrs     r0, PRIMASK\n"
          "    cpsie   i\n");

    // "Warning[Pe940]: missing return statement at end of non-void function"
    // is suppressed here to avoid putting a "bx lr" in the inline assembly
    // above and a superfluous return statement here.
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm uint32_t
CPUcpsie(void)
{
    // Read PRIMASK and enable interrupts.
    mrs     r0, PRIMASK;
    cpsie   i;
    bx      lr
}
#elif defined(__TI_COMPILER_VERSION__)
uint32_t
CPUcpsie(void)
{
    // Read PRIMASK and enable interrupts.
    __asm("    mrs     r0, PRIMASK\n"
          "    cpsie   i\n"
          "    bx      lr\n");

    // The following keeps the compiler happy, because it wants to see a
    // return value from this function.  It will generate code to return
    // a zero.  However, the real return is the "bx lr" above, so the
    // return(0) is never executed and the function returns with the value
    // you expect in R0.
    return(0);
}
#else
uint32_t __attribute__((naked))
CPUcpsie(void)
{
    uint32_t ui32Ret;

    // Read PRIMASK and enable interrupts.
    __asm volatile ("    mrs     %0, PRIMASK\n"
                    "    cpsie   i\n"
                    "    bx      lr\n"
                    : "=r"(ui32Ret)
                   );

    // The return is handled in the inline assembly, but the compiler will
    // still complain if there is not an explicit return here (despite the fact
    // that this does not result in any code being produced because of the
    // naked attribute).
    return(ui32Ret);
}
#endif

//*****************************************************************************
//
// Get the interrupt priority disable level
//
//*****************************************************************************
#if defined(DOXYGEN)
uint32_t
CPUbasepriGet(void)
{
    // This function is written in assembly. See cpu.c for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
uint32_t
CPUbasepriGet(void)
{
    // Read BASEPRI.
    __asm("    mrs     r0, BASEPRI\n");

    // "Warning[Pe940]: missing return statement at end of non-void function"
    // is suppressed here to avoid putting a "bx lr" in the inline assembly
    // above and a superfluous return statement here.
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm uint32_t
CPUbasepriGet(void)
{
    // Read BASEPRI.
    mrs     r0, BASEPRI;
    bx      lr
}
#elif defined(__TI_COMPILER_VERSION__)
uint32_t
CPUbasepriGet(void)
{
    // Read BASEPRI.
    __asm("    mrs     r0, BASEPRI\n"
          "    bx      lr\n");

    // The following keeps the compiler happy, because it wants to see a
    // return value from this function.  It will generate code to return
    // a zero.  However, the real return is the "bx lr" above, so the
    // return(0) is never executed and the function returns with the value
    // you expect in R0.
    return(0);
}
#else
uint32_t __attribute__((naked))
CPUbasepriGet(void)
{
    uint32_t ui32Ret;

    // Read BASEPRI.
    __asm volatile ("    mrs     %0, BASEPRI\n"
                    "    bx      lr\n"
                    : "=r"(ui32Ret)
                   );

    // The return is handled in the inline assembly, but the compiler will
    // still complain if there is not an explicit return here (despite the fact
    // that this does not result in any code being produced because of the
    // naked attribute).
    return(ui32Ret);
}
#endif
//*****************************************************************************
//
// Provide a small delay
//
//*****************************************************************************
#if defined(DOXYGEN)
void
CPUdelay(uint32_t ui32Count)
{
    // This function is written in assembly. See cpu.c for compiler specific implementation.
}
#elif defined(__IAR_SYSTEMS_ICC__)
void
CPUdelay(uint32_t ui32Count)
{
    // Loop the specified number of times
    __asm("CPUdelay:\n"
          "    subs    r0, #1\n"
          "    bne.n   CPUdelay\n"
          "    bx      lr");
#pragma diag_suppress=Pe940
}
#pragma diag_default=Pe940
#elif defined(__CC_ARM) || defined(__ARMCC_VERSION)
__asm void
CPUdelay(uint32_t ui32Count)
{
    // Delay the specified number of times (3 cycles pr. loop)
CPUdel
    subs    r0, #1;
    bne     CPUdel;
    bx      lr;
}
#elif defined(__TI_COMPILER_VERSION__)
    // For CCS implement this function in pure assembly. This prevents the TI
    // compiler from doing funny things with the optimizer.

    // Loop the specified number of times
__asm("    .sect \".text:NOROM_CPUdelay\"\n"
      "    .clink\n"
      "    .thumbfunc NOROM_CPUdelay\n"
      "    .thumb\n"
      "    .global NOROM_CPUdelay\n"
      "NOROM_CPUdelay:\n"
      "    subs r0, #1\n"
      "    bne.n NOROM_CPUdelay\n"
      "    bx lr\n");
#else
// GCC
void __attribute__((naked))
CPUdelay(uint32_t ui32Count)
{
    // Loop the specified number of times
    __asm volatile ("%=:  subs  %0, #1\n"
                    "     bne   %=b\n"
                    "     bx    lr\n"
                    : /* No output */
                    : "r" (ui32Count)
                   );
}
#endif
