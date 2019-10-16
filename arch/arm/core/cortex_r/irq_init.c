/*
 * Copyright (c) 2019 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-R interrupt initialization
 */

#include <arch/cpu.h>

/**
 * @brief Initialize interrupts
 *
 * @return N/A
 */

void z_arm_int_lib_init(void)
{
    /* Invoke SoC-specific interrupt initialisation */
    z_soc_irq_init();

    /* Configure hardware vectored interrupt mode.*/
#if defined(CONFIG_VIC_IRQ_VECTOR)
    __asm__ volatile(
        "mrc p15, #0, r0, c1, c0, #0;"
        "orr r0,  r0, #0x01000000;"     /* [24] VE bit */
        "mcr p15, #0, r0, c1, c0, #0;"
        : : : "memory");
#endif
}
