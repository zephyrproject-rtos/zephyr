/*
 * Copyright (c) 2025, Microchip Technology Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/pm/pm.h>
#include <zephyr/sw_isr_table.h>
#include <kernel_arch_func.h>

/* dsPIC33A interrtup exit routine. Will check if a context
 * switch is required. If so, z_dspic_do_swap() will be called
 * to affect the context switch
 */
void __attribute__((naked)) z_dspic_exc_exit(void)
{
}

void __attribute__((interrupt, naked)) _COMMONInterrupt(void)
{
}
