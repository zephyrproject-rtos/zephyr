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
#include <ti/devices/msp432p4xx/driverlib/fpu.h>

void FPU_enableModule(void)
{
    //
    // Enable the coprocessors used by the floating-point unit.
    //
    SCB->CPACR = ((SCB->CPACR & ~(SCB_CPACR_CP11_MASK | SCB_CPACR_CP10_MASK))
            | SCB_CPACR_CP11_MASK | SCB_CPACR_CP10_MASK);
}

void FPU_disableModule(void)
{
    //
    // Disable the coprocessors used by the floating-point unit.
    //
    SCB->CPACR = ((SCB->CPACR & ~(SCB_CPACR_CP10_MASK | SCB_CPACR_CP11_MASK)));
}

void FPU_enableStacking(void)
{
    //
    // Enable automatic state preservation for the floating-point unit, and
    // disable lazy state preservation (meaning that the floating-point state
    // is always stacked when floating-point instructions are used).
    //
    FPU->FPCCR = (FPU->FPCCR & ~FPU_FPCCR_LSPEN_Msk) | FPU_FPCCR_ASPEN_Msk;
}

void FPU_enableLazyStacking(void)
{
    //
    // Enable automatic and lazy state preservation for the floating-point
    // unit.
    //
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;
}

void FPU_disableStacking(void)
{
    //
    // Disable automatic and lazy state preservation for the floating-point
    // unit.
    //
    FPU->FPCCR &= ~(FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk);
}

void FPU_setHalfPrecisionMode(uint32_t mode)
{
    //
    // Set the half-precision floating-point format.
    //
    FPU->FPDSCR = (FPU->FPDSCR & ~(FPU_FPDSCR_AHP_Msk)) | mode;
}

void FPU_setNaNMode(uint32_t mode)
{
    //
    // Set the NaN mode.
    //
    FPU->FPDSCR = (FPU->FPDSCR & ~(FPU_FPDSCR_DN_Msk)) | mode;
}

void FPU_setFlushToZeroMode(uint32_t mode)
{
    //
    // Set the flush-to-zero mode.
    //
    FPU->FPDSCR = (FPU->FPDSCR & ~(FPU_FPDSCR_FZ_Msk)) | mode;
}

void FPU_setRoundingMode(uint32_t mode)
{
    //
    // Set the rounding mode.
    //
    FPU->FPDSCR = (FPU->FPDSCR & ~(FPU_FPDSCR_RMode_Msk)) | mode;
}

