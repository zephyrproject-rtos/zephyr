/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * All rights reserved.
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
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
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
#include <stdbool.h>
#include "MCIMX7D_M4.h"

/* ----------------------------------------------------------------------------
   -- Helper macro
   ---------------------------------------------------------------------------- */
#define EXTRACT_BITFIELD(reg, shift, width) ((*(reg) & (((1 << (width)) - 1) << (shift))) >> (shift))

/* ----------------------------------------------------------------------------
   -- Vector Table offset
   ---------------------------------------------------------------------------- */
#define VECT_TAB_OFFSET  0x0

/* ----------------------------------------------------------------------------
   -- Core clock
   ---------------------------------------------------------------------------- */
uint32_t SystemCoreClock = 240000000ul;

/* ----------------------------------------------------------------------------
   -- SystemInit()
   ---------------------------------------------------------------------------- */
void SystemInit(void)
{
    /* The Vector table base address is given by linker script. */
#if defined(__CC_ARM)
    extern uint32_t Image$$VECTOR_ROM$$Base[];
#else
    extern uint32_t __VECTOR_TABLE[];
#endif

    /* Enable FPU */
#if ((1 == __FPU_PRESENT) && (1 == __FPU_USED))
    SCB->CPACR |= ((3UL << 10*2)|(3UL << 11*2));  /* set CP10 and CP11 Full Access */
#endif

    /* Set M4 core clock to SYS_PLL_Div2 @ 240MHz. */
    CCM_TARGET_ROOT1 = 0x11000000;

    /* Initialize MPU */
    /* Make sure outstanding transfers are done. */
    __DMB();
    /* Disable the MPU. */
    MPU->CTRL = 0;

    /* Select Region 0 to configure. */
    MPU->RNR  = 0;
    /* Set base address of Region 0 to the base of Default CODE + SRAM memory region(1GB). */
    MPU->RBAR = 0x00000000;
    /* Region 0 setting:
     * 1) Enable Instruction Access;
     * 2) Full Data Access Permission;
     * 3) Outer and inner Non-Cacheable;
     * 4) Region Not Shared;
     * 5) All Sub-Region Enable;
     * 6) MPU Protection Region size = 1GB;
     * 7) Enable Region 0.
     */
    MPU->RASR = 0x0308003B;

    /* Select Region 1 to configure. */
    MPU->RNR  = 1;
    /* Set base address of Region 1 to the base of Default RAM memory region(2GB). */
    MPU->RBAR = 0x60000000;
    /* Region 1 setting:
     * 1) Enable Instruction Access;
     * 2) Full Data Access Permission;
     * 3) Outer and inner Non-Cacheable;
     * 4) Region Not Shared;
     * 5) All Sub-Region Enable;
     * 6) MPU Protection Region size = 2GB;
     * 7) Enable Region 1.
     */
    MPU->RASR = 0x0308003D;

    /* Select Region 2 to configure. */
    MPU->RNR  = 2;
    /* Set base address of Region 2 to the base of Cacheable OCRAM region(128KB). */
    MPU->RBAR = 0x20200000;
    /* Region 2 setting:
     * 1) Enable Instruction Access;
     * 2) Full Data Access Permission;
     * 3) Write Back, Write Allocate;
     * 4) Region Not Shared;
     * 5) All Sub-Region Enable;
     * 6) MPU Protection Region size = 128KB;
     * 7) Enable Region 2.
     */
    MPU->RASR = 0x030B0021;

    /* Select Region 3 to configure. */
    MPU->RNR  = 3;
    /* Set base address of Region 3 to the base of Cacheable QSPI Flash region(2MB). */
    MPU->RBAR = 0x60000000;
    /* Region 3 setting:
     * 1) Enable Instruction Access;
     * 2) Full Data Access Permission;
     * 3) Write Back, Write Allocate;
     * 4) Region Not Shared;
     * 5) All Sub-Region Enable;
     * 6) MPU Protection Region size = 2MB;
     * 7) Enable Region 3.
     */
    MPU->RASR = 0x030B0029;

    /* Select Region 4 to configure. */
    MPU->RNR  = 4;
    /* Set base address of Region 4 to the base of Cacheable DDR RAM region(2MB). */
    MPU->RBAR = 0x80000000;
    /* Region 4 setting:
     * 1) Enable Instruction Access;
     * 2) Full Data Access Permission;
     * 3) Write Back, Write Allocate;
     * 4) Region Not Shared;
     * 5) All Sub-Region Enable;
     * 6) MPU Protection Region size = 2MB;
     * 7) Enable Region 4.
     */
    MPU->RASR = 0x030B0029;

    /* Disable unused regions. */
    MPU->RNR  = 5;
    MPU->RBAR = 0;
    MPU->RASR = 0;
    MPU->RNR  = 6;
    MPU->RBAR = 0;
    MPU->RASR = 0;
    MPU->RNR  = 7;
    MPU->RBAR = 0;
    MPU->RASR = 0;

    /* Enable Privileged default memory map and the MPU. */
    MPU->CTRL = MPU_CTRL_ENABLE_Msk |
                MPU_CTRL_PRIVDEFENA_Msk;
    /* Memory barriers to ensure subsequence data & instruction
     * transfers using updated MPU settings.
     */
    __DSB();
    __ISB();

    /* Initialize Cache */
    /* Enable System Bus Cache */
    /* set command to invalidate all ways, enable write buffer
       and write GO bit to initiate command */
    LMEM_PSCCR = LMEM_PSCCR_INVW1_MASK | LMEM_PSCCR_INVW0_MASK;
    LMEM_PSCCR |= LMEM_PSCCR_GO_MASK;
    /* wait until the command completes */
    while (LMEM_PSCCR & LMEM_PSCCR_GO_MASK);
    /* Enable cache, enable write buffer */
    LMEM_PSCCR = (LMEM_PSCCR_ENWRBUF_MASK | LMEM_PSCCR_ENCACHE_MASK);
    __DSB();
    __ISB();

    /* Relocate vector table */
#if defined(__CC_ARM)
    SCB->VTOR = (uint32_t)Image$$VECTOR_ROM$$Base + VECT_TAB_OFFSET;
#else
    SCB->VTOR = (uint32_t)__VECTOR_TABLE + VECT_TAB_OFFSET;
#endif
}

