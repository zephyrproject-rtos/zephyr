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
#include <arch/arm/cortex_r/cmsis.h>

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
    unsigned int sctlr = __get_SCTLR();
    sctlr |= SCTLR_VE_Msk;
    __set_SCTLR(sctlr);
#endif
}
