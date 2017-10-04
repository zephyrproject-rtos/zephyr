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
#include <ti/devices/msp432p4xx/driverlib/debug.h>
#include <ti/devices/msp432p4xx/driverlib/interrupt.h>
#include <ti/devices/msp432p4xx/driverlib/mpu.h>

void MPU_enableModule(uint32_t mpuConfig)
{
    //
    // Check the arguments.
    //
    ASSERT(!(mpuConfig & ~(MPU_CONFIG_PRIV_DEFAULT | MPU_CONFIG_HARDFLT_NMI)));

    //
    // Set the MPU control bits according to the flags passed by the user,
    // and also set the enable bit.
    //
    MPU->CTRL = mpuConfig | MPU_CTRL_ENABLE_Msk;
}

void MPU_disableModule(void)
{
    //
    // Turn off the MPU enable bit.
    //
    MPU->CTRL &= ~MPU_CTRL_ENABLE_Msk;

}

uint32_t MPU_getRegionCount(void)
{
    //
    // Read the DREGION field of the MPU type register and mask off
    // the bits of interest to get the count of regions.
    //
    return ((MPU->TYPE & MPU_TYPE_DREGION_Msk) >> NVIC_MPU_TYPE_DREGION_S);
}

void MPU_enableRegion(uint32_t region)
{
    //
    // Check the arguments.
    //
    ASSERT(region < 8);

    //
    // Select the region to modify.
    //
    MPU->RNR = region;

    //
    // Modify the enable bit in the region attributes.
    //
    MPU->RASR |= MPU_RASR_ENABLE_Msk;
}

void MPU_disableRegion(uint32_t region)
{
    //
    // Check the arguments.
    //
    ASSERT(region < 8);

    //
    // Select the region to modify.
    //
    MPU->RNR = region;

    //
    // Modify the enable bit in the region attributes.
    //
    MPU->RASR &= ~MPU_RASR_ENABLE_Msk;
}

void MPU_setRegion(uint32_t region, uint32_t addr, uint32_t flags)
{
    //
    // Check the arguments.
    //
    ASSERT(region < 8);

    //
    // Program the base address, use the region field to select the
    // region at the same time.
    //
    MPU->RBAR = addr | region | MPU_RBAR_VALID_Msk;

    //
    // Program the region attributes.  Set the TEX field and the S, C,
    // and B bits to fixed values that are suitable for all Stellaris
    // memory.
    //
    MPU->RASR = (flags & ~(MPU_RASR_TEX_Msk | MPU_RASR_C_Msk)) | MPU_RASR_S_Msk
            | MPU_RASR_B_Msk;
}

void MPU_getRegion(uint32_t region, uint32_t *addr, uint32_t *pflags)
{
    //
    // Check the arguments.
    //
    ASSERT(region < 8);
    ASSERT(addr);
    ASSERT(pflags);

    //
    // Select the region to get.
    //
    MPU->RNR = region;

    //
    // Read and store the base address for the region.
    //
    *addr = MPU->RBAR & MPU_RBAR_ADDR_Msk;

    //
    // Read and store the region attributes.
    //
    *pflags = MPU->RASR;
}

void MPU_registerInterrupt(void (*intHandler)(void))
{
    //
    // Check the arguments.
    //
    ASSERT(intHandler);

    //
    // Register the interrupt handler.
    //
    Interrupt_registerInterrupt(FAULT_MPU, intHandler);

}

void MPU_unregisterInterrupt(void)
{
    //
    // Unregister the interrupt handler.
    //
    Interrupt_unregisterInterrupt(FAULT_MPU);
}

void MPU_enableInterrupt(void)
{

    //
    // Enable the memory management fault.
    //
    Interrupt_enableInterrupt(FAULT_MPU);

}

void MPU_disableInterrupt(void)
{
    //
    // Disable the interrupt.
    //
    Interrupt_disableInterrupt(FAULT_MPU);
}