/* ----------------------------------------------------------------------------
   -- SystemCoreClockUpdate()
   ---------------------------------------------------------------------------- */
void SystemCoreClockUpdate(void)
{
    uint8_t coreClockRoot    = EXTRACT_BITFIELD(&CCM_TARGET_ROOT1, 24, 3);
    uint8_t coreClockPreDiv  = EXTRACT_BITFIELD(&CCM_TARGET_ROOT1, 16, 3) + 1;
    uint8_t coreClockPostDiv = EXTRACT_BITFIELD(&CCM_TARGET_ROOT1, 0, 6) + 1;
    float temp;

    switch (coreClockRoot)
    {
        /* OSC_24M: */
        case 0:
            SystemCoreClock = 24000000ul;
            break;

        /* SYS_PLL_DIV2: */
        case 1:
            /* Check SYS PLL bypass bit. */
            if (!EXTRACT_BITFIELD(&CCM_ANALOG_PLL_480_REG(CCM_ANALOG), 16, 1))
            {
                SystemCoreClock = (1 == EXTRACT_BITFIELD(&CCM_ANALOG_PLL_480_REG(CCM_ANALOG), \
                                   0, 1)) ? 264000000ul : 240000000ul;
            }
            else
            {
                SystemCoreClock = 24000000ul;
            }
            break;

        /* ENET_PLL_DIV4: */
        case 2:
            /* Check ENET PLL bypass bit. */
            if (!EXTRACT_BITFIELD(&CCM_ANALOG_PLL_ENET_REG(CCM_ANALOG), 16, 1))
                SystemCoreClock = 250000000ul;
            else
                SystemCoreClock = 24000000ul;
            break;

        /* SYS_PLL_PFD2: */
        case 3:
            /* Check SYS SYS PLL bypass bit. */
            if (!EXTRACT_BITFIELD(&CCM_ANALOG_PLL_480_REG(CCM_ANALOG), 16, 1))
            {
                SystemCoreClock = (1 == EXTRACT_BITFIELD(&CCM_ANALOG_PLL_480_REG(CCM_ANALOG), \
                                   0, 1)) ? 528000000ul : 480000000ul;
                SystemCoreClock /= EXTRACT_BITFIELD(&CCM_ANALOG_PFD_480A_REG(CCM_ANALOG), 16, 6);
                SystemCoreClock *= 18;
            }
            else
            {
                SystemCoreClock = 24000000ul;
            }
            break;

        /* DDR_PLL_DIV2: */
        case 4:
            /* Check DDR PLL bypass bit. */
            if (!EXTRACT_BITFIELD(&CCM_ANALOG_PLL_DDR_REG(CCM_ANALOG), 16, 1))
            {
                if (1 == EXTRACT_BITFIELD(&CCM_ANALOG_PLL_DDR_SS_REG(CCM_ANALOG), 15, 1))
                {
                    temp = (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_DDR_SS_REG(CCM_ANALOG), 0, 15) /
                           (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_DDR_DENOM_REG(CCM_ANALOG), 0, 30) *
                           (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_DDR_NUM_REG(CCM_ANALOG), 0, 30);
                    temp += EXTRACT_BITFIELD(&CCM_ANALOG_PLL_DDR_REG(CCM_ANALOG), 0, 7);
                    SystemCoreClock = (uint32_t)(24000000ul * temp);
                }
                else
                    SystemCoreClock = 24000000ul * EXTRACT_BITFIELD(&CCM_ANALOG_PLL_DDR_REG(CCM_ANALOG), 0, 7);

                switch (EXTRACT_BITFIELD(&CCM_ANALOG_PLL_DDR_REG(CCM_ANALOG), 21, 2))
                {
                    case 0:
                        SystemCoreClock >>= 2;
                        break;
                    case 1:
                        SystemCoreClock >>= 1;
                        break;
                    case 2:
                    case 3:
                        break;
                }

                SystemCoreClock >>= 1;
            }
            else
            {
                SystemCoreClock = 24000000ul;
            }
            break;

        /* AUDIO_PLL: */
        case 5:
            /* Check AUDIO PLL bypass bit. */
            if (!EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_REG(CCM_ANALOG), 16, 1))
            {
                if (1 == EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_SS_REG(CCM_ANALOG), 15, 1))
                {
                    temp = (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_SS_REG(CCM_ANALOG), 0, 15) /
                           (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_DENOM_REG(CCM_ANALOG), 0, 30) *
                           (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_NUM_REG(CCM_ANALOG), 0, 30);
                    temp += EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_REG(CCM_ANALOG), 0, 7);
                    SystemCoreClock = (uint32_t)(24000000ul * temp);
                }
                else
                    SystemCoreClock = 24000000ul * EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_REG(CCM_ANALOG), 0, 7);

                switch (EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_REG(CCM_ANALOG), 19, 2))
                {
                    case 0x0:
                        SystemCoreClock >>= 2;
                        break;
                    case 0x1:
                        SystemCoreClock >>= 1;
                        break;
                    case 0x2:
                    case 0x3:
                        break;
                }

                switch (EXTRACT_BITFIELD(&CCM_ANALOG_PLL_AUDIO_REG(CCM_ANALOG), 22, 2))
                {
                    case 0x0:
                    case 0x2:
                        break;
                    case 0x1:
                        SystemCoreClock >>= 1;
                        break;
                    case 0x3:
                        SystemCoreClock >>= 2;
                        break;
                }
            }
            else
            {
                SystemCoreClock = 24000000ul;
            }
            break;

        /* VIDEO_PLL: */
        case 6:
            /* Check VIDEO PLL bypass bit. */
            if (!EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_REG(CCM_ANALOG), 16, 1))
            {
                if (1 == EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_SS_REG(CCM_ANALOG), 15, 1))
                {
                    temp = (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_SS_REG(CCM_ANALOG), 0, 15) /
                           (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_DENOM_REG(CCM_ANALOG), 0, 30) *
                           (float)EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_NUM_REG(CCM_ANALOG), 0, 30);
                    temp += EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_REG(CCM_ANALOG), 0, 7);
                    SystemCoreClock = (uint32_t)(24000000ul * temp);
                }
                else
                    SystemCoreClock = 24000000ul * EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_REG(CCM_ANALOG), 0, 7);

                switch (EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_REG(CCM_ANALOG), 19, 2))
                {
                    case 0x0:
                        SystemCoreClock >>= 2;
                        break;
                    case 0x1:
                        SystemCoreClock >>= 1;
                        break;
                    case 0x2:
                    case 0x3:
                        break;
                }

                switch (EXTRACT_BITFIELD(&CCM_ANALOG_PLL_VIDEO_REG(CCM_ANALOG), 22, 2))
                {
                    case 0x0:
                    case 0x2:
                        break;
                    case 0x1:
                        SystemCoreClock >>= 1;
                        break;
                    case 0x3:
                        SystemCoreClock >>= 2;
                        break;
                }
            }
            else
            {
                SystemCoreClock = 24000000ul;
            }
            break;

        /* USB_PLL: */
        case 7:
            SystemCoreClock = 480000000ul;
            break;

        default:
            /* Set SystemCoreClock to default clock freq. */
            SystemCoreClock = 240000000ul;
            break;
    }

    SystemCoreClock = (SystemCoreClock / coreClockPreDiv) / coreClockPostDiv;
}

/*******************************************************************************
 * EOF
 ******************************************************************************/
